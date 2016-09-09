#include "HandShake.h"
#include <string.h>
#include <iostream>
#include "../Common/Util.h"

HandShake::HandShake()
{
	MessageType = PP_MESSAGETYPE_HANDSHAKE;

	SourceChannelID = 0;

	Version = 0x01;
	MinVersion = 0x01;
	SwarmIdentifierString = "";
	ContentIntegrityProtectionMethod = PP_HANDSHAKE_VALUE_CIPM_MERKLEHASHTREE;
	MerkleHashTreeFunction = PP_HANDSHAKE_VALUE_MHF_SHA1;
	LiveSignatureAlgorithm = PP_HANDSHAKE_VALUE_LSA_ECDSAP256SHA256;
	ChunkAddressingMethod = PP_HANDSHAKE_VALUE_CAM_32BITCHUNKRANGES;
	LiveDiscardWindow = 0;
	SupportedMessagesBitmap = 0;
	SupportedMessagesBitmapLength = 0;
	ChunkSize = 0;
	PeerListeningPort = 0;

	SwarmIdentifier = 0;

	HaveVersion = 0;
	HaveMinVersion = 0;
	HaveSwarmIdentifier = 0;
	HaveContentIntegrityProtectionMethod = 0;
	HaveMerkleHashTreeFunction = 0;
	HaveLiveSignatureAlgorithm = 0;
	HaveChunkAddressingMethod = 0;
	HaveLiveDiscardWindow = 0;
	HaveSupportedMessages = 0;
	HaveChunkSize = 0;
}

HandShake::HandShake(char* data)
{
	HandShake();

	Create(data);
}

HandShake::~HandShake()
{
	if (SupportedMessagesBitmap != 0)
		delete[] SupportedMessagesBitmap;

	if (LiveDiscardWindow != 0)
		delete[] LiveDiscardWindow;

	if (SwarmIdentifier != 0)
		delete[] SwarmIdentifier;
}

