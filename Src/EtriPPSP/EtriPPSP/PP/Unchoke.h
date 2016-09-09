#pragma once
#include "PPMessage.h"

#define PP_MESSAGETYPE_UNCHOKE 0x0b

class Unchoke : public PPMessage
{
public:
	Unchoke();
	~Unchoke();

	int Create(char* data);
	char* GetBuffer(int *len);
};