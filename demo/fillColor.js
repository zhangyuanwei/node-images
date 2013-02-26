var fs = require("fs"),
    images = require(__dirname + "/../index");

var background, picture, dstImg, x, y, width, height, srcData, dstData;

picture = images.loadFromBuffer(fs.readFileSync(__dirname + "/img/PNG_transparency_demonstration.png"));

background = images.createImage(picture.width(), picture.height())
    .fillColor(0xff, 0x00, 0x00, 0.5)
    .drawImage(picture, 0, 0);

fs.writeFileSync(__dirname + "/output/fillColor.png", background.toBuffer(images.TYPE_PNG));
