XSynth = function (host, path)
{
	this.type = "synth";	
	this.name = "xsynth";
	this.patchNames = [];
	
	// -- extract patchnames
	this.setBank = function (ibank, bank)
	{
		this.bank = bank;
		this.patchNames = [];
		for (var i=0; i<bank.length; i++)
			this.patchNames.push(bank[i][0]);
		this.base.setBank.call(this, ibank, bank);
	}

	if (!path) path = XSynth.prototype.path;
	host.createPlugin(this, path + "xsynth.nmf");
}

XSynth.prototype = new DAWPlugin();
XSynth.prototype.constructor = XSynth;
XSynth.prototype.base = DAWPlugin.prototype;
XSynth.prototype.path = DAWPlugin.prototype.getPath();

