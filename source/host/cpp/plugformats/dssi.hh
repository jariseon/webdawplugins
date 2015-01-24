//
//  dssi.hh
//
//  Created by Jari Kleimola on 26/10/14.
//
//

#ifndef ____dssi_hh__
#define ____dssi_hh__

#include "ladspa.hh"
#include <dssi.h>

typedef const DSSI_Descriptor*	DssiDescriptor;
const int DSSI_EVENTBUFSIZE = 128;

#ifdef PNACL
typedef void (*FSetPatch)(LADSPA_Handle Instance, int32_t ibank, int32_t ipatch, pp::Var data);
typedef void (*FSetBank)(LADSPA_Handle Instance, int32_t ibank, pp::VarArray data);
typedef struct
{
	FSetPatch setPatch;
	FSetBank setBank;
} TWebExtension;
#endif


class MidiEvent
{
public:
	MidiEvent(unsigned char s, unsigned char d1, unsigned char d2)
	{
		status = s;
		data1 = d1;
		data2 = d2;
	}
	unsigned char status;
	unsigned char data1;
	unsigned char data2;
};

class DssiBundle : public LadspaBundle
{
public:
	virtual DAWPlugin* createPlugin(int descID);
	virtual const char* getDescriptorString(int index);
protected:
	virtual const void* getDescriptor(int i);
	virtual void disposePlugin(void* plug);
};

class DssiPlugin : public LadspaPlugin
{
public:
	DssiPlugin(DAWBundle* bundle) : LadspaPlugin(bundle) {}
	virtual bool init(const void* desc, int sr, int frameLength);
	virtual void process();
	virtual void onMidi(unsigned char status, unsigned char data1, unsigned char data2);
#ifdef PNACL
	virtual void setPatch(int32_t ibank, int32_t ipatch, pp::Var values);
	virtual void setBank(int32_t ibank, pp::VarArray values);
#endif
	void selectProgram(long ibank, long iprogram);
private:
	void processEvents();
	DssiDescriptor m_desc;
	int m_numEvents;
	long m_icurbank;
	long m_icurprogram;
	std::vector<MidiEvent> m_midibuf;
	snd_seq_event_t m_events[DSSI_EVENTBUFSIZE];
};

#endif
