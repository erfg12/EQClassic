// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstdarg>

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <errno.h>
	#include <pthread.h>
	#include "unix.h"

	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
	extern int errno;
#endif

#include "servertalk.h"
#include "eq_packet_structs.h"
#include "packet_dump.h"
#include "database.h"
#include "classes.h"
#include "races.h"
#include "seperator.h"
#include "zoneserver.h"
#include "client.h"
#include "LoginServer.h"
#include "ZSList.h"
#include "ClientList.h"
#include "ConsoleList.h"
#include "EQCUtils.hpp"
#include "config.h"
#include "../../zone/include/zone.h"
#include "WorldGuildManager.h"
#include "BoatsManager.h"

using namespace std;

extern uint32				numclients;
extern EQC::World::ClientList		client_list;
extern volatile bool		RunLoops;

extern bool GetZoneLongName(char* short_name, char** long_name);

extern EQC::World::LoginServer* loginserver;
volatile bool ZoneLoopRunning = false;

#ifdef WIN32
void ZoneServerLoop(void *tmp);
#else
void *ZoneServerLoop(void *tmp);
#endif
void ListenNewZoneServer();
bool InitZoneServer();

// Constructor
ZoneServer::ZoneServer(int32 in_ip, int16 in_port, int in_send_socket) : TCPConnection()
{
	ip = in_ip;
	port = in_port;
	send_socket = in_send_socket;
	ID = zoneserver_list.GetNextID();
	timeout_timer = new Timer(SERVER_TIMEOUT);
	memset(zone_name, 0, sizeof(zone_name));
	memset(clientaddress, 0, sizeof(clientaddress));
	clientport = 0;
	BootingUp = false;
}

// Destructor
ZoneServer::~ZoneServer()
{
	safe_delete(timeout_timer);//delete timeout_timer;
	if (RunLoops)
	{
		zoneserver_list.ClientRemove(0, this->GetZoneName());
	}
	shutdown(send_socket, 0x01);
	shutdown(send_socket, 0x00);
	// TODO: Unregister zone
}

// Set which Zone to bootup?
bool ZoneServer::SetZone(char* zonename)
{
	BootingUp = false;
	if (strlen(zonename) >= 15) 
	{
		// Dunno why this would ever happen, but...
		cerr << "ERROR: Zone name is greater than 15 in length. This cannot be." << endl;
		return false;
	}
	char tmp[ZONE_SHORTNAME_LENGTH];
	strcpy(tmp, zone_name);
	strcpy(zone_name, zonename);
	cout << "Zoneserver SetZone: " << GetCAddress() << ":" << GetCPort() << " " << zonename << endl;
	// TODO: Register zone in DB
	client_list.ZoneBootup(this);
	EQC::World::BoatsManager::getInst()->ZoneStateChange(tmp,zonename); // Tazadar : We update the zone avaible
	cout << endl;
	return true;
}

bool ZoneServer::SetConnectInfo(char* in_address, int16 in_port) 
{
	client_list.ZoneBootup(this);
	strcpy(clientaddress, in_address);
	clientport = in_port;
	struct in_addr  in;
	in.s_addr = GetIP();
	cout << "Zoneserver SetConnectInfo: " << inet_ntoa(in) << ":" << GetPort() << ": " << clientaddress << ":" << clientport << endl;
	return true;
}

