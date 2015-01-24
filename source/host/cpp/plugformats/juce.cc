//
//  juce.cc
//  
//
//  Created by Jari Kleimola on 09/12/14.
//
//

#include "juce.hh"
#include "JuceHeader.h"
#include "utils.h"
#include <sstream>
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

extern "C" { void jslog(int); void jslogs(const char*); }


// ------------------------------------------------------------------------------
// entry point
//
DAWPlugin* createDAWPlugin(int frameLength, int sr)
{
	juce::AudioProcessor* proc = createPluginFilter();
	if (!proc) return nullptr;
	
	JucePlugin* plug = new JucePlugin();
	if (plug->init(proc, frameLength, sr))
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
	((JucePlugin*)plug)->dispose();
}


// ==============================================================================

JucePlugin::JucePlugin() : DAWPlugin(nullptr) { }

bool JucePlugin::init(juce::AudioProcessor* proc, int frameLength, int sr)
{
	m_proc = proc;
	m_icount = iclamp(proc->getNumInputChannels(), 0, 2);
	m_ocount = iclamp(proc->getNumOutputChannels(), 0, 2);
	
	m_audiobufs = new AudioSampleBuffer(2, frameLength); // m_icount + m_ocount, frameLength);
	m_midibuf = new MidiBuffer();
	proc->prepareToPlay(sr, frameLength);
	
	return true;
}

void JucePlugin::dispose()
{
	m_proc->releaseResources();
	delete m_audiobufs;
	delete m_midibuf;
}

const char* JucePlugin::getDescriptor()
{
	std::stringstream json("{\"descriptor\":{");
	json.seekp(0, std::ios::end);
	
	if (m_proc)
	{
		json << "\"numberOfInputs\":" << m_proc->getNumInputChannels();
		json << ",\"numberOfOutputs\":" << m_proc->getNumOutputChannels();
		json << ",\"params\":[";

		int numParams = m_proc->getNumParameters();
		bool paramsInserted = false;
		for (int i=0; i<numParams; i++)
		{
			if (paramsInserted) json << ","; else paramsInserted = true;
			json << "{\"id\":" << i << ",";
			json << "\"name\":\"" << m_proc->getParameterName(i).toRawUTF8() << "\"";
			json << "}";
		}
		json << "]";
	}
	json << "}}";
	
	m_desc = json.str();
	return m_desc.c_str();
}

void JucePlugin::activate(bool active)
{
}

Plugin_Data* JucePlugin::getInputBuffer(int channel)
{
	if (channel >= m_icount) return nullptr;
	else return m_audiobufs->getSampleData(channel);	// OBxd uses this
	// else return m_audiobufs->getWritePointer(channel);		// Dexed uses this
}

Plugin_Data* JucePlugin::getOutputBuffer(int channel)
{
	if (channel >= m_ocount) return nullptr;
	else return m_audiobufs->getSampleData(channel + m_icount);		// OBxd uses this
	// else return (Plugin_Data*)m_audiobufs->getReadPointer(channel + m_icount);		// Dexed uses this
}

void JucePlugin::process()
{
	m_proc->processBlock(*m_audiobufs, *m_midibuf);
	m_midibuf->clear();
}

void JucePlugin::onMidi(unsigned char status, unsigned char data1, unsigned char data2)
{
	if ((status & 0xF0) == 0xC0)
		m_proc->setCurrentProgram(data1);
	else if (status != 0xF0)
	{
		unsigned char msg[3] = { status, data1, data2 };
		m_midibuf->addEvent(msg, 3, 0);
	}
}

void JucePlugin::getPatchNames(std::vector<std::string>& names, int)
{
	int numPatches = m_proc->getNumPrograms();
	for (int i=0; i<numPatches; i++)
	{
		String s = m_proc->getProgramName(i);
		names.push_back(std::string(s.toRawUTF8()));
	}
}

