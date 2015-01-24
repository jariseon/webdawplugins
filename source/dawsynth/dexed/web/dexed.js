var Dexed = function (host, path)
{
	this.type = "synth";
	this.name = "dexed";
	this.patchNames = [];
	
	this.setBank = function (ibank, bank)
	{
		// -- extract patchnames
		this.patchNames = [];
		var bytes = new Uint8Array(bank);
		var offset = 6;
		for (var i = 0; i < 32; i++)
		{
			var patch = bytes.subarray(offset, offset+128);			
			var name = "";
			for (var n = 0; n < 10; n++)
			{
				var c = patch[n + 118];
				switch (c) {
					case  92:  c = 'Y';  break;  // yen
					case 126:  c = '>';  break;  // >>
					case 127:  c = '<';  break;  // <<
					default: if (c < 32 || c > 127) c = 32; break;
				}
				name += String.fromCharCode(c);
			}
			this.patchNames.push(name);			

			offset += 128;
		}
		
		this.bank = bytes.subarray(6);
		this.base.setBank.call(this, ibank, bank);
	}
	
	if (!path) path = Dexed.prototype.path;
	host.createPlugin(this, path + "dexed.nmf");	
}

Dexed.prototype = new DAWPlugin();
Dexed.prototype.constructor = Dexed;
Dexed.prototype.base = DAWPlugin.prototype;
Dexed.prototype.path = DAWPlugin.prototype.getPath();
