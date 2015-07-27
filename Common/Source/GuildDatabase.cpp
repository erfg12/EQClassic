#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errmsg.h>

#include "config.h"
#include "EQCUtils.hpp"
#include "GuildDatabase.h"
#include "database.h"

using namespace std;
using namespace EQC::Common::Network;

namespace EQC
{
	namespace Common
	{
		// Loads guilds from the database and into out_guilds with relevent guild and rank information
		void GuildDatabase::LoadGuilds(Guild_Struct** out_guilds)
		{
			char errbuf[MYSQL_ERRMSG_SIZE];
			char *query = 0;

			MYSQL_RES *result;
			MYSQL_ROW row;

			*out_guilds = new Guild_Struct[MAX_GUILDS];

			// Loop though the array and set the defaults
			for(int GuildInex = 0; GuildInex < MAX_GUILDS; GuildInex++)
			{
				(*out_guilds)[GuildInex].databaseID = 0;
				(*out_guilds)[GuildInex].leader = 0;
				memset((*out_guilds)[GuildInex].name, 0, sizeof((*out_guilds)[GuildInex].name));
			}

			// run the query
			if (Database::Instance()->RunQuery(query, MakeAnyLenString(&query, "SELECT id, eqid, name, leader from guilds"), errbuf, &result))
			{
				safe_delete_array(query);

				int32 GuildID = 0xFFFFFFFF; // EQ Guild ID
				
				while (row = mysql_fetch_row(result))
				{
					// Set the EQ Guid ID ID
					GuildID = atoi(row[1]);

					// Set Leader Account ID
					//TODO: Change this to char ID
					(*out_guilds)[GuildID].leader = atoi(row[3]);
					
					// Set Guild Database ID
					(*out_guilds)[GuildID].databaseID = atoi(row[0]);
					
					// Set Guild Name
					strncpy((*out_guilds)[GuildID].name, row[2], sizeof((*out_guilds)[GuildID].name));

					// Set Member Rank information
					strcpy((*out_guilds)[GuildID].rank[GuildMember].rankname, "Member");
					(*out_guilds)[GuildID].rank[GuildMember].heargu = 1;
					(*out_guilds)[GuildID].rank[GuildMember].speakgu = 1;
					(*out_guilds)[GuildID].rank[GuildMember].invite = 0;
					(*out_guilds)[GuildID].rank[GuildMember].remove = 1;
					(*out_guilds)[GuildID].rank[GuildMember].promote = 0;
					(*out_guilds)[GuildID].rank[GuildMember].demote = 0;
					(*out_guilds)[GuildID].rank[GuildMember].motd = 0;
					(*out_guilds)[GuildID].rank[GuildMember].warpeace = 0;

					// Set Officer Rank information
					strcpy((*out_guilds)[GuildID].rank[GuildOffice].rankname, "Officer");
					(*out_guilds)[GuildID].rank[GuildOffice].heargu = 1;
					(*out_guilds)[GuildID].rank[GuildOffice].speakgu = 1;
					(*out_guilds)[GuildID].rank[GuildOffice].invite = 1;
					(*out_guilds)[GuildID].rank[GuildOffice].remove = 1;
					(*out_guilds)[GuildID].rank[GuildOffice].promote = 1;
					(*out_guilds)[GuildID].rank[GuildOffice].demote = 1;
					(*out_guilds)[GuildID].rank[GuildOffice].motd = 1;
					(*out_guilds)[GuildID].rank[GuildOffice].warpeace = 0;

					// Set Leader Rank information
					strcpy((*out_guilds)[GuildID].rank[GuildLeader].rankname, "Leader");
					(*out_guilds)[GuildID].rank[GuildLeader].heargu = 1;
					(*out_guilds)[GuildID].rank[GuildLeader].speakgu = 1;
					(*out_guilds)[GuildID].rank[GuildLeader].invite = 1;
					(*out_guilds)[GuildID].rank[GuildLeader].remove = 1;
					(*out_guilds)[GuildID].rank[GuildLeader].promote = 1;
					(*out_guilds)[GuildID].rank[GuildLeader].demote = 1;
					(*out_guilds)[GuildID].rank[GuildLeader].motd = 1;
					(*out_guilds)[GuildID].rank[GuildLeader].warpeace = 1;
					
					// Set Non-Guild-Member Rank information
					strcpy((*out_guilds)[GuildID].rank[NotInaGuild].rankname, "Guild Free");
					(*out_guilds)[GuildID].rank[NotInaGuild].heargu = 0;
					(*out_guilds)[GuildID].rank[NotInaGuild].speakgu = 0;
					(*out_guilds)[GuildID].rank[NotInaGuild].invite = 0;
					(*out_guilds)[GuildID].rank[NotInaGuild].remove = 0;
					(*out_guilds)[GuildID].rank[NotInaGuild].promote = 0;
					(*out_guilds)[GuildID].rank[NotInaGuild].demote = 0;
					(*out_guilds)[GuildID].rank[NotInaGuild].motd = 0;
					(*out_guilds)[GuildID].rank[NotInaGuild].warpeace = 0;
				}
				mysql_free_result(result);
			}
			else
			{
				cerr << "Error in Load Guilds query '" << query << "' " << errbuf << endl;
				safe_delete_array(query);
			}
		}
	}
}

