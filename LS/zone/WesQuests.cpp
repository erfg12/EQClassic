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
#include "client.h"
#include "entity.h"
#include "zone.h"

#include <iostream.h>
#include <iomanip.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

// Disgrace: for windows compile
#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
	#include <process.h>

	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
#else
	#include <pthread.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include "../common/unix.h"
#endif

#include "worldserver.h"
#include "net.h"
#include "skills.h"
#include "../common/classes.h"
#include "../common/races.h"
#include "../common/database.h"
#include "spdat.h"
#include "../common/packet_functions.h"
#include "PlayerCorpse.h"
#include "spawn2.h"
#include "zone.h"
#include "../common/seperator-2.h"

extern Database database;
extern Zone* zone;
extern WorldServer worldserver;

bool Client::IsCommented(char* string) {
	if (string[0] == '#') {
		return true;
	}
	for (int32 u=0; u < strlen(string); u++)
	{
		if (string[u] == '#' && string[0] == '	') {
			return true;
		}
	}
	return false;
}

bool Client::IsEnd(char* string)
{
	if (string[0] == 125) {
		return true;
	}
	for (int32 u=0; u < strlen(string); u++)
	{
		if (string[u] == '}' && (string[0] == '	' || string[0] == ' ')) {
			return true;
		}
	}
	return false;
}

char * Client::strreplace(const char* searchstring,const char* searchquery, const char* replacement)
{
  static char newstringza[1024] = "";
memset(newstringza, 0, sizeof(newstringza));
  int32 i, j = 0, k;
  for (i=0; i < strlen(searchstring); ++i)
  {
    if (!strncmp(&searchstring[i], searchquery, strlen(searchquery)))
    {
      for (k = 0; k < strlen(replacement); ++k)
        newstringza[j++] = replacement[k];
      i += strlen(searchquery) - 1;
    }
    else
    {
      newstringza[j] = searchstring[i];
      ++j;
    }
  }
  return newstringza;
}

char * Client::rmnl(char* nstring)
{
	for (int32 e=0; e < strlen(nstring); e++)
	{
		if (nstring[e] == 13 || nstring[e] == 10) {
			printf("Edited!!! %d and %d\n", e, strlen(nstring));
			nstring[e] = ' ';
		}
	}
return nstring;
}

