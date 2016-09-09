#include "HashTree.h"
#include "Common/Util.h"
#include "SHA/sha1.h"
#include "SHA/sha256.h"
#include <math.h>


HashTree::HashTree()
{
	MerkleHashTreeFunction = MHF_SHA1;
	ChunkSize = DEFAULT_CHUNK_SIZE;
	ContentSize = 0;
	ChunkRange = -1;
	LastChunkSize = 0;
	LastChunkIndex = 0;
	RootNode = 0;
	LeafNodeCount = 0;
	RootHash = 0;
	IsPeakMapCreated = 0;
}


HashTree::~HashTree()
{
	HashTreeMapMutex.lock();
	for (MerkleHashmapIter it = HashTreeMap.begin(); it != HashTreeMap.end(); ++it)
	{
		delete (*it).second;
	}
	HashTreeMapMutex.unlock();

	IntegrityMapMutex.lock();
	for (MultiMerkleHashmapIter it = IntegrityMap.begin(); it != IntegrityMap.end(); ++it)
	{
		for (MerkleHashmapIter it2 = (*it).second->begin(); it2 != (*it).second->end(); ++it2)
		{
			delete (*it2).second;
		}

		delete (*it).second;
	}
	IntegrityMapMutex.unlock();

	if (RootHash != 0) delete[] RootHash;
}

unsigned char* HashTree::SHA_Encrypt(unsigned char *data, int len)
{
	unsigned char *hashout;

	if (MerkleHashTreeFunction == MHF_SHA1)
	{
		hashout = new unsigned char[SHA1_DIGEST_BLOCKLEN];
		memset(hashout, 0, SHA1_DIGEST_BLOCKLEN);
		SHA1_Encrypt(data, len, hashout);
	}
	else // if (MerkleHashTreeFunction == MHF_SHA256)
	{
		hashout = new unsigned char[SHA256_DIGEST_BLOCKLEN];
		memset(hashout, 0, SHA256_DIGEST_BLOCKLEN);
		SHA256_Encrypt(data, len, hashout);
	}

	return hashout;
}

unsigned char* HashTree::SHA_EncryptLR(unsigned char *left, unsigned char *right)
{
	unsigned char *hashout;
	unsigned char *allzero;

	if (left == 0 && right == 0)
	{
		return 0;
	}

	if (MerkleHashTreeFunction == MHF_SHA1)
	{
		hashout = new unsigned char[SHA1_DIGEST_BLOCKLEN];
		memset(hashout, 0, SHA1_DIGEST_BLOCKLEN);

		if (left == 0)
		{
			allzero = new unsigned char[SHA1_DIGEST_BLOCKLEN];
			memset(allzero, 0, SHA1_DIGEST_BLOCKLEN);

			SHA1_EncryptLR(allzero, right, hashout);
			delete[] allzero;
		}
		else if (right == 0)
		{
			allzero = new unsigned char[SHA1_DIGEST_BLOCKLEN];
			memset(allzero, 0, SHA1_DIGEST_BLOCKLEN);

			SHA1_EncryptLR(left, allzero, hashout);
			delete[] allzero;
		}
		else
		{
			SHA1_EncryptLR(left, right, hashout);
		}
	}
	else // if (MerkleHashTreeFunction == MHF_SHA256)
	{
		hashout = new unsigned char[SHA256_DIGEST_BLOCKLEN];
		memset(hashout, 0, SHA256_DIGEST_BLOCKLEN);

		if (left == 0)
		{
			allzero = new unsigned char[SHA256_DIGEST_BLOCKLEN];
			memset(allzero, 0, SHA256_DIGEST_BLOCKLEN);

			SHA256_EncryptLR(allzero, right, hashout);
			delete[] allzero;
		}
		else if (right == 0)
		{
			allzero = new unsigned char[SHA256_DIGEST_BLOCKLEN];
			memset(allzero, 0, SHA256_DIGEST_BLOCKLEN);

			SHA256_EncryptLR(left, allzero, hashout);
			delete[] allzero;
		}
		else
		{
			SHA256_EncryptLR(left, right, hashout);
		}
	}

	return hashout;
}