int HandShake::Create(char* data)
{
	short len = 0;
	int idx = 0;

	/*
	for (int i = 0; i < 1000; i++)
	{
		if (data[i] == 0xFF) break;

		int x = data[i];

		//std::cout.write(reinterpret_cast<const char*>(&x), sizeof x);
		printf("%s ", BitToStr(data + i, 1));
	}*/

	memcpy(&DestinationChannelID, data, 4);
	DestinationChannelID = CalcEndianN2H(DestinationChannelID);
	idx += 4;

	LogPrint(LOG_LEVEL_DEBUG, "DestinationChannelID : %d\n", DestinationChannelID);

	idx++; // message type;

	memcpy(&SourceChannelID, data + idx, 4);
	SourceChannelID = CalcEndianN2H(SourceChannelID);
	idx += 4;

	LogPrint(LOG_LEVEL_DEBUG, "SourceChannelID : %d\n", SourceChannelID);
	
	while ((unsigned char)data[idx] != PP_HANDSHAKE_CODE_ENDOPTION)
	{
		int unknown = 0;
		switch (data[idx])
		{
		case PP_HANDSHAKE_CODE_VERSION:
			HaveVersion = 1;
			idx++;
			Version = data[idx++];

			LogPrint(LOG_LEVEL_DEBUG, "Version : %d\n", Version);
			break;

		case PP_HANDSHAKE_CODE_MINIMUMVERSION:
			HaveMinVersion = 1;
			idx++;
			MinVersion = data[idx++];

			LogPrint(LOG_LEVEL_DEBUG, "MinVersion : %d\n", MinVersion);
			break;

		case PP_HANDSHAKE_CODE_SWARMIDENTIFIER:
			HaveSwarmIdentifier = 1;
			idx++;
			memcpy(&len, data + idx, 2);
			len = CalcEndianN2H(len);

			idx += 2;

			if (len > 0)
			{
				SwarmIdentifier = new unsigned char[len];
				memcpy(SwarmIdentifier, data + idx, len);

				SwarmIdentifierString = GetStringFromHash(SwarmIdentifier, len);
				idx += len;
			}
			LogPrint(LOG_LEVEL_DEBUG, "SwarmIdentifier Len : %d, %s\n", len, SwarmIdentifierString.c_str());
			break;

		case PP_HANDSHAKE_CODE_CONTENTINTEGRITYPROTECTIONMETHOD:
			HaveContentIntegrityProtectionMethod = 1;
			idx++;
			ContentIntegrityProtectionMethod = data[idx++];
			LogPrint(LOG_LEVEL_DEBUG, "ContentIntegrityProtectionMethod : %d\n", ContentIntegrityProtectionMethod);
			break;

		case PP_HANDSHAKE_CODE_MERKLEHASHTREEFUNCTION:
			HaveMerkleHashTreeFunction = 1;
			idx++;
			MerkleHashTreeFunction = data[idx++];
			LogPrint(LOG_LEVEL_DEBUG, "MerkleHashTreeFunction : %d\n", MerkleHashTreeFunction);
			break;

		case PP_HANDSHAKE_CODE_LIVESIGNATUREALGORITHM:
			HaveLiveSignatureAlgorithm = 1;
			idx++;
			LiveSignatureAlgorithm = data[idx++];
			LogPrint(LOG_LEVEL_DEBUG, "LiveSignatureAlgorithm : %d\n", LiveSignatureAlgorithm);
			break;

		case PP_HANDSHAKE_CODE_CHUNKADDRESSINGMETHOD:
			HaveChunkAddressingMethod = 1;
			idx++;
			ChunkAddressingMethod = data[idx++];
			LogPrint(LOG_LEVEL_DEBUG, "ChunkAddressingMethod : %d\n", ChunkAddressingMethod);
			break;

		case PP_HANDSHAKE_CODE_LIVEDISCARDWINDOW:
			HaveLiveDiscardWindow = 1;
			idx++;

			if (ChunkAddressingMethod == PP_HANDSHAKE_VALUE_CAM_32BITBINS || ChunkAddressingMethod == PP_HANDSHAKE_VALUE_CAM_32BITCHUNKRANGES)
			{
				LiveDiscardWindow = new char[4];
				memcpy(&LiveDiscardWindow, data + idx, 4);
				idx += 4;
			}
			else if (ChunkAddressingMethod == PP_HANDSHAKE_VALUE_CAM_64BITBINS || ChunkAddressingMethod == PP_HANDSHAKE_VALUE_CAM_64BITCHUNKRANGES || ChunkAddressingMethod == PP_HANDSHAKE_VALUE_CAM_64BITBYTERANGES)
			{
				LiveDiscardWindow = new char[8];
				memcpy(&LiveDiscardWindow, data + idx, 8);
				idx += 8;
			}

			break;

		case PP_HANDSHAKE_CODE_SUPPORTEDMESSAGES:
			HaveSupportedMessages = 1;
			idx++;
			len = data[idx++];

			if (len > 0)
			{
#pragma warning(disable:4244)
				SupportedMessagesBitmapLength = len;
#pragma warning(default:4244)
				SupportedMessagesBitmap = new unsigned char[len];
				memcpy(SupportedMessagesBitmap, data + idx, len);
				idx += len;
			}

			break;

		case PP_HANDSHAKE_CODE_CHUNKSIZE:
			HaveChunkSize = 1;
			idx++;
			memcpy(&ChunkSize, data + idx, 4);
			ChunkSize = CalcEndianN2H(ChunkSize);

			idx += 4;

			LogPrint(LOG_LEVEL_DEBUG, "ChunkSize : %d\n", ChunkSize);
			
			break;

		case PP_HANDSHAKE_CODE_PEER_LISTENING_PORT:
			idx++;
			memcpy(&PeerListeningPort, data + idx, 2);
			PeerListeningPort = CalcEndianN2H(PeerListeningPort);

			idx += 2;

			LogPrint(LOG_LEVEL_DEBUG, "PeerListeningPort : %d\n", PeerListeningPort);
			
			break;

		default:
			unknown = 1;
			LogPrint(LOG_LEVEL_ERROR, "Unknown HandShake Code : %x\n", data[idx]);
			break;
		}

		if (unknown) break;
	}

	return idx + 1;
}

