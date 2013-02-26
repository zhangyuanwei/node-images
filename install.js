try {
    require("./binding.js"); //try to load pre-build lib
} catch (e) {
	// build addon
    var spawn = require('child_process').spawn,
        gyp = spawn('node-gyp', ['rebuild']);

    gyp.stdout.pipe(process.stdout);
    gyp.stderr.pipe(process.stderr);

    gyp.on('exit', function(code) {
        process.exit(code);
    });
}
