#pragma once
#include "PPMessage.h"

#define PP_MESSAGETYPE_PEXREQ 0x06

class PEXReq : public PPMessage
{
public:
	PEXReq();
	~PEXReq();

	int Create(char* data);
	char* GetBuffer(int* len);
};

