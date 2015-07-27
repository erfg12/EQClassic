/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "../common/debug.h"
#include <iostream.h>
#include <string.h>
#include <stdio.h>
#include <iomanip.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef WIN32
	#include <process.h>
	#include <windows.h>

	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp	_stricmp
#endif

#include "../common/servertalk.h"
#include "worldserver.h"
#include "../common/eq_packet_structs.h"
#include "../common/packet_dump.h"
#include "../common/database.h"
#include "mob.h"
#include "zone.h"
#include "client.h"
#include "entity.h"
#include "net.h"
#include "petitions.h"
#include "../common/packet_functions.h"
#include "PlayerCorpse.h"
#include "../common/md5.h"

extern Database database;
extern EntityList    entity_list;
extern Zone* zone;
extern volatile bool ZoneLoaded;
extern void CatchSignal(int);
extern WorldServer worldserver;
extern NetConnection net;
extern PetitionList petition_list;
extern GuildRanks_Struct guilds[512];
extern int32 numclients;
extern volatile bool RunLoops;

WorldServer::WorldServer() {
	tcpc = new TCPConnection();
	pTryReconnect = true;
	pConnected = false;
}

WorldServer::~WorldServer() {
	safe_delete(tcpc);
}

void WorldServer::SetZone(int32 iZoneID) {
	ServerPacket* pack = new ServerPacket(ServerOP_SetZone, sizeof(SetZone_Struct));
	SetZone_Struct* szs = (SetZone_Struct*) pack->pBuffer;
	szs->zoneid = iZoneID;
	if (zone) {
		szs->staticzone = zone->IsStaticZone();
	}
	SendPacket(pack);
	delete pack;
}

void WorldServer::SetConnectInfo() {
	ServerPacket* pack = new ServerPacket(ServerOP_SetConnectInfo, sizeof(ServerConnectInfo));
	ServerConnectInfo* sci = (ServerConnectInfo*) pack->pBuffer;
	sci->port = net.GetZonePort();
	strcpy(sci->address, net.GetZoneAddress());
	SendPacket(pack);
	delete pack;
}

bool WorldServer::SendPacket(ServerPacket* pack) {
	if (!Connected())
		return false;
	return tcpc->SendPacket(pack);
}

