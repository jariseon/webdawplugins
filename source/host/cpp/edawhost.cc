//
//  edawhost.cc
//  
//
//  Created by Jari Kleimola on 01/11/14.
//
//

#include "dawhost.h"
DAWHost host;

extern "C"
{

//	const char* initBundle(int bufsize) { return host.initBundle(bufsize, true); }
//	int createPlug(int index) { return host.createPlugin(index); }

const char* createPlugin(int plugid, int index, int frameLength)
{
#if defined(LADSPA) || defined(DSSI)
	const char* pdesc = host.initBundle(index, frameLength);
	bool success = host.createPlugin(plugid, index);
	if (!success) pdesc = "";
	return pdesc;
#elif defined(VST)
	const char* pdesc = "";
	bool success = host.createPlugin(plugid, frameLength, 44100);
	if (success)
	{
		DAWPlugin* plug = host.getPlugin(plugid);
		pdesc = plug->getDescriptor();
	}
	return pdesc;
#endif
}

void* getBuffer(int plugid, int type, int channel)
{
	DAWPlugin* plug = host.getPlugin(plugid);
	if (!plug || channel < 0 || channel >= 2) return 0;
	if (type == 0) return plug->getInputBuffer(channel);
	else if (type == 1) return plug->getOutputBuffer(channel);
	else return 0; // host.midibuf;
}
void process() { host.onProcess(); }

void onMidi(int status, int data1, int data2)
{
	host.onMidi(status, data1, data2);
}

void setParam(int index, float value) { host.setParam(index, value); }
	
};

