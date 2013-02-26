var USE_OLD_API = false,
    _images = require("./binding.js"),
    _Image = _images.Image,
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
    fillColor: function(red, green, blue, alpha) {
        this._handle.fillColor(red, green, blue, alpha);
    },
    drawImage: function(img, x, y) {
        if (img instanceof WrappedImage) {
            img = img._handle;
        }
        this._handle.drawImage(img, x, y);
    },
    toBuffer: function(type) {
        //TODO fastBuffer
        return this._handle.toBuffer(type);
    },
    size: function(width, height) {
        //TODO set width and height
        return {
            width: this.width(),
            height: this.height()
        };
    },
    width: function(width) {
        //TODO set width
        return this._handle.width;
    },
    height: function(height) {
        //TODO set height
        return this._handle.height;
    }
};

function bind(target, obj) {
    var item,
    slice = Array.prototype.slice;
    for (item in obj) {
        if (obj.hasOwnProperty(item)) {
            target[item] = (function(fn) {
                return function() {
                    var ret = fn.apply(this, slice.call(arguments, 0));
                    return ret === undefined ? this : ret;
                };
            })(obj[item]);
        }
    }
}

bind(WrappedImage.prototype, prototype);

module.exports = USE_OLD_API ? _images : {
    TYPE_PNG: _images.TYPE_PNG,
    TYPE_JPEG: _images.TYPE_JPEG,
    TYPE_GIF: _images.TYPE_GIF,
    TYPE_BMP: _images.TYPE_BMP,
    TYPE_RAW: _images.TYPE_RAW,

    Image: WrappedImage,

    createImage: function(width, height) {
        return WrappedImage(width, height);
    },

    loadFromBuffer: function(buffer, start, end) {
        return WrappedImage().loadFromBuffer(buffer, start, end);
    },

    copyFromImage: function(src, x, y, width, height) {
        return WrappedImage().copyFromImage(src, x, y, width, height);
    }
};
