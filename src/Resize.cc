#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string.h>
#include "Image.h"
#include "resampler.h"



Pixel *get_subpixel( PixelArray *pixels, int x, int y ) {
    return &(pixels->data[ y ][ x ]);
}

void resample(PixelArray *src, PixelArray *dst, const char *filter);

void resize(PixelArray *src, PixelArray *dst, const char *filter = NULL) {

    if ( filter == NULL ) {
        float scale = dst->width / src->width;

        if ( scale >= 0.25 ) {
            resample( src, dst, "bicubic" );
        } else {
            resample( src, dst, "box");
            resample( dst, dst, "bicubic");
        }

    } else {
        resample( src, dst, filter );
    }
}

void resample(PixelArray *src, PixelArray *dst, const char *pFilter) {
    int src_width = src->width, src_height = src->height,
        dst_width = dst->width, dst_height = dst->height;

    // Filter scale - values < 1.0 cause aliasing, but create sharper looking mips.
    const float filter_scale = 1.0f;//.75f;

    // 0 - R
    // 1 - G
    // 2 - B
    // 3 - A
    std::vector<float> samples[4];
    Resampler* resamplers[4];

    // Now create a Resampler instance for each component to process. The first instance will create new contributor tables, which are shared by the resamplers
    // used for the other components (a memory and slight cache efficiency optimization).
    resamplers[0] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, NULL, NULL, filter_scale, filter_scale);
    samples[0].resize(src_width);
    for (int i = 1; i < 4; i++)
    {
      resamplers[i] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, resamplers[0]->get_clist_x(), resamplers[0]->get_clist_y(), filter_scale, filter_scale);
      samples[i].resize(src_width);
    }

    for (int src_y = 0; src_y < src_height; src_y++)
    {
        for (int x = 0; x < src_width; x++)
        {
            samples[0][x] = get_subpixel( src, x, src_y )->R * (1.0f/255.0f);
            samples[1][x] = get_subpixel( src, x, src_y )->G * (1.0f/255.0f);
            samples[2][x] = get_subpixel( src, x, src_y )->B * (1.0f/255.0f);
            samples[3][x] = get_subpixel( src, x, src_y )->A * (1.0f/255.0f);
        }

        for (int c = 0; c < 4; c++)
        {
            if (!resamplers[c]->put_line(&samples[c][0]))
            {
                // printf("Out of memory!\n");
                return;
            }
        }
    }

    for (int dst_y = 0; dst_y < dst_height; dst_y++)
    {
        const float* rOutput_samples = resamplers[0]->get_line();
        const float* gOutput_samples = resamplers[1]->get_line();
        const float* bOutput_samples = resamplers[2]->get_line();
        const float* aOutput_samples = resamplers[3]->get_line();

        if (!rOutput_samples || !gOutput_samples || !bOutput_samples || !aOutput_samples)
           break;

        for (int x = 0; x < dst_width; x++)
        {

            //printf( "%d : %f - %f\n", x, samples[0][x], rOutput_samples[x] );
            int r = (int)(255.0f * rOutput_samples[x] + .5f);

            if (r < 0) r = 0; else if (r> 255) r = 255;
            get_subpixel( dst, x, dst_y )->R = r;

            int g = (int)(255.0f * gOutput_samples[x] + .5f);

            if (g < 0) g = 0; else if (g> 255) g = 255;
            get_subpixel( dst, x, dst_y )->G = g;

            int b = (int)(255.0f * bOutput_samples[x] + .5f);

            if (b < 0) b = 0; else if (b> 255) b = 255;
            get_subpixel( dst, x, dst_y )->B = b;

            int a = (int)(255.0f * aOutput_samples[x] + .5f);

            if (a < 0) a = 0; else if (a> 255) a = 255;
            get_subpixel( dst, x, dst_y )->A = a;

            // printf( "(%d, %d, %d, %d), ", get_subpixel( dst, x, dst_y )->R, get_subpixel( dst, x, dst_y )->G, get_subpixel( dst, x, dst_y )->B, get_subpixel( dst, x, dst_y )->A );
        }

        // printf("\n");
    }

    delete resamplers[0];
    delete resamplers[1];
    delete resamplers[2];
    delete resamplers[3];
}