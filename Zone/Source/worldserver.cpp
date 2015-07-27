#include <cstdarg>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include "servertalk.h"
#include "worldserver.h"
#include "eq_packet_structs.h"
#include "packet_dump.h"
#include "database.h"
#include "mob.h"
#include "zone.h"
#include "client.h"
#include "entity.h"
#include "net.h"
#include "petitions.h"
#include "EQCUtils.hpp"
#include "MessageTypes.h"
#include "ZoneGuildManager.h"

using namespace std;
using namespace EQC::Zone;

extern ZoneGuildManager zgm;
extern EntityList    entity_list;
extern Zone* zone;
extern volatile bool ZoneLoaded;
extern void CatchSignal(int);
extern WorldServer worldserver;
extern NetConnection net;
extern PetitionList petition_list;

extern volatile bool RunLoops;
bool WorldLoopRunning = false;

WorldServer::WorldServer() 
{
	this->ip = 0;
	this->port = 0;
	this->send_socket = SOCKET_ERROR;
	this->timeout_timer = new Timer(SERVER_TIMEOUT);
	this->ReconnectTimer = new Timer(INTERSERVER_TIMER);
	this->connection_state = WSCS_Construction;
	this->recvbuf = 0;
}

WorldServer::~WorldServer() 
{
	safe_delete(timeout_timer);
	safe_delete(ReconnectTimer);
	safe_delete(recvbuf);
}

bool WorldServer::Init() 
{
	if (GetState() == WSCS_Construction)
	{
		_beginthread(WorldServerLoop, 0, NULL);
		return Connect(true);
	}
	else
	{
		return Connect();
	}
}

// Tell World which Zone we are
void WorldServer::SetZone(char* zonename)
{
	ServerPacket* pack = new ServerPacket;
	pack->opcode = ServerOP_SetZone;
	pack->size = strlen(zonename) + 1;
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	memcpy(pack->pBuffer, zonename, strlen(zonename));
	SendPacket(pack);
	safe_delete(pack);//delete pack;
}

void WorldServer::SetConnectInfo()
{
	ServerPacket* pack = new ServerPacket(ServerOP_SetConnectInfo, sizeof(ServerConnectInfo));


	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);

	ServerConnectInfo* sci = (ServerConnectInfo*) pack->pBuffer;
	sci->port = net.GetZonePort();
	strcpy(sci->address, net.GetZoneAddress());

	SendPacket(pack);

	safe_delete(pack);//delete pack;
}


bool WorldServer::SendPacket(ServerPacket* pack)
{
	SPackSendQueue* spsq = (SPackSendQueue*) new uchar[sizeof(SPackSendQueue) + pack->size + 4];

	if (pack->pBuffer != 0 && pack->size != 0)
	{
		memcpy((char *) &spsq->buffer[4], (char *) pack->pBuffer, pack->size);
	}

	memcpy((char *) &spsq->buffer[0], (char *) &pack->opcode, 2);
	spsq->size = pack->size+4;
	memcpy((char *) &spsq->buffer[2], (char *) &spsq->size, 2);
	
	if (Connected())
	{
		MQueueLock.lock();
		ServerSendQueue.push(spsq);
		MQueueLock.unlock();
		return true;
	}
	else
	{
		safe_delete(spsq);//delete spsq;
		return false;
	}
}

SPackSendQueue* WorldServer::ServerSendQueuePop(bool block)
{
	SPackSendQueue* ret = 0;

	if (block)
	{
		MQueueLock.lock();
	}
	else if (!MQueueLock.trylock())
	{
		return 0;
	}
	ret = ServerSendQueue.pop();
	MQueueLock.unlock();
	return ret;
}

ServerPacket* WorldServer::ServerRecvQueuePop(bool block)
{
	ServerPacket* ret = 0;

	if (block)
	{
		MQueueLock.lock();
	}
	else if (!MQueueLock.trylock())
	{
		return 0;
	}
	ret = ServerRecvQueue.pop();
	MQueueLock.unlock();
	return ret;
}

