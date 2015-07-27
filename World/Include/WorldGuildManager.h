// ***************************************************************
//  World Guild Manager   ·  date: 12/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// This class is for use when the world needs to handle guilds
// ***************************************************************


#ifndef WORLDGUILDMANAGER_H
#define WORLDGUILDMANAGER_H

#include "WorldGuildNetworkManager.h"
#include "WorldGuildDatabaseManager.h"
#include "GuildNetwork.h"
#include "servertalk.h"
#include "database.h"
#include "zoneserver.h"
#include "zslist.h"
#include "ClientListEntry.h"
#include "client.h"
#include "PacketUtils.hpp"

using namespace EQC::World::Network;
using namespace EQC::World::WorldDatabase;
using namespace EQC::Common::Network;

extern ZSList zoneserver_list;
namespace EQC
{
	namespace World
	{
		class WorldGuildManager 
		{
		public:
			WorldGuildManager();
			~WorldGuildManager();
			
			
			void ProcessServerOP_RefreshGuild(ServerPacket* pack);
			void ProcessServerOP_Guilds(ServerPacket* pack);
			GuildsListEntry_Struct CreateBlankGuildsListEntry_Struct();
			void SendGuildsList(Client* c);
			void ProcessServerOP_GuildCreateRequest(ServerPacket* pack);
			int32 CreateGuild(Client* Persondoingit, char* LeaderCharName, char* GuildName);
			void LoadGuilds();
			Guild_Struct* GetGuildList();
		protected:

		private:
			WorldGuildNetworkManager* guild_network_mgr;
			WorldGuildDatabaseManager* guild_database_mgr;
			Guild_Struct* guilds;
			
		};
	}
}

extern EQC::World::WorldGuildManager guild_mgr;

#endif