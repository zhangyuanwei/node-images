node-images
===========

Cross-platform image decoder(png/jpeg/gif) and encoder(png) for Nodejs

## Installation
	$ npm install images

## Usage

``` javascript
var images = require("images"), //npm
//  images = require("./index.js"), //git
    fs = require(""fs);

var img = images.loadFromBuffer(fs.readFileSync("demo/img/rotating_earth.gif"));
fs.writeFileSync("demo/output/gif2png.png", img.toBuffer(images.TYPE_PNG));
```

## API
node-images provide jQuery-like Chaining API,
You can start the chain like this:
```javascript
images.createImage(width, height).XXXX()
// OR
images.loadFromBuffer(buffer[, start[, end]]).XXXX()
// OR
images.copyFromImage(image[, x, y, width, height]).XXXX()
```

### .createImage(width, height)
Create a new transparent image

### .loadFromBuffer(buffer[, start[, end]])
Load and decode image from a buffer
See `demo/loadFromBuffer.js`

### .copyFromImage(image[, x, y, width, height])
Copy from another image
See `demo/copyFromImage.js`

### .fillColor(red, green, blue[, alpha])
See `demo/fillColor.js`

### .drawImage(image, x, y)
See `demo/drawImage.js`

### .size([width, height])
Return image size or **TODO:set image size**
```javascript
{
	width:123,
	height:456
}
```

### .width([width])
Return image width or **TODO:set image width**

### .height([height])
Return image height or **TODO:set image height**