bool WorldServer::SendChannelMessage(Client* from, char* to, int8 chan_num, int32 guilddbid, int8 language, char* message, ...)
{
	va_list argptr;
	char buffer[256];

	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);

	ServerPacket* pack = new ServerPacket;

	pack->size = sizeof(ServerChannelMessage_Struct) + strlen(buffer) + 1;
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	ServerChannelMessage_Struct* scm = (ServerChannelMessage_Struct*) pack->pBuffer;

	pack->opcode = ServerOP_ChannelMessage;

	if (from == 0)
	{
		strcpy(scm->from, "ZServer");
	}
	else
	{
		strcpy(scm->from, from->GetName());
		scm->fromadmin = from->Admin();
	}
	
	if (to == 0)
	{
		scm->to[0] = 0;
	}
	else
	{
		strcpy(scm->to, to);
		strcpy(scm->deliverto, to);
	}
	scm->chan_num = chan_num;
	scm->guilddbid = guilddbid;
	scm->language = language;
	strcpy(&scm->message[0], buffer);

	bool ret = SendPacket(pack);
	safe_delete(pack);//delete pack;
	return ret;
}

bool WorldServer::SendEmoteMessage(char* to, int32 to_guilddbid, int32 type, char* message, ...)
{
	va_list argptr;
	char buffer[256];

	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);

	ServerPacket* pack = new ServerPacket;
	pack->opcode = ServerOP_EmoteMessage;
	pack->size = sizeof(ServerEmoteMessage_Struct)+strlen(buffer)+1;
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;
	sem->type = type;

	if (to != 0)
	{
		strcpy(sem->to, to);
	}

	sem->guilddbid = to_guilddbid;
	strcpy(sem->message, buffer);

	bool ret = SendPacket(pack);
	safe_delete(pack);//delete pack;
	return ret;
}

