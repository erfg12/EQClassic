#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <assert.h>
#include <math.h>

#ifndef WIN32
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "client.h"
#include "database.h"
#include "EQCUtils.hpp"
#include "packet_functions.h"
#include "packet_dump.h"
#include "worldserver.h"
#include "packet_dump_file.h"
#include "PlayerCorpse.h"
#include "spdat.h"
#include "petitions.h"
#include "NpcAI.h"
#include "skills.h"
#include "EQCException.hpp"
#include "MessageTypes.h"
#include "forage.h"
#include "npc.h"
#include "eq_packet_structs.h"
#include "GuildNetwork.h"
#include "PacketUtils.hpp"
#include "ZoneGuildManager.h"
#include "SpellsHandler.hpp"

using namespace std;
using namespace EQC::Common;
using namespace EQC::Zone;

extern Database			database;
extern Zone*			zone;
extern volatile bool	ZoneLoaded;
extern WorldServer		worldserver;
extern ZoneGuildManager zgm;
extern SpellsHandler	spells_handler;
extern PetitionList		petition_list;


struct GuildManageStatus_Struct
{
	int32	guildid;
	int32	oldrank;
	int32	newrank;
	char	name[64];
};

bool Client::RemoveFromGuild(int32 in_guilddbid)
{
	// update DB
	if (!Database::Instance()->SetGuild(character_id, 0, GUILD_MAX_RANK))
	{
		return false;
	}
	// clear guildtag
	this->guilddbid = in_guilddbid;
	this->guildeqid = 0xFFFFFFFF;
	this->guildrank = GuildUnknown;

	this->SendAppearancePacket(this->GetID(), SAT_Guild_ID, 0xFFFFFFFF, true); // Whats 22 mean?

	this->SendAppearancePacket(this->GetID(), SAT_Guild_Rank, 3, true); // Whats 3 mean?

	UpdateWho();
	return true;
}

bool Client::SetGuild(int32 in_guilddbid, int8 in_rank)
{
	bool bval = false;

	if (in_guilddbid == 0) // if its zero, its a guild removal
	{
		bval = this->RemoveFromGuild(in_guilddbid);
	}
	else
	{
		int32 eqGuildID = Database::Instance()->GetGuildEQID(in_guilddbid);

		if (eqGuildID != 0xFFFFFFFF) // could we just use 0? lol
		{
			// update the database with what guild we are in and what is our rank
			if (!Database::Instance()->SetGuild(this->character_id, in_guilddbid, in_rank))
			{
				return false; // if the update fails.. bail out!
			}

			// Update what guild we are in
			this->guildeqid = eqGuildID;
			this->guildrank = (GUILDRANK)in_rank;

			APPLAYER* outapp;

			if (guilddbid != in_guilddbid)
			{
				this->guilddbid = in_guilddbid;			
				this->SendAppearancePacket(this->GetID(), SAT_Guild_ID, guildeqid, true);
			}
					
			int32 Parameter = 0;

			zgm.LoadGuilds();
			Guild_Struct* guilds = zgm.GetGuildList();


			if ((guilds[eqGuildID].rank[in_rank].warpeace) || (guilds[eqGuildID].leader == this->account_id))
			{
				Parameter = 2;
			}
			else if ((guilds[eqGuildID].rank[in_rank].invite) || (guilds[eqGuildID].rank[in_rank].remove) || (guilds[eqGuildID].rank[in_rank].motd))
			{
				Parameter = 1;
			}
			else
			{
				Parameter = 0;
			}
			this->SendAppearancePacket(this->GetID(), SAT_Guild_Rank, Parameter, true);
			
			// if we get here, its successful
			bval = true;
		}
	}

	// Update /Who
	this->UpdateWho();

	return bval;
}


