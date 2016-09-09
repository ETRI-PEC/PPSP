#pragma once
#include "PP/Have.h"
#include "Common/Common.h"
#include <list>
#include <mutex>
#include <string>

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

typedef struct _ChunkRangeSt
{
	int Start;
	int End;

	_ChunkRangeSt()
	{
		Start = 0;
		End = 0;
	}

}ChunkRangeSt;

typedef std::list<ChunkRangeSt*> ChunkRangeStList;
typedef std::list<ChunkRangeSt*>::iterator ChunkRangeStIter;

#if defined(WIN32) || defined(MACOS)
typedef std::unordered_map<int, int> DoubleIntMap;
typedef std::unordered_map<int, int>::iterator DoubleIntIter;
#else
typedef unordered_map<int, int> DoubleIntMap;
typedef unordered_map<int, int>::iterator DoubleIntIter;
#endif

class Binmap
{
private:
	DoubleIntMap RequestedMap;
	std::mutex RequestedMutex;
	std::string FileName;
	FILE *FilePointer;
	std::mutex FileMutex;

	int LogPrintSize;

	int CP;

	ChunkRangeStList ChunkRangeList;
public:
	Binmap();
	~Binmap();

	std::mutex ChunkRangeMutex;

	int LastChunk;
	int LastChunkSize;

	void SetHave(Have *have);
	void SetHave(int start, int end);
	int GetRequestData(Binmap* binmap);
	int IsHaveData(int index);
	int SetRequested(int index);
	void ReleaseRequested(int index);
	char* GetData(int size, int index, int *length);
	void SetFileName(std::string name);
	int SetData(int start, int end, int size, char* data);
	void SetLastChunk(int last, int size);
	void GetBiggestHavedChunkRange(int* start, int* end);
	int CheckFinish();
};

