#include <stdarg.h>
#include <algorithm>
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include < time.h >
#define close(x)			closesocket(x)
#define sscanf				sscanf_s
//#include "pthreadWin32/include/pthread.h"

#pragma comment (lib, "ws2_32.lib")
//#pragma comment (lib, "Common/pthreadWin32/lib/pthreadVC.lib")

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone
{
	int  tz_minuteswest;
	int  tz_dsttime;
};

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

//#include <pthread.h>

#include <sys/time.h>
#endif

#include "Util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int HTTP_Startup()
{
#ifdef WIN32
	int nErrorStatus;
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;

	nErrorStatus = WSAStartup(wVersionRequested, &wsaData);
	if (nErrorStatus != 0)
	{
		return 0;
	}
#endif
	return 1;
}

void HTTP_Cleanup()
{
#ifdef WIN32
	WSACleanup();
#endif
}

void SLEEP(unsigned long ulMilliseconds)
{
#ifdef WIN32
	Sleep(ulMilliseconds);
#else
	usleep(ulMilliseconds * 1000);
#endif
}

/*pthread_t PTHREAD_CREATE(void *(*start) (void *), void *param)
{
	pthread_t tid;
	pthread_attr_t attr;

	if (pthread_attr_init(&attr) < 0) return 0;
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
	{
		pthread_attr_destroy(&attr);
		return 0;
	}
	if (pthread_create(&tid, &attr, start, param) != 0)
	{
		pthread_attr_destroy(&attr);
		return 0;
	}
	return tid;
}*/

int SHOW_LOG_LEVEL = LOG_LEVEL_ERROR;

void SetLogLevel(int level)
{
	SHOW_LOG_LEVEL = level;
}

void LogPrint(int level, const char *format, ...)
{
	//if (LOG_LEVEL > level) return;
	if (SHOW_LOG_LEVEL > level) return;
	
	va_list arglist;
	//	int buffing;
	//int retval;
	char szBuf[4096];

	static int isFirst = 1;
	FILE *fp;
	char filename[20];

	sprintf(filename, "ppsp.log");

	va_start(arglist, format);

	if (level > LOG_LEVEL_DEBUG)
	{
#ifdef WIN32
	
		vsprintf(szBuf, format, arglist);
		OutputDebugStringA(szBuf);
		printf(szBuf);
#else
		vprintf(format, arglist);
#endif // WIN32
	}	

	if (isFirst)
	{
		isFirst = 0;
		fp = fopen(filename, "w+");
	}
	else
	{
		fp = fopen(filename, "a");
	}

	if (fp)
	{
		fwrite(szBuf, strlen(szBuf), 1, fp);
		//fwrite("\n", strlen("\n"), 1, fp);

		fclose(fp);
	}

	va_end(arglist);
	fflush(stdout);
}

template <class T>
void endswap(T *objp)
{
	unsigned char *memp = reinterpret_cast<unsigned char*>(objp);
	std::reverse(memp, memp + sizeof(T));
}

int IsLittleEndian()
{
	short a = 0x0001;

	char *as = (char *)&a;

	if (as[0])
		return 1;
	
	return 0;
}

short CalcEndianH2N(short val)
{
	return htons(val);
}

int CalcEndianH2N(int val)
{
	return htonl(val);
}

unsigned long long CalcEndianH2N(unsigned long long val)
{
	if (IsLittleEndian())
	{
		unsigned long long retval;

		retval = val;

		retval = ((unsigned long long) htonl(val & 0xFFFFFFFF)) << 32;
		retval |= htonl((val & 0xFFFFFFFF00000000) >> 32);

		return(retval);
	}

	return val;
}

short CalcEndianN2H(short val)
{
	return ntohs(val);
}

int CalcEndianN2H(int val)
{
	return ntohl(val);
}

unsigned long long CalcEndianN2H(unsigned long long val)
{
	if (IsLittleEndian())
	{
		unsigned long long retval;

		retval = val;

		retval = ((unsigned long long) ntohl(val & 0xFFFFFFFF)) << 32;
		retval |= ntohl((val & 0xFFFFFFFF00000000) >> 32);

		return(retval);
	}

	return val;
}

std::string GetStringFromHash(unsigned char* hash, int len)
{
	std::string rslt = "";
	
	for (int i = 0; i < len; i++)
	{
		char tmp[5];
		//sprintf(tmp + (i * 2), "%02x", (len * 2 + 1) - (i * 2), hash[i]);
		sprintf(tmp, "%02x", hash[i]);
		rslt.append(tmp);
	}

	return rslt;
}

unsigned char* GetHashFromString(std::string str)
{
	const char *hash = str.c_str();

	int len = str.length() / 2;

	unsigned char *bits = new unsigned char[len];
	memset(bits, 0, len);

	int val;
	for (int i = 0; i < 20; i++) {
		if (sscanf(hash + i * 2, "%2x", &val) != 1) {
			memset(bits, 0, len);
			return 0;
		}
		bits[i] = val;
	}

	return bits;
}

char* BitToStr(char* val, int len)
{
	static char buf[100];

	int   l = 0;
	int   i = 0;

	memset(buf, 0, 100);

#define PRN_BIT(v) (v) ? (buf[l++] = '1') : (buf[l++] = '0')
#define PRN_SPC()  buf[l++] = ' '

	for (i = 0; i < len; ++i) {
		PRN_BIT(val[i] & 128);
		PRN_BIT(val[i] & 64);
		PRN_BIT(val[i] & 32);
		PRN_BIT(val[i] & 16);
		PRN_BIT(val[i] & 8);
		PRN_BIT(val[i] & 4);
		PRN_BIT(val[i] & 2);
		PRN_BIT(val[i] & 1);

		if (i != len - 1) PRN_SPC();
	}

#undef PRN_BIT
#undef PRN_SPC

	buf[l] = '\0';

	return buf;
}

unsigned long long GetCurrentTimeMicrosecond()
{
	unsigned long long timestamp_usec = 0;

#ifdef WIN32
	FILETIME ft;
	unsigned long long tmpres = 0;
	//static int tzflag;

	GetSystemTimeAsFileTime(&ft);

	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;

	tmpres -= DELTA_EPOCH_IN_MICROSECS;

	tmpres /= 10;

	//tv->tv_sec = (tmpres / 1000000UL);
	//tv->tv_usec = (tmpres % 1000000UL);

	timestamp_usec = tmpres + (unsigned long long)(tmpres % 1000000UL);

	// timezone Ã³¸®
	/*if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}*/
#else
	struct timeval timer_usec;
	if (!gettimeofday(&timer_usec, NULL))
	{
		timestamp_usec = ((unsigned long long) timer_usec.tv_sec) * 1000000ll + (unsigned long long) timer_usec.tv_usec;
	}
#endif

	return timestamp_usec;
}