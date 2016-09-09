#include "PPServer.h"

PPServer *ThisServer = 0;

void _stopThreadMain(PPClient * ppc)
{
	if (ThisServer != 0)
	{
		ThisServer->ClientStop(ppc);
	}
}

void StopCallback(PPClient * ppc)
{
	std::thread stopThread(_stopThreadMain, ppc);
	stopThread.detach();
}

PPClientsList* PEXCallback(PPClient *me)
{
	if (ThisServer != 0)
	{
		return ThisServer->GetPeerList(me);
	}

	return 0;
}

void _cleanuprecvThreadFunc(void* arg)
{
	PPServer *ppserver = (PPServer*)arg;
	
	delete ppserver;
}

void _recvThreadFunc(void *pInfo)
{
	PPServer *ppserver = (PPServer*)pInfo;
	
// 	pthread_cleanup_push(_cleanuprecvThreadFunc, ppserver);
// 	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
// 	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	LogPrint(LOG_LEVEL_DEBUG, "Start Server recv thread.\n");

	ppserver->Receive();
	
	LogPrint(LOG_LEVEL_DEBUG, "End Server recv thread.\n");

// 	pthread_exit(0);
// 
// 	pthread_cleanup_pop(0);
// 
// 	return 0;
}

PPServer::PPServer()
{
	IsStop = 0;
	Socket = 0;
	
	RecvThread = 0;
	NextChannelID = 1;

	PPClientList.clear();

	TrackerIpAddress = "";
	TrackerPort = 0;

	SetVersion(1);
	SetMinVersion(1);
	SetContentIntegrityProtectionMethod(PP_HANDSHAKE_VALUE_CIPM_MERKLEHASHTREE);
	SetMerkleHashTreeFunction(PP_HANDSHAKE_VALUE_MHF_SHA1);
	//SetLiveSignatureAlgorithm(PP_HANDSHAKE_VALUE_LSA_ECDSAP256SHA256); not use
	SetChunkAddressingMethod(PP_HANDSHAKE_VALUE_CAM_32BITCHUNKRANGES);
	//SetLiveDiscardWindow(0); not use
	SetSupportedMessages(1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0);

	ThisServer = this;
}

PPServer::~PPServer()
{
	LogPrint(LOG_LEVEL_DEBUG, "destroy PP Server start.\n");

	Stop();

	LogPrint(LOG_LEVEL_DEBUG, "destroy PP Server end.\n");

	delete RecvThread;
}

int PPServer::Start()
{
	CreateSocket();

	if (TrackerIpAddress.length() > 0 && Info.SwarmID.length() > 0 && TrackerPort > 0 && Info.Port > 0)
	{
		if (Info.FileName.length() <= 0)
		{
			Info.SetFileName(Info.SwarmID);
		}

		PPClient *ppc = new PPClient(Socket, TrackerIpAddress, TrackerPort, &Info);
		AddPeer(ppc);
	}
	else if (Info.Port > 0 && Info.FileName.length() > 0)
	{
		Info.IsSeeder = 1;

		if (!Info.MakeHashTree())
			return 0;
	}
	else
	{
		LogPrint(LOG_LEVEL_SILENT, "Failed to start server. Check usage.");
		return 0;
	}

	//RecvThread = PTHREAD_CREATE(_recvThreadFunc, this);
	RecvThread = new std::thread(_recvThreadFunc, this);

	PPClientMutex.lock();
	for (PPClientIter it = PPClientList.begin(); it != PPClientList.end(); ++it)
	{
		(*it)->Start(NextChannelID++);
	}
	PPClientMutex.unlock();

	return 1;
}

void PPServer::ClientStop(PPClient *ppc)
{
	if (IsStop) return;
	
	PPClientMutex.lock();

	PPClientList.remove(ppc);
	delete ppc;

	PPClientMutex.unlock();
}

void PPServer::Join()
{
	if (RecvThread->joinable())
		RecvThread->join();
}

void PPServer::Stop()
{
	IsStop = 1;

	if (RecvThread != 0)
	{
		shutdown(Socket, 2);
		close(Socket);
	}

	PPClientMutex.lock();
	for (PPClientIter it = PPClientList.begin(); it != PPClientList.end(); ++it)
	{
		//(*it)->Stop();
		delete (*it);
	}
	PPClientList.clear();
	PPClientMutex.unlock();
}

