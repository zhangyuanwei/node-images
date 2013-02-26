var path = require("path"),
    fs = require("fs"),
    images = require(__dirname + "/../index");

var DIR = __dirname + "/img";

fs.readdir(DIR, function(err, items) {
    if (err) {
        console.log("Error on read dir.");
        return;
    }

    items.forEach(function(entry) {
        fs.readFile(DIR + "/" + entry, function(err, data) {
            if (err) {
                console.log("Error on read file.");
                return;
            }

            console.log(entry + ":");
            console.log(images(data).size());
            console.log();
        });
    });
});
