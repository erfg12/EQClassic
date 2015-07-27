// Quest Parser written by Wes, Leave this here =P

//#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream.h>

#include "../common/debug.h"
#include "client.h"
#include "entity.h"

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
#include "event_codes.h"
#include <time.h>
#include "basic_functions.h"

extern Database database;
extern Zone* zone;
extern WorldServer worldserver;
extern EntityList entity_list;

#define Parser_DEBUG 0

int Parser::CheckAliases(const char * alias, int32 npcid, Mob* npcmob, Mob* mob)
{
	MyListItem <Alias> * Ptr = AliasList.First;

	while (Ptr) {
		if ( (int32)Ptr->Data->npcid == npcid) {
			for (int i=0; i <= Ptr->Data->index; i++) {
				if (!strcmp(strlwr(Ptr->Data->name[i]),alias)) {
					CommandEx(Ptr->Data->command[i], npcid, npcmob, mob);
					return 1;
				}
			}
		}
		Ptr = Ptr->Next;
	}
	return 0;
}


int Parser::pcalc(char * string) {
	char temp[100];
	memset(temp, 0, sizeof(temp));
	char temp2[100];
	memset(temp2, 0, sizeof(temp2));
	int p =0;
	char temp3[100];
	memset(temp3, 0, sizeof(temp3));
	char temp4[100];
	memset(temp4, 0, sizeof(temp4));
	while (strrchr(string,'(')) {
		strn0cpy(temp,strrchr(string,'('), sizeof(temp));
		for ( unsigned int i=0;i < strlen(temp); i++ ) {
			if (temp[i] != '(' && temp[i] != ')') {
				temp2[p] = temp[i];
				p++;
			}
			else if (temp[i] == ')') {
				snprintf(temp3, sizeof(temp3), "(%s)", temp2);
				Replace(string,temp3,itoa(calc(temp2),temp4,10),0);
				memset(temp, 0, sizeof(temp));
				memset(temp2, 0, sizeof(temp2));
				memset(temp3, 0, sizeof(temp3));
				memset(temp4, 0, sizeof(temp4));
				p=0;
			}
		}
	}
	return calc(string);
}

char * Parser::strrstr(char* string, const char * sub) {





	static char temp[1000];
	memset(temp, 0, sizeof(temp));
	char temp2[1000];
	memset(temp2, 0, sizeof(temp2));
	int32 pchlen = strlen(string) + 1;
	char *pchbak, *pch = pchbak = new char[pchlen];
	memset(pch, 0, pchlen);
	strn0cpy(pch, string, pchlen);
	while(( pch = strstr(pch, sub))) {
		strn0cpy(temp2, strstr(pch, sub), sizeof(temp2));
		pch+=strlen(sub);

		if (strstr(pch, sub)) {
			strn0cpy(temp, strstr(pch, sub), sizeof(temp));
		}
		else if (strstr(temp2,sub)) {
			strn0cpy(temp, strstr(temp2, sub), sizeof(temp));
		}

	}
	safe_delete(pchbak);
	if (strstr(temp, sub))
		return temp;
	else
		return 0;
}

void Parser::MakeVars(const char * string, int32 npcid) {
	char temp[100];
	memset(temp, 0, sizeof(temp));
	char temp2[100];
	memset(temp2, 0, sizeof(temp2));

	int tmpfor = numtok((char*)string, " ")+1;
	for ( int i=0; i < tmpfor; i++) {
		memset(temp2, 0, sizeof(temp2));
		strn0cpy(temp2, gettok((char*)string, 32, i), sizeof(temp2));
		snprintf(temp, sizeof(temp), "%s.%d", itoa(i+1 ,temp, 10),npcid);
		AddVar(temp, temp2);
		memset(temp, 0, sizeof(temp));
		snprintf(temp, sizeof(temp), "%s-.%d", itoa(i+1, temp, 10),npcid);
		AddVar(temp, strstr(string, temp2));
	}
}

void Parser::MakeParms(const char * string, int32 npcid) {
	char temp[100];
	memset(temp, 0, sizeof(temp));
	char temp2[100];
	memset(temp2, 0, sizeof(temp2));
	char temp3[100];
	memset(temp3, 0, sizeof(temp3));

	int tmpfor = numtok((char*)string, ",")+1;
	for ( int i=0; i < tmpfor; i++) {
		memset(temp2, 0, sizeof(temp2));
		strn0cpy(temp2, gettok((char*)string, ',', i), sizeof(temp2));
		snprintf(temp, sizeof(temp), "param%s.%d", itoa(i+1 ,temp3, 10),npcid);
		AddVar(temp, temp2);
	}
}

int Parser::GetItemCount(char * itemid, int32 npcid)
{
	int a=0;
	char temp[100];
	memset(temp,0x0,100);
	for (int i=1;i<5;i++)
	{
		sprintf(temp,"item%d",i);
		if (!strcmp(GetVar(temp,npcid),itemid)) {
			a++;
		}
	}
	return a;
}
void Parser::Event(int event, int32 npcid, const char * data, Mob* npcmob, Mob* mob) {
	if (npcid == 0)
		return;
//	EventList.ClearListAndData();
	LoadScript(npcid, zone->GetShortName());
	int32 qstID = GetNPCqstID(npcid);
	if ((qstID == 0 && pNPCqstID[0] >= 0xFFFFFFF0) || qstID >= 0xFFFFFFF0)
		return;

	int8 fac = 0;

	char temp[255];
	memset(temp,0x0,255);

	if (mob && mob->IsClient()) {
		
	database.GetGuildNameByID(mob->CastToClient()->GuildDBID(),temp);
	AddVar("guild_name.g",temp);
	AddVar("uguildid.g", itoa(mob->CastToClient()->GuildDBID(),temp,10));
	AddVar("uguildrank.g", itoa(mob->CastToClient()->GuildRank(),temp,10));
#ifdef GUILDWARS
	AddVar("zoneownedguildid.g", itoa(zone->GetGuildOwned(),temp,10));
#ifdef GUILDWARS2
	AddVar("zoneownedfunds.g", itoa(zone->GetAvailableFunds()/1000,temp,10));
#endif
#endif
	//printf("Debug Event: %s\n", mob->GetName());
	printf("Debug Event: %s\n", mob->GetName());
	memset(temp,0x0,255);
	}

	if (mob && npcmob && mob->IsClient()) {
		fac = mob->CastToClient()->GetFactionLevel(mob->CastToClient()->GetID(), npcmob->GetID(), mob->CastToClient()->GetRace(), mob->CastToClient()->GetClass(), DEITY_AGNOSTIC, npcmob->CastToNPC()->GetPrimaryFaction(), npcmob);
		memset(temp,0x0,255);
	}
	if (mob) {
	AddVar("name.g",   mob->GetName());
	AddVar("race.g",   GetRaceName(mob->GetRace()));
	AddVar("class.g",  GetEQClassName(mob->GetClass()));
	AddVar("ulevel.g", itoa(mob->GetLevel(),temp,10));
	AddVar("userid.g", itoa(mob->GetID(),temp,10));
	memset(temp,0x0,255);
	}

	if (fac) {
		database.GetFactionName(npcmob->CastToNPC()->GetPrimaryFaction(),temp,sizeof(temp));
		AddVar("faction_name.g", temp);
		AddVar("faction.g", itoa(fac,temp,10));
		memset(temp,0x0,255);
	}

	if (zone) {
		char timeA[255];

		AddVar("zoneln.g",zone->GetLongName());
		AddVar("zonesn.g",zone->GetShortName());
		
		memset(timeA,0x0,255);
		time_t timeCurrent = time(NULL);
		TimeOfDay_Struct eqTime;
		zone->zone_time.getEQTimeOfDay( timeCurrent, &eqTime);
		snprintf(timeA, sizeof(timeA), "%02d:%s%d %s" , (eqTime.hour % 12) == 0 ? 12 : (eqTime.hour % 12), (eqTime.minute < 10) ? "0" : "",eqTime.minute, (eqTime.hour >= 12) ? "pm" : "am");
		snprintf(temp, sizeof(temp), "%s", (
			(eqTime.day == 1) ? "Sunday"    : 
			(eqTime.day == 2) ? "Monday"    : 
			(eqTime.day == 3) ? "Tuesday"   : 
			(eqTime.day == 4) ? "Wednesday" : 
			(eqTime.day == 5) ? "Thursday"  : 
			(eqTime.day == 6) ? "Friday"    : 
			(eqTime.day == 7) ? "Saturday"  : "No Day"
				)
				);
		AddVar("day.g",temp);
		AddVar("time.g",timeA);
	memset(temp,0x0,255);
	}

//#ifdef CATCH_CRASH
//	try {
//#endif
	switch (event) {
		case EVENT_SAY: {
			MakeVars(data, npcid);
			SendCommands("event_say", qstID, npcmob, mob);
			break;
		}
		case EVENT_DEATH: {
			SendCommands("event_death", qstID, npcmob, mob);
			break;
		}
		case EVENT_ITEM: {
			SendCommands("event_item", qstID, npcmob, mob);
			break;
		}
		case EVENT_SPAWN: {
			SendCommands("event_spawn", qstID, npcmob, mob);
			break;
		}
		case EVENT_ATTACK: {
			SendCommands("event_attack", qstID, npcmob, mob);
			break;
		}
		case EVENT_SLAY: {
			SendCommands("event_slay", qstID, npcmob, mob);
			break;
		}
		case EVENT_WAYPOINT: {
			sprintf(temp,"wp.%d",npcid);
			AddVar(temp,data);
			SendCommands("event_waypoint", qstID, npcmob, mob);
			break;
		}
		default: {
			// should we do anything here?
			break;
		}
	}
//#ifdef CATCH_CRASH
//	}
/*	catch(char* errstr) {
		LogFile->write(EQEMuLog::Error, "Parser crash caught: event: %i  npcid: %u  qstID: %u  '%s'", event, npcid, qstID, errstr);
		worldserver.SendEmoteMessage(0, 0, 100, 13, "Parser error caught on ZSPID: %i, EventID: %i, NPCID: %u, qstID: %u", getpid(), event, npcid, qstID);
		ClearCache();
	}
	catch(...) {
		LogFile->write(EQEMuLog::Error, "Parser crash caught: event: %i  npcid: %u  qstID: %u", event, npcid, qstID);
		worldserver.SendEmoteMessage(0, 0, 100, 13, "Parser error caught on ZSPID: %i, EventID: %i, NPCID: %u, qstID: %u", getpid(), event, npcid, qstID);
	}
#endif*/
	DelChatAndItemVars(npcid);

}

