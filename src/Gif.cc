//
// Gif.cc
//
// Copyright (c) 2013 ZhangYuanwei <zhangyuanwei1988@gmail.com>

#include "Image.h"



#ifdef HAVE_GIF

#include <stdio.h>
#include <gif_lib.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

int ReadFromMemory (GifFileType *gif, GifByteType *dst, int size){

	ImageData *data;
	int len; 

	if(!gif->UserData) return -1;

	data = (ImageData *) gif->UserData;
	len = data->length - data->position;

	if(size > len) size = len;

	memcpy(dst, data->data + data->position, size);

	data->position += size;
	return size;
}

void DrawImage2PixelArray(PixelArray* dst, GifRowType *rows, ColorMapObject *map, int transparent){
	size_t x, y, w, h;

	w = dst->width;
	h = dst->height;
	Pixel *pixel;
	GifColorType *colors, *entry;
	GifPixelType id;

	colors = map->Colors;

	for(y = 0; y < h; y++){
		for(x = 0; x < w; x++){
			id = rows[y][x];
			pixel = &(dst->data[y][x]);
			if(transparent != -1 && transparent == id){
				continue;
			}
			entry = &(colors[id]);
			pixel->R = entry->Red;
			pixel->G = entry->Green;
			pixel->B = entry->Blue;
			pixel->A = 0xFF;
		}
	}
	dst->DetectTransparent();
}

static int InterlacedOffset[] =  { 0, 4, 2, 1 },
		   InterlacedJumps[] =  { 8, 8, 4, 2 };

DECODER_FN(Gif){
	ImageState ret;

	GifFileType *gif;
	GifRowType  *rows, row;
	GifRecordType type;
	GifImageDesc *img;
	ColorMapObject *map;
	int extcode, transparent;
	GifByteType *extension;

	GifWord width, height, i, x, y, w, h;
	size_t size, count;

	ret = FAIL;

	input->position = 0;
	if((gif = DGifOpen((void *) input, ReadFromMemory)) == NULL) goto RETURN;
	width = gif->SWidth;
	height = gif->SHeight;
	//printf("width:%d,height:%d\n", width, height);

	size = height * sizeof(GifRowType *);
	if((rows = (GifRowType *) malloc(size)) == NULL) goto CLOSE_GIF;
	memset(rows, 0x00, size);

	size = width * sizeof(GifPixelType);
	if ((row = (GifRowType) malloc(size)) == NULL) goto FREE_ROWS;

	if(sizeof(GifPixelType) == 1){
		memset(row, gif->SBackGroundColor, size);
	}else{
		for(i = 0; i < width; i++){
			row[i] = gif->SBackGroundColor;
		}
	}

	rows[0] = row;

	for(i = 1; i < height; i++){
		if((rows[i] = (GifRowType) malloc(size)) == NULL) goto FREE_ROWS;
		memcpy(rows[i], row, size);
	}

	if(output->Malloc(width, height) != SUCCESS) goto FREE_ROWS;
	transparent = -1;

	do{
		if (DGifGetRecordType(gif, &type) == GIF_ERROR) goto FREE_SCREEN;
		//printf("RecordType:%X\n", type);
		switch(type){
			case IMAGE_DESC_RECORD_TYPE:
				if (DGifGetImageDesc(gif) == GIF_ERROR) goto FREE_SCREEN;

				img = &(gif->Image);
				x = img->Left;
				y = img->Top;
				w = img->Width;
				h = img->Height;
				//printf("(%d,%d,%d,%d)\n", x, y, w, h);

				if(x < 0 || y < 0 || x + w > width || y + h > height) goto FREE_SCREEN;

				if(img->Interlace){
					for(count = 0; count < 4; count++)
						for(i = InterlacedOffset[count]; i < h; i += InterlacedJumps[count]){
							if(DGifGetLine(gif, &rows[y+i][x], w) == GIF_ERROR) goto FREE_SCREEN;
						}
				}else{
					for(i = 0; i < h; i++)
						if(DGifGetLine(gif, &rows[y+i][x], w) == GIF_ERROR) goto FREE_SCREEN;
				}

				if((map = img->ColorMap ? img->ColorMap : gif->SColorMap) == NULL) goto FREE_SCREEN;
				DrawImage2PixelArray(output, rows, map, transparent);

				break;
			case EXTENSION_RECORD_TYPE:
				if(DGifGetExtension(gif, &extcode, &extension) == GIF_ERROR) goto FREE_SCREEN;
				//printf("extcode:%X\n", extcode);
				switch(extcode){
					case COMMENT_EXT_FUNC_CODE:
						break;
					case GRAPHICS_EXT_FUNC_CODE:
						if(extension != NULL){
							if((extension[1] & 0x01) == 0x01){
								transparent = extension[4];
							}else{
								transparent = -1;
							}
						}
						break;
					case PLAINTEXT_EXT_FUNC_CODE:
						break;
					case APPLICATION_EXT_FUNC_CODE:
						break;
				}
				while(extension != NULL){
					//int length = extension[0];
					//printf("[");
					//for(int i = 0; i <= length; i++)
					//	printf("0x%02X,", extension[i]);
					//printf("]\n");
					if(DGifGetExtensionNext(gif, &extension) == GIF_ERROR) goto FREE_SCREEN;
				}

				break;
			case TERMINATE_RECORD_TYPE:
				// Do nothing
				break;
			case SCREEN_DESC_RECORD_TYPE:
				// Should not happen
			case UNDEFINED_RECORD_TYPE:
			default:
				goto FREE_ROWS;
				break;
		}
	}while(type != TERMINATE_RECORD_TYPE);

	//printf("Record End!\n");


	ret = SUCCESS;
	goto FREE_ROWS;

FREE_SCREEN:
	output->Free();

FREE_ROWS:
	for(i = 0; i < height; i++){
		if(rows[i] != NULL)
			free(rows[i]);
	}
	free(rows);

CLOSE_GIF:
	DGifCloseFile(gif);

RETURN:
	//if(ret!=SUCCESS)
	//	PrintGifError();
	return ret;
}

ENCODER_FN(Gif){
	//TODO
	return FAIL;
}

#endif
