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
#ifndef FACTION_H
#define FACTION_H

#include "../common/types.h"

enum FACTION_VALUE {    FACTION_ALLY = 1, FACTION_WARMLY = 2, FACTION_KINDLY = 3, FACTION_AMIABLE = 4,
                        FACTION_INDIFFERENT = 5, FACTION_APPREHENSIVE = 9, FACTION_DUBIOUS = 8,
                        FACTION_THREATENLY = 7, FACTION_SCOWLS = 6};

#define MAX_NPC_FACTIONS 10	// max factions per list
#define MAX_FACTION	 1500
#define MIN_FACTION -1500

struct NPCFactionList {
	uint32	id;
	uint32	primaryfaction;
	uint32	factionid[MAX_NPC_FACTIONS];
	sint32	factionvalue[MAX_NPC_FACTIONS];
};

struct FactionMods
{
	sint32 base;
	sint32 class_mod;
	sint32 race_mod;
	sint32 deity_mod;
};
struct Faction {
	sint32	id;
	sint16	mod_c[15];
	sint16	mod_r[20];
	sint16	mod_d[17];
	sint16	base;
	char	name[50];
};

struct FactionValue {
	sint32	factionID;
	sint16	value;
};

struct NPCFaction    
{    
sint32 factionID;    
sint32 value_mod;    
bool primary;    
}; 

char* BuildFactionMessage(sint32 tmpvalue, sint32 faction_id, sint32 totalvalue);
FACTION_VALUE CalculateFaction(FactionMods* fm, sint32 tmpCharacter_value);
#endif
