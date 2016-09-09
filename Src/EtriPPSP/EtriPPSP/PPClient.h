#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include "Common/Common.h"
#include "PPCommon.h"
#include "Binmap.h"
#include "HashTree.h"

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

typedef struct _MessageSt
{
	int Length;
	char* Buffer;

	_MessageSt()
	{
		Length = 0;
		Buffer = 0;
	}

	~_MessageSt()
	{
		if (Buffer != 0)
		{
			delete[] Buffer;
		}
	}
}MessageSt;

class PPClient
{
public:

	PPClient(SOCKET sock, std::string ip, unsigned int port, PPInfo* sinfo);
	PPClient(SOCKET sock, struct sockaddr_in *clientaddr, PPInfo* sinfo);
	~PPClient();

	int ChannelID;
	int DestChannelID;

	std::string IpAddress;
	unsigned int Port;
	std::string SwarmID;

	void SetMessage(int len, char* msg);

	void Start(int chid);
	void Stop();

	void HandleMessage();

	PPInfo *MyInfo;
	PPInfo PeerInfo;

	void (*StopCallback)(PPClient*);
	void SetStopCallback(void callback(PPClient*));
	std::list<PPClient*>* (*PEXCallback)(PPClient *me);
	void SetPEXCallback(std::list<PPClient*>* callback(PPClient *me));
	void KeepAliveCheck();
	void PEXReqCheck();
	
	void ReceiveKeepAlive();

	SOCKET PPSocket;

private:
	char IsStop;
	char IsFirstRequest;
	char IsHandshaked;
	char SendHandshake;
	char IsChoke;

	int LastRequestedIndex;
	int RetryCount;
	int SilentSec;

#if defined(WIN32) || defined(MACOS)
	std::unordered_map<int, char> CancelChunkList;
#else
	unordered_map<int, char> CancelChunkList;
#endif
	std::mutex CancelMutex;
	
	std::queue<MessageSt*> MessageQueue;
	std::mutex MessageMutex;

	std::thread* RecvThread;

	struct sockaddr_in PeerAddr;

	void RetryRequest();

	int CheckHandshaked();

	int SendToPeer(int len, char* msg);
	
	void SendPeakHash();
	
	void SendHandShake();
	void ReceiveHandShake(char* msg);

	void SendHandShakeHave();
	void ReceiveHave(char* msg, int isHandshakeHave = 0);

	void SendRequest(int index);
	void SendNextRequest();
	void ReceiveRequest(char* msg);

	void SendIntegrity(int start, int end, unsigned char* hash);
	void ReceiveIntegrity(char* msg);
	
	void SendData(int index);
	void ReceiveData(char* msg, int len);

	void SendAck(int index, unsigned long long timestamp);
	void ReceiveAck(char* msg);

	void SendKeepAlive();

	void SendPEXReq();
	void ReceivePEXReq(char* msg);

	void SendPEXRes();

	void SendCancel(int start, int end);
	void ReceiveCancel(char* msg);

	void SendChoke();
	void ReceiveChoke(char* msg);
	
	void SendUnchoke();
	void ReceiveUnchoke(char* msg);
};

