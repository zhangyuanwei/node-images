var imagesFn = require(__dirname + '/../');
var images = function (file) { return imagesFn(__dirname + '/' + file); };

images("input.png")
    .resize(200)
    .save("output_new.png");

images("input.png")
    .rotate(45)
    .save("output_rotate.png");

images("input.png")
    .size(200)
    .save("output_old.png");


images("input.jpg")
    .resize(200)
    .save("output_new.jpg");


images("input.jpg")
    .size(200)
    .save("output_old.jpg");

images("input.gif")
    .resize(200)
    .save("output_new_gif.jpg");


images("input.gif")
    .size(200)
    .save("output_old_gif.jpg");

images("input.png")
    .resize(200)
    .save("output_new.blp");

images("input.png")
    .rotate(45)
    .save("output_rotate.blp");

images("input.png")
    .size(200)
    .save("output_old.blp");

images("input.jpg")
    .resize(200)
    .save("output_new.blp");


images("input.jpg")
    .size(200)
    .save("output_old.blp");

images("input.gif")
    .resize(200)
    .save("output_new_gif.blp");


images("input.gif")
    .size(200)
    .save("output_old_gif.blp");

images('logo.png')
    .save('output_logo.blp');