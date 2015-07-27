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
#include <stdlib.h>
#include "spawn2.h"
#include "entity.h"
#include "npc.h"
#include "zone.h"
#include "spawngroup.h"
#include "../common/database.h"

extern EntityList entity_list;
extern Zone* zone;
extern Database database;


Spawn2::Spawn2(int32 in_spawn2_id, int32 spawngroup_id, float in_x, float in_y, float in_z, float in_heading, int32 respawn, int32 variance, int32 timeleft, int16 grid)
{
	spawn2_id = in_spawn2_id;
	spawngroup_id_ = spawngroup_id;
	x = in_x;
	y = in_y;
	z = in_z;
	heading = in_heading;
    respawn_ = respawn;
	variance_ = variance;
	grid_ = grid;
	
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
}

Spawn2::~Spawn2()
{
	safe_delete(timer);
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
	if (timer->Check())	{
		timer->Disable();
		
		SpawnGroup* sg = zone->spawn_group_list->GetSpawnGroup(spawngroup_id_);
		if (sg == 0)
			return false;
		
		int32 npcid = sg->GetNPCType();
		if (npcid) {
			const NPCType* tmp = database.GetNPCType(npcid);
			if (tmp) {
				currentnpcid = npcid;
				NPC* npc = new NPC(tmp, this, x, y, z, heading);
				npc->AddLootTable();
				npc->SetGrid(grid_);
				npc->SetSp2(spawngroup_id_);
				entity_list.AddNPC(npc);
				if (grid_ > 0)
					npc->AssignWaypoints(grid_);
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

