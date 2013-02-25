node-images
===========

Cross-platform image decoder(png/jpeg/gif) and encoder(png) for Nodejs

## Installation
	$ npm install images

## Usage

``` js
var images = require('images'); //npm
// var images = require('./index.js'); //git

var img = new images.Image();
img.loadFromBuffer(fs.readFileSync("test/img/rotating_earth.gif"));
fs.writeFileSync("test/output/gif2png.png", img.toBuffer(images.TYPE_PNG));
```

## Api

### images.Image([width, heigth])

### loadFromBuffer(buffer[, start, [end]]);
See `test/loadFromBuffer.js`

### copyFromImage(x, y, width, height);
See `test/copyFromImage.js`

### fillColor(red, green, blue[, alpha]);
See `test/fillColor.js`

### drawImage(img, x, y);
See `test/drawImage.js`

### toBuffer(type);
See `test/toBuffer.js`

### width
*readonly*

### height
*readonly*

