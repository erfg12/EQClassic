
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <float.h>
#include <fstream>
#include <ctime>
#include <math.h>

#ifndef WIN32
#include <pthread.h>
#include "unix.h"
#endif
#include "EQCUtils.hpp"
#include "spawngroup.h"
#include "spawn2.h"
#include "zone.h"
#include "worldserver.h"
#include "npc.h"
#include "net.h"
#include "seperator.h"
#include "packet_dump_file.h"
#include "map.h"
#include "watermap.h"
#include "ClientList.h"
#include "petitions.h"
#include "questmgr.h"
#include "Client_Commands.h"

#ifdef EMBPERL
	#include "embparser.h"
#endif


using namespace std;

extern WorldServer worldserver;
extern Zone* zone;
extern int32 numclients;
extern NetConnection net;
extern PetitionList petition_list;
Mutex MZoneShutdown;
extern ClientList client_list;
extern Mutex MNetLoop;
bool loadingNPC = true;
Mutex MBoatsToRemove;

Zone* zone = 0;
volatile bool ZoneLoaded = false;

void CleanupLoadZoneState(int32 spawn2_count, ZSDump_Spawn2** spawn2_dump, ZSDump_NPC** npc_dump, ZSDump_NPC_Loot** npcloot_dump, NPCType** gmspawntype_dump, Spawn2*** spawn2_loaded, NPC*** npc_loaded, MYSQL_RES** result);

bool Zone::Bootup(char* zone_name)
{
	srand(time(NULL));

	if (zone != 0 || ZoneLoaded)
	{
		cerr << "Error: Zone::Bootup call when zone already booted!" << endl;
		worldserver.SetZone("");
		return false;
	}

	numclients = 0;
	zone = new Zone(zone_name, net.GetZoneAddress(), net.GetZonePort());

	if (!zone->Init())
	{
		safe_delete(zone);
		cerr << "Zone->Init failed" << endl;
		worldserver.SetZone("");
		return false;
	}
	if (!net.Init())
	{
		safe_delete(zone);
		cerr << "NetConnection::Init failed" << endl;
		worldserver.SetZone("");
		return false;
	}

	//Wizzel: Load the appearance information for the current zone
	zone->LoadZoneCFG(zone->GetFileName());

	petition_list.ClearPetitions();
	petition_list.ReadDatabase();
	ZoneLoaded = true;
	worldserver.SetZone(zone_name);
	char bfr[64];memset(bfr, 0, sizeof(bfr));
	sprintf(bfr, "EQC: -Running %s:%i", zone_name, net.GetZonePort());
	cout << "-----------" << endl;
	EQC::Common::PrintF(CP_ZONESERVER, "Zone server '%s' now listening on port %i\n", zone_name, net.GetZonePort());
	cout << "-----------" << endl;
	EQC::Common::PrintF(CP_ZONESERVER, "Loading NPC's for '%s'...\n",zone_name);
	Database::Instance()->LoadNPCTypes(zone_name);
	SetConsoleTitleA(bfr);
	entity_list.WriteEntityIDs();
	//zone->weather_type=Database::Instance()->GetZoneW(zone->short_name); // Tazadar : we load the weather of the zone (snow/rain/nothing)
	//cout << "Default weather type for zone is " << (long)zone->weather_type << endl; // Tazadar : if the load failed
	//zone->weather_timer=0; //Tazadar:We initialise the timer
	//zone->zone_weather=zone->weather_type;	//Tazadar:We put the weather type to start
	//zone->weatherProc(); //Tazadar:We start the timer once the zone is up 

	// Kibanu - Set Time of Day
	zone->SetTimeOfDay( Database::Instance()->IsDaytime() );

	
	#ifdef EMBPERL
		#ifdef EMBPERL_XS
			EQC::Common:: , "Loading embedded perl XS");
 			AutoDelete<PerlXSParser> ADparse;
			try {
				ADparse.init((PerlXSParser **)(&parse), new PerlXSParser);
			}
		#else //old EMBPERL
		EQC::Common::Log(EQCLog::Status,CP_ZONESERVER, "Loading embedded perl");		
			try {
				PerlembParser::Init();
				EQC::Common::Log(EQCLog::Status,CP_ZONESERVER, "Loading quests...");
				perlParser->ReloadQuests();
			}
		#endif
			catch(const char *err)
			{//this should never happen, so if it does, it is something really serious (like a bad perl install), so we'll shutdown.
				EQC::Common::Log(EQCLog::Error,CP_ZONESERVER, "Fatal error initializing perl: %s", err);						
			}					
	#endif //EMBPERL

   int retval=command_init();
	if(retval<0)
		EQC::Common::Log(EQCLog::Error,CP_ZONESERVER, "Command loading FAILED");
	else
		EQC::Common::Log(EQCLog::Status,CP_ZONESERVER, "%d commands loaded", retval);

	return true;
}

void Zone::Shutdown(bool quite) {
	if (!ZoneLoaded)
		return;
	LockMutex lock2(&MNetLoop);
	worldserver.SetZone("");
	EQC::Common::PrintF(CP_ZONESERVER, "----SERVER SHUTDOWN----\n");
	petition_list.ClearPetitions();
	if (!quite) {
		EQC::Common::PrintF(CP_ZONESERVER, "Zone shutdown, going to sleep! ZzzZzz\n");
		SetConsoleTitleA("EQC: -Sleeping zZz");
	}
	ZoneLoaded = false;
	char pzs[3] = "";
	if (Database::Instance()->GetVariable("PersistentZoneState", pzs, 2)) {
		if (pzs[0] == '1') {
			Sleep(100);
			cout << "Saving zone state...." << endl;
			Database::Instance()->DumpZoneState();
		}
	}
	client_list.SendPacketQueues(true);
	entity_list.Clear();
	net.Close();
	safe_delete(zone);
}

Zone::Zone(char* in_short_name, char* in_address, int16 in_port)
{
	bulkOnly = true;
	bulkOnly_timer = new Timer(10000);
	bulkOnly_timer->Start();
	zone_weather = WEATHER_OFF;
	spawn_group_list = 0;
	short_name = strcpy(new char[strlen(in_short_name)+1], in_short_name);
	memset(file_name, 0, sizeof(file_name));
	long_name = 0;
	aggroedmobs = 0;
	address = strcpy(new char[strlen(in_address)+1], in_address);
	port = in_port;

	Database::Instance()->GetZoneLongName(short_name, &long_name, file_name, &psafe_x, &psafe_y, &psafe_z);

	if (long_name == 0)
	{
		long_name = strcpy(new char[18], "Long zone missing");
	}

	autoshutdown_timer = new Timer(ZONE_AUTOSHUTDOWN_DELAY);
	autoshutdown_timer->Start(AUTHENTICATION_TIMEOUT * 1000, false);
	//Yeahlight: Our zones are now static, they do not shutdown
	autoshutdown_timer->Disable();
	bindCondition = 0;
	levCondition = 0;
	outDoorZone = false;

	cout<<"Loading zone safe points"<<endl;
	SetSafeX(-1);
	SetSafeY(-1);
	SetSafeZ(-1);
	SetSafeHeading(-1);
	if(Database::Instance()->GetSafePoints(short_name, &safeX, &safeY, &safeZ, 0, 0, &safeHeading) == false)
		cout<<"ERROR loading zone safe points"<<endl;

	zoneStatus_timer = new Timer(ZONE_STATUS_CHECK_DELAY);
}

