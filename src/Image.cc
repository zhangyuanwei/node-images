/*
 * Image.cc
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 ZhangYuanwei <zhangyuanwei1988@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "Image.h"
#include "Resize.h"
#include "Rotate.h"
#include <node_buffer.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>

//#define SET_ERROR_FILE_LINE(file, line, msg) Image::SetError( file #line msg)
//#define SET_ERROR(msg) SET_ERROR_FILE_LINE(__FILE__, __LINE__, meg)

#define DECLARE_NAPI_METHOD(name, func)                                                             \
  { name, 0, func, 0, 0, 0, napi_default, 0 }
#define DECLARE_NAPI_ACCESSOR(name, get, set)                                                       \
  { name, 0, 0, get, set, 0, napi_default, 0 }
#define DEFINE_NAPI_CONSTANT(name, value)                                                           \
  do {                                                                                              \
    napi_value _define_value;                                                                       \
    napi_status _define_status;                                                                     \
    _define_status = napi_create_uint32(env, value, &_define_value);                                \
    assert(_define_status == napi_ok);                                                              \
    _define_status = napi_set_named_property(env, exports, name, _define_value);                    \
    assert(_define_status == napi_ok);                                                              \
  } while(0);

#define GET_VALUE_WITH_NAPI_FUNC(func, arg, valueRef)                                               \
    do {                                                                                            \
        napi_valuetype valuetype;                                                                   \
        status = napi_typeof(env, arg, &valuetype);                                                 \
        assert(status == napi_ok);                                                                  \
        if (valuetype != napi_undefined) {                                                          \
            status = func(env, arg, valueRef);                                                      \
            assert(status == napi_ok);                                                              \
        }                                                                                                                                                                                    \
    } while(0);                                                                                                                                                                        

#define STRINGFY(n) #n
#define MERGE_FILE_LINE(file, line, msg) ( file ":" STRINGFY(line) " " msg)
#define FILE_LINE(msg) MERGE_FILE_LINE(__FILE__, __LINE__, msg)
#define ERROR(type, msg) napi_throw_error(env, "50000", msg)
#define THROW(err)  napi_throw(env, err)
#define SET_ERROR(msg) (Image::setError(FILE_LINE(msg)))
#define GET_ERROR() (Image::getError(env, info))
#define THROW_ERROR(msg) napi_throw_error(env, "50000", FILE_LINE(msg))
#define THROW_GET_ERROR() THROW(GET_ERROR())

#define THROW_TYPE_ERROR(msg) napi_throw_type_error(env, "50010", msg)
#define THROW_INVALID_ARGUMENTS_ERROR(msg) THROW_TYPE_ERROR("Invalid arguments" msg)

#define DEFAULT_WIDTH_LIMIT  10240 // default limit 10000x10000
#define DEFAULT_HEIGHT_LIMIT 10240 // default limit 10000x10000

#define AdjustAmountOfExternalAllocatedMemory(bc) static_cast<int>( \
        v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(bc));

// Persistent<Function> Image::constructor;


Image::Image(): env_(nullptr), wrapper_(nullptr) {
    size_t size;
    pixels = (PixelArray *) malloc(sizeof(PixelArray));
    pixels->width = pixels->height = 0;
    pixels->type = EMPTY;
    pixels->data = NULL;
    size = sizeof(PixelArray) + sizeof(Image);
    AdjustAmountOfExternalAllocatedMemory(size);
    usedMemory += size;
}

Image::~Image() { 
    napi_delete_reference(env_, wrapper_);

    int32_t size;
    size = sizeof(PixelArray) + sizeof(Image);
    pixels->Free();
    free(pixels);
    AdjustAmountOfExternalAllocatedMemory(-size);
    
    usedMemory -= size;
}

void Image::Destructor(napi_env env, void* nativeObejct, void*) {
    reinterpret_cast<Image*>(nativeObejct)->~Image();
}

napi_ref Image::constructor;

//size_t Image::survival;
ImageCodec *Image::codecs;

size_t Image::maxWidth = DEFAULT_WIDTH_LIMIT;
size_t Image::maxHeight = DEFAULT_HEIGHT_LIMIT;
const char *Image::error = NULL;


napi_value Image::Init(napi_env env, napi_value exports) { // {{{
    regAllCodecs();

    napi_status status;
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_ACCESSOR("width", GetWidth, SetWidth),
        DECLARE_NAPI_ACCESSOR("height", GetHeight, SetHeight),
        DECLARE_NAPI_ACCESSOR("transparent", GetTransparent, nullptr),
        DECLARE_NAPI_ACCESSOR("maxWidth", GetMaxWidth, SetMaxWidth),
        DECLARE_NAPI_ACCESSOR("maxHeight", GetMaxHeight, SetMaxHeight),
        DECLARE_NAPI_ACCESSOR("usedMemory", GetUsedMemory, nullptr),
        DECLARE_NAPI_METHOD("resize", Resize),
        DECLARE_NAPI_METHOD("rotate", Rotate),
        DECLARE_NAPI_METHOD("fillColor", FillColor),
        DECLARE_NAPI_METHOD("loadFromBuffer", LoadFromBuffer),
        DECLARE_NAPI_METHOD("copyFromImage", CopyFromImage),
        DECLARE_NAPI_METHOD("drawImage", DrawImage),
        DECLARE_NAPI_METHOD("toBuffer", ToBuffer),
        DECLARE_NAPI_METHOD("gc", GC)
    };

    napi_value cons;
    status = napi_define_class(env, "Image", NAPI_AUTO_LENGTH, New, nullptr, 14, properties, &cons);
    assert(status == napi_ok); 

    status = napi_create_reference(env, cons, 1, &constructor);
    assert(status == napi_ok);

    DEFINE_NAPI_CONSTANT("TYPE_PNG", TYPE_PNG);
    DEFINE_NAPI_CONSTANT("TYPE_JPEG", TYPE_JPEG);
    DEFINE_NAPI_CONSTANT("TYPE_GIF", TYPE_GIF);
    DEFINE_NAPI_CONSTANT("TYPE_BMP", TYPE_BMP);
    DEFINE_NAPI_CONSTANT("TYPE_RAW", TYPE_RAW);
    DEFINE_NAPI_CONSTANT("TYPE_WEBP", TYPE_WEBP);

    status = napi_set_named_property(env, exports, "Image", cons);
    assert(status == napi_ok);

    return exports;
} //}}}

void Image::setError(const char * err){ // {{{
    error = err;
} // }}}

napi_value Image::getError(napi_env env, napi_callback_info info) { // {{{
    napi_status status;
    napi_value code, msg, err;

    napi_create_string_utf8(env, "50030", 6, &code);
    napi_create_string_utf8(env, error ? error : "Unkonw Error", 1024, &msg);
    napi_create_error(env, code, msg, &err);

    error = NULL;

    return err;
} // }}}

bool Image::isError(){ // {{{
    return error != NULL;
} // }}}

napi_value Image::GetMaxWidth(napi_env env, napi_callback_info info) { // {{{
    napi_status status;
    napi_value width;

    status = napi_create_int32(env, maxWidth, &width);
    assert(status == napi_ok);

    return width;

} // }}}

napi_value Image::SetMaxWidth(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    size_t argc = 1;
    napi_value value;
    napi_value jsthis;

    status = napi_get_cb_info(env, info, &argc, &value, &jsthis, nullptr);
    assert(status == napi_ok);

    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, value, (uint32_t *) &maxWidth);

    return value;
} // }}}

napi_value Image::GetMaxHeight(napi_env env, napi_callback_info info) { // {{{
    napi_status status;
    napi_value height;

    status = napi_create_int32(env, maxHeight, &height);
    assert(status == napi_ok);

    return height;
} // }}}

napi_value Image::SetMaxHeight(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    size_t argc = 1;
    napi_value value;
    napi_value jsthis;
    uint32_t maxHeight;

    status = napi_get_cb_info(env, info, &argc, &value, &jsthis, nullptr);
    assert(status == napi_ok);

    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, value, &maxHeight);

    return value;
} // }}}

// Memory
size_t Image::usedMemory = 0;

napi_value Image::GetUsedMemory(napi_env env, napi_callback_info info) { // {{{
    napi_status status;
    napi_value value;

    status = napi_create_int32(env, usedMemory, &value);
    assert(status == napi_ok);

    return value;
} // }}}

napi_value Image::GC(napi_env env, napi_callback_info info) { // {{{
    //V8::LowMemoryNotification();
    napi_value undefined;
    napi_get_undefined(env, &undefined);

    return undefined;
} // }}}

napi_value Image::New(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    napi_value target;
    status = napi_get_new_target(env, info, &target);
    assert(status == napi_ok);

    bool is_constructor = target != nullptr;
    if (is_constructor) {
        size_t argc = 2;
        napi_value args[2];
        napi_value jsthis;

        status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
        assert(status == napi_ok);

        uint32_t width, height;
        width = height = 0;

        GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[0], &width);
        GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[1], &height);

        Image *img = new Image();

        if (img->pixels->Malloc(width, height) != SUCCESS) {
            THROW_GET_ERROR();
            return nullptr;
        }

        img->env_ = env;
        status = napi_wrap(env, jsthis, reinterpret_cast<void*>(img), Image::Destructor, nullptr, &img->wrapper_);
        assert(status == napi_ok);

        return jsthis;
    } else {
        // @TODO
        return nullptr;
    }
} // }}}

napi_value Image::GetWidth(napi_env env, napi_callback_info info) { // {{{
    napi_status status;
    napi_value jsthis;

    status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
    assert(status == napi_ok);

    Image *img;
    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    napi_value value;
    status = napi_create_int32(env, img->pixels->width, &value);
    assert(status == napi_ok);

    return value;
} // }}}

napi_value Image::SetWidth(napi_env env, napi_callback_info info){ // {{{
    napi_status status;

    napi_value jsthis;
    size_t argc = 1;
    napi_value args[1];
    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);

    Image *img;
    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    uint32_t value;
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[0], &value);
    img->pixels->SetWidth((size_t) value);

    return jsthis;
} // }}}

napi_value Image::GetHeight(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    napi_value jsthis;

    status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
    assert(status == napi_ok);

    Image *img;
    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    napi_value value;
    status = napi_create_int32(env, img->pixels->height, &value);
    assert(status == napi_ok);

    return value;

} // }}}

napi_value Image::SetHeight(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    napi_value jsthis;
    size_t argc = 1;
    napi_value args[1];
    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);

    Image *img;
    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    uint32_t value;
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[0], &value);
    img->pixels->SetHeight((size_t) value);

    return jsthis;

} // }}}


/**
 * Scale image with bicubic.
 * @since 1.5.5+
 */