int HashTree::Make(std::string file)
{
	FILE *fp = fopen(file.c_str(), "rb");
	
	if (!fp)
	{
		LogPrint(LOG_LEVEL_SILENT, "Can't read the file.\n");
		return 0;
	}

	LogPrint(LOG_LEVEL_DEBUG, "Make HashTree Start...\n");

	unsigned char *fbuf = new unsigned char[ChunkSize];
		
	unsigned char *hashout;

	int ret = 0, idx = 0, cidx = 0;

	HashTreeMapMutex.lock();

	HashTreeMap.clear();

	while (!feof(fp))
	{
		memset(fbuf, 0, ChunkSize);
		ret = fread(fbuf, 1, ChunkSize, fp);

		if (ret <= 0)
		{
			break;
		}

		hashout = SHA_Encrypt(fbuf, ret);

		BTreeNode *node = new BTreeNode;
		node->TreeIndex = idx;
		node->IsAllZero = 0;
		node->IsInComplete = 0;
		node->IsRoot = 0;
		node->StartChunk = cidx;
		node->EndChunk = cidx;
		node->Hash = hashout;

		HashTreeMap[idx] = node;

		if ((cidx + 1) % 2 == 0 && cidx > 0)
		{
			node->IsLeft = 0;
			
			MerkleHashmapIter sibling = HashTreeMap.find(idx - 2);

			if (sibling != HashTreeMap.end())
			{
				node->Sibling = (*sibling).second;
				(*sibling).second->Sibling = node;
			}
		}
		else
		{
			node->IsLeft = 1;
		}
		
		cidx++;
		idx += 2;

		if (ret != ChunkSize)
		{
			break;
		}
	}
	
	ContentSize = ChunkSize * (cidx - 1) + ret;
	LastChunkSize = ret;

	ChunkRange = HashTreeMap.size() - 1;
	LastChunkIndex = ChunkRange * 2;

	while ((idx & (idx - 1)) != 0)
	{
		/*if (MerkleHashTreeFunction == MHF_SHA1)
		{
			hashout = new unsigned char[SHA1_DIGEST_BLOCKLEN];
			memset(hashout, 0, SHA1_DIGEST_BLOCKLEN);
		}
		else // if (MerkleHashTreeFunction == MHF_SHA256)
		{
			hashout = new unsigned char[SHA256_DIGEST_BLOCKLEN];
			memset(hashout, 0, SHA256_DIGEST_BLOCKLEN);
		}*/

		BTreeNode *node = new BTreeNode;
		node->TreeIndex = idx;
		node->IsAllZero = 1;
		node->IsInComplete = 1;
		node->IsRoot = 0;
		node->StartChunk = cidx;
		//node->EndChunk = ChunkRange;
		node->EndChunk = cidx;
		//node->Hash = hashout;
		node->Hash = 0;

		if ((cidx + 1) % 2 == 0 && cidx > 0)
		{
			node->IsLeft = 0;

			MerkleHashmapIter sibling = HashTreeMap.find(idx - 2);

			if (sibling != HashTreeMap.end())
			{
				node->Sibling = (*sibling).second;
				(*sibling).second->Sibling = node;
			}
		}
		else
		{
			node->IsLeft = 1;
		}

		HashTreeMap[idx] = node;

		idx += 2;
		cidx++;
	}

	LeafNodeCount = HashTreeMap.size();

	delete[] fbuf;
	fclose(fp);

	int startidx = 0;
	int startgap = 1;
	int gap = 2;
		
	while (true)
	{
		idx = startidx;

		if (idx >= LeafNodeCount - 1)
		{
			break;
		}

		int didx = 0;

		while (true)
		{
			MerkleHashmapIter iter = HashTreeMap.find(idx);

			if (iter == HashTreeMap.end())
				break;

			//LogPrint(LOG_LEVEL_DEBUG, "%d   ", idx);

			BTreeNode* left = (*iter).second;

			idx += gap;

			iter = HashTreeMap.find(idx);

			if (iter == HashTreeMap.end())
				break;

			//LogPrint(LOG_LEVEL_DEBUG, "%d   ", idx);

			didx++;

			BTreeNode* right = (*iter).second;

			hashout = SHA_EncryptLR(left->Hash, right->Hash);

			int pidx = (idx + (idx - gap)) / 2;

			BTreeNode *node = new BTreeNode;
			node->TreeIndex = pidx;
			node->IsAllZero = (left->IsAllZero && right->IsAllZero);
			node->IsInComplete = (left->IsInComplete || right->IsInComplete);
			node->IsRoot = pidx == LeafNodeCount - 1;
			node->StartChunk = left->StartChunk;
			node->EndChunk = right->EndChunk;
			node->Hash = hashout;
			node->LeftChild = left;
			node->RightChild = right;
			left->Parent = node;
			right->Parent = node;

			if (node->IsRoot)
			{
				RootNode = node;
			}

			left->Sibling = right;
			right->Sibling = left;
			/*else
			{
				if (didx % 2 == 0)
				{
					node->IsLeft = 0;

					MerkleHashmapIter sibling = HashTreeMap.find(idx - gap);

					if (sibling != HashTreeMap.end())
					{
						node->Sibling = (*sibling).second;
						(*sibling).second->Sibling = node;
					}
				}
				else
				{
					node->IsLeft = 1;
				}
			}*/
			
			HashTreeMap[pidx] = node;

			idx += gap;
		}

		//LogPrint(LOG_LEVEL_DEBUG, "\n");

		gap *= 2;
		startgap *= 2;
		startidx = startgap - 1;
	}

	HashTreeMapMutex.unlock();

	LogPrint(LOG_LEVEL_DEBUG, "Make HashTree End. End chunk range : %d, Total node count : %d\n", RootNode->EndChunk, HashTreeMap.size());

	SetRootHash(hashout);
	/*if (MerkleHashTreeFunction == MHF_SHA1)
	{
		RootHash = new unsigned char[SHA1_DIGEST_BLOCKLEN];
		memcpy(RootHash, hashout, SHA1_DIGEST_BLOCKLEN);
		RootHashString = GetStringFromHash(hashout, SHA1_DIGEST_BLOCKLEN);
	}
	else // if (MerkleHashTreeFunction == MHF_SHA256)
	{
		RootHash = new unsigned char[SHA256_DIGEST_BLOCKLEN];
		memcpy(RootHash, hashout, SHA256_DIGEST_BLOCKLEN);
		RootHashString = GetStringFromHash(hashout, SHA256_DIGEST_BLOCKLEN);
	}*/

	LogPrint(LOG_LEVEL_SILENT, "Root Hash : %s\n", RootHashString.c_str());
	
	return 1;
}

