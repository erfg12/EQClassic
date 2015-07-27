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
#ifndef CLASSES_CH
#define CLASSES_CH
#include "../common/types.h"

#define Array_Class_UNKNOWN 0
#define WARRIOR      1
#define CLERIC       2
#define PALADIN      3
#define RANGER       4
#define SHADOWKNIGHT 5
#define DRUID        6
#define MONK         7
#define BARD         8
#define ROGUE        9
#define SHAMAN      10
#define NECROMANCER 11
#define WIZARD      12
#define MAGICIAN    13
#define ENCHANTER   14
#define BEASTLORD   15
#define Count_Array_Class	16 // used for array defines, must be the max + 1
#define BANKER		16
#define WARRIORGM	17
#define CLERICGM	18
#define PALADINGM	19
#define RANGERGM	20
#define SHADOWKNIGHTGM	21
#define DRUIDGM		22
#define MONKGM		23
#define BARDGM		24
#define ROGUEGM		25
#define SHAMANGM	26
#define NECROMANCERGM	27
#define WIZARDGM	28
#define MAGICIANGM	29
#define ENCHANTERGM	30
#define BEASTLORDGM	31
#define MERCHANT	32

#define warrior_1 1
#define monk_1 64
#define paladin_1 4
#define shadow_1 16
#define bard_1 128
#define cleric_1 2
#define necromancer_1 1024
#define ranger_1 8
#define druid_1 32
#define mage_1 4096
#define wizard_1 2048
#define enchanter_1 8192
#define rogue_1 256
#define shaman_1 512
#define beastlord_1 16384
#define call_1 32768

const char* GetEQClassName(int8 class_, int8 level = 0);
int32 GetArrayEQClass(int8 eqclass);
int8 GetEQArrayEQClass(int8 eqclass);
#endif

