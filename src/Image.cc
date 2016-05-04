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
#include <node_buffer.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iostream>



using v8::Isolate;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Function;
using v8::Value;
using v8::Number;
using v8::String;
using v8::Exception;
using v8::Persistent;
using v8::Local;
using v8::MaybeLocal;
using v8::ObjectTemplate;
using v8::PropertyCallbackInfo;
using v8::Object;


//#define SET_ERROR_FILE_LINE(file, line, msg) Image::SetError( file #line msg)
//#define SET_ERROR(msg) SET_ERROR_FILE_LINE(__FILE__, __LINE__, meg)

#define STRINGFY(n) #n
#define MERGE_FILE_LINE(file, line, msg) ( file ":" STRINGFY(line) " " msg)
#define FILE_LINE(msg) MERGE_FILE_LINE(__FILE__, __LINE__, msg)
#define ERROR(type, msg) Exception::type(String::NewFromUtf8( Isolate::GetCurrent(), msg ))
#define THROW(err) Isolate::GetCurrent()->ThrowException(err)
#define SET_ERROR(msg) (Image::setError(FILE_LINE(msg)))
#define GET_ERROR() (Image::getError())
#define THROW_ERROR(msg) THROW(ERROR(Error,FILE_LINE(msg)))
#define THROW_GET_ERROR() THROW(GET_ERROR())

#define THROW_TYPE_ERROR(msg) THROW(ERROR(TypeError, FILE_LINE(msg)))
#define THROW_INVALID_ARGUMENTS_ERROR(msg) THROW_TYPE_ERROR("Invalid arguments" msg)

#define DEFAULT_WIDTH_LIMIT  10240 // default limit 10000x10000
#define DEFAULT_HEIGHT_LIMIT 10240 // default limit 10000x10000

#define AdjustAmountOfExternalAllocatedMemory(bc) static_cast<int>( \
        v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(bc));

Nan::Persistent<Function> Image::constructor;

//size_t Image::survival;
ImageCodec *Image::codecs;

size_t Image::maxWidth = DEFAULT_WIDTH_LIMIT;
size_t Image::maxHeight = DEFAULT_HEIGHT_LIMIT;
const char *Image::error = NULL;

void Image::Init(Local<Object> exports) { // {{{
    Nan::HandleScope scope;

    regAllCodecs();
    //survival = 0;

    // Constructor
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);

    tpl->SetClassName(Nan::New("Image").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    Local<ObjectTemplate> proto = tpl->PrototypeTemplate();

    Nan::SetPrototypeMethod(tpl, "resize", Resize);
    Nan::SetPrototypeMethod(tpl, "fillColor", FillColor);
    Nan::SetPrototypeMethod(tpl, "loadFromBuffer", LoadFromBuffer);
    Nan::SetPrototypeMethod(tpl, "copyFromImage", CopyFromImage);
    Nan::SetPrototypeMethod(tpl, "drawImage", DrawImage);
    Nan::SetPrototypeMethod(tpl, "toBuffer", ToBuffer);

    Nan::SetAccessor(proto, Nan::New("width").ToLocalChecked(), GetWidth, SetWidth);
    Nan::SetAccessor(proto, Nan::New("height").ToLocalChecked(), GetHeight, SetHeight);
    Nan::SetAccessor(proto, Nan::New("transparent").ToLocalChecked(), GetTransparent);

    constructor.Reset(tpl->GetFunction());

    NODE_DEFINE_CONSTANT(exports, TYPE_PNG);
    NODE_DEFINE_CONSTANT(exports, TYPE_JPEG);
    NODE_DEFINE_CONSTANT(exports, TYPE_GIF);
    NODE_DEFINE_CONSTANT(exports, TYPE_BMP);
    NODE_DEFINE_CONSTANT(exports, TYPE_RAW);

    Nan::SetAccessor(exports, Nan::New("maxWidth").ToLocalChecked(), GetMaxWidth, SetMaxWidth);
    Nan::SetAccessor(exports, Nan::New("maxHeight").ToLocalChecked(), GetMaxHeight, SetMaxHeight);
    Nan::SetAccessor(exports, Nan::New("usedMemory").ToLocalChecked(), GetUsedMemory);

    Nan::SetMethod(exports, "gc", GC);

    exports->Set(Nan::New("Image").ToLocalChecked(), tpl->GetFunction());

} //}}}

ImageState Image::setError(const char * err){ // {{{
    error = err;
    return FAIL;
} // }}}

Local<Value> Image::getError(){ // {{{
    Local<Value> err = Exception::Error(Nan::New(error ? error : "Unknow Error").ToLocalChecked());
    error = NULL;
    return err;
} // }}}

bool Image::isError(){ // {{{
    return error != NULL;
} // }}}

NAN_GETTER(Image::GetMaxWidth) { // {{{
    info.GetReturnValue().Set(Nan::New<Number>(maxWidth));

} // }}}

NAN_SETTER(Image::SetMaxWidth) { // {{{
    if(value->IsNumber())
        maxWidth = value->Uint32Value();
} // }}}

NAN_GETTER(Image::GetMaxHeight) {
    info.GetReturnValue().Set(Nan::New<Number>(maxHeight));
} // }}}

