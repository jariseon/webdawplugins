//
//  pdawhost.cc
//  
//
//  Created by Jari Kleimola on 01/11/14.
//
//

#include "pdawhost.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include <time.h>

DAWHost host;

extern "C"
{
	PDAWHost* ginst = 0;
	void jslog(int i)
	{
		char log[128];
		sprintf(log, "PNaCl %d", i);
		if (ginst) ginst->PostMessage(pp::Var(log));
	}
	void jslogs(const char* s)
	{
		if (ginst) ginst->PostMessage(pp::Var(s));
	}
}

int getInt(const pp::VarDictionary& dict, const char* key, int def)
{
	int ivalue = def;
	pp::Var value = dict.Get(pp::Var(key));
	if (value.is_int()) ivalue = value.AsInt();
	return ivalue;
}

bool getDict(pp::Var& v, TDict& d)
{
	if (!v.is_dictionary()) return false;
	pp::VarDictionary dict(v);
	pp::VarArray keys = dict.GetKeys();
	for (int i=0; i<keys.GetLength(); i++)
	{
		TVariant* var;
		pp::Var key = keys.Get(i);
		pp::Var value = dict.Get(key);
		if (value.is_int()) var = new TVariant(value.AsInt());
		else if (value.is_string()) var = new TVariant(value.AsString());
		else if (value.is_array_buffer())
		{
			pp::VarArrayBuffer ab(value);
			int length = ab.ByteLength();
			unsigned char* buf = new unsigned char [length];
			memcpy(buf, (unsigned char*)ab.Map(), length);
			var = new TVariant(buf, length);
		}
		else continue;
		d.bind(key.AsString(), var);
	}
	return true;
}

// ---------------------------------------------------------------------------
//
PDAWHost::PDAWHost(PP_Instance instance) : pp::Instance(instance)
{
	ginst = this;
	m_direct = -1;					// -1: needs init, 0: uses web audio api, 1: direct out
	m_waaFrameLength = 0;		// for Web Audio API : default 512
	m_directFrameLength = 0;	// for direct out : default 128
}

// ---------------------------------------------------------------------------
// marshals and dispatches messages received from JS
//
void PDAWHost::HandleMessage(const pp::Var& dict)
{
	pp::Var data("data");
	pp::VarDictionary msg(dict);
	std::string verb = msg.Get(pp::Var("verb")).AsString();
	std::string prop = msg.Get(pp::Var("resource")).AsString();
	pp::Var content = msg.Get(data);
	// int msgid = getInt(msg, "id", 0);
		
	// ----
	// -- process plugid (Array samplebuffers)
	// ----
	if (verb == "process")
	{
		pp::VarArray inbufs(content);
		pp::VarArray outbufs;
			process(inbufs, outbufs);
		msg.Set(data, outbufs);
		PostMessage(msg);
	}
	
	// ----
	// -- pub midi (Array midimsg)
	// ----
	else if (verb == "pub" && prop == "midi")
	{
		pp::VarArray bytes(content);
		int status = bytes.Get(0).AsInt();
		int data1 = bytes.Get(1).AsInt();
		int data2 = bytes.Get(2).AsInt();
			host.onMidi(status, data1, data2);
	}
	
	// ----
	// -- set param|patch|bank|directOut|active (data)
	// ----
	else if (verb == "set")
	{
		pp::VarDictionary args(content);
		
		// -- param(int index, var value)
		if (prop == "param")
		{
			pp::Var index = args.Get(pp::Var("index"));
			pp::Var value = args.Get(pp::Var("value"));
			if (index.is_number() && value.is_number())
				host.setParam(index.AsInt(), value.AsDouble());
		}
		
		// -- patch(int ibank, int ipatch, Dict|Array|ArrayBuffer value)
		else if (prop == "patch")
		{
			int plugid = getInt(msg, "plugid", -1);
			DAWPlugin* plug = host.getPlugin(plugid);
			if (!plug) return;
			
			int32_t ibank = getInt(args, "bank", 0);
			int32_t ipatch = getInt(args, "patch", 0);
			pp::Var value(args.Get(pp::Var("value")));
			
			TDict dict;
			if (getDict(value, dict))
				plug->setPatch(dict, ipatch, ibank);
			else if (value.is_array_buffer())
			{
				pp::VarArrayBuffer patch(value);
				plug->setPatch(patch.Map(), patch.ByteLength(), ipatch, ipatch);
			}
			else
			{
				pp::VarArray patch(value);
				plug->setPatch(ibank, ipatch, patch);
			}
		}
		
		// -- bank(int ibank, Array|ArrayBuffer data)
		else if (prop == "bank")
		{
			int plugid = getInt(msg, "plugid", -1);
			DAWPlugin* plug = host.getPlugin(plugid);
			if (!plug) return;
			
			int32_t ibank = getInt(args, "bank", 0);
			pp::Var value = args.Get(pp::Var("value"));
			if (value.is_array_buffer())
			{
				pp::VarArrayBuffer bank(value);
				plug->setBank(ibank, bank);
			}
			else
			{
				pp::VarArray bank(value);
				plug->setBank(ibank, bank);
			}
		}
		
		// -- active (bool onoff) : only for directOut mode
		else if (prop == "active")
		{
			bool onoff = args.Get(data).AsBool();
			if (onoff) m_audio.StartPlayback();
			else m_audio.StopPlayback();
		}
		
		// -- directOut (bool yesno)
		else if (prop == "directOut")
		{
			bool direct = content.AsBool(); // args.Get(data).AsBool();
			if (!direct)
			{
			jslog(0);
				m_audio.StopPlayback();
				m_direct = 0;
				host.setFrameLength(m_waaFrameLength);
			}
			else
			{
			jslog(1);
				if (m_direct < 0)
				{
					int frameLength = pp::AudioConfig::RecommendSampleFrameCount(this, PP_AUDIOSAMPLERATE_44100, 128);
					m_audio = pp::Audio(this, pp::AudioConfig(this, PP_AUDIOSAMPLERATE_44100, frameLength), onProcessDirect, this);
					m_directFrameLength = frameLength;
				}
				m_direct = 1;
				host.setFrameLength(m_directFrameLength);
				m_audio.StartPlayback();
			}
		}
	}
	
	// ----
	// -- set param|patch|bank|directOut|active (data)
	// ----
	else if (verb == "get")
	{
		pp::VarDictionary args(content);
		
		// -- patchNames(int bank)
		if (prop == "patchNames")
		{
			pp::VarArray arr;
			int plugid = getInt(msg, "plugid", -1);
			DAWPlugin* plug = host.getPlugin(plugid);
			if (plug)
			{
				std::vector<std::string> names;
				int ibank = getInt(args, "bank", 0);
				plug->getPatchNames(names, ibank);
				
				int ilength = names.size();
				arr.SetLength(ilength);
				for (int i=0; i<ilength; i++)
					arr.Set(i, pp::Var(names[i]));
			}
			msg.Set(data, arr);
		}
		else
			msg.Set(data, pp::Var(false));
		
		msg.Set(pp::Var("verb"), pp::Var("reply"));
		PostMessage(msg);
	}
	
	// ----
	// -- create plugin (int plugid, int index, int frameLength)
	// ----
	else if (verb == "create" && prop == "plugin")
	{
		int plugid = getInt(msg, "plugid", -1);
		pp::VarDictionary resp;
		pp::VarDictionary args(content);
			createPlugin(plugid, args, resp);
		msg.Set(data, resp);
		PostMessage(msg);
	}
}

