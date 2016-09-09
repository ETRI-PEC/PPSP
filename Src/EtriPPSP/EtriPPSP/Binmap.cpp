#include "Binmap.h"
#include <string.h>
#include "Common/Util.h"
Binmap::Binmap()
{
	FilePointer = 0;

	LastChunk = -1;
	LastChunkSize = 0;

	CP = -1;
}


Binmap::~Binmap()
{
	ChunkRangeMutex.lock();
	for (ChunkRangeStIter it = ChunkRangeList.begin(); it != ChunkRangeList.end(); ++it)
	{
		delete (*it);
	}
	ChunkRangeMutex.unlock();
	
	RequestedMutex.lock();
	RequestedMap.clear();
	RequestedMutex.unlock();
	
	FileMutex.lock();
	if (FilePointer != 0)
	{
		fclose(FilePointer);
	}
	FileMutex.unlock();
}

void Binmap::SetHave(Have *have)
{
	SetHave(have->StartChunk, have->EndChunk);
}

void Binmap::SetHave(int start, int end)
{
	ChunkRangeMutex.lock();
	ChunkRangeStIter changeit = ChunkRangeList.end();

	for (ChunkRangeStIter it = ChunkRangeList.begin(); it != ChunkRangeList.end(); ++it)
	{
		if ((*it)->Start <= start && (*it)->End >= end)
		{
			ChunkRangeMutex.unlock();
			return;
		}
		else if ((*it)->Start <= start && (*it)->End + 1 >= start && (*it)->End < end)
		{
			changeit = it;
			//newend = end;
			(*it)->End = end;

			if ((*it)->Start == 0)
			{
				CP = end;
			}

			break;
		}
		else if ((*it)->Start > start && (*it)->End >= end && (*it)->Start - 1 <= end)
		{
			changeit = it;
			//newstart = start;
			(*it)->Start = start;
		
			if ((*it)->Start == 0)
			{
				CP = (*it)->End;
			}

			break;
		}
		else if ((*it)->Start > start && (*it)->End < end)
		{
			changeit = it;
			//newstart = start;
			//newend = end;
			(*it)->Start = start;
			(*it)->End = end;

			if ((*it)->Start == 0)
			{
				CP = end;
			}

			break;
		}
	}

	//if ((newstart > -1 || newend > -1) && ChunkRangeList.size() > 1)
	if (changeit != ChunkRangeList.end() && ChunkRangeList.size() > 1)
	{
		for (ChunkRangeStIter it = ChunkRangeList.begin(); it != ChunkRangeList.end(); ++it)
		{
			int set = 0;

			if ((*it)->End + 1 >= (*changeit)->Start && (*it)->End < (*changeit)->End)
			{
				(*it)->End = (*changeit)->End;
				if ((*it)->Start == 0 && CP < (*it)->End)
				{
					CP = (*it)->End;
				}
				set = 1;
			}

			if ((*it)->Start - 1 <= (*changeit)->End && (*it)->Start > (*changeit)->End)
			{
				(*it)->Start = (*changeit)->Start;
				if ((*it)->Start == 0 && CP < (*it)->End)
				{
					CP = (*it)->End;
				}
				set = 1;
			}

			if (set)
			{
				ChunkRangeList.erase(changeit);
				break;
			}
		}
	}
	else if (changeit == ChunkRangeList.end())
	{
		ChunkRangeSt *crst = new ChunkRangeSt;
		crst->Start = start;
		crst->End = end;
		ChunkRangeList.push_back(crst);
	}

	ChunkRangeMutex.unlock();
}

int Binmap::IsHaveData(int index)
{
	for (ChunkRangeStIter it = ChunkRangeList.begin(); it != ChunkRangeList.end(); ++it)
	{
		if (((*it)->Start <= index && (*it)->End >= index) || RequestedMap.find(index) != RequestedMap.end())
		{
			return 1;
		}	
	}

	return 0;
}

int Binmap::GetRequestData(Binmap* peerbinmap)
{
	RequestedMutex.lock();
	if (CP >= 0)
	{
		if (CP == LastChunk)
		{
			RequestedMutex.unlock();
			return -1;
		}

		for (int idx = CP + 1; idx <= LastChunk; idx++)
		{
			ChunkRangeMutex.lock();
			if (!IsHaveData(idx))
			{
				peerbinmap->ChunkRangeMutex.lock();
				for (ChunkRangeStIter it = peerbinmap->ChunkRangeList.begin(); it != peerbinmap->ChunkRangeList.end(); ++it)
				{
					if ((*it)->Start <= idx && (*it)->End >= idx)
					{
						SetRequested(idx);
						
						peerbinmap->ChunkRangeMutex.unlock();
						ChunkRangeMutex.unlock();
						RequestedMutex.unlock();
						return idx;
					}
				}
				peerbinmap->ChunkRangeMutex.unlock();
			}
			ChunkRangeMutex.unlock();
		}
	}
	else
	{
		ChunkRangeMutex.lock();
		peerbinmap->ChunkRangeMutex.lock();
		for (ChunkRangeStIter it = peerbinmap->ChunkRangeList.begin(); it != peerbinmap->ChunkRangeList.end(); ++it)
		{
			for (int i = (*it)->Start; i <= (*it)->End; i++)
			{
				if (!IsHaveData(i))
				{
					SetRequested(i);
					peerbinmap->ChunkRangeMutex.unlock();
					ChunkRangeMutex.unlock();
					RequestedMutex.unlock();
					return i;
				}
			}
		}
		peerbinmap->ChunkRangeMutex.unlock();
		ChunkRangeMutex.unlock();
	}

	RequestedMutex.unlock();

	return -1;
}

