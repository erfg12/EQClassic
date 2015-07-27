//o--------------------------------------------------------------
//| SpellData.cpp; Yeahlight, Aug 24, 2009
//o--------------------------------------------------------------
//| File for all the missing / hidden spell data in spdat.eff
//o--------------------------------------------------------------
#include <cmath>
#include <queue>

#include "Config.h"
#include "skills.h"
#include "mob.h"
#include "client.h"
#include "PlayerCorpse.h"
#include "zone.h"
#include "spdat.h"
#include "map.h"
#include "moremath.h"
#include "SpellsHandler.hpp"
#include <fstream>

#include "eq_opcodes.h"
#include "eq_packet_structs.h"
#include "packet_functions.h"
#include "PlayerProfile.h"
#include "EQCException.hpp"
#include "itemtypes.h"
#include <cstdio>

using namespace EQC::Common::Network;
using namespace std;
extern EntityList		entity_list;
extern SpellsHandler	spells_handler;
extern Database			database;
extern Zone*			zone;

//o--------------------------------------------------------------
//| GetCharmLevelCap; Yeahlight, Aug 24, 2009
//o--------------------------------------------------------------
//| Returns the highest level that a given charm spell will work
//o--------------------------------------------------------------
int16 Mob::GetCharmLevelCap(Spell* spell)
{
	//Yeahlight: No valid spell was supplied; abort
	if(spell == NULL || (spell && spell->IsValidSpell() == false))
		return 0;

	//Yeahlight: Switch based on the spell's ID
	switch(spell->GetSpellID())
	{
		//Yeahlight: Allure [Charm up to level 51]
		case 184:
		//Yeahlight: Cajole Undead [Charm up to level 51]
		case 198:
		//Yeahlight: Solon's Bewitching Bravura [Charm up to level 51]
		case 750:
		{
			return 51;
			break;
		}
		//Yeahlight: Allure of the Wild [Charm up to level 49]
		case 142:
		//Yeahlight: Alluring Whispers [Charm up to level 49]
		case 1817:
		{
			return 49;
			break;
		}		
		//Yeahlight: Befriend Animal [Charm up to level 24]
		case 245:
		{
			return 24;
			break;
		}
		//Yeahlight: Beguile [Charm up to level 37]
		case 182:
		//Yeahlight: Solon's Song of the Sirens [Charm up to level 37]
		case 725:
		{
			return 37;
			break;
		}
		//Yeahlight: Beguile Animals [Charm up to level 43]
		case 141:
		{
			return 43;
			break;
		}
		//Yeahlight: Beguile Plants [Charm up to level 25]
		case 753:
		{
			return 25;
			break;
		}
		//Yeahlight: Beguile Undead [Charm up to level 46]
		case 197:
		//Yeahlight: Cajoling Whispers [Charm up to level 46]
		case 183:
		{
			return 46;
			break;
		}
		//Yeahlight: Boltran's Agacerie [Charm up to level 53]
		case 1706:
		//Yeahlight: Boltran`s Agacerie [Charm up to level 53]
		case 1705:
		//Yeahlight: Call of Karana [Charm up to level 53]
		case 1553:
		//Yeahlight: Thrall of Bones [Charm up to level 53]
		case 1624:
		{
			return 53;
			break;
		}
		//Yeahlight: Charm [Charm up to level 25]
		case 300:
		{
			return 25;
			break;
		}
		//Yeahlight: Charm Animals [Charm up to level 33]
		case 260:
		{
			return 33;
			break;
		}
		//Yeahlight: Dictate [Charm up to level 58]
		case 1707:
		{
			return 58;
			break;
		}
		//Yeahlight: Dominate Undead [Charm up to level 32]
		case 196:
		{
			return 32;
			break;
		}
		//Yeahlight: Dragon Charm [Charm all levels]
		case 841:
		//Yeahlight: Vampire Charm [Charm all levels]
		case 912:
		//Yeahlight: All Your Base Are Belong To Us [Charm all levels]
		case 2009:
		{
			return 999;
			break;
		}
		//Yeahlight: Enslave Death [Charm up to level 55]
		case 1629:
		{
			return 55;
			break;
		}
		//Yeahlight: HarpyCharm [Unknown]
		case 923:
		{
			return 0;
			break;
		}
		//Yeahlight: Persuade [Charm up to level 35]
		case 1822:
		//Yeahlight: Tunare's Request [Charm up to level 35]
		case 1556:
		{
			return 35;
			break;
		}
		default:
		{
			return 0;
			break;
		}
	}

	return 0;
}

