#include "PEXRes.h"
#include "../Common/Util.h"

PEXRes::PEXRes()
{
	IPv4Address = 0;
	Port = 0;
	IPv4AddressString = "";

	MessageType = PP_MESSAGETYPE_PEXRESV4;
}

PEXRes::~PEXRes()
{
	
}

int PEXRes::Create(char* data)
{
	int idx = 0;

	memcpy(&DestinationChannelID, data, 4);
	DestinationChannelID = CalcEndianN2H(DestinationChannelID);
	idx += 4;

	LogPrint(LOG_LEVEL_DEBUG, "DestinationChannelID : %d\n", DestinationChannelID);

	idx++; // message type;

	memcpy(&IPv4Address, data + idx, 4);
	IPv4Address = CalcEndianN2H(IPv4Address);
	idx += 4;

	struct in_addr ip_addr;
	ip_addr.s_addr = IPv4Address;
	IPv4AddressString = inet_ntoa(ip_addr);

	LogPrint(LOG_LEVEL_DEBUG, "IPAddress : %s\n", IPv4AddressString.c_str());

	memcpy(&Port, data + idx, 2);
	Port = CalcEndianN2H(Port);
	idx += 2;

	LogPrint(LOG_LEVEL_DEBUG, "Port : %d\n", Port);

	return idx;
}

char* PEXRes::GetBuffer(int *len)
{
	int idx = 0, tmpl = 0;
	short tmps;

	char buf[1024];
	memset(buf, 0, 1024);

	tmpl = CalcEndianH2N(DestinationChannelID);
	memcpy(buf, &tmpl, 4);
	idx += 4;

	memcpy(buf + idx, &MessageType, 1);
	idx += 1;

	if (IPv4AddressString.length() > 0 && IPv4Address <= 0)
	{
		IPv4Address = inet_addr(IPv4AddressString.c_str());
	}

	tmpl = CalcEndianH2N(IPv4Address);
	memcpy(buf + idx, &tmpl, 4);
	idx += 4;

	tmps = CalcEndianH2N(Port);
	memcpy(buf + idx, &tmps, 2);
	idx += 2;

	if (Buffer != 0) delete[] Buffer;

	Buffer = new char[idx];

	memset(Buffer, 0, idx);
	memcpy(Buffer, buf, idx);

	*len = idx;

	return Buffer;
}