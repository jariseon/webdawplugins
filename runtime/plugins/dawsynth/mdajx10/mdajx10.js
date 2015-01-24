var MDAJX10 = function (host, path)
{
	this.type = "synth";
	this.name = "mdajx10";
	this.patchNames = [];
	
	this.setBank = function (ibank, bank)
	{
		this.bank = bank;
		this.patchNames = [];
		for (var i=0; i<bank.length; i++)
			this.patchNames.push(bank[i][0]);
		this.base.setBank.call(this, ibank, bank);
	}
	
	if (!path) path = MDAJX10.prototype.path;
	host.createPlugin(this, path + "mdajx10.nmf");
}

MDAJX10.prototype = new DAWPlugin();
MDAJX10.prototype.constructor = MDAJX10;
MDAJX10.prototype.base = DAWPlugin.prototype;
MDAJX10.prototype.path = DAWPlugin.prototype.getPath();
