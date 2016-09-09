#include "PPClient.h"
#include "Common/Util.h"
#include "PP/HandShake.h"
#include "PP/Have.h"
#include "PP/Request.h"
#include "PP/Data.h"
#include "PP/Ack.h"
#include "PP/Integrity.h"
#include "PP/PEXReq.h"
#include "PP/PEXRes.h"
#include "PP/Cancel.h"
#include "PP/Choke.h"
#include "PP/Unchoke.h"

PPClient::PPClient(SOCKET sock, struct sockaddr_in *clientaddr, PPInfo* sinfo)
{
	//IpAddress = ip;
	//Port = port;
	MyInfo = sinfo;
	
	IsStop = 0;

	IsChoke = 0;

	ChannelID = 0;
	DestChannelID = 0;

	RecvThread = 0;

	SwarmID = MyInfo->SwarmID;

	PPSocket = sock;
	memcpy(&PeerAddr, clientaddr, sizeof(struct sockaddr_in));

	IpAddress = inet_ntoa(clientaddr->sin_addr);

	IsFirstRequest = 1;
	IsHandshaked = 0;
	SendHandshake = 0;

	LastRequestedIndex = -1;

	SilentSec = 0;
}

PPClient::PPClient(SOCKET sock, std::string ip, unsigned int port, PPInfo* sinfo)
{
	IpAddress = ip;
	Port = port;
	MyInfo = sinfo;

	IsStop = 0;

	IsChoke = 0;

	ChannelID = 0;
	DestChannelID = 0;

	RecvThread = 0;

	IsFirstRequest = 1;
	IsHandshaked = 0;
	SendHandshake = 0;

	LastRequestedIndex = -1;

	SilentSec = 0;

	if (MyInfo != 0)
	{
		SwarmID = MyInfo->SwarmID;
	}

	PPSocket = sock;
	
	memset(&PeerAddr, 0, sizeof(PeerAddr));
	PeerAddr.sin_family = AF_INET;
	PeerAddr.sin_addr.s_addr = inet_addr(IpAddress.c_str());
	PeerAddr.sin_port = htons(Port);
}

PPClient::~PPClient()
{
	if (RecvThread->joinable())
		RecvThread->join();

	delete RecvThread;

	MessageMutex.lock();
	while (!MessageQueue.empty())
	{
		MessageSt* msg = MessageQueue.front();
		MessageQueue.pop();
		delete msg;
	}
	MessageMutex.unlock();
}
/*
void _cleanupCliRecvThreadFunc(void* arg)
{
	PPClient *ppclient = (PPClient*)arg;

	//ppclient->Stop();
}*/

void _cliRecvThreadFunc(void *pInfo)
{
	PPClient *ppclient = (PPClient*)pInfo;

	//pthread_cleanup_push(_cleanupCliRecvThreadFunc, ppclient);
	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	ppclient->HandleMessage();
	
	//pthread_exit(0);

	//pthread_cleanup_pop(0);
}

void _keepAliveThreadFunc(void *pInfo)
{
	PPClient *ppclient = (PPClient*)pInfo;

	ppclient->KeepAliveCheck();
}

void _pexreqThreadThreadFunc(void *pInfo)
{
	PPClient *ppclient = (PPClient*)pInfo;

	ppclient->PEXReqCheck();
}

void PPClient::PEXReqCheck()
{
	if (MyInfo == 0) return;
	
	int sendcnt = MyInfo->PEX_REQInterval + 1;

	while (!IsStop)
	{
		if (IsStop) break;

		if (MyInfo == 0) return;

		if (MyInfo->PEX_REQInterval < sendcnt)
		{
			sendcnt = 0;
			SendPEXReq();
			continue;
		};

		sendcnt++;

		SLEEP(1000);
	}
}

void PPClient::KeepAliveCheck()
{
	int sendcnt = 0;

	while (!IsStop)
	{
		SLEEP(1000);

		if (IsStop) break;

		if (MyInfo == 0) return;
		
		if (MyInfo->KeepAliveInterval < SilentSec)
		{
			Stop();
			break;
		}

		SilentSec++;
		sendcnt++;

		if (MyInfo == 0) return;

		if (sendcnt > MyInfo->KeepAliveInterval - (MyInfo->KeepAliveInterval * 0.2))
		{
			SendKeepAlive();
			sendcnt = 0;
		}
	}
}

