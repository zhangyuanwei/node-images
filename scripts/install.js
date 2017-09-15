/*
 * install.js
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 ZhangYuanwei <zhangyuanwei1988@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL INTEL AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// copy from https://github.com/sass/node-sass/blob/master/scripts/install.js
// thanks, node-sass

var fs = require('fs'),
  eol = require('os').EOL,
  mkdir = require('mkdirp'),
  path = require('path'),
  extensions = require('./util/extensions'),
  request = require('request'),
  log = require('npmlog'),
  downloadOptions = require('./util/downloadoptions');

/**
 * Download file, if succeeds save, if not delete
 * 
 * @param {String} url
 * @param {String} dest
 * @param {Function} cb
 * @api private
 */

function download(url, dest, cb) {
  var reportError = function(err) {
    var timeoutMessge;

    if (err.code === 'ETIMEDOUT') {
      if (err.connect === true) {
        // timeout is hit while your client is attempting to establish a connection to a remote machine
        timeoutMessge = 'Timed out attemping to establish a remote connection';
      } else {
        timeoutMessge = 'Timed out whilst downloading the prebuilt binary';
        // occurs any time the server is too slow to send back a part of the response
      }

    }
    cb(['Cannot download "', url, '": ', eol, eol,
      typeof err.message === 'string' ? err.message : err, eol, eol,
      timeoutMessge ? timeoutMessge + eol + eol : timeoutMessge,
      'Hint: If github.com is not accessible in your location', eol,
      '      try setting a proxy via HTTP_PROXY, e.g. ', eol, eol,
      '      export HTTP_PROXY=http://example.com:1234',eol, eol,
      'or configure npm proxy via', eol, eol,
      '      npm config set proxy http://example.com:8080'].join(''));
  };

  var successful = function(response) {
    return response.statusCode >= 200 && response.statusCode < 300;
  };

  console.log('Downloading binary from', url);

  try {
    request(url, downloadOptions(), function(err, response) {
      if (err) {
        reportError(err);
      } else if (!successful(response)) {
        reportError(['HTTP error', response.statusCode, response.statusMessage].join(' '));
      } else {
        console.log('Download complete');
        cb();
      }
    })
    .on('response', function(response) {
      var length = parseInt(response.headers['content-length'], 10);
      var progress = log.newItem('', length);

      if (successful(response)) {
        response.pipe(fs.createWriteStream(dest));
      }

      // The `progress` is true by default. However if it has not
      // been explicitly set it's `undefined` which is considered
      // as far as npm is concerned.
      if (process.env.npm_config_progress === 'true') {
        log.enableProgress();

        response.on('data', function(chunk) {
          progress.completeWork(chunk.length);
        })
        .on('end', progress.finish);
      }
    });
  } catch (err) {
    cb(err);
  }
}

/**
 * Check and download binary
 *
 * @api private
 */

function checkAndDownloadBinary() {
  if (process.env.SKIP_DOWNLOAD_FOR_CI) {
    console.log('Skipping downloading binaries on CI builds');
    return;
  }

  var cachedBinary = extensions.getCachedBinary(),
    cachePath = extensions.getBinaryCachePath(),
    binaryPath = extensions.getBinaryPath();

  if (extensions.hasBinary(binaryPath)) {
    console.log('component build', 'Binary found at', binaryPath);
    return;
  }

  try {
    mkdir.sync(path.dirname(binaryPath));
  } catch (err) {
    console.error('Unable to save binary', path.dirname(binaryPath), ':', err);
    return;
  }

  if (cachedBinary) {
    console.log('Cached binary found at', cachedBinary);
    fs.createReadStream(cachedBinary).pipe(fs.createWriteStream(binaryPath));
    return;
  }

  download(extensions.getBinaryUrl(), binaryPath, function(err) {
    if (err) {
      console.error(err);
      return;
    }

    console.log('Binary saved to', binaryPath);

    cachedBinary = path.join(cachePath, extensions.getBinaryName());

    if (cachePath) {
      console.log('Caching binary to', cachedBinary);

      try {
        mkdir.sync(path.dirname(cachedBinary));
        fs.createReadStream(binaryPath)
          .pipe(fs.createWriteStream(cachedBinary))
          .on('error', function (err) {
            console.log('Failed to cache binary:', err);
          });
      } catch (err) {
        console.log('Failed to cache binary:', err);
      }
    }
  });
}

/**
 * If binary does not exist, download it
 */

checkAndDownloadBinary();