#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Common/Util.h"
#include "PPServer.h"

typedef struct _MetaDataSt
{
	char version;
	char mkinVersion;
	char contentIntegrityProtectionMethod;
	char merkleHashTreeFunction;
	char liveSignatureAlgorithm;
	char chunkAddressingMethod;
	int chunkSize;
	int retryCount;
	int pexReqInterval;
	int keepAliveInterval;

	_MetaDataSt()
	{
		version = 1;
		mkinVersion = 1;
		contentIntegrityProtectionMethod = PP_HANDSHAKE_VALUE_CIPM_MERKLEHASHTREE;
		merkleHashTreeFunction = PP_HANDSHAKE_VALUE_MHF_SHA1;
		liveSignatureAlgorithm = PP_HANDSHAKE_VALUE_LSA_ECDSAP256SHA256;
		chunkAddressingMethod = PP_HANDSHAKE_VALUE_CAM_32BITCHUNKRANGES;
		chunkSize = DEFAULT_CHUNK_SIZE;
		retryCount = 3;
		pexReqInterval = 5;
		keepAliveInterval = 10;
	}
}MetaDataSt;

void _usage(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  -h, --hash\t\tswarm ID of the content (root hash or public key)\n");
	fprintf(stderr, "  -f, --file\t\tname of file to use (root hash by default)\n");
	fprintf(stderr, "  -l, --listen\t\t[ip:]port to listen to. MUST set for IPv4\n");
	fprintf(stderr, "  -t, --tracker\t\tip:port of the tracker\n");
	fprintf(stderr, "  -d, --debug\t\tsave debug logs.\n");
	fprintf(stderr, "  -o, --listening port option on\t\tinclude listening port in HandShake.\n");

	fprintf(stderr, "\nex)1.1. Start seeder with file. (Port only)\n");
	fprintf(stderr, "\tEtriPPSP.exe -f xxx.mp4 -l 20000\n");
	fprintf(stderr, "\nex)1.2. Start seeder with file. (Port only) + debug logs\n");
	fprintf(stderr, "\tEtriPPSP.exe -f xxx.mp4 -l 20000 -d\n");
	fprintf(stderr, "\nex)1.3. Start seeder with file. (IP & Port)\n");
	fprintf(stderr, "\tEtriPPSP.exe -f xxx.mp4 -l 192.168.0.2:20000\n");
	fprintf(stderr, "\nex)1.4. Start seeder with file. (IP & Port) + listening port option\n");
	fprintf(stderr, "\tEtriPPSP.exe -f xxx.mp4 -l 192.168.0.2:20000 -o\n");
	fprintf(stderr, "\nex)2.1. Start peer with tracker(Listen port).\n");
	fprintf(stderr, "\tEtriPPSP.exe -t 192.168.0.2:20000 -h e13dd7bd5c7d4f60f7c598da27cd669af6576680e13dd7bd5c7d4f60f7c598da -l 20001 -f xxx.mp4\n");
	fprintf(stderr, "\nex)2.2. Start peer with tracker(Listen port). + debug logs\n");
	fprintf(stderr, "\tEtriPPSP.exe -t 192.168.0.2:20000 -h e13dd7bd5c7d4f60f7c598da27cd669af6576680e13dd7bd5c7d4f60f7c598da -l 20001 -f xxx.mp4 -d\n");
	fprintf(stderr, "\nex)2.3. Start peer with tracker(Listen IP & port).\n");
	fprintf(stderr, "\tEtriPPSP.exe -t 192.168.0.2:20000 -h e13dd7bd5c7d4f60f7c598da27cd669af6576680e13dd7bd5c7d4f60f7c598da -l 192.168.0.3:20001 -f xxx.mp4\n");
}

