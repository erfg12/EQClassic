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
#include "spawngroup.h"
#include <string.h>
#include <stdlib.h>
#include <iostream.h>
#include "../common/types.h"

SpawnEntry::SpawnEntry( uint32 in_NPCType, int in_chance ) 
{
	NPCType = in_NPCType;
	chance = in_chance;
}

SpawnGroup::SpawnGroup( uint32 in_id, char* name ) {
	id = in_id;
	strcpy( name_, name );
}

uint32 SpawnGroup::GetNPCType() {
#if DEBUG >= 10
	LogFile->write(EQEMuLog::Debug, "SpawnGroup[%08x]::GetNPCType()", (int32) this);
#endif
	int npcType = -1;
	int totalchance = 0;
	
	LinkedListIterator<SpawnEntry*> iterator(list_);
	iterator.Reset();
	while(iterator.MoreElements()) {
		totalchance += iterator.GetData()->chance;
		iterator.Advance();
	}
	sint32 roll = 0;
	if(totalchance != 0)
		roll = (rand()%totalchance);
	else
		return 0;
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (roll < iterator.GetData()->chance) {
			npcType = iterator.GetData()->NPCType;
			break;
		}
		else {
			roll -= iterator.GetData()->chance;
		}
		iterator.Advance();
	}
	//CODER  implement random table
	return npcType;
}

void SpawnGroup::AddSpawnEntry( SpawnEntry* newEntry ) {
	list_.Insert( newEntry );
}

void SpawnGroupList::AddSpawnGroup(SpawnGroup* newGroup) {
	list_.Insert(newGroup);
}

SpawnGroup* SpawnGroupList::GetSpawnGroup(uint32 in_id) {
	LinkedListIterator<SpawnGroup*> iterator(list_);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->id == in_id)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
    return 0;  // shouldn't happen, but need to handle it anyway
}

bool SpawnGroupList::RemoveSpawnGroup(uint32 in_id) {
	LinkedListIterator<SpawnGroup*> iterator(list_);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->id == in_id)
		{
			iterator.RemoveCurrent();
			return true;
		}
		iterator.Advance();
	}
    return 0;  // shouldn't happen, but need to handle it anyway
}

