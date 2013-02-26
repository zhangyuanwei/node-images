var fs = require("fs"),
    images = require(__dirname + "/../index");

var img;
img = images.loadFromBuffer(fs.readFileSync(__dirname + "/img/rotating_earth.gif"));
fs.writeFileSync(__dirname + "/output/toBuffer.png", img.toBuffer(images.TYPE_PNG));