napi_value Image::Resize(napi_env env, napi_callback_info info) {
    napi_status status;

    napi_value jsthis;
    napi_value value;

    size_t argc = 3;
    napi_value args[3];

    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok);

    uint32_t width = 0, height = 0;

    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[0], &width);
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[1], &height);

    char *filter = nullptr;
    size_t result_length;

    if (args[2]) {
        napi_valuetype typ;
        char buf[64] = {'\0'};
        napi_typeof(env, args[2], &typ);
        if (typ == napi_string) {
            status = napi_get_value_string_utf8(env, args[2], buf, sizeof(buf), &result_length);
            assert(status == napi_ok);
            assert(result_length > 0);
            filter = buf;
        }
    }

    Image *img;
    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    img->pixels->Resize(width, height, filter);

    return nullptr;
}

/**
 * Rotate image.
 * @since 1.5.5+
 */
napi_value Image::Rotate(napi_env env, napi_callback_info info) {

    napi_status status;

    napi_value jsthis;

    size_t argc = 1;
    napi_value args[1];

    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok);

    uint32_t rotate;
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[0], &rotate);

    Image *img;
    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    img->pixels->Rotate(rotate);
    
    return jsthis;
}

napi_value Image::GetTransparent(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    napi_value jsthis;

    status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
    assert(status == napi_ok);

    Image *img;
    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    napi_value value;
    status = napi_create_int32(env, img->pixels->type, &value);
    assert(status == napi_ok);

    return value;
} // }}}

