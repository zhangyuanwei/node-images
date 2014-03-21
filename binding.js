var binding;

try {
    binding = require('./build/Release/binding.node');
} catch ( e ) {
    throw new Error('Can\'t load the addon. please try to run `node install.js` command.');
}

module.exports = binding;
