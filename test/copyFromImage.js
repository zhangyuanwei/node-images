var fs = require("fs"),
    images = require("../index");

var srcImg = new images.Image(),
    dstImg = new images.Image(),
    x = 100,
    y = 50,
    width = 100,
    height = 100,
    srcData, dstData;


srcData = fs.readFileSync("img/PNG_transparency_demonstration.png");

srcImg.loadFromBuffer(srcData);

dstImg.copyFromImage(srcImg, x, y, width, height);

dstData = dstImg.toBuffer(images.TYPE_PNG);

fs.writeFileSync("output/copyFromImage.png", dstData);
