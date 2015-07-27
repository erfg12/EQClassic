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
#include <iomanip.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "npc.h"
#ifdef WIN32
#include <windows.h>
#include <winsock.h>

#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#else
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#endif

#include <math.h>
#include <zlib.h>

#include "client.h"
#include "../common/database.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"
#include "worldserver.h"
#include "../common/packet_dump_file.h"
#include "PlayerCorpse.h"
#include "spdat.h"
#include "petitions.h"
#include "NpcAI.h"
#include "skills.h"
#include "forage.h"
#include "object.h"
#include "groups.h"
#include "zone.h"
#include "event_codes.h"
#include "faction.h"
#include "doors.h"

extern Database database;
extern Zone* zone;
extern volatile bool ZoneLoaded;
extern WorldServer worldserver;
extern GuildRanks_Struct guilds[512];
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern PetitionList petition_list;
extern EntityList entity_list;

int glob=0;
//#define DEBUG 9

int Client::HandlePacket(const APPLAYER *app)
{
		
bool ret = true;

		if (app->opcode == 0) {
			delete app;
    		return true;
		}

#if 0
	if (app->opcode == 0x7642) {
#if DEBUG == 9
		LogFile->write(EQEMuLog::Debug, "Compressed packet recieved");
		DumpPacket(app);
		cout << "\n";
#endif
		z_stream t;
		APPLAYER *tmper,*tmp2;
		tmper = new APPLAYER(0, 1024);

		if (app->pBuffer[0] == 0xff)
		{
#if DEBUG == 9
			LogFile->write(EQEMuLog::Debug, "Compressed packet: 0xFF found");
#endif
			t.next_in = app->pBuffer + 5;
			t.avail_in = app->size - 5;
		}
		else
		{
#if DEBUG == 9
			LogFile->write(EQEMuLog::Debug, "Compressed packet: 0xFF not found");
#endif
			t.next_in = app->pBuffer + 3;
			t.avail_in = app->size - 3;
		}

		t.next_out = tmper->pBuffer;
		t.avail_out = tmper->size;
		t.zalloc = (alloc_func)0;
		t.zfree = (free_func)0;
		
		inflateInit(&t);
		inflate(&t, Z_FINISH);

#if DEBUG == 9
		LogFile->write(EQEMuLog::Debug, "Compressed packet: total_out:%i", t.total_out);
#endif

		int16 *intptr   = (int16*) app->pBuffer;

		int16 OpCode = ntohs(*intptr); 

		tmp2 = new APPLAYER(OpCode, t.total_out);
		memcpy(tmp2->pBuffer, tmper->pBuffer, t.total_out);
		inflateEnd(&t);
#if DEBUG >= 1
		cout << "Unknown opcode: 0x" << hex << setfill('0') << setw(4) << tmp2->opcode << dec;
		cout << " size:" << tmp2->size << endl;
		DumpPacket(tmp2);
#endif

		// This will push the packet onto the stack for processing
		HandlePacket(tmp2);
		safe_delete(tmp2);
		safe_delete(tmper);
#if DEBUG == 9
		cout << "END DECOMPRESSION\n";
#endif
		return true;
	}
#endif
		switch(client_state)
		{
		case CLIENT_CONNECTING1:
			{
				this->IsOnBoat=false;
				if(app->opcode == 0x4141)
				{
					for(int i=0;i<4;i++)
						app->pBuffer[i] = 0;

					QueuePacket(app);
				}




				if (app->opcode == OP_SetDataRate)
				{
					if (app->size != sizeof(float)) {
						LogFile->write(EQEMuLog::Error,"Wrong size on OP_SetDatarate. Got: %i, Expected: %i", app->size, sizeof(float));
						break;
					}
					LogFile->write(EQEMuLog::Debug, "HandlePacket() OP_SetDataRate request : %f",  *(float*) app->pBuffer);
					float tmpDR = *(float*) app->pBuffer;
					if (tmpDR <= 3.0f) {
						LogFile->write(EQEMuLog::Error,"HandlePacket() OP_SetDataRate INVALID request : %f <= 0", tmpDR);
						LogFile->write(EQEMuLog::Normal,"WARNING: Setting datarate for client to 5.0 expect a client lock up =(");
						tmpDR = 5.0f;
					}
					if (tmpDR > 10.0f)
						tmpDR = 10.0f;
					eqnc->SetDataRate(tmpDR);
					client_state = CLIENT_CONNECTING2;
				}
				else {
					LogFile->write(EQEMuLog::Error, "HandlePacket() Opcode error: Unexpected packet during CLIENT_CONNECTING1: opcode: 0x%04x, size: %i", app->opcode, app->size);
#if DEBUG == 9
					cout << "Unexpected packet during CLIENT_CONNECTING1: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
					DumpPacket(app);
					cout<<endl;
#endif

				}
				break;
			}
		case CLIENT_CONNECTING2:
		case CLIENT_CONNECTING2_5: {
			if(app->opcode == 0x4141) {
				QueuePacket(app);
				break;
			}
			// Client wants to set thier datarate again?
			if (app->opcode == OP_SetDataRate)
			{
				if (app->size != sizeof(float)) {
					LogFile->write(EQEMuLog::Error,"Wrong size on OP_SetDatarate. Got: %i, Expected: %i", app->size, sizeof(float));
					break;
				}
				LogFile->write(EQEMuLog::Debug, "HandlePacket() OP_SetDataRate request : %f",  *(float*) app->pBuffer);
				float tmpDR = *(float*) app->pBuffer;
				if (tmpDR <= 0.0f) {
					LogFile->write(EQEMuLog::Error,"HandlePacket() OP_SetDataRate INVALID request : %f <= 0", tmpDR);
					LogFile->write(EQEMuLog::Normal,"WARNING: Setting datarate for client to 5.0 expect a client lock up =(");
					tmpDR = 5.0f;
				}

				if (tmpDR > 25.0f)
					tmpDR = 25.0f;



				eqnc->SetDataRate(tmpDR);
				break;
			}
			if (app->opcode == OP_ZoneEntry && client_state == CLIENT_CONNECTING2) {
				//APPLAYER *outapp;
				LogFile->write(EQEMuLog::Debug,"HandlePacket() Login stage 2");
				//DumpPacket(app);

				// Quagmire - Antighost code
				// tmp var is so the search doesnt find this object
				char tmp[64];
				snprintf(tmp, 64, "%s", (char*)&app->pBuffer[4]);
				Client* client = entity_list.GetClientByName(tmp);
/*				account_id = database.GetAuthentication(name, zone->GetShortName(), ip);
				if (account_id == 0) {
#ifdef _DEBUG
					if (ip == 16777343) {
						LogFile->write(EQEMuLog::Warning, "GetAuthentication = 0, but ignoring for 127.0.0.1 in debug mode.");
						account_id = database.GetAccountIDByChar(name);
					}
					else
#endif
					{
						LogFile->write(EQEMuLog::Warning, "Client dropped: GetAuthentication = 0, n=%s", tmp);
						ret = false; // TODO: Can we tell the client to get lost in a good way
						break;
					}
				}*/
				if (!zone->GetAuth(ip, tmp, &WID, &account_id, &character_id, &admin, lskey, &tellsoff)) {
					LogFile->write(EQEMuLog::Error, "GetAuth() returned false kicking client");
					if (client != 0)
						client->Kick();
					ret = false; // TODO: Can we tell the client to get lost in a good way
					break;
				}
				strcpy(name, tmp);
				if (client != 0) {
					struct in_addr ghost_addr;
					ghost_addr.s_addr = eqnc->GetrIP();

					LogFile->write(EQEMuLog::Error,"Ghosting client: Account ID:%i Name:%s Character:%s IP:%s",
										client->AccountID(), client->AccountName(), client->GetName(), inet_ntoa(ghost_addr));
					client->Kick();  // Bye Bye ghost
				}
				char* query = 0;
				uint32_breakdown workpt;
				workpt.b4() = DBA_b4_Entity;
				workpt.w2_3() = this->GetID();
				workpt.b1() = DBA_b1_Entity_Client_InfoForLogin;
				DBAsyncWork* dbaw = new DBAsyncWork(MTdbafq, workpt, DBAsync::Read);
				dbaw->AddQuery(1, &query, MakeAnyLenString(&query, "SELECT status, name, lsaccount_id, gmspeed FROM account WHERE id=%i", this->account_id));
				dbaw->AddQuery(2, &query, MakeAnyLenString(&query, "SELECT id, profile, zonename, x, y, z, alt_adv, guild, guildrank FROM character_ WHERE id=%i", this->character_id));
				dbaw->AddQuery(3, &query, MakeAnyLenString(&query, "SELECT faction_id,current_value FROM faction_values WHERE char_id = %i", this->character_id));
				if (!(pDBAsyncWorkID = dbasync->AddWork(&dbaw))) {
					delete dbaw;
					LogFile->write(EQEMuLog::Error,"dbasync->AddWork() returned false, client crash");
					ret = false;
					break;
				}
				client_state = CLIENT_CONNECTING2_5;
				break;
			}
			else {
				LogFile->write(EQEMuLog::Error, "HandlePacket() Opcode error: Unexpected packet during %s: opcode: 0x%04x, size: %i", (client_state == CLIENT_CONNECTING2) ? "CLIENT_CONNECTING2":"CLIENT_CONNECTING2_5", app->opcode, app->size);
#if DEBUG == 9
				LogFile->write(EQEMuLog::Error, "Unexpected packet: client_state:%s", (client_state == CLIENT_CONNECTING2) ? "CLIENT_CONNECTING2":"CLIENT_CONNECTING2_5" );
				cout << ": OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
				DumpPacket(app);
#endif
			}
			break;
		}
		case CLIENT_CONNECTING3: {
			if(app->opcode == 0x2841) {
				QueuePacket(app);
				break;
			}
			if(app->opcode == 0x4841) {
				QueuePacket(app);
				break;
			}
			if (app->opcode == 0x5d40) {
				LogFile->write(EQEMuLog::Debug,"HandlePacket() Login stage 3");
				// Here the client sends the character name again..
				// not sure why, nothing else in this packet
				//SendInventoryItems();


				APPLAYER* outapp;
				outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
				memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
				NewZone_Struct* nza = (NewZone_Struct*)outapp->pBuffer;
				strncpy(nza->char_name,this->GetName(),64);
				nza->safe_x = zone->safe_x();
				nza->safe_y = zone->safe_y();
				nza->safe_z = zone->safe_z();
#ifdef GUILDWARS2
				if(zone->GetGuildOwned() != 0)
				strncpy(nza->zone_long_name,zone->GetCityName(),272);
#endif
				/*	strncpy(nza->zone_short_name,zone->GetShortName(),32);
				strncpy(nza->zone_long_name,zone->GetLongName(),272);
				nza->unknown230[0] = 0; nza->unknown230[1] = 0; nza->unknown230[2] = 0; nza->unknown230[3] = 0;
				nza->unknown230[4] = 0; nza->unknown230[5] = 0; nza->unknown230[7] = 0xE6; nza->unknown230[8] = 0xE6;
				nza->unknown230[9] = 0xE6; nza->unknown230[10] = 0xE6; nza->unknown230[10] = 0xE6; nza->unknown230[11] = 0xC8;
				nza->unknown230[12] = 0xC8; nza->unknown230[13] = 0xC8; nza->unknown230[14] = 0xC8; nza->unknown230[15] = 0xC8;
				nza->unknown230[16] = 0xC8; nza->unknown230[17] = 0xC8; nza->unknown230[18] = 0xC8;
				nza->unknown230[22] = 0x20; nza->unknown230[23] = 0x41; nza->unknown230[26] = 0x20; nza->unknown230[27] = 0x41;
				nza->unknown230[30] = 0x20; nza->unknown230[31] = 0x41; nza->unknown230[34] = 0x20; nza->unknown230[35] = 0x41;
				nza->unknown230[38] = 0xE1; nza->unknown230[39] = 0x43; nza->unknown230[42] = 0xE1; nza->unknown230[43] = 0x43;
				nza->unknown230[46] = 0xE1; nza->unknown230[47] = 0x43;	nza->unknown230[50] = 0xE1; nza->unknown230[51] = 0x43;
				nza->unknown230[52] = 0xCD; nza->unknown230[53] = 0xCC;	nza->unknown230[54] = 0xCC; nza->unknown230[55] = 0x3E;
				nza->unknown230[56] = 0x02; nza->unknown230[57] = 0x0A;	nza->unknown230[58] = 0x0A; nza->unknown230[62] = 0x0A;
				nza->unknown230[64] = 0x0A;*/
				QueuePacket(outapp);
//				DumpPacket(outapp);
				safe_delete(outapp);					
				weapon1 = database.GetItem(pp.inventory[13]);
				weapon2 = database.GetItem(pp.inventory[14]);

				client_state = CLIENT_CONNECTING4;
			}
			else {
				LogFile->write(EQEMuLog::Error, "HandlePacket() Opcode error: Unexpected packet during CLIENT_CONNECTING3: opcode: 0x%04x, size: %i", app->opcode, app->size);
#if DEBUG == 9
				cout << "Unexpected packet during CLIENT_CONNECTING3: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
				DumpPacket(app);
#endif
			}
			break;
		}
		case CLIENT_CONNECTING4: {
			if (admin < 50)
                		pp.gm = 0;
			if (app->opcode == 0x0a40) {
				APPLAYER* outapp = new APPLAYER;
#if DEBUG >= 1
				LogFile->write(EQEMuLog::Debug,"HandlePacket() Login stage 4");
				LogFile->write(EQEMuLog::Debug,"HandlePacket() sending doors");
#endif
	    		if (entity_list.MakeDoorSpawnPacket(outapp)) {
			    	//DumpPacket(outapp2);
				    QueuePacket(outapp);
#if DEBUG >= 1
					LogFile->write(EQEMuLog::Debug,"HandlePacket() send doors SUCCESS");
#endif
    			}
    			else
					LogFile->write(EQEMuLog::Error,"HandlePacket() send doors ERROR no doors to send?");
				safe_delete(outapp);
                
#if DEBUG >= 1
				LogFile->write(EQEMuLog::Debug,"HandlePacket() send objects BEGIN");
#endif
				entity_list.SendZoneObjects(this);
#if DEBUG >= 1
				LogFile->write(EQEMuLog::Debug,"HandlePacket() send objects SUCCESS");
#endif
				if (zone->GetZonePointsRaw()) {
#if DEBUG >= 1
					LogFile->write(EQEMuLog::Debug,"HandlePacket() send zonepoints BEGIN");
#endif
					outapp = new APPLAYER(OP_SendZonepoints, zone->GetZonePointsSize() + 26);
					outapp->pBuffer[0] = zone->GetZonePointsSize()/24;
					memcpy(&outapp->pBuffer[1], zone->GetZonePointsRaw(), zone->GetZonePointsSize());
					QueuePacket(outapp);
					safe_delete(outapp);
#if DEBUG >= 1
					LogFile->write(EQEMuLog::Debug,"HandlePacket() send zonepoints SUCCESS");
#endif
				}

				outapp = new APPLAYER(0xd840); // Unknown
				QueuePacket(outapp);
				safe_delete(outapp);

			/*	outapp = new APPLAYER;
				outapp->opcode = 0x0a40; // Unknown
				outapp->size = 0;
				QueuePacket(outapp);
				delete outapp;*/

				// Inform the client about the world
				//	entity_list.SendZoneSpawns(this);
				//

				client_state = CLIENT_CONNECTING5;
			}
			else if (app->opcode == 0xf540){
			    // FIXME Client sends two spawn apppearance's here for some reason
			}
			else {
				LogFile->write(EQEMuLog::Error, "HandlePacket() Opcode error: Unexpected packet during CLIENT_CONNECTING4: opcode: 0x%04x, size: %i", app->opcode, app->size);
#if DEBUG == 9
				cout << "Unexpected packet during CLIENT_CONNECTING4: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
				DumpPacket(app);
#endif
			}
			break;
		}
			case CLIENT_CONNECTING5:
			{
				if (app->opcode == 0xd840)
				{
					APPLAYER* outapp;
					LogFile->write(EQEMuLog::Debug, "HandlePacket() login stage 5");

				/*	outapp = new APPLAYER(OP_SetServerFilterAck, 68);
					QueuePacket(outapp);
					delete outapp;*/


					SendAAStats();


					outapp = new APPLAYER(0x6f40,sizeof(int8));
					QueuePacket(outapp);
					delete outapp;

					outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
					SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)outapp->pBuffer;
					sa->type = 0x10;			// Is 0x10 used to set the player id?
					sa->parameter = GetID();	// Four bytes for this parameter...
					QueuePacket(outapp);
					delete outapp;
					outapp = new APPLAYER;
					// Inform the world about the client
					CreateSpawnPacket(outapp);
					entity_list.QueueClients(this, outapp, false);
					safe_delete(outapp);

					outapp = new APPLAYER(OP_Stamina, sizeof(Stamina_Struct));
					Stamina_Struct* sta = (Stamina_Struct*)outapp->pBuffer;
					sta->food = pp.hungerlevel;
					sta->water = pp.thirstlevel;
					sta->fatigue = pp.Fatigue;
					QueuePacket(outapp);
					safe_delete(outapp);

					SendAATable();

	/*				outapp = new APPLAYER;
					outapp->opcode = OP_Disciplines;
					outapp->size = sizeof(Discipline_Struct);
					outapp->pBuffer = new uchar[outapp->size];
					Discipline_Struct* ds = (Discipline_Struct*)outapp->pBuffer;
					ds->unknown0000[0] = 0x0A;
					strncpy(ds->charname,GetName(),64);
					strncpy(ds->charname2,GetName(),64);
					QueuePacket(outapp);
					delete outapp;*/
					// Guild ID is in sze now, rank still using this tho
					if (guilddbid != 0) {
						outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
						SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)outapp->pBuffer;
						sa->spawn_id = GetID();
						sa->type = 23;
						if (guilds[guildeqid].rank[guildrank].warpeace || guilds[guildeqid].leader == account_id)
							sa->parameter = 2;
						else if (guilds[guildeqid].rank[guildrank].invite || guilds[guildeqid].rank[guildrank].remove || guilds[guildeqid].rank[guildrank].motd)
							sa->parameter = 1;
						else
							sa->parameter = 0;
						QueuePacket(outapp);



						delete outapp;
					}

					//PP PVP flag doesn't work, setting it manually here...
					if(GetPVP()) {
						SendAppearancePacket(4, 1);
					}

					position_timer->Start();
					hpregen_timer->Start();
					SetDuelTarget(0);
					SetDueling(false);

                    for ( int i = 0; i < BUFF_COUNT; i++)
                    {
                        if (buffs[i].spellid != 0xFFFF)
                        {
                            SpellEffect(NULL, buffs[i].spellid, buffs[i].casterlevel, buffs[i].ticsremaining);
                        }
                    }
					if (pp.platinum_bank<0)
						SendClientMoneyUpdate(3,(pp.platinum_bank*-1));
					if (pp.gold_bank<0)
						SendClientMoneyUpdate(2,(pp.gold_bank*-1));
					if (pp.silver_bank<0)
						SendClientMoneyUpdate(1,(pp.silver_bank*-1));
					if (pp.copper_bank<0)
						SendClientMoneyUpdate(0,(pp.copper_bank*-1));

					if (pp.platinum<0)
						SendClientMoneyUpdate(3,(pp.platinum*-1));
					if (pp.gold<0)
						SendClientMoneyUpdate(2,(pp.gold*-1));
					if (pp.silver<0)
						SendClientMoneyUpdate(1,(pp.silver*-1));
					if (pp.copper<0)
						SendClientMoneyUpdate(0,(pp.copper*-1));
                    // Sets GM Flag if needed & Sends Petition Queue
					UpdateAdmin(false);

					outapp = new APPLAYER(0xd840); // Unknown
					QueuePacket(outapp);
					safe_delete(outapp);
					tradeitems[0]=NULL;
					UpdateWho();
					// We are now fully connected and ready to play


#ifdef GUILDWARS2
					int32 guildid = database.GetCityGuildOwned(0,zone->GetZoneID());

					if(guildid != 0)
					{
					if(GuildDBID() == 0)
					{
					Message(2,"Welcome to the city of %s, the guild %s is currently in control of this city.",zone->GetCityName(),guilds[guildid].name);
					}
					else if(guildid == GuildDBID())
					{
					Message(10,"Welcome to the city of %s, your guild is currently in control of this city.",zone->GetCityName());
					}
					else if(guildid == 3)
					{
					Message(2,"Welcome to the city of %s, this city is currently neutral but will defend itself if a guild attempts to seize control.",zone->GetCityName());
					}
					else if(database.GetGuildAlliance(GuildDBID(),guildid))
					{
					Message(4,"Welcome to the city of %s, your ally, %s is currently in control of this city.",zone->GetCityName(),guilds[guildid].name);
					}
					else
					{
					Message(13,"Welcome to the city of %s, the guild %s is currently in control of this city and is not your ally, be cautious.",zone->GetCityName(),guilds[guildid].name);
					}
					}
					if(GuildDBID())
					{
					int8 total = database.TotalCitiesOwned(GuildDBID());
					if(total == 0)
						Message(13,"Your guild currently owns no cities and you therefor receive no damage bonuses.");
					else if(total == 1)
					{
						Message(4,"Your guild currently owns %i city, each player in your guild currently receives a %f percent melee damage bonus and a %f percent casting damage bonus.",total,((float)total/55*100),((float)total/85*100));
					}
					else
					{
						Message(4,"Your guild currently owns %i cities, each player in your guild currently receives a %f percent melee damage bonus and a %f percent casting damage bonus.",total,((float)total/55*100),((float)total/85*100));
					}
					}
#endif

					client_state = CLIENT_CONNECTED;
				}
				else {
					LogFile->write(EQEMuLog::Error, "HandlePacket() Opcode error: Unexpected packet during CLIENT_CONNECTING5: opcode: 0x%04x, size: %i", app->opcode, app->size);
#if DEBUG == 9
					cout << "Unexpected packet during CLIENT_CONNECTING5: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
					DumpPacket(app);
#endif
				}
				break;
			}
			case CLIENT_CONNECTED:
				{
					adverrorinfo = app->opcode;
#if DEBUG == 9
					LogFile->write(EQEMuLog::Debug,"HandlePacket() OPCODE debug enabled about to process");
					cerr << "OPCODE: " << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
					DumpPacket(app);
#endif
					//cout << "OPCODE: " << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
					//DumpPacket(app);
					switch(app->opcode)
					{
					case OP_AutoAttack: {
						if (app->size == 4)	{
							if (app->pBuffer[0] == 0) {
								auto_attack = false;
								if (IsAIControlled())
									break;
								attack_timer->Disable();
								attack_timer_dw->Disable();
							}
							else if (app->pBuffer[0] == 1) {
								auto_attack = true;
								if (IsAIControlled())
									break;
								SetAttackTimer();
							}
						}
						else {
							LogFile->write(EQEMuLog::Error, "OP size error: OP_AutoAttack expected:4 got:%i", app->size);
						}
						break;
					}
					case OP_ClientUpdate: {
						if (IsAIControlled())
							break;
						SpawnPositionUpdate_Struct* cu=(SpawnPositionUpdate_Struct*)app->pBuffer;
						if (app->size == sizeof(SpawnPositionUpdate_Struct))
						{
							//							DumpPacketHex(app);
                            // moved spell interrupts to finishedspell()
							pRunAnimSpeed = cu->anim_type;
							x_pos = (float) cu->x_pos;
							y_pos = (float) cu->y_pos;
							
							z_pos = (float)cu->z_pos / 10;
							if (zone->GetZoneID()==68 && (x_pos>3550 && x_pos<3700 && y_pos>950 && y_pos<1200) && this->IsOnBoat==true){
								printf("Sending him to new zone.\n");
								this->MovePC(69,7529,-2088,4);
								
							}
							if (zone->GetZoneID()==69 && (x_pos>7300 && x_pos<8000 && y_pos>1600 && y_pos<2000) && this->IsOnBoat==true){
								printf("Sending him to new zone.\n");
								this->MovePC(68,3254,1357,11);
							}
							if (zone->GetZoneID()==10 && (x_pos<-1100 && x_pos>-1500 && y_pos>133 && y_pos<400) && this->IsOnBoat==true){
								printf("Sending him to new zone.\n");
								this->MovePC(69,-9197,269,3);
							}
							if (zone->GetZoneID()==69 && (x_pos>-8700 && x_pos<-8500 && y_pos>-5100 && y_pos<-4000) && this->IsOnBoat==true){
								printf("Sending him to new zone.\n");
								this->MovePC(10,-1033,-13,-50);
							}
							delta_x = cu->delta_x/125;

							delta_y = cu->delta_y/125;
							delta_z = cu->delta_z;
							heading = cu->heading;
							delta_heading = cu->delta_heading;
							/*
							cout << "X:"  << x_pos << " ";
							cout << "Y:"  << y_pos << " ";
							cout << "Z:"  << z_pos << " ";
							cout << "dX:" << delta_x << " ";
							cout << "dY:" << delta_y << " ";
							cout << "dZ:" << delta_z << " ";
							cout << "H:"  << (int)heading << " ";
							cout << "dH:" << (int)delta_heading << " ";
							cout << "Heading hex 0x" << hex << (int)heading << dec;




									cout << endl;
							*/

							//							SendPosUpdate(); // TODO: Move this outside this case
						}
						else {
							LogFile->write(EQEMuLog::Error, "OP size error: OP_ClientUpdate expected:%i got:%i", sizeof(SpawnPositionUpdate_Struct), app->size);
						}
						break;
					}
					case OP_ClientTarget: {
						if (app->size != sizeof(ClientTarget_Struct)) {
							LogFile->write(EQEMuLog::Error, "OP size error: OP_ClientTarget expected:%i got:%i", sizeof(ClientTarget_Struct), app->size);
							break;
						}
						ClientTarget_Struct* ct=(ClientTarget_Struct*)app->pBuffer;
						pClientSideTarget = ct->new_target;
						if (IsAIControlled())
							break;
						target = entity_list.GetMob(ct->new_target);
						break;
					}
					case OP_Jump: {
						//						cout << name << " jumped." << endl;
                        // neotokyo: here we could reduce fatigue, if we knew how
						pp.Fatigue += 10;
						if(pp.Fatigue > 100)
							pp.Fatigue = 100;
						break;
					}
                    case OP_Consume:
                    {
						if (app->size != sizeof(Consume_Struct))
                        {
							LogFile->write(EQEMuLog::Error, "OP size error: OP_Consume expected:%i got:%i", sizeof(Consume_Struct), app->size);
							break;
                        }

                        struct Consume_Struct *pcs = (struct Consume_Struct *)app->pBuffer;

                        if (pcs->type == 0x01)
                        {
                            // not used - database.GetItem( pp.inventory[pcs->slot] );
                            pp.invitemproperties[pcs->slot].charges--;
                            if (pp.invitemproperties[pcs->slot].charges <= 0)
                            {
#if DEBUG >= 1
                              LogFile->write(EQEMuLog::Debug, "Eating from slot:%i", (int)pcs->slot);
#endif
                                pp.inventory[pcs->slot] = 0;
                            }
                            // 6000 is the max. value
                            pp.hungerlevel += 1000;
                            // food
                        }
                        else if (pcs->type == 0x02)
                        {
                            // not used - database.GetItem( pp.inventory[pcs->slot] );
                            pp.invitemproperties[pcs->slot].charges--;
                            if (pp.invitemproperties[pcs->slot].charges <= 0)
                            {
#if DEBUG >= 1
                              LogFile->write(EQEMuLog::Debug, "Drinking from slot:%i", (int)pcs->slot);
#endif
                                pp.inventory[pcs->slot] = 0;
                            }
                            // 6000 is the max. value
                            pp.thirstlevel += 1000;
                            // water
                        }
                        else
                        {

                            LogFile->write(EQEMuLog::Error, "OP_Consume: unknown type, type:%i", (int)pcs->type);
                            break;
                        }
                        DumpPacket(app);

                        APPLAYER *outapp;
                        outapp = new APPLAYER(OP_Stamina, sizeof(Stamina_Struct));
	    		        Stamina_Struct* sta = (Stamina_Struct*)outapp->pBuffer;
    		        	sta->food = pp.hungerlevel;
        	    		sta->water = pp.thirstlevel;
	            		sta->fatigue = pp.Fatigue;
	    	        	QueuePacket(outapp);
    			        delete outapp;

                        break;
								  }
					case OP_ConsiderCorpse:
                    {
                        //DumpPacket(app);
                        if (app->size == 24)
                        {
                            //Message(10, "This corpse regards you indifferently. What would you like your tombstone to say?");
                            //Message(10, "Oops - wrong message: This corpse will decay soon.");
                            Consider_Struct* conin = (Consider_Struct*)app->pBuffer;
                            Corpse* tcorpse = entity_list.GetCorpseByID(conin->targetid);
                            if (tcorpse && tcorpse->IsNPCCorpse()) {
                                Message(10, "This corpse regards you sadly. Noone asked what i wanted my tombstone to say!");
                                int32 min; int32 sec; int32 ttime;
                                if ((ttime = tcorpse->GetDecayTime()) != 0) {
                                sec = (ttime/1000)%60; // Total seconds
                                min = (ttime/60000)%60; // Total seconds / 60 drop .00
                                Message(10,  "This corpse will decay in %i minutes, %i seconds.", min, sec);
                                }
                                else {
                                Message(10,  "This corpse is waiting to expire.");
                                }

                            }
                            if (tcorpse && tcorpse->IsPlayerCorpse()) {
                                Message(10, "This corpse glares at you threateningly! I told you that wasn't going to work");
                                int32 min; int32 sec; int32 ttime;
                                if ((ttime = tcorpse->GetDecayTime()) != 0) {
                                sec = (ttime/1000)%60; // Total seconds
                                min = (ttime/60000)%60; // Total seconds / 60 drop .00
                                Message(10,  "This corpse will decay in %i minutes, %i seconds.", min, sec);
                                }
                                else {
                                Message(10,  "This corpse is waiting to expire.");
                                }
                                // FIXME show rez timer
                            }
                        }
                        break;
                    }
					case OP_Consider:     //03/09   -Merkur
					{
						Consider_Struct* conin = (Consider_Struct*)app->pBuffer;
						//if (conin->targetid == GetID())
						//	break;		// don't consider yourself
                        // actually you can consider yourself
						Mob* tmob = entity_list.GetMob(conin->targetid);
						if (tmob == 0)
							break;
						APPLAYER* outapp = new APPLAYER(OP_Consider, sizeof(Consider_Struct));
						Consider_Struct* con = (Consider_Struct*)outapp->pBuffer;
						con->playerid = GetID();
						con->targetid = conin->targetid;
						if(tmob->IsNPC())
							con->faction = GetFactionLevel(character_id,tmob->GetNPCTypeID(), race, class_, deity,(tmob->IsNPC()) ? tmob->CastToNPC()->GetPrimaryFaction():0, tmob); // rembrant, Dec. 20, 2001; TODO: Send the players proper deity
						else
							con->faction = 1;
#ifdef GUILDWARS
						if(tmob->IsClient()) {
							if((tmob->CastToClient()->GuildDBID() == GuildDBID()) || (database.GetGuildAlliance(tmob->CastToClient()->GuildDBID(),GuildDBID())))
								con->faction = FACTION_ALLY;
							else
								con->faction = FACTION_SCOWLS;
						}
						else if(tmob->IsNPC() && tmob->CastToNPC()->GetGuildOwner() != 0) {
							if((tmob->CastToNPC()->GetGuildOwner() == GuildDBID()) || (database.GetGuildAlliance(tmob->CastToNPC()->GetGuildOwner(),GuildDBID())))
								con->faction = FACTION_ALLY;
							else
								con->faction = FACTION_SCOWLS;
						}
						else if(tmob->IsNPC() && tmob->CastToNPC()->GetGuildOwner() == 3)
							con->faction = FACTION_INDIFFERENT;
#endif
						con->level = GetLevelCon(tmob->GetLevel());
						if(database.GetServerType() == 1) {
							if (!tmob->IsNPC() )
								con->pvpcon = tmob->CastToClient()->GetPVP();
						}
						QueuePacket(outapp);
/*
						Message(0, "Starting debug: %i", con->unknown3);
						con->unknown3 = 0;
						for (int fred = 0; fred <= 127; fred++){
							con->unknown3++;
							Message(0, "val:%i", con->unknown3);
							QueuePacket(outapp);
							Sleep(50);
						}
						Message(0, "Ending debug");
*/
						delete outapp;
						break;
					}
					case OP_Surname: {
						Surname_Struct* surname = (Surname_Struct*) app->pBuffer;
						if (strstr(surname->lastname, " ") != 0) {
							Message(13, "Surnames may not contain spaces");
							break;
						}
						ChangeLastName(surname->lastname);

						APPLAYER* outapp = new APPLAYER(OP_GMLastName, sizeof(GMLastName_Struct));
						GMLastName_Struct* lnc = (GMLastName_Struct*) outapp->pBuffer;
						strcpy(lnc->name, surname->name);
						strcpy(lnc->lastname, surname->lastname);
						strcpy(lnc->gmname, "SurnameOP");
						lnc->unknown[0] = 1;
						lnc->unknown[1] = 1;
						entity_list.QueueClients(this, outapp, false);
						delete outapp;

						outapp = app->Copy();
						surname = (Surname_Struct*) outapp->pBuffer;
						surname->s_unknown1 = 0;
						surname->s_unknown2 = 1;
						surname->s_unknown3 = 1;
						surname->s_unknown4 = 1;
						FastQueuePacket(&outapp);
						break;
					}
					case OP_YellForHelp: { // Pyro
						entity_list.QueueCloseClients(this,app, true, 100.0);
						// TODO: write a msg send routine
						break;
					}
					case OP_SafePoint: { // Pyro
						//cout << name << "is beneath the world. Packet:" << endl;
						//DumpPacket(app);
						QueuePacket(app);
						//this->GMMove(100,0,0,0);
						//Underworld_Struct* uw = (Underworld_Struct*)app;
						//cout << name << " is beneath the world at " << uw->x << " " << uw->y << " " << uw->z << ". Speed=" << uw->speed << endl;
						// This is handled clientside
						break;
					}
					case OP_Assist: {
						if (app->size != sizeof(int16)) {
							break;
						}
						GlobalID_Struct* gid = (GlobalID_Struct*) app->pBuffer;
						Entity* entity = entity_list.GetID(gid->entity_id);


						APPLAYER* outapp = app->Copy();
						gid = (GlobalID_Struct*) outapp->pBuffer;

						if(entity->IsClient() && entity->CastToMob()->GetTarget() != 0)
							gid->entity_id =entity->CastToMob()->GetTarget()->GetID();

						FastQueuePacket(&outapp);
						break;
					}
					case OP_GMTraining: {
						APPLAYER* outapp = app->Copy();
						GMTrainee_Struct* gmtrain = (GMTrainee_Struct*) outapp->pBuffer;

						for(int tey = 0; tey < 148; tey += 2)
							gmtrain->unknown000[tey]=0xFA;
						FastQueuePacket(&outapp);
						Entity * pTrainer = entity_list.GetID(gmtrain->npcid);

			                        if (pTrainer && pTrainer->IsNPC()) {
						    outapp = new APPLAYER(OP_CommonMessage, sizeof(CommonMessage_Struct)+strlen(pTrainer->GetName())+strlen(GetName())+7);
						    CommonMessage_Struct *p = (CommonMessage_Struct*) outapp->pBuffer;
                        			    p->unknown[0] = 0x0000;
                        			    p->unknown[1] = 0x022a;
                        			    p->unknown[2] = 0x000a;
						    int length = 0;
                            			    strcpy(p->rest, pTrainer->GetName());
						    length+=(1+strlen(pTrainer->GetName()));

							sprintf(&p->rest[length], "%d", 1204+rand()%4);
                            			    length+=5;
						    strcpy(&p->rest[length], GetName());
                        			    QueuePacket(outapp);
                        			    delete outapp;
                    				}
                    				break;
					}
					case OP_GMEndTraining: {
					    GMTrainEnd_Struct* p1 = (GMTrainEnd_Struct*) app->pBuffer;
					    Entity * pTrainer = entity_list.GetID(p1->npcid);
					    if (pTrainer && pTrainer->IsNPC())
 {
						APPLAYER* outapp = new APPLAYER;
                            			outapp->pBuffer = new uchar[200];
                            			CommonMessage_Struct *p = (CommonMessage_Struct*) outapp->pBuffer;
                            			memset(outapp->pBuffer, 0x00, 200);
                            			p->unknown[0] = 0x0000;
                            			p->unknown[1] = 0x022a;
                            			p->unknown[2] = 0x000a;
                            			outapp->size = sizeof(CommonMessage_Struct);
                            			outapp->opcode = OP_CommonMessage;

                            			int length = 0;
                            			if (strlen(pTrainer->GetName()) < 15)
                            			{
                                		    strcpy(p->rest, pTrainer->GetName());
                                		    length = 16;
                            			}
                            			else
                            			{
                                		    strcpy(p->rest, pTrainer->GetName());
                                		    length = strlen(pTrainer->GetName()) + 1;
                            			}
                            			outapp->size += length;
                            			sprintf(&p->rest[length], "%d", 1208+rand()%4);
                            			outapp->size += 5;
                            			length += 5;

                            			if (strlen(GetName()) < 15)
                            			{
                                		    strcpy(&p->rest[length], GetName());
                                		    length = 15;
                            			}
                            			else
                            			{
                                		    strcpy(&p->rest[length], GetName());
                                		    length = strlen(GetName()) + 1;
                            			}
                            			outapp->size += length;
                            			QueuePacket(outapp);
                            			outapp->opcode = OP_4441;
                            			outapp->size = 0;
                            			QueuePacket(outapp);
                            			delete outapp;
                        		    }
					    break;
					}
					case OP_GMTrainSkill:
					{
						GMSkillChange_Struct* gmskill = (GMSkillChange_Struct*) app->pBuffer;
						if (gmskill->skillbank == 0x01) {
							// languages go here
							if (gmskill->skill_id > 25) {
								LogFile->write(EQEMuLog::Error, "Wrong Training Skill (languages) client=%s", GetName());
								DumpPacket(app);
								break;
								}
								//cout << "Training language: " << gmskill->skill_id << endl;
								IncreaseLanguageSkill(gmskill->skill_id);
								pp.points--;
						}

						else if (gmskill->skillbank == 0x00) {
						// normal skills go here
							if (gmskill->skill_id > 73) {
							    LogFile->write(EQEMuLog::Error, "Wrong Training Skill (abilities) client=%s", GetName());
							    DumpPacket(app);
							    break;
							}
							if ( GetSkill(gmskill->skill_id) == 255) {
								// Client never gets this skill check for gm status or fail
								break;
							}
							else if (GetSkill(gmskill->skill_id) == 254) {



								// Client training new skill for the first time set the skill to level-1
								    int16 t_level = database.GetTrainlevel(GetClass(), gmskill->skill_id);
								    //cout<<"t_level:"<<t_level<<endl;
								    if (t_level == 66 || t_level == 0) {
									break;
								    }
								    pp.skills[gmskill->skill_id+1] = t_level;
							}
							else if (GetSkill(gmskill->skill_id) >= 0 && GetSkill(gmskill->skill_id) <= 251) {
								// Client train a valid skill
								// FIXME If the client doesn't do the "You are more skilled than I" check we should do it here
								IncreaseSkill(gmskill->skill_id);
									/*{ 
									APPLAYER* outapp = new APPLAYER(OP_SkillUpdate, sizeof(SkillUpdate_Struct)); 
									SkillUpdate_Struct* skill = (SkillUpdate_Struct*)outapp->pBuffer; 
									skill->skillId=gmskill->skill_id; 
									skill->value=GetSkill(gmskill->skill_id); 
									QueuePacket(outapp); 
									delete outapp; 
									}*/
							}
							else {
								// Log a warning someones been hacking
								LogFile->write(EQEMuLog::Error, "OP_GMTrainSkill: failed client: %s", GetName());
								break;
							}
							pp.points--;
						}
						break;
					}
					case OP_DuelResponse:
						{

							if(app->size != sizeof(DuelResponse_Struct))
								break;
							DuelResponse_Struct* ds = (DuelResponse_Struct*) app->pBuffer;
							//	DumpPacket(app);
							Entity* entity = entity_list.GetID(ds->target_id);
							Entity* initiator = entity_list.GetID(ds->entity_id);
							if(!entity->IsClient() || !initiator->IsClient())
								break;

							entity->CastToClient()->SetDuelTarget(0);
							entity->CastToClient()->SetDueling(false);
							initiator->CastToClient()->SetDuelTarget(0);
							initiator->CastToClient()->SetDueling(false);
							if(GetID() == initiator->GetID())
								entity->CastToClient()->Message(15,"%s has declined the duel.",initiator->GetName());
							else
								initiator->CastToClient()->Message(15,"%s has declined the duel.",entity->GetName());

							break;

						}
					case OP_DuelResponse2: {
						if(app->size != sizeof(Duel_Struct))
							break;

						Duel_Struct* ds = (Duel_Struct*) app->pBuffer;
						Entity* entity = entity_list.GetID(ds->duel_target);
						Entity* initiator = entity_list.GetID(ds->duel_initiator);

						if (entity && initiator && entity == this && initiator->IsClient()) {
							APPLAYER* outapp = new APPLAYER(OP_RequestDuel, sizeof(Duel_Struct));
							Duel_Struct* ds2 = (Duel_Struct*) outapp->pBuffer;

							ds2->duel_initiator = entity->GetID();
							ds2->duel_target = entity->GetID();
							initiator->CastToClient()->QueuePacket(outapp);

							outapp->opcode = OP_DuelResponse2;
							ds2->duel_initiator = initiator->GetID();
							initiator->CastToClient()->QueuePacket(outapp);

							QueuePacket(outapp);
							SetDueling(true);
							initiator->CastToClient()->SetDueling(true);
							SetDuelTarget(ds->duel_initiator);
							delete outapp;
						}

						break;
					}
					case OP_RequestDuel: {
						if(app->size != sizeof(Duel_Struct))
							break;

						APPLAYER* outapp = app->Copy();
						Duel_Struct* ds = (Duel_Struct*) outapp->pBuffer;
						int16 duel = ds->duel_initiator;
						ds->duel_initiator = ds->duel_target;
						ds->duel_target = duel;
						Entity* entity = entity_list.GetID(ds->duel_target);
						if(GetID() != ds->duel_target && entity->IsClient() && (entity->CastToClient()->IsDueling() && entity->CastToClient()->GetDuelTarget() != 0)) {
							Message(15,"%s has already been challenged in a duel.",entity->GetName());
							break;
						}
						if(IsDueling()) {
							Message(15,"You have already initiated a duel.");
							break;

						}

						if(GetID() != ds->duel_target && entity->IsClient() && GetDuelTarget() == 0 && !IsDueling() && !entity->CastToClient()->IsDueling() && entity->CastToClient()->GetDuelTarget() == 0) {
							SetDuelTarget(ds->duel_target);
							entity->CastToClient()->SetDuelTarget(GetID());
							ds->duel_target = ds->duel_initiator;
							entity->CastToClient()->FastQueuePacket(&outapp);
							entity->CastToClient()->SetDueling(false);
							SetDueling(false);
						}
						else
							delete outapp;

						break;
					}
					case OP_SpawnAppearance: {
							//DumpPacket(app);
						if (app->size != sizeof(SpawnAppearance_Struct)) {
							cout << "Wrong size on OP_SpawnAppearance. Got: " << app->size << ", Expected: " << sizeof(SpawnAppearance_Struct) << endl;
							break;
						}
						SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)app->pBuffer;
						if ((sa->type == 0x03) && (sa->parameter == 0)) { 
							APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
							SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
							sa_out->spawn_id = GetID();
							sa_out->type = 0x03;
							this->invisible = false;
							sa_out->parameter = 0;
							entity_list.QueueClients(this, outapp, true);
							delete outapp;
							/*
							if (sa->parameter == 0) // Visible
								cout << "Visible" << endl;
							else if (sa->parameter == 1) // Invisi
								cout << "Invisi" << endl;
							else cout << "Unknown Invisi" << endl;
							*/
							break;
						}
#if 0
						else if ((sa->type == 0x03) && (sa->parameter == 1)) { 
							APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
							SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
							sa_out->spawn_id = GetID();
							sa_out->type = 0x03;
							this->invisible = true;
							sa_out->parameter = 1;
							entity_list.QueueClients(this, outapp, true);
							delete outapp;
							break;
						}
#endif
						if (sa->type == 0x0e) {
							if (IsAIControlled())
								break;
							switch(sa->parameter)
							{
							case 0x64:
								{
								SetAppearance(0);
								playeraction = 0;
								SetFeigned(false);
								BindWound(this, false, true);
								break;
								}
							case 0x6e:
								{
								SetAppearance(1);
								playeraction = 1;
								InterruptSpell();
								SetFeigned(false);
								BindWound(this, false, true);
								break;
								}
							case 0x6f:
								{
								SetAppearance(2);
								playeraction = 2;
								InterruptSpell();
								SetFeigned(false);
								bardsong = 0;
								bardsong_timer->Disable();
								break;
								}
							case 0x05:
								{
								// Illusion
								break;
								}
							case 0x73:
								{
								SetAppearance(3);
								playeraction = 3;
								InterruptSpell();
								bardsong = 0;
								bardsong_timer->Disable();
								break;
								}
							case 105:
								{
								SetAppearance(4);
								playeraction = 4;
								SetFeigned(false);
								break;
								}
							default:
								{
								cerr << "Client " << name << " unknown apperance " << (int)sa->parameter << endl;
								}
							}
							APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
							SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
							sa_out->spawn_id = GetID();
							sa_out->type = 0x0e;
							sa_out->parameter = sa->parameter;
							entity_list.QueueClients(this, app, true);
							delete outapp;
						}
						else if (sa->type == 0x15) { // For Anon/Roleplay
							if (sa->parameter == 1) { // This is Anon
								LogFile->write(EQEMuLog::Normal, "Client \"%s\" Going Anon", GetName());
								pp.anon = 1;
							}
							else if (sa->parameter == 2 || sa->parameter == 3) { // This is Roleplay, or anon+rp
								LogFile->write(EQEMuLog::Normal, "Client \"%s\" Going Roleplay", GetName());
								pp.anon = 2;

							}
							else if (sa->parameter == 0) { // This is Non-Anon
								LogFile->write(EQEMuLog::Normal, "Client \"%s\" Removing Anon/Roleplay", GetName());
								pp.anon = 0;
							}
							else {
								LogFile->write(EQEMuLog::Error, "Client \"%s\" Unknown Anon/Roleplay %i", GetName(), (int)sa->parameter);
								break;
							}
							/*							APPLAYER* outapp = new APPLAYER;;
							outapp->opcode = OP_SpawnAppearance;
							outapp->size = sizeof(SpawnAppearance_Struct);
							outapp->pBuffer = new uchar[outapp->size];
							memset(outapp->pBuffer, 0, outapp->size);
							SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
							sa_out->spawn_id = GetID();
							sa_out->type = 0x15;
							sa_out->parameter = sa->parameter;*/
							entity_list.QueueClients(this, app, true);
							UpdateWho();
							//							delete outapp;
						}
						else if (sa->type == 0x11 && dead == 0) { // this is the client notifing the server of a hp regen tic
							sint32 hpregen = itembonuses->HPRegen;
							sint32 normalregen = LevelRegen();
							sint32 spellbonuses = CastToMob()->GetSpellHPRegen();

							if (DEBUG>=11) LogFile->write(EQEMuLog::Debug, " Regen tick: for %s - server hp: %d requested hp: %d normal regen: %i bonus regen: %i spell regen: %i", GetName(), GetHP(), sa->parameter,normalregen,hpregen,spellbonuses);
							if(sa->parameter != GetHP()+hpregen+normalregen+spellbonuses+1 && hpregen_timer->Check() ) {
								// Client HP don't match Server HP
								//cout<<"DEBUG: hp mismatch server:"<< GetHP() <<" client:"<<sa->parameter<<endl;
								SetHP(GetHP()+hpregen+normalregen+spellbonuses+1);
							} else if(hpregen_timer->Check()){
							    // Client matchs server
								SetHP(sa->parameter);
							}
							else {
							    // Client sending regen to fast!
							    SetHP(GetHP());
							}
							SendHPUpdate();
						}
						else {
							cout << "Unknown SpawnAppearance type: 0x" << hex << setw(4) << setfill('0') << sa->type << dec << " value: 0x" << hex << setw(8) << setfill('0') << sa->parameter << dec << endl;
						}
						break;
					}
					case OP_Death:
						{
							if(app->size != sizeof(Death_Struct))
								break;

							Death_Struct* ds = (Death_Struct*)app->pBuffer;

							if(GetHP() > 0)
								break;

							Mob* killer = entity_list.GetMob(ds->killer_id);

							Death(killer, ds->damage, ds->spell_id,ds->type);
							break;
						}
					case 0x2921:
						{
							break;
						}
					case OP_MoveCoin:
						{
							MoveCoin_Struct* mc = (MoveCoin_Struct*)app->pBuffer;
							switch(mc->from_slot)
							{
							case 0: // cursor
								{
									switch(mc->cointype1)
									{
									case 0: // copper
										{
											if((cursorcopper - mc->amount) < 0)
												break;
											else
												cursorcopper -= mc->amount;
											break;
										}
									case 1: // silver
										{
											if((cursorsilver - mc->amount) < 0)
												break;
											else
												cursorsilver -= mc->amount;
											break;
										}
									case 2: // gold
										{
											if((cursorgold - mc->amount) < 0)
												break;
											else
												cursorgold -= mc->amount;
											break;
										}
									case 3: // platinum
										{
											if((cursorplatinum - mc->amount) < 0)
												break;
											else
												cursorplatinum -= mc->amount;
											break;
										}
									}
									break;
								}
							case 1: // inventory
								{
									switch (mc->cointype2)
									{
									case 0:
										{
											if ((pp.copper - mc->amount) < 0)
												break;
											else
												pp.copper = pp.copper - mc->amount;
											break;
										}
									case 1:
										{
											if ((pp.silver - mc->amount) < 0)
												break;
											else
												pp.silver = pp.silver - mc->amount;
											break;
										}
									case 2:
										{
											if ((pp.gold - mc->amount) < 0)
												break;
											else
												pp.gold = pp.gold - mc->amount;
											break;
										}
									case 3:
										{
											if ((pp.platinum - mc->amount) < 0)
												break;
											else
												pp.platinum = pp.platinum - mc->amount;
											break;
										}
									}
									break;
								}
							case 2: // bank
								{
									switch (mc->cointype2)
									{
									case 0:
										{
											if ((pp.copper_bank - mc->amount) < 0)
												break;
											else
												pp.copper_bank = pp.copper_bank - mc->amount;
											break;
										}
									case 1:
										{
											if ((pp.silver_bank - mc->amount) < 0)
												break;
											else
												pp.silver_bank = pp.silver_bank - mc->amount;
											break;
										}
									case 2:



										{
											if ((pp.gold_bank - mc->amount) < 0)
												break;
											else
												pp.gold_bank = pp.gold_bank - mc->amount;
											break;
										}
									case 3:
										{
											if ((pp.platinum_bank - mc->amount) < 0)
												break;
											else
												pp.platinum_bank = pp.platinum_bank - mc->amount;
											break;
										}
									}
									break;
								}
								}
								
								switch(mc->to_slot)
								{
								case 0xFFFF: // destroy
									{
										// its gone dont worry about it
										break;
									}
								case 0: // cursor
									{
										switch(mc->cointype2)
										{
										case 0: // copper
											{
												cursorcopper += mc->amount;
												break;
											}
										case 1: // silver
											{
												cursorsilver += mc->amount;
												break;
											}
										case 2: // gold
											{
												cursorgold += mc->amount;
												break;
											}
										case 3: // platinum
											{
												cursorplatinum += mc->amount;
												break;
											}
										}
										break;
									}
								case 1: // inventory
									{
										if (mc->cointype2!=mc->cointype1){
											if (mc->cointype2<mc->cointype1)
											{
												int amount2=mc->cointype1-mc->cointype2;
												amount2=pow(10,amount2);
												mc->amount=mc->amount*amount2;
											}
											else if (mc->cointype2>mc->cointype1){
												int amount2=mc->cointype2-mc->cointype1;
												amount2=pow(10,amount2);
												mc->amount=(int)((mc->amount)/(amount2));
											}
										}
										
										switch (mc->cointype2)
										{
										case 0:
											AddMoneyToPP(mc->amount,0,0,0,false);
											break;
										case 1:
											AddMoneyToPP(0,mc->amount,0,0,false);
											break;
										case 2:
											AddMoneyToPP(0,0,mc->amount,0,false);



											break;
										case 3:
											AddMoneyToPP(0,0,0,mc->amount,false);
											break;
										}
										
										break;
									}
								case 2: // bank
									{
										if (mc->cointype2!=mc->cointype1){
											if (mc->cointype2<mc->cointype1)
											{
												int amount2=mc->cointype1-mc->cointype2;
												amount2=pow(10,amount2);
												mc->amount=mc->amount*amount2;
											}
											else if (mc->cointype2>mc->cointype1){
												int amount2=mc->cointype2-mc->cointype1;
												amount2=pow(10,amount2);
												mc->amount=(int)((mc->amount)/(amount2));
											}
										}
										
										switch (mc->cointype2)
										{
										case 0:
											pp.copper_bank=pp.copper_bank + mc->amount;
											break;
											
										case 1:
											pp.silver_bank=pp.silver_bank + mc->amount;
											break;
										case 2:
											pp.gold_bank=pp.gold_bank + mc->amount;
											break;
										case 3:
											pp.platinum_bank=pp.platinum_bank + mc->amount;
											break;
										}
										
										break;
									}
								case 3: // trade
									{
										Entity* ent = entity_list.GetID(this->TradeWithEnt);
										if (this->TradeWithEnt != 0 && ent && ent->IsClient()){
											switch (mc->cointype1) {
											case 0:
												ent->CastToClient()->tradecp = ent->CastToClient()->tradecp+mc->amount;
												break;
											case 1:
												ent->CastToClient()->tradesp = ent->CastToClient()->tradesp+mc->amount;
												break;
											case 2:
												ent->CastToClient()->tradegp = ent->CastToClient()->tradegp+mc->amount;
												break;
											case 3:
												ent->CastToClient()->tradepp = ent->CastToClient()->tradepp+mc->amount;
												break;
											}
											
											ent->CastToClient()->Message(15,"%s adds some coins to the trade.",this->GetName());
											ent->CastToClient()->Message(15,"The total trade is: %i PP %i GP %i SP %i CP", ent->CastToClient()->tradepp, ent->CastToClient()->tradegp, ent->CastToClient()->tradesp, ent->CastToClient()->tradecp);
										}
										else if(ent->IsNPC())
										{
											switch (mc->cointype1) {
											case 0:
												ent->CastToClient()->tradecp += mc->amount;
												break;
											case 1:
												ent->CastToClient()->tradesp += mc->amount;
												break;
											case 2:
												ent->CastToClient()->tradegp += mc->amount;
												break;
											case 3:
												ent->CastToClient()->tradepp += mc->amount;
												break;
											}
										}
										break;
									}
								}
								break;
						}
					case OP_ItemView:
						{
							if(app->size != sizeof(ItemViewRequest_Struct))
								break;


							ItemViewRequest_Struct* ivrs = (ItemViewRequest_Struct*)app->pBuffer;

							const Item_Struct* item = database.GetItem(ivrs->itm_id);

							if(item == 0)
								break;

							APPLAYER* outapp = new APPLAYER(OP_ItemView, sizeof(ItemViewResponse_Struct));
							memcpy(outapp->pBuffer, item, sizeof(Item_Struct));
							QueuePacket(outapp);
							delete outapp;

							break;
						}
					case OP_MoveItem:
						{
//#define MOVEITEMDEBUG 1
							if (app->size != sizeof(MoveItem_Struct)) {
								cout << "Wrong size on OP_MoveItem. Got: " << app->size << ", Expected: " << sizeof(MoveItem_Struct) << endl;
								break;
							}
							MoveItem_Struct* mi = (MoveItem_Struct*)app->pBuffer;
#if 0
    if ( !MoveItem(mi->to_slot, mi->from_slot, mi->number_in_stack) ) {
//	    BulkSendInventoryItems();
    }
#else
							if(mi->from_slot == 0 && mi->to_slot == 0)//SummonItem
							break;

							int16	itemintoslotitemid = 0xFFFF;
							ItemProperties_Struct	itemintoslotproperties;
							int16	itemintoslotbagitems[10];
							ItemProperties_Struct	itemintoslotbagproperties[10];
							
							int16	iteminfromslotitemid = 0xFFFF;
							ItemProperties_Struct	iteminfromslotproperties;
							int16	iteminfromslotbagitems[10];
							ItemProperties_Struct	iteminfromslotbagproperties[10];
							
							for(int ib=0;ib<10;ib++){
								itemintoslotbagitems[ib] = 0xFFFF;
								memset(&itemintoslotbagproperties[ib],0,sizeof(ItemProperties_Struct));
								iteminfromslotbagitems[ib] = 0xFFFF;
								memset(&iteminfromslotbagproperties[ib],0,sizeof(ItemProperties_Struct));
							}
							
							if(mi->from_slot == 0){ // item is coming from cursor to an inventory, bank, etc.
#ifdef MOVEITEMDEBUG
Message(0,"FromSlot==0;ToSlot==%i;pp.inventory[0]==%i",mi->to_slot,pp.inventory[0]);
#endif
								iteminfromslotitemid = pp.inventory[0];
								memcpy(&iteminfromslotproperties,&pp.invitemproperties[0],sizeof(ItemProperties_Struct));
								
								for(int bag=0;bag<10;bag++){
									int32 currentslot = 80+bag;
									memcpy(&iteminfromslotbagproperties[bag],&pp.bagitemproperties[currentslot],sizeof(ItemProperties_Struct));
									iteminfromslotbagitems[bag] = pp.containerinv[currentslot];
								}
								//taken to pp.inventory[0] pp.invitemproperties[0] and bag information taken to pp.containerinv[80] and pp.bagitemproperties[80]
								if(mi->to_slot >= 1 && mi->to_slot <= 21){ // equipped items
                                    if(!database.GetItem(pp.inventory[mi->from_slot])->IsEquipable(GetRace(),GetClass())){
                                        LogFile->write(EQEMuLog::Error, "%s attempting bad item equip!", GetName());
                                        break; 
                                    }
									itemintoslotitemid = pp.inventory[mi->to_slot];
									memcpy(&itemintoslotproperties,&pp.invitemproperties[mi->to_slot],sizeof(ItemProperties_Struct));
									
									pp.inventory[mi->to_slot] = iteminfromslotitemid;
									memcpy(&pp.invitemproperties[mi->to_slot],&iteminfromslotproperties,sizeof(ItemProperties_Struct));
								}
								else if(mi->to_slot >= 22 && mi->to_slot <= 29){ // main inventory
									itemintoslotitemid = pp.inventory[mi->to_slot];
									memcpy(&itemintoslotproperties,&pp.invitemproperties[mi->to_slot],sizeof(ItemProperties_Struct));
									
									pp.inventory[mi->to_slot] = iteminfromslotitemid;
									memcpy(&pp.invitemproperties[mi->to_slot],&iteminfromslotproperties,sizeof(ItemProperties_Struct));
									for(int bag=0;bag<10;bag++)
									{
										int32 currentslot = ((mi->to_slot-22)*10)+bag;
										memcpy(&itemintoslotbagproperties[bag],&pp.bagitemproperties[currentslot],sizeof(ItemProperties_Struct));
										itemintoslotbagitems[bag] = pp.containerinv[currentslot];
										
										memcpy(&pp.bagitemproperties[currentslot],&iteminfromslotbagproperties[bag],sizeof(ItemProperties_Struct));
										pp.containerinv[currentslot] = iteminfromslotbagitems[bag];
									}
								}
								else if (mi->to_slot >= 250 && mi->to_slot <= 339){ // items in bags
									itemintoslotitemid = pp.containerinv[mi->to_slot-250];
									memcpy(&itemintoslotproperties,&pp.bagitemproperties[mi->to_slot-250],sizeof(ItemProperties_Struct));
									
									pp.containerinv[mi->to_slot-250] = iteminfromslotitemid;
									memcpy(&pp.bagitemproperties[mi->to_slot-250],&iteminfromslotproperties,sizeof(ItemProperties_Struct));
								}
								else if(mi->to_slot >= 2000 && mi->to_slot <= 2007){  // bank
									itemintoslotitemid = pp.bank_inv[mi->to_slot-2000];
									memcpy(&itemintoslotproperties,&pp.bankinvitemproperties[mi->to_slot-2000],sizeof(ItemProperties_Struct));
									
									pp.bank_inv[mi->to_slot-2000] = iteminfromslotitemid;
									memcpy(&pp.bankinvitemproperties[mi->to_slot-2000],&iteminfromslotproperties,sizeof(ItemProperties_Struct));

									for(int bag=0;bag<10;bag++)
									{
										int32 currentslot = ((mi->to_slot-2000)*10)+bag;
										memcpy(&itemintoslotbagproperties[bag],&pp.bankbagitemproperties[currentslot],sizeof(ItemProperties_Struct));
										itemintoslotbagitems[bag] = pp.bank_cont_inv[currentslot];
										
										memcpy(&pp.bankbagitemproperties[currentslot],&iteminfromslotbagproperties[bag],sizeof(ItemProperties_Struct));
										pp.bank_cont_inv[currentslot] = iteminfromslotbagitems[bag];
									}
								}
								else if (mi->to_slot >= 2030 && mi->to_slot <= 2109){
									itemintoslotitemid = pp.bank_cont_inv[mi->to_slot-2030];
									memcpy(&itemintoslotproperties,&pp.bankbagitemproperties[mi->to_slot-2030],sizeof(ItemProperties_Struct));
									
									pp.bank_cont_inv[mi->to_slot-2030] = iteminfromslotitemid;
									memcpy(&pp.bankbagitemproperties[mi->to_slot-2030],&iteminfromslotproperties,sizeof(ItemProperties_Struct));
								}
								else if(mi->to_slot >=3000 && mi->to_slot <= 3016){// Item Trade

								TradeList[mi->to_slot-3000] = GetItemAt(0);
								TradeCharges[mi->to_slot-3000] = (mi->number_in_stack) ? mi->number_in_stack : pp.invitemproperties[0].charges;
								for (int j = 0;j != 10;j++) {
									TradeList[((mi->to_slot-2999)*10)+j] = pp.containerinv[80+j];
									TradeCharges[((mi->to_slot-2999)*10)+j] = pp.bagitemproperties[80+j].charges;
								}
								if (this->TradeWithEnt){
									Client* tradecl = entity_list.GetClientByID(this->TradeWithEnt);
									if (tradecl) {
										const Item_Struct* item = database.GetItem(TradeList[mi->to_slot-3000]);
										if (item) {
											APPLAYER* outapp = new APPLAYER(OP_ItemToTrade,sizeof(ItemToTrade_Struct));
											ItemToTrade_Struct* ti = (ItemToTrade_Struct*)outapp->pBuffer;
											memcpy(&ti->item,item,sizeof(Item_Struct));
											ti->item.common.charges = TradeCharges[mi->to_slot-3000];
											ti->toslot = mi->to_slot-3000;
											ti->playerid = this->TradeWithEnt;
											tradecl->FastQueuePacket(&outapp);
											char itemid[100];
											memset(itemid,0,sizeof(itemid));
											if (strlen(this->tradeitems)>1)
												strcat(this->tradeitems,", ");
											itoa(item->item_nr,itemid,10);
											strcat(this->tradeitems,item->name);
											strcat(this->tradeitems,"(");
											strcat(this->tradeitems,itemid);
											strcat(this->tradeitems,")");
											printf("this->tradeitems: %s\n",this->tradeitems);
										}
									}
								}
								}
								else if(mi->to_slot == 0xFFFFFFFF){ // Remove and from slot must be 0
									pp.inventory[0] = 0xFFFF;
									memset(&pp.invitemproperties[0],0,sizeof(ItemProperties_Struct));
									for(int i=80;i<90;i++)
									{
										pp.containerinv[i] = 0xFFFF;
										memset(&pp.bagitemproperties[i],0,sizeof(ItemProperties_Struct));
									}
								}
								
								if(mi->to_slot != 0xFFFFFFFF){
									pp.inventory[0] = itemintoslotitemid;
									memcpy(&pp.invitemproperties[0],&itemintoslotproperties,sizeof(ItemProperties_Struct));
									
									for(int bag=0;bag<10;bag++){
										int32 currentslot = 80+bag;
										memcpy(&pp.bagitemproperties[currentslot],&itemintoslotbagproperties[bag],sizeof(ItemProperties_Struct));
										pp.containerinv[currentslot] = itemintoslotbagitems[bag];
									}
							        if (GetItemAt(mi->from_slot) == GetItemAt(mi->to_slot) && database.GetItem(GetItemAt(mi->from_slot)) != 0 && database.GetItem(GetItemAt(mi->from_slot))->IsStackable()){
								         if ((GetItemPropAt(mi->from_slot).charges + GetItemPropAt(mi->to_slot).charges) >= 21) {
                                             int temp = (int)GetItemPropAt(mi->from_slot).charges;
								             PutItemInInventory(mi->from_slot, GetItemAt(mi->from_slot), GetItemPropAt(mi->to_slot).charges);
								             PutItemInInventory(mi->to_slot, GetItemAt(mi->from_slot), temp);
								             PutItemInInventory(mi->from_slot, GetItemAt(mi->from_slot), (GetItemPropAt(mi->to_slot).charges+GetItemPropAt(mi->from_slot).charges)-20 );
								             PutItemInInventory(mi->to_slot, GetItemAt(mi->from_slot), 20);
								         }
								         else if ((GetItemPropAt(mi->from_slot).charges + GetItemPropAt(mi->to_slot).charges) == 20) {
								             PutItemInInventory(mi->to_slot, GetItemAt(mi->from_slot), 20);
								             DeleteItemInInventory(mi->from_slot, 0);
								         }
								         else {
                                            PutItemInInventory(mi->to_slot, GetItemAt(mi->to_slot), (GetItemPropAt(mi->from_slot).charges + GetItemPropAt(mi->to_slot).charges));
                                            DeleteItemInInventory(mi->from_slot);
								         }
								    }
								}
							} // End of items going from cursor to inventory

							else if(mi->to_slot == 0) { // Items coming from a slot in inventory, bank, etc. into the cursor
#ifdef MOVEITEMDEBUG
Message(0,"FromSlot==%i;ToSlot==0;pp.inventory[0]==%i",mi->from_slot,pp.inventory[0]);
#endif
								itemintoslotitemid = pp.inventory[0];
								memcpy(&itemintoslotproperties,&pp.invitemproperties[0],sizeof(ItemProperties_Struct));
								
								for(int bag=0;bag<10;bag++){
									int32 currentslot = 80+bag;
									memcpy(&itemintoslotbagproperties[bag],&pp.bagitemproperties[currentslot],sizeof(ItemProperties_Struct));
									itemintoslotbagitems[bag] = pp.containerinv[currentslot];
								}
								if(mi->from_slot >= 1 && mi->from_slot <= 21){// equipped items
									iteminfromslotitemid = pp.inventory[mi->from_slot];
									memcpy(&iteminfromslotproperties,&pp.invitemproperties[mi->from_slot],sizeof(ItemProperties_Struct));
									
									pp.inventory[mi->from_slot] = itemintoslotitemid;
									memcpy(&pp.invitemproperties[mi->from_slot],&itemintoslotproperties,sizeof(ItemProperties_Struct));
								}
								else if(mi->from_slot >= 22 && mi->from_slot <= 29){// main inventory
									iteminfromslotitemid = pp.inventory[mi->from_slot];
									memcpy(&iteminfromslotproperties,&pp.invitemproperties[mi->from_slot],sizeof(ItemProperties_Struct));
									
									pp.inventory[mi->from_slot] = itemintoslotitemid;
									memcpy(&pp.invitemproperties[mi->from_slot],&itemintoslotproperties,sizeof(ItemProperties_Struct));
									for(int bag=0;bag<10;bag++)
									{
										int32 currentslot = ((mi->from_slot-22)*10)+bag;
										memcpy(&iteminfromslotbagproperties[bag],&pp.bagitemproperties[currentslot],sizeof(ItemProperties_Struct));
										iteminfromslotbagitems[bag] = pp.containerinv[currentslot];

										memcpy(&pp.bagitemproperties[currentslot],&itemintoslotbagproperties[bag],sizeof(ItemProperties_Struct));
										pp.containerinv[currentslot] = itemintoslotbagitems[bag];
									}
								}
								else if (mi->from_slot >= 250 && mi->from_slot <= 339) { // items in bags
									iteminfromslotitemid = pp.containerinv[mi->from_slot-250];
									memcpy(&iteminfromslotproperties,&pp.bagitemproperties[mi->from_slot-250],sizeof(ItemProperties_Struct));
									
									pp.containerinv[mi->from_slot-250] = itemintoslotitemid;
									memcpy(&pp.bagitemproperties[mi->from_slot-250],&itemintoslotproperties,sizeof(ItemProperties_Struct));
								}
								else if(mi->from_slot >= 2000 && mi->from_slot <= 2007){ // bank
									iteminfromslotitemid = pp.bank_inv[mi->from_slot-2000];
									memcpy(&iteminfromslotproperties,&pp.bankinvitemproperties[mi->from_slot-2000],sizeof(ItemProperties_Struct));
									
									pp.bank_inv[mi->from_slot-2000] = itemintoslotitemid;
									memcpy(&pp.bankinvitemproperties[mi->from_slot-2000],&itemintoslotproperties,sizeof(ItemProperties_Struct));
									for(int bag=0;bag<10;bag++){
										int32 currentslot = ((mi->from_slot-2000)*10)+bag;
										memcpy(&iteminfromslotbagproperties[bag],&pp.bankbagitemproperties[currentslot],sizeof(ItemProperties_Struct));
										iteminfromslotbagitems[bag] = pp.bank_cont_inv[currentslot];
										
										memcpy(&pp.bankbagitemproperties[currentslot],&itemintoslotbagproperties[bag],sizeof(ItemProperties_Struct));
										pp.bank_cont_inv[currentslot] = itemintoslotbagitems[bag];
									}
								}
								else if (mi->from_slot >= 2030 && mi->from_slot <= 2109){ // items in bank
									iteminfromslotitemid = pp.bank_cont_inv[mi->from_slot-2030];
									memcpy(&iteminfromslotproperties,&pp.bankbagitemproperties[mi->from_slot-2030],sizeof(ItemProperties_Struct));
									
									pp.bank_cont_inv[mi->from_slot-2030] = itemintoslotitemid;
									memcpy(&pp.bankbagitemproperties[mi->from_slot-2030],&itemintoslotproperties,sizeof(ItemProperties_Struct));
								}

								if(mi->to_slot != 0xFFFFFFFF) { // Put this here because bag was already 'declared'
									pp.inventory[0] = iteminfromslotitemid;
									memcpy(&pp.invitemproperties[0],&iteminfromslotproperties,sizeof(ItemProperties_Struct));
									
									for(int bag=0;bag<10;bag++){
										int32 currentslot = 80+bag;
										memcpy(&pp.bagitemproperties[currentslot],&iteminfromslotbagproperties[bag],sizeof(ItemProperties_Struct));
										pp.containerinv[currentslot] = iteminfromslotbagitems[bag];
									}
								}
								if ( mi->number_in_stack && database.GetItem(GetItemAt(mi->to_slot)) != 0 && database.GetItem(GetItemAt(mi->to_slot))->IsStackable()){
                                        PutItemInInventory(mi->from_slot, GetItemAt(mi->to_slot), GetItemPropAt(mi->to_slot).charges);
                                        PutItemInInventory(mi->to_slot, GetItemAt(mi->to_slot), mi->number_in_stack);
                                        DeleteItemInInventory(mi->from_slot, mi->number_in_stack, true);
								}
							}
#endif
							// Always update bonus might be food
								this->CalcBonuses(); // Update item/spell bonuses
							
							if(mi->to_slot == 13 || mi->from_slot == 13) {
								uint16 item_id = pp.inventory[13];
								weapon1 = database.GetItem(item_id);
								SetAttackTimer();
							}
							if (mi->to_slot == 14 || mi->from_slot == 14) {
								uint16 item_id = pp.inventory[14];
								weapon2 = database.GetItem(item_id);
								SetAttackTimer();
							}
							Save();
							break;
					}
					case OP_Camp: {
						if (GetGM() && admin >= 100) {
/*							APPLAYER* outapp;
							outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
							SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)outapp->pBuffer;
							sa->type = 0x10;			// Is 0x10 used to set the player id?
							sa->parameter = 0;	// Four bytes for this parameter...
							QueuePacket(outapp);
							delete outapp;*/
							Save();
							Disconnect();
						}
						// TODO: Implement camp, LD and all that
						camp_timer->Start();
						break;
					}
					case OP_SenseHeading:
					{

						if (rand()%100 <= 15 && (GetSkill(SENSE_HEADING) < 200) && (GetSkill(SENSE_HEADING) < this->GetLevel()*5+5)) {

							this->SetSkill(SENSE_HEADING, GetSkill(SENSE_HEADING) + 1);
						}
						break;
					}

					case OP_FeignDeath:
					{

						int16 primfeign = GetSkill(FEIGN_DEATH);
						int16 secfeign = GetSkill(FEIGN_DEATH);
						if (primfeign > 100)
						{
							primfeign = 100;
							secfeign = secfeign - 100;
							secfeign = secfeign / 2;
						}
						else
							secfeign = 0;

						int16 totalfeign = primfeign + secfeign;
						if (rand()%160 > totalfeign)
						{
							SetFeigned(false);
							entity_list.MessageClose(this,false,200,0,"%s has fallen to the ground.",this->GetName());
						}
						else
						{
							SetFeigned(true);
						}
						if (rand()%300 > GetSkill(FEIGN_DEATH) && rand()%4 == 1 && GetSkill(FEIGN_DEATH) < 200 && GetSkill(FEIGN_DEATH) < GetLevel()*5+5)
						{
						    SetSkill(FEIGN_DEATH, GetSkill(FEIGN_DEATH) + 1);
						}
						break;
					}
					case OP_Sneak:
						{
							float hidechance = (GetSkill(SNEAK)/300.0f) + .25;
							float random = (float)rand()/RAND_MAX;		
							CheckIncreaseSkill(SNEAK,15);					
							// TODO: Implement sneak
							if(random < hidechance) {
							    if (sneaking){
							        Message(0, "You are no longer sneaking!");
							        sneaking = false;
							    }
							    else {
							        Message(0, "You begin to sneak!");
							        sneaking = true;
							        APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
							        SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
							        sa_out->spawn_id = GetID();
							        sa_out->type = 0x03;
							        sa_out->parameter = 1;
							        //entity_list.QueueClients(this, outapp, true);
							        //QueuePacket(outapp);
			                        QueuePacket(outapp);
							        delete outapp;
							    }
                            }
							break;
						}
					case OP_Hide:
						{
							float hidechance = (GetSkill(HIDE)/300.0f) + .25;
							float random = (float)rand()/RAND_MAX;		
							CheckIncreaseSkill(HIDE,15);					
							//Message(4,"Hide not implemented, oh wait yes it is, you ATTEMPT to hide :P");
							if(random < hidechance) {
							//CreateDespawnPacket(&app);
							//entity_list.QueueClientsStatus(this, &app, true, 0, admin-1);
							//entity_list.RemoveFromTargets(this);
							APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
							SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
							sa_out->spawn_id = GetID();
							sa_out->type = 0x03;
							this->invisible = true;
							sa_out->parameter = 1;
							//entity_list.QueueClients(this, outapp, true);
							QueuePacket(outapp);
							delete outapp;
					        if ( target && target->CheckAggro(this->CastToMob()) ) {
							        if ( target->IsNPC() ){
							            target->SetHate(this->CastToMob(), 
							                target->GetHateAmount(this->CastToMob())-(GetSkill(HIDE)+GetLevel()+100),
							                target->GetHateAmount(this->CastToMob())-(GetSkill(HIDE)+GetLevel()+100) );
							        }
							}
							}
							break;
						}
					case OP_ChannelMessage: {
						ChannelMessage_Struct* cm=(ChannelMessage_Struct*)app->pBuffer;
						if (app->size < sizeof(ChannelMessage_Struct)) {
							cout << "Wrong size " << app->size << ", should be " << sizeof(ChannelMessage_Struct) << "+ on 0x" << hex << setfill('0') << setw(4) << app->opcode << dec << endl;
							break;
						}
						if (IsAIControlled()) {
							Message(13, "You try to speak but cant move your mouth!");
							break;

						}

						ChannelMessageReceived(cm->chan_num, cm->language, &cm->message[0], &cm->targetname[0]);
						break;
					}
					case 0x4141: {
						QueuePacket(app);
						break;
					}
					case OP_WearChange: {
						if (app->size != sizeof(WearChange_Struct)) {
							cout << "Wrong size: OP_WearChange, size=" << app->size << ", expected " << sizeof(WearChange_Struct) << endl;
							DumpPacket(app);
							break;
						}
						WearChange_Struct* wc=(WearChange_Struct*)app->pBuffer;
						entity_list.QueueClients(this, app, true);
						break;


					}
					case OP_ZoneChange: {
						if (app->size != sizeof(ZoneChange_Struct)) {

							cout << "Wrong size: OP_ZoneChange, size=" << app->size << ", expected " << sizeof(ZoneChange_Struct) << endl;
							//							DumpPacket(app);
							break;
						}
						if (this->isgrouped && entity_list.GetGroupByClient(this) != 0)
							entity_list.GetGroupByClient(this)->DelMember(this);

						ZoneChange_Struct* zc=(ZoneChange_Struct*)app->pBuffer;
						float tarx = -1, tary = -1, tarz = -1;
						sint16 minstatus = 0;
						int8 minlevel = 0;
						sint8 myerror=ZONE_ERROR_NOTREADY;
						char tarzone[32];
						if (zc->zoneID == 0)
							strcpy(tarzone, zonesummon_name);
						else if (database.GetZoneName(zc->zoneID))
							strcpy(tarzone, database.GetZoneName(zc->zoneID));
						else
							tarzone[0] = 0;
						cout << "Zone request for:" << zc->char_name << " to: " << tarzone << "(" << database.GetZoneID(tarzone) << ")" << endl;

						// this both loads the safe points and does a sanity check on zone name
						if (!database.GetSafePoints(tarzone, &tarx, &tary, &tarz, &minstatus, &minlevel)) {
							tarzone[0] = 0;

						}
						int8 tmpzonesummon_ignorerestrictions = zonesummon_ignorerestrictions;
						if (zonesummon_ignorerestrictions) {
							minstatus = 0;
							minlevel = 0;
						}
						zonesummon_ignorerestrictions = 0;

						cout << "Player at x:" << x_pos << " y:" << y_pos << " z:" << z_pos << endl;
						ZonePoint* zone_point = zone->GetClosestZonePoint(x_pos, y_pos, z_pos, zc->zoneID);

						tarx=zonesummon_x;
						tary=zonesummon_y;
						tarz=zonesummon_z;

						// -1, -1, -1 = code for zone safe point
						if ((x_pos == -1 && y_pos == -1 && (z_pos == -1 || z_pos == -10)) ||
							(zonesummon_x == -1 && zonesummon_y == -1 && (zonesummon_z == -1 || zonesummon_z == -10))) {
							cout << "Zoning to safe coords: " << tarzone << " (" << database.GetZoneID(tarzone) << ")" << endl;
							tarx=database.GetSafePoint(tarzone,"x");
							tary=database.GetSafePoint(tarzone,"y");
							tarz=database.GetSafePoint(tarzone,"z");
							zonesummon_x = -2;
							zonesummon_y = -2;
							zonesummon_z = -2;
						} // -3 -3 -3 = bind point
						else if (zonesummon_x == -3 && zonesummon_y == -3 && (zonesummon_z == -3 || zonesummon_z == -30) && database.GetZoneName(pp.bind_point_zone)) {
							strcpy(tarzone, database.GetZoneName(pp.bind_point_zone)); //copy bindzone to target
							tarx=pp.bind_location[0][0]; //copy bind cords to target
							tary=pp.bind_location[1][0];
							tarz=pp.bind_location[2][0];
							cout << "Zoning: Death, zoning to bind point: " << tarzone << " (" << database.GetZoneID(tarzone) << ")" << ", x=" << tarx << ", y=" << tary << ", z=" << tarz << endl;
							zonesummon_x = -2;
							zonesummon_y = -2;
							zonesummon_z = -2;
							minstatus = 0;
							minlevel = 0;
						}
						else if (zone_point != 0) {
							cout << "Zone point found at x:" << zone_point->x << " y:" << zone_point->y << " z:" << zone_point->z << endl;
							tarx = zone_point->target_x;
							tary = zone_point->target_y;
							tarz = zone_point->target_z;
						} // if not -2 -2 -2, zone to these coords. -2, -2, -2 = not a zonesummon zonerequest
						else if (!(zonesummon_x == -2 && zonesummon_y == -2 && (zonesummon_z == -2 || zonesummon_z == -20))) {
							tarx = zonesummon_x;
							tary = zonesummon_y;
							tarz = zonesummon_z;
							cout << "Zoning to specified cords: " << tarzone << " (" << database.GetZoneID(tarzone) << ")" << ", x=" << tarx << ", y=" << tary << ", z=" << tarz << endl;
							zonesummon_x = -2;
							zonesummon_y = -2;
							zonesummon_z = -2;
						}
						else
						{
							cout << "WARNING: No target coords for this zone in DB found" << endl;
							cout << "Zoning to safe coords: " << tarzone << " (" << database.GetZoneID(tarzone) << ")" << ", x=" << tarx << ", y=" << tary << ", z=" << tarz << endl;
							tarx=-1;
							tary=-1;
							tarz=-1;
							zonesummon_x = -2;
							zonesummon_y = -2;
							zonesummon_z = -2;
						}
						/*else
						{

						cout << "Zoning to coords x = " << tarx << "; y = " << tary << "; z = " << tarz << endl;
										}*/

						if (admin < minstatus || GetLevel() < minlevel)
							myerror = ZONE_ERROR_NOEXPERIENCE;


						//Message(13, "Zone request to %s (%i).", tarzone, database.GetZoneID(tarzone));

						APPLAYER* outapp;
						if (tarzone[0] != 0 && admin >= minstatus && GetLevel() >= minlevel) {
							UpdateWho(1);
							cout << "Zoning player to:" << tarzone << " (" << database.GetZoneID(tarzone) << ")" << " x:" << tarx << " y:" << tary << " z:" << tarz << endl;

							/*				int8 blah[4] = { 0x4E,0x01,0x05,0x06 };
							outapp = new APPLAYER(0xdb20, 4);
							memcpy(outapp->pBuffer, blah, 4);
							QueuePacket(outapp);
							delete outapp;

							  outapp = new APPLAYER;
							  outapp->opcode = 0x1020;
							  outapp->size = 0;
							  QueuePacket(outapp);
							delete outapp;*/

							/*		  outapp = new APPLAYER(OP_DeleteSpawn, sizeof(DeleteSpawn_Struct));
							DeleteSpawn_Struct *ds = (DeleteSpawn_Struct*)outapp->pBuffer;
							ds->spawn_id = GetID();
							QueuePacket(outapp);
							delete outapp;*/

							//outapp = new APPLAYER(OP_ZoneChange, sizeof(ZoneChange_Struct));
							//ZoneChange_Struct *zc2 = (ZoneChange_Struct*)app->pBuffer;
							//strcpy(zc2->char_name, zc->char_name);
							//zc2->zoneID = database.GetZoneID(tarzone);

							//zc->success = 1;
							//memset(zc2->unknown0073,0,sizeof(zc2->unknown0073));

							// The client seems to dump this profile on us, but I ignore it for now. Saving is client initiated?
							x_pos = tarx; // Hmm, these coordinates will now be saved when ~client is called
							y_pos = tary;
							z_pos = tarz;
							pp.current_zone = database.GetZoneID(tarzone);
							Save();

//							database.SetAuthentication(account_id, zc->char_name, tarzone, ip); // We have to tell the world server somehow?
							if (pp.current_zone == zone->GetZoneID()) {
								// no need to ask worldserver if we're zoning to ourselves (most likely to a bind point), also fixes a bug since the default response was failure
								APPLAYER* outapp = new APPLAYER(OP_ZoneChange,sizeof(ZoneChange_Struct));
								ZoneChange_Struct* zc2 = (ZoneChange_Struct*) outapp->pBuffer;
								strcpy(zc2->char_name, GetName());
								zc2->zoneID = pp.current_zone;
								zc2->success = 1;
								QueuePacket(outapp);

								delete outapp;
								zone->StartShutdownTimer(AUTHENTICATION_TIMEOUT * 1000);
							}
							else {
								ServerPacket* pack = new ServerPacket(ServerOP_ZoneToZoneRequest, sizeof(ZoneToZone_Struct));
								ZoneToZone_Struct* ztz = (ZoneToZone_Struct*) pack->pBuffer;
								ztz->response = 0;
								ztz->current_zone_id = zone->GetZoneID();
								ztz->requested_zone_id = database.GetZoneID(tarzone);
								ztz->admin = admin;
								ztz->ignorerestrictions = tmpzonesummon_ignorerestrictions;
								strcpy(ztz->name, GetName());
								worldserver.SendPacket(pack);
								delete pack;
							}
							//QueuePacket(app);
							//delete outapp;
							//cout << "Zoning complete" << endl;
							break;
						}
						else
						{

							cerr << "Zone " << zc->zoneID << " is not available" << endl;

							/*			int8 blah[4] = { 0x4E,0x01,0x05,0x06 };
							outapp = new APPLAYER(0xdb20, 4);
							memcpy(outapp->pBuffer, blah, 4);
							QueuePacket(outapp);
							delete outapp;

							  outapp = new APPLAYER;
							  outapp->opcode = 0x1020;
							  outapp->size = 0;
							  QueuePacket(outapp);

							delete outapp;*/

							outapp = new APPLAYER(OP_ZoneChange, sizeof(ZoneChange_Struct));
							ZoneChange_Struct *zc2 = (ZoneChange_Struct*)outapp->pBuffer;
							strcpy(zc2->char_name, zc->char_name);
							zc2->zoneID = zc->zoneID;
							zc2->success = myerror;
							memset(zc2->unknown0073,0xff,sizeof(zc2->unknown0073));
							QueuePacket(outapp);


							delete outapp;

							int8 stuffdata[16] = {0xE6, 0x02, 0x10, 0x00, 0x00, 0x00, 0x68, 0x42, 0x00, 0x00, 0xDB, 0xC3, 0xFA, 0xFE, 0x00, 0xC2};
							outapp = new APPLAYER(0x2120, sizeof(stuffdata));
							memcpy(outapp->pBuffer, stuffdata, sizeof(stuffdata));
							QueuePacket(outapp);
							delete outapp;
							break;

						}
						break;
					}
					case 0x5021:
					case OP_DeleteSpawn: {
						// The client will send this with his id when he zones, maybe when he disconnects too?
						// When zoning he seems to want 5921 and finish flag
						cout << "Player attempting to delete spawn: " << name << endl;
						//FileDumpPacket("DelSpwnQ.txt", outapp);  // Just want to see what this bitch has in it. -Kasai
						RemoveData(); // Flushing the queue of packet data to allow for proper zoning -Kasai
						APPLAYER* outapp;
						outapp = new APPLAYER(OP_DeleteSpawn, sizeof(GlobalID_Struct));
						GlobalID_Struct *gis = (GlobalID_Struct*)outapp->pBuffer;
						gis->entity_id = GetID();
						entity_list.QueueClients(this,outapp,false);
						delete outapp;

						outapp = new APPLAYER(0x5941);
						QueuePacket(outapp);
						delete outapp;
						Disconnect();
						break;
					}
					case 0x5901: {
						APPLAYER* outapp;
						outapp = new APPLAYER(0x5901);
						QueuePacket(outapp);
						delete outapp;
						break;
					}
					case 0x5541: { // Client dumps player profile on zoning, is this same opcode as saving?
						//						FileDumpPacket("player.txt",app);
						if (app->size != sizeof(PlayerProfile_Struct)-8) { //							DumpPacket(app);
							cout << "Wrong size on 0x5541. Got: " << app->size << ", Expected: " << sizeof(PlayerProfile_Struct) << endl;
							//Save();
							break;
						}
						//This Isnt really needed at the moment.
						//cout << "Got a player save request (0x5521)" << endl;
						//PlayerProfile_Struct* in_pp = (PlayerProfile_Struct*) app->pBuffer;
						//memset(in_pp, 0, sizeof(PlayerProfile_Struct));
						//memcpy((char*) in_pp->unknown0004, app->pBuffer, sizeof(PlayerProfile_Struct)-8);
						
						Save();
						break;
								 }
					case OP_Save: {
#ifdef _DEBUG
						//FileDumpPacket("playerprofile.txt", app);
#endif

						//DumpPacket(app);
						if (app->size != sizeof(PlayerProfile_Struct)) {
							cout << "Wrong size on OP_Save. Got: " << app->size << ", Expected: " << sizeof(PlayerProfile_Struct) << endl;
							break;
						}

						cout << "Got a player save request (OP_Save)" << endl;
						PlayerProfile_Struct* in_pp = (PlayerProfile_Struct*) app->pBuffer;
						x_pos = in_pp->x;
						y_pos = in_pp->y;
						z_pos = in_pp->z;
						heading = in_pp->heading;
						/*cout << "***************************************" << endl;
						cout << "*NEW PLAYERPROFILE * NEW PLAYERPROFILE*" << endl;
						cout << "***************************************" << endl;
						cout << "Got PlayerProfileSave... PLAYERPROFILE=" << endl;

						DumpPacket(app);*/
						/*						memcpy(&pp, app->pBuffer, app->size);
						if (!database.SetPlayerProfile(account_id, name, &pp)) {
						cerr << "Failed to update player profile" << endl;
						}
						int tmp = 50;
						tmp = tmp / 0;

						*/
						/*						const Item_Struct* item = 0;
						int i;
						for (i=0; i < 8; i++) {
						item = database.GetItem(in_pp->bank_inv[i]);
						if (item != 0)
						cout << 3980 + (i * 2) << ": Slot " << i << ": " << item->name << endl;
						else
						cout << 3980 + (i * 2) << ": Slot " << i << ": empty" << endl;
						}
						for (i=0; i < 80; i++) {
						item = database.GetItem(in_pp->bank_cont_inv[i]);
						if (item != 0)
						cout << 3980 + (i * 2) << ": Bag: " << i/10 << ", Slot " << i%10 << ": " << item->name << endl;
						else
						cout << 3980 + (i * 2) << ": Bag: " << i/10 << ", Slot " << i%10 << ": empty" << endl;
								  }*/

						Save();
						break;
								  }
					case OP_AutoAttack2: { // Why 2
						break;
					}
					case OP_WhoAll: { // Pyro
						if (app->size != sizeof(Who_All_Struct)) {
							cout << "Wrong size on OP_WhoAll. Got: " << app->size << ", Expected: " << sizeof(Who_All_Struct) << endl;
							//							DumpPacket(app);
							break;
						}
						Who_All_Struct* whoall = (Who_All_Struct*) app->pBuffer;

						WhoAll(whoall);
						break;
					}
					case OP_GMZoneRequest: { // Quagmire
						//	DumpPacket(app);

						if (app->size != sizeof(GMZoneRequest_Struct)) {
							cout << "Wrong size on OP_GMZoneRequest. Got: " << app->size << ", Expected: " << sizeof(GMZoneRequest_Struct) << endl;
							break;
						}
						if (this->Admin() < 80) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/zone");
							break;
						}
						GMZoneRequest_Struct* gmzr = (GMZoneRequest_Struct*) app->pBuffer;
						/*BEGIN CODE FROM ZoneChange - Slightly modified to work with the GMZR struct...*/
						float tarx = -1, tary = -1, tarz = -1;
						sint16 minstatus = 0;
						int8 minlevel = 0;
						char tarzone[32];
						if (gmzr->zoneID == 0)
							strcpy(tarzone, zonesummon_name);
						else if (database.GetZoneName(gmzr->zoneID))
							strcpy(tarzone, database.GetZoneName(gmzr->zoneID));
						else
							tarzone[0] = 0;
						cout << "GMZone request for:" << name << " to: " << tarzone << "(" << database.GetZoneID(tarzone) << ")" << endl;

						// this both loads the safe points and does a sanity check on zone name
						if (!database.GetSafePoints(tarzone, &tarx, &tary, &tarz, &minstatus, &minlevel)) {
							tarzone[0] = 0;
						}


						/*END CODE FROM ZoneChange*/
						APPLAYER* outapp = new APPLAYER(OP_GMZoneRequest, sizeof(GMZoneRequest_Struct));
						GMZoneRequest_Struct* gmzr2 = (GMZoneRequest_Struct*) outapp->pBuffer;
						strcpy(gmzr2->charname, this->GetName());
						gmzr2->zoneID = gmzr->zoneID;
						//Next line stolen from ZoneChange as well... - This gives us a nicer message than the normal "zone is down" message...
						if (tarzone[0] != 0 && admin >= minstatus && GetLevel() >= minlevel)
							gmzr2->success = 1;
						else {
							cout << "GetZoneSafeCoords failed. zoneid = " << gmzr->zoneID << "; czone = " << zone->GetZoneID() << endl;
							gmzr2->success = 0;
						}
						//In case of fire, break glass...
						//this->MovePC(gmzr->zoneID, -1, -1, -1);
						QueuePacket(outapp);
						delete outapp;
						break;
										   }
					case OP_GMZoneRequest2: {
						int32 zonereq = (int32)*app->pBuffer;
						cout << "OP_GMZoneRequest2: size=" << app->size << "; data = " << zonereq << "; czone = " << zone->GetZoneID() << endl;
						//DumpPacket(app);
						//if (strcasecmp((char*) app->pBuffer, zone->GetShortName()) == 0)
						if(zonereq == zone->GetZoneID())
						{
							//Fixed XYZ
							this->MovePC(zonereq, zone->safe_x(), zone->safe_y(), zone->safe_z(), 0, false);
						}
						else
							this->MovePC(zonereq, -1, -1, -1, 0, false);
						break;

											}
					case OP_EndLootRequest: {
						if (app->size != sizeof(int16)) {
							cout << "Wrong size: OP_EndLootRequest, size=" << app->size << ", expected " << sizeof(int16) << endl;
							break;
						}
						//cout << "got end loot request" << endl;

						//APPLAYER* outapp = 0;
						Entity* entity = entity_list.GetID(*((int16*)app->pBuffer));
						if (entity == 0)
						{
							//								DumpPacket(app);
							Message(13, "Error: OP_EndLootRequest: Corpse not found (ent = 0)");
							Corpse::SendLootReqErrorPacket(this);
							break;
						}
						else if (!entity->IsCorpse()) {
							//								DumpPacket(app);
							Message(13, "Error: OP_EndLootRequest: Corpse not found (!entity->IsCorpse())");

							Corpse::SendLootReqErrorPacket(this);
							break;

						}
						else
						{
							entity->CastToCorpse()->EndLoot(this, app);
						}
						break;
					}
					case OP_LootRequest:
					{
						if (app->size != sizeof(int16)) {
							cout << "Wrong size: OP_LootRequest, size=" << app->size << ", expected " << sizeof(int16) << endl;

							break;
						}
						Entity* ent = entity_list.GetID(*((int16*)app->pBuffer));
						if (ent == 0) {
							//								DumpPacket(app);
							Message(13, "Error: OP_LootRequest: Corpse not found (ent = 0)");
							Corpse::SendLootReqErrorPacket(this);
							break; // Quagmire - crash bug fix
						}
						if (ent->IsCorpse()) {
							ent->CastToCorpse()->MakeLootRequestPackets(this, app);
							break;
						}
						else {
							cout << "npc == 0 LOOTING FOOKED3" << endl;
							Message(13, "Error: OP_LootRequest: Corpse not a corpse?");
							Corpse::SendLootReqErrorPacket(this);
						}
						break;

					}
					case OP_LootItem:
					{

							DumpPacket(app);
						if (app->size != sizeof(LootingItem_Struct)) {
							cout << "Wrong size: OP_LootItem, size=" << app->size << ", expected " << sizeof(LootingItem_Struct) << endl;
							break;
						}
						/*
						** Disgrace:
						**	fixed the looting code so that it sends the correct opcodes

						  **	and now correctly removes the looted item the player selected
						  **	as well as gives the player the proper item.
						  **	Also fixed a few UI lock ups that would occur.
						*/

						APPLAYER* outapp = 0;
						Entity* entity = entity_list.GetID(*((int16*)app->pBuffer));
						if (entity == 0) {
							//								DumpPacket(app);

							Message(13, "Error: OP_LootItem: Corpse not found (ent = 0)");
							outapp = new APPLAYER(OP_LootComplete, 0);
							QueuePacket(outapp);
							delete outapp;
							break;
						}

						if (entity->IsCorpse()) {
							entity->CastToCorpse()->LootItem(this, app);
							break;
						}
						else {
							Message(13, "Error: Corpse not found! (!ent->IsCorpse())");
							Corpse::SendEndLootErrorPacket(this);
						}

						break;
					}
					case OP_GuildMOTD: {
						//						app->pBuffer[0] = name of me
						//						app->pBuffer[36] = new motd
						char tmp[255];
						if (guilddbid == 0) {
							// client calls for a motd on login even if they arent in a guild
							//							Message(0, "Error: You arent in a guild!");
						}
						else if (guilds[guildeqid].rank[guildrank].motd && !(strlen((char*) &app->pBuffer[68]) == 0)) {
							if (strcasecmp((char*) &app->pBuffer[68+strlen((char*) &app->pBuffer[0])], " - none") == 0)
								tmp[0] = 0;
							else
								strn0cpy(tmp, (char*) &app->pBuffer[68], sizeof(tmp)); // client includes the "name - "

							if (!database.SetGuildMOTD(guilddbid, tmp)) {
								Message(0, "Motd update failed.");
							}
							worldserver.SendEmoteMessage(0, guilddbid, MT_Guild, "Guild MOTD: %s", tmp);
						}
						else {
							database.GetGuildMOTD(guilddbid, tmp);
							if (strlen(tmp) != 0)
								Message(MT_Guild, "Guild MOTD: %s", tmp);
						}

						break;
					}
					case OP_GuildPeace: {
#ifdef GUILDWARS
						if(target == this || target == 0 ) {
							Message(0,"You must have a target.");
							break;
						}
						if(target->IsNPC() || (target->IsClient() && target->CastToClient()->GuildRank() != 0))
						{
							Message(0,"Must target the other guilds guild leader.");
							break;
						}
						if(GuildRank() == 0 && target->IsClient() && target->CastToClient()->GuildRank() == 0) {
							target->CastToClient()->guildchange = 1;
							target->CastToClient()->otherleaderid = GetID();
							if(guildchange == 0 && otherleaderid == 0)
								target->Message(0,"%s wishes to make a peace treaty with you, to do so, select him/her and type /guildpeace",GetName());
							else {
								Entity* entity = entity_list.GetID(otherleaderid);
								if(guildchange != 1 && target->IsClient() && entity->CastToClient() != target->CastToClient())
									Message(0,"A guild alliance was never offered or your target is not the one who initiated the guild status change.");
								else {
									int32 other = entity->CastToClient()->GuildDBID();
									int32 guildid = GuildDBID();
									if(database.SetGuildAlliance(other,guildid) && database.SetGuildAlliance(guildid,other)) {
										guildchange = 0;
										otherleaderid = 0;
										target->CastToClient()->guildchange = 0;
										target->CastToClient()->otherleaderid = 0;

										Message(0,"The alliance has been completed!");
										entity->CastToClient()->Message(0,"The alliance has been completed!");
										char guildone[400];
										char guildtwo[400];
										sprintf(guildone,"%i",other);


										sprintf(guildtwo,"%i",guildid);
										database.InsertNewsPost(2,guildone,guildtwo,0,"");
								ServerPacket* pack = new ServerPacket;
								pack->opcode = ServerOP_ReloadAlliances;
								pack->size = 0;
								worldserver.SendPacket(pack);
								delete pack;

									}
								}
							}
						}
						else {
							Message(0,"You can only make a peace treaty with another guild leader.");
						}
#endif
						break;
					}
					case OP_GuildWar: {
#ifdef GUILDWARS

						if(target == this || target == 0 ) {
							Message(0,"You must have a target.");
							break;
						}
						if(target->IsNPC() || (target->IsClient() && target->CastToClient()->GuildRank() != 0))
						{
							Message(0,"Must target the other guilds guild leader.");
							break;
						}
						if(GuildRank() == 0 && target->IsClient() && target->CastToClient()->GuildRank() == 0) {
							target->CastToClient()->guildchange = 2;
							target->CastToClient()->otherleaderid = GetID();
							if(guildchange == 0 && otherleaderid == 0)
								target->Message(0,"%s wishes to make war with you, to do so, select him/her and type /guildwar",GetName());
							else {
								Entity* entity = entity_list.GetID(otherleaderid);
								if(guildchange != 2 && target->IsClient() && entity->CastToClient() != target->CastToClient())
									Message(0,"A guild war was never offered or your target is not the one who initiated the guild status change.");
								else {
									int32 other = entity->CastToClient()->GuildDBID();
									int32 guildid = GuildDBID();
									if(database.RemoveGuildAlliance(other,guildid) && database.RemoveGuildAlliance(guildid,other)) {
										Message(0,"The alliance has been removed!");
										entity->CastToClient()->Message(0,"The alliance has been removed!");
										char guildone[400];
										char guildtwo[400];
										sprintf(guildone,"%i",other);
										sprintf(guildtwo,"%i",guildid);
										database.InsertNewsPost(3,guildone,guildtwo,0,"");
								ServerPacket* pack = new ServerPacket;
								pack->opcode = ServerOP_ReloadAlliances;
								pack->size = 0;
								worldserver.SendPacket(pack);
								delete pack;
									}
								}
							}
						}
						else {
							Message(0,"You can only make war with another guild leader.");
						}
#endif
						break;
					}
					case OP_GuildLeader: {
						if (app->size <= 1)
							break;
						app->pBuffer[app->size-1] = 0;
						if (guilddbid == 0)
							Message(0, "Error: You arent in a guild!");
						else if (!(guilds[guildeqid].leader == account_id))
							Message(0, "Error: You arent the guild leader!");
						else if (!worldserver.Connected())
							Message(0, "Error: World server disconnected");
						else {
							ServerPacket* pack = new ServerPacket;
							pack->opcode = ServerOP_GuildLeader;
							pack->size = sizeof(ServerGuildCommand_Struct);
							pack->pBuffer = new uchar[pack->size];
							memset(pack->pBuffer, 0, pack->size);
							ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
							sgc->guilddbid = guilddbid;
							sgc->guildeqid = guildeqid;
							sgc->fromrank = guildrank;
							sgc->fromaccountid = account_id;
							sgc->admin = admin;
							strcpy(sgc->from, name);
							strcpy(sgc->target, (char*) app->pBuffer);
							worldserver.SendPacket(pack);
							delete pack;
						}
						break;
										 }
					case OP_GuildInvite:
						{
							if (app->size != sizeof(GuildCommand_Struct)) {


								cout << "Wrong size: OP_GuildInvite, size=" << app->size << ", expected " << sizeof(GuildCommand_Struct) << endl;
								break;
							}
							if (guilddbid == 0)
								Message(0, "Error: You arent in a guild!");
							else if (!guilds[guildeqid].rank[guildrank].invite)
								Message(0, "You dont have permission to invite.");
							else if (!worldserver.Connected())
								Message(0, "Error: World server disconnected");
/*#ifdef GUILDWARS
							else if (zone->GetGuildOwned() != 0 && !database.GetGuildAlliance(GuildDBID(),zone->GetGuildOwned()) && zone->GetGuildOwned() != GuildDBID() && zone->GetGuildOwned() != 3)
								Message(0, "You cannot invite guild members in an enemy city unless it is a neutral guild.");
#endif*/
							else {
#ifdef GUILDWARS
								if (database.NumberInGuild(guilddbid)>20){
									Message(15,"Your Guild has reached its Guildwars size limit.  You cannot invite any more people.");
									break;
								}
#endif
								GuildCommand_Struct* gc = (GuildCommand_Struct*) app->pBuffer;
								ServerPacket* pack = new ServerPacket;
								pack->opcode = ServerOP_GuildInvite;
								pack->size = sizeof(ServerGuildCommand_Struct);
								pack->pBuffer = new uchar[pack->size];
								memset(pack->pBuffer, 0, pack->size);
								ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
								sgc->guilddbid = guilddbid;
								sgc->guildeqid = guildeqid;
								if(gc->officer)
								sgc->fromrank = 1;
								else
								sgc->fromrank = guildrank;
								sgc->fromaccountid = account_id;
								sgc->admin = admin;
								strcpy(sgc->from, name);
								strcpy(sgc->target, gc->othername);
								worldserver.SendPacket(pack);
								delete pack;
							}
							break;
						}
					case OP_GuildRemove:
						{
							if (app->size != sizeof(GuildCommand_Struct)) {
								cout << "Wrong size: OP_GuildRemove, size=" << app->size << ", expected " << sizeof(GuildCommand_Struct) << endl;
								break;
							}
							GuildCommand_Struct* gc = (GuildCommand_Struct*) app->pBuffer;
							if (guilddbid == 0)
								Message(0, "Error: You arent in a guild!");
							else if (!(guilds[guildeqid].rank[guildrank].remove || strcasecmp(gc->othername, this->GetName()) == 0))
								Message(0, "You dont have permission to remove.");
							else if (!worldserver.Connected())
								Message(0, "Error: World server disconnected");
							else {
								ServerPacket* pack = new ServerPacket;
								pack->opcode = ServerOP_GuildRemove;
								pack->size = sizeof(ServerGuildCommand_Struct);
								pack->pBuffer = new uchar[pack->size];
								memset(pack->pBuffer, 0, pack->size);
								ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
								sgc->guilddbid = guilddbid;
								sgc->guildeqid = guildeqid;
								sgc->fromrank = guildrank;
								sgc->fromaccountid = account_id;
								sgc->admin = admin;
								strcpy(sgc->from, name);
								strcpy(sgc->target, gc->othername);
								worldserver.SendPacket(pack);
								delete pack;
							}
							break;
						}
					case OP_GuildInviteAccept:
						{
							if (app->size != sizeof(GuildCommand_Struct)) {
								cout << "Wrong size: OP_GuildInviteAccept, size=" << app->size << ", expected " << sizeof(GuildCommand_Struct) << endl;
								break;
							}
							GuildCommand_Struct* gc = (GuildCommand_Struct*) app->pBuffer;
							if (gc->guildeqid == 0) {
								int32 tmpeq = database.GetGuildEQID(this->PendingGuildInvite);
								if (guilddbid != 0)
									Message(0, "Error: You're already in a guild!");
								else if (!worldserver.Connected())
									Message(0, "Error: World server disconnected");
								else if (this->PendingGuildInvite == 0)
									Message(0, "Error: No guild invite pending.");
								else {
									this->PendingGuildInvite = 0;
									if (this->SetGuild(guilds[tmpeq].databaseID, GUILD_MAX_RANK))
										worldserver.SendEmoteMessage(0, guilds[tmpeq].databaseID, MT_Guild, "%s has joined the guild. Rank: %s.", this->GetName(), guilds[tmpeq].rank[GUILD_MAX_RANK].rankname);
									else {
										worldserver.SendEmoteMessage(gc->othername, 0, 0, "Guild invite accepted, but failed.");
										Message(0, "Guild invite accepted, but failed.");
									}
								}
							}
							else if (gc->guildeqid == 4) {
								this->PendingGuildInvite = 0;
								worldserver.SendEmoteMessage(gc->othername, 0, 0, "%s's guild invite window timed out.", this->GetName());
							}
							else if (gc->guildeqid == 5) {
								this->PendingGuildInvite = 0;
								worldserver.SendEmoteMessage(gc->othername, 0, 0, "%s has declined to join the guild.", this->GetName());
							}
							else {

								this->PendingGuildInvite = 0;
								SetGuild(0, GUILD_MAX_RANK);
								worldserver.SendEmoteMessage(gc->othername, 0, 0, "Unknown response from %s to guild invite.", this->GetName());
							}
							break;

						}
                			case OP_ManaChange: {
						if(app->size == 0) {
							// i think thats the sign to stop the songs
							if (bardsong != 0) {
								LogFile->write(EQEMuLog::Debug, "BARD: stop song requested bardsong:%i", bardsong);
								bardsong = 0;
								bardsong_timer->Disable();
								spellend_timer->Disable();
							} else {
								LogFile->write(EQEMuLog::Debug, "BARD: stop song requested bardsong:NULL");
								bardsong_timer->Disable();
							}
							break;
						}
						ManaChange_Struct* p = (ManaChange_Struct*)app->pBuffer;
						if (p->spell_id != casting_spell_id && p->spell_id != bardsong) {
							cout << "Something strange happened. ManaChange_Struct->Spell_id != castingspellid" << endl;
						}
						Mob::SetMana(p->new_mana);
						break;
					}
					case OP_MemorizeSpell: {
						OPMemorizeSpell(app);
						break;
					}
					case OP_SwapSpell:
					{
						if (app->size != sizeof(SwapSpell_Struct)) {
							cout << "Wrong size on OP_SwapSpell. Got: " << app->size << ", Expected: " << sizeof(SwapSpell_Struct) << endl;


							break;
						}
						const SwapSpell_Struct* swapspell = (const SwapSpell_Struct*) app->pBuffer;
						short swapspelltemp;

						swapspelltemp = pp.spell_book[swapspell->from_slot];
						pp.spell_book[swapspell->from_slot] = pp.spell_book[swapspell->to_slot];
						if(swapspelltemp == 0)
						pp.spell_book[swapspell->to_slot] = 0xFFFF;
						else
						pp.spell_book[swapspell->to_slot] = swapspelltemp;


						QueuePacket(app);

						break;
					}
					case OP_CastSpell: {
						if (app->size != sizeof(CastSpell_Struct)) {
							cout << "Wrong size: OP_CastSpell, size=" << app->size << ", expected " << sizeof(CastSpell_Struct) << endl;
							break;
						}
						if (IsAIControlled()) {
							Message(13, "You cant cast right now, you arent in control of yourself!");
							break;
						}
						CastSpell_Struct* castspell = (CastSpell_Struct*)app->pBuffer;
#if (DEBUG >=11)
						DumpPacket(app);
						//cout<<"cs_unknown2: (uint32)"<<(uint32)castspell->cs_unknown[0]<<"(int32)"<<(int32)castspell->cs_unknown[0]<<":"<<endl;
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: %u %i", (uint8)castspell->cs_unknown[0], castspell->cs_unknown[0]);
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: %u %i", (uint8)castspell->cs_unknown[1], castspell->cs_unknown[1]);
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: %u %i", (uint8)castspell->cs_unknown[2], castspell->cs_unknown[2]);
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: %u %i", (uint8)castspell->cs_unknown[3], castspell->cs_unknown[3]);
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: 32 %p %u", &castspell->cs_unknown, *(uint32*) castspell->cs_unknown );
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: 32 %p %i", &castspell->cs_unknown, *(int32*) castspell->cs_unknown );
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: 16 %p %u %u", &castspell->cs_unknown, *(uint16*) castspell->cs_unknown, *(uint16*) castspell->cs_unknown+sizeof(uint16) );
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: 16 %p %i %i", &castspell->cs_unknown, *(int16*) castspell->cs_unknown, *(int16*) castspell->cs_unknown+sizeof(int16) );
						//cout<<"cs_unknown2: (uint16)"<<(uint16)castspell->cs_unknown[0]<<"(int16)"<<(int16)castspell->cs_unknown[0]<<":"<<endl;
#endif
						if (bardsong != 0 && IsBardSong(castspell->spell_id)) {
							LogFile->write(EQEMuLog::Normal, "BARD: already casting disabling bardsong:%i", bardsong);
							//bardsong = 0;
							//bardsong_timer->Disable();
                        				SendSpellBarDisable(false);
                        				break;
						}
						//DumpPacketHex(app);
						cout << this->GetName() << " casting #" << castspell->spell_id << ", slot=" << castspell->slot << ", invslot=" << castspell->inventoryslot << endl;
						if (castspell->slot == 10) { // this means right-click
							if (castspell->inventoryslot < 30) { // santity check
								const Item_Struct* item = database.GetItem(pp.inventory[castspell->inventoryslot]);
								
								bool cancast = true;
								if (item == 0) {
									Message(0, "Error: item==0 for inventory slot #%i", castspell->inventoryslot);
								}
								else if (item->common.effecttype == 1 || item->common.effecttype == 3 || item->common.effecttype == 4 || item->common.effecttype == 5) {
									cancast = DecreaseCharges(castspell->inventoryslot);
									if(!cancast) {
										InterruptSpell();
										Message(4,"This item is out of charges");
										break;
									}
									else if (item->common.casttime == 0)
										SpellFinished(item->common.spellId, castspell->target_id, castspell->slot, 0);
									else
										CastSpell(item->common.spellId, castspell->target_id, castspell->slot, item->common.casttime);
								}
								else {
									Message(0, "Error: unknown item->common.effecttype (0x%02x)", item->common.effecttype);
								}
							}
							else
								Message(0, "Error: castspell->inventoryslot >= 30 (0x%04x)", castspell->inventoryslot);
						}
						else {
//                    		if (IsBardSong(castspell->spell_id) && spells[castspell->spell_id].targettype == ST_Self || spells[castspell->spell_id].targettype == ST_Group || spells[castspell->spell_id].targettype == ST_AECaster)
//								castspell->target_id = GetID();
							CastSpell(castspell->spell_id, castspell->target_id, castspell->slot);
							//SimpleCheckIncreaseSkill(CHANNELING);
							//SimpleCheckIncreaseSkill(spells[castspell->spell_id].skill);
						}
						break;
					}
					//heko: replaced the MonkAtk, renamed Monk_Attack_Struct
					case OP_CombatAbility: {
						if (app->size != sizeof(CombatAbility_Struct)) {
							cout << "Wrong size on OP_CombatAbility. Got: " << app->size << ", Expected: " << sizeof(CombatAbility_Struct) << endl;

							break;
						}

						if (target) {
							if(!IsAttackAllowed(target))
								break;

							CombatAbility_Struct* ca_atk = (CombatAbility_Struct*) app->pBuffer;						
							if ((ca_atk->m_atk == 100)&&(ca_atk->m_type==10)) {    // SLAM - Bash without a shield equipped
								DoAnim(7);
								sint32 dmg=(sint32) ((level/10)  * 3  * (GetSkill(BASH) + GetSTR() + level) / (700-GetSkill(BASH)));
								//this->Message(MT_Emote, "You Bash for a total of %d damage.",  dmg);
								target->Damage(this, dmg, 0xffff, BASH);

								CheckIncreaseSkill(BASH);

								if(GetClass()==WARRIOR&&(GetRace()==BARBARIAN||GetRace()==TROLL||GetRace()==OGRE))  // large race warriors only *
								{
									float wisebonus =  (pp.WIS > 200) ? 20 + ((pp.WIS - 200) * 0.05) : pp.WIS * 0.1;

									if (((55-(GetSkill(BASH)*0.240))+wisebonus > (float)rand()/RAND_MAX*100)&& (GetSkill(BASH)<(pp.level+1)*5))
											this->SetSkill(BASH,GetSkill(BASH)+1);
								}
								break;
							}

							if ((ca_atk->m_atk == 11)&&(ca_atk->m_type==51)) {
								const Item_Struct* Rangeweapon = 0;
								Rangeweapon = database.GetItem(pp.inventory[11]);
								if (!Rangeweapon) {
									Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have nothing to throw!", pp.inventory[11]);
									break;
								}
								uint8 WDmg = Rangeweapon->common.damage;
								// Throw stuff
								DoAnim(5);
								sint32 TotalDmg = 0;
								
								// borrowed this from attack.cpp
								// chance to hit

								float chancetohit = GetSkill(51) / 3.75;    // throwing
								
								if (pp.level-target->GetLevel() < 0)

								{
									chancetohit -= (float)((target->GetLevel()-pp.level)*(target->GetLevel()-pp.level))/4;
								}

								int16 targetagi = target->GetAGI();
								int16 playerDex = (int16)(this->itembonuses->DEX + this->spellbonuses->DEX)/2;

								targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);
								chancetohit -= (float)targetagi*0.05;
								chancetohit += playerDex;
								chancetohit = (chancetohit > 0) ? chancetohit+30:30;
								chancetohit = chancetohit > 95 ? 95 : chancetohit; /* cap to 95% */

								uint8 levelBonus = (pp.STR+pp.level+GetSkill(51)) / 100;
								uint8 MaxDmg = (WDmg)*levelBonus;
								if (MaxDmg == 0)
									MaxDmg = 1;
								TotalDmg = 1 + rand()%MaxDmg;

								// Hit?
								if (((float)rand()/RAND_MAX)*100 > chancetohit)
								{
										target->CastToNPC()->Damage(this, 0, 0xffff, THROWING);
								}
								else
								{
//										this->Message(MT_Emote, "You Hit for a total of %d damage.", TotalDmg);
										target->CastToNPC()->Damage(this, TotalDmg, 0xffff, THROWING);
								}
								// See if the player increases their skill - with cap
								float wisebonus =  (pp.WIS > 200) ? 20 + ((pp.WIS - 200) * 0.05) : pp.WIS * 0.1;

								if (((55-(GetSkill(51)*0.240))+wisebonus > (float)rand()/RAND_MAX*100)&& (GetSkill(51)<(pp.level+1)*5))
										this->SetSkill(51,GetSkill(51)+1);

								break;
							}

							if ((ca_atk->m_atk == 11)&&(ca_atk->m_type==7)) {
								DoAnim(9);

								// This is a Ranged Weapon Attack / Bow
								const Item_Struct* Rangeweapon = 0;
								const Item_Struct* Ammo = 0;

								Rangeweapon = database.GetItem(pp.inventory[11]);
								Ammo = database.GetItem(pp.inventory[21]);
								if (!Rangeweapon) {
									Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have nothing to throw!", pp.inventory[11]);
									break;
								}
								if (!Ammo) {
									Message(0, "Error: Ammo: GetItem(%i)==0, you have nothing to throw!", pp.inventory[11]);
									break;
								}
								
								uint16 WDmg = Rangeweapon->common.damage;
								uint16 ADmg = Ammo->common.damage;
								
								// These dmg formulae were taken from all over the net.
								// No-one knows but Verant. This should be fairly close -BoB
								
								// ** ToDO: take into account trueshot disc (x2.0) and 
								//          Archery Mastery AA when they are available.
								//          I am still looking for 'double' information too.
								
								// Note: Rangers have a chance of crit dmg with a bow (affected by Dex)

								uint16 levelBonus = (pp.STR+pp.level+GetSkill(ARCHERY)) / 100;
								uint16 MaxDmg = (WDmg+ADmg)*levelBonus;
								
								sint32 TotalDmg = 0;
								sint32 critDmg = 0;

								if(GetClass()==RANGER)
								{
									critDmg = (sint32)(MaxDmg * 1.72);
								}

								if (MaxDmg == 0)
									MaxDmg = 1;
								TotalDmg = 1 + rand()%MaxDmg;

								// TODO: post 50 dmg bonus
								// TODO: Tone down the PvP dmg

								// borrowed this from attack.cpp
								// chance to hit

								float chancetohit = GetSkill(ARCHERY) / 3.75;
								if (pp.level-target->GetLevel() < 0) {
									chancetohit -= (float)((target->GetLevel()-pp.level)*(target->GetLevel()-pp.level))/4;
								}

								int16 targetagi = target->GetAGI();
								int16 playerDex = (int16)(this->itembonuses->DEX + this->spellbonuses->DEX)/2;

								targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);
								chancetohit -= (float)targetagi*0.05;
								chancetohit += playerDex;
								chancetohit = (chancetohit > 0) ? chancetohit+30:30;
								chancetohit = chancetohit > 95 ? 95 : chancetohit; /* cap to 95% */

								// Hit?
								if (((float)rand()/RAND_MAX)*100 > chancetohit)
								{
									this->Message(MT_Emote, "You missed your target");
									target->CastToNPC()->Damage(this, 0, 0xffff, 0x07);
								}
								else
								{
									// no crits before level 12 cap is maxed
									if((GetClass()==RANGER)&&(GetSkill(ARCHERY)>65)&&(rand()%255<(GetSkill(ARCHERY)+playerDex)/2)&&(chancetohit > 85))
									{

										this->Message(MT_Emote, "You score a critical hit!(%d)", critDmg);
										target->CastToNPC()->Damage(this, critDmg, 0xffff, 0x07);

									}
									else
									{
										this->Message(MT_Emote, "You Hit for a total of %d non-melee damage.", TotalDmg);
										target->CastToNPC()->Damage(this, TotalDmg, 0xffff, 0x07);
									}
								}

								// See if the player increases their skill - with cap
								float wisebonus =  (pp.WIS > 200) ? 20 + ((pp.WIS - 200) * 0.05) : pp.WIS * 0.1;

								if (((55-(GetSkill(ARCHERY)*0.240))+wisebonus > (float)rand()/RAND_MAX*100)&& (GetSkill(ARCHERY)<(pp.level+1)*5) && GetSkill(ARCHERY) < 252)
										this->SetSkill(ARCHERY,GetSkill(ARCHERY)+1);

								break;
							}
							switch(GetClass())
							{
							case WARRIOR:
								if (target!=this) {
									if (target->IsNPC()){ //Warriors should kick harder than hybrid classes
										float dmg=(((GetSkill(KICK) + GetSTR())/25) *4 +level)*(((float)rand()/RAND_MAX));
										target->CastToNPC()->Damage(this, (int32)dmg, 0xffff, 0x1e);
										CheckIncreaseSkill(KICK);
										DoAnim(1);

									}
									else if(target->IsClient()) {
										float dmg=(((GetSkill(KICK) + GetSTR())/25) *3 +level)*(((float)rand()/RAND_MAX));
										//cout << "Dmg:"<<dmg;
										target->CastToNPC()->Damage(this, (int32)dmg, 0xffff, 0x1e);
										DoAnim(1);
									}
								}
								break;
							case RANGER:

							case PALADIN:
							case SHADOWKNIGHT:
								if (target!=this) {
									if (target->IsNPC()){
										float dmg=(((GetSkill(KICK) + GetSTR())/25) *3 +level)*(((float)rand()/RAND_MAX));

										//cout << "Dmg:"<<dmg;
										target->CastToNPC()->Damage(this, (int32)dmg, 0xffff, 0x1e);
										CheckIncreaseSkill(KICK);
										DoAnim(1);
									}
									else if(target->IsClient()) {
										float dmg=(((GetSkill(KICK) + GetSTR())/25) *2 +level)*(((float)rand()/RAND_MAX));
										//cout << "Dmg:"<<dmg;
										target->CastToNPC()->Damage(this, (int32)dmg, 0xffff, 0x1e);
										DoAnim(1);
									}


								}
								break;


							case MONK:
								CheckIncreaseSkill(ca_atk->m_type);
								MonkSpecialAttack(target->CastToMob(), ca_atk->m_type);
								break;
							case ROGUE:
								if (ca_atk->m_atk == 100)
								{uint8 *aa_item = &(((uint8 *)&aa)[124]);// Chaotic backstab TODO make it do min damage
								if (BehindMob(target, GetX(), GetY())) // Player is behind target
								{
									// solar - chance to assassinate
									// TODO: it's set to 40% chance, should be a formula involving DEX
									if(
										level >= 60 && /* player is 60 or higher */
										target->GetLevel() <= 45 && /* mob 45 or under */
										!target->CastToNPC()->IsEngaged() && /* not aggro */
										((float)rand()/RAND_MAX) < 0.40 /* chance */
										)
									{
										char temp[100];
										snprintf(temp, 100, "%s ASSASSINATES their victim!!", this->GetName());
										CheckIncreaseSkill(BACKSTAB);
										entity_list.MessageClose(this, 0, 200, 10, temp);
										RogueAssassinate(target);
									}
									else
									{
										
										RogueBackstab(target, weapon1, GetSkill(BACKSTAB));
										if ((level > 54) && (target != 0))
										{
											float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max

											CheckIncreaseSkill(BACKSTAB);
											// Check for double attack with main hand assuming maxed DA Skill (MS)
											float random = (float)rand()/RAND_MAX;

											if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
												if(target && target->GetHP() > 0)
													RogueBackstab(target, weapon1, GetSkill(BACKSTAB));
										}
									}
								}

								else if(*aa_item>0)
								{
					
									RogueBackstab(target, weapon1, GetSkill(BACKSTAB));
									if ((level > 54) && (target != 0))
									{
										float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max
										CheckIncreaseSkill(BACKSTAB);
										// Check for double attack with main hand assuming maxed DA Skill (MS)
										float random = (float)rand()/RAND_MAX;
										if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
											if(target && target->GetHP() > 0)
												RogueBackstab(target, weapon1, GetSkill(BACKSTAB));
									}
								}
								else	// Player is in front of target
								{

									Attack(target, 13);
									if ((level > 54) && (target != 0))
									{
										float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max

										// Check for double attack with main hand assuming maxed DA Skill (MS)
										float random = (float)rand()/RAND_MAX;
										if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
											if(target && target->GetHP() > 0)
												Attack(target, 13);
									}

								}

								}
								break;
							}

						}

						break;
					}
					//heko:
					//Packet length 4
					//0000: 00 00		NPC ID
					case OP_Taunt:
						{
							if (this->GetTarget() == 0)
								break;
							if (!this->GetTarget()->IsNPC())
								break;
							sint32 newhate, tauntvalue;
							if (app->size != sizeof(ClientTarget_Struct)) {
								cout << "Wrong size on OP_Taunt. Got: " << app->size << ", Expected: "<< sizeof(ClientTarget_Struct) << endl;
								break;
							}
							//ClientTarget_Struct* taunt_npc_id = (ClientTarget_Struct*) app->pBuffer;
							//cout << "Taunting NPC number: " << taunt_npc_id << endl;

							// Check to see if we're already at the top of the target's hate list
							if ((target->CastToNPC()->GetHateTop() != this) && (target->GetLevel() < level))
							{

								// no idea how taunt success is actually calculated
								// TODO: chance for level 50+ mobs should be lower

								CheckIncreaseSkill(TAUNT);
								float tauntchance;
								int level_difference = level - target->GetLevel();
								if (level_difference <= 5)
								{
									tauntchance = 25.0;	// minimum
									tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier
									if (tauntchance > 65.0)
										tauntchance = 65.0;
								}
								else if (level_difference <= 10)
								{
									tauntchance = 30.0;	// minimum
									tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier
									if (tauntchance > 85.0)
										tauntchance = 85.0;

								}
								else if (level_difference <= 15)
								{
									tauntchance = 40.0;	// minimum
									tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier
									if (tauntchance > 90.0)
										tauntchance = 90.0;
								}
								else
								{
									tauntchance = 50.0;	// minimum
									tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier
									if (tauntchance > 95.0)
										tauntchance = 95.0;
								}
								if (tauntchance > ((float)rand()/(float)RAND_MAX)*100.0)
								{
									// this is the max additional hate added per succesfull taunt
									tauntvalue = (sint32) ((float)level * 10.0 * (float)rand()/(float)RAND_MAX + 1);
									// new hate: find diff of player's hate and whoever's at top of list, add that plus tauntvalue to players hate
									newhate = target->CastToNPC()->GetNPCHate(target->CastToNPC()->GetHateTop()) - target->CastToNPC()->GetNPCHate(this) + tauntvalue;
									// add the hate
									target->CastToNPC()->AddToHateList(this, newhate);
								}
							}

							break;
						}

					case OP_InstillDoubt: {
						//						Fear Spell not yet implemented
						Instill_Doubt_Struct* iatk = (Instill_Doubt_Struct*) app->pBuffer;
						if (iatk->i_atk == 0x2E) {
							Message(4, "You\'re not scaring anyone.");
						}
						break;
					}
					case OP_RezzAnswer: {
						OPRezzAnswer(app);
						break;
					}
					case OP_GMSummon: { // and /corpse
						if (app->size != sizeof(GMSummon_Struct)) {
							cout << "Wrong size on OP_GMSummon. Got: " << app->size << ", Expected: " << sizeof(GMSummon_Struct) << endl;
							break;
						}
						//	DumpPacket(app);
						GMSummon_Struct* gms = (GMSummon_Struct*) app->pBuffer;
						Mob* st = entity_list.GetMob(gms->charname);
						if (admin >= 80) {
							int8 tmp = gms->charname[strlen(gms->charname)-1];
							if (st != 0) {
								this->Message(0, "Local: Summoning %s to %i, %i, %i", gms->charname, gms->x, gms->y, gms->z);
								if (st->IsClient() && (st->CastToClient()->GetAnon() != 1 || this->Admin() >= st->CastToClient()->Admin()))
									st->CastToClient()->MovePC((char*) 0, gms->x, gms->y, gms->z, 2, true);
								else
									st->GMMove(gms->x, gms->y, gms->z);
							}
							else if (!worldserver.Connected())
								Message(0, "Error: World server disconnected");
							else if (tmp < '0' || tmp > '9') { // dont send to world if it's not a player's name
								ServerPacket* pack = new ServerPacket;
								pack->opcode = ServerOP_ZonePlayer;
								pack->size = sizeof(ServerZonePlayer_Struct);
								pack->pBuffer = new uchar[pack->size];
								memset(pack->pBuffer, 0, pack->size);
								ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
								strcpy(szp->adminname, this->GetName());
								szp->adminrank = this->Admin();
								strcpy(szp->name, gms->charname);
								strcpy(szp->zone, zone->GetShortName());
								szp->x_pos = gms->x;
								szp->y_pos = gms->y;
								szp->z_pos = gms->z;
								szp->ignorerestrictions = 2;
								worldserver.SendPacket(pack);

								delete pack;
							}
							else {
								Message(13, "Error: '%s' not found.", gms->charname);
							}
						}
						else if (st && st->IsCorpse()) {

							st->CastToCorpse()->Summon(this, false);
						}
						else if (st) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/summon");
						}
						break;
					}


					case OP_TradeRequest:
						{
							//							DumpPacket(app);
							Trade_Window_Struct* msg = (Trade_Window_Struct*) app->pBuffer;
							Mob* tmp = entity_list.GetMob(msg->fromid);
							if (!tmp)
								break;
							if (tmp->IsClient()) {
								tmp->CastToClient()->tradepp=0;
								tmp->CastToClient()->tradegp=0;
								tmp->CastToClient()->tradesp=0;
								tmp->CastToClient()->tradecp=0;
							}
							this->tradepp=0;
							this->tradegp=0;
							this->tradesp=0;
							this->tradecp=0;
							for(int x=0; x != 80; x++){
								TradeList[x]=0;
								TradeCharges[x]=0;
							}
							if (tmp != 0) {
								if (tmp->IsClient()) {
									tmp->CastToClient()->QueuePacket(app);
								}
								else
								{	//npcs always accept
									APPLAYER* outapp = new APPLAYER(OP_TradeAccepted,4);
									Trade_Window_Struct* acc = (Trade_Window_Struct*) outapp->pBuffer;
									acc->fromid = msg->toid;


									acc->toid	= msg->fromid;
									FastQueuePacket(&outapp);
									InTrade = 1;
									this->TradeWithEnt = msg->fromid;
								}

							}
							break;

						}
					case OP_BoardBoat: {
						char *boatname;
						this->IsOnBoat=true;
						boatname = new char[app->size-4];
						memset(boatname, 0, app->size-4);
						memcpy(boatname, app->pBuffer, app->size-4);
						printf("%s has gotten on the boat %s\n",GetName(),boatname);
						Mob* boat = entity_list.GetMob(boatname);
						if (boat){
							boat->CastToNPC()->passengers=true;
						}
						safe_delete(boatname);
						break;
					}
					case OP_LeaveBoat: {
						this->IsOnBoat=false;
						break;
					}
					case OP_TradeAccepted: {
						const Trade_Window_Struct* msg = (const Trade_Window_Struct*) app->pBuffer;
						InTrade= 1;
						this->TradeWithEnt = msg->fromid;
						Mob* tmp = entity_list.GetMob(msg->fromid);
						if (tmp != 0) {
							if (tmp->IsClient()) {
								for(int x=0; x != 80; x++){
									TradeList[x]=0;
									TradeCharges[x]=0;
								}
								tmp->CastToClient()->tradeitems[0]=NULL;
								tmp->CastToClient()->QueuePacket(app);
								tmp->CastToClient()->TradeWithEnt=GetID();
								tmp->CastToClient()->InTrade=1;
							}

						}
						break;
					}
					case OP_CancelTrade: {
						APPLAYER* outapp = app->Copy();
						CancelTrade_Struct* msg = (CancelTrade_Struct*) outapp->pBuffer;
						Mob* tmp = entity_list.GetMob(TradeWithEnt);
						if (tmp != 0) {

							msg->fromid = this->GetID();
							QueuePacket(outapp);
							this->FinishTrade(this);
							//cout << "deal canceld" << endl;
							InTrade = 0;
							TradeWithEnt = 0;
							if (tmp->IsClient()) {
								if ((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp) > 0)
									this->AddMoneyToPP(tmp->CastToClient()->tradecp,tmp->CastToClient()->tradesp,tmp->CastToClient()->tradegp,tmp->CastToClient()->tradepp,true);
								if ((this->tradecp+this->tradesp+this->tradegp+this->tradepp)>0)
									tmp->CastToClient()->AddMoneyToPP(this->tradecp,this->tradesp,this->tradegp,this->tradepp,true);
								tmp->CastToClient()->QueuePacket(outapp);
								tmp->CastToClient()->tradeitems[0]=NULL;
								this->tradeitems[0]=NULL;
								tmp->CastToClient()->FinishTrade(tmp->CastToClient());
								tmp->CastToClient()->InTrade = 0;
								tmp->CastToClient()->TradeWithEnt = 0;
							}
						}
						delete outapp;
						break;
					}
					case OP_Click_Give:
					{
						//							DumpPacket(app);
						this->InTrade = 2;
						Mob* tmp = entity_list.GetMob(TradeWithEnt);
						if (tmp != 0) {
							if (tmp->IsClient()) {
								if (tmp->CastToClient()->InTrade == 2) {
									cout << "deal ended" << endl;
									bool logtrade=false;
									int admin2=0;
									if (zone->tradevar!=0){
										if (((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp)>0) || (strlen(this->tradeitems)>1))
											admin2=tmp->CastToClient()->Admin();
										else if (((this->tradecp+this->tradesp+this->tradegp+this->tradepp)>0) || (strlen(tmp->CastToClient()->tradeitems)>1))
											admin2=this->Admin();
										else
											admin2=999;
										if (zone->tradevar==7){
												logtrade=true;
										}
										else if ((admin2>=10) && (admin2<20)){
											if ((zone->tradevar<8) && (zone->tradevar>5))
												logtrade=true;
										}
										else if (admin2<=20){
											if ((zone->tradevar<8) && (zone->tradevar>4))
												logtrade=true;
										}
										else if (admin2<=80){
											if ((zone->tradevar<8) && (zone->tradevar>3))
												logtrade=true;
										}
										else if (admin2<=100){
											if ((zone->tradevar<9) && (zone->tradevar>2))
												logtrade=true;
										}
										else if (admin2<=150){
											if (((zone->tradevar<8) && (zone->tradevar>1)) || (zone->tradevar==9))
												logtrade=true;
										}
										else if (admin2<=255){
											if ((zone->tradevar<8) && (zone->tradevar>0))
												logtrade=true;	
										}
									}

									if (logtrade==true){
									char* logtext;
									char tradername[71];
									memset(tradername,0,sizeof(tradername));
									if (((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp)>0) || (strlen(this->tradeitems)>1))
									{
										char money[100];
										memset(money,0,sizeof(money));
										sprintf(tradername,"%s gave ",this->GetName());
										logtext=tradername;

									if (strlen(this->tradeitems)>1)
										strcat(logtext,this->tradeitems);

									if ((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp)>0){	
										sprintf(money," %i PP %i GP %i SP %i CP",tmp->CastToClient()->tradepp,tmp->CastToClient()->tradegp,tmp->CastToClient()->tradesp,tmp->CastToClient()->tradecp);
										strcat(logtext,money);
									}
									if (((this->tradecp+this->tradesp+this->tradegp+this->tradepp) > 0) || strlen(tmp->CastToClient()->tradeitems)>1){
										strcat(logtext,"and received ");
										if (strlen(tmp->CastToClient()->tradeitems)>1)
											strcat(logtext,tmp->CastToClient()->tradeitems);
										if ((this->tradecp+this->tradesp+this->tradegp+this->tradepp) > 0){
											sprintf(money," %i PP %i GP %i SP %i CP",this->tradepp,this->tradegp,this->tradesp,this->tradecp);
											strcat(logtext,money);
										}
									}
									database.logevents(this->AccountName(),this->AccountID(),this->admin,this->GetName(),tmp->CastToClient()->GetName(),"Trade",logtext,6);
									}
									else if (((this->tradecp+this->tradesp+this->tradegp+this->tradepp)>0) || (strlen(tmp->CastToClient()->tradeitems)>1))
									{
										char money[100];
										memset(money,0,sizeof(money));
										sprintf(tradername,"%s gave ",tmp->CastToClient()->GetName());
										logtext=tradername;
									if (strlen(tmp->CastToClient()->tradeitems)>1)
										strcat(logtext,tmp->CastToClient()->tradeitems);
									if ((this->tradecp+this->tradesp+this->tradegp+this->tradepp)>0){
										sprintf(money," %i PP %i GP %i SP %i CP",this->tradepp,this->tradegp,this->tradesp,this->tradecp);
										strcat(logtext,money);
									}
									if (((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp) > 0) || strlen(this->tradeitems)>1){
										strcat(logtext,"and received ");
										if (strlen(this->tradeitems)>1)
											strcat(logtext,this->tradeitems);
										if ((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp) > 0){
											sprintf(money," %i PP %i GP %i SP %i CP",tmp->CastToClient()->tradepp,tmp->CastToClient()->tradegp,tmp->CastToClient()->tradesp,tmp->CastToClient()->tradecp);
											strcat(logtext,money);
										}
									}
									database.logevents(tmp->CastToClient()->AccountName(),tmp->CastToClient()->AccountID(),tmp->CastToClient()->admin,tmp->CastToClient()->GetName(),this->GetName(),"Trade",logtext,6);
									}
									}
									tmp->CastToClient()->tradeitems[0]=NULL;
									this->tradeitems[0]=NULL;
									//entity_list.GetID(this->TradeWithEnt)->CastToClient()->Message
									if ((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp) > 0)
										tmp->CastToClient()->AddMoneyToPP(tmp->CastToClient()->tradecp,tmp->CastToClient()->tradesp,tmp->CastToClient()->tradegp,tmp->CastToClient()->tradepp,true);
									if ((this->tradecp+this->tradesp+this->tradegp+this->tradepp)>0)
										this->AddMoneyToPP(this->tradecp,this->tradesp,this->tradegp,this->tradepp,true);
									tmp->CastToClient()->QueuePacket(app);
									this->FinishTrade(tmp->CastToClient());
									tmp->CastToClient()->FinishTrade(this);
									APPLAYER* outapp = new APPLAYER(OP_CloseTrade,0);
									tmp->CastToClient()->QueuePacket(outapp);
									FastQueuePacket(&outapp);
									tmp->CastToClient()->InTrade = 0;
									InTrade = 0;
									tmp->CastToClient()->TradeWithEnt = 0;
									TradeWithEnt = 0;
								}
								else
									tmp->CastToClient()->QueuePacket(app);
							}
							else {
								APPLAYER* outapp = new APPLAYER(OP_CloseTrade,0);
								FastQueuePacket(&outapp);
								cout << "npc deal ended" << endl;

								InTrade = 0;
								TradeWithEnt = 0;
								if (tmp->IsNPC()) {
									char temp1[100];
									memset(temp1,0x0,100);
									char temp2[100];
									memset(temp2,0x0,100);
									for ( int z=0; z < 4; z++ ) {
										sprintf(temp1,"item%d.%d", z+1,tmp->GetNPCTypeID());
										sprintf(temp2,"%d", TradeList[z]);
										parse->AddVar(temp1,temp2);
										memset(temp1,0x0,100);
										memset(temp2,0x0,100);
										const Item_Struct* item2 = 0;
										item2 = database.GetItem(TradeList[z]);
										if (item2) {
										sprintf(temp1,"item%d.stack.%d", z+1, tmp->GetNPCTypeID());
										sprintf(temp2,"%d",glob);
//										printf("Adding: %s and %s\n", temp1, temp2);
										parse->AddVar(temp1,temp2);
										memset(temp1,0x0,100);
										memset(temp2,0x0,100);
										glob = 0;
										}
									}
									sprintf(temp1,"copper.%d",tmp->GetNPCTypeID());
									sprintf(temp2,"%i",tradecp);
									parse->AddVar(temp1,temp2);
									memset(temp1,0x0,100);
									memset(temp2,0x0,100);

									sprintf(temp1,"silver.%d",tmp->GetNPCTypeID());
									sprintf(temp2,"%i",tradesp);
									parse->AddVar(temp1,temp2);
									memset(temp1,0x0,100);
									memset(temp2,0x0,100);

									sprintf(temp1,"gold.%d", tmp->GetNPCTypeID());
									sprintf(temp2,"%i",tradegp);
									parse->AddVar(temp1,temp2);
									memset(temp1,0x0,100);
									memset(temp2,0x0,100);

									sprintf(temp1,"platinum.%d", tmp->GetNPCTypeID());
									sprintf(temp2,"%i",tradepp);
									parse->AddVar(temp1,temp2);
									memset(temp1,0x0,100);
									memset(temp2,0x0,100);

									pp.copper -= tradecp; // Cant put more in trade than you have so no need for checks
									pp.silver -= tradesp;
									pp.gold -= tradegp;
									pp.platinum -= tradepp;
									for(int y=0;y<4;y++) {
										if (TradeList[y]!=0) {
											LinkedListIterator<ServerLootItem_Struct*> iterator(*tmp->CastToNPC()->itemlist);
											iterator.Reset();
											int xy = 0;

											while(iterator.MoreElements()) {
												xy++;
												iterator.Advance();
											}
											if (xy <20){
												NPC* npc=tmp->CastToNPC();
												const Item_Struct* item2 = 0;
												item2 = database.GetItem(TradeList[y]);
												if (item2 && (item2->common.magic != 0 && Admin() == 80) || (item2->nodrop != 0 || this->GetGM())) { //no "no drop" items for j00!
													ServerLootItem_Struct* item = new ServerLootItem_Struct;
													item->item_nr = item2->item_nr;
													item->charges = TradeCharges[y];
													char newid[20];
													memset(newid, 0, sizeof(newid));
													for(int i=0;i<7;i++){
														if (!isalpha(item2->idfile[i])){
															strncpy(newid, &item2->idfile[i],5);
															i=8;
														}
													}
													APPLAYER* outapp = new APPLAYER(OP_WearChange, sizeof(WearChange_Struct));
	 												WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;
	 												wc->spawn_id = npc->GetID();
													wc->material=0;
													if (((item2->equipableSlots==24576) || (item2->equipableSlots==8192)) && (npc->d_meele_texture1==0)) {
														wc->wear_slot_id=7;
														if (item2->common.spellId0!=0)
															npc->CastToMob()->AddProcToWeapon(item2->common.spellId0,true);
														npc->equipment[7]=item2->item_nr;
														npc->d_meele_texture1=atoi(newid);
														if (item2->common.material >0)
															wc->material=item2->common.material;
														else

															wc->material=atoi(newid);
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;
														npc->INT+=item2->common.INT;
													}
													else if (((item2->equipableSlots==24576) || (item2->equipableSlots==16384)) && (npc->d_meele_texture2 ==0) && ((npc->GetLevel()>=13) || (item2->common.damage==0))) 
													{
														if (item2->common.spellId0!=0)
															npc->CastToMob()->AddProcToWeapon(item2->common.spellId0,true);
														npc->d_meele_texture2=atoi(newid);
														npc->equipment[8]=item2->item_nr;
														wc->wear_slot_id=8;
														if (item2->common.material >0)
															wc->material=item2->common.material;
														else
															wc->material=atoi(newid);
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;
														npc->INT+=item2->common.INT;
													}
													else if ((item2->equipableSlots==4) && (npc->equipment[0]==0)){

														npc->equipment[0]=atoi(newid);
														if (item2->common.material >0)
															wc->material=item2->common.material;
														else
															wc->material=atoi(newid);
														wc->wear_slot_id=0;
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;
														npc->INT+=item2->common.INT;
													}
													else if ((item2->equipableSlots==131072) && (npc->equipment[1]==0)){
														npc->equipment[1]=atoi(newid);
														if (item2->common.material >0)
															wc->material=item2->common.material;
														else
															wc->material=atoi(newid);
														wc->wear_slot_id=1;
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;
														npc->INT+=item2->common.INT;
													}
													else if ((item2->equipableSlots==128) && (npc->equipment[2]==0)){
														npc->equipment[2]=atoi(newid);
														if (item2->common.material >0)
															wc->material=item2->common.material;
														else
															wc->material=atoi(newid);
														wc->wear_slot_id=2;
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;

														npc->INT+=item2->common.INT;
													}
													else if ((item2->equipableSlots==1536) && (npc->equipment[3]==0)){
														npc->equipment[3]=atoi(newid);
														if (item2->common.material >0)
															wc->material=item2->common.material;
														else
															wc->material=atoi(newid);
														wc->wear_slot_id=3;
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;
														npc->INT+=item2->common.INT;
													}

													else if ((item2->equipableSlots==4096) && (npc->equipment[4]==0)){
														npc->equipment[4]=atoi(newid);
														if (item2->common.material >0)
															wc->material=item2->common.material;
														else
															wc->material=atoi(newid);
														wc->wear_slot_id=4;
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;
														npc->INT+=item2->common.INT;
													}
													else if ((item2->equipableSlots==262144) && (npc->equipment[5]==0)){
														npc->equipment[5]=atoi(newid);
														if (item2->common.material >0)

															wc->material=item2->common.material;
														else
															wc->material=atoi(newid);
														wc->wear_slot_id=5;
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;
														npc->INT+=item2->common.INT;
													}
													else if ((item2->equipableSlots==524288) && (npc->equipment[6]==0)){
														npc->equipment[6]=atoi(newid);
														if (item2->common.material >0)
															wc->material=item2->common.material;
														else
															wc->material=atoi(newid);
														wc->wear_slot_id=6;
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;


														npc->INT+=item2->common.INT;
													}
													if (((npc->GetRace()==127) && (npc->CastToMob()->GetOwnerID()!=0)) && (item2->equipableSlots==24576) || (item2->equipableSlots==8192) || (item2->equipableSlots==16384)){
														npc->d_meele_texture2=atoi(newid);
														wc->wear_slot_id=8;
														if (item2->common.material >0)
															wc->material=item2->common.material;
														else
															wc->material=atoi(newid);
														npc->AC+=item2->common.AC;
														npc->STR+=item2->common.STR;
														npc->INT+=item2->common.INT;
													}
													item->equipSlot = item2->equipableSlots;
													(*npc->itemlist).Append(item);
	 												entity_list.QueueClients(this, outapp);
	 												delete outapp;
												}
											}
										}
									}
									parse->Event(EVENT_ITEM, tmp->GetNPCTypeID(), 0, tmp, this->CastToMob());
								}
							}
						}
						break;
					}
					case OP_Random: {
						const Random_Struct* rndq = (const Random_Struct*) app->pBuffer;
						if (rndq->low == rndq->high)
							break; // This stops a Zone Crash.
						uint32 randLow;
						uint32 randHigh;
						uint32 rndqa = 0;
						if (rndq->low > rndq->high) { // They did Big number first.
							randHigh = rndq->low;
							randLow = rndq->high;
						}
						else {


							randHigh = rndq->high;
							randLow = rndq->low;
						}
						if (randLow == 0)

							rndqa = 2;
						else
							rndqa = randLow;

						srand( (Timer::GetCurrentTime() * Timer::GetCurrentTime()) / rndqa);
						uint32 tmp = (randHigh - randLow);
						if (tmp == 0)
							tmp = 1;
						rndqa = (rand() % tmp) + randLow;

						entity_list.Message(0, 8, "%s rolled their dice and got a %d out of %d to %d!", this->GetName(), rndqa, randLow, randHigh);

						break;
					}
					case OP_Buff: {
						//						if (app->size != sizeof(SpellBuffFade_Struct)) {
						if (app->size != sizeof(Buff_Struct)) {
							cout << "Wrong size on OP_Buff. Got: " << app->size << ", Expected: " << sizeof(SpellBuffFade_Struct) << endl;
							break;
						}

						DumpPacket(app);
						SpellBuffFade_Struct* sbf = (SpellBuffFade_Struct*) app->pBuffer;
						if(sbf->spellid == 0xFFFF)
							QueuePacket(app);
						else
							BuffFade(sbf->spellid);
						break;
					}
					case OP_GMHideMe: {
						//cout << "OP_GMHideMe" << endl;
						//DumpPacket(app);
						if(this->Admin() < 80) {
							Message(13, "Your account has been reported for hacking.");

							database.SetHackerFlag(this->account_name, this->name, "/hideme");
							break;
						}
						APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
						SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
						sa_out->spawn_id = GetID();
						sa_out->type = 0x03;
						if (IsInvisible() == false) {
							this->invisible = true;
							sa_out->parameter = 1;
						}
						else {
							this->invisible = false;
							sa_out->parameter = 0;
						}

						entity_list.QueueClients(this, outapp, true);
						delete outapp;
						break;
					}
					case OP_GMNameChange: {
						const GMName_Struct* gmn = (const GMName_Struct *)app->pBuffer;
						if(this->Admin() < 80){
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/name");
							break;
						}
						Client* client = entity_list.GetClientByName(gmn->oldname);
						printf("%s %s\n",gmn->oldname,gmn->newname);
						bool usedname = database.CheckUsedName((char*) gmn->newname);
						if(client==0) {
							Message(13, "%s not found for name change. Operation failed!", gmn->oldname);
							break;
						}
						if((strlen(gmn->newname) > 63) || (strlen(gmn->newname) == 0)) {
							Message(13, "Invalid number of characters in new name (%s).", gmn->newname);
							break;
						}
						if (!usedname) {
							Message(13, "%s is already in use.  Operation failed!", gmn->newname);
							break;
						}
						database.UpdateName(gmn->oldname, gmn->newname);
						strcpy(client->name, gmn->newname);
						client->Save();

						if(gmn->badname==1) {
							database.AddToNameFilter(gmn->oldname);
						}

						APPLAYER* outapp = app->Copy();
						GMName_Struct* gmn2 = (GMName_Struct*) outapp->pBuffer;
						gmn2->unknown[0] = 1;
						gmn2->unknown[1] = 1;
						gmn2->unknown[2] = 1;
						entity_list.QueueClients(this, outapp, false);
						delete outapp;
						UpdateWho();
						break;
					}
					case OP_GMKill: {
						if(this->Admin() < 80) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/kill");
							break;
						}
						GMKill_Struct* gmk = (GMKill_Struct *)app->pBuffer;
						Mob* obj = entity_list.GetMob(gmk->name);
						Client* client = entity_list.GetClientByName(gmk->name);
						if(obj!=0) {
							if(client!=0) {
								entity_list.QueueClients(this,app);
							}
							else {
								obj->Kill();
							}
						}
						else {
							if (!worldserver.Connected())
								Message(0, "Error: World server disconnected");
							else {
								ServerPacket* pack = new ServerPacket;
								pack->opcode = ServerOP_KillPlayer;
								pack->size = sizeof(ServerKillPlayer_Struct);
								pack->pBuffer = new uchar[pack->size];
								ServerKillPlayer_Struct* skp = (ServerKillPlayer_Struct*) pack->pBuffer;
								strcpy(skp->gmname, gmk->gmname);
								strcpy(skp->target, gmk->name);
								skp->admin = this->Admin();
								worldserver.SendPacket(pack);
								delete pack;
							}
						}
						break;
					}
					case OP_GMLastName: {
						//cout << "OP_GMLastName" << endl;
						//DumpPacket(app);
						if (app->size != sizeof(GMLastName_Struct)) {

							cout << "Wrong size on OP_GMLastName. Got: " << app->size << ", Expected: " << sizeof(GMLastName_Struct) << endl;
							break;
						}
						GMLastName_Struct* gmln = (GMLastName_Struct*) app->pBuffer;
						if (strlen(gmln->lastname) >= 64) {
							Message(13, "/LastName: New last name too long. (max=63)");
						}
						else {

							Client* client = entity_list.GetClientByName(gmln->name);

							if (client == 0) {
								Message(13, "/LastName: %s not found", gmln->name);
							}

							else {
								if (this->Admin() < 80) {
									Message(13, "Your account has been reported for hacking.");
									database.SetHackerFlag(client->account_name, client->name, "/lastname");
									break;
								}

								else
									client->ChangeLastName(gmln->lastname);
							}
							gmln->unknown[0] = 1;
							gmln->unknown[1] = 1;
							entity_list.QueueClients(this, app, false);
						}
						break;
					}
					case OP_GMToggle: {
						if (app->size != 68) {

							cout << "Wrong size on OP_GMToggle. Got: " << app->size << ", Expected: " << 36 << endl;
							break;
						}
						if (this->Admin() < 80) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/toggle");
							break;
						}
						if (app->pBuffer[64] == 0) {
							Message(0, "Turning tells OFF");
							tellsoff = true;
						}
						else if (app->pBuffer[64] == 1) {
							Message(0, "Turning tells ON");

							tellsoff = false;
						}
						else {
							Message(0, "Unkown value in /toggle packet");
						}
						UpdateWho();
						break;
					}
					case OP_LFG: {
						if (app->size != sizeof(LFG_Struct)) {
							cout << "Wrong size on OP_LFG. Got: " << app->size << ", Expected: " << sizeof(LFG_Struct) << endl;
							break;
						}
						LFG_Struct* lfg = (LFG_Struct*) app->pBuffer;
						if (lfg->value == 1) {
							//cout << this->GetName() << " turning LFG on." << endl;
							LFG = true;
						}
						else if (lfg->value == 0) {
							//cout << this->GetName() << " turning LFG off." << endl;
							LFG = false;
						}
						else
							Message(0, "Error: unknown LFG value");
						UpdateWho();
						break;
					}
					case OP_GMGoto: {
						if (app->size != sizeof(GMSummon_Struct)) {
							cout << "Wrong size on OP_GMGoto. Got: " << app->size << ", Expected: " << sizeof(GMSummon_Struct) << endl;
							break;
						}
						if (this->Admin() < 80) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/goto");
							break;
						}
						GMSummon_Struct* gmg = (GMSummon_Struct*) app->pBuffer;
						Mob* gt = entity_list.GetMob(gmg->charname);
						if (gt != 0) {
							this->MovePC((char*) 0, gt->GetY(), gt->GetX(), gt->GetZ()); // Y and X have to be reversed
						}
						else if (!worldserver.Connected())
							Message(0, "Error: World server disconnected.");

						else {
							ServerPacket* pack = new ServerPacket;
							pack->opcode = ServerOP_GMGoto;
							pack->size = sizeof(ServerGMGoto_Struct);
							pack->pBuffer = new uchar[pack->size];
							memset(pack->pBuffer, 0, pack->size);
							ServerGMGoto_Struct* wsgmg = (ServerGMGoto_Struct*) pack->pBuffer;
							strcpy(wsgmg->myname, this->GetName());
							strcpy(wsgmg->gotoname, gmg->charname);
							wsgmg->admin = admin;
							worldserver.SendPacket(pack);
							delete pack;
						}
						break;
									}
									/*
									0 is e7 from 01 to
									1 is 03
									2 is 00
									3 is 00
									4 is ??
									5 is ??
									6 is 00 from a0 to
									7 is 00 from 3f to */
					case OP_ShopRequest: {
						/* this works*/			Merchant_Click_Struct* mc=(Merchant_Click_Struct*)app->pBuffer;
						//						DumpPacket(app);
						if (app->size == sizeof(Merchant_Click_Struct)) {
							// Send back opcode OP_ShopRequest - tells client to open merchant window.
							APPLAYER* outapp = new APPLAYER(OP_ShopRequest, sizeof(Merchant_Click_Struct));
							//DumpPacket(app);
							Merchant_Click_Struct* mco=(Merchant_Click_Struct*)outapp->pBuffer;
							mco->npcid = mc->npcid;
							mco->playerid = mc->playerid;

							mco->unknown[0] = 1; // Merchant command 0x01 = open
							mco->unknown[1] = 0xD4;
							mco->unknown[2] = 0x53;
							mco->unknown[3] = 0x00;
							mco->unknown[4] = 0x64;
							mco->unknown[5] = 0x21; // 32
							mco->unknown[6] = 0x8B; // 139
							mco->unknown[7] = 0x3F;// 63
							
							//DumpPacket(outapp);
							//DumpPacketHex(outapp);
							QueuePacket(outapp);
							delete outapp;
							int merchantid=0;
							Mob* tmp = entity_list.GetMob(mc->npcid);
							if (tmp != 0)
								merchantid=tmp->CastToNPC()->MerchantType;

							if(merchantid == 0)
								break;

							BulkSendMerchantInventory(merchantid,mc->npcid);
						}
						break;
					}
					case OP_BazaarWindow: {
						BazaarWindowStart_Struct* bws=(BazaarWindowStart_Struct*)app->pBuffer;

						if (bws->action == 0x09) {
							APPLAYER* outapp = new APPLAYER(OP_BazaarWindow, sizeof(BazaarWelcome_Struct));
							BazaarWelcome_Struct* bwelcome=(BazaarWelcome_Struct*)outapp->pBuffer;
							bwelcome->beginning.action = 0x09;
							bwelcome->traders = 1;
							const Item_Struct* item = 0;
							int16 count = 0;
							for (unsigned int i=0; i < database.GetMaxItem(); i++) {
								item = database.GetItem(i);
								if (item)
									count++;
							}
							bwelcome->items = count;
							QueuePacket(outapp);
						}
						else if (bws->action == 0x07) {
							BazaarSearch_Struct* bss=(BazaarSearch_Struct*)app->pBuffer;
							APPLAYER* outapp = new APPLAYER(OP_BazaarWindow, sizeof(BazaarSearchResults_Struct));
							BazaarSearchResults_Struct* bsrs=(BazaarSearchResults_Struct*)outapp->pBuffer;
							bsrs->beginning.action = 0x07;
							bsrs->beginning.unknown2 = 0x0001;
							bsrs->seller_nr = this->GetID();

							const Item_Struct* item = 0;
							char* pdest;
							char sName[64];
							int32 temp, count = 0;
							bool class_, race, stat, slot, type;
							strupr(bss->name);
							for (unsigned int i=0; i < database.GetMaxItem(); i++) {
								item = database.GetItem(i);
								if (item) {
									class_ = true; race = true; stat = true; slot = true; type = true;
									strcpy(sName, item->name);
									strupr(sName);
									pdest = strstr(sName, bss->name);
									if (bss->slot != 0xFFFF) {
										temp = item->equipableSlots;
										slot = false;
										for (int i = 1; i <= 21; i++) {
											if (temp % 2 == 1) {
												switch (i - 1)
												{
												case 1:
													if (bss->slot == 1)
														slot = true;
													break;
												case 2:
													if (bss->slot == 2)
														slot = true;
													break;
												case 3:
													if (bss->slot == 3)
														slot = true;
													break;
												case 4:
													if (bss->slot == 4)
														slot = true;

													break;
												case 5:
													if (bss->slot == 5)
														slot = true;
													break;
												case 6:
													if (bss->slot == 6)
														slot = true;
													break;
												case 7:
													if (bss->slot == 7)
														slot = true;

													break;
												case 8:
													if (bss->slot == 8)
														slot = true;
													break;
												case 9:
													if (bss->slot == 9)
														slot = true;
													break;
												case 10:
													if (bss->slot == 10)

														slot = true;
													break;
												case 11:
													if (bss->slot == 11)
														slot = true;
													break;
												case 12:
													if (bss->slot == 12)
														slot = true;
													break;
												case 13:

													if (bss->slot == 13 || bss->slot == 0)
														slot = true;
													break;
												case 14:
													if (bss->slot == 14 || bss->slot == 0)
														slot = true;
													break;
												case 15:
													if (bss->slot == 15)
														slot = true;
													break;
												case 16:
													if (bss->slot == 16)

														slot = true;
													break;
												case 17:
													if (bss->slot == 17)
														slot = true;
													break;
												case 18:
													if (bss->slot == 18)
														slot = true;
													break;
												case 19:
													if (bss->slot == 19)
														slot = true;
													break;
												case 20:



													if (bss->slot == 20)
														slot = true;
													break;
												case 21:
													if (bss->slot == 21)
														slot = true;
													break;
												}
											}
											temp = temp / 2;
										}
									}
									if (bss->type != 0xFFFF) {
										type = false;
										if (bss->type == item->common.skill)
											type = true;
									}
									if (bss->stat != 0xFFFF) {
										stat = false;
										if (bss->stat == 0 && item->common.STR != 0)
											stat = true;
										else if (bss->stat == 1 && item->common.STA != 0)
											stat = true;
										else if (bss->stat == 2 && item->common.AGI != 0)
											stat = true;
										else if (bss->stat == 3 && item->common.DEX != 0)
											stat = true;
										else if (bss->stat == 4 && item->common.INT != 0)
											stat = true;
										else if (bss->stat == 5 && item->common.WIS != 0)
											stat = true;
										else if (bss->stat == 6 && item->common.CHA != 0)

											stat = true;
										else if (bss->stat == 7 && item->common.MR != 0)
											stat = true;
										else if (bss->stat == 8 && item->common.CR != 0)
											stat = true;
										else if (bss->stat == 9 && item->common.FR != 0)
											stat = true;
										else if (bss->stat == 10 && item->common.PR != 0)
											stat = true;
										else if (bss->stat == 11 && item->common.DR != 0)
											stat = true;
										else if (bss->stat == 12 && item->common.MANA != 0)
											stat = true;
										else if (bss->stat == 13 && item->common.HP != 0)
											stat = true;
									}
									if (bss->class_ != 0xFFFF) {
										temp = item->common.classes;
										class_ = false;

										if (temp == 65407)
											class_ = true;
										else {
											for (int i = 1; i <= 15; i++) {

												if (temp % 2 == 1) {
													switch (i)
													{
													case 1:
														if (bss->class_ == 1)
															class_ = true;
														break;
													case 2:
														if (bss->class_ == 2)
															class_ = true;
														break;

													case 3:
														if (bss->class_ == 3)
															class_ = true;
														break;
													case 4:
														if (bss->class_ == 4)
															class_ = true;
														break;
													case 5:
														if (bss->class_ == 5)
															class_ = true;
														break;
													case 6:
														if (bss->class_ == 6)

															class_ = true;
														break;
													case 7:
														if (bss->class_ == 7)
															class_ = true;
														break;
													case 8:
														if (bss->class_ == 8)
															class_ = true;
														break;
													case 9:
														if (bss->class_ == 9)
															class_ = true;
														break;
													case 10:
														if (bss->class_ == 10)
															class_ = true;
														break;
													case 11:
														if (bss->class_ == 11)
															class_ = true;
														break;
													case 12:
														if (bss->class_ == 12)
															class_ = true;
														break;
													case 13:

														if (bss->class_ == 13)
															class_ = true;
														break;
													case 14:
														if (bss->class_ == 14)
															class_ = true;

														break;
													case 15:
														if (bss->class_ == 15)
															class_ = true;
														break;
													}
												}
												temp = temp / 2;
											}
										}
									}
									if (bss->race != 0xFFFF) {
										temp = item->common.normal.races;
										race = false;

										if (temp == 16383)
											race = true;
										else {
											for (int i = 1; i <= 14; i++) {
												if (temp % 2 == 1) {

													switch (i)
													{
													case 1:
														if (bss->race == 1)
															race = true;
														break;

													case 2:
														if (bss->race == 2)
															race = true;
														break;
													case 3:
														if (bss->race == 3)
															race = true;
														break;
													case 4:
														if (bss->race == 4)
															race = true;
														break;
													case 5:
														if (bss->race == 5)
															race = true;
														break;
													case 6:
														if (bss->race == 6)
															race = true;
														break;
													case 7:
														if (bss->race == 7)
															race = true;
														break;
													case 8:
														if (bss->race == 8)
															race = true;
														break;
													case 9:
														if (bss->race == 9)
															race = true;
														break;
													case 10:

														if (bss->race == 10)
															race = true;
														break;
													case 11:
														if (bss->race == 11)
															race = true;
														break;
													case 12:
														if (bss->race == 12)
															race = true;
														break;
													case 13:
														if (bss->race == 13)
															race = true;
														break;
													case 14:
														if (bss->race == 14)
															race = true;
														break;
													}
												}
												temp = temp / 2;
											}
										}
									}
									if (pdest != NULL && slot && type && stat && class_) {
										bsrs->item_nr = item->item_nr;
										bsrs->cost = item->cost;
										strcpy(bsrs->name, item->name);
										count++;
										QueuePacket(outapp);
									}
									if (count == 100)
										break;
								}
							}
						}
						else if (bws->action == 0x0E) {
							APPLAYER* outapp = new APPLAYER(OP_BazaarWindow, sizeof(BazaarInspectItem_Struct));

							BazaarInspectItem_Struct* bis=(BazaarInspectItem_Struct*)outapp->pBuffer;
							bis->action = 0x0E;
							const Item_Struct* item = database.GetItem(bws->unknown2);



							if (item)
								memcpy(&bis->itemdata, item, sizeof(Item_Struct));
							QueuePacket(outapp);
						}
						else {
							cout << "I suck" << endl;
							//							DumpPacket(app);
						}
						break;
					}
					case OP_ShopPlayerBuy: { // Edited Merkur  03/12
						//cout << name << " is attempting to purchase an item..  " << endl;
						Merchant_Purchase_Struct* mp=(Merchant_Purchase_Struct*)app->pBuffer;
						if (DEBUG >= 5){
                            LogFile->write(EQEMuLog::Debug, "%s, purchase item..", GetName());
                            LogFile->write(EQEMuLog::Debug,"MPS: cost:%i", mp->itemcost);
                            DumpPacket(app);
                        }
						
						int merchantid;
						Mob* tmp = entity_list.GetMob(mp->npcid);
						if (tmp != 0)
							merchantid=tmp->CastToNPC()->MerchantType;
						else
							break;
						uint16 item_nr = database.GetMerchantData(merchantid, mp->itemslot+1);
						const Item_Struct* item = NULL;
						if (item_nr == 0) { // Inventory item?
							char mki[3] = "";
							if (database.GetVariable("MerchantsKeepItems", mki, 3) && mki[0] == '1'  && tmp->CastToNPC()->CountLoot() != 0 ) {
								int vlc = tmp->CastToNPC()->CountLoot();
								int i_slot = ( mp->itemslot - database.GetMerchantListNumb(merchantid) );
								int i_quan = tmp->CastToNPC()->GetItem(i_slot)->charges;
								int i_num = tmp->CastToNPC()->GetItem(i_slot)->item_nr;
                                if (DEBUG>=5) LogFile->write(EQEMuLog::Debug,
                                  "MerchantsKeepItems: vlc:%i i_slot:%i i_quan:%i i_num:%i",
                                    vlc, i_slot, i_quan, i_num);
								item = database.GetItem( i_num );
								if (i_quan < mp->quantity) {
									mp->quantity = (1 * tmp->CastToNPC()->GetItem(i_slot)->charges);
								}
								tmp->CastToNPC()->RemoveItem(i_num, mp->quantity,i_slot);
							} else {
								// MerchantsKeepItems off, not setup, or Vendor has no non-db items
								APPLAYER* outapp = new APPLAYER(OP_ShopPlayerBuy, sizeof(Merchant_Purchase_Struct));
								Merchant_Purchase_Struct* mpo=(Merchant_Purchase_Struct*)outapp->pBuffer;
								mpo->quantity = mp->quantity;
								mpo->playerid = mp->playerid;
								mpo->npcid = mp->npcid;
								mpo->itemslot = mp->itemslot;
								// ? Client says the correct price
								mpo->itemcost = 0;
								// 0x01 0x02 0xFF has no effect client side as long as it's not 0x00 the client doesn't subtract money
								// aka bool NoSale
								mpo->IsSold = 0x01;
								// Changeing this changes the cost the client displays
								// cost multplier for purchaseing in stacks?
								mpo->unknown005 = 0x00;
								QueuePacket(outapp);
								delete outapp;
								Message(0, "%s tells you, Sorry I seem to have misplaced that item try back later.", tmp->GetName());
								break;
							}
						} else {
							item = database.GetItem(item_nr);
						}
						if (!item)
							break;
						
						APPLAYER* outapp = new APPLAYER(OP_ShopPlayerBuy, sizeof(Merchant_Purchase_Struct));
						Merchant_Purchase_Struct* mpo=(Merchant_Purchase_Struct*)outapp->pBuffer;
						mpo->quantity = mp->quantity;
						mpo->playerid = mp->playerid;
						mpo->npcid = mp->npcid;
						mpo->itemslot = mp->itemslot;
						mpo->IsSold = 0x00;
/*						mpo->unknown001 = 0x00;
						mpo->unknown002 = 0x00;
						mpo->unknown003 = 0x00;
						mpo->unknown004 = 0x00;
						mpo->unknown005 = 0x00;
*/
						// need to find the formula to calculate sell prices -Merkur
						//mpo->itemcost = (int)((item->cost*mp->quantity)+(item->cost*mp->quantity*.0875));
						mpo->itemcost = (int)((item->cost*mp->quantity)* 5.435f);
						LogFile->write(EQEMuLog::Debug, "Item cost:%i cost:%i", item->cost, mpo->itemcost);
						//	DumpPacketHex(app);
						if (zone->merchantvar!=0){
							if (zone->merchantvar==7){
									LogMerchant(this,tmp,mpo,item,true);
							}
							else if ((admin>=10) && (admin<20)){
								if ((zone->merchantvar<8) && (zone->merchantvar>5))
									LogMerchant(this,tmp,mpo,item,true);
							}
							else if (admin<=20){
								if ((zone->merchantvar<8) && (zone->merchantvar>4))
									LogMerchant(this,tmp,mpo,item,true);
							}
							else if (admin<=80){
								if ((zone->merchantvar<8) && (zone->merchantvar>3))
									LogMerchant(this,tmp,mpo,item,true);
							}
							else if (admin<=100){
								if ((zone->merchantvar<9) && (zone->merchantvar>2))
									LogMerchant(this,tmp,mpo,item,true);
							}
							else if (admin<=150){
								if (((zone->merchantvar<8) && (zone->merchantvar>1)) || (zone->merchantvar==9))
									LogMerchant(this,tmp,mpo,item,true);
							}
							else if (admin<=255){
								if ((zone->merchantvar<8) && (zone->merchantvar>0))
									LogMerchant(this,tmp,mpo,item,true);	
							}
							}
						int16 slot = FindFreeInventorySlot(0, (item->type==0x01), false);
						if(slot != 0xFFFF && TakeMoneyFromPP((int)((item->cost*mp->quantity)+(item->cost*mp->quantity*.0875))))
						{
						if(item->common.charges == 255)
							mp->quantity = 255;
						PutItemInInventory(slot, item, mp->quantity);
						}

						else
						{
						//mpo->itemcost = 0;

						mpo->IsSold = 0x01;
						mpo->unknown001 = 0x00;
						mpo->unknown003 = 0x00;
						mpo->unknown004 = 0x00;
						mpo->unknown005 = 0x00;
						}
						QueuePacket(outapp);
						delete outapp;
						break;
					}
					case OP_ShopPlayerSell: {
						Merchant_Purchase_Struct* mp=(Merchant_Purchase_Struct*)app->pBuffer;
						// Give the player thier money
						Mob* vendor = entity_list.GetMob(mp->npcid);
						Item_Struct* item2 = NULL;
						if (mp->itemslot >= 250 && mp->itemslot <= 339) {
							// Bagged item
							// mp->quantity is always atleast one, don't decrease the charge here we're using DeleteItemInInventory() to remove the charge and empty the slot if it's the last/only one

							//pp.bagitemproperties[mp->itemslot-250].charges = (mp->quantity == 0) ? 0: pp.bagitemproperties[mp->itemslot-250].charges - mp->quantity;
							const Item_Struct* item = database.GetItem(pp.containerinv[mp->itemslot-250]);
							if (item){
								AddMoneyToPP((int)(item->cost*((mp->quantity == 0) ? 1:mp->quantity))-(item->cost * ((mp->quantity == 0) ? 1:mp->quantity) *0.08),false);
								if (zone->merchantvar!=0){
									if (zone->merchantvar==7){
											LogMerchant(this,vendor,mp,item,false);
									}
									else if ((admin>=10) && (admin<20)){
										if ((zone->merchantvar<8) && (zone->merchantvar>5))
											LogMerchant(this,vendor,mp,item,false);
									}
									else if (admin<=20){
										if ((zone->merchantvar<8) && (zone->merchantvar>4))
											LogMerchant(this,vendor,mp,item,false);
									}
									else if (admin<=80){
										if ((zone->merchantvar<8) && (zone->merchantvar>3))
											LogMerchant(this,vendor,mp,item,false);
									}
									else if (admin<=100){
										if ((zone->merchantvar<9) && (zone->merchantvar>2))
											LogMerchant(this,vendor,mp,item,false);
									}
									else if (admin<=150){
										if (((zone->merchantvar<8) && (zone->merchantvar>1)) || (zone->merchantvar==9))
											LogMerchant(this,vendor,mp,item,false);
									}
									else if (admin<=255){
										if ((zone->merchantvar<8) && (zone->merchantvar>0))
											LogMerchant(this,vendor,mp,item,false);	
									}
									}
							}


							else
								Message(0, "Error #1, item == 0");
							// Don't delete the item yet the merchant needs to make a copy
						}
						else if(mp->itemslot <= 30) {
							// Main inventory or worn items
							// Again using DeleteItemInInventory()
							// pp.invitemproperties[mp->itemslot].charges = (mp->quantity == 0) ? 0: pp.invitemproperties[mp->itemslot].charges - mp->quantity;
							const Item_Struct* item = database.GetItem(pp.inventory[mp->itemslot]);
							if (item){
								AddMoneyToPP((int)(item->cost*((mp->quantity == 0) ? 1:mp->quantity))-(item->cost * ((mp->quantity == 0) ? 1:mp->quantity) *0.08),false);
								if (zone->merchantvar!=0){
									if (zone->merchantvar==7){
											LogMerchant(this,vendor,mp,item,false);
									}
									else if ((admin>=10) && (admin<20)){
										if ((zone->merchantvar<8) && (zone->merchantvar>5))
											LogMerchant(this,vendor,mp,item,false);
									}
									else if (admin<=20){
										if ((zone->merchantvar<8) && (zone->merchantvar>4))
											LogMerchant(this,vendor,mp,item,false);
									}
									else if (admin<=80){
										if ((zone->merchantvar<8) && (zone->merchantvar>3))
											LogMerchant(this,vendor,mp,item,false);
									}

									else if (admin<=100){
										if ((zone->merchantvar<9) && (zone->merchantvar>2))
											LogMerchant(this,vendor,mp,item,false);
									}
									else if (admin<=150){
										if (((zone->merchantvar<8) && (zone->merchantvar>1)) || (zone->merchantvar==9))
											LogMerchant(this,vendor,mp,item,false);
									}
									else if (admin<=255){
										if ((zone->merchantvar<8) && (zone->merchantvar>0))
											LogMerchant(this,vendor,mp,item,false);	
									}


									}
							}
							else
								Message(0, "Error #2, item == 0");
						}
						else {
							Message(0, "Error #3, no catch for slot ID#%i", mp->itemslot);
						}
						QueuePacket(app);

						char mki[3] = "";
						if (database.GetVariable("MerchantsKeepItems", mki, 3)) {
#if DEBUG >= 11
  cout<<"MerchantKeepItems: OP_ShopPlayerSell mki="<<mki<<endl;
#endif

						if ( mki[0] == '1' ) {
							Mob* vendor = entity_list.GetMob(mp->npcid);
							if (!vendor)
							    break; // DT who ever just killed the merchant and the player and 12 random people
							int vlc = vendor->CastToNPC()->CountLoot();
							int vdbc = database.GetMerchantListNumb(vendor->CastToNPC()->MerchantType);
							int32 cur_inr = 0;
							bool have_item = false;
							if (mp->itemslot >= 250 && mp->itemslot <= 339) {
							    cur_inr = pp.containerinv[mp->itemslot-250];
							}
							else if (mp->itemslot <= 30) {
							    cur_inr = pp.inventory[mp->itemslot];
							}
							else {
							    // Selling from loy inventory or banker?
							    //cout<<"Error: "<<GetName()<<" selling item from magic rabbit hat!!"<<endl;
							}
							int cur_i; // Give WIN32 a nice big hug
							// Step thru Merchants inventory/loot
							for (cur_i = 0; cur_i < vlc; cur_i++) {
							    if (vendor->CastToNPC()->GetItem(cur_i)->item_nr == cur_inr) {
								// If we find the item in the loottable
								have_item = true; // Trip have_item flag
								vendor->CastToNPC()->GetItem(cur_i)->charges += mp->quantity; // And increase the number merchant has available
								break; // Exit for loop
							    }
							}
							// Now step thru the Merchants db items only if we don't already any of the item in the loottable
							if (!have_item){
							    for (cur_i = 0; cur_i < vdbc; cur_i++) {
									const Item_Struct* tmpItem = database.GetItem(database.GetMerchantData(vendor->CastToNPC()->MerchantType,cur_i+1));
									if ( tmpItem->item_nr == cur_inr) {
										have_item = true; // Found the item in the Merchantlist trip the flag
										break;
									}
							    }
							}
							//cout<<"MerchantsKeepItems vlc:"<<vlc;
							//cout<<" item_nr:"<< cur_inr;
							//cout<<" charge:"<< 1*mp->quantity;
							//cout<<endl;
							if (!have_item) {
							    // Add Item to Merchants inventory
							    vendor->CastToNPC()->AddItem( cur_inr, 1*mp->quantity, vlc);
							}
							// Then remove the item from the player
							this->DeleteItemInInventory(mp->itemslot,1*mp->quantity,false);
						    } else {
							this->DeleteItemInInventory(mp->itemslot,1*mp->quantity,false);
						    }
						} else {
						    // Update zones copy of player inventory
						    cout<<"Deleting item..   MerchantsKeepItems not configured"<<endl;
						    this->DeleteItemInInventory(mp->itemslot);
				
						}
							APPLAYER* outapp = new APPLAYER(OP_ShopRequest, sizeof(Merchant_Click_Struct));
							Merchant_Click_Struct* mco=(Merchant_Click_Struct*)outapp->pBuffer;
							mco->npcid = vendor->GetID();
							mco->playerid = this->GetID();
							mco->unknown[0] = 0x00; // Merchant command 0x01 = open
							mco->unknown[1] = 0xD4;
							mco->unknown[2] = 0x53;
							mco->unknown[3] = 0x00;
							mco->unknown[4] = 0x64;
							mco->unknown[5] = 0x21; // 32
							mco->unknown[6] = 0x8B; // 139
							mco->unknown[7] = 0x3F;// 63
							QueuePacket(outapp);
							delete outapp;
							BulkSendMerchantInventory(vendor->CastToNPC()->MerchantType,vendor->GetID());
						// Just send it back to accept the deal
						//DumpPacket(app);
						Save();
						cout << "response from sell action.." << endl;
						break;
					}
					case OP_ShopEnd: {
						cout << name << " is ending merchant interaction." << endl;

						APPLAYER* outapp = new APPLAYER(OP_ShopEndConfirm, 2);
						outapp->pBuffer[0] = 0x0a;
						outapp->pBuffer[1] = 0x66;
						QueuePacket(outapp);
						delete outapp;
						Save();
						break;
					}
					case OP_ClickObject: {
						ClickObject_Struct* co = (ClickObject_Struct*)app->pBuffer;
						if(co && entity_list.GetID(co->objectID)){
							/*APPLAYER* outapp = new APPLAYER(0x2a40, sizeof(ClickObjectServer_Struct));
							ClickObjectServer_Struct* cos = (ClickObjectServer_Struct*)outapp->pBuffer;
							cos->playerid=co->PlayerID;
							cos->objectid=co->objectID;
							cos->type=0x0F;
							cos->unknown0=0x18;
							cos->unknown9=0x01;
							cos->unknown15[2]=0x0a;
							cos->unknown21=0x045A;
							cos->unknown23[2]=0x05;
							cos->unknown23[3]=0x40;
							entity_list.QueueClients(this,outapp,false);
							DumpPacket(outapp);
							delete outapp;*/ 
							entity_list.GetID(co->objectID)->CastToObject()->HandleClick(this,app);
					/*		APPLAYER* outapp = new APPLAYER(0x2b40, 8);
							int i=co->objectID;
						//	uchar *blah;
						//	int blah2;
						//	blah2=co->objectID;
						//	blah=blah2;
						outapp->pBuffer[0] = 0x00;
						outapp->pBuffer[1] = 0x05;
						outapp->pBuffer[2] = 0x00;
						outapp->pBuffer[3] = 0x00;
						outapp->pBuffer[4] = 0x00;
						outapp->pBuffer[5] = 0x00;
						outapp->pBuffer[6] = 0x00;
						outapp->pBuffer[7] = 0x00;
						entity_list.QueueClients(this,outapp,false);
						DumpPacket(outapp);
						delete outapp;*/
						APPLAYER* outapp2 = new APPLAYER(0x2b40, 8);
						outapp2->pBuffer[0] = 0x00;
						outapp2->pBuffer[1] = 0x05;
						outapp2->pBuffer[2] = 0x00;
						outapp2->pBuffer[3] = 0x00;
						outapp2->pBuffer[4] = 0x00;
						outapp2->pBuffer[5] = 0x00;
						outapp2->pBuffer[6] = 0x00;
						outapp2->pBuffer[7] = 0x00;
						entity_list.QueueClients(this,outapp2,false);
						DumpPacket(outapp2);
						delete outapp2;
						}
							
						break;
						
					}
					case OP_ClickDoor: {
						ClickDoor_Struct* cd = (ClickDoor_Struct*)app->pBuffer;
						Doors* currentdoor = entity_list.FindDoor(cd->doorid);
						if(!currentdoor)
						{
							Message(0,"Unable to find door, please notify a GM (DoorID: %i).",cd->doorid);
							break;
						}

						if (IsSettingGuildDoor){
							if (database.SetGuildDoor(cd->doorid,SetGuildDoorID,zone->GetShortName())){
								cout << SetGuildDoorID << endl;
								if (SetGuildDoorID){
									Message(0,"This is now a guilddoor of '%s'",guilds[SetGuildDoorID].name);
									currentdoor->SetGuildID(SetGuildDoorID);
								}
								else{
											Message(0,"Guildoor deleted");
											currentdoor->SetGuildID(0);
								}
							}
							else
								Message(0,"Failed to edit guilddoor!");
							IsSettingGuildDoor = false;
							break;
						}

						if (cd->doorid >= 128){
							if(!database.CheckGuildDoor(cd->doorid,GuildEQID(),zone->GetShortName())){
								// heh hope grammer on this is right lol
								this->Message(0,"A magic barrier protects this hall against intruders!");

								break;
							}
							else
								this->Message(0,"The magic barrier disappears and you open the door!");

						}

						APPLAYER* outapp = new APPLAYER(OP_MoveDoor, sizeof(MoveDoor_Struct));
						MoveDoor_Struct* md=(MoveDoor_Struct*)outapp->pBuffer;
						//DumpPacket(app);
						md->doorid = cd->doorid;

						if(currentdoor->GetTriggerType() == 255) // this object isnt triggered
						{
						delete outapp;
						break;
						}

						
/*
						//////////////////////////////////////////////////////////////////
						//used_pawn: Locked doors! Rogue friendly too =) 
						//TODO: add check for other lockpick items 
						//////////////////////////////////////////////////////////////////
						if((currentdoor->GetKeyItem()==0) || (currentdoor->GetKeyItem()==this->GetItemAt(0))) 
						{ 
							if(!currentdoor->IsDoorOpen()) 
							{ 
							currentdoor->HandleClick(this); 
							md->action = 0x02; 
							} 
							else 
							{ 
								currentdoor->HandleClick(this); 
								md->action = 0x03; 
							} 
						} 
						else 
						{ 
							if((this->GetItemAt(0)==13010)&&(this->GetSkill(35)>0)) 
							{ 
								if(rand()%100<2)
									this->SetSkill(35,this->GetSkill(35)+1); 
								if(rand()%currentdoor->GetLockpick()<=this->GetSkill(35)) 
								{ 
									if(!currentdoor->IsDoorOpen()) 
									{ 
										currentdoor->HandleClick(this); 
										md->action = 0x02; 
									} 
									else 
									{ 
										currentdoor->HandleClick(this); 
										md->action = 0x03; 
									} 
									Message(4,"You picked the lock!"); 
									} 
								else 
								{ 
									Message(4,"Your skill is not high enough for this door."); 
								} 
							} 
						else 
							{ 
								Message(4,"It's locked and you don't have the key!"); 
							} 
						} 
						//end used_pawn 
						////////////////////////////////////////////////////////////////////////
*/
						if(!currentdoor->IsDoorOpen())
						{
							    currentdoor->HandleClick(this);
								md->action = 0x02;
						}
						else
						{
						currentdoor->HandleClick(this);
						md->action = 0x03;
						}

						//#ifdef WIN32
						int8 action = md->action;
						//if(strcasecmp("FELE2", currentdoor->GetDoorName()) == 0)
						//		md->action = 0x03;
						//#endif

						entity_list.QueueClients(this,outapp,false);

						//printf("ID: %i\n",cd->doorid);
						//DumpPacket(outapp);
						delete outapp;
						
						//#ifdef WIN32
						if(currentdoor->GetTriggerDoorID() != 0)
						{
						Doors* triggerdoor = entity_list.FindDoor(currentdoor->GetTriggerDoorID());
						if(triggerdoor)
						{
						if(!triggerdoor->IsDoorOpen())
						{
							triggerdoor->HandleClick(this);
							action = 0x02;
						}
						else
						{
							triggerdoor->HandleClick(this);
							action = 0x03;
						}

						//printf("%i %i\n",door->trigger_door,action);
						//printf("Triggering: %i\n",door->trigger_door);
						outapp = new APPLAYER(OP_MoveDoor, sizeof(MoveDoor_Struct));
						MoveDoor_Struct* md=(MoveDoor_Struct*)outapp->pBuffer;
						md->doorid = triggerdoor->GetDoorID();
						md->action = action;
						entity_list.QueueClients(this,outapp,false);
						delete outapp;
						}
						}
						//#endif

						break;
					}
					case OP_CreateObject: {
						//DumpPacket(app);
						//DumpPacket(app->pBuffer,app->size);
						//printf("CreateObject\n");
						
						Object_Struct* co = (Object_Struct*)app->pBuffer;
						if (admin<100 && ((co->itemid > 17845 && co->itemid < 17859) || co->itemid==17913 || co->itemid==17914 || co->itemid==17915 || co->itemid==17911 || co->itemid==17907 || co->itemid==17909 || co->itemid==17885)){
						      DeleteItemInInventory(0);
						      Save();
							break;
						}
						if((rand()%100)<20){
							Message(13,"The item you dropped becomes lost on the ground.");
							//pp.inventory[0] = 0xFFFF;
							DeleteItemInInventory(0);
							Save();
							break;
						}
						if (pp.inventory[0] != co->itemid) {
						      LogFile->write(EQEMuLog::Error, "%s droping item not on thier cursor! %i:%i", GetName(), pp.inventory[0], co->itemid);
								break;
						}
						for(int c=0;c<10;c++)
						{
						if (pp.containerinv[80+c] != 0 && pp.containerinv[80+c] != co->itemsinbag[c])
						{
							co->itemsinbag[c] = 0xFFFF;
						}
						}
						Object* no = new Object(OT_DROPPEDITEM,co->itemid,co->ypos,co->xpos,co->zpos,co->heading,co->objectname,pp.invitemproperties[0].charges);
						//strncpy(no->idfile,co->idFile,16);
						for (int i = 0;i < 10;i++) {
							if (co->itemsinbag[i] != 0xFFFF && co->itemsinbag[i] != 0){
								no->SetBagItems(i,co->itemsinbag[i],pp.bagitemproperties[80+i].charges);
							}
							else
								co->itemsinbag[i] = 0xFFFF;
						}
						entity_list.AddObject(no,true);
						for (int j=0;j < 10;j++){
							pp.containerinv[80+j] = 0xFFFF;
							pp.bagitemproperties[80+j].charges = 0;
						}
						//pp.inventory[0] = 0xFFFF;
						DeleteItemInInventory(0);
						Save();
						break;
					}
					case 0x2542: { // When client changes their face & stuff
								   /*cout << "OP: 0x2522" << endl;
						DumpPacket(app);*/
						entity_list.QueueClients(this, app, false);
						ChangeLooks_Struct* cl = (ChangeLooks_Struct*)app->pBuffer;
						pp.haircolor=cl->haircolor;
						pp.beardcolor=cl->beardcolor;
						pp.eyecolor1=cl->eyecolor1;

						pp.eyecolor2=cl->eyecolor2;
						pp.hairstyle=cl->hairstyle;
						pp.title=cl->title;
						pp.luclinface=cl->face;
						Save();
						Message(13, "Facial features updated.");
						break;
					}
					case OP_GroupInvite:
					case OP_GroupInvite2: {
						//DumpPacket(app);
						if(this->GetTarget() != 0 && this->GetTarget()->IsClient())
						{
							this->GetTarget()->CastToClient()->QueuePacket(app);
							break;
						}

						if(this->GetTarget() != 0 && this->GetTarget()->IsNPC() && this->GetTarget()->CastToNPC()->IsInteractive())
						{
							if(!this->GetTarget()->CastToNPC()->IsGrouped())
							{
								APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
								GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;
								gu->action = 9;
								//strcpy(gu->membername,GetTarget()->CastToNPC()->GetName());
								//strcpy(gu->yourname,GetName());
								strcpy(gu->membername,GetName());
								strcpy(gu->yourname,GetTarget()->CastToNPC()->GetName());
								FastQueuePacket(&outapp);
								if (!isgrouped){
									Group* ng = new Group(this);
									entity_list.AddGroup(ng);
								}
								entity_list.GetGroupByClient(this->CastToClient())->AddMember(GetTarget());
								this->GetTarget()->CastToNPC()->TakenAction(22,this->CastToMob());
							}
							else {
							       LogFile->write(EQEMuLog::Debug, "IPC: %s already grouped.", this->GetTarget()->GetName());
							}
						}
						break;
					}
					case OP_CancelInvite: {
						GroupGeneric_Struct* gf = (GroupGeneric_Struct*) app->pBuffer;
						Mob* inviter = entity_list.GetClientByName(gf->name1);
						if(inviter != 0 && inviter->IsClient())
							inviter->CastToClient()->QueuePacket(app);
						break;
					}
					case OP_GroupFollow:
					case OP_GroupFollow2: {
						GroupGeneric_Struct* gf = (GroupGeneric_Struct*) app->pBuffer;
						Mob* inviter = entity_list.GetClientByName(gf->name1);
						if(inviter != 0 && inviter->IsClient()) {
							isgrouped = true;
							strcpy(gf->name1,inviter->GetName());
							strcpy(gf->name2,this->GetName());
							inviter->CastToClient()->QueuePacket(app);

							APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
							memset(outapp->pBuffer,0,outapp->size);
							GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;
							gu->action = 9;
							strcpy(gu->membername,inviter->GetName());
							strcpy(gu->yourname,inviter->GetName());
							inviter->CastToClient()->FastQueuePacket(&outapp);
							if (!inviter->CastToClient()->isgrouped){
								Group* ng = new Group(inviter);
								entity_list.AddGroup(ng);
								cout << "New group created" << endl;
							}
							entity_list.GetGroupByClient(inviter->CastToClient())->AddMember(this);

						}

						break;
					}
					case OP_GroupDisband: {
						printf("Member Disband Request\n");
						GroupGeneric_Struct* gd = (GroupGeneric_Struct*) app->pBuffer;
						Group* group = entity_list.GetGroupByClient(this);
						if(target == 0) {
							if (group)
								group->DisbandGroup();
						}
						else if (this->isgrouped && group != 0)
							group->DelMember(entity_list.GetMob(gd->name2),false);
						break;
					}
					case OP_GroupDelete: {
						printf("Group Delete Request\n");
						Group* group = entity_list.GetGroupByClient(this);
						if (this->isgrouped && group != 0)
							group->DisbandGroup();
						break;
					}
					case OP_GMEmoteZone: {
						if(this->Admin() < 80) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/emote");

							break;
						}
						GMEmoteZone_Struct* gmez = (GMEmoteZone_Struct*)app->pBuffer;
						entity_list.Message(0, 0, gmez->text);
						break;
					}
					case OP_InspectRequest: { // 03/09 - Merkur
						Inspect_Struct* ins = (Inspect_Struct*) app->pBuffer;
						Mob* tmp = entity_list.GetMob(ins->TargetID);
						if(tmp != 0 && tmp->IsClient())
							tmp->CastToClient()->QueuePacket(app); // Send request to target
						else {
							if(tmp != 0 && tmp->IsNPC() && tmp->CastToNPC()->IsInteractive()) {
								APPLAYER* outapp = app->Copy();
								ins = (Inspect_Struct*) outapp->pBuffer;
								int16 id1,id2;
								id1 = ins->PlayerID;
								id2 = ins->TargetID;
								ins->TargetID = id1;
								ins->PlayerID = id2;
								outapp->opcode = OP_InspectAnswer;
								FastQueuePacket(&outapp);
								tmp->CastToNPC()->TakenAction(20,this);
							}
						}
						break;
					}
					case OP_InspectAnswer: {
						Inspect_Struct* ins = (Inspect_Struct*) app->pBuffer;
						Mob* tmp = entity_list.GetMob(ins->TargetID);
						if(tmp != 0 && tmp->IsClient())
							tmp->CastToClient()->QueuePacket(app); // Send answer to requester
						break;
					}
					case OP_Medding: {
						if (app->pBuffer[0])
							medding = true;
						else
							medding = false;
						break;
					}
					case OP_DeleteSpell: {
						if(app->size != sizeof(DeleteSpell_Struct))
							break;
						APPLAYER* outapp = app->Copy();
						DeleteSpell_Struct* dss = (DeleteSpell_Struct*) outapp->pBuffer;
						if(pp.spell_book[dss->spell_slot] != 0) {
							pp.spell_book[dss->spell_slot] = 0xFFFF;
							dss->success = 1;
						}
						else
							dss->success = 0;

						FastQueuePacket(&outapp);
						break;

					}
