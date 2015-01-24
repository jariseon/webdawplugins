//
//  dawplugin.h
//
//  Created by Jari Kleimola on 26/10/14.
//
//

#ifndef ____dawplugin__
#define ____dawplugin__

#include "utils.h"

#ifdef PNACL
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_array_buffer.h"
#endif

class DAWBundle;
typedef float Plugin_Data;

class DAWPlugin
{
public:
	DAWPlugin(DAWBundle* bundle) { m_bundle = bundle; }
	virtual ~DAWPlugin() {}
	// virtual bool init(const void* desc, int sr, int frameLength) { return true; }
	virtual void activate(bool active = true) {}
	virtual void process() {}
	virtual int numOutputs() { return 0; }
	virtual Plugin_Data* getInputBuffer(int channel) { return 0; }
	virtual Plugin_Data* getOutputBuffer(int channel) { return 0; }
	virtual void setParameter(int index, float value) {}
	virtual void onMidi(unsigned char status, unsigned char data1, unsigned char data2) {}
	virtual void setFrameLength(int frameLength) {}
	virtual void getPatchNames(std::vector<std::string>& names, int ibank=0) {}
	virtual const char* getDescriptor() {return nullptr; }
#ifdef PNACL
	virtual void setPatch(int32_t ibank, int32_t ipatch, pp::Var values) {}
	virtual void setBank(int32_t ibank, pp::VarArray data) {}
	virtual void setBank(int32_t ibank, pp::VarArrayBuffer data) {}
#endif
	virtual void setPatch(void* data, int sizeInBytes, int32_t ipatch = 0, int32_t ibank = 0) {}
	virtual void setPatch(TDict& data, int32_t ipatch = 0, int32_t ibank = 0) {}
public:
	int id;
protected:
	DAWBundle* m_bundle;
};

#endif