int Parser::numtok(char * string, const char * tok) {
	int q = 0;
	int32 tmpStringLen = strlen(string);
	for (unsigned int i=0;i<tmpStringLen;i++) {
		if (string[i] == tok[0])
			q++;
	}
	return q;
}

Parser::Parser() {
	pMaxNPCID = database.GetMaxNPCType();
	pNPCqstID = new int32[pMaxNPCID+1];
	for (int32 i=0; i<pMaxNPCID+1; i++)
		pNPCqstID[i] = 0xFFFFFFFF;
	commands = new command_list[100];
}

Parser::~Parser() {
	EventList.ClearListAndData();
	varlist.ClearListAndData();
	AliasList.ClearListAndData();
	safe_delete(pNPCqstID);
	safe_delete(commands);
}

bool Parser::LoadAttempted(int32 iNPCID) {
	if (iNPCID > pMaxNPCID)
		return false;
	return (bool) (pNPCqstID[iNPCID] != 0xFFFFFFFF);
}

bool Parser::SetNPCqstID(int32 iNPCID, int32 iValue) {
	if (iNPCID > pMaxNPCID)
		return false;
	pNPCqstID[iNPCID] = iValue;
	return true;
}

int32 Parser::GetNPCqstID(int32 iNPCID) {
	if (iNPCID > pMaxNPCID || iNPCID == 0)
		return 0xFFFFFFFE;
	return pNPCqstID[iNPCID];
}

void Parser::ClearCache() {
#if Parser_DEBUG >= 2
	cout << "Parser::ClearCache" << endl;
#endif
	for (int32 i=0; i<pMaxNPCID+1; i++)
		pNPCqstID[i] = 0xFFFFFFFF;
}

int Parser::ParseIt(char * info, int32 npcid, Mob* mob, Client* client)
{
	char buffer[100];
	char command[2048];
	char temp[100];
	int a=0;
	int paren=0;
	memset(buffer,0x0,100);
	memset(command,0x0,2048);
	for (int i=0; i < strlen(info);i++)
	{
		if (info[i] == ';')
		{
			if (paren == 0) {
//				if (command[0] == '%') { command[strlen(command)] = ';'; EnumerateVars(command,npcid); }
//			else { CommandEx(command, npcid, mob, client); }
			memset(command,0x0,2048);
			a=0;
			}
			else {
				printf("Error in script near \"%s\" missing closing \")\"?\n", command);
				return 0;
			}
		}
		else {
			command[a] = info[i];
			a++;
		}
		if (info[i] == '(') paren++;
		if (info[i] == ')') paren--;
	}
	return 1;
}

void Parser::SendCommands(const char * event, int32 npcid, Mob* npcmob, Mob* mob) {
	MyListItem <Events> * Ptr = EventList.First;

	while (Ptr) {
		if ( (int32)Ptr->Data->npcid == npcid) {
			for (int i=0; i <= Ptr->Data->index; i++) {
				if (!strcmp(strlwr(Ptr->Data->event[i]),event)) {
					Line_Number = Ptr->Data->line[i];
					CommandEx(Ptr->Data->command[i], npcid, npcmob, mob);
				}
			}
		}
		Ptr = Ptr->Next;
	}
	
}

void Parser::scanformat(char *string, const char *format, char arg[10][1024])
{
	int increment_arglist = 0;
	int argnum = 0;
	int i = 0;
	char lookfor = 0;

	// someone forgot to set string or format
	if(!format)
		return;
	if(!string)
		return;

	for(;;)
	{
		// increment while they're the same (and not NULL)
		while(*format && *string && *format == *string) {
			format++;
			string++;
		}

		// format string is gone
		if(!format)
			break;
		// string is gone while the format string is still there (ERRor)
		if(!string)
			return;

		// the format string HAS to be equal to ÿ or else things are messed up
		if(*format != 'ÿ')
			return;

		format++;
		lookfor = *format;  // copy until we find char after 'y'
		format++;

		if(!lookfor)
			break;

		// start on a new arg
		if(increment_arglist) {
			arg[argnum][i] = 0;
			argnum++;
		}

		increment_arglist = 1; // we found the first crazy y
		i = 0;  // reset index

		while(*string && *string != lookfor)
			arg[argnum][i++] = *string++;
		string++;
	}

	// final part of the string
	if(increment_arglist) {
		arg[argnum][i] = 0;
		argnum++;
		i = 0;
	}

	while(*string)
		arg[argnum][i++] = *string++;

	arg[argnum][i] = 0;
}

bool Parser::EnumerateVars(char * string, int32 npcid, Mob* mob) {
	//printf("Debug Enumerate Vars: %s\n", mob->GetName());
	char varname[100];
	char varval[100];
	char buffer[100];
	char temp[100];
	int pos = 0;
	int pos2 = 0;
	int record = 0;
	int equal = 0;
	memset(buffer, 0x0, sizeof(buffer));
	memset(varname, 0x0, sizeof(varname));
	memset(varval, 0x0, sizeof(varval));
	memset(temp, 0x0, sizeof(temp));
	for (int i=0; i < strlen(string); i++) {
		if (string[i] == 'ÿ' && varname[0]) {
			printf("Parser::EnumerateVars():: Run away %%variable \"%s\". missing \";\" in statement? Line: %d\n",varname,Line_Number);
			return false;
		}
		if ( record || string[i] == '%') {
			buffer[pos2] = string[i];
			pos2++;
		}
		if (record) {
			if (string[i] != '=' && string[i] != ';') {
				temp[pos] = string[i];
				pos++;
			}
			if (string[i] == '=' && string[i-1] != '>' && string[i-1] != '<' && (string[i+1] != '=' && string[i-1] != '=') && string[i-1] != '!') {
				strn0cpy(varname, temp, sizeof(varname));
				memset(temp, 0, sizeof(temp));
				pos = 0;
				equal = 1;
			}
			if (string[i] == ';' && equal == 1) {
				strn0cpy(varval, temp, sizeof(varval));
			Replace(varval,"\"","",1);
			if (strstr(varval,"$calc"))
				 ParseVars(varval,npcid,mob);
			if (!(varname[strlen(varname)-2] == '.' && (varname[strlen(varname)-1] == 'g' || varname[strlen(varname)-1] == 'G')))
				snprintf(varname, sizeof(varname), "%s.%d", varname, npcid);
				AddVar(varname,varval);
#if Parser_DEBUG >= 9
				printf("Parser::EnumerateVars() - Adding %s and %s\n", varname, varval);
#endif
				Replace(string,buffer,"");
				memset(temp, 0, sizeof(temp));
				memset(buffer, 0, sizeof(buffer));
				memset(varname, 0, sizeof(varname));
				memset(varval, 0, sizeof(varval));
				pos2 = 0;
				pos = 0;
				i=-1;
				record = 0;
				equal = 0;
			}
			if (string[i+1] == '%' && equal == 0) {
				memset(temp, 0, sizeof(temp));
				memset(buffer, 0, sizeof(buffer));
				pos = 0;
				pos2 = 0;
				record = 0;
			}
		}
		else {
			if (string[i] == '%') {
				record = 1;
			}
		}
	}
	return true;
}