// ---------------------------------------------------------------------------
// create(int plugid, int index, int frameLength)
//
void PDAWHost::createPlugin(int plugid, pp::VarDictionary& args, pp::VarDictionary& resp)
{
	int frameLength = getInt(args, "frameLength", 0);
	m_waaFrameLength = frameLength | 512;
	
	#if defined(LADSPA) || defined(DSSI)
		int index = getInt(args, "index", 0);
		const char* pdesc = host.initBundle(index, frameLength);
		bool success = host.createPlugin(plugid, index);
	#else
		const char* pdesc = "{}";
		bool success = host.createPlugin(plugid, frameLength, 44100);
		if (success)
		{
			DAWPlugin* plug = host.getPlugin(plugid);
			pdesc = plug->getDescriptor();
		}
	#endif

	resp.Set(pp::Var("status"), pp::Var(success));
	resp.Set(pp::Var("desc"), pp::Var(pdesc));
}
int xx = 0;
// ---------------------------------------------------------------------------
// DSP
//
void PDAWHost::process(pp::VarArray& bufs, pp::VarArray& outbufs)
{
	uint32_t length = bufs.GetLength();
	if (length == 4)
	{
		// -- inputs
		for (int i=0; i<2; i++)
		{
			Plugin_Data* inbuf = (Plugin_Data*)host.getInputBuffer(i);
			if (!inbuf) break;
			pp::VarArrayBuffer ivab(bufs.Get(i));
			int ibuflen = ivab.ByteLength();
			float* ibuf = static_cast<float*>(ivab.Map());
			memcpy(inbuf, ibuf, ibuflen);
			ivab.Unmap();
		}
		
		host.onProcess();

		// -- outputs
		int numouts = host.getNumOutputs();
		if (numouts > 2) numouts = 2;
		outbufs.SetLength(numouts);

		for (int i=0; i<numouts; i++)
		{
			Plugin_Data* outbuf = (Plugin_Data*)host.getOutputBuffer(i);
			if (!outbuf) break;
			pp::VarArrayBuffer ovab(bufs.Get(i+2));
			int obuflen = ovab.ByteLength();
			float* obuf = static_cast<float*>(ovab.Map());
			if (obuf)
				memcpy(obuf, outbuf, obuflen);
			ovab.Unmap();
			outbufs.Set(i, ovab);
		}
	}
}

// ---------------------------------------------------------------------------
//
void PDAWHost::onProcessDirect(void* samples, uint32_t bufsize, void* data)
{
	int16_t* outbuf = reinterpret_cast<int16_t*>(samples);

	if (!host.onProcess())
	{
		memset(outbuf, 0, sizeof(int16_t) * bufsize);
		return;
	}
	
	uint32_t nsamples = host.getFrameLength();
	int numouts = host.getNumOutputs();
	if (numouts > 2) numouts = 2;
	
	if (numouts == 2)
	{
		float* outbufL = (float*)host.getOutputBuffer(0);
		float* outbufR = (float*)host.getOutputBuffer(1);
		for (uint32_t n=0; n<nsamples; n++)
		{
			int16_t yL = static_cast<int16_t>(outbufL[n] * 32767);
			int16_t yR = static_cast<int16_t>(outbufR[n] * 32767);
			*outbuf++ = yL;
			*outbuf++ = yR;
		}
	}
	else if (numouts == 1)
	{
		float* outbufL = (float*)host.getOutputBuffer(0);
		for (uint32_t n=0; n<nsamples; n++)
		{
			double y = *outbufL++;
			int16_t scal = static_cast<int16_t>(y * 32767);
			*outbuf++ = scal;
			*outbuf++ = scal;
		}
	}
	else memset(outbuf, 0, sizeof(int16_t) * bufsize);
}