napi_value Image::FillColor(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    napi_value jsthis;
    napi_value value;

    size_t argc = 4;
    napi_value args[4];

    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok);

    uint32_t r, g, b, a = 0;

    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[0], &r);
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[1], &g);
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[2], &b);
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[3], &a);


    Pixel color, *cp;

    cp = &color;
    cp->R = r;
    cp->G = g;
    cp->B = b;
    cp->A = 0xFF;

    if (a > 0) {
        cp->A = (uint8_t) (a * 0xFF);
    }
    
    Image *img;
    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    img->pixels->Fill(cp);

    return jsthis;
} // }}}

napi_value Image::LoadFromBuffer(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    napi_value jsthis;
    napi_value value;

    size_t argc = 3;
    napi_value args[3];

    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok);

    bool is_flag = false;

    status = napi_is_buffer(env, args[0], &is_flag);
    assert(status == napi_ok);

    if (!is_flag) {
        THROW_TYPE_ERROR(": first argument must be a buffer.");
        assert(is_flag);
    }

    Image *img;

    uint8_t *buffer;
    uint32_t start, end, length;

    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    ImageCodec *codec;
    ImageDecoder decoder;
    ImageData input_data, *input;

    status = napi_get_buffer_info(env, args[0], (void **) &buffer, (size_t *)&length);
    assert(status == napi_ok);

    start = 0;
    if (argc >= 2) {          
        GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[1], &start);
    }

    end = length;
    if (argc >= 3) {
        GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[2], &end);
        if(end < start || end > length){
            THROW_TYPE_ERROR("invalid `start` and `end`");
            return nullptr;
        }
    }

    input = &input_data;
    input->data = &buffer[start];
    input->length = end - start;

    img->pixels->Free();
    codec = codecs;

    while (codec != NULL && !isError()) {
        decoder = codec->decoder;
        input->position = 0;
        if (decoder != NULL && decoder(img->pixels, input) == SUCCESS) {
            return jsthis;
        }
        codec = codec->next;
    }

    isError() ? (THROW_GET_ERROR()) : THROW_ERROR("Unknow format");
    return jsthis;
} // }}}

