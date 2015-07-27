
#include <cstdlib>
#include "spawn2.h"
#include "entity.h"
#include "entitylist.h"
#include "npc.h"
#include "zone.h"
#include "spawngroup.h"
#include "database.h"

extern EntityList entity_list;
extern Zone* zone;

Entity* entity;
Mob* mob;

#define ZONE_WIDE_ROAM 5000

Spawn2::Spawn2(int32 in_spawn2_id, int32 spawngroup_id, float in_x, float in_y, float in_z, float in_heading, int32 respawn, int32 variance, int32 timeleft, int in_roamRange, int16 in_pathgrid)
{
	spawn2_id = in_spawn2_id;
	spawngroup_id_ = spawngroup_id;
	roamRange = in_roamRange;
	//Yeahlight: Table grid_entries has grids for our roamers, we only want to use grids for patrollers (spawn2s with NO roam range)
	if(roamRange == 0)
		myPathGrid = in_pathgrid;
	else
		myPathGrid = 0;
	x = in_x;
	y = in_y;
	if(roamRange == 0)
		z = mob->FindGroundZWithZ(x, y, in_z, 2);
	else
		z = mob->FindGroundZWithZ(x, y, in_z, 200);
	myRoamBox = 999;
	//Yeahlight: roamRange is not zero, meaning the mob is not static (does not stand still; moves around)
	if(roamRange != 0) //newage: I wanna see stuff move
	{
		int tempRange = roamRange;
		//Yeahlight: If tempRange is -1, then the roam range is infinite
		if(tempRange == -1)
			tempRange = ZONE_WIDE_ROAM;
		int abortCounter = 0;
		bool permit = false;
		float tempX, tempY, tempZ;
		//Yeahlight: Try values until we find one within one zone roam box
		do
		{
			//Yeahlight: Calculate new x & y values
			tempX = x + ((rand() % (tempRange * 2)) - tempRange);
			tempY = y + ((rand() % (tempRange * 2)) - tempRange);
			tempZ = mob->FindGroundZWithZ(tempX, tempY, z, 250);
			//Yeahlight: See if our values lie within a roam box
			for(int i = 0; i < zone->numberOfRoamBoxes; i++)
			{
				//Yeahlight: Temp values are within a roam box
				if(tempX < zone->roamBoxes[i].max_x && tempX > zone->roamBoxes[i].min_x && tempY < zone->roamBoxes[i].max_y && tempY > zone->roamBoxes[i].min_y)
				{
					//Yeahlight: Old location has elevated LoS to new location 
					if(entity->CheckCoordLos(tempX, tempY, tempZ + 5, GetX(), GetY(), GetZ() + 75))
					{
						permit = true;
						myRoamBox = i;
					}
				}
			}
			abortCounter++;
			//Yeahlight: Zone freeze debug
			if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
				EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		} 
		while(!permit && abortCounter < 100);
		
		//Yeahlight: abortCounter is less than 100, save values
		if(permit)
		{
			//Yeahlight: Assign our new, approved values
			x = tempX;
			y = tempY;
			z = tempZ;
		}
		//Yeahlight: abortCounter is greater than 100, use old values
		else
		{
			//Yeahlight: TODO: Handle this better (log the NPC)
		}
	}
	heading = in_heading;
    respawn_ = respawn;
	variance_ = variance;
	if (spawn2_id == 1720) {
		int i = 0;
	}

	timer = new Timer( resetTimer() );
	if (timeleft == 0xFFFFFFFF) {
		timer->Disable();
	}
	else if (timeleft == 0) {
		timer->Trigger();
	}
	else {
		timer->Start(timeleft);
	}
	
	//Yeahlight: NPC is a patroller and the zone has a path resolved for this gridID
	if(myPathGrid > 0 && zone->gridsLoaded && zone->zoneGrids[myPathGrid].startID != NULL_NODE && zone->zoneGrids[myPathGrid].endID != NULL_NODE)
	{
		int16 randomNodeID;
		if(rand()%10 < 5)
			randomNodeID = zone->zoneGrids[myPathGrid].startID;
		else
			randomNodeID = zone->zoneGrids[myPathGrid].endID;
		x = zone->thisZonesNodes[randomNodeID]->x;
		y = zone->thisZonesNodes[randomNodeID]->y;
		z = zone->thisZonesNodes[randomNodeID]->z;
		myPathGridStart = randomNodeID;
	}
	else
	{
		myPathGridStart = 0;
	}
}

Spawn2::~Spawn2()
{
	safe_delete(timer);//delete timer;
}

int32 Spawn2::resetTimer()
{
	int32 rspawn = respawn_ * 1000;

	if (variance_ != 0) {
		int32 vardir = (rand()%50);
		int32 variance = (rand()%variance_);
		float varper = variance*0.01;
		float varvalue = varper*(rspawn);
		if (vardir < 50)
		{
		  varvalue = varvalue * -1;
		}

		rspawn += (int32) varvalue;
	}

	return (rspawn);

}

bool Spawn2::Process() {
	//Yeahlight: This is where an NPC spawn/respawn happens
	if (timer->Check())	{
		timer->Disable();

		SpawnGroup* sg = zone->spawn_group_list->GetSpawnGroup(spawngroup_id_);
		if (sg == 0)
			return false;

		SPAWN_TIME_OF_DAY tmp_time_of_day;

		int32 npcid = sg->GetNPCType(&tmp_time_of_day);
		if (npcid) {
			NPCType* tmp = Database::Instance()->GetNPCType(npcid);
			//Yeahlight: Preventing boats from autospawning for Tazadar
			if (tmp && tmp->race != 72 && tmp->race != 73) {
				tmp->time_of_day = tmp_time_of_day; // Kibanu
				NPC* npc = new NPC(tmp, this, x, y, z, heading, false, roamRange, myRoamBox, myPathGrid, myPathGridStart);
				npc->AddLootTable();
				npc->GetMeleeWeapons();
				npc->CalcBonuses(true, true);
				entity_list.AddNPC(npc);
			}
		}
		else {
			Reset();
		}
	}
	return true;
}

void Spawn2::Reset()
{
	timer->Start(resetTimer());
}

void Spawn2::Depop() {
	timer->Disable();
}

void Spawn2::Repop(int32 delay) {
	if (delay == 0)
		timer->Trigger();
	else
		timer->Start(delay);
}

