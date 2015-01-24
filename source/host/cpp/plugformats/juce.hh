//
//  juce.h
//  
//
//  Created by Jari Kleimola on 09/12/14.
//
//

#ifndef ____juce__
#define ____juce__

#include "dawbundle.h"
namespace juce {
	class AudioProcessor;
	class AudioSampleBuffer;
	class MidiBuffer;
	}

class JucePlugin : public DAWPlugin
{
public:
	JucePlugin();
	void dispose();
	bool init(juce::AudioProcessor* proc, int frameLength=512, int sr=44100);
	virtual const char* getDescriptor();
	virtual void activate(bool active = true);
	virtual int numOutputs() { return m_ocount; }
	virtual Plugin_Data* getInputBuffer(int channel);
	virtual Plugin_Data* getOutputBuffer(int channel);
	virtual void onMidi(unsigned char status, unsigned char data1, unsigned char data2);
	virtual void process();
	virtual void getPatchNames(std::vector<std::string>& names, int ibank=0);
#ifdef PNACL
	virtual void setBank(int32_t ibank, pp::VarArrayBuffer data);
#endif
	virtual void setPatch(TDict& data, int32_t ipatch = 0, int32_t ibank = 0);
	virtual void setParameter(int index, float value);
private:
	juce::AudioProcessor* m_proc;
	juce::AudioSampleBuffer* m_audiobufs;
	juce::MidiBuffer* m_midibuf;
	int m_icount;
	int m_ocount;
	std::string	m_desc;
};

#endif /* defined(____juce__) */
