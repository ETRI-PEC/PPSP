#pragma once

#include <string>
#include <mutex>
#include "Common/Common.h"
#include "PP/Integrity.h"
#include "PP/Data.h"

#ifdef WIN32
#include <unordered_map>
#else
#ifdef MACOS
#include <unordered_map>
#else
#include <tr1/unordered_map>
using namespace std::tr1;
#endif
#endif

#define MHF_SHA256 2
#define MHF_SHA1 0

typedef struct _BTreeNode
{
	char IsAllZero;
	char IsInComplete;
	char IsRoot;
	char IsLeft;
	char IsPreview;

	unsigned int TreeIndex;

	unsigned int StartChunk;
	unsigned int EndChunk;

	unsigned char *Hash;

	_BTreeNode *Parent;
	_BTreeNode *Sibling;
	_BTreeNode *LeftChild;
	_BTreeNode *RightChild;

	_BTreeNode()
	{
		IsAllZero = 0;
		IsInComplete = 0;
		IsRoot = 0;
		IsLeft = 0;
		IsPreview = 0;

		TreeIndex = -1;

		StartChunk = -1;
		EndChunk = -1;

		Hash = 0;

		Parent = 0;
		Sibling = 0;
		LeftChild = 0;
		RightChild = 0;
	}

	~_BTreeNode()
	{
		if (Hash != 0) delete[] Hash;
	}

	void CopyHash(unsigned char* hash, int len)
	{
		if (Hash != 0) delete[] Hash;
		
		Hash = new unsigned char[len];
		memcpy(Hash, hash, len);
	}
}BTreeNode;

#if defined(WIN32) || defined(MACOS)
typedef std::unordered_map<int, BTreeNode*> MerkleHashmap;
typedef std::unordered_map<int, MerkleHashmap*> MultiMerkleHashmap;
typedef std::unordered_map<int, char> CheckHashmap;
typedef std::unordered_map<int, CheckHashmap*> CheckIndexHashmap;
#else
typedef unordered_map<int, BTreeNode*> MerkleHashmap;
typedef unordered_map<int, MerkleHashmap*> MultiMerkleHashmap;
typedef unordered_map<int, char> CheckHashmap;
typedef unordered_map<int, CheckHashmap*> CheckIndexHashmap;
#endif

typedef MerkleHashmap::iterator MerkleHashmapIter;
typedef MultiMerkleHashmap::iterator MultiMerkleHashmapIter;


class HashTree
{
public:
	HashTree();
	~HashTree();

	int MerkleHashTreeFunction;
	int ChunkSize;
	int ChunkRange;
	int LastChunkIndex;
	int LeafNodeCount;
	unsigned long ContentSize;
	unsigned long LastChunkSize;
	std::string RootHashString;
	unsigned char *RootHash;

	int Make(std::string file);
	void SetRootHash(std::string hash);
	void SetRootHash(unsigned char* hash);
	MerkleHashmap* GetPeakHashes();
	MerkleHashmap* GetDataIntegrity(int channelid, int index);
	int CheckPeakHash(int channelid);
	void AddIntegrity(int channelid, Integrity *integrity);
	int CheckDataIntegrity(int channelid, Data *data);
	unsigned char* SHA_Encrypt(unsigned char *data, int len);
	unsigned char* SHA_EncryptLR(unsigned char *left, unsigned char *right);

private:
	MerkleHashmap HashTreeMap;
	std::mutex HashTreeMapMutex;
	BTreeNode* RootNode;
	MultiMerkleHashmap IntegrityMap;
	std::mutex IntegrityMapMutex;
	MerkleHashmap PeakHashMap;
	int IsPeakMapCreated;
	std::mutex PeakHashMapMutex;
	CheckIndexHashmap IntegritySendMap;
};

