#include <string.h>
#pragma once
class PPMessage
{
protected:
	char *Buffer;

public:
	PPMessage();
	virtual ~PPMessage();

	int DestinationChannelID;
	char MessageType;

	virtual int Create(char *buffer);
	virtual char * GetBuffer(int *len);
};

