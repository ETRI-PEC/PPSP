#include "PEXReq.h"
#include "../Common/Util.h"

PEXReq::PEXReq()
{
	MessageType = PP_MESSAGETYPE_PEXREQ;
}

PEXReq::~PEXReq()
{
	
}

int PEXReq::Create(char* data)
{
	int idx = 0;

	memcpy(&DestinationChannelID, data, 4);
	DestinationChannelID = CalcEndianN2H(DestinationChannelID);
	idx += 4;

	LogPrint(LOG_LEVEL_DEBUG, "DestinationChannelID : %d\n", DestinationChannelID);

	idx++; // message type;

	return idx;
}

char* PEXReq::GetBuffer(int *len)
{
	int idx = 0, tmpl = 0;

	char buf[1024];
	memset(buf, 0, 1024);

	tmpl = CalcEndianH2N(DestinationChannelID);
	memcpy(buf, &tmpl, 4);
	idx += 4;

	memcpy(buf + idx, &MessageType, 1);
	idx += 1;

	if (Buffer != 0) delete[] Buffer;

	Buffer = new char[idx];

	memset(Buffer, 0, idx);
	memcpy(Buffer, buf, idx);

	*len = idx;

	return Buffer;
}