//
// Raw.cc
//
// Copyright (c) 2013 ZhangYuanwei <zhangyuanwei1988@gmail.com>

#include "Image.h"



#ifdef HAVE_RAW

#include <stdlib.h>

#define RAW_HEADER_SIZE 12

DECODER_FN(Raw){ // {{{
	uint32_t  width, height, x, y;
	Pixel *sp, *dp;

	if(input->length < RAW_HEADER_SIZE)
		return FAIL;

	if(	input->data[0] != 'R' ||
			input->data[1] != 'A' ||
			input->data[2] != 'W' ||
			input->data[3] != '\n' )
		return FAIL;

	width = input->data[4] << 24
		| input->data[5] << 16
		| input->data[6] << 8
		| input->data[7] << 0;

	height = input->data[8] << 24
		| input->data[9] << 16
		| input->data[10] << 8
		| input->data[11] << 0;

	if(input->length != RAW_HEADER_SIZE + sizeof(Pixel) * width * height)
		return FAIL;

	if(output->Malloc(width, height) != SUCCESS)
		return FAIL;

	sp = (Pixel *) (input->data + RAW_HEADER_SIZE);

	for(y = 0; y < height; y++){
		for(x = 0; x < width; x++){
			dp = &(output->data[y][x]);
			dp->R = sp->B;
			dp->G = sp->G;
			dp->B = sp->R;
			dp->A = 0xFF;
			sp++;
		}
	}
	output->type = SOLID;
	return SUCCESS;
} // }}}

ENCODER_FN(Raw){ // {{{
	uint32_t width, height, x, y;
	size_t length;
	Pixel *sp, *dp;
	uint8_t *data;

	width = input->width;
	height = input->height;
	length = RAW_HEADER_SIZE + width * height * 4;
	data = (uint8_t*) malloc(length);

	output->data = data;
	output->length = output->position = length;

	if(!data) return FAIL;

	data[0] = 'R';
	data[1] = 'A';
	data[2] = 'W';
	data[3] = '\n';

	data[4]  = (width >> 24)  & 0xff;
	data[5]  = (width >> 16)  & 0xff;
	data[6]  = (width >> 8)   & 0xff;
	data[7]  = (width >> 0)   & 0xff;

	data[8]  = (height >> 24) & 0xff;
	data[9]  = (height >> 16) & 0xff;
	data[10] = (height >> 8)  & 0xff;
	data[11] = (height >> 0)  & 0xff;

	dp = (Pixel *)(data + RAW_HEADER_SIZE);

	for(y = 0; y < height; y++){
		for(x = 0; x < width; x++){
			sp = &(input->data[y][x]);
			dp->R = sp->B;
			dp->G = sp->G;
			dp->B = sp->R;
			dp->A = 0x00;
			dp++;
		}
	}

	return SUCCESS;
} // }}}

#endif

// vim600: sw=4 ts=4 fdm=marker syn=cpp
