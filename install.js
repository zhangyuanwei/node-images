/**
 * This script will be excuted while npm installing this package.
 *
 * The flow of this script.
 * 1. Accroding to the client's platform download an addon binary
 *    package from github.
 * 2. Try the addon if fail then try `node-gyp` to rebuild a new addon bindary
 *    package.
 * 3. Run the test script.
 */

var step = require('step'),
    fs = require('fs'),
    path = require('path');

function mkdirP ( p, mode, made ) {
    if (mode === undefined) {
        mode = 0777 & (~process.umask());
    }
    if (!made) made = null;

    if (typeof mode === 'string') mode = parseInt(mode, 8);
    p = path.resolve(p);

    try {
        fs.mkdirSync(p, mode);
        made = made || p;
    }
    catch (err0) {
        switch (err0.code) {
            case 'ENOENT' :
                made = mkdirP(path.dirname(p), mode, made);
                mkdirP(p, mode, made);
                break;

            // In the case of any other error, just see if there's a dir
            // there already.  If so, then hooray!  If not, then something
            // is borked.
            default:
                var stat;
                try {
                    stat = fs.statSync(p);
                }
                catch (err1) {
                    throw err0;
                }
                if (!stat.isDirectory()) throw err0;
                break;
        }
    }

    return made;
}

// Try to download binding.node from github.
function download() {
    var pkg = require('./package.json'),
        https = require('https'),
        url = require('url'),
        done = this,
        githubUser, githubRepos, downloadUrl;

    if ( pkg.bindingsCDN ) {
        downloadUrl = pkg.bindingsCDN;
    } else if ( /([^\/]+?)\/([^\.\/]+?)\.git$/i.test( pkg.repository.url ) ) {
        githubUser = RegExp.$1;
        githubRepos = RegExp.$2;
        downloadUrl = 'https://raw.github.com/' + githubUser + '/' +
                githubRepos + '/master/bindings/';
    }

    // Accroding to the client's platform and try to download an addon binary
    // package from github.
    if ( downloadUrl ) {
        var map = require('./map.json'),
            candidates, version, modPath, candidate;

        version = process.versions.node;

        try {
            candidates = map[ process.platform ][ process.arch ];
        } catch ( e ) {
            return done( true, '' );
        }


        function versionCompare(left, right) {
            if (typeof left + typeof right != 'stringstring')
                return false;

            var a = left.split('.'),
                b = right.split('.'),
                i = 0,
                len = Math.max(a.length, b.length);

            for (; i < len; i++) {
                if ((a[i] && !b[i] && parseInt(a[i], 10) > 0) || (parseInt(a[i], 10) > parseInt(b[i], 10))) {
                    return 1;
                } else if ((b[i] && !a[i] && parseInt(b[i]) > 0) || (parseInt(a[i], 10) < parseInt(b[i], 10))) {
                    return -1;
                }
            }

            return 0;
        }

        if ( candidates.length ) {
            do {
                candidate = candidates.pop();

                if ( versionCompare( version, candidate ) >= 0 ) {
                    break;
                }

            } while ( candidates.length );

            modPath = process.platform + '/' + process.arch + '/' +
                    candidate + '/binding.node';
        } else {
            console.error( 'Can\'t find the binding.node file.' );
            return done( true );
        }

        // start to download.
        var options = url.parse( downloadUrl + modPath ),
            dest = './build/Release/binding.node',
            client;

        if ( fs.existsSync( dest ) ) {
            console.log( 'The binding.node file exist, skip download.' );
            done( false );
            return;
        }

        console.log('Downloading', options.href );
        client = https.get( options, function( res ) {
            var count = 0,
                notifiedCount = 0,
                outFile;

            if ( res.statusCode === 200 ) {
                mkdirP( path.dirname( dest ) );
                outFile = fs.openSync( dest, 'w' );

                res.on('data', function( data ) {
                    fs.writeSync(outFile, data, 0, data.length, null);
                    count += data.length;

                    if ( (count - notifiedCount) > 512 * 1024 ) {
                      console.log('Received ' + Math.floor( count / 1024 ) + 'K...');
                      notifiedCount = count;
                    }
                });

                res.addListener('end', function() {
                    console.log('Received ' + Math.floor(count / 1024) + 'K total.');
                    fs.closeSync( outFile );
                    done( false );
                });

            } else {
                client.abort()
                console.error('Error requesting archive');
                done( true );
            }
        }).on('error', function(e) {
            console.error( e.message );
            done( true, e );
        });
    } else {
        done( true );
    }
}

// try to rebuild this.
function rebuild( error ) {
    var done = this,
        cp = require('child_process');

    cp.spawn(
        process.platform === 'win32' ? 'node-gyp.cmd' : 'node-gyp', ['rebuild'], {
        customFds: [0, 1, 2]
    })
    .on('exit', function(err) {
        if (err) {
            if (err === 127) {
                console.error(
                    'node-gyp not found! Please upgrade your install of npm! You need at least 1.1.5 (I think) ' +
                    'and preferably 1.1.30.'
                );
            } else {
                console.error('Build failed');
            }

            done( err );
        }

        done( false );
    });
}

// simply include the binding.js script.
function test( err ) {
    // already failed?
    if ( !err ) {
        try {
            delete require.cache[ require.resolve('./binding.js') ];

            require('./binding.js');
            this( false );
        } catch ( e ) {
            console.error('Test failed!');
            err = true;
        }
    }

    this( err );
}

// donwload first, if fail then rebuild it.
step( download, test, function( error ) {
    // Do I need to rebuild?
    if ( error ) {
        step( rebuild, test, this );
    } else {
        return;
    }
}, function( error ) {
    process.exit( error ? 1 : 0 );
});