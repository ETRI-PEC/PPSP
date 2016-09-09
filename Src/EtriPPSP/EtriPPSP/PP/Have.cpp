#include "Have.h"
#include <string.h>
#include "../Common/Util.h"

Have::Have()
{
	MessageType = PP_MESSAGETYPE_HAVE;
}


Have::~Have()
{
	
}

int Have::Create(char* data, int isHandshakeHave)
{
	int idx = 0;

	if (!isHandshakeHave)
	{
		memcpy(&DestinationChannelID, data, 4);
		DestinationChannelID = CalcEndianN2H(DestinationChannelID);
		idx += 4;
		LogPrint(LOG_LEVEL_DEBUG, "DestinationChannelID : %d\n", DestinationChannelID);
	}
	
	idx++; // message type;

	memcpy(&StartChunk, data + idx, 4);
	StartChunk = CalcEndianN2H(StartChunk);
	idx += 4;

	LogPrint(LOG_LEVEL_DEBUG, "StartChunk : %d\n", StartChunk);

	memcpy(&EndChunk, data + idx, 4);
	EndChunk = CalcEndianN2H(EndChunk);
	idx += 4;

	LogPrint(LOG_LEVEL_DEBUG, "EndChunk : %d\n", EndChunk);

	return idx;
}

char* Have::GetBuffer(int* len, int isHandshakeHave)
{
	*len = 4 + 1 + 4 + 4;

	if (isHandshakeHave)
	{
		*len -= 4;
	}	
	
	if (Buffer != 0)
		delete[] Buffer;

	Buffer = new char[*len];

	memset(Buffer, 0, *len);

	int idx = 0, tmpl = 0;
	
	if (!isHandshakeHave)
	{
		tmpl = CalcEndianH2N(DestinationChannelID);
		memcpy(Buffer, &tmpl, 4);
		idx += 4;
	}

	memcpy(Buffer + idx, &MessageType, 1);
	idx += 1;

	tmpl = CalcEndianH2N(StartChunk);
	memcpy(Buffer + idx, &tmpl, 4);
	idx += 4;

	tmpl = CalcEndianH2N(EndChunk);
	memcpy(Buffer + idx, &tmpl, 4);
	idx += 4;

	return Buffer;
}