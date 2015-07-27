#ifndef WORLDSERVER_H
#define WORLDSERVER_H

#include "servertalk.h"
#include "ServerPacket.h"
#include "linked_list.h"
#include "timer.h"
#include "queue.h"

#include "eq_packet_structs.h"
#include "Mutex.h"
#include "mob.h"

#include "config.h"

#ifdef WIN32
	void WorldServerLoop(void *tmp);
#else
	void *WorldServerLoop(void *tmp);
#endif

#define WSCS_Construction	0
#define WSCS_Ready			1
#define WSCS_Connecting		2
#define WSCS_Authenticating	3
#define WSCS_Connected		100
#define WSCS_Disconnecting	200

class WorldServer {
public:
	WorldServer();
    ~WorldServer();

	bool Init();
	void Process();
	bool ReceiveData();
	bool SendPacket(ServerPacket* pack);
	bool SendChannelMessage(Client* from, char* to, int8 chan_num, int32 guilddbid, int8 language, char* message, ...);
	bool SendEmoteMessage(char* to, int32 to_guilddbid, int32 type, char* message, ...);
	void SetZone(char* zone);
	void SetConnectInfo();

	bool RezzPlayer(APPLAYER* rpack, char* ownerName, int32 rezzexp, int16 opcode, int32 dbid);

	int32	GetIP()		{ return ip; }
	int16	GetPort()	{ return port; }
	int8	GetState();
	void	SetState(int8 in_state);
	bool	Connected()	{ return (GetState() == WSCS_Connected); }

	bool	SendPacketQueue(bool block = false);
	bool	Connect(bool FromInit = false);
	void	Disconnect();

	Timer*	ReconnectTimer;
private:
	void SendKeepAlive();
	int32 ip;
	int16 port;
#ifdef WIN32
	SOCKET send_socket;
#else
	int send_socket;
#endif
	int8 connection_state;
	uchar*	recvbuf;
	int32	recvbuf_size;
	int32	recvbuf_used;

	MyQueue<ServerPacket>	ServerRecvQueue;
	MyQueue<SPackSendQueue>	ServerSendQueue;
	SPackSendQueue*			ServerSendQueuePop(bool block = false);
	ServerPacket*			ServerRecvQueuePop(bool block = false);

	Timer*	timeout_timer;

	Mutex	MQueueLock;
	Mutex	MStateLock;
};
#endif