case OP_Petition: {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int petfound = 0;
	int maxpet = 1;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT dib, charname, accountname from petitions order by dib"), errbuf, &result))
	{
		delete[] query;
		while ((row = mysql_fetch_row(result)) && petfound < maxpet) {
			const char* petdbname = row[1];
			if(petdbname == this->GetName() ){ petfound++; }
		}
	}
	mysql_free_result(result);
	if (app->size <= 1)
		break;
	//app->pBuffer[app->size - 1] = 0;
	if (!worldserver.Connected())
		Message(0, "Error: World server disconnected");
	else if(petfound >= maxpet)
		Message(10, "Your petition query list is full.", this->GetName());
	else
	{
		Petition* pet = new Petition;
		pet->SetAName(this->AccountName());
		pet->SetClass(this->GetClass());
		pet->SetLevel(this->GetLevel());
		pet->SetCName(this->GetName());
		pet->SetRace(this->GetRace());
		pet->SetLastGM("");
		pet->SetCName(this->GetName());
		pet->SetPetitionText((char*) app->pBuffer);
		pet->SetZone(zone->GetZoneID());
		pet->SetUrgency(0);
		petition_list.AddPetition(pet);							
		database.InsertPetitionToDB(pet);
		petition_list.UpdateGMQueue();
		petition_list.UpdateZoneListQueue();
		worldserver.SendEmoteMessage(0, 0, 80, 15, "%s has made a petition. #%i", GetName(), pet->GetID());
	}
	break;
}
					case OP_PetitionCheckIn: {
						Petition_Struct* inpet = (Petition_Struct*) app->pBuffer;
						Petition* pet = petition_list.GetPetitionByID(inpet->petnumber);
						if (inpet->urgency != pet->GetUrgency())
							pet->SetUrgency(inpet->urgency);
						pet->SetLastGM(this->GetName());
						pet->SetCheckedOut(false);
						petition_list.UpdatePetition(pet);
						petition_list.UpdateGMQueue();
						petition_list.UpdateZoneListQueue();
						break;
					}
					case OP_PetitionDelete: {
						APPLAYER* outapp = new APPLAYER(0x0f20,sizeof(PetitionClientUpdate_Struct));
						PetitionClientUpdate_Struct* pet = (PetitionClientUpdate_Struct*) outapp->pBuffer;
						pet->petnumber = *((int*) app->pBuffer);
						pet->color = 0x00;
						pet->status = 0xFFFFFFFF;
						/*pet->unknown[3] = 0x00;
						pet->unknown[2] = 0x00;
						pet->unknown[1] = 0x00;
						pet->unknown[0] = 0x00;*/
						pet->senttime = 0;
						strcpy(pet->accountid, "");
						strcpy(pet->gmsenttoo, "");
						pet->quetotal = petition_list.GetMaxPetitionID();
						strcpy(pet->charname, "");
						FastQueuePacket(&outapp);

						if (petition_list.DeletePetition(pet->petnumber) == -1)
							cout << "Something is borked with: " << pet->petnumber << endl;
						petition_list.ClearPetitions();
						petition_list.UpdateGMQueue();
						petition_list.ReadDatabase();
						petition_list.UpdateZoneListQueue();
						break;
					}
					case OP_PetCommands: {
						PetCommand_Struct* pet = (PetCommand_Struct*) app->pBuffer;

						Mob* mypet = this->GetPet();

						if(mypet == 0)
                        {
                            // try to get a familiar if we dont have a real pet
                            mypet = this->GetFamiliar();
                            if (mypet == NULL)
							    break;
                        }
						
                        if(mypet->GetPetType() == 1)
							break;
						
                        // just let the command "/pet get lost" work for familiars
                        if(mypet == GetFamiliar() && pet->command != PET_GETLOST)
							break;


						switch(pet->command)
						{
						case PET_ATTACK: {
                            if (!target)
                                break;
                            if (target->IsMezzed()) {
                                Message(10,"%s tells you, 'Cannot wake up target, Master.'", mypet->GetName());
                                break;
                            }
							if (mypet->GetHateTop()==0 && target != this) {
								zone->AddAggroMob();
								mypet->AddToHateList(target, 1);
								Message(10,"%s tells you, 'Attacking %s, Master.'", mypet->GetName(), this->target->GetName());
							}

							break;
						}
						case PET_BACKOFF: {
							entity_list.MessageClose(mypet, false, 200, 10, "%s says, 'Sorry, Master..calming down.'", mypet->GetName());
							mypet->WhipeHateList();
							break;
						}
						case PET_HEALTHREPORT: {
							Message(10,"%s tells you, 'I have %d percent of my hit points left.'",mypet->GetName(),(int8)mypet->GetHPRatio());
							break;
						}
						case PET_GETLOST: {
                            if (mypet->GetPetType() == 0xFF || !mypet->IsNPC())
                            {

                                // eqlive ignores this command
                                // we could just remove the charm
                                // and continue
								mypet->BuffFadeByEffect(SE_Charm);
                                break;
                            }
                            if (mypet == GetFamiliar())
                            {
                                SetFamiliarID(0);
                            }
							entity_list.MessageClose(mypet, false, 200, 10, "%s says, 'As you wish, oh great one.'",mypet->GetName());
							mypet->CastToNPC()->Depop();
							break;
						}
						case PET_LEADER: {
							entity_list.MessageClose(mypet, false, 200, 10, "%s says, 'My leader is %s'",mypet->GetName(),this->GetName());
                            break;
						}
						case PET_GUARDHERE: {
                            entity_list.MessageClose(mypet, false, 200, 10, "%s says, 'Guarding with my life..oh splendid one.'",mypet->GetName());
                            mypet->SetPetOrder(SPO_Guard);
                            mypet->SaveGuardSpot();
							break;
						}
						case PET_FOLLOWME: {
                            mypet->SetPetOrder(SPO_Follow);
                            entity_list.MessageClose(mypet, false, 200, 10, "%s says, 'Following Master.'",mypet->GetName());
							mypet->SendAppearancePacket(0x0e,0x64);
							break;
						}
						case PET_TAUNT: {
							break;
						}
						case PET_GUARDME: {
                            entity_list.MessageClose(mypet, false, 200, 10, "%s says, 'Guarding you, Master.'",mypet->GetName());
                            mypet->SetPetOrder(SPO_Follow);
							mypet->SendAppearancePacket(0x0e,0x64);
							break;
						}
						case PET_SITDOWN: {
                            mypet->SetPetOrder(SPO_Sit);
							entity_list.MessageClose(mypet, false, 200, 10, "%s says, 'Changing position, Master.'",mypet->GetName());
							mypet->InterruptSpell(); //Baron-Sprite: No cast 4 u. // neotokyo: i guess the pet should start casting
							mypet->SendAppearancePacket(0x0e,0x6e);
							break;
						}
						case PET_STANDUP: {
                        	mypet->SetPetOrder(SPO_Follow);
							entity_list.MessageClose(mypet, false, 200, 10, "%s says, 'Changing position, Master.'",mypet->GetName());
							mypet->SendAppearancePacket(0x0e,0x64);

							break;
						}
						default: {
							printf("Client attempted to use a unknown pet command:\n");
							//							DumpPacket(app);
							break;
						}
						}
						break;
					}
					case OP_PetitionCheckout: {
						if (app->size != sizeof(int32)) {
							cout << "Wrong size: OP_PetitionCheckout, size=" << app->size << ", expected " << sizeof(int32) << endl;
							break;
						}
						if (!worldserver.Connected())
							Message(0, "Error: World server disconnected");
						else {
							int32 getpetnum = *((int32*) app->pBuffer);
							Petition* getpet = petition_list.GetPetitionByID(getpetnum);
							if (getpet != 0) {
								getpet->AddCheckout();
								getpet->SetCheckedOut(true);
								getpet->SendPetitionToPlayer(this->CastToClient());
								petition_list.UpdatePetition(getpet);
								petition_list.UpdateGMQueue();
								petition_list.UpdateZoneListQueue();
							}
							else Message(0, "Automated System Message: Your client is bugged.  Please camp to the login server to fix!");
						}
						break;
					}
					case OP_PetitionRefresh: {
						// This is When Client Asks for Petition Again and Again...

						// break is here because it floods the zones and causes lag if it
						// Were to actually do something:P  We update on our own schedule now.
						break;
					}
					case OP_TradeSkillCombine: {
						cout << " TradeskillCombine Request" << endl;
						Combine_Struct* combin = (Combine_Struct*) app->pBuffer;
						int16 skillneeded = 0;
						int16 trivial = 0;
						int16 product = 0;
						int16 product2 = 0;
						int16 productcount = 0;
						int16 failproduct = 0;
						int16 tradeskill = 0;
						float chance = 0;
						// statbonus 20%/10% with 200 + 0.05% / 0.025% per point above 200
						float wisebonus =  (pp.WIS > 200) ? 20 + ((pp.WIS - 200) * 0.05) : pp.WIS * 0.1;
						float intbonus =  (pp.INT > 200) ? 10 + ((pp.INT - 200) * 0.025) : pp.INT * 0.05;

						switch(combin->tradeskill)

						{
						case 0x10:	//Tailoring
							tradeskill = 61;
							break;
						case 0x11: //Blacksmithing 
							tradeskill=63; 
							break; 
						case 0x12:	//Fletching
							tradeskill = 64;
							break;
						case 0x13:	//Brewing
							tradeskill = 65;
							break;
						case 0x14:	//Jewelry Making
							tradeskill = 68;
							break;
						case 0x16: //Pottery 
							tradeskill=69; 

							break;
						case 0x0F: // Baking 
							tradeskill = 60;
							break;
						case 0x09: //Alchemy 
							if ((this->class_==10) & (this->level >=25)) 
								tradeskill=59; 
							else if (this->class_!=10) 
								this->Message(4,"This tradeskill can only be used by the Shaman class.");    
							else if (this->level<25) 
								this->Message(4,"You can use this tradeskill until you reach a level of 25."); 
							break;    	 
						case 0x0A: //Tinkering 
							if (this->race==12) 
								tradeskill = 57; 
							else 
								this->Message(4,"This tradeskill is only available to the Gnomish race."); 
							break; 
						case 0x1A: //Research 
							tradeskill = 58; 
							break; 
						case 0x0C: //Make Poison 
							if (this->class_==9) 
								tradeskill=56; 
							else 
								this->Message(4,"This tradeskill is only available to the Rogue class."); 
							break; 
						case 0x1E: //Quest Containers 
							tradeskill=75;//Making our own type here
							break;
						case 0x0D: //Quest Containers-Most use 1E but item 17111 uses this one, odd
							tradeskill=75;// Making our own type here
							break;
						case 0x2E: //Fishing 
							tradeskill=55; 
							break; 
						default:
							cout << combin->tradeskill; 
							this->Message(4,"This tradeskill has not been implemented yet, if you get this message send a petition and let them know what tradeskill you were trying to use."); 
						}

						if (tradeskill == 0)
							break;

						if (database.GetTradeRecipe(combin,tradeskill,&product,&product2,&failproduct,&skillneeded,&trivial,&productcount))
						{
						//cout <<"Product:"<<product<<" Product2:"<<product2<<" FailProduct:"<<failproduct<<" ProductCount:"<<productcount<<" trivial:"<<trivial<<endl;	
						
							if (productcount<1)
								productcount=1;  //Make sure we return at least one item.

							if (tradeskill==75){ //Our Quest Container tradeskill
								this->Message(4,"You have fashioned the items together to create something new!");
								PutItemInInventory(0,product,productcount);
								for (int k=0; k<10; k++)
								{
									int32 slotid = ((combin->containerslot-22)*10)+k;
									int32 realslotid = ((combin->containerslot+3)*10)+k;
									if (pp.containerinv[slotid] != 0xFFFF)
										DeleteItemInInventory(realslotid, 1, true);
								}
								cout << "Chance: " << chance << endl;
								QueuePacket(app);
								break;
							}
							if ((int)GetSkill(tradeskill) - skillneeded > 0)
							{
								chance = 80+wisebonus-10; // 80% basechance + max 20% stats
								this->Message(4,"This item is trivial to make at your level");
							}
							else
							{

								if (skillneeded - (int)GetSkill(tradeskill) < 20)
									// 40 base chance success + max 40% skill + 20% max stats
									chance = 40+ wisebonus + 40-((skillneeded - (int)GetSkill(tradeskill))*2);
								else
									// 0 base chance success + max 30% skill + 10% max stats
									chance = 0 + (wisebonus/2) + 30 - (((skillneeded - (int)GetSkill(tradeskill)) * (skillneeded - (int)GetSkill(tradeskill)))*0.01875);
								//skillincrease
								if ((55-(GetSkill(tradeskill)*0.236))+intbonus > (float)rand()/RAND_MAX*100)
									this->SetSkill(tradeskill, GetSkill(tradeskill) + 1);
							}
							if (GetGM() || (chance > ((float)rand()/RAND_MAX*100))){
								this->Message(4,"You have fashioned the items together to create something new!");
								PutItemInInventory(0,product,productcount);
								for (int k=0; k<10; k++)
								{
									int32 slotid = ((combin->containerslot-22)*10)+k;
									int32 realslotid = ((combin->containerslot+3)*10)+k;
									if (pp.containerinv[slotid] != 0xFFFF)
										DeleteItemInInventory(realslotid, 1, true);
								}
							}
							else
							{
								this->Message(4,"You lacked the skills to fashion the items together.");

								for (int k=0; k<10; k++)
								{
									int32 slotid = ((combin->containerslot-22)*10)+k;
									int32 realslotid = ((combin->containerslot+3)*10)+k;
									if (pp.containerinv[slotid] != 0xFFFF)
										DeleteItemInInventory(realslotid, 1, true);
								}
								if (failproduct!=0)
									PutItemInInventory(0,failproduct,1);
							
							}


						}
						else
						{
							this->Message(4,"You cannot combine these items in this container type!");
						}
						cout << "Chance: " << chance << endl;
						QueuePacket(app);
						break;
					}
					case OP_ReadBook: {
						BookRequest_Struct* book = (BookRequest_Struct*) app->pBuffer;
						ReadBook(book->txtfile);
						break;
					}
					case OP_Social_Text: {
						cout << "Social Text: " << app->size << endl;		//DumpPacket(app);
						APPLAYER *outapp = new APPLAYER(app->opcode, sizeof(Emote_Text));
						outapp->pBuffer[0] = app->pBuffer[0];
						outapp->pBuffer[1] = app->pBuffer[1];
						uchar *cptr = outapp->pBuffer + 2;
						cptr += sprintf((char *)cptr, "%s", GetName());
						cptr += sprintf((char *)cptr, "%s", app->pBuffer + 2);
						cout << "Check target" << endl;
						if(target != NULL && target->IsClient())
						{
							cout << "Client targeted" << endl;
							entity_list.QueueCloseClients(this, outapp, true, 100, target);
							//cptr += sprintf((char *)cptr, " Special message for you, the target");
							cptr = outapp->pBuffer + 2;

                            // not sure if live does this or not.  thought it was a nice feature, but would take a lot to clean up grammatical and other errors.  Maybe with a regex parser...
							/*
							replacestr((char *)cptr, target->GetName(), "you");
							replacestr((char *)cptr, " he", " you");
							replacestr((char *)cptr, " she", " you");
							replacestr((char *)cptr, " him", " you");
							replacestr((char *)cptr, " her", " you");
							*/
							target->CastToClient()->QueuePacket(outapp);
						}
						else
							entity_list.QueueCloseClients(this, outapp, true, 100);
						delete outapp;
						break;
										 }
					case OP_Social_Action: {
						cout << "Social Action:  " << app->size << endl;
						//DumpPacket(app);
						entity_list.QueueCloseClients(this, app, true);
						break;
					}
					case OP_SetServerFilter: {
					/*	APPLAYER* outapp = new APPLAYER;
					outapp = new APPLAYER;
					outapp->opcode = OP_SetServerFilterAck;
					outapp->size = sizeof(SetServerFilterAck_Struct);
					outapp->pBuffer = new uchar[outapp->size];
					memset(outapp->pBuffer, 0, outapp->size);
					QueuePacket(outapp);
						delete outapp;*/
						break;
					}
						//Broke
					case OP_GMDelCorpse: {
						if(this->Admin() < 80) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/delcorpse");
							break;
						}
						GMDelCorpse_Struct* dc = (GMDelCorpse_Struct *)app->pBuffer;
						Mob* corpse = entity_list.GetMob(dc->corpsename);
						if(corpse==0) {
							break;
						}
						if(corpse->IsCorpse() != true) {
							break;
						}
						corpse->CastToCorpse()->Delete();
						cout << name << " deleted corpse " << dc->corpsename << endl;
						Message(13, "Corpse %s deleted.", dc->corpsename);
						break;
					}
					case OP_GMKick: {
						if(this->Admin() < 100) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/kick");
							break;
						}
						GMKick_Struct* gmk = (GMKick_Struct *)app->pBuffer;
						Client* client = entity_list.GetClientByName(gmk->name);
						if(client==0) {
							if (!worldserver.Connected())
								Message(0, "Error: World server disconnected");

							else {
								ServerPacket* pack = new ServerPacket;

								pack->opcode = ServerOP_KickPlayer;
								pack->size = sizeof(ServerKickPlayer_Struct);
								pack->pBuffer = new uchar[pack->size];
								ServerKickPlayer_Struct* skp = (ServerKickPlayer_Struct*) pack->pBuffer;
								strcpy(skp->adminname, gmk->gmname);
								strcpy(skp->name, gmk->name);
								skp->adminrank = this->Admin();
								worldserver.SendPacket(pack);
								delete pack;
							}
						}
						else {
							entity_list.QueueClients(this,app);
							//client->Kick();
						}

						break;
					}
					case OP_GMServers: {
						if (!worldserver.Connected())

							Message(0, "Error: World server disconnected");
						else {
							ServerPacket* pack = new ServerPacket;
							pack->size = strlen(this->GetName())+2;
							pack->pBuffer = new uchar[pack->size];
							memset(pack->pBuffer, 0, pack->size);
							pack->opcode = ServerOP_ZoneStatus;
							memset(pack->pBuffer, (int8) admin, 1);
							strcpy((char *) &pack->pBuffer[1], this->GetName());
							worldserver.SendPacket(pack);
							delete pack;
						}
						break;
					}
					case OP_Illusion: {
						//DumpPacket(app);
						Illusion_Struct* bnpc = (Illusion_Struct*)app->pBuffer;
						texture=bnpc->texture;
						helmtexture=bnpc->helmtexture;
						luclinface=bnpc->luclinface;
						race=bnpc->race;
						size=0;
						entity_list.QueueClients(this,app);
						break;
					}
					case OP_BecomeNPC: {
						if(this->Admin() < 80) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/becomenpc");
							break;
						}
						entity_list.QueueClients(this, app, false);
						BecomeNPC_Struct* bnpc = (BecomeNPC_Struct*)app->pBuffer;

						Mob* cli = (Mob*) entity_list.GetMob(bnpc->id);
						if(cli==0)
							break;

						cli->SendAppearancePacket(30, 1, true);
						cli->CastToClient()->SetBecomeNPC(true);
						cli->CastToClient()->SetBecomeNPCLevel(bnpc->maxlevel);
						//TODO: Make this toggle a BecomeNPC flag so that it gets updated when people zone in as well; Make combat work with this.
						//DumpPacket(app);
						break;
					}
					case OP_SafeFallSuccess: {
						break;
					}
						// Changes made based on Bobs work on foraging.  Now can set items in the forage database table to 
						// forage for.
					case OP_Forage:	{
						uint32 food_id = ForageItem(pp.current_zone, GetSkill(FORAGE));
						const Item_Struct* food_item = database.GetItem(food_id);

						if (food_item && food_item->name!=0) {
							this->Message(MT_Emote, "You forage a %s",food_item->name);
							this->PutItemInInventory(0,food_item);
						}
						else {
							if (rand()%4==1) {
								this->Message(MT_Emote, "You failed to find anything worthwhile.");
							} 
							else {
								this->Message(MT_Emote, "You fail to find anything to forage.");
							}
						}

						//See if the player increases their skill
						float wisebonus =  (pp.WIS > 200) ? 20 + ((pp.WIS - 200) * 0.05) : pp.WIS * 0.1;
						if ((55-(GetSkill(FORAGE)*0.236))+wisebonus > (float)rand()/RAND_MAX*100)
							this->SetSkill(FORAGE,GetSkill(FORAGE)+1);

						break;
					}
					case OP_Mend:
						{
							int mendhp = GetMaxHP() / 4;
							int noadvance = (rand()%200);
							int currenthp = GetHP();
							if (rand()%100 <= GetSkill(MEND)) {
								SetHP(GetHP() + mendhp);
								SendHPUpdate();
								Message(4, "You mend your wounds and heal some damage");
							}
							else if (noadvance > 175 && currenthp > mendhp) {
								SetHP(GetHP() - mendhp);
								SendHPUpdate();
								Message(4, "You fail to mend your wounds and damage yourself!");
							}
							else if (noadvance > 175 && currenthp <= mendhp) {
								SetHP(1);
								SendHPUpdate();
								Message(4, "You fail to mend your wounds and damage yourself!");
							}
							else	{
								Message(4, "You fail to mend your wounds");
							}

							if ((GetSkill(MEND) < noadvance) && (rand()%100 < 35) && (GetSkill(MEND) < 101))
								this->SetSkill(MEND,GetSkill(MEND)+1);
							break;
						}
					case OP_Drink: {
						cout << "Drinking packet" << endl;
						//						DumpPacket(app);
						break;
								   }
					case OP_ControlBoat: {
						cout << "ControlBoat packet" << endl;
						//						DumpPacket(app);
						break;
										 }
					case OP_SetRunMode: {
						break;
					}
						//This needs to be handled better...
					case OP_EnvDamage: {
						EnvDamage2_Struct* ed = (EnvDamage2_Struct*)app->pBuffer;
						if(admin>=100 && GetGM()) {
							Message(13, "Your GM status protects you from %i points of type %i environmental damage.", ed->damage, ed->dmgtype);
						}
						if(!GetGM()) {
							SetHP(GetHP()-ed->damage);
						}
						SendHPUpdate();
						break;
					}
					case OP_EnvDamage2: {
						//Broadcast to other clients...
						entity_list.QueueClients(this, app, false);
						break;
					}
					case OP_Report: {
						cout << "Report packet" << endl;
						//						DumpPacket(app);
						break;
					}
					case OP_UpdateAA: {
						if(strncmp((char *)app->pBuffer,"on ",3) == 0) {
							// turn on percentage
							pp.perAA = atoi((char *)&app->pBuffer[3]);
							// send an update
							SendAAStats();
							SendAATable();
							//	cout << "UpdateAA packet: ON. Percent: " << (int)pp.perAA << endl;
						} else if(strcmp((char *)app->pBuffer,"off") == 0) {
							// set percentage to 0%
							pp.perAA = 0;
							// send an update
							SendAAStats();
							SendAATable();
							//	cout << "UpdateAA packet: OFF." << endl;
						} else if(strncmp((char *)app->pBuffer,"buy ",4) == 0) {
							char item_name[128];
							int buy_item = atoi((char *)&app->pBuffer[4]);

							int item_cost = 1;
							int max_level = 1;
							int bought = 0;
							if(database.GetAASkillVars(buy_item,item_name,&item_cost,&max_level)) {
								uint8 *aa_item = &(((uint8 *)&aa)[buy_item]);
								int32 cur_level = (*aa_item)+1;
								int32 cost = buy_item <= 17 ? item_cost : item_cost * cur_level;
								//Disabled for now due to problems... - NOT
								if(pp.aapoints >= cost && cur_level <= (int32)max_level) {
									*aa_item = cur_level;
									pp.aapoints -= cost;

									database.SetPlayerAlternateAdv(account_id, pp.name, &aa);
									Message(15,"Skill \'%s\' (%d) purchased for the cost of %d ability point(s)",item_name,cur_level,cost);
									bought = 1;
								}
								SendAATable();
								SendAAStats();
									cout << "UpdateAA packet: BUY. Item: " << buy_item << " Cost: " << item_cost;
									cout << (bought ? " (bought)" : " (not enough points or max level)") << endl;
							}
						} else {
							//cout << "Unknown command in UpdateAA opcode: 0x" << hex << setfill('0') << setw(4) << app->opcode << dec;
							//cout << " size:" << app->size << endl;
							//							DumpPacket(app->pBuffer, app->size);
						}
						break;
									  }
					case OP_GMFind: {
						if (this->Admin() < 80) {
							Message(13, "Your account has been reported for hacking.");
							database.SetHackerFlag(this->account_name, this->name, "/find");
							break;
						}
						//Break down incoming
						GMSummon_Struct* request=(GMSummon_Struct*)app->pBuffer;
						//Create a new outgoing
						APPLAYER *outapp = new APPLAYER(OP_GMFind, sizeof(GMSummon_Struct));
						GMSummon_Struct* foundplayer=(GMSummon_Struct*)outapp->pBuffer;
						//Copy the constants
						strcpy(foundplayer->charname,request->charname);
						strcpy(foundplayer->gmname, request->gmname);



						//Check if the NPC exits intrazone...
						Mob* gt = entity_list.GetMob(request->charname);
						if (gt != 0) {
							foundplayer->success=1;
							foundplayer->x=(sint32)gt->GetX();
							foundplayer->y=(sint32)gt->GetY();
							foundplayer->z=(sint32)gt->GetZ();
							foundplayer->zoneID=zone->GetZoneID();
						}
						//Send the packet...
						FastQueuePacket(&outapp);
						break;
					}
                    case OP_TargetGroupBuff: { 
                        GlobalID_Struct* tgbs = (GlobalID_Struct*)app->pBuffer; 
                        int16 tgbid = tgbs->entity_id ; 
                        if (tgbid == 1) 
                        { 
                            tgb = true; 
                        } 
                        else if (tgbid == 0) 
                        { 
                            tgb = false; 
                        }
                        else if (tgbid == 2) {
                           // Info request
                           Message(0, "Target group buff is :%s", (tgb) ? ("ON"):("OFF"));
                        }
                        break; 
                    } 
					case OP_PickPocket: { // Pickpocket

					     PickPocket_Struct* pick_in = (PickPocket_Struct*) app->pBuffer;
					     LogFile->write(EQEMuLog::Debug,
					               "PickPocket to:%i from:%i myskill:%i type:%i",
                                        pick_in->to, pick_in->from ,pick_in->myskill, pick_in->type);
                                        
					     APPLAYER* outapp = new APPLAYER(OP_PickPocket, sizeof(PickPocket_Struct));
					     PickPocket_Struct* pick_out = (PickPocket_Struct*) outapp->pBuffer;
					     Mob* victim = entity_list.GetMob(pick_in->to);
                         if (!victim)
                             break;
                         if (pick_in->myskill == 0) {
                            LogFile->write(EQEMuLog::Debug,
                                "Client pick pocket response");
                                DumpPacket(app);
                                delete(outapp);
                                break;
                         }
                         uint8 success = 0;
                         if (RandomTimer(1,900) >= (GetSkill(PICK_POCKETS)+GetDEX()))
                             success = RandomTimer(1,5);
                         if ( (victim->IsNPC() && victim->CastToNPC()->IsInteractive()) || (victim->IsClient()) ) {
                            if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "PickPocket picking ipc/client");
                            pick_out->to   = pick_in->from;
                            pick_out->from = pick_in->to;
                            if (   victim->IsClient()
                                && !success
                               ){
                                   victim->CastToClient()->QueuePacket(app);
                            }
                            else if (   victim->IsNPC()
                                && victim->CastToNPC()->IsInteractive()
                                && GetPVP() 
                                && (GetPVP() == victim->CastToClient()->GetPVP())
                                && !success
                               ) {
                                   int16 hate = RandomTimer(1,503);
                                   int16 tmpskill = (GetSkill(PICK_POCKETS)+GetDEX());
                                   if( hate >= tmpskill ){
                                      victim->AddToHateList(this, 1, 0);
                                      entity_list.MessageClose(victim, true, 200, 0, "%s says, your not as quick as you thought you were thief!", victim->GetName());
                                   }
                            }
                            if(!success){
                              // Failed
                               QueuePacket(outapp);
                            }
                            else if (success) {
                               pick_out->to   = pick_in->from;
                               pick_out->from = pick_in->to;
                               pick_out->type = success;
                               pick_out->coin = (RandomTimer(1,RandomTimer(2,10)));
                               switch (success){
                                 case 1:
                                   AddMoneyToPP(0,0,0,pick_out->coin, false);
                                 break;
                                 case 2:
                                   AddMoneyToPP(0,0,pick_out->coin,0, false);
                                 break;
                                 case 3:
                                   AddMoneyToPP(0,pick_out->coin,0,0, false);
                                 break;
                                 case 4:
                                   AddMoneyToPP(pick_out->coin,0,0,0, false);
                                 break;
                                 case 5:
                                 // Item
                                   pick_out->type = 0;
                                  break;
                                 default: LogFile->write(EQEMuLog::Error,"Unknown success in OP_PickPocket %i", __LINE__); break;
                               }
                               QueuePacket(outapp);
                            }
                            else {
                                LogFile->write(EQEMuLog::Error, "Unknown error in OP_PickPocket");
                            }
                         }
                         else {
                         if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "PickPocket picking npc");
                            if (success) {
                               pick_out->to   = pick_in->from;
                               pick_out->from = pick_in->to;
                               pick_out->type = success;
                               pick_out->coin = (RandomTimer(1,RandomTimer(2,10)));
                               switch (success){
                                 case 1:
                                   AddMoneyToPP(0,0,0,pick_out->coin, false);
                                 break;
                                 case 2:
                                   AddMoneyToPP(0,0,pick_out->coin,0, false);
                                 break;
                                 case 3:
                                   AddMoneyToPP(0,pick_out->coin,0,0, false);
                                 break;
                                 case 4:
                                   AddMoneyToPP(pick_out->coin,0,0,0, false);
                                 break;
                                 case 5:
                                 // Item
                                   pick_out->type = 0;
                                  break;
                                 default: LogFile->write(EQEMuLog::Error,"Unknown success in OP_PickPocket %i", __LINE__); break;
                               }
                               QueuePacket(outapp);
                            }
                            else {
                                   int16 hate = RandomTimer(1,503);
                                   int16 tmpskill = (GetSkill(PICK_POCKETS)+GetDEX());
                                   if( hate >= tmpskill ){
                                      victim->AddToHateList(this, 1, 0);
                                      entity_list.MessageClose(victim, true, 200, 0, "%s says, your not as quick as you thought you were thief!", victim->GetName());
                                   }
                             QueuePacket(outapp);
                            }
                         }
					     delete outapp;
					     break;
					}
					case OP_Bind_Wound:{
					     BindWound_Struct* bind_in = (BindWound_Struct*) app->pBuffer;
					     Mob* bindmob = entity_list.GetMob(bind_in->to);
					     if (!bindmob){
                              LogFile->write(EQEMuLog::Error, "Bindwound on non-exsistant mob from %s", this->GetName());
                         }
					     LogFile->write(EQEMuLog::Debug, "BindWound in: to:\'%s\' from=\'%s\'", bindmob->GetName(), GetName());
					     BindWound(bindmob, true);
					     break;
					}
						case 0x4841: { // Invalid item
					     cout<<"invalid item"<<endl;

					     DumpPacket(app);
					     break;
					}
                    case OP_Target: 
                    case OP_Target2: 
                    {
                          GlobalID_Struct* trg = (GlobalID_Struct*)app->pBuffer; 
                          Mob* targ = entity_list.GetMob(trg->entity_id); 
                          if (targ && Dist(targ) <= 100) 
                          {
                              target = targ; 
                              APPLAYER* outapp = app->Copy(); 
                              FastQueuePacket(&outapp); 
                              delete outapp; 
                          } 
                          else 
                              Message(0,"Could not find any being with that name."); 
                          break; 
                    }

					default: {
						cout << "Unknown opcode: 0x" << hex << setfill('0') << setw(4) << app->opcode << dec;
						cout << " size:" << app->size << endl;
						//						DumpPacket(app->pBuffer, app->size);  						break;
					}
				}
				break;
			}
			case CLIENT_KICKED:
			case DISCONNECTED:
				break;
			default:
				{
					cerr << "Unknown client_state:" << (int16) client_state << endl;
					break;
				}
		}

	return ret;
}

