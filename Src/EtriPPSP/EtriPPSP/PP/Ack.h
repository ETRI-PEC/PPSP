#pragma once
#include "PPMessage.h"

#define PP_MESSAGETYPE_ACK 0x02

class Ack : public PPMessage
{
public:
	Ack();
	~Ack();

	int StartChunk;
	int EndChunk;

	unsigned long long OnewayDelaySample;

	int Create(char* data);
	char* GetBuffer(int* len);
};

