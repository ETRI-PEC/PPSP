#pragma once
#include "PPMessage.h"

#define PP_MESSAGETYPE_HAVE 0x03

class Have : public PPMessage
{
public:
	Have();
	~Have();

	int StartChunk;
	int EndChunk;

	int Create(char* data, int isHandshakeHave = 0);
	char* GetBuffer(int *len, int isHandshakeHave = 0);
};