void Parser::CommandEx(char * command, int32 npcid, Mob* other, Mob* mob) {
	//printf("Debug CommandEx: %s\n", mob->GetName());
	char temp[1024];
	char arg1[1024];
	char tempBuff1[2048];
	char varstat[2048];
	char com[100];
	memset(com,0x0,100);
	memset(temp, 0, sizeof(temp));
	memset(arg1, 0, sizeof(arg1));
	memset(tempBuff1, 0, sizeof(tempBuff1));
	memset(varstat, 0, sizeof(varstat));
	int ty = 0;
	int a = 0;
	int ignore = 0;
	int numargs = 0;
	int while2 = 0;
	char arglist[10][1024];
	memset(arglist, 0, sizeof(arglist));

	strn0cpy(tempBuff1, command, sizeof(tempBuff1));
	EnumerateVars(tempBuff1,npcid,mob);
	ParseVars(tempBuff1,npcid,mob);
	int number = numtok(tempBuff1,"ÿ");
	int while1 = 0;

	for (int i=0; i <= strlen(tempBuff1); i++)
	{
		temp[ty] = tempBuff1[i];
		ty++;
		if (tempBuff1[i] == 'ÿ')  {
#if Parser_DEBUG >= 9
			printf("While1: %d\n", while1);
#endif
			if (!while2) {
				a++;
#if Parser_DEBUG >= 9
				printf("Parser::CommandEx() Ignoring %s because if statement was false.\n", temp);
#endif
				ignore = 0;
			}
			else {
				memset(tempBuff1, 0, sizeof(tempBuff1));
				char t1[1000];
				memset(t1, 0, sizeof(t1));
				strn0cpy(t1, postring(command, while1), sizeof(t1));
				strn0cpy(tempBuff1, strstr(t1, "while("), sizeof(tempBuff1));
				EnumerateVars(tempBuff1,npcid,mob);
				ParseVars(tempBuff1,npcid,mob);
				number = numtok(tempBuff1,"ÿ");
				i = -1;
				a=0;
			}
			memset(temp, 0, sizeof(temp));
			ty = 0;
		}

		if (tempBuff1[i] == '(') {
			memset(com,0x0,sizeof(com));
			strn0cpy(com,temp, sizeof(com));
			com[strlen(com) - 1] = '\0'; // Quag th is used to kill the "(" at the end of the string so "com" will return example "say" instead of "say(".
		}
		if (!ignore && (tempBuff1[i] == ')' || i == (int)strlen(tempBuff1)-1)) {
			if (a >= number) return;
			numargs = GetArgs(strlwr(com));
			if ( numargs > 0) {
				//snprintf(varstat, sizeof(varstat), "ÿ(");
				strcat(varstat, "ÿ(");
				if (numargs > 1) {
					for (int h=0; h < numargs; h++)
					//snprintf(varstat, sizeof(varstat), "%s\"ÿ\",", varstat);
					strcat(varstat, "\"ÿ\",");
					if (varstat[strlen(varstat)-1] == ',')
						varstat[strlen(varstat)-1] = '\0';
					//snprintf(varstat, sizeof(varstat), "%s)", varstat);
					strcat(varstat, ")");
				}
				else {
					//snprintf(varstat, sizeof(varstat), "%s\"ÿ\")", varstat);
					strcat(varstat, "\"ÿ\")");
				}
			}
			else {
				//snprintf(varstat, sizeof(varstat), "ÿ(ÿ)");
				strcat(varstat, "ÿ(ÿ)");
			}

			memset(arglist, 0, sizeof(arglist));
			//printf("varstat=%s\n",varstat);
			scanformat(temp, varstat, arglist);
#if Parser_DEBUG >= 9
			printf("Parser::CommandEx() Command - Name: %s - FirstArg: \"%s\" ArgCount: %i \n", com, arglist[0], GetArgs(strlwr(com)));
#endif
			memset(varstat, 0, sizeof(varstat));
			memset(temp, 0, sizeof(temp));
			ty = 0;


			if (strstr(strlwr(arglist[0]),"if")) {
				int GoOn = ParseIf(arglist[1]);
			#if Parser_DEBUG >= 9
				if (GoOn) {
				printf("Parser::CommandEx() IF statement returned true: %s\n", arglist[1]);
				}
				else if (!GoOn) {
				printf("Parser::CommandEx() IF statement returned failed: %s\n", arglist[1]);
				}
			#endif
				if (!GoOn && (a <= number))
					ignore = 1;
			}
			else if (!strcmp(strlwr(arglist[0]),"break")) {
				break;
			}
			else if (!strcmp(strlwr(arglist[0]),"write")) {
				char temp[1024];
				memset(temp,0x0,1024);
				snprintf(temp, sizeof(temp), "%s\n", arglist[2]);
					FILE * pFile;
					pFile = fopen (arglist[1], "a");
					fwrite (temp , 1 , strlen(temp) , pFile);
					fclose (pFile);
			}
			else if (!strcmp(strlwr(arglist[0]),"me")) {
				if (mob->IsClient()) entity_list.MessageClose(mob->CastToClient(), false, 200, 10, "%s", arglist[1]);
			}
			else if (!strcmp(strlwr(arglist[0]),"while")) {
				int GoOn = ParseIf(arglist[1]);
				if (GoOn) {
//					printf("Test-TempBuffarg: %s\n", postring(tempBuff1,i-(strlen(arglist[1])+strlen("while("))));
					while1 = i-(strlen(arglist[1])+strlen("while("));
					while2 = 1;
				}
				else {
					ignore = 1;
					while1 = 0;
					while2 = 0;
				}
			}
			else if (!strcmp(strlwr(arglist[0]),"spawn")) {
				//char tempa[100];
				const NPCType* tmp = 0;
				int16 grid = atoi(arglist[2]);
				int8 guildwarset = atoi(arglist[3]);
				if ((tmp = database.GetNPCType(atoi(arglist[1])))) {
					NPC* npc = new NPC(tmp, 0, atof(arglist[4]), atof(arglist[5]), atof(arglist[6]), mob->CastToClient()->GetHeading());
					npc->AddLootTable();
					entity_list.AddNPC(npc,true,true);
					// Quag: Sleep in main thread? ICK!
					// Sleep(200);
					// Quag: check is irrelevent, it's impossible for npc to be 0 here
					// (we're in main thread, nothing else can possibly modify it)
//					if(npc != 0) {
						if(grid > 0)
							npc->AssignWaypoints(grid);
#ifdef GUILDWARS
						if(guildwarset > 0 && guildwarset == 1 && mob->CastToClient()->GuildDBID() > 0)
							npc->SetGuildOwner(mob->CastToClient()->GuildDBID());
#endif
						npc->SendPosUpdate();
//					}
				}
			}
#ifdef GUILDWARS
			else if (strstr(strlwr(arglist[0]),"dbspawnadd")) {
				const NPCType* tmp = 0;
				if ((tmp = database.GetNPCType(atoi(arglist[1])))) {
				NPC* npc = new NPC(tmp, 0, mob->CastToClient()->GetX(), mob->CastToClient()->GetY(), mob->CastToClient()->GetZ(), mob->CastToClient()->GetHeading());
				npc->AddLootTable();
				npc->SetGuildOwner(atoi(arglist[2]));
				//npc->respawn2->SetRespawnTimer(900);
				database.NPCSpawnDB(1,zone->GetShortName(),npc,1800);
				}
			else if (strstr(strlwr(arglist[0]),"cityfaction")) { // not khuong
					if(mob->IsClient() && other->IsNPC() && other->CastToNPC()->GetGuildOwner() != 0 && other->CastToNPC()->GetGuildOwner() == mob->CastToClient()->GuildDBID() && other->CastToNPC()->respawn2 != 0 && strlen(arglist[1]) != 0)
					{
					int8 newstatus=0;
					if(strstr(strlwr(arglist[1]),"neutral"))
					{
						entity_list.NPCMessage(other, false, 400, 0, "%s says, 'I will remain neutral for un-allied guild members.'", other->GetName());
						newstatus = 1;
					}
					else if(strstr(strlwr(arglist[1]),"scowl"))
					{
						entity_list.NPCMessage(other, false, 400, 0, "%s says, 'I will now scowl at un-allied guild members.'", other->GetName());
						newstatus = 0;
					}
					else
						entity_list.NPCMessage(other, false, 400, 0, "%s says, 'Available factions are neutral and scowl.'", other->GetName());



					other->CastToNPC()->cityfactionstanding = newstatus;
					database.UpdateCityDefenseFaction(zone->GetZoneID(),other->GetNPCTypeID(),other->CastToNPC()->respawn2->SpawnGroupID(),newstatus);
					}
			}
			else if (strstr(strlwr(arglist[0]),"report")) { // not khuong
					if(mob->IsClient() && other->IsNPC() && other->CastToNPC()->GetGuildOwner() != 0 && other->CastToNPC()->GetGuildOwner() == mob->CastToClient()->GuildDBID() && other->CastToNPC()->respawn2 != 0 && strlen(arglist[1]) != 0)
					{
						char faction[100];
						switch(other->CastToNPC()->cityfactionstanding)
						{
						case 0:
							sprintf(faction,"scowling");
							break;
						case 1:
							sprintf(faction,"neutral");
							break;
						}
						entity_list.NPCMessage(other, false, 400, 0, "%s says, 'I am currently %s at un-allied guilds at this time.'", other->GetName(),faction);
					}
			}
			}
#endif
			else if (strstr(strlwr(arglist[0]),"echo")) {
				printf("%s\n", arglist[1]);
			}
			else if (strstr(strlwr(arglist[0]),"summonitem")) {
				mob->CastToClient()->SummonItem(atoi(arglist[1]));
			}
			else if (strstr(strlwr(arglist[0]),"castspell")) {
				other->CastSpell(atoi(arglist[2]), atoi(arglist[1]));
			}
			else if (strstr(strlwr(arglist[0]),"say")) {
				entity_list.NPCMessage(other, false, 400, 0, "%s says, '%s'", other->GetName(), arglist[1]);
			}
			else if (strstr(strlwr(arglist[0]),"emote")) {
				entity_list.NPCMessage(other, false, 400, 0, "%s %s", other->GetName(), arglist[1]);
			}
			else if (strstr(strlwr(arglist[0]),"shout")) {
				entity_list.NPCMessage(other, false, 0, 13, "%s shouts, '%s'", other->GetName(), arglist[1]);
			}
			else if (strstr(strlwr(arglist[0]),"depop")) {
				other->CastToNPC()->Depop();
			}
			else if (strstr(strlwr(arglist[0]),"settarget")) {
				Mob* tmp=0;
				if (!strcmp(strlwr(arglist[1]),"npctype")) {
					tmp = entity_list.GetMobByNpcTypeID(atoi(arglist[2]));
					printf("Npctype\n");
				}
				else if (!strcmp(strlwr(arglist[1]),"entity")) {
					tmp = entity_list.GetMob(atoi(arglist[2]));
					printf("Entity\n");
				}
				if (tmp) printf("Target set to %s\n", tmp->GetName());
				if (tmp) {
					printf("Debug SetTarget - %s and %s\n", tmp->GetName(), other->GetName());
					other->SetTarget(tmp);
				}
			}
			else if (strstr(strlwr(arglist[0]),"follow"))  {
				if (other) other->SetFollowID(atoi(arglist[1]));
			}
			else if (strstr(strlwr(arglist[0]),"sfollow"))  {
				if (other) other->SetFollowID(0);
			}
			else if (strstr(strlwr(arglist[0]),"cumflag")) {
				other->flag[50] = other->flag[50] + 1;
			}
			else if (strstr(strlwr(arglist[0]),"flagnpc")) {
				int32 tmpFlagNum = atoi(arglist[1]);
				if (tmpFlagNum >= (sizeof(other->flag) / sizeof(other->flag[0])))
					other->flag[tmpFlagNum] = atoi(arglist[2]);
				else {
					// Quag: TODO: Script error here, handle it somehow?
				}
			}
			else if (strstr(strlwr(arglist[0]),"flagmob->CastToClient()")) {
				if (mob->IsClient())
					mob->CastToClient()->flag[atoi(arglist[1])] = atoi(arglist[2]);
			}
			else if (strstr(strlwr(arglist[0]),"exp")) {
				if (mob->IsClient())
					mob->CastToClient()->AddEXP(atoi(arglist[1]));
			}
#ifdef GUILDWARS2
			else if (strstr(strlwr(arglist[0]),"sayavailablefunds")) {
sint32 funds = zone->GetAvailableFunds()/1000;
entity_list.NPCMessage(other, false, 400, 0, "%s says, 'There is a total of %i platinum in this cities bank.'", other->GetName(),funds);
			}
			else if (strstr(strlwr(arglist[0]),"giveguildfunds")) {
					sint32 avail = zone->GetAvailableFunds()/1000;
					if(atoi(arglist[1]) > 0 && avail >= atoi(arglist[1]))
					{
					zone->SetAvailableFunds(zone->GetAvailableFunds()-atoi(arglist[1])*1000);
					mob->CastToClient()->AddMoneyToPP(0,0,0,atoi(arglist[1]),true);
entity_list.NPCMessage(other, false, 400, 0, "%s says, 'You have taken %i platinum from the bank.'",other->GetName(), atoi(arglist[1]));
					}
					else
entity_list.NPCMessage(other, false, 400, 0, "%s says, 'There is not enough platinum in the bank to cover that request.'",other->GetName());
			}
#endif
			else if (strstr(strlwr(arglist[0]),"level")) {
				if (mob->IsClient())
					mob->CastToClient()->SetLevel(atoi(arglist[1]), true);
			}
			else if (strstr(strlwr(arglist[0]),"safemove")) {
				if (mob->IsClient())
					mob->CastToClient()->MovePC(zone->GetShortName(),database.GetSafePoint(zone->GetShortName(),"x"),database.GetSafePoint(zone->GetShortName(),"y"),database.GetSafePoint(zone->GetShortName(),"z"));
			}
			else if (strstr(strlwr(arglist[0]),"rain")) {
				zone->zone_weather = atoi(arglist[1]);
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				*((int32*) outapp->pBuffer[4]) = atoi(arglist[1]); // Why not just use 0x01/2/3?
				entity_list.QueueClients(other, outapp);
				delete outapp;
			}
			else if (strstr(strlwr(arglist[0]),"snow")) {
				zone->zone_weather = atoi(arglist[1]) + 1;
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				outapp->pBuffer[0] = 0x01;
				*((int32*) outapp->pBuffer[4]) = atoi(arglist[1]);
				entity_list.QueueClients(mob->CastToClient(), outapp);
				delete outapp;
			}
			else if (strstr(strlwr(arglist[0]),"givecash")) {
				APPLAYER* outapp = new APPLAYER(OP_MoneyOnCorpse, sizeof(moneyOnCorpseStruct));
				moneyOnCorpseStruct* d = (moneyOnCorpseStruct*) outapp->pBuffer;
		
				//d->response		= 1;
				d->unknown1		= 0x5a;
				d->unknown2		= 0x40;
				if (mob->IsClient()) { 
					d->copper		= atoi(arglist[1]);
					d->silver		= atoi(arglist[2]);
					d->gold			= atoi(arglist[3]);
					mob->CastToClient()->AddMoneyToPP(d->platinum, d->gold, d->silver, d->copper,true);
					mob->CastToClient()->QueuePacket(outapp); 
				}
				delete outapp;
			}
			else if (strstr(strlwr(arglist[0]),"pvp")) {
				if (!strcmp(strlwr(arglist[1]),"on") != NULL)
					mob->CastToClient()->SetPVP(true);
				else

					mob->CastToClient()->SetPVP(false);
			}
			else if (strstr(strlwr(arglist[0]),"movepc")) { 
				if (mob->IsClient()) 
					 mob->CastToClient()->MovePC((atoi(arglist[1])),(atoi(arglist[2])),(atoi(arglist[3])),(atoi(arglist[4]))); 
			}
			else if (strstr(strlwr(arglist[0]),"doanim")) {
				other->DoAnim(atoi(arglist[1]));
			}
			else if (strstr(strlwr(arglist[0]),"addskill")) {
				if (mob->IsClient()) mob->CastToClient()->AddSkill(atoi(arglist[1]), atoi(arglist[2]));
			}
			else if (strstr(strlwr(arglist[0]),"setallskill")) { // khuong
				int8 skill_id = atoi(arglist[1]);
				if (mob->IsClient()) { 
				for(int skill_num=0;skill_num<74;skill_num++){
				mob->CastToClient()->SetSkill(skill_num, skill_id);
					}
				}
			else if (strstr(strlwr(arglist[0]),"attack")) { // not khuong
				Client* getclient = entity_list.GetClientByName(arglist[1]);
				if(getclient && other && other->IsNPC() && other->IsAttackAllowed(getclient))
				{
				other->CastToNPC()->AddToHateList(getclient,1);
				}
				else
				entity_list.NPCMessage(other, false, 400, 0, "%s says, 'I am unable to attack %s.'", other->GetName(), arglist[1]);
			}
			}
			else if (strstr(strlwr(arglist[0]),"save")) { // khuong
				if (mob->IsClient()) mob->CastToClient()->Save();
			}
			else if (strstr(strlwr(arglist[0]),"flagcheck")) {
				int32 tmpFlagNum1 = atoi(arglist[1]);
				int32 tmpFlagNum2 = atoi(arg1);
				if (tmpFlagNum1 >= (sizeof(other->flag) / sizeof(other->flag[0])) || tmpFlagNum2 >= (sizeof(other->flag) / sizeof(other->flag[0]))) {
					if (mob->CastToClient()->flag[tmpFlagNum1] != 0)
						mob->CastToClient()->flag[tmpFlagNum2] = 0;
				}
				else {
					// Quag: TODO: Script error here, handle it somehow?
				}
				// Quag: Orignal code, not sure how this is supposed to work
//				if (mob->CastToClient()->flag[atoi(arglist[1])] != 0)
//					mob->CastToClient()->flag[atoi(arg1)] = 0;
			}
			//else if (strstr(strlwr(arglist[0]),"faction")) {
			//    if (mob && mob->IsClient()) {
			//	int32 faction_id = atoi(   )
			//	cout<<"Faction("<<faction_id<<","<<faction_value<<")"<<endl;
			//	//mob->CastToClient()->SetFactionLevel2(mob->GetID(), ), mob->GetClass(), mob->GetRace(), mob->GetDeity(), atoi(arglist[2]));
			//    }
			//}
			else {
				char tmp[100];
				memset(tmp,0x0,100);
				strn0cpy(tmp,strlwr(arglist[0]), sizeof(tmp));
				if (tmp[0]) {
					MakeParms(arglist[1],npcid);
					int re = CheckAliases(tmp,npcid,other,mob->CastToClient());
					if (!re)
					printf("Unrecognized command: %s\n", tmp);
				}
			}
			numargs = 0;
		}
    }
}