void Client::GuildPCCommandHelp()
{
	this->Message(BLACK, "Guild commands:");
	this->Message(BLACK, "  #guild status [name] - shows guild and rank of target");
	this->Message(BLACK, "  #guild info guildnum - shows info/current structure");
	this->Message(BLACK, "  #guild invite [charname]");
	this->Message(BLACK, "  #guild remove [charname]");
	this->Message(BLACK, "  #guild promote charname rank");
	this->Message(BLACK, "  #guild demote charname rank");
	this->Message(BLACK, "  /guildmotd [newmotd]    (use 'none' to clear)");
	this->Message(BLACK, "  #guild edit rank title newtitle");
	this->Message(BLACK, "  #guild edit rank permission 0/1");
	this->Message(BLACK, "  #guild leader newleader (they must be rank0)");
}

void Client::GuildGMCommandHelp()
{
	if (this->admin >= 100)
	{
		this->Message(BLACK, "GM Guild commands:");
		this->Message(BLACK, "  #guild list - lists all guilds on the server");
		this->Message(BLACK, "  #guild create {guildleader charname or AccountID} guildname");
		this->Message(BLACK, "  #guild delete guildDBID");
		this->Message(BLACK, "  #guild rename guildDBID newname");
		this->Message(BLACK, "  #guild set charname guildDBID    (0=no guild)");
		this->Message(BLACK, "  #guild setrank charname rank");
		this->Message(BLACK, "  #guild gmedit guilddbid rank title newtitle");
		this->Message(BLACK, "  #guild gmedit guilddbid rank permission 0/1");
		this->Message(BLACK, "  #guild setleader guildDBID {guildleader charname or AccountID}");
	}
}

