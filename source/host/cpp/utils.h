//
//  utils.h
//  
//
//  Created by Jari Kleimola on 11/12/14.
//
//

#ifndef ____utils__
#define ____utils__

#include <vector>
#include <map>
#include <string>

int iclamp(int v, int min, int max);
std::vector<unsigned char> base64Decode(const std::basic_string<char>& input);

extern "C" { void jslog(int); void jslogs(const char*); }

struct TBlob {
	void* data;
	int length;
};

class TVariant
{
public:
	TVariant() { m_itype = 0; }
	TVariant(int i) { m_itype = 1; m_int = i; }
	TVariant(std::string s) { m_itype = 2; m_data = new std::string(s); }
	TVariant(void* data, int length) { m_itype = 3; m_blob.data = data; m_blob.length = length; }
	
	~TVariant()
	{
		if (m_itype == 2) delete ((std::string*)m_data);
		else if (m_itype == 3) delete [] ((unsigned char*)m_blob.data);
	}
	
	int asInt() { return (m_itype == 1) ? m_int : 0; }
	std::string& asString() { return (m_itype == 2) ? *((std::string*)m_data) : m_defstr; }
	TBlob& asBlob() { return (m_itype == 3) ? m_blob : m_defblob; }
private:
	int m_itype;
	union {
		int m_int;
		void* m_data;
		TBlob m_blob;
	};
	static std::string m_defstr;
	static TBlob m_defblob;
};

class TDict
{
public:
	TDict() {}
	~TDict()
	{
		std::map<std::string, TVariant*>::iterator it;
		for (it = m_map.begin(); it != m_map.end(); it++)
			delete (TVariant*)it->second;
	}
	void bind(const char* key, TVariant* value) { m_map[std::string(key)] = value; }
	void bind(std::string key, TVariant* value) { m_map[key] = value; }
	TVariant* operator [] (const char* p) { std::string s(p); return safeget(s); }
	TVariant* operator [] (std::string s) { return safeget(s); }
private:
	TVariant* safeget(std::string s)
	{
		TVariant* v = m_map[s];
		if (!v) v = &m_empty;
		return v;
	}
	std::map<std::string, TVariant*> m_map;
	TVariant m_empty;
};

// typedef std::map<std::string, TVariant*> TDict;

#endif /* defined(____utils__) */
