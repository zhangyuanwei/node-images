var path = require("path"),
    fs = require("fs");

fs.readdir(__dirname, function(err, items) {

    if (err) {
        console.log("Error on read dir.");
        return;
    }

    items.forEach(function(entry) {
        if (entry.substr(-3) == ".js") {
            console.log("Run " + entry);
            require(__dirname + "/" + entry);
        }
    });

});
