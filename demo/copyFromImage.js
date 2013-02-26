var fs = require("fs"),
    images = require(__dirname + "/../index");

var srcData, dstData,
srcImg, dstImg,
x = 100,
    y = 50,
    width = 100,
    height = 100;


srcData = fs.readFileSync(__dirname + "/img/PNG_transparency_demonstration.png");
srcImg = images(srcData);

dstImg = images(srcImg, x, y, width, height);
dstData = dstImg.encode(images.TYPE_PNG);

fs.writeFileSync(__dirname + "/output/copyFromImage.png", dstData);
