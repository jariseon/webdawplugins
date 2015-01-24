//
//  vst.cc
//  
//
//  Created by Jari Kleimola on 04/12/14.
//
//

#include "vst.hh"
#include "dawhost.h"
#include "audioeffectx.h"
#include <sstream>

// extern void* entrypoint(int& type);
extern "C" { extern void jslog(int); }

extern DAWHost* ghost;
static VstTimeInfo timeinfo;
static VstIntPtr VSTCALLBACK AMC(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	switch (opcode)
	{
		case audioMasterVersion:
			return 2400;
		case audioMasterGetTime:
			if (value & kVstTempoValid)
			{ timeinfo.tempo = ghost->getTempo(); timeinfo.flags = kVstTempoValid; return (VstIntPtr)&timeinfo; }
			break;
	}
	return 0;
}

/*
#if ENTRY == 1
AEffect* VSTBundle::createInstance()
{
	// typedef AEffect* (*VstMain)(audioMasterCallback);
	// m_desc = ((VstMain)createPlug)(AMC);
	extern AEffect* plugin_main(audioMasterCallback amc);
	return plugin_main(AMC);
}
#elif ENTRY == 2
AEffect* VSTBundle::createInstance()
{
	AudioEffect* aef = createEffectInstance(AMC);
	if (aef)
	{
		aef->dispatcher(effSetSampleRate, 0, 0, 0, m_sr);
		aef->dispatcher(effSetBlockSize, 0, m_frameLength, 0, 0);
		aef->dispatcher(effOpen, 0, 0, 0, 0);
		m_desc.aef = aef;
		m_desc.ae = aef->getAeffect();
	}
}
#endif

std::string VSTBundle::getDescriptorString()
{
	std::stringstream json("{\"descriptor\":[");
	json.seekp(0, std::ios::end);
	
	if (m_desc)
	{
		json << "{";
		json << "\"numberOfInputs\":" << m_desc->numInputs << ",\"numberOfOutputs\":" << m_desc->numOutputs;
		json << ",\"params\":[";
		
		char name[kVstMaxParamStrLen];
		bool paramsInserted = false;
		for (int i=0; i<m_desc->numParams; i++)
		{
			m_desc->dispatcher(m_desc, effGetParamName, i, 0, name, 0);
			
			if (paramsInserted) json << ","; else paramsInserted = true;
			json << "{\"id\":" << i << ",";
			json << "\"name\":\"" << name << "\"";
			
			// TODO
			// VstParameterProperties props;
			// if (m_desc.aef->dispatcher(effGetParameterProperties, i, 0, &props, 0))
			//{
			//	json << ",";
			//	json << "effGetParameterProperties ok,";
			//}
			
			json << "}";
		}
		
		json << "]}";
	}
	
	json << "]}";
	return json.str();
}

// =================

VSTBundle::VSTBundle() : DAWBundle() { m_desc = 0; }

const void* VSTBundle::getDescriptor(int i)
{
	if (i != 0) return 0;
	m_desc = createInstance();
	return m_desc;
}

DAWPlugin* VSTBundle::createPlugin(int descID)
{
	VSTPlugin* plug = new VSTPlugin(this);
	if (plug->init(m_desc))
	{
		plug->activate();
		m_plugs.push_back(plug);
		return plug;
	}
	return 0;
}

void VSTBundle::disposePlugin(void* plug) {} */

// =================

// ------------------------------------------------------------------------------
// entry point
//
DAWPlugin* createDAWPlugin(int frameLength, int sr)
{
	AudioEffect* aef = nullptr;
#if ENTRY == 1
	extern AEffect* plugin_main(audioMasterCallback);
	AEffect* ae = plugin_main(AMC);
	if (!ae) return nullptr;
#elif ENTRY == 2
	extern AudioEffect* createEffectInstance(audioMasterCallback);
	aef = createEffectInstance(AMC);
	if (!aef) return nullptr;
	AEffect* ae = aef->getAeffect();
#endif

	VSTPlugin* plug = new VSTPlugin();
	if (plug->init(ae, aef, frameLength, sr))
	{
		plug->activate();
		return plug;
	}
	return nullptr;
}

// ------------------------------------------------------------------------------
// exit point
//
void disposeDAWPlugin(void* plug)
{
	((VSTPlugin*)plug)->dispose();
}


// ==============================================================================

VSTPlugin::VSTPlugin() : DAWPlugin(nullptr) { m_ae = nullptr; m_aef = nullptr; }
void VSTPlugin::dispose() {}

VstIntPtr VSTPlugin::dispatch(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	return m_ae->dispatcher(m_ae, opcode, index, value, ptr, opt);
}

bool VSTPlugin::init(AEffect* ae, AudioEffect* aef, int frameLength, int sr)
{
	m_ae = ae;
	m_aef = aef;
	dispatch(effSetBlockSize, 0, frameLength, 0, 0);
	dispatch(effSetSampleRate, 0, 0, 0, sr);
	dispatch(effOpen, 0, 0, 0, 0);

	m_icount = iclamp(m_ae->numInputs, 0, 2);
	m_ocount = iclamp(m_ae->numOutputs, 0, 2);
	
	m_canReplace = true;
	m_frameLength = frameLength;
	m_byteLength = frameLength * sizeof(Plugin_Data);
	for (int i=0; i<m_icount; i++) m_ibufs[i] = new Plugin_Data[frameLength];
	for (int i=0; i<m_ocount; i++) m_obufs[i] = new Plugin_Data[frameLength];

	m_events.numEvents = 0;
	m_events.reserved = 0;
	memset(m_midiEvents, 0, VstEventBlock::MAXEVENTS * sizeof(VstMidiEvent));
	for (int i=0; i<VstEventBlock::MAXEVENTS; i++)
	{
		m_midiEvents[i].type = kVstMidiType;
		m_midiEvents[i].byteSize = sizeof(VstMidiEvent);
		m_events.events[i] = (VstEvent*)&m_midiEvents[i];
	}
	
	return true;
}

