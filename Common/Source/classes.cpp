// ***************************************************************
//  classesEQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include "types.h"
#include "classes.h"

namespace EQC
{
	namespace Common
	{
		// Converts the int8 value of class to a text value
		char* GetClassNameW(int8 in_class)
		{
			char* tmpclass = "";

			switch(in_class)
			{
			case WARRIOR:
				tmpclass = "Warrior";
				break;
			case CLERIC:
				tmpclass = "Cleric";
				break;
			case PALADIN:
				tmpclass = "Paladin";
				break;
			case RANGER:
				tmpclass = "Ranger";
				break;
			case SHADOWKNIGHT:
				tmpclass = "ShadowKnight";
				break;
			case DRUID:
				tmpclass = "Druid";
				break;
			case MONK:
				tmpclass = "Monk";
				break;
			case BARD:
				tmpclass = "Bard";
				break;
			case ROGUE:
				tmpclass = "Rogue";
				break;
			case SHAMAN:
				tmpclass = "Shaman";
				break;
			case NECROMANCER:
				tmpclass = "Necromancer";
				break;
			case WIZARD:
				tmpclass = "Wizard";
				break;
			case MAGICIAN:
				tmpclass = "Magician";
				break;
			case ENCHANTER:
				tmpclass = "Enchanter";
				break;
			case BEASTLORD:
				tmpclass = "Beastlord";
				break;
			case BANKER:
				tmpclass = "Banker";
				break;
			case WARRIORGM:
				tmpclass = "Warrior Guildmaster";
				break;
			case CLERICGM:
				tmpclass = "Cleric Guildmaster";
				break;
			case PALADINGM:
				tmpclass = "Paladin Guildmaster";
				break;
			case RANGERGM:
				tmpclass = "Ranger Guildmaster";
				break;
			case SHADOWKNIGHTGM:
				tmpclass = "Shadowknight Guildmaster";
				break;
			case DRUIDGM:
				tmpclass = "Druid Guildmaster";
				break;
			case MONKGM:
				tmpclass = "Monk Guildmaster";
				break;
			case BARDGM:
				tmpclass = "Bard Guildmaster";
				break;
			case ROGUEGM:
				tmpclass = "Rogue Guildmaster";
				break;
			case SHAMANGM:
				tmpclass = "Shaman Guildmaster";
				break;
			case NECROMANCERGM:
				tmpclass = "Necromancer Guildmaster";
				break;
			case WIZARDGM:
				tmpclass = "Wizard Guildmaster";
				break;
			case MAGICIANGM:
				tmpclass = "Magician Guildmaster";
				break;
			case ENCHANTERGM:
				tmpclass = "Enchantet Guildmaster";
				break;
			case BEASTLORDGM:
				tmpclass = "Beastlord Guildmaster";
				break;
			case MERCHANT:
				tmpclass = "Merchant";
				break;
			default:
				tmpclass = "Unknown Class"; // Unknown class passed, return Unknown Class
				break;
			}

			return tmpclass; // return the text value
		}
	}
}