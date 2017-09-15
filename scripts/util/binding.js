/*!
 * node-sass: lib/binding.js
 */

var errors = require('./errors'),
  extensions = require('./extensions.js');
/**
 * Require binding
 */
module.exports = function() {
  if (!extensions.hasBinary(extensions.getBinaryPath())) {
    if (!extensions.isSupportedEnvironment()) {
      throw new Error(errors.unsupportedEnvironment());
    } else {
      throw new Error(errors.missingBinary());
    }
  }

  return require(extensions.getBinaryPath());
};