bool ZoneServer::Process()
{
#ifdef WIN32
    SOCKADDR_IN to;
#else
    struct sockaddr_in to;
#endif

	memset((char *) &to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_port = port;
    to.sin_addr.s_addr = ip;
    
	/************ Get all packets from packet manager out queue and process them ************/
	ServerPacket *pack = 0;
	while(pack = RecvQueuePop())
	{
		switch(pack->opcode) 
		{
			case 0:
			break;
			case ServerOP_KeepAlive: 
				{
				// ignore this
				break;
				}
			case ServerOP_ChannelMessage: 
				{
					this->ProcessServerOP_ChannelMessage(pack);
					break;
				}
			case ServerOP_EmoteMessage: 
				{
					this->ProcessServerOP_EmoteMessage(pack);	//Kibanu: Changed to emote message, 
					break;										// was pointing to channel message
				}
			case ServerOP_GroupRefresh:
				{
					this->ProcessServerOP_GroupRefresh(pack);	//Kibanu - Added 4/22/2009
					break;
				}
			case ServerOP_MultiLineMsg: 
				{
					this->ProcessServerOP_MultiLineMsg(pack);
					break;
				}
			case ServerOP_RezzPlayer: 
				{
					this->ProcessServerOP_RezzPlayer(pack);
					break;
				}		    
			case ServerOP_SetZone: 
				{
					this->ProcessServerOP_SetZone(pack);
					break;
				}
			case ServerOP_ZoneRunStatus:
				{
					this->ProcessServerOP_ZoneRunStatus(pack);
					break;
				}
			case ServerOP_SetConnectInfo: 
				{
					this->ProcessServerOP_SetConnectInfo(pack);
					break;
				}
			case ServerOP_ShutdownAll: 
				{
					this->ProcessServerOP_ShutdownAll(pack);
					break;
				}
			case ServerOP_ZoneShutdown:
				{
					this->ProcessServerServerOP_ZoneShutdown(pack);
					break;
				}
			case ServerOP_ZoneBootup: 
				{
					this->ProcessServerOP_ZoneBootup(pack);
					break;
				}
			case ServerOP_ZoneStatus: 
				{
					this->ProcessServerOP_ZoneStatus(pack);
					break;
				}
			case ServerOP_ClientList:
				{
					this->ProcessServerOP_ClientList(pack);
					break;
				}
			case ServerOP_Who:
				{
					this->ProcessServerOP_Who(pack);
					break;
				}
			case ServerOP_BoatNP:
				{
					this->ProcessServerOP_BoatNP(pack);
					break;
				}
			case ServerOP_BoatPL:
				{
					this->ProcessServerOP_BoatPL(pack);
					break;
				}
			case ServerOP_ZonePlayer: 
				{
					this->ProcessServerOP_ZonePlayer(pack);
					break;
				}
			case ServerOP_TravelDone:
				{
					this->ProcessServerOP_TravelDone(pack);
					break;
				}
			case ServerOP_RefreshGuild: 
				{
					guild_mgr.ProcessServerOP_RefreshGuild(pack);
					break;
				}
			case ServerOP_GuildCreateRequest:
				{
					guild_mgr.ProcessServerOP_GuildCreateRequest(pack);
					break;
				}

			case ServerOP_GuildLeader:
			case ServerOP_GuildInvite:
			case ServerOP_GuildRemove:
			case ServerOP_GuildPromote:
			case ServerOP_GuildDemote:
			case ServerOP_GuildGMSet:
			case ServerOP_GuildGMSetRank:
				{
					guild_mgr.ProcessServerOP_Guilds(pack);
				break;
				}
			case ServerOP_GMGoto: 
				{
					this->ProcessServerOP_GMGoto(pack);
					break;
				}
			case ServerOP_Lock: 
				{
					this->ProcessServerOP_Lock(pack);
					break;
			}
			case ServerOP_Motd: 
				{
					this->ProcessServerOP_Motd(pack);
					break;
				}
			case ServerOP_Uptime: 
				{
					this->ProcessServerOP_Uptime(pack);
					break;
				}
			// Cofruben: used to send a packet directly to a client.
			case ServerOP_SendPacket:
				{
					this->ProcessServerOP_SendPacket(pack);
					break;
				}
			case ServerOP_Petition: 
			case ServerOP_FlagUpdate:
			case ServerOP_KickPlayer: 
			case ServerOP_KillPlayer: 
			{
				zoneserver_list.SendPacket(pack);
				break;
			}		
			default:
			{
				this->ProcessServerOP_Unknown(pack);
				break;
			}
		}
		safe_delete(pack);//delete pack;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}    
	/************ Get first send packet on queue and send it! ************/
	SPackSendQueue* p = 0;
	int status = 0;
	while(p = SendQueuePop())
	{
#ifdef WIN32
		status = send(send_socket, (const char *) p->buffer, p->size, 0);
#else
		status = send(send_socket, p->buffer, p->size, 0);
#endif
		safe_delete(p);//delete p;
		if (status == SOCKET_ERROR) 
		{
			struct in_addr  in;
			in.s_addr = GetIP();
			cout << "Zoneserver send() failed, assumed disconnect: #" << GetID() << " " << zone_name << " " << inet_ntoa(in) << ":" << GetPort() << " (" << GetCAddress() << ":" << GetCPort() << ")" << endl;
			cout << "send_socket=" << send_socket << ", Status: " << status << ", Errorcode: " << EQC::Common::GetLastSocketError() << endl;
			return false;
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	/************ Processing finished ************/
    if (timeout_timer->Check())
    {
		// this'll hit the "send error, assumed disconnect" if the zoneserver has gone byebye
		this->SendKeepAlive();
    }

	return true;
}

void ZoneServer::SendKeepAlive()
{
	ServerPacket *pack = new ServerPacket(ServerOP_KeepAlive, 0);
	this->SendPacket(pack);
	safe_delete(pack);//delete pack;
}

void ZoneServer::SendPacket(ServerPacket* pack) 
{
	SPackSendQueue* spsq = (SPackSendQueue*) new uchar[sizeof(SPackSendQueue) + pack->size + 4];
	if (pack->pBuffer != 0 && pack->size != 0)
	{
		memcpy((char *) &spsq->buffer[4], (char *) pack->pBuffer, pack->size);
	}
	memcpy((char *) &spsq->buffer[0], (char *) &pack->opcode, 2);
	spsq->size = pack->size+4;
	memcpy((char *) &spsq->buffer[2], (char *) &spsq->size, 2);
	LockMutex lock(&MPacketLock);
	ServerSendQueue.push(spsq);
}

SPackSendQueue* ZoneServer::SendQueuePop()
{
	SPackSendQueue* spsq = 0;
	LockMutex lock(&MPacketLock);
	spsq = ServerSendQueue.pop();
	return spsq;
}

void ZoneServer::RecvPacket(ServerPacket* pack)
{
	LockMutex lock(&MPacketLock);
	ServerRecvQueue.push(pack);
}

ServerPacket* ZoneServer::RecvQueuePop()
{
	ServerPacket* pack = 0;
	LockMutex lock(&MPacketLock);
	pack = ServerRecvQueue.pop();
	return pack;
}

bool ZoneServer::ReceiveData()
{
    uchar			buffer[10240];
    int				status;
    unsigned short	port;
    struct in_addr	in;
    
	status = recv(send_socket, (char *) &buffer, sizeof(buffer), 0);

    if (status > 1)
    {
		in.s_addr = GetIP();
		//cout << "TCPData from ip: " << inet_ntoa(in) << " port:" << GetPort() << " length: " << status << endl;
		//DumpPacket(buffer, status);

		timeout_timer->Start();
		int16 base = 0;
		int16 size = 0;
		// Fucking TCP stack will throw all the packets queued at you at once in one buffer
		// Have to find the packet splitpoints
		// Hoping it'll never throw only half a packet, but who knows :/
		while (base < status) 
		{
			size = 4;
			if ((status - base) < 4)
			{
				cout << "ZoneServer: packet too short for processing" << endl;
			}
			else
			{
				ServerPacket* pack = new ServerPacket;
				memcpy((char*) &size, (char*) &buffer[base + 2], 2);
				memcpy((char*) &pack->opcode, (char*) &buffer[base + 0], 2);
				pack->size = size - 4;
				if (size > 4) 
				{
					pack->pBuffer = new uchar[pack->size];
					memcpy((char*) pack->pBuffer, (char*) &buffer[base + 4], pack->size);
				} 
				else
				{
					pack->pBuffer = 0;
				}
				//cout << "Packet from zoneserver: OPcode=" << pack->opcode << " size=" << pack->size << endl;
				//DumpPacket(buffer, status);
				RecvPacket(pack);
			}
			base += size;
		}
	} 
	else if (status == SOCKET_ERROR) 
	{
#ifdef WIN32
		if (!(WSAGetLastError() == WSAEWOULDBLOCK)) 
#else
		if (!(errno == EWOULDBLOCK)) 
#endif
		{
			struct in_addr  in;
			in.s_addr = GetIP();
			cout << "Zoneserver connection lost: " << zone_name << " " << inet_ntoa(in) << ":" << GetPort() << " (" << GetCAddress() << ":" << GetCPort() << ")" << endl;
			EQC::World::BoatsManager::getInst()->ZoneShutDown(zone_name); // Tazadar : we lost a zone !
			return false;
		}
	}
	return true;
}

void ZoneServer::SendEmoteMessage(char* to, int32 to_guilddbid, int32 type, char* message, ...)
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
	
	if (to != 0) 
	{
		strcpy((char *) sem->to, to);
	}
	else 
	{
		sem->to[0] = 0;
	}

	sem->guilddbid = to_guilddbid;
	sem->type = type;
	strcpy(&sem->message[0], buffer);

	this->SendPacket(pack);
	safe_delete(pack);//delete pack;
}

#ifdef WIN32
void ZoneServerLoop(void *tmp)
#else
void *ZoneServerLoop(void *tmp)
#endif
{
	ZoneLoopRunning = true;
	while(RunLoops) 
	{
		zoneserver_list.ReceiveData();
		zoneserver_list.Process();
		zoneserver_list.CLCheckStale();
		Sleep(1);
	}
	ZoneLoopRunning = false;
}

void ZoneServer::TriggerBootup(char* zonename, char* adminname) 
{
	BootingUp = true;
	ServerPacket* pack = new ServerPacket;
	pack->opcode = ServerOP_ZoneBootup;
	pack->size = sizeof(ServerZoneStateChange_struct);
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
	s->ZoneServerID = ID;
	if (adminname != 0)
	{
		strcpy(s->adminname, adminname);
	}
	if (zonename == 0)
	{
		strcpy(s->zonename, this->zone_name);
	}
	else
	{
		strcpy(s->zonename, zonename);
	}
	SendPacket(pack);
	safe_delete(pack);//delete pack;
}