//o--------------------------------------------------------------
//| GetMezLevelCap; Yeahlight, Aug 24, 2009
//o--------------------------------------------------------------
//| Returns the highest level that a given mez spell will work
//o--------------------------------------------------------------
int16 Mob::GetMezLevelCap(Spell* spell)
{
	//Yeahlight: No valid spell was supplied; abort
	if(spell == NULL || (spell && spell->IsValidSpell() == false))
		return 0;

	//Yeahlight: Switch based on the spell's ID
	switch(spell->GetSpellID())
	{
		//Yeahlight: Disjunction [Mesmerize (2/55)]
		case 1813:
		//Yeahlight: Screaming Terror [Mesmerize (1/55)]
		case 549:
		//Yeahlight: Dazzle [Mesmerize (2/55)]
		case 190:
		//Yeahlight: Enthrall [Mesmerize (2/55)]
		case 187:
		//Yeahlight: Entrance [Mesmerize (2/55)]
		case 188:
		//Yeahlight: Fascination [Mesmerize (2/55)]
		case 1690:
		//Yeahlight: Harpy Voice [Mesmerize (2/55)]
		case 986:
		//Yeahlight: Melodious Befuddlement [Mesmerize (2/55)]
		case 1808:
		//Yeahlight: Mesmerization [Mesmerize (2/55)]
		case 307:
		//Yeahlight: Mesmerize [Mesmerize (2/55)]
		case 292:
		//Yeahlight: Mistmoore Charm [Mesmerize (2/55)]
		case 1268:
		//Yeahlight: Song of Twilight [Mesmerize (2/55)]
		case 1753:
		{
			return 55;
			break;
		}
		//Yeahlight: Glamour of Kintaz [Mesmerize (2/57)]
		case 1691:
		{
			return 57;
			break;
		}
		//Yeahlight: Rapture [Mesmerize (2/61)]
		case 1692:
		{
			return 61;
			break;
		}
		//Yeahlight: Kelin's Lucid Lullaby  [Mesmerize (1/30)]
		case 724:
		{
			return 30;
			break;
		}
		//Yeahlight: Crission`s Pixie Strike [Mesmerize (1/45)]
		case 741:
		{
			return 45;
			break;
		}
		default:
		{
			return 0;
			break;
		}
	}

	return 0;
}

//o--------------------------------------------------------------
//| GetSpellHate; Yeahlight, Mar 12, 2009
//o--------------------------------------------------------------
//| Returns the total hate generated for a spell
//o--------------------------------------------------------------
sint16 Mob::GetSpellHate(Spell* spell, bool resisted)
{
	//Yeahlight: No valid spell was supplied; abort
	if(spell == NULL || (spell && spell->IsValidSpell() == false))
		return 0;

	sint16 hate = 0;
	TSpellEffect effect_id = SE_Blank;
	int16 minSpellLevel = spell->GetMinLevel();

	//Yeahlight: Generate hate based on each of the spell's effects
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		effect_id = spell->GetSpellEffectID(i);
		//Yeahlight: A spell effect has been identified; retrieve its hate value
		if(effect_id < SE_Blank)
		{
			hate = hate + GetSpellHate(effect_id, minSpellLevel, resisted);
		}
	}
	return hate;
}

