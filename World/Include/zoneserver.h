// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef ZONESERVER_H
#define ZONESERVER_H

#include "linked_list.h"
#include "timer.h"
#include "queue.h"
#include "servertalk.h"
#include "eq_packet_structs.h"
#include "TCPConnection.h"
#include "console.h"
#include "Mutex.h"
#include "ServerPacket.h"

class ClientListEntry;

using namespace EQC::World::Network;

#ifdef WIN32
void ZoneServerLoop(void *tmp);
#else
void *ZoneServerLoop(void *tmp);
#endif

class ZoneServer : public TCPConnection
{
public:
	ZoneServer(int32 ip, int16 port, int send_socket);
	~ZoneServer();

	bool	Process();
	bool	ReceiveData();
	void	SendPacket(ServerPacket* pack);
	void	SendEmoteMessage(char* to, int32 to_guilddbid, int32 type, char* message, ...);
	bool	SetZone(char* zonename);
	bool	SetConnectInfo(char* in_address, int16 in_port);
	void	TriggerBootup(char* zonename = 0, char* adminname = 0);

	char*	GetZoneName()	{ return zone_name; }
	int32	GetIP()			{ return ip; }
	int16	GetPort()		{ return port; }
	char*	GetCAddress()	{ return clientaddress; }
	int16	GetCPort()		{ return clientport; }
	int32	GetID()			{ return ID; }
	bool	IsBootingUp()	{ return BootingUp; }

private:
	Mutex	MPacketLock;
	SPackSendQueue* SendQueuePop();
	void RecvPacket(ServerPacket* pack);
	ServerPacket* RecvQueuePop();
	void SendKeepAlive();

	int32	ID;
	int		send_socket;
	int32	ip;
	int16	port;
	char	clientaddress[250];
	int16	clientport;
	bool	BootingUp;

	Timer* timeout_timer;

	MyQueue<ServerPacket> ServerRecvQueue;
	MyQueue<SPackSendQueue> ServerSendQueue;

	char zone_name[16];

	void ProcessServerOP_ChannelMessage(ServerPacket* pack);
	void ProcessServerOP_EmoteMessage(ServerPacket* pack);
	void ProcessServerOP_GroupRefresh(ServerPacket* pack);
	void ProcessServerOP_MultiLineMsg(ServerPacket* pack);
	void ProcessServerOP_SetZone(ServerPacket* pack);
	void ProcessServerOP_SetConnectInfo(ServerPacket* pack);
	void ProcessServerOP_ShutdownAll(ServerPacket* pack);
	void ProcessServerServerOP_ZoneShutdown(ServerPacket* pack);
	void ProcessServerOP_ZoneBootup(ServerPacket* pack);
	void ProcessServerOP_ZoneStatus(ServerPacket* pack);
	void ProcessServerOP_ClientList(ServerPacket* pack);
	void ProcessServerOP_Who(ServerPacket* pack);
	void ProcessServerOP_ZonePlayer(ServerPacket* pack);
	//void ProcessServerOP_RefreshGuild(ServerPacket* pack);
	void ProcessServerOP_Guilds(ServerPacket* pack);
	void ProcessServerOP_Motd(ServerPacket* pack);
	void ProcessServerOP_Uptime(ServerPacket* pack);
	void ProcessServerOP_Unknown(ServerPacket* pack);
	void ProcessServerOP_GMGoto(ServerPacket* pack);
	void ProcessServerOP_Lock(ServerPacket* pack);
	void ProcessServerOP_RezzPlayer(ServerPacket* pack);
	void ProcessServerOP_SendPacket(ServerPacket* pack);
	// Tazadar : Boat related functions
	void ProcessServerOP_BoatNP(ServerPacket* pack);
	void ProcessServerOP_BoatPL(ServerPacket* pack);
	void ProcessServerOP_TravelDone(ServerPacket* pack);
	void ProcessServerOP_ZoneRunStatus(ServerPacket* pack);
};




#endif
