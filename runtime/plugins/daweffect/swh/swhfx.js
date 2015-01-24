var SWH = SWH || {};

SWH.Phaser = function (host)
{
	this.type = "effect";
	this.name = "swhphaser";
	this.gui = [];
	
	this.onready = function ()
	{
		this.setParam(0, 0.7);
		this.setParam(2, -0.9);
	}

	host.createPlugin(this, this.path + "swh_phasers.js", 0);	
}

SWH.Phaser.prototype = new DAWPlugin();
SWH.Phaser.prototype.constructor = SWH.Phaser;
SWH.Phaser.prototype.base = DAWPlugin.prototype;
SWH.Phaser.prototype.path = DAWPlugin.prototype.getPath();

SWH.BodeShifter = function (host)
{
	this.type = "effect";
	this.name = "swhbode";

	this.onready = function ()
	{
		this.numInputs = 1;
	}

	host.createPlugin(this, this.path + "swh_bode.js", 0);	
}

SWH.BodeShifter.prototype = new DAWPlugin();
SWH.BodeShifter.prototype.constructor = SWH.BodeShifter;
SWH.BodeShifter.prototype.base = DAWPlugin.prototype;
SWH.BodeShifter.prototype.path = DAWPlugin.prototype.getPath();

