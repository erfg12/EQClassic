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
#ifndef EQ_PACKET_STRUCTS_H
#define EQ_PACKET_STRUCTS_H 

#include "types.h"
#include <time.h>
#include <string.h>

#define BUFF_COUNT 15

/*
** Compiler override to ensure
** byte aligned structures
*/
#pragma pack(1)
//#define INVERSEXY = 1

/* Name Approval Struct */
/* Len: */
/* Opcode: 0x8B20*/
struct NameApproval
{
	char name[64];
	int32 race;
	int32 class_;
	int32 deity;
};

struct GlobalID_Struct
{
int16 entity_id;
};

struct Duel_Struct
{
int16 duel_initiator;
int16 duel_target;
};

struct DuelResponse_Struct
{
int16 target_id;
int16 entity_id;
int8 response[2];
};
/*
** Character Selection Struct
** Length: 1620 Bytes
**
*/
struct CharacterSelect_Struct {
/*0000*/	char	name[10][64];		// Characters Names
/*0640*/	int8	level[10];			// Characters Levels
/*0650*/	int8	class_[10];			// Characters Classes
/*0660*/	int16	race[10];			// Characters Race
/*0680*/	int32	zone[10];			// Characters Current Zone
/*0720*/	int8	gender[10];			// Characters Gender
/*0730*/	int8	face[10];			// Characters Face Type
/*0740*/	int32	equip[10][9];		// 0=helm, 1=chest, 2=arm, 3=bracer, 4=hand, 5=leg, 6=boot, 7=melee1, 8=melee2
/*1100*/	int32	cs_colors[10][9];	// Characters Equipment Colors (RR GG BB 00)
/*1460*/	int8	cs_unknown[160];	// ***Placeholder
};

/*
** Server Zone Entry Struct
** Length: 356 Bytes
**
*/
struct ServerZoneEntry_Struct {
/*0000*/	int8	checksum[4];		// Checksum
/*0004*/	int8	unknown0004;		// ***Placeholder
/*0005*/	char	name[64];			// Name
/*0069*/	int8	sze_unknown0069;	// ***Placeholder
/*0070*/	int16	unknown0070;		// ***Placeholder
/*0072*/	int32	zone;				// Current Zone
#ifndef INVERSEXY
/*0076*/	float	x;					// X Position (Not Inversed)
/*0080*/	float	y;					// Y Position (Not Inversed)
#else
/*0076*/	float	y;					// Y Position (Inversed)
/*0080*/	float	x;					// X Position (Inversed)
#endif
/*0084*/	float	z;					// Z Position
/*0088*/	float	heading;			// Heading
/*0092*/	int32	unknown0092[18];	// ***Placeholder
/*0164*/	int16	guildeqid;			// Guild ID Number
/*0166*/	int8	unknown0166[7];		// ***Placeholder
/*0173*/	int8	class_;				// Class
/*0174*/	int16	race;				// Race
/*0176*/	int8	gender;				// Gender
/*0177*/	int8	level;				// Level
/*0178*/	int16	unknown0178;		// ***Placeholder	
/*0180*/	int8	pvp;				// PVP Flag
/*0181*/	int16	unknown0181;		// ***Placeholder
/*0183*/	int8	face;				// Face Type
/*0184*/	int16	helmet;				// Helmet Visual
/*0186*/	int16	chest;				// Chest Visual
/*0188*/	int16	arms;				// Arms Visual
/*0190*/	int16	bracers;			// Bracers Visual
/*0192*/	int16	hands;				// Hands Visual
/*0194*/	int16	legs;				// Legs Visual
/*0196*/	int16	feet;				// Feet Visual
/*0198*/	int16	melee1;				// Weapon 1 Visual
/*0200*/	int16	melee2;				// Weapon 2 Visual
/*0202*/	int16	unknown;			// ***Placeholder
/*0204*/	int32	helmet_color;		// Helmet Visual Color
/*0208*/	int32	chest_color;		// Chest Visual Color
/*0212*/	int32	arms_color;			// Arms Visual Color
/*0216*/	int32	bracers_color;		// Bracers Visual Color
/*0220*/	int32	hands_color;		// Hands Visual Color
/*0224*/	int32	legs_color;			// Legs Visual Color
/*0228*/	int32	feet_color;			// Feet Visual Color
/*0232*/	int32	melee1_color;		// Weapon 1 Visual Color
/*0236*/	int32	melee2_color;		// Weapon 2 Visual Color
/*0240*/	int8	npc_armor_graphic;	// Texture (0xFF=Player - See list of textures for more)
/*0241*/	int8	unknown0241[19];	// ***Placeholder
/*0260*/	float	walkspeed;			// Speed when you walk
/*0264*/	float	runspeed;			// Speed when you run
/*0268*/	int32	unknown0268[3];		// ***Placeholder (At least one flag in here disables a zone point or all)
/*0280*/	int8	anon;				// Anon. Flag
/*0281*/	int8	unknown0281[23];	// ***Placeholder (At least one flag in here disables a zone point or all)
/*0304*/	char	lastname[34];		// Lastname (This has to be wrong.. but 70 is to big =/..)
/*0338*/	int16	deity;				// Diety (Who you worship for those less literate)
/*0340*/	int8	unknown0340;		// ***Placeholder
/*0341*/	int8	haircolor;			// Hair Color
/*0342*/	int8	beardcolor;			// Beard Color
/*0343*/	int8	eyecolor1;			// Left Eye Color
/*0344*/	int8	eyecolor2;			// Right Eye Color
/*0345*/	int8	hairstyle;			// Hair Style
/*0346*/	int8	title;				// AA Title
/*0347*/	int8	luclinface;			// Face Type
/*0348*/	int8	skyradius;			// Skyradius (Lyenu: Not sure myself what this is)
/*0349*/	int8	unknown321[7];		// ***Placeholder
};

/*
** New Zone Struct
** Length: 452 Bytes
**
*/
struct NewZone_Struct {
/*0000*/	char	char_name[64];			// Character Name
/*0064*/	char	zone_short_name[32];	// Zone Short Name
/*0096*/	char	zone_long_name[278];	// Zone Long Name
/*0278*/	int8	unknown230[100];		// ***Placeholder
/*0378*/	int8	sky;					// Sky Type
/*0379*/	int8	unknown331[9];			// ***Placeholder
/*0388*/	float	zone_exp_multiplier;	// Experience Multiplier
#ifndef INVERSEXY
/*0392*/	float	safe_x;					// Zone Safe X (Not Inversed)
/*0396*/	float	safe_y;					// Zone Safe Y (Not Inversed)
#else
/*0392*/	float	safe_y;					// Zone Safe Y (Inversed)
/*0396*/	float	safe_x;					// Zone Safe X (Inversed)
#endif
/*0400*/	float	safe_z;					// Zone Safe Z
/*0404*/	float	unknown256;				// ***Placeholder
/*0408*/	float	underworld;				// Underworld (Not Sure?)
/*0412*/	float	minclip;				// Minimum View Distance
/*0416*/	float	maxclip;				// Maximum View DIstance
/*0420*/	int8	unknown_end[56];		// ***Placeholder
};

struct CFGNewZone_Struct {
/*0000*///	char	char_name[64];			// Character Name
/*0064*/	char	zone_short_name[32];	// Zone Short Name
/*0096*/	char	zone_long_name[278];	// Zone Long Name
/*0278*/	int8	unknown230[100];		// ***Placeholder
/*0378*/	int8	sky;					// Sky Type
/*0379*/	int8	unknown331[9];			// ***Placeholder
/*0388*/	float	zone_exp_multiplier;	// Experience Multiplier
#ifndef INVERSEXY
/*0392*/	float	safe_x;					// Zone Safe X (Not Inversed)
/*0396*/	float	safe_y;					// Zone Safe Y (Not Inversed)
#else
/*0392*/	float	safe_y;					// Zone Safe Y (Inversed)
/*0396*/	float	safe_x;					// Zone Safe X (Inversed)
#endif
/*0400*/	float	safe_z;					// Zone Safe Z
/*0404*/	float	unknown256;				// ***Placeholder
/*0408*/	float	underworld;				// Underworld (Not Sure?)
/*0412*/	float	minclip;				// Minimum View Distance
/*0416*/	float	maxclip;				// Maximum View DIstance
/*0420*/	int8	unknown_end[56];		// ***Placeholder
};

struct Discipline_Struct {
/*0000*/	int8 unknown0000[4];
/*0004*/	char  charname[64];
/*0068*/	char  charname2[64];
/*0132*/	int32 unknown0132;
};

/*
** Memorize Spell Struct
** Length: 12 Bytes
**
*/
struct MemorizeSpell_Struct { 
int32 slot;     // Spot in the spell book/memorized slot 
int32 spell_id; // Spell id (200 or c8 is minor healing, etc) 
int32 scribing; // 1 if memorizing a spell, set to 0 if scribing to book 
}; 

/*
** Spell Cast On Struct
** Length: 36 Bytes
**
*/
struct CastOn_Struct {
/*00*/	int16	target_id;
/*02*/	int16	source_id;
/*04*/	int8	source_level;
/*05*/	int8	unknown1[3];
/*08*/	int8	unknown2; // = 0A
/*09*/	int8	unknown_zero1[7];
/*16*/	float	heading;
/*20*/	int8	unknown_zero2[4];
/*24*/	int32	action;
/*28*/	int8	unknown3[2];
/*30*/	int16	spell_id;
/*32*/	int8	unknown4[4];
};

/*
** Make Charmed Pet
** Length: 6 Bytes
**
*/
struct Charm_Struct {
/*00*/	int16	owner_id;
/*02*/	int16	pet_id;
/*04*/	int16	command;    // 1: make pet, 0: release pet
};