NAN_SETTER(Image::SetMaxHeight) { // {{{
    if(value->IsNumber())
        maxHeight = value->Uint32Value();
} // }}}

// Memory
size_t Image::usedMemory = 0;

NAN_GETTER(Image::GetUsedMemory) { // {{{

    info.GetReturnValue().Set(Nan::New<Number>(usedMemory));
} // }}}

NAN_METHOD(Image::GC) { // {{{
    //V8::LowMemoryNotification();
    info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
} // }}}

NAN_METHOD(Image::New) { // {{{

    Image *img;

    size_t width, height;

    width = height = 0;

    if (info.IsConstructCall()) {
        if(info[0]->IsNumber()) width = info[0]->Uint32Value();
        if(info[1]->IsNumber()) height = info[1]->Uint32Value();

        img = new Image();

        if(img->pixels->Malloc(width, height) != SUCCESS) {
            THROW_GET_ERROR();
        }

        img->Wrap(info.This());

        info.GetReturnValue().Set(info.This());
    } else {
        THROW_ERROR("must be use new Image.");
    }
} // }}}

NAN_GETTER(Image::GetWidth) { // {{{
    Image *img = ObjectWrap::Unwrap<Image>(info.This());
    info.GetReturnValue().Set(Nan::New<Number>(img->pixels->width));
} // }}}

NAN_SETTER(Image::SetWidth) { // {{{
    if(value->IsNumber()){
        Image *img = ObjectWrap::Unwrap<Image>(info.This());
        img->pixels->SetWidth(value->Uint32Value());
    }
} // }}}

NAN_GETTER(Image::GetHeight) { // {{{

    Image *img = ObjectWrap::Unwrap<Image>(info.This());
    info.GetReturnValue().Set(Nan::New<Number>(img->pixels->height));

} // }}}

NAN_SETTER(Image::SetHeight) { // {{{
    if(value->IsNumber()){
        Image *img = ObjectWrap::Unwrap<Image>(info.This());
        img->pixels->SetHeight(value->Uint32Value());
    }
} // }}}


/**
 * Scale image with bicubic.
 * @since 1.5.5+
 */
NAN_METHOD(Image::Resize) {

    char *filter = NULL;

    if( (!info[0]->IsNull() && !info[0]->IsUndefined () && !info[0]->IsNumber()) ||
            (!info[1]->IsNull() && !info[1]->IsUndefined () && !info[1]->IsNumber()) ) {
        THROW_INVALID_ARGUMENTS_ERROR("Arguments error");
        return;
    }

    if ( info[2]->IsString() ) {
        String::Utf8Value cstr(info[2]);
        filter = new char[strlen(*cstr)+1];
        strcpy(filter, *cstr);
    }

    Image *img = ObjectWrap::Unwrap<Image>(info.This());
    img->pixels->Resize(info[0]->ToNumber()->Value(), info[1]->ToNumber()->Value(), filter);

    info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
}



NAN_GETTER(Image::GetTransparent) { // {{{
    Image *img = ObjectWrap::Unwrap<Image>(info.This());
    info.GetReturnValue().Set(Nan::New<Number>(img->pixels->type));
} // }}}

NAN_METHOD(Image::FillColor) { // {{{

    Image *img;
    Pixel color, *cp;

    if(!info[0]->IsNumber()
            || !info[1]->IsNumber()
            || !info[2]->IsNumber()) {
        THROW_INVALID_ARGUMENTS_ERROR("");
        return;
    }

    cp = &color;
    cp->R = info[0]->Uint32Value();
    cp->G = info[1]->Uint32Value();
    cp->B = info[2]->Uint32Value();
    cp->A = 0xFF;

    if(info[3]->IsNumber()){
        cp->A = (uint8_t) (info[3]->NumberValue() * 0xFF);
    }

    img = ObjectWrap::Unwrap<Image>(info.This());
    img->pixels->Fill(cp);

    info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
} // }}}

NAN_METHOD(Image::LoadFromBuffer) { // {{{

    Image *img;

    uint8_t *buffer;
    unsigned start, end, length;

    ImageCodec *codec;
    ImageDecoder decoder;
    ImageData input_data, *input;

    if(!node::Buffer::HasInstance(info[0])){
        THROW_TYPE_ERROR(": first argument must be a buffer.");
        return;
    }

    img = ObjectWrap::Unwrap<Image>(info.This());

    buffer = (uint8_t *) node::Buffer::Data(info[0]);
    length = (unsigned) node::Buffer::Length(info[0]);

    start = 0;
    if(info[1]->IsNumber()){
        start = info[1]->Uint32Value();
    }

    end = length;
    if(info[2]->IsNumber()){
        end = info[2]->Uint32Value();
        if(end < start || end > length){
            THROW_TYPE_ERROR("");
            return;
        }
    }

    input = &input_data;
    input->data = &buffer[start];
    input->length = end - start;

    img->pixels->Free();
    codec = codecs;
    while(codec != NULL && !isError()){
        decoder = codec->decoder;
        input->position = 0;
        if(decoder != NULL && decoder(img->pixels, input) == SUCCESS){
            info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
            return;
        }
        codec = codec->next;
    }
    isError() ? (THROW_GET_ERROR()) : THROW_ERROR("Unknow format");
    return;
} // }}}

