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
#ifndef SPAWNGROUP_H
#define SPAWNGROUP_H

#include "../common/linked_list.h"
#include "../common/types.h"

class SpawnEntry
{
public:
	SpawnEntry(uint32 in_NPCType, int in_chance );
	~SpawnEntry() { }
	uint32 NPCType;
	int chance;
};

class SpawnGroup
{
public:
	SpawnGroup(uint32 in_id, char* name );
	~SpawnGroup() { }
	uint32 GetNPCType();
	void AddSpawnEntry( SpawnEntry* newEntry );
	uint32 id;
private:
	char name_[64];
    LinkedList<SpawnEntry*> list_;
};

class SpawnGroupList
{
public:
	SpawnGroupList() {};
	~SpawnGroupList() {};

	void AddSpawnGroup(SpawnGroup* newGroup);
	SpawnGroup* GetSpawnGroup(uint32 id);
	bool RemoveSpawnGroup(uint32 in_id);
private:
    LinkedList<SpawnGroup*> list_;
};

#endif
