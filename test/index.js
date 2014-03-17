var images = require("../");

images("input.png")
    .resize( 200 )
    .save("output_200x100%.png");

images("input.png")
    .resize( null, 400 )
    .save("output_100%x400.png");