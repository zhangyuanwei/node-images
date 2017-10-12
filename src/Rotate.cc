/*
 * Rotate.cc
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
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <math.h>
#include "Image.h"
#define PI 3.141592 

int max(int a,int b)
{
    return a>b?a:b;
}

ImageState rotate(PixelArray *src, PixelArray *dst, const size_t deg = 0) {

    float rad = deg * ( PI / 180 );  
    float cos_rad = cos(rad);
    float sin_rad = sin(rad);

    int w = src->width;  
    int h = src->height;  
  
    int src_x1 =- w/2;  
    int src_y1 =  h/2;  
    int src_x2 =  w/2;  
    int src_y2 =  h/2;  
    int src_x3 =  w/2;  
    int src_y3 =- h/2;  
    int src_x4 =- w/2;  
    int src_y4 =- h/2;  
  
    int dst_x1 = (int)(( cos_rad * src_x1 + sin_rad * src_y1 ) + 0.5);  
    int dst_y1 = (int)((-sin_rad * src_x1 + cos_rad * src_y1 ) + 0.5);  
    int dst_x2 = (int)(( cos_rad * src_x2 + sin_rad * src_y2 ) + 0.5);  
    int dst_y2 = (int)((-sin_rad * src_x2 + cos_rad * src_y2 ) + 0.5);  
    int dst_x3 = (int)(( cos_rad * src_x3 + sin_rad * src_y3 ) + 0.5);  
    int dst_y3 = (int)((-sin_rad * src_x3 + cos_rad * src_y3 ) + 0.5);  
    int dst_x4 = (int)(( cos_rad * src_x4 + sin_rad * src_y4 ) + 0.5);  
    int dst_y4 = (int)((-sin_rad * src_x4 + cos_rad * src_y4 ) + 0.5);  
  
    int dst_width = max(abs(dst_x1 - dst_x3),abs(dst_x2 - dst_x4)) + 1;  
    int dst_height= max(abs(dst_y1 - dst_y3),abs(dst_y2 - dst_y4)) + 1;  
  
    if(dst->Malloc(dst_width, dst_height) != SUCCESS){
        return FAIL;
    }
    
    float var_x = (float)(-dst_width * cos_rad / 2.0f - dst_height * sin_rad / 2.0f + w / 2.0f);  
    float var_y = (float)( dst_width * sin_rad / 2.0f - dst_height * cos_rad / 2.0f + h / 2.0f);  
  
    for( int i = 0;i < dst_height; i++ ) {
        // i,j为现在的图的坐标  
        float sin_rad_i = sin_rad * i + var_x;
        float cos_rad_i = cos_rad * i + var_y;
        for( int j=0;j < dst_width; j++) {  
            int x = (int)( cos_rad * j + sin_rad_i); //x，y为原来图中的像素坐标  
            int y = (int)(-sin_rad * j + cos_rad_i);  
            if( x >= w || x < 0 || y >= h || y < 0 ) {  
                dst->data[i][j].R = 255;
                dst->data[i][j].G = 255;
                dst->data[i][j].B = 255;
                dst->data[i][j].A = 0;
            }  
            else {  
                dst->data[i][j] = src->data[y][x];
            }  
        }  
    }
    return SUCCESS;
}
