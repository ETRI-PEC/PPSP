#pragma once
#include "PPMessage.h"

#define PP_MESSAGETYPE_CANCEL 0x09

class Cancel : public PPMessage
{
public:
	Cancel();
	~Cancel();

	int StartChunk;
	int EndChunk;

	int Create(char* data);
	char* GetBuffer(int *len);
};