struct InterruptCast_Struct
{
	int16	message; //message id
	int16	color;  
	int16	unknown2;
	//char	message[0];
};

struct DeleteSpell_Struct
{
/*000*/sint16	spell_slot;
/*002*/int8	unknowndss002[2];
/*004*/int8	success;
/*005*/int8	unknowndss006[3];
/*008*/
};

struct ManaChange_Struct
{
uint16 new_mana;                  // New Mana AMount
uint16 spell_id;                  // Last Spell Cast
};

struct SwapSpell_Struct 
{ 
int32 from_slot; 
int32 to_slot; 
}; 

struct BeginCast_Struct
{
	// len = 8
/*000*/	int16	caster_id;

/*002*/	int16	spell_id;
/*004*/	int32	cast_time;		// in miliseconds
};

struct Buff_Struct
{
uint32 target_id;
uint32 b_unknown1;
uint16 spell_id;
uint32 b_unknown2;
uint16 b_unknown3;
uint32 buff_slot;
};

struct CastSpell_Struct
{
	int16	slot;
	int16	spell_id;
	int16	inventoryslot;  // slot for clicky item, 0xFFFF = normal cast
	int16	target_id;
	//int32	cs_unknown2;
	int8    cs_unknown[4];
};

#define AT_Level		1	// the level that shows up on /who
/*2 = No Effect?*/
#define AT_Invis		3
#define AT_PVP			4
#define AT_Light		5
/*6 = No Effect?*/
#define AT_Position		14	// 100=standing, 110=sitting, 111=ducking, 115=feigned, 105=looting
#define AT_SpawnID		15	// i guess, kinda looks like that's what it's doing
#define AT_LD			18
#define AT_HP			17	// Client->Server, my HP has changed (like regen tic)
#define AT_Levitate		19	// 0=off, 1=flymode, 2=levitate
#define AT_GuildID		22
#define AT_GuildRank	23	// 0=member, 1=officer, 2=leader
#define AT_AFK			24
#define AT_Size			29	// spawn's size
#define AT_NPCName		30	// change PC's name's color to NPC color (unreversable)
#define AT_Trader		31  // Bazzar Trader Mode

struct SpawnAppearance_Struct
{
// len = 8
/*0000*/ uint16 spawn_id;          // ID of the spawn
/*0002*/ uint16 type;              // Type of data sent
                                   // 0x10 = Set ID, 0x23 Guildstatus?, 0x03 Invisible, 0x0e Sit stand ect, 0x15 Anon/Role, 0x11 Tick update/Regen?
/*0004*/ uint32 parameter;         // Values associated with the type
};


/*
** Buffs
** Length: 10 Bytes
** Used in:
**    playerProfileStruct(2d20)
*/
struct SpellBuff_Struct
{
	int8  b_unknown1;			// ***Placeholder
	int8  level;				// Level of person who casted buff
	int8  b_unknown2;			// ***Placeholder
	int8  b_unknown3;			// ***Placeholder
	int16 spellid;				// Spell
	int32 duration;				// Duration in ticks
};

// Length: 24
struct SpellBuffFade_Struct {
/*000*/ int16   player_id;
/*002*/	int8	unknown000[4];
/*006*/	int16	spellid;
/*008*/ int8	unknown008[4];
/*012*/	int32	slotid;
/*016*/ int32	bufffade;	// 1 = True??
};

// Length: 10
struct ItemProperties_Struct {

int8	unknown01[2];
int8	charges;
int8	unknown02[7];
};

struct GMTrainee_Struct {
	/*000*/ int16 npcid;
	/*002*/ int16 playerid;
	/*004*/ int8 unknown000[240];
};

struct GMTrainEnd_Struct {
	/*000*/ int16 npcid;
	/*002*/ int16 unknown;
};

struct GMSkillChange_Struct {
/*000*/	int16		npcid;
/*002*/ int8		unknown1[2];    // something like PC_ID, but not really. stays the same thru the session though
/*002*/ int16       skillbank;      // 0 if normal skills, 1 if languages
/*002*/ int8		unknown2[2];
/*008*/ uint16		skill_id;
/*010*/ int8		unknown3[2];
};

// all strings zero terminated
struct CommonMessage_Struct {
    uint16 unknown[3]; // 00 00 2A 02 0A 00
    char rest[0]; // npcname[at least 16], code[5], pcname[at least 15]
};

struct OpCode4141_Struct {
    uint16 unknown;
};

/* 
** Diety List
*/
#define DEITY_UNKNOWN                   0
#define DEITY_AGNOSTIC			396
#define DEITY_BRELL			202
#define DEITY_CAZIC			203
#define DEITY_EROL			204
#define DEITY_BRISTLE			205
#define DEITY_INNY			206
#define DEITY_KARANA			207
#define DEITY_MITH			208
#define DEITY_PREXUS			209
#define DEITY_QUELLIOUS			210
#define DEITY_RALLOS			211
#define DEITY_SOLUSEK			213
#define DEITY_TRIBUNAL			214
#define DEITY_TUNARE			215

//Guessed:
#define DEITY_BERT			201	
#define DEITY_RODCET			212
#define DEITY_VEESHAN			216

/*
** Player Profile
** Length: 8244 Bytes
** OpCode: 2e20
*/
// TODO:
struct PlayerProfile_Struct
{
/* Global Definitions */
#define pp_inventory_size 30
#define pp_containerinv_size 90
#define pp_bank_inv_size 8
#define pp_bank_cont_inv_size 80
/* ***************** */

/*0000*/	int8	checksum[4];		// Checksum
/*0004*/	int8	unknown0004[2];		// ***Placeholder
/*0006*/	char	name[64];			// Player First Name
/*0070*/	char	last_name[70];		// Player Last Name
/*0140*/	int8	gender;				// Player Gender
/*0141*/	int8	unknown0141;		// ***Placeholder
/*0142*/	int16	race;				// Player Race (Lyenu: Changed to an int16, since races can be over 255)
/*0144*/	int16	class_;				// Player Class
/*0146*/	int8	unknown0145;		// ***Placeholder
/*0147*/	int8	unknown0147;		// ***Placeholder
/*0148*/	int8	level;				// Player Level
/*0149*/	int8	unknown0149;		// ***Placeholder
/*0150*/	int8	unknown0150[2];		// ***Placeholder
/*0152*/	int32	exp;				// Current Experience
/*0156*/	int16	points;				// Players Points
/*0158*/	int16	mana;				// Player Mana
/*0160*/	int16	cur_hp;				// Player Health
/*0162*/	int16	unknown162;         // Players Face
/*0164*/	int16	STR;				// Player Strength
/*0166*/	int16	STA;				// Player Stamina
/*0168*/	int16	CHA;				// Player Charisma
/*0170*/	int16	DEX;				// Player Dexterity
/*0172*/	int16	INT;				// Player Intelligence
/*0174*/	int16	AGI;				// Player Agility
/*0176*/	int16	WIS;				// Player Wisdom
/*0178*/	int8	face;               //
/*0179*/    int8    EquipType[9];       // i think its the visible parts of the body armor
/*0188*/    int32   EquipColor[9];      //
/*0224*/	int16	inventory[30];		// Player Inventory Item Numbers
/*0284*/	int8	languages[26];		// Player Languages
/*0310*/	int8	unknown0310[6];		// ***Placeholder
/*0316*/	struct	ItemProperties_Struct	invitemproperties[30];
										// These correlate with inventory[30]
/*0616*/	struct	SpellBuff_Struct	buffs[BUFF_COUNT];
										// Player Buffs Currently On
/*0766*/	int16	containerinv[pp_containerinv_size];	// Player Items In "Bags"
										// If a bag is in slot 0, this is where the bag's items are
/*0946*/	struct	ItemProperties_Struct	bagitemproperties[pp_containerinv_size];
										// Just like InvItemProperties
/*1846*/	sint16	spell_book[256];	// Player spells scribed in their book
/*2358*/	int8	unknown2374[512];	// ***Placeholder
/*2870*/	sint16	spell_memory[8];	// Player spells memorized
/*2886*/	int8	unknown[18];			// ***Placeholder
#ifndef INVERSEXY
/*2904*/	float	x;					// Player X (Not Inversed)
/*2908*/	float	y;					// Player Y (Not Inversed)
#else
/*2904*/	float	y;					// Player Y (Inversed)
/*2908*/	float	x;					// Player X (Inversed)
#endif
/*2912*/	float	z;					// Player Z
/*2916*/	float	heading;			// Player Heading
/*2920*/	int8	unknown2920[4];		// ***Placeholder
/*2924*/	int32	platinum;			// Player Platinum (Character)
/*2928*/	int32	gold;				// Player Gold (Character)
/*2932*/	int32	silver;				// Player Silver (Character)
/*2936*/	int32	copper;				// Player Copper (Character)
/*2940*/	int32	platinum_bank;		// Player Platinum (Bank)
/*2944*/	int32	gold_bank;			// Player Gold (Bank)
/*2948*/	int32	silver_bank;		// Player Silver (Bank)
/*2952*/	int32	copper_bank;		// Player Copper (Bank)
/*2956*/	int8	unknown2956[30];	// ***Placeholder
/*2986*/	int16	skills[75];			// Player Skills
/*3134*/	int8	unknown3134[280];	// ***Placeholder (ETHQ says there are more skills and boat info, perhaps we need that sometime (neotokyo))
/*3116*/    int32   hungerlevel;        // need to find max value ... < 10000 ; 0 is max famished?
/*3120*/    int32   thirstlevel;        // need to find max value ... < 10000 ; 0 is max famished?
/*3424*/	int8	unknown3424[20];	// ***Placeholder
/*3444*/	int32	current_zone;		// Player Zone (Lyenu: Zones are now saved as int32)
/*3448*/	int8	unknown3448[336];	// ***Placeholder
/*3784*/	int32	bind_point_zone;	// Lyenu: Bind zone is saved as a int32 now
/*3788*/	int32	start_point_zone[4];
										// Lyenu: Start Point Zones are saved as int32s now
/*3804*/	float	bind_location[3][5];
										// Player Bind Location (5 different X,Y,Z - Multiple bind points?)
/*3864*/	int8	unknown3656[20];	// ***Placeholder
/*3884*/	ItemProperties_Struct	bankinvitemproperties[8];
/*3964*/	ItemProperties_Struct	bankbagitemproperties[80];
/*4764*/	int8	unknown4556[4];
/*4768*/	int16	bank_inv[8];		// Player Bank Inventory Item Numbers
/*4784*/	int16	bank_cont_inv[80];	// Player Bank Inventory Item Numbers (Bags)
/*4944*/	int16	deity;		// ***Placeholder
/*4946*/	int16	guildid;			// Player Guild ID Number
/*4948*/	int32   BirthdayTime;
/*4952*/	int32   Unknown_4952;
/*4956*/	int32   TimePlayedMin;
/*4960*/	int8    Unknown_4960[2];
/*4962*/	int8    Fatigue;
/*4963*/	int8	pvp;				// Player PVP Flag
/*4964*/	int8	unknown4756;		// ***Placeholder
/*4965*/	int8	anon;				// Player Anon. Flag
/*4966*/	int8	gm;					// Player GM Flag
/*4967*/	sint8	guildrank;			// Player Guild Rank (0=member, 1=officer, 2=leader)
/*4968*/	int8	unknown4760[44];
/*5012*/	char	GMembers[6][64];	// Group Members
/*5396*/	int8	unknown5124[29];	// ***Placeholder 
/*5425*/	int8	AAPercent;			// Player AA Percent
/*5426*/	int8	haircolor;			// Player Hair Color
/*5427*/	int8	beardcolor;			// Player Beard Color
/*5428*/	int8	eyecolor1;			// Player Left Eye Color
/*5429*/	int8	eyecolor2;			// Player Right Eye Color
/*5430*/	int8	hairstyle;			// Player Hair Style
/*5431*/	int8	beard_t;			// T7g: Beard Type, formerly title - I have no clue why, Title moved a few lines below this one
/*5432*/	int8	luclinface;			// Player Face Type (Is that right?)
/*5433*/	int8	unknown5225[195];	// ***Placeholder
/*5628*/	int32	expAA;				// AA Exp
/*5632*/	int8	title;				// AA Title
/*5633*/	int8	perAA;				// AA Percentage
/*5634*/	int32	aapoints;			// AA Points
/*5638*/	int8	unknown5426[2822];	// Unknown
//			int32	raid_id;			// Raid ID?
//			int32	unknown5450;		// Unknown (Added 09 Oct 2002)
};

