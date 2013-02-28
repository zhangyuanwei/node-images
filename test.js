var fs = require("fs"),
    crypto = require("crypto"),
    assert = require('assert'),
    images = require("./index.js");

var WH_123_456 = {
    width: 123,
    height: 456
};

function md5(data) {
    var md5 = crypto.createHash("md5");
    md5.update(data);
    return md5.digest("hex");
}

assert.deepEqual(images("demo/img/JPEG_example_donkey_010.jpg").size(), {
    width: 150,
    height: 271
});


assert.deepEqual(images("demo/img/PNG_transparency_demonstration.png").size(), {
    width: 300,
    height: 225
});

assert.deepEqual(images("demo/img/rotating_earth.gif").size(), {
    width: 200,
    height: 200
});

assert.deepEqual(images(123, 456).size(), WH_123_456);

[
    "png",
    "jpg",
    "jpeg",
//    "gif",
"raw" //
].forEach(function(ext) {
    images(123, 456).save("tmp." + ext);

    assert.deepEqual(images("tmp." + ext).size(), WH_123_456);

    fs.unlink("tmp." + ext);
});

assert.deepEqual(images(images(1230, 4560), 0, 0, 123, 456).size(), WH_123_456);

assert.equal(
	md5(images(100, 100).encode(images.TYPE_RAW)),
	md5(images(images("demo/img/rotating_earth.gif"), 0, 0, 100, 100).fill(0, 0, 0, 0).encode(images.TYPE_RAW))
);

assert.equal(
	md5(
		images(
		images("demo/img/PNG_transparency_demonstration.png")
			.draw(images("demo/img/rotating_earth.gif"), 10, 10)
		, 10, 10, 200, 200).encode(images.TYPE_RAW)
	),
	md5(images("demo/img/rotating_earth.gif").encode(images.TYPE_RAW))
);

images("demo/img/JPEG_example_donkey_050.jpg").save("tmp.png")
assert.equal(
		md5(images("demo/img/JPEG_example_donkey_050.jpg").encode(images.TYPE_RAW)),
		md5(images("tmp.png").encode(images.TYPE_RAW))
);
fs.unlink("tmp.png");


assert.deepEqual(images("demo/img/PNG_transparency_demonstration.png").size(123, 456).size(), WH_123_456);

images.setLimit(200, 271);

assert.doesNotThrow(function(){
	images("demo/img/rotating_earth.gif");
	images("demo/img/JPEG_example_donkey_010.jpg");
});

assert.throws(function(){
	images("demo/img/PNG_transparency_demonstration.png");
});

console.log("pass all test! ^_^");
