// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// Adding config.h so we can put more common config values in a single place and reuse them.
// ***************************************************************

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>

#ifdef WIN32
	#include <process.h>
	#include <windows.h>
	#include <winsock.h>
#endif

#define DB_FILE "db.ini"

// World Server Port - This cannot be changed it is used by the client
const int WORLD_PORT = 9000;

// Login Server Port - This cannot be changed it is used by the client
#define LOGIN_PORT 5999

// used in zone/attack.cpp for working out damage
#define MONK_EPIC_FISTS_ITEMID 10652

// Max Shrot Zone Name
#define MAX_ZONE_NAME 16

//Max Guilds
#define MAX_GUILDS 512 //TODO: Why is max guilds 512?

// Max Server MOTD Length
#define SERVER_MOTD_LENGTH 500

// Abnormal packet size - anypacket which is larger than this size, is marked as abnomral
#define ABNORMAL_PACKET_SIZE 31500

// Used in arrays for char creation validation, must be the total name of races
// solar: if this is increased you'll have to add a column to the classrace table (Search for : ClassRaceLookupTable)
#define TOTAL_RACES	13

// Used in arrays for char creation validation, must be the total name of classes
#define TOTAL_CLASSES 15 

// The MAXIMUM a playerable char can obtain - Including GM's - Dark-Prince 22/12/2007
#define MAX_PLAYERLEVEL 60

// The MAXIMUM level a NPC can obtain - Dark-Prince 07/06/2008
#define MAX_NPCLEVEL 99

// defines for making these methods work in Windows
#ifdef WIN32
	#define snprintf	_snprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp	_stricmp
	//#define vsnprintf	_vsnprintf
#endif

// Name of the Shared Memory library
#define SHAREDMEM_NAME "EMuShareMem"

// Checksum Length of the PlayerProfile struct
#define PLAYERPROFILE_CHECKSUM_LENGTH		4

// Checksum Length of the ServerZoneEntry struct
#define SERVER_ZONE_ENTRY_CHECKSUMLENGTH	4

// Item Max Name Length
#define ITEM_MAX_NAME_LENGTH	35

// Max Item Lore text
#define ITEM_MAX_LORE_LENGTH	60

// Max Length of a players name
#define PC_MAX_NAME_LENGTH		30

// Max Length of a players Surname
#define PC_SURNAME_MAX_LENGTH	20
// Max Length of a NPC's name
#define NPC_MAX_NAME_LENGTH		30

// Max length of zone short name
#define ZONE_SHORTNAME_LENGTH	16

// Max lenth of zone long name
#define ZONE_LONGNAME_LENGTH	180

// Max amount you can have in a stat
#define STAT_CAP 255

// MAX Damage that can be inflicted with #damage
#define MAX_DAMAGE 32000

// MIN Damage that can be inflicted with #damage
#define MIN_DAMAGE 1

// MAX Player Experiance obtainable in 1 Hit -Dark-Prince
#define MAX_PC_EXPERIANCE 2140000000

// Default Item Charges - Dark-Prince
#define DEFAULT_ITEM_CHARGES 1

// Out going pakcet threashold (was orginally 9) - Dark-Prince
#define EQPACKETMANAGER_OUTBOUD_THRESHOLD 9

// Cofruben: new defines.
#define TIC_TIME	6000		// 6 Seconds.
// Harakiri: Perl has a Debug Var, dont use this #define DEBUG		10

// MAX Skill that a class trainer will train you to.
#define MAX_TRAIN_SKILL	100		// Placeholder. Max is 199 (1 base skill + 199 trains = 200 max skill)

#define MAX_TRAIN_LANGUAGE	5		// Placeholder. Max is 199 (1 base skill + 199 trains = 200 max skill)

//Yeahlight: Range for which the server will up the range about an NPC in LoS
#define NPC_UPDATE_RANGE 400

//Yeahlight: Range for which the server will up the range about a PC in LoS
#define PC_UPDATE_RANGE 1200

//Yeahlight: Maximum number of nodes in any given stored path
#define MAX_PATH_SIZE 35

//Yeahlight: Maximum number of nodes allowed in any given zone
#define MAXZONENODES 500

//Yeahlight: Maximum number of children per zone node
#define MAXZONENODESCHILDREN 100

//Yeahlight: Default roam range for zone wide roamers
#define ZONE_WIDE_ROAM 5000

//Yeahlight: Default agro range for an NPC
#define BASE_AGRO_RANGE 145.0f

//Yeahlight: Maximum number of patrolling grids for any given zone
#define MAX_GRIDS 250

//Yeahlight: Maximum number of nodes in any given patrol grid
#define MAX_GRID_SIZE 100

//Yeahlight: Maximum social range for NPC buffing
#define NPC_BUFF_RANGE 100

//Yeahlight: Base size for each hit box value
#define HIT_BOX_MULTIPLIER 65 

//Yeahlight: Default base NPC social range
#define DEFAULT_NPC_SOCIAL_RANGE 5000

//Yeahlight: Value for a null pathing node
#define NULL_NODE 9999

//Yeahlight: Value for a null buff/debuff spell
#define NULL_SPELL 50000