void VSTPlugin::activate(bool active)
{
	dispatch(effMainsChanged, 0, active ? 1 : 0, 0, 0);
}

const char* VSTPlugin::getDescriptor()
{
	std::stringstream json("{\"descriptor\":{");
	json.seekp(0, std::ios::end);
	
	if (m_ae)
	{
		json << "\"numberOfInputs\":" << m_ae->numInputs << ",\"numberOfOutputs\":" << m_ae->numOutputs;
		json << ",\"params\":[";
		
		char name[kVstMaxParamStrLen];
		bool paramsInserted = false;
		for (int i=0; i<m_ae->numParams; i++)
		{
			dispatch(effGetParamName, i, 0, name, 0);
			
			if (paramsInserted) json << ","; else paramsInserted = true;
			json << "{\"id\":" << i << ",";
			json << "\"name\":\"" << name << "\"";
			
			// TODO
			// VstParameterProperties props;
			/* if (dispatch(effGetParameterProperties, i, 0, &props, 0))
			{
				json << ",";
				json << "effGetParameterProperties ok,";
			} */
			
			json << "}";
		}
		
		json << "]";
	}
	
	json << "}}";
	m_strdesc = json.str();
	return m_strdesc.c_str();
}

Plugin_Data* VSTPlugin::getInputBuffer(int channel)
{
	if (channel >= m_icount) return 0;
	else return m_ibufs[channel];
}

Plugin_Data* VSTPlugin::getOutputBuffer(int channel)
{
	if (channel >= m_ocount) return 0;
	else return m_obufs[channel];
}

void VSTPlugin::clearout()
{
	for (int i=0; i<m_ocount; i++)
		memset(m_obufs[i], 0, m_byteLength);
}

void VSTPlugin::process()
{
	// -- events
	dispatch(effProcessEvents, 0, 0, (VstEvents*)&m_events, 0);
	m_events.numEvents = 0;

	// -- audio
	if (m_canReplace)
		m_ae->processReplacing(m_ae, m_ibufs, m_obufs, m_frameLength);
	else
	{
		clearout();
		m_ae->process(m_ae, m_ibufs, m_obufs, m_frameLength);
	}
}

void VSTPlugin::onMidi(unsigned char status, unsigned char data1, unsigned char data2)
{
	if (m_events.numEvents < VstEventBlock::MAXEVENTS)
	{
		VstMidiEvent& me = m_midiEvents[m_events.numEvents];
		me.midiData[0] = status;
		me.midiData[1] = data1;
		me.midiData[2] = data2;
		m_events.numEvents++;
	}
	// else TODO m_midibuf.push_back(MidiEvent(status,data1,data2));
}

void VSTPlugin::getPatchNames(std::vector<std::string>& names, int)
{
	char name[kVstMaxProgNameLen];
	int numPatches = m_ae->numPrograms;
	for (int i=0; i<numPatches; i++)
	{
		dispatch(effGetProgramNameIndexed, i, 0, name, 0);
		names.push_back(std::string(name));
	}
}

void VSTPlugin::setParameter(int index, float value)
{
	m_ae->setParameter(m_ae, index, value);
}

#ifdef PNACL
void VSTPlugin::setBank(int32_t ibank, pp::VarArray patches)
{
	const int maxBankSize = 8192;
	std::vector<std::string> names;
	
	// -- collect patch names and make sure all patches are of same size
	int numParams = 0;
	int numPatches = patches.GetLength();
	if (numPatches > m_ae->numPrograms) numPatches = m_ae->numPrograms;
	for (int i=0; i<numPatches; i++)
	{
		pp::VarArray patch(patches.Get(i));
		std::string s = patch.Get(0).AsString();
		names.push_back(s);
		if (numParams == 0) numParams = patch.GetLength();
		else if (patch.GetLength() != numParams) return;
	}

	// -- pass bank as an opaque chunk of floats
	int numBytes = numPatches * (numParams-1) * sizeof(float);
	if (numParams < maxBankSize)
	{
		int j = 0;
		float* chunk = new float[numBytes];
		for (int i=0; i<numPatches; i++)
		{
			pp::VarArray patch(patches.Get(i));
			for (int p=1; p<numParams; p++)
				chunk[j++] = (float)patch.Get(p).AsDouble();
		}
		dispatch(effSetChunk, 0, numBytes, chunk, 0);
		delete [] chunk;
	}
	
	// -- set patch names
	char name[kVstMaxProgNameLen+1];
	name[kVstMaxProgNameLen] = 0;
	for (int i=0; i<names.size(); i++)
	{
		std::string s = names[i];
		strncpy(name, s.c_str(), kVstMaxProgNameLen);
		dispatch(effSetProgramName, i, 0, name, 0);
	}
}

void VSTPlugin::setBank(int32_t ibank, pp::VarArrayBuffer patches)
{
	dispatch(effSetChunk, 0, patches.ByteLength(), patches.Map(), 0);
}
#endif