/*
** Client Target Struct
** Length: 2 Bytes
** OpCode: 6221
*/
struct ClientTarget_Struct {
/*000*/	int16	new_target;			// Target ID
};

struct PetCommand_Struct {
/*000*/ int8	command;
/*000*/ int8	unknownpcs000[7];
};


/*
** Generic Spawn Struct
** Length: 218 Bytes
** Used in:
**
*/
struct Spawn_Struct
{
/*0000*/	int8	animation;
/*0001*/	int8	heading;			// Current Heading
/*0002*/	int8	deltaHeading;		// Delta Heading
#ifndef INVERSEXY
/*0003*/	sint16	x_pos;				// X Position
/*0005*/	sint16	y_pos;				// Y Position
#else
/*0003*/	sint16	y_pos;				// Y Position
/*0005*/	sint16	x_pos;				// X Position
#endif
/*0007*/	sint16	z_pos;				// Z Position
/*0009*/	sint32	deltaY:10,			// Velocity Y
					spacer1:1,			// Placeholder
					deltaZ:10,			// Velocity Z
					spacer2:1,			// ***Placeholder
					deltaX:10;			// Velocity X
/*0013*/	int8	unknown0051;
/*0014*/	int16	pet_owner_id;		// Id of pet owner (0 if not a pet)
/*0016*/	int8	s_unknown1a[8];		// Placeholder
/*0024*/	float	size;
/*0028*/	float	walkspeed;
/*0032*/	float	runspeed;
/*0036*/	int32	equipcolors[7];
/*0064*/	int8	unknown0064[8];
/*0072*/	uint16	spawn_id;			// Id of new spawn
/*0074*/	int8	traptype;			// 65 is disarmable trap, 66 and 67 are invis triggers/traps
/*0075*/	int8	unknown0075;
/*0076*/	sint16	cur_hp;				// Current hp's of Spawn
/*0078*/	int16	GuildID;			// GuildID - previously Current hp's of Spawn
/*0080*/	int16	race;				// Race
/*0082*/	int8	NPC;				// NPC type: 0=Player, 1=NPC, 2=Player Corpse, 3=Monster Corpse, 4=???, 5=Unknown Spawn,10=Self
/*0083*/	int8	class_;				// Class
/*0084*/	int8	gender;				// Gender Flag, 0 = Male, 1 = Female, 2 = Other
/*0085*/	int8	level;				// Level of spawn (might be one sint8)
/*0086*/	int8	invis;				// 0=visable, 1=invisable
/*0087*/	int8	unknown0087;
/*0088*/	int8	pvp;
/*0089*/	int8	anim_type;
/*0090*/	int8	light;				// Light emitting
/*0091*/	int8	anon;				// 0=normal, 1=anon, 2=RP
/*0092*/	int8	AFK;				// 0=off, 1=on
/*0093*/	int8	unknown078;
/*0094*/	int8	LD;					// 0=NotLD, 1=LD
/*0095*/	int8	GM;					// 0=NotGM, 1=GM
/*0096*/	int8	s_unknown5_5;		// used to be s_unknown5[5]
/*0097*/	int8	npc_armor_graphic;	// 0xFF=Player, 0=none, 1=leather, 2=chain, 3=steelplate
/*0098*/	int8	npc_helm_graphic;	// 0xFF=Player, 0=none, 1=leather, 2=chain, 3=steelplate
/*0099*/	int8	s_unknown0099;		// used to be s_unknown5[2]
/*0100*/	int16	equipment[9];		// Equipment worn: 0=helm, 1=chest, 2=arm, 3=bracer, 4=hand, 5=leg, 6=boot, 7=melee1, 8=melee2
/*0118*/	sint8	guildrank;			// ***Placeholder
/*0119*/	int8	unknown0207;
/*0120*/	int16	deity;				// Deity.
/*0122*/	int8	unknown122;			// ***Placeholder
/*0123*/	char	name[64];			// Name of spawn (len is 30 or less)
/*0187*/	char	lastname[20];		// Last Name of player
/*0207*/	int8	haircolor;
/*0208*/	int8	beardcolor;
/*0209*/	int8	eyecolor1;			// the eyecolors always seem to be the same, maybe left and right eye?
/*0210*/	int8	eyecolor2;
/*0211*/	int8	hairstyle;
/*0212*/	int8	title;				//Face Overlay? (barbarian only)
/*0213*/	int8	luclinface;			// and beard
			int8	unknownpop[6];
};

/*
** New Spawn
** Length: 176 Bytes
** OpCode: 4921
*/
struct NewSpawn_Struct
{
	int32  ns_unknown1;            // ***Placeholder
	struct Spawn_Struct spawn;     // Spawn Information
};

/*
** Delete Spawn
** Length: 2 Bytes
** OpCode: 2b20
*/
struct DeleteSpawn_Struct
{
/*00*/ int16 spawn_id;             // Spawn ID to delete
};

/*
** Channel Message received or sent
** Length: 71 Bytes + Variable Length + 4 Bytes
** OpCode: 0721
*/
struct ChannelMessage_Struct
{
/*000*/	char	targetname[64];		// Tell recipient
/*064*/	char	sender[64];			// The senders name (len might be wrong)
/*128*/	int16	language;			// Language
/*130*/	int16	chan_num;			// Channel
/*132*/	int8	cm_unknown4[2];		// ***Placeholder
/*134*/	int16	skill_in_language;	// The players skill in this language? might be wrong
/*136*/	char	message[0];			// Variable length message
};


/*
** When somebody changes what they're wearing
**      or give a pet a weapon (model changes)
** Length: 16 Bytes
** Opcode: 9220
*/
struct WearChange_Struct{
/*000*/ int16 spawn_id;
/*002*/ int16 wear_slot_id;
/*004*/ int16 material;
/*005*/ //int8 unknown;
/*006*/ int8 unknown2;
/*007*/ int8 unknown3;
/*009*/ int8 blue;
/*010*/ int8 green;
/*011*/ int8 red;
/*012*/ int8 unknown4;

//Old Wearchange.
/*000*/	//int16	spawn_id;				// SpawnID
/*002*/	//int8	wear_slot_id;			// Slot
/*003*/	//int8	wc_unknown1;
/*004*/	//int8	graphic;				// 0-1-2-3 for armor, etc. First byte of IT# for weapons (yep, higher bytes are discarded, lol)
/*005*/ //int16	idfile;					// the # part of the IDFile
/*007*/	//int8	wc_unknown2[1];
/*008*/	//int32	color;					// color of the armor
/*012*/	//int8	wc_unknown3[4];
};

/*
** Type:   Bind Wound Structure
** Length: 8 Bytes
** OpCode: 9320
*/
struct bindWoundStruct
{
/*002*/	int16	playerid;			// TargetID
/*004*/	int8	unknown0004[2];		// ***Placeholder
/*006*/	sint32	hp;					// Hitpoints -- Guess
};

