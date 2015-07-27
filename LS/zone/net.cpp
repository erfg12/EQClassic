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
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <fstream.h>
#ifdef WIN32
#include <conio.h>
#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#endif

#include "../common/queue.h"
#include "../common/timer.h"
#include "../common/EQNetwork.h"
#include "../common/eq_packet_structs.h"
#include "../common/Mutex.h"
#include "../common/version.h"
#include "../common/files.h"
#include "../common/EQEMuError.h"

#include "net.h"
#include "client.h"
#include "zone.h"
#include "worldserver.h"
#include "spdat.h"
#include "../common/packet_dump_file.h"
#include "parser.h"

NetConnection		net;
EntityList			entity_list;
WorldServer			worldserver;
int32				numclients = 0;
#ifdef CATCH_CRASH
int8			error = 0;
#endif
GuildRanks_Struct	guilds[512];
char errorname[32];
int16 adverrornum = 0;
extern Zone* zone;
extern volatile bool ZoneLoaded;
EQNetworkServer eqns;
npcDecayTimes_Struct npcCorpseDecayTimes[100];
volatile bool RunLoops = true;
bool zoneprocess;
#ifdef SHAREMEM
	#include "../common/EMuShareMem.h"
	extern LoadEMuShareMemDLL EMuShareMemDLL;
    #ifndef WIN32
	#include <sys/types.h>
	#include <sys/ipc.h>
	#include <sys/sem.h>
	#include <sys/shm.h>
	union semun {
	    int val;
	    struct semid_ds *buf;
	    ushort *array;
	    struct seminfo *__buf;
	    void *__pad;
	};	  
    #endif
#endif

#ifdef NEW_LoadSPDat
	// For NewLoadSPDat function
	const SPDat_Spell_Struct* spells; 
	SPDat_Spell_Struct* spells_delete; 
	sint32 GetMaxSpellID();


	void LoadSPDat();
	bool FileLoadSPDat(SPDat_Spell_Struct* sp, sint32 iMaxSpellID);
	sint32 SPDAT_RECORDS = -1;
#else
	#define SPDat_Location	"spdat.eff"
	SPDat_Spell_Struct spells[SPDAT_RECORDS];
	void LoadSPDat(SPDat_Spell_Struct** SpellsPointer = 0);


#endif

bool spells_loaded = false;

Database database;

#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#include "../common/unix.h"
#endif

void Shutdown();

//bool ZoneBootup(int32 iZoneID, bool iStaticZone = false);
char *strsep(char **stringp, const char *delim);

#ifdef ADDONCMD
#include "addoncmd.h"
extern  AddonCmd addonCmd;
#endif

int main(int argc, char** argv) {
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//	_crtBreakAlloc = 2025;
#endif
	srand(time(NULL));
  
    for (int logs = 0; logs < EQEMuLog::MaxLogID; logs++) {
      LogFile->write((EQEMuLog::LogIDs)logs, "CURRENT_ZONE_VERSION: %s", CURRENT_ZONE_VERSION);
    }

	if (argc != 5)
	{
		cerr << "Usage: zone zone_name address port worldaddress" << endl;
		exit(0);
	}
	char* filename = argv[0];
	char* zone_name = argv[1];
	char* address = argv[2];
	int32 port = atoi(argv[3]);
	char* worldaddress = argv[4];
	
	if (strlen(address) <= 0) {
		cerr << "Invalid address" << endl;

		exit(0);
	}
	if (port <= 0) {
		cerr << "Bad port specified" << endl;
		exit(0);
	}
	if (strlen(worldaddress) <= 0) {
		cerr << "Invalid worldaddress" << endl;
		exit(0);
	}
	if (signal(SIGINT, CatchSignal) == SIG_ERR) {
		cerr << "Could not set signal handler" << endl;
		return 0;
	}
#ifndef WIN32
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		cerr << "Could not set signal handler" << endl;
		return 0;
	}
#endif
	net.SaveInfo(address, port, worldaddress,filename);
	
	LogFile->write(EQEMuLog::Status, "Loading Variables");
	database.LoadVariables();
	LogFile->write(EQEMuLog::Status, "Loading zone names");
	database.LoadZoneNames();
	LogFile->write(EQEMuLog::Status, "Loading items");
	if (!database.LoadItems()) {
		LogFile->write(EQEMuLog::Error, "Loading items FAILED!");
		cout << "Failed.  But ignoring error and going on..." << endl;
	}
	LogFile->write(EQEMuLog::Status, "Loading npcs");
	if (!database.LoadNPCTypes()) {
		LogFile->write(EQEMuLog::Error, "Loading npcs FAILED!");
		CheckEQEMuErrorAndPause();
		return 0;
	}