void Client::GuildCommand(Seperator* sep)
{
		zgm.LoadGuilds();
		Guild_Struct* guilds = zgm.GetGuildList();


	if (strcasecmp(sep->arg[1], "help") == 0)
	{
		this->GuildPCCommandHelp();
		this->GuildGMCommandHelp();
	}
	else if (strcasecmp(sep->arg[1], "status") == 0 || strcasecmp(sep->arg[1], "stat") == 0)
	{
		Client* client = 0;

		if (sep->arg[2][0] != 0)
		{
			client = entity_list.GetClientByName(sep->argplus[2]);
		}
		else if (target != 0 && target->IsClient())
		{
			client = target->CastToClient();
		}

		if (client == 0)
		{
			this->Message(BLACK, "You must target someone or specify a character name");
		}
		else if ((client->Admin() >= 100 && admin < 100) && client->GuildDBID() != guilddbid) // no peeping for GMs, make sure tell message stays the same
		{
			this->Message(BLACK, "You must target someone or specify a character name.");
		}
		else 
		{
			if (client->GuildDBID() == 0)
			{
				this->Message(BLACK, "%s is not in a guild.", client->GetName());
			}
			else if (guilds[client->GuildEQID()].leader == client->AccountID())
			{
				this->Message(BLACK, "%s is the leader of <%s> rank: %s", client->GetName(), guilds[client->GuildEQID()].name, guilds[client->GuildEQID()].rank[client->GuildRank()].rankname);
			}
			else
			{
				this->Message(BLACK, "%s is a member of <%s> rank: %s", client->GetName(), guilds[client->GuildEQID()].name, guilds[client->GuildEQID()].rank[client->GuildRank()].rankname);
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "info") == 0)
	{
		if (sep->arg[2][0] == 0 && guilddbid == 0)
		{
			if (admin >= 100)
			{
				this->Message(BLACK, "Usage: #guildinfo guilddbid");
			}
			else
			{
				this->Message(BLACK, "You're not in a guild");
			}
		}
		else
		{
			int32 tmp;
		
			if (sep->arg[2][0] == 0)
			{
				tmp = Database::Instance()->GetGuildEQID(guilddbid);
			}
			else
			{
				tmp = Database::Instance()->GetGuildEQID(atoi(sep->arg[2]));
			}

			if (tmp < 0 || tmp >= 512)
			{
				this->Message(BLACK, "Guild not found.");
			}
			else
			{
				this->Message(BLACK, "Guild info DB# %i, %s", guilds[tmp].databaseID, guilds[tmp].name);
			
				if (this->admin >= 100 || guildeqid == tmp)
				{
					if (this->account_id == guilds[tmp].leader || guildrank == 0 || admin >= 100)
					{
						char leadername[32];
						Database::Instance()->GetAccountName(guilds[tmp].leader, leadername);
						this->Message(BLACK, "Guild Leader: %s", leadername);
					}

					for (int i = 0; i <= GUILD_MAX_RANK; i++)
					{
						this->Message(BLACK, "Rank %i: %s", i, guilds[tmp].rank[i].rankname);
						this->Message(BLACK, "  HearGU: %i  SpeakGU: %i  Invite: %i  Remove: %i  Promote: %i  Demote: %i  MOTD: %i  War/Peace: %i", guilds[tmp].rank[i].heargu, guilds[tmp].rank[i].speakgu, guilds[tmp].rank[i].invite, guilds[tmp].rank[i].remove, guilds[tmp].rank[i].promote, guilds[tmp].rank[i].demote, guilds[tmp].rank[i].motd, guilds[tmp].rank[i].warpeace);
					}
				}
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "leader") == 0) 
	{
		if (guilddbid == 0)
		{
			this->Message(BLACK, "You arent in a guild!");
		}
		else if (guilds[guildeqid].leader != account_id)
		{
			this->Message(BLACK, "You aren't the guild leader.");
		}
		else
		{
			char* tmptar = 0;

			if (sep->arg[2][0] != 0)
			{
				tmptar = sep->argplus[2];
			}
			else if (tmptar == 0 && target != 0 && target->IsClient())
			{
				tmptar = target->CastToClient()->GetName();
			}
			
			if (tmptar == 0)
			{
				Message(BLACK, "You must target someone or specify a character name.");
			}
			else
			{
				// Dark-Prince - 10/05/2008 - Code Consolidation
				//zgm.InviteToGuild(
				//this->SendGuildInvitePacket(tmptar);
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "invite") == 0)
	{
		int32 GuildDBID = atoi(sep->arg[2]);
		Client* Invitee = entity_list.GetClientByName(sep->arg[3]);
		GUILDRANK Rank = (GUILDRANK)atoi(sep->arg[4]);

		int32 GuidlEQID = Database::Instance()->GetGuildEQID(GuildDBID);

		zgm.InviteToGuild(GuildDBID, GuidlEQID, this, Invitee, Rank);
	}
	else if (strcasecmp(sep->arg[1], "remove") == 0)
	{
		Client* Removee = entity_list.GetClientByName(sep->arg[2]);

		if(Removee != 0)
		{
			zgm.RemovePCFromGuild(this, Removee);
		}
	}
	else if (strcasecmp(sep->arg[1], "promote") == 0)
	{
		if (guilddbid == 0)
		{
			Message(BLACK, "You arent in a guild!");
		}
		else if (!(strlen(sep->arg[2]) == 1 && sep->arg[2][0] >= '0' && sep->arg[2][0] <= '9'))
		{
			Message(BLACK, "Usage: #guild promote rank [charname]");
		}
		else if (atoi(sep->arg[2]) < 0 || atoi(sep->arg[2]) > GUILD_MAX_RANK)
		{
			Message(BLACK, "Error: invalid rank #.");
		}
		else
		{
			char* tmptar = 0;

			if (sep->arg[3][0] != 0)
			{
				tmptar = sep->argplus[3];
			}
			else if (tmptar == 0 && target != 0 && target->IsClient())
			{
				tmptar = target->CastToClient()->GetName();
			}

			if (tmptar == 0)
			{
				Message(BLACK, "You must target someone or specify a character name.");
			}
			else
			{
				zgm.SendGuildPromotePacket(tmptar, this->GetName());
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "demote") == 0)
	{
		if (guilddbid == 0)
		{
			Message(BLACK, "You arent in a guild!");
		}
		else if (!(strlen(sep->arg[2]) == 1 && sep->arg[2][0] >= '0' && sep->arg[2][0] <= '9'))
		{
			Message(BLACK, "Usage: #guild demote rank [charname]");
		}
		else if (atoi(sep->arg[2]) < 0 || atoi(sep->arg[2]) > GUILD_MAX_RANK)
		{
			Message(BLACK, "Error: invalid rank #.");
		}
		else 
		{
			char* tmptar = 0;
			if (sep->arg[3][0] != 0)
			{
				tmptar = sep->argplus[3];
			}
			else if (tmptar == 0 && target != 0 && target->IsClient())
			{
				tmptar = target->CastToClient()->GetName();
			}
			if (tmptar == 0)
			{
				Message(BLACK, "You must target someone or specify a character name.");
			}
			else
			{
				zgm.SendGuildDemotePacket(tmptar);
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "motd") == 0)
	{
		if (guilddbid == 0)
		{
			Message(BLACK, "You arent in a guild!");
		}
		else if (!guilds[guildeqid].rank[guildrank].motd)
		{
			Message(BLACK, "You dont have permission to change the motd.");
		}
		else if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server dirconnected");
		}
		else
		{
			char tmp[255];

			if (strcasecmp(sep->argplus[2], "none") == 0)
			{
				strcpy(tmp, "");
			}
			else
			{
				snprintf(tmp, sizeof(tmp), "%s - %s", this->GetName(), sep->argplus[2]);
			}

			if (Database::Instance()->SetGuildMOTD(guilddbid, tmp))
			{
				// Dark-Prince - 10/05/2008 - Code Consolidation
/*
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_RefreshGuild;
				pack->size = 5;
				pack->pBuffer = new uchar[pack->size];
				memcpy(pack->pBuffer, &guildeqid, 4);
				worldserver.SendPacket(pack);
				safe_delete(pack);//delete pack;
*/
			}
			else
			{
				this->Message(BLACK, "Motd update failed.");
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "edit") == 0)
	{
		if (guilddbid == 0)
		{
			Message(BLACK, "You arent in a guild!");
		}
		else if (!sep->IsNumber(2))
		{
			Message(BLACK, "Error: invalid rank #.");
		}
		else if (atoi(sep->arg[2]) < 0 || atoi(sep->arg[2]) > GUILD_MAX_RANK)
		{
			Message(BLACK, "Error: invalid rank #.");
		}
		else if (!guildrank == 0)
		{
			Message(BLACK, "You must be rank %s to use edit.", guilds[guildeqid].rank[0].rankname);
		}
		else if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server dirconnected");
		}
		else
		{
			if (!GuildEditCommand(guilddbid, guildeqid, atoi(sep->arg[2]), sep->arg[3], sep->argplus[4]))
			{
				Message(BLACK, "  #guild edit rank title newtitle");
				Message(BLACK, "  #guild edit rank permission 0/1");
			}
			else
			{
				zgm.SendGuildRefreshPacket(guildeqid);
			}
		}
	}
	// Start of GM Guild Commands
	else if (strcasecmp(sep->arg[1], "gmedit") == 0 && admin >= 100) 
	{
		if (!sep->IsNumber(2))
		{
			Message(BLACK, "Error: invalid guilddbid.");
		}
		else if (!sep->IsNumber(3))
		{
			Message(BLACK, "Error: invalid rank #.");
		}
		else if (atoi(sep->arg[3]) < 0 || atoi(sep->arg[3]) > GUILD_MAX_RANK)
		{
			Message(BLACK, "Error: invalid rank #.");
		}
		else if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server dirconnected");
		}
		else 
		{
			int32 eqid = Database::Instance()->GetGuildEQID(atoi(sep->arg[2]));
			
			if (eqid == 0xFFFFFFFF)
			{
				Message(BLACK, "Error: Guild not found");
			}
			else if (!GuildEditCommand(atoi(sep->arg[2]), eqid, atoi(sep->arg[3]), sep->arg[4], sep->argplus[5]))
			{
				Message(BLACK, "  #guild gmedit guilddbid rank title newtitle");
				Message(BLACK, "  #guild gmedit guilddbid rank permission 0/1");
			}
			else
			{
				zgm.SendGuildRefreshPacket(eqid);
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "set") == 0 && admin >= 100)
	{
		if (!sep->IsNumber(3))
		{
			Message(BLACK, "Usage: #guild set charname guildgbid (0 = clear guildtag)");
		}
		else
		{
			/* -- Work in progress
			GUILDRANK rank = GuildMember;

			Client* c = entity_list.GetClientByName(sep->arg[2]);

			if(c == 0)
			{
				Message(BLACK, "Player not found!");
				return;
			}

			int GuidDBID = atoi(sep->arg[3]);

			if(sep->IsNumber(4))
			{
				rank = (GUILDRANK)atoi(sep->arg[4]);
			}
			
			zgm.LoadGuilds();

			
			int32 GuidEQID = Database::Instance()->GetGuildEQID(GuidDBID);
			zgm.InviteToGuild(GuidDBID, GuidEQID, this, c, rank);
			*/
			
			ServerPacket* pack = new ServerPacket(ServerOP_GuildGMSet, sizeof(ServerGuildCommand_Struct));
			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, pack->size);
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			sgc->guilddbid = atoi(sep->arg[3]);
			sgc->admin = admin;
			strcpy(sgc->from, name);
			strcpy(sgc->target, sep->arg[2]);
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
	else if (strcasecmp(sep->arg[1], "setrank") == 0 && admin >= 100)
	{
		if (!sep->IsNumber(3))
		{
			Message(BLACK, "Usage: #guild setrank charname rank");
		}
		else if (atoi(sep->arg[3]) < 0 || atoi(sep->arg[3]) > GUILD_MAX_RANK)
		{
			Message(BLACK, "Error: invalid rank #.");
		}
		else
		{
			ServerPacket* pack = new ServerPacket(ServerOP_GuildGMSetRank, sizeof(ServerGuildCommand_Struct));


			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, pack->size);
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			sgc->newrank = (GUILDRANK)atoi(sep->arg[3]);
			sgc->admin = admin;
			strcpy(sgc->from, name);
			strcpy(sgc->target, sep->arg[2]);
			worldserver.SendPacket(pack);
			

			target->CastToClient()->SetGuild(target->CastToClient()->guilddbid, sgc->newrank);

			safe_delete(pack);//delete pack;
		}
	}
	/* New Guild Creation -> Challenge/Response method */
	else if (strcasecmp(sep->arg[1], "dpcreate") == 0 && admin >= 100)
	{
		if (sep->arg[3][0] == 0)
		{
			Message(BLACK, "Usage: #guild create {guildleader charname or AccountID} guild name");
		}
		else if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server dirconnected");
		}
		else
		{
			zgm.CreateGuildRequest(this, sep->arg[2], sep->argplus[3]);
		}
	}
	/* END New Guild Creation -> Challenge/Response method */
	else if (strcasecmp(sep->arg[1], "create") == 0 && admin >= 100)
	{
		if (sep->arg[3][0] == 0)
		{
			Message(BLACK, "Usage: #guild create {guildleader charname or AccountID} guild name");
		}
		else if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server dirconnected");
		}
		else
		{
			Client* Leader = entity_list.GetClientByName(sep->arg[2]);

			int32 guilddbid = 0;

			if(Leader != 0)
			{
				zgm.CreateGuild(this, Leader, sep->argplus[3], guilddbid);
			}			
		}
	}
	else if (strcasecmp(sep->arg[1], "delete") == 0 && admin >= 100)
	{
		if (!sep->IsNumber(2))
		{
			Message(BLACK, "Usage: #guild delete guildDBID");
		}
		else if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server dirconnected");
		}
		else
		{
			int32 tmpeq = Database::Instance()->GetGuildEQID(atoi(sep->arg[2]));
			char tmpname[32];

			if (tmpeq != 0xFFFFFFFF)
			{
				strcpy(tmpname, guilds[tmpeq].name);
			}

			if (!Database::Instance()->DeleteGuild(atoi(sep->arg[2])))
			{
				Message(BLACK, "Guild delete failed.");
			}
			else
			{
				if (tmpeq != 0xFFFFFFFF)
				{
					zgm.LoadGuilds();

					ServerPacket* pack = new ServerPacket(ServerOP_RefreshGuild, 5);
					
					pack->pBuffer = new uchar[pack->size];
					memcpy(pack->pBuffer, &tmpeq, 4);
					pack->pBuffer[4] = 1;
					worldserver.SendPacket(pack);
					safe_delete(pack);//delete pack;

					Message(BLACK, "Guild deleted: DB# %i, EQ# %i: %s", atoi(sep->arg[2]), tmpeq, tmpname);
				} 
				else
				{
					Message(BLACK, "Guild deleted: DB# %i", atoi(sep->arg[2]));
				}
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "rename") == 0 && admin >= 100)
	{
		if ((!sep->IsNumber(2)) || sep->arg[3][0] == 0)
		{
			Message(BLACK, "Usage: #guild rename guildDBID newname");
		}
		else if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server dirconnected");
		}
		else
		{
			int32 tmpeq = Database::Instance()->GetGuildEQID(atoi(sep->arg[2]));
			char tmpname[32];

			if (tmpeq != 0xFFFFFFFF)
			{
				strcpy(tmpname, guilds[tmpeq].name);
			}

			if (!Database::Instance()->RenameGuild(atoi(sep->arg[2]), sep->argplus[3]))
			{
				Message(BLACK, "Guild rename failed.");
			}
			else 
			{
				if (tmpeq != 0xFFFFFFFF) 
				{
					ServerPacket* pack = new ServerPacket(ServerOP_RefreshGuild, 5);
					pack->pBuffer = new uchar[pack->size];
					memcpy(pack->pBuffer, &tmpeq, 4);
					pack->pBuffer[4] = 1;
					worldserver.SendPacket(pack);
					safe_delete(pack);//delete pack;

					Message(BLACK, "Guild renamed: DB# %i, EQ# %i, OldName: %s, NewName: %s", atoi(sep->arg[2]), tmpeq, tmpname, sep->argplus[3]);
				}
				else
				{
					Message(BLACK, "Guild renamed: DB# %i, NewName: %s", atoi(sep->arg[2]), sep->argplus[3]);
				}
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "setleader") == 0 && admin >= 100) 
	{
		if (sep->arg[3][0] == 0 || !sep->IsNumber(2))
		{
			Message(BLACK, "Usage: #guild setleader guilddbid {guildleader charname or AccountID}");
		}
		else if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server dirconnected");
		}
		else
		{
			int32 leader = 0;
			
			if (sep->IsNumber(3))
			{
				leader = atoi(sep->arg[3]);
			}
			else
			{
				leader = Database::Instance()->GetAccountIDByChar(sep->argplus[3]);
			}

			int32 tmpdb = Database::Instance()->GetGuildDBIDbyLeader(leader);
			
			if (leader == 0)
			{
				Message(BLACK, "New leader not found.");
			}
			else if (tmpdb != 0) 
			{
				int32 tmpeq = Database::Instance()->GetGuildEQID(tmpdb);

				if (tmpeq >= 512)
				{
					Message(BLACK, "Error: %s already is the leader of DB# %i.", sep->argplus[3], tmpdb);
				}
				else
				{
					Message(BLACK, "Error: %s already is the leader of DB# %i <%s>.", sep->argplus[3], tmpdb, guilds[tmpeq].name);
				}
			}
			else 
			{
				int32 tmpeq = Database::Instance()->GetGuildEQID(atoi(sep->arg[2]));

				if (tmpeq == 0xFFFFFFFF)
				{
					Message(BLACK, "Guild not found.");
				}
				else if (!Database::Instance()->SetGuildLeader(atoi(sep->arg[2]), leader))
				{
					Message(BLACK, "Guild leader change failed.");
				}
				else
				{
					zgm.SendGuildRefreshPacket(tmpeq);

					Message(BLACK, "Guild leader changed: DB# %s, Leader: %s, Name: <%s>", sep->arg[2], sep->argplus[3], guilds[tmpeq].name);
				}
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "list") == 0 && admin >= 100)
	{
		int x = 0;
		Message(BLACK, "Listing guilds on the server:");
		char leadername[32];
		int magicnumber = 512;

		zgm.LoadGuilds();
		guilds = zgm.GetGuildList();

		for (int i = 0; i < magicnumber; i++)
		{
			if (guilds[i].databaseID != 0)
			{
				leadername[0] = 0;
				Database::Instance()->GetAccountName(guilds[i].leader, leadername);

				if (leadername[0] == 0)
				{
					Message(BLACK, "  DB# %i EQ# %i  <%s>", guilds[i].databaseID, i, guilds[i].name);
				}
				else
				{
					Message(BLACK, "  DB# %i EQ# %i  <%s> Leader: %s", guilds[i].databaseID, i, guilds[i].name, leadername);
				}

				x++;
			}
		}
		Message(BLACK, "%i guilds listed.", x);
	}
	else if (strcasecmp(sep->arg[1], "reload") == 0 && admin >= 100)
	{

		Message(BLACK, "Reloading Guilds");

		int32 eqdbid = atoi(sep->arg[2]);
		zgm.LoadGuilds();
		zgm.SendGuildRefreshPacket(eqdbid);
	}
	else 
	{
		Message(BLACK, "Unknown guild command, try #guild help");
	}
}


bool Client::GuildEditCommand(int32 dbid, int32 eqid, int8 rank, char* what, char* value)
{
			zgm.LoadGuilds();
		Guild_Struct* guilds = zgm.GetGuildList();

	struct GuildRankLevel_Struct grl;
	strcpy(grl.rankname, guilds[eqid].rank[rank].rankname);
	grl.demote = guilds[eqid].rank[rank].demote;
	grl.heargu = guilds[eqid].rank[rank].heargu;
	grl.invite = guilds[eqid].rank[rank].invite;
	grl.motd = guilds[eqid].rank[rank].motd;
	grl.promote = guilds[eqid].rank[rank].promote;
	grl.remove = guilds[eqid].rank[rank].remove;
	grl.speakgu = guilds[eqid].rank[rank].speakgu;
	grl.warpeace = guilds[eqid].rank[rank].warpeace;

	if (strcasecmp(what, "title") == 0)
	{
		if (strlen(value) > 100)
		{
			Message(BLACK, "Error: Title has a maxium length of 100 characters.");
		}
		else
		{
			strcpy(grl.rankname, value);
		}
	}
	else if (rank == 0)
	{
		Message(BLACK, "Error: Rank 0's permissions can not be changed.");
	}
	else 
	{
		if (!(strlen(value) == 1 && (value[0] == '0' || value[0] == '1')))
		{
			return false;
		}
		if (strcasecmp(what, "demote") == 0)
		{
			grl.demote = (value[0] == '1');
		}
		else if (strcasecmp(what, "heargu") == 0)
		{
			grl.heargu = (value[0] == '1');
		}
		else if (strcasecmp(what, "invite") == 0)
		{
			grl.invite = (value[0] == '1');
		}
		else if (strcasecmp(what, "motd") == 0)
		{
			grl.motd = (value[0] == '1');
		}
		else if (strcasecmp(what, "promote") == 0)
		{
			grl.promote = (value[0] == '1');
		}
		else if (strcasecmp(what, "remove") == 0)
		{
			grl.remove = (value[0] == '1');
		}
		else if (strcasecmp(what, "speakgu") == 0)
		{
			grl.speakgu = (value[0] == '1');
		}
		else if (strcasecmp(what, "warpeace") == 0)
		{
			grl.warpeace = (value[0] == '1');
		}
		else
		{
			Message(BLACK, "Error: Permission name not recognized.");
		}
	}

	if (!Database::Instance()->EditGuild(dbid, rank, &grl))
	{
		Message(BLACK, "Error: database.EditGuild() failed");
	}
	return true;
}









bool Client::IsValidGuild() // Need to find a better name for this!
{
	/*bool bresult = false;

	if (guilddbid == 0)
	{
		Message(BLACK, "Error: You arent in a guild!");
		bresult = false;
	}
	else if (!(guilds[guildeqid].leader == account_id))
	{
		Message(BLACK, "Error: You arent the guild leader!");
		bresult = false;
	}
	else if (!worldserver.Connected())
	{
		Message(BLACK, "Error: World server disconnected");
		bresult = false;
	}
	else if (this->PendingGuildInvite == 0)
	{
		Message(BLACK, "Error: No guild invite pending.");
		bresult = false;
	}
	else
	{
		bresult = true;
	}
	return bresult;*/
	return true;//Dark-Prince for testing - 11/05/2008
}



