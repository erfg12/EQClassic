// ***************************************************************
//  Petition Manager   ·  date: 16/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// This class is for use with handling peitions
// ***************************************************************

#ifndef ZONEGUILDMANAGER_H
#define ZONEGUILDMANAGER_H
#include <map>
#include <string>
#include <vector>

#include "client.h"
#include "database.h"

#include "worldserver.h"
#include "spdat.h"
#include "MessageTypes.h"
#include "eq_packet_structs.h"
#include "GuildNetwork.h"
#include "GuildDatabase.h"
#include "PacketUtils.hpp"

#include "entitylist.h"

namespace EQC
{
	namespace Zone
	{
		class ZoneGuildManager 
		{
		public:
			ZoneGuildManager();
			~ZoneGuildManager();
			bool AddPCToGuild(int32 GuidID, int32 Inviter, int32 Invitee, int8 rank);
			bool RemovePCFromGuild(Client* Remover, Client* Removee);
			bool Promote(int32 GuidID, int32 Promoter, int32 Promtee, int8 rank);
			bool Demote(int32 GuidID, int32 Demoter, int32 Demotee, int8 rank);
			void CreateGuild(Client* Persondoingit, Client* leader, char* GuildName, int32 out_guilddbid);
			bool DeleteGuild(int32 GuildLeader, int32 GuildID);
			bool SetMOTD(int32 GuildID, int MOTDSetter, char* MOTD);
			int CountGuilds();
			void Process(APPLAYER* pApp, Client* c);
			void Process_ServerTalk(ServerPacket *pack);
			bool InviteToGuild(int32 guilddbid, int32 guildeqid, Client* Inviter, Client* Invitee, GUILDRANK rank);
			void SendGuildRefreshPacket(int32 eqid);
			
			void LoadGuilds();
			Guild_Struct* GetGuildList();

			void SendGuildDemotePacket(char* target);
			void SendGuildPromotePacket(char* PromoterName, char* PromoteeName);
			void CreateGuildRequest(Client* Persondoingit, char* leader, char* GuildName);
void SendGuildInvitePacketToClient(Client* Inviter, Client* Invitee, int32 guilddbid, int32 guildeqid, GUILDRANK Rank);
		protected:

		private:
			int TotalGuids;
			Guild_Struct* GuildList;
			void InitGuilds();
			void InitZoneDatabase();
			EQC::Common::GuildDatabase* guild_database;

			void SendGuildInvitePacketToWorldServer(Client* Inviter, Client* Invitee, int32 guilddbid, int32 guildeqid, GUILDRANK rank );
			
			void SendGuildRemovePacketToWorldServer(Client* Remover, Client* Removee);
			
			void ProcessOP_GuildInvite(APPLAYER* pApp);
			void ProcessOP_GuildInviteAccept(APPLAYER* pApp);
			void ProcessOP_GuildRemove(APPLAYER* pApp);
			void ProcessOP_GuildLeader(APPLAYER* pApp);
			void ProcessOP_GuildMOTD(APPLAYER* pApp);
			void SendGuildCreationRequest(char* guildleader, char* GuildName);
			std::map<uint32, Guild_Struct *> m_guilds;	//we own the pointers in this map



			
		};
	}
}

#endif