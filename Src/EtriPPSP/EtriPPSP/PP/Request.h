#pragma once
#include "PPMessage.h"

#define PP_MESSAGETYPE_REQUEST 0x08

class Request : public PPMessage
{
public:
	Request();
	~Request();

	int StartChunk;
	int EndChunk;

	int Create(char* data);
	char* GetBuffer(int *len);
};