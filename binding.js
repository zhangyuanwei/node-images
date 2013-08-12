var fs = require("fs"),
    path = require("path"),
    name = "node_images.node",
    tryList, i, count, file;

var get_special_addon = function() {
    /**
     *  0.8.0-0.8.25 : 0.8.0
     *  0.9.9-0.10.13 : 0.9.9
     *  0.9.2-0.9.8: 0.9.2
     *  0.9.1-0.9.1: 0.9.1
     *  0.9.0-0.9.0: 0.9.0
     */
    var map =  {
        '0.8.0-0.8.25': '0.8.0',
        '0.9.9-latest': '0.9.9'
    };
    var tmp;
    for (var vers in map) {
        if (map.hasOwnProperty(vers)) {
            var head, tail, version;
            tmp = vers.split('-');
            head = tmp[0].split('.', 3);
            if (tmp[1] == 'latest') {
                tail = null;
            } else {
                tail = tmp[1].split('.', 3);
            }
            version = process.version.split('.', 3);
            if (tail) {
                if (head[0] < version[0] && version[0] < tail[0]) {
                    return map[vers];
                }
                if (head[1] < version[1] && version[1] < tail[1]) {
                    return map[vers];
                }
                if (head[2] > version[2] && version[2] < tail[2]){
                    return map[vers];
                }
            } else {
                if (head[0] < version[0]) {
                    return map[vers];
                }
                if (head[1] < version[1]) {
                    return map[vers];
                }
                if (head[2] < version[2]) {
                    return map[vers];
                }
            }
        }
    }
    return false;
};

tryList = [
    path.join(__dirname, "build", "Release", name) // build dir
];

var wish = get_special_addon();
if (wish) {
    tryList.push(path.join(__dirname, "lib", process.platform, process.arch, wish, name));
}


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
