#pragma once
#include "PPMessage.h"

#define PP_MESSAGETYPE_CHOKE 0x0a

class Choke : public PPMessage
{
public:
	Choke();
	~Choke();

	int Create(char* data);
	char* GetBuffer(int *len);
};