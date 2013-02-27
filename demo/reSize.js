var fs = require("fs"),
    images = require(__dirname + "/../index");

var picture;

picture = images(fs.readFileSync(__dirname + "/img/PNG_transparency_demonstration.png"));

fs.writeFileSync(__dirname + "/output/reSize120.png",picture.size(120).encode(images.TYPE_PNG));
fs.writeFileSync(__dirname + "/output/reSize48.png",picture.size(48).encode(images.TYPE_PNG));
fs.writeFileSync(__dirname + "/output/reSize24.png",picture.size(24).encode(images.TYPE_PNG));