void Parser::ClearEventsByNPCID(int32 iNPCID) {
	MyListItem<Events>* Ptr = EventList.First;
	MyListItem<Events>* next = 0;
	while (Ptr) {
		next = Ptr->Next;
		if ( (int32)Ptr->Data->npcid == iNPCID) {
			EventList.DeleteItemAndData(Ptr);
		}
		Ptr = next;
	}
}

void Parser::ClearAliasesByNPCID(int32 iNPCID) {
	MyListItem<Alias>* Ptr = AliasList.First;
	MyListItem<Alias>* next = 0;
	while (Ptr) {
		next = Ptr->Next;
		if ( (int32)Ptr->Data->npcid == iNPCID) {
			AliasList.DeleteItemAndData(Ptr);
		}
		Ptr = next;
	}
}

void Parser::LoadScript(int npcid, const char *zone) {
#if Parser_DEBUG >= 4
	cout << "Parser::LoadScript(" << npcid << ", '" << zone << "') LoadAttempted(): " << LoadAttempted(npcid) << endl;
#endif
	if (LoadAttempted(npcid))
		return;
	SetNPCqstID(npcid, npcid);
	ClearEventsByNPCID(npcid);
	ClearAliasesByNPCID(npcid);
	FILE *input;
	char *buffer;

	int bracket = 0;
	int size = 0;
	int comment = 0;
	int quote = 0;
	//int ignore = 0;
	int line_num = 0;
	char temp[2048];
	char finish[2048];
	char t_finish[2048];

	int p = 0;

	char strnpcid[25] = "default";
	if (npcid)
		snprintf(strnpcid, sizeof(strnpcid), "%u", npcid);

	char filename[100];
	memset(filename, 0, sizeof(filename));
	snprintf(filename, sizeof(filename),"quests/%s/%s.qst", zone, strnpcid);
	#if Parser_DEBUG >= 8
		cout << "Parser::LoadScript(" << npcid << ", '" << zone << "') Attempting: " << filename << endl;
	#endif
	input = fopen(filename,"rb");
	if (input == NULL) {
		memset(filename, 0, sizeof(filename));
		snprintf(filename, sizeof(filename), "quests/all/%s.qst", strnpcid);
		#if Parser_DEBUG >= 8
			cout << "Parser::LoadScript(" << npcid << ", '" << zone << "') Attempting: " << filename << endl;
		#endif
		input = fopen(filename,"rb");
		if (input == NULL) {
		    memset(filename, 0, sizeof(filename));
		    snprintf(filename, sizeof(filename), "quests/%s/all.qst", zone);
		    #if Parser_DEBUG >= 8
			cout << "Parser::LoadScript(" << npcid << ", '" << zone << "') Attempting: " << filename << endl;
		    #endif
		    input = fopen(filename,"rb");
		    if (input == NULL) {

			if (npcid) {
				SetNPCqstID(npcid, 0);
				LoadScript(0, zone);
			}
			else
				SetNPCqstID(0, 0xFFFFFFFE);
			#if Parser_DEBUG >= 3
				cout << "Parser::LoadScript(" << npcid << ", '" << zone << "') No Scripts found, abandoning. " << endl;
			#endif
			return;
		    }
		    #if Parser_DEBUG >= 3
		    else {
			cout << "Parser::LoadScript(" << npcid << ", '" << zone << "') Loaded: " << filename << endl;
		    }
		    #endif
		}
		#if Parser_DEBUG >= 3
		else {
		    cout << "Parser::LoadScript(" << npcid << ", '" << zone << "') Loaded: " << filename << endl;
		}
		#endif
	}
	#if Parser_DEBUG >= 3
		else {
			cout << "Parser::LoadScript(" << npcid << ", '" << zone << "') Loaded: " << filename << endl;
		}
	#endif
	
	Events* NewEventList = new Events;
	Alias* Aliases = new Alias;
	fseek(input, 0, SEEK_END);
    size = ftell(input);
	rewind(input);
	buffer = new char[size];
	memset(buffer, 0, size);
	fread(buffer, 1, size, input);

	NewEventList->index = -1;
	NewEventList->npcid = npcid;
	Aliases->index = -1;
	Aliases->npcid = npcid;
	memset(finish, 0, sizeof(finish));
	memset(t_finish, 0, sizeof(t_finish));
	memset(temp, 0, sizeof(temp));
	int alias=0;
	int event=0;
#ifdef GUILDWARS
	strn0cpy(com_list, "if 0|break 1|while 0|spawn 6|dbspawnadd 2|echo 1|summonitem 1|castspell 2|say 1|emote 1|shout 1|depop 1|cumflag 1|flagclient 2|exp 1|level 1|safemove 1|rain 1|snow 1|givecash 4|pvp 1|doanim 1|addskill 2|flagcheck 1|me 1|write 2|settarget 2|follow 1|sfollow 0|save 0|setallskill 1|attack 1|cityfaction 1|report 0|movepc 4|", sizeof(com_list));
#else
	strn0cpy(com_list, "if 0|break 1|while 0|spawn 6|dbspawnadd 2|echo 1|summonitem 1|castspell 2|say 1|emote 1|shout 1|depop 1|cumflag 1|flagclient 2|exp 1|level 1|safemove 1|rain 1|snow 1|givecash 4|pvp 1|doanim 1|addskill 2|flagcheck 1|me 1|write 2|settarget 2|follow 1|sfollow 0|save 0|setallskill 1|attack 1|movepc 4|", sizeof(com_list));
#endif
#ifdef GUILDWARS2
	strn0cpy(com_list, "if 0|break 1|while 0|spawn 6|dbspawnadd 2|echo 1|summonitem 1|castspell 2|say 1|emote 1|shout 1|depop 1|cumflag 1|flagclient 2|exp 1|level 1|safemove 1|rain 1|snow 1|givecash 4|pvp 1|doanim 1|addskill 2|flagcheck 1|me 1|write 2|settarget 2|follow 1|sfollow 0|save 0|setallskill 1|attack 1|cityfaction 1|report 0|movepc 4|giveguildfunds 1|sayavailablefunds 0|", sizeof(com_list));
#endif
	for (int i=0; i < size; i++) {
		if (p>2048) {
		    LogFile->write(EQEMuLog::Error,"Script error %i", npcid);
		    break;
		}
		if (buffer[i] == '\n') line_num++;
		if (buffer[i] == '/' && buffer[i+1] == '*') { comment = 1; } 
		if ((strrchr(charIn,buffer[i]) || quote) && !comment) {
			temp[p] = buffer[i];
			p++;
		}
		if (buffer[i] == '/' && buffer[i-1] == '*') { comment = 0; }
		if (buffer[i] == '\"') {
			if (!quote)
				quote = 1;
			else
				quote = 0;
		}
		if (buffer[i] == '{' && !quote) {
			if (strstr(strlwr(temp),"event")) {
				NewEventList->index += 1;
#if Parser_DEBUG >= 9
				printf("New Event: %s - Index: %d\n", temp, NewEventList->index);
#endif
				strn0cpy(NewEventList->event[NewEventList->index], temp, sizeof(NewEventList->event[0]));
				memset(temp, 0, sizeof(temp));
				p = 0;
				event = 1;
				alias = 0;
			}
			else if (strstr(strlwr(temp),"function"))
			{
//				strcpy(tmp1,gettok(temp,' ',1));
				Replace(temp,"function","");
				Aliases->index += 1;
#if Parser_DEBUG >= 9
				printf("New Alias: %s - Index: %d\n", temp, Aliases->index);
#endif
				strn0cpy(Aliases->name[Aliases->index],temp, sizeof(Aliases->name[0]));
				memset(temp,0x0,sizeof(temp));
				p = 0;
				alias=1;
				event=0;
			}
			bracket++;
		}
		if (buffer[i] == '}' && !quote) {
			bracket--;
			if (bracket == 1) {
				if (strstr(finish,"ÿ")){
					snprintf(t_finish, sizeof(t_finish), "%s%sÿ", finish, temp);
					snprintf(finish, sizeof(finish), "%s", t_finish);
				}
				else {
					snprintf(t_finish, sizeof(finish), "%sÿ", temp);
					snprintf(finish, sizeof(finish), "%s", t_finish);
				}
				memset(temp, 0, sizeof(temp));
				p = 0;
			}
			if (bracket == -1) {
				printf("[Error] Parser::LoadScript() - Missing \"}\" around \"%s\"\n",temp);
				return;
			}
			else if (bracket == 0) {
				if (temp[0] != 0) {
					snprintf(finish, sizeof(finish), "%s%sÿ", finish, temp);
#if Parser_DEBUG >= 9
					printf("Finishadd: %s\n", temp);
#endif
					memset(temp, 0, sizeof(temp));
					p = 0;
				}
#if Parser_DEBUG >= 9
				printf("New Command: %s - Index: %d\n", finish, (event ? NewEventList->index : alias ? Aliases->index :0));
#endif
				if (event) {
				strn0cpy(NewEventList->command[NewEventList->index], finish, sizeof(NewEventList->command[0]));
				NewEventList->line[NewEventList->index] = line_num;
				memset(finish, 0, sizeof(temp));
				}
				else {
					snprintf(com_list, sizeof(com_list), "%s%s 1|",com_list,Aliases->name);
				strn0cpy(Aliases->command[Aliases->index], finish, sizeof(Aliases->command[0]));
				memset(finish, 0, sizeof(temp));
				}
			}
		}
	}
	if (quote) {
		printf("Parser::LoadScript() Error, runaway quote\n");
		return;
	}
	EventList.AddItem(NewEventList);
	AliasList.AddItem(Aliases);
	fclose(input);
	safe_delete(buffer);
}


