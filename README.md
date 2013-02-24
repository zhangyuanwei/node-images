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

### copyFromImage(x, y, width, height);

### fillColor(red, green, blue[, alpha]);

### drawImage(img, x, y);

### toBuffer(type);

### width

### height

