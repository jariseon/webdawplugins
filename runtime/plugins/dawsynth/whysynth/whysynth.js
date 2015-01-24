var Whysynth = function (host, path)
{
	this.type = "synth";
	this.name = "whysynth";
	this.bankType = "string";
	this.patchNames = [];
	
	function asNumber(a)
	{
		if (Array.isArray(a))
		{
			a.forEach(function(elem,i) { a[i] = parseFloat(elem); });
			return a;
		}
		else return parseFloat(a);
	}

	function parseLine(line, patch)
	{
		var t = line.split(' ');
		switch (t[0])
		{
			case "name": patch.name = decodeURIComponent(t[1]); break;
			case "comment": patch.comment = decodeURIComponent(t[1]); break;
			case "oscY": patch.osc[parseInt(t[1])-1] = asNumber(t.slice(2)); break;
			case "vcfY": patch.vcf[parseInt(t[1])-1] = asNumber(t.slice(2)); break;
			case "mix": patch.mix = asNumber(t.slice(1)); break;
			case "volume": patch.volume = asNumber(t[1]); break;
			case "effects": patch.effects = asNumber(t.slice(1)); break;
			case "glide": patch.glide = asNumber(t[1]); break;
			case "bend": patch.bend = asNumber(t[1]); break;
			case "lfoY":
				var lfonames = "gvm";
				patch.lfo[lfonames.indexOf(t[1])] = asNumber(t.slice(2)); break;
			case "mlfo": patch.mlfo = asNumber(t.slice(1)); break;
			case "egY":
				var i = (t[1] == 'o') ? "0" : t[1];
				patch.eg[parseInt(i)] = asNumber(t.slice(2)); break;
			case "modmix": patch.modmix = asNumber(t.slice(1)); break;
		}
	}

	this.setBank = function (ibank, inbank)
	{
		var lines = inbank.split('\n');
		var state = 0;
		var patch = {};
		var bank = [];
		for (var n=0; n<lines.length; n++)
		{
			if (lines[n] == "WhySynth patch format 0 begin")
			{
				patch = { osc:[], vcf:[], lfo:[], eg:[] };
				state = 1;
			}
			else if (lines[n] == "WhySynth patch end")
			{
				bank.push(patch);
				state = 0;
			}
			else if (state == 1)
				parseLine(lines[n], patch);
		}
		
		this.patchNames = [];
		for (var i=0; i<bank.length; i++)
			this.patchNames.push(bank[i].name);
		
		this.base.setBank.call(this, ibank, bank);
	}
	
	if (!path) path = Whysynth.prototype.path;
	else Whysynth.prototype.path = path;
	host.createPlugin(this, path + "whysynth.nmf");
}

Whysynth.prototype = new DAWPlugin();
Whysynth.prototype.constructor = Whysynth;
Whysynth.prototype.base = DAWPlugin.prototype;
Whysynth.prototype.path = DAWPlugin.prototype.getPath();
