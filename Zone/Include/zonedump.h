/*
Below are the blob structures for zone state dumping to the database
-Quagmire

create table zone_state_dump (zonename varchar(16) not null primary key, spawn2_count int unsigned not null default 0, 
npc_count int unsigned not null default 0, npcloot_count int unsigned not null default 0, gmspawntype_count int unsigned not null default 0, 
spawn2 mediumblob, npcs mediumblob, npc_loot mediumblob, gmspawntype mediumblob, time timestamp(14));
*/

#ifndef ZONEDUMP_H
#define ZONEDUMP_H

#pragma pack(1)

#include "spawngroup.h"

struct NPCType
{
    char    name[30];
    char    Surname[20];

    sint32  cur_hp;
    sint32  max_hp;

	float	size;
	float	walkspeed;
	float	runspeed;
    int8    gender;
    int8    race;
    int8    class_;
    int8    deity;
    int8    level;
	int32   npc_id; // rembrant, Dec. 20, 2001
	int16   skills[74]; // socket 12-29-01
	int8	texture;
	int8	helmtexture;
	int32	loottable_id;
	uint32	merchanttype;
    int8    light;
    int8    equipment[9];
	TBodyType	body_type;		//Cofruben: 16/08/08.

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
	int32	min_dmg; //Tazadar 06/01/08 we need to use this for pets dmg
	int32	max_dmg;	//Tazadar 06/01/08 we need to use this for pets dmg
	char	special_attacks[20];
	sint16  attack_speed;
	int16	main_hand_texture;
	int16	off_hand_texture;
	sint16  hp_regen_rate;
	bool	passiveSeeInvis;
	bool	passiveSeeInvisToUndead;
	int8	spawn_limit; // Kibanu - 8/12/2009
	SPAWN_TIME_OF_DAY time_of_day;
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
	bool			corpse;
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
	sint8	charges;
	int16	lootslot;
	bool	looted;
};

struct DBPlayerCorpse_Struct {
	int32	itemcount;
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


#pragma pack()

#endif

