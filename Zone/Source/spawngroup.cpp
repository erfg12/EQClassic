
#include "spawngroup.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include "EQCUtils.hpp"
#include "zone.h"
#include "EntityList.h"
#include <vector>

extern Zone*		zone;
extern EntityList	entity_list;

SpawnEntry::SpawnEntry( int in_NPCType, int in_chance, SPAWN_TIME_OF_DAY in_time_of_day ) 
{
	NPCTypeID = in_NPCType;
	chance = in_chance;
	time_of_day = in_time_of_day;
}

SpawnGroup::SpawnGroup( int in_id, char name[30] )
{
	id = in_id;
	strcpy( name_, name );
}

bool CheckSpawnLimit(int32 in_NPCTypeID)
{
	if ( in_NPCTypeID <= 0 )
		return false;

	// check limit from database
	int db_limit = Database::Instance()->CheckNPCTypeSpawnLimit(in_NPCTypeID);
	if (db_limit > 0) // Doing this check first because we want to avoid a zone entity_list iteration if we can help it
	{
		// There should be a limit now, let's check to see how many exist in the zone
		if ( entity_list.GetNPCTypeCount(in_NPCTypeID) >= db_limit )
			return false; // Hit the cap, don't allow any more to spawn!
	}
	return true;
}

int SpawnGroup::GetNPCType(SPAWN_TIME_OF_DAY *tmp_time_of_day)
{
	if ( !zone ) return -1;

	int npcTypeid = -1;
	int totalchance = 0;

	vector<SEMember>::iterator it;

	SpawnEntry* se;

	for ( it = selist.begin(); it < selist.end(); it++ )
	{
		se = it->Ptr;
		if (!se)
		{
			it->UseSpawn = false;
			continue;
		}
		if(se->chance <= 0)
		{
			EQC::Common::PrintF(CP_ZONESERVER, "SpawnEntry NPCType '%i' has 0 percent change of spawning!\n", se->NPCTypeID);
			it->UseSpawn = false;
			continue;
		}
		if ( !CheckSpawnLimit(se->NPCTypeID))
		{
			EQC::Common::PrintF(CP_ZONESERVER, "SpawnEntry NPCType '%i' has reached spawn limit! Skipping new spawn.\n", se->NPCTypeID);
			it->UseSpawn = false;
			continue;
		}
		if ( se->time_of_day != Any_Time )
		{
			// Time of Day check
			if ( ( zone->IsDaytime() && ( se->time_of_day == Night_Time || se->time_of_day == Night_Time_Only ) )
				|| ( !zone->IsDaytime() && ( se->time_of_day == Day_Time || se->time_of_day == Day_Time_Only ) ) )
			{ 
				// EQC::Common::PrintF(CP_ZONESERVER, "SpawnEntry Time of Day check has prevented SpawnEntry from spawning.\n" );
				it->UseSpawn = false;
				continue;
			}
		}
		it->UseSpawn = true;
		totalchance += se->chance;

		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	int roll = 0;
	if(totalchance > 0)
	{
		roll = (rand()%totalchance);
	}

	for (it = selist.begin(); it < selist.end(); it++)
	{
		if ( !it->UseSpawn )
			continue; // Skip it!!
		if (roll < it->Ptr->chance)
		{
			*tmp_time_of_day = it->Ptr->time_of_day;
			npcTypeid = it->Ptr->NPCTypeID;
			break;
		}
		else
		{
			roll = roll - it->Ptr->chance;
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	//CODER  implement random table
	return npcTypeid;
}

void SpawnGroup::AddSpawnEntry( SpawnEntry* newEntry )
{
	if ( newEntry )
		selist.insert(selist.begin(),newEntry);
}

void SpawnGroupList::AddSpawnGroup(SpawnGroup* newGroup)
{
	if ( newGroup )
		sglist.insert(sglist.begin(),newGroup);
}

SpawnGroup* SpawnGroupList::GetSpawnGroup(int in_id)
{
	vector<SGMember>::iterator it;

	for ( it = sglist.begin(); it < sglist.end(); it++ )
	{
		if (it->id == in_id)
			return it->Ptr;

		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
    return 0;  // shouldn't happen, but need to handle it anyway
}