void HashTree::SetRootHash(std::string hash)
{
	RootHashString = hash;

	RootHash = GetHashFromString(hash);

	/*unsigned char *hashval = GetHashFromString(hash);

	
	if (MerkleHashTreeFunction == MHF_SHA1)
	{
		RootHash = new unsigned char[SHA1_DIGEST_BLOCKLEN];
		memcpy(RootHash, hashval, SHA1_DIGEST_BLOCKLEN);
	}
	else // if (MerkleHashTreeFunction == MHF_SHA256)
	{
		RootHash = new unsigned char[SHA256_DIGEST_BLOCKLEN];
		memcpy(RootHash, hashval, SHA256_DIGEST_BLOCKLEN);
	}*/
}

void HashTree::SetRootHash(unsigned char* hash)
{
	if (MerkleHashTreeFunction == MHF_SHA1)
	{
		RootHash = new unsigned char[SHA1_DIGEST_BLOCKLEN];
		memcpy(RootHash, hash, SHA1_DIGEST_BLOCKLEN);
		RootHashString = GetStringFromHash(hash, SHA1_DIGEST_BLOCKLEN);
	}
	else // if (MerkleHashTreeFunction == MHF_SHA256)
	{
		RootHash = new unsigned char[SHA256_DIGEST_BLOCKLEN];
		memcpy(RootHash, hash, SHA256_DIGEST_BLOCKLEN);
		RootHashString = GetStringFromHash(hash, SHA256_DIGEST_BLOCKLEN);
	}
}

int _getSiblingIndex(int start, int end, int *s_start, int *s_end)
{
	int cnt = end - start + 1;
	int dep = cnt * 2;

	if ((end + 1) % dep == 0)
	{
		*s_start = start - cnt;
		*s_end = end - cnt;
	}
	else
	{
		*s_start = start + cnt;
		*s_end = end + cnt;
	}

	return (((*s_start) * 2) + ((*s_end) * 2)) / 2;
}

