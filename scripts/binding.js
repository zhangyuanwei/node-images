
'use strict';

var fs = require('fs');
var os = require('os');
var path = require('path');
var root = path.resolve(__dirname, '..');
var binding = 'binding.node';

/**
 * Require binding
 */
module.exports = function() {
  var binaryPath;
  var loadErrorMessage = ' , You can rebuild it, to directory ' + root + ',  Run `node-gyp rebuild` Command.';
  if (hasBinary(getBuildBinaryPath())) {
    return require(getBuildBinaryPath());
  }

  if (!hasBinary(binaryPath = getBinaryPath())) {
    throw new Error('Not found ' + binaryPath + loadErrorMessage);
  }
  try {
    return require(getBinaryPath());
  } catch (e) {
    throw new Error(e.message + loadErrorMessage);
  }
};

function getBuildBinaryPath() {
  return path.join(
    root,
    'build',
    process.env['NODE_DEBUG'] == 'dev' ? 'Debug' : 'Release',
    binding
  )
}

function getBinaryPath() {
  return path.join(
    root,
    'vendor',
    os.platform() + '-' + os.arch() + '-' + binding
  );
}

function hasBinary(path) {
  return fs.existsSync(path);
}