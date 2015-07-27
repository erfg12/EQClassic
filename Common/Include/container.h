//o--------------------------------------------------------------
//| container.h; Harakiri, August 26, 2009
//o--------------------------------------------------------------
//| taken from the eqgame.exe sub_416FDD
//o--------------------------------------------------------------

#ifndef CONTAINER_H
#define CONTAINER_H

#include "types.h"

// Harakiri all ids verified by the eqgame.exe
#define	CONTAINER_SMALL_BAG					0x0
#define	CONTAINER_LARGE_BAG					0x1 	
#define	CONTAINER_QUIVER					0x2 	
#define	CONTAINER_BELT_POUCH				0x3 	
#define	CONTAINER_WRIST_POUCH				0x4 	
#define	CONTAINER_BACK_PACK					0x5 	
#define	CONTAINER_SMALL_CHEST				0x6 	
#define	CONTAINER_LARGE_CHEST				0x7 	
#define	CONTAINER_MEDICINE_BAG				0x9 	
#define	CONTAINER_TOOL_BOX					0xA 
#define	CONTAINER_LEXICON					0xB	
#define	CONTAINER_MORTAR					0xC 
#define	CONTAINER_SELF_DUSTING				0xD 
#define	CONTAINER_MIXING_BOWL				0xE 
#define	CONTAINER_OVEN						0xF 
#define	CONTAINER_SEWING_KIT				0x10
#define	CONTAINER_FORGE						0x11
#define	CONTAINER_FLETCHING_KIT				0x12
#define	CONTAINER_BREW_BARREL				0x13
#define	CONTAINER_JEWELERS_KIT				0x14
#define	CONTAINER_POTTERY_WHEEL				0x15
#define	CONTAINER_KILN						0x16
#define	CONTAINER_KEYMAKER					0x17
#define	CONTAINER_WIZARDS_LEXICON			0x18
#define	CONTAINER_MAGES_LEXICON				0x19
#define	CONTAINER_NECROMANCERS_LEXICON		0x1A
#define	CONTAINER_ENCHANTERS_LEXICON		0x1B
#define	CONTAINER_ALWAYS_WORKS				0x1E
#define	CONTAINER_KOADADAL_FORGE			0x1F
#define	CONTAINER_TEIRDAL_FORGE				0x20
#define	CONTAINER_OGGOK_FORGE				0x21
#define	CONTAINER_STORMGUARD_FORGE			0x22
#define	CONTAINER_AKANON_FORGE				0x23
#define	CONTAINER_NORTHMAN_FORGE			0x24
#define	CONTAINER_CABILIS_FORGEE			0x26
#define	CONTAINER_FREEPORT_FORGE			0x27
#define	CONTAINER_ROYAL_QEYNOS_FORGE		0x28
#define	CONTAINER_HALFLING_TAILORING_KIT	0x29
#define	CONTAINER_ERUD_TAILORING_KIT		0x2A
#define	CONTAINER_FIERDAL_TAILORING_KIT		0x2B
#define	CONTAINER_FIERDAL_FLETCHING_KIT		0x2C
#define	CONTAINER_IKSAR_POTTERY_WHEEL		0x2D
#define	CONTAINER_TACKLE_BOX				0x2E
#define	CONTAINER_GROBB_FORGE				0x2F
#define	CONTAINER_VALE_FORGE				0x31
#define	CONTAINER_FIERDAL_FORGE				0x30
#define	CONTAINER_ERUD_FORGE				0x32

namespace EQC
{
	namespace Common
	{
		char* GetContainerName(int8 itemType);
	}
}

#endif