void PPClient::SetStopCallback(void callback(PPClient*))
{
	StopCallback = callback;
}

void PPClient::SetPEXCallback(std::list<PPClient*>* callback(PPClient *me))
{
	PEXCallback = callback;
}

void PPClient::Start(int chid)
{
	/*if (PPSocket == 0)
	{
		PPSocket = socket(AF_INET, SOCK_DGRAM, 0);

		memset(&PeerAddr, 0, sizeof(PeerAddr));
		PeerAddr.sin_family = AF_INET;
		PeerAddr.sin_addr.s_addr = inet_addr(IpAddress.c_str());
		PeerAddr.sin_port = htons(Port);
	}*/

	RecvThread = new std::thread(_cliRecvThreadFunc, this);

	std::thread keepAliveThread = std::thread(_keepAliveThreadFunc, this);
	keepAliveThread.detach();
	
	//RecvThread = PTHREAD_CREATE(_cliRecvThreadFunc, this);

	if (ChannelID > 0)
	{

	}
	else
	{
		ChannelID = chid;
		SendHandShake();
	}
}

void PPClient::Stop()
{
	if (IsStop) return;

	LogPrint(LOG_LEVEL_SILENT, "Client Stop! ChannelID : %d, %s:%d\n", ChannelID, IpAddress.c_str(), Port);
		
	IsStop = 1;
	MyInfo = 0;
	//shutdown(PPSocket, 2);
	//close(PPSocket);

	StopCallback(this);
}

void PPClient::SetMessage(int len, char* msg)
{
	if (len <= 0 || msg == 0 || IsStop)
		return;

	if (msg[4] != PP_MESSAGETYPE_HANDSHAKE && ChannelID == 0)
		return;

	MessageSt *msgst = new MessageSt;
	msgst->Buffer = new char[len];
	msgst->Length = len;
	memcpy(msgst->Buffer, msg, len);

	MessageMutex.lock();
	if (!IsStop) MessageQueue.push(msgst);
	MessageMutex.unlock();

	if (RecvThread == 0 && ChannelID > 0)
	{
		Start(ChannelID);
	}

	SilentSec = 0;
}

void PPClient::HandleMessage()
{
// 	char buf[MAX_RECV_DGRAM_SIZE];
// 	int ret = 0;
	
	while (!IsStop)
	{
		MessageMutex.lock();
		if (MessageQueue.size() <= 0)
		{
			MessageMutex.unlock();
			//SLEEP(10);
			continue;
		}

		if (IsStop)
		{
			MessageMutex.unlock();
			break;
		}

		MessageSt *msg = MessageQueue.front();
		MessageQueue.pop();
		MessageMutex.unlock();

		if (IsStop)
		{
			if (msg != 0)
			{
				delete msg;
			}

			break;
		}

		char msgtype = msg->Buffer[4];

		switch (msgtype)
		{
		case PP_MESSAGETYPE_HANDSHAKE:
			ReceiveHandShake(msg->Buffer);
			break;

		case PP_MESSAGETYPE_HAVE:
			ReceiveHave(msg->Buffer);
			break;

		case PP_MESSAGETYPE_REQUEST:
			ReceiveRequest(msg->Buffer);
			break;

		case PP_MESSAGETYPE_DATA:
			ReceiveData(msg->Buffer, msg->Length);
			break;

		case PP_MESSAGETYPE_ACK:
			ReceiveAck(msg->Buffer);
			break;

		case PP_MESSAGETYPE_INTEGRITY:
			ReceiveIntegrity(msg->Buffer);
			break;

		case PP_MESSAGETYPE_PEXREQ:
			ReceivePEXReq(msg->Buffer);
			break;

		default:

			break;
		}

		delete msg;
	}
}

int PPClient::CheckHandshaked()
{
	if (!IsHandshaked)
	{
		LogPrint(LOG_LEVEL_DEBUG, "Not handshaked. Ignore this peer.");
		Stop();
		return 0;
	}

	return 1;
}