//o--------------------------------------------------------------
//| GetSpellHate; Yeahlight, Mar 12, 2009
//o--------------------------------------------------------------
//| Returns hate based on effect type and spell level
//| TODO: This requires a TON of tweaking and balancing
//o--------------------------------------------------------------
sint16 Mob::GetSpellHate(TSpellEffect effect_id, int spell_level, bool resisted, sint32 amount)
{
	sint16 hate = 0;
	float levelModifier = spell_level * 2;

	//Yeahlight: Switch based on spell type
	switch(effect_id)
	{
		case SE_ArmorClass: case SE_ATK: case SE_STR: case SE_DEX: case SE_AGI: case SE_STA: case SE_INT: case SE_WIS: case SE_CHA: 
		{
			hate = 1 * levelModifier;
			break;
		}
		case SE_Stun: case SE_SpinTarget:
		{
			hate = 3 * levelModifier;
			break;
		}
		case SE_AttackSpeed:
		{
			hate = 6 * levelModifier;
			break;
		}
		case SE_MovementSpeed:
		{
			hate = 4 * levelModifier;
			break;
		}
		case SE_Charm:
		{
			hate = 6 * levelModifier;
			break;
		}
		case SE_Mez:
		{
			hate = 4 * levelModifier;
			break;
		}
		case SE_Fear:
		{
			hate = 2 * levelModifier;
			break;
		}
		case SE_Root:
		{
			hate = 2 * levelModifier;
			break;
		}
		case SE_CancelMagic:
		{
			hate = levelModifier/2;
			break;
		}
		case SE_Blind:
		{
			hate = 2 * levelModifier;
			break;
		}
		case SE_CurrentMana:
		{
			if(amount)
				hate = abs(2 * amount);
			else
				hate = 2 * levelModifier;
			break;
		}
		case SE_Lull:
		{
			if(resisted)
				hate = 4 * levelModifier;
			else
				hate = 0;
			break;
		}
		default:
		{
			hate = 0;
			break;
		}
	}

	//Yeahlight: Resists return partial hate
	if(resisted)
		return (hate / 2);
	else
		return hate;
}