/*
** Drop Item On Ground
** Length: 226 Bytes
** OpCode: 3520
*/
struct dropItemStruct {
/*000*/ uint8	unknown000[8];			// ***Placeholder
/*008*/ uint16	itemNr;					// Item ID
/*010*/ uint8	unknown010[2];			// ***Placeholder
/*012*/ uint16	dropId;					// DropID
/*014*/ uint8	unknown014[10];			// ***Placeholder
/*024*/	float	yPos;
/*028*/	float	xPos;
/*032*/ float	zPos;					// Z Position
/*036*/ uint8	unknown036[4];			// heading maybe?
/*040*/ char	idFile[16];				// ACTOR ID
/*056*/ uint8	unknown056[168];		// ***Placeholder
};

/*
** Drop Coins
** Length: 114 Bytes
** OpCode: 0720
*/
struct dropCoinsStruct
{
/*0002*/ uint8	unknown0002[24];		// ***Placeholder
/*0026*/ uint16	dropId;					// Drop ID
/*0028*/ uint8	unknown0028[22];		// ***Placeholder
/*0050*/ uint32	amount;					// Total Dropped
/*0054*/ uint8	unknown0054[4];			// ***Placeholder
#ifndef INVERSEXY
/*0058*/ float	xPos;					// Y Position
/*0062*/ float	yPos;					// X Position
#else
	float yPos;
	float xPos;
#endif
/*0066*/ float	zPos;					// Z Position
/*0070*/ uint8	unknown0070[12];		// blank space
/*0082*/ int8	type[15];				// silver gold whatever
/*0097*/ uint8	unknown0097[17];		// ***Placeholder
};

/*
** Type:   Zone Change Request (before hand)
** Length: 70 Bytes-2 = 68 bytes 
** OpCode: a320
*/

#define ZONE_ERROR_NOMSG 0
#define ZONE_ERROR_NOTREADY -1
#define ZONE_ERROR_VALIDPC -2
#define ZONE_ERROR_STORYZONE -3
#define ZONE_ERROR_NOEXPANSION -6
#define ZONE_ERROR_NOEXPERIENCE -7

struct ZoneChange_Struct {
/*000*/	char	char_name[64];     // Character Name
/*064*/	int32	zoneID;
/*068*/ int8	unknown0072[4];
/*072*/	sint8	success;		// =0 client->server, =1 server->client, -X=specific error
/*073*/	int8	unknown0073[3]; // =0 ok, =ffffff error
};

struct Attack_Struct
{
/*00*/ int16   spawn_id;               // Spawn ID
/*02*/ int16   a_unknown1;             // ***Placeholder
/*04*/ int8    type;
/*05*/ int8    a_unknown2[7];          // ***Placeholder
};

/*
** Battle Code
** Length: 30 Bytes
** OpCode: 5820
*/
struct Action_Struct
{
	// len = 24
/*000*/	int16	target;
/*002*/	int16	source;
/*004*/	int8	type;
/*005*/	int8	unknown005;
/*006*/	int16	spell;
/*008*/	sint32	damage;
/*012*/	int8	unknown4[12];
};

/*
** Consider Struct
** Length: 24 Bytes
** OpCode: 3721
*/
struct Consider_Struct{
/*000*/ int16	playerid;               // PlayerID
/*002*/ int16	targetid;               // TargetID
/*004*/ int32	faction;                // Faction
/*008*/ int32	level;                  // Level
/*012*/ sint32	cur_hp;                  // Current Hitpoints
/*016*/ sint32	max_hp;                  // Maximum Hitpoints
/*020*/ int8 pvpcon;     // Pvp con flag 0/1
/*021*/ int8	unknown3[3];
};

/*
** Spawn Death Blow
** Length: 22 Bytes
** OpCode: 4a20
*/
struct Death_Struct
{
	// len = 16
	// Quag - Pretty much complete guesses here, might work tho  =p
/*000*/	int16	spawn_id;
/*002*/	int16	killer_id;

/*004*/	int16	corpseid;		// Client autorenames corpses "blah's corpse0", 0 is replaced with the corpseid as a normal number
/*006*/	int8	unknown006[2];
/*008*/	int16	spell_id;
/*010*/	int8	type;
/*011*/	int8	unknown011;
/*012*/	sint32	damage;
/*016*/ int32	unknownds016;
};

/*
** Generic Spawn Update
** Length: 15 Bytes
** Used in:
**
*/
struct SpawnPositionUpdate_Struct
{
/*0000*/ int16	spawn_id;               // Id of spawn to update
/*0002*/ sint8	anim_type; // run speed
/*0003*/ int8	heading;                // Heading
/*0004*/ sint8	delta_heading;          // Heading Change
#ifndef INVERSEXY
/*0005*/ sint16 x_pos;                  // New X position of spawn
/*0007*/ sint16 y_pos;                  // New Y position of spawn
#else
	sint16 y_pos;
	sint16 x_pos;
#endif
/*0009*/ sint16 z_pos;                  // New Z position of spawn
/*0011*/ int32  delta_y:10,             // Y Velocity
                spacer1:1,              // ***Placeholder
                delta_z:10,             // Z Velocity
                spacer2:1,              // ***Placeholder
                delta_x:10;             // Z Velocity
};

/*
** Spawn Position Update
** Length: 6 Bytes + Number of Updates * 15 Bytes
** OpCode: a120
*/
struct SpawnPositionUpdates_Struct
{
/*0000*/ int32  num_updates;               // Number of SpawnUpdates
/*0004*/ struct SpawnPositionUpdate_Struct // Spawn Position Update
                     spawn_update[0];
};

/*
** Spawn HP Update
** Length: 12 Bytes
** OpCode: b220
*/
struct SpawnHPUpdate_Struct
{
/*00*/ int16  spawn_id;               // Id of spawn to update
/*02*/ int16  shu_unknown1;           // ***Placeholder
/*04*/ sint32 cur_hp;                 // Current hp of spawn
/*08*/ sint32 max_hp;                 // Maximum hp of spawn
};

/*
** Stamina
** Length: 8 Bytes
** OpCode: 5721
*/
struct Stamina_Struct {
/*00*/ int16 food;                     // (low more hungry 127-0)
/*02*/ int16 water;                    // (low more thirsty 127-0)
/*04*/ int16 fatigue;                  // (high more fatigued 0-100)
};

/*
** Special Message
** Length: 6 Bytes + Variable Text Length
** OpCode: 8021
*/
struct SpecialMesg_Struct
{
/*0000*/ int32 msg_type;                // Type of message
/*0004*/ char  message[0];              // Message, followed by four bytes?
};
// msg_type's for custom usercolors 
#define MT_Say					256
#define MT_Tell					257
#define MT_Group				258
#define MT_Guild				259
#define MT_OOC					260
#define MT_Auction				261
#define MT_Shout				262
#define MT_Emote				263
#define MT_Spells				264
#define MT_YouHitOther			265
#define MT_OtherHitsYou			266
#define MT_YouMissOther			267
#define MT_OtherMissesYou		268
#define MT_Broadcasts			269
#define MT_Skills				270
#define MT_Disciplines			271

/*
** Level Update
** Length: 14 Bytes
** OpCode: 9821
*/
struct LevelUpdate_Struct
{
/*00*/ uint32 level;                  // New level
/*04*/ uint32 level_old;              // Old level
/*08*/ uint32 exp;                    // Current Experience
};

/*
** Experience Update
** Length: 14 Bytes
** OpCode: 9921
*/
struct ExpUpdate_Struct
{
/*0000*/ uint32 exp;                    // Current experience value
};

