// ***************************************************************
//  Player Profile   ·  date: 7/06/2008
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifndef PLAYERPROFILE_H
#define PLAYERPROFILE_H

#include "types.h"
#include "config.h"
#include "GuildNetwork.h"
#include "../../Zone/Include/spdat.h"

using namespace EQC::Common::Network;

// Compiler override to ensure byte aligned structures
#pragma pack(1)

/*
** Buffs
** Length: 10 Bytes
** Used in:
**    playerProfileStruct(2d20)
*/
struct SpellBuff_Struct
{
	/*000*/int8  visable;		// Comment: Cofruben: 0 = Buff not visible, 1 = Visible and permanent buff(Confirmed by Tazadar) , 2 = Visible and timer on(Confirmed by Tazadar) 
	/*001*/int8  level;			// Comment: Level of person who casted buff
	/*002*/int8  bard_modifier;	// Comment: Harakiri: this seems to be the bard modifier, it is normally 0x0A because we set in in the CastOn_Struct when its not a bard, else its the instrument mod
	/*003*/int8  b_unknown3;	// Comment: ***Placeholder
	/*004*/int16 spellid;		// Comment: Unknown -> needs confirming -> ID of spell?
	/*006*/int32 duration;		// Comment: Unknown -> needs confirming -> Duration in ticks
};

// Length: 10
struct ItemProperties_Struct {

int8	unknown01[2];
sint8	charges;				// Comment: Harakiri signed int because unlimited charges are -1
int8	unknown02[7];
};