int PPServer::CreateSocket()
{
	struct sockaddr_in server_addr;

	if ((Socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)  {
		LogPrint(LOG_LEVEL_ERROR, "Server: Can't open socket.\n");
		return 0;
	}

	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	
	if (Info.IpAddress.length() > 0)
	{
		server_addr.sin_addr.s_addr = inet_addr(Info.IpAddress.c_str());
	}
	else
	{
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	server_addr.sin_port = htons(Info.Port);

	if (bind(Socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		LogPrint(LOG_LEVEL_ERROR, "Server: Can't bind local address.\n");
		return 0;
	}

	return 1;
}

void PPServer::Receive()
{
	if (Socket <= 0) return;

	int ret = 0;
	socklen_t len = 0;
	char buf[MAX_RECV_DGRAM_SIZE];
	
	while (!IsStop)
	{
		memset(buf, 0, MAX_RECV_DGRAM_SIZE);

		struct sockaddr_in client_addr;

		len = sizeof(client_addr);

		memset(&client_addr, 0, len);
		
		ret = recvfrom(Socket, buf, MAX_RECV_DGRAM_SIZE, 0, (struct sockaddr *)&client_addr, &len);

		if (ret <= 0)
		{
			//LogPrint(LOG_LEVEL_ERROR, "recvfrom error.\n");
			SLEEP(100);
			continue;
		}

		if (IsStop)
		{
			break;
		}

		int channelID = 0;

		memcpy(&channelID, buf, 4);
		channelID = CalcEndianN2H(channelID);

		if (channelID >= NextChannelID)
		{
			continue;
		}

		PPClient* client = 0;
		if (channelID == 0)
		{
			//u_short aa = client_addr.sin_port;
			//aa = ntohs(aa);

			//client = new PPClient(inet_ntoa(client_addr.sin_addr), aa, &Info);
			client = new PPClient(Socket, &client_addr, &Info);
			client->ChannelID = NextChannelID++;
			AddPeer(client);

			LogPrint(LOG_LEVEL_SILENT, "new peer %s, %d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
		}
		else
		{
			client = GetPeer(channelID);
		}

		if (client != 0)
		{
			if (ret == 4)
			{
				client->ReceiveKeepAlive();
			}
			else
			{
				if (buf[4] == PP_MESSAGETYPE_PEXRESV4)
				{
					PEXRes pexres;
					pexres.Create(buf);

					AddPeer(pexres.IPv4AddressString, pexres.Port);
				}
				else
				{
					client->SetMessage(ret, buf);
				}
			}
		}
	}
}

void PPServer::AddPeer(std::string ip, unsigned int port)
{
	if (GetPeer(ip, port) != 0)
	{
		return;
	}

	LogPrint(LOG_LEVEL_SILENT, "Connect to peer %s, %d\n", ip.c_str(), port);
	
	PPClient* client = new PPClient(Socket, ip, port, &Info);
	client->SetStopCallback(StopCallback);
	client->SetPEXCallback(PEXCallback);

	PPClientMutex.lock();
	if (!IsStop) PPClientList.push_back(client);
	PPClientMutex.unlock();

	if (!Info.IsSeeder)
	{
		client->Start(NextChannelID++);
	}
}

void PPServer::AddPeer(PPClient* client)
{
	client->SetStopCallback(StopCallback);
	client->SetPEXCallback(PEXCallback);
	PPClientMutex.lock();
	if (!IsStop) PPClientList.push_back(client);
	PPClientMutex.unlock();
}

PPClient* PPServer::GetPeer(int channelID)
{
	PPClient *client = 0;

	if (channelID > 0)
	{
		PPClientMutex.lock();
		for (PPClientIter it = PPClientList.begin(); it != PPClientList.end(); ++it)
		{
			if ((*it)->ChannelID == channelID)
			{
				client = (*it);
				break;
			}
		}
		PPClientMutex.unlock();
	}

	return client;
}

PPClient* PPServer::GetPeer(std::string ipaddr, unsigned int port)
{
	PPClient *client = 0;

	PPClientMutex.lock();
	for (PPClientIter it = PPClientList.begin(); it != PPClientList.end(); ++it)
	{
		if ((*it)->IpAddress == ipaddr && (*it)->Port == port)
		{
			client = (*it);
			break;
		}
	}
	PPClientMutex.unlock();

	return client;
}

PPClientsList* PPServer::GetPeerList(PPClient *me)
{
	PPClientsList* list = new PPClientsList();

	PPClientMutex.lock();

	for (PPClientIter it = PPClientList.begin(); it != PPClientList.end(); ++it)
	{
		if ((*it) == me) continue;
		
		if ((*it)->Port > 0)
		{
			PPClient *client = new PPClient(0, (*it)->IpAddress, (*it)->Port, 0);
			list->push_back(client);
		}
	}
	PPClientMutex.unlock();

	return list;
}

void PPServer::SetPort(int port)
{
	Info.Port = port;
}

void PPServer::SetIpAddress(std::string ipaddress)
{
	Info.IpAddress = ipaddress;
}

void PPServer::SetFileName(std::string filename)
{
	Info.SetFileName(filename);
}

void PPServer::SetSwarmID(std::string hash)
{
	Info.SetSwarmID(hash);
}

void PPServer::SetTrackerIpAddress(std::string ip)
{
	TrackerIpAddress = ip;
}

void PPServer::SetTrackerPort(int port)
{
	TrackerPort = port;
}

void PPServer::SetVersion(char version)
{
	Info.Version = version;
}

void PPServer::SetMinVersion(char minversion)
{
	Info.MinVersion = minversion;
}

void PPServer::SetContentIntegrityProtectionMethod(char cipm)
{
	Info.ContentIntegrityProtectionMethod = cipm;
}

void PPServer::SetMerkleHashTreeFunction(char mhf)
{
	Info.MerkleHashTreeFunction = mhf;
}

void PPServer::SetLiveSignatureAlgorithm(char lsa)
{
	Info.LiveSignatureAlgorithm = lsa;
}

void PPServer::SetChunkAddressingMethod(char cam)
{
	Info.ChunkAddressingMethod = cam;
}

void PPServer::SetLiveDiscardWindow(char* ldw)
{
	Info.LiveDiscardWindow = ldw;
}

void PPServer::SetChunkSize(int size)
{
	Info.ChunkSize = size;
}

void PPServer::SetRetryCount(int cnt)
{
	Info.RetryCount = cnt;
}

void PPServer::SetKeepAliveInterval(int sec)
{
	Info.KeepAliveInterval = sec;
}

void PPServer::SetPexReqInterval(int sec)
{
	Info.PEX_REQInterval = sec;
}

void PPServer::SetListenPortOption(int use)
{
	Info.ListenPortOption = use;
}

unsigned char _setBit(unsigned char x, int n, int b)
{
	if (b)
		return (unsigned char)(x | (1 << n));

	return (unsigned char)(x & (~(1 << n)));
}

void PPServer::SetSupportedMessages(int handshake, int data, int ack, int have, int integrity, int pex_resv4, int pex_req, int signed_integrity, int request, int cancel, int choke, int unchoke, int pex_resv6, int pex_rescert)
{
	Info.SupportedMessagesBitmapLength = 2;
	Info.SupportedMessagesBitmap = new unsigned char[2];

	Info.SupportedMessagesBitmap[0] = 0;
	Info.SupportedMessagesBitmap[0] = _setBit(Info.SupportedMessagesBitmap[0], 7, handshake);
	Info.SupportedMessagesBitmap[0] = _setBit(Info.SupportedMessagesBitmap[0], 6, data);
	Info.SupportedMessagesBitmap[0] = _setBit(Info.SupportedMessagesBitmap[0], 5, ack);
	Info.SupportedMessagesBitmap[0] = _setBit(Info.SupportedMessagesBitmap[0], 4, have);
	Info.SupportedMessagesBitmap[0] = _setBit(Info.SupportedMessagesBitmap[0], 3, integrity);
	Info.SupportedMessagesBitmap[0] = _setBit(Info.SupportedMessagesBitmap[0], 2, pex_resv4);
	Info.SupportedMessagesBitmap[0] = _setBit(Info.SupportedMessagesBitmap[0], 1, pex_req);
	Info.SupportedMessagesBitmap[0] = _setBit(Info.SupportedMessagesBitmap[0], 0, signed_integrity);

	Info.SupportedMessagesBitmap[1] = 0;
	Info.SupportedMessagesBitmap[1] = _setBit(Info.SupportedMessagesBitmap[1], 7, request);
	Info.SupportedMessagesBitmap[1] = _setBit(Info.SupportedMessagesBitmap[1], 6, cancel);
	Info.SupportedMessagesBitmap[1] = _setBit(Info.SupportedMessagesBitmap[1], 5, choke);
	Info.SupportedMessagesBitmap[1] = _setBit(Info.SupportedMessagesBitmap[1], 4, unchoke);
	Info.SupportedMessagesBitmap[1] = _setBit(Info.SupportedMessagesBitmap[1], 3, pex_resv6);
	Info.SupportedMessagesBitmap[1] = _setBit(Info.SupportedMessagesBitmap[1], 2, pex_rescert);

	char s[16 + 1] = { '0', };
	
	int count = 16;

	for (int i = 1; i >= 0; i--)
	{
		char b = Info.SupportedMessagesBitmap[i];

		for (int j = 8; j > 0; j--)
		{
			s[--count] = '0' + (char)(b & 1);
			b = b >> 1;
		}
	}
	
	LogPrint(LOG_LEVEL_SILENT, "SupportedMessagesBitmap : %s\n", s);
}