void Client::DBAWComplete(int8 workpt_b1, DBAsyncWork* dbaw) {
	Entity::DBAWComplete(workpt_b1, dbaw);
	switch (workpt_b1) {

		case DBA_b1_Entity_Client_InfoForLogin: {
			if (!FinishConnState2(dbaw))
				client_state = CLIENT_ERROR;
			break;
		}
		case DBA_b1_Entity_Client_Save: {
			char errbuf[MYSQL_ERRMSG_SIZE];
			int32 affected_rows = 0;
			DBAsyncQuery* dbaq = dbaw->PopAnswer();
			if (dbaq->GetAnswer(errbuf, 0, &affected_rows) && affected_rows == 1) {
				if (dbaq->QPT())
					SaveBackup();
			}
			else {
				cout << "Async client save failed. '" << errbuf << "'" << endl;

				Message(13, "Error: Asyncronous save of your character failed.");
				if (Admin() >= 200)
					Message(13, "errbuf: %s", errbuf);
			}
			pQueuedSaveWorkID = 0;
			break;
		}
		default: {
			cout << "Error: Client::DBAWComplete(): Unknown workpt_b1" << endl;
			break;
		}
	}
}


bool Client::FinishConnState2(DBAsyncWork* dbaw) {
	if (client_state != CLIENT_CONNECTING2_5) {
		cout << "Error in FinishConnState2(): client_state != CLIENT_CONNECTING2_5" << endl;
		return false;
	}
	int8 gmspeed = 0;
	int32 pplen = 0, aalen = 0;
	memset(&pp, 0, sizeof(PlayerProfile_Struct));
	DBAsyncQuery* dbaq = 0;

	MYSQL_RES* result = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	for (int32 i=1; i<=3; i++) {
		dbaq = dbaw->PopAnswer();
		if (!dbaq) {
			cout << "Error in FinishConnState2(): dbaq==0" << endl;
			return false;
		}
		if (!dbaq->GetAnswer(errbuf, &result)) {
			cout << "Error in FinishConnState2(): !dbaq[" << dbaq->QPT() << "]->GetAnswer(): " << errbuf << endl;
			return false;
		}
		if (dbaq->QPT() == 1) {
			database.GetAccountInfoForLogin_result(result, 0, this->account_name, &this->lsaccountid, &gmspeed);



		}
		else if (dbaq->QPT() == 2) {
			database.GetCharacterInfoForLogin_result(result, 0, 0, &this->pp, &pplen, &this->aa, &aalen, &this->guilddbid, &this->guildrank);
		}
		else if (dbaq->QPT() == 3) {
			database.LoadFactionValues_result(result, &this->factionvalue_list);
		}
		else {
			cout << "Error in FinishConnState2(): dbaq->PQT() unknown" << endl;
			return false;
		}
	}

	cout << "Loaded playerprofile for " << name << " - size: " << pplen << "/" << sizeof(PlayerProfile_Struct) << endl;


	char temp1[64];
	if (database.GetVariable("Max_AAXP", temp1, sizeof(temp1)-1)) {
		max_AAXP = atoi(temp1);
	}
//				int32 aalen = database.GetPlayerAlternateAdv(account_id, name, &aa);
	if(aalen == 0) {
		cout << "Client dropped: !GetPlayerAlternateAdv, name=" << name << endl;
		return false;
	}

	cout << "Loaded alt_adv_table for " << name << " - size: " << aalen << "/" << sizeof(PlayerAA_Struct) << endl;

	pp.gm = 1; //trust me :P
	// Try to find the EQ ID for the guild, if doesnt exist, guild has been deleted.
	// Clear memory, but leave it in the DB (no reason not to, guild might be restored?)
	guildeqid = database.GetGuildEQID(guilddbid);
	if (guildeqid == 0xFFFFFFFF) {
		guilddbid = 0;
		guildrank = GUILD_MAX_RANK;
		pp.guildid = 0xFFFF;
	}
	else {
		pp.guildid = guildeqid;
	}
	strcpy(pp.name, name);
	strcpy(lastname, pp.last_name);
	if((pp.x == -1 && pp.y == -1 && pp.z == -1)||(pp.x == -2 && pp.y == -2 && pp.z == -2)) {
		//Fixed XYZ
		pp.x = zone->safe_x();
		pp.y = zone->safe_y();
		pp.z = zone->safe_z();
	}
	x_pos = pp.x;
	y_pos = pp.y;
	z_pos = pp.z/10;
	
	heading = pp.heading;
	race = pp.race;
	base_race = pp.race;
	class_ = pp.class_;
	gender = pp.gender;
	base_gender = pp.gender;
	level = pp.level;
	deity = pp.deity;//DEITY_AGNOSTIC;
	haircolor = pp.haircolor;
	beardcolor = pp.beardcolor;
	eyecolor1 = pp.eyecolor1;
	eyecolor2 = pp.eyecolor2;
	hairstyle = pp.hairstyle;
	title = pp.title;
	luclinface = pp.luclinface;
	switch (race)
	{

	case OGRE:
		size = 9;break;
	case TROLL:
		size = 8;break;
	case VAHSHIR:
	case BARBARIAN:
		size = 7;break;
	case HUMAN:
	case HIGH_ELF:
	case ERUDITE:
	case IKSAR:
		size = 6;break;
    case HALF_ELF:
        size = 5.5;break;
	case WOOD_ELF:
	case DARK_ELF:
		size = 5;break;
	case DWARF:
		size = 4;break;
    case HALFLING:
        size = 3.5;break;
	case GNOME:
		size = 3;break;
	default:
		size = 0;break;
	}
	if (spells_loaded) {
		for (int i=0; i<15; i++) {
			for(int z=0;z<15;z++) {
                // check for duplicates
				if(buffs[z].spellid == pp.buffs[i].spellid) {
					buffs[z].spellid = 0xFFFF;
				}
			}

			if (pp.buffs[i].spellid <= SPDAT_RECORDS && pp.buffs[i].spellid != 0 && pp.buffs[i].duration > 0) {
				buffs[i].spellid = pp.buffs[i].spellid;
				buffs[i].ticsremaining = pp.buffs[i].duration;
				buffs[i].casterlevel = pp.buffs[i].level;
				buffs[i].casterid = 0;
				buffs[i].durationformula = spells[buffs[i].spellid].buffdurationformula;
    			pp.buffs[i].b_unknown1 = 1;
			}
            else {
				buffs[i].spellid = 0xFFFF;
    			pp.buffs[i].b_unknown1 = 0;
	    		pp.buffs[i].level = 0;

		    	pp.buffs[i].b_unknown2 = 0;
			    pp.buffs[i].b_unknown3 = 0;
				pp.buffs[i].spellid = 0;
				pp.buffs[i].duration = 0;
            }
		}

        for (int j1=0; j1<15; j1++) {
		    if (buffs[j1].spellid <= SPDAT_RECORDS) {
			    for (int x1=0; x1 < 12; x1++) {
					switch (spells[buffs[j1].spellid].effectid[x1]) {
						case SE_SummonHorse: {
							hasmount = false;
							break;
						}
						case SE_Illusion: {
							buffs[j1].spellid = 0xFFFF;
							x1 = 12;
							break;
						}
						case SE_Charm: {
							buffs[j1].spellid = 0xFFFF;
							x1 = 12;
							break;
						}
					}
                }
            }
        }
	}
	if(this->isgrouped)
	{
		Group* group;
		group = entity_list.GetGroupByClient(this);
		for(int z=0; z<5; z++)
		{
			memset(pp.GMembers[z],0,sizeof(group->members[z]->GetName()));
		}

	}
	else
	{
		for(int z=0; z<5; z++)
		{
			memset(pp.GMembers[z],0,sizeof(pp.GMembers[0]));
		}
	}
	CalcBonuses();
	CalcMaxHP();
	CalcMaxMana();
	if (pp.cur_hp <= 0)
		pp.cur_hp = GetMaxHP();
	SetHP(pp.cur_hp);
	Mob::SetMana(pp.mana);

	pp.current_zone = database.GetZoneID(zone->GetFileName());
	APPLAYER* outapp = new APPLAYER(OP_PlayerProfile);
	outapp->pBuffer = new uchar[10000];
	int32 realxp = pp.exp;
	double tmpxp = (double) ( (double) pp.exp - GetEXPForLevel( GetLevel() )) / ( (double) GetEXPForLevel(GetLevel()+1) - GetEXPForLevel(GetLevel()));
	pp.exp = (int32)(330.0f * tmpxp);

	SetEQChecksum((unsigned char*)&pp, sizeof(PlayerProfile_Struct)-4);
	outapp->size = DeflatePacket((unsigned char*)&pp, sizeof(PlayerProfile_Struct), outapp->pBuffer, 10000);
	pp.exp = realxp; // but keep serverexp correct
	EncryptProfilePacket(outapp);
	QueuePacket(outapp);
	delete outapp;
	pp.current_zone = zone->GetZoneID();


	outapp = new APPLAYER(OP_ZoneEntry, sizeof(ServerZoneEntry_Struct));
	ServerZoneEntry_Struct *sze = (ServerZoneEntry_Struct*)outapp->pBuffer;
	memset(sze, 0, sizeof(ServerZoneEntry_Struct));
	strcpy(sze->name, name);
	strcpy(sze->lastname, lastname);

	sze->zone = zone->GetZoneID();
	sze->class_ = pp.class_;
	sze->race = pp.race;
	sze->gender = pp.gender;
	sze->level = pp.level;


	sze->x = pp.x;
	sze->y = pp.y;
	sze->z = pp.z/10;
	sze->heading = pp.heading;
	if(GetPVP()) {
		sze->pvp=1;

	}

	//GM Speed Hack
	if (GetGM() && admin>=100) {
		if (gmspeed == 0) {

			sze->walkspeed = walkspeed;
			sze->runspeed = runspeed;
		}
		else {
			sze->walkspeed = 1.0;
			sze->runspeed = 3.0;
		}
	}
	else {
		sze->walkspeed = walkspeed;
		sze->runspeed = runspeed;
	}
	sze->anon = pp.anon;

	// Disgrace: proper face selection
	sze->face = GetFace();

	// Quagmire - added coder?'s code from messageboard
	const Item_Struct* item = database.GetItem(pp.inventory[2]);
	if (item) {
		sze->helmet = item->common.material;
		// FIXME!						sze->helmcolor = item->common.color;
	}
	sze->npc_armor_graphic = 0xFF;  // tell client to display PC's gear
	// Quagmire - Found guild id =)
	// havent found the rank yet tho
	sze->guildeqid = (int16) guildeqid;
	
	//Grass clipping radius.
	sze->skyradius=244;
	//*shrug*

//						sze->unknown304[0]=255;
	sze->haircolor = haircolor;
	sze->beardcolor = beardcolor;
	sze->eyecolor1 = eyecolor1;
	sze->eyecolor2 = eyecolor2;
	sze->hairstyle = hairstyle;
	sze->title = title;
	sze->luclinface = luclinface;
//	sze->sze_unknown5_2_2[5] = 0xA0;

	SetEQChecksum((unsigned char*)sze, sizeof(ServerZoneEntry_Struct));
	this->QueuePacket(outapp,true);
	delete outapp;
	entity_list.SendZoneSpawnsBulk(this);

	outapp = new APPLAYER(OP_TimeOfDay, sizeof(TimeOfDay_Struct));
	TimeOfDay_Struct* tod = (TimeOfDay_Struct*)outapp->pBuffer;
	zone->zone_time.getEQTimeOfDay(time(0), tod);
	this->QueuePacket(outapp);
	delete outapp;
	this->BulkSendInventoryItems(); // don't move this


	outapp = new APPLAYER(OP_Weather, 8);
	if (zone->zone_weather == 1 || zone->zone_weather == 2)
		outapp->pBuffer[4] = zone->zone_weather;
	if (zone->zone_weather == 2)
	{
		outapp->pBuffer[0] = 0x01;
		outapp->pBuffer[4] = 0x02;
	}

	this->QueuePacket(outapp);
	delete outapp;

	//cout << "Sent addplayer... (EQ disconnects here on error)" << endl;

	//cout << "Sent timeofday... Next stop: Login 3." << endl;

	this->SetAttackTimer();
	client_state = CLIENT_CONNECTING3;
	return true;
}