#ifdef SHAREMEM
	LogFile->write(EQEMuLog::Status, "Loading npc faction lists");
	if (!database.LoadNPCFactionLists()) {
		LogFile->write(EQEMuLog::Error, "Loading npcs faction lists FAILED!");
		CheckEQEMuErrorAndPause();
		return 0;
	}
#endif
	LogFile->write(EQEMuLog::Status, "Loading loot tables");
	if (!database.LoadLoot()) {
		LogFile->write(EQEMuLog::Error, "Loading loot FAILED!");
		CheckEQEMuErrorAndPause();
		return 0;
	}
#ifdef SHAREMEM
	LogFile->write(EQEMuLog::Status, "Loading doors");
	database.LoadDoors();
#endif
	LoadSPDat();

#ifdef GUILDWARS
	LogFile->write(EQEMuLog::Status, "Loading guild alliances");
	//database.LoadGuildAlliances();
#endif

	// New Load function.  keeping it commented till I figure out why its not working correctly in linux. Trump.
	// NewLoadSPDat();
	LogFile->write(EQEMuLog::Status, "Loading guilds");
	database.LoadGuilds(guilds);
	LogFile->write(EQEMuLog::Status, "Loading factions");
	database.LoadFactionData();
	LogFile->write(EQEMuLog::Status, "Loading corpse timers");
	database.GetDecayTimes(npcCorpseDecayTimes);
	LogFile->write(EQEMuLog::Status, "Loading what ever is left");
	database.ExtraOptions();
	AutoDelete<Parser> ADparse(&parse, new Parser);
#ifdef ADDONCMD	
	LogFile->write(EQEMuLog::Status, "Looding addon commands from dll");
	if ( !addonCmd.openLib() ) {
		LogFile->write(EQEMuLog::Error, "Loading addons failed =(");
	}
#endif	
	if (!worldserver.Connect()) {
		LogFile->write(EQEMuLog::Error, "worldserver.Connect() FAILED!");
	}
	
	if (strcmp(zone_name, ".") == 0 || strcasecmp(zone_name, "sleep") == 0) {
		LogFile->write(EQEMuLog::Status, "Entering sleep mode");
	} else if (!Zone::Bootup(database.GetZoneID(zone_name), true)) {
		LogFile->write(EQEMuLog::Error, "Zone bootup FAILED!");
		zone = 0;
	}
	
	Timer InterserverTimer(INTERSERVER_TIMER); // does MySQL pings and auto-reconnect
	UpdateWindowTitle();
	bool worldwasconnected = worldserver.Connected();
	EQNetworkConnection* eqnc;
	while(RunLoops) {
		Timer::SetCurrentTime();
		while ((eqnc = eqns.NewQueuePop())) {
			struct in_addr	in;
			in.s_addr = eqnc->GetrIP();
			LogFile->write(EQEMuLog::Status, "%i New client from ip:%s port:%i", Timer::GetCurrentTime(), inet_ntoa(in), ntohs(eqnc->GetrPort()));
			Client* client = new Client(eqnc);
			entity_list.AddClient(client);
		}
#ifdef CATCH_CRASH
		try{
#endif
			worldserver.Process();
#ifdef CATCH_CRASH
		}
		catch(...){
			error = 1;
			adverrornum = worldserver.GetErrorNumber();
			worldserver.Disconnect();
			worldwasconnected = false;
		}
#endif			
		if (worldserver.Connected()) {
			worldwasconnected = true;
		}
		else {
			if (worldwasconnected && ZoneLoaded)
				entity_list.ChannelMessageFromWorld(0, 0, 6, 0, 0, "WARNING: World server connection lost");
			worldwasconnected = false;
		}
		if (ZoneLoaded) {
			{	
#ifdef CATCH_CRASH
				try{
#endif
					entity_list.Process();
#ifdef CATCH_CRASH
				}
				catch(...){
					error = 4;
				}
				try{
#endif
					zoneprocess= zone->Process();
					if (!zoneprocess) {
						Zone::Shutdown();
					}
#ifdef CATCH_CRASH
				}
				catch(...){
					error = 2;
				}
#endif
			}
		}
		DBAsyncWork* dbaw = 0;
		while ((dbaw = MTdbafq->Pop())) {
			DispatchFinishedDBAsync(dbaw);
		}
		if (InterserverTimer.Check()
#ifdef CATCH_CRASH
			&& !error
#endif
			) {
#ifdef CATCH_CRASH
			try{
#endif
				InterserverTimer.Start();
				database.ping();
				AsyncLoadVariables();
//				NPC::GetAILevel(true);
				entity_list.UpdateWho();
				if (worldserver.TryReconnect() && (!worldserver.Connected()))
					worldserver.AsyncConnect();
#ifdef CATCH_CRASH
			}
			catch(...)
			{
				error = 16;
				RunLoops = false;
			}
#endif
		}
#ifdef CATCH_CRASH
		if (error){
			RunLoops = false;
		}
#endif
#if defined(_DEBUG) && defined(DEBUG_PC)
QueryPerformanceCounter(&tmp3);
mainloop_time += tmp3.QuadPart - tmp2.QuadPart;
if (!--tmp0) {
	tmp0 = 200;
	printf("Elapsed Tics  : %9.0f (%1.4f sec)\n", (double)mainloop_time, ((double)mainloop_time/tmp.QuadPart));
	printf("NPCAI Tics    : %9.0f (%1.2f%%)\n", (double)npcai_time, ((double)npcai_time/mainloop_time)*100);
	printf("FindSpell Tics: %9.0f (%1.2f%%)\n", (double)findspell_time, ((double)findspell_time/mainloop_time)*100);
	printf("AtkAllowd Tics: %9.0f (%1.2f%%)\n", (double)IsAttackAllowed_time, ((double)IsAttackAllowed_time/mainloop_time)*100);
	printf("ClientPro Tics: %9.0f (%1.2f%%)\n", (double)clientprocess_time, ((double)clientprocess_time/mainloop_time)*100);
	printf("ClientAtk Tics: %9.0f (%1.2f%%)\n", (double)clientattack_time, ((double)clientattack_time/mainloop_time)*100);
mainloop_time = 0;
npcai_time = 0;
findspell_time = 0;
IsAttackAllowed_time = 0;
clientprocess_time = 0;
clientattack_time = 0;
}
#endif
		Sleep(1);
	}
	