void Parser::Replace(char * string, const char * repstr, const char * rep, int all) {
	int32 templen = strlen(string)+1;
	char *temp, *tempdel = temp = new char[templen];
	memset(temp, 0, templen);
	char* tmpposition = 0;
	while ((tmpposition = strstr(string, repstr)) != NULL) {
		temp = tempdel;
		strn0cpy(temp, tmpposition, templen);
		temp += strlen(repstr);
		tmpposition[0] = 0;
		char* temp2 = strcpy(new char[strlen(string)+1], string);
		sprintf(string,"%s%s%s", temp2, rep, temp);
		delete temp2;
		if (!all)
			break;
	}
	safe_delete(tempdel);
}


char* Parser::GetVar(char * varname, int32 npcid)
{
	MyListItem <vars> * Ptr = varlist.First;
	static char temp[100];
	char temp2[100];
	memset(temp,0x0,100); memset(temp2,0x0,100);

	snprintf(temp, sizeof(temp), "%s.%d",varname,npcid); snprintf(temp2, sizeof(temp2), "%s.g",varname);
	while (Ptr) {
		if (!strcmp(Ptr->Data->name,temp) || !strcmp(Ptr->Data->name,temp2)) {
			return Ptr->Data->value;
		}
		Ptr = Ptr->Next;
	}
	memset(temp,0x0,100);
	strcpy(temp,"NULL");
	return temp;
}

