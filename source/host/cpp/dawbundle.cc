//
//  dawbundle.cc
//
//  Created by Jari Kleimola on 17/10/14.
//
//

#include "dawbundle.h"

#ifdef DLLstubs
void _init() {};
void _fini() {};
#elif defined(DLLweb)
extern "C"
{
extern void web_init();
extern void web_fini();
}
#else
extern void _init();
extern void _fini();
#endif


int DAWBundle::init(int sr, int frameLength)
{
	m_sr = sr;
	m_frameLength = frameLength;
	
	#ifdef DLLweb
	web_init();
	#else
	_init();
	#endif
	
	int i = 0;
	const void* desc;
	while ((desc = getDescriptor(i++)))
		m_descs.push_back(desc);
	return m_descs.size();
}

void DAWBundle::exit()
{
	int nplugs = m_plugs.size();
	for (std::vector<void*>::size_type i = 0; i < nplugs; i++)
		disposePlugin(m_plugs[i]);
		
	#ifdef DLLweb
	web_fini();
	#else
	_fini();
	#endif
}