#ifdef CATCH_CRASH
	if (error)
		FilePrint("eqemudebug.log",true,true,"Zone %i crashed. Errorcode: %i/%i. Current zone loaded:%s. Current clients:%i. Caused by: %s",net.GetZonePort(), error,adverrornum, zone->GetShortName(), numclients,errorname);
	try{
		entity_list.Message(0, 15, "ZONEWIDE_MESSAGE: This zone caused a fatal error and will shut down now. Your character will be restored to the last saved status. We are sorry for any inconvenience!");
	}
	catch(...){}
	if (error){
#ifdef WIN32		
		ExitProcess(error);
#else	
		entity_list.Clear();
		safe_delete(zone);
#endif
	}
#endif

	entity_list.Clear();
	if (zone != 0
#ifdef CATCH_CRASH
		& !error
#endif
		)
		Zone::Shutdown(true);
	//Fix for Linux world server problem.
	eqns.Close();
	worldserver.Disconnect();
	dbasync->CommitWrites();
	dbasync->StopThread();
#ifdef NEW_LoadSPDat
	safe_delete(spells_delete);
#endif

	CheckEQEMuErrorAndPause();
	return 0;
}

void CatchSignal(int sig_num) {
	LogFile->write(EQEMuLog::Status, "Recieved signal:%i", sig_num);
	RunLoops = false;
}

void Shutdown()
{
	Zone::Shutdown(true);
	RunLoops = false;
	worldserver.Disconnect();
	//	safe_delete(worldserver);
	LogFile->write(EQEMuLog::Status, "Shutting down...");
}

int32 NetConnection::GetIP()
{
	char     name[255+1];
	size_t   len = 0;
	hostent* host = 0;
	
	if (gethostname(name, len) < 0 || len <= 0)
	{
		return 0;
	}
	
	host = gethostbyname(name);
	if (host == 0)
	{
		return 0;
	}
	
	return inet_addr(host->h_addr);
}

int32 NetConnection::GetIP(char* name)
{
	hostent* host = 0;
	
	host = gethostbyname(name);
	if (host == 0)
	{
		return 0;
	}
	
	return inet_addr(host->h_addr);
	
}

void NetConnection::SaveInfo(char* address, int32 port, char* waddress, char* filename) {

	ZoneAddress = new char[strlen(address)+1];
	strcpy(ZoneAddress, address);
	ZonePort = port;
	WorldAddress = new char[strlen(waddress)+1];
	strcpy(WorldAddress, waddress);
	strcpy(ZoneFileName, filename);
}

