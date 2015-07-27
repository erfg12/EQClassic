// ***************************************************************
//  World Guild Manager   ·  date: 12/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// This class is for use when the world needs to handle guilds
// ***************************************************************

#include "WorldGuildManager.h"
#include "GuildDatabase.h"

EQC::World::WorldGuildManager guild_mgr;
EQC::Common::GuildDatabase gb;

namespace EQC
{
	namespace World
	{
		//Constructor
		WorldGuildManager::WorldGuildManager()
		{
		}

		// Destructor
		WorldGuildManager::~WorldGuildManager()
		{
		}
		

		// Handles the ServerOP_GuildCreateRequest (Challenge/Response autheorization)  -- Dark-Prince
		// ServerOP_GuildCreateRequest is sent by the zone when a client types : #guild dpcreate <charname> <guildname>
		// Upon reciving the ServerOP_GuildCreateRequest World :
		// 1. Validates the packet
		// 2. Creates a ServerOP_GuildCreateRequest from the packet
		// 3. Creates the Guild & Marks the requestor as the leader
		// 4. Sends a ServerOP_GuildCreateResponse back to the issuing zone, notifying them of the response
		void WorldGuildManager::ProcessServerOP_GuildCreateRequest(ServerPacket* pack)
		{
			// Check Packet Size
			if (pack->size != sizeof(GuildCreateReqest_Struct))
			{
				cout << "Wrong size on ServerOP_GuildCreateRequest. Got: " << pack->size << ", Expected: " << sizeof(GuildCreateReqest_Struct) << endl;
				return;
			}

			GuildCreateReqest_Struct* sgc = (GuildCreateReqest_Struct*) pack->pBuffer;

			ClientListEntry* Leader =  zoneserver_list.FindCharacter(sgc->LeaderCharName);

			if(Leader == 0)
			{
				// Cant find leader... bail out
				//TODO: Notify the client
				return;
			}

			int32 EQGuildID = 0;
			
			// Create the Guild
			EQGuildID = CreateGuild(0, sgc->LeaderCharName, sgc->GuildName);
			
			if(EQGuildID == 0xFFFFFFFF)
			{
				// Guild creation failed, notify client
				//TODO: Notify the client
				return;
			}
			else
			{
				// Guild Creation was a success, notify client

				// Create a ServerPacker of ServerOP_GuildCreateResponse
				ServerPacket* pack = EQC::Common::CreateServerPacket(ServerOP_GuildCreateResponse, sizeof(GuildCreateResponse_Struct));

				// Map ServerPacker to a GuildCreateResponse_Struct
				GuildCreateResponse_Struct* gcrs = (GuildCreateResponse_Struct*) pack->pBuffer;

				// Populate GuildCreateResponse_Struct
				gcrs->Created = true; // Yes, it is created
					
				strncpy(gcrs->LeaderCharName, sgc->LeaderCharName, sizeof(gcrs->LeaderCharName)); // Set the Leader of the Guild
				
				strncpy(gcrs->GuildName, sgc->GuildName, sizeof(gcrs->GuildName)); // Set the Guild Name
					
				gcrs->eqid = EQGuildID; // set the EQ Guild ID
				Leader->Server()->SendPacket(pack); // send packet to issuing zone sever 
				
				// Reload Guilds Cache
				this->LoadGuilds();
			}
		}

		// Handles Guild Creation by World  -- Dark-Prince
		// Steps performed:
		// 1. Create a ClientListEntry for the proposed Leader
		// 2. Validate purposed Leader (i.e actually exists)
		// 3. Make sure purposed leader is not already in a exsisting guild
		// 4. Creates the relevent database records
		int32 WorldGuildManager::CreateGuild(Client* Persondoingit, char* LeaderCharName, char* GuildName)
		{	
			// Look up purposed leader's ClientListEntry object
			ClientListEntry* cle =  zoneserver_list.FindCharacter(LeaderCharName);
			
			// Check if we have a leader
			if (cle == 0)
			{
	//			Persondoingit->Message(BLACK, "Guild leader not found.");
				return 0xFFFFFFFF;
			}

			// check if the leader is not already in a guild
			if(cle->GuildDBID() != 0)
			{
	//			Persondoingit->Message(BLACK, "%s is already in a guild.", cle->GetName());
				return 0xFFFFFFFF;
			}

			// Create the Guild in the DB
			int32 out_GuildEQID = Database::Instance()->CreateGuild(GuildName, cle->AccountID()); // needs to be charid

			return out_GuildEQID;			
		}


