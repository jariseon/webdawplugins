Hexter = function (host, path)
{
	this.type = "synth";
	this.name = "hexter";
	this.patchNames = [];
	
	this.setBank = function (ibank, inbank)
	{
		var bank = [];
		this.patchNames = [];
		var bytes = new Uint8Array(inbank);
		var offset = 6;
		for (var i = 0; i < 32; i++)
		{
			var patch = bytes.subarray(offset, offset+128);			
			bank.push(Array.prototype.slice.call(patch));
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
		
		this.base.setBank.call(this, ibank, bank);
	}

	if (!path) path = Hexter.prototype.path;
	else Hexter.prototype.path = path;
	host.createPlugin(this, path + "hexter.nmf");
}

Hexter.prototype = new DAWPlugin();
Hexter.prototype.constructor = Hexter;
Hexter.prototype.base = DAWPlugin.prototype;
Hexter.prototype.path = DAWPlugin.prototype.getPath();

