//
// Image.cc
//
// Copyright (c) 2013 ZhangYuanwei <zhangyuanwei1988@gmail.com>

#include "Image.h"
#include "Resize.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

//#define SET_ERROR_FILE_LINE(file, line, msg) Image::SetError( file #line msg)
//#define SET_ERROR(msg) SET_ERROR_FILE_LINE(__FILE__, __LINE__, meg)

#define STRINGFY(n) #n
#define MERGE_FILE_LINE(file, line, msg) ( file ":" STRINGFY(line) " " msg)
#define FILE_LINE(msg) MERGE_FILE_LINE(__FILE__, __LINE__, msg)
#define ERRORR(type, msg) (msg)
#define THROW(err) NanThrowError(err)

#define SET_ERROR(msg) (Image::setError(FILE_LINE(msg)))
#define GET_ERROR() (Image::getError())
#define THROW_ERROR(msg) THROW(ERRORR(,FILE_LINE(msg)))
#define THROW_GET_ERROR() THROW(GET_ERROR())

#define THROW_TYPE_ERROR(msg) NanThrowTypeError(FILE_LINE(msg)))
#define THROW_INVALID_ARGUMENTS_ERROR(msg) NanThrowTypeError("Invalid arguments" msg)

#define DEFAULT_WIDTH_LIMIT  10240 // default limit 10000x10000
#define DEFAULT_HEIGHT_LIMIT 10240 // default limit 10000x10000

Persistent<FunctionTemplate> Image::constructor;
//size_t Image::survival;
ImageCodec *Image::codecs;

size_t Image::maxWidth = DEFAULT_WIDTH_LIMIT;
size_t Image::maxHeight = DEFAULT_HEIGHT_LIMIT;
const char *Image::error = NULL;

void Image::Initialize(Handle<Object> exports){ // {{{
    NanScope();

    regAllCodecs();
    //survival = 0;

    // Constructor
    Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
    //constructor = Persistent<FunctionTemplate>::New(tpl);
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    tpl->SetClassName(NanNew("Image"));

    // Prototype
    Local<ObjectTemplate> proto = tpl->PrototypeTemplate();
    NODE_SET_PROTOTYPE_METHOD(tpl, "resize", Resize);
    NODE_SET_PROTOTYPE_METHOD(tpl, "fillColor", FillColor);
    NODE_SET_PROTOTYPE_METHOD(tpl, "loadFromBuffer", LoadFromBuffer);
    NODE_SET_PROTOTYPE_METHOD(tpl, "copyFromImage", CopyFromImage);
    NODE_SET_PROTOTYPE_METHOD(tpl, "drawImage", DrawImage);
    NODE_SET_PROTOTYPE_METHOD(tpl, "toBuffer", ToBuffer);

    proto->SetAccessor(NanNew("width"), GetWidth, SetWidth);
    proto->SetAccessor(NanNew("height"), GetHeight, SetHeight);
    proto->SetAccessor(NanNew("transparent"), GetTransparent);

    NODE_DEFINE_CONSTANT(exports, TYPE_PNG);
    NODE_DEFINE_CONSTANT(exports, TYPE_JPEG);
    NODE_DEFINE_CONSTANT(exports, TYPE_GIF);
    NODE_DEFINE_CONSTANT(exports, TYPE_BMP);
    NODE_DEFINE_CONSTANT(exports, TYPE_RAW);

    exports->SetAccessor(NanNew("maxWidth"), GetMaxWidth, SetMaxWidth);
    exports->SetAccessor(NanNew("maxHeight"), GetMaxHeight, SetMaxHeight);
    exports->SetAccessor(NanNew("usedMemory"), GetUsedMemory);
    NODE_SET_METHOD(exports, "gc", GC);

    NanAssignPersistent(constructor, tpl);

    exports->Set(NanNew("Image"), tpl->GetFunction());
} //}}}

ImageState Image::setError(const char * err){ // {{{
    error = err;
    return FAIL;
} // }}}

Local<Value> Image::getError(){ // {{{
    Local<Value> err = Exception::Error(NanNew(error ? error : "Unknow Error"));
    error = NULL;
    return err;
} // }}}

bool Image::isError(){ // {{{
    return error != NULL;
} // }}}

NAN_GETTER(Image::GetMaxWidth) { // {{{
    NanScope();
    NanReturnValue(NanNew<Number>(maxWidth));
} // }}}

NAN_SETTER(Image::SetMaxWidth) { // {{{
    if(value->IsNumber())
        maxWidth = value->Uint32Value();
} // }}}

