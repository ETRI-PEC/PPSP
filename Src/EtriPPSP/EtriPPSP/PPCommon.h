#pragma once

#include <string>
#include "HashTree.h"
#include "Binmap.h"
#include "PP/HandShake.h"
#include "PP/Data.h"
#include "Common/Util.h"
#include <list>

class PPInfo
{
public:
	std::string FileName;
	std::string IpAddress;
	unsigned int Port;

	int IsSeeder;
	int ChunkSize;

	HashTree FileHashTree;
	Binmap FileBinmap;

	std::string SwarmID;

	char Version;
	char MinVersion;
	char ContentIntegrityProtectionMethod;
	char MerkleHashTreeFunction;
	char LiveSignatureAlgorithm;
	char ChunkAddressingMethod;
	char* LiveDiscardWindow;
	unsigned char* SupportedMessagesBitmap; //[variable, max 256]
	unsigned char SupportedMessagesBitmapLength;

	int RetryCount;

	int KeepAliveInterval;
	int PEX_REQInterval;

	int ListenPortOption;

	int IsFirstData;

	PPInfo()
	{
		Port = 0;
		IsSeeder = 0;
		ChunkSize = DEFAULT_CHUNK_SIZE;
		LiveDiscardWindow = 0;
		SupportedMessagesBitmap = 0;
		RetryCount = 3;
		KeepAliveInterval = 10;
		PEX_REQInterval = 5;
		ListenPortOption = 0;
		IsFirstData = 1;
	}

	~PPInfo()
	{
		if (LiveDiscardWindow != 0)
			delete[] LiveDiscardWindow;
		
		if (SupportedMessagesBitmap != 0)
			delete[] SupportedMessagesBitmap;
	}

	void SetFileName(std::string name)
	{
		FileName = name;
		FileBinmap.SetFileName(FileName);
	}

	void SetSwarmID(std::string id)
	{
		SwarmID = id;
		FileHashTree.SetRootHash(id);
	}

	int MakeHashTree()
	{
		FileHashTree.MerkleHashTreeFunction = MerkleHashTreeFunction;
		FileHashTree.ChunkSize = ChunkSize;

		if (FileName.length() > 0)
		{
			int rslt = FileHashTree.Make(FileName);

			if (rslt)
			{
				SwarmID = FileHashTree.RootHashString;
				FileBinmap.SetHave(0, FileHashTree.ChunkRange);
			}

			return rslt;
		}
		
		return 0;
	}

	int IsSupported(char code)
	{
		if (SupportedMessagesBitmapLength * 8 < code - 1)
		{
			return 0;
		}

		if (code < 8)
		{
			unsigned char a = (1 << (7 - code));

			return (SupportedMessagesBitmap[0] & a) > 0;
		}
		else
		{
			unsigned char a = (1 << (7 - (code - 8)));

			return (SupportedMessagesBitmap[1] & a) > 0;
		}
	}

	void SetInfo(HandShake* handshake)
	{
		Version = handshake->Version;
		MinVersion = handshake->MinVersion;
		SwarmID = handshake->SwarmIdentifierString;
		ContentIntegrityProtectionMethod = handshake->ContentIntegrityProtectionMethod;
		MerkleHashTreeFunction = handshake->MerkleHashTreeFunction;
		LiveSignatureAlgorithm = handshake->LiveSignatureAlgorithm;
		ChunkAddressingMethod = handshake->ChunkAddressingMethod;
		//LiveDiscardWindow = handshake->LiveDiscardWindow unsupported
		SupportedMessagesBitmap = new unsigned char[handshake->SupportedMessagesBitmapLength];
		SupportedMessagesBitmapLength = handshake->SupportedMessagesBitmapLength;
		memcpy(SupportedMessagesBitmap, handshake->SupportedMessagesBitmap, handshake->SupportedMessagesBitmapLength);
		ChunkSize = handshake->ChunkSize;
	}

	char* GetDataFromFile(int index, int *length)
	{
		return FileBinmap.GetData(ChunkSize, index, length);
	}

	int SetData(Data* data)
	{
		return FileBinmap.SetData(data->StartChunk, data->EndChunk, ChunkSize, data->DataBuffer);
	}

	int CheckFinish()
	{
		return FileBinmap.CheckFinish();
	}

	void GetBiggestHavedChunkRange(int* start, int* end)
	{
		FileBinmap.GetBiggestHavedChunkRange(start, end);
	}

	MerkleHashmap* GetPeakHashes()
	{
		return FileHashTree.GetPeakHashes();
	}

	MerkleHashmap* GetDataIntegrity(int channelid, int index)
	{
		return FileHashTree.GetDataIntegrity(channelid, index);
	}

	void AddIntegrity(int channelid, Integrity *inte)
	{
		FileHashTree.AddIntegrity(channelid, inte);
	}

	int CheckDataIntegrity(int channelid, Data *data)
	{
		LogPrint(LOG_LEVEL_DEBUG, "CheckDataIntegrity start.\n");

		/*if (!IsSupported(PP_MESSAGETYPE_INTEGRITY))
		{
			LogPrint(LOG_LEVEL_DEBUG, "Unsupported Integrity.\n");
			return 1;
		}*/

		int rslt = FileHashTree.CheckDataIntegrity(channelid, data);

		LogPrint(LOG_LEVEL_DEBUG, "CheckDataIntegrity end.\n");

		return rslt;
	}
};
