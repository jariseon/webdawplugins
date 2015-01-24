var MDA = MDA || {};

MDA.Leslie = function (host)
{
	this.type = "effect";
	this.name = "mdaleslie";
	
	host.createPlugin(this, this.path + "mda_leslie.js", 0);	
}

MDA.Leslie.prototype = new DAWPlugin();
MDA.Leslie.prototype.constructor = MDA.Leslie;
MDA.Leslie.prototype.base = DAWPlugin.prototype;
MDA.Leslie.prototype.path = DAWPlugin.prototype.getPath();
