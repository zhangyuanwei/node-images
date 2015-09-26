//
// Png.cc
//
// Copyright (c) 2013 ZhangYuanwei <zhangyuanwei1988@gmail.com>

#include "Image.h"


#ifdef HAVE_PNG

#include <png.h>
#include <stdlib.h>
#include <string.h>

#define testType(type, test) if(type == test) printf("color_type:"#test"\n")
#define echoType(type) do{ \
	testType(type, PNG_COLOR_TYPE_GRAY); \
	testType(type, PNG_COLOR_TYPE_PALETTE); \
	testType(type, PNG_COLOR_TYPE_RGB); \
	testType(type, PNG_COLOR_TYPE_RGB_ALPHA); \
	testType(type, PNG_COLOR_TYPE_GRAY_ALPHA); \
}while(0)

void read_from_memory(png_structp png_ptr, png_bytep buffer, png_size_t size){ // {{{
    ImageData *img;
    size_t len;

    img = (ImageData *) png_get_io_ptr(png_ptr);
    if(img == NULL || img->data == NULL){
        png_error(png_ptr, "No data");
        return;
    }

    len = img->length - img->position;

    if(size > len){
        png_error(png_ptr, "No enough data");
        return;
    }

    memcpy(buffer, &(img->data[img->position]), size);
    img->position += size;
} // }}}

#define PNG_INIT_FILE_SIZE 1024
#define PNG_BIG_FILE_SIZE 10240
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

void write_to_memory(png_structp png_ptr, png_bytep buffer, png_size_t size){ // {{{
    ImageData *img;
    size_t len;

    img = (ImageData *) png_get_io_ptr(png_ptr);
    if(img == NULL){
        png_error(png_ptr, "What happend?!");
        return;
    }

    if(img->data == NULL){
        len = PNG_INIT_FILE_SIZE + size;
        if((img->data = (uint8_t *) malloc(len)) == NULL){
            png_error(png_ptr, "Out of memory.");
            return;
        }
        img->length = len;
        img->position = 0;
    }else if((len = img->position + size) > img->length){
        len = (len < PNG_BIG_FILE_SIZE) ? (len << 1) :(len + PNG_INIT_FILE_SIZE);
        if((img->data = (uint8_t *) realloc(img->data, len)) == NULL){
            png_error(png_ptr, "Out of memory.");
            return;
        }
        img->length = len;
    }

    memcpy(&(img->data[img->position]), buffer, size);
    img->position += size;
} // }}}

void flush_memory(png_structp png_ptr){ // {{{
    //DO Nothing
} // }}}

#define PNG_BYTES_TO_CHECK 4

DECODER_FN(Png){ // {{{
    png_structp png_ptr;
    png_infop info_ptr;

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;

    if(input->length < PNG_BYTES_TO_CHECK) return FAIL;
    if(png_sig_cmp(input->data, 0, PNG_BYTES_TO_CHECK)) return FAIL;
    if((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL) return FAIL;

    if((info_ptr = png_create_info_struct(png_ptr)) == NULL){
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return FAIL;
    }

    if (setjmp(png_jmpbuf(png_ptr))){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return FAIL;
    }

    png_set_read_fn(png_ptr, (void *) input, read_from_memory);

    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                 &interlace_type, NULL, NULL);


    if (color_type == PNG_COLOR_TYPE_PALETTE){
        png_set_expand(png_ptr);
    }

    //echoType(color_type);

    if(color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type != PNG_COLOR_TYPE_GRAY_ALPHA){
        png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type== PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    if (bit_depth < 8)
        png_set_packing(png_ptr);

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)){
        png_set_tRNS_to_alpha(png_ptr);
    }

    if(bit_depth == 16)
        png_set_strip_16(png_ptr);

    png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr,info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

    if(png_get_rowbytes(png_ptr,info_ptr) != width * sizeof(Pixel)){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return FAIL;
    }

    if(output->Malloc(width, height) != SUCCESS){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return FAIL;
    }
    //output->Malloc(width, height);
    png_read_image(png_ptr, (png_bytepp) output->data);
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    output->DetectTransparent();
    return SUCCESS;
} // }}}

ENCODER_FN(Png){ // {{{
    png_structp png_ptr;
    png_infop info_ptr;

    if((png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL) return FAIL;
    if((info_ptr = png_create_info_struct(png_ptr)) == NULL){
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return FAIL;
    }

    if (setjmp(png_jmpbuf(png_ptr))){
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return FAIL;
    }

    output->data = NULL;
    png_set_write_fn(png_ptr, (void *) output, write_to_memory, flush_memory);

    //printf("%d\n", info_ptr->width);

    png_set_IHDR(png_ptr, info_ptr, input->width, input->height, 8,
                 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, (png_bytepp) input->data);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return SUCCESS;
} // }}}

#endif
// vim600: sw=4 ts=4 fdm=marker syn=cpp
