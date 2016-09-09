#pragma once
#include <string>
#include "PPMessage.h"

#define PP_MESSAGETYPE_HANDSHAKE 0x00

#define PP_HANDSHAKE_CODE_VERSION 0x00
#define PP_HANDSHAKE_CODE_MINIMUMVERSION 0x01
#define PP_HANDSHAKE_CODE_SWARMIDENTIFIER 0x02
#define PP_HANDSHAKE_CODE_CONTENTINTEGRITYPROTECTIONMETHOD 0x03
#define PP_HANDSHAKE_CODE_MERKLEHASHTREEFUNCTION 0x04
#define PP_HANDSHAKE_CODE_LIVESIGNATUREALGORITHM 0x05
#define PP_HANDSHAKE_CODE_CHUNKADDRESSINGMETHOD 0x06
#define PP_HANDSHAKE_CODE_LIVEDISCARDWINDOW 0x07
#define PP_HANDSHAKE_CODE_SUPPORTEDMESSAGES 0x08
#define PP_HANDSHAKE_CODE_CHUNKSIZE 0x09
#define PP_HANDSHAKE_CODE_PEER_LISTENING_PORT 0x0A
#define PP_HANDSHAKE_CODE_ENDOPTION 0xFF

#define PP_HANDSHAKE_VALUE_CIPM_NOINTEGRITYPROTECTION 0
#define PP_HANDSHAKE_VALUE_CIPM_MERKLEHASHTREE 1
#define PP_HANDSHAKE_VALUE_CIPM_SIGNALL 2
#define PP_HANDSHAKE_VALUE_CIPM_UNIFIEDMERKLETREE 3

#define PP_HANDSHAKE_VALUE_MHF_SHA1 0
#define PP_HANDSHAKE_VALUE_MHF_SHA224 1
#define PP_HANDSHAKE_VALUE_MHF_SHA256 2 // default
#define PP_HANDSHAKE_VALUE_MHF_SHA384 3
#define PP_HANDSHAKE_VALUE_MHF_SHA512 4

#define PP_HANDSHAKE_VALUE_LSA_RSASHA1 5
#define PP_HANDSHAKE_VALUE_LSA_RSASHA256 8
#define PP_HANDSHAKE_VALUE_LSA_ECDSAP256SHA256 13 // default
#define PP_HANDSHAKE_VALUE_LSA_ECDSAP384SHA384 14

#define PP_HANDSHAKE_VALUE_CAM_32BITBINS 0
#define PP_HANDSHAKE_VALUE_CAM_64BITBYTERANGES 1
#define PP_HANDSHAKE_VALUE_CAM_32BITCHUNKRANGES 2 // default 
#define PP_HANDSHAKE_VALUE_CAM_64BITBINS 3
#define PP_HANDSHAKE_VALUE_CAM_64BITCHUNKRANGES 4

class HandShake : public PPMessage
{
public:
	HandShake();
	HandShake(char* data);
	~HandShake();

	int SourceChannelID;

	/*Version = Code (0x00)  [ 8 ] / Version [ 8 ]
	Minimum Version =  Code (0x01)  [ 8 ] / Min Version [ 8 ]
	Swarm Identifier =  Code (0x02)  [ 8 ] / Swarm ID Length [ 16 ] / Swarm Identifier [ variable ]
	Content Integrity Protection Method =  Code (0x03)  [ 8 ] / C.I.P.M [ 8 ]
	Merkle Hash Tree Function =  Code (0x04)  [ 8 ] / M.H.F [ 8 ]
	Live Signature Algorithm =  Code (0x05)  [ 8 ] / L.S.A [ 8 ]
	Chunk Addressing Method = Code (0x06)  [ 8 ] / C.A.M [ 8 ]
	Live Discard Window = Code (0x07)  [ 8 ] / Live Discard Window [ 32 or 64 ]  C.A.M값에 따라
	Supported Messages = Code (0x08)  [ 8 ] / S.M Length [ 8 ] / Supported Messages Bitmap [ variable, max 256 ]
	Chunk Size = Code (0x09)  [ 8 ] / Chunk Size [ 32 ]
	: big-endian format , variable chunk sizes 0xFFFFFFFF
	End Option = Code (0xFF)  [ 8 ]*/

	char Version;
	char MinVersion;
	unsigned char *SwarmIdentifier;
	std::string SwarmIdentifierString;
	char ContentIntegrityProtectionMethod;
	char MerkleHashTreeFunction;
	char LiveSignatureAlgorithm;
	char ChunkAddressingMethod;
	char* LiveDiscardWindow;
	unsigned char* SupportedMessagesBitmap; //[variable, max 256]
	unsigned char SupportedMessagesBitmapLength;
	int ChunkSize; // big-endian format , variable chunk sizes 0xFFFFFFFF
	short PeerListeningPort;

	int HaveVersion;
	int HaveMinVersion;
	int HaveSwarmIdentifier;
	int HaveContentIntegrityProtectionMethod;
	int HaveMerkleHashTreeFunction;
	int HaveLiveSignatureAlgorithm;
	int HaveChunkAddressingMethod;
	int HaveLiveDiscardWindow;
	int HaveSupportedMessages;
	int HaveChunkSize;

	int Create(char* data);
	char* GetBuffer(int *len);
};