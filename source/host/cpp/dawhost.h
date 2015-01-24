//
//  dawhost.h
//
//  Created by Jari Kleimola on 11/10/14.
//

#ifndef ____dawhost__
#define ____dawhost__

#include "dawbundle.h"
typedef unsigned char byte;

class DAWHost
{
// -- ctor/dtor
public:
	explicit DAWHost();
	virtual ~DAWHost();
	
// -- members
public:
	DAWPlugin* getPlugin(int) { return m_plug; }		// todo: use vector
	uint32_t getFrameLength() { return m_frameLength; }
	void setFrameLength(uint32_t length);
	float getTempo() { return m_tempo; }
protected:
	DAWPlugin* m_plug;
	uint32_t m_frameLength;	// in samples
	float m_tempo;	// in bpm
#if defined(LADSPA) || defined(DSSI)
	DAWBundle* m_bundle;
#endif

// -- implementation
public:
#if defined(LADSPA) || defined(DSSI)
	const char* initBundle(int index, int frameLength);
	bool createPlugin(int plugid, int bundleIndex=0);
#elif defined(VST) || defined(JUCE)
	bool createPlugin(int plugid, int frameLength, int sr);
#endif
	void setParam(int index, float value);
	bool onProcess();
	void onMidi(byte status, byte data1, byte data2);
	void* getInputBuffer(int i);
	void* getOutputBuffer(int i);
	int getNumOutputs();
};

#endif