MerkleHashmap* HashTree::GetDataIntegrity(int channelid, int index)
{
	HashTreeMapMutex.lock();
	MerkleHashmap *peakmap = new MerkleHashmap;
	
	int start = index, end = index, s_start = 0, s_end = 0;

	if (IntegritySendMap.find(channelid) == IntegritySendMap.end())
	{
		IntegritySendMap[channelid] = new CheckHashmap;
	}

	CheckHashmap * checkmap = IntegritySendMap[channelid];

	while (true)
	{
		int s_index = _getSiblingIndex(start, end, &s_start, &s_end);

		if (checkmap->find(s_index) == checkmap->end())
		{
			MerkleHashmapIter iter = HashTreeMap.find(s_index);

			if (iter != HashTreeMap.end())
			{
				if ((*iter).second->Hash != 0)
				{
					(*peakmap)[(*iter).second->TreeIndex] = (*iter).second;
					(*checkmap)[s_index] = 0;
				}
			}
			else
			{
				break;
			}
		}

		start = start > s_start ? s_start : start;
		end = end > s_end ? end : s_end;
	}
	HashTreeMapMutex.unlock();
	return peakmap;
}

MerkleHashmap* HashTree::GetPeakHashes()
{
	MerkleHashmap *peakmap = new MerkleHashmap;

	PeakHashMapMutex.lock();

	if (IsPeakMapCreated)
	{
		for (auto it = PeakHashMap.begin(); it != PeakHashMap.end(); ++it)
		{
			(*peakmap)[(*it).second->TreeIndex] = (*it).second;
		}
	}
	else
	{
		IsPeakMapCreated = 1;

		if (((ChunkRange + 1) & ChunkRange) != 0)
		{
			HashTreeMapMutex.lock();
			MerkleHashmapIter iter = HashTreeMap.find(LastChunkIndex);

			if (iter != HashTreeMap.end())
			{
				BTreeNode* node = (*iter).second;

				while (node != 0)
				{
					if (node->Sibling != 0 && node->Sibling->IsInComplete && !node->IsInComplete)
					{
						(*peakmap)[node->TreeIndex] = node;
						PeakHashMap[node->TreeIndex] = node;
						LogPrint(LOG_LEVEL_SILENT, "Peak Hash Index : %d\n", node->TreeIndex);
					}
					else if (node->Sibling != 0 && node->IsInComplete && !node->Sibling->IsInComplete)
					{
						(*peakmap)[node->Sibling->TreeIndex] = node->Sibling;
						PeakHashMap[node->Sibling->TreeIndex] = node->Sibling;
						LogPrint(LOG_LEVEL_SILENT, "Peak Hash Index : %d\n", node->Sibling->TreeIndex);
					}

					node = node->Parent;
				}
			}
			HashTreeMapMutex.unlock();
		}
	}

	PeakHashMapMutex.unlock();

	return peakmap;
}

void HashTree::AddIntegrity(int channelid, Integrity *integrity)
{
	BTreeNode *node = new BTreeNode;
	node->TreeIndex = ((integrity->StartChunk * 2) + (integrity->EndChunk * 2)) / 2;
	node->StartChunk = integrity->StartChunk;
	node->EndChunk = integrity->EndChunk;
	node->CopyHash(integrity->Hash, MerkleHashTreeFunction == MHF_SHA1 ? SHA1_DIGEST_BLOCKLEN : SHA256_DIGEST_BLOCKLEN);
	
	IntegrityMapMutex.lock();

	MultiMerkleHashmapIter iter = IntegrityMap.find(channelid);

	if (iter == IntegrityMap.end())
	{
		MerkleHashmap *map = new MerkleHashmap;
		(*map)[node->TreeIndex] = node;

		IntegrityMap[channelid] = map;
	}
	else
	{
		(*(*iter).second)[node->TreeIndex] = node;
	}

	IntegrityMapMutex.unlock();
}