bool WorldServer::SendPacketQueue(bool block)
{
	/************ Get first send packet on queue and send it! ************/
	SPackSendQueue* p = 0;    
	int status = 0;

	while(p = ServerSendQueuePop(block))
	{
		//		cout << "Worldserver packet sent: OPcode=" << p->opcode << " size=" << p->size << endl;

		status = send(send_socket, (const char *) p->buffer, p->size, 0);

		safe_delete(p);//delete p;
		if (status == SOCKET_ERROR)
		{
			cout << "Worldserver send(): status=" << status  << ", Errorcode: " << EQC::Common::GetLastSocketError() << endl;
			return false;
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	if (timeout_timer->Check())
	{
		// Keepalive packet doesnt actually do anything
		// just makes sure that send() still works, if not connection is dead
		// actual timeout is handeled by the TCP stack
		this->SendKeepAlive();
	}
	return true;
}

void WorldServer::SendKeepAlive()
{
	ServerPacket* pack = new ServerPacket(ServerOP_KeepAlive, 0);
	SendPacket(pack);
	safe_delete(pack);//delete pack;
}

bool WorldServer::ReceiveData()
{
	struct timeval tvtimeout = { 0, 0 };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(send_socket, &fds);
	
	if (select(0, &fds, NULL, NULL, &tvtimeout) == 0)
	{
		// no data to receive
		return true;
	}

	LockMutex lock(&MQueueLock);
	uchar* tmpbuf;

	if (recvbuf == 0)
	{
		recvbuf = new uchar[5120];
		recvbuf_size = 5120;
		recvbuf_used = 0;
	}
	else if ((recvbuf_size - recvbuf_used) < 2048)
	{
		tmpbuf = new uchar[recvbuf_size + 5120];
		memcpy(tmpbuf, recvbuf, recvbuf_used);
		recvbuf_size += 5120;
		safe_delete(recvbuf);//delete recvbuf;
		recvbuf = tmpbuf;
	}
	
	int status = recv(send_socket, (char *) &recvbuf[recvbuf_used], (recvbuf_size - recvbuf_used), 0);
	
	if (status >= 1)
	{
		timeout_timer->Start();
		recvbuf_used += status;
		int32 base = 0;
		int32 size = 4;
		uchar* buffer;
		ServerPacket* pack = 0;

		while ((recvbuf_used - base) >= size)
		{
			buffer = &recvbuf[base];
			memcpy(&size, &buffer[2], 2);
			if ((recvbuf_used - base) >= size)
			{
				// ok, we got enough data to make this packet!
				pack = new ServerPacket;
				memcpy(&pack->opcode, &buffer[0], 2);
				pack->size = size - 4;
				/*				if () { // TODO: Checksum or size check or something similar
				// Datastream corruption, get the hell outta here!
				safe_delete(pack);//delete pack;
				return false;
				}*/
				pack->pBuffer = new uchar[pack->size];
				memcpy(pack->pBuffer, &buffer[4], pack->size);
				ServerRecvQueue.push(pack);
				base += size;
				size = 4;
			}
		}
		if (base != 0)
		{
			if (base >= recvbuf_used)
			{
				safe_delete(recvbuf);
			}
			else
			{
				tmpbuf = new uchar[recvbuf_size - base];
				memcpy(tmpbuf, &recvbuf[base], recvbuf_used - base);
				safe_delete(recvbuf);//delete recvbuf;
				recvbuf = tmpbuf;
				recvbuf_used -= base;
				recvbuf_size -= base;
			}
		}
	} 
	else if (status == SOCKET_ERROR)
	{
		if (!(WSAGetLastError() == WSAEWOULDBLOCK))
		{
			struct in_addr  in;
			in.s_addr = GetIP();
			cout << "Worldserver connection lost: " << inet_ntoa(in) << ":" << GetPort() << endl;
			return false;
		}
	}
	return true;
}

int8 WorldServer::GetState()
{
	LockMutex lock(&MStateLock);
	int8 ret = connection_state;
	return ret;
}

void WorldServer::SetState(int8 in_state)
{
	LockMutex lock(&MStateLock);
	connection_state = in_state;
}

bool WorldServer::Connect(bool FromInit)
{
	MStateLock.lock();
	
	if (!FromInit && connection_state != WSCS_Ready)
	{
		cout << "Error: WorldServer::Connect() while not ready" << endl;
		MStateLock.unlock();
		return false;
	}
	
	connection_state = WSCS_Connecting;
	MStateLock.unlock();
	//	unsigned long nonblocking = 1;
	struct sockaddr_in	server_sin;
	struct in_addr	in;
	WORD version = MAKEWORD (1,1);
	WSADATA wsadata;
	PHOSTENT phostent = NULL;

	WSAStartup (version, &wsadata);

	if ((send_socket = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)	
	{
		cout << "WorldServer connect: Allocating socket failed. Error: " << EQC::Common::GetLastSocketError();
		SetState(WSCS_Ready);
		return false;
	}


	server_sin.sin_family = AF_INET;

	if ((phostent = gethostbyname(net.GetWorldAddress())) == NULL) 
	{
		cout << "Unable to get the host name. Error: " << EQC::Common::GetLastSocketError();
		EQC::Common::CloseSocket(send_socket);
		SetState(WSCS_Ready);
		return false;
	}
	
	memcpy ((char FAR *)&(server_sin.sin_addr), phostent->h_addr, phostent->h_length);
	server_sin.sin_port = htons(WORLD_PORT);

	// Establish a connection to the server socket.
	if (connect (send_socket, (PSOCKADDR) &server_sin, sizeof (server_sin)) == SOCKET_ERROR)
	{

		cout << "WorldServer connect: Connecting to the server failed. Error: " << EQC::Common::GetLastSocketError() << endl;
		EQC::Common::CloseSocket(send_socket);
		SetState(WSCS_Ready);
		return false;
	}	
	/*#ifdef WIN32
	ioctlsocket (send_socket, FIONBIO, &nonblocking);
	#else
	fcntl(send_socket, F_SETFL, O_NONBLOCK);
	#endif*/

	// had to put a mini-receivedata here to handel switching the TCP
	// connection to Zoneserver mode when enabled console commands
	int status = 0;
	uchar buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	Timer con_timeout(15000);
	con_timeout.Start();
	SetState(WSCS_Authenticating);
	
	while (1)
	{
		if (!RunLoops)
		{
			EQC::Common::CloseSocket(send_socket);
			SetState(WSCS_Ready);
			return false;
		}
		if (FromInit)
		{
			Timer::SetCurrentTime();
		}

		status = recv(send_socket, (char*) buffer, sizeof(buffer), 0);
		
		if (status >= 1)
		{
			char Bf[64]; memset(Bf, 0, sizeof(Bf));
			char Bf2[64]; memset(Bf2, 0, sizeof(Bf2));
			char Bf3[64]; memset(Bf3, 0, sizeof(Bf3));
			sprintf(Bf, "**ZONESERVER*%s*\r\n", EQC_VERSION);
			sprintf(Bf2, "**ZONESERVER*%s*\r\nZoneServer Mode", EQC_VERSION);
			sprintf(Bf3, "**ZONESERVER*%s*\r\nNot Authorized.", EQC_VERSION);

			if (strncmp((char*) buffer, "Username: ", 10) == 0)
			{
				send(send_socket, Bf, strlen(Bf), 0);
			}
			else if (strncmp((char*) buffer, "ZoneServer Mode", 15) == 0 || strncmp((char*) buffer, Bf2, strlen(Bf2)) == 0)
			{
				// Connection successful
				break;
			}
			else if (strncmp((char*) buffer, "Not Authorized.", 15) == 0 || strncmp((char*) buffer, Bf3, strlen(Bf3)) == 0)
			{
				// Connection failed. Worldserver said get lost
				EQC::Common::CloseSocket(send_socket);
				SetState(WSCS_Ready);
				return false;
			}
			else if (strncmp((char*) buffer, Bf, strlen(Bf)) == 0) 
			{
				// Catch the echo, wait for next packet
			}
			else
			{
				cout << "WorldServer connect: Connecting to the server failed: switch stage. Unexpected response." << endl;
				//DumpPacket(buffer, status);
				EQC::Common::CloseSocket(send_socket);
				SetState(WSCS_Ready);
				return false;
			}
		}
		else if (status == SOCKET_ERROR)
		{
			if (!(WSAGetLastError() == WSAEWOULDBLOCK)) {
				cout << "WorldServer connect: Connecting to the server failed: switch stage. Error: " << EQC::Common::GetLastSocketError() << endl;
				EQC::Common::CloseSocket(send_socket);
				SetState(WSCS_Ready);
				return false;
			}
		}
		
		
		if (con_timeout.Check()) 
		{
			cout << "WorldServer connect: Connecting to the server failed: switch stage. Time out." << endl;
			EQC::Common::CloseSocket(send_socket);
			SetState(WSCS_Ready);
			return false;
		}

		if (GetState() != WSCS_Authenticating)
		{
			EQC::Common::CloseSocket(send_socket);
			SetState(WSCS_Ready);
			return false;
		}
		Sleep(1);
	}

	if (GetState() != WSCS_Authenticating)
	{
		EQC::Common::CloseSocket(send_socket);

		SetState(WSCS_Ready);
		return false;
	}

	in.s_addr = server_sin.sin_addr.s_addr;
	EQC::Common::PrintF(CP_WORLDSERVER, "Connected to worldserver: %s:%i\n", inet_ntoa(in), ntohs(server_sin.sin_port));
	ip = in.s_addr;
	port = server_sin.sin_port;
	SetState(WSCS_Connected);
	this->SetConnectInfo();

	if (ZoneLoaded)
	{
		this->SetZone(zone->GetShortName());
		entity_list.UpdateWho();
		this->SendEmoteMessage(0, 0, 15, "Zone connect: %s", zone->GetLongName());
	}
	return true;
}

void WorldServer::Disconnect()
{
	ReconnectTimer->Start();
	LockMutex lock(&MStateLock);

	if (connection_state == WSCS_Ready || connection_state == WSCS_Construction)
	{
		return;
	}
	else if (connection_state == WSCS_Connected)
	{
		this->SendPacketQueue(true);
		shutdown(send_socket, 0x02);
		EQC::Common::CloseSocket(send_socket);
		connection_state = WSCS_Ready;
	}
	else
	{
		connection_state = WSCS_Disconnecting;
	}
}

bool WorldServer::RezzPlayer(APPLAYER* rpack, char* ownerName, int32 rezzexp, int16 opcode, int32 dbid)
{
	ServerPacket* pack = new ServerPacket(ServerOP_RezzPlayer, sizeof(RezzPlayer_Struct));
	RezzPlayer_Struct* sem = (RezzPlayer_Struct*)pack->pBuffer;
	Resurrect_Struct* rez = (Resurrect_Struct*)rpack->pBuffer;
	memset(pack->pBuffer, 0, sizeof(RezzPlayer_Struct));
	memcpy(&sem->rez, (Resurrect_Struct*)rpack->pBuffer, sizeof(Resurrect_Struct));
	sem->rezzopcode = opcode;
	sem->exp = rezzexp;
	strcpy(sem->owner, ownerName);
	sem->corpseDBID = dbid;
	bool ret = SendPacket(pack);
	safe_delete(pack);
	return ret;
}

// this should always be called in a new thread
void WorldServerLoop(void *tmp)
{
	WorldLoopRunning = true;
	while(RunLoops)
	{
		if (worldserver.Connected())
		{
			if (!(worldserver.ReceiveData() && worldserver.SendPacketQueue()))
			{
				worldserver.Disconnect();
			}
		}
		else if (worldserver.GetState() == WSCS_Ready)
		{
			if (worldserver.ReconnectTimer->Check())
			{
				worldserver.Connect();
			}
		}
		Sleep(1);
	}
	WorldLoopRunning = false;
}
