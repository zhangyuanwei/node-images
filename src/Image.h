//
// Image.h
//
// Copyright (c) 2013 ZhangYuanwei <zhangyuanwei1988@gmail.com>

#ifndef __NODE_IMAGE_H__
#define __NODE_IMAGE_H__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <node_version.h>

using namespace v8;
using namespace node;

typedef enum {
	TYPE_PNG = 1,
	TYPE_JPEG,
	TYPE_GIF,
	TYPE_BMP,
	TYPE_RAW,
} ImageType;

typedef enum {
	FAIL = 0,
	SUCCESS,
} ImageState;

typedef struct Pixel{
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

typedef struct PixelArray{
	Pixel **data;
	size_t width;
	size_t height;
	PixelArrayType type;
	int32_t Size(){
		return (height * sizeof(Pixel**)) + (width * height * sizeof(Pixel));
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

	void DetectTransparent();
} PixelArray;

typedef struct {
	uint8_t *data;
	size_t length;
	size_t position;
	//ImageType type;
} ImageData;

typedef struct {
	char *data;
	size_t length;
} ImageConfig;

typedef ImageState (* ImageEncoder)(PixelArray *input, ImageData *output, ImageConfig *config);
typedef ImageState (* ImageDecoder)(PixelArray *output, ImageData *input);

typedef struct ImageCodec{
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

class Image: public node::ObjectWrap {
	public:

		static Persistent<FunctionTemplate> constructor;

		static void Initialize(Handle<Object> target);

		// Error Handle
		static ImageState setError(const char * err);
		static Local<Value> getError();
		static bool isError();

		// Size Limit
		static size_t maxWidth, maxHeight;
		static Handle<Value> GetMaxWidth(Local<String> prop, const AccessorInfo &info);
		static void SetMaxWidth(Local<String> prop, Local<Value> value, const AccessorInfo &info);
		static Handle<Value> GetMaxHeight(Local<String> prop, const AccessorInfo &info);
		static void SetMaxHeight(Local<String> prop, Local<Value> value, const AccessorInfo &info);

		// Image constructor
		static Handle<Value> New(const Arguments &args);

		// Image.prototype
		static Handle<Value> GetWidth(Local<String> prop, const AccessorInfo &info);
		static void SetWidth(Local<String> prop, Local<Value> value, const AccessorInfo &info);
		static Handle<Value> GetHeight(Local<String> prop, const AccessorInfo &info);
		static void SetHeight(Local<String> prop, Local<Value> value, const AccessorInfo &info);
		static Handle<Value> GetTransparent(Local<String> prop, const AccessorInfo &info);

		static Handle<Value> FillColor(const Arguments &args);
		static Handle<Value> LoadFromBuffer(const Arguments &args);
		static Handle<Value> CopyFromImage(const Arguments &args);

		static Handle<Value> DrawImage(const Arguments &args);
		static Handle<Value> ToBuffer(const Arguments &args);

	private:	
		static const char *error;
		static int errno;

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
		
		PixelArray *pixels;

		Image();
		~Image();
};

#endif
