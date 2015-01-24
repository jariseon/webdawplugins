//
//  pdawhost.h
//  
//
//  Created by Jari Kleimola on 01/11/14.
//
//

#ifndef ____pdawhost__
#define ____pdawhost__

#include "dawhost.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/audio.h"
#include "ppapi/cpp/var_dictionary.h"


class PDAWHost : public pp::Instance
{
public:
	explicit PDAWHost(PP_Instance instance);
	virtual ~PDAWHost() {}
protected:
	virtual void HandleMessage(const pp::Var& var_message);
private:
	void createPlugin(int msgid, pp::VarDictionary& args, pp::VarDictionary& ret);

	void process(pp::VarArray& bufs, pp::VarArray& outbufs);
	static void onProcessDirect(void* samples, uint32_t bufsize, void* data);
	pp::Audio m_audio;
	int m_direct;
	int m_waaFrameLength;
	int m_directFrameLength;
};

class PDAWHostModule : public pp::Module
{
public:
	PDAWHostModule() : pp::Module() {}
	~PDAWHostModule() {}
	virtual pp::Instance* CreateInstance(PP_Instance instance) { return new PDAWHost(instance); }
};

namespace pp
{
	Module* CreateModule() { return new PDAWHostModule(); }
}

#endif /* defined(____pdawhost__) */