bool Zone::Init() {
	map = Map::LoadMapfile(short_name);
	// Harakiri load water map from directory
	watermap = WaterMap::LoadWaterMapfile(short_name);

	EQC::Common::PrintF(CP_ZONESERVER, "Loading roam boxes...\n");

	if (!Database::Instance()->LoadRoamBoxes(this->short_name, this->roamBoxes, this->numberOfRoamBoxes))
		cout<<"LOADROAMBOXES load failed\n"; 

	zoneID = Database::Instance()->LoadZoneID(short_name);

	if (!loadNodesIntoMemory())
		cout<<"NODE load failed"<<endl;

	//Yeahlight: Attempt to load preprocessed grid data. If the zone fails, then load the grid data from the database
	if (!LoadProcessedGridData())
		cout<<"PREPROCESSEDGRID load failed"<<endl;

	if (!Database::Instance()->LoadPatrollingNodes(zoneID))
		cout<<"PATROLLINGNODE load failed"<<endl;

	spawn_group_list = new SpawnGroupList();
	if (!Database::Instance()->PopulateZoneLists(short_name, &zone_point_list, spawn_group_list))
		return false;

	char pzs[3] = "";
	if (Database::Instance()->GetVariable("PersistentZoneState", pzs, 2)) {
		cout << pzs[0] << endl;
		if (pzs[0] == '1') {
			sint8 tmp = Database::Instance()->LoadZoneState(short_name, spawn2_list);
			if (tmp == 1) {
				cout << "Zone state loaded." << endl;
			}
			else if (tmp == -1) {
				cout << "Error loading zone state" << endl;
				return false;
			}
			else if (tmp == 0) {
				cout << "Zone state not found, loading from spawn lists." << endl;
				if (!Database::Instance()->PopulateZoneSpawnList(short_name, spawn2_list))
					return false;
			}
			else
				cout << "Unknown LoadZoneState return value" << endl;
		}
		else if (!Database::Instance()->PopulateZoneSpawnList(short_name, spawn2_list))
			return false;
	}
	else if (!Database::Instance()->PopulateZoneSpawnList(short_name, spawn2_list))
	{
		cout << "Populate zone spawn list failed" << endl;
		return false;
	}
	if (!Database::Instance()->loadZoneLines(&zone_line_data, short_name))
		cout<<"ZONELINE load failed"<<endl;

	zone->numberOfZoneLineNodes = zone->zone_line_data.size();
	for(int i = 0; i < (int)zone->numberOfZoneLineNodes; i++){
		zone->thisZonesZoneLines[i] = new ZoneLineNode;
		zone->thisZonesZoneLines[i]->x = zone->zone_line_data.at(i)->x;
		zone->thisZonesZoneLines[i]->y = zone->zone_line_data.at(i)->y;
		zone->thisZonesZoneLines[i]->z = zone->zone_line_data.at(i)->z;
		strcpy(zone->thisZonesZoneLines[i]->target_zone, zone->zone_line_data.at(i)->target_zone);
		zone->thisZonesZoneLines[i]->target_x = zone->zone_line_data.at(i)->target_x;
		zone->thisZonesZoneLines[i]->target_y = zone->zone_line_data.at(i)->target_y;
		zone->thisZonesZoneLines[i]->target_z = zone->zone_line_data.at(i)->target_z;
		zone->thisZonesZoneLines[i]->range = zone->zone_line_data.at(i)->range;
		zone->thisZonesZoneLines[i]->heading = zone->zone_line_data.at(i)->heading;
		zone->thisZonesZoneLines[i]->id = zone->zone_line_data.at(i)->id;
		zone->thisZonesZoneLines[i]->maxZDiff = zone->zone_line_data.at(i)->maxZDiff;
		zone->thisZonesZoneLines[i]->keepX = zone->zone_line_data.at(i)->keepX;
		zone->thisZonesZoneLines[i]->keepY = zone->zone_line_data.at(i)->keepY;
		zone->thisZonesZoneLines[i]->keepZ = zone->zone_line_data.at(i)->keepZ;
	}

	if (!Database::Instance()->LoadDoorData(&door_list, short_name))
		cout<<"DOOR load failed.\n";

	EQC::Common::PrintF(CP_ZONESERVER, "Loading objects...\n");

	if (!Database::Instance()->LoadObjects(&object_list_data, short_name))
		cout<<"Object loading failed.\n";

	for(int i=0;i<(int)zone->object_list_data.size();i++){
		zone->object_list.push_back(new Object(zone->object_list_data.at(i)));
		entity_list.AddObject(zone->object_list.at(i), false);
	}

	if (!loadPathsIntoMemory())
		cout<<"PATH load failed"<<endl;

	if (!Database::Instance()->LoadPlayerCorpses(short_name))
		cout<<"PLAYER CORPSE load failed"<<endl;

	zoneAgroCounter = 0;
	for(int i = 0; i < MAX_ZONE_AGRO; i++)
		this->zoneAgro[i] = NULL;

	if(!Database::Instance()->LoadZoneRules(short_name, bindCondition, levCondition, outDoorZone))
		cout<<"ZONE RULES load failed"<<endl;

	return true;
}

Zone::~Zone()
{
	safe_delete(map);//delete map;
	safe_delete(spawn_group_list);
	if (worldserver.Connected()) {
		worldserver.SetZone("");

		ServerPacket* pack = new ServerPacket(ServerOP_ClientList, sizeof(ServerClientList_Struct));

		pack->pBuffer = new uchar[pack->size];
		memset(pack->pBuffer, 0, pack->size);
		ServerClientList_Struct* scl = (ServerClientList_Struct*) pack->pBuffer;
		scl->remove = true;
		strcpy(scl->zone, short_name);
		worldserver.SendPacket(pack);
		safe_delete(pack);//delete pack;
	}
	safe_delete_array(short_name);//delete[] short_name;
	safe_delete_array(long_name);//delete[] long_name;
	safe_delete_array(address);//delete[] address;
	safe_delete(autoshutdown_timer);//delete autoshutdown_timer;
	for(int i = 0; i < MAXZONENODES; i++){
		safe_delete(thisZonesNodes[i]);//delete thisZonesNodes[i];
	}
	for(int i = 0; i < zone->numberOfZoneLineNodes; i++){
		safe_delete(zone->thisZonesZoneLines[i]);//delete zone->thisZonesZoneLines[i];
	}
	zone_line_data.clear();
	safe_delete(bulkOnly_timer);
	safe_delete(zoneStatus_timer);
}
//Wizzel:Loads information about the current zone appearance.
void Zone::LoadZoneCFG(const char* filename) {

	//Wizzel: Set up the variables
	int8 sky;
	int8 fog_red;
	int8 fog_green;
	int8 fog_blue;
	float fog_minclip;
	float fog_maxclip;
	float minclip;
	float maxclip;
	int8 ztype;
	float safe_x;
	float safe_y;
	float safe_z;
	float underworld;

	//Wizzel: Grab the zone ID
	zoneID = Database::Instance()->LoadZoneID(short_name);

	//Wizzel: Loads a default zone struct -- Will eliminate most of this in the upcoming days.
	int16 zhdr_data[71+14] = {
	0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x2041, 0x0000, 0x2041, 0x0000, 0x2041, 0x0000, 0x2041, 0x0040, 
	0x9C46, 0x0040, 0x9C46, 0x0040, 0x9C46, 0x0040, 0x9C46, 0xCDCC, 
	0xCC3E, 0x020A, 0x0A0A, 0x0A18, 0x0602, 0x0A00, 0x0000, 0x0000, 
	0x0000, 0x00FF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
	0xFFFF, 0xFF00, 0x0A00, 0x0400, 0x0200, 0x0000, 0x0000, 0x0000, 
	0x403F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xA040, 0x0000, 
	0xD841, 0x0000, 0xFAC6, 0x0000, 0x7A44, 0x0040, 0x9C46, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000}; 

	int16* temp3 = (int16*) &((uchar*)&newzone_data)[230];
	for (int i=0;i<71;i++)
	{
		*temp3 = ntohs(zhdr_data[i]); temp3++;
	}


	
	//Wizzel: Loads the relevant information from the database
	Database::Instance()->GetZoneCFG(zoneID, &sky, &fog_red, &fog_green, &fog_blue, &fog_minclip, &fog_maxclip, &minclip, &maxclip, &ztype, &safe_x, &safe_y, &safe_z, &underworld);

	strcpy(newzone_data.zone_short_name, GetShortName());
	strcpy(newzone_data.zone_long_name, GetLongName());
	newzone_data.sky = sky;
	newzone_data.minclip = minclip;
	newzone_data.maxclip = maxclip;
	newzone_data.zonetype = ztype;
	newzone_data.safe_x = safe_x;
	newzone_data.safe_y = safe_y;
	newzone_data.safe_z = safe_z;
	newzone_data.underworld = underworld;


	//Wizzel: Apply the variables to all parts of the same byte type
	int bananacake=0;
	for(bananacake=0;bananacake<4;bananacake++){
	newzone_data.fog_red[bananacake] = fog_red;
	newzone_data.fog_green[bananacake] = fog_green;
	newzone_data.fog_blue[bananacake] = fog_blue;
	newzone_data.fog_minclip[bananacake] = fog_minclip;
	newzone_data.fog_maxclip[bananacake] = fog_maxclip;
	}

	LogFile->write(EQCLog::Status, "Successfully loaded Zone Config.");
}

