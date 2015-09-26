#ifndef __NODE_IMAGE_RESIZE__
#define __NODE_IMAGE_RESIZE__

#include "Image.h"



void resize(PixelArray *src, PixelArray *dst, const char *filter = NULL);
void resample(PixelArray *src, PixelArray *dst, const char *filter = NULL);
Pixel *get_subpixel( PixelArray *pixels, int x, int y );

#endif