NAN_METHOD(Image::CopyFromImage) { // {{{

    Image *src, *dst;
    uint32_t x, y, w, h;

    Local<Object> obj = info[0]->ToObject();

    //@TODO

    src = ObjectWrap::Unwrap<Image>(obj);
    dst = ObjectWrap::Unwrap<Image>(info.This());

    x = y = 0;
    w = src->pixels->width;
    h = src->pixels->height;

    if(info[1]->IsNumber()   // x
            && info[2]->IsNumber()){ // y
        x = info[1]->Uint32Value();
        y = info[2]->Uint32Value();
    }

    if(info[3]->IsNumber()   // w
            && info[4]->IsNumber()){ // h
        w = info[3]->Uint32Value();
        h = info[4]->Uint32Value();
    }

    if(dst->pixels->CopyFrom(src->pixels, x, y, w, h) != SUCCESS){
        THROW_GET_ERROR();
    }

}// }}}

NAN_METHOD(Image::DrawImage) { // {{{

    Image *src, *dst;
    size_t x, y;

    Local<Object> obj = info[0]->ToObject();

    //if(!NanHasInstance(Image::constructor, obj)
    //   || !args[1]->IsNumber() // x
    //   || !args[2]->IsNumber()) // y
    if (!info[1]->IsNumber() || !info[2]->IsNumber()) {
        THROW_INVALID_ARGUMENTS_ERROR("");
        return;
    }

    src = ObjectWrap::Unwrap<Image>(obj);
    dst = ObjectWrap::Unwrap<Image>(info.This());

    x = info[1]->Uint32Value();
    y = info[2]->Uint32Value();

    dst->pixels->Draw(src->pixels, x, y);

    info.GetReturnValue().Set(v8::Undefined(info.GetIsolate()));
} // }}}

NAN_METHOD(Image::ToBuffer) { //{{{

    Image *img;
    ImageType type;
    PixelArray *pixels;
    ImageConfig _config, *config;
    ImageCodec *codec;
    ImageEncoder encoder;

    ImageData output_data, *output;

    Local<Object> buffer;

    int length;

    if(!info[0]->IsNumber()) {
        THROW_INVALID_ARGUMENTS_ERROR("");
        return;
    }

    type = (ImageType) info[0]->Uint32Value();
    config = NULL;
    if(node::Buffer::HasInstance(info[1])){
        config = &_config;
        config->data = node::Buffer::Data(info[1]->ToObject());
        config->length = node::Buffer::Length(info[1]->ToObject());
    }

    img = ObjectWrap::Unwrap<Image>(info.This());
    pixels = img->pixels;

    if(pixels->data != NULL){
        codec = codecs;
        output = &output_data;
        output->data = NULL;
        output->length = 0;
        output->position = 0;

        while(codec != NULL && !isError()){
            if(codec->type == type){
                encoder = codec->encoder;
                if(encoder != NULL){
                    if(encoder(pixels, output, config) == SUCCESS){
                        length = output->position;
                        MaybeLocal<Object> maybeBuffer = node::Buffer::New(info.GetIsolate(), (size_t) length);
                        maybeBuffer.ToLocal(&buffer);
                        memcpy(node::Buffer::Data(buffer), output->data, length);
                        free(output->data);
                        info.GetReturnValue().Set(buffer);
                        return;
                    }else{
                        if(output->data != NULL)
                            free(output->data);
                        THROW_ERROR("Encode fail.");
                        return;
                    }
                }else{
                    THROW_ERROR("Can't encode to this format.");
                }
            }
            codec = codec->next;
        }
        isError() ? (THROW_GET_ERROR()) : (THROW_ERROR("Unsupported type."));
        return;
    }else{
        THROW_ERROR("Image uninitialized.");
        return;
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

Image::Image(){ // {{{
    size_t size;
    pixels = (PixelArray *) malloc(sizeof(PixelArray));
    pixels->width = pixels->height = 0;
    pixels->type = EMPTY;
    pixels->data = NULL;
    size = sizeof(PixelArray) + sizeof(Image);
    AdjustAmountOfExternalAllocatedMemory(size);
    usedMemory += size;
    //survival++;
} // }}}

Image::~Image(){ // {{{
    int32_t size;
    size = sizeof(PixelArray) + sizeof(Image);
    pixels->Free();
    free(pixels);
    AdjustAmountOfExternalAllocatedMemory(-size);
    usedMemory -= size;
    //survival--;
    //printf("survival:%d\n", survival);
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
            return SET_ERROR("Beyond the height limit.");
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
            return SET_ERROR("Beyond the width limit.");
        }

        if(h > Image::maxHeight){
            return SET_ERROR("Beyond the height limit.");
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

extern "C" {
void InitAll (Local<Object> exports) { // {{{
    Image::Init(exports);
} // }}}
}


NODE_MODULE(binding, InitAll);

// vim600: sw=4 ts=4 fdm=marker syn=cpp
