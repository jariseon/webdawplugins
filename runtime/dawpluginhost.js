var DAWPluginHost = function (actx)
{
	this.actx = actx;
	var modules = {};		// associates module src first with plugin id, then with module
	var pending = [];		// plugins waiting to be created while module is still loading
	var plugins = [];
	var currID = 0;
	var reqID = 0;
	var self = this;
	
	this.load = function (filename, directOut)
	{
		console.log("load is deprecated, use createPlugin()");
	}
	
	this.createPlugin = function (jsplug, src, index)
	{
		plugins[++currID] = jsplug;
		jsplug.id = currID;
		jsplug.index = index || 0;
		jsplug.isPNaCl = src.indexOf(".nmf", src.length - 4) !== -1;
		jsplug.host = this;
		
		// -- build full path
		var name = src;
		if (name.indexOf("http://") != 0)
		{
			var root = location.href.slice(0, location.href.lastIndexOf("/"));
			name = root + "/" + name;
		}
		jsplug.fullName = name;
		
		// -- load module or create new instance
		var module = modules[name];
		if (!module)
		{
			modules[name] = currID;
			module = inject(src, jsplug);
			if (module) jsplug.module = module;
		}
		else
		{
			// module already loaded
			if (typeof module == "object")
			{
				jsplug.module = module;
				jsplug.create();
			}
			else	// module still loading
			{
				if (!pending[name]) pending[name] = [];
				pending[name].push(jsplug);
			}
		}
	}
	
	this.disposePlugin = function (instance, modulename)
	{
		for (var i=0; i<plugins.length; i++)
			if (plugins[i] && plugins[i].inst == instance) { plugins.splice(i,1); break; }
		if (modulename)
		{
			if (modules[modulename]) delete modules[modulename];
			if (instance.isPNaCl)
			{
				var embeds = document.querySelectorAll("embed");
				for (var i=0; i<embeds.length; i++)
					if (embeds[i].src == modulename)
						document.body.removeChild(embeds[i]);	// devtools shows an error here
			}
			else
			{
				var scripts = document.querySelectorAll("script");
				for (var i=0; i<scripts.length; i++)
					if (scripts[i].src == modulename)
					{
						if (Module) {delete Module; Module = null; }
						scripts[i].parentElement.removeChild(scripts[i]); 
					}
			}
		}
	}

	function inject (src, instance)
	{
		if (instance.isPNaCl)
		{
			var embed = document.createElement("embed");
			embed.setAttribute("src", src);
			embed.setAttribute("width", 0);
			embed.setAttribute("height", 0);
			embed.setAttribute("type", "application/x-pnacl");
			embed.addEventListener("load", onModuleLoaded, true);
			embed.addEventListener("message", instance.onmessage.bind(instance), true);
			document.body.appendChild(embed);
			return embed;
		}
		else
		{
			var script = document.createElement("script");
			script.onload = onModuleLoaded;
			script.src = src;
			document.head.appendChild(script);
			return script;
		}
	}

	// -------------------------------------------------------------------------
	// messaging and events

	this.getMessageID = function () { return ++reqID; }
	
	// asm.js script or pnacl module has been loaded
	// note that this is called only once per script/module
	function onModuleLoaded(e)
	{
		// -- resolve plugin id
		if (e.url) var name = e.url;
		else var name = e.target.src;
		var dot = name.lastIndexOf(".");
		var tail = name.slice(dot);
		if (tail == ".pexe") name = name.slice(0,dot) + ".nmf"; 
		var id = modules[name];
		modules[name] = e.path[0];
		
		if (id)
		{
			var plug = plugins[id];
			if (!plug.isPNaCl)	// todo: how to load multiple emscripten modules ?
				DAWPlugin.prototype.c_createPlugin = Module.cwrap("createPlugin", "string", ["number","number","number"]);
			plug.create();
			if (pending[name])
			{
				pending[name].forEach(function(plug) { plug.create(); });
				pending.splice(pending.indexOf(pending[name]), 1);
			}
		}
	}
	
	
	// =========================================================================
	// PRESET HANDLING
	// =========================================================================
	
	this.presets = { presetPath: "" };
	
	// -------------------------------------------------------------------------
	//
	this.presets.loadPatch = function (patch, pathprefix)
	{
		var self = this;
		return new Promise(function (resolve, reject) {
			var path = self.getPath(patch, pathprefix); 
			if (patch.type == "fxp") var format = "arraybuffer";
			else if (patch.type == "aupreset") var format = "string";
			else reject("unknown type");
			load(path, format).then( function (data)
				{
					var dict;
					if (patch.type == "fxp") dict = parseVST(data);
					else dict = parseAU(data);
					resolve(dict);
				},
				function (err) { reject(err); }) });
	};
	
	// -------------------------------------------------------------------------
	// currently all patches (per synth) need to be under a common root
	//
	this.presets.loadDB = function (path)
	{
		return new Promise(function (resolve, reject) {
			load(path, "string").then( function (json)
			{
				var dict = JSON.parse(json);
				dict.type = "dir";

				// -- parse item filename into name and type, and set path for dirs
				function parse(item)
				{
					if (typeof item == "object")
						return { name:item.name, type:"dir", path:item.path };
					else if (typeof item == "string")
					{
						var dot = item.lastIndexOf(".");
						if (dot >= 0)
						{
							var type = item.substr(dot+1);
							if (type != "dir")
								return { name:item.substr(0,dot), type:type };
						}
					}
					return undefined;
				}

				// -- iterate 'container' recursively, mutating children with parse()
				function iterate(container, arr, path)
				{
					var tree = container.items;
					for (var i=0; i<tree.length; i++)
					{
						var item = tree[i];
						var entry = parse(item);
						if (entry)
						{
							if (entry.type == "dir")
							{
								entry.path = path + "/" + entry.path;
								arr = [];
								iterate(item, arr, entry.path);
								entry.items = arr;
							}
							else
							{
								entry.path = path;
								arr.push(entry);
							}
							tree[i] = entry;
						}
						else tree[i] = undefined;
					}
				}
				
				iterate(dict, [], dict.root);
				resolve(dict);
			},
			function (err) { reject(err); }) });
	};
	
	// -------------------------------------------------------------------------
	// if item.path begins with "http://" it is treated as an absolute path
	// otherwise, if prefix is given, it overrides this.presetPath
	//
	this.presets.getPath = function (item, prefix)
	{
		var path = item.path + "/" + item.name + "." + item.type;
		if (path.indexOf("http") != 0)
		{
			if (prefix) path = prefix + path;
			else path = this.presetPath + path;
		}
		return path;
	};
	
	function load(url, responseType)
	{
		return new Promise(function (resolve, reject) {
			var xhr = new XMLHttpRequest();
			xhr.onload = function ()
			{
				if (xhr.status == 200) resolve(xhr.response);
				else reject(xhr.statusText);
			}
			xhr.onerror = function () { reject(Error("load error")); };
			xhr.onabort = function () { reject(Error("load aborted")); };
			if (responseType) xhr.responseType = responseType;
			xhr.open("get", url, true);
			xhr.send();
			});
	}

	function parseAU (resp)
	{
		var dict = {};
		var parser = new DOMParser();
		var doc = parser.parseFromString(resp, "text/xml");
		if (doc.doctype.name == "plist")
		{
			var keys = doc.getElementsByTagName("key");
			for (var k=0; k<keys.length; k++)
			{
				var key = keys[k].textContent;
				var node = keys[k].nextElementSibling;
				var value = node.textContent;
				switch (node.nodeName)
				{
					case "string": break;
					case "integer": value = parseInt(value); break;
					case "data":
						var bin = atob(value);
						var len = bin.length;
						value = new Uint8Array(len);
						for (var i=0; i<len; i++)
							value[i] = bin.charCodeAt(i);
						value = value.buffer;
						break;
				}
				dict[key] = value;
			}
		}
		return dict;
	}
	
	function parseVST (resp)
	{
		var dict = {};
		var dv = new DataView(resp);
		dict.type = dv.getUint32(0, false);			// CcnK 0x43636E4B
		dict.subtype = dv.getUint32(8, false);		// FPCh 0x46504368
		dict.id = dv.getUint32(16, false);
		dict.version = dv.getUint32(20, false);
		dict.count = dv.getUint32(24, false);

		if (dict.subtype == 0x46504368)
		{
			var str = new Array(28);
			for (var i=0; i<28; i++)
				str[i] = dv.getUint8(28+i);
			dict.name = String.fromCharCode.apply(null, str);
			var len1 = dv.getUint32(0x38, false);
			var len2 = resp.byteLength - 0x3C;
			var length = Math.min(len1, len2);
			var arr = new Uint8Array(resp, 0x3C, length);
			dict.data = arr.buffer.slice(0x3C, 0x3C + length);
		}
		return dict;
	}
}