#define ITEM_MAX_STACK 20
/*
** Generic Item structure
** Length: 244 bytes
** Used in:
**    itemShopStruct(0c20), itemReceivedPlayerStruct(5220),
**    itemPlayerStruct(6421), bookPlayerStruct(6521),
**    containerPlayerStruct(6621), summonedItemStruct(7821),
**    tradeItemStruct(df20),
*/
#define ITEM_STRUCT_SIZE 360
#define SHORT_BOOK_ITEM_STRUCT_SIZE	264
#define SHORT_CONTAINER_ITEM_STRUCT_SIZE 276
struct Item_Struct
{
/*0000*/ char      name[64];        // Name of item
/*0064*/ char      lore[80];        // Lore text
/*0144*/ char      idfile[6];       // This is the filename of the item graphic when held/worn.
/*0150*/ sint16    flag;
/*0152*/ uint8	   unknown0150[22]; // Placeholder
/*0174*/ uint8	   weight;          // Weight of item
/*0175*/ sint8     nosave;          // Nosave flag 1=normal, 0=nosave, -1=spell?
/*0176*/ sint8     nodrop;          // Nodrop flag 1=normal, 0=nodrop, -1=??
/*0177*/ uint8     size;            // Size of item
/*0178*/ int8      type;
/*0179*/ uint8     unknown0178;     // ***Placeholder
/*0180*/ uint16    item_nr;         // Unique Item number
/*0182*/ uint16    icon_nr;         // Icon Number
/*0184*/ sint16    equipSlot;       // Current Equip slot
/*0186*/ uint8     unknwn0186[2];   // Equip slot cont.?
/*0188*/ uint32    equipableSlots;  // Slots where this item goes
/*0192*/ sint32    cost;            // Item cost in copper
/*0196*/ uint8     unknown0196[32]; // ***Placeholder
union
{
	struct
	{
	// 0228- have different meanings depending on flags
	/*0228*/ sint8    STR;              // Strength
	/*0229*/ sint8    STA;              // Stamina
	/*0230*/ sint8    CHA;              // Charisma
	/*0231*/ sint8    DEX;              // Dexterity
	/*0232*/ sint8    INT;              // Intelligence
	/*0233*/ sint8    AGI;              // Agility
	/*0234*/ sint8    WIS;              // Wisdom
	/*0235*/ sint8    MR;               // Magic Resistance
	/*0236*/ sint8    FR;               // Fire Resistance
	/*0237*/ sint8    CR;               // Cold Resistance
	/*0238*/ sint8    DR;               // Disease Resistance
	/*0239*/ sint8    PR;               // Poison Resistance
	/*0240*/ sint16   HP;               // Hitpoints
	/*0242*/ sint16   MANA;             // Mana
	/*0244*/ sint16   AC;               // Armor Class
	/*0246*/ uint8    MaxCharges;       // Maximum number of charges, for rechargable? (Sept 25, 2002)
	/*0247*/ sint8    GMFlag;           // GM flag 0  - normal item, -1 - gm item (Sept 25, 2002)
	/*0248*/ uint8    light;            // Light effect of this item
	/*0249*/ uint8    delay;            // Weapon Delay
	/*0250*/ uint8    damage;           // Weapon Damage
	/*0251*/ sint8    effecttype0;      // 0=combat, 1=click anywhere w/o class check, 2=latent/worn, 3=click anywhere EXPENDABLE, 4=click worn, 5=click anywhere w/ class check, -1=no effect
	/*0252*/ uint8    range;            // Range of weapon
	/*0253*/ uint8    skill;            // Skill of this weapon, refer to weaponskill chart
	/*0254*/ sint8    magic;            // Magic flag
                        //   00  (0000)  =   ???
                        //   01  (0001)  =  magic
                        //   12  (1100)  =   ???
                        //   14  (1110)  =   ???
                        //   15  (1111)  =   ???
	/*0255*/ sint8    level0;           // Casting level
	/*0256*/ uint8    material;         // Material?
	/*0257*/ uint8    unknown0258[3];   // ***Placeholder
	/*0260*/ uint32   color;            // Amounts of RGB in original color
	/*0264*/ uint8    unknown0264[2];   // ***Placeholder (Asiel: Has to do with Diety, will unwrap later)
	/*0266*/ uint16   spellId0;         // SpellID of special effect
	/*0268*/ uint16   classes;          // Classes that can use this item
	/*0270*/ uint8    unknown0270[2];   // ***Placeholder
	union
	{
		struct
		{
		/*0272*/ uint16   races;            // Races that can use this item
		/*0274*/ sint8    unknown0274[2];   // ***Placeholder
		/*0276*/ sint8    stackable;        //  1= stackable, 3 = normal, 0 = ? (not stackable)
		} normal;
	};
	/*0277*/ uint8    level;            // Casting level
	union // 0278 has different meanings depending on an stackable
	{
	/*0278*/ sint8    number;          // Number of items in stack
	/*0278*/ int8    charges;         // Number of charges (-1 = unlimited)
	};
	/*0279*/ sint8    effecttype;      // 0=combat, 1=click anywhere w/o class check, 2=latent/worn, 3=click anywhere EXPENDABLE, 4=click worn, 5=click anywhere w/ class check, -1=no effect
		 uint16   spellId;         // spellId of special effect
		 uint8    unknown0282[10]; // ***Placeholder 0288
		 uint32   casttime;        // Cast time of clicky item in miliseconds
		 uint8    unknown0296[16]; // ***Placeholder
		 uint16   skillModId;
		 sint16   skillModPercent;
		 sint16   BaneDMGRace;
		 sint16   BaneDMGBody;
			 // 1 Humanoid, 2 Lycanthrope, 3 Undead, 4 Giant, 5 Construct, 6 Extraplanar, 7 Magical
		 uint8    BaneDMG;
		 uint8    unknown0316[3];
		 uint8    RecLevel;         // max should be 65
		 uint8    RecSkill;         // Max should be 252
		 uint8    unknown0325[2];
		 uint8    ElemDmgType; 
			// 1 Magic, 2 Fire, 3 Cold, 4 Poison, 5 Disease
		 uint8    ElemDmg;
		 uint8    unknown0330[22];
		 uint8    ReqLevel; // Required level
		 uint8    unknown0352[5];
	/*0358*/ int16    focusspellId;
	} common;
	struct // Book Structure (flag == 0x7669)
	{
	/*0228*/ sint8    unknown0172[6];      // ***Placeholder
	/*0234*/ char     file[15];            // Filename of book text on server
	/*0249*/ sint8    unknown0190[15];    // ***Placeholder
	} book;
	struct // containers flag == 0x5400 or 0x5450
	{
		/*0228*/ sint8    unknown0212[41];     // ***Placeholder
		/*0269*/ uint8    numSlots;        // number of slots in container
		/*0270*/ sint8    unknown0214;     // ***Placeholder
		/*0271*/ sint8    sizeCapacity;    // Maximum size item container can hold
		/*0272*/ uint8    weightReduction; // % weight reduction of container
		/*0273*/ uint8    unknown0273[3];     // ***Placeholder
		} container;
};
	inline bool	IsNormal() const		{ return (bool) (type == 0x00); } // ie, not book, not bag
	inline bool	IsBook() const			{ return (bool) (type == 0x02); }
	inline bool	IsBag() const			{ return (bool) (type == 0x01); }
	inline bool	IsStackable() const		{ return (bool) ((type == 0x00) && (common.normal.stackable == 1 || (common.effecttype == 0 && common.normal.stackable == 2))); }
	inline bool	IsEquipable(int16 race, int16 class_) const		{
	 if (!this) return false;
	 bool israce = false,isclass = false;
	 if (type != 0x00 && equipableSlots == 0)
      return false;
     else if (common.classes == 0)
    return false;
 else if(common.classes == 32767 && common.normal.races == 32767)
 {
    return true;
 }
 else
 {
     int16 classes_ = common.classes;
     int16 races_ = common.normal.races;
     if(common.classes == 32767) {
	 isclass = true;
     }
     if(common.normal.races == 32767)
 {
    	 israce = true;
     }
     for (int i = 1; i <= 12; i++) {
        if (classes_ % 2 == 1) {
    	    if(i == class_)
 {
    		isclass = true;
	    }
        }
        if (races_ %2 == 1) {
    	    if(i == race)
 {
    		israce = true;
	    }
        }
        classes_ = classes_/2;
        races_ = races_/2;
     }
 }
 if(israce && isclass)
	 return true;
 else
 return false;
}
	inline bool	IsExpendable() const	{ return (bool) (common.skill == 21 || common.skill == 11); }
	inline bool	IsGM() const			{ return (bool) (common.GMFlag == -1); }
	inline bool	IsLore() const			{ return (bool) (strstr(lore, "*") != 0); }
	inline bool	IsPendingLore() const	{ return (bool) (strstr(lore, "~") != 0); }
	inline bool	IsArtifact() const		{ return (bool) (strstr(lore, "#") != 0); }
	inline bool IsWeapon() const	{ return (bool) common.damage; }
};

/*
** Summoned Item - Player Made Item?
** Length: 244 Bytes
** OpCode: 7821
*/
struct SummonedItem_Struct
{
/*0000*/ struct Item_Struct item;        // Refer to itemStruct for members
};

struct Consume_Struct
{
/*0000*/ int32 slot;
/*0004*/ int32 auto_consumed; // 0xffffffff when auto eating e7030000 when right click
/*0008*/ int8  c_unknown1[4];
/*0012*/ int8  type; // 0x01=Food 0x02=Water
/*0013*/ int8  unknown13[3];
};

struct MoveItem_Struct
{
/*0000*/ uint32 from_slot;
/*0004*/ uint32 to_slot;
/*0008*/ uint32 number_in_stack;
	//uint16 stackable; 
	//uint16 number_in_stack;

};

struct MoveCoin_Struct
{
		 uint32 from_slot;
		 uint32 to_slot;
		 uint32 cointype1;
		 uint32 cointype2;
		 uint32	amount;
};

struct Surname_Struct 
{ 
       char name[64]; 
       char s_unknown1; 
       char s_unknown2; 
       char s_unknown3; 
       char s_unknown4; 
       char lastname[64]; 
}; 

struct GuildsListEntry_Struct {
	int32 guildID;  // empty = 0xFFFFFFFF
//	int32 guildIDx; // another guild ID???? Im not sure whats going on, but in-game it doesnt work with this here
	char name[32];
	int32 guildIDx; // another guild ID????
	int8 unknown3[8]; // = 0x00
	int8 unknown4[16];
	int8 regguild[8];
	int8 unknown6[4]; // = 0xFF
	int8 exists;	 // = 1 if exists, 0 on empty
	int8 unknown8[3]; // = 0x00
	int8 unknown9[4]; // = 0x00
	int8 unknown10[4]; // = 0xFF
	int8 unknown11[8]; // = 0x00
};

struct GuildsList_Struct {
	int8 head[4];
	GuildsListEntry_Struct Guilds[512];
};

//#define CODE_NEW_GUILD                  0x7b21
struct GuildUpdate_Struct {
	int32	guildID;
	GuildsListEntry_Struct entry;
};

/*
** Money Loot
** Length: 22 Bytes
** OpCode: 5020
*/
struct moneyOnCorpseStruct {
/*0000*/ uint8	response;		// 0 = someone else is, 1 = OK, 2 = not at this time
/*0001*/ uint8	unknown1;		// = 0x5a
/*0002*/ uint8	unknown2;		// = 0x40
/*0003*/ uint8	unknown3;		// = 0
/*0004*/ uint32	platinum;		// Platinum Pieces
/*0008*/ uint32	gold;			// Gold Pieces
/*0012*/ uint32	silver;			// Silver Pieces
/*0016*/ uint32	copper;			// Copper Pieces
};

//opcode = 0x5220
// size 292


struct LootingItem_Struct {
/*000*/	int16	lootee;
/*002*/	int16	looter;
/*004*/	int16	slot_id;
/*006*/	int8	unknown3[2];
/*008*/	int32	type;
};

