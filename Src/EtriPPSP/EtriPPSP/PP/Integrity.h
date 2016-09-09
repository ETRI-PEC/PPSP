#pragma once
#include "PPMessage.h"

#define PP_MESSAGETYPE_INTEGRITY 0x04

class Integrity : public PPMessage
{
public:
	Integrity(char mhf);
	~Integrity();

	int StartChunk;
	int EndChunk;

	char MerkleHashTreeFunction;

	unsigned char* Hash;

	int Create(char* data);
	char* GetBuffer(int *len);
	void SetHash(unsigned char* hash);
};

