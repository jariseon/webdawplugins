//
//  ladspa.hh
//
//  Created by Jari Kleimola on 26/10/14.
//
//

#ifndef ____ladspa_hh__
#define ____ladspa_hh__

#include "dawbundle.h"
#include "dawplugin.h"
#include <ladspa.h>

typedef const LADSPA_Descriptor*	LadspaDescriptor;
typedef const LADSPA_PortDescriptor* LadspaPortDescriptor;
const int LADSPA_MAXCHANNELS = 2;

class LadspaBundle : public DAWBundle
{
public:
	virtual DAWPlugin* createPlugin(int descID);
	virtual const char* getDescriptorString(int index);
	virtual float getDefaultValue(void* hint);
protected:
	virtual const void* getDescriptor(int i);
	virtual void disposePlugin(void* plug);
	void buildDescriptorString(LadspaDescriptor desc, std::stringstream& ss);
	const char* getDefaultType(LADSPA_PortRangeHint hint);
	int getScaler(LADSPA_PortRangeHint hint);
	std::string m_strdesc;
};

class LadspaPlugin : public DAWPlugin
{
public:
	LadspaPlugin(DAWBundle* bundle);
	virtual bool init(const void* desc, int sr, int buflen);
	void exit();
	virtual void activate(bool active = true);
	virtual void process();
	virtual int numOutputs() { return m_ocount; }
	virtual void setParameter(int index, float value);
	LADSPA_Data* getInputBuffer(int channel);
	LADSPA_Data* getOutputBuffer(int channel);
	virtual void setFrameLength(int frameLength);

protected:
	void clearout();
	int m_frameLength;
	LADSPA_Handle m_handle;
	LADSPA_Data* m_ibufs[LADSPA_MAXCHANNELS];
	LADSPA_Data* m_obufs[LADSPA_MAXCHANNELS];
	
private:
	int m_icount;
	int m_ocount;
	int m_byteLength;
	bool m_active;
	LadspaDescriptor m_desc;
	LADSPA_Data* m_portdata;
};

#endif