// Guild invite, remove
struct GuildCommand_Struct {
	char othername[64];
	char myname[64];
	int16 guildeqid;
	int8 unknown[2]; // for guildinvite all 0's, for remove 0=0x56, 2=0x02
	int32 officer;
};

// Opcode 0x4f21
// Size = 84 bytes
struct GMZoneRequest_Struct {
	char	charname[64];
	int32	zoneID;
	int8	unknown1[16];
	int8	success; // 0 if command failed, 1 if succeeded?
	int8	unknown2[3]; // 0x00
	//int32	unknown3;
};

struct GMSummon_Struct {
/*  0*/	char    charname[64];
/* 30*/	char    gmname[64];
/* 60*/ int32	success;
/* 61*/	int32	zoneID;
#ifndef INVERSEXY
/*92*/	sint32  x;
/*96*/	sint32  y;
#else
	sint32 y;
	sint32 x;
#endif
/*100*/ sint32  z;
/*104*/	int32 unknown2; // E0 E0 56 00
};

struct GMGoto_Struct { // x,y is swapped as compared to summon and makes sense as own packet
/*  0*/	char    charname[64];
/* 30*/	char    gmname[64];
/* 60*/ int32	success;
/* 61*/	int32	zoneID;

/*92*/	sint32  x;
/*96*/	sint32  y;
/*100*/ sint32  z;
/*104*/	int32 unknown2; // E0 E0 56 00
};

struct GMLastName_Struct {
	char name[64];
	char gmname[64];
	char lastname[64];
	int16 unknown[2];	// 0x00, 0x00
					    // 0x01, 0x00 = Update the clients
};

//Combat Abilities
struct CombatAbility_Struct {
	int32 m_id;
	int32 m_atk;
	int32 m_type;
};

//Instill Doubt
struct Instill_Doubt_Struct {
	int8 i_id;
	int8 ia_unknown;
	int8 ib_unknown;
	int8 ic_unknown;
	int8 i_atk;
	int8 id_unknown;
	int8 ie_unknown;
	int8 if_unknown;
	int8 i_type;
	int8 ig_unknown;
	int8 ih_unknown;
	int8 ii_unknown;
};

struct GiveItem_Struct {
	uint16 to_entity;
	sint16 to_equipSlot;
	uint16 from_entity;
	sint16 from_equipSlot;
};

struct Random_Struct {
	int32 low;
	int32 high;
};

struct LFG_Struct {
	char	name[64];
	int32	value;
};

// EverQuest Time Information:
// 72 minutes per EQ Day
// 3 minutes per EQ Hour
// 6 seconds per EQ Tick (2 minutes EQ Time)
// 3 seconds per EQ Minute

struct TimeOfDay_Struct {
	int8	hour;
	int8	minute;
	int8	day;
	int8	month;
	int16	year;
};

// Darvik: shopkeeper structs
struct Merchant_Click_Struct {
/*000*/ int16	npcid;			// Merchant NPC's entity id
/*002*/ int16	playerid;
/*004*/ int8	unknown[8]; /*
0 is e7 from 01 to // MAYBE SLOT IN PURCHASE
1 is 03
2 is 00
3 is 00
4 is ??
5 is ??
6 is 00 from a0 to
7 is 00 from 3f to */
/*
0 is F6 to 01
1 is CE CE
4A 4A
00 00
00 E0
00 CB
00 90
00 3F
*/
};			
struct Merchant_Purchase_Struct {
/*000*/	int16	npcid;			// Merchant NPC's entity id
/*002*/	int16	playerid;		// Player's entity id
/*004*/	int16	itemslot;
/*006*/	int8	IsSold;		// Already sold
/*007*/	int8	unknown001;	// always 0x0b, 0x7c, 0x00 ??
//		int8	unknown002;  On live, it looks like it is only 16 bytes now and quanity is put at byte 9 (Hogie on 12/29/2002)
/*008*/	int8	quantity;		// Qty - when used in Merchant_Purchase_Struct
/*009*/	int8	unknown003;
/*010*/	int8	unknown004;
/*011*/	int8	unknown005;
/*012*/	sint32  itemcost;
};

struct Item_Shop_Struct {
	uint16 merchantid;
	int8 itemtype;
	Item_Struct item;
	int8 iss_unknown001[6];
};
/*struct Item_Shop_Struct {
	int16 numitems;
*/

struct Illusion_Struct {
/*000*/	int16	spawnid;
/*002*/	int16	race;
/*004*/	int8	gender;
/*005*/ int8	texture;
/*006*/	int8	helmtexture;
/*007*/ int8	unknown_26; //Always 26
/*008*/	int8	haircolor;
/*009*/	int8	beardcolor;
/*010*/	int8	eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
/*011*/	int8	eyecolor2;
/*012*/	int8	hairstyle;
/*013*/	int8	title; //Face Overlay? (barbarian only)
/*014*/	int8	luclinface; // and beard
/*015*/ int8	unknown015;
/*016*/ int32	unknown016; // Always 0xFFFFFFFF it seems.
};

struct CPlayerItems_packet_Struct {
/*000*/	int16		opcode;		// OP_ItemTradeIn
/*002*/	Item_Struct	item;
};

struct CPlayerItems_Struct {
/*000*/	int16		count;
/*002*/	CPlayerItems_packet_Struct	packets[0];;
};

struct MerchantItemD_Struct {
	int8 unknown0000[2];
Item_Struct	item;
};

struct ItemOnCorpse_Struct {
  int16 count;
  struct MerchantItemD_Struct item[0];
};

struct MerchantItem_Struct {
/*000*/	int16		count;
/*002*/	MerchantItemD_Struct	packets[0];;
};

//By Scruffy... Thanks ShowEQ and sins!
struct SkillUpdate_Struct {
	uint8 skillId;
	int8 unknown1[3];
	uint16 value;
	int8 unknown2[3];
};

struct ZoneUnavail_Struct {
	//This actually varies, but...
	char zonename[16];
	short int unknown[4];
};

struct GroupGeneric_Struct {
	char name1[64];
	char name2[64];
};

struct GroupUpdate_Struct {	
	int32	action;
	char	yourname[64];
	char	membername[64];
	int8	unknown[4];	
};

struct ChangeLooks_Struct {
/*000*/	int8	haircolor;
/*001*/	int8	beardcolor;
/*002*/	int8	eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
/*003*/	int8	eyecolor2;
/*004*/	int8	hairstyle;
/*005*/	int8	title; //Face Overlay? (barbarian only)
/*006*/	int8	face; // and beard
};
struct Trade_Window_Struct {
	int16 fromid; 
	int16 toid;
};
struct Give_Struct {
	int16 unknown[2];
};

struct PetitionClientUpdate_Struct {
	int32 petnumber;    // Petition Number
	int32 color;		// 0x00 = green, 0x01 = yellow, 0x02 = red
	int32 status;
	time_t senttime;    // 4 has to be 0x1F
	char accountid[32];
	char gmsenttoo[64];
	long quetotal;
	char charname[64];
};

/*struct Petition_Struct {
	int32 petnumber;
	int32 urgency;
	char accountid[32];
	char lastgm[64];
	int32	zone;
	//char zone[32];
	char charname[32];
	int32 charlevel;
	int32 charclass;
	int32 charrace;
	time_t senttime; // Time?
	int32 checkouts;
	int32 unavail;
	int8 unknown5[4];
	char petitiontext[1024];
};*/

struct Petition_Struct {
	int32 petnumber;
	int32 urgency;
	char accountid[32];
	char lastgm[32];
	int32	zone;
	//char zone[32];
	char charname[64];
	int32 charlevel;
	int32 charclass;
	int32 charrace;
	int32 unknown;
	//time_t senttime; // Time?
	int32 checkouts;
	int32 unavail;
	//int8 unknown5[4];
	time_t senttime;
	char petitiontext[1024];
};


struct Who_All_Struct { // 76 length total
/*000*/	char	whom[64];
/*064*/	int16	wrace;		// FF FF = no race
/*066*/	int16	wclass;		// FF FF = no class
/*068*/	int16	lvllow;		// FF FF = no numbers
/*070*/	int16	lvlhigh;	// FF FF = no numbers
/*072*/	int16	gmlookup;	// FF FF = not doing /who all gm
/*074*/	int8	unknown074[2];
/*076*/	int8	unknown076[64];
/*140*/
};

struct Stun_Struct { // 4 bytes total 
	int32 duration; // Duration of stun
};

struct Combine_Struct { 
	int8 tradeskill;
	int8 unknown001;
	int8 unknown002;
	int8 unknown003;
	int8 containerslot;
	int8 unknown004;
	int16 iteminslot[10]; // IDs of items in container
	int16 unknown005;
	int16 unknown006;
	int16 containerID; // ID of container item

};

//OP_Social_Text
struct Emote_Text {
	int16 unknown1;
	char message[1024];
};

//OP_Social_Action
struct Social_Action_Struct {
	int8 unknown1[4];
	int8 action;
	int8 unknown2[7];
};

//Inspect 
struct Inspect_Struct { 
	int16 TargetID; 
	int16 PlayerID; 
}; 

//OP_SetDataRate
struct SetDataRate_Struct {
	float newdatarate;
};

//OP_SetServerFilter
struct SetServerFilter_Struct {
	int8 unknown[60];
};

//Op_SetServerFilterAck
struct SetServerFilterAck_Struct {
	int8 blank[8];
};

struct GMName_Struct {
	char oldname[64];
	char gmname[64];
	char newname[64];
	int8 badname;
	int8 unknown[3];
};

struct GMDelCorpse_Struct {
	char corpsename[64];
	char gmname[64];
	int8 unknown;
};

