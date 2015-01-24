var OBxd = function (host, path)
{
	this.type = "synth";
	this.name = "obxd";
	this.patchNames = [];
	
	// oh dear, cannot use DOMParser since fxb attribute names start with a number
	this.setBank = function (ibank, bank)
	{
		this.bank = [];
		this.patchNames = [];
		var arr = new Uint8Array(bank);
		var s = String.fromCharCode.apply(null, arr.subarray(168, arr.length-1));
		
		var i1 = s.indexOf("<programs>");
		var i2 = s.indexOf("</programs>");
		if (i1 > 0 && i2 > 0)
		{
			s = s.slice(i1+10,i2);
			i2 = 0;
			i1 = s.indexOf("programName");
			var patchCount = 0;
			while (i1 > 0 && patchCount++ < 128)
			{
				var n1 = s.indexOf('\"',i1);
				var n2 = s.indexOf('\"',n1+1);
				if (n1 < 0 || n2 < 0) break;
				this.patchNames.push(s.slice(n1+1,n2));
				i2 = s.indexOf("/>", n2);
				if (i2 > 0)
				{
					var s2 = s.slice(n2+2,i2);
					var tokens = s2.split(' ');
					if (tokens.length == 71)
					{
						var patch = [];
						for (var i=0; i<tokens.length; i++)
						{
							var pair = tokens[i].split('"');
							patch.push(parseFloat(pair[1]));
						}
						this.bank.push(patch);
					}
				}
				i1 = s.indexOf("programName", i2);
			}
		}
		this.base.setBank.call(this, ibank, bank);
	}

	if (!path) path = OBxd.prototype.path;
	host.createPlugin(this, path + "obxd.nmf");	
}

OBxd.prototype = new DAWPlugin();
OBxd.prototype.constructor = OBxd;
OBxd.prototype.base = DAWPlugin.prototype;
OBxd.prototype.path = DAWPlugin.prototype.getPath();
