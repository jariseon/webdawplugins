var WAA = WAA || {};

WAA.Convolver = function (host)
{
	this.type = "effect";
	this.name = "waa-convolver";
	this.host = host;
	this.input = host.actx.createConvolver();
	
	this.loadIR = function(ir)
	{
		var self = this;
		var xhr = new XMLHttpRequest();
		xhr.responseType = "arraybuffer";
		xhr.onload = function ()
		{
			host.actx.decodeAudioData(xhr.response, function (buf) {
				self.input.buffer = buf;
				});
		}
		xhr.open("get", ir, true);
		xhr.send(null);
	}
};

WAA.Convolver.prototype = new DAWPlugin();
WAA.Convolver.prototype.constructor = WAA.Convolver;
WAA.Convolver.prototype.base = DAWPlugin.prototype;
WAA.Convolver.prototype.path = DAWPlugin.prototype.getPath();
