var USE_OLD_API = false,
    _images = require("./binding.js"),
    _Image = _images.Image,
    slice = Array.prototype.slice,
    prototype;

function WrappedImage(width, height) {
    if (!(this instanceof WrappedImage)) return new WrappedImage(width, height);
    this._handle = new _Image(width, height);
}

prototype = {
    loadFromBuffer: function(buffer, start, end) {
        this._handle.loadFromBuffer(buffer, start, end);
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
    encode: function(type) {
        //TODO fastBuffer
        return this._handle.toBuffer(type);
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
    } else {
        constructor = images.createImage;
    }
    return constructor.apply(images, slice.call(arguments, 0));
}

images.TYPE_PNG = _images.TYPE_PNG;
//images.TYPE_JPEG = _images.TYPE_JPEG;
//images.TYPE_GIF = _images.TYPE_GIF;
//images.TYPE_BMP = _images.TYPE_BMP;
images.TYPE_RAW = _images.TYPE_RAW;

images.Image = WrappedImage;

images.createImage = function(width, height) {
    return WrappedImage(width, height);
};

images.loadFromBuffer = function(buffer, start, end) {
    return WrappedImage().loadFromBuffer(buffer, start, end);
};

images.copyFromImage = function(src, x, y, width, height) {
    return WrappedImage().copyFromImage(src, x, y, width, height);
};

images.setLimit = function(maxWidth, maxHeight) {
    _images.maxHeight = maxHeight;
    _images.maxWidth = maxWidth;
    return images;
};

module.exports = USE_OLD_API ? _images : images;