NetConnection::~NetConnection() {
	if (ZoneAddress != 0)
		delete ZoneAddress;
	if (WorldAddress != 0)
		delete WorldAddress;
}
bool chrcmpI(const char* a, const char* b) {
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug, "crhcmpl() a:%i b:%i", (int*) a, (int*) b);
#endif
	if(((int)* a)==((int)* b))
		return false;
	else
		return true;
}

#ifdef NEW_LoadSPDat
sint32 GetMaxSpellID() {
	int tempid=0, oldid=-1;
	char spell_line_start[2048];
	char* spell_line = spell_line_start;
	char token[64]="";
	char seps[] = "^";
#ifdef WIN32
	ifstream in(SPELLS_FILE,ios::nocreate);
#else
	ifstream in(SPELLS_FILE);
#endif


	if(!in) {
		LogFile->write(EQEMuLog::Error, "File '%s' not found, spell loading FAILED!", SPELLS_FILE);
		return -1;
	}

	in.getline(spell_line, sizeof(spell_line_start));
	while(strlen(spell_line)>1)
	{
		strcpy(token,strtok(spell_line, seps));
		if(token!=NULL);
		{
			tempid = atoi(token);
			if(tempid>oldid)
				oldid = tempid;
			else
				break;
		}
		in.getline(spell_line, sizeof(spell_line_start));
	}
		
	return oldid;
}
#ifdef SHAREMEM
extern "C" bool extFileLoadSPDat(void* sp, sint32 iMaxSpellID) { return FileLoadSPDat((SPDat_Spell_Struct*) sp, iMaxSpellID); }
#endif

void LoadSPDat() {
	if (SPDAT_RECORDS != -1) {
		LogFile->write(EQEMuLog::Debug, "LoadSPDat() SPDAT_RECORDS:%i != -1, spells already loaded?", SPDAT_RECORDS);
	}
	sint32 MaxSpellID = GetMaxSpellID();
	if (MaxSpellID == -1) {
		LogFile->write(EQEMuLog::Debug, "LoadSPDat() MaxSpellID == -1, %s missing?", SPELLS_FILE);
		return;
	}
#ifdef SHAREMEM
	if (!EMuShareMemDLL.Load())
		return;
	SPDAT_RECORDS = MaxSpellID+1;
	if (EMuShareMemDLL.Spells.DLLLoadSPDat(&extFileLoadSPDat, (const void**) &spells, &SPDAT_RECORDS, sizeof(SPDat_Spell_Struct))) {
		spells_loaded = true;
	}
	else {
		SPDAT_RECORDS = -1;
		LogFile->write(EQEMuLog::Error, "LoadSPDat() EMuShareMemDLL.Spells.DLLLoadSPDat() returned false");
		return;
	}
#else
	spells_delete = new SPDat_Spell_Struct[MaxSpellID+1];
	if (FileLoadSPDat(spells_delete, MaxSpellID)) {
		spells = spells_delete;
		SPDAT_RECORDS = MaxSpellID+1;
		spells_loaded = true;
	}
	else {
		safe_delete(spells_delete);
		LogFile->write(EQEMuLog::Error, "LoadSPDat() FileLoadSPDat() returned false");
		return;
	}
#endif
}

