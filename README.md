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
img.loadFromBuffer(fs.readFileSync("demo/img/rotating_earth.gif"));
fs.writeFileSync("demo/output/gif2png.png", img.toBuffer(images.TYPE_PNG));
```

## API

### new images.Image([width, heigth])
Create a new transparent image

### loadFromBuffer(buffer[, start, [end]]);
See `demo/loadFromBuffer.js`

### copyFromImage(image, x, y, width, height);
See `demo/copyFromImage.js`

### fillColor(red, green, blue[, alpha]);
See `demo/fillColor.js`

### drawImage(img, x, y);
See `demo/drawImage.js`

### toBuffer(type);
See `demo/toBuffer.js`

### width
*readonly*

### height
*readonly*