int HashTree::CheckDataIntegrity(int channelid, Data *data)
{
	if (data->DataBuffer == 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "data->DataBuffer == 0\n");
		return 0;
	}

	if (data->StartChunk != data->EndChunk)
	{
		LogPrint(LOG_LEVEL_ERROR, "data->StartChunk != data->EndChunk\n");
		return 0;
	}

	//int isLeft = !(data->StartChunk % 2);

	unsigned char *datahash = SHA_Encrypt((unsigned char*)data->DataBuffer, data->BufferSize);

	if (ChunkRange < 0)
	{
		int s_start = 0, s_end = 0;
		int sibling = _getSiblingIndex(data->StartChunk, data->EndChunk, &s_start, &s_end);

		IntegrityMapMutex.lock();

		MultiMerkleHashmapIter iter = IntegrityMap.find(channelid);

		if (iter == IntegrityMap.end() || (*iter).second->find(sibling) == (*iter).second->end())
		{
			unsigned char *hashout = SHA_EncryptLR(datahash, 0);

			if (!memcmp(RootHash, hashout, MerkleHashTreeFunction == MHF_SHA1 ? SHA1_DIGEST_BLOCKLEN : SHA256_DIGEST_BLOCKLEN))
			{
				LogPrint(LOG_LEVEL_DEBUG, "Data Integrity check ok!, chunkrange 0\n");
				ChunkRange = 0;
				LastChunkIndex = 0;
				LastChunkSize = data->BufferSize;
				ContentSize = data->BufferSize;
				LeafNodeCount = 2;

				IntegrityMapMutex.unlock();

				delete[] datahash;
				delete[] hashout;

				return 1;
			}
			else
			{
				LogPrint(LOG_LEVEL_DEBUG, "Data Integrity check error! wrong hasg.\n");

				IntegrityMapMutex.unlock();

				delete[] datahash;
				delete[] hashout;

				return 0;
			}
		}
		else
		{
			MerkleHashmapIter miter = (*iter).second->find(sibling);

			int start = data->StartChunk;
			int end = data->EndChunk;
			unsigned char *hashout = 0;

			MerkleHashmap tmpmap;

			while (miter != (*iter).second->end())
			{
				tmpmap[(*miter).second->TreeIndex] = (*miter).second;

				if (start < s_start)
				{
					hashout = SHA_EncryptLR(datahash, (*miter).second->Hash);
				}
				else
				{
					hashout = SHA_EncryptLR((*miter).second->Hash, datahash);
				}

				delete[] datahash;
				datahash = hashout;
				
				start = start > s_start ? s_start : start;
				end = end > s_end ? end : s_end;

				sibling = _getSiblingIndex(start, end, &s_start, &s_end);

				miter = (*iter).second->find(sibling);
			}

			if (!memcmp(RootHash, hashout, MerkleHashTreeFunction == MHF_SHA1 ? SHA1_DIGEST_BLOCKLEN : SHA256_DIGEST_BLOCKLEN))
			{
				LogPrint(LOG_LEVEL_DEBUG, "Data Integrity check ok!, chunkrange %d\n", end);
				ChunkRange = end;
				LastChunkIndex = end * 2;
				LeafNodeCount = end + 1;

				HashTreeMapMutex.lock();

				LogPrint(LOG_LEVEL_DEBUG, "!!!! move to hash from integrity\n");
				for (auto it = tmpmap.begin(); it != tmpmap.end(); ++it)
				{
					if (HashTreeMap.find((*it).first) == HashTreeMap.end())
					{
						HashTreeMap[(*it).first] = (*it).second;
					}					
					IntegrityMap[channelid]->erase((*it).first);

					LogPrint(LOG_LEVEL_DEBUG, "%d\n", (*it).first);
				}

				tmpmap.clear();

				HashTreeMapMutex.unlock();
				IntegrityMapMutex.unlock();

				delete[] hashout;

				return 1;
			}
			else
			{
				LogPrint(LOG_LEVEL_DEBUG, "Data Integrity check error! \n");

				IntegrityMapMutex.unlock();

				delete[] hashout;

				return 0;
			}
		}
	}
	else
	{
		int start = data->StartChunk;
		int end = data->EndChunk;
		int s_start = data->StartChunk;
		int s_end = data->EndChunk;
		unsigned char *hashout = 0;
		unsigned char *siblinghash = 0;

		int sibling = _getSiblingIndex(data->StartChunk, data->EndChunk, &s_start, &s_end);

		/*MultiMerkleHashmapIter iter = IntegrityMap.find(channelid);

		if (iter == IntegrityMap.end() || (*iter).second->find(sibling) == (*iter).second->end())
		{
			LogPrint(LOG_LEVEL_DEBUG, "Data Integrity check error! \n");
			delete[] datahash;
			return 0;
		}*/

		MerkleHashmap tmpmap;

		HashTreeMapMutex.lock();
		
		while (sibling <= (LeafNodeCount - 1) * 2)
		{
			siblinghash = 0;

			if (s_start <= ChunkRange)
			{
				MultiMerkleHashmapIter iter = IntegrityMap.find(channelid);

				if (iter != IntegrityMap.end())
				{
					MerkleHashmapIter miter = (*iter).second->find(sibling);

					if (miter != (*iter).second->end())
					{
						siblinghash = (*miter).second->Hash;
						tmpmap[(*miter).first] = (*miter).second;
					}
				}

				if (siblinghash == 0)
				{
					MerkleHashmapIter miter = HashTreeMap.find(sibling);

					if (miter == HashTreeMap.end())
					{
						LogPrint(LOG_LEVEL_DEBUG, "Data Integrity check error! \n");
						delete[] datahash;
						HashTreeMapMutex.unlock();
						return 0;
					}

					siblinghash = (*miter).second->Hash;
				}

				if (siblinghash == 0)
				{
					LogPrint(LOG_LEVEL_DEBUG, "Data Integrity check error! \n");
					delete[] datahash;
					HashTreeMapMutex.unlock();
					return 0;
				}
			}

			if (start < s_start)
			{
				hashout = SHA_EncryptLR(datahash, siblinghash);
			}
			else
			{
				hashout = SHA_EncryptLR(siblinghash, datahash);
			}

			if (datahash != 0) delete[] datahash;

			datahash = hashout;

			start = start > s_start ? s_start : start;
			end = end > s_end ? end : s_end;

			sibling = _getSiblingIndex(start, end, &s_start, &s_end);
		}

		HashTreeMapMutex.unlock();
		
		if (!memcmp(RootHash, datahash, MerkleHashTreeFunction == MHF_SHA1 ? SHA1_DIGEST_BLOCKLEN : SHA256_DIGEST_BLOCKLEN))
		{
			LogPrint(LOG_LEVEL_DEBUG, "Data Integrity check ok!\n");

			if (ContentSize <= 0 && LastChunkSize <= 0 && data->StartChunk == data->EndChunk && data->EndChunk == ChunkRange)
			{
				ContentSize = (ChunkSize * ChunkRange) + data->BufferSize;
				LastChunkSize = data->BufferSize;

				LogPrint(LOG_LEVEL_SILENT, "!!!! Auto detected content size : %ld\n", ContentSize);
			}

			HashTreeMapMutex.lock();
			LogPrint(LOG_LEVEL_DEBUG, "!!!! move to hash from integrity\n");
			for (auto it = tmpmap.begin(); it != tmpmap.end(); ++it)
			{
				if (HashTreeMap.find((*it).first) == HashTreeMap.end())
				{
					HashTreeMap[(*it).first] = (*it).second;
				}
				IntegrityMap[channelid]->erase((*it).first);

				LogPrint(LOG_LEVEL_DEBUG, "%d\n", (*it).first);
			}
			HashTreeMapMutex.unlock();
			delete[] datahash;

			return 1;
		}
		else
		{
			LogPrint(LOG_LEVEL_DEBUG, "Data Integrity check error! \n");
			delete[] datahash;
			return 0;
		}
	}
}

