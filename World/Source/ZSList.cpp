// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstdarg>

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "unix.h"

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

extern int errno;
#endif

#include "servertalk.h"
#include "eq_packet_structs.h"
#include "packet_dump.h"
#include "database.h"
#include "classes.h"
#include "races.h"
#include "seperator.h"
#include "zoneserver.h"
#include "ClientListEntry.h"
#include "client.h"
#include "LoginServer.h"
#include "ZSList.h"
#include "config.h"
#include "WorldGuildManager.h"

using namespace std;
extern uint32 numzones;

namespace EQC
{
	namespace World
	{
		namespace Network
		{
			// Constructor
			ZSList::ZSList() 
			{
				NextID = 1;
			}

			// Destructor
			ZSList::~ZSList()
			{
			}

			// Add a Zone Server to the list
			void ZSList::Add(ZoneServer* zoneserver)
			{
				LockMutex lock(&MListLock);
				list.Insert(zoneserver);
			}

			// Process a Zone Server
			void ZSList::Process()
			{
				LinkedListIterator<ZoneServer*> iterator(list);

				LockMutex lock(&MListLock);
				iterator.Reset();
				while(iterator.MoreElements())
				{
					if (!iterator.GetData()->Process())
					{
						struct in_addr  in;
						in.s_addr = iterator.GetData()->GetIP();
						cout << "Removing zoneserver from ip:" << inet_ntoa(in) << " port:" << (int16)(iterator.GetData()->GetPort()) << " (" << iterator.GetData()->GetCAddress() << ":" << iterator.GetData()->GetCPort() << ")" << endl;
						iterator.RemoveCurrent();
						numzones--;
					}
					else
					{
						iterator.Advance();
					}
					////Yeahlight: Zone freeze debug
					//if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					//	EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
			}

			// Receive Data from the Zone Server
			void ZSList::ReceiveData()
			{
				LinkedListIterator<ZoneServer*> iterator(list);

				LockMutex lock(&MListLock);
				iterator.Reset();
				while(iterator.MoreElements())
				{
					if (!iterator.GetData()->ReceiveData())
					{
						struct in_addr  in;
						in.s_addr = iterator.GetData()->GetIP();
						cout << "Removing zoneserver: " << iterator.GetData()->GetZoneName() << " " << inet_ntoa(in) << ":" << (int16)(iterator.GetData()->GetPort()) << " (" << iterator.GetData()->GetCAddress() << ":" << iterator.GetData()->GetCPort() << ")" << endl;
						iterator.RemoveCurrent();
						numzones--;
					}
					else
					{
						iterator.Advance();
					}
					////Yeahlight: Zone freeze debug
					//if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					//	EQC_FREEZE_DEBUG(__LINE__, __FILE__);

				}
			}

			// Sends a packet to the Zone Server
			void ZSList::SendPacket(ServerPacket* pack) 
			{
				LinkedListIterator<ZoneServer*> iterator(list);

				LockMutex lock(&MListLock);
				iterator.Reset();
				while(iterator.MoreElements())
				{
					iterator.GetData()->SendPacket(pack);
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
			}

			void ZSList::SendPacket(char* to, ServerPacket* pack)
			{
				if (to == 0 || to[0] == 0)
				{
					zoneserver_list.SendPacket(pack);
				}
				else if (to[0] == '*') 
				{
					return; // Cant send a packet to a console....
				}
				else 
				{
					ClientListEntry* cle = zoneserver_list.FindCharacter(to);
					if (cle != 0) 
					{
						if (cle->Server() != 0)
						{
							cle->Server()->SendPacket(pack);
						}
						return;
					}
					else 
					{
						ZoneServer* zs = zoneserver_list.FindByName(to);

						if (zs != 0) 
						{
							zs->SendPacket(pack);
							return;
						}
					}
				}
			}

			ZoneServer* ZSList::FindByName(char* zonename)
			{
				LinkedListIterator<ZoneServer*> iterator(list);

				LockMutex lock(&MListLock);
				iterator.Reset();

				while(iterator.MoreElements())
				{
					if (strcasecmp(iterator.GetData()->GetZoneName(), zonename) == 0)
					{
						ZoneServer* tmp = iterator.GetData();
						return tmp;
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				return 0;
			}

			ZoneServer* ZSList::FindByID(int32 ZoneID) 
			{
				LinkedListIterator<ZoneServer*> iterator(list);

				LockMutex lock(&MListLock);
				iterator.Reset();

				while(iterator.MoreElements())
				{
					if (iterator.GetData()->GetID() == ZoneID) 
					{
						ZoneServer* tmp = iterator.GetData();
						return tmp;
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				return 0;
			}

			void ZSList::SendZoneStatus(char* to, int8 admin, EQC::World::Network::TCPConnection* connection) 
			{
				LinkedListIterator<ZoneServer*> iterator(list);
				struct in_addr  in;

				LockMutex lock(&MListLock);
				iterator.Reset();
				char locked[4];

				if (net.world_locked == true)
				{
					strcpy(locked, "Yes");
				}
				else
				{
					strcpy(locked, "No");
				}

				connection->SendEmoteMessage(to, 0, 0, "World Locked: %s", locked);
				connection->SendEmoteMessage(to, 0, 0, "Zoneservers online:");
				int x=0, y=0;
				while(iterator.MoreElements())
				{
					in.s_addr = iterator.GetData()->GetIP();

					if (admin >= 150)
					{
						connection->SendEmoteMessage(to, 0, 0, "  #%i  %s:%i  %s:%i  %s", iterator.GetData()->GetID(), inet_ntoa(in), iterator.GetData()->GetPort(), iterator.GetData()->GetCAddress(), iterator.GetData()->GetCPort(), iterator.GetData()->GetZoneName());
						x++;
					}
					else if (strlen(iterator.GetData()->GetZoneName()) != 0)
					{
						connection->SendEmoteMessage(to, 0, 0, "  #%i  %s", iterator.GetData()->GetID(), iterator.GetData()->GetZoneName());
						x++;
					}
					y++;
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				connection->SendEmoteMessage(to, 0, 0, "%i servers listed. %i servers online.", x, y);
			}

			void ZSList::SendChannelMessage(char* from, char* to, int8 chan_num, int8 language, char* message, ...)
			{
				va_list argptr;
				char buffer[256];

				va_start(argptr, message);
				vsnprintf(buffer, 256, message, argptr);
				va_end(argptr);

				ServerPacket* pack = new ServerPacket;

				pack->opcode = ServerOP_ChannelMessage;
				pack->size = sizeof(ServerChannelMessage_Struct)+strlen(buffer)+1;
				pack->pBuffer = new uchar[pack->size];
				memset(pack->pBuffer, 0, pack->size);
				ServerChannelMessage_Struct* scm = (ServerChannelMessage_Struct*) pack->pBuffer;

				if (from == 0)
				{
					strcpy(scm->from, "WServer");
					scm->noreply = true;
				}
				else if (from[0] == 0)
				{
					strcpy(scm->from, "WServer");
					scm->noreply = true;
				}
				else
				{
					strcpy(scm->from, from);
				}

				if (to != 0)
				{
					strcpy((char *) scm->to, to);
					strcpy((char *) scm->deliverto, to);
				}
				else
				{
					scm->to[0] = 0;
					scm->deliverto[0] = 0;
				}

				scm->language = language;
				scm->chan_num = chan_num;
				strcpy(&scm->message[0], buffer);
				zoneserver_list.SendPacket(pack);
				safe_delete(pack);//delete pack;
			}


			void ZSList::SendEmoteMessage(char* to, int32 to_guilddbid, int32 type, char* message, ...)
			{
				va_list argptr;
				char buffer[256];

				va_start(argptr, message);
				vsnprintf(buffer, 256, message, argptr);
				va_end(argptr);

				ServerPacket* pack = new ServerPacket;

				pack->opcode = ServerOP_EmoteMessage;
				pack->size = sizeof(ServerEmoteMessage_Struct)+strlen(buffer)+1;
				pack->pBuffer = new uchar[pack->size];
				memset(pack->pBuffer, 0, pack->size);
				ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;

				if (to != 0)
				{
					strcpy((char *) sem->to, to);
				}
				else
				{
					sem->to[0] = 0;
				}

				sem->guilddbid = to_guilddbid;
				sem->type = type;
				strcpy(&sem->message[0], buffer);

				if (sem->to[0] == 0)
				{
					zoneserver_list.SendPacket(pack);
				}
				else
				{
					ZoneServer* zs = zoneserver_list.FindByName(sem->to);
					EQC::World::Network::Console* con = 0;
					if (sem->to[0] == '*')
					{
						con = console_list.FindByAccountName(&sem->to[1]);
					}

					if (zs != 0)
					{
						zs->SendPacket(pack);
					}
					else if (con != 0)
					{
						con->SendExtMessage(sem->message);
					}
					else
					{
						zoneserver_list.SendPacket(pack);
					}
				}
				safe_delete(pack);//delete pack;
			}

			void ZSList::ForceGroupRefresh(char* member, int32 groupid, int8 action)
			{
				cout << "ZSList: Refreshing group for " << member << endl;
				if (strlen(member) > 0)
				{
					ServerPacket* pack = new ServerPacket( ServerOP_GroupRefresh, sizeof(ServerGroupRefresh_Struct));
					memset(pack->pBuffer, 0, pack->size);
					ServerGroupRefresh_Struct* gr = (ServerGroupRefresh_Struct*) pack->pBuffer;

					strcpy(gr->member, member);
					gr->action = action;
					gr->gid = groupid;

					zoneserver_list.SendPacket(pack);
					safe_delete(pack);//delete pack;
				}				
			}

			void ZSList::ClientRemove(char* name, char* zone)
			{
				LinkedListIterator<ClientListEntry*> iterator(clientlist);

				iterator.Reset();
				while(iterator.MoreElements())
				{
					if (strcasecmp(iterator.GetData()->zone(), zone) == 0)
					{
						if (name == 0)
						{
							iterator.RemoveCurrent();
						}
						else if (name[0] == 0)
						{
							iterator.RemoveCurrent();
						}
						else if (strcasecmp(iterator.GetData()->name(), name) == 0)
						{
							iterator.RemoveCurrent();
							return;
						}
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
			}



			ClientListEntry* ZSList::FindCharacter(char* name)
			{
				LinkedListIterator<ClientListEntry*> iterator(clientlist);

				iterator.Reset();
				while(iterator.MoreElements())
				{
					if (strcasecmp(iterator.GetData()->name(), name) == 0)
					{
						return iterator.GetData();
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				return 0;
			}

			void ZSList::CLCheckStale()
			{
				return; // 15/10/2007 - froglok23 -- why is this here? this is not implimented???
				LinkedListIterator<ClientListEntry*> iterator(clientlist);

				iterator.Reset();
				while(iterator.MoreElements())
				{
					if (iterator.GetData()->CheckStale())
					{
						iterator.RemoveCurrent();
					}
					else
					{
						iterator.Advance();
					}
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
			}

			void ZSList::ClientUpdate(ZoneServer* zoneserver, char* name, int32 accountid, char* accountname, int8 admin, char* zone, int8 level, int8 class_, int8 race, int8 anon, bool tellsoff, int32 guilddbid, int32 guildeqid, bool LFG)
			{
				LinkedListIterator<ClientListEntry*> iterator(clientlist);

				iterator.Reset();
				while(iterator.MoreElements())
				{
					if (strcasecmp(iterator.GetData()->name(), name) == 0) 
					{
						iterator.GetData()->Update(zoneserver, name, accountid, accountname, admin, zone, level, class_, race, anon, tellsoff, guilddbid, guildeqid, LFG);
						return;
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				ClientListEntry* tmp = new ClientListEntry(zoneserver, name, accountid, accountname, admin, zone, level, class_, race, anon, tellsoff, guilddbid, guildeqid, LFG);
				clientlist.Insert(tmp);
			}

			// Kibanu - 7/28/2009
			LinkedList<ClientListEntry*>* ZSList::GetClientList()
			{
				return &clientlist;
			}

			void ZSList::SendWhoAll(char* to, int8 admin, Who_All_Struct* whom, EQC::World::Network::TCPConnection* connection)
			{
				LinkedListIterator<ClientListEntry*> iterator(clientlist);
				iterator.Reset();
				char tmpgm[25] = "";
				char accinfo[150] = "";
				char line[300] = "";
				char tmpguild[50] = "";
				char LFG[10] = "";
				int32 x = 0;
				int whomlen = 0;
				if (whom)
				{
					whomlen = strlen(whom->whom);
				}

				connection->SendEmoteMessage(to, 0, 10, "Players on server:");

				Guild_Struct* guilds = guild_mgr.GetGuildList();

				while(iterator.MoreElements())
				{
					if (whom == 0 || (
						(iterator.GetData()->Admin() >= 80 || whom->gmlookup == 0xFFFF) && 
						(whom->wclass == 0xFFFF || iterator.GetData()->class_() == whom->wclass) && 
						(whom->wrace == 0xFFFF || iterator.GetData()->race() == whom->wrace) && 
						(whomlen == 0 || strncasecmp(iterator.GetData()->zone(),whom->whom, whomlen) == 0 || strncasecmp(iterator.GetData()->name(),whom->whom, whomlen) == 0 || (iterator.GetData()->GuildEQID() != 0xFFFFFFFF && strncasecmp(guilds[iterator.GetData()->GuildEQID()].name, whom->whom, whomlen) == 0) || (admin >= 100 && strncasecmp(iterator.GetData()->AccountName(), whom->whom, whomlen) == 0))
						)) 
					{
						line[0] = 0;
						if (iterator.GetData()->Admin() == 101)
						{
							strcpy(tmpgm, "* GM-Events *");
						}
						else if (iterator.GetData()->Admin() == 80)
						{
							strcpy(tmpgm, "* Quest Troupe *");
						}
						else if (iterator.GetData()->Admin() >= 200)
						{
							strcpy(tmpgm, "* ServerOP *");
						}
						else if (iterator.GetData()->Admin() >= 150)
						{
							strcpy(tmpgm, "* Lead-GM *");
						}
						else if (iterator.GetData()->Admin() >= 100)
						{
							strcpy(tmpgm, "* GM *");
						}
						else
						{
							tmpgm[0] = 0;
						}

						if (iterator.GetData()->GuildEQID() != 0xFFFFFFFF)
						{
							if (guilds[iterator.GetData()->GuildEQID()].databaseID == 0)
							{
								tmpguild[0] = 0;
							}
							else
							{
								snprintf(tmpguild, 36, " <%s>", guilds[iterator.GetData()->GuildEQID()].name);
							}
						}
						else
						{
							tmpguild[0] = 0;
						}

						if (iterator.GetData()->LFG())
						{
							strcpy(LFG, " LFG");
						}
						else
						{
							LFG[0] = 0;
						}

						if (admin >= 150 && admin >= iterator.GetData()->Admin()) 
						{
							sprintf(accinfo, " AccID: %i AccName: %s Status: %i", iterator.GetData()->AccountID(), iterator.GetData()->AccountName(), iterator.GetData()->Admin());
						}
						else
						{
							accinfo[0] = 0;
						}

						if (iterator.GetData()->Anon() == 2) // Roleplay
						{ 
							if (admin >= 100 && admin >= iterator.GetData()->Admin())
							{
								sprintf(line, "  %s[RolePlay %i %s] %s (%s)%s zone: %s%s%s", tmpgm, iterator.GetData()->level(), EQC::Common::GetClassName(iterator.GetData()->class_()), iterator.GetData()->name(), EQC::Common::GetRaceName(iterator.GetData()->race()), tmpguild, iterator.GetData()->zone(), LFG, accinfo);
							}
							else if (iterator.GetData()->Admin() >= 100 && admin < 100)
							{
								iterator.Advance();
								continue;
							}
							else
							{
								sprintf(line, "  %s[ANONYMOUS] %s%s%s%s", tmpgm, iterator.GetData()->name(), tmpguild, LFG, accinfo);
							}
						}
						else if (iterator.GetData()->Anon() == 1) // Anon
						{ 
							if (admin >= 100 && admin >= iterator.GetData()->Admin())
							{
								sprintf(line, "  %s[ANON %i %s] %s (%s)%s zone: %s%s%s", tmpgm, iterator.GetData()->level(), EQC::Common::GetClassName(iterator.GetData()->class_()), iterator.GetData()->name(), EQC::Common::GetRaceName(iterator.GetData()->race()), tmpguild, iterator.GetData()->zone(), LFG, accinfo);
							}
							else if (iterator.GetData()->Admin() >= 100) 
							{
								iterator.Advance();
								continue;
							}
							else
							{
								sprintf(line, "  %s[ANONYMOUS] %s%s%s", tmpgm, iterator.GetData()->name(), LFG, accinfo);
							}
						}
						else
						{
							sprintf(line, "  %s[%i %s] %s (%s)%s zone: %s%s%s", tmpgm, iterator.GetData()->level(), EQC::Common::GetClassName(iterator.GetData()->class_()), iterator.GetData()->name(), EQC::Common::GetRaceName(iterator.GetData()->race()), tmpguild, iterator.GetData()->zone(), LFG, accinfo);
						}

						connection->SendEmoteMessage(to, 0, 10, line);
						x++;
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}


				if (connection == 0)
				{
					zoneserver_list.SendEmoteMessage(to, 0, 10, "%i players online", x);
				}
				else
				{
					connection->SendEmoteMessage(to, 0, 10, "%i players online", x);
				}

				if (admin >= 100)
				{
					console_list.SendConsoleWho(to, admin, connection);
				}
			}
			void ZSList::SOPZoneBootup(char* adminname, int32 ZoneServerID, char* zonename) 
			{
				ZoneServer* zs = 0;
				ZoneServer* zs2 = 0;
				char* buffer;

				if (!Database::Instance()->GetZoneLongName(zonename, &buffer))
				{
					zoneserver_list.SendEmoteMessage(adminname, 0, 0, "Error: SOP_ZoneBootup: zone '%s' not found in 'zone' table. Typo protection=ON.", zonename);
				}
				else
				{
					if (ZoneServerID != 0)
					{
						zs = zoneserver_list.FindByID(ZoneServerID);
					}
					else
					{
						zoneserver_list.SendEmoteMessage(adminname, 0, 0, "Error: SOP_ZoneBootup: ServerID must be specified");
					}

					if (zs == 0)
					{
						zoneserver_list.SendEmoteMessage(adminname, 0, 0, "Error: SOP_ZoneBootup: zoneserver not found");
					}
					else 
					{
						zs2 = zoneserver_list.FindByName(zonename);
						if (zs2 != 0)
						{
							zoneserver_list.SendEmoteMessage(adminname, 0, 0, "Error: SOP_ZoneBootup: zone '%s' already being hosted by ZoneServer #%i", zonename, zs2->GetID());
						}
						else
						{
							zs->TriggerBootup(zonename, adminname);
						}
					}
				}
				safe_delete_array(buffer);//delete[] buffer;
			}
			int32 ZSList::TriggerBootup(char* zonename)
			{
				LinkedListIterator<ZoneServer*> iterator(list);

				LockMutex lock(&MListLock);
				srand(time(NULL));
				int32 x = 0;
				iterator.Reset();
				while(iterator.MoreElements())
				{
					x++;
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}

				if (x == 0) 
				{
					return 0;
				}

				ZoneServer** tmp = new ZoneServer*[x];
				int32 y = 0;

				iterator.Reset();
				while(iterator.MoreElements())
				{
					if (iterator.GetData()->GetZoneName()[0] == 0 && !iterator.GetData()->IsBootingUp())
					{
						tmp[y++] = iterator.GetData();
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				if (y == 0) 
				{
					safe_delete(tmp);
					return 0;
				}

				int32 z = rand() % y;
				tmp[z]->TriggerBootup(zonename);
				int32 ret = tmp[z]->GetID();

				safe_delete(tmp);
				return ret;
			}
		}
	}
}