char* HandShake::GetBuffer(int* len)
{
	char buf[1024];

	memset(buf, 0, 1024);

	unsigned char tmp;
	int idx = 0, tmpl = 0;
	short tmps = 0;

	tmpl = CalcEndianH2N(DestinationChannelID);
	memcpy(buf, &tmpl, 4);
	idx += 4;

	memcpy(buf + idx, &MessageType, 1);
	idx += 1;

	tmpl = CalcEndianH2N(SourceChannelID);
	memcpy(buf + idx, &tmpl, 4);
	idx += 4;

	tmp = PP_HANDSHAKE_CODE_VERSION;
	memcpy(buf + idx, &tmp, 1);
	idx += 1;
	memcpy(buf + idx, &Version, 1);
	idx += 1;

	tmp = PP_HANDSHAKE_CODE_MINIMUMVERSION;
	memcpy(buf + idx, &tmp, 1);
	idx += 1;
	memcpy(buf + idx, &MinVersion, 1);
	idx += 1;

	if (SwarmIdentifierString.length() > 0)
	{
		short silen = SwarmIdentifierString.length() / 2;

		if (SwarmIdentifier != 0) delete[] SwarmIdentifier;

		SwarmIdentifier = GetHashFromString(SwarmIdentifierString);

		tmp = PP_HANDSHAKE_CODE_SWARMIDENTIFIER;
		memcpy(buf + idx, &tmp, 1);
		idx += 1;
		tmps = CalcEndianH2N(silen);
		memcpy(buf + idx, &tmps, 2);
		idx += 2;
		memcpy(buf + idx, SwarmIdentifier, silen);
		idx += silen;
	}

	tmp = PP_HANDSHAKE_CODE_CONTENTINTEGRITYPROTECTIONMETHOD;
	memcpy(buf + idx, &tmp, 1);
	idx += 1;
	memcpy(buf + idx, &ContentIntegrityProtectionMethod, 1);
	idx += 1;

	tmp = PP_HANDSHAKE_CODE_MERKLEHASHTREEFUNCTION;
	memcpy(buf + idx, &tmp, 1);
	idx += 1;
	memcpy(buf + idx, &MerkleHashTreeFunction, 1);
	idx += 1;
	/*
	tmp = PP_HANDSHAKE_CODE_LIVESIGNATUREALGORITHM;
	memcpy(buf + idx, &tmp, 1);
	idx += 1;
	memcpy(buf + idx, &LiveSignatureAlgorithm, 1);
	idx += 1;*/

	tmp = PP_HANDSHAKE_CODE_CHUNKADDRESSINGMETHOD;
	memcpy(buf + idx, &tmp, 1);
	idx += 1;
	memcpy(buf + idx, &ChunkAddressingMethod, 1);
	idx += 1;

	/*tmp = PP_HANDSHAKE_CODE_LIVEDISCARDWINDOW;
	memcpy(buf + idx, &tmp, 1);
	idx += 1;
	memcpy(buf + idx, &LiveDiscardWindow, ldwlen);
	idx += 1;
	*/

	if (SupportedMessagesBitmap != 0)
	{
		tmp = PP_HANDSHAKE_CODE_SUPPORTEDMESSAGES;
		memcpy(buf + idx, &tmp, 1);
		idx += 1;
		memcpy(buf + idx, &SupportedMessagesBitmapLength, 1);
		idx += 1;
		memcpy(buf + idx, SupportedMessagesBitmap, SupportedMessagesBitmapLength);
		idx += SupportedMessagesBitmapLength;
	}

	if (ChunkSize > 0)
	{
		tmp = PP_HANDSHAKE_CODE_CHUNKSIZE;
		memcpy(buf + idx, &tmp, 1);
		idx += 1;
		tmpl = CalcEndianH2N(ChunkSize);
		memcpy(buf + idx, &tmpl, 4);
		idx += 4;
	}

	if (PeerListeningPort > 0)
	{
		tmp = PP_HANDSHAKE_CODE_PEER_LISTENING_PORT;
		memcpy(buf + idx, &tmp, 1);
		idx += 1;
		tmps = CalcEndianH2N(PeerListeningPort);
		memcpy(buf + idx, &tmps, 2);
		idx += 2;
	}

	tmp = PP_HANDSHAKE_CODE_ENDOPTION;
	memcpy(buf + idx, &tmp, 1);
	idx += 1;

	if (Buffer != 0) delete[] Buffer;
	
	Buffer = new char[idx];
	memset(Buffer, 0, idx);
	memcpy(Buffer, buf, idx);

	*len = idx;
	
	return Buffer;
}