//o--------------------------------------------------------------
//| GetSpellAttackSpeedModifier; Yeahlight, Dec 21, 2008
//o--------------------------------------------------------------
//| Returns the amount of spell haste/slow of a given spell
//| Note: A 35% slow is represnted by the number 65 and a 35%
//|       haste is represented by the number 135
//o--------------------------------------------------------------
int16 Mob::GetSpellAttackSpeedModifier(Spell* spell, int8 casterLevel)
{
	//Yeahlight: No valid spell was supplied; abort
	if(spell == NULL || (spell && spell->IsValidSpell() == false))
		return BASE_MOD;

	//Yeahlight: Switch based on the spell's ID
	switch(spell->GetSpellID())
	{
		//Yeahlight: Celestial Tranquility [Increase Attack Speed by 40% (L1)]
		case 1940:
		{
			sint16 maxModifier = 40;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Jonthan's Whistling Warsong [Increase Attack Speed for between 16% (L7) to 25% (L40)]
		case 734:
		{
			sint16 maxModifier = 25;
			sint16 minModifier = 16;
			int16  maxLevel    = 40;
			int16  minLevel    = 7;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Seething Fury [Increase Attack Speed by 40% (L1)]
		case 1927:
		{
			sint16 maxModifier = 40;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Captain Nalots Quickening [Increase Attack Speed by 20% (L1)]
		case 1925:
		{
			sint16 maxModifier = 20;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Dance of the Blade [Increase Attack Speed by 55% (L1)]
		case 1937:
		{
			sint16 maxModifier = 55;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Haste [Increase Attack Speed for between 2% (L1) to 50% (L49)]
		case 998:
		{
			sint16 maxModifier = 50;
			sint16 minModifier = 2;
			int16  maxLevel    = 49;
			int16  minLevel    = 1;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Speed of the Shissar [Increase Attack Speed by 66% (L1)]
		case 1939:
		{
			sint16 maxModifier = 66;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Aanya's Quickening [Increase Attack Speed by 64% (L53)]
		case 1708:
		{
			int16 maxModifier = 64;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Alacrity [Increase Attack Speed for between 34% (L24) to 40% (L36)]
		case 170:
		{
			sint16 maxModifier = 40;
			sint16 minModifier = 34;
			int16  maxLevel    = 36;
			int16  minLevel    = 24;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Angstlich's Assonance [Decrease Attack Speed by 31% (L60)]
		case 1748:
		{
			sint16 maxModifier = -31;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Anthem de Arms [Increase Attack Speed by 10% (L10)]
		case 701:
		{
			sint16 maxModifier = 10;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Augment [Increase Attack Speed for between 43% (L56) to 45% (L60)]
		case 1729:
		{
			sint16 maxModifier = 45;
			sint16 minModifier = 43;
			int16  maxLevel    = 60;
			int16  minLevel    = 56;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Augment Death [Increase Attack Speed for between 49% (L39) to 55% (L45)]
		case 661:
		{
			sint16 maxModifier = 55;
			sint16 minModifier = 49;
			int16  maxLevel    = 45;
			int16  minLevel    = 39;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Augmentation [Increase Attack Speed for between 22% (L29) to 28% (L52)]
		case 10:
		{
			sint16 maxModifier = 28;
			sint16 minModifier = 22;
			int16  maxLevel    = 52;
			int16  minLevel    = 29;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Augmentation of Death [Increase Attack Speed by 65% (L55)]
		case 1414:
		{
			sint16 maxModifier = 65;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Blessing of the Grove [Increase Attack Speed by 40% (L1)]
		case 1964:
		{
			sint16 maxModifier = 40;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Can o` Whoop Ass [Increase Attack Speed by 200% (L1)]
		case 911:
		{
			sint16 maxModifier = 200;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Celerity [Increase Attack Speed for between 47% (L39) to 50% (L44)]
		case 171:
		{
			sint16 maxModifier = 50;
			sint16 minModifier = 47;
			int16  maxLevel    = 44;
			int16  minLevel    = 39;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Cog Boost [Increase Attack Speed by 40% (L1)]
		case 1999:
		{
			sint16 maxModifier = 40;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Feral Spirit [Increase Attack Speed for between 16% (L19) to 20% (L32)]
		case 139:
		{
			sint16 maxModifier = 20;
			sint16 minModifier = 16;
			int16  maxLevel    = 32;
			int16  minLevel    = 19;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Fist of Water [Decrease Attack Speed by 30% (L1)]
		case 795:
		{
			sint16 maxModifier = -30;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Flurry [Increase Attack Speed for between 11% (L1) to 40% (L30)]
		case 1266:
		{
			sint16 maxModifier = 40;
			sint16 minModifier = 11;
			int16  maxLevel    = 30;
			int16  minLevel    = 1;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Graveyard Dust [Unknown Formula]
		case 885:
		{
			sint16 maxModifier = 0;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Intensify Death [Increase Attack Speed for between 22% (L24) to 30% (L40)]
		case 449:
		{
			sint16 maxModifier = 30;
			sint16 minModifier = 22;
			int16  maxLevel    = 40;
			int16  minLevel    = 24;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Jonthan's Inspiration [Increase Attack Speed for between 61% (L58) to 63% (L60)]
		case 1762:
		{
			sint16 maxModifier = 63;
			sint16 minModifier = 61;
			int16  maxLevel    = 60;
			int16  minLevel    = 58;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Jonthan's Provocation [Increase Attack Speed for between 48% (L45) to 50% (L47)]
		case 749:
		{
			sint16 maxModifier = 50;
			sint16 minModifier = 48;
			int16  maxLevel    = 47;
			int16  minLevel    = 45;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: McVaxius` Berserker Crescendo [Increase Attack Speed for between 18% (L42) to 23% (L60)]
		case 702:
		{
			sint16 maxModifier = 23;
			sint16 minModifier = 18;
			int16  maxLevel    = 60;
			int16  minLevel    = 42;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: McVaxius` Rousing Rondo [Increase Attack Speed for between 21% (L57) to 22% (L60)]
		case 1760:
		{
			sint16 maxModifier = 22;
			sint16 minModifier = 21;
			int16  maxLevel    = 60;
			int16  minLevel    = 57;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Potion of Assailing [Increase Attack Speed for between 30% (L1) to 50% (L60)]
		case 883:
		{
			sint16 maxModifier = 50;
			sint16 minModifier = 30;
			int16  maxLevel    = 60;
			int16  minLevel    = 1;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Quickness [Increase Attack Speed for between 28% (L16) to 30% (L20)]
		case 39:
		{
			sint16 maxModifier = 30;
			sint16 minModifier = 28;
			int16  maxLevel    = 20;
			int16  minLevel    = 16;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Savage Spirit [Increase Attack Speed for between 64% (L44) to 70% (L50)]
		case 140:
		{
			sint16 maxModifier = 70;
			sint16 minModifier = 64;
			int16  maxLevel    = 50;
			int16  minLevel    = 44;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Strengthen Death [Increase Attack Speed by 20% (L29)]
		case 1289:
		{
			sint16 maxModifier = 20;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Swift like the Wind [Increase Attack Speed by 60% (L49)]
		case 172:
		{
			sint16 maxModifier = 60;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Swift Spirit [Increase Attack Speed by 40% (L1)]
		case 1929:
		{
			sint16 maxModifier = 40;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Verses of Victory [Increase Attack Speed by 30% (L50)]
		case 747:
		{
			sint16 maxModifier = 30;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Vilia`s Chorus of Celerity [Increase Attack Speed by 45% (L54)]
		case 1757:
		{
			sint16 maxModifier = 45;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Vilia`s Verses of Celerity [Increase Attack Speed by 20% (L36)]
		case 740:
		{
			sint16 maxModifier = 20;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Visions of Grandeur [Increase Attack Speed by 58% (L60)]
		case 1710:
		{
			sint16 maxModifier = 58;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Wonderous Rapidity [Increase Attack Speed by 70% (L58)]
		case 1709:
		{
			sint16 maxModifier = 70;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Selo`s Consonant Chain [Decrease Attack Speed for between 16% (L23) to 35% (L60)]
		case 738:
		{
			sint16 maxModifier = -35;
			sint16 minModifier = -16;
			int16  maxLevel    = 60;
			int16  minLevel    = 23;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Song of the Deep Sea's [Increase Attack Speed by 60% (L1)]
		case 1973:
		{
			sint16 maxModifier = 60;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Largo`s Absonant Binding [Decrease Attack Speed for between 30% (L51) to 35% (L60)]
		case 1751:
		{
			sint16 maxModifier = -35;
			sint16 minModifier = -30;
			int16  maxLevel    = 60;
			int16  minLevel    = 51;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Largo`s Melodic Binding [Decrease Attack Speed for between 15% (L20) to 35% (L60)]
		case 705:
		{
			sint16 maxModifier = -35;
			sint16 minModifier = -15;
			int16  maxLevel    = 60;
			int16  minLevel    = 20;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Ancient Breath [Decrease Attack Speed by 40% (L1)]
		case 1486:
		{
			sint16 maxModifier = -40;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Breath of the Sea [Decrease Attack Speed by 20% (L1)]
		case 1978:
		{
			sint16 maxModifier = -20;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Daze [Decrease Attack Speed by 1% (L1)]
		case 983:
		{
			sint16 maxModifier = -1;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Dazed [Decrease Attack Speed by 1% (L1)]
		case 984:
		{
			sint16 maxModifier = -1;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Drowsy [Decrease Attack Speed for between 11% (L5) to 25% (L60)]
		case 270:
		{
			sint16 maxModifier = -25;
			sint16 minModifier = -11;
			int16  maxLevel    = 60;
			int16  minLevel    = 5;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Earthcall [Decrease Attack Speed for between 20% (L1) to 50% (L60)]
		case 1928:
		{
			sint16 maxModifier = -50;
			sint16 minModifier = -20;
			int16  maxLevel    = 60;
			int16  minLevel    = 1;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Energy Sap [Decrease Attack Speed for between 20% (L1) to 35% (L60)]
		case 1960:
		{
			sint16 maxModifier = -35;
			sint16 minModifier = -20;
			int16  maxLevel    = 60;
			int16  minLevel    = 1;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Forlorn Deeds [Decrease Attack Speed for between 67% (L57) to 70% (L60)]
		case 1712:
		{
			sint16 maxModifier = -70;
			sint16 minModifier = -67;
			int16  maxLevel    = 60;
			int16  minLevel    = 57;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Languid Pace [Decrease Attack Speed for between 18% (L12) to 30% (L60)]
		case 302:
		{
			sint16 maxModifier = -30;
			sint16 minModifier = -18;
			int16  maxLevel    = 60;
			int16  minLevel    = 12;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Lethargy [Decrease Attack Speed for between 20% (L1) to 50% (L60)]
		case 1315:
		{
			sint16 maxModifier = -50;
			sint16 minModifier = -20;
			int16  maxLevel    = 60;
			int16  minLevel    = 1;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Mort Drowsy [Decrease Attack Speed for between 11% (L1) to 70% (L60)]
		case 933:
		{
			sint16 maxModifier = -70;
			sint16 minModifier = -11;
			int16  maxLevel    = 60;
			int16  minLevel    = 1;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Shiftless Deeds [Decrease Attack Speed for between 49% (L44) to 65% (L60)]
		case 186:
		{
			sint16 maxModifier = -65;
			sint16 minModifier = -49;
			int16  maxLevel    = 60;
			int16  minLevel    = 44;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Slow [Decrease Attack Speed for between 10% (L1) to 25% (L60)]
		case 954:
		{
			sint16 maxModifier = -25;
			sint16 minModifier = -10;
			int16  maxLevel    = 60;
			int16  minLevel    = 1;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Tagar's Insects [Decrease Attack Speed for between 34% (L29) to 50% (L60)]
		case 506:
		{
			sint16 maxModifier = -50;
			sint16 minModifier = -34;
			int16  maxLevel    = 60;
			int16  minLevel    = 29;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Tepid Deeds [Decrease Attack Speed for between 32% (L24) to 50% (L60)]
		case 185:
		{
			sint16 maxModifier = -50;
			sint16 minModifier = -32;
			int16  maxLevel    = 60;
			int16  minLevel    = 24;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Tigir's Insects [Decrease Attack Speed for between 49% (L58) to 50% (L60)]
		case 1589:
		{
			sint16 maxModifier = -50;
			sint16 minModifier = -49;
			int16  maxLevel    = 60;
			int16  minLevel    = 58;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Togor's Insects [Decrease Attack Speed for between 49% (L39) to 70% (L60)]
		case 507:
		{
			sint16 maxModifier = -70;
			sint16 minModifier = -49;
			int16  maxLevel    = 60;
			int16  minLevel    = 39;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Turgur's Insects [Decrease Attack Speed for between 66% (L51) to 75% (L60)]
		case 1588:
		{
			sint16 maxModifier = -75;
			sint16 minModifier = -66;
			int16  maxLevel    = 60;
			int16  minLevel    = 51;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Walking Sleep [Decrease Attack Speed for between 23% (L14) to 35% (L60)]
		case 505:
		{
			sint16 maxModifier = -35;
			sint16 minModifier = -23;
			int16  maxLevel    = 60;
			int16  minLevel    = 14;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Diseased Cloud [Decrease Attack Speed by 55% (L1)]
		case 836:
		{
			sint16 maxModifier = -55;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Selo`s Chords of Cessation [Decrease Attack Speed for between 17% (L48) to 20% (L60)]
		case 746:
		{
			sint16 maxModifier = -20;
			sint16 minModifier = -17;
			int16  maxLevel    = 60;
			int16  minLevel    = 48;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Slime Mist [Decrease Attack Speed by 25% (L1)]
		case 1491:
		{
			sint16 maxModifier = -25;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Soluan`s Vigor [Decrease Attack Speed for between 10% (L1) to 25% (L60)]
		case 876:
		{
			sint16 maxModifier = -25;
			sint16 minModifier = -10;
			int16  maxLevel    = 60;
			int16  minLevel    = 1;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Waves of the Deep Sea [Decrease Attack Speed by 10% (L1)]
		case 1972:
		{
			sint16 maxModifier = -10;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Rejuvenation [Decrease Attack Speed by 20% (L1)]
		case 1252:
		{
			sint16 maxModifier = -20;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Selo`s Assonant Strane [Decrease Attack Speed for between 23% (L54) to 25% (L60)]
		case 1758:
		{
			sint16 maxModifier = -25;
			sint16 minModifier = -23;
			int16  maxLevel    = 60;
			int16  minLevel    = 54;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Aura of Marr [Decrease Attack Speed by 10% (L1)]
		case 881:
		{
			sint16 maxModifier = -10;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Brittle Haste I [Increase Attack Speed by 10% (L1)]
		case 1838:
		{
			sint16 maxModifier = 10;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Brittle Haste II [Increase Attack Speed by 15% (L1)]
		case 1849:
		{
			sint16 maxModifier = 15;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Brittle Haste III [Increase Attack Speed by 20% (L1)]
		case 1850:
		{
			sint16 maxModifier = 20;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Brittle Haste IV [Increase Attack Speed by 25% (L1)]
		case 1880:
		{
			sint16 maxModifier = 25;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: GuideActing [Increase Attack Speed by 20% (L1)]
		case 778:
		case 1209:
		{
			sint16 maxModifier = 20;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Illusion: Werewolf [Increase Attack Speed by 40% (L44)]
		case 585:
		{
			sint16 maxModifier = 40;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Freezing Breath [Decrease Attack Speed by 70% (L1)]
		case 845:
		{
			sint16 maxModifier = -70;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Paralyzing Poison I [Decrease Attack Speed by 50% (L1)]
		case 1833:
		{
			sint16 maxModifier = -50;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Paralyzing Poison II [Decrease Attack Speed by 50% (L1)]
		case 1841:
		{
			sint16 maxModifier = -50;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Paralyzing Poison III [Decrease Attack Speed by 50% (L1)]
		case 1842:
		{
			sint16 maxModifier = -50;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Spirit Quickening [Increase Attack Speed by 20% (L50)]
		case 1430:
		{
			sint16 maxModifier = 20;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Chill of Unlife [Decrease Attack Speed by 5% (L1)]
		case 1365:
		{
			sint16 maxModifier = -5;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Burnout IV [Increase Attack Speed by 65% (L55)]
		case 1472:
		{
			sint16 maxModifier = 65;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Burnout III [Increase Attack Speed by 60% (L49)]
		case 107:
		{
			sint16 maxModifier = 60;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Burnout II [Increase Attack Speed for between 29% (L29) to 35% (L40)]
		case 106:
		{
			sint16 maxModifier = 35;
			sint16 minModifier = 29;
			int16  maxLevel    = 40;
			int16  minLevel    = 29;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Burnout [Increase Attack Speed by 12% (L11) to 15% (L18)]
		case 327:
		{
			sint16 maxModifier = 15;
			sint16 minModifier = 12;
			int16  maxLevel    = 18;
			int16  minLevel    = 11;
			if(casterLevel <= minLevel)
				return BASE_MOD + minModifier;
			else if(casterLevel >= maxLevel)
				return BASE_MOD + maxModifier;
			return BASE_MOD + minModifier + (maxModifier - minModifier) * (float)(casterLevel - minLevel) / (float)(maxLevel - minLevel);
			break;
		}
		//Yeahlight: Composition of Ervaj  [Increase Haste v2 by 10%]
		case 1452:
		{
			sint16 maxModifier = 10;
			return BASE_MOD + maxModifier;
			break;
		}
		//Yeahlight: Composition of Ervaj  [Increase Haste v2 by 5%]
		case 1449:
		{
			sint16 maxModifier = 5;
			return BASE_MOD + maxModifier;
			break;
		}
		default:
		{
			return BASE_MOD;
		}
	}

	return BASE_MOD;
}