struct GMKick_Struct {
	char name[64];
	char gmname[64];
	int8 unknown;
};

struct GMKill_Struct {
	char name[64];
	char gmname[64];
	int8 unknown;
};

struct GMEmoteZone_Struct {
	char text[512];
};

struct Unknown_Struct {
	int8 unknown1[60*4];
};

// This is where the Text is sent to the client.
// Use ` as a newline character in the text.
// Variable length.
struct BookText_Struct {
	char* booktext; // Variable Length
};
// This is the request to read a book.
// This is just a "text file" on the server
// or in our case, the 'name' column in our books table.
struct BookRequest_Struct {
	char txtfile[14];
};

struct Object_Struct {
		int8	unknown001[8];
		uint16  itemid;
		uint16  unknown010;
		int32	dropid;	//this id will be used, if someone clicks on this object
		uint8   unknown014[8];
		uint8   charges;
		uint8   unknown25;
		uint8   maxcharges;
		uint8	unknown027[117];		// ***Placeholder
		float	zpos;
		float	xpos;
		float	ypos;					// Z Position
		char	idFile[16];				// ACTOR ID
		int16	heading;
		char	objectname[16];
		int16	itemsinbag[10]; //if you drop a bag, thats where the items are
		int8	unknowneos[14];
};

struct ClickObject_Struct {
	int32	objectID;
	int16	PlayerID;
	int16	unknown;
};

struct ClickObjectServer_Struct {
	int8	unknown0; //Always seems to be 18
	int16 	playerid;
	int16	unknown3;
	int32	objectid;
	int32	unknown9;
	int16	type;
	int8    unknown15[6];
	int16	unknown21;
	int8	unknown23[5];
};

struct Door_Struct
{
/*0000*/ char    name[16];            // Filename of Door // Was 10char long before... added the 6 in the next unknown to it: Daeken M. BlackBlade
//		 uint8	 unknown0008[6];	 // This is "supposed" to be with name hehe
#ifndef INVERSEXY
/*0016*/ float   xPos;               // y loc
/*0020*/ float   yPos;               // x loc
#else
		 float yPos;
		 float xPos;
#endif
/*0024*/ float   zPos;               // z loc
/*0026*/ float	 heading;
/*0028*/ uint8	 unknown003[6];
/*0038*/ uint8	 doorId;             // door's id #
/*0039*/ uint8	 opentype;
/*0040*/ uint8	 initialstate;
/*0040*/ uint16	 holdstateforever;		//FFFF
/*0042*/ uint16 unknown3;
/*0044*/ int8 auto_return;
/*0045*/ int8 unknown6;
};
struct DoorSpawns_Struct	//SEQ
{
	uint16 count;            
	struct Door_Struct doors[0];
};

struct ClickDoor_Struct {
	int8	doorid;
	int8	unknowncds000[5];
	int16	playerid;
};

struct MoveDoor_Struct {
	int8	doorid;
	int8	action;
};


struct BecomeNPC_Struct {
	int32 id;
	long maxlevel;
};

struct ItemToTrade_Struct { // 302
	int16 playerid;
	int16 toslot;
	int8  unknown01;
	Item_Struct item;
	int8  unknown02[53];
};
struct CancelTrade_Struct { // 302
	int16 fromid;
	int16 action;
	// Bigpull 04/27/03
	//int16 withid;
	//int16 myid;
};

struct Underworld_Struct {
	float speed;
#ifndef INVERSEXY
	float x;
	float y;
#else
	float y;
	float x;
#endif
	float z;
};

struct Resurrect_Struct	//160
{
	int16	unknown_01;            
	char	zone[15];
	int8	unknown_02[19];
#ifndef INVERSEXY
		float	x;
		float	y;
#else
		float	y;
		float	x;
#endif
		float	z;
		int32	unknown_02_1;
	char	your_name[64];
	int8	unknown_03[6];
	char	rezzer_name[64];
	int8	unknown_04[2];
	int16	spellid;
	char	corpse_name[64];
	int8	unknown_05[4];
	int32	action;
};

// updates client with alternate advancement (non-AA table) information
// Length: 8 Bytes
// OpCode: 2322
struct AltAdvStats_Struct {
   /*int32 unspent;
  int32 percentage;
  int32 experience;*/
  int32 experience;
  int16 unspent;//turning on (percent to 10) giving you 10 points
  int8	percentage;
  int8	unknownaas007;
  //int16 percentage;
};