void WorldServer::Process() {
	if (this == 0)
		return;
	if (!Connected()) {
		pConnected = tcpc->Connected();
		if (pConnected) {
			cout << "Connected to worldserver: " << net.GetWorldAddress() << ":" << PORT << endl;
			char tmp[100];
			if (database.GetVariable("ZSPassword", tmp, sizeof(tmp))) {
				ServerPacket* pack = new ServerPacket(ServerOP_ZAAuth, 16);
				MD5::Generate((uchar*) tmp, strlen(tmp), pack->pBuffer);
				SendPacket(pack);
				delete pack;
			}
			this->SetConnectInfo();
			if (ZoneLoaded) {
				this->SetZone(zone->GetZoneID());
				entity_list.UpdateWho(true);
				this->SendEmoteMessage(0, 0, 15, "Zone connect: %s", zone->GetLongName());
			}
		}
		else
			return;
	}

	ServerPacket *pack = 0;
	while((pack = tcpc->PopPacket())) {
		adverrornum = pack->opcode;
		switch(pack->opcode) {
		case 0: {
			break;
		}
		case ServerOP_KeepAlive: {
			// ignore this
			break;
		}
		case ServerOP_ZAAuthFailed: {
			cout << "World server responded 'Not Authorized', disabling reconnect" << endl;
			pTryReconnect = false;
			Disconnect();
			break;
		}
		case ServerOP_ChannelMessage: {
			if (!ZoneLoaded) break;
			ServerChannelMessage_Struct* scm = (ServerChannelMessage_Struct*) pack->pBuffer;
			if (scm->deliverto[0] == 0) {
				entity_list.ChannelMessageFromWorld(scm->from, scm->to, scm->chan_num, scm->guilddbid, scm->language, scm->message);
			}
			else {
				Client* client;
				client = entity_list.GetClientByName(scm->deliverto);
				if (client != 0) {
					if (client->Connected()) {
						client->ChannelMessageSend(scm->from, scm->to, scm->chan_num, scm->language, scm->message);
						if (!scm->noreply) {
							// if it's a tell, echo back so it shows up
							scm->noreply = true;
							scm->chan_num = 14;
							memset(scm->deliverto, 0, sizeof(scm->deliverto));
							strcpy(scm->deliverto, scm->from);
							SendPacket(pack);
						}
					}
				}
			}
			break;
		}
		case ServerOP_AcceptWorldEntrance: {
			if(pack->size != sizeof(WorldToZone_Struct))
				break;
			if (!ZoneLoaded)
				break;
			WorldToZone_Struct* wtz = (WorldToZone_Struct*) pack->pBuffer;

			if(zone->GetMaxClients() != 0 && numclients >= zone->GetMaxClients())
				wtz->response = -1;
			else
				wtz->response = 1;

			worldserver.SendPacket(pack);
			break;		
		}
		case ServerOP_ZoneToZoneRequest: {
			if(pack->size != sizeof(ZoneToZone_Struct))
				break;
			if (!ZoneLoaded)
				break;
			ZoneToZone_Struct* ztz = (ZoneToZone_Struct*) pack->pBuffer;
			
			if(ztz->current_zone_id == zone->GetZoneID()) {
				// it's a response
				Entity* entity = entity_list.GetClientByName(ztz->name);
				if(entity == 0)
					break;

				APPLAYER *outapp;
				outapp = new APPLAYER(OP_ZoneChange,sizeof(ZoneChange_Struct));
				ZoneChange_Struct* zc2=(ZoneChange_Struct*)outapp->pBuffer;

				if(ztz->response == 0 || ztz->response == -1) {
					zc2->success = ZONE_ERROR_NOTREADY;
					entity->CastToMob()->SetZone(ztz->current_zone_id);
				}
				else {
					strncpy(zc2->char_name,entity->CastToMob()->GetName(),64);
					zc2->zoneID=ztz->requested_zone_id;
					zc2->success = 1;
				}
				entity->CastToClient()->QueuePacket(outapp);
				delete outapp;

				switch(ztz->response)
				{
				case -1: {
					entity->CastToMob()->Message(13,"The zone is currently full, please try again later.");
					break;
				}
				case 0:	{
					entity->CastToMob()->Message(13,"All zone servers are taken at this time, please try again later.");
					break;
				}
				}
			}
			else {
				// it's a request
				if(zone->GetMaxClients() != 0 && numclients >= zone->GetMaxClients())
					ztz->response = -1;
				else {
					ztz->response = 1;
					// since they asked about comming, lets assume they are on their way and not shut down.
					zone->StartShutdownTimer(AUTHENTICATION_TIMEOUT * 1000);
				}

				SendPacket(pack);
				break;
			}
			break;
		}
		case ServerOP_EmoteMessage: {
			if (!ZoneLoaded)
				break;
			ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;
			if (sem->to[0] != 0) {
				if (strcasecmp(sem->to, zone->GetShortName()) == 0)
					entity_list.MessageStatus(sem->guilddbid, sem->minstatus, sem->type, (char*)sem->message);
				else {
					Client* client = entity_list.GetClientByName(sem->to);
					if (client != 0)
						client->Message(sem->type, (char*)sem->message);
				}
			}
			else
				entity_list.MessageStatus(sem->guilddbid, sem->minstatus, sem->type, (char*)sem->message);
			break;
		}
		case ServerOP_ShutdownAll: {
			entity_list.Save();
			CatchSignal(2);
			break;
		}
		case ServerOP_ReloadAlliances: {
#ifdef GUILDWARS
		database.LoadGuildAlliances();
#endif
		break;
		}
		case ServerOP_ReloadOwnedCities: {
#ifdef GUILDWARS
		database.LoadCityControl();
#endif
		break;
										 }
		case ServerOP_ZoneShutdown: {
			if (pack->size != sizeof(ServerZoneStateChange_struct)) {
				cout << "Wrong size on ServerOP_ZoneShutdown. Got: " << pack->size << ", Expected: " << sizeof(ServerZoneStateChange_struct) << endl;
				break;
			}
			// Annouce the change to the world
			if (!ZoneLoaded) {
				SetZone(0);
			}
			else {
				worldserver.SendEmoteMessage(0, 0, 15, "Zone shutdown: %s", zone->GetLongName());
				
				ServerZoneStateChange_struct* zst = (ServerZoneStateChange_struct *) pack->pBuffer;
				cout << "Zone shutdown by " << zst->adminname << endl;
				Zone::Shutdown();
			}
			break;
		}
		case ServerOP_ZoneBootup: {
			if (pack->size != sizeof(ServerZoneStateChange_struct)) {
				cout << "Wrong size on ServerOP_ZoneBootup. Got: " << pack->size << ", Expected: " << sizeof(ServerZoneStateChange_struct) << endl;
				break;
			}
			ServerZoneStateChange_struct* zst = (ServerZoneStateChange_struct *) pack->pBuffer;
			if (ZoneLoaded) {
				SetZone(zone->GetZoneID());
				if (zst->zoneid == zone->GetZoneID()) {
					// This packet also doubles as "incomming client" notification, lets not shut down before they get here
					zone->StartShutdownTimer(AUTHENTICATION_TIMEOUT * 1000);
				}
				else {
					worldserver.SendEmoteMessage(zst->adminname, 0, 0, "Zone bootup failed: Already running '%s'", zone->GetShortName());
				}
				break;
			}
			
			if (zst->adminname[0] != 0)
				cout << "Zone bootup by " << zst->adminname << endl;
			
			if (!(Zone::Bootup(zst->zoneid, zst->makestatic))) {
				worldserver.SendChannelMessage(0, 0, 10, 0, 0, "%s:%i Zone::Bootup failed: %s", net.GetZoneAddress(), net.GetZonePort(), database.GetZoneName(zst->zoneid));
			}
			// Moved annoucement to ZoneBootup() - Quagmire
			//			else
			//				worldserver.SendEmoteMessage(0, 0, 15, "Zone bootup: %s", zone->GetLongName());
			break;
		}
		case ServerOP_ZoneIncClient: {
			if (pack->size != sizeof(ServerZoneIncommingClient_Struct)) {
				cout << "Wrong size on ServerOP_ZoneIncClient. Got: " << pack->size << ", Expected: " << sizeof(ServerZoneIncommingClient_Struct) << endl;
				break;
			}
			ServerZoneIncommingClient_Struct* szic = (ServerZoneIncommingClient_Struct*) pack->pBuffer;
			if (ZoneLoaded) {
				SetZone(zone->GetZoneID());
				if (szic->zoneid == zone->GetZoneID()) {
					zone->AddAuth(szic);
					// This packet also doubles as "incomming client" notification, lets not shut down before they get here
					zone->StartShutdownTimer(AUTHENTICATION_TIMEOUT * 1000);
				}
			}
			else {
				if ((Zone::Bootup(szic->zoneid))) {
					zone->AddAuth(szic);
				}
				else
					SendEmoteMessage(0, 0, 100, 0, "%s:%i Zone::Bootup failed: %s (%i)", net.GetZoneAddress(), net.GetZonePort(), database.GetZoneName(szic->zoneid, true), szic->zoneid);
			}
			break;
		}
		case ServerOP_ZonePlayer: {
			ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(szp->name);
			if (client != 0) {
				if (strcasecmp(szp->adminname, szp->name) == 0)
					client->Message(0, "Zoning to: %s", szp->zone);
				else if (client->GetAnon() == 1 && client->Admin() > szp->adminrank)
					break;
				else {
					worldserver.SendEmoteMessage(szp->adminname, 0, 0, "Summoning %s to %s %1.1f, %1.1f, %1.1f", szp->name, szp->zone, szp->x_pos, szp->y_pos, szp->z_pos);
				}
				client->MovePC(szp->zone, szp->x_pos, szp->y_pos, szp->z_pos, szp->ignorerestrictions, true);
			}
			break;
		}
		case ServerOP_KickPlayer: {
			ServerKickPlayer_Struct* skp = (ServerKickPlayer_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(skp->name);
			if (client != 0) {
				if (skp->adminrank >= client->Admin()) {
					client->WorldKick();
					if (ZoneLoaded)
						worldserver.SendEmoteMessage(skp->adminname, 0, 0, "Remote Kick: %s booted in zone %s.", skp->name, zone->GetShortName());
					else
						worldserver.SendEmoteMessage(skp->adminname, 0, 0, "Remote Kick: %s booted.", skp->name);
				}
				else if (client->GetAnon() != 1)
					worldserver.SendEmoteMessage(skp->adminname, 0, 0, "Remote Kick: Your avatar level is not high enough to kick %s", skp->name);
			}
			break;
		}
		case ServerOP_KillPlayer: {
			ServerKillPlayer_Struct* skp = (ServerKillPlayer_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(skp->target);
			if (client != 0) {
				if (skp->admin >= client->Admin()) {
					client->GMKill();
					if (ZoneLoaded)
						worldserver.SendEmoteMessage(skp->gmname, 0, 0, "Remote Kill: %s killed in zone %s.", skp->target, zone->GetShortName());
					else
						worldserver.SendEmoteMessage(skp->gmname, 0, 0, "Remote Kill: %s killed.", skp->target);
				}
				else if (client->GetAnon() != 1)
					worldserver.SendEmoteMessage(skp->gmname, 0, 0, "Remote Kill: Your avatar level is not high enough to kill %s", skp->target);
			}
			break;
		}
		case ServerOP_RefreshGuild: {
			if (pack->size == 5) {
				int32 guildeqid = 0;
				memcpy(&guildeqid, pack->pBuffer, 4);
				database.GetGuildRanks(guildeqid, &guilds[guildeqid]);
				if (pack->pBuffer[4] == 1) {
					APPLAYER* outapp = new APPLAYER(OP_GuildUpdate, sizeof(GuildUpdate_Struct));
					GuildUpdate_Struct* gu = (GuildUpdate_Struct*) outapp->pBuffer;
					gu->guildID = guildeqid;
					gu->entry.guildID = guildeqid;
					gu->entry.guildIDx = guildeqid;
					gu->entry.unknown6[0] = 0xFF;
					gu->entry.unknown6[1] = 0xFF;
					gu->entry.unknown6[2] = 0xFF;
					gu->entry.unknown6[3] = 0xFF;
					gu->entry.exists = 0;
					gu->entry.unknown10[0] = 0xFF;
					gu->entry.unknown10[1] = 0xFF;
					gu->entry.unknown10[2] = 0xFF;
					gu->entry.unknown10[3] = 0xFF;
					if (guilds[guildeqid].databaseID == 0) {
						gu->entry.exists = 0; // = 0x01 if exists, 0x00 on empty
					}
					else {
						gu->entry.unknown4[1] = 0x75;
						gu->entry.unknown4[2] = 0x5B;
						gu->entry.unknown4[3] = 0xF6;
						gu->entry.unknown4[4] = 0x77;
						gu->entry.unknown4[5] = 0x5C;
						gu->entry.unknown4[6] = 0xEC;
						gu->entry.unknown4[7] = 0x12;
						gu->entry.unknown4[9] = 0xD4;
						gu->entry.unknown4[10] = 0x2C;
						gu->entry.unknown4[11] = 0xF9;
						gu->entry.unknown4[12] = 0x77;
						gu->entry.unknown4[13] = 0x90;
						gu->entry.unknown4[14] = 0xD7;
						gu->entry.unknown4[15] = 0xF9;
						gu->entry.unknown4[16] = 0x77;
						gu->entry.regguild[1] = 0xFF;
						gu->entry.regguild[2] = 0xFF;
						gu->entry.regguild[3] = 0xFF;
						gu->entry.regguild[4] = 0xFF;
						gu->entry.regguild[5] = 0x6C;
						gu->entry.regguild[6] = 0xEC;
						gu->entry.regguild[7] = 0x12;

						strcpy(gu->entry.name, guilds[guildeqid].name);
						gu->entry.exists = 1; // = 0x01 if exists, 0x00 on empty
					}
					
					entity_list.QueueClients(0, outapp, false);
					delete outapp;
				}
			}
			else
				cout << "Wrong size: ServerOP_RefreshGuild. size=" << pack->size << endl;
			break;
		}
		case ServerOP_GuildLeader: {
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(sgc->target);
			
			if (client == 0) {
				// do nothing
			}
			else if (client->GuildDBID() != sgc->guilddbid)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not in your guild.", client->GetName());
			else if (client->GuildRank() != 0)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not rank 0.", client->GetName());
			else {
				if (database.SetGuildLeader(sgc->guilddbid, client->AccountID())) {
					worldserver.SendEmoteMessage(0, sgc->guilddbid, MT_Guild, "%s is now the leader of your guild.", client->GetName());
					ServerPacket* pack2 = new ServerPacket;
					pack2->opcode = ServerOP_RefreshGuild;
					pack2->size = 4;
					pack2->pBuffer = new uchar[pack->size];
					memcpy(pack2->pBuffer, &sgc->guildeqid, 4);
					worldserver.SendPacket(pack2);
					delete pack2;
				}
				else
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "Guild leadership transfer failed.");
			}
			break;
		}
		case ServerOP_GuildInvite: {
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(sgc->target);
			
			if (client == 0) {
				// do nothing
			}
			else if (!guilds[sgc->guildeqid].rank[sgc->fromrank].invite)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "You dont have permission to invite.");
			else if (client->GuildDBID() != 0)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is already in another guild.", client->GetName());
			else if (client->PendingGuildInvite != 0 && !(client->PendingGuildInvite == sgc->guilddbid))
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s has another pending guild invite.", client->GetName());
			else {
				client->PendingGuildInvite = sgc->guilddbid;
				APPLAYER* outapp = new APPLAYER;
				outapp->opcode = OP_GuildInvite;
				outapp->size = sizeof(GuildCommand_Struct);
				outapp->pBuffer = new uchar[outapp->size];
				memset(outapp->pBuffer, 0, outapp->size);
				GuildCommand_Struct* gc = (GuildCommand_Struct*) outapp->pBuffer;
				gc->guildeqid = sgc->guildeqid;
				strcpy(gc->othername, sgc->target);
				strcpy(gc->myname, sgc->from);
				client->QueuePacket(outapp);
				/*				
				if (client->SetGuild(sgc->guilddbid, GUILD_MAX_RANK))
				worldserver.SendEmoteMessage(0, sgc->guilddbid, MT_Guild, "%s has joined the guild. Rank: %s.", client->GetName(), guilds[sgc->guildeqid].rank[GUILD_MAX_RANK].rankname);
				else
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "Guild invite failed.");
				*/
			}
			break;
		}
		case ServerOP_GuildRemove: {
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(sgc->target);
			
			if (client == 0) {
				// do nothing
			}
			else if ((!guilds[sgc->guildeqid].rank[sgc->fromrank].remove) && !(strcasecmp(sgc->from, sgc->target) == 0))
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "You dont have permission to remove.");
			else if (client->GuildDBID() != sgc->guilddbid)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not in your guild.", client->GetName());
			else if (client->GuildRank() <= sgc->fromrank && !(sgc->fromaccountid == guilds[sgc->guildeqid].leader) && !(strcasecmp(sgc->from, sgc->target) == 0))
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s's rank is too high for you to remove them.", client->GetName());
			else {
				if (client->SetGuild(0, GUILD_MAX_RANK)) {
					if (strcasecmp(sgc->from, sgc->target) == 0)
						worldserver.SendEmoteMessage(0, sgc->guilddbid, MT_Guild, "%s has left the guild.", client->GetName());
					else {
						worldserver.SendEmoteMessage(0, sgc->guilddbid, MT_Guild, "%s has been removed from the guild by %s.", client->GetName(), sgc->from);
						client->Message(MT_Guild, "You have been removed from the guild by %s.", sgc->from);
					}
				}
				else
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "Guild remove failed.");
			}
			break;
		}
		case ServerOP_GuildPromote: {
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(sgc->target);
			
			if (client == 0) {
				// do nothing
			}
			else if (client->GuildDBID() != sgc->guilddbid)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not in your guild.", client->GetName());
			else if ((!guilds[sgc->guildeqid].rank[sgc->fromrank].promote) && !(strcasecmp(sgc->from, sgc->target) == 0))
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "You dont have permission to promote.");
			else if (client->GuildDBID() != sgc->guilddbid)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s isnt in your guild.", client->GetName());
			else if (client->GuildRank() <= sgc->newrank)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is already rank %i.", client->GetName(), client->GuildRank());
			else if (sgc->newrank <= sgc->fromrank && sgc->fromaccountid != guilds[sgc->guildeqid].leader)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "You cannot promote people to a greater or equal rank than yourself.");
			else {
				if (client->SetGuild(sgc->guilddbid, sgc->newrank)) {
					worldserver.SendEmoteMessage(0, sgc->guilddbid, MT_Guild, "%s has been promoted to %s by %s.", client->GetName(), guilds[sgc->guildeqid].rank[sgc->newrank].rankname, sgc->from);
				}
				else
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "Guild promote failed");
			}
			break;
		}
		case ServerOP_GuildDemote: {
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(sgc->target);
			
			if (client == 0) {
				// do nothing
			}
			else if (client->GuildDBID() != sgc->guilddbid)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not in your guild.", client->GetName());
			else if (!guilds[sgc->guildeqid].rank[sgc->fromrank].demote)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "You dont have permission to demote.");
			else if (client->GuildRank() >= sgc->newrank)
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is already rank %i.", client->GetName(), client->GuildRank());
			else if (sgc->newrank <= sgc->fromrank && sgc->fromaccountid != guilds[sgc->guildeqid].leader && !(strcasecmp(sgc->from, sgc->target) == 0))
				worldserver.SendEmoteMessage(sgc->from, 0, 0, "You cannot demote people with a greater or equal rank than yourself.");
			else {
				if (client->SetGuild(sgc->guilddbid, sgc->newrank)) {
					worldserver.SendEmoteMessage(0, sgc->guilddbid, MT_Guild, "%s has been demoted to %s by %s.", client->GetName(), guilds[sgc->guildeqid].rank[sgc->newrank].rankname, sgc->from);
				}
				else
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "Guild demote failed");
			}
			break;
		}
		case ServerOP_GuildGMSet: {
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(sgc->target);
			if (client != 0) {
				if (client->GuildDBID() == 0 || sgc->guilddbid == 0) {
					int32 tmpeq = database.GetGuildEQID(sgc->guilddbid);
					if (tmpeq != 0xFFFFFFFF && guilds[tmpeq].minstatus > sgc->admin && sgc->admin < 250) {
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Access denied.");
					}
					else if (!client->SetGuild(sgc->guilddbid, GUILD_MAX_RANK))
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Error: Guild #%i not found", sgc->guilddbid);
				}
				else
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "Error: %s is already in a guild", sgc->target);
			}
			break;
		}
		case ServerOP_GuildGMSetRank: {
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(sgc->target);
			if (client != 0) {
				if (client->GuildDBID() != 0) {
					int32 tmpeq = database.GetGuildEQID(sgc->guilddbid);
					if (guilds[tmpeq].minstatus > sgc->admin && sgc->admin < 250) {
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Access denied.");
					}
					else if (!client->SetGuild(client->GuildDBID(), sgc->newrank))
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Error: SetRank failed.", sgc->guilddbid);
				}
				else
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "Error: %s is not in a guild", sgc->target);
			}
			break;
		}
		case ServerOP_FlagUpdate: {
			Client* client = entity_list.GetClientByAccID(*((int32*) pack->pBuffer));
			if (client != 0) {
				client->UpdateAdmin();
			}
			break;
		}
		case ServerOP_GMGoto: {
			if (pack->size != sizeof(ServerGMGoto_Struct)) {
				cout << "Wrong size on ServerOP_GMGoto. Got: " << pack->size << ", Expected: " << sizeof(ServerGMGoto_Struct) << endl;
				break;
			}
			if (!ZoneLoaded)
				break;
			ServerGMGoto_Struct* gmg = (ServerGMGoto_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(gmg->gotoname);
			if (client != 0) {
				worldserver.SendEmoteMessage(gmg->myname, 0, 13, "Summoning you to: %s @ %s, %1.1f, %1.1f, %1.1f", client->GetName(), zone->GetShortName(), client->GetX(), client->GetY(), client->GetZ());
				ServerPacket* outpack = new ServerPacket(ServerOP_ZonePlayer, sizeof(ServerZonePlayer_Struct));
				ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) outpack->pBuffer;
				strcpy(szp->adminname, gmg->myname);
				strcpy(szp->name, gmg->myname);
				strcpy(szp->zone, zone->GetShortName());
				szp->x_pos = client->GetX();
				szp->y_pos = client->GetY();
				szp->z_pos = client->GetZ();
				worldserver.SendPacket(outpack);
				delete outpack;
			}
			else {
				worldserver.SendEmoteMessage(gmg->myname, 0, 13, "Error: %s not found", gmg->gotoname);
			}
			break;
		}
		case ServerOP_MultiLineMsg: {
			ServerMultiLineMsg_Struct* mlm = (ServerMultiLineMsg_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByName(mlm->to);
			if (client) {
				APPLAYER* outapp = new APPLAYER(OP_MultiLineMsg, strlen(mlm->message));
				strcpy((char*) outapp->pBuffer, mlm->message);
				client->QueuePacket(outapp);
				delete outapp;
			}
			break;
		}
		case ServerOP_Uptime: {
			if (pack->size != sizeof(ServerUptime_Struct)) {
				cout << "Wrong size on ServerOP_Uptime. Got: " << pack->size << ", Expected: " << sizeof(ServerUptime_Struct) << endl;
				break;
			}
			ServerUptime_Struct* sus = (ServerUptime_Struct*) pack->pBuffer;
			int32 ms = Timer::GetCurrentTime();
			int32 d = ms / 86400000;
			ms -= d * 86400000;
			int32 h = ms / 3600000;
			ms -= h * 3600000;
			int32 m = ms / 60000;
			ms -= m * 60000;
			int32 s = ms / 1000;
			if (d)
				this->SendEmoteMessage(sus->adminname, 0, 0, "Zone #%i Uptime: %02id %02ih %02im %02is", sus->zoneserverid, d, h, m, s);
			else if (h)
				this->SendEmoteMessage(sus->adminname, 0, 0, "Zone #%i Uptime: %02ih %02im %02is", sus->zoneserverid, h, m, s);
			else
				this->SendEmoteMessage(sus->adminname, 0, 0, "Zone #%i Uptime: %02im %02is", sus->zoneserverid, m, s);
		}
		case ServerOP_Petition: {
			cout << "Got Server Requested Petition List Refresh" << endl;
			ServerPetitionUpdate_Struct* sus = (ServerPetitionUpdate_Struct*) pack->pBuffer;
			// solar: this was typoed to = instead of ==, not that it acts any different now though..
			if (sus->status == 0) petition_list.ReadDatabase();
			else if (sus->status == 1) petition_list.ReadDatabase(); // Until I fix this to be better....
			break;
		}
		case ServerOP_RezzPlayer: {
			RezzPlayer_Struct* srs = (RezzPlayer_Struct*) pack->pBuffer;
			if (srs->rezzopcode == OP_RezzRequest){
				Client* client = entity_list.GetClientByName(srs->rez.your_name);
				if (client){
					//client->SetZoneSummonCoords(srs->x_pos, srs->y_pos, srs->z_pos);
                    //client->pendingrezzexp = srs->exp;
                    if (srs->rez.spellid != 994) {
                      LogFile->write(EQEMuLog::Debug, "Sending player cast rez spellid:%i", srs->rez.spellid);
                      // Not gm resurrection
                        client->BuffFade(0xFFFe);
                        client->SpellOnTarget(756,client);
                        client->AddEXP(srs->exp);
                    }
                    else {
                      // GM resurrection
                      LogFile->write(EQEMuLog::Debug, "Sending gm cast rez");
                        client->AddEXP(srs->exp);
                    }
                    ServerPacket* pack = new ServerPacket;
                    pack->opcode = ServerOP_ZonePlayer;
                    pack->size = sizeof(ServerZonePlayer_Struct);
                    pack->pBuffer = new uchar[pack->size];
                    ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
                    strcpy(szp->adminname, srs->rez.corpse_name);
                    szp->adminrank = 0;//entity_list.GetClientByName(rezz->rezzer_name)->Admin();
                    szp->ignorerestrictions = 2;
                    strcpy(szp->name, srs->rez.your_name);
                    strcpy(szp->zone, srs->rez.zone);
                    szp->x_pos = srs->rez.x;
                    szp->y_pos = srs->rez.y;
                    szp->z_pos = srs->rez.z;
                    worldserver.SendPacket(pack);
                    delete pack;
                    
					//APPLAYER* outapp = new APPLAYER(srs->rezzopcode, sizeof(Resurrect_Struct));
					//memcpy(outapp->pBuffer,srs->packet, sizeof(srs->packet));
					//client->QueuePacket(outapp);
					//delete outapp;
				}
			}
			if (srs->rezzopcode == OP_RezzComplete){
				Mob* corpse =entity_list.GetMob(srs->rez.corpse_name);
				if (corpse && corpse->IsCorpse())
					corpse->CastToCorpse()->CompleteRezz();
				
			}
			
			break;
		}
		case ServerOP_ZoneReboot: {
			cout << "Got Server Requested Zone reboot" << endl;
			ServerZoneReboot_Struct* zb = (ServerZoneReboot_Struct*) pack->pBuffer;
		//	printf("%i\n",zb->zoneid);
			struct in_addr	in;
			in.s_addr = GetIP();
#ifdef WIN32
			char buffer[200];
			snprintf(buffer,200,". %s %i %s",zb->ip2, zb->port, inet_ntoa(in));
			if(zb->zoneid != 0) {
				snprintf(buffer,200,"%s %s %i %s",database.GetZoneName(zb->zoneid),zb->ip2, zb->port ,inet_ntoa(in));
				cout << "executing: " << buffer; 
				ShellExecute(0,"Open",net.GetZoneFileName(), buffer, 0, SW_SHOWDEFAULT);
			}
			else
			{
				cout << "executing: " << net.GetZoneFileName() << " " << buffer; 
				ShellExecute(0,"Open",net.GetZoneFileName(), buffer, 0, SW_SHOWDEFAULT);
			}
#else
			char buffer[5];
			snprintf(buffer,5,"%i",zb->port); //just to be sure that it will work on linux	
			if(zb->zoneid != 0)
				execl(net.GetZoneFileName(),net.GetZoneFileName(),database.GetZoneName(zb->zoneid),zb->ip2, buffer,inet_ntoa(in), NULL);
			else
				execl(net.GetZoneFileName(),net.GetZoneFileName(),".",zb->ip2, buffer,inet_ntoa(in), NULL);
#endif
			break;
		}
		case ServerOP_SyncWorldTime: {
			if(zone!=0) {
				cout << "Received Message SyncWorldTime" << endl;
				eqTimeOfDay* newtime = (eqTimeOfDay*) pack->pBuffer;
				zone->zone_time.setEQTimeOfDay(newtime->start_eqtime, newtime->start_realtime);
				APPLAYER* outapp = new APPLAYER;
				outapp->opcode = OP_TimeOfDay;
				outapp->size = sizeof(TimeOfDay_Struct);
				outapp->pBuffer = new uchar[outapp->size];
				memset(outapp->pBuffer, 0, outapp->size);
				TimeOfDay_Struct* tod = (TimeOfDay_Struct*)outapp->pBuffer;
				zone->zone_time.getEQTimeOfDay(time(0), tod);
				entity_list.QueueClients(0, outapp, false);
				delete outapp;
				//TEST
				char timeMessage[255];
				time_t timeCurrent = time(NULL);
				TimeOfDay_Struct eqTime;
				zone->zone_time.getEQTimeOfDay( timeCurrent, &eqTime);
				//if ( eqTime.hour >= 0 && eqTime.minute >= 0 )
				//{
					sprintf(timeMessage,"EQTime [%02d:%s%d %s]",
						(eqTime.hour % 12) == 0 ? 12 : (eqTime.hour % 12),
						(eqTime.minute < 10) ? "0" : "",
						eqTime.minute,
						(eqTime.hour >= 12) ? "pm" : "am"
						);
					cout << "Time Broadcast Packet: " << timeMessage << endl;
					zone->GotCurTime(true);
				//}
				//Test
			}
			break;
		}
		case ServerOP_ChangeWID: {
			if (pack->size != sizeof(ServerChangeWID_Struct)) {
				cout << "Wrong size on ServerChangeWID_Struct. Got: " << pack->size << ", Expected: " << sizeof(ServerChangeWID_Struct) << endl;
				break;
			}
			ServerChangeWID_Struct* scw = (ServerChangeWID_Struct*) pack->pBuffer;
			Client* client = entity_list.GetClientByCharID(scw->charid);
			if (client)
				client->SetWID(scw->newwid);
			break;
		}
		case ServerOP_ItemStatus: {
			if (pack->size != 5) {
				cout << "Wrong size on ServerChangeWID_Struct. Got: " << pack->size << ", Expected: 5" << endl;
				break;
			}
			database.SetItemStatus(*((int32*) &pack->pBuffer[0]), *((int8*) &pack->pBuffer[4]));
			break;
		}
		default: {
			cout << " Unknown ZSopcode:" << (int)pack->opcode;
			cout << " size:" << pack->size << endl;
			//DumpPacket(pack->pBuffer, pack->size);
			break;
		}
		}
		delete pack;
	}
	
	return;
}