NAN_GETTER(Image::GetMaxHeight) { // {{{
    NanScope();
    NanReturnValue(NanNew<Number>(maxHeight));
} // }}}

NAN_SETTER(Image::SetMaxHeight) { // {{{
    if(value->IsNumber())
        maxHeight = value->Uint32Value();
} // }}}
        
// Memory
size_t Image::usedMemory = 0;

NAN_GETTER(Image::GetUsedMemory) { // {{{
    NanScope();
    NanReturnValue(NanNew<Number>(usedMemory));
} // }}}

NAN_METHOD(Image::GC) { // {{{
    NanScope();
    //V8::LowMemoryNotification();
    NanLowMemoryNotification();
    NanReturnUndefined();
} // }}}

NAN_METHOD(Image::New) { // {{{
    NanScope();

    Image *img;

    size_t width, height;

    width = height = 0;

    if(args[0]->IsNumber()) width = args[0]->Uint32Value();
    if(args[1]->IsNumber()) height = args[1]->Uint32Value();

    img = new Image();

    if(img->pixels->Malloc(width, height) != SUCCESS){
        return THROW_GET_ERROR();
    }

    img->Wrap(args.This());

    NanReturnThis();
} // }}}

NAN_GETTER(Image::GetWidth) { // {{{
    NanScope();

    Image *img = ObjectWrap::Unwrap<Image>(args.This());
    NanReturnValue(NanNew<Number>(img->pixels->width));
} // }}}

NAN_SETTER(Image::SetWidth) { // {{{
    if(value->IsNumber()){
        Image *img = ObjectWrap::Unwrap<Image>(args.This());
        img->pixels->SetWidth(value->Uint32Value());
    }
} // }}}

NAN_GETTER(Image::GetHeight) { // {{{
    NanScope();

    Image *img = ObjectWrap::Unwrap<Image>(args.This());
    NanReturnValue(NanNew<Number>(img->pixels->height));
} // }}}

NAN_SETTER(Image::SetHeight) { // {{{
    if(value->IsNumber()){
        Image *img = ObjectWrap::Unwrap<Image>(args.This());
        img->pixels->SetHeight(value->Uint32Value());
    }
} // }}}


/**
 * Scale image with bicubic.
 * @since 1.5.5+
 */
NAN_METHOD(Image::Resize) {
    NanScope();

    char *filter = NULL;

    if( (!args[0]->IsNull() && !args[0]->IsUndefined () && !args[0]->IsNumber()) ||
        (!args[1]->IsNull() && !args[1]->IsUndefined () && !args[1]->IsNumber()) )
        return THROW_INVALID_ARGUMENTS_ERROR("Arguments error");

    if ( args[2]->IsString() ) {
        String::Utf8Value cstr(args[2]);
        filter = new char[strlen(*cstr)+1];
        strcpy(filter, *cstr);
    }

    Image *img = ObjectWrap::Unwrap<Image>(args.This());
    img->pixels->Resize(args[0]->ToNumber()->Value(), args[1]->ToNumber()->Value(), filter);
    
    NanReturnUndefined();
}



NAN_GETTER(Image::GetTransparent) { // {{{
    NanScope();

    Image *img = ObjectWrap::Unwrap<Image>(args.This());
    NanReturnValue(NanNew<Number>(img->pixels->type));
} // }}}

NAN_METHOD(Image::FillColor) { // {{{
    NanScope();

    Image *img;
    Pixel color, *cp;

    if(!args[0]->IsNumber()
    || !args[1]->IsNumber()
    || !args[2]->IsNumber())
        return THROW_INVALID_ARGUMENTS_ERROR("");

    cp = &color;
    cp->R = args[0]->Uint32Value();
    cp->G = args[1]->Uint32Value();
    cp->B = args[2]->Uint32Value();
    cp->A = 0xFF;

    if(args[3]->IsNumber()){
        cp->A = (uint8_t) (args[3]->NumberValue() * 0xFF);
    }

    img = ObjectWrap::Unwrap<Image>(args.This());
    img->pixels->Fill(cp);
    
    NanReturnUndefined();
} // }}}

NAN_METHOD(Image::LoadFromBuffer) { // {{{
    NanScope();

    Image *img;

    uint8_t *buffer;
    unsigned start, end, length;

    ImageCodec *codec;
    ImageDecoder decoder;
    ImageData input_data, *input;

    if(!Buffer::HasInstance(args[0])){
        return THROW_INVALID_ARGUMENTS_ERROR(": first argument must be a buffer.");
    }

    img = ObjectWrap::Unwrap<Image>(args.This());

    buffer = (uint8_t *) Buffer::Data(args[0]->ToObject());
    length = Buffer::Length(args[0]->ToObject());

    start = 0;
    if(args[1]->IsNumber()){
        start = args[1]->Uint32Value();
    }

    end = length;
    if(args[2]->IsNumber()){
        end = args[2]->Uint32Value();
        if(end < start || end > length){
            return THROW_INVALID_ARGUMENTS_ERROR("");
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
            NanReturnUndefined();
        }
        codec = codec->next;
    }
    return isError() ? (THROW_GET_ERROR()) : THROW_ERROR("Unknow format");
} // }}}