int PPClient::SendToPeer(int len, char* msg)
{
	if (PPSocket == 0)
	{
		PPSocket = socket(AF_INET, SOCK_DGRAM, 0);

		memset(&PeerAddr, 0, sizeof(PeerAddr));
		PeerAddr.sin_family = AF_INET;
		PeerAddr.sin_addr.s_addr = inet_addr(IpAddress.c_str());
		PeerAddr.sin_port = htons(Port);
	}
	
	int ret = 0;
	ret = sendto(PPSocket, msg, len, 0, (struct sockaddr *)&PeerAddr, sizeof(PeerAddr));
	
	return ret;
}

void PPClient::ReceiveHandShake(char* msg)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recv Handshake.\n");

	HandShake handshake;

	int idx = handshake.Create(msg);

	if ((MyInfo->IsSeeder && SwarmID != handshake.SwarmIdentifierString) || (handshake.HaveSwarmIdentifier && SwarmID != handshake.SwarmIdentifierString))
	{
		LogPrint(LOG_LEVEL_SILENT, "SwarmIdentifier is different!\n");

		Stop();

		return;
	}

	DestChannelID = handshake.SourceChannelID;

	PeerInfo.SetInfo(&handshake);

	if (handshake.HaveChunkAddressingMethod && MyInfo->ChunkAddressingMethod != PeerInfo.ChunkAddressingMethod)
	{
		LogPrint(LOG_LEVEL_SILENT, "ChunkAddressingMethod is different!\n");

		Stop();

		return;
	}

	if (handshake.HaveContentIntegrityProtectionMethod && MyInfo->ContentIntegrityProtectionMethod != PeerInfo.ContentIntegrityProtectionMethod)
	{
		LogPrint(LOG_LEVEL_SILENT, "ContentIntegrityProtectionMethod is different!\n");

		Stop();

		return;
	}

	if (handshake.HaveChunkSize && MyInfo->ChunkSize != PeerInfo.ChunkSize)
	{
		LogPrint(LOG_LEVEL_SILENT, "ChunkSize is different!\n");

		Stop();

		return;
	}

	if (handshake.HaveMerkleHashTreeFunction && MyInfo->MerkleHashTreeFunction != PeerInfo.MerkleHashTreeFunction)
	{
		LogPrint(LOG_LEVEL_SILENT, "MerkleHashTreeFunction is different!\n");

		Stop();

		return;
	}

	if (handshake.HaveMerkleHashTreeFunction && PeerInfo.MerkleHashTreeFunction != PP_HANDSHAKE_VALUE_MHF_SHA1 && PeerInfo.MerkleHashTreeFunction != PP_HANDSHAKE_VALUE_MHF_SHA256)
	{
		LogPrint(LOG_LEVEL_SILENT, "Unsupported MerkleHashTreeFunction!\n");

		Stop();

		return;
	}

	if (handshake.HaveContentIntegrityProtectionMethod && PeerInfo.ContentIntegrityProtectionMethod != PP_HANDSHAKE_VALUE_CIPM_MERKLEHASHTREE)
	{
		LogPrint(LOG_LEVEL_SILENT, "Unsupported ContentIntegrityProtectionMethod!\n");

		Stop();

		return;
	}

	if (handshake.HaveChunkAddressingMethod && PeerInfo.ChunkAddressingMethod != PP_HANDSHAKE_VALUE_CAM_32BITCHUNKRANGES)
	{
		LogPrint(LOG_LEVEL_SILENT, "Unsupported ChunkAddressingMethod!\n");

		Stop();

		return;
	}
	
	if (handshake.PeerListeningPort > 0)
	{
		Port = handshake.PeerListeningPort;
	}

	IsHandshaked = 1;

	if (msg[idx] == PP_MESSAGETYPE_HAVE)
	{
		ReceiveHave(msg + idx, 1);
	}
	else if (!SendHandshake)
	{
		SendHandShakeHave();
	}
}

