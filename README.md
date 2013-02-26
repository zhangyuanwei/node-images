node-images
===========

Cross-platform image decoder(png/jpeg/gif) and encoder(png) for Nodejs

## Installation
	$ npm install images

## Usage

``` javascript
var fs = require("fs"),
    images = require("images"),         //npm
//  images = require("./index.js"),     //git
    input, logo, output;

input = fs.readFileSync("input.jpg");   // Load input data
logo = fs.readFileSync("logo.jpg");     // Load logo data
output = images(input)                  // Decode input data to image
    .draw(images(logo), 0, 0)           // Decode and draw logo on image
    .encode(images.TYPE_PNG);           // Encode image to a buffer

fs.writeFileSync("output.png", output); // Sava output image
```

## API

node-images provide jQuery-like Chaining API,
You can start the chain like this:
```javascript
/* Create a new transparent image */
images(width, height)

/* Load and decode image from a buffer */
images(buffer[, start[, end]])

/* Copy from another image */
images(image[, x, y, width, height])
```

### images(width, height)
Create a new transparent image

### images(buffer[, start[, end]])
Load and decode image from a buffer
See `demo/loadFromBuffer.js`

### images(image[, x, y, width, height])
Copy from another image
See `demo/copyFromImage.js`

### .fill(red, green, blue[, alpha])
See `demo/fillColor.js`

### .draw(image, x, y)
See `demo/drawImage.js`

### .encode(type)
Encode image to buffer.
Return buffer
See `demo/toBuffer.js`

### .size([width, height])
Return image size or **TODO:** *set image size*
```javascript
{
	width:123,
	height:456
}
```

### .width([width])
Return image width or **TODO:** *set image width*

### .height([height])
Return image height or **TODO:** *set image height*

### images.setLimit(width, height)
Set the limit size of each image
