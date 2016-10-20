/*
 * Webp.cc
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
#include "webp/decode.h"
#include "webp/encode.h"

#include <stdlib.h>
#include <string.h>
//int WebPGetInfo(const uint8_t* data, size_t data_size, int* width, int* height);

#ifdef HAVE_WEBP

#include <stdlib.h>

DECODER_FN(Webp){ // {{{
    int width;
    int height;
    int line;
    int size;
    uint8_t *rgba;

    rgba = WebPDecodeRGBA(input->data, input->length, &width, &height);
    
    if (rgba == NULL){
        return FAIL;
    }

    if(output->Malloc(width, height) != SUCCESS){
        WebPFree(rgba);
        return FAIL;
    }

    size = width * sizeof(Pixel);
    for(line = 0; line < height; line++){
        memcpy(output->data[line], &(rgba[size * line]), size);
    }
    WebPFree(rgba);
    output->DetectTransparent();
    return SUCCESS;
} // }}}

ENCODER_FN(Webp){ // {{{
    uint8_t *rgba;
    uint8_t *dist;
    int width;
    int height;
    int line;
    int line_size;
    size_t size;
    uint8_t *buffer;

    width = input->width;
    height = input->height;
    line_size = width * sizeof(Pixel);
    rgba = (uint8_t *) malloc(height * line_size);

    if(rgba == NULL){
        return FAIL;
    }

    dist = rgba;
    for(line = 0;line < height; line++){
        memcpy(dist, input->data[line], line_size);
        dist += line_size;
    }

    size = WebPEncodeLosslessRGBA(rgba, width, height, line_size, &buffer);
    if(size != 0){
        output->data = (uint8_t *) malloc(size);
        output->length = size;
        output->position = size;
        memcpy(output->data, buffer, size);
    }
    WebPFree(buffer);

    return SUCCESS;
} // }}}

#endif

// vim600: sw=4 ts=4 fdm=marker syn=cpp