int Binmap::SetRequested(int index)
{
	if (RequestedMap.find(index) != RequestedMap.end())
	{
		return 0;
	}
	RequestedMap[index] = 1;

	return 1;
}

void Binmap::ReleaseRequested(int index)
{
	if (index < 0) return;
	
	RequestedMutex.lock();
	RequestedMap.erase(index);
	RequestedMutex.unlock();
}

char* Binmap::GetData(int size, int index, int *length)
{
	FileMutex.lock();

	if (FilePointer == 0)
	{
		FilePointer = fopen(FileName.c_str(), "r+b");
	}

	if (FilePointer != 0)
	{
		char* buf = new char[size];
		memset(buf, 0, size);

		fseek(FilePointer, index * size, SEEK_SET);

		*length = fread(buf, 1, size, FilePointer);

		if (*length != size)
		{
			char* newbuf = new char[*length];
			memcpy(newbuf, buf, *length);
			delete[] buf;

			FileMutex.unlock();

			return newbuf;
		}

		FileMutex.unlock();

		return buf;
	}

	FileMutex.unlock();

	return 0;
}

void Binmap::SetFileName(std::string name)
{
	FileName = name;
}

int Binmap::CheckFinish()
{
	int start = -1;
	int end = -1;

	ChunkRangeMutex.lock();

	ChunkRangeStList tmpList;

	for (ChunkRangeStIter it = ChunkRangeList.begin(); it != ChunkRangeList.end(); ++it)
	{
		ChunkRangeSt *crst = new ChunkRangeSt;
		crst->Start = (*it)->Start;
		crst->End = (*it)->End;
		tmpList.push_back(crst);
	}

	while (true)
	{
		unsigned int bfcnt = tmpList.size();

		for (ChunkRangeStIter it = tmpList.begin(); it != tmpList.end(); )
		{
			if (start < 0)
			{
				start = (*it)->Start;
				end = (*it)->End;

				it = tmpList.erase(it);

				continue;
			}
			else
			{
				if ((*it)->Start < start && (*it)->End <= end && (*it)->End >= start - 1)
				{
					start = (*it)->Start;

					it = tmpList.erase(it);
					
					continue;
				}

				if ((*it)->Start >= start && (*it)->End > end && (*it)->Start <= end + 1)
				{
					end = (*it)->End;

					it = tmpList.erase(it);

					continue;
				}

				if ((*it)->Start <= start && (*it)->End >= end)
				{
					start = (*it)->Start;
					end = (*it)->End;

					it = tmpList.erase(it);

					continue;
				}
			}

			it++;
		}

		if (tmpList.size() <= 0)
		{
			break;
		}

		if (bfcnt == tmpList.size())
		{
			break;
		}
	}
		
	ChunkRangeMutex.unlock();

	tmpList.clear();

	if (start == 0 && end == LastChunk)
	{
		LogPrint(LOG_LEVEL_SILENT, "Content download finished.\n");
		FileMutex.lock();
		if (FilePointer != 0)
		{
			fclose(FilePointer);
			FilePointer = 0;
		}
		FileMutex.unlock();
		return 1;
	}
	
	return 0;
}

int Binmap::SetData(int start, int end, int size, char* data)
{
	FileMutex.lock();

	if (FilePointer == 0)
	{
		FilePointer = fopen(FileName.c_str(), "w+b");
	}

	if (FilePointer != 0 && data != 0)
	{
		SetHave(start, end);
		
		for (int i = start; i <= end; i++)
		{
			ReleaseRequested(i);
		}

		fseek(FilePointer, start * size, SEEK_SET);

		int datasize = size * (end - start + 1);

		if (end == LastChunk)
		{
			datasize -= size - LastChunkSize;
		}

		int ret = fwrite(data, 1, datasize, FilePointer);

		FileMutex.unlock();

		if (LogPrintSize < end)
		{
			std::string tmpnow = std::to_string(end);
			std::string tmplast = std::to_string(LogPrintSize);

			if (LogPrintSize <= 0 || tmplast[0] != tmpnow[0])
			{
				LogPrint(LOG_LEVEL_SILENT, "Chunk downloaded : %d\n", end);
			}

			if (LogPrintSize == LastChunk)
			{
				LastChunkSize = 0;
			}
		}

		LogPrintSize = end;

		return ret;
	}

	FileMutex.unlock();

	return 0;
}

void Binmap::SetLastChunk(int last, int size)
{
	LastChunk = last;
	LastChunkSize = size;
}

void Binmap::GetBiggestHavedChunkRange(int* start, int* end)
{
	int s = -1;
	int e = -1;
	int gap = 0;
	
	ChunkRangeMutex.lock();
	for (ChunkRangeStIter it = ChunkRangeList.begin(); it != ChunkRangeList.end(); ++it)
	{
		int g = (*it)->End - (*it)->Start + 1;
		if (g > gap)
		{
			gap = g;
			s = (*it)->Start;
			e = (*it)->End;
		}
	}
	ChunkRangeMutex.unlock();

	*start = s;
	*end = e;
}
