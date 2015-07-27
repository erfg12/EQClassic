// ***************************************************************
//  Zone Guild Manager World Processor  ·  date: 16/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// Will handle everything to do with guilds on the zone level
// ***************************************************************

#include <cstdio>
#include <cstring>
#include <string>
#include <iomanip>
#include "ZoneGuildManager.h"

extern WorldServer worldserver;

using namespace std;
using namespace EQC::Zone;

namespace EQC
{
	namespace Zone
	{

		void ZoneGuildManager::Process_ServerTalk(ServerPacket *pack)
		{
			if(pack == 0)
			{
				return; // dont even bother
			}

			switch(pack->opcode)
			{
			case ServerOP_RefreshGuild:
				break;
			case ServerOP_GuildLeader:
				break;
			case ServerOP_GuildInvite:
				break;
			case ServerOP_GuildRemove:
				break;
			case ServerOP_GuildPromote: 
				break;
			case ServerOP_GuildDemote: 
				break;
			case ServerOP_GuildGMSet: 
				break;
			case ServerOP_GuildGMSetRank: 
				break;
			}
		}
	}
}