bool Client::Process() {
	adverrorinfo = 1;
	bool ret = true;
	if(!process_timer->Check())
		return true;
	//bool throughpacket = true;
	if (Connected())
	{

        // try to send all packets that weren't send before
        SendAllPackets();

		if(dead)
			SetHP(-100);

		if (camp_timer && camp_timer->Check()) {
		Save();
		}

		if (IsStunned() && stunned_timer->Check()) {
			this->stunned = false;
			this->stunned_timer->Disable();
		}

		if (bardsong_timer->Check() && bardsong != 0)
		{
			if (target == 0)
				SpellFinished(bardsong, this->GetID());


			else
				SpellFinished(bardsong, target->GetID());
		}
		if (bindwound_timer->Check() && bindwound_target != 0)
		{
		    BindWound(bindwound_target, false);
		}
		if (auto_attack && !IsAIControlled() && !(spellend_timer->Enabled() && (spells[casting_spell_id].classes[7] < 1 && spells[casting_spell_id].classes[7] > 65)) && target != 0 && attack_timer->Check() && !IsStunned() && !IsMezzed() && dead == 0)
		{
			if (!CombatRange(target))
			{
				Message(13,"Your target is too far away, get closer!");
			}
			else if (target == this)
			{
				Message(13,"Try attacking someone else then yourself!");

			}
			/*			else if (CantSee(target))
			{
			Message(13,"You can't see your target from here.");
		}*/
			else if (!IsNPC() && appearance == 3)
			{
			}
			else if (target->GetHP() > 0)
			{
				Attack(target, 13); 	// Kaiyodo - added attacking hand to arguments
				// Kaiyodo - support for double attack. Chance based on formula from Monkly business
				if(CanThisClassDoubleAttack())
				{
					float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max
					// Check for double attack with main hand assuming maxed DA Skill (MS)
					float random = (float)rand()/RAND_MAX;
					if (random > 0.9)
						CheckIncreaseSkill(DOUBLE_ATTACK);
					if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
						if(target && target->GetHP() > 0){
							Attack(target, 13);
							CheckIncreaseSkill(DOUBLE_ATTACK);
						}
				}

			}
		}
		if (GetClass() == WARRIOR && dead == 0 && !this->berserk && this->GetHPRatio() < 30) {
			char temp[100];
			snprintf(temp, 100, "%s goes into a berserker frenzy!", this->GetName());
			this->berserk = true;
			entity_list.MessageClose(this, 0, 200, 10, temp);
		}

		if (GetClass() == WARRIOR && this->berserk && this->GetHPRatio() > 30) {
			char temp[100];
			snprintf(temp, 100, "%s is no longer berserk.", this->GetName());
			this->berserk = false;
			entity_list.MessageClose(this, 0, 200, 10, temp);
		}
		// Kaiyodo - Check offhand attack timer
		if(auto_attack && !IsAIControlled() && CanThisClassDuelWield() && target != 0 && attack_timer_dw->Check()&& !IsStunned() && !IsMezzed() && dead == 0)
		{
			if(!CombatRange(target))
			{
				Message(13,"Your target is too far away, get closer! (dual)");
			}		// Range check
			else if(target == this)
        		{
				Message(13,"Try attacking someone else then yourself! (dual)");
			}						// Don't attack yourself
			else if(!IsNPC() && appearance == 3)// Mezzed? Stunned?
			{
        		}
			else if(target->GetHP() > 0)
			{
				float DualWieldProbability = (GetSkill(DUEL_WIELD) + GetLevel()) / 400.0f; // 79.25 max at level 65
				float random = (float)rand()/RAND_MAX;
				if (random > 0.9)
					CheckIncreaseSkill(DUEL_WIELD);
				if(random < DualWieldProbability)
				{
					Attack(target, 14);	// Single attack with offhand
					CheckIncreaseSkill(DUEL_WIELD);
					float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 63.4 max at level 65
					random = (float)rand()/RAND_MAX;
					if(random <= DoubleAttackProbability)
						if(target && target->GetHP() > 0)
							Attack(target, 14);
				}
			}
		}
		adverrorinfo = 2;
		if (position_timer->Check()) {
			if (IsAIControlled())
				SendPosUpdate(2);
			//			SendPosUpdate();
			// Send position updates, whole zone every 5 seconds, close mobs every 250ms
			if (position_timer_counter >= 20) {
				entity_list.SendPositionUpdates(this, pLastUpdateWZ);
				pLastUpdate = Timer::GetCurrentTime();
				pLastUpdateWZ = pLastUpdate;
				position_timer_counter = 0;
			}
			else {
				// send an update every 2.5 seconds that disreguards the LastUpdate counter, in case an update was dropped by network layer
				entity_list.SendPositionUpdates(this, pLastUpdate, 400, target, (position_timer_counter == 10));
				pLastUpdate = Timer::GetCurrentTime();
				position_timer_counter++;
			}
		}
		adverrorinfo = 3;
		SpellProcess();
		adverrorinfo = 4;
		if (tic_timer->Check() && dead == 0)
		{
			CalcMaxHP();
			CalcMaxMana();
			TicProcess();
            // todo: send stamina_packet somewhere and do the calculations
			staminacount++;
            APPLAYER *outapp;
            outapp = new APPLAYER(OP_Stamina, sizeof(Stamina_Struct));
			Stamina_Struct* sta = (Stamina_Struct*)outapp->pBuffer;

            pp.hungerlevel--;
            pp.thirstlevel--;


            // add the same fatigue per tick as subtracted per jump (like 800?)
			if(pp.Fatigue <= 9)
				pp.Fatigue = 0;
			else
				pp.Fatigue -= 10;

            sta->food = pp.hungerlevel;
			sta->water = pp.thirstlevel;
			sta->fatigue = pp.Fatigue;
			if(staminacount >= 2)
			{
			    staminacount = 0;
			    QueuePacket(outapp);
			}
			delete outapp;

			if (GetMana() < max_mana)
			{
				// TODOmake manaregen 100% snyc with client

				if (IsSitting())

                {
					if (GetLevel() > 0)
						medding = true;
					if (medding)
                    {
							if ((GetMana()+(GetSkill(MEDITATE)/10)+(GetLevel()-(GetLevel()/4))+4) < (CalcMaxMana()/2))
								Mob::SetMana(GetMana()+(GetSkill(MEDITATE)/10)+(GetLevel()-(GetLevel()/4))+4);
							else
								Mob::SetMana(GetMana()+GetLevel()+6);
						if (GetSkill(MEDITATE) >0)
							CheckIncreaseSkill(MEDITATE);
					}
					else{
						Mob::SetMana(GetMana()+4);
					}
				}
				else{
					medding = false;

					Mob::SetMana(GetMana()+2);
				}
			}
			//CalcMaxMana();
			/*

			if (GetHP() < max_hp) // TODO: Get regen/tick confirmed
			{
			uint8 *aa_item = &(((uint8 *)&aa)[14]);
			int amount = *aa_item;
			amount=amount*2;
			aa_item = &(((uint8 *)&aa)[29]);
			int amount2 = *aa_item;
			amount=amount+(amount2*2);
			if (IsSitting()) // If we are sitting
			{
			if (GetRace() == TROLL || GetRace() == IKSAR)
			{
			if (GetLevel() < 20)
			SetHP(GetHP()+4+amount);
			else if (GetLevel() < 50)
			SetHP(GetHP()+6+amount);
			else
			SetHP(GetHP()+12+amount);
			}
			else
			{
			if (GetLevel() < 20)
			SetHP(GetHP()+2+amount);
			else if (GetLevel() < 50)
			SetHP(GetHP()+3+amount);
			else if (GetLevel() < 51)
			SetHP(GetHP()+4+amount);

			else
			SetHP(GetHP()+5+amount);
			}
			}
			else
			{
			if (GetRace() == TROLL || GetRace() == IKSAR)
			{
			if (GetLevel() < 20)
			SetHP(GetHP()+1+amount);
			else if (GetLevel() < 50)
			SetHP(GetHP()+2+amount);
			else

			SetHP(GetHP()+4+amount);
			}
			else
			{
			if (GetLevel() < 51)
			SetHP(GetHP()+1+amount);
			else
			SetHP(GetHP()+2+amount);
			}
			}
			if (GetHP() > max_hp)
			SetHP(max_hp);
			SendHPUpdate();
		}*/
		}
	}
else
    adverrorinfo = 5;
	/************ Get all packets from packet manager out queue and process them ************/
	APPLAYER *app = 0;
	while(ret && (app = eqnc->PopPacket())) {
		// Is it compressed?
		if ((app->opcode & 0x0080) && app->Inflate()) {
			app->opcode = htons(app->opcode & ~0x0080);
		}
		if(app!=0){
		ret = HandlePacket(app);
		safe_delete(app);
		}
		else
			printf("ret==0\n");
	}

	AI_Process();

	if (client_state == CLIENT_KICKED) {
		eqnc->Close();
		cout << "Client disconnected (cs=k): " << GetName() << endl;
		return false;
	}

	if (client_state == DISCONNECTED) {
		eqnc->Close();
		cout << "Client disconnected (cs=d): " << GetName() << endl;
		return false;
	}

	if (client_state == CLIENT_ERROR) {
		eqnc->Close();
		cout << "Client disconnected (cs=e): " << GetName() << endl;
		return false;
	}

	if (client_state != CLIENT_LINKDEAD && !eqnc->CheckActive()) {
		cout << "Client linkdead: " << name << endl;
		eqnc->Close();
		if (GetGM()) {
			return false;
		}
		else {
			client_state = CLIENT_LINKDEAD;

			AI_Start(CLIENT_LD_TIMEOUT);
			SendAppearancePacket(AT_LD, 1);
		}
	}

	return ret;
}

