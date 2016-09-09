#pragma once
#include "PPMessage.h"
#include <string>

#define PP_MESSAGETYPE_PEXRESV4 0x05

class PEXRes : public PPMessage
{
public:
	PEXRes();
	~PEXRes();

	int IPv4Address;
	std::string IPv4AddressString;
	short Port;

	int Create(char* data);
	char* GetBuffer(int* len);
};

