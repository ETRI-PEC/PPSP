#pragma once

#include "Common/Common.h"
#include "Common/Util.h"

#include "PP/HandShake.h"
#include "PP/PEXRes.h"
#include "PPClient.h"
#include "PPCommon.h"
#include <list>
#include <mutex>
#include <thread>

typedef std::list<PPClient*> PPClientsList;
typedef PPClientsList::iterator PPClientIter;

class PPServer
{
public:
	PPServer();
	~PPServer();

// 	string FileName;
// 	string IpAddress;
// 	string SwarmID;
// 	unsigned int Port;
// 
// 	int IsSeeder;
// 	int ChunkSize;

	PPInfo Info;

	std::thread* RecvThread;
	
	int Start();
	void Stop();

	int CreateSocket();
	void Receive();
	void AddPeer(std::string ip, unsigned int port);
	void AddPeer(PPClient* client);
	PPClient* GetPeer(int channelID);
	PPClient* GetPeer(std::string ipaddr, unsigned int port);
	PPClientsList* GetPeerList(PPClient *me);

	void SetPort(int port);
	void SetIpAddress(std::string ipaddress);
	void SetFileName(std::string filename);
	void SetSwarmID(std::string hash);
	void SetTrackerIpAddress(std::string ip);
	void SetTrackerPort(int port);

	void SetVersion(char version);
	void SetMinVersion(char minversion);
	void SetContentIntegrityProtectionMethod(char cipm);
	void SetMerkleHashTreeFunction(char mhf);
	void SetLiveSignatureAlgorithm(char lsa);
	void SetChunkAddressingMethod(char cam);
	void SetLiveDiscardWindow(char* ldw);
	void SetChunkSize(int size);
	void SetSupportedMessages(int handshake, int data, int ack, int have, int integrity, int pex_resv4, int pex_req, int signed_integrity, int request, int cancel, int choke, int unchoke, int pex_resv6, int pex_rescert);

	void SetRetryCount(int cnt);
	void SetKeepAliveInterval(int sec);
	void SetPexReqInterval(int sec);
	void SetListenPortOption(int use);

	void ClientStop(PPClient *ppc);

	void Join();
private:
	int NextChannelID;
	int IsStop;

	SOCKET Socket;

	PPClientsList PPClientList;
	std::mutex PPClientMutex;

	//HashTree FileHashTree;

	std::string TrackerIpAddress;
	int TrackerPort;

	void *recvThreadFunc(void *pInfo);
	void cleanuprecvThreadFunc(void* arg);
};