napi_value Image::CopyFromImage(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    napi_value jsthis;

    size_t argc = 5;
    napi_value args[5];

    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok);

    Image *src, *dst;
    uint32_t x, y, w, h = 0;

    // @TODO

    if (argc == 0) {
        return nullptr;
    }
    
    napi_value obj;
    status = napi_coerce_to_object(env, args[0], &obj);
    assert(status == napi_ok);

    status = napi_unwrap(env, obj, reinterpret_cast<void **>(&src));
    assert(status == napi_ok);

    status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&dst));
    assert(status == napi_ok);

    x = y = 0;
    w = src->pixels->width;
    h = src->pixels->height;

    if (argc >= 3) {
        GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[1], &x);
        GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[2], &y);
    }

    if(argc >= 5) {
        GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[3], &w);
        GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[4], &h);
    }

    if(dst->pixels->CopyFrom(src->pixels, x, y, w, h) != SUCCESS){
        THROW_GET_ERROR();
    }

    return nullptr;
}// }}}

napi_value Image::DrawImage(napi_env env, napi_callback_info info) { // {{{
    napi_status status;

    napi_value jsthis;

    size_t argc = 3;
    napi_value args[3];

    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok);

    if (argc == 0) {
        return nullptr;
    }

    Image *src, *dst = nullptr;
    uint32_t x, y = 0;

    if (argc < 3) {
        napi_throw_error(env, "50000", "ERROR: invalid arguments.");
        return nullptr;
    }

    napi_value obj;
    status = napi_coerce_to_object(env, args[0], &obj);
    assert(status == napi_ok);
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[1], &x);
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[2], &y);

    status = napi_unwrap(env, obj, reinterpret_cast<void **>(&src));
    assert(status == napi_ok);

    status = napi_unwrap(env, jsthis, reinterpret_cast<void **>(&dst));
    assert(status == napi_ok);

    dst->pixels->Draw(src->pixels, x, y);

    return jsthis;
} // }}}

