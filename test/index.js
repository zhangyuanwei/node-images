var images = require("../");

images("input.png")
    .resize( 200 )
    .save("output_new.png");

images("input.png")
    .size( 200 )
    .save("output_old.png");

// images("input.png")
//     .resize( null, 400 )
//     .save("output_100%x400.png");