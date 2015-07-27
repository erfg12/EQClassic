// ***************************************************************
//  Zone Guild Manager Processor  ·  date: 16/05/2007
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
		// Processes income packets relating to guild and the client
		void ZoneGuildManager::Process(APPLAYER* pApp, Client* c)
		{
			if(pApp == 0)
			{
				return; // not a valid packet
			}

			if(c == 0)
			{
				return; // cant use this on a invalid char!
			}

			// Check if the zone is connected to the world or not
			if (!worldserver.Connected())
			{
				c->Message(BLACK, "Error: World server is disconnected, guild commands are locked.");
				return;
			}			

			switch(pApp->opcode)
			{
			case OP_GuildMOTD:
				{
					this->ProcessOP_GuildMOTD(pApp);
					break;
				}
			case OP_GuildPeace:
			case OP_GuildWar:
				{
					c->Message(BLACK, "Guildwars is not implemented.");
					break;
				}
			case OP_GuildLeader: 
				{
					this->ProcessOP_GuildLeader(pApp);
					break;
				}
			case OP_GuildInvite:
				{
					this->ProcessOP_GuildInvite(pApp);
					break;
				}
			case OP_GuildRemove:
				{
					this->ProcessOP_GuildRemove(pApp);
					break;
				}
			case OP_GuildInviteAccept:
				{
					this->ProcessOP_GuildInviteAccept(pApp);
					break;
				}

			}
		}





		// Sends the Guild Invite
		void ZoneGuildManager::ProcessOP_GuildInvite(APPLAYER* pApp)
		{
			// Check pApp
			if(pApp == 0)
			{
				return; // Dont even bother processing
			}

			// Check pApp size againt the Struct Size
			if (pApp->size != sizeof(GuildInvite_Struct))
			{
				cout << "Wrong size: OP_GuildInvite, size=" << pApp->size << ", expected " << sizeof(GuildInvite_Struct) << endl;
				return;
			}

			// Cast the pApp buffer to a GuildInvite Struct
			GuildInvite_Struct* gi = (GuildInvite_Struct*) pApp->pBuffer;

			// Get the Inviter
			Client* Inviter = entity_list.GetClientByName(gi->Inviter);

			// Get the Invitee
			Client* Invitee = entity_list.GetClientByName(gi->Invitee);

			// Get the Rank
			GUILDRANK Rank = (GUILDRANK)gi->rank;

			if((Inviter != 0) && (Invitee != 0)) // Check if Inviter and Invitee are not null
			{
				// Invite the Invitee into the guild of the Inviter
				this->InviteToGuild(Inviter->GuildDBID(), Inviter->GuildEQID(), Inviter, Invitee, Rank);
			}

			return;
		}




		
		
		// Handles the player reciviing a guild invite
		void ZoneGuildManager::ProcessOP_GuildInviteAccept(APPLAYER* pApp)
		{
			if (pApp->size != sizeof(GuildCommand_Struct))
			{
				cout << "Wrong size: OP_GuildInviteAccept, size=" << pApp->size << ", expected " << sizeof(GuildCommand_Struct) << endl;
				return;
			}

			GuildCommand_Struct* gc = (GuildCommand_Struct*) pApp->pBuffer;
			GuildInviteAccept_Struct* gia = (GuildInviteAccept_Struct*) pApp->pBuffer;

			Client* Inviter = entity_list.GetClientByName(gia->inviter);

			Client* Invitee = entity_list.GetClientByName(gia->newmember);


			int32 tmpeq = Invitee->PendingGuildInvite;

			GUILDRANK RankInvite = (GUILDRANK)gia->rank;

			this->LoadGuilds();

			
			switch(RankInvite)
			{
			case GuildLeader:
			case GuildOffice:
			case GuildMember:
				{
					if (Invitee->SetGuild(this->GuildList[tmpeq].databaseID, gia->rank))
					{
						worldserver.SendEmoteMessage(0, this->GuildList[tmpeq].databaseID, MESSAGETYPE_Guild, "%s has joined the guild. Rank: %s.", Invitee->GetName(), this->GuildList[tmpeq].rank[gia->rank].rankname);
					}
					else 
					{
						worldserver.SendEmoteMessage(gia->inviter, 0, 0, "Guild invite accepted, but failed.");
						Invitee->Message(BLACK, "Guild invite accepted, but failed.");
					}
					break;
				}
			case GuildDeclined:
				{
					worldserver.SendEmoteMessage(gc->Inviter, 0, 0, "%s has declined to join the guild.", Invitee->GetName());
					break;
				}
			case GuildInviteTimeOut:
				{
					worldserver.SendEmoteMessage(gc->Inviter, 0, 0, "%s's guild invite window timed out.", Invitee->GetName());
					break;
				}
			case NotInaGuild:
			default:
				worldserver.SendEmoteMessage(gc->Inviter, 0, 0, "Unknown response from %s to guild invite.", Invitee->GetName());
				break;
			}

			Invitee->SetGuild(this->GuildList[tmpeq].databaseID, gia->rank);
			//Invitee->SendAppearancePacket(Invitee->GetID(), SAT_Guild_ID, tmpeq, true);

			SendGuildRefreshPacket(tmpeq);
			Invitee->PendingGuildInvite = 0;
		}






		
		
		void ZoneGuildManager::ProcessOP_GuildLeader(APPLAYER* pApp)
		{
			if (pApp->size <= 1)
				return;

			pApp->pBuffer[pApp->size-1] = 0;

			char* NewLeadersName = (char*)pApp->pBuffer;

			Client* NewGuildLeader = entity_list.GetClientByName(NewLeadersName);


			ServerPacket* pack = new ServerPacket(ServerOP_GuildLeader, sizeof(ServerGuildCommand_Struct));
			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, pack->size);
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;

			sgc->guilddbid = NewGuildLeader->GuildDBID();
			sgc->guildeqid = NewGuildLeader->GuildEQID();
			sgc->fromrank = (int8)GuildLeader;
			//sgc->fromaccountid = account_id;
			sgc->admin = NewGuildLeader->Admin();
			//strcpy(sgc->from, name);
			strcpy(sgc->target, NewLeadersName);


			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}


		
		
		
		// Process;'s the /guildremove command
		void ZoneGuildManager::ProcessOP_GuildRemove(APPLAYER* pApp)
		{
			// Check pApp
			if(pApp == 0)
			{
				return; // Dont even bother processing
			}

			// Check pApp size againt the Struct Size
			if (pApp->size != sizeof(GuildRemove_Struct)) 
			{
				cout << "Wrong size: OP_GuildRemove, size=" << pApp->size << ", expected " << sizeof(GuildRemove_Struct) << endl;
				return;
			}

			GuildRemove_Struct* gr = (GuildRemove_Struct*) pApp->pBuffer;

			// Get the Remover
			Client* Remover = entity_list.GetClientByName(gr->Remover);

			// Get the Removee
			Client* Removee = entity_list.GetClientByName(gr->Removee);

			// Get the Rank
			GUILDRANK Rank = (GUILDRANK)gr->rank;

			// Remove the Removee from the Remover's Guild
			this->RemovePCFromGuild(Remover,Removee);

			return;
		}



		
		
		void ZoneGuildManager::ProcessOP_GuildMOTD(APPLAYER* pApp)
		{
			int guilddbid = 0;
			int guildeqid = 0;

			char tmp[255];
			if (guilddbid == 0) 
			{
				return;
			}

			this->LoadGuilds();

			Guild_Struct* guilds = this->GetGuildList();


			/*if (guilds[guildeqid].rank[guildrank].motd && !(strlen((char*) &pApp->pBuffer[36]) == 0)) 
			{
			if (strcasecmp((char*) &pApp->pBuffer[36+strlen((char*) &pApp->pBuffer[0])], " - none") == 0)
			{
			strcpy(tmp, "");
			}
			else
			{
			strncpy(tmp, (char*) &pApp->pBuffer[36], sizeof(tmp)); // client includes the "name - "
			}

			if (!Database::Instance()->SetGuildMOTD(guilddbid, tmp))
			{
			//Message(BLACK, "Motd update failed.");

			}

			worldserver.SendEmoteMessage(0, guilddbid, MESSAGETYPE_Guild, "Guild MOTD: %s", tmp);
			return;
			}
			*/

			Database::Instance()->GetGuildMOTD(guilddbid, tmp);

			if (strlen(tmp) != 0)
			{
				//Message(GREEN, "Guild MOTD: %s", tmp);
			}
		}


	}
}