void PPClient::SendHandShake()
{
	HandShake handshake;
	handshake.SourceChannelID = ChannelID;
	handshake.DestinationChannelID = DestChannelID;
	handshake.SwarmIdentifierString = SwarmID;
	handshake.Version = MyInfo->Version;
	handshake.MinVersion = MyInfo->MinVersion;
	handshake.ContentIntegrityProtectionMethod = MyInfo->ContentIntegrityProtectionMethod;
	handshake.MerkleHashTreeFunction = MyInfo->MerkleHashTreeFunction;
	//handshake.LiveSignatureAlgorithm = ServerInfo->LiveSignatureAlgorithm; unsupported
	handshake.ChunkAddressingMethod = MyInfo->ChunkAddressingMethod;
	//handshake.LiveDiscardWindow = ServerInfo->LiveDiscardWindow; unsupported
	handshake.SupportedMessagesBitmap = new unsigned char[MyInfo->SupportedMessagesBitmapLength];
	memcpy(handshake.SupportedMessagesBitmap, MyInfo->SupportedMessagesBitmap, MyInfo->SupportedMessagesBitmapLength);
	handshake.SupportedMessagesBitmapLength = MyInfo->SupportedMessagesBitmapLength;
	handshake.ChunkSize = MyInfo->ChunkSize;

	if (MyInfo->ListenPortOption > 0)
	{
		handshake.PeerListeningPort = MyInfo->Port;
	}

	int len = 0;

	char *buf = handshake.GetBuffer(&len);

	if (buf == 0 || len <= 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "Failed to create Handshake buffer.\n");
		return;
	}

	SendHandshake = 1;
	int ret = SendToPeer(len, buf);

	LogPrint(LOG_LEVEL_DEBUG, "Send Handshake, len(%d)\n", ret);
}

void PPClient::SendHandShakeHave()
{
	HandShake handshake;
	handshake.SourceChannelID = ChannelID;
	handshake.DestinationChannelID = DestChannelID;
	handshake.SwarmIdentifierString = SwarmID;
	handshake.Version = MyInfo->Version;
	handshake.MinVersion = MyInfo->MinVersion;
	handshake.ContentIntegrityProtectionMethod = MyInfo->ContentIntegrityProtectionMethod;
	handshake.MerkleHashTreeFunction = MyInfo->MerkleHashTreeFunction;
	//handshake.LiveSignatureAlgorithm = ServerInfo->LiveSignatureAlgorithm; unsupported
	handshake.ChunkAddressingMethod = MyInfo->ChunkAddressingMethod;
	//handshake.LiveDiscardWindow = ServerInfo->LiveDiscardWindow; unsupported
	handshake.SupportedMessagesBitmap = new unsigned char[MyInfo->SupportedMessagesBitmapLength];
	memcpy(handshake.SupportedMessagesBitmap, MyInfo->SupportedMessagesBitmap, MyInfo->SupportedMessagesBitmapLength);
	handshake.SupportedMessagesBitmapLength = MyInfo->SupportedMessagesBitmapLength;
	handshake.ChunkSize = MyInfo->ChunkSize;

	if (MyInfo->ListenPortOption > 0)
	{
		handshake.PeerListeningPort = MyInfo->Port;
	}

	Have have;
	have.DestinationChannelID = DestChannelID;

	if (MyInfo->IsSeeder)
	{
		have.StartChunk = 0;
		have.EndChunk = MyInfo->FileHashTree.ChunkRange;
	}
	else
	{
		MyInfo->GetBiggestHavedChunkRange(&have.StartChunk, &have.EndChunk);
	}

	int handshakelen = 0;
	char *handshakebuf = handshake.GetBuffer(&handshakelen);

	if (handshakebuf == 0 || handshakelen <= 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "Failed to create Handshake buffer.\n");
		return;
	}

	if (have.StartChunk >= 0 && have.EndChunk >= 0)
	{
		int havelen = 0;
		char *havebuf = have.GetBuffer(&havelen, 1);

		if (havebuf == 0 || havelen <= 0)
		{
			LogPrint(LOG_LEVEL_ERROR, "Failed to create Have buffer.\n");
			return;
		}

		int bothlen = handshakelen + havelen;

		char *bothbuf = new char[bothlen];

		memcpy(bothbuf, handshakebuf, handshakelen);
		memcpy(bothbuf + handshakelen, havebuf, havelen);

		int ret = SendToPeer(bothlen, bothbuf);

		delete[] bothbuf;

		LogPrint(LOG_LEVEL_DEBUG, "Send Handshake+Have, len(%d)\n", ret);
	}
	else
	{
		int ret = SendToPeer(handshakelen, handshakebuf);

		LogPrint(LOG_LEVEL_DEBUG, "Send Handshake, len(%d)\n", ret);
	}
}

