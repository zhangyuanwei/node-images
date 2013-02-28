var fs = require('fs'),
    path = require('path'),
    express = require('express'),
    images = require('../index');

var app = express();

app.use(express.bodyParser({
    uploadDir: __dirname + '/upload'
}));

app.use(express.static(__dirname + '/upload'));

app.get('/', function(req, res) {
    res.send('<form method="post" enctype="multipart/form-data" action="/upload"><input type="file" name="photo" /><input type="submit" /></form>');
});

app.post('/upload', function(req, res) {
    var tmp_path = req.files.photo.path,
        out_path = tmp_path + '.jpg',
        photo;

    photo = images(tmp_path);
    photo.size(800)
        .draw(images('./logo.png'), 800 - 421, photo.height() - 117)
        .save(out_path, {
        quality: 80
    });

    fs.unlink(tmp_path, function(err) {
        if (err) throw err;
        res.send('<a href="/" title="upload"><img src="/' + path.basename(out_path) + '" /></a>');
    });
});

app.listen(3000);
