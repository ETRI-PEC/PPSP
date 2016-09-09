#pragma once
#include <string>
#include "Common.h"

#define LOG_LEVEL_SILENT 3
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_DEBUG 1

#define LOG_LEVEL LOG_LEVEL_SILENT

void SetLogLevel(int level);

int HTTP_Startup();
void HTTP_Cleanup();
void SLEEP(unsigned long ulMilliseconds);

//pthread_t PTHREAD_CREATE(void *(*start) (void *), void *param);

void LogPrint(int level, const char *format, ...);
template <class T>
void endswap(T *objp);
int IsLittleEndian();
int CalcEndianH2N(int val);
short CalcEndianH2N(short val);
int CalcEndianN2H(int val);
short CalcEndianN2H(short val);
unsigned long long CalcEndianH2N(unsigned long long val);
unsigned long long CalcEndianN2H(unsigned long long val);

unsigned long long GetCurrentTimeMicrosecond();

std::string GetStringFromHash(unsigned char* hash, int len);
unsigned char* GetHashFromString(std::string str);
char* BitToStr(char* val, int len);
