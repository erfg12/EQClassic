//o--------------------------------------------------------------
//| container.cpp; Harakiri, August 26, 2009
//o--------------------------------------------------------------
//| taken from the eqgame.exe sub_416FDD
//o--------------------------------------------------------------

#include "container.h"

namespace EQC
{
	namespace Common
	{		
		char* GetContainerName(int8 itemType) 
		{
			char* tmpname = "";
			
		   switch(itemType) {
			case CONTAINER_SMALL_BAG:
				tmpname = "Small Bag";
			   break;
			case CONTAINER_LARGE_BAG:
				tmpname = "Large Bag";
			   break;
			  case CONTAINER_QUIVER:
				tmpname = "Quiver";
			   break;
			  case CONTAINER_BELT_POUCH:
				tmpname = "Belt Pouch";
			   break;
			  case CONTAINER_WRIST_POUCH:
				tmpname = "Wrist Pouch";
			   break;
			  case CONTAINER_BACK_PACK:
				tmpname = "Back Pack";
			   break;
			  case CONTAINER_SMALL_CHEST:
				tmpname = "Small Chest";
			   break;
			  case CONTAINER_LARGE_CHEST:
				tmpname = "Large Chest";
			   break;
			  case CONTAINER_MEDICINE_BAG:
				tmpname = "Medicine Bag";
			   break;
			  case CONTAINER_TOOL_BOX:
				tmpname = "Tool Box";
			   break;
			  case CONTAINER_LEXICON:
				tmpname = "Lexicon";
			   break;
			  case CONTAINER_MORTAR:
				tmpname = "Mortar";
			   break;
			  case CONTAINER_SELF_DUSTING:
				tmpname = "Self Dusting";
			   break;
			  case CONTAINER_MIXING_BOWL:
				tmpname = "Mixing Bowl";
			   break;
			  case CONTAINER_OVEN:
				tmpname = "Oven";
			   break;
			  case CONTAINER_SEWING_KIT:
				tmpname = "Sewing Kit";
			   break;
			  case CONTAINER_FORGE:
				tmpname = "Forge";
			   break;
			  case CONTAINER_FLETCHING_KIT:
				tmpname = "Fletching Kit";
			   break;
			  case CONTAINER_BREW_BARREL:
				tmpname = "Brew Barrel";
			   break;
			  case CONTAINER_JEWELERS_KIT:
				tmpname = "Jeweler's Kit";
			   break;
			  case CONTAINER_POTTERY_WHEEL:
				tmpname = "Pottery Wheel";
			   break;
			  case CONTAINER_KILN:
				tmpname = "Kiln";
			   break;
			  case CONTAINER_KEYMAKER:
				tmpname = "Keymaker";
			   break;
			  case CONTAINER_WIZARDS_LEXICON:
				tmpname = "Wizard's Lexicon";
			   break;
			  case CONTAINER_MAGES_LEXICON:
				tmpname = "Mage's Lexicon";
			   break;
			  case CONTAINER_NECROMANCERS_LEXICON:
				tmpname = "Necromancer's Lexicon";
			   break;
			  case CONTAINER_ENCHANTERS_LEXICON:
				tmpname = "Enchanter's Lexicon";
			   break;
			  case CONTAINER_ALWAYS_WORKS:
				tmpname = "Always Works";
			   break;
			  case CONTAINER_KOADADAL_FORGE:
				tmpname = "Koada`Dal Forge";
			   break;
			  case CONTAINER_TEIRDAL_FORGE:
				tmpname = "Teir`Dal Forge";
			   break;
			  case CONTAINER_OGGOK_FORGE:
				tmpname = "Oggok Forge";
			   break;
			  case CONTAINER_STORMGUARD_FORGE:
				tmpname = "Stormguard Forge";
			   break;
			  case CONTAINER_AKANON_FORGE:
				tmpname = "Ak`anon Forge";
			   break;
			  case CONTAINER_NORTHMAN_FORGE:
				tmpname = "Northman Forge";
			   break;
			  case CONTAINER_CABILIS_FORGEE:
				tmpname = "Cabilis Forge";
			   break;
			  case CONTAINER_FREEPORT_FORGE:
				tmpname = "Freeport Forge";
			   break;
			  case CONTAINER_ROYAL_QEYNOS_FORGE:
				tmpname = "Royal Qeynos Forge";
			   break;
			  case CONTAINER_IKSAR_POTTERY_WHEEL:
				tmpname = "Iksar Pottery Wheel";
			   break;
			  case CONTAINER_ERUD_TAILORING_KIT:
				tmpname = "Erud Tailoring Kit";
			   break;
			  case CONTAINER_HALFLING_TAILORING_KIT:
				tmpname = "Halfling Tailoring Kit";
			   break;
			  case CONTAINER_FIERDAL_TAILORING_KIT:
				tmpname = "Fier`Dal Tailoring Kit";
			   break;
			  case CONTAINER_FIERDAL_FLETCHING_KIT:
				tmpname = "Fier`Dal Fletching Kit";
			   break;
			  case CONTAINER_TACKLE_BOX:
				tmpname = "Tackle Box";
			   break;
			  case CONTAINER_GROBB_FORGE:
				tmpname = "Grobb Forge";
			   break;
			  case CONTAINER_VALE_FORGE:
				tmpname = "Vale Forge";
			   break;
			  case CONTAINER_FIERDAL_FORGE:
				tmpname = "Fier`Dal Forge";
			   break;
			  case CONTAINER_ERUD_FORGE:
				tmpname = "Erud Forge";
				break;			
			  default:
				tmpname = "unknown container"; 
				break;
			}

			return tmpname;
		}
	}
}
			
		