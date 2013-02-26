var fs = require("fs"),
    path = require("path"),
    name = "images.node",
    tryList, i, count, file;

tryList = [
	path.join(__dirname, "build", "Release", name), // build dir
	path.join(__dirname, "lib", process.platform, name) // prebuild lib
];

count = tryList.length;
for (i = 0; i < count; i++) {
    file = tryList[i];
    if (fs.existsSync(file)) {
        try {
            exports = require(file);
        } catch (e) {
            continue; // try next
        }
        break;
    }
}

if (exports && exports.Image) {
    module.exports = exports;
} else {
    throw new Error("Can't load addon.");
}
