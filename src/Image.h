
//
// Image.h
//
// Copyright (c) 2010 LearnBoost <tj@learnboost.com>
//

#ifndef __NODE_IMAGE_H__
#define __NODE_IMAGE_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <node_version.h>

using namespace v8;
using namespace node;

typedef enum {
	TYPE_PNG,
	TYPE_JPEG,
	TYPE_GIF,
	TYPE_BMP,
	TYPE_RAW,
} ImageType;

typedef enum {
	FAIL,
	SUCCESS,
} ImageState;

typedef struct Pixel{
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t A;
	void Merge(struct Pixel *pixel);
} Pixel;

typedef struct PixelArray{
	Pixel **data;
	size_t width;
	size_t height;
	bool alpha;
	int32_t Size(){
		return (height * sizeof(Pixel**)) + (width * height * sizeof(Pixel));
	}
	ImageState Malloc(size_t w, size_t h);
	void Free();
	ImageState CopyFrom(struct PixelArray *src, size_t x, size_t y, size_t w, size_t h);
	ImageState Draw(struct PixelArray *src, size_t x, size_t y);
	void Fill(Pixel *color);
	void DetectTransparent();
} PixelArray;

typedef struct {
	uint8_t *data;
	size_t length;
	size_t position;
	//ImageType type;
} ImageData;

typedef ImageState (* ImageEncoder)(PixelArray *input, ImageData *output);
typedef ImageState (* ImageDecoder)(PixelArray *output, ImageData *input);

typedef struct ImageCodec{
	ImageType type;
	ImageEncoder encoder;
	ImageDecoder decoder;
	struct ImageCodec *next;
} ImageCodec;

#define ENCODER(type) encode ## type
#define ENCODER_FN(type) ImageState ENCODER(type)(PixelArray *input, ImageData *output)
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

class Image: public node::ObjectWrap {
	public:
		//static size_t survival;
		static Persistent<FunctionTemplate> constructor;

		static void Initialize(Handle<Object> target);
		static Handle<Value> New(const Arguments &args);

		static Handle<Value> GetWidth(Local<String> prop, const AccessorInfo &info);
		static Handle<Value> GetHeight(Local<String> prop, const AccessorInfo &info);
		static Handle<Value> GetTransparent(Local<String> prop, const AccessorInfo &info);

		static Handle<Value> FillColor(const Arguments &args);
		static Handle<Value> LoadFromBuffer(const Arguments &args);
		static Handle<Value> CopyFromImage(const Arguments &args);

		static Handle<Value> DrawImage(const Arguments &args);

		static Handle<Value> ToBuffer(const Arguments &args);

	private:
		PixelArray *pixels;

		static ImageCodec *codecs;
		static void regCodec(ImageDecoder decoder, ImageEncoder encoder, ImageType type);
		static void regAllCodecs(){
			codecs = NULL;
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

		Image();
		~Image();
};

#endif