void Parser::DeleteVar(const char *name)
{
	MyListItem <vars> * Ptr = varlist.First;
	while (Ptr) {
		if (!strcmp(Ptr->Data->name,name)) {
			varlist.DeleteItem(Ptr);
			break;
		}
		Ptr = Ptr->Next;
	}
}

void Parser::DelChatAndItemVars(int32 npcid)
{
	MyListItem <vars> * Ptr;
	char temp[100];
	memset(temp,0x0,100);
	int i=0;
	for (i=0;i<10;i++)
	{
		sprintf(temp,"%d.%d",i,npcid);
		DeleteVar(temp);
		memset(temp,0x0,100);
		sprintf(temp,"%d-.%d",i,npcid);
		DeleteVar(temp);
		memset(temp,0x0,100);
	}
	for (i=1;i<5;i++)
	{
		sprintf(temp,"item%d.%d",i,npcid);
		DeleteVar(temp);
		memset(temp,0x0,100);
		sprintf(temp,"item%d.stack.%d",i,npcid);
		DeleteVar(temp);
		memset(temp,0x0,100);
	}
}

void Parser::AddVar(const char * varname, const char * varval)
{
	MyListItem <vars> * Ptr = varlist.First;
	while (Ptr) {
		if (!strcmp(Ptr->Data->name,varname)) {
			strn0cpy(Ptr->Data->value, varval, Parser_MaxVars);
			return;
		}
		Ptr = Ptr->Next;
	}
	vars * newvar = new vars;
	strn0cpy(newvar->name, varname, Parser_MaxVars);
	strn0cpy(newvar->value, varval, Parser_MaxVars);
	varlist.AddItem(newvar);
}