void PPClient::ReceiveHave(char* msg, int isHandshakeHave)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recv Have.\n");

	if (!CheckHandshaked()) return;
	
	Have have;

	int idx = have.Create(msg, isHandshakeHave);

	//TODO Choke ±¸Çö

	PeerInfo.FileBinmap.SetHave(&have);
	//MyInfo->FileBinmap.SetLastChunk(have.EndChunk, 677);
	
	if (isHandshakeHave)
	{
		if (PeerInfo.IsSupported(PP_MESSAGETYPE_PEXREQ))
		{
			if (PeerInfo.IsSupported(PP_MESSAGETYPE_PEXRESV4))
			{
				std::thread pexreqThread = std::thread(_pexreqThreadThreadFunc, this);
				pexreqThread.detach();
			}
		}

		if (msg[idx] == PP_MESSAGETYPE_CHOKE)
		{
			ReceiveChoke(msg + idx);
		}
		else
		{
			SendNextRequest();
		}
	}
}

void PPClient::SendNextRequest()
{
	int reqidx = MyInfo->FileBinmap.GetRequestData(&PeerInfo.FileBinmap);

	if (reqidx >= 0)
	{
		SendRequest(reqidx);
	}
	else
	{
		if (!MyInfo->CheckFinish())
		{
			Stop();
			//SLEEP(3000);
			//SendHandShake();
		}
	}
}

void PPClient::SendRequest(int index)
{
	//int tmpidx = index;
	
// 	while (!MyInfo->FileBinmap.SetRequested(tmpidx))
// 	{
// 		tmpidx = MyInfo->FileBinmap.GetRequestData(&PeerInfo.FileBinmap);
// 	}

	LastRequestedIndex = index;

	Request req;
	req.DestinationChannelID = DestChannelID;
	req.StartChunk = index;
	req.EndChunk = index;

	int len = 0;
	char* buf = req.GetBuffer(&len);	

	LogPrint(LOG_LEVEL_DEBUG, "Send Request : %d\n", index);
	SendToPeer(len, buf);
}

void PPClient::SendPeakHash()
{
	MerkleHashmap *map = MyInfo->GetPeakHashes();

	if (map == 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "Failed to make peak hashes.\n");
		return;
	}

	for (MerkleHashmapIter it = map->begin(); it != map->end(); ++it)
	{
		SendIntegrity((*it).second->StartChunk, (*it).second->EndChunk, (*it).second->Hash);
	}

	delete map;
}

void PPClient::ReceiveRequest(char* msg)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recevice Request\n");

	if (!CheckHandshaked()) return;

	Request req;

	req.Create(msg);

	if (IsFirstRequest)
	{
		IsFirstRequest = 0;

		SendPeakHash();
	}

	for (int i = req.StartChunk; i <= req.EndChunk; i++)
	{
		CancelMutex.lock();
		if (CancelChunkList.size() > 0)
		{
			if(CancelChunkList.find(i) != CancelChunkList.end())
				continue;
		}
		CancelMutex.unlock();
		
		SendData(i);
	}

	CancelChunkList.clear();
}

void PPClient::SendIntegrity(int start, int end, unsigned char* hash)
{
	Integrity inte(MyInfo->MerkleHashTreeFunction);
	inte.DestinationChannelID = DestChannelID;
	inte.StartChunk = start;
	inte.EndChunk = end;
	inte.SetHash(hash);

	int len = 0;
	char *buf = inte.GetBuffer(&len);

	LogPrint(LOG_LEVEL_DEBUG, "Send Integrity %d-%d, len(%d)\n", start, end, len);
	SendToPeer(len, buf);
}