bool WorldServer::SendChannelMessage(Client* from, const char* to, int8 chan_num, int32 guilddbid, int8 language, const char* message, ...) {
	va_list argptr;
	char buffer[256];
	
	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);
	
	ServerPacket* pack = new ServerPacket;
	
	pack->size = sizeof(ServerChannelMessage_Struct) + strlen(buffer) + 1;
	pack->pBuffer = new uchar[pack->size];
    memset(pack->pBuffer, 0, pack->size);
	ServerChannelMessage_Struct* scm = (ServerChannelMessage_Struct*) pack->pBuffer;
	
	pack->opcode = ServerOP_ChannelMessage;
	if (from == 0)
		strcpy(scm->from, "ZServer");
	else {
		strcpy(scm->from, from->GetName());
		scm->fromadmin = from->Admin();
	}
	if (to == 0)
		scm->to[0] = 0;
	else {
		strcpy(scm->to, to);
		strcpy(scm->deliverto, to);
	}
	scm->chan_num = chan_num;
	scm->guilddbid = guilddbid;
	scm->language = language;
	strcpy(&scm->message[0], buffer);
	
	bool ret = SendPacket(pack);
	delete pack;
	return ret;
}

bool WorldServer::SendEmoteMessage(const char* to, int32 to_guilddbid, int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[256];
	
	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);
	
	return SendEmoteMessage(to, to_guilddbid, 0, type, buffer);
}

