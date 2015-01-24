//
//  ladspa.cc
//
//  Created by Jari Kleimola on 26/10/14.
//
//

#include "ladspa.hh"
#include <sstream>
#include <cmath>

const void* LadspaBundle::getDescriptor(int i) { return ladspa_descriptor(i); }
void LadspaBundle::disposePlugin(void* plug) { delete (LadspaPlugin*)plug; }

DAWPlugin* LadspaBundle::createPlugin(int descID)
{
	LadspaDescriptor desc = (LadspaDescriptor)m_descs[descID];
	if (desc)
	{
		LadspaPlugin* plug = new LadspaPlugin(this);
		if (plug->init(desc, m_sr, m_frameLength))
		{
			plug->id = m_plugs.size();
			plug->activate();
			m_plugs.push_back(plug);
			return plug;
		}
	}
	return 0;
}

const char* LadspaBundle::getDescriptorString(int index)
{
	std::stringstream json("{\"descriptor\":");
	json.seekp(0, std::ios::end);

	if (0 <= index && index < m_descs.size())
	{
		LadspaDescriptor desc = (LadspaDescriptor)m_descs[index];
		buildDescriptorString(desc, json);
	}
	
	json << "}";
	m_strdesc = json.str();
	return m_strdesc.c_str();
}

void LadspaBundle::buildDescriptorString(LadspaDescriptor desc, std::stringstream& ss)
{
	ss << "{";

	LadspaPortDescriptor portdesc = desc->PortDescriptors;
	
	// todo: how to separate source and modulation inputs ??
	int arInputs = 0;
	int arOutputs = 0;
	for (int i=0; i<desc->PortCount; i++)
	{
		if (LADSPA_IS_PORT_AUDIO(portdesc[i]))
		{
			if (LADSPA_IS_PORT_INPUT(portdesc[i])) arInputs++;
			else if (LADSPA_IS_PORT_OUTPUT(portdesc[i])) arOutputs++;
		}
	}
	ss << "\"numberOfInputs\":" << arInputs << ",\"numberOfOutputs\":" << arOutputs;
	
	ss << ",\"params\":[";
	bool paramsInserted = false;
	for (int i=0; i<desc->PortCount; i++)
	{
		if (LADSPA_IS_PORT_CONTROL(portdesc[i]) && LADSPA_IS_PORT_INPUT(portdesc[i]))
		{
			if (paramsInserted) ss << ","; else paramsInserted = true;
			ss << "{\"id\":" << i << ",";
			ss << "\"name\":\"" << desc->PortNames[i] << "\",";
			ss << "\"hint\":" << desc->PortRangeHints[i].HintDescriptor << ",";
			ss << "\"lowerbound\":" << desc->PortRangeHints[i].LowerBound << ",";
			ss << "\"upperbound\":" << desc->PortRangeHints[i].UpperBound << ",";
			ss << "\"defvalue\":" << getDefaultValue((void*)&desc->PortRangeHints[i]) << ",";
			ss << "\"deftype\":\"" << getDefaultType(desc->PortRangeHints[i]) << "\",";
			ss << "\"scaler\":" << getScaler(desc->PortRangeHints[i]) << "}";
		}
	}
	ss << "]}";
}

int LadspaBundle::getScaler(LADSPA_PortRangeHint hint)
{
	int scaler = 1;
	LADSPA_Data delta = hint.UpperBound - hint.LowerBound;
	if (delta > 1) scaler = 10;
	else scaler = 100;
	return scaler;
}

float LadspaBundle::getDefaultValue(void* phint)
{
	LADSPA_PortRangeHint* hint = (LADSPA_PortRangeHint*)phint;
	float v = 0;
	int h = hint->HintDescriptor;
	int d = hint->HintDescriptor & LADSPA_HINT_DEFAULT_MASK;
	bool weighted = false;
	
	if (d == LADSPA_HINT_DEFAULT_MINIMUM) v = hint->LowerBound;
	else if (d == LADSPA_HINT_DEFAULT_MAXIMUM) v = hint->UpperBound;
	else if (d == LADSPA_HINT_DEFAULT_0) v = 0;
	else if (d == LADSPA_HINT_DEFAULT_1) v = 1;
	else if (d == LADSPA_HINT_DEFAULT_100) v = 100;
	else if (d == LADSPA_HINT_DEFAULT_440) v = 440;

	else if (d == LADSPA_HINT_DEFAULT_LOW || d == LADSPA_HINT_DEFAULT_MIDDLE || d == LADSPA_HINT_DEFAULT_HIGH)
	{
		weighted = true;
		float lo = hint->LowerBound;
		float hi = hint->UpperBound;
		if (h & LADSPA_HINT_SAMPLE_RATE) { lo *= m_sr; hi *= m_sr; }
		if (h & LADSPA_HINT_LOGARITHMIC) { lo = log(lo); hi = log(hi); }
		if (d == LADSPA_HINT_DEFAULT_LOW) v = 0.75 * lo + 0.25 * hi;
		else if (d == LADSPA_HINT_DEFAULT_MIDDLE) v = 0.5 * lo + 0.5 * hi;
		else v = 0.25 * lo + 0.75 * hi;
		if (h & LADSPA_HINT_LOGARITHMIC) { v = exp(v); }
	}
	
	if (!weighted && h & LADSPA_HINT_SAMPLE_RATE) v *= m_sr;
	if (h & LADSPA_HINT_INTEGER) v = round(v);
	return v;
}

