#ifndef SPAWNGROUP_H
#define SPAWNGROUP_H

//#include "linked_list.h"
#include "config.h"
#include <vector>
#include "EQCException.hpp"

// Kibanu - 8/10/2009
enum SPAWN_TIME_OF_DAY : int8
{
	Any_Time = 0,
	Day_Time = 1,
	Night_Time = 2,
	Day_Time_Only = 3,
	Night_Time_Only = 4
};

class SpawnEntry
{
public:
	SpawnEntry( int in_NPCType, int in_chance, SPAWN_TIME_OF_DAY in_time_of_day );
	~SpawnEntry() { }
	bool CheckSpawnLimit(int32 in_NPCTypeID);
	int NPCTypeID;
	int chance;
	SPAWN_TIME_OF_DAY time_of_day;
};

class SpawnGroup
{
public:
	SpawnGroup( int in_id, char name[30] );
	~SpawnGroup() { }
	int GetNPCType(SPAWN_TIME_OF_DAY *tmp_time_of_day);
	void AddSpawnEntry( SpawnEntry* newEntry );
	int id;
private:
	char name_[30];
	struct SEMember
	{
		SEMember (SpawnEntry* se) {
			this->Ptr = se;
			this->UseSpawn = true;
		}
		SpawnEntry*	Ptr;
		bool		UseSpawn;
	};
	vector<SEMember> selist;
};

class SpawnGroupList
{
public:
	SpawnGroupList() {};
	~SpawnGroupList() {};

	void AddSpawnGroup(SpawnGroup* newGroup);
	SpawnGroup* GetSpawnGroup(int id);
private:
	struct SGMember
	{
		SGMember (SpawnGroup* sg) {
			this->Ptr = sg;
			this->id = sg->id;
		}
		SpawnGroup*	Ptr;
		int			id;
	};
	vector<SGMember> sglist;
};

#endif