napi_value Image::ToBuffer(napi_env env, napi_callback_info info) { //{{{
    napi_status status;

    napi_value jsthis;

    size_t argc = 2;
    napi_value args[2];

    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok);

    Image *img;
    ImageType type;
    PixelArray *pixels;
    ImageConfig _config, *config;
    ImageCodec *codec;
    ImageEncoder encoder;

    ImageData output_data, *output;

    int length = 0;

    if (argc == 0) {
        THROW_INVALID_ARGUMENTS_ERROR("");
        return nullptr;
    }

    uint32_t result = 0;
    GET_VALUE_WITH_NAPI_FUNC(napi_get_value_uint32, args[0], &result);

    type = (ImageType) result;
    config = NULL;

    bool is_flag = false;

    status = napi_is_buffer(env, args[1], &is_flag);
    assert(status == napi_ok);

    if (is_flag) {
        char* buffer;
        unsigned length;
        
        status = napi_get_buffer_info(env, args[1], (void **)&buffer, (size_t *)&length);
        assert(status == napi_ok);

        config = &_config;
        config->data = buffer;
        config->length = length;
    }

    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&img));
    assert(status == napi_ok);

    pixels = img->pixels;

    if(pixels->data != NULL){
        codec = codecs;
        output = &output_data;
        output->data = NULL;
        output->length = 0;
        output->position = 0;

        while (codec != NULL && !isError()) {
            if (codec->type == type) {
                encoder = codec->encoder;
                if (encoder != NULL) {
                    if (encoder(pixels, output, config) == SUCCESS) {
                        length = output->position;

                        // MaybeLocal<Object> maybeBuffer = node::Buffer::New(args.GetIsolate(), (size_t) length);
                        // maybeBuffer.ToLocal(&buffer);
                        // memcpy(node::Buffer::Data(buffer), output->data, length);
                        // 
                        // args.GetReturnValue().Set(buffer);
                        napi_value result_buffer;
                        status = napi_create_buffer_copy(env, length, output->data, nullptr, &result_buffer);
                        assert(status == napi_ok);

                        free(output->data);
                        return result_buffer;
                    } else {
                        if (output->data != NULL)
                            free(output->data);
                        THROW_ERROR("Encode fail.");
                        return nullptr;
                    }
                } else {
                    THROW_ERROR("Can't encode to this format.");
                }
            }
            codec = codec->next;
        }
        isError() ? (THROW_GET_ERROR()) : (THROW_ERROR("Unsupported type."));
        return nullptr;
    }else{
        THROW_ERROR("Image uninitialized.");
        return nullptr;
    }

} // }}}

void Image::regCodec(ImageDecoder decoder, ImageEncoder encoder, ImageType type){ // {{{
    ImageCodec *codec;
    codec = (ImageCodec *) malloc(sizeof(ImageCodec));
    codec->next = codecs;
    codec->decoder = decoder;
    codec->encoder = encoder;
    codec->type = type;
    codecs = codec;
} // }}}


void Pixel::Merge(Pixel *pixel){ // {{{
    double a, af, ab;
    ab = (double) A / 0xFF;
    af = (double) pixel->A / 0xFF;
    a  = (1 - (1 - af) * (1 - ab));

    R = (uint8_t) ((pixel->R * af + R * ab * (1 - af)) / a);
    G = (uint8_t) ((pixel->G * af + G * ab * (1 - af)) / a);
    B = (uint8_t) ((pixel->B * af + B * ab * (1 - af)) / a);
    A = (uint8_t) (a * 0xFF);
} // }}}

ImageState PixelArray::Malloc(size_t w, size_t h){ // {{{
    int32_t size;
    Pixel *line;

    if(w > 0 && h > 0){
        if(w > Image::maxWidth || h > Image::maxHeight){
            SET_ERROR("Beyond the pixel size limit.");
            goto fail;
        }

        if((data = (Pixel**) malloc(h * sizeof(Pixel**))) == NULL){
            SET_ERROR("Out of memory.");
            goto fail;
        }

        width = w;
        size = width * sizeof(Pixel);
        for(height = 0; height < h; height++){
            if((line = (Pixel*) malloc(size)) == NULL){
                SET_ERROR("Out of memory.");
                goto free;
            }
            memset(line, 0x00, size);
            data[height] = line;
        }
    }
    size = Size();
    AdjustAmountOfExternalAllocatedMemory(size);
    Image::usedMemory += size;
    return SUCCESS;

free:
    while(height--)
        free(data[height]);
    free(data);

fail:
    width = height = 0;
    type = EMPTY;
    data = NULL;
    return FAIL;
} // }}}