bool WorldServer::SendEmoteMessage(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[256];
	
	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);

	if (!Connected() && to == 0) {
		entity_list.MessageStatus(to_guilddbid, to_minstatus, type, buffer);
		return false;
	}
	
	ServerPacket* pack = new ServerPacket(ServerOP_EmoteMessage, sizeof(ServerEmoteMessage_Struct)+strlen(buffer)+1);
	ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;
	sem->type = type;
	if (to != 0)
		strcpy(sem->to, to);
	sem->guilddbid = to_guilddbid;
	sem->minstatus = to_minstatus;
	strcpy(sem->message, buffer);
	
	bool ret = SendPacket(pack);
	delete pack;
	return ret;
}

bool WorldServer::RezzPlayer(APPLAYER* rpack,int32 rezzexp, int16 opcode) {
	ServerPacket* pack = new ServerPacket(ServerOP_RezzPlayer, sizeof(RezzPlayer_Struct));
	RezzPlayer_Struct* sem = (RezzPlayer_Struct*) pack->pBuffer;
	sem->rezzopcode = opcode;
	//memcpy(sem->packet,rpack->pBuffer,sizeof(sem->packet));
	sem->rez = *(Resurrect_Struct*) rpack->pBuffer;
	sem->exp = rezzexp;
	//Resurrect_Struct* rezz = (Resurrect_Struct*) rpack->pBuffer;
	bool ret = SendPacket(pack);
	delete pack;
	if (ret)
     LogFile->write(EQEMuLog::Debug, "Sending player rezz packet to world spellid:%i", sem->rez.spellid);
    else
     LogFile->write(EQEMuLog::Debug, "NOT Sending player rezz packet to world");
	return ret;
}

void WorldServer::AsyncConnect() {
	if (tcpc->ConnectReady())
		tcpc->AsyncConnect(net.GetWorldAddress(), PORT);
}
		
bool WorldServer::Connect() {
	char errbuf[TCPConnection_ErrorBufferSize];
	if (tcpc->Connect(net.GetWorldAddress(), PORT, errbuf)) {	
		return true;
	}
	else {
		cout << "WorldServer connect: Connecting to the server failed: " << errbuf << endl;
	}
	return false;
}

void WorldServer::Disconnect() {
	tcpc->Disconnect();
}

