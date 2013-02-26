var fs = require("fs"),
    images = require(__dirname + "/../index");

var img;
img = images(fs.readFileSync(__dirname + "/img/rotating_earth.gif"));
fs.writeFileSync(__dirname + "/output/toBuffer.png", img.encode(images.TYPE_PNG));
