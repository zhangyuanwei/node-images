var fs = require("fs"),
    images = require("../index");

var img;

img = new images.Image();
img.loadFromBuffer(fs.readFileSync("img/rotating_earth.gif"));
fs.writeFileSync("output/toBuffer.png", img.toBuffer(images.TYPE_PNG));
