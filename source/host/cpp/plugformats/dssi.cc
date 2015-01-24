//
//  dssi.cc
//  
//  Created by Jari Kleimola on 26/10/14.
//
//

#include "dssi.hh"
#include <dssi.h>
#include <sstream>


#ifdef LADSPA
const void* dssi_descriptor(int i) { return 0; }
#endif

typedef const DSSI_Descriptor*	DssiDescriptor;

FSetPatch extSetPatch = 0;
FSetBank extSetBank = 0;

TWebExtension webext;



void DssiBundle::disposePlugin(void* plug) { delete (DssiPlugin*)plug; }

const void* DssiBundle::getDescriptor(int i)
{
	return dssi_descriptor(i);
}

const char* DssiBundle::getDescriptorString(int index)
{
	std::stringstream json("{\"descriptor\":");
	json.seekp(0, std::ios::end);

	if (0 <= index && index < m_descs.size())
	{
		DssiDescriptor desc = (DssiDescriptor)m_descs[index];
		buildDescriptorString(desc->LADSPA_Plugin, json);
	}
	
	json << "}";
	m_strdesc = json.str();
	return m_strdesc.c_str();
}

DAWPlugin* DssiBundle::createPlugin(int descID)
{
	DssiDescriptor desc = (DssiDescriptor)m_descs[descID];
	if (desc)
	{
		DssiPlugin* plug = new DssiPlugin(this);
		if (plug->init(desc, m_sr, m_frameLength))
		{
			plug->id = m_plugs.size();
			plug->activate();
			m_plugs.push_back(plug);
			return plug;
		}
		else delete plug;
	}
	return 0;
}

// ===============

bool DssiPlugin::init(const void* desc, int sr, int frameLength)
{
	m_desc = (DssiDescriptor)desc;
	LadspaDescriptor ladspa = m_desc->LADSPA_Plugin;
	bool ret = LadspaPlugin::init(ladspa, sr, frameLength);
	
	// -- programs
	if (ret)
	{
	}
	
	// -- parameters

	// -- web extras
	if (m_desc->configure)
		(*(m_desc->configure))(m_handle, "webconfig", (const char*)&webext);
		// (*(m_desc->configure))(m_handle, "webconfig", (const char*)&extSetPatch);

	m_icurbank = 0;
	m_icurprogram = 0;
	m_numEvents = 0;
	return ret;
}

void DssiPlugin::selectProgram(long ibank, long iprogram)
{
	if (m_desc->select_program)
	{
		if (ibank < 0) ibank = m_icurbank;
		else m_icurbank = ibank;
		m_icurprogram = iprogram;
		m_desc->select_program(m_handle, ibank, iprogram);
	}
}

void DssiPlugin::process()
{
	processEvents();

	if (m_desc->run_synth)
		(*(m_desc->run_synth))(m_handle, m_frameLength, m_events, m_numEvents);
	else if (m_desc->run_synth_adding)
	{
		clearout();
		(*(m_desc->run_synth_adding))(m_handle, m_frameLength, m_events, m_numEvents);
	}
	else if (m_desc->run_multiple_synths)
	{
		LADSPA_Handle insts[1] = { m_handle };
		snd_seq_event_t* pevents = m_events;
		unsigned long nevents = (unsigned long)m_numEvents;
		(*(m_desc->run_multiple_synths))(1, insts, m_frameLength, &pevents, &nevents);
	}
	else LadspaPlugin::process();
}

// todo: currently omni
void DssiPlugin::processEvents()
{
	m_numEvents = m_midibuf.size();
	for (int i=0; i<m_numEvents && i<DSSI_EVENTBUFSIZE; i++)
	{
		MidiEvent e = m_midibuf[i];
		switch (e.status & 0xF0)
		{
			case 0x80:
			{
				m_events[i].type = SND_SEQ_EVENT_NOTEOFF;
				m_events[i].data.note.channel = e.status & 0x0F;
				m_events[i].data.note.note = e.data1;
				m_events[i].data.note.velocity = e.data2;
				break;
			}
			case 0x90:
			{
				m_events[i].type = SND_SEQ_EVENT_NOTEON;
				m_events[i].data.note.channel = e.status & 0x0F;
				m_events[i].data.note.note = e.data1;
				m_events[i].data.note.velocity = e.data2;
				break;
			}
			case 0xC0:
			{
				selectProgram(-1, e.data1);
				continue;
			}
			default: continue;
		}
		m_events[i].time.tick = 0;
	}
	m_midibuf.clear();
}

void DssiPlugin::onMidi(unsigned char status, unsigned char data1, unsigned char data2)
{
	m_midibuf.push_back(MidiEvent(status,data1,data2));
}

void DssiPlugin::setPatch(int32_t ibank, int32_t ipatch, pp::Var values)
{
	if (webext.setPatch)
		(*webext.setPatch)(m_handle, ibank, ipatch, values);
}

void DssiPlugin::setBank(int32_t ibank, pp::VarArray values)
{
	if (webext.setBank)
		(*webext.setBank)(m_handle, ibank, values);
}
