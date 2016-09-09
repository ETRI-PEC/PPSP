#include "Data.h"
#include <string.h>
#include "../Common/Util.h"

Data::Data()
{
	ChunkSize = 0;
	DataBuffer = 0;
	BufferSize = 0;

	StartChunk = 0;
	EndChunk = 0;
	Timestamp = 0;

	MessageType = PP_MESSAGETYPE_DATA;
}


Data::~Data()
{
	if (DataBuffer != 0)
		delete[] DataBuffer;
}

int Data::Create(char* data, int len)
{
	int idx = 0;

	memcpy(&DestinationChannelID, data , 4);
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

	memcpy(&Timestamp, data + idx, 8);
	Timestamp = CalcEndianN2H(Timestamp);
	idx += 8;

	LogPrint(LOG_LEVEL_DEBUG, "Timestamp : %lld\n", Timestamp);

	if (len > 0)
	{
		BufferSize = len - idx;
		DataBuffer = new char[BufferSize];
		memcpy(DataBuffer, data + idx, BufferSize);
		idx += BufferSize;
	}
	else
	{
		DataBuffer = new char[ChunkSize * (EndChunk - StartChunk + 1)];
		memcpy(DataBuffer, data + idx, ChunkSize * (EndChunk - StartChunk + 1));
		idx += ChunkSize * (EndChunk - StartChunk + 1);
	}

	return idx;
}

char* Data::GetBuffer(int *len)
{
	int idx = 0, tmpl = 0;

	if (DataBuffer == 0)
	{
		*len = 0;
		return 0;
	}

	if (Buffer != 0) delete[] Buffer;

	if (BufferSize > 0)
	{
		*len = 4 + 1 + 4 + 4 + 8 + BufferSize;
	}
	else
	{
		*len = 4 + 1 + 4 + 4 + 8 + (ChunkSize * (EndChunk - StartChunk + 1));
	}

	Buffer = new char[*len];

	memset(Buffer, 0, *len);

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

	if (Timestamp == 0)
	{
		Timestamp = GetCurrentTimeMicrosecond();
	}

	unsigned long long tmpll = CalcEndianH2N(Timestamp);
	memcpy(Buffer + idx, &tmpll, 8);
	idx += 8;

	if (BufferSize > 0)
	{
		memcpy(Buffer + idx, DataBuffer, BufferSize);
	}
	else
	{
		memcpy(Buffer + idx, DataBuffer, ChunkSize * (EndChunk - StartChunk + 1));
	}
	
	return Buffer;
}