bool FileLoadSPDat(SPDat_Spell_Struct* sp, sint32 iMaxSpellID) {
	int tempid=0;
	int16 counter=0;
	char spell_line[2048];
	LogFile->write(EQEMuLog::Status,"FileLoadSPDat() Loading spells from %s", SPELLS_FILE);

#ifdef WIN32
	ifstream in(SPELLS_FILE,ios::nocreate);
#else
	ifstream in(SPELLS_FILE);
#endif

	if (iMaxSpellID < 0) {
		LogFile->write(EQEMuLog::Error,"FileLoadSPDat() Loading spells FAILED! iMaxSpellID:%i < 0", iMaxSpellID);
		return false;
	}
#if DEBUG >= 1
	else {
		LogFile->write(EQEMuLog::Debug,"FileLoadSPDat() Highest spell ID:%i", iMaxSpellID);
	}
#endif

	in.close();
#ifdef WIN32
	in.open(SPELLS_FILE, ios::nocreate);
#else
	in.open(SPELLS_FILE);
#endif

	if(!in) {
		LogFile->write(EQEMuLog::Error, "File '%s' not found, spell loading FAILED!", SPELLS_FILE);
		return false;
	}

	while(!in.eof()) {
		in.getline(spell_line, sizeof(spell_line));
		Seperator sep(spell_line, '^', 150, 75, false, 0, 0, false);

		if(spell_line[0]=='\0')
			break;

		tempid = atoi(sep.arg[0]);
		if (tempid > iMaxSpellID) {
			LogFile->write(EQEMuLog::Error, "FATAL FileLoadSPDat() tempid:%i >= iMaxSpellID:%i", tempid, iMaxSpellID);
			return false;
		}
		counter++;
		strcpy(sp[tempid].name, sep.arg[1]);
		strcpy(sp[tempid].player_1, sep.arg[2]);
		strcpy(sp[tempid].teleport_zone, sep.arg[3]);
		strcpy(sp[tempid].you_cast,  sep.arg[4]);
		strcpy(sp[tempid].other_casts, sep.arg[5]);
		strcpy(sp[tempid].cast_on_you, sep.arg[6]);
		strcpy(sp[tempid].cast_on_other, sep.arg[7]);
		strcpy(sp[tempid].spell_fades, sep.arg[8]);
//		strcpy(sp[tempid].player_1,strsep(&spell_line, "^"));
//		strcpy(sp[tempid].player_1,strsep(&spell_line, "^"));
//		strcpy(sp[tempid].player_1,strsep(&spell_line, "^"));
//		strcpy(sp[tempid].player_1,strsep(&spell_line, "^"));

		sp[tempid].range=atoi(sep.arg[9]);
		sp[tempid].aoerange=atof(sep.arg[10]);
		sp[tempid].pushback=atof(sep.arg[11]);
		sp[tempid].pushup=atof(sep.arg[12]);
		sp[tempid].cast_time=atoi(sep.arg[13]);
		sp[tempid].recovery_time=atoi(sep.arg[14]);
		sp[tempid].recast_time=atoi(sep.arg[15]);
		sp[tempid].buffdurationformula=atoi(sep.arg[16]);
		sp[tempid].buffduration=atoi(sep.arg[17]);
		sp[tempid].ImpactDuration=atoi(sep.arg[18]);
		sp[tempid].mana=atoi(sep.arg[19]);

		int y=0;

		for(y=0; y< 12;y++)
			sp[tempid].base[y]=atoi(sep.arg[20+y]);

		for(y=0; y< 12;y++)
			sp[tempid].max[y]=atoi(sep.arg[32+y]);

		sp[tempid].icon=atoi(sep.arg[44]);
		sp[tempid].memicon=atoi(sep.arg[45]);

		for(y=0; y< 4;y++)
			sp[tempid].components[y]=atoi(sep.arg[46+y]);

		for(y=0; y< 4;y++)
			sp[tempid].component_counts[y]=atoi(sep.arg[50+y]);

		for(y=0; y< 4;y++)
			sp[tempid].NoexpendReagent[y]=atoi(sep.arg[54+y]);

		for(y=0; y< 12;y++)
			sp[tempid].formula[y]=atoi(sep.arg[58+y]);

		sp[tempid].LightType=atoi(sep.arg[70]);
		sp[tempid].goodEffect=atoi(sep.arg[71]);
		sp[tempid].Activated=atoi(sep.arg[72]);
		sp[tempid].resisttype=atoi(sep.arg[73]);

		for(y=0; y< 12;y++)
			sp[tempid].effectid[y]=atoi(sep.arg[74+y]);

		sp[tempid].targettype=atoi(sep.arg[86]);
		sp[tempid].basediff=atoi(sep.arg[87]);
		sp[tempid].skill=atoi(sep.arg[88]);
		sp[tempid].zonetype=atoi(sep.arg[89]);
		sp[tempid].EnvironmentType=atoi(sep.arg[90]);
		sp[tempid].TimeOfDay=atoi(sep.arg[91]);

		for(y=0; y< 15;y++)
			sp[tempid].classes[y]=atoi(sep.arg[92+y]);

		sp[tempid].CastingAnim=atoi(sep.arg[107]);
		sp[tempid].TargetAnim=atoi(sep.arg[108]);
		sp[tempid].TravelType=atoi(sep.arg[109]);
		sp[tempid].SpellAffectIndex=atoi(sep.arg[110]);
		
		for(y=0; y< 23;y++) {
			sp[tempid].Spacing2[y]=atoi(sep.arg[111+y]);
		}

		sp[tempid].ResistDiff=atoi(sep.arg[134]);

		for(y=0; y< 2;y++) {
			sp[tempid].Spacing3[y]=atoi(sep.arg[135+y]);
		}

		sp[tempid].RecourseLink = atoi(sep.arg[137]);

		sp[tempid].Spacing4[0] = atoi(sep.arg[138]);

	} 
	LogFile->write(EQEMuLog::Status, "FileLoadSPDat() spells loaded: %i", counter);
	in.close();

	return true;
}
#else
void LoadSPDat(SPDat_Spell_Struct** SpellsPointer) {
	//FILE *fp;
	//cout << "Beginning Spells memset." << endl;
	int u;
	//for (u = 0; u < SPDAT_RECORDS; u++)
	//{	//cout << u << ' ';
	memset((char*) &spells,0,sizeof(SPDat_Spell_Struct)*SPDAT_RECORDS);
	//}
	//cout << "Memset finished\n";
	char temp=' ';
	int tempid=0;
	char token[64]="";
	int a = 0;
	char sep='^';
	LogFile->write(EQEMuLog::Normal, "If this is the last message you see, you forgot to move spells_en.txt from your EQ dir to this dir.");

	ifstream in;in.open(SPELLS_FILE);

	if(!in.is_open()){
		LogFile->write(EQEMuLog::Error, "File '%s' not found, spell loading FAILED!", SPELLS_FILE);
		return;
	}
	//while(!in.eof())
	//{in >> temp;}
	//for(int x =0; x< spellsen_size; x++)
	//	memset((char*) &spells[x],0,sizeof(SPDat_Spell_Struct));
	while(tempid <= SPDAT_RECORDS-1)
	{
		//if(tempid>3490)
		//{
		//	cout << "BLEH";
		//	getch();
		//}
		
		//in.getline(&temp, 624);
		in.get(temp);
		while(chrcmpI(&temp, &sep))
		{
			strncat(token,&temp,1);
			a++;//cout << temp<< ' ';
			in.get(temp);
		}
		tempid=atoi(token);
		if(tempid>=SPDAT_RECORDS)
			break;
		//cout << "TempID: " << tempid << endl;
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		strncpy(spells[tempid].name,token,a);
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		//cout << spells[tempid].name << '^';
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		strncpy(spells[tempid].player_1,token,a);
		//cout << spells[tempid].player_1 << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		strncpy(spells[tempid].teleport_zone,token,a);
		//cout << spells[tempid].teleport_zone << '^';
		a=0;

		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		strncpy(spells[tempid].you_cast,token,a);
		//cout << spells[tempid].you_cast << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		strncpy(spells[tempid].other_casts,token,a);
		//cout << spells[tempid].other_casts << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		strncpy(spells[tempid].cast_on_you,token,a);
		//cout << spells[tempid].cast_on_you << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		strncpy(spells[tempid].cast_on_other,token,a);
		//cout << spells[tempid].cast_on_other << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		strncpy(spells[tempid].spell_fades,token,a);
		//cout << spells[tempid].spell_fades << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			in.get(temp);
		}
		spells[tempid].range=atof(token);
		//cout << spells[tempid].range << '^';
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].aoerange=atof(token);
		//cout << spells[tempid].aoerange << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].pushback=atof(token);
		//cout << spells[tempid].pushback << '^';
		a=0;

		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].pushup=atof(token);
		//cout << spells[tempid].pushup << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp); 
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].cast_time=atoi(token);
		
		//cout << spells[tempid].cast_time << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].recovery_time=atoi(token);
		//cout << spells[tempid].recovery_time << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].recast_time=atoi(token);
		//cout << spells[tempid].recast_time << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].buffdurationformula=atoi(token);
		//cout << spells[tempid].buffdurationformula << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].buffduration=atoi(token);
		//cout << spells[tempid].buffduration << '^';

		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].ImpactDuration=atoi(token);
		//cout << spells[tempid].ImpactDuration<< '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].mana=atoi(token);
		//cout << spells[tempid].mana << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		int y;
		for(y=0; y< 12;y++)
		{
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
				strncat(token,&temp,1);
				a++;
				in.get(temp);
			}
			spells[tempid].base[y]=atoi(token);
			//cout << spells[tempid].base[y] << '^';
			a=0;
			for(u=0;u<64;u++)
				token[u]=(char)0;
			
			
		}
		for(y=0; y< 12;y++)
		{
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
				strncat(token,&temp,1);
				a++;
				in.get(temp);
			}
			spells[tempid].max[y]=atoi(token);
			//cout << spells[tempid].max[y] << '^';
			a=0;
			for(u=0;u<64;u++)
				token[u]=(char)0;
			
		}
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].icon=atoi(token); 
		//cout << spells[tempid].icon << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].memicon=atoi(token); 
		//cout << spells[tempid].memicon << '^';
		
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		for(y=0; y< 4;y++)
		{
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
				strncat(token,&temp,1);
				a++;
				in.get(temp);
			}
			spells[tempid].components[y]=atoi(token);
			//cout << spells[tempid].components[y] << '^';
			a=0;
			for(u=0;u<64;u++)
				token[u]=(char)0;
			
		}
		for(y=0; y< 4;y++)
		{
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
				strncat(token,&temp,1);
				a++;
				in.get(temp);
			}
			spells[tempid].component_counts[y]=atoi(token);//atoi(token);
			//cout << spells[tempid].component_counts[y] << '^';
			a=0;
			for(u=0;u<64;u++)
				token[u]=(char)0;
		}	
		for(y=0; y< 4;y++)
		{
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
				strncat(token,&temp,1);
				a++;
				in.get(temp);
			}
			spells[tempid].NoexpendReagent[y]=atoi(token); //NoExpend Reagent
			//cout << spells[tempid].NoexpendReagent[y] << '^';
			a=0;
			for(u=0;u<64;u++)
				token[u]=(char)0;
			
		}
		for(y=0; y< 12;y++)
		{
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
				strncat(token,&temp,1);
				a++;
				in.get(temp);
			}
			spells[tempid].formula[y]=atoi(token);
			//cout << spells[tempid].formula[y] << '^';
			a=0;
			for(u=0;u<64;u++)
				token[u]=(char)0;
			
		}
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}

		spells[tempid].LightType=atoi(token);
		//cout << spells[tempid].LightType << '^';
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].goodEffect=atoi(token);
		//cout << spells[tempid].goodEffect << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].Activated=atoi(token);
		//cout << spells[tempid].Activated << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);

		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].resisttype=atoi(token);
		//cout << spells[tempid].resisttype << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		for(y=0; y< 12;y++)
		{
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
				strncat(token,&temp,1);
				a++;
				in.get(temp);
			}
			spells[tempid].effectid[y]=atoi(token);
			//cout << spells[tempid].effectid[y] << '^';
			a=0;
			for(u=0;u<64;u++)
				token[u]=(char)0;
			
		}
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].targettype=atoi(token);
		//cout << spells[tempid].targettype << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].basediff=atoi(token);
		//cout << spells[tempid].basediff<< '^';
		a=0;
		for(u=0;u<64;u++)

			token[u]=(char)0;
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].skill=atoi(token);
		//cout << spells[tempid].skill << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].zonetype=atoi(token);
		//cout << spells[tempid].zonetype << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].EnvironmentType=atoi(token);
		//cout << spells[tempid].EnvironmentType << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;

			in.get(temp);
		}
		spells[tempid].TimeOfDay=atoi(token);
		//cout << spells[tempid].TimeOfDay << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		for(y=0; y< 15;y++)
		{
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
				strncat(token,&temp,1);
				a++;
				in.get(temp);
			}
			spells[tempid].classes[y]= atoi(token);
			//cout << spells[tempid].classes[y] << '^';
			a=0;
			for(u=0;u<64;u++)
				token[u]=(char)0;
			
		} //cout << "end class";
		  /*for(y=0; y< 3;y++)
		  {
		  in.get(temp);
		  while(chrcmpI(&temp,&sep))
		  {
		  strncat(token,&temp,1);
		  a++;
		  in.get(temp);
		  }
		  spells[tempid].unknown1[y]=atoi(token);
		  cout << spells[tempid].unknown1[y] << '^';
		  a=0;
		  for(u=0;u<64;u++)
		  token[u]=(char)0;
		  
			}
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
			}
			spells[tempid].unknown2=atoi(token);
			cout << spells[tempid].unknown2 << '^';
			a=0;
			for(u=0;u<64;u++)
			token[u]=(char)0;
		*/
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].CastingAnim=atoi(token);
		//cout << spells[tempid].CastingAnim << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].TargetAnim=atoi(token);
		//cout << spells[tempid].TargetAnim << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].TravelType=atoi(token);
		//cout << spells[tempid].TravelType << '^';
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		in.get(temp);
		while(chrcmpI(&temp,&sep))
		{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		}
		spells[tempid].SpellAffectIndex=atoi(token);
		//cout << spells[tempid].SpellAffectIndex << '^';
		
		a=0;
		for(u=0;u<64;u++)
			token[u]=(char)0;
		
		for(y=0; y< 23;y++)
        {
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
		    }
			spells[tempid].Spacing2[y]=atoi(token);
			//cout << spells[tempid].base[y] << '^';
			a=0;
			for(u=0;u<64;u++)
			    token[u]=(char)0;
		}
		
        in.get(temp);
	    while(chrcmpI(&temp,&sep))
			{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
			}
		spells[tempid].ResistDiff=atoi(token);
			//cout << spells[tempid].ResistDiff << '^';
		a=0;
		for(u=0;u<64;u++)
		    token[u]=(char)0;
			
        in.get(temp);
		for(y=0; y< 2;y++)
			{
			in.get(temp);
			while(chrcmpI(&temp,&sep))
			{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
			}
			spells[tempid].Spacing3[y]=atoi(token);
			//cout << spells[tempid].base[y] << '^';
			a=0;
			for(u=0;u<64;u++)
			token[u]=(char)0;
			}

        in.get(temp);
	    while(chrcmpI(&temp,&sep))
			{
			strncat(token,&temp,1);
			a++;
			in.get(temp);
			}
		spells[tempid].RecourseLink = atoi(token);
			//cout << spells[tempid].RecourseLink << '^';
		a=0;
		for(u=0;u<64;u++)
		    token[u]=(char)0;

        while(temp!='\n')
			in.get(temp);
		
		//cout << endl;
		if(tempid==SPDAT_RECORDS-1) break;
	} 
	//for(u=0;u< SPDAT_RECORDS;u++)
	// cout << u << ' ' << spells[u].name << '^';
	
	spells_loaded = true;
	cout << "Spells loaded.\n";
	in.close();
	
}
#endif


