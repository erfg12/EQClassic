#include <cstdarg>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include "servertalk.h"
#include "worldserver.h"
#include "eq_packet_structs.h"
#include "packet_dump.h"
#include "database.h"
#include "mob.h"
#include "zone.h"
#include "client.h"
#include "entity.h"
#include "net.h"
#include "petitions.h"
#include "EQCUtils.hpp"
#include "MessageTypes.h"
#include "ZoneGuildManager.h"
#include "SpellsHandler.hpp"

using namespace std;
using namespace EQC::Zone;

extern Database			database;
extern ZoneGuildManager	zgm;
extern EntityList		entity_list;
extern Zone* zone;
extern volatile bool	ZoneLoaded;
extern WorldServer		worldserver;
extern NetConnection	net;
extern PetitionList		petition_list;
extern SpellsHandler	spells_handler;
extern volatile bool	RunLoops;
extern void CatchSignal(int);

void WorldServer::Process()
{
	if(this == 0)
	{
		return;
	}
	if(!Connected())
	{
		return;
	}

	SOCKADDR_IN to;
	memset((char *) &to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_port = port;
	to.sin_addr.s_addr = ip;

	/************ Get all packets from packet manager out queue and process them ************/
	ServerPacket *pack = 0;
	while(pack = ServerRecvQueuePop())
	{
		switch(pack->opcode) 
		{
		case 0: 
			{
				break;
			}
		case ServerOP_KeepAlive: 
			{
				// ignore this
				break;
			}
		case ServerOP_ChannelMessage:
			{
				if (!ZoneLoaded) break;
				ServerChannelMessage_Struct* scm = (ServerChannelMessage_Struct*) pack->pBuffer;
				if (scm->deliverto[0] == 0) 
				{
					entity_list.ChannelMessageFromWorld(scm->from, scm->to, scm->chan_num, scm->guilddbid, scm->language, scm->message);
				}
				else 
				{
					Client* client;
					client = entity_list.GetClientByName(scm->deliverto);
					if (client != 0)
					{
						if (client->Connected()) 
						{
							client->ChannelMessageSend(scm->from, scm->to, scm->chan_num, scm->language, scm->message);
							if (!scm->noreply) 
							{
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
		case ServerOP_EmoteMessage: 
			{
				if (!ZoneLoaded)
				{
					break;
				}

				ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;

				if (sem->to[0] != 0)
				{
					if (strcasecmp(sem->to, zone->GetShortName()) == 0)
					{
						entity_list.Message(sem->guilddbid, (MessageFormat)sem->type, sem->message);
					}
					else
					{
						Client* client = entity_list.GetClientByName(sem->to);
						if (client != 0)
						{
							client->Message((MessageFormat)sem->type, sem->message);
						}
					}
				}
				else
				{
					entity_list.Message(sem->guilddbid, (MessageFormat)sem->type, sem->message);
				}
				break;
			}
		case ServerOP_GroupRefresh: //Kibanu - 4/22/2009
			{
				cout << "WorldServer_Process: In Group Refresh" << endl;
				if (!ZoneLoaded)
				{
					break;
				}

				ServerGroupRefresh_Struct* gr = (ServerGroupRefresh_Struct*) pack->pBuffer;

				if (gr->member[0] != 0)
				{
					cout << "WorldServer_Process: Refreshing group for " << gr->member << endl;
					Client* client = entity_list.GetClientByName(gr->member);
					if (client != 0)
					{
						client->RefreshGroup(gr->gid, gr->action);
					}
				}
				break;
			}
		case ServerOP_ShutdownAll: 
			{
				entity_list.Save();
				CatchSignal(2);
				break;
			}
		case ServerOP_ZoneShutdown: 
			{
				// Annouce the change to the world
				if (!ZoneLoaded)
				{
					SetZone("");
				}
				else
				{
					worldserver.SendEmoteMessage(0, 0, 15, "Zone shutdown: %s", zone->GetLongName());

					ServerZoneStateChange_struct* zst = (ServerZoneStateChange_struct *) pack->pBuffer;
					cout << "Zone shutdown by " << zst->adminname << endl;
					Zone::Shutdown();
				}
				break;
			}
		case ServerOP_ZoneBootup:
			{
				ServerZoneStateChange_struct* zst = (ServerZoneStateChange_struct *) pack->pBuffer;
				if (ZoneLoaded)
				{
					SetZone(zone->GetShortName());
					if (strcasecmp(zst->zonename, zone->GetShortName()) == 0) 
					{
						// This packet also doubles as "incomming client" notification, lets not shut down before they get here
						zone->StartShutdownTimer(AUTHENTICATION_TIMEOUT * 1000);
					}
					else 
					{
						worldserver.SendEmoteMessage(zst->adminname, 0, 0, "Zone bootup failed: Already running '%s'", zone->GetShortName());
					}
					break;
				}

				if (zst->adminname[0] != 0)
				{
					cout << "Zone bootup by " << zst->adminname << endl;
				}

				if (!(Zone::Bootup(zst->zonename))) 
				{
					worldserver.SendChannelMessage(0, 0, 10, 0, 0, "%s:%i Zone::Bootup failed: %s", net.GetZoneAddress(), net.GetZonePort(), zst->zonename);
				}
				break;
			}
		case ServerOP_ZonePlayer: 
			{
				ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(szp->name);
				if (client != 0) 
				{
					if (strcasecmp(szp->adminname, szp->name) == 0)
					{
						client->Message(BLACK, "Zoning to: %s", szp->zone);
					}
					else if (client->GetAnon() == 1 && client->Admin() > szp->adminrank)
					{
						break;
					}
					else 
					{
						worldserver.SendEmoteMessage(szp->adminname, 0, 0, "Summoning %s to %s %1.1f, %1.1f, %1.1f", szp->name, szp->zone, szp->x_pos, szp->y_pos, szp->z_pos);
					}
					client->SetIsZoning(true);
					client->SetUsingSoftCodedZoneLine(true);
					client->SetZoningHeading(client->GetHeading());
					client->SetZoningX(szp->x_pos);
					client->SetZoningY(szp->y_pos);
					client->SetZoningZ(szp->z_pos);
					client->ZonePC(szp->zone, szp->x_pos, szp->y_pos, szp->z_pos);
				}
				break;
			}
			// Cofruben: used to send a packet directly to a client.
		case ServerOP_SendPacket: 
			{
				ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*)pack->pBuffer;
				int32 PacketSize = pack->size - sizeof(sss->charname) - sizeof(sss->opcode);
				Client* PacketTo = entity_list.GetClientByName(sss->charname);
				if(!PacketTo)
				{
					break;
				}
				APPLAYER* outapp = new APPLAYER(sss->opcode, PacketSize);
				memcpy(outapp->pBuffer, sss->packet, PacketSize);
				PacketTo->QueuePacket(outapp);
				safe_delete(outapp);//delete outapp;
				break;
			}
		case ServerOP_KickPlayer:
			{
				ServerKickPlayer_Struct* skp = (ServerKickPlayer_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(skp->name);
				if (client != 0) 
				{
					if (skp->adminrank >= client->Admin()) 
					{
						client->Kick();
						if (ZoneLoaded)
						{
							worldserver.SendEmoteMessage(skp->adminname, 0, 0, "Remote Kick: %s booted in zone %s.", skp->name, zone->GetShortName());
						}
						else
						{
							worldserver.SendEmoteMessage(skp->adminname, 0, 0, "Remote Kick: %s booted.", skp->name);
						}
					}
					else if (client->GetAnon() != 1)
					{
						worldserver.SendEmoteMessage(skp->adminname, 0, 0, "Remote Kick: Your avatar level is not high enough to kick %s", skp->name);
					}
				}
				break;
			}
		case ServerOP_KillPlayer:
			{
				ServerKillPlayer_Struct* skp = (ServerKillPlayer_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(skp->target);
				if (client != 0) 
				{
					if (skp->admin >= client->Admin()) 
					{
						client->Kill();
						if (ZoneLoaded)
						{
							worldserver.SendEmoteMessage(skp->gmname, 0, 0, "Remote Kill: %s killed in zone %s.", skp->target, zone->GetShortName());
						}
						else
						{
							worldserver.SendEmoteMessage(skp->gmname, 0, 0, "Remote Kill: %s killed.", skp->target);
						}
					}
					else if (client->GetAnon() != 1)
					{
						worldserver.SendEmoteMessage(skp->gmname, 0, 0, "Remote Kill: Your avatar level is not high enough to kill %s", skp->target);
					}
				}
				break;
			}
		case ServerOP_RefreshGuild:
			{
				if (pack->size == 5)
				{
					int32 guildeqid = 0;
					memcpy(&guildeqid, pack->pBuffer, 4);

					zgm.LoadGuilds();
					Guild_Struct* guilds = zgm.GetGuildList();


					if (pack->pBuffer[4] == 1)
					{
						APPLAYER* outapp = new APPLAYER(OP_GuildUpdate, 64);
						memset(outapp->pBuffer, 0, outapp->size);
						GuildUpdate_Struct* gu = (GuildUpdate_Struct*) outapp->pBuffer;
						gu->guildID = guildeqid;
						gu->entry.guildID = guildeqid;

						if (guilds[guildeqid].databaseID == 0) 
						{
							gu->entry.exists = 0; // = 0x01 if exists, 0x00 on empty
						}
						else
						{
							strcpy(gu->entry.name, guilds[guildeqid].name);
							gu->entry.exists = 1; // = 0x01 if exists, 0x00 on empty
						}
						gu->entry.unknown1[0] = 0xFF;
						gu->entry.unknown1[1] = 0xFF;
						gu->entry.unknown1[2] = 0xFF;
						gu->entry.unknown1[3] = 0xFF;
						gu->entry.unknown3[0] = 0xFF;
						gu->entry.unknown3[1] = 0xFF;
						gu->entry.unknown3[2] = 0xFF;
						gu->entry.unknown3[3] = 0xFF;

						entity_list.QueueClients(0, outapp, false);
						safe_delete(outapp);//delete outapp;
					}
				}
				else
					cout << "Wrong size: ServerOP_RefreshGuild. size=" << pack->size << endl;
				break;
			}
		case ServerOP_GuildLeader:
			{
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(sgc->target);

				if (client == 0)
				{
					// do nothing
				}
				else if (client->GuildDBID() != sgc->guilddbid)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not in your guild.", client->GetName());
				}
				else if ((client->GuildRank() != 0) || (client->GuildRank() != 1))
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not rank 0.", client->GetName());
				}
				else 
				{
					if (Database::Instance()->SetGuildLeader(sgc->guilddbid, client->AccountID())) 
					{
						worldserver.SendEmoteMessage(0, sgc->guilddbid, MESSAGETYPE_Guild, "%s is now the leader of your guild.", client->GetName());
						ServerPacket* pack2 = new ServerPacket(ServerOP_RefreshGuild, 4);

						pack2->pBuffer = new uchar[pack->size];
						memcpy(pack2->pBuffer, &sgc->guildeqid, 4);
						worldserver.SendPacket(pack2);
						safe_delete(pack2);//delete pack2;
					}
					else
					{
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Guild leadership transfer failed.");
					}
				}
				break;
			}
		case ServerOP_GuildInvite: 
			{
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(sgc->target);
				zgm.LoadGuilds();
				Guild_Struct* guilds = zgm.GetGuildList();

				if (client == 0)
				{
					// do nothing
				}
				else if (!guilds[sgc->guildeqid].rank[sgc->fromrank].invite)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "You dont have permission to invite.");
				}
				else if (client->GuildDBID() != 0)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is already in another guild.", client->GetName());
				}
				else if (client->PendingGuildInvite != 0 && !(client->PendingGuildInvite == sgc->guilddbid))
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s has another pending guild invite.", client->GetName());
				}
				else
				{
					client->PendingGuildInvite = sgc->guilddbid;
					APPLAYER* outapp = new APPLAYER(OP_GuildInvite, sizeof(GuildCommand_Struct));
					memset(outapp->pBuffer, 0, outapp->size);
					GuildCommand_Struct* gc = (GuildCommand_Struct*) outapp->pBuffer;
					gc->guildeqid = sgc->guildeqid;
					strcpy(gc->Inviter, sgc->target);
					strcpy(gc->Invitee, sgc->from);
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
		case ServerOP_GuildRemove:
			{
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(sgc->target);
				zgm.LoadGuilds();
				Guild_Struct* guilds = zgm.GetGuildList();

				if (client == 0)
				{
					// do nothing
				}
				else if ((!guilds[sgc->guildeqid].rank[sgc->fromrank].remove) && !(strcasecmp(sgc->from, sgc->target) == 0))
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "You dont have permission to remove.");
				}
				else if (client->GuildDBID() != sgc->guilddbid)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not in your guild.", client->GetName());
				}
				else if (client->GuildRank() <= sgc->fromrank && !(sgc->fromaccountid == guilds[sgc->guildeqid].leader) && !(strcasecmp(sgc->from, sgc->target) == 0))
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s's rank is too high for you to remove them.", client->GetName());
				}
				else
				{
					if (client->SetGuild(0, GUILD_MAX_RANK))
					{
						if (strcasecmp(sgc->from, sgc->target) == 0)
						{
							worldserver.SendEmoteMessage(0, sgc->guilddbid, GUILD, "%s has left the guild.", client->GetName());
						}
						else
						{
							worldserver.SendEmoteMessage(0, sgc->guilddbid, GUILD, "%s has been removed from the guild by %s.", client->GetName(), sgc->from);
							client->Message(GUILD, "You have been removed from the guild by %s.", sgc->from);
						}
					}
					else
					{
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Guild remove failed.");
					}
				}
				break;
			}
		case ServerOP_GuildPromote:
			{
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(sgc->target);
				zgm.LoadGuilds();
				Guild_Struct* guilds = zgm.GetGuildList();

				if (client == 0)
				{
					// do nothing
				}

				else if (client->GuildDBID() != sgc->guilddbid)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not in your guild.", client->GetName());
				}
				else if ((!guilds[sgc->guildeqid].rank[sgc->fromrank].promote) && !(strcasecmp(sgc->from, sgc->target) == 0))
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "You dont have permission to promote.");
				}
				else if (client->GuildDBID() != sgc->guilddbid)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s isnt in your guild.", client->GetName());
				}
				else if (client->GuildRank() <= sgc->newrank)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is already rank %i.", client->GetName(), client->GuildRank());
				}
				else if (sgc->newrank <= sgc->fromrank && sgc->fromaccountid != guilds[sgc->guildeqid].leader)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "You cannot promote people to a greater or equal rank than yourself.");
				}
				else 
				{
					if (client->SetGuild(sgc->guilddbid, sgc->newrank)) 
					{
						worldserver.SendEmoteMessage(0, sgc->guilddbid, MESSAGETYPE_Guild, "%s has been promoted to %s by %s.", client->GetName(), guilds[sgc->guildeqid].rank[sgc->newrank].rankname, sgc->from);
					}
					else
					{
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Guild promote failed");
					}
				}
				break;
			}
		case ServerOP_GuildDemote:
			{
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(sgc->target);

				zgm.LoadGuilds();
				Guild_Struct* guilds = zgm.GetGuildList();

				if (client == 0)
				{
					// do nothing
				}
				else if (client->GuildDBID() != sgc->guilddbid)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is not in your guild.", client->GetName());
				}
				else if (!guilds[sgc->guildeqid].rank[sgc->fromrank].demote)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "You dont have permission to demote.");
				}
				else if (client->GuildRank() >= sgc->newrank)
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "%s is already rank %i.", client->GetName(), client->GuildRank());
				}
				else if (sgc->newrank <= sgc->fromrank && sgc->fromaccountid != guilds[sgc->guildeqid].leader && !(strcasecmp(sgc->from, sgc->target) == 0))
				{
					worldserver.SendEmoteMessage(sgc->from, 0, 0, "You cannot demote people with a greater or equal rank than yourself.");
				}
				else
				{
					if (client->SetGuild(sgc->guilddbid, sgc->newrank)) 
					{
						worldserver.SendEmoteMessage(0, sgc->guilddbid, MESSAGETYPE_Guild, "%s has been demoted to %s by %s.", client->GetName(), guilds[sgc->guildeqid].rank[sgc->newrank].rankname, sgc->from);
					}
					else
					{
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Guild demote failed");
					}
				}
				break;
			}
		case ServerOP_GuildGMSet:
			{
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(sgc->target);
				if (client != 0) 
				{
					if (client->GuildDBID() == 0 || sgc->guilddbid == 0) 
					{
						if (!client->SetGuild(sgc->guilddbid, GUILD_MAX_RANK))
						{
							worldserver.SendEmoteMessage(sgc->from, 0, 0, "Error: Guild #%i not found", sgc->guilddbid);
						}
					}
					else
					{
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Error: %s is already in a guild", sgc->target);
					}
				}
				break;
			}
		case ServerOP_GuildGMSetRank:
			{
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(sgc->target);
				if (client != 0)
				{
					if (client->GuildDBID() != 0)
					{
						if (!client->SetGuild(client->GuildDBID(), sgc->newrank))
						{
							worldserver.SendEmoteMessage(sgc->from, 0, 0, "Error: SetRank failed.", sgc->guilddbid);
						}
					}
					else
					{
						worldserver.SendEmoteMessage(sgc->from, 0, 0, "Error: %s is not in a guild", sgc->target);
					}
				}
				break;
			}
		case ServerOP_GuildCreateResponse:
			{
				GuildCreateResponse_Struct* gcrs = (GuildCreateResponse_Struct*) pack->pBuffer;

				if(gcrs->Created)
				{
					cout << "Guild " << gcrs->GuildName << " created." << endl;
					Client* client = entity_list.GetClientByName(gcrs->LeaderCharName);
					// invite the client to guild
					zgm.InviteToGuild(0, gcrs->eqid, client, client, GuildLeader);
				}
				
				break;
			}
		case ServerOP_FlagUpdate:
			{
				Client* client = entity_list.GetClientByAccID(Database::Instance()->GetAccountIDByName((char*) pack->pBuffer));
				if (client != 0) 
				{
					client->UpdateAdmin();
				}
				break;
			}
		case ServerOP_GMGoto: 
			{
				if (pack->size != sizeof(ServerGMGoto_Struct)) 
				{
					cout << "Wrong size on ServerOP_GMGoto. Got: " << pack->size << ", Expected: " << sizeof(ServerGMGoto_Struct) << endl;
					break;
				}
				if (!ZoneLoaded) 
				{
					break;
				}
				ServerGMGoto_Struct* gmg = (ServerGMGoto_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(gmg->gotoname);
				if (client != 0) 
				{
					worldserver.SendEmoteMessage(gmg->myname, 0, 13, "Summoning you to: %s @ %s, %1.1f, %1.1f, %1.1f", client->GetName(), zone->GetShortName(), client->GetX(), client->GetY(), client->GetZ());

					ServerPacket* outpack = new ServerPacket(ServerOP_ZonePlayer, sizeof(ServerZonePlayer_Struct));


					outpack->pBuffer = new uchar[outpack->size];
					memset(outpack->pBuffer, 0, outpack->size);
					ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) outpack->pBuffer;
					strcpy(szp->adminname, gmg->myname);
					strcpy(szp->name, gmg->myname);
					strcpy(szp->zone, zone->GetShortName());
					szp->x_pos = client->GetX();
					szp->y_pos = client->GetY();
					szp->z_pos = client->GetZ();
					worldserver.SendPacket(outpack);
					safe_delete(outpack);//delete outpack;
				}
				else
				{
					worldserver.SendEmoteMessage(gmg->myname, 0, 13, "Error: %s not found", gmg->gotoname);
				}
				break;
			}
		case ServerOP_MultiLineMsg:
			{
				ServerMultiLineMsg_Struct* mlm = (ServerMultiLineMsg_Struct*) pack->pBuffer;
				Client* client = entity_list.GetClientByName(mlm->to);
				if (client) 
				{
					APPLAYER* outapp = new APPLAYER(OP_MultiLineMsg, strlen(mlm->message));
					strcpy((char*) outapp->pBuffer, mlm->message);
					client->QueuePacket(outapp);
					safe_delete(outapp);//delete outapp;
				}
				break;
			}
		case ServerOP_RezzPlayer:
			{
				RezzPlayer_Struct* srs = (RezzPlayer_Struct*)pack->pBuffer;
				//Yeahlight: A resurrection request has been made
				if(srs->rezzopcode == OP_RezzRequest)
				{
					Client* client = entity_list.GetClientByName(srs->owner);
					//Yeahlight: Owner of the corpse is online
					if(client)
					{
						Spell* spell = spells_handler.GetSpellPtr(srs->rez.spellID);
						//Yeahlight: Spell passed through the packet is a legit spell
						if(spell)
						{
							//Yeahlight: Record the pending resurrection information
							client->SetPendingRez(true);
							client->SetPendingRezExp(srs->exp);
							client->SetPendingCorpseID(srs->corpseDBID);
							client->SetPendingRezX(srs->rez.x);
							client->SetPendingRezY(srs->rez.y);
							client->SetPendingRezZ(srs->rez.z);
							client->SetPendingRezZone(srs->rez.zoneName);
							client->SetPendingSpellID(spell->GetSpellID());
							client->StartPendingRezTimer();
							if(client->GetDebugMe())
								client->Message(YELLOW, "Debug: Forwarding the resurrection request to your client (%s)", srs->rez.targetName);
							//Yeahlight: Forward the resurrection packet to the corpse's owner
							APPLAYER* outapp = new APPLAYER(OP_RezzRequest, sizeof(Resurrect_Struct));
							Resurrect_Struct* rez = (Resurrect_Struct*)outapp->pBuffer;
							memset(rez, 0, sizeof(Resurrect_Struct));
							memcpy(rez, &srs->rez, sizeof(Resurrect_Struct));
							client->QueuePacket(outapp);
							safe_delete(outapp);
						}
					}
				}
				//Yeahlight: A resurrection confirmation has been made
				else if(srs->rezzopcode == OP_RezzAnswer)
				{
					Client* client = entity_list.GetClientByName(srs->owner);
					//Yeahlight: Owner of the corpse is online
					if(client)
					{
						bool fullGMRez = false;
						//Yeahlight: The corpse owner answers 'yes' to the request
						if(srs->action == 1)
						{
							Database::Instance()->SetCorpseRezzed(client->GetPendingCorpseID());
							//Yeahlight: Player casted resurrection
							if(client->GetPendingSpellID() != 994)
							{
								float expRefund = 0.00;
								//Yeahlight: PC has exp on the corpse
								if(client->GetPendingRezExp())
								{
									//Yeahlight: Switch based on spell ID
									switch(client->GetPendingSpellID())
									{
										//Yeahlight: Revive
										case 391:
										{
											expRefund = 0.00;
											break;
										}
										//Yeahlight: Resuscitate
										case 388:
										{
											expRefund = 0.50;
											break;
										}
										//Yeahlight: Resurrection
										case 392:
										{
											expRefund = 0.90;
											break;
										}
										//Yeahlight: Reviviscence
										case 1524:
										{
											expRefund = 0.96;
											break;
										}
										//Yeahlight: Convergence
										case 1733:
										{
											expRefund = 0.93;
											break;
										}
										default:
										{
											client->Message(RED, "ERROR: Unknown resurrection spell ID: %i", client->GetPendingSpellID());
											expRefund = 0.00;
										}
									}
									if(client->GetDebugMe())
										client->Message(LIGHTEN_BLUE, "Debug: Your resurrection return on exp: %1.2f", expRefund);
									client->AddEXP(client->GetPendingRezExp() * expRefund, true);
								}
							}
							//Yeahlight: GM casted resurrection
							else
							{
								fullGMRez = true;
								if(client->GetDebugMe())
									client->Message(LIGHTEN_BLUE, "Debug: Your resurrection return on exp: 1.00");
								client->AddEXP(client->GetPendingRezExp(), true);
							}
							//Yeahlight: Client is being resurrected in another zone; prepare them for transit
							if(strcmp(zone->GetShortName(), srs->rez.zoneName) != 0)
							{
								client->SetTempHeading(0);
								client->SetUsingSoftCodedZoneLine(true);
								client->SetIsZoning(true);
								client->SetZoningX(client->GetPendingRezX());
								client->SetZoningY(client->GetPendingRezY());
								client->SetZoningZ(client->GetPendingRezZ());
							}
							//Yeahlight: Send the packet back to the owner of the corpse
							APPLAYER* outapp = new APPLAYER(OP_RezzComplete, sizeof(Resurrect_Struct));
							memcpy(outapp->pBuffer, &srs->rez, sizeof(Resurrect_Struct));
							srs->rez.fullGMRez = fullGMRez;
							client->QueuePacket(outapp);
							safe_delete(outapp);
						}
						//Yeahlight: The corpse owner answers 'no' to the request
						else if(srs->action == 0)
						{
							client->Message(BLACK, "You decline the request to be resurrected.");
							client->SetPendingRez(false);
							client->SetPendingRezExp(0);
						}
					}
				}
				break;
			}
		case ServerOP_Uptime: 
			{
				if (pack->size != sizeof(ServerUptime_Struct))
				{
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
				{
					this->SendEmoteMessage(sus->adminname, 0, 0, "Zone #%i Uptime: %02id %02ih %02im %02is", sus->zoneserverid, d, h, m, s);
				}
				else if (h)
				{
					this->SendEmoteMessage(sus->adminname, 0, 0, "Zone #%i Uptime: %02ih %02im %02is", sus->zoneserverid, h, m, s);
				}
				else
				{
					this->SendEmoteMessage(sus->adminname, 0, 0, "Zone #%i Uptime: %02im %02is", sus->zoneserverid, m, s);
				}
				break;
			}
		case ServerOP_Petition:
			{
				cout << "Got Server Requested Petition List Refresh" << endl;
				ServerPetitionUpdate_Struct* sus = (ServerPetitionUpdate_Struct*) pack->pBuffer;
				if (sus->status = 0) 
				{
					petition_list.ReadDatabase();
				}

				else if (sus->status = 1)
				{
					petition_list.ReadDatabase(); // Until I fix this to be better....
				}
				break;
			}
		case ServerOP_RemoveBoat: // Tazadar (06/12/09) : We remove the boat from the zone 
			{
				BoatName_Struct* bs = (BoatName_Struct*) pack->pBuffer;
				
				cout << "Trying to remove the boat : " << bs->boatname << endl;
	
				Mob * tmp = entity_list.GetMob(bs->boatname);
				
				if(tmp){
					tmp->CastToNPC()->Depop(false);
					entity_list.RemoveNPC(tmp->CastToNPC());
				}

				break;
			}
		case ServerOP_ZoneBoat	: // Tazadar (06/16/09) : We teleport player to the other zone !
			{
				cout << "A boat is zoning ! Teleporting clients !" << endl;
				ZoneBoat_Struct * zb = (ZoneBoat_Struct *) pack->pBuffer;
				Entity * tmp = 0;
				Mob * boat = entity_list.GetMob(zb->boatname);
				float xboat,yboat,zboat,headingboat;
				if(boat){
					xboat = boat->GetX();
					yboat = boat->GetY();
					zboat = boat->GetZ();;
					headingboat = boat->GetHeading();
				}
				else{
					cerr << "Error in ServerOP_ZoneBoat boat : "<< zb->boatname << " does not exist."<<endl;
					break;
				}
				char  * clients = (char *) pack->pBuffer;
				char zone[16] = "";
				strcpy(zone, zb->zonename);
				char name[30];
				float x,y,z,heading;
				for(int i=0 ; i < zb->numberOfPlayers ; i++){
					memcpy(name,clients+sizeof(ZoneBoat_Struct)+30*i,30*sizeof(char));
					cout << "Trying to teleport : "<< name << endl;
					tmp = entity_list.GetClientByName(name);
					if(tmp != 0 && tmp->IsClient()){
						//cout << "ZonePC to " << zone << endl;

						Client * client = tmp->CastToClient();

						heading = client->GetHeading()-headingboat; // Tazadar : Relative heading from the boat
						z = client->GetZ()-zboat; // Tazadar : Relative z from the boat
						
						
						client->GetRelativeCoordToBoat(client->GetY()-yboat,client->GetX()-xboat,(zb->heading-headingboat)*1.4,y,x);
						client->SetZoningHeading(zb->heading+heading);
						client->SetUsingSoftCodedZoneLine(true);
						client->SetIsZoning(true);
						client->SetZoningX(zb->x+x);
						client->SetZoningY(zb->y+y);
						client->SetZoningZ(zb->z+z);
						client->ZonePC(zone,0,0,10);
					}
					else{ // Tazadar : Player has crashed ? We remove it from boat infos

						// Tazadar : Make the packet for the World Serv
						ServerPacket* pack = new ServerPacket(ServerOP_BoatPL, sizeof(ServerBoat_Struct));

						ServerBoat_Struct* servBoatInfos = (ServerBoat_Struct*) pack->pBuffer;

						// Tazadar : Fill the packet
						memcpy(servBoatInfos->player_name,name,sizeof(servBoatInfos->player_name));

						// Tazadar : Send the packet
						worldserver.SendPacket(pack);

					}
				}

				break;
			}
		case ServerOP_SpawnBoat: // Tazadar (06/13/09) : We make the boat spawn before the zone in at the correct location.
			{

				SpawnBoat_Struct * bs = (SpawnBoat_Struct *) pack->pBuffer; 
				
				Mob * tmp = 0;
				
				cout << "Spawning the boat :" << bs->boatname << "at loc " << bs->x <<","<< bs->y << "," << bs->z <<" heading " << bs->heading<<endl;
				//cout << "Loading boat from DB " << endl;
				string name = bs->boatname;
				name.resize(name.length()-2);
				NPCType boat;
				bool lineroute = false;
				Database::Instance()->LoadBoatData(name.c_str(),boat,lineroute);
				NPC* npc = new NPC(&boat, 0, bs->x, bs->y, bs->z, bs->heading);
				npc->SetLineRoute(lineroute);
				entity_list.AddNPC(npc);
				npc->SendPosUpdate(false);
				
				break;
			}

		case ServerOP_BoatGoTo :  // Tazadar (06/14/09) : We make the boat move
			{
				BoatGoTo_Struct * gt = (BoatGoTo_Struct *) pack->pBuffer;
				cout << "World orders " << gt->boatname << " to go to node number " << gt->toNode << endl;
				
				Mob * tmp = entity_list.GetMob(gt->boatname);
				if(tmp && tmp->IsNPC()){
					if(tmp->CastToNPC()->IsBoat()){
						tmp->CastToNPC()->setBoatDestinationNode(gt->fromNode,gt->toNode);
					}
					else{
						cerr << "Tried to give an order to the npc "<< gt->boatname <<" which is not a boat " << endl;
					}
				}
				else{
					cerr << "Error in ServerOP_BoatGoTo boat " << gt->boatname << " does not exist"<<endl;
				}

				break;
			}
		case ServerOP_DayTime : // Kibanu - 7/30/2009 : Switch to day time for spawn data
			{
				if (ZoneLoaded)
					zone->SetTimeOfDay(true);
				break;
			}
		case ServerOP_NightTime : // Kibanu - 7/30/2009 : Switch to day time for spawn data
			{
				if (ZoneLoaded)
					zone->SetTimeOfDay(false);
				break;
			}
		default:
			{
				cout << "Unknown ZSopcode:" << (int)pack->opcode;
				cout << "size:" << pack->size << endl;
				//DumpPacket(pack->pBuffer, pack->size);
				break;
			}
		}

		safe_delete(pack);//delete pack;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	return;
}