int HashTree::CheckPeakHash(int channelid)
{
	if (IntegrityMap.size() <= 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "Peak hash not received.\n");
		return 0;
	}

	MultiMerkleHashmapIter iter = IntegrityMap.find(channelid);

	if (iter == IntegrityMap.end())
	{
		LogPrint(LOG_LEVEL_ERROR, "Peak hash not received.\n");
		return 0;
	}

	MerkleHashmap *intemap = (*iter).second;

	if (intemap->size() <= 0 && ChunkRange >= 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "Full tree.\n");
		IsPeakMapCreated = 1;
		return 1;
	}
	
	BTreeNode *leafnode = 0;
	BTreeNode *unclenode = 0;

	IntegrityMapMutex.lock();

	for (auto it = intemap->begin(); it != intemap->end(); ++it)
	{
		if ((*it).second->StartChunk == (*it).second->EndChunk)
		{
			leafnode = (*it).second;
			it = intemap->erase(it);

			unclenode = 0;

			break;
		}
		else// if ((*it).second->StartChunk == (*it).second->EndChunk - 1)
		{
			if (unclenode == 0)
			{
				unclenode = (*it).second;
			}
			else
			{
				if (unclenode->EndChunk < (*it).second->EndChunk)
				{
					unclenode = (*it).second;
				}
			}
			//unclenode = (*it).second;
			//it = intemap->erase(it);
		}
	}

	if (leafnode == 0 && unclenode == 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "Wrong Peak hash. peak hash count : %d\n", intemap->size());
		IntegrityMapMutex.unlock();
		return 0;
	}

	MerkleHashmap tmpmap;

	if (unclenode != 0)
	{
		intemap->erase(unclenode->TreeIndex);

		leafnode = unclenode;
	}

	if (leafnode != 0)
	{
		tmpmap[leafnode->TreeIndex] = leafnode;

		unsigned char *hashout = 0;
		unsigned char *siblinghash = 0;
		unsigned char *datahash = leafnode->Hash;
		//unsigned char *allzero = 0;

		int start = leafnode->StartChunk;
		int end = leafnode->EndChunk;
		int s_start = 0, s_end = 0;

		int sibling = _getSiblingIndex(start, end, &s_start, &s_end);

		//int tmpNodeCount = (int)pow(2, log2(leafnode->EndChunk) + 1);

		int idx = leafnode->EndChunk;

		if (idx % 2 != 0)
		{
			idx++;
		}

		while ((idx & (idx - 1)) != 0)
		{
			idx += 2;
		}

		LeafNodeCount = idx;

		int tmpNodeCount = (int)pow(2, log2(LeafNodeCount - 1) + 1);

		//while (sibling <= (tmpNodeCount - 1) * 2)
		while (sibling <= tmpNodeCount)
		{
			siblinghash = 0;

			if (sibling <= (int)(leafnode->EndChunk * 2))
			{
				auto inteit = intemap->find(sibling);

				if (inteit == intemap->end())
				{
					LogPrint(LOG_LEVEL_ERROR, "Wrong Peak hash. peak hash count : %d\n", intemap->size());
					IntegrityMapMutex.unlock();
					if (hashout != 0) delete[] hashout;
					return 0;
				}

				siblinghash = (*inteit).second->Hash;
				tmpmap[(*inteit).second->TreeIndex] = (*inteit).second;
			}

			if (start < s_start)
			{
				hashout = SHA_EncryptLR(datahash, siblinghash);
			}
			else
			{
				hashout = SHA_EncryptLR(siblinghash, datahash);
			}

			if (datahash != 0 && datahash != leafnode->Hash) delete[] datahash;

			datahash = hashout;

			start = start > s_start ? s_start : start;
			end = end > s_end ? end : s_end;

			sibling = _getSiblingIndex(start, end, &s_start, &s_end);
		}

		if (!memcmp(RootHash, datahash, MerkleHashTreeFunction == MHF_SHA1 ? SHA1_DIGEST_BLOCKLEN : SHA256_DIGEST_BLOCKLEN))
		{
			ChunkRange = leafnode->EndChunk;
			LastChunkIndex = ChunkRange * 2;
			//LeafNodeCount = (int)pow(2, log2(ChunkRange) + 1);

			LogPrint(LOG_LEVEL_SILENT, "Peak Hash checked, chunkrange %d\n", ChunkRange);

			HashTreeMapMutex.lock();
			PeakHashMapMutex.lock();
			LogPrint(LOG_LEVEL_DEBUG, "!!!! move to hash from integrity\n");
			for (auto it = tmpmap.begin(); it != tmpmap.end(); ++it)
			{
				if (HashTreeMap.find((*it).first) == HashTreeMap.end())
				{
					HashTreeMap[(*it).first] = (*it).second;
				}
				PeakHashMap[(*it).first] = (*it).second;
				IntegrityMap[channelid]->erase((*it).first);

				LogPrint(LOG_LEVEL_DEBUG, "%d\n", (*it).first);
			}
			PeakHashMapMutex.unlock();
			HashTreeMapMutex.unlock();
			IntegrityMapMutex.unlock();
			
			IsPeakMapCreated = 1;

			delete[] datahash;

			return 1;
		}
		else
		{
			LogPrint(LOG_LEVEL_DEBUG, "Peak Hash check error! \n");
			delete[] datahash;
			return 0;
		}
	}

	IntegrityMapMutex.unlock();
	return 1;

	/*
	IntegrityMapMutex.lock();
	if (IntegrityMap.size() == 1)
	{
		BTreeNode *node = (*(++IntegrityMap.begin())).second;

		if (node->StartChunk == node->EndChunk)
		{
			LogPrint(LOG_LEVEL_ERROR, "Wrong Peak hash. peak hash count : 1, start : %d, end %d\n", node->StartChunk, node->EndChunk);

			delete node;
			IntegrityMap.clear();
			IntegrityMapMutex.unlock();
			return 0;
		}

		if (memcmp(RootHash, node->Hash, MerkleHashTreeFunction == MHF_SHA1 ? SHA1_DIGEST_BLOCKLEN : SHA256_DIGEST_BLOCKLEN))
		{
			LogPrint(LOG_LEVEL_ERROR, "Wrong Peak hash. peak hash count : 1, start : %d, end %d\n", node->StartChunk, node->EndChunk);

			delete node;
			IntegrityMap.clear();
			IntegrityMapMutex.unlock();
			return 0;
		}
		else
		{
			LogPrint(LOG_LEVEL_ERROR, "Peak hash checked. peak hash count : 1, start : %d, end %d\n", node->StartChunk, node->EndChunk);
			ChunkRange = node->EndChunk;
			LastChunkIndex = ChunkRange * 2;
			LeafNodeCount = (int)pow(2, log2(ChunkRange + 1) + 1);
			node->IsRoot = 1;
			
			HashTreeMapMutex.lock();
			HashTreeMap[node->TreeIndex] = node;
			HashTreeMapMutex.unlock();

			PeakHashMapMutex.lock();
			PeakHashMap[node->TreeIndex] = node;
			PeakHashMapMutex.unlock();

			IntegrityMap.erase(node->TreeIndex);
			IntegrityMapMutex.unlock();
			return 1;
		}
	}
	else
	{
		BTreeNode *leafnode = 0;
		BTreeNode *unclenode = 0;

		for (auto it = IntegrityMap.begin(); it != IntegrityMap.end(); ++it)
		{
			if ((*it).second->StartChunk == (*it).second->EndChunk)
			{
				leafnode = (*it).second;
				it = IntegrityMap.erase(it);
				break;
			}
			else if ((*it).second->StartChunk == (*it).second->EndChunk - 1)
			{
				unclenode = (*it).second;
				it = IntegrityMap.erase(it);
			}
		}

		if (leafnode == 0 && unclenode == 0)
		{
			LogPrint(LOG_LEVEL_ERROR, "Wrong Peak hash. peak hash count : %d\n", IntegrityMap.size());
			IntegrityMapMutex.unlock();
			return 0;
		}

		if (leafnode != 0)
		{
			ChunkRange = leafnode->EndChunk;
			LastChunkIndex = ChunkRange * 2;
			LeafNodeCount = (int)pow(2, log2(ChunkRange + 1) + 1);
			leafnode->IsLeft = 1;
			HashTreeMap[leafnode->TreeIndex] = leafnode;

			int uncleidx = leafnode->StartChunk + 1;

			unsigned char *hashout = 0;
			unsigned char *allzero = 0;

			if (MerkleHashTreeFunction == MHF_SHA1)
			{
				hashout = new unsigned char[SHA1_DIGEST_BLOCKLEN];
				memset(hashout, 0, SHA1_DIGEST_BLOCKLEN);
				allzero = new unsigned char[SHA1_DIGEST_BLOCKLEN];
				memset(allzero, 0, SHA1_DIGEST_BLOCKLEN);
				SHA1_EncrpytLR(leafnode->Hash, allzero, hashout);
			}
			else // if (MerkleHashTreeFunction == MHF_SHA256)
			{
				hashout = new unsigned char[SHA256_DIGEST_BLOCKLEN];
				memset(hashout, 0, SHA256_DIGEST_BLOCKLEN);
				allzero = new unsigned char[SHA1_DIGEST_BLOCKLEN];
				memset(allzero, 0, SHA1_DIGEST_BLOCKLEN);
				SHA256_EncrpytLR(leafnode->Hash, allzero, hashout);
			}

			auto iter = IntegrityMap.find()
		}
		else if (unclenode != 0)
		{

		}
		
		IntegrityMapMutex.unlock();
		return 1;
	}*/

	return 1;
}
