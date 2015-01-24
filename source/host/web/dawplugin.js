function DAWPlugin ()
{
	this.numInputs = this.numOutputs = 0;
	this.input = null;
	this.bankType = "arraybuffer";
	var buflen = 512;
	var self;
	
	// double buffering for PNaCl
	// currently max two channels
	var bufs = [[],[]];
	var curbuf = 0;

	// -- invoked after the module has been loaded
	this.baseinit = function (desc, actx)
	{
		self = this;
		this.descriptor = desc;
		this.initParams(desc);
		this.initAudio(desc, actx, buflen, bufs);
	}

	// -- PNaCl has computed a buffer
	this.onPNaClAudio = function (msg)
	{
		var ibuf = (curbuf == 1) ? 0 : 1;
		bufs[0][ibuf] = msg[0];
		if (msg[1])
			bufs[1][ibuf] = msg[1];
		curbuf = (curbuf == 0) ? 1 : 0;
	}

	// -- ScriptProcessor requests a new buffer
	this.onProcess = function (ape)
	{
		if (self.isPNaCl)
			self.processPNaCl(ape, bufs, curbuf, buflen);
		else self.processEmscripten(ape, buflen);
	};
}

DAWPlugin.prototype.create = function ()
{
	if (this.isPNaCl)
	{
		var msg = { verb:"create", resource:"plugin", plugid:this.id };
		msg.data = { index:this.index, frameLength:512 };
		msg.id = this.host.getMessageID();
		this.postMessage(msg);
	}
	else
	{
		var desc = this.c_createPlugin(this.id, this.index, 512);
		this.onReady({data:{desc:desc}});
	}
}

DAWPlugin.prototype.onReady = function (msg)
{
	var desc = JSON.parse(msg.data.desc);
	if (desc)
	{
		if (this.type == "effect")
			for (var i=0; i<desc.descriptor.params.length; i++)
			{
				var p = desc.descriptor.params[i];
				if (!p.lowerBound) p.lowerBound = 0;
				if (!p.upperBound) p.upperBound = 1;
				if (!p.scaler) p.scaler = 100;
				if (!p.defvalue) p.defvalue = 0;
			}
		
		this.baseinit(desc.descriptor, this.host.actx);
		if (this.onready) this.onready();
		if (this.host.onpluginready) this.host.onpluginready(this);
		
		var e = new Event('pluginready');
		e.detail = { plug: this };
		window.dispatchEvent(e);
	}	
}

DAWPlugin.prototype.initParams = function (descriptor)
{
	this.params = {};
	var params = descriptor.params;
	if (params) for (var i=0; i<params.length; i++)
	{
		var param = params[i];
		param.value = param.defvalue;
		if (param.deftype == "int") param.scaler = 1;
		if (param.id == undefined) // todo: old
		{
			var key = Object.keys(param)[0];
			this.params[key] = param[key];
		}
		else
		{
			if (!this.params.length) this.params = [];
			this.params.push(param);
		}
	}
}

DAWPlugin.prototype.initAudio = function (descriptor, actx, buflen, bufs)
{
	this.numInputs = descriptor.numberOfInputs;
	this.numOutputs = descriptor.numberOfOutputs;

	if (this.numOutputs > 0)
	{
		this.input = actx.createScriptProcessor(buflen, this.numInputs, this.numOutputs);
		this.input.onaudioprocess = this.onProcess;
		if (this.isPNaCl)
		{
			for (var i=0; i<2; i++)
			{
				bufs[0].push(new ArrayBuffer(buflen*4));
				if (this.numOutputs > 1) bufs[1].push(new ArrayBuffer(buflen*4));
			}
		}
		else
		{
			DAWPlugin.prototype.c_onMidi = Module.cwrap("onMidi", null, ["number", "number", "number"]);
			DAWPlugin.prototype.c_getBuffer = Module.cwrap("getBuffer", "number", ["number", "number","number"]);
			DAWPlugin.prototype.c_process = Module.cwrap("process", null, []);
			DAWPlugin.prototype.c_setParam = Module.cwrap("setParam", null, ["number","number"]);
			this.ibufs = [];
			this.obufs = [];
			for (var i = 0; i<2; i++)
			{
				this.ibufs[i] = this.c_getBuffer(this.id, 0, i) / 4;
				this.obufs[i] = this.c_getBuffer(this.id, 1, i) / 4;
			}
		}
	}
};