void Parser::HandleVars(char * varname, char * varparms, char * origstring, char * format, int32 npcid, Mob* mob)
{
#if Parser_DEBUG >= 9
	printf("Debug HandleVars: %s\n", mob->GetName());
#endif
	char arglist[10][1024];
	memset(arglist, 0,sizeof(arglist));
	char varstat[200];
	memset(varstat, 0, sizeof(varstat));

	int numargs = 0;
	if (varparms) {
		numargs = numtok(varparms,",") + 1;

		if (numargs > 1) {
			for (int h=0; h < numargs; h++)
				snprintf(varstat, sizeof(varstat), "%sÿ,",varstat);
		}
		else {
			snprintf(varstat, sizeof(varstat), "%s",varparms);
		}

		scanformat(varparms,varstat,arglist);
#if Parser_DEBUG >= 9
		printf("Test: %s\n", arglist[0]);
		printf("Varparms: %s\n", varparms);
#endif
	}

	if (!strcmp(varname,"mid")) {
	int pos = 0;
	char * temp = new char[1024];
	memset(temp,0x0,1024);
	char temp1[1024];
	memset(temp1,0x0,1024);
	strn0cpy(temp,arglist[0], 1024);
	int ind=0;
	int index = atoi(arglist[1]);
	int index2 = atoi(arglist[2]);
	while (*temp)
	{
		ind++;
		if (ind >=index)
			temp1[pos++] = *temp++;
		else temp++;
		if (pos == index2) break;
	}
	Replace(origstring,format,temp1);
	}
	else if (!strcmp(varname,"+")) {
		int32 templen = strlen(format)+10;
		char* temp = new char[templen];
		memset(temp,0x0,sizeof(temp));
		snprintf(temp, templen," %s ", format);
		Replace(origstring, temp, " REPLACETHISSHIT ",1);
		safe_delete(temp);
	}
#ifdef GUILDWARS
	else if (!strcmp(varname,"guildnpcplaceable"))
	{
		char temp[6];
		memset(temp, 0, sizeof(temp));
		if(entity_list.GuildNPCPlaceable(mob))
		Replace(origstring,format,"1");
		else
		Replace(origstring,format,"0");
	}
#endif
	else if (!strcmp(varname,"replace")) {
		char temp[1024];
		memset(temp, 0, sizeof(temp));
		strn0cpy(temp, arglist[0], sizeof(temp));
		Replace(temp, arglist[1], arglist[2],1);
		Replace(origstring, format, temp);
	}
	else if (!strcmp(varname,"itemcount")) {
		char temp[10];
		int o=0;
		o = GetItemCount(varparms,npcid);
		sprintf(temp,"%s",itoa(o,temp,10));
		Replace(origstring,format,temp,1);
	}
	else if (!strcmp(varname,"calc")) {
		char temp[100];
		memset(temp, 0, sizeof(temp));
		Replace(origstring,format,itoa(pcalc(varparms),temp,10));
#if Parser_DEBUG >= 9
		printf("New string calc: %s\n", origstring);
#endif
	}
	else if (!strcmp(varname,"status") && mob->IsClient()) {
		char temp[6];
		memset(temp, 0, sizeof(temp));
		Replace(origstring,format,itoa(mob->CastToClient()->Admin(),temp,10));
	}
	else if (!strcmp(varname,"hasitem") && mob->IsClient()) {
		int has=0;
		for (int i=0; i<=30;i++) {
			if (mob->CastToClient()->GetItemAt(i) == atoi(varparms)) {
				Replace(origstring,format,"true");
				has = 1;
				break;
			}
		}
		if (!has)
			Replace(origstring,format,"false");
	}
	else if (!strcmp(varname,"read")) {
  FILE * pFile;
  long lSize;
  char * buffer;
  pFile = fopen ( arglist[0] , "rb" );
  if (pFile==NULL) return;
  fseek (pFile , 0 , SEEK_END);
  lSize = ftell (pFile);
  rewind (pFile);
  buffer = new char[lSize];
  memset(buffer,0x0,sizeof(buffer));
  if (buffer == NULL) return;
  fread (buffer,1,lSize,pFile);
  fclose (pFile);
  char temp[1024];
  memset(temp,0x0,1024);
  int i=0;
  int num=0;
  if (!strchr(buffer,10)) {
	  for (int a=0;a<strlen(buffer);a++)
		  if (!strchr(charIn2,buffer[a])) buffer[a] = '\0';
  }
  else {
  while (*buffer)
  {
	  if (*buffer != 10 && *buffer != 13) temp[i++] = *buffer++;
	  else {
		  if (*buffer == 10) {
		  num++;
		  if (num >= atoi(arglist[1])) { break; }
		  else { memset(temp,0x0,1024);i=0; }
		  }
		  buffer++;
	  }
  }
  }
  if (!temp)
	  strn0cpy(temp, buffer, sizeof(temp));
			Replace(origstring,format,temp);
	}
	else if (!strcmp(varname,"npc_status")) {
		Mob * tmp;
		if (!atoi(varparms)) {
			tmp = entity_list.GetMob(varparms);
		}
		else {
			tmp = entity_list.GetMobByNpcTypeID(atoi(varparms));
		}
		if (tmp && tmp->GetHP() > 0) Replace(origstring,format,"up");
		else Replace(origstring,format,"down");
	}
	else if (!strcmp(varname,"strlen")) {
		char temp[6];
		memset(temp, 0, sizeof(temp));
		Replace(origstring,format,itoa(strlen(varparms),temp,10));
	}
	else if (!strcmp(varname,"chr")) {
		char temp[4];
		memset(temp, 0x0, 4);
		temp[0] = atoi(varparms);
		Replace(origstring,format,temp);
	}
	else if (!strcmp(varname,"asc")) {
		char temp[10];
		Replace(origstring,format,itoa(varparms[0],temp,10));
	}
	else if (!strcmp(varname,"gettok")) {
		Replace(origstring,format,gettok(arglist[0],atoi(arglist[1]),atoi(arglist[2])));
	}
	else {
		char temp[1024];
		memset(temp,0x0,1024);
		strn0cpy(temp, GetVar(varname,npcid), sizeof(temp));
		Replace(origstring,format,temp);
	}
	gClient = 0;
}

char * Parser::ParseVars(char * string, int32 npcid, Mob* mob)
{
	//printf("Debug ParseVars: %s\n", mob->GetName());
	if (!strstr(string,"$") && !strstr(string,"%"))
		return string;
	char * pch;
	char buffer[500];
	char buffer2[500];
	char varname[20];
	char params[500];
	char eh[500];
	char eh1[500];
	int scalar = 0;
	int paren = 0;
	int pos = 0;
	int pos2 = 0;
	int record = 0;
	memset(buffer, 0, sizeof(buffer));
	memset(buffer2, 0, sizeof(buffer2));
	memset(varname, 0, sizeof(varname));
	memset(params, 0, sizeof(params));
	memset(eh, 0, sizeof(eh));
	memset(eh1, 0, sizeof(eh1));

#if Parser_DEBUG >= 9
	printf("About to parse: %s\n", string);
#endif

	while((pch = strrchr(string,'%'))) {
//		if (pch == NULL)
//			return string;

		for (unsigned int i=0; i<strlen(pch);i++) {
			eh[pos2] = pch[i];
			pos2++;
			if (scalar && !strrchr(notin,pch[i])) {
				varname[pos] = pch[i];
				pos++;
			}

			if (record == 1)
			{
				buffer[pos] = pch[i];
				pos++;
			}

			if (pch[i] == '(')
			{
				scalar = 0;
				paren = 1;
				pos = 0;
				record = 1;
			}
			if (pch[i] == ')')
			{
				paren--;
				if (paren == 0 || paren == -1)
				{
					if (eh[strlen(eh) - 2] != '(')
						eh[strlen(eh) - 1] = '\0';
//					sprintf(varname,"%s.%d",varname,npcid);
#if Parser_DEBUG >= 9
					printf("Varname: %s - Parms: %s - Format: %s\n", varname, buffer, eh);
#endif
					HandleVars(varname, buffer, string, eh, npcid,mob);
					memset(varname, 0, sizeof(varname));
					memset(buffer, 0, sizeof(buffer));
					memset(eh, 0, sizeof(eh));
					pos = 0;
					pos2 = 0;
					scalar = 0;
					paren = 0;
					break;
				}
			}
			else {
				if (strrchr(notin,pch[i]) && paren == 0) {
					eh[strlen(eh) - 1] = '\0';
				//	sprintf(varname,"%s.%d",varname,npcid);
#if Parser_DEBUG >= 9
					printf("Varname: %s - Format: %s\n", varname, eh);
#endif
					HandleVars(varname, 0, string, eh, npcid,mob);
					memset(varname, 0, sizeof(varname));
					memset(buffer, 0, sizeof(buffer));
					memset(eh, 0, sizeof(eh));
					pos = 0;
					pos2 = 0;
					scalar = 0;
					break;
				}
			}
			if (pch[i] == '%')
				scalar = 1;
		}
	}

	scalar = 0;
	paren = 0;
	pos = 0;
	pos2 = 0;
	record = 0;
	memset(buffer, 0, sizeof(buffer));
	memset(buffer2, 0, sizeof(buffer2));
	memset(varname, 0, sizeof(varname));
	memset(params, 0, sizeof(params));
	memset(eh, 0, sizeof(eh));
	memset(eh1, 0, sizeof(eh1));


	while(strrchr(string,'$')) {
		pch = strrchr(string,'$');
		if (pch == NULL) return string;

		for (unsigned int i=0; i<strlen(pch);i++) {
			eh[pos2] = pch[i];
			pos2++;
			if (scalar && !strrchr(notin,pch[i])) {
				varname[pos] = pch[i];
				pos++;
			}

			if (record == 1 && pch[i] != ')')
			{
				buffer[pos] = pch[i];
				pos++;
			}

			if (pch[i] == '(')
			{
				scalar = 0;
				paren = 1;
				pos = 0;
				record = 1;
			}
			if (pch[i] == ')')
			{
#if Parser_DEBUG >= 9
				printf("test Varname: %s - Parms: %s - Format: %s test too %d\n", varname, buffer, eh, paren);
#endif
				paren--;
				if (paren == 0 || paren == -1)
				{
					if (eh[strlen(eh) - 2] != '(' && paren == -1)
						eh[strlen(eh) - 1] = '\0';
					//sprintf(varname,"%s.%d",varname,npcid);
#if Parser_DEBUG >= 9
					printf("Varname: %s - Parms: %s - Format: %s\n", varname, buffer, eh);
#endif
					HandleVars(varname, buffer, string, eh, npcid,mob);
					memset(varname, 0, sizeof(varname));
					memset(buffer, 0, sizeof(buffer));
					memset(eh, 0, sizeof(eh));
					pos = 0;
					pos2 = 0;
					scalar = 0;
					paren = 0;
					record = 0;
					break;
				}
			}
			else {
				if (strrchr(notin,pch[i]) && paren == 0) {
					eh[strlen(eh) - 1] = '\0';
					//sprintf(varname,"%s.%d",varname,npcid);
#if Parser_DEBUG >= 9
					printf("Varname: %s - Format: %s\n", varname, eh);
#endif
					HandleVars(varname, 0, string, eh, npcid,mob);
					memset(varname, 0, sizeof(varname));
					memset(buffer, 0, sizeof(buffer));
					memset(eh, 0, sizeof(eh));
					pos = 0;
					pos2 = 0;
					scalar = 0;
					record = 0;
					break;
				}
			}
			if (pch[i] == '$')
				scalar = 1;
		}
	}
	Replace(string," REPLACETHISSHIT ","");
	return 0;
}	