		// Handles the ProcessServerOP_RefreshGuild  -- Dark-Prince
		// 1. Validate Packet Size
		// 2. Reload World's cache of the Guilds
		//TODO: 3. Extract EQ Guild ID from ServerPacket (Usless as werelaod all guilds)
		//TODO: 4. Copy EQ Guild ID to ServerPacket (Usless as werelaod all guilds)
		//TODO: 5. Send ServerPacket to all zones (Usless as werelaod all guilds)
		void WorldGuildManager::ProcessServerOP_RefreshGuild(ServerPacket* pack)
		{
			if (pack->size == 5) 
			{
				LoadGuilds();
		
				int32 guildeqid = 0xFFFFFFFF;
				memcpy(&guildeqid, pack->pBuffer, 4);
				
				zoneserver_list.SendPacket(pack);
			}
			else
			{
				cout << "Wrong size: ServerOP_RefreshGuild. size=" << pack->size << endl;
			}
		}

		void WorldGuildManager::ProcessServerOP_Guilds(ServerPacket* pack)
		{
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			ClientListEntry* cle = zoneserver_list.FindCharacter(sgc->target);

			if (cle == 0)
			{
				zoneserver_list.SendEmoteMessage(sgc->from, 0, 0, "%s is not online.", sgc->target);
			}
			else if (cle->Server() == 0)
			{
				zoneserver_list.SendEmoteMessage(sgc->from, 0, 0, "%s is not online.", sgc->target);
			}
			else if ((cle->Admin() >= 100 && sgc->admin < 100) && cle->GuildDBID() != sgc->guilddbid) // no peeping for GMs, make sure tell message stays the same
			{
				zoneserver_list.SendEmoteMessage(sgc->from, 0, 0, "%s is not online.", sgc->target);
			}
			else
			{
				cle->Server()->SendPacket(pack);
			}
		}

		// Creates a empty GuildsListEntry_Struct struct
		GuildsListEntry_Struct WorldGuildManager::CreateBlankGuildsListEntry_Struct()
		{
			GuildsListEntry_Struct g;
			g.guildID = 0xFFFFFFFF;
			g.unknown1[0] = 0xFF;
			g.unknown1[1] = 0xFF;
			g.unknown1[2] = 0xFF;
			g.unknown1[3] = 0xFF;
			g.exists = 0;
			g.unknown3[0] = 0xFF;
			g.unknown3[1] = 0xFF;
			g.unknown3[2] = 0xFF;
			g.unknown3[3] = 0xFF;

			return g; // return the empty struct
		}

		// Quagmire - tring to send list of guilds
		void WorldGuildManager::SendGuildsList(Client* c)
		{
			// create the buffer and struct
			uchar* pBuffer = new uchar[sizeof(GuildsList_Struct)];
			GuildsList_Struct* gl = (GuildsList_Struct*)pBuffer;

			// Relaod the guilds from the database
			LoadGuilds();

			// loop though and zero out a total of MAX_GUILDS guild structs
			for (int GuildCount = 0; GuildCount < MAX_GUILDS; GuildCount++)
			{
				gl->Guilds[GuildCount] = guild_mgr.CreateBlankGuildsListEntry_Struct();

				// check if the database id is not null
				if (guild_mgr.guilds[GuildCount].databaseID != 0) 
				{
					// if its not null..
					gl->Guilds[GuildCount].guildID = GuildCount; // set the guild ID
					strncpy(gl->Guilds[GuildCount].name, guild_mgr.guilds[GuildCount].name, sizeof(gl->Guilds[GuildCount].name)); // set the guild name
					gl->Guilds[GuildCount].exists = 1; // set that it does indeed exsit
				}
			}

			// Send the Guild list to the network so it can be send to the client - Dark-Prince - 11/05/2008 - Whty does the client need to know about all guilds????
			c->network->SendGuildsList(gl);

		}


		// Populates the Guild Managers Guild List (this->guilds) with a copy from the database each time when called. -- Dark-Prince
		void WorldGuildManager::LoadGuilds()
		{
			// Call the LoadGuilds method from the Instanceof GuildDatabase Class
			gb.LoadGuilds(&this->guilds);
		}

		// Provides a public method to access this->guilds  -- Dark-Prince
		//TODO: Make this read only.
		Guild_Struct* WorldGuildManager::GetGuildList()
		{
			return this->guilds;
		}
	}
}