// -------------------------------------------------------------------------
// DSP
DAWPlugin.prototype.processPNaCl = function (ape, bufs, curbuf, buflen)
{
	var outL = ape.outputBuffer.getChannelData(0);
	var ibuf = ape.inputBuffer;

	// -- request for a new buffer (to be used in next call)
	var ins = [null,null];
	ins[0] = this.numInputs > 0 ? ibuf.getChannelData(0).buffer : null;
	ins[1] = this.numInputs > 1 ? ibuf.getChannelData(1).buffer : null;
	var buf = this.numOutputs > 1 ? bufs[1][curbuf] : null;
	var msg = { verb:"process", plugid:this.id };
	msg.data = [ins[0], ins[1], bufs[0][curbuf], buf];
	this.postMessage(msg);

	// -- copy samples from current buffer
	curbuf = (curbuf == 0) ? 1 : 0;
	if (this.numOutputs == 1)
	{
		var wsinL = new Float32Array(bufs[0][curbuf]);
		for (var n = 0; n < buflen; n++)
			outL[n] = wsinL[n];
	}
	else
	{
		var outR = ape.outputBuffer.getChannelData(1);
		var wsinL = new Float32Array(bufs[0][curbuf]);
		var wsinR = new Float32Array(bufs[1][curbuf]);
		for (var n = 0; n < buflen; n++)
		{
			outL[n] = wsinL[n];
			outR[n] = wsinR[n];
		}
	}
}

DAWPlugin.prototype.processEmscripten = function (ape, buflen)
{
	for (var i = 0; i < 2, i < this.numInputs; i++)
		Module.HEAPF32.set(ape.inputBuffer.getChannelData(i), this.ibufs[i]);

	this.c_process();
	
	var outL = ape.outputBuffer.getChannelData(0);
	outL.set(Module.HEAPF32.subarray(this.obufs[0], this.obufs[0] + 512));
	for (var n=0; n<512; n++)
	{
		if (outL[n] < -1) outL[n] = 0;
		else if (outL[n] > 1) outL[n] = 0;
	}

	if (this.numOutputs > 1)
	{
		var outR = ape.outputBuffer.getChannelData(1);
		outR.set(Module.HEAPF32.subarray(this.obufs[1], this.obufs[1] + 512));
		for (var n=0; n<512; n++)
		{
			if (outR[n] < -1) outR[n] = 0;
			else if (outR[n] > 1) outR[n] = 0;
		}
	}
}

// -------------------------------------------------------------------------
// messaging

DAWPlugin.prototype.requests = [];	
DAWPlugin.prototype.request = function (prop, data)
{
	var self = this;
	return new Promise(function (resolve, reject) {
		var req = { verb:"get", plugid:this.id, resource:prop, data:data };
		req.msgid = self.host.getMessageID();
		DAWPlugin.prototype.requests[req.msgid] = { resolve:resolve, reject:reject };
		self.postMessage(req);
		});
}

DAWPlugin.prototype.postMessage = function (msg)
{
	this.module.postMessage(msg);
}

DAWPlugin.prototype.onmessage = function (msgin)
{
	var msg = msgin.data;
	if (msg.verb == "process" && msg.plugid == this.id) this.onPNaClAudio(msg.data);
	else if (typeof msg == "string") console.log(msg);
	else if (msg.verb == "reply")
	{
		var req = this.requests[msg.msgid];
		if (req)
		{
			req.resolve(msg.data);
			this.requests.splice(this.requests.indexOf(req), 1);
		}
	}
	else if (msg.verb == "create") this.onReady(msg);
}

