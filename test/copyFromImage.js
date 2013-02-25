var fs = require("fs"),
    images = require(__dirname + "/../index");

var srcImg = new images.Image(),
    dstImg = new images.Image(),
    x = 100,
    y = 50,
    width = 100,
    height = 100,
    srcData, dstData;


srcData = fs.readFileSync(__dirname + "/img/PNG_transparency_demonstration.png");

srcImg.loadFromBuffer(srcData);

dstImg.copyFromImage(srcImg, x, y, width, height);

dstData = dstImg.toBuffer(images.TYPE_PNG);

fs.writeFileSync(__dirname + "/output/copyFromImage.png", dstData);