#ifdef PNACL
void JucePlugin::setBank(int32_t ibank, pp::VarArrayBuffer patches)
{
	void* data = patches.Map();

	// -- fxb
	int32_t* p = (int32_t*)data;
	if (*p == 0x4b6e6343)	// CcnK
	{
		p += 2;
		if (*p != 0x68434246) return;	p += 2;	// FBCh
	//	if (*p != 0x32435363) return;	p += 2;	// cSC2
	//	if (*p != 0x80000000) return;	p += 2;	// 128 programs
		data = ((char*)data) + 0xA0;				// magic: 0x21324356
		int sizeInBytes = patches.ByteLength() - 0xA0;

		// -- swap magic and string length to little endian
		p = (int32_t*)data;
		*p = ByteOrder::swap((uint32)*p); p++;
		*p = ByteOrder::swap((uint32)*p);
		
		m_proc->setStateInformation(data, sizeInBytes);
		return;
	}
	
	// -- sysex
	uint8_t* p2 = (uint8_t*)data;
	if (*p2 == 0xF0)
	{
		int length = patches.ByteLength();
		if (*(p2+length-1) != 0xF7) return;
		MidiMessage msg(data, length);
		MidiInputCallback* midiHandler = dynamic_cast<MidiInputCallback*>(m_proc);
		if (midiHandler) {
			midiHandler->handleIncomingMidiMessage(nullptr, msg);
		}
	}
}
#endif

void JucePlugin::setPatch(TDict& data, int32_t ipatch, int32_t ibank)
{
	TBlob patch;
	int type = data["type"]->asInt();

	// -- aupreset
	if (type == 1635085685)			// aumu
	{
		patch = data["jucePluginState"]->asBlob();
		if (!patch.data) return;
	}
	// -- vst
	else if (type == 0x43636E4B)	// CcnK
	{
		if (data["subtype"]->asInt() != 0x46504368) return;	// FPCh
		patch = data["data"]->asBlob();
		if (!patch.data) return;
	}
	else return;
	
	// -- swap magic and string length to little endian
	uint32_t* p = (uint32_t*)patch.data;
	*p = ByteOrder::swap((uint32)*p); p++;
	*p = ByteOrder::swap((uint32)*p);

	m_proc->setCurrentProgramStateInformation(patch.data, patch.length);
}

void JucePlugin::setParameter(int index, float value)
{
	m_proc->setParameter(index, value);
}


#if FALSE
// JS posts a blob, parsing done in PNaCl
void JucePlugin::setPatch(void* data, int sizeInBytes, int32_t ipatch, int32_t ibank)
{
	int32_t* p = (int32_t*)data;

	// -- VST
	if (*p == 0x4b6e6343) // CcnK
	{
		p += 2;
		if (*p != 0x68435046) return;	p += 2;	// FPCh
		data = ((char*)data) + 0x3C;				// magic: 0x21324356
		sizeInBytes -= 0x3C;
	}
	// -- AUPreset
	else
	{
		bool found = false;
		std::vector<unsigned char> v;
		XmlElement* doc = XmlDocument::parse(String::fromUTF8(static_cast<const char*>(data), sizeInBytes));
		XmlElement* plist = doc->getFirstChildElement();
		forEachXmlChildElementWithTagName(*plist, key, "key")
		{
			if (key->getAllSubText() == "jucePluginState")
			{
				found = true;
				XmlElement* value = key->getNextElement();
				String content = value->getAllSubText();
				std::string s = content.toStdString();
				
				std::basic_string<char> c;
				for (int i=0; i<s.size(); i++)
					if (s[i] >= 0x21) c += (char)s[i];

				v = base64Decode(c);
				data = v.data();
				sizeInBytes = v.size();
			}
		}
		if (!found) return;
	}
	
	// -- swap magic and string length to little endian
	p = (int32_t*)data;
	*p = ByteOrder::swap((uint32)*p); p++;
	*p = ByteOrder::swap((uint32)*p);

	m_proc->setCurrentProgramStateInformation(data, sizeInBytes);
}
#endif
