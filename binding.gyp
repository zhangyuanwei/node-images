{
     'variables': {
         'with_png%':  'true',
         'with_jpeg%': 'true',
         'with_gif%':  'true',
         'with_webp%': 'true',
         'with_bmp%':  'false',
         'with_raw%':  'true',
     },
    'targets': [{
        'target_name': 'binding',
        'sources': [
            'src/Image.cc',
            'src/Resize.cc',
            'src/Rotate.cc',
            'src/resampler.cpp'
        ],
        "include_dirs" : [
        ],
        'conditions': [
            ['with_png=="true"', {
                'defines': ['HAVE_PNG'],
                'sources': ['src/Png.cc'],
                'dependencies': [
                    'gyp/gyp/libpng.gyp:libpng',
                ]
            }],
            ['with_jpeg=="true"', {
                'defines': ['HAVE_JPEG'],
                'sources': ['src/Jpeg.cc'],
                'dependencies': [
                    'gyp/gyp/libjpeg-turbo.gyp:libjpeg-turbo',
                ]
            }],
            ['with_gif=="true"', {
                'defines': ['HAVE_GIF'],
                'sources': ['src/Gif.cc'],
                'dependencies': [
                    'gyp/gyp/giflib.gyp:giflib',
                ]
            }],
            ['with_webp=="false"', {
                'defines': ['HAVE_WEBP'],
                'sources': ['src/Webp.cc'],
                'dependencies': [
                    'gyp/gyp/libwebp.gyp:libwebp',
                ]
            }],
            ['with_bmp=="true"', {
                'defines': ['HAVE_BMP'],
                'sources': ['src/Bmp.cc']
            }],
            ['with_raw=="true"', {
                'defines': ['HAVE_RAW'],
                'sources': ['src/Raw.cc']
            }]
        ]
    }]
}