void PPClient::ReceiveIntegrity(char* msg)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recevice Integrity\n");

	if (!CheckHandshaked()) return;

	Integrity inte(MyInfo->MerkleHashTreeFunction);
	inte.Create(msg);

	MyInfo->AddIntegrity(ChannelID, &inte);
}

void PPClient::RetryRequest()
{
	if (RetryCount > MyInfo->RetryCount)
	{
		Stop();
	}
	else
	{
		SLEEP(1000);
		SendNextRequest();
		RetryCount++;
	}
}

void PPClient::SendData(int index)
{
	MerkleHashmap *map = MyInfo->GetDataIntegrity(ChannelID, index);

	if (map == 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "Failed to make data integrities.\n");
		return;
	}

	for (MerkleHashmapIter it = map->begin(); it != map->end(); ++it)
	{
		SendIntegrity((*it).second->StartChunk, (*it).second->EndChunk, (*it).second->Hash);
	}

	delete map;

	Data data;
	data.DestinationChannelID = DestChannelID;
	data.StartChunk = index;
	data.EndChunk = index;
	data.ChunkSize = MyInfo->ChunkSize;
	data.DataBuffer = MyInfo->GetDataFromFile(index, &data.BufferSize);

	if (data.DataBuffer == 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "File read error! \n");
		return;
	}

	int len = 0;
	char* buf = data.GetBuffer(&len);

	LogPrint(LOG_LEVEL_DEBUG, "Send Data : %d, Timestamp: %lld\n", len, data.Timestamp);
	SendToPeer(len, buf);
}

void PPClient::ReceiveData(char* msg, int len)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recevice Data\n");

	if (!CheckHandshaked()) return;

	Data data;
	data.ChunkSize = MyInfo->ChunkSize;
	data.Create(msg, len);

	if (!MyInfo->CheckDataIntegrity(ChannelID, &data))
	{
		LogPrint(LOG_LEVEL_ERROR, "Failed to check data integrity.\n");
		MyInfo->FileBinmap.ReleaseRequested(LastRequestedIndex);
		
		RetryRequest();
		
		return;
	}

	
	if (MyInfo->IsFirstData)
	{
		if (!MyInfo->FileHashTree.CheckPeakHash(ChannelID))
		{
			LogPrint(LOG_LEVEL_ERROR, "failed to check peak hashes.\n");
			MyInfo->FileBinmap.ReleaseRequested(LastRequestedIndex);

			RetryRequest();

			return;
		}

		MyInfo->IsFirstData = 0;
	}

	RetryCount = 0;
	
	SendAck(data.StartChunk, data.Timestamp);

	if (MyInfo->FileHashTree.ContentSize > 0)
	{
		if (MyInfo->FileBinmap.LastChunkSize <= 0)
		{
			MyInfo->FileBinmap.SetLastChunk(MyInfo->FileHashTree.ChunkRange, MyInfo->FileHashTree.LastChunkSize);
		}
	}

	int ret = MyInfo->SetData(&data);

	LogPrint(LOG_LEVEL_DEBUG, "Receive Chunk %d from Channel %d - file write size : %d\n", data.StartChunk, ChannelID, ret);

	if (MyInfo->FileHashTree.ContentSize <= 0)
	{
		SendRequest(MyInfo->FileHashTree.ChunkRange);
	}
	else
	{
// 		if (MyInfo->FileBinmap.LastChunkSize <= 0)
// 		{
// 			MyInfo->FileBinmap.SetLastChunk(MyInfo->FileHashTree.LastChunkIndex, MyInfo->FileHashTree.LastChunkSize);
// 		}
		SendNextRequest();
	}
}

void PPClient::SendAck(int index, unsigned long long timestamp)
{
	Ack ack;
	ack.DestinationChannelID = DestChannelID;
	ack.StartChunk = index;
	ack.EndChunk = index;

	ack.OnewayDelaySample = GetCurrentTimeMicrosecond() - timestamp;

	int len = 0;
	char* buf = ack.GetBuffer(&len);

	if (len <= 0 || buf == 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "Failed to create Ack buffer.\n");
		return;
	}

	LogPrint(LOG_LEVEL_DEBUG, "Send Ack : %d, delay : %lld\n", len, ack.OnewayDelaySample);
	SendToPeer(len, buf);
}

