{
    'conditions': [
        ['OS=="win"', {
            'variables': {
                'THIRD_PATH%': 'D:/dev/node/node-images/third',
                'with_png%': 'true',
                'with_jpeg%': 'true',
                'with_gif%': 'true',
                'with_bmp%': 'false',
                'with_raw%': 'true'
            }
        }, {
            'variables': {
                'with_png%': 'true',
                'with_jpeg%': 'true',
                'with_gif%': 'true',
                'with_bmp%': 'false',
                'with_raw%': 'true',
            }
        }]
    ],
    'targets': [{
        'target_name': 'binding',
        'sources': [
            'src/Image.cc',
            'src/Resize.cc',
            'src/resampler.cpp',
            'src/Png.cc',
            'src/Gif.cc',
            'src/Jpeg.cc',
            'src/Raw.cc'
        ],
        "include_dirs" : [
        ],
        'conditions': [
            ['OS=="win"', {}, {}],
            ['with_png=="true"', {
                'defines': ['HAVE_PNG'],
                'sources': ['src/Png.cc'],
                'conditions': [
                    ['OS=="win"', {
                        'dependencies': [
                            './gyp/gyp/libpng.gyp:libpng',
                        ]
                    }, {
                        'dependencies': [
                            './gyp/gyp/libpng.gyp:libpng',
                        ]
                    }]
                ]
            }],
            ['with_jpeg=="true"', {
                'defines': ['HAVE_JPEG'],
                'sources': ['src/Jpeg.cc'],
                'conditions': [
                    ['OS=="win"', {
                        'dependencies': [
                            './gyp/gyp/libjpeg-turbo.gyp:libjpeg-turbo',
                        ]
                    }, {
                        'dependencies': [
                            './gyp/gyp/libjpeg-turbo.gyp:libjpeg-turbo',
                        ]
                    }]
                ]
            }],
            ['with_gif=="true"', {
                'defines': ['HAVE_GIF'],
                'sources': ['src/Gif.cc'],
                'conditions': [
                    ['OS=="win"', {
                        'dependencies': [
                            './gyp/gyp/giflib.gyp:giflib',
                        ]
                    }, {
                        'dependencies': [
                            './gyp/gyp/giflib.gyp:giflib',
                        ]
                    }]
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
