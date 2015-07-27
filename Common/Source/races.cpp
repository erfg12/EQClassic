// ***************************************************************
//  races   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include "eq_packet_structs.h"
#include "races.h"

namespace EQC
{
	namespace Common
	{
		// Converts the int8 race to a text value
		char* GetRaceName(int8 in_race) 
		{
			char* result = "";
			
			switch(in_race) 
			{
				case HUMAN:
					  result = "Human";
					  break;
				case BARBARIAN:
					  result = "Barbarian";
					  break;
				case ERUDITE:
				  result = "Erudite";
				  break;
				case WOOD_ELF:
				  result = "Wood Elf";
				  break;
				case HIGH_ELF:
				  result = "High Elf";
				  break;
				case DARK_ELF:
				  result = "Dark Elf";
				  break;
				case HALF_ELF:
				  result = "Half Elf";
				  break;
				case DWARF:
				  result = "Dwarf";
				  break;
				case TROLL:
				  result = "Troll";
				  break;
				case OGRE:
				  result = "Ogre";
				  break;
				case HALFLING:
				  result = "Halfling";
				  break;
				case GNOME:
				  result = "Gnome";
				  break;
				case AVIAK:
				  result = "Aviak";
				  break;
				case HIGHPASS_CITIZEN:
				  result = "Highpass Citizen";
				  break;
				case BROWNIE:
				  result = "Brownie";
				  break;
				case CENTAUR:
				  result = "Centaur";
				  break;
				case GOLEM:
				  result = "Golem";
				  break;
				case GIANT:
				  result = "Giant / Cyclops";
				  break;
				case TRAKANON:
				  result = "Trakanon"; // Harakiri misspelled as Trakenon in eqgame
				  break;
				case VENRIL_SATHIR:
				  result = "Venril Sathir"; // Harakiri misspelled as "Venril Sathor in eqgame
				  break;
				case EVIL_EYE:
				  result = "Evil Eye";
				  break;
				case BEETLE:
				  result = "Beetle";
				  break;
				case KERRA:
				  result = "Kerra";
				  break;
				case FISH:
				  result = "Fish";
				  break;
				case FAIRY:
				  result = "Fairy";
				  break;
				case FROGLOK:
				  result = "Froglok";
				  break;
				case FROGLOK_GHOUL:
				  result = "Froglok Ghoul";
				  break;
				case FUNGUSMAN:
				  result = "Fungusman";
				  break;
				case GARGOYLE:
				  result = "Gargoyle";
				  break;
				case GASBAG:
				  result = "Gasbag";
				  break;
				case GELATINOUS_CUBE:
				  result = "Gelatinous Cube";
				  break;
				case GHOST:
				  result = "Ghost";
				  break;
				case GHOUL:
				  result = "Ghoul";
				  break;
				case GIANT_BAT:
				  result = "Giant Bat";
				  break;
				case GIANT_EEL:
				  result = "Giant Eel";
				  break;
				case GIANT_RAT:
				  result = "Giant Rat";
				  break;
				case GIANT_SNAKE:
				  result = "Giant Snake";
				  break;
				case GIANT_SPIDER:
				  result = "Giant Spider";
				  break;
				case GNOLL:
				  result = "Gnoll";
				  break;
				case GOBLIN:
				  result = "Goblin";
				  break;
				case GORILLA:
				  result = "Gorilla";
				  break;
				case WOLF:
				  result = "Wolf";
				  break;
				case BEAR:
				  result = "Bear";
				  break;
				case FREEPORT_GUARDS:
				  result = "Freeport Guards";
				  break;
				case DEMI_LICH:
				  result = "Demi Lich";
				  break;
				case IMP:
				  result = "Imp";
				  break;
				case GRIFFIN:
				  result = "Griffin";
				  break;
				case KOBOLD:
				  result = "Kobold";
				  break;
				case LAVA_DRAGON:
				  result = "Lava Dragon";
				  break;
				case LION:
				  result = "Lion";
				  break;
				case LIZARD_MAN:
				  result = "Lizard Man";
				  break;
				case MIMIC:
				  result = "Mimic";
				  break;
				case MINOTAUR:
				  result = "Minotaur";
				  break;
				case ORC:
				  result = "Orc";
				  break;
				case HUMAN_BEGGAR:
				  result = "Human Beggar";
				  break;
				case PIXIE:
				  result = "Pixie";
				  break;
				case DRACNID:
				  result = "Dracnid";
				  break;
				case SOLUSEK_RO:
				  result = "Solusek Ro";
				  break;
				case BLOODGILLS:
				  result = "Bloodgills";
				  break;
				case SKELETON:
				  result = "Skeleton";
				  break;
				case SHARK:
				  result = "Shark";
				  break;
				case TUNARE:
				  result = "Tunare";
				  break;
				case TIGER:
				  result = "Tiger";
				  break;
				case TREANT:
				  result = "Treant";
				  break;
				case VAMPIRE:
				  result = "Vampire";
				  break;
				case RALLOS_ZEK:
				  result = "Rallos Zek";
				  break;
				case WEREWOLF:
				  result = "Were Wolf";
				  break;
				case TENTACLE:
				  result = "Tentacle";
				  break;
				case WILL_O_WISP:
				  result = "Will 'O Wisp";
				  break;
				case ZOMBIE:
				  result = "Zombie";
				  break;
				case QEYNOS_CITIZEN:
				  result = "Qeynos Citizen";
				  break;
				case SHIP:
				  result = "Ship";
				  break;
				case LAUNCH:
				  result = "Launch";
				  break;
				case PIRANHA:
				  result = "Piranha";
				  break;
				case ELEMENTAL:
				  result = "Elemental";
				  break;
				case BOAT:
				  result = "Boat";
				  break;
				case PUMA:
				  result = "Puma";
				  break;
				case NERIAK_CITIZEN:
				  result = "Neriak Citizen";
				  break;
				case ERUDIN_CITIZEN:
				  result = "Erudite Citizen";
				  break;
				case BIXIE:
				  result = "Bixie";
				  break;
				case REANIMATED_HAND:
				  result = "Reanimated Hand";
				  break;
				case RIVERVALE_CITIZEN:
				  result = "Rivervale Citizen";
				  break;
				case SCARECROW:
				  result = "Scarecrow";
				  break;
				case SKUNK:
				  result = "Skunk";
				  break;
				case SNAKE_ELEMENTAL:
				  result = "Snake Elemental";
				  break;
				case SPECTRE:
				  result = "Spectre";
				  break;
				case SPHINX:
				  result = "Sphinx";
				  break;
				case ARMADILLO:
				  result = "Armadillo";
				  break;
				case CLOCKWORK_GNOME:
				  result = "Clockwork Gnome";
				  break;
				case DRAKE:
				  result = "Drake";
				  break;
				case HALAS_CITIZEN:
				  result = "Halas Citizen";
				  break;
				case ALLIGATOR:
				  result = "Alligator";
				  break;
				case GROBB_CITIZEN:
				  result = "Grobb Citizen";
				  break;
				case OGGOK_CITIZEN:
				  result = "Oggok Citizen";
				  break;
				case KALADIM_CITIZEN:
				  result = "Kaladim Citizen";
				  break;
				case CAZIC_THULE:
				  result = "Cazic Thule";
				  break;
				case COCKATRICE:
				  result = "Cockatrice";
				  break;
				case DAISY_MAN:
				  result = "Daisy Man";
				  break;
				case ELF_VAMPIRE:
				  result = "Elf Vampire";
				  break;
				case DENIZEN:
				  result = "Denizen";
				  break;
				case DERVISH:
				  result = "Dervish";
				  break;
				case EFREETI:
				  result = "Efreeti";
				  break;
				case FROGLOK_TADPOLE:
				  result = "Froglok Tadpole";
				  break;
				case KEDGE:
				  result = "Kedge";
				  break;
				case LEECH:
				  result = "Leech";
				  break;
				case SWORDFISH:
				  result = "Swordfish";
				  break;
				case FELGUARD:
				  result = "Felguard";
				  break;
				case MAMMOTH:
				  result = "Mammoth";
				  break;
				case EYE_OF_ZOMM:
				  result = "Eye of Zomm";
				  break;
				case WASP:
				  result = "Wasp";
				  break;
				case MERMAID:
				  result = "Mermaid";
				  break;
				case HARPIE:
				  result = "Harpie";
				  break;
				case FAYGUARD:
				  result = "Fayguard";
				  break;
				case DRIXIE:
				  result = "Drixie";
				  break;
				case GHOST_SHIP:
				  result = "Ghost Ship";
				  break;
				case CLAM:
				  result = "Clam";
				  break;
				case SEA_HORSE:
				  result = "Sea Horse";
				  break;
				case GHOST_DWARF:
				  result = "Ghost Dwarf";
				  break;
				case ERUDITE_GHOST:
				  result = "Erudite Ghost";
				  break;
				case SABERTOOTH_CAT:
				  result = "Sabertooth Cat";
				  break;
				case WOLF_ELEMENTAL:
				  result = "Wolf Elemental";
				  break;
				case GORGON:
				  result = "Gorgon";
				  break;
				case DRAGON_SKELETON:
				  result = "Dragon Skeleton";
				  break;
				case INNORUK:
				  result = "Innoruuk";
				  break;
				case UNICORN:
				  result = "Unicorn";
				  break;
				case PEGASUS:
				  result = "Pegasus";
				  break;
				case DJINN:
				  result = "Djinn";
				  break;
				case INVISIBLE_MAN:
				  result = "Invisible Man";
				  break;
				case IKSAR:
				  result = "Iksar";
				  break;
				case SCORPION:
				  result = "Scorpion";
				  break;
				case SARNAK:
				  result = "Sarnak";
				  break;
				case DRAGLOCK:
				  result = "Draglock";
				  break;
				case LYCANTHROPE:
				  result = "Lycanthrope";
				  break;
				case MOSQUITO:
				  result = "Mosquito";
				  break;
				case RHINO:
				  result = "Rhino";
				  break;
				case XALGOZ:
				  result = "Xalgoz";
				  break;
				case KUNARK_GOBLIN:
				  result = "Kunark Goblin";
				  break;
				case YETI:
				  result = "Yeti";
				  break;
				case IKSAR_CITIZEN:
				  result = "Iksar Citizen";
				  break;
				case FOREST_GIANT:
				  result = "Forest Giant";
				  break;
				case BURYNAI:
				  result = "Burynai";
				  break;
				case GOO:
				  result = "Goo";
				  break;
				case SPECTRAL_SARNAK:
				  result = "Spectral Sarnak";
				  break;
				case SPECTRAL_IKSAR:
				  result = "Spectral Iksar";
				  break;
				case KUNARK_FISH:
				  result = "Kunark Fish";
				  break;
				case IKSAR_SCORPION:
				  result = "Iksar Scorpion";
				  break;
				case EROLLISI:
				  result = "Erollisi";
				  break;
				case TRIBUNAL:
				  result = "Tribunal";
				  break;
				case BERTOXXULOUS:
				  result = "Bertoxxulous";
				  break;
				case BRISTLEBANE:
				  result = "Bristlebane";
				  break;
				case FAY_DRAKE:
				  result = "Fay Drake";
				  break;
				case SARNAK_SKELETON:
				  result = "Sarnak Skeleton";
				  break;
				case RATMAN:
				  result = "Ratman";
				  break;
				case WYVERN:
				  result = "Wyvern";
				  break;
				case WURM:
				  result = "Wurm";
				  break;
				case DEVOURER:
				  result = "Devourer";
				  break;
				case IKSAR_GOLEM:
				  result = "Iksar Golem";
				  break;
				case IKSAR_SKELETON:
				  result = "Iksar Skeleton";
				  break;
				case MAN_EATING_PLANT:
				  result = "Man Eating Plant";
				  break;
				case RAPTOR:
				  result = "Raptor";
				  break;
				case SARNAK_GOLEM:
				  result = "Sarnak Golem";
				  break;
				case WATER_DRAGON:
				  result = "Water Dragon";
				  break;
				case IKSAR_HAND:
				  result = "Iksar Hand";
				  break;
				case SUCCULENT:
				  result = "Succulent";
				  break;
				case FLYING_MONKEY:
				  result = "Flying Monkey";
				  break;
				case BRONTOTHERIUM:
				  result = "Brontotherium";
				  break;
				case SNOW_DERVISH:
				  result = "Snow Dervish";
				  break;
				case DIRE_WOLF:
				  result = "Dire Wolf";
				  break;
				case MANTICORE:
				  result = "Manticore";
				  break;
				case TOTEM:
				  result = "Totem";
				  break;
				case COLD_SPECTRE:
				  result = "Cold Spectre";
				  break;
				case ENCHANTED_ARMOR:
				  result = "Enchanted Armor";
				  break;
				case SNOW_BUNNY:
				  result = "Snow Bunny";
				  break;
				case WALRUS:
				  result = "Walrus";
				  break;
				case ROCK_GEM_MEN:
				  result = "Rock-gem Men";
				  break;
				case YAK_MAN:
				  result = "Yak Man";
				  break;
				case FAUN:
				  result = "Faun";
				  break;
				case COLDAIN:
				  result = "Coldain";
				  break;
				case VELIOUS_DRAGONS:
				  result = "Velious Dragons";
				  break;
				case UNKNOWN_185:
				  result = "UNKNOWN_185";
				  break;
				case HIPPOGRIFF:
				  result = "Hippogriff";
				  break;
				case SIREN:
				  result = "Siren";
				  break;
				case FROST_GIANT:
				  result = "Frost Giant";
				  break;
				case STORM_GIANT:
				  result = "Storm Giant";
				  break;
				case OTTERMEN:
				  result = "Ottermen";
				  break;
				case WALRUS_MAN:
				  result = "Walrus Man";
				  break;
				case CLOCKWORK_DRAGON:
				  result = "Clockwork Dragon";
				  break;
				case ABHORENT:
				  result = "Abhorent";
				  break;
				case SEA_TURTLE:
				  result = "Sea Turtle";
				  break;
				case BLACK_AND_WHITE_DRAGONS:
				  result = "Black and White Dragons";
				  break;
				case GHOST_DRAGON:
				  result = "Ghost Dragon";
				  break;
				case RONNIE_TEST:
				  result = "Ronnie Test";
				  break;
				case PRISMATIC_DRAGON:
				  result = "Prismatic Dragon";
				  break;
				case SHIKNAR:
				  result = "ShikNar";
				  break;
				case ROCKHOPPER:
				  result = "Rockhopper";
				  break;
				case UNDERBULK:
				  result = "Underbulk";
				  break;
				case GRIMLING:
				  result = "Grimling";
				  break;
				case VAHSHIR:
				  result = "Vah Shir";
				  break;
				case VACUUM_WORM:
				  result = "Vacuum Worm";
				  break;
				case EVAN_TEST:
				  result = "Evan Test";
				  break;
				case KAHLI_SHAH:
				  result = "Kahli Shah";
				  break;
				case OWLBEAR:
				  result = "Owlbear";
				  break;
				case RHINO_BEETLE:
				  result = "Rhino Beetle";
				  break;
				case VAMPYRE:
				  result = "Vampyre";
				  break;
				case EARTH_ELEMENTAL:
				  result = "Earth Elemental";
				  break;
				case AIR_ELEMENTAL:
				  result = "Air Elemental";
				  break;
				case WATER_ELEMENTAL:
				  result = "Water Elemental";
				  break;
				case FIRE_ELEMENTAL:
				  result = "Fire Elemental";
				  break;
				case WETFANG_MINNOW:
				  result = "Wetfang Minnow";
				  break;
				default:
				  result = "UNKNOWN RACE";
				  break;
						}

			return result;
		}
	}
}