void PixelArray::Free(){ // {{{
    size_t h, size;

    if(data != NULL){
        h = height;
        while(h--){
            if(data[h] != NULL) free(data[h]);
        }
        free(data);
        size = Size();
        AdjustAmountOfExternalAllocatedMemory(-size);
        Image::usedMemory -= size;
    }

    width = height = 0;
    type = EMPTY;;
    data = NULL;
} // }}}

ImageState PixelArray::CopyFrom(PixelArray *src, size_t x, size_t y, size_t w, size_t h){ // {{{
    size_t sw, sh, size;

    sw = src->width;
    sh = src->height;

    if(src->data && x < sw && y < sh){
        if(x + w > sw) w = sw - x;
        if(y + h > sh) h = sh - y;

        Free();
        size = w * sizeof(Pixel);
        if(Malloc(w, h) != SUCCESS) return FAIL;

        while(h--){
            memcpy(data[h], &(src->data[y+h][x]), size);
        }
        type = src->type;
    }

    return SUCCESS;
} // }}}

void PixelArray::Draw(PixelArray *src, size_t x, size_t y){ // {{{
    //TODO
    size_t sw, sh, dw, dh, w, h, sx, sy, size;
    PixelArrayType st;
    Pixel *sp, *dp;

    sw = src->width;
    sh = src->height;
    dw = width;
    dh = height;
    st = src->type;

    /* for TEST out put first pixel;

       printf("0x%02X%02X%02X\n",
       src->data[0][0].R,
       src->data[0][0].G,
       src->data[0][0].B
       );
    //*/

    if(x < dw && y < dh){
        w = (x + sw < dw) ? sw : (dw - x);
        h = (y + sh < dh) ? sh : (dh - y);
        size = w * sizeof(Pixel);

        if(type == EMPTY || st == SOLID){ // src opaque or dest empty
            for(sy = 0; sy < h; sy++){
                sp = src->data[sy];
                dp = &(data[y + sy][x]);
                memcpy(dp, sp, size);
            }
        }else{
            for(sy = 0; sy < h; sy++){
                for(sx = 0; sx < w; sx++){
                    sp = &(src->data[sy][sx]);
                    dp = &(data[y + sy][x + sx]);
                    if(sp->A == 0x00){ // src pixel transparent
                        //DO Nothing
                    }else if(sp->A == 0xFF || dp->A == 0x00){ // src pixel opaque or dest pixel transparent
                        *dp = *sp;
                    }else{
                        dp->Merge(sp);
                    }
                }
            }
        }
        DetectTransparent();
    }
} // }}}

void PixelArray::Fill(Pixel *color){ // {{{
    size_t i, size;
    uint8_t a;
    bool same;
    Pixel *row, *p;

    if(data != NULL){
        a = color->A;
        if(a == 0x00 && type == EMPTY) return;

        same = (color->R == a && color->G == a && color->B == a);
        row = data[0];
        if(same){
            size = width * sizeof(Pixel);
            memset(row, a, size);
        }else{
            for(i = 0, p = row; i < width; i++, p++){
                *p = *color;
            }
        }

        size = width * sizeof(Pixel);
        for(i = 1; i < height; i++){
            memcpy(data[i], row, size);
        }

        type = ((a == 0xFF) ? SOLID :((a == 0x00) ? EMPTY : ALPHA));
    }
} // }}}

