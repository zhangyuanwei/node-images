/*
 * Image.h
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

#ifndef __NODE_IMAGE_H__
#define __NODE_IMAGE_H__

#include <node_api.h>
#include <stdlib.h>
#include <stdio.h>

typedef enum {
    TYPE_PNG = 1,
    TYPE_JPEG,
    TYPE_GIF,
    TYPE_BMP,
    TYPE_RAW,
    TYPE_WEBP,
} ImageType;

typedef enum {
    FAIL = 0,
    SUCCESS,
} ImageState;

typedef struct Pixel {
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t A;

    void Merge(struct Pixel *pixel);
} Pixel;

typedef enum {
    EMPTY = 0,
    ALPHA,
    SOLID,
} PixelArrayType;

typedef struct PixelArray {
    Pixel **data;
    size_t width;
    size_t height;
    PixelArrayType type;

    int32_t Size() {
        return (height * sizeof(Pixel **)) + (width * height * sizeof(Pixel));
    }

    // Memory
    ImageState Malloc(size_t w, size_t h);

    ImageState CopyFrom(struct PixelArray *src, size_t x, size_t y, size_t w, size_t h);

    void Free();

    // Draw
    void Draw(struct PixelArray *src, size_t x, size_t y);

    void Fill(Pixel *color);

    // Transform
    ImageState SetWidth(size_t w);

    ImageState SetHeight(size_t h);

    ImageState Resize(size_t w, size_t h, const char *filter);
    
    ImageState Rotate(size_t deg);

    void DetectTransparent();
} PixelArray;

typedef struct {
    uint8_t *data;
    unsigned long length;
    unsigned long position;
    //ImageType type;
} ImageData;

typedef struct {
    char *data;
    unsigned long length;
} ImageConfig;

typedef ImageState (*ImageEncoder)(PixelArray *input, ImageData *output, ImageConfig *config);

typedef ImageState (*ImageDecoder)(PixelArray *output, ImageData *input);

typedef struct ImageCodec {
    ImageType type;
    ImageEncoder encoder;
    ImageDecoder decoder;
    struct ImageCodec *next;
} ImageCodec;

#define ENCODER(type) encode ## type
#define ENCODER_FN(type) ImageState ENCODER(type)(PixelArray *input, ImageData *output, ImageConfig *config)
#define DECODER(type) decode ## type
#define DECODER_FN(type) ImageState DECODER(type)(PixelArray *output, ImageData *input)
#define IMAGE_CODEC(type) DECODER_FN(type); ENCODER_FN(type)


#ifdef HAVE_PNG
IMAGE_CODEC(Png);
#endif

#ifdef HAVE_JPEG
IMAGE_CODEC(Jpeg);
#endif

#ifdef HAVE_GIF
IMAGE_CODEC(Gif);
#endif

#ifdef HAVE_BMP
IMAGE_CODEC(Bmp);
#endif

#ifdef HAVE_RAW
IMAGE_CODEC(Raw);
#endif

#ifdef HAVE_WEBP
IMAGE_CODEC(Webp);
#endif

class Image {
    public:

        // static v8::Persistent<v8::Function> constructor;
        static napi_ref constructor;
        // static void Init(v8::Local<v8::Object> exports);
        static napi_value Init(napi_env env, napi_value exports);
        static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);
        static napi_value New(napi_env env, napi_callback_info info);

        // Error Handle
        static void setError(const char *err);

        static napi_value getError(napi_env, napi_callback_info info);

        static bool isError();

        // Size Limit
        static size_t maxWidth, maxHeight;

        static napi_value GetMaxWidth(napi_env env, napi_callback_info info);
        static napi_value SetMaxWidth(napi_env env, napi_callback_info info);

        static napi_value GetMaxHeight(napi_env env, napi_callback_info info);
        static napi_value SetMaxHeight(napi_env env, napi_callback_info info);

        // Memory
        static size_t usedMemory;

        static napi_value GetUsedMemory(napi_env env, napi_callback_info info);
        static napi_value GC(napi_env env, napi_callback_info info);

        // Image constructor
        // static void New(const v8::FunctionCallbackInfo<v8::Value> &args);

        // Image.prototype
        static napi_value GetWidth(napi_env env, napi_callback_info info);
        static napi_value SetWidth(napi_env env, napi_callback_info info);

        static napi_value GetHeight(napi_env env, napi_callback_info info);
        static napi_value SetHeight(napi_env env, napi_callback_info info);

        static napi_value GetTransparent(napi_env env, napi_callback_info info);

        static napi_value Resize(napi_env env, napi_callback_info info);

        static napi_value Rotate(napi_env env, napi_callback_info info);
        
        static napi_value FillColor(napi_env env, napi_callback_info info);

        static napi_value LoadFromBuffer(napi_env env, napi_callback_info info);

        static napi_value ToBuffer(napi_env env, napi_callback_info info);

        static napi_value CopyFromImage(napi_env env, napi_callback_info info);

        static napi_value DrawImage(napi_env env, napi_callback_info info);

    private:
        static const char *error;
        static int errno;

        static ImageCodec *codecs;

        static void regCodec(ImageDecoder decoder, ImageEncoder encoder, ImageType type);

        static void regAllCodecs() {
            codecs = NULL;
#ifdef HAVE_WEBP
            regCodec(DECODER(Webp), ENCODER(Webp), TYPE_WEBP);
#endif
#ifdef HAVE_RAW
            regCodec(DECODER(Raw), ENCODER(Raw), TYPE_RAW);
#endif
#ifdef HAVE_BMP
            regCodec(DECODER(Bmp), ENCODER(Bmp), TYPE_BMP);
#endif
#ifdef HAVE_GIF
            regCodec(DECODER(Gif), ENCODER(Gif), TYPE_GIF);
#endif
#ifdef HAVE_JPEG
            regCodec(DECODER(Jpeg), ENCODER(Jpeg), TYPE_JPEG);
#endif
#ifdef HAVE_PNG
            regCodec(DECODER(Png), ENCODER(Png), TYPE_PNG);
#endif
        }

        PixelArray *pixels;

        Image();

        ~Image();

        // napi env & wrapper
        napi_env env_;
        napi_ref wrapper_;
};

#endif
