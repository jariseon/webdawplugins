//
//  utils.cpp
//  
//
//  Created by Jari Kleimola on 11/12/14.
//
//

#include "utils.h"
#include <string>

std::string TVariant::m_defstr;
TBlob TVariant::m_defblob = { 0, 0 };

int iclamp(int v, int min, int max)
{
	if (v < min) v = min;
	if (v > max) v = max;
	return v;
}

#define TCHAR char    //Not unicode
#define TEXT(x) x     //Not unicode
#define DWORD long
#define BYTE unsigned char

const static TCHAR padCharacter = TEXT('=');
std::vector<unsigned char> base64Decode(const std::basic_string<char>& input)
{
	if (input.length() % 4) //Sanity check
		throw std::runtime_error("Non-Valid base64!");
	size_t padding = 0;
	if (input.length())
	{
		if (input[input.length()-1] == padCharacter)
			padding++;
		if (input[input.length()-2] == padCharacter)
			padding++;
	}
	//Setup a vector to hold the result
	std::vector<BYTE> decodedBytes;
	decodedBytes.reserve(((input.length()/4)*3) - padding);
	DWORD temp=0; //Holds decoded quanta
	std::basic_string<TCHAR>::const_iterator cursor = input.begin();
	while (cursor < input.end())
	{
		for (size_t quantumPosition = 0; quantumPosition < 4; quantumPosition++)
		{
			temp <<= 6;
			if       (*cursor >= 0x41 && *cursor <= 0x5A) // This area will need tweaking if
				temp |= *cursor - 0x41;		              // you are using an alternate alphabet
			else if  (*cursor >= 0x61 && *cursor <= 0x7A)
				temp |= *cursor - 0x47;
			else if  (*cursor >= 0x30 && *cursor <= 0x39)
				temp |= *cursor + 0x04;
			else if  (*cursor == 0x2B)
				temp |= 0x3E; //change to 0x2D for URL alphabet
			else if  (*cursor == 0x2F)
				temp |= 0x3F; //change to 0x5F for URL alphabet
			else if  (*cursor == padCharacter) //pad
			{
				switch( input.end() - cursor )
				{
				case 1: //One pad character
					decodedBytes.push_back((temp >> 16) & 0x000000FF);
					decodedBytes.push_back((temp >> 8 ) & 0x000000FF);
					return decodedBytes;
				case 2: //Two pad characters
					decodedBytes.push_back((temp >> 10) & 0x000000FF);
					return decodedBytes;
				default:
					throw std::runtime_error("Invalid Padding in Base 64!");
				}
			}  else
				throw std::runtime_error("Non-Valid Character in Base 64!");
			cursor++;
		}
		decodedBytes.push_back((temp >> 16) & 0x000000FF);
		decodedBytes.push_back((temp >> 8 ) & 0x000000FF);
		decodedBytes.push_back((temp      ) & 0x000000FF);
	}
	return decodedBytes;
}