void _readConfigFile(MetaDataSt& meta)
{
	FILE *fp;
	char buf[80];

	fp = fopen("ppsp.conf", "r");
	if (fp != NULL)
	{
		while (!feof(fp))
		{
			if (fgets(buf, 80, fp) == NULL)
				break;

			if (strlen(buf) <= 3)
				continue;

			if (buf[0] == '#')
				continue;

			if (buf[0] == '\n')
				continue;

			if (!strcmp(buf, CONF_KEY_VERSION))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.version = atoi(buf);
				}
			}
			else if (!strcmp(buf, CONF_KEY_MINVERSION))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.mkinVersion = atoi(buf);
				}
			}
			else if (!strcmp(buf, CONF_KEY_CONTENT_INTEGRITY_PROTECTION_METHOD))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.contentIntegrityProtectionMethod = atoi(buf);
				}
			}
			else if (!strcmp(buf, CONF_KEY_MERKLE_HASH_TREE_FUNCTION))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.merkleHashTreeFunction = atoi(buf);
				}
			}
			else if (!strcmp(buf, CONF_KEY_LIVE_SIGNATURE_ALGORITHM))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.liveSignatureAlgorithm = atoi(buf);
				}
			}
			else if (!strcmp(buf, CONF_KEY_CHUNK_ADDRESSING_METHOD))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.chunkAddressingMethod = atoi(buf);
				}
			}
			else if (!strcmp(buf, CONF_KEY_CHUNK_SIZE))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.chunkSize = atoi(buf);
				}
			}
			else if (!strcmp(buf, CONF_KEY_RETRY_COUNT))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.retryCount = atoi(buf);
				}
			}
			else if (!strcmp(buf, CONF_KEY_KEEP_ALIVE_INTERVAL))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.keepAliveInterval = atoi(buf);
				}
			}
			else if (!strcmp(buf, CONF_KEY_PEX_REQ_INTERVAL))
			{
				if (fgets(buf, 80, fp) == NULL)
					break;
				else
				{
					meta.pexReqInterval = atoi(buf);
				}
			}
		}

		fclose(fp);
	}
}

int main(int argc, char* argv[])
{
	int idx = 1;
	std::string filename = "";
	std::string ipaddress = "";
	int port = 0;

	std::string trackerip = "";
	int trackerport = 0;

	std::string hash = "";

	int listenOpt = 0;

	if (argc < 2)
	{
		_usage();
		exit(0);
	}

	while (argc > idx)
	{

		char* arg = argv[idx];

		if (strlen(arg) == 2 && arg[0] == '-')
		{
			idx++;

			switch (arg[1])
			{
			case 'h':
				printf("h : %s\n", argv[idx]);
				hash = argv[idx];

				break;

			case 't':
			{
				printf("t : %s\n", argv[idx]);
				char* semi = strchr(argv[idx], ':');
				if (semi)
				{
					*semi = 0;
					trackerip = argv[idx];
					trackerport = atoi(semi + 1);
				}
				else
				{
					port = atoi(argv[idx]);
				}

				break;
			}

			case 'f':
				printf("f : %s\n", argv[idx]);
				filename = argv[idx];
				break;

			case 'l':
			{
				printf("l : %s\n", argv[idx]);
				char* semi = strchr(argv[idx], ':');
				if (semi)
				{
					*semi = 0;
					ipaddress = argv[idx];
					port = atoi(semi + 1);
				}
				else
				{
					port = atoi(argv[idx]);
				}
				break;
			}
			case 'd':
				SetLogLevel(LOG_LEVEL_DEBUG);
				idx--;
				break;
			case 'o':
				listenOpt = 1;
				idx--;
				break;
			}

			idx++;
		}		
	}

	if (trackerip.length() > 0)
	{
		if (hash.length() <= 0)
		{
			_usage();
			exit(0);
		}

		if (trackerport <= 0)
		{
			_usage();
			exit(0);
		}

		if (port <= 0)
		{
			_usage();
			exit(0);
		}
	}
	else
	{
		if (port <= 0)
		{
			_usage();
			exit(0);
		}

		if (filename.length() <= 0)
		{
			_usage();
			exit(0);
		}
	}

	MetaDataSt metaData;
	_readConfigFile(metaData);

	HTTP_Startup();
	
	PPServer ppserver;
	ppserver.SetPort(port);
	ppserver.SetIpAddress(ipaddress);
	ppserver.SetFileName(filename);
	ppserver.SetSwarmID(hash);
	ppserver.SetTrackerIpAddress(trackerip);
	ppserver.SetTrackerPort(trackerport);

	ppserver.SetVersion(metaData.version);
	ppserver.SetMinVersion(metaData.mkinVersion);
	ppserver.SetContentIntegrityProtectionMethod(metaData.contentIntegrityProtectionMethod);
	ppserver.SetMerkleHashTreeFunction(metaData.merkleHashTreeFunction);
	ppserver.SetLiveSignatureAlgorithm(metaData.liveSignatureAlgorithm);
	ppserver.SetChunkAddressingMethod(metaData.chunkAddressingMethod);
	ppserver.SetChunkSize(metaData.chunkSize);
	ppserver.SetRetryCount(metaData.retryCount);
	ppserver.SetKeepAliveInterval(metaData.keepAliveInterval);
	ppserver.SetPexReqInterval(metaData.pexReqInterval);
	ppserver.SetListenPortOption(listenOpt);
	
	if (!ppserver.Start())
	{
		exit(0);
	}
	
	ppserver.Join();

	HTTP_Cleanup();

	return 0;
}