//  Alternate Advancement table.  holds all the skill levels for the AA skills.
//  Length: 256 Bytes
//  OpCode: 1422
struct PlayerAA_Struct {
/*    0 */  uint16 unknown0;
  union {
    uint16 unnamed[17];
    struct {  
/*    1 */  uint16 innate_strength;
/*    2 */  uint16 innate_stamina;
/*    3 */  uint16 innate_agility;
/*    4 */  uint16 innate_dexterity;
/*    5 */  uint16 innate_intelligence;
/*    6 */  uint16 innate_wisdom;
/*    7 */  uint16 innate_charisma;
/*    8 */  uint16 innate_fire_protection;
/*    9 */  uint16 innate_cold_protection;
/*   10 */  uint16 innate_magic_protection;
/*   11 */  uint16 innate_poison_protection;
/*   12 */  uint16 innate_disease_protection;
/*   13 */  uint16 innate_run_speed;
/*   14 */  uint16 innate_regeneration;
/*   15 */  uint16 innate_metabolism;
/*   16 */  uint16 innate_lung_capacity;
/*   17 */  uint16 first_aid;
    } named;
  } general_skills;
  union {
    uint16 unnamed[17];
    struct {
/*   18 */  uint16 healing_adept;
/*   19 */  uint16 healing_gift;
/*   20 */  uint16 spell_casting_mastery;
/*   21 */  uint16 spell_casting_reinforcement;
/*   22 */  uint16 mental_clarity;
/*   23 */  uint16 spell_casting_fury;
/*   24 */  uint16 chanelling_focus;
/*   25 */  uint16 spell_casting_subtlety;
/*   26 */  uint16 spell_casting_expertise;
/*   27 */  uint16 spell_casting_deftness;
/*   28 */  uint16 natural_durability;
/*   29 */  uint16 natural_healing;
/*   30 */  uint16 combat_fury;
/*   31 */  uint16 fear_resistance;
/*   32 */  uint16 finishing_blow;
/*   33 */  uint16 combat_stability;
/*   34 */  uint16 combat_agility;
    } named;
  } archetype_skills;
  union {
    uint16 unnamed[93];
    struct {
/*   35 */  uint16 mass_group_buff; // All group-buff-casting classes(?)
// ===== Cleric =====
/*   36 */  uint16 divine_resurrection;
/*   37 */  uint16 innate_invis_to_undead; // cleric, necromancer
/*   38 */  uint16 celestial_regeneration;
/*   39 */  uint16 bestow_divine_aura;
/*   40 */  uint16 turn_undead;
/*   41 */  uint16 purify_soul;
// ===== Druid =====
/*   42 */  uint16 quick_evacuation; // wizard, druid
/*   43 */  uint16 exodus; // wizard, druid
/*   44 */  uint16 quick_damage; // wizard, druid
/*   45 */  uint16 enhanced_root; // druid
/*   46 */  uint16 dire_charm; // enchanter, druid, necromancer
// ===== Shaman =====
/*   47 */  uint16 cannibalization;
/*   48 */  uint16 quick_buff; // shaman, enchanter
/*   49 */  uint16 alchemy_mastery;
/*   50 */  uint16 rabid_bear;
// ===== Wizard =====
/*   51 */  uint16 mana_burn;
/*   52 */  uint16 improved_familiar;
/*   53 */  uint16 nexus_gate;
// ===== Enchanter =====
/*   54 */  uint16 unknown54;
/*   55 */  uint16 permanent_illusion;
/*   56 */  uint16 jewel_craft_mastery;
/*   57 */  uint16 gather_mana;
// ===== Mage =====
/*   58 */  uint16 mend_companion; // mage, necromancer
/*   59 */  uint16 quick_summoning;
/*   60 */  uint16 frenzied_burnout;
/*   61 */  uint16 elemental_form_fire;
/*   62 */  uint16 elemental_form_water;
/*   63 */  uint16 elemental_form_earth;
/*   64 */  uint16 elemental_form_air;
/*   65 */  uint16 improved_reclaim_energy;
/*   66 */  uint16 turn_summoned;
/*   67 */  uint16 elemental_pact;
// ===== Necromancer =====
/*   68 */  uint16 life_burn;
/*   69 */  uint16 dead_mesmerization;
/*   70 */  uint16 fearstorm;
/*   71 */  uint16 flesh_to_bone;
/*   72 */  uint16 call_to_corpse;
// ===== Paladin =====
/*   73 */  uint16 divine_stun;
/*   74 */  uint16 improved_lay_of_hands;
/*   75 */  uint16 slay_undead;
/*   76 */  uint16 act_of_valor;
/*   77 */  uint16 holy_steed;
/*   78 */  uint16 fearless; // paladin, shadowknight
/*   79 */  uint16 two_hand_bash; // paladin, shadowknight
// ===== Ranger =====
/*   80 */  uint16 innate_camouflage; // ranger, druid
/*   81 */  uint16 ambidexterity; // all "dual-wield" users
/*   82 */  uint16 archery_mastery; // ranger
/*   83 */  uint16 unknown83;
/*   84 */  uint16 endless_quiver; // ranger
// ===== Shadow Knight =====
/*   85 */  uint16 unholy_steed;
/*   86 */  uint16 improved_harm_touch;
/*   87 */  uint16 leech_touch;
/*   88 */  uint16 unknown88;
/*   89 */  uint16 soul_abrasion;
// ===== Bard =====
/*   90 */  uint16 instrument_mastery;
/*   91 */  uint16 unknown91;
/*   92 */  uint16 unknown92;
/*   93 */  uint16 unknown93;
/*   94 */  uint16 jam_fest;
/*   95 */  uint16 unknown95;
/*   96 */  uint16 unknown96;
// ===== Monk =====
/*   97 */  uint16 critical_mend;
/*   98 */  uint16 purify_body;
/*   99 */  uint16 unknown99;
/*  100 */  uint16 rapid_feign;
/*  101 */  uint16 return_kick;
// ===== Rogue =====
/*  102 */  uint16 escape;
/*  103 */  uint16 poison_mastery;
/*  104 */  uint16 double_riposte; // all "riposte" users
/*  105 */  uint16 unknown105;
/*  106 */  uint16 unknown106;
/*  107 */  uint16 purge_poison; // rogue
// ===== Warrior =====
/*  108 */  uint16 flurry;
/*  109 */  uint16 rampage;
/*  110 */  uint16 area_taunt;
/*  111 */  uint16 warcry;
/*  112 */  uint16 bandage_wound;
// ===== (Other) =====
/*  113 */  uint16 spell_casting_reinforcement_mastery; // all "pure" casters
/*  114 */  uint16 unknown114;
/*  115 */  uint16 extended_notes; // bard
/*  116 */  uint16 dragon_punch; // monk
/*  117 */  uint16 strong_root; // wizard
/*  118 */  uint16 singing_mastery; // bard
/*  119 */  uint16 body_and_mind_rejuvenation; // paladin, ranger, bard
/*  120 */  uint16 physical_enhancement; // paladin, ranger, bard
/*  121 */  uint16 adv_trap_negotiation; // rogue, bard
/*  122 */  uint16 acrobatics; // all "safe-fall" users
/*  123 */  uint16 scribble_notes; // bard
/*  124 */  uint16 chaotic_stab; // rogue

/*  125 */  uint16 pet_discipline; // all pet classes except enchanter
/*  126 */  uint16 unknown126;
/*  127 */  uint16 unknown127;
/*  128 */  uint16 unknown128;
    } named;
  } class_skills;
#ifdef TEST_SERVERAA
  union {
    uint16 unnamed[18];
    struct {  
/*  129 */  uint16 advanced_innate_strength;
/*  130 */  uint16 advanced_innate_stamina;
/*  131 */  uint16 advanced_innate_agility;
/*  132 */  uint16 advanced_innate_dexterity;
/*  133 */  uint16 advanced_innate_intelligence;
/*  134 */  uint16 advanced_innate_wisdom;
/*  135 */  uint16 advanced_innate_charisma;
/*  136 */  uint16 warding_of_solusek;
/*  137 */  uint16 blessing_of_eci;
/*  138 */  uint16 marrs_protection;
/*  139 */  uint16 shroud_of_the_faceless;
/*  140 */  uint16 bertoxxulous_gift;
/*  141 */  uint16 new_tanaan_crafting_mastery;
/*  142 */  uint16 planar_power;
/*  143 */  uint16 planar_durability;
    } named;
  } pop_advance;
    union {
    uint16 unnamed[14];
    struct {  
/*  144 */  uint16 unknown144;
/*  145 */  uint16 unknown145;
/*  146 */  uint16 unknown146;
/*  147 */  uint16 unknown147;
/*  148 */  uint16 coup_de_grace;
/*  149 */  uint16 fury_of_the_ages;
/*  150 */  uint16 unknown150;
/*  151 */  uint16 lightning_reflexes;
/*  152 */  uint16 innate_defense;
/*  153 */  uint16 unknown153;
/*  154 */  uint16 unknown154;
/*  155 */  uint16 unknown155;
/*  156 */  uint16 unknown156;
/*  157 */  uint16 unknown157;
/*  158 */  uint16 unknown158;
/*  159 */  uint16 unknown159;
/*  160 */  uint16 unknown160;
/*  161 */  uint16 unknown161;
/*  162 */  uint16 unknown162;
/*  163 */  uint16 unknown163;
/*  164 */  uint16 unknown164;
/*  165 */  uint16 unknown165;
/*  166 */  uint16 hasty_exit;
/*  167 */  uint16 unknown167;
/*  168 */  uint16 unknown168;
/*  169 */  uint16 unknown169;
/*  170 */  uint16 unknown170;
/*  171 */  uint16 unknown171;
/*  172 */  uint16 unknown172;
/*  173 */  uint16 unknown173;
/*  174 */  uint16 unknown174;
/*  175 */  uint16 unknown175;
/*  176 */  uint16 unknown176;
/*  177 */  uint16 unknown177;
/*  178 */  uint16 unknown178;
/*  179 */  uint16 unknown179;
/*  180 */  uint16 unknown180;
/*  181 */  uint16 unknown181;
/*  182 */  uint16 unknown182;
/*  183 */  uint16 unknown183;
/*  184 */  uint16 unknown184;
/*  185 */  uint16 unknown185;
/*  186 */  uint16 unknown186;
/*  187 */  uint16 unknown187;
/*  188 */  uint16 unknown188;
/*  189 */  uint16 unknown189;
/*  190 */  uint16 unknown190;
/*  191 */  uint16 unknown191;
/*  192 */  uint16 unknown192;
/*  193 */  uint16 unknown193;
/*  194 */  uint16 unknown194;
/*  195 */  uint16 unknown195;
/*  196 */  uint16 unknown196;
/*  197 */  uint16 unknown197;
/*  198 */  uint16 unknown198;
/*  199 */  uint16 unknown199;
/*  200 */  uint16 unknown200;
/*  201 */  uint16 unknown201;
/*  202 */  uint16 unknown202;
/*  203 */  uint16 unknown203;
/*  204 */  uint16 unknown204;
/*  205 */  uint16 unknown205;
/*  206 */  uint16 unknown206;
/*  207 */  uint16 unknown207;
/*  208 */  uint16 unknown208;
/*  209 */  uint16 unknown209;
/*  210 */  uint16 unknown210;
/*  211 */  uint16 unknown211;
/*  212 */  uint16 unknown212;
/*  213 */  uint16 unknown213;
/*  214 */  uint16 unknown214;
/*  215 */  uint16 unknown215;
/*  216 */  uint16 unknown216;
/*  217 */  uint16 unknown217;
/*  218 */  uint16 unknown218;
/*  219 */  uint16 unknown219;
/*  220 */  uint16 unknown220;
/*  221 */  uint16 unknown221;
/*  222 */  uint16 unknown222;
/*  223 */  uint16 unknown223;
/*  224 */  uint16 unknown224;
/*  225 */  uint16 unknown225; // New innate regen
    } named;
  } pop_ability;
#endif
};

struct SetRunMode_Struct {
	int8 mode;
	int8 unknown[3];
};

//EnvDamage is EnvDamage2 without a few bytes at the end.

struct EnvDamage2_Struct {
	int16 id;
	int16 unknown;
	sint8 dmgtype; //FA = Lava; FC = Falling
	int8 unknown2;
	int16 constant; //Always FFFF
	int16 damage;
	int8 unknown3[14]; //A bunch of 00's...
};

//Bazaar Stuff =D

struct BazaarWindowStart_Struct {
	int8   action;
	int8   unknown1;
	int16  unknown2;
};

struct BazaarWelcome_Struct {
	BazaarWindowStart_Struct beginning;
	int32  traders;
	int32  items;
	int8   unknown1[8];
};

struct BazaarSearch_Struct {
	BazaarWindowStart_Struct beginning;
	int16  class_;
	int16  race;
	int16  stat;
	int16  slot;
	int16  type;
	char   name[64];
};

struct BazaarSearchResults_Struct {
	BazaarWindowStart_Struct beginning;
	int16  item_nr;
	int16  seller_nr;
	int32  cost;
	char   name[64];
};

struct BazaarInspectItem_Struct {
	int8   action;
	int8   unknown1;
#define SIZEOFITEMSTRUCT sizeof(Item_Struct)
	int8   itemdata[SIZEOFITEMSTRUCT];
};

struct SpecialMsg_Struct {
	int8 unknown[6]; //Always: 00 00 37 04 0a 00 for syswide
	char message[];
};

struct ServerSideFilters_Struct {
int8	clientattackfilters; // 0) No, 1) All (players) but self, 2) All (players) but group
int8	npcattackfilters;	 // 0) No, 1) Ignore NPC misses (all), 2) Ignore NPC Misses + Attacks (all but self), 3) Ignores NPC Misses + Attacks (all but group)
int8	clientcastfilters;	 // 0) No, 1) Ignore PC Casts (all), 2) Ignore PC Casts (not directed towards self)
int8	npccastfilters;		 // 0) No, 1) Ignore NPC Casts (all), 2) Ignore NPC Casts (not directed towards self)
};

struct	ItemViewRequest_Struct {
/*000*/int16	itm_id;
/*002*/char	itm_name[64];
/*066*/
};

struct ItemViewResponse_Struct {
struct Item_Struct item;
};

struct PickPocket_Struct {
// Size 18
    uint16 to;
    uint16 from;
    uint8 myskill;
    uint8 unknown0;
    uint8 type; // -1 you are being picked, 0 failed , 1 = plat, 2 = gold, 3 = silver, 4 = copper, 5 = item
    uint8 unknown1; // 0 for response, unknown for input
    uint32 coin;
    uint8 lastsix[6];
};

struct BindWound_Struct {
// Size 4
    uint16  to; // entity id
    uint8    type; // This could be an int16
    // 0 or 1 complete, 2 Unknown, 3 ACK, 4 Died, 5 Left, 6 they moved, 7 you moved
    uint8    placeholder; //
};

// Restore structure packing to default
#pragma pack()

#endif
