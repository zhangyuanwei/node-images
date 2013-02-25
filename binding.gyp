{
    'conditions': [
        ['OS=="win"', {
            'variables': {
                'THIRD_PATH%': 'D:/node-images/third',
                'with_png%': 'true',
                'with_jpeg%': 'true',
                'with_gif%': 'true',
                'with_bmp%': 'false',
                'with_raw%': 'true'
            }
        }, {
            'variables': {
                'with_png%': '<!(./util/has_lib.sh png)',
                'with_jpeg%': '<!(./util/has_lib.sh jpeg)',
                'with_gif%': '<!(./util/has_lib.sh gif)',
                'with_bmp%': 'false',
                'with_raw%': 'true',
            }
        }]
    ],
    'targets': [{
        'target_name': 'images',
        'sources': ['src/Image.cc'],
        'conditions': [
            ['OS=="win"', {}, {}],
            ['with_png=="true"', {
                'defines': ['HAVE_PNG'],
                'sources': ['src/Png.cc'],
                'conditions': [
                    ['OS=="win"', {
                        'libraries': [
                            '-l<(THIRD_PATH)/libpng/projects/visualc71/Win32_LIB_Release/libpng.lib',
                            '-l<(THIRD_PATH)/libpng/projects/visualc71/Win32_LIB_Release/Zlib/zlib.lib'],
                        'include_dirs': [
                            '<(THIRD_PATH)/libpng',
                            '<(THIRD_PATH)/zlib']
                    }, {
                        'libraries': ['-lpng']
                    }]
                ]
            }],
            ['with_jpeg=="true"', {
                'defines': ['HAVE_JPEG'],
                'sources': ['src/Jpeg.cc'],
                'conditions': [
                    ['OS=="win"', {
                        'libraries': [
							'-l<(THIRD_PATH)/libjpeg-turbo/build/Release/jpeg-static.lib',
                            #'-l<(THIRD_PATH)/jpeg/Release/jpeg.lib',
						],
                        'include_dirs': [
							'<(THIRD_PATH)/libjpeg-turbo',
							'<(THIRD_PATH)/libjpeg-turbo/build',
                            #'<(THIRD_PATH)/jpeg',
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
                        'libraries': ['-l<(THIRD_PATH)/giflib/windows/giflib/Release/giflib.lib'],
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