void Client::CheckQuests(const char* zonename, const char * message, int32 npc_id, uint16 item_id, Mob* other)
{
	bool ps = false;
	bool tt = false;
	bool ti = false;
	bool tt2 = false;
	bool ti2 = false;
	bool stt = false;
	bool std = false;
	bool td = false;
	bool sta = false;
	bool ta = false;
	bool sti = false;
	bool stf = false;
	bool tf = false;
	bool tk = false;
	bool stk = false;
	bool levelcheck = false;
	int8 fac = GetFactionLevel(GetID(),other->GetNPCTypeID(), GetRace(), GetClass(), DEITY_AGNOSTIC, other->CastToNPC()->GetPrimaryFaction(), other);
	FILE * pFile;
  long lSize;
  char * buffer;
  char filename[255];
  #ifdef WIN32
  snprintf(filename, 254, "quests\\%i.qst", npc_id);
  #else
  snprintf(filename, 254, "quests/%i.qst", npc_id);
  #endif
  adverrorinfo = 940;
  if ((pFile=fopen(filename,"rb")))
  {
		#ifdef DEBUG
		printf("Reading quests from %s\n", filename);
		#endif
  }
  else {
		#ifdef DEBUG
		printf("Error: No quests file found for %s\n", filename);
		#endif
		return;
  }
  if (pFile==NULL) exit (1);

  // obtain file size.
  fseek (pFile , 0 , SEEK_END);
  lSize = ftell (pFile);
  rewind (pFile);

  adverrorinfo = 941;
  // allocate memory to contain the whole file.
  buffer = (char*) malloc (lSize);
  if (buffer == NULL) exit (2);

  // copy the file into the buffer.
  fread (buffer,1,lSize,pFile);
  fclose(pFile);
  Seperator3 sep3(buffer);
  for(int i=0; i < 2048; ++i)
  {
//  printf("Temp: %s\n", sep3.arghz[i]);
	char * command;
	char * temp;
	temp = sep3.arghz[i];
	if (temp == NULL) return;
	Seperator sep(temp);
	Seperator4 sep4(temp);
	command = sep4.arghza[0];
	command = strupr(command);
#ifdef DEBUG
	cout<<sep.argplus[0]<<endl;
#endif
	if (!IsCommented(temp))
	{
	if (strstr(command,"END_FILE") != NULL || command == NULL)
	{
		break;
	}
	if (ti2 || tt2 || tf || td || ta) {
		char lvl[5];
		sprintf(lvl, "%i", this->GetLevel());
		strcpy(sep4.arghza[1], strreplace(sep4.arghza[1],"%CHARRACE%", GetRaceName(this->GetRace())));
		strcpy(sep4.arghza[1], strreplace(sep4.arghza[1],"%CHARLEVEL%", lvl));
		strcpy(sep4.arghza[1], strreplace(sep4.arghza[1],"%CHARCLASS%", GetEQClassName(this->GetClass(), 50)));
		char * nmessage = strreplace(sep4.arghza[1],"%CHARNAME%", this->GetName());
		nmessage[strlen(nmessage) - 1] = '\0';
		if (ti2 || tt2 || ta)
		{
			other->CastToNPC()->FaceTarget(this, true);
		}
		if ((fac == 7 || fac == 6) && (ti2 || tt2))
		{
			entity_list.NPCMessage(other,true,200,0,"%s says, 'I will have nothing to do with such as you. Begone.'",other->GetName());
			break;
		}
		else if (strstr(command,"FACTION_CHECK") != NULL) {
			int8 fac2 = atoi(sep.arg[1]);
			if ((fac2 <= 5 && fac > fac2) || (fac2 == 6 && fac == 8))
			{
				entity_list.NPCMessage(other,true,200,0,"%s says, 'I will have nothing to do with such as you. Begone.'",other->GetName());
				break;
			}
			ps = true;
		}
		else if (strstr(command,"SAY") != NULL) {
			entity_list.NPCMessage(this, false, 400, 0, "%s says, '%s'", other->GetName(), nmessage);
			ps = true;
		}
		else if (strstr(command,"EMOTE") != NULL) {
			cout<<"Emoting!"<<endl;
			entity_list.NPCMessage(this, false, 400, 0, "%s %s", other->GetName(), nmessage);
			ps = true;
		}
		else if (strstr(command,"TEXT") != NULL) {
			entity_list.NPCMessage(this, false, 400, 0, "%s", nmessage);
			ps = true;
		}
		else if (strstr(command,"SHOUT") != NULL) {
			entity_list.NPCMessage(this, false, 0, 13, "%s shouts, '%s'", other->GetName(), nmessage);
			ps = true;
		}
		else if (strstr(command,"SPAWN_ITEM") != NULL) {
			this->SummonItem(atoi(sep.arg[1]));
			ps = true;
		}
		else if (strstr(command,"ADD_HATELIST") != NULL) {
			other->CastToNPC()->AddToHateList(this, 100);
			ps = true;
		}
		else if (strstr(command,"DEPOP") != NULL) {
			other->CastToNPC()->Depop();
			ps = true;
		}
#ifdef GUILDWARS
		else if (strstr(command,"CITYTAKE") != NULL) {
			if(IsClient() && GuildDBID() != 0)
			{
				if(target->CastToNPC()->GetGuildOwner() == GuildDBID())
				{
			entity_list.NPCMessage(this, false, 400, 0, "%s says, 'You are a member of the guild that owns this city, why would you want to conquere it?'", other->GetName(), nmessage);				
				}
				else if(!IsAttackAllowed(other))
				{
			entity_list.NPCMessage(this, false, 400, 0, "%s says, 'You cannot conquere an allied city.'", other->GetName(), nmessage);				
				}
				else if(database.CityDefense(zone->GetZoneID(),0))
				{
			entity_list.NPCMessage(this, false, 400, 0, "%s says, 'This city still has men standing, I will not surrender!'", other->GetName(), nmessage);				
				}
				else if(database.SetCityGuildOwned(GuildDBID(),other->GetNPCTypeID()))
				{
			entity_list.NPCMessage(this, false, 400, 0, "%s says, 'You have won, I surrender to you.'", other->GetName(), nmessage);	
			zone->SetGuildOwned(GuildDBID());
			zone->Repop();			
				}
			}
			ps = true;
		}
#endif
		else if (strstr(command,"SPAWN_NPC") != NULL) {
				adverrorinfo = 751;
				const NPCType* tmp = 0;
				adverrorinfo = 752;
				int16 grid = atoi(sep.arg[2]);
				int8 guildwarset = atoi(sep.arg[3]);
				if ((tmp = database.GetNPCType(atoi(sep.arg[1]))))
				{
					adverrorinfo = 753;
					NPC* npc = new NPC(tmp, 0, other->GetX(), other->GetY(), other->GetZ(), heading);
					adverrorinfo = 754;
					npc->AddLootTable();
					adverrorinfo = 755;
					entity_list.AddNPC(npc,true,true);
					adverrorinfo = 756;
					Sleep(200);
					if(npc != 0)
					{
					if(grid > 0)
						npc->AssignWaypoints(grid);
					adverrorinfo = 757;
#ifdef GUILDWARS
					if(guildwarset > 0 && guildwarset == 1 && GuildDBID() > 0)
						npc->SetGuildOwner(GuildDBID());
#endif
					adverrorinfo = 758;
					npc->SendPosUpdate();
					}
			}
			ps = true;
		}
		else if (strstr(command,"LEVEL_CHECK") != NULL) 
      { 
         int8 Level1 = atoi(sep.arg[1]); 
         int8 Level2 = atoi(sep.arg[2]); 
         int8 Levelp = this->GetLevel(); 
          
         if(Level2 == 0 || Level2 == NULL){ 
            //Min Level 
            if(Level1 > Levelp) 
            { 
               //not high enough to talk too 
               break; 
            } 
         } 
         else{ 
            //Level Range 
            if(Level1 < Levelp && Level2 < Levelp) 
            { 
               //not in the req level range 
               break; 
            } 
         } 
         levelcheck = true; 
      }
		else if (strstr(command,"CUMULATIVE_FLAG") != NULL) {
			other->flag[50] = other->flag[50] + 1;
			ps = true;
		}
		else if (strstr(command,"NPC_FLAG") != NULL) {
			other->flag[atoi(sep.arg[1])] = atoi(sep.arg[2]);
			ps = true;
		}
		else if (strstr(command,"PLAYER_FLAG") != NULL) {
			this->flag[atoi(sep.arg[1])] = atoi(sep.arg[2]);
			ps = true;
		}
		else if (strstr(command,"EXP") != NULL) {
			this->AddEXP (atoi(sep.arg[1]));
			ps = true;
		}
		else if (strstr(command,"LEVEL") != NULL) {
			this->SetLevel(atoi(sep.arg[1]), true);
			ps = true;
		}
		else if (strstr(command,"SAFEMOVE") != NULL) {
			this->MovePC(zone->GetShortName(),database.GetSafePoint(zone->GetShortName(),"x"),database.GetSafePoint(zone->GetShortName(),"y"),database.GetSafePoint(zone->GetShortName(),"z"),false,false);
			ps = true;
		}
		else if (strstr(command,"RAIN") != NULL) {
			zone->zone_weather = atoi(sep.arg[1]);
			APPLAYER* outapp = new APPLAYER;
			outapp = new APPLAYER;
			outapp->opcode = OP_Weather;
			outapp->pBuffer = new uchar[8];
			memset(outapp->pBuffer, 0, 8);
			outapp->size = 8;
			outapp->pBuffer[4] = atoi(sep.arg[1]); // Why not just use 0x01/2/3?
			entity_list.QueueClients(this, outapp);
			delete outapp;
			ps = true;
		}
		else if (strstr(command,"SNOW") != NULL) {
			zone->zone_weather = atoi(sep.arg[1]) + 1;
			APPLAYER* outapp = new APPLAYER;
			outapp = new APPLAYER;
			outapp->opcode = OP_Weather;
			outapp->pBuffer = new uchar[8];
			memset(outapp->pBuffer, 0, 8);
			outapp->size = 8;
			outapp->pBuffer[0] = 0x01;
			outapp->pBuffer[4] = atoi(sep.arg[1]);
			entity_list.QueueClients(this, outapp);
			delete outapp;
			ps = true;
		}
		else if (strstr(command,"GIVE_CASH") != NULL) {
			this->AddMoneyToPP(atoi(sep.arg[1]),true);
			ps = true;
		} 
		else if (strstr(command,"GIVE_ITEM") != NULL) { 
			int16 ItemID = atoi(sep.arg[1]); 
			sint8 ItemCharges = atoi(sep.arg[2]); 
			this->SummonItem(ItemID, ItemCharges); 
			ps = true; 
		} 
		else if (strstr(command,"CAST_SPELL") != NULL) {
			other->CastSpell(atoi(sep.arg[1]),this->GetID());
			ps = true;
		}
		else if (strstr(command,"PVP") != NULL) {
			if (strstr(strupr(sep.arg[1]),"ON") != NULL)
				this->CastToClient()->SetPVP(true);
			else
				this->CastToClient()->SetPVP(false);
			ps = true;
		}
		/*else if (strstr(command,"CHANGEFACTION") != NULL) {
			if ((sep.IsNumber(1)) && (sep.IsNumber(2))) {
				this->CastToClient()->SetFactionLevel2(this->CastToClient()->CharacterID(),atoi(sep.arg[1]), this->CastToClient()->GetClass(), this->CastToClient()->GetRace(), this->CastToClient()->GetDeity(), atoi(sep.arg[2]));
				this->CastToClient()->Message(0,BuildFactionMessage(GetCharacterFactionLevel(atoi(sep.arg[1])),atoi(sep.arg[1])));
			}
			else
			{
				cout << "Error in script!!!!!  Bad CHANGEFACTION Line! " << endl;
			}
			ps = true;
		}*/
		else if (strstr(command,"DO_ANIMATION") != NULL) {
			other->DoAnim(atoi(sep.arg[1]));
			ps = true;
		}
		else if (strstr(command,"ADDSKILL") != NULL) {
			if (sep.IsNumber(1) && sep.IsNumber(2) && (atoi(sep.arg[2]) >= 0 || atoi(sep.arg[2]) <= 252) && (atoi(sep.arg[1]) >= 0 && atoi(sep.arg[1]) < 74)) {
				this->AddSkill(atoi(sep.arg[1]), atoi(sep.arg[2]));
			}
			else
				cout << "Error in script!!!! Bad ADDSKILL line!" << endl;
			ps = true;
		}
		else if (strstr(command,"FLAG_CHECK") != NULL)
		{
			if (this->flag[atoi(sep.arg[1])] == 0)
				break;
			else
				this->flag[atoi(sep.arg[1])] = 0;
			ps = true;
		}
		else if (strstr(command,"CHANCE") != NULL)
		{
			if (rand()%100 > atoi(sep.arg[1]))
				break;
			ps = true;
		}
	}
	if (IsEnd(temp)) 
	{
		tt = false; tt2 = false; stt = false; ti = false; ti2 = false; sti = false; tk = false;	td = false; std = false; ta = false; sta = false; tf = false; stf = false; stk = false; 
	}
		if (strstr(command,"TRIGGER_ATTACK") != NULL)
		{
			if (strstr(message,"%%ATTACK%%") != NULL) {
				adverrorinfo = 943;
				ta = true;
			}
			else { sta = true; }
		}
		else if (strstr(command,"TRIGGER_DEATH") != NULL)
		{
			if (strstr(message,"%%DEATH%%") != NULL)
			{
				td = true;
			}
			else { std = true; }
		}
		else if (strstr(command,"TRIGGER_KILLED") != NULL)
		{
			if (strstr(message,"%%KILLED%%") != NULL)
			{
				tk = true;
			}
			else { stk = true; }
		}
		else if (strstr(command,"TRIGGER_FLAGS") != NULL)
		{
			int8 flag1 = atoi(sep.arg[1]);
			int8 flag2 = atoi(sep.arg[2]);
			int8 flag3 = atoi(sep.arg[3]);
			int8 flag4 = atoi(sep.arg[4]);
			if (flag1 == 50)
			{
				if (other->flag[50] >= atoi(sep.arg[2]))
				{
					tf = true;
					other->flag[50] = 0;
				}
				else
					stf = true;
			}
			else
			{
				if (ta || tt || ti)
					stf = true;
				else if (flag1 > 0 && other->flag[flag1] == 0)
					stf = true;
				else if (flag2 > 0 && other->flag[flag2] == 0)
					stf = true;
				else if (flag3 > 0 && other->flag[flag3] == 0)
					stf = true;
				else if (flag4 > 0 && other->flag[flag4] == 0)
					stf = true;
				else
				{
					tf = true;
					if (flag1 > 0)
						other->flag[flag1] = 0;
					if (flag2 > 0)
						other->flag[flag2] = 0;
					if (flag3 > 0)
						other->flag[flag3] = 0;
				if (flag4 > 0)
							other->flag[flag4] = 0;
				}
			}
		}
		else if (strstr(command,"TRIGGER_ITEM") != NULL) {
			if (!ta && !tt && !ti)
			{
				if ((uint16)item_id == atoi(sep4.arghza[1]))
				{
					ti = true; ti2 = true;
				}
				else { sti = true; }
			}
			else
			{
				printf("TRIGGER_ITEM Error at line %d: missing left curly bracket\n", i);
				return;
			}
		}
		else if (strstr(command,"TRIGGER_TEXT") != NULL)
		{
			if (strstr(message,"%%DEATH%%") == NULL && strstr(message,"%%ITEM%%") == NULL && strstr(message,"%%ATTACK%%") == NULL && strstr(message,"%%KILLED%%") == NULL && Dist(other) <= 50) {
				char* tmp = new char[strlen(message)+1];
				strcpy(tmp, message);
				strupr(tmp);
				if (strstr(tmp,strupr(sep4.arghza[1])) != NULL)
				{
					tt2 = true; tt = true;
				}
				else
				{
					stt = true;
				}
				delete tmp;
			}
			else if (strstr(message,"%%DEATH%%") != NULL || strstr(message,"%%item%%") != NULL || strstr(message,"%%attack%%")) { stt = true; }
			else if (!ps) { printf("TRIGGER_TEXT Error at line %d: Not in NPC_Script\n", i); return; }
			else
			{
				printf("TRIGGER_TEXT Error at line %d: missing left curly bracket\n", i);
				return;
			}
		}
	/*#ifdef WIN32
		delete command;
		delete temp;
	#endif*/
}
}
	if (!ps && item_id != 0)
	{
		entity_list.NPCMessage(this,true,200,0,"%s has no use for this item.",other->GetName());
		SummonItem(item_id);
	}
	delete buffer;
}

