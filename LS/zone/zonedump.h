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
/*
Below are the blob structures for zone state dumping to the database
-Quagmire

create table zone_state_dump (zonename varchar(16) not null primary key, spawn2_count int unsigned not null default 0, 
npc_count int unsigned not null default 0, npcloot_count int unsigned not null default 0, gmspawntype_count int unsigned not null default 0, 
spawn2 mediumblob, npcs mediumblob, npc_loot mediumblob, gmspawntype mediumblob, time timestamp(14));
*/

#ifndef ZONEDUMP_H
#define ZONEDUMP_H
#include "faction.h"

#pragma pack(1)

struct NPCType
{
    char    name[64];
    char    lastname[70];

    sint32  cur_hp;
    sint32  max_hp;

	float	size;
	float	walkspeed;
	float	runspeed;
    int8    gender;
    int16    race;
    int8    class_;
    int8    bodytype;   // added for targettype support - NEOTOKYO
    int8    deity;
    int8    level;
	int32   npc_id; // rembrant, Dec. 20, 2001
	int8    skills[75]; // socket 12-29-01
	int8	texture;
	int8	helmtexture;
	int32	loottable_id;
	int32	npc_spells_id;
	sint32	npc_faction_id;
	uint32	merchanttype;
    int8    light;
    int8    equipment[9];


	int16	AC;
	int16	Mana;
	int16	ATK;
	int8	STR;
	int8	STA;
	int8	DEX;
	int8	AGI;
	int8	INT;
	int8	WIS;
	int8	CHA;
	sint16	MR;
	sint16	FR;
	sint16	CR;
	sint16	PR;
	sint16	DR;
	int8	haircolor;
	int8	beardcolor;
	int8	eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
	int8	eyecolor2;
	int8	hairstyle;
	int8	title; //Face Overlay? (barbarian only)
	int8	luclinface; // and beard);
	int8    banish;
	int16	min_dmg;
	int16	max_dmg;
	char	npc_attacks[30];
	float	fixedZ;
    int16	d_meele_texture1;
	int16	d_meele_texture2;
	sint32	hp_regen;
	sint32  mana_regen;
	sint32 aggroradius; // added for AI improvement - neotokyo
	bool	ipc;
};

struct ZSDump_Spawn2 {
	int32	spawn2_id;
	int32	time_left;
};

struct ZSDump_NPC {
	int32			spawn2_dump_index;
	int32			gmspawntype_index;
	int32			npctype_id;
	sint32			cur_hp;
	int8			corpse; // 0=no, 1=yes, 2=yes and locked
	int32			decay_time_left;
//	needatype		buffs;		// decided not to save these, would be hard because if expired them on bootup, wouldnt take into account the npcai refreshing them, etc
	float			x;
	float			y;
	float			z;
	float			heading;
	uint32			copper;
	uint32			silver;
	uint32			gold;
	uint32			platinum;
};

struct ZSDump_NPC_Loot {
	int32	npc_dump_index;
	int16	itemid;
	sint8	charges;
	sint16	equipSlot;
};

/*
Below are the blob structures for saving player corpses to the database
-Quagmire

create table player_corpses (id int(11) unsigned not null auto_increment primary key, charid int(11) unsigned not null, 
charname varchar(30) not null, zonename varchar(16)not null, x float not null, y float not null, z float not null,
heading float not null, data blob not null, time timestamp(14), index zonename (zonename));
*/

struct ServerLootItem_Struct {
	uint16	item_nr;
	sint16	equipSlot;
	int8	charges;
	int16	lootslot;
	bool	looted;
};

struct DBPlayerCorpse_Struct {
	int32	crc;
	bool	locked;
	int32	itemcount;
	int32	exp;
	float	size;
	int8	level;
	int8	race;
	int8	gender;
	int8	class_;
	int8	deity;
	int8	texture;
	int8	helmtexture;
	int32	copper;
	int32	silver;
	int32	gold;
	int32	plat;
	ServerLootItem_Struct	items[0];
};

struct Door {
int32	db_id;
int8	door_id;
char	zone_name[16];
char	door_name[16];
float	pos_x;
float	pos_y;
float	pos_z;
float	heading;
int8	opentype;
int32	guildid;
int16	lockpick;
int16	keyitem;
int8	trigger_door;
int8	trigger_type;
uint16	liftheight;

char    dest_zone[16];
float   dest_x;
float   dest_y;
float   dest_z;
float   dest_heading;

};


#pragma pack()

#endif