// -- midi
// -- input is just routed to PNaCl/emscripten implementation
DAWPlugin.prototype.onMidi = function (e)
{
	var msg = e.data;
	this.sendMidi(msg[0], msg[1], msg[2]);
}
DAWPlugin.prototype.sendMidi = function (status, data1, data2)
{
	data1 = data1 ? data1 : 0;
	data2 = data2 ? data2 : 0;
	
	if (this.isPNaCl)
	{
		var msg = { verb:"pub", resource:"midi" };
		msg.data = [status,data1,data2];
		msg.plugid = this.id;
		this.postMessage(msg);
	}
	else this.c_onMidi(status, data1, data2);
}

// -- for webAudioAPI nodes
DAWPlugin.prototype.connect = function (dst)
{
	if (this.input)
		this.input.connect(dst);
}
DAWPlugin.prototype.disconnect = function (index)
{
	if (this.input)
	{
		if (index != undefined) this.input.disconnect(index);
		else this.input.disconnect();
	}
}

// -- for non-webAudioAPI nodes
DAWPlugin.prototype.start = function () { this.postMessage({ verb:"set", resource:"active", plugid:this.id, data:true }); }
DAWPlugin.prototype.stop = function () { this.postMessage({ verb:"set", resource:"active", plugid:this.id, data:false }); }
DAWPlugin.prototype.directOut = function (onoff)
{
	this.postMessage({ verb:"set", resource:"directOut", plugid:this.id, data:onoff });
	this.input.onaudioprocess = onoff ? null : this.onProcess;
}

// -- presets
DAWPlugin.prototype.setParam = function (iparam, value)
{
	if (this.isPNaCl)
	{
		var msg = { verb:"set", resource:"param", plugid:this.id };
		msg.data = { index:iparam, value:value };
		this.postMessage(msg);
	}
	else this.c_setParam(iparam, value);
	if (0 <= iparam && iparam < this.params.length)
		this.params[iparam].value = value;
}
DAWPlugin.prototype.loadPreset = function (url, responseType)
{
	return new Promise(function (resolve, reject) {
		var xhr = new XMLHttpRequest();
		xhr.onload = function ()
		{
			if (xhr.status == 200) resolve(xhr.response);
			else reject(Error(xhr.statusText));
		}
		xhr.onerror = function () { reject(Error("load error")); };
		xhr.onabort = function () { reject(Error("load aborted")); };
		if (responseType) xhr.responseType = responseType;
		xhr.open("get", url, true);
		xhr.send();
		});
};

DAWPlugin.prototype.loadBank = function (src, type)
{
	var self = this;
	return new Promise(function (resolve, reject) {
		type = type || self.bankType;
		if (src.indexOf(".json") == src.length - 5) type = "string";
		self.loadPreset(src, type).then(
			function (resp)
			{
				if (src.indexOf(".json") == src.length - 5)
					resp = JSON.parse(resp);
				self.setBank(0, resp);
				resolve(resp);
			},
			function (err) { reject(err); });
	});
}

DAWPlugin.prototype.setBank = function (ibank, bank)
{
	// this.bank = bank;
	var msg = { verb:"set", resource:"bank", plugid:this.id };
	msg.data = { bank:ibank, value:bank };
	this.postMessage(msg);
}

DAWPlugin.prototype.setPatch = function (ibank, ipatch, data)
{
	var msg = { verb:"set", resource:"patch", plugid:this.id };
	msg.data = { bank:ibank, patch:ipatch, value:data };
	this.postMessage(msg);
}

// assume .nmf file is in the same location as js part
DAWPlugin.prototype.getPath = function ()
{
	var scripts = document.getElementsByTagName("script");
	var path = scripts[scripts.length-1].src.split('?')[0];
	return path.split('/').slice(0, -1).join('/')+'/';
}

DAWPlugin.prototype.createControl = function (param, onchange)
{
	var container = document.createElement("div"); container.className = "param";
	var slider = document.createElement("input"); slider.type = "range";
	var label = document.createElement("div"); label.className = "paramLabel";				
	slider.min = param.lowerbound * param.scaler;
	slider.max = param.upperbound * param.scaler;
	slider.value = param.value * param.scaler;
	slider.oninput = onchange;
	slider.param = param;
	slider.plug = this;
	label.innerHTML = param.name;
	container.appendChild(slider);
	container.appendChild(label);
	return container;
}