void Client::SendInventoryItems() {
	this->RepairInventory();
	int i;

	const Item_Struct* item = 0;
	Item_Struct* outitem = 0;
	for (i=0;i<pp_inventory_size; i++) {
		item = database.GetItem(pp.inventory[i]);
		if (item) {
			//							cout << "Sending inventory slot:" << i << endl;
			APPLAYER* app = new APPLAYER(0x3120, sizeof(Item_Struct));
			memcpy(app->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app->pBuffer;
			outitem->equipSlot = i;
			outitem->common.charges = pp.invitemproperties[i].charges;
			QueuePacket(app);
			delete app;
		}
	}

	// Coder_01's container code
	for (i=0; i < pp_containerinv_size; i++) {
		item = database.GetItem(pp.containerinv[i]);
		if (item) {
			//							cout << "Sending inventory slot:" << 250+j << endl;
			APPLAYER* app = new APPLAYER(0x3120, sizeof(Item_Struct));
			memcpy(app->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app->pBuffer;
			outitem->equipSlot = 250 + i;
			outitem->common.charges = pp.bagitemproperties[i].charges;
			QueuePacket(app);
			delete app;
		}
	}


	// Quagmire - Bank code, these should be the proper pp locs too
	for (i=0; i < pp_bank_inv_size; i++) {
		item = database.GetItem(pp.bank_inv[i]);
		if (item) {
			APPLAYER* app = new APPLAYER(0x3120, sizeof(Item_Struct));
			memcpy(app->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app->pBuffer;

			outitem->equipSlot = 2000 + i;
			outitem->common.charges = pp.bankinvitemproperties[i].charges;
			QueuePacket(app);
			delete app;

		}
	}
	for (i=0; i < pp_bank_cont_inv_size; i++) {
		item = database.GetItem(pp.bank_cont_inv[i]);
		if (item) {
			APPLAYER* app = new APPLAYER(0x3120, sizeof(Item_Struct));
			memcpy(app->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app->pBuffer;
			outitem->equipSlot = 2030 + i;
			outitem->common.charges = pp.bankbagitemproperties[i].charges;
			QueuePacket(app);
			delete app;
		}
	}

}

void Client::BulkSendInventoryItems() {
	this->RepairInventory();
	int i;
	const Item_Struct* item = 0;
	int cpisize = sizeof(CPlayerItems_Struct) + (250 * sizeof(CPlayerItems_packet_Struct));
	CPlayerItems_Struct* cpi = (CPlayerItems_Struct*) new uchar[cpisize];
	memset(cpi, 0, cpisize);
	for (i=0;i<pp_inventory_size; i++) {
		item = database.GetItem(pp.inventory[i]);
		if (item) {
			cpi->packets[cpi->count].opcode = ntohs(OP_CPlayerItem);
			memcpy(&cpi->packets[cpi->count].item, item, sizeof(Item_Struct));
			cpi->packets[cpi->count].item.equipSlot = i;
			cpi->packets[cpi->count].item.common.charges = pp.invitemproperties[i].charges;
			cpi->count++;
			if (cpi->count >= 250) {
				cout << "ERROR: cpi->count>=250 in BulkSendInventoryItems()" << endl;
				return;
			}
		}
	}

	// Coder_01's container code
	for (i=0; i < pp_containerinv_size; i++) {
		item = database.GetItem(pp.containerinv[i]);
		if (item) {
			cpi->packets[cpi->count].opcode = ntohs(OP_CPlayerItem);
			memcpy(&cpi->packets[cpi->count].item, item, sizeof(Item_Struct));
			cpi->packets[cpi->count].item.equipSlot = 250+i;
			cpi->packets[cpi->count].item.common.charges = pp.bagitemproperties[i].charges;
			cpi->count++;
			if (cpi->count >= 250) {
				cout << "ERROR: cpi->count>=250 in BulkSendInventoryItems()" << endl;
				return;
			}
		}
	}

	// Quagmire - Bank code, these should be the proper pp locs too
	for (i=0; i < pp_bank_inv_size; i++) {
		item = database.GetItem(pp.bank_inv[i]);
		if (item) {
			cpi->packets[cpi->count].opcode = ntohs(OP_CPlayerItem);
			memcpy(&cpi->packets[cpi->count].item, item, sizeof(Item_Struct));
			cpi->packets[cpi->count].item.equipSlot = 2000+i;
			cpi->packets[cpi->count].item.common.charges = pp.bankinvitemproperties[i].charges;
			cpi->count++;
			if (cpi->count >= 250) {
				cout << "ERROR: cpi->count>=250 in BulkSendInventoryItems()" << endl;

				return;
			}
		}
	}
	for (i=0; i < pp_bank_cont_inv_size; i++) {

		item = database.GetItem(pp.bank_cont_inv[i]);
		if (item) {
			cpi->packets[cpi->count].opcode = ntohs(OP_CPlayerItem);
			memcpy(&cpi->packets[cpi->count].item, item, sizeof(Item_Struct));
			cpi->packets[cpi->count].item.equipSlot = 2030+i;
			cpi->packets[cpi->count].item.common.charges = pp.bankbagitemproperties[i].charges;
			cpi->count++;
			if (cpi->count >= 250) {
				cout << "ERROR: cpi->count>=250 in BulkSendInventoryItems()" << endl;
				return;
			}
		}
	}
	APPLAYER* outapp = new APPLAYER(OP_CPlayerItems, 5000);
	outapp->size = 2 + DeflatePacket((uchar*) cpi->packets, cpi->count * sizeof(CPlayerItems_packet_Struct), &outapp->pBuffer[2], 5000-2);
	CPlayerItems_Struct* cpi2 = (CPlayerItems_Struct*) outapp->pBuffer;
	cpi2->count = cpi->count;
	QueuePacket(outapp);
	//FileDumpPacket("BulkItem.txt", (uchar*) cpi, sizeof(CPlayerItems_Struct) + (cpi->count * sizeof(CPlayerItems_packet_Struct)));
	//FileDumpPacket("BulkItem.txt", outapp);
	delete outapp;
	delete cpi;
}


void Client::RemoveData() {
	eqnc->RemoveData();
}

void Client::BulkSendMerchantInventory(int merchant_id, int16 npcid) {
  const Item_Struct* handyitem = NULL;
  int32 numItemSlots=80;  //The max number of items passed in the transaction.   // We don't have 81 slots we have 80 it's misleading (BigPull)
  int32 cpisize = sizeof(MerchantItem_Struct) + (numItemSlots * sizeof(MerchantItemD_Struct));
  MerchantItem_Struct* cpi = (MerchantItem_Struct*) new uchar[cpisize];
  memset(cpi, 0, cpisize);
  const Item_Struct *item;
  for ( int32 i=0;i<database.GetMerchantListNumb(merchant_id) && i < numItemSlots; i++) {
    if (cpi->count >= numItemSlots + 1) { // Check if we're over the limit of items to send before we copy any data
      cout << "ERROR IN DATABASE: merchant("<<merchant_id<<") has more than 80 items players list will be truncated" << endl;
      cpi->count = 80;
      break; // Exit the loop and send what we have
    }
    item=database.GetItem(database.GetMerchantData(merchant_id,i+1));
    if (item) {
      if(!cpi->count)
        handyitem=item;
      memcpy(&cpi->packets[cpi->count].item, item, sizeof(Item_Struct));
      cpi->packets[cpi->count].item.equipSlot = i;
      cpi->count++;
    }
  }
  // And now proccess the merchants loot table as this is where player sold items end up
  char mki[3] = "";
  if (database.GetVariable("MerchantsKeepItems", mki, 2)) {
    if ( mki[0] == '1' ) {
      Entity* vendor = entity_list.GetMob(npcid);
      if (cpi->count < 80 && vendor->CastToNPC()->CountLoot() > 0) {
        int db_count = cpi->count;
        for ( int i = 0; i < 80 - db_count && i < vendor->CastToNPC()->CountLoot(); i++ ) {
          ServerLootItem_Struct* t_item = vendor->CastToNPC()->GetItem(i);
          if (! t_item ) {
            // Empty inventory slot
            continue;
          } else {
            item = database.GetItem(t_item->item_nr);
            if (item) {
              memcpy(&cpi->packets[cpi->count].item, item, sizeof(Item_Struct));
              cpi->packets[cpi->count].item.equipSlot = db_count;
              cpi->count++; db_count++;
            } else {

              // Shouldn't happen but if it does we'll fail nice and silent like
              continue;
            }
          }
        }
      }
    }// MerchantsKeepItems off
  } // MerchantsKeepItems configured

	APPLAYER* outapp = new APPLAYER(OP_ShopItem, 5000);
	outapp->size = 2 + DeflatePacket((uchar*) cpi->packets, cpi->count * sizeof(MerchantItemD_Struct), &outapp->pBuffer[2], 5000-2);
	QueuePacket(outapp);
    delete outapp;

   	if(cpi->count > 1)
	{
		int randitem=rand()%cpi->count;
		if(randitem)	/* not sure why, but using item 0 causes the name to be corrupt? */
			handyitem=&cpi->packets[randitem].item;
	}

	//DumpPacket(outapp);
	//printf("Merchant has %i items available.\n",cpi->count);

	Mob* merch = entity_list.GetMob(npcid);
	if(merch != NULL && handyitem)
	{
		char unknown[6]={0x00,0x00,0x2a,0x02,0x0a,0x00};
		char number[5]="1148";
        int packet_len;
        int pos;

        if (handyitem->name != NULL)
        {
		    printf("handyitem is %s\n", handyitem->name);
        }

        packet_len = (sizeof(unknown)) + \
                     (strlen(number) + 1);

        if (handyitem->name != NULL)
        {
            packet_len += (strlen(handyitem->name) + 1);
        }
        else
        {
            packet_len += 1;
        }

        if (this->GetName() != NULL)
        {
            packet_len += (strlen(this->GetName()) + 1);
        }
        else
        {
            packet_len += 1;
        }

        if (merch->GetName() != NULL)
        {
            packet_len += (strlen(merch->GetName()) + 1);
        }
        else
        {

            packet_len += 1;
        }

        outapp = new APPLAYER(0x3642, packet_len);

		memset(outapp->pBuffer, 0, packet_len);
        pos=0;

        memcpy(outapp->pBuffer + pos, unknown, 6);
		pos+=(sizeof(unknown));

        if (merch->GetName() != NULL)
        {
            memcpy(outapp->pBuffer + pos, merch->GetName(), strlen(merch->GetName()));
		    pos+=strlen(merch->GetName())+1;
        }
        else
        {
            pos+=1; //leave string as null
        }

		memcpy(outapp->pBuffer + pos, number, 4);
		pos+=5;

        if (this->GetName() != NULL)
        {
		    memcpy(outapp->pBuffer + pos, this->GetName(), strlen(this->GetName()));
		    pos+=strlen(this->GetName())+1;
        }
        else
        {
            pos+=1; //leave string as null
        }

        if (handyitem->name != NULL)
        {
		    memcpy(outapp->pBuffer + pos, handyitem->name, strlen(handyitem->name));
		    pos+=strlen(handyitem->name)+1;
        }
        else
        {
            pos+=1; //leave string as null
        }


        QueuePacket(outapp);
        delete cpi;
		delete outapp;
	}
}

void Client::OPRezzAnswer(const APPLAYER* app) {
	if (!pendingrezzexp)
		return;
	const Resurrect_Struct* ra = (const Resurrect_Struct*) app->pBuffer;
	if (ra->action == 1){
		cout << "Player " << this->name << " got a " << (int16)spells[ra->spellid].base[0] << "% Rezz" << endl;
		this->BuffFade(0xFFFe);
		SetMana(0);
		SetHP(GetMaxHP()/5);
		APPLAYER* outapp = app->Copy();
		outapp->opcode = OP_RezzComplete;
		worldserver.RezzPlayer(outapp,0,OP_RezzComplete);
		cout << "pe: " << pendingrezzexp << endl;
		SetEXP(((int)(GetEXP()+((float)((pendingrezzexp/100)*spells[ra->spellid].base[0])))),GetAAXP(),true);
		pendingrezzexp = 0;
		if (strcmp(ra->zone,zone->GetShortName()) != 0){
			SetZoneSummonCoords(ra->x,ra->y,ra->z);
		}
		this->FastQueuePacket(&outapp);
	}
}

void Client::OPMemorizeSpell(const APPLAYER* app) {
	if(app->size != sizeof(MemorizeSpell_Struct))
		return;

	const MemorizeSpell_Struct* memspell = (const MemorizeSpell_Struct*) app->pBuffer;

	//							DumpPacketHex(app);
	//printf("Spell Scribe/Mem: %i(spellID) %i(slot) %i(scribing) %i(levelreq) %i(curlevel)\n",memspell.spell_id,memspell.slot,memspell.scribing,spells[memspell.spell_id].classes[GetClass()],GetLevel());
	//printf("Class: %i ClassReqAbove: %i ClassReqBelow: %i\n",GetClass(),spells[memspell.spell_id].classes[GetClass()+1],spells[memspell.spell_id].classes[GetClass()-1]);
	if (memspell->spell_id > SPDAT_RECORDS) {
		Message(13, "Unexpected error: spell id out of range");
		return;
	}

	if(spells[memspell->spell_id].classes[GetClass()-1] > GetLevel()) {
		APPLAYER* outapp = app->Copy();
		MemorizeSpell_Struct* mss = (MemorizeSpell_Struct*) outapp->pBuffer;
		mss->slot = 0;
		mss->spell_id = 0;
		mss->scribing = 2;
		FastQueuePacket(&outapp);
		return;
	}
	else
		QueuePacket(app);

	if (memspell->scribing == 0) {
		pp.spell_book[memspell->slot] = memspell->spell_id;
		APPLAYER* outapp = new APPLAYER(OP_MoveItem, sizeof(MoveItem_Struct));
		MoveItem_Struct* spellmoveitem = (MoveItem_Struct*) outapp->pBuffer;
		spellmoveitem->from_slot = 0;
		spellmoveitem->to_slot = 0xffffffff;

		spellmoveitem->number_in_stack = 0;

		QueuePacket(outapp);
		delete outapp;
		pp.inventory[0] = 0xFFFF;
	}
	else if (memspell->scribing == 1) {
		APPLAYER* outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
		MemorizeSpell_Struct* mem = (MemorizeSpell_Struct*)outapp->pBuffer;
		mem->spell_id = memspell->spell_id;
		mem->slot = memspell->slot;
		mem->scribing = memspell->scribing;
		QueuePacket(outapp);
		delete outapp;
		pp.spell_memory[memspell->slot] = memspell->spell_id;
	}
	else if (memspell->scribing == 2) {
		pp.spell_memory[memspell->slot] = 0xffff;
	}

	Save();
}
