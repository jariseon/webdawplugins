//
//  dawbundle.h
//
//  Created by Jari Kleimola on 17/10/14.
//
//

#ifndef ____dawbundle__
#define ____dawbundle__

#include "dawplugin.h"
#include <vector>
#include <string>

class DAWBundle
{
public:
	DAWBundle() {}
	virtual ~DAWBundle() {}
	virtual int init(int sr, int frameLength);
	virtual void exit();
	virtual const char* getDescriptorString(int index) = 0;
	virtual DAWPlugin* createPlugin(int descID) = 0;
	virtual float getDefaultValue(void* hint) { return 0; }
	int getSampleRate() { return m_sr; }
	int getFrameLength() { return m_frameLength; }
	void setFrameLength(int frameLength) { m_frameLength = frameLength; }
protected:
	virtual const void* getDescriptor(int i) = 0;
	virtual void disposePlugin(void* plug) = 0;
	int m_sr;
	int m_frameLength;
	std::vector<const void*> m_descs;
	std::vector<void*> m_plugs;
};

#endif
