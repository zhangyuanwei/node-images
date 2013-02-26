var fs = require("fs"),
    images = require(__dirname + "/../index");

var img1, img2, img3, img4, dstImg, x, y, width, height, srcData, dstData;

img1 = images(fs.readFileSync(__dirname + "/img/JPEG_example_donkey_010.jpg"));
img2 = images(fs.readFileSync(__dirname + "/img/JPEG_example_donkey_050.jpg"));
img3 = images(fs.readFileSync(__dirname + "/img/JPEG_example_donkey_100.jpg"));
img4 = images(fs.readFileSync(__dirname + "/img/PNG_transparency_demonstration.png"));

width = img1.width() + img2.width() + img3.width();
height = Math.max(img1.height(), img2.height(), img3.height());

dstImg = images(width, height);

x = y = 0;
dstImg.draw(img1, x, y);

x += img1.width();
dstImg.draw(img2, x, y);

x += img2.width();
dstImg.draw(img3, x, y);

dstImg.draw(img4, 0, 0);

fs.writeFileSync(__dirname + "/output/drawImage.png", dstImg.toBuffer(images.TYPE_PNG));
