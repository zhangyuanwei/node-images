var fs = require("fs"),
    images = require(__dirname + "/../index");

var img1, img2, img3, img4, dstImg, x, y, width, height, srcData, dstData;

img1 = images.loadFromBuffer(fs.readFileSync(__dirname + "/img/JPEG_example_donkey_010.jpg"));
img2 = images.loadFromBuffer(fs.readFileSync(__dirname + "/img/JPEG_example_donkey_050.jpg"));
img3 = images.loadFromBuffer(fs.readFileSync(__dirname + "/img/JPEG_example_donkey_100.jpg"));
img4 = images.loadFromBuffer(fs.readFileSync(__dirname + "/img/PNG_transparency_demonstration.png"));

width = img1.width() + img2.width() + img3.width();
height = Math.max(img1.height(), img2.height(), img3.height());

dstImg = images.createImage(width, height);

x = y = 0;
dstImg.drawImage(img1, x, y);

x += img1.width();
dstImg.drawImage(img2, x, y);

x += img2.width();
dstImg.drawImage(img3, x, y);

dstImg.drawImage(img4, 0, 0);

fs.writeFileSync(__dirname + "/output/drawImage.png", dstImg.toBuffer(images.TYPE_PNG));
