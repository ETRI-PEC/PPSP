#include "Cancel.h"
#include <string.h>
#include "../Common/Util.h"

Cancel::Cancel()
{
	MessageType = PP_MESSAGETYPE_CANCEL;
}

Cancel::~Cancel()
{

}

int Cancel::Create(char* data)
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

char* Cancel::GetBuffer(int* len)
{
	char buf[1024];
	memset(buf, 0, 1024);

	int idx = 0, tmpl = 0;

	tmpl = CalcEndianH2N(DestinationChannelID);
	memcpy(buf, &tmpl, 4);
	idx += 4;

	memcpy(buf + idx, &MessageType, 1);
	idx += 1;

	tmpl = CalcEndianH2N(StartChunk);
	memcpy(buf + idx, &tmpl, 4);
	idx += 4;

	tmpl = CalcEndianH2N(EndChunk);
	memcpy(buf + idx, &tmpl, 4);
	idx += 4;
	
	if (Buffer != 0) delete[] Buffer;

	Buffer = new char[idx];

	memset(Buffer, 0, idx);
	memcpy(Buffer, buf, idx);

	*len = idx;

	return Buffer;
}