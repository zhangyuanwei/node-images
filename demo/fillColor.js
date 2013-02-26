var fs = require("fs"),
    images = require(__dirname + "/../index");

var background, picture, dstImg, x, y, width, height, srcData, dstData;

picture = images(fs.readFileSync(__dirname + "/img/PNG_transparency_demonstration.png"));

background = images(picture.width(), picture.height())
    .fill(0xff, 0x00, 0x00, 0.5)
    .draw(picture, 0, 0);

fs.writeFileSync(__dirname + "/output/fillColor.png", background.encode(images.TYPE_PNG));
