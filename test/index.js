var images = require("../");
var path = require('path');

images(path.join(__dirname, "input.png"))
    .resize( 200 )
    .save(path.join(__dirname, "output_new.png"));


images(path.join(__dirname, "input.png"))
    .size( 200 )
    .save(path.join(__dirname, "output_old.png"));


images(path.join(__dirname, "input.jpg"))
    .resize( 200 )
    .save(path.join(__dirname, "output_new.jpg"));


images(path.join(__dirname, "input.jpg"))
    .size( 200 )
    .save(path.join(__dirname, "output_old.jpg"));

images(path.join(__dirname, "input.gif"))
    .resize( 200 )
    .save(path.join(__dirname, "output_new_gif.jpg"));


images(path.join(__dirname, "input.gif"))
    .size( 200 )
    .save(path.join(__dirname, "output_old_gif.jpg"));
