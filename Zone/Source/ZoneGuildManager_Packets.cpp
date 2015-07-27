// ***************************************************************
//  Zone Guild Manager Packet  ·  date: 16/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// Handles the sendin of Guild Packets
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
		void ZoneGuildManager::SendGuildCreationRequest(char* guildleader, char* GuildName)
			{
				// Check if the zone is connected to the world or not

				Client* GuildLeader = entity_list.GetClientByName(guildleader);

				ServerPacket* pack = EQC::Common::CreateServerPacket(ServerOP_GuildCreateRequest, sizeof(GuildCreateReqest_Struct));
				GuildCreateReqest_Struct* gcrs = (GuildCreateReqest_Struct*) pack->pBuffer;
				strncpy(gcrs->LeaderCharName, guildleader, sizeof(gcrs->LeaderCharName ));
				strncpy(gcrs->GuildName, GuildName, sizeof(gcrs->GuildName));


				worldserver.SendPacket(pack);
				safe_delete(pack);//delete pack;
			}


		void ZoneGuildManager::SendGuildInvitePacketToWorldServer(Client* Inviter, Client* Invitee, int32 guilddbid, int32 guildeqid, GUILDRANK rank )
		{
			// Create a new ServerPacket
			ServerPacket* pack = EQC::Common::CreateServerPacket(ServerOP_GuildInvite, sizeof(ServerGuildCommand_Struct));

			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;

			sgc->guilddbid = guilddbid;					// database guild ID - same as guildeqid?
			sgc->guildeqid = guildeqid;					// everquest guild ID - Same as guilddbid?
			sgc->fromrank = rank;						// the rank of the player being invited (member, officer, leader)
			sgc->fromaccountid = Inviter->AccountID();	// the account of the player being invited
			sgc->admin = Inviter->Admin();				// is the invitee an admin?
			strcpy(sgc->from, Inviter->GetName());		// name of theplayer the invite is coming from
			strcpy(sgc->target, Invitee->GetName());	// target of the invite
			sgc->newrank = rank;

			// Send the packet to the world
			worldserver.SendPacket(pack);

			// clean up the packet
			safe_delete(pack);//delete pack;
		}



		void ZoneGuildManager::SendGuildInvitePacketToClient(Client* Inviter, Client* Invitee, int32 guilddbid, int32 guildeqid, GUILDRANK Rank)
		{
			APPLAYER* outapp = new APPLAYER(OP_GuildInvite, sizeof(GuildCommand_Struct));
			memset(outapp->pBuffer, 0, outapp->size);

			GuildCommand_Struct* gc = (GuildCommand_Struct*) outapp->pBuffer;

			gc->guildeqid = guildeqid;
			strcpy(gc->Inviter, Inviter->GetName());
			strcpy(gc->Invitee, Invitee->GetName());
			gc->rank = Rank;
			gc->unknown[0] = 0x0;
			gc->unknown[1] = 0x0;

			// Send out Packet
			Invitee->QueuePacket(outapp);

			// cleanup
			safe_delete(outapp);//delete outapp;
		}


		void ZoneGuildManager::SendGuildRemovePacketToWorldServer(Client* Remover, Client* Removee)
		{
			ServerPacket* pack = EQC::Common::CreateServerPacket(ServerOP_GuildRemove, sizeof(ServerGuildCommand_Struct));

			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;

			sgc->guilddbid = Remover->GuildDBID();			// database guild ID - same as guildeqid?
			sgc->guildeqid = Remover->GuildEQID();			// everquest guild ID - Same as guilddbid?
			sgc->fromrank = Remover->GuildRank();			// the rank of the player being removed
			sgc->fromaccountid = Remover->AccountID();		// the account of the player being removed
			sgc->admin = Remover->Admin();					// is the remover an admin?
			strcpy(sgc->from, Remover->GetName());			// name of the player the remove request is coming from
			strcpy(sgc->target, Removee->GetName());		// target of the invit

			// Send the packet to the world
			worldserver.SendPacket(pack);

			// clean up the packet
			safe_delete(pack);//delete pack;
		}

		// Dark-Prince 10/05/2008 - Code Consolidation
		void ZoneGuildManager::SendGuildRefreshPacket(int32 eqid)
		{
			// Check if the zone is connected to the world or not
			if (worldserver.Connected())
			{
				ServerPacket* pack = EQC::Common::CreateServerPacket(ServerOP_RefreshGuild, 5);
				memcpy(pack->pBuffer, &eqid, 4);

				/*
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_RefreshGuild;
				pack->size = 5;
				pack->pBuffer = new uchar[pack->size];
				memcpy(pack->pBuffer, &eqid, 4);
				*/

				// Send the packet to the world
				worldserver.SendPacket(pack);

				// clean up the packet
				safe_delete(pack);//delete pack;

			}
		}

		// Dark-Prince 10/05/2008 - Code Consolidation
		void ZoneGuildManager::SendGuildDemotePacket(char* target)
		{
			// Check if the zone is connected to the world or not

			Client* Demotee = entity_list.GetClientByName(target);

			ServerPacket* pack = EQC::Common::CreateServerPacket(ServerOP_GuildDemote, sizeof(ServerGuildCommand_Struct));

			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;

			sgc->guilddbid = Demotee->GuildDBID();			// database guild ID - same as guildeqid?
			sgc->guildeqid = Demotee->GuildEQID();			// everquest guild ID - Same as guilddbid?
			sgc->fromrank = GuildOffice;			// the rank of the player being invited (member, officer, leader)
			//sgc->fromaccountid = account_id;	// the account of the player being invited
			sgc->admin = Demotee->Admin();					// is the invitee an admin?
			//strcpy(sgc->from, name);			// name of theplayer the invite is coming from
			strcpy(sgc->target, target);		// target of the invit
			sgc->newrank = GuildMember;				// New rank of the target

			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;

		}
		void ZoneGuildManager::SendGuildPromotePacket(char* PromoterName, char* PromoteeName)
		{	
			Client* Promoter = entity_list.GetClientByName(PromoterName);
			Client* Promotee = entity_list.GetClientByName(PromoteeName);

			if((Promoter != 0) && (Promotee != 0))
			{
				ServerPacket* pack = EQC::Common::CreateServerPacket(ServerOP_GuildPromote, sizeof(ServerGuildPromoteCommand_Struct));

				ServerGuildPromoteCommand_Struct* sgc = (ServerGuildPromoteCommand_Struct*) pack->pBuffer;

				
				sgc->guilddbid = Promotee->GuildDBID();			// database guild ID - same as guildeqid?
				sgc->guildeqid = Promotee->GuildEQID();			// everquest guild ID - Same as guilddbid?
				sgc->PromoterRank = Promoter->GuildRank();		// the rank of the player being invited (member, officer, leader)
				sgc->PromoterAccountID = Promoter->AccountID();	// the account of the player being invited
				sgc->admin = Promotee->Admin();					// is the invitee an admin?
				strcpy(sgc->PromoterName, Promoter->GetName());	// name of theplayer the invite is coming from
				strcpy(sgc->Promotee, Promotee->GetName());					// target of the invit
				sgc->PromoteeRank = GuildOffice;				// New rank of the target

				worldserver.SendPacket(pack);
				safe_delete(pack);//delete pack;
			}

		}
	}
}