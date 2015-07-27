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
#ifndef RACES_H
#define RACES_H
#include "../common/types.h"

#define HUMAN			  1
#define BARBARIAN		  2
#define ERUDITE			  3
#define WOOD_ELF		  4
#define HIGH_ELF		  5
#define DARK_ELF		  6
#define HALF_ELF		  7
#define DWARF			  8
#define TROLL			  9
#define OGRE			 10
#define HALFLING		 11
#define GNOME			 12
#define WEREWOLF		 14
#define SKELETON		 60
#define ELEMENTAL		 75
#define EYE_OF_ZOMM		108
#define WOLF_ELEMENTAL		120
#define IKSAR			128
#define VAHSHIR			130
#define IKSAR_SKELETON		161
#define EMU_RACE_NPC		65533
#define EMU_RACE_PET		65534
#define EMU_RACE_UNKNOWN	65535

#define human_1			1
#define barbarian_1		2
#define erudite_1		4
#define woodelf_1		8
#define highelf_1		16
#define darkelf_1		32
#define halfelf_1		64
#define dwarf_1			128
#define troll_1			256
#define ogre_1			512
#define halfling_1		1024
#define gnome_1			2048
#define iksar_1			4096
#define vahshir_1		8192
#define rall_1			16384

const char* GetRaceName(int16 race);

int32 GetArrayRace(int16 race);

#define Array_Race_UNKNOWN		0
#define Array_Race_HUMAN		1
#define Array_Race_BARBARIAN	2
#define Array_Race_ERUDITE		3
#define Array_Race_WOOD_ELF		4
#define Array_Race_HIGH_ELF		5
#define Array_Race_DARK_ELF		6
#define Array_Race_HALF_ELF		7
#define Array_Race_DWARF		8
#define Array_Race_TROLL		9
#define Array_Race_OGRE			10
#define Array_Race_HALFLING		11
#define Array_Race_GNOME		12
#define Array_Race_IKSAR		13
#define Array_Race_VAHSHIR		14
#define Array_Race_NPC			15
#define Array_Race_PET			16
#define Count_Array_Race		17 // used for array defines, must be the max + 1

#endif
