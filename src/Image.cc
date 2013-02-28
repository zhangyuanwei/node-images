//
// Image.cc
//
// Copyright (c) 2013 ZhangYuanwei <zhangyuanwei1988@gmail.com>

#include "Image.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <node_buffer.h>


//#define SET_ERROR_FILE_LINE(file, line, msg) Image::SetError( file #line msg)
//#define SET_ERROR(msg) SET_ERROR_FILE_LINE(__FILE__, __LINE__, meg)

#define STRINGFY(n) #n
#define MERGE_FILE_LINE(file, line, msg) ( file ":" STRINGFY(line) " " msg)
#define FILE_LINE(msg) MERGE_FILE_LINE(__FILE__, __LINE__, msg)
#define ERROR(type, msg) Exception::type##Error(String::New(msg))
#define THROW(err) ThrowException(err)

#define SET_ERROR(msg) (Image::setError(FILE_LINE(msg)))
#define GET_ERROR() (Image::getError())
#define THROW_ERROR(msg) THROW(ERROR(,FILE_LINE(msg)))
#define THROW_GET_ERROR() THROW(GET_ERROR())

#define THROW_TYPE_ERROR(msg) THROW(ERROR(Type, FILE_LINE(msg)))
#define THROW_INVALID_ARGUMENTS_ERROR(msg) THROW_TYPE_ERROR("Invalid arguments" msg)

#define DEFAULT_WIDTH_LIMIT  10240 // default limit 10000x10000
#define DEFAULT_HEIGHT_LIMIT 10240 // default limit 10000x10000

Persistent<FunctionTemplate> Image::constructor;
//size_t Image::survival;
ImageCodec *Image::codecs;

size_t Image::maxWidth = DEFAULT_WIDTH_LIMIT;
size_t Image::maxHeight = DEFAULT_HEIGHT_LIMIT;
const char *Image::error = NULL;

void Image::Initialize(Handle<Object> target){ // {{{
	HandleScope scope;

	regAllCodecs();

	//survival = 0;

	// Constructor
	constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Image::New));
	constructor->InstanceTemplate()->SetInternalFieldCount(1);
	constructor->SetClassName(String::NewSymbol("Image"));

	// Prototype
	Local<ObjectTemplate> proto = constructor->PrototypeTemplate();
	NODE_SET_PROTOTYPE_METHOD(constructor, "fillColor", FillColor);
	NODE_SET_PROTOTYPE_METHOD(constructor, "loadFromBuffer", LoadFromBuffer);
	NODE_SET_PROTOTYPE_METHOD(constructor, "copyFromImage", CopyFromImage);
	NODE_SET_PROTOTYPE_METHOD(constructor, "drawImage", DrawImage);
	NODE_SET_PROTOTYPE_METHOD(constructor, "toBuffer", ToBuffer);

	proto->SetAccessor(String::NewSymbol("width"), GetWidth, SetWidth);
	proto->SetAccessor(String::NewSymbol("height"), GetHeight, SetHeight);
	proto->SetAccessor(String::NewSymbol("transparent"), GetTransparent);

	NODE_DEFINE_CONSTANT(target, TYPE_PNG);
	NODE_DEFINE_CONSTANT(target, TYPE_JPEG);
	NODE_DEFINE_CONSTANT(target, TYPE_GIF);
	NODE_DEFINE_CONSTANT(target, TYPE_BMP);
	NODE_DEFINE_CONSTANT(target, TYPE_RAW);

	target->SetAccessor(String::NewSymbol("maxWidth"), GetMaxWidth, SetMaxWidth);
	target->SetAccessor(String::NewSymbol("maxHeight"), GetMaxHeight, SetMaxHeight);

	target->Set(String::NewSymbol("Image"), constructor->GetFunction());
} //}}}

ImageState Image::setError(const char * err){ // {{{
	error = err;
	return FAIL;
} // }}}

Local<Value> Image::getError(){ // {{{
	Local<Value> err = Exception::Error(String::New(error ? error : "Unknow Error"));
	error = NULL;
	return err;
} // }}}

bool Image::isError(){ // {{{
	return error != NULL;
} // }}}

Handle<Value> Image::GetMaxWidth(Local<String> prop, const AccessorInfo &info){ // {{{
	HandleScope scope;
	return scope.Close(Number::New(maxWidth));
} // }}}

void Image::SetMaxWidth(Local<String> prop, Local<Value> value, const AccessorInfo &info){ // {{{
	if(value->IsNumber())
		maxWidth = value->Uint32Value();
} // }}}

Handle<Value> Image::GetMaxHeight(Local<String> prop, const AccessorInfo &info){ // {{{
	HandleScope scope;
	return scope.Close(Number::New(maxHeight));
} // }}}

void Image::SetMaxHeight(Local<String> prop, Local<Value> value, const AccessorInfo &info){ // {{{
	if(value->IsNumber())
		maxHeight = value->Uint32Value();
} // }}}