void UpdateWindowTitle(char* iNewTitle) {
#ifdef WIN32
	char tmp[500];
	if (iNewTitle) {
		snprintf(tmp, sizeof(tmp), "%i: %s", net.GetZonePort(), iNewTitle);
	}
	else {
		if (zone) {
			#if defined(GOTFRAGS) || defined(_DEBUG)
				snprintf(tmp, sizeof(tmp), "%i: %s, %i clients, %i", net.GetZonePort(), zone->GetShortName(), numclients, getpid());
			#else
				snprintf(tmp, sizeof(tmp), "%i: %s, %i clients", net.GetZonePort(), zone->GetShortName(), numclients);
			#endif
		}
		else {
			#if defined(GOTFRAGS) || defined(_DEBUG)
				snprintf(tmp, sizeof(tmp), "%i: sleeping, %i", net.GetZonePort(), getpid());
			#else
				snprintf(tmp, sizeof(tmp), "%i: sleeping", net.GetZonePort());
			#endif
		}
	}
	SetConsoleTitle(tmp);
#endif
}

/*bool ZoneBootup(int32 iZoneID, bool iStaticZone) {
	const char* zonename = database.GetZoneName(iStaticZone);
	if (iZoneID == 0 || zonename == 0)
		return false;
	srand(time(NULL));
	if (zone != 0 || ZoneLoaded) {
		cerr << "Error: Zone::Bootup call when zone already booted!" << endl;
		worldserver.SetZone(0);
		return false;
	}
	numclients = 0;
	zone = new Zone(iZoneID, zonename, net.GetZoneAddress(), net.GetZonePort());
	if (!zone->Init(iStaticZone)) {
		safe_delete(zone);
		cerr << "Zone->Init failed" << endl;
		worldserver.SetZone(0);
		return false;
	}
	if (!eqns.Open(net.GetZonePort())) {
		safe_delete(zone);
		cerr << "NetConnection::Init failed" << endl;
		worldserver.SetZone(0);
		return false;
	}
	if (!zone->LoadZoneCFG(zone->GetShortName(), true)) // try loading the zone name...
		zone->LoadZoneCFG(zone->GetFileName()); // if that fails, try the file name, then load defaults
	
	//petition_list.ClearPetitions();
	//petition_list.ReadDatabase();
	ZoneLoaded = true;
	worldserver.SetZone(iZoneID);
	zone->GetTimeSync();
	cout << "-----------" << endl << "Zone server '" << zonename << "' listening on port:" << net.GetZonePort() << endl << "-----------" << endl;
	entity_list.WriteEntityIDs();
	UpdateWindowTitle();
	return true;
}*/

// Original source found at http://www.castaglia.org/proftpd/doc/devel-guide/src/lib/strsep.c.html
char *strsep(char **stringp, const char *delim)
{
  char *res;

  if(!stringp || !*stringp || !**stringp)
    return (char*)0;

  res = *stringp;
  while(**stringp && !strchr(delim,**stringp))
    (*stringp)++;
  
  if(**stringp) {
    **stringp = '\0';
    (*stringp)++;
  }

  return res;
}