ImageState PixelArray::SetWidth(size_t w){ // {{{
    size_t size, *index, *p, x, y;
    double scale;
    Pixel *src, *dst;
    PixelArray newArray, *pixels;


    if(data != NULL){
        if(w > Image::maxWidth){
            SET_ERROR("Beyond the width limit.");
            return FAIL;
        }

        if(w == width){
            return SUCCESS;
        }

        size = w * sizeof(size_t);
        if((index = (size_t *) malloc(size)) == NULL){
            SET_ERROR("Out of memory.");
            return FAIL;
        }

        scale = ((double) width) / w;
        for(x = 0, p = index; x < w; x++, p++){
            *p = (size_t) (scale * x);
        }

        pixels = &newArray;
        if(pixels->Malloc(w, height) != SUCCESS){
            free(index);
            return FAIL;
        }
        pixels->type = type;

        for(y = 0; y < height; y++){
            src = data[y];
            dst = pixels->data[y];
            for(x = 0, p = index; x < w; x++, p++){
                dst[x] = src[*p];
            }
        }
        free(index);
        Free();
        *this = *pixels;
    }
    return SUCCESS;
} // }}}

ImageState PixelArray::SetHeight(size_t h){ // {{{
    PixelArray newArray, *pixels;
    size_t size, y;
    double scale;
    Pixel *src, *dst;

    if(data != NULL){

        if(h > Image::maxHeight){
            SET_ERROR("Beyond the height limit.");
            return FAIL;
        }

        if(h == height){
            return SUCCESS;
        }

        pixels = &newArray;
        if(pixels->Malloc(width, h) != SUCCESS){
            return FAIL;
        }
        pixels->type = type;

        size = width * sizeof(Pixel);
        scale = ((double) height) / h;
        for(y = 0; y < h; y++){
            src = data[(size_t) (scale * y)];
            dst = pixels->data[y];
            memcpy(dst, src, size);
        }

        Free();
        *this = *pixels;
    }
    return SUCCESS;
} // }}}

ImageState PixelArray::Resize(size_t w, size_t h, const char *filter){
    PixelArray newArray, *pixels;

    if ( (int)w < 1 ) {
        w = width * h / height;
    }

    if ( (int)h < 1 ) {
        h = height * w / width;
    }

    if(data != NULL){
        if(w > Image::maxWidth){
            SET_ERROR("Beyond the width limit.");
            return FAIL;
        }

        if(h > Image::maxHeight){
            SET_ERROR("Beyond the height limit.");
            return FAIL;
        }

        if(w == width && h == height){
            return SUCCESS;
        }

        pixels = &newArray;
        if(pixels->Malloc(w, h) != SUCCESS){
            return FAIL;
        }
        pixels->type = type;

        resize( this, pixels, filter );

        Free();
        *this = *pixels;

        // printf( "%d, %d, %d\n", this->data[122][267].R, this->data[122][267].G, this->data[122][267].B);
    }
    return SUCCESS;
}

ImageState PixelArray::Rotate(size_t deg){
    PixelArray newArray, *pixels;
    size_t w,h;
    
    deg = deg % 360;

    if(data != NULL){
        if(deg==0){
            return SUCCESS;
        }

        pixels = &newArray;
        pixels->type = type;
        
        if(rotate( this, pixels, (const size_t) deg) != SUCCESS){
            return FAIL;
        }

        Free();
        *this = *pixels;
    }
    return SUCCESS;
}

void PixelArray::DetectTransparent(){ // {{{
    size_t x, y;
    Pixel *pixel;
    bool empty, opaque, alpha;
    type = EMPTY;

    empty = opaque = alpha = false;

    for(y = 0; y < height; y++){
        pixel = data[y];
        for(x = 0; x < width; x++, pixel++){
            switch(pixel->A){
                case 0x00:
                    empty = true;
                    break;
                case 0xFF:
                    opaque = true;
                    break;
                default:
                    alpha = true;
                    break;
            }

            if(alpha || (empty && opaque)){
                type = ALPHA;
                return;
            }
        }
    }
    type = opaque ? SOLID : EMPTY;
} // }}}

extern "C" { // {{{
    napi_value Init(napi_env env, napi_value exports) {
      return Image::Init(env, exports);
    }
} // }}}


NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);

// vim600: sw=4 ts=4 fdm=marker syn=cpp
