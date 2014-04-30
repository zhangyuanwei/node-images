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
        'sources': ['src/Image.cc', 'src/Resize.cc', 'src/resampler.cpp'],
        'conditions': [
            ['OS=="win"', {}, {}],
            ['with_png=="true"', {
                'defines': ['HAVE_PNG'],
                'sources': ['src/Png.cc'],
                'conditions': [
                    ['OS=="win"', {
                        'libraries': [
                            '-l<(THIRD_PATH)/libpng/projects/vstudio/ReleaseLibrary/libpng15.lib',
                            '-l<(THIRD_PATH)/libpng/projects/vstudio/ReleaseLibrary/zlib.lib'],
                        'include_dirs': [
                            '<(THIRD_PATH)/libpng',
                            '<(THIRD_PATH)/zlib']
                    }, {
                        'libraries': ['-lpng', '-lz', '-lm']
                    }]
                ]
            }],
            ['with_jpeg=="true"', {
                'defines': ['HAVE_JPEG'],
                'sources': ['src/Jpeg.cc'],
                'conditions': [
                    ['OS=="win"', {
                        'libraries': [
							'-l<(THIRD_PATH)/libjpeg-turbo/lib/jpeg-static.lib',
						],
                        'include_dirs': [
							'<(THIRD_PATH)/libjpeg-turbo/include',
						]
                    }, {
                        'libraries': ['-ljpeg']
                    }]
                ]
            }],
            ['with_gif=="true"', {
                'defines': ['HAVE_GIF'],
                'sources': ['src/Gif.cc'],
                'conditions': [
                    ['OS=="win"', {
                        'libraries': ['-l<(THIRD_PATH)\giflib\windows\giflib\Release\giflib.lib'],
                        'include_dirs': [
                            '<(THIRD_PATH)/giflib/lib']
                    }, {
                        'libraries': ['-lgif']
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
