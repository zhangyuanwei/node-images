var fs = require("fs"),
    path = require("path"),
    name = "images.node",
    test, i, count, file;

test = [path.join(__dirname, "build", "Release", name),
path.join(__dirname, "lib", process.platform, name)];

count = test.length;
for (i = 0; i < count; i++){
	file = test[i];
	if (fs.existsSync(file)) {
		module.exports = require(file);
		break;
	}
}
