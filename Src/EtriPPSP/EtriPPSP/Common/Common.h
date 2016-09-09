#pragma once

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#define close(x)			closesocket(x)
#define sscanf				sscanf_s
//#include "pthreadWin32/include/pthread.h"

#pragma comment (lib, "ws2_32.lib")
//#pragma comment (lib, "Common/pthreadWin32/lib/pthreadVC.lib")
typedef int socklen_t;
#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
typedef int SOCKET;
//#include <pthread.h>

#endif

#define DEFAULT_CHUNK_SIZE 1024
#define MAX_UDP_OVER_ETH_PAYLOAD (1500-20-8)
#define MAX_NONDATA_DGRAM_SIZE (MAX_UDP_OVER_ETH_PAYLOAD-DEFAULT_CHUNK_SIZE-1-4)
#define MAX_SEND_DGRAM_SIZE (MAX_NONDATA_DGRAM_SIZE+1+4+8192)
#define MAX_RECV_DGRAM_SIZE (MAX_SEND_DGRAM_SIZE*2)

#define CONF_KEY_VERSION "[VERSION]\n"
#define CONF_KEY_MINVERSION "[MINVERSION]\n"
#define CONF_KEY_CONTENT_INTEGRITY_PROTECTION_METHOD "[CONTENT_INTEGRITY_PROTECTION_METHOD]\n"
#define CONF_KEY_MERKLE_HASH_TREE_FUNCTION "[MERKLE_HASH_TREE_FUNCTION]\n"
#define CONF_KEY_LIVE_SIGNATURE_ALGORITHM "[LIVE_SIGNATURE_ALGORITHM]\n"
#define CONF_KEY_CHUNK_ADDRESSING_METHOD "[CHUNK_ADDRESSING_METHOD]\n"
#define CONF_KEY_CHUNK_SIZE "[CHUNK_SIZE]\n"
#define CONF_KEY_KEEP_ALIVE_INTERVAL "[KEEP_ALIVE_INTERVAL]\n"
#define CONF_KEY_PEX_REQ_INTERVAL "[PEX_REQ_INTERVAL]\n"
#define CONF_KEY_RETRY_COUNT "[RETRY_COUNT]\n"