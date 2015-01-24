var AZR3 = function (host, path)
{
	this.type = "synth";
	this.name = "azr3";
	this.patchNames = [];

	// -- AZR3 does not support preset banks besides the built-in bank
	// -- grab a copy of the built-in bank here for easier GUI handling
	this.setBank = function (ibank, bank)
	{
		this.bank = bank;
		this.patchNames = [];
		for (var i=0; i<bank.length; i++)
			this.patchNames.push(bank[i][0]);
	}
	
	this.sendMidi = function (status, data1, data2)
	{
		if (this.gui) this.gui.onMidi(status, data1, data2);
		this.base.sendMidi.call(this, status, data1, data2);
	}
	
	if (!path) path = AZR3.prototype.path;
	host.createPlugin(this, path + "azr3.nmf");
}

AZR3.prototype = new DAWPlugin();
AZR3.prototype.constructor = AZR3;
AZR3.prototype.base = DAWPlugin.prototype;
AZR3.prototype.path = DAWPlugin.prototype.getPath();
