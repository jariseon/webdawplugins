//
//  vst.hh
//  
//
//  Created by Jari Kleimola on 04/12/14.
//
//

#ifndef ____vst_hh__
#define ____vst_hh__

#include "dawbundle.h"
#include "audioeffectx.h"
#include <vector>

struct VSTDescriptor
{
	AudioEffect* aef;
	AEffect* ae;
};

struct VstEventBlock
{
	static const int MAXEVENTS = 1024;
	VstInt32 numEvents;
	VstIntPtr reserved;
	VstEvent* events[MAXEVENTS];
};

/* class VSTBundle : public DAWBundle
{
public:
	VSTBundle();
	virtual std::string getDescriptorString();
	virtual DAWPlugin* createPlugin(int descID);
protected:
	virtual const void* getDescriptor(int i);
	virtual void disposePlugin(void* plug);
private:
	AEffect* createInstance();
	// VSTDescriptor m_desc;
	AEffect* m_desc;
}; */

class VSTPlugin : public DAWPlugin
{
public:
	VSTPlugin();
	void dispose();
	bool init(AEffect* desc, AudioEffect* aef, int frameLength, int sr);
	virtual const char* getDescriptor();
	
	virtual void activate(bool active = true);
	virtual int numOutputs() { return m_ocount; }
	virtual Plugin_Data* getInputBuffer(int channel);
	virtual Plugin_Data* getOutputBuffer(int channel);
	virtual void onMidi(unsigned char status, unsigned char data1, unsigned char data2);
	virtual void process();
	virtual void getPatchNames(std::vector<std::string>& names, int ibank=0);
	virtual void setParameter(int index, float value);
#ifdef PNACL
	virtual void setBank(int32_t ibank, pp::VarArray patches);
	virtual void setBank(int32_t ibank, pp::VarArrayBuffer data);
#endif
private:
	VstIntPtr dispatch(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
	void clearout();
	AEffect* m_ae;
	AudioEffect* m_aef;
	
	std::string m_strdesc;
	bool m_canReplace;
	int m_frameLength;
	int m_byteLength;
	int m_icount;
	int m_ocount;
	Plugin_Data* m_ibufs[2];
	Plugin_Data* m_obufs[2];
	VstMidiEvent m_midiEvents[VstEventBlock::MAXEVENTS];
	VstEventBlock m_events;
	// std::vector<VstEvent*> m_eventbuf;	// overflows
};

#endif /* defined(____vst__) */
