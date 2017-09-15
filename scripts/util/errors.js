/*!
 * from: 
 */

var extensions = require('./extensions'),
  pkg = require('../../package.json'),
  packageName = pkg.packageName;

function humanEnvironment() {
  return extensions.getHumanEnvironment(extensions.getBinaryName());
}

function foundBinaries() {
  return [
    'Found bindings for the following environments:',
    foundBinariesList(),
  ].join('\n');
}

function foundBinariesList() {
  return extensions.getInstalledBinaries().map(function(env) {
    return '  - ' + extensions.getHumanEnvironment(env);
  }).join('\n');
}

function missingBinaryFooter() {
  return [
    'This usually happens because your environment has changed since running `npm install`.',
    'Run `npm rebuild ' + pkg.name + ' --force` to build the binding for your current environment.',
  ].join('\n');
}

module.exports.unsupportedEnvironment = function() {
  return [
    pkg.name + 'does not yet support your current environment: ' + humanEnvironment(),
    'For more information on which environments are supported please see:',
    'https://github.com/' + packageName + '/releases/tag/v' + pkg.version
  ].join('\n');
};

module.exports.missingBinary = function() {
  return [
    'Missing binding ' + extensions.getBinaryPath(),
    pkg.name + 'could not find a binding for your current environment: ' + humanEnvironment(),
    '',
    foundBinaries(),
    '',
    missingBinaryFooter(),
  ].join('\n');
};