void PPClient::ReceiveAck(char* msg)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recevice Ack\n");

	if (!CheckHandshaked()) return;

	Ack ack;
	ack.Create(msg);
}

void PPClient::SendKeepAlive()
{
	char buf[4] = { 0, };
	int tmpl = CalcEndianH2N(DestChannelID);
	memcpy(buf, &tmpl, 4);

	SendToPeer(4, buf);
}

void PPClient::ReceiveKeepAlive()
{
	LogPrint(LOG_LEVEL_DEBUG, "Recevice KeepAlive\n");

	SilentSec = 0;
}

void PPClient::SendPEXReq()
{
	PEXReq pex;
	pex.DestinationChannelID = DestChannelID;

	int len = 0;
	char* buf = pex.GetBuffer(&len);

	if (len <= 0 || buf == 0)
	{
		LogPrint(LOG_LEVEL_ERROR, "Failed to create PEXReq buffer.\n");
		return;
	}

	LogPrint(LOG_LEVEL_DEBUG, "Send PEXReq\n");
	SendToPeer(len, buf);
}

void PPClient::ReceivePEXReq(char* msg)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recevice PEXReq\n");

	if (!CheckHandshaked()) return;

	SendPEXRes();
}

void PPClient::SendPEXRes()
{
	std::list<PPClient*>* list = PEXCallback(this);

	if (list == 0) return;

	for (auto it = list->begin(); it != list->end(); ++it)
	{
		PEXRes pex;
		pex.DestinationChannelID = DestChannelID;
		pex.IPv4AddressString = (*it)->IpAddress;
		pex.Port = (*it)->Port;

		int len = 0;
		char* buf = pex.GetBuffer(&len);

		if (len <= 0 || buf == 0)
		{
			LogPrint(LOG_LEVEL_ERROR, "Failed to create PEXRes buffer.\n");
			delete list;
			return;
		}

		LogPrint(LOG_LEVEL_DEBUG, "Send PEXRes\n");
		SendToPeer(len, buf);
	}
}

void PPClient::ReceiveCancel(char* msg)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recevice Cancel\n");

	if (!CheckHandshaked()) return;

	Cancel cancel;

	cancel.Create(msg);

	CancelMutex.lock();
	for (int i = cancel.StartChunk; i <= cancel.EndChunk; i++)
	{
		CancelChunkList[i] = 0;
	}

	CancelMutex.unlock();
}

void PPClient::SendCancel(int start, int end)
{
	Cancel cancel;
	cancel.DestinationChannelID = DestChannelID;
	cancel.StartChunk = start;
	cancel.EndChunk = end;

	int len = 0;
	char* buf = cancel.GetBuffer(&len);

	LogPrint(LOG_LEVEL_DEBUG, "Send Cancel : %d~%d\n", start, end);
	SendToPeer(len, buf);
}

void PPClient::ReceiveChoke(char* msg)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recevice Choke\n");

	if (!CheckHandshaked()) return;

	Choke choke;

	choke.Create(msg);

	IsChoke = 1;
}

void PPClient::SendChoke()
{
	Choke choke;
	choke.DestinationChannelID = DestChannelID;
	
	int len = 0;
	char* buf = choke.GetBuffer(&len);

	LogPrint(LOG_LEVEL_DEBUG, "Send Choke\n");
	SendToPeer(len, buf);
}

void PPClient::ReceiveUnchoke(char* msg)
{
	LogPrint(LOG_LEVEL_DEBUG, "Recevice Unchoke\n");

	if (!CheckHandshaked()) return;

	Unchoke unchoke;

	unchoke.Create(msg);

	if (IsChoke)
	{
		SendNextRequest();
	}

	IsChoke = 0;
}

void PPClient::SendUnchoke()
{
	Unchoke unchoke;
	unchoke.DestinationChannelID = DestChannelID;

	int len = 0;
	char* buf = unchoke.GetBuffer(&len);

	LogPrint(LOG_LEVEL_DEBUG, "Send Unchoke\n");
	SendToPeer(len, buf);
}