int Parser::ParseIf(char * string) {
	Replace(string,"\"","",1);
	int equal = 0;
	int nequal = 0;
	int por = 0;
	int pos = 0;
	//int pos2 = 0;
	//int pos3 = 0;
	char buffer[100];
	char buffer2[100];
	char buffer3[100];
	char condition[100];
	char condition2[100];
	memset(condition, 0, sizeof(condition));
	memset(condition2, 0, sizeof(condition2));
	memset(buffer, 0, sizeof(buffer));
	memset(buffer2, 0, sizeof(buffer2));
	memset(buffer3, 0, sizeof(buffer3));
	for (unsigned int i=0; i < strlen(string); i++)	{
		if (string[i] != '!' && string[i] != '|' && string[i] != '&' && string[i] != '=') {
			buffer[pos] = string[i];
			pos++;
		}
		if (string[i] == '=' && string[i+1] == '=') {
			condition[0] = string[i];
			condition[1] = string[i+1];
			equal = 1;
			i++;
			strn0cpy(buffer2, buffer, sizeof(buffer2));
			memset(buffer, 0, sizeof(buffer));
			pos = 0;
		}
		else if (string[i] == '=' && string[i+1] == '~') {
			condition[0] = string[i];
			condition[1] = string[i+1];
			equal = 1;
			i++;
			strn0cpy(buffer2, buffer, sizeof(buffer2));
			memset(buffer, 0, sizeof(buffer));
			pos = 0;
		}
		else if (string[i] == '!' && string[i+1] == '=') {
			condition[0] = string[i];
			condition[1] = string[i+1];
			nequal = 1;
			i++;
			strn0cpy(buffer2, buffer, sizeof(buffer2));
			memset(buffer, 0, sizeof(buffer));
			pos = 0;
		}
		else if (string[i] == '<' || string[i] == '>') {
			condition[0] = string[i];
			if (string[i+1] == '=') {
				condition[1] = string[i+1];
				i++;
			}

#if Parser_DEBUG >= 9
			printf("Test condition: %s\n", condition);
#endif
			strn0cpy(buffer2, buffer, sizeof(buffer2));
			memset(buffer, 0, sizeof(buffer));
			pos = 0;
		}
		else if (string[i] == '&' && string[i+1] == '&') {
			if (!strcmp(condition2,"||"))
				por = 1;
			condition2[0] = string[i];
			condition2[1] = string[i+1];
			strn0cpy(buffer3, buffer, sizeof(buffer3));
			if (!strcmp(condition,"==")) {
				if (strcmp(strlwr(buffer2),strlwr(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,"!=")) {
				if (!strcmp(strlwr(buffer2),strlwr(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,"=~")) {
				if (!strstr(strlwr(buffer2),strlwr(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,"<")) {
				if ((atoi(buffer2) > atoi(buffer3) || atoi(buffer2) == atoi(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,">")) {
				if ((atoi(buffer2) < atoi(buffer3) || atoi(buffer2) == atoi(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,"<=")) {
				if ((atoi(buffer2) >= atoi(buffer3) && atoi(buffer2) != atoi(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,">=")) {
				if ((atoi(buffer2) <= atoi(buffer3) && atoi(buffer2) != atoi(buffer3)) && !por)
					return 0;
			}
			memset(buffer, 0, sizeof(buffer));
			memset(buffer2, 0, sizeof(buffer2));
			memset(buffer3, 0, sizeof(buffer3));
			memset(condition, 0, sizeof(condition));
			pos = 0;
			if (por)
				por = 0;
			i++;
		}
		else if (string[i] == '|' && string[i+1] == '|') {
			strn0cpy(buffer3, buffer, sizeof(buffer3));
			if (!strcmp(condition2,"||") || condition2[0] == 0)
				por = 1;
			condition2[0] = string[i];
			condition2[1] = string[i+1];
			if (!strcmp(condition,"==")) {
				if (strcmp(strlwr(buffer2),strlwr(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,"!=")) {
				if (!strcmp(strlwr(buffer2),strlwr(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,"=~")) {
				if (!strstr(strlwr(buffer2),strlwr(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,"<")) {
				if ((atoi(buffer2) > atoi(buffer3) || atoi(buffer2) == atoi(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,">")) {
				if ((atoi(buffer2) < atoi(buffer3) || atoi(buffer2) == atoi(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,"<=")) {
				if ((atoi(buffer2) >= atoi(buffer3) && atoi(buffer2) != atoi(buffer3)) && !por)
					return 0;
			}
			else if (!strcmp(condition,">=")) {
				if ((atoi(buffer2) <= atoi(buffer3) && atoi(buffer2) != atoi(buffer3)) && !por)
					return 0;
			}
			memset(condition, 0, sizeof(condition));
			memset(buffer, 0, sizeof(buffer));
			memset(buffer2, 0, sizeof(buffer2));
			memset(buffer3, 0, sizeof(buffer3));
			pos = 0;
			por = 0;
			i++;
		}
		else if (i == (strlen(string) - 1)) {
			strncpy(buffer3,buffer, sizeof(buffer3));
			if (!strcmp(condition2,"||"))
				por = 1;
			if (!strcmp(condition,"==")) {
				if (strcmp(strlwr(buffer2),strlwr(buffer3)))
					return 0;
			}
			else if (!strcmp(condition,"!=")) {
				if (!strcmp(strlwr(buffer2),strlwr(buffer3)))
					return 0;
			}
			else if (!strcmp(condition,"=~")) {
				if (!strstr(strlwr(buffer2),strlwr(buffer3)))
					return 0;
			}
			else if (!strcmp(condition,"<")) {
				if ((atoi(buffer2) > atoi(buffer3) || atoi(buffer2) == atoi(buffer3)))
					return 0;
			}
			else if (!strcmp(condition,">")) {
				if ((atoi(buffer2) < atoi(buffer3) || atoi(buffer2) == atoi(buffer3)))
					return 0;
			}
			else if (!strcmp(condition,"<=")) {
				if ((atoi(buffer2) >= atoi(buffer3) && atoi(buffer2) != atoi(buffer3)))
					return 0;
			}
			else if (!strcmp(condition,">=")) {
				if ((atoi(buffer2) <= atoi(buffer3) && atoi(buffer2) != atoi(buffer3)))
					return 0;
			}
		}
	}
	return 1;
}