NAN_METHOD(Image::CopyFromImage) { // {{{
    NanScope();

    Image *src, *dst;
    uint32_t x, y, w, h;

    Local<Object> obj = args[0]->ToObject();

    if(!NanHasInstance(Image::constructor, obj))
        return THROW_INVALID_ARGUMENTS_ERROR("");

    src = ObjectWrap::Unwrap<Image>(obj);
    dst = ObjectWrap::Unwrap<Image>(args.This());

    x = y = 0;
    w = src->pixels->width;
    h = src->pixels->height;

    if(args[1]->IsNumber()   // x
    && args[2]->IsNumber()){ // y
        x = args[1]->Uint32Value();
        y = args[2]->Uint32Value();
    }

    if(args[3]->IsNumber()   // w
    && args[4]->IsNumber()){ // h
        w = args[3]->Uint32Value();
        h = args[4]->Uint32Value();
    }

    if(dst->pixels->CopyFrom(src->pixels, x, y, w, h) != SUCCESS){
        return THROW_GET_ERROR();
    }
    
    NanReturnUndefined();
}// }}}

NAN_METHOD(Image::DrawImage) { // {{{
    NanScope();

    Image *src, *dst;
    size_t x, y;

    Local<Object> obj = args[0]->ToObject();

    if(!NanHasInstance(Image::constructor, obj)
    || !args[1]->IsNumber() // x
    || !args[2]->IsNumber()) // y
        return THROW_INVALID_ARGUMENTS_ERROR("");

    src = ObjectWrap::Unwrap<Image>(obj);
    dst = ObjectWrap::Unwrap<Image>(args.This());

    x = args[1]->Uint32Value();
    y = args[2]->Uint32Value();

    dst->pixels->Draw(src->pixels, x, y);

    NanReturnUndefined();
} // }}}

NAN_METHOD(Image::ToBuffer) { //{{{
    NanScope();

    Image *img;
    ImageType type;
    PixelArray *pixels;
    ImageConfig _config, *config;
    ImageCodec *codec;
    ImageEncoder encoder;

    ImageData output_data, *output;

    Local<Object> buffer;
    int length;

    if(!args[0]->IsNumber())
        return THROW_INVALID_ARGUMENTS_ERROR("");
    type = (ImageType) args[0]->Uint32Value();

    config = NULL;
    if(Buffer::HasInstance(args[1])){
        config = &_config;
        config->data = Buffer::Data(args[1]->ToObject());
        config->length = Buffer::Length(args[1]->ToObject());
    }

    img = ObjectWrap::Unwrap<Image>(args.This());
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
                        buffer = NanNewBufferHandle(length);
                        memcpy(node::Buffer::Data(buffer), output->data, length);
                        free(output->data);
                        NanReturnValue(buffer);
                    }else{
                        if(output->data != NULL)
                            free(output->data);
                        return  THROW_ERROR("Encode fail.");
                    }
                }else{
                    return THROW_ERROR("Can't encode to this format.");
                }
            }
            codec = codec->next;
        }
        return isError() ? (THROW_GET_ERROR()) : (THROW_ERROR("Unsupported type."));
    }else{
        return THROW_ERROR("Image uninitialized.");
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
    NanAdjustExternalMemory(size);
    usedMemory += size;
    //survival++;
} // }}}

Image::~Image(){ // {{{
    int32_t size;
    size = sizeof(PixelArray) + sizeof(Image);
    pixels->Free();
    free(pixels);
    NanAdjustExternalMemory(-size);
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
    NanAdjustExternalMemory(size);
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
        NanAdjustExternalMemory(-size);
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
            return SET_ERROR("Beyond the width limit.");
        }

        if(w == width){
            return SUCCESS;
        }

        size = w * sizeof(size_t);
        if((index = (size_t *) malloc(size)) == NULL){
            return SET_ERROR("Out of memory.");
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
    void NODE_MODULE_EXPORT initialize (Handle<Object> target) { // {{{
        Image::Initialize(target);
    } // }}}
}


NODE_MODULE(binding, initialize);
// vim600: sw=4 ts=4 fdm=marker syn=cpp
