/*
 * Jpeg.cc
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



#ifdef HAVE_JPEG

#include <stdlib.h>
#include <string.h>

#include <setjmp.h>
#include <jpeglib.h>

typedef struct {
	char J;
	char P;
	char E;
	char G;
	uint8_t quality;
} jpeg_compress_config;

jpeg_compress_config default_compress_config = {
	'J','P','E','G',
	100,
};

jpeg_compress_config *get_compress_config(ImageConfig *config){
	if(config == NULL || config->data == NULL 
	|| config->length != sizeof(jpeg_compress_config)
	|| config->data[0] != default_compress_config.J
	|| config->data[1] != default_compress_config.P
	|| config->data[2] != default_compress_config.E
	|| config->data[3] != default_compress_config.G)
			return &default_compress_config;

	return (jpeg_compress_config*) config->data;
}

struct my_jpeg_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

void jpeg_cb_error_exit(j_common_ptr cinfo){
	struct my_jpeg_error_mgr *mptr;
	mptr=(struct my_jpeg_error_mgr*) cinfo->err;
	//(*cinfo->err->output_message) (cinfo);
	longjmp(mptr->setjmp_buffer, 1);
}

DECODER_FN(Jpeg){ // {{{
	struct jpeg_decompress_struct cinfo;
	struct my_jpeg_error_mgr jerr;

	int width, height, line;
	JSAMPROW row_pointer[1];


	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit=jpeg_cb_error_exit;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		output->Free();
		return FAIL;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, (unsigned char *) input->data, input->length);	
	jpeg_read_header(&cinfo, TRUE);
	cinfo.out_color_space = JCS_EXT_RGBA;
	jpeg_start_decompress(&cinfo);

	width = cinfo.output_width;
	height = cinfo.output_height;
	//components = cinfo.output_components;

	if(output->Malloc(width, height) != SUCCESS) 
		longjmp(jerr.setjmp_buffer, 1);

	while((line = cinfo.output_scanline) < height){
		row_pointer[0] = (JSAMPROW) output->data[line];
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
	}
	output->type = SOLID;

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return SUCCESS;
} // }}}

ENCODER_FN(Jpeg){ // {{{
	struct jpeg_compress_struct cinfo;
	struct my_jpeg_error_mgr jerr;
	jpeg_compress_config *conf;

	int width, height, line;
	JSAMPROW row_pointer[1];

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit=jpeg_cb_error_exit;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_compress(&cinfo);
		return FAIL;
	}

	jpeg_create_compress(&cinfo);
	jpeg_mem_dest(&cinfo, (unsigned char **) &(output->data), &output->length);

	width = input->width;
	height = input->height;
	conf = get_compress_config(config);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 4;
	cinfo.in_color_space = JCS_EXT_RGBA;

	jpeg_set_defaults(&cinfo);
	
	jpeg_set_quality(&cinfo, conf->quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	//printf("%d %s\n", cinfo.input_components, cinfo.in_color_space == JCS_EXT_RGBA ? "true" : "false");

	while((line = cinfo.next_scanline) < height){
		row_pointer[0] = (JSAMPROW) input->data[line];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	output->position = output->length;
	return SUCCESS;
} // }}}

#endif
// vim600: sw=4 ts=4 fdm=marker syn=cpp