bool Zone::SaveZoneCFG(char* filename) {
	char fname[255]="./cfg/custom.cfg";
	sprintf(fname, "./cfg/custom-%s.cfg", filename);
	FILE *zhdr_file;
	if ((zhdr_file = fopen(filename, "wb")) == 0) {
		return false;
	}
	fseek(zhdr_file, 0, SEEK_SET);
	uchar shortname[20] = "";
	uchar longname[180] = "";
	strcpy((char*)shortname, zone->GetShortName());
	strcpy((char*)longname, zone->GetLongName());
	fwrite(&shortname, sizeof(uchar), 20, zhdr_file);
	fwrite(&longname, sizeof(uchar), 180, zhdr_file);
	fwrite(&((uchar*)&newzone_data)[230], sizeof(uchar), 142, zhdr_file);
	fclose(zhdr_file);
	return true;
}

bool Zone::Process()
{
	LockMutex lock(&MZoneLock);
	LinkedListIterator<Spawn2*> iterator(spawn2_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->Process())
		{
			iterator.Advance();
		}
		else
		{
			iterator.RemoveCurrent();
		}
	}

	//Yeahlight: We use static zones now; no more going to sleep
	//if (autoshutdown_timer->Check()) {
	//	StartShutdownTimer();
	//	if (numclients == 0) {
	//		EQC::Common::PrintF(CP_ZONESERVER, "Automatic shutdown\n");
	//		return false;
	//	}
	//}

	//Yeahlight: Bulk spawn only timer has expired, permit respawns to send out spawn packets now
	if(bulkOnly == true && bulkOnly_timer->Check())
	{
		bulkOnly = false;
		bulkOnly_timer->Disable();
	}

	//Yeahlight: Time for this zone server to inform World that it is still running loops
	if(zoneStatus_timer->Check())
	{
		ServerPacket* pack = new ServerPacket(ServerOP_ZoneRunStatus, sizeof(ZoneStatus_Struct));
		ZoneStatus_Struct* zs = (ZoneStatus_Struct*)pack->pBuffer;
		zs->zoneID = this->zoneID;
		worldserver.SendPacket(pack);
		safe_delete(pack);
	}

	//Tazadar : we check the timer to see if we change the weather or not
	//if(weather_timer-time(0)<0){
	//	weatherProc();
	//}

	#ifdef EMBPERL
	if(quest_manager.quest_timers->Check()) {
			quest_manager.Process();
		}
	#endif EMBPERL

	return true;
}

void Zone::StartShutdownTimer(int32 set_time) {
	MZoneLock.lock();
	if (set_time > autoshutdown_timer->GetRemainingTime()) {
		autoshutdown_timer->Start(set_time, false);
	}
	MZoneLock.unlock();
}

void Zone::Depop() {
	LinkedListIterator<Spawn2*> iterator(spawn2_list);

	MZoneLock.lock();
	iterator.Reset();
	while (iterator.MoreElements())
	{
		iterator.GetData()->Depop();
		iterator.Advance();
	}
	MZoneLock.unlock();
	entity_list.Depop();
}

void Zone::Repop(int32 delay) {
	zone->aggroedmobs = 0;
	Depop();
	entity_list.Message(0, BLACK, "<SYSTEM_MSG>:Zone REPOP IMMINENT");

	LinkedListIterator<Spawn2*> iterator(spawn2_list);

	MZoneLock.lock();
	iterator.Reset();
	while (iterator.MoreElements())
	{
		iterator.RemoveCurrent();
	}

	if (!Database::Instance()->PopulateZoneSpawnList(short_name, spawn2_list, delay)) {
		entity_list.Message(0, BLACK, "Error in Zone::Repop: database.PopulateZoneSpawnList failed");
	}

	MZoneLock.unlock();
}