/*
** Player Profile
** Length: 8104 Bytes
*/
struct PlayerProfile_Struct
{
/*0000*/ int8   checksum[PLAYERPROFILE_CHECKSUM_LENGTH];				// Comment: 
/*0004*/ char   name[PC_MAX_NAME_LENGTH];					// Comment: First Name of Character
/*0034*/ char   Surname[PC_SURNAME_MAX_LENGTH];				// Comment: Last Name of Character
/*0054*/ int8   gender;						// Comment: (Confirmed by Sp0tter).
/*0055*/ int8   deity;						// Comment: Bertoxxoulus=0x10, (Confirmed by Sp0tter). Needs full deity list fixed in header. 
/*0056*/ int16  race;						// Comment: Race of Character
/*0058*/ int8   class_;						// Comment: Class of Character
/*0059*/ int8   pp_unknown3;				// Comment: Cofruben: should be uknownn?
/*0060*/ int8   level;						// Comment: Level of Character
/*0061*/ int8   pp_unknown4[3];				// Comment: 
/*0064*/ int32  exp;						// Comment: Needs confirming -> Current Experiance Character has?
/*0068*/ int16	trainingpoints;				// Comment: Class training points (Confirmed by Wizzel)
/*0070*/ int16  mana;						// Comment: Needs confirming -> Current Mana Character has?
/*0072*/ int8	face;						// Comment: What face the Character has? (Values?)
/*0073*/ int8   unknown0073[47];			// Comment: 
/*0120*/ int16  cur_hp;						// Comment: Needs Confirming -> Current HP of Character
/*0122*/ int8   pp_unknown7;				// Comment: 
/*0123*/ int8   STR;						// Comment: STR of Character
/*0124*/ int8   STA;						// Comment: STA of Character
/*0125*/ int8   CHA;						// Comment: CHA of Character
/*0126*/ int8   DEX;						// Comment: DEX of Character
/*0127*/ int8   INT;						// Comment: INT of Character
/*0128*/ int8   AGI;						// Comment: AGI of Character
/*0129*/ int8   WIS;						// Comment: WIS of Character
/*0130*/ int8   languages[24];				// Comment: Language of a char (Confirmed by Harakiri)
/*0154*/ int8   pp_unknown8_2[14];			// Comment: 
/*0168*/ uint16 inventory[30];				// Comment: Id of items in inventory (Confirmed by Tazadar)
/*0228*/ uint32 inventoryitemPointers[30];	// Comment: Harakiri these seems to be only used by the client, they are the pointers to the actual item_struct of each item see offset 004C71A3, 
											// if these are not cleared before sending, the client will print "Got a bogus item, deleting it" - another indication these are pointers
/*0348*/ struct	ItemProperties_Struct	invItemProprieties[30]; // Comment: infos about the items in the inventory (confirmed by Tazadar)
/*0648*/ struct SpellBuff_Struct buffs[BUFF_COUNT];	// Comment:
/*0798*/ uint16 containerinv[80];           // Comment: Ids of items in bags (confirmed by Tazadar)
/*0958*/ uint16 cursorbaginventory[10];     // Comment: Ids of items in the bag of your cursor (confirmed by Tazadar)
/*0978*/ struct	ItemProperties_Struct	bagItemProprieties[80]; // Comment: infos about the items in the bag (confirmed by Tazadar)
/*1778*/ struct	ItemProperties_Struct	cursorItemProprieties[10]; // Comment: infos about the items in the cursor bag (confirmed by Tazadar)
/*1878*/ short  spell_book[256];			// Comment: 
/*2390*/ short  spell_memory[8];			// Comment: 
/*2406*/ int8   pp_unknown10[2];			// Comment: 
/*2408*/ float  y;							// Comment: 
/*2412*/ float  x;							// Comment: 
/*2416*/ float  z;							// Comment: 
/*2420*/ float  heading;					// Comment: 
/*2424*/ char   current_zone[15];			// Comment: 
/*2439*/ int8   pp_unknown13[21];			// Comment: 
/*2460*/ int32  platinum;					// Comment: 
/*2464*/ int32  gold;						// Comment: 
/*2468*/ int32  silver;						// Comment: 
/*2472*/ int32  copper;						// Comment: 
/*2476*/ int32  platinum_bank;				// Comment: 
/*2480*/ int32  gold_bank;					// Comment: 
/*2484*/ int32  silver_bank;				// Comment: 
/*2488*/ int32  copper_bank;				// Comment: 
/*2492*/ int32  platinum_cursor;			// Comment:
/*2496*/ int32  gold_cursor;				// Comment:
/*2500*/ int32  silver_cursor;				// Comment:
/*2504*/ int32  copper_cursor;				// Comment:
/*2508*/ int8   skills[74];					// Comment: FF = Cannot Learn, FE = Have Not Trained Yet (Confirmed by Wizzel)
/*2582*/ int8	unknown2582[34];			// Comment: 
/*2616*/ int8	test_unknown;				// Comment:
/*2617*/ int8	unknown2617[127];			// Comment: Harakiri some of these bits are very important, if they are not 0 the client will request something from the server with opcode 0x0e40 during login and will not load the zone further (there seems be a 60sec timeout on this), the opcode will be sent from sub_4D5968 if at byte 2648 there is a bit > 0, this bit is set by sending OP_ZoneUnavailable 0xa220 - seems like this is the fallback zone when one zone is down?
/*2744*/ int8	autosplit;					// Comment: 00 = autosplit off ; 01 = autosplit on (Confirmed by Tazadar)
/*2745*/ int8	unknown2745[3];				// Comment:
/*2748*/ int8   pvpEnabled;					// Comment: Harakiri Client sets this bit when sending OP_PKAcknowledge back with 1, player name does not go red tho
/*2749*/ int8	unknown2749[15];			// Comment:
/*2764*/ int8	gm;							// Comment: 
/*2765*/ int8	unknown2765[23]; 			// Comment: 
/*2788*/ int8   discplineAvailable;			// Comment: Harakiri needs to be 1 or the client wont sent opcode
/*2789*/ int8	unknown2789[23];
/*2812*/ int32	hungerlevel;				// Comment: Harakiri: the clients max level is actually 6000 for rightclick check, however to hardcap is 32k ("You could not possibly eat any more, you would explode!"), interesting to know is - MONK class has a longer period to autoconsume food in the client (92000 vs 46000) Min(not hungry): 32000+ Max(very hungry): 0 (Confirmed by Wizzel)
/*2816*/ int32	thirstlevel;				// Comment: Harakiri: the clients max level is actually 6000 for rightclick check, however to hardcap is 32k ("You could not possibly drink any more, you would explode!") Min(not thirsty): 32000+ Max(very thirsty): 0 (Confirmed by Wizzel)
/*2820*/ int8	unknown2820[24];			// Comment: 
/*2844*/ char   bind_point_zone[20];		// Comment: 
/*2864*/ char   start_point_zone[4][20];	// Comment: 
/*2944*/ struct	ItemProperties_Struct bankinvitemproperties[8];			// Comment: infos of items in bank (Confirmed by Tazadar)
/*3024*/ struct	ItemProperties_Struct bankbagitemproperties[80];		// Comment: infos of items in bank containers (Confirmed by Tazadar)
/*3824*/ int8	unknown2944[4];				// Comment: 
/*3828*/ float	bind_location[3][5];		// Comment: Bind affinity location, Harakiri Confirmed by client method translocatePC in eqgame offset 00495996 : bind_location[1][1]; == x bind location bind_location[0][1] == y bind location bind_location[2][1]; == z bind location
/*3888*/ int8	unknown3888[24];			// Comment: 
/*3912*/ int32  bankinvitemPointers[8];		// Comment: Harakiri these seems to be only used by the client, they are the pointers to the actual item_struct of each item see offset 004C71A3
/*3944*/ int8	unknown3944[12];
/*3956*/ int32  time1;						// Comment: 
/*3960*/ int8   unknown3960[20];			// Comment: 
/*3980*/ int16	bank_inv[8];				// Comment: id of items in bank (Confirmed by Tazadar)
/*3996*/ int16	bank_cont_inv[80];			// Comment: id of items in bank containers  (Confirmed by Tazadar)
/*4156*/ int8	unknown4156[2];				// Comment: 
/*4158*/ int16	guildid;					// Comment: 
/*4160*/ int32  time2;						// Comment:
/*4164*/ int8	unknown4164[6];			    // Comment:
/*4170*/ int8	fatigue;					// Comment: full endu => fatigue = 0 ; zero endu => fatigue = 100 (Confirmed by Tazadar)
/*4171*/ int8	unknown4171[2];				// Comment:
/*4173*/ int8	anon;						// Comment: 
/*4174*/ int8	unknown4174;				// Comment: 
/*4175*/ GUILDRANK	guildrank;				// Comment: 0=member, 1=officer, 2=leader
/*4176*/ int8	drunkeness;					// Comment: 0 = Not Drunk, 200ish = Max Drunk (Confirmed by Wizzel)
/*4177*/ int8	eqbackground;				// Comment: Cofruben: if not set to 0, it'll show the eq background (unuseful?)		
/*4178*/ int8	unknown4178;
/*4179*/ int8	unknown4179;
/*4180*/ uint32 spellSlotRefresh[8];		// Comment: Harakiri time in ms before a spell slot refreshes, i.e. after zoning to prevent that players refresh a spell just through camp or zone, i havent found out why it doesnt work for spell slot 1
/*4212*/ int8	unknown4180[4];
/*4216*/ uint32 abilityCooldown;			// Comment: Harakiri cooldown for Lay on Hands or Death Touch from Paladin/SK in ms
/*4220*/ char	GroupMembers[6][48];		// Comment: Cofruben: 15 max length of each player name.
/*4508*/ int8	unknown4532[3592];			// Comment:
/*8100*/ int32	logtime;                    // Comment: Tazadar: The client does not seem to use the last bytes so we store the time in order to delete no rent items
/*8104*/
};

// Restore structure packing to default
#pragma pack()

#endif