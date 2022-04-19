/*
 * Index.js
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

var USE_OLD_API = false,
    fs = require("fs"),
    path = require("path"),
    _images = require("./scripts/binding.js")(),
    _Image = _images.Image,
    slice = Array.prototype.slice,
    FILE_TYPE_MAP,
    CONFIG_GENERATOR,
    prototype,
    nextGCThreshold = 0,
    gcThreshold = 0;

function WrappedImage(width, height) {
    if (!(this instanceof WrappedImage)) return new WrappedImage(width, height);
    if (gcThreshold && nextGCThreshold) {
        if (images.getUsedMemory() > nextGCThreshold) {
            images.gc();
            nextGCThreshold = images.getUsedMemory() + gcThreshold;
        }
    }
    this._handle = new _Image(width, height);
}

prototype = {
    loadFromBuffer: function(buffer, start, end) {
        start = start || 0;
        end = end || buffer.length;
        this._handle.loadFromBuffer(buffer, start, end);
    },
    loadFromFile: function(path) {
        this._handle.loadFromFile(path);
    },
    copyFromImage: function(img, x, y, width, height) {
        if (img instanceof WrappedImage) {
            img = img._handle;
        }
        this._handle.copyFromImage(img, x, y, width, height);
    },
    fill: function(red, green, blue, alpha) {
        this._handle.fillColor(red, green, blue, alpha);
    },
    draw: function(img, x, y) {
        if (img instanceof WrappedImage) {
            img = img._handle;
        }
        this._handle.drawImage(img, x, y);
    },
    encode: function(type, config) {
        var configurator;
        if (typeof(type) != "number") {
            type = String(type).toLowerCase();
            type = (FILE_TYPE_MAP["." + type] || FILE_TYPE_MAP[type]);
        }
        if (config != undefined) {
            configurator = CONFIG_GENERATOR[type];
            config = configurator && configurator(config);
        }
        return this._handle.toBuffer(type, config);
    },
    save: function(file, type, config) {
        if (type && typeof(type) == "object") {
            config = type;
            type = undefined;
        }

        fs.writeFileSync(file, this.encode(type || path.extname(file), config));
    },
    saveAsync: function (file, type, config, callback) {
        if (type && typeof(type) === 'object') {
            config = type;
            type = undefined;
        }
        if (!callback) {
            if (typeof type === 'function') {
                callback = type;
                type = undefined;
            }
            if (typeof config === 'function') {
                callback = config;
                config = undefined;
            }
        }
        fs.writeFile(file, this.encode(type || path.extname(file), config), callback);
        return this;
    },
    resize: function(width, height, filter) {
        this._handle.resize(width, height, filter);
        return this;
    },
    rotate: function(deg) {
        this._handle.rotate(deg);
        return this;
    },

    size: function(width, height) {
        var size;
        if (width === undefined) return {
            width: this.width(),
            height: this.height()
        };

        if (height === undefined) {
            size = this.size();
            height = width * size.height / size.width;
        }
        this.width(width).height(height);
    },
    width: function(width) {
        if (width === undefined) return this._handle.width;
        this._handle.width = width;
    },
    height: function(height) {
        if (height === undefined) return this._handle.height;
        this._handle.height = height;
    }
};

function bind(target, obj, aliases) {
    var item;
    for (item in obj) {
        if (obj.hasOwnProperty(item)) {
            target[item] = (function(fn) {
                return function() {
                    var ret = fn.apply(this, slice.call(arguments, 0));
                    return ret === undefined ? this : ret;
                };
            })(obj[item]);
            if (aliases.hasOwnProperty(item)) {
                aliases[item].forEach(function(alias) {
                    target[alias] = target[item];
                });
            }
        }
    }
}

bind(WrappedImage.prototype, prototype, {
    "fill": ["fillColor"],
    "encode": ["toBuffer"],
    "draw": ["drawImage"]
});

function images(obj) {
    var constructor;
    if (obj instanceof Buffer) {
        constructor = images.loadFromBuffer;
    } else if (obj instanceof WrappedImage) {
        constructor = images.copyFromImage;
    } else if (typeof(obj) == "string") {
        constructor = images.loadFromFile;
    } else {
        constructor = images.createImage;
    }
    return constructor.apply(images, slice.call(arguments, 0));
}

images.TYPE_PNG = _images.TYPE_PNG;
images.TYPE_JPEG = _images.TYPE_JPEG;
images.TYPE_GIF = _images.TYPE_GIF;
images.TYPE_BMP = _images.TYPE_BMP;
images.TYPE_RAW = _images.TYPE_RAW;
images.TYPE_WEBP = _images.TYPE_WEBP;

FILE_TYPE_MAP = {
    ".png": images.TYPE_PNG,
    ".jpg": images.TYPE_JPEG,
    ".jpeg": images.TYPE_JPEG,
    ".gif": images.TYPE_GIF,
    ".bmp": images.TYPE_BMP,
    ".raw": images.TYPE_RAW,
    ".webp": images.TYPE_WEBP
};

CONFIG_GENERATOR = [];
//CONFIG_GENERATOR[images.TYPE_PNG]
CONFIG_GENERATOR[images.TYPE_JPEG] = function(config) {
    var JPEG_CONFIG_SIZE = 5,
        ret = new Buffer(JPEG_CONFIG_SIZE);

    ret.write("JPEG", 0, 4, "ascii");
    ret[4] = config.quality === undefined ? 100 : config.quality;
    return ret;
};

images.Image = WrappedImage;

images.loadFromFile = function(file) {
    return images.loadFromBuffer(fs.readFileSync(file));
};

images.loadFromFileAsync = function(file) {
    return new Promise((resolve, reject) => {
        fs.readFile(file, (err, data) => {
            if (err) {
                reject(err)

                return
            }

            resolve(images.loadFromBuffer(data))
        })
    })
}

images.createImage = function(width, height) {
    return WrappedImage(width, height);
};

images.loadFromBuffer = function(buffer, start, end) {
    return WrappedImage().loadFromBuffer(buffer, start, end);
};

images.loadFromFileNew = function(path) {
    return WrappedImage().loadFromFile(path);
}

images.copyFromImage = function(src, x, y, width, height) {
    return WrappedImage().copyFromImage(src, x, y, width, height);
};

images.setLimit = function(maxWidth, maxHeight) {
    _images.maxHeight = maxHeight;
    _images.maxWidth = maxWidth;
    return images;
};

images.setGCThreshold = function(value) {
    gcThreshold = value;
    nextGCThreshold = value;
};

images.getUsedMemory = function() {
    return _images.usedMemory;
};

images.gc = function() {
    return _images.gc();
};

module.exports = USE_OLD_API ? _images : images;
