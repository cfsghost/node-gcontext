var _gcontext = require('../build/Release/gcontext.node');

var GContext = module.exports = {};

GContext.init = function() {
	_gcontext.init();
};

GContext.uninit = function() {
	_gcontext.uninit();
};
