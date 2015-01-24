//
//  dawhost.cc
//  
//
//  Created by Jari Kleimola on 11/10/14.
//
//

#include "dawhost.h"
#include <string>

#ifdef LADSPA
#include "ladspa.hh"
#elif defined(DSSI)
#include "dssi.hh"
#elif defined(VST)
#include "vst.hh"
#elif defined(JUCE)
#include "juce.hh"
#endif

extern "C" { void jslog(int); }

DAWHost* ghost = 0;

DAWHost::DAWHost()
{
	ghost = this;
	m_frameLength = 0;
	m_plug = 0;
	m_tempo = 120;
#if defined(LADSPA) || defined(DSSI)
	m_bundle = 0;
#endif
}

DAWHost::~DAWHost()
{
}

#if defined(LADSPA) || defined(DSSI)
const char* DAWHost::initBundle(int index, int frameLength)
{
	m_frameLength = frameLength;
	if (m_frameLength == 0) m_frameLength = 512;
	
	#ifdef LADSPA
		m_bundle = new LadspaBundle();
	#elif defined(DSSI)
		m_bundle = new DssiBundle();
	#endif
	
	if (!m_bundle) return "";
	m_bundle->init(44100, m_frameLength);
	return m_bundle->getDescriptorString(index);
}

bool DAWHost::createPlugin(int, int bundleIndex)
{
	if (!m_bundle) return 0;
	DAWPlugin* plug = m_bundle->createPlugin(bundleIndex);
	if (plug) m_plug = plug;
	return (plug != 0);
}

#elif defined(VST) || defined(JUCE)
bool DAWHost::createPlugin(int plugid, int frameLength, int sr)
{
	extern DAWPlugin* createDAWPlugin(int,int);
	DAWPlugin* plug = createDAWPlugin(frameLength, sr);
	if (plug) m_plug = plug;
	return (plug != 0);
}
#endif

void DAWHost::setParam(int index, float value)
{
	if (m_plug) m_plug->setParameter(index, value);
}

bool DAWHost::onProcess()
{
	if (m_plug) m_plug->process();
	return (m_plug != 0);
}

void DAWHost::onMidi(byte status, byte data1, byte data2)
{
	if (m_plug) m_plug->onMidi(status, data1, data2);
}

void* DAWHost::getInputBuffer(int i)  { if (m_plug) return m_plug->getInputBuffer(i); else return 0; }
void* DAWHost::getOutputBuffer(int i) { if (m_plug) return m_plug->getOutputBuffer(i); else return 0; }
int DAWHost::getNumOutputs() { if (m_plug) return m_plug->numOutputs(); else return 0; }

void DAWHost::setFrameLength(uint32_t frameLength)
{
	m_frameLength = frameLength;
	if (m_plug) m_plug->setFrameLength(frameLength);
#if defined(LADSPA) || defined(DSSI)
	if (m_bundle) m_bundle->setFrameLength(frameLength);
#endif
}