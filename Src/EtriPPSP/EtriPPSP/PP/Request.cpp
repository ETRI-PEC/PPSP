#include "Request.h"
#include <string.h>
#include "../Common/Util.h"

Request::Request()
{
	MessageType = PP_MESSAGETYPE_REQUEST;
}

Request::~Request()
{

}

int Request::Create(char* data)
{
	int idx = 0;

	memcpy(&DestinationChannelID, data, 4);
	DestinationChannelID = CalcEndianN2H(DestinationChannelID);
	idx += 4;
	LogPrint(LOG_LEVEL_DEBUG, "DestinationChannelID : %d\n", DestinationChannelID);
	
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

char* Request::GetBuffer(int* len)
{
	*len = 4 + 1 + 4 + 4;

	Buffer = new char[*len];

	memset(Buffer, 0, *len);

	int idx = 0, tmpl = 0;

	tmpl = CalcEndianH2N(DestinationChannelID);
	memcpy(Buffer, &tmpl, 4);
	idx += 4;

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