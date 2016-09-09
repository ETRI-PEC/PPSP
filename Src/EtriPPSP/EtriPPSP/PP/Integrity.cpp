#include "Integrity.h"
#include "HandShake.h"
#include "../Common/Util.h"
#include "../SHA/sha1.h"
#include "../SHA/sha256.h"

Integrity::Integrity(char mhf)
{
	MessageType = PP_MESSAGETYPE_INTEGRITY;
	StartChunk = 0;
	EndChunk = 0;
	Hash = 0;

	MerkleHashTreeFunction = mhf;
}


Integrity::~Integrity()
{
	if (Hash != 0)
		delete[] Hash;
}

void Integrity::SetHash(unsigned char* hash)
{
	if (MerkleHashTreeFunction == PP_HANDSHAKE_VALUE_MHF_SHA1)
	{
		Hash = new unsigned char[SHA1_DIGEST_BLOCKLEN];
		memset(Hash, 0, SHA1_DIGEST_BLOCKLEN);

		memcpy(Hash, hash, SHA1_DIGEST_BLOCKLEN);
	}
	else if (MerkleHashTreeFunction == PP_HANDSHAKE_VALUE_MHF_SHA256)
	{
		Hash = new unsigned char[SHA256_DIGEST_BLOCKLEN];
		memset(Hash, 0, SHA256_DIGEST_BLOCKLEN);

		memcpy(Hash, hash, SHA256_DIGEST_BLOCKLEN);
	}
}

int Integrity::Create(char* data)
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

	if (MerkleHashTreeFunction == PP_HANDSHAKE_VALUE_MHF_SHA1)
	{
		Hash = new unsigned char[SHA1_DIGEST_BLOCKLEN];
		memset(Hash, 0, SHA1_DIGEST_BLOCKLEN);

		memcpy(Hash, data + idx, SHA1_DIGEST_BLOCKLEN);
		idx += SHA1_DIGEST_BLOCKLEN;
	}
	else if (MerkleHashTreeFunction == PP_HANDSHAKE_VALUE_MHF_SHA256)
	{
		Hash = new unsigned char[SHA256_DIGEST_BLOCKLEN];
		memset(Hash, 0, SHA256_DIGEST_BLOCKLEN);

		memcpy(Hash, data + idx, SHA256_DIGEST_BLOCKLEN);
		idx += SHA256_DIGEST_BLOCKLEN;
	}

	return idx;
}

char* Integrity::GetBuffer(int* len)
{
	if (Hash == 0)
	{
		*len = 0;
		return 0;
	}

	*len = 4 + 1 + 4 + 4;

	if (MerkleHashTreeFunction == PP_HANDSHAKE_VALUE_MHF_SHA1)
	{
		*len += SHA1_DIGEST_BLOCKLEN;
	}
	else if (MerkleHashTreeFunction == PP_HANDSHAKE_VALUE_MHF_SHA256)
	{
		*len += SHA256_DIGEST_BLOCKLEN;
	}

	if (Buffer != 0)
		delete[] Buffer;

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

	if (MerkleHashTreeFunction == PP_HANDSHAKE_VALUE_MHF_SHA1)
	{
		memcpy(Buffer + idx, Hash, SHA1_DIGEST_BLOCKLEN);
		idx += SHA1_DIGEST_BLOCKLEN;
	}
	else if (MerkleHashTreeFunction == PP_HANDSHAKE_VALUE_MHF_SHA256)
	{
		memcpy(Buffer + idx, Hash, SHA256_DIGEST_BLOCKLEN);
		idx += SHA256_DIGEST_BLOCKLEN;
	}
	
	return Buffer;
}
