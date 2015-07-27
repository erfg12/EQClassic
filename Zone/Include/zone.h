#ifndef ZONE_H
#define ZONE_H

#define ZONE_AUTOSHUTDOWN_DELAY	30000

#include "Mutex.h"
#include "linked_list.h"
#include "types.h"
#include "database.h"
//#include "spawn.h"
#include "mob.h"
#include "zonedump.h"
#include "mob.h"
#include "entity.h"
#include "nodes.h"
class Map;
class WaterMap;
class Children;
class Node;
class ZoneLineNode;

struct ZonePoint 
{
	float x;
	float y;
	float z;
	float target_x;
	float target_y;
	float target_z;
	char  target_zone[16];
	int8  heading;
};

struct zoneNode 
{
	int16 nodeID;
	float x;
	float y;
	float z;
	//Yeahlight: waterNodeFlag: 0 - None; 1 - Above water, 2 - Below water
	int16 waterNodeFlag; 
	int16 distanceFromCenter;
};

struct ZonePath 
{
	int16 path[MAX_PATH_SIZE];
};

struct GridPath
{
	int16 startID;
	int16 endID;
};

struct PatrollingNode
{
	int16	gridID;
	int16	number;
	float	x;
	float	y;
	float	z;
	int16	pause;
};

extern EntityList entity_list;

enum WeatherTypesEnum : int8
{
	WEATHER_OFF = 0,
	WEATHER_RAIN = 1,
	WEATHER_SNOW = 2,
	WEATHER_MANUAL = 3
};

class Zone
{
public:
	static bool Bootup(char* zone_name);
	static void Shutdown(bool quite = false);
	Zone(char* in_short_name, char* in_address, int16 in_port);
	~Zone();

	bool	Init();
	void	LoadZoneCFG(const char* filename);
	bool	SaveZoneCFG(char* filename);
	char*	GetAddress() { return address; }
	char*	GetLongName() { return long_name; }
	char*	GetFileName()	{ return file_name; }
	char*	GetShortName() { return short_name; }
	int16	GetPort() { return port; }
	int16	GetZoneID() { return zoneID; }
	float	safe_x() { return psafe_x; }
	float	safe_y() { return psafe_y; }
	float	safe_z() { return psafe_z; }

	int32	CountSpawn2();
	ZonePoint* GetClosestZonePoint(float x, float y, float z, char* to_name);
	SpawnGroupList* spawn_group_list;

//	NPCType* GetNPCType(uint16 id);
//	Item_Struct* GetItem(uint16 id);

	bool	Process();
	void	DumpAllSpawn2(ZSDump_Spawn2* spawn2dump, int32* spawn2index);
	int32	DumpSpawn2(ZSDump_Spawn2* spawn2dump, int32* spawn2index, Spawn2* spawn2);

	void	Depop();
	void	Repop(int32 delay = 0);
	void	SpawnStatus(Mob* client);
	void	StartShutdownTimer(int32 set_time = ZONE_AUTOSHUTDOWN_DELAY);

	void	AddAggroMob()		{aggroedmobs++;}
	void	DelAggroMob()		{aggroedmobs--;}
	bool    AggroLimitReached()	{return (aggroedmobs>80)?true:false;} //Yeahlight: Setting this value to 80 for now
	sint32	MobsAggroCount()			{return aggroedmobs;}

	Map*	map;
	WaterMap* watermap;
	NewZone_Struct	newzone_data;
//	uchar	zone_header_data[142];
	//WeatherTypesEnum zone_weather; //int8	zone_weather;
	LinkedList<Door_Struct*> door_list; // bad andy
	vector<Object_Struct*> object_list_data;
	vector<Object*> object_list;	
	//Tazadar : all the things we need for the weather :)
	void	weatherProc();
	void	weatherSend();
	void	GetTimeSync();
	time_t	weather_timer;
	int8	weather_type;
	int8	zone_weather;

	//Yeahlight: start zonenodes
	bool	loadNodesIntoMemory();
	Node*	thisZonesNodes[MAXZONENODES];
	int16	numberOfNodes;
	//end zonenodes

	//Yeahlight: start zonelines
	ZoneLineNode*	thisZonesZoneLines[100];
	int16	numberOfZoneLineNodes;
	vector<zoneLine_Struct*> zone_line_data;
	//end zonelines

	//Yeahlight: start roamboxes
	RoamBox_Struct	roamBoxes[100];
	int16	numberOfRoamBoxes;
	//Yeahlight: end roamboxes

	//Yeahlight: start paths
	bool loadPathsIntoMemory();
	ZonePath zonePaths[MAXZONENODES][MAXZONENODES];
	bool pathsLoaded;
	//Yeahlight: end paths

	//Yeahlight: start patrolling nodes
	PatrollingNode patrollingNodes[15000];
	int16	zoneID;
	int16	numberOfPatrollingNodes;
	bool	ParseGridData(Mob* parser);
	bool	LoadProcessedGridData();
	GridPath zoneGrids[MAX_GRIDS];
	bool	gridsLoaded;
	//Yeahlight: end patrolling nodes

	int16	zoneAgro[MAX_ZONE_AGRO];
	int16	zoneAgroCounter;

	bool	GetBulkOnly() {return bulkOnly;}
	int8	GetBindCondition() {return bindCondition;}
	int8	GetLevCondition() {return levCondition;}
	bool	GetOutDoorZone() {return outDoorZone;}

	double	GetAverageProcessTime() { return averageProcessTime; }
	void	SetAverageProcessTime(double in) { averageProcessTime = in; }

	float	GetSafeX() { return safeX; }
	float	GetSafeY() { return safeY; }
	float	GetSafeZ() { return safeZ; }
	float	GetSafeHeading() { return safeHeading; }
	void	SetSafeX(float in) { safeX = in; }
	void	SetSafeY(float in) { safeY = in; }
	void	SetSafeZ(float in) { safeZ = in; }
	void	SetSafeHeading(float in) { safeHeading = in; }

	void	SetTimeOfDay(bool IsDayTime); // Kibanu - Time of Day
	bool	IsDaytime(); // Kibanu - Time of Day

	float	GetSpeed() { return speed; }
	void	SetSpeed(float in) { speed = in; }

protected:
	friend class database;
	LinkedList<Spawn2*> spawn2_list; // CODER new spawn list
	sint32	aggroedmobs;
private:
	char*	short_name;
	char	file_name[16];
	char*	long_name;
	char*	address;
	int16	port;
	float	psafe_x, psafe_y, psafe_z;
	bool	is_daytime; // Kibanu - Time of Day

	Timer*	autoshutdown_timer;

//	LinkedList<Spawn*> spawn_list;
	LinkedList<ZonePoint*> zone_point_list;
	
	Mutex	MZoneLock;
	bool	bulkOnly;
	Timer*	bulkOnly_timer;
	int8	bindCondition;
	int8	levCondition;
	bool	outDoorZone;
	double	averageProcessTime;

	Timer*	zoneStatus_timer;

	float	safeX;
	float	safeY;
	float	safeZ;
	float	safeHeading;
	float	speed;
};

#endif

//
