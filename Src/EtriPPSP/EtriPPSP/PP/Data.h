#pragma once
#include "PPMessage.h"

#define PP_MESSAGETYPE_DATA 0x01

class Data : public PPMessage
{
public:
	Data();
	~Data();
	
	int StartChunk;
	int EndChunk;
	
	int ChunkSize;
	int BufferSize;

	unsigned long long Timestamp;
	char* DataBuffer;

	int Create(char* data, int len = 0);
	char* GetBuffer(int *len);
};