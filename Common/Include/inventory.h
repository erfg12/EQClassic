#include "itemtypes.h"


/*
** Inventory Slot Equipment Enum
** Mostly used for third-party tools to reference inventory slots
**
** NOTE: Numbering for personal inventory goes top to bottom, then left to right
**	It's the opposite for inside bags: left to right, then top to bottom
**	Example:
**	inventory:	containers:
**	1 6			1 2
**	2 7			3 4
**	3 8			5 6
**	4 9			7 8
**	5 10		9 10
**
*/
enum InventorySlot
{
	////////////////////////
	// Equip slots
	////////////////////////
	
	SLOT_CHARM		= 0,
	SLOT_EAR01		= 1,
	SLOT_HEAD		= 2,
	SLOT_FACE		= 3,
	SLOT_EAR02		= 4,
	SLOT_NECK		= 5,
	SLOT_SHOULDER	= 6,
	SLOT_ARMS		= 7,
	SLOT_BACK		= 8,
	SLOT_BRACER01	= 9,
	SLOT_BRACER02	= 10,
	SLOT_RANGE		= 11,
	SLOT_HANDS		= 12,
	SLOT_PRIMARY	= 13,
	SLOT_SECONDARY	= 14,
	SLOT_RING01		= 15,
	SLOT_RING02		= 16,
	SLOT_CHEST		= 17,
	SLOT_LEGS		= 18,
	SLOT_FEET		= 19,
	SLOT_WAIST		= 20,
	SLOT_AMMO		= 21,
	
	////////////////////////
	// All other slots
	////////////////////////
	SLOT_PERSONAL_BEGIN = 22,
	SLOT_PERSONAL_END = 29,
	
	SLOT_CURSOR		= 30,
	
	SLOT_CURSOR_END	= (sint16)0xFFFE,	// Last item on cursor queue
	// Cursor bag slots are 331->340 (10 slots)
	
	// Personal Inventory Slots
	// Slots 1 through 8 are slots 22->29
	// Inventory bag slots are 251->330 (10 slots per bag)
	
	// Tribute slots are 400-404? (upper bound unknown)
	// storing these in worn item's map
	
	// Bank slots
	// Bank slots 1 through 16 are slots 2000->2015
	// Bank bag slots are 2031->2190
	
	// Shared bank slots
	// Shared bank slots 1 through 2 are slots 2500->2501
	// Shared bank bag slots are 2531->2550
	
	// Trade session slots
	// Trade slots 1 through 8 are slots 3000->3007
	// Trade bag slots are technically 0->79 when passed to client,
	// but in our code, we treat them as slots 3100->3179
	
	// Slot used in OP_TradeSkillCombine for world tradeskill containers
	SLOT_TRADESKILL = 1000,
	SLOT_AUGMENT = 1001,
	// SLOT_POWER_SOURCE = 9999,
	// Value recognized by client for destroying an item
	SLOT_INVALID = (sint16)0xFFFF
};

// Indexing positions into item material arrays
#define MATERIAL_HEAD		0
#define MATERIAL_CHEST		1
#define MATERIAL_ARMS		2
#define MATERIAL_BRACER		3
#define MATERIAL_HANDS		4
#define MATERIAL_LEGS		5
#define MATERIAL_FEET		6
#define MATERIAL_PRIMARY	7
#define MATERIAL_SECONDARY	8
#define MAX_MATERIALS 9	//number of equipables

#define StackSize 20