//Yeahlight: Heal buff reference value
#define HEAL 2

//Yeahlight: Gate spell reference value
#define GATE 3

//Yeahlight: Ratio of NPC to target hp to determine if a flee is necessary
#define FLEE_RATIO 1.45f

//Yeahlight: Maximum number of agro'ed entities in any given zone
#define MAX_ZONE_AGRO 500

//Yeahlight: Default social range to prevent flee in group NPC battles
#define NPC_ANTIFLEE_DISTANCE 150

//Yeahlight: Default social range for sharing PC leash memory by NPCs
#define NPC_LEASH_SHARE_DISTANCE 25

//Yeahlight: Total number of custom spells loaded
#define NUMBER_OF_CUSTOM_SPELLS 1

//Yeahlight: Main hand's proc interval in ms
#define	MAIN_HAND_PROC_INTERVAL 30000.00f

//Yeahlight: Off hand's proc interval in ms
#define	OFF_HAND_PROC_INTERVAL 45000.00f

//Yeahlight: Flurry and rampage's proc interval in ms
#define FLURRY_AND_RAMPAGE_PROC_INTERVAL 6000.00f

//Yeahlight: Maximum size of any given NPC's rampage list
#define RAMPAGE_LIST_SIZE 100

//Yeahlight: Resist reference value for a spell resisted via immunity
#define IMMUNE_RESIST_FLAG 555

//Yeahlight: Maximum number of entity IDs for any given zone
#define MAX_ENTITY_LIST_IDS 5000

//Yeahlight: Used with /target and /assist
#define TARGETING_RANGE	200

//Yeahlight: Frontal attack cone in degrees
#define FRONTAL_ATTACK_CONE	140

//Yeahlight: Used to enable debug messages in potentially infinite loops
//           Note: This define is also used as the range for the random pass
//                 condition. A value of 10000 or more is good.
#define ZONE_FREEZE_DEBUG 0

//Yeahlight: The range for which an NPC will consider checking for LoS to target 
//           while on a path
#define NPC_LOS_SEARCH_RANGE 200

//Yeahlight: Toggles sending damage packets to every client in the zone
#define	SEND_PC_DAMAGE_PACKETS 0

//Yeahlight: Duration in ms for a dropped item to remain on the ground
#define DROPPED_ITEM_DURATION 300000

//Yeahlight: EQC Tester admin level
#define EQC_Alpha_Tester 5

//Yeahlight: NPC leash trigger distance
#define NPC_LEASH_DISTANCE 3000

//Tazadar: EQC Total number of boats
#define TOTAL_BOATS 3

//Yeahlight: Hate modifier for sitting
#define SITTING_HATE_MODIFIER 1.30

//Yeahlight: Boat POS update range
#define BOAT_UPDATE_RANGE 3000

//Yeahlight: Default message range
#define DEFAULT_MESSAGE_RANGE 15000

//Yeahlight: Default combat message range
#define DEFAULT_COMBAT_MESSAGE_RANGE 40000

//Yeahlight: Extended rot timer
#define EXTENDED_NPC_ROT_TIMER 1800000

//Yeahlight: Default FFA loot timer
#define DEFAULT_FFA_LOOT_TIMER 165000

//Yeahlight: Default empty corpse rot timer
#define DEFAULT_EMPTY_NPC_CORPSE_ROT_TIMER 45000

//Yeahlight: PC rot timer for nooby corpses
#define PC_CORPSE_UNDER_SIX_ROT_TIMER 1800000

//Yeahlight: PC rot timer for non-nooby corpses
#define PC_CORPSE_OVER_SIX_ROT_TIMER 86400000

//Yeahlight: PC rot timer for non-nooby, naked corpses
#define PC_CORPSE_NAKED_OVER_SIX_ROT_TIMER 10800000

//Yeahlight: Automatic PC corpse DB save time duration
#define PC_CORPSE_SAVE_TIMER 1800000

//Yeahlight: The interval the corpse waits to retreave the new PC corpse rot timer from the DB
#define PC_CORPSE_INCREMENT_DECAY_TIMER 15000

//Yeahlight: Default PC corpse rez timer
#define PC_CORPSE_REZ_DECAY_TIMER 10800000

//Yeahlight: Time for the PC to answer the resurrection request
#define PC_RESURRECTION_ANSWER_DURATION 180000

//Yeahlight: How often the zone tells the World server it is still running loops
#define	ZONE_STATUS_CHECK_DELAY 10000

//Yeahlight: Boolean flag for the world server to monitor its zone servers
#define CHECKING_ZONE_SERVER_STATUS 0

//Yeahlight: Range for a projectile to collide with an entity
#define PROJECTILE_COLLISION_RANGE 4

//Yeahlight: Base starting attack speed modifier
#define BASE_MOD 100

//Yeahlight: Regen level modifiers
#define REGEN_LEVEL_FIRST  20
#define REGEN_LEVEL_SECOND 50
#define REGEN_LEVEL_THIRD  51
#define REGEN_LEVEL_FOURTH 56
#define REGEN_LEVEL_FIFTH  59

//Enraged: Time for the PC to answer the translocate request
#define PC_TRANSLOCATE_ANSWER_DURATION 20000

#endif