Handle<Value> Image::New(const Arguments &args){ // {{{
	HandleScope scope;
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
	return args.This();
} // }}} 

Handle<Value> Image::GetWidth(Local<String> prop, const AccessorInfo &info){ // {{{
	HandleScope scope;
	Image *img = ObjectWrap::Unwrap<Image>(info.This());
	return scope.Close(Number::New(img->pixels->width));
} // }}}

void Image::SetWidth(Local<String> prop, Local<Value> value, const AccessorInfo &info){ // {{{
	if(value->IsNumber()){
		Image *img = ObjectWrap::Unwrap<Image>(info.This());
		img->pixels->SetWidth(value->Uint32Value());
	}
} // }}}

Handle<Value> Image::GetHeight(Local<String> prop, const AccessorInfo &info){ // {{{
	HandleScope scope;
	Image *img = ObjectWrap::Unwrap<Image>(info.This());
	return scope.Close(Number::New(img->pixels->height));
} // }}}

void Image::SetHeight(Local<String> prop, Local<Value> value, const AccessorInfo &info){ // {{{
	if(value->IsNumber()){
		Image *img = ObjectWrap::Unwrap<Image>(info.This());
		img->pixels->SetHeight(value->Uint32Value());
	}
} // }}}

Handle<Value> Image::GetTransparent(Local<String> prop, const AccessorInfo &info){ // {{{
	HandleScope scope;
	Image *img = ObjectWrap::Unwrap<Image>(info.This());
	return scope.Close(Number::New(img->pixels->type));
} // }}}

Handle<Value> Image::FillColor(const Arguments &args){ // {{{
	HandleScope scope;
	Image *img;
	Pixel color, *cp;

	if(!args[0]->IsNumber()
	|| !args[1]->IsNumber()
	|| !args[2]->IsNumber())
		return THROW_INVALID_ARGUMENTS_ERROR();

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

	return scope.Close(Undefined());
} // }}}

Handle<Value> Image::LoadFromBuffer(const Arguments &args){ // {{{
	HandleScope scope;
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
			return THROW_INVALID_ARGUMENTS_ERROR();
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
			return scope.Close(Undefined());
		}
		codec = codec->next;
	}
	return isError() ? (THROW_GET_ERROR()) : THROW_ERROR("Unknow format");
} // }}}

Handle<Value> Image::CopyFromImage(const Arguments &args){ // {{{
	HandleScope scope;
	Image *src, *dst;
	uint32_t x, y, w, h;

	Local<Object> obj = args[0]->ToObject();

	if(!Image::constructor->HasInstance(obj))
		return THROW_INVALID_ARGUMENTS_ERROR();

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

	return scope.Close(Undefined());
}// }}}

Handle<Value> Image::DrawImage(const Arguments &args) { // {{{
	HandleScope scope;

	Image *src, *dst;
	size_t x, y;

	Local<Object> obj = args[0]->ToObject();

	if(!Image::constructor->HasInstance(obj)
	|| !args[1]->IsNumber() // x
	|| !args[2]->IsNumber()) // y
		return THROW_INVALID_ARGUMENTS_ERROR();

	src = ObjectWrap::Unwrap<Image>(obj);
	dst = ObjectWrap::Unwrap<Image>(args.This());

	x = args[1]->Uint32Value();
	y = args[2]->Uint32Value();

	dst->pixels->Draw(src->pixels, x, y);
	return scope.Close(Undefined());
} // }}}

Handle<Value> Image::ToBuffer(const Arguments &args){ //{{{
	HandleScope scope;

	Image *img;
	ImageType type;
	PixelArray *pixels;
	ImageConfig _config, *config;
	ImageCodec *codec;
	ImageEncoder encoder;

	ImageData output_data, *output;

	Buffer *buffer;
	int length;

	if(!args[0]->IsNumber())
		return THROW_INVALID_ARGUMENTS_ERROR();
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
						buffer = Buffer::New(length);
						memcpy(Buffer::Data(buffer), output->data, length);
						free(output->data);
						return scope.Close(buffer->handle_);
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
	pixels = (PixelArray *) malloc(sizeof(PixelArray));
	pixels->width = pixels->height = 0;
	pixels->type = EMPTY;
	pixels->data = NULL;
	V8::AdjustAmountOfExternalAllocatedMemory(sizeof(PixelArray) + sizeof(Image));
	//survival++;
} // }}}

Image::~Image(){ // {{{
	int32_t size;
	size = sizeof(PixelArray) + sizeof(Image);
	pixels->Free();
	free(pixels);
	V8::AdjustAmountOfExternalAllocatedMemory(-size);
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
	V8::AdjustAmountOfExternalAllocatedMemory(Size());
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
	size_t h;

	if(data != NULL){
		h = height;
		while(h--){
			if(data[h] != NULL) free(data[h]);
		}
		free(data);
		V8::AdjustAmountOfExternalAllocatedMemory(-Size());
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

NODE_MODULE(images, initialize);
// vim600: sw=4 ts=4 fdm=marker syn=cpp
