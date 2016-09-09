#include "PPMessage.h"


PPMessage::PPMessage()
{
	Buffer = 0;
	DestinationChannelID = 0;
	MessageType = 0;
}


PPMessage::~PPMessage()
{
	if (Buffer != 0)
		delete[] Buffer;
}

int PPMessage::Create(char *buffer)
{
	buffer = 0;
	return 0;
}

char *PPMessage::GetBuffer(int *len)
{
	*len = 0;
	return 0;
}