ZonePoint* Zone::GetClosestZonePoint(float x, float y, float z, char* to_name)
{
	LinkedListIterator<ZonePoint*> iterator(zone_point_list);
	ZonePoint* closest_zp = 0;
	float closest_dist = FLT_MAX;

	iterator.Reset();
	while(iterator.MoreElements())	
	{
		ZonePoint* zp = iterator.GetData();
		if (strcmp(zp->target_zone, to_name) == 0)
		{
			float dist = (zp->x-x)*(zp->x-x)+(zp->y-y)*(zp->y-y)+(zp->z-z)*(zp->z-z);
			if (dist < closest_dist)
			{
				closest_zp = zp;
				closest_dist = dist;	
			}
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return closest_zp;
}

//bool Database::PopulateZoneLists(char* zone_name, NPCType* &npc_type_array, uint32 &max_npc_type, LinkedList<Spawn*>& spawn_list, LinkedList<ZonePoint*>& zone_point_list, LinkedList<Spawn2*> &spawn2_list, SpawnGroupList* spawn_group_list)
//bool Database::PopulateZoneLists(char* zone_name, NPCType* &npc_type_array, uint32 &max_npc_type, LinkedList<ZonePoint*>& zone_point_list, SpawnGroupList* spawn_group_list)
bool Database::PopulateZoneLists(char* zone_name, LinkedList<ZonePoint*>* zone_point_list, SpawnGroupList* spawn_group_list) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	query = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT DISTINCT(spawngroupID), spawngroup.name FROM spawn2,spawngroup WHERE spawn2.spawngroupID=spawngroup.ID and zone='%s'", zone_name), errbuf, &result))
	{
		safe_delete_array(query);//delete[] query;
		while (row = mysql_fetch_row(result)) {
			SpawnGroup* newSpawnGroup = new SpawnGroup(atoi(row[0]), row[1]);
			spawn_group_list->AddSpawnGroup(newSpawnGroup);
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error1 in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	query = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT spawnentry.spawngroupID, npcid, chance, time_of_day from spawnentry, spawn2 where spawnentry.spawngroupID=spawn2.spawngroupID and zone='%s' ORDER by chance", zone_name), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
		while (row = mysql_fetch_row(result))
		{
			SpawnGroup* sg = spawn_group_list->GetSpawnGroup(atoi(row[0]));

			if (sg == 0)
			{
				// ERROR... unable to find the spawn group!
				EQC::Common::PrintF(CP_ZONESERVER, "Unable to locate a spawn group for NPC ID: %i!\n", atoi(row[1]));
			}
			else
			{
				SpawnEntry* newSpawnEntry = new SpawnEntry(atoi(row[1]), atoi(row[2]), (SPAWN_TIME_OF_DAY)atoi(row[3]));
				sg->AddSpawnEntry(newSpawnEntry);
			}

		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error2 in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	// CODER end new spawn code

	return true;
}
/*bool Database::PopulateZoneLists(char* zone_name, LinkedList<ZonePoint*>* zone_point_list, SpawnGroupList* spawn_group_list) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	MakeAnyLenString(&query, "SELECT x,y,z,target_x,target_y,target_z,target_zone,heading FROM zone_points WHERE zone='%s'", zone_name);
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete_array(query);//delete[] query;
		while(row = mysql_fetch_row(result))
		{
			ZonePoint* zp = new ZonePoint;
			zp->x = atof(row[0]);
			zp->y = atof(row[1]);
			zp->z = atof(row[2]);
			zp->target_x = atof(row[3]);
			zp->target_y = atof(row[4]);
			zp->target_z = atof(row[5]);
			strncpy(zp->target_zone, row[6], 16);
			zp->heading = atoi(row[7]);
			zone_point_list->Insert(zp);
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error1 in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	// CODER new spawn code
	query = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT DISTINCT(spawngroupID), spawngroup.name FROM spawn2,spawngroup WHERE spawn2.spawngroupID=spawngroup.ID and zone='%s'", zone_name), errbuf, &result))
	{
		safe_delete_array(query);//delete[] query;
		while(row = mysql_fetch_row(result)) {
			SpawnGroup* newSpawnGroup = new SpawnGroup( atoi(row[0]), row[1]);
			spawn_group_list->AddSpawnGroup( newSpawnGroup );
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error2 in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	query = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT spawnentry.spawngroupID, npcid, chance, time_of_day from spawnentry, spawn2 where spawnentry.spawngroupID=spawn2.spawngroupID and zone='%s' ORDER by chance", zone_name), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
		while(row = mysql_fetch_row(result))
		{
			SpawnGroup* sg = spawn_group_list->GetSpawnGroup(atoi(row[0]));

			if(sg == 0)
			{
				// ERROR... unable to find the spawn group!
				EQC::Common::PrintF(CP_ZONESERVER, "Unable to locate a spawn group for NPC ID: %i!\n", atoi(row[1]));
			}
			else
			{
				SpawnEntry* newSpawnEntry = new SpawnEntry( atoi(row[1]), atoi(row[2]), (SPAWN_TIME_OF_DAY)atoi(row[3]));			
				sg->AddSpawnEntry(newSpawnEntry);
			}

		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error3 in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	// CODER end new spawn code

	return true;
}*/

bool Database::PopulateZoneSpawnList(char* zone_name, LinkedList<Spawn2*> &spawn2_list, int32 repopdelay) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	MakeAnyLenString(&query, "SELECT id, spawngroupID, x, y, z, heading, respawntime, variance, roamRange, pathgrid FROM spawn2 WHERE zone='%s'", zone_name);
	
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete_array(query);//delete[] query;
		int timeleft_placeholder = 0;
		while(row = mysql_fetch_row(result))
		{
			Spawn2* newSpawn = new Spawn2(atoi(row[0]), atoi(row[1]), atof(row[2]), atof(row[3]), atof(row[4]), atof(row[5]), atoi(row[6]), atoi(row[7]), timeleft_placeholder, atoi(row[8]), atoi(row[9]));
			newSpawn->Repop(repopdelay);
			spawn2_list.Insert( newSpawn );
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	return true;
}

int32 Zone::CountSpawn2() {
	LinkedListIterator<Spawn2*> iterator(spawn2_list);
	int16 count = 0;

	iterator.Reset();
	while(iterator.MoreElements())	
	{
		count++;
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return count;
}

int32 Zone::DumpSpawn2(ZSDump_Spawn2* spawn2dump, int32* spawn2index, Spawn2* spawn2) {
	if (spawn2 == 0)
		return 0;
	LinkedListIterator<Spawn2*> iterator(spawn2_list);
	int32	index = 0;

	iterator.Reset();
	while(iterator.MoreElements())	
	{
		if (iterator.GetData() == spawn2) {
			spawn2dump[*spawn2index].spawn2_id = iterator.GetData()->spawn2_id;
			spawn2dump[*spawn2index].time_left = iterator.GetData()->timer->GetRemainingTime();
			iterator.RemoveCurrent();
			return (*spawn2index)++;
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return 0xFFFFFFFF;
}

void Zone::DumpAllSpawn2(ZSDump_Spawn2* spawn2dump, int32* spawn2index) {
	LinkedListIterator<Spawn2*> iterator(spawn2_list);
	int32	index = 0;

	iterator.Reset();
	while(iterator.MoreElements())	
	{
		spawn2dump[*spawn2index].spawn2_id = iterator.GetData()->spawn2_id;
		spawn2dump[*spawn2index].time_left = iterator.GetData()->timer->GetRemainingTime();
		(*spawn2index)++;
		iterator.RemoveCurrent();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

bool Database::DumpZoneState() {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM zone_state_dump WHERE zonename='%s'", zone->GetShortName()), errbuf)) {
		cerr << "Error in DumpZoneState query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;



	int32	spawn2_count = zone->CountSpawn2();
	int32	npc_count = 0;
	int32	npcloot_count = 0;
	int32	gmspawntype_count = 0;
	entity_list.CountNPC(&npc_count, &npcloot_count, &gmspawntype_count);

	cout << "DEBUG: spawn2count=" << spawn2_count << ", npc_count=" << npc_count << ", npcloot_count=" << npcloot_count << ", gmspawntype_count=" << gmspawntype_count << endl;

	ZSDump_Spawn2* spawn2_dump = 0;
	ZSDump_NPC*	npc_dump = 0;
	ZSDump_NPC_Loot* npcloot_dump = 0;
	NPCType* gmspawntype_dump = 0;
	if (spawn2_count > 0) {
		spawn2_dump = (ZSDump_Spawn2*) new uchar[spawn2_count * sizeof(ZSDump_Spawn2)];
		memset(spawn2_dump, 0, sizeof(ZSDump_Spawn2) * spawn2_count);
	}
	if (npc_count > 0) {
		npc_dump = (ZSDump_NPC*) new uchar[npc_count * sizeof(ZSDump_NPC)];
		memset(npc_dump, 0, sizeof(ZSDump_NPC) * npc_count);
		for (int i=0; i < npc_count; i++) {
			npc_dump[i].spawn2_dump_index = 0xFFFFFFFF;
			npc_dump[i].gmspawntype_index = 0xFFFFFFFF;
		}
	}
	if (npcloot_count > 0) {
		npcloot_dump = (ZSDump_NPC_Loot*) new uchar[npcloot_count * sizeof(ZSDump_NPC_Loot)];
		memset(npcloot_dump, 0, sizeof(ZSDump_NPC_Loot) * npcloot_count);
		for (int k=0; k < npc_count; k++)
			npcloot_dump[k].npc_dump_index = 0xFFFFFFFF;
	}
	if (gmspawntype_count > 0) {
		gmspawntype_dump = (NPCType*) new uchar[gmspawntype_count * sizeof(NPCType)];
		memset(gmspawntype_dump, 0, sizeof(NPCType) * gmspawntype_count);
	}

	entity_list.DoZoneDump(spawn2_dump, npc_dump, npcloot_dump, gmspawntype_dump);
	query = new char[512 + ((sizeof(ZSDump_Spawn2) * spawn2_count + sizeof(ZSDump_NPC) * npc_count + sizeof(ZSDump_NPC_Loot) * npcloot_count + sizeof(NPCType) * gmspawntype_count) * 2)];
	char* end = query;

	end += sprintf(end, "Insert Into zone_state_dump (zonename, spawn2_count, npc_count, npcloot_count, gmspawntype_count, spawn2, npcs, npc_loot, gmspawntype) values ('%s', %i, %i, %i, %i, ", zone->GetShortName(), spawn2_count, npc_count, npcloot_count, gmspawntype_count);
	*end++ = '\'';
	if (spawn2_dump != 0) {
		end += DoEscapeString(end, (char*)spawn2_dump, sizeof(ZSDump_Spawn2) * spawn2_count);
		safe_delete_array(spawn2_dump);//delete[] spawn2_dump;
	}
	*end++ = '\'';
	end += sprintf(end, ", ");
	*end++ = '\'';
	if (npc_dump != 0) {
		end += DoEscapeString(end, (char*)npc_dump, sizeof(ZSDump_NPC) * npc_count);
		safe_delete_array(npc_dump);//delete[] npc_dump;
	}
	*end++ = '\'';
	end += sprintf(end, ", ");
	*end++ = '\'';
	if (npcloot_dump != 0) {
		end += DoEscapeString(end, (char*)npcloot_dump, sizeof(ZSDump_NPC_Loot) * npcloot_count);
		safe_delete_array(npcloot_dump);//delete[] npcloot_dump;
	}
	*end++ = '\'';
	end += sprintf(end, ", ");
	*end++ = '\'';
	if (gmspawntype_dump != 0) {
		end += DoEscapeString(end, (char*)gmspawntype_dump, sizeof(NPCType) * gmspawntype_count);
		safe_delete_array(gmspawntype_dump);//delete[] gmspawntype_dump;
	}
	*end++ = '\'';
	end += sprintf(end, ")");

	int32 affected_rows = 0;
	if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
		//    if (DoEscapeString(query, (unsigned int) (end - query))) {
		safe_delete_array(query);//delete[] query;
		cerr << "Error in ZoneDump query " << errbuf << endl;
		return false;
	}
	safe_delete_array(query);//delete[] query;

	if (affected_rows == 0) {
		cerr << "Zone dump failed. (affected rows = 0)" << endl;
		return false;
	}
	return true;
}

sint8 Database::LoadZoneState(char* zonename, LinkedList<Spawn2*>& spawn2_list) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	int32 i;
	unsigned long* lengths;
	int32	elapsedtime = 0;
	int32	spawn2_count = 0;
	int32	npc_count = 0;
	int32	npcloot_count = 0;
	int32	gmspawntype_count = 0;
	ZSDump_Spawn2* spawn2_dump = 0;
	ZSDump_NPC*	npc_dump = 0;
	ZSDump_NPC_Loot* npcloot_dump = 0;
	NPCType* gmspawntype_dump = 0;
	Spawn2** spawn2_loaded = 0;
	NPC** npc_loaded = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT spawn2_count, npc_count, npcloot_count, gmspawntype_count, spawn2, npcs, npc_loot, gmspawntype, (UNIX_TIMESTAMP()-UNIX_TIMESTAMP(time)) as elapsedtime FROM zone_state_dump WHERE zonename='%s'", zonename), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;

		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			cout << "Elapsed time: " << row[8] << endl;
			elapsedtime = atoi(row[8]) * 1000;
			lengths = mysql_fetch_lengths(result);
			spawn2_count = atoi(row[0]);
			cout << "Spawn2count: " << spawn2_count << endl;

			if (lengths[4] != (sizeof(ZSDump_Spawn2) * spawn2_count)) {
				cerr << "Error in LoadZoneState: spawn2_dump length mismatch l=" << lengths[4] << ", e=" << (sizeof(ZSDump_Spawn2) * spawn2_count) << endl;
				CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
				return -1;
			}
			else if (spawn2_count > 0) {
				spawn2_dump = new ZSDump_Spawn2[spawn2_count];
				spawn2_loaded = new Spawn2*[spawn2_count];
				memcpy(spawn2_dump, row[4], lengths[4]);
				for (i=0; i < spawn2_count; i++) {
					if (spawn2_dump[i].time_left == 0xFFFFFFFF) // npc spawned, timer should be disabled
						spawn2_loaded[i] = LoadSpawn2(spawn2_list, spawn2_dump[i].spawn2_id, 0xFFFFFFFF);
					else if (spawn2_dump[i].time_left <= elapsedtime)
						spawn2_loaded[i] = LoadSpawn2(spawn2_list, spawn2_dump[i].spawn2_id, 0);
					else
						spawn2_loaded[i] = LoadSpawn2(spawn2_list, spawn2_dump[i].spawn2_id, spawn2_dump[i].time_left - elapsedtime);
					if (spawn2_loaded[i] == 0) {
						cerr << "Error in LoadZoneState: spawn2_loaded[" << i << "] == 0" << endl;
						CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
						return -1;
					}
				}
			}

			gmspawntype_count = atoi(row[3]);
			cout << "gmspawntype_count: " << gmspawntype_count << endl;

			if (lengths[7] != (sizeof(NPCType) * gmspawntype_count)) {
				cerr << "Error in LoadZoneState: gmspawntype_dump length mismatch l=" << lengths[7] << ", e=" << (sizeof(NPCType) * gmspawntype_count) << endl;
				CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
				return -1;
			}
			else if (gmspawntype_count > 0) {
				gmspawntype_dump = new NPCType[gmspawntype_count];
				memcpy(gmspawntype_dump, row[7], lengths[7]);
			}

			npc_count = atoi(row[1]);
			cout << "npc_count: " << npc_count << endl;
			if (lengths[5] != (sizeof(ZSDump_NPC) * npc_count)) {
				cerr << "Error in LoadZoneState: npc_dump length mismatch l=" << lengths[5] << ", e=" << (sizeof(ZSDump_NPC) * npc_count) << endl;
				CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
				return -1;
			}
			else if (npc_count > 0) {
				npc_dump = new ZSDump_NPC[npc_count];
				npc_loaded = new NPC*[npc_count];
				for (i=0; i < npc_count; i++) {
					npc_loaded[i] = 0;
				}
				memcpy(npc_dump, row[5], lengths[5]);
				for (i=0; i < npc_count; i++) {
					if (npc_loaded[i] != 0) {
						cerr << "Error in LoadZoneState: npc_loaded[" << i << "] != 0" << endl;
						CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
						return -1;
					}
					Spawn2* tmp = 0;
					if (!npc_dump[i].corpse && npc_dump[i].spawn2_dump_index != 0xFFFFFFFF) {
						if (spawn2_loaded == 0 || npc_dump[i].spawn2_dump_index >= spawn2_count) {
							cerr << "Error in LoadZoneState: (spawn2_loaded == 0 || index >= count) && npc_dump[" << i << "].spawn2_dump_index != 0xFFFFFFFF" << endl;
							CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
							return -1;
						}
						tmp = spawn2_loaded[npc_dump[i].spawn2_dump_index];
						spawn2_loaded[npc_dump[i].spawn2_dump_index] = 0;
					}
					if (npc_dump[i].npctype_id == 0) {
						if (npc_dump[i].gmspawntype_index == 0xFFFFFFFF) {
							cerr << "Error in LoadZoneState: gmspawntype index invalid" << endl;
							safe_delete(tmp);

							CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
							return -1;
						}
						else {
							if (gmspawntype_dump == 0 || npc_dump[i].gmspawntype_index >= gmspawntype_count) {
								cerr << "Error in LoadZoneState: (gmspawntype_dump == 0 || index >= count) && npc_dump[" << i << "].npctype_id == 0" << endl;
								safe_delete(tmp);

								CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
								return -1;
							}
							NPCType* crap = (NPCType*) new uchar[sizeof(NPCType)]; // this is freed in the NPC deconstructor
							memcpy(crap, &gmspawntype_dump[npc_dump[i].gmspawntype_index], sizeof(NPCType));
							npc_loaded[i] = new NPC(crap, tmp, npc_dump[i].x, npc_dump[i].y, npc_dump[i].z, npc_dump[i].heading, npc_dump[i].corpse);
						}
					}
					else {
						NPCType* crap = Database::Instance()->GetNPCType(npc_dump[i].npctype_id);
						if (crap != 0)
							npc_loaded[i] = new NPC(crap, tmp, npc_dump[i].x, npc_dump[i].y, npc_dump[i].z, npc_dump[i].heading, npc_dump[i].corpse);
						else {
							cerr << "Error in LoadZoneState: Unknown npctype_id: " << npc_dump[i].npctype_id << endl;
							safe_delete(tmp);
						}
					}
					if (npc_loaded[i] != 0) {
						npc_loaded[i]->AddCash(npc_dump[i].copper, npc_dump[i].silver, npc_dump[i].gold, npc_dump[i].platinum);
						//							if (npc_dump[i].corpse) {
						//								if (npc_dump[i].decay_time_left <= elapsedtime)
						//									npc_loaded[i]->SetDecayTimer(0);
						//								else
						//									npc_loaded[i]->SetDecayTimer(npc_dump[i].decay_time_left - elapsedtime);
						//							}
						entity_list.AddNPC(npc_loaded[i]);
					}
				}
			}

			npcloot_count = atoi(row[2]);
			cout << "npcloot_count: " << npcloot_count << endl;
			if (lengths[6] != (sizeof(ZSDump_NPC_Loot) * npcloot_count)) {
				cerr << "Error in LoadZoneState: npcloot_dump length mismatch l=" << lengths[6] << ", e=" << (sizeof(ZSDump_NPC_Loot) * npcloot_count) << endl;
				CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
				return -1;
			}
			else if (npcloot_count > 0) {
				if (npc_loaded == 0) {
					cerr << "Error in LoadZoneState: npcloot_count > 0 && npc_loaded == 0" << endl;
					CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
					return -1;
				}
				npcloot_dump = new ZSDump_NPC_Loot[npcloot_count];
				memcpy(npcloot_dump, row[6], lengths[6]);
				for (i=0; i < npcloot_count; i++) {
					if (npcloot_dump[i].npc_dump_index >= npc_count) {
						cerr << "Error in LoadZoneState: npcloot_dump[" << i << "].npc_dump_index >= npc_count" << endl;
						CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
						return -1;
					}
					if (npc_loaded[npcloot_dump[i].npc_dump_index] != 0) {
						npc_loaded[npcloot_dump[i].npc_dump_index]->AddItem(npcloot_dump[i].itemid, npcloot_dump[i].charges, npcloot_dump[i].equipSlot);
					}
				}
			}
			CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
		}
		else {
			CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
			return 0;
		}
		CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
	}
	else {
		cerr << "Error in LoadZoneState query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return -1;
	}

	return 1;
}

void CleanupLoadZoneState(int32 spawn2_count, ZSDump_Spawn2** spawn2_dump, ZSDump_NPC** npc_dump, ZSDump_NPC_Loot** npcloot_dump, NPCType** gmspawntype_dump, Spawn2*** spawn2_loaded, NPC*** npc_loaded, MYSQL_RES** result) {
	safe_delete(*spawn2_dump);
	safe_delete(*spawn2_loaded);
	safe_delete(*gmspawntype_dump);
	safe_delete(*npc_dump);
	safe_delete(*npc_loaded);
	safe_delete(*npcloot_dump);
	if (*result) {
		mysql_free_result(*result);
		*result = 0;
	}
}

Spawn2* Database::LoadSpawn2(LinkedList<Spawn2*> &spawn2_list, int32 spawn2id, int32 timeleft) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, spawngroupID, x, y, z, heading, respawntime, variance, roamRange, pathgrid FROM spawn2 WHERE id=%i", spawn2id), errbuf, &result))
	{
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			Spawn2* newSpawn = new Spawn2(atoi(row[0]), atoi(row[1]), atof(row[2]), atof(row[3]), atof(row[4]), atof(row[5]), atoi(row[6]), atoi(row[7]), timeleft, atoi(row[8]), atoi(row[9]));
			spawn2_list.Insert( newSpawn );
			mysql_free_result(result);
			safe_delete_array(query);//delete[] query;
			return newSpawn;
		}
		mysql_free_result(result);
	}

	cerr << "Error in LoadSpawn2 query '" << query << "' " << errbuf << endl;
	safe_delete_array(query);//delete[] query;
	return 0;
}

void Zone::SpawnStatus(Mob* client) {
	LinkedListIterator<Spawn2*> iterator(spawn2_list);

	int32 x = 0;
	iterator.Reset();
	while(iterator.MoreElements())	
	{
		if (iterator.GetData()->timer->GetRemainingTime() == 0xFFFFFFFF)
			client->Message(BLACK, "  %d:  %1.1f, %1.1f, %1.1f:  disabled", iterator.GetData()->GetID(), iterator.GetData()->GetX(), iterator.GetData()->GetY(), iterator.GetData()->GetZ());
		else
			client->Message(BLACK, "  %d:  %1.1f, %1.1f, %1.1f:  %1.2f", iterator.GetData()->GetID(), iterator.GetData()->GetX(), iterator.GetData()->GetY(), iterator.GetData()->GetZ(), (float)iterator.GetData()->timer->GetRemainingTime() / 1000);

		x++;
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	client->Message(BLACK, "%i spawns listed.", x);
}

// Added By Hogie 
bool Database::GetDecayTimes(npcDecayTimes_Struct* npcCorpseDecayTimes) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	int i = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT varname, value FROM variables WHERE varname like 'decaytime%' ORDER BY varname"), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
		while(row = mysql_fetch_row(result)) {
			Seperator sep(row[0]);
			npcCorpseDecayTimes[i].minlvl = atoi(sep.arg[1]);
			npcCorpseDecayTimes[i].maxlvl = atoi(sep.arg[2]);
			if (atoi(row[1]) > 7200)
				npcCorpseDecayTimes[i].seconds = 720;
			else
				npcCorpseDecayTimes[i].seconds = atoi(row[1]);
			i++;
		}
		mysql_free_result(result);
	}
	else {
		safe_delete_array(query);//delete[] query;
		return false;
	}
	return true;
}
// Added By Hogie -- End
/********************************************************************
*                        Tazadar - 05/27/08                        *
********************************************************************
*                          weatherProc                             *
********************************************************************
*  + Start the time to trigger the next weather(sun or rain/snow)  *
*                                                                  *
********************************************************************/

void Zone::weatherProc()
{
	if(time(0)>=weather_timer && weather_type != 0x00)
	{
		if(zone_weather==0)
			//weather_timer=time(0)+(rand()%(20))+30;//to test the weather
			weather_timer=time(0)+(rand()%(2400-30))+30;//real time (dunno if its the correct classic time)
		else
			//weather_timer=time(0)+(rand()%(20))+30;//to test the weather
			weather_timer=time(0)+(rand()%(3600-30))+30;//real time (dunno if its the correct classic time)
		if(zone_weather>0)
			zone_weather=0;
		else
			zone_weather=weather_type;
		weatherSend();
		cout << "Weather changes in " << weather_timer-time(0) << " seconds. (weather is now " << (long)zone_weather << ")" << endl;
	}
}

/********************************************************************
*                        Tazadar - 05/27/08                        *
********************************************************************
*                          weatherSend                             *
********************************************************************
*  + Send the weather to the server once the timer is over         *
*                                                                  *
********************************************************************/
void Zone::weatherSend()
{
	/*switch(zone_weather)
	{
	case 0:
	entity_list.Message(0, BLACK, "The sky clears.");
	break;
	case 1:
	entity_list.Message(0, BLACK, "Raindrops begin to fall from the sky.");
	break;
	case 2:
	entity_list.Message(0, BLACK, "Snowflakes begin to fall from the sky.");
	break;
	default:
	entity_list.Message(0, BLACK, "Strange weather patterns form in the sky. (%i)", zone_weather);
	break;
	}*/
	APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
	memset(outapp->pBuffer, 0, 8);
	if(zone_weather>0){
		outapp->pBuffer[4] = 0x10+(rand()%10); // Tazadar : This number changes in the packets, intensity?(I changed it but nothing happens)
		if(weather_type==2){
			outapp->pBuffer[0] = 0x01;
		}
	}
	if(zone_weather==0 && weather_type==2){
		memset(outapp->pBuffer, 0, 8);
		outapp->size = 8; // To shutoff weather you send an empty 8 byte packet (You get this everytime you zone even if the sky is clear)
		outapp->pBuffer[0] = 0x01; // Snow has it's own shutoff packet
		entity_list.Message(0, BLACK, "The sky clears as the snow stops falling.");
	}
	if(zone_weather==0 && weather_type==1){
		entity_list.Message(0, BLACK, "The sky clears as the rain ceases to fall.");
	}

	entity_list.QueueClients(0, outapp);
	safe_delete(outapp);
}

//o--------------------------------------------------------------
//| createDirectedGraph; Yeahlight, June 30, 2008
//o--------------------------------------------------------------
//| Creates a directed graph from a supplied node file
//o--------------------------------------------------------------
void Entity::createDirectedGraph()
{
	float x, y, z;
	int16 waterFlag;
	int16 distance;
	char fileNameSuffix[] = "Nodes.txt";
	char fileNamePrefix[100] = "./Maps/Nodes/";
	strcat(fileNamePrefix, zone->GetShortName());
	strcat(fileNamePrefix, fileNameSuffix);
	zoneNode tempNodes[5000] = {0}; //Yeahlight: TODO: 5,000 may not be enough nodes for some zones
	zoneNode thisZonesNodes[5000] = {0};
	ifstream myfile (fileNamePrefix);
	int i = -1;
	if (myfile.is_open())
	{
		while (! myfile.eof() )
		{
			i++;
			myfile>>x>>y>>z>>waterFlag>>distance;
			tempNodes[i].nodeID = i;
			tempNodes[i].x = x;
			tempNodes[i].y = y;
			tempNodes[i].z = z;
			tempNodes[i].waterNodeFlag = waterFlag;
		}
		myfile.close();
	}
	else
	{
		myfile.close();
		return;
	}

	char fileNameSuffix2[] = "DirectedGraph.txt";
	char fileNamePrefix2[100] = "./Maps/Graphs/";
	strcat(fileNamePrefix2, zone->GetShortName());
	strcat(fileNamePrefix2, fileNameSuffix2);
	ofstream myfile2;
	myfile2.open(fileNamePrefix2);

	for(int ix = 0; ix <= i; ix++){
		cout<<"On: "<<ix<<" of "<<i<<" with a waterFlag of: "<<tempNodes[ix].waterNodeFlag<<endl;
		myfile2<<"<NID> "<<ix<<" "<<tempNodes[ix].x<<" "<<tempNodes[ix].y<<" "<<tempNodes[ix].z<<endl;
		//Yeahlight: If this is a "waterNode," then we need to find its paired node and grant it LoS
		if(tempNodes[ix].waterNodeFlag == 1 || tempNodes[ix].waterNodeFlag == 2){
			cout<<"Water node found"<<endl;
			for(int iz = 0; iz <= i; iz++){
				if(iz == ix)
					continue;
				if(tempNodes[ix].x == tempNodes[iz].x && tempNodes[ix].y == tempNodes[iz].y && abs(tempNodes[ix].z - tempNodes[iz].z) < 20){
					cout<<"Water node's pair found"<<endl;
					myfile2<<" <N> "<<iz<<endl;
					myfile2<<"  <C> "<<abs(tempNodes[ix].z - tempNodes[iz].z)<<endl;
				}	
			}
		}

		for(int iy = 0; iy <= i; iy++){
			float distance = sqrt((double)(pow(tempNodes[ix].x - tempNodes[iy].x, 2) + pow(tempNodes[ix].y - tempNodes[iy].y, 2) + pow(tempNodes[ix].z - tempNodes[iy].z, 2)));
			if(distance < 300 && CheckCoordLosNoZLeaps(tempNodes[ix].x,tempNodes[ix].y,tempNodes[ix].z,tempNodes[iy].x,tempNodes[iy].y,tempNodes[iy].z) && ix != iy){
				myfile2<<" <N> "<<iy<<endl;
				myfile2<<"  <C> "<<distance<<endl;
			}
		}
		myfile2<<"</NID>"<<endl;
	}
	myfile2.close();
}

//o--------------------------------------------------------------
//| loadNodesIntoMemory; Yeahlight, June 30, 2008
//o--------------------------------------------------------------
//| Pulls the data from the directed graph into memory
//o--------------------------------------------------------------
bool Zone::loadNodesIntoMemory(){

	for(int i = 0; i < MAXZONENODES; i++) {
		thisZonesNodes[i] = new Node();
	}
	char fileNameSuffix[] = "DirectedGraph.txt";
	char fileNamePrefix[100] = "./Maps/Graphs/";
	strcat(fileNamePrefix, this->GetShortName());
	strcat(fileNamePrefix, fileNameSuffix);

	float x, y, z;
	string buffer = "";
	ifstream myfile (fileNamePrefix);
	int16 nodeCounter = -1;
	int16 number;
	if (myfile.is_open())
	{
		while (! myfile.eof() )
		{
			myfile>>buffer;
			if(buffer.compare("<NID>") == 0) {
				myfile>>number>>x>>y>>z;
				nodeCounter++;
				thisZonesNodes[nodeCounter]->nodeID = number;
				thisZonesNodes[nodeCounter]->x = x;
				thisZonesNodes[nodeCounter]->y = y;
				thisZonesNodes[nodeCounter]->z = z;
				thisZonesNodes[nodeCounter]->distanceFromCenter = sqrt((float)(pow(x - 0, 2) + pow(y - 0, 2) + pow(z - 0, 2)));
			}
		}
	}
	else
	{
		myfile.close();
		numberOfNodes = 0;
		return false;
	}
	myfile.close();
	int16 onNode = 0;
	int16 childCount = 0;
	float cost = 0;

	ifstream myfile2 (fileNamePrefix);
	if (myfile2.is_open())
	{
		while (! myfile2.eof() )
		{
			myfile2>>buffer;
			if(buffer.compare("<NID>") == 0) {
				myfile2>>number>>x>>y>>z;
				onNode = number;
			}
			if(buffer.compare("<N>") == 0) {
				myfile2>>number;
				thisZonesNodes[onNode]->myChildren[childCount].nodeID = number;
				myfile2>>buffer>>cost;
				thisZonesNodes[onNode]->myChildren[childCount].cost = cost;
				childCount++;
			}
			if(buffer.compare("</NID>") == 0) {
				thisZonesNodes[onNode]->childCount = childCount;
				childCount = 0;
			}
		}
	}
	else
	{
		myfile2.close();
		return false;
	}
	numberOfNodes = onNode;
	myfile2.close();
	return true;
}

//o--------------------------------------------------------------
//| loadNodesPathsMemory; Yeahlight, October 10, 2008
//o--------------------------------------------------------------
//| Loads preprocessed paths into memory
//o--------------------------------------------------------------
bool Zone::loadPathsIntoMemory()
{
	char fileNameSuffix[] = "Paths.txt";
	char fileNamePrefix[100] = "./Maps/Paths/";
	strcat(fileNamePrefix, this->GetShortName());
	strcat(fileNamePrefix, fileNameSuffix);
	string buffer = "";
	string buffer2 = "";
	string buffer3 = "";
	ifstream myfile (fileNamePrefix);
	int16 startID;
	int16 endID;
	int16 nodeID;
	int16 counter;
	if (myfile.is_open())
	{
		while (! myfile.eof() )
		{
			counter = 0;
			myfile>>buffer;
			if(buffer.compare("<N>") == 0) {
				myfile>>startID;
				myfile>>buffer2>>endID;
				myfile>>buffer3>>nodeID;
				while(nodeID != NULL_NODE && counter < MAX_PATH_SIZE - 1)
				{
					this->zonePaths[startID][endID].path[counter] = nodeID;
					counter++;
					myfile>>nodeID;
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				this->zonePaths[startID][endID].path[counter] = NULL_NODE;
			}
		}
	}
	else
	{
		myfile.close();
		this->pathsLoaded = false;
		return false;
	}
	myfile.close();
	this->pathsLoaded = true;
	return true;
}

//o--------------------------------------------------------------
//| LoadPatrollingNodes; Yeahlight, Oct 18, 2008
//o--------------------------------------------------------------
//| Loads the patrolling nodes for a specific zoneID
//o--------------------------------------------------------------
bool Database::LoadPatrollingNodes(int16 zoneID)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int16 counter = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT gridid, number, x, y, z, pause FROM grid_entries WHERE zoneid = '%i'", zoneID), errbuf, &result))
	{
		safe_delete_array(query);
		counter = 0;
		while(row = mysql_fetch_row(result))
		{
			zone->patrollingNodes[counter].gridID = atoi(row[0]);
			zone->patrollingNodes[counter].number = atoi(row[1]);
			zone->patrollingNodes[counter].x = atof(row[2]);
			zone->patrollingNodes[counter].y = atof(row[3]);
			zone->patrollingNodes[counter].z = atof(row[4]);
			zone->patrollingNodes[counter].pause = atoi(row[5]);
			counter++;
		}
		zone->numberOfPatrollingNodes = counter;
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "Error in LoadZoneID query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return false;
}

//o--------------------------------------------------------------
//| ParseGridData; Yeahlight, Oct 19, 2008
//o--------------------------------------------------------------
//| Parses the zone's grid data from the grid_entries table
//| to use the zone's existing node system data.
//o--------------------------------------------------------------
bool Zone::ParseGridData(Mob* parser)
{
	char fileNameSuffix[] = "Grids.txt";
	char fileNamePrefix[100] = "./Maps/Grids/";
	strcat(fileNamePrefix, zone->GetShortName());
	strcat(fileNamePrefix, fileNameSuffix);
	ofstream myfile;
	myfile.open(fileNamePrefix);
	Node* myNode = NULL;
	Node* myDestination = NULL;
	int16 gridID = this->patrollingNodes[0].gridID;
	int16 parseArray[MAXZONENODES];
	int16 parseArrayCounter = 0;

	for(int i = 0; i < this->numberOfPatrollingNodes - 1; i++)
	{
		//Yeahlight: Keep iterating until we find the end of the grid or the end of the grid data
		if(gridID == this->patrollingNodes[i+1].gridID && i+2 != this->numberOfPatrollingNodes)
		{
			//Yeahlight: Assign the patrol data to the NPC
			parser->SetX(zone->patrollingNodes[i].x);
			parser->SetY(zone->patrollingNodes[i].y);
			parser->SetZ(zone->patrollingNodes[i].z + 3);
			myNode = parser->findClosestNode(parser, true);
			if(myNode != NULL)
			{
				//Yeahlight: parseArray is empty, start if off with this value
				if(parseArrayCounter == 0)
				{
					parseArray[0] = myNode->nodeID;
					parseArrayCounter++;
				}
				else
				{
					bool skip = false;
					//Yeahlight: Check to see if this node already exists in the array
					for(int j = 0; j < parseArrayCounter; j++)
					{
						if(myNode->nodeID == parseArray[j])
							skip = true;
					}
					//Yeahlight: The node does not exist in the array; add it
					if(!skip)
					{
						parseArray[parseArrayCounter] = myNode->nodeID;
						parseArrayCounter++;
					}
				}
			}
			continue;
		}

		queue<int16> returnQueue;
		int16 maxSize = 0;
		//Yeahlight: Iterate through all the stored nodes and select the path with the MOST resistance
		for(int k = 0; k < parseArrayCounter; k++)
		{
			for(int j = 0; j < parseArrayCounter; j++)
			{
				if(j == k)
					continue;
				Node* a = zone->thisZonesNodes[parseArray[k]];
				Node* b = zone->thisZonesNodes[parseArray[j]];
				parser->findMyPreprocessedPath(a, b, true);
				//Yeahlight: A larger path has been found; record it
				if(parser->CastToNPC()->GetMyPath().size() > maxSize)
				{
					maxSize = parser->CastToNPC()->GetMyPath().size();
					returnQueue = parser->CastToNPC()->GetMyPath();
				}
			}
		}

		//Yeahlight: If a path has been found for this grid, print it to file
		if(!returnQueue.empty())
		{
			//Yeahlight: We only record the start and ending nodes
			myfile<<"<G> "<<gridID<<endl;
			myfile<<" <P> "<<returnQueue.front()<<" "<<returnQueue.back()<<endl;
		}

		//Yeahlight: Assign variables for next gridID
		gridID = this->patrollingNodes[i+1].gridID;
		parseArrayCounter = 0;
	}

	myfile.close();
	return true;
}

//o--------------------------------------------------------------
//| LoadProcessedGridData; Yeahlight, Oct 19, 2008
//o--------------------------------------------------------------
//| Loads a preprocessed grid file
//o--------------------------------------------------------------
bool Zone::LoadProcessedGridData()
{
	char fileNameSuffix[] = "Grids.txt";
	char fileNamePrefix[100] = "./Maps/Grids/";
	strcat(fileNamePrefix, this->GetShortName());
	strcat(fileNamePrefix, fileNameSuffix);
	string buffer = "";
	string buffer2 = "";
	ifstream myfile (fileNamePrefix);
	int16 startID;
	int16 endID;
	int16 counter;
	int16 gridID;

	//Yeahlight: Set the zone's grid array to "empty"
	for(int i = 0; i < MAX_GRIDS; i++)
	{
		zoneGrids[i].startID = NULL_NODE;
		zoneGrids[i].endID = NULL_NODE;
	}

	if (myfile.is_open())
	{
		while (! myfile.eof() )
		{
			counter = 0;
			myfile>>buffer;
			if(buffer.compare("<G>") == 0)
			{
				myfile>>gridID;
				myfile>>buffer2>>startID>>endID;
				this->zoneGrids[gridID].startID = startID;
				this->zoneGrids[gridID].endID = endID;
			}
		}
	}
	else
	{
		myfile.close();
		this->gridsLoaded = false;
		return false;
	}

	myfile.close();
	this->gridsLoaded = true;
	return true;
}
//


/**************************************
 * Time of Day - Kibanu - 7/30/2009   *
 *------------------------------------*
 * Used for spawn data                *
 **************************************/
void Zone::SetTimeOfDay(bool IsDayTime)
{
	if ( IsDayTime )
		cout << "Time Of Day : Now Day time!" << endl;
	else
		cout << "Time Of Day : Now Night time!" << endl;
	is_daytime = IsDayTime;
}

bool Zone::IsDaytime() {
	return is_daytime;
}


