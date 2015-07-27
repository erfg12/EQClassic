// ***************************************************************
//  Zone Guild Manager   ·  date: 16/05/2007
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
		ZoneGuildManager::ZoneGuildManager()
		{
			this->InitZoneDatabase();
			this->TotalGuids = 0; // Init Value
			this->LoadGuilds(); // Init the Guilds List by Zero'ing it out and setting some defaults
		}

		ZoneGuildManager::~ZoneGuildManager()
		{
		}


		// Creates a Guild
		void ZoneGuildManager::CreateGuild(Client* Persondoingit, Client *leader, char* GuildName, int32 out_guilddbid)
		{
			// Check if we have a leader
			if (leader == 0)
			{
				Persondoingit->Message(BLACK, "Guild leader not found.");
				return;
			}

			// check if the leader is not already in a guild
			if(leader->GuildDBID() != 0)
			{
				Persondoingit->Message(BLACK, "%s is already in a guild.", leader->GetName());
				return;
			}

			// Create the Guild in the DB
			int32 EQGuildID = Database::Instance()->CreateGuild(GuildName, leader->AccountID());

			// set the outvalue
			out_guilddbid = EQGuildID;

			if (EQGuildID == 0xFFFFFFFF)
			{
				Persondoingit->Message(BLACK, "Guild creation failed.");
				return;
			}
			else
			{
				// Reload Guilds
				this->LoadGuilds();

				this->SendGuildRefreshPacket(EQGuildID);

				// Invite the user into the guild as a leader
				this->InviteToGuild(this->GuildList[EQGuildID].databaseID, EQGuildID, Persondoingit, leader, GuildLeader);

				// Guild Created
				Persondoingit->Message(BLACK, "Guild %s created.", GuildName);

				return;
			}

		}



		//--
		// Step 1 : Check if Guid Leader is valid
		/*
		if(leader == 0)
		{
		return false;
		}

		// Step 2: Check if Leader is already in a guild
		if(leader->GuildDBID() != 0)
		{
		// already in a guild;
		}

		// Step 2 : Check if Guild name is not already in use


		// Step 3: Create Guild

		// Step 4: Set Guild Leader (DB)

		// Step 5: Set Guild Leader (Packet)

		// Step 6: Save the Leaer
		leader->Save();

		// Step 6: Reload Guilds
		this->LoadGuilds();

		// Step 7: return true on success, false on faliure
		return false;
		*/
		//}




		bool ZoneGuildManager::AddPCToGuild(int32 GuidID, int32 Inviter, int32 Invitee, int8 rank)
		{
			// Step 1 : Check if Inviter is valid

			// Step 2: Check if Invitee is valid

			// Step 3 : Check if Guild is Valid

			// Step 4: Check if Inviter can invite invitee

			// Step 5: Add invitee to Guild (DB)

			// Step 6: Add invitee to Guild (Packet)

			// Step 7: return true on success, false on faliure
			return false;
		}

		bool ZoneGuildManager::RemovePCFromGuild(Client* Remover, Client* Removee)
		{
			// Step 1 : Check if Remover is valid
			if(Remover == 0)
			{
				return false;
			}

			// Step 2: Check if Removee is valid
			if(Removee == 0)
			{
				// No one to remove, assume self removal
			}

			// Step 3 : Check if Guild is Valid

			// Step 4: Check if Remover can remove Removeee

			// Step 5: Remove removee from Guild (DB)

			// Step 6: Remove removee from Guild (Packet)

			// Step 7: return true on success, false on faliure

			//--


			if (Remover->GuildDBID() == 0)
			{
				Remover->Message(BLACK, "You arent in a guild!");
				return false;
			}

			if(Removee == 0)
			{
				this->SendGuildRemovePacketToWorldServer(Remover, Remover);
				return true;
			}
			else
			{

				if(Removee->IsClient())
				{
					if ((!this->GuildList[Remover->GuildEQID()].rank[Remover->GuildRank()].remove) && !(Removee == Remover))					
					{
						Remover->Message(BLACK, "You dont have permission to remove.");
						return false;
					}
					else
					{
						this->SendGuildRemovePacketToWorldServer(Remover, Removee);

						Removee->SetGuild(Removee->GuildDBID(), GUILD_MAX_RANK);

						return true;
					}
				}
				else
				{
					Remover->Message(BLACK, "You must target someone or specify a character name.");
					return false;
				}
			}

		}

		bool ZoneGuildManager::Promote(int32 GuidID, int32 Promoter, int32 Promtee, int8 rank)
		{
			// Step 1 : Check if Promoter is valid

			// Step 2: Check if Prmotee is valid

			// Step 3 : Check if Guild is Valid

			// Step 4: Check if Prmoter can promote Promotee

			// Step 5: Promote Promotee (DB)

			// Step 6: Promote Promotee (Packet)

			// Step 7: return true on success, false on faliure
			return false;
		}

		bool ZoneGuildManager::Demote(int32 GuidID, int32 Demoter, int32 Demotee, int8 rank)
		{
			// Step 1 : Check if Demoter is valid

			// Step 2: Check if Demotee is valid

			// Step 3 : Check if Guild is Valid

			// Step 4: Check if Demote can demote Demotee

			// Step 5: Demote Demotee (DB)

			// Step 6: Demote Demotee (Packet)

			// Step 7: return true on success, false on faliure
			return false;
		}



		bool ZoneGuildManager::DeleteGuild(int32 GuildLeader, int32 GuildID)
		{

			// Step 1: Check if Guild Leader is Valid

			// Step 2: Make sure that the GuildLeader is the Guilds actual leader

			// Step 3: Remove all members from guild

			// Step 4: Send Guild Remove Packet to all online

			// Step 5: Remove Guild Leader from Guild

			// Step 6: Send Guild Remove Packet to Guild Leader

			// Step 7: Delete from Database

			// Step 8: return true on success, false on faliure
			return false;			
		}

		bool ZoneGuildManager::SetMOTD(int32 GuildID, int MOTDSetter, char* MOTD)
		{
			// Step 1: Check if MOTDSetter is valid

			// Step 2: Check if Guild is Valid

			// Step 3: Check if MOTDSetter has permisson to set the Guild MOTD

			// Step 4: Update MOTD

			// Step 5: Send Packet to Guild Members

			// Step 6: return true on success, false on faliure

			// Step 7: return true on success, false on faliure
			return false;
		}

		int ZoneGuildManager::CountGuilds()
		{
			return this->TotalGuids;
		}



		// Returns the list of Guilds and thier Ranks
		Guild_Struct* ZoneGuildManager::GetGuildList()
		{
			return this->GuildList;
		}


		// Inits the Zone Guild Managers Database class  -- Dark-Prince
		void ZoneGuildManager::InitZoneDatabase()
		{
			if(this->guild_database == 0)
			{
				// If its 0, create a new instance
				this->guild_database = new EQC::Common::GuildDatabase();
			}
		}


		// Loads the Guilds from the database into the List  -- Dark-Prince
		void ZoneGuildManager::LoadGuilds()
		{
			// Init Database
			this->InitZoneDatabase();

			// Load up the guilds from the database (also repopulates ranks)
			this->guild_database->LoadGuilds(&this->GuildList);
		}


		bool ZoneGuildManager::InviteToGuild(int32 guilddbid, int32 guildeqid, Client* Inviter, Client* Invitee, GUILDRANK rank)
		{
			// Step 1 : Check if Guild Exists

			// Step 2 : Check if Inviter Exists
			if(Inviter == 0)
			{
				return false;
			}

			// Step 3: Check if Inviteee Exists
			if(Invitee == 0)
			{
				return false;
			}

			// Step 4: Send World Packet
			//this->SendGuildInvitePacketToWorldServer(Inviter, Invitee, guilddbid, guildeqid, rank);

			// Step 5:Set the client is pending a invite to a guild
			Invitee->PendingGuildInvite = guildeqid;

			// Step 6: Send Packet to Client
			this->SendGuildInvitePacketToClient(Inviter, Invitee, guilddbid, guildeqid, rank);

			// Step 7: Save the User
			Invitee->Save();

			return true;
		}




		// Checks if the supplied rank is actually valid (Memeber, Officer or Leader)  -- Dark-Prince
		bool IsValidGuildRank(int8 in_rank)
		{
			bool bval = false;

			GUILDRANK r = (GUILDRANK)in_rank;

			switch(r)
			{
			case GuildMember:
			case GuildOffice:
			case GuildLeader:
				bval = true; // If its one of our valid ranks, set bval to true
				break;
			default:
				bval = false; // anything else, set it to false;
				break;
			}

			return bval; // return our value
		}


		//  Creates the GuildCreateRequest packet to send to zone (Challenge/Response authorization) -- Dark-Prince
		void ZoneGuildManager::CreateGuildRequest(Client* Persondoingit, char* leadername, char* GuildName)
		{
			Client* leader = entity_list.GetClientByName(leadername);

			// Check if we have a leader
			if (leader == 0)
			{
				Persondoingit->Message(BLACK, "Guild leader not found.");
				return;
			}

			// check if the leader is not already in a guild
			if(leader->GuildDBID() != 0)
			{
				Persondoingit->Message(BLACK, "%s is already in a guild.", leader->GetName());
				return;
			}

			SendGuildCreationRequest(leader->GetName(), GuildName);
		}
	}
}