const char* LadspaBundle::getDefaultType(LADSPA_PortRangeHint hint)
{
	int h = hint.HintDescriptor;
	if (h & LADSPA_HINT_INTEGER) return "int";
	else if (h & LADSPA_HINT_LOGARITHMIC) return "log";
	else if (h & LADSPA_HINT_TOGGLED) return "bool";
	else return "float";
}

// =================

LadspaPlugin::LadspaPlugin(DAWBundle* bundle) : DAWPlugin(bundle)
{
	id = -1;
	m_active = false;
	m_icount = m_ocount = 0;
	m_portdata = 0;
	m_handle = 0;
	m_frameLength = bundle->getFrameLength();
	m_byteLength = m_frameLength * sizeof(LADSPA_Data);
	for (int i=0; i<LADSPA_MAXCHANNELS; i++)
		m_ibufs[i] = m_obufs[i] = 0;
}

bool LadspaPlugin::init(const void* d, int sr, int frameLength)
{
	LADSPA_Descriptor* desc = (LADSPA_Descriptor*)d;
	if (m_handle) return true;
	if (!desc->instantiate) return false;
	m_handle = (*(desc->instantiate))(desc, sr);
	if (m_handle)
	{
		m_desc = desc;
		m_frameLength = frameLength;
		m_portdata = new LADSPA_Data[desc->PortCount];
		for (int i=0; i<LADSPA_MAXCHANNELS; i++)
		{
			m_ibufs[i] = (LADSPA_Data*)malloc(frameLength * sizeof(LADSPA_Data)); // new LADSPA_Data[frameLength];
			m_obufs[i] = (LADSPA_Data*)malloc(frameLength * sizeof(LADSPA_Data)); // new LADSPA_Data[frameLength];
		}

		LadspaPortDescriptor portdesc = desc->PortDescriptors;
		for (int i=0; i<desc->PortCount; i++)
		{
			if (LADSPA_IS_PORT_CONTROL(portdesc[i]))
			{
				m_portdata[i] = m_bundle->getDefaultValue((void*)&desc->PortRangeHints[i]);
				(*(desc->connect_port))(m_handle, i, &m_portdata[i]);
			}
			else
			{
				if (LADSPA_IS_PORT_INPUT(portdesc[i]) && m_icount < LADSPA_MAXCHANNELS)
					(*(desc->connect_port))(m_handle, i, m_ibufs[m_icount++]);
				else if (LADSPA_IS_PORT_OUTPUT(portdesc[i]) && m_ocount < LADSPA_MAXCHANNELS)
					(*(desc->connect_port))(m_handle, i, m_obufs[m_ocount++]);
			}
		}
		return true;
	}
	return false;
}

void LadspaPlugin::exit()
{
	delete [] m_portdata;
	for (int i=0; i<LADSPA_MAXCHANNELS; i++)
	{
		free(m_ibufs[i]);		// delete [] m_ibufs[i];
		free(m_obufs[i]);		// delete [] m_obufs[i];
	}
}

void LadspaPlugin::clearout()
{
	for (int i=0; i<LADSPA_MAXCHANNELS; i++)
		memset(m_obufs[i], 0, m_byteLength);
}

void LadspaPlugin::activate(bool active)
{
	if (active && m_desc->activate) (*(m_desc->activate))(m_handle);
	else if (!active && m_desc->deactivate) (*(m_desc->deactivate))(m_handle);
	else return;
	m_active = active;
}

void LadspaPlugin::process()
{
	if (m_desc->run)
		(*(m_desc->run))(m_handle, m_frameLength);
	else if (m_desc->run_adding)
	{
		clearout();
		(*(m_desc->run_adding))(m_handle, m_frameLength);
	}
	else clearout();
}

LADSPA_Data* LadspaPlugin::getOutputBuffer(int channel)
{
	if (channel >= m_ocount) return 0;
	else return m_obufs[channel];
}

LADSPA_Data* LadspaPlugin::getInputBuffer(int channel)
{
	if (channel >= m_icount) return 0;
	else return m_ibufs[channel];
}

void LadspaPlugin::setParameter(int index, float value)
{
	if (0 <= index && index <= m_desc->PortCount)
		m_portdata[index] = value;
}

// todo: resize buffers
void LadspaPlugin::setFrameLength(int frameLength)
{
	m_frameLength = frameLength;
	m_byteLength = frameLength * sizeof(LADSPA_Data);
}
