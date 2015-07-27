// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef ZSLIST_H
#define ZSLIST_H


#include "linked_list.h"
#include "timer.h"
#include "queue.h"
#include "servertalk.h"
#include "eq_packet_structs.h"
#include "console.h"
#include "Mutex.h"
#include "ConsoleList.h"
#include "GuildNetwork.h"
#include "net.h"
#include "database.h"

namespace EQC
{
	namespace World
	{
		namespace Network
		{
			class ZSList
			{
			public:
				ZSList();
				~ZSList();
				ZoneServer* FindByName(char* zonename);
				ZoneServer* FindByID(int32 ZoneID);

				void	SendChannelMessage(char* from, char* to, int8 chan_num, int8 language, char* message, ...);
				void	SendEmoteMessage(char* to, int32 to_guilddbid, int32 type, char* message, ...);
				void	ForceGroupRefresh(char* member, int32 groupid, int8 action);

				void	ClientUpdate(ZoneServer* zoneserver, char* name, int32 accountid, char* accountname, int8 admin, char* zone, int8 level, int8 class_, int8 race, int8 anon, bool tellsoff, int32 in_guilddbid, int32 in_guildeqid, bool LFG);
				void	ClientRemove(char* name, char* zone);
				LinkedList<ClientListEntry*>* GetClientList();

				void	SendWhoAll(char* to, int8 admin, Who_All_Struct* whom, TCPConnection* connection);
				void	SendZoneStatus(char* to, int8 admin, TCPConnection* connection);
				ClientListEntry* FindCharacter(char* name);
				void	CLCheckStale();

				void	Add(ZoneServer* zoneserver);
				void	Process();
				void	ReceiveData();
				void	SendPacket(ServerPacket* pack);
				void	SendPacket(char* to, ServerPacket* pack);
				int32	GetNextID()		{ return NextID++; }

				int32	TriggerBootup(char* zonename);
				static void SOPZoneBootup(char* adminname, int32 ZoneServerID, char* zonename);
				Mutex	MListLock;
			private:
				int32 NextID;
				LinkedList<ZoneServer*> list;
				LinkedList<ClientListEntry*> clientlist;
			};
		}
	}
}

extern EQC::World::Network::ZSList			zoneserver_list;
extern EQC::World::Network::ConsoleList		console_list;
extern EQC::World::Network::NetConnection net;
extern Database			database;

#endif
