// ***************************************************************
//  EQCException   ·  date: 22/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "eq_packet_structs.h"
#include "races.h"
#include "languages.h"
#include "client.h"

namespace EQC
{
	namespace World
	{
		// Dark-Prince - 22/12/2007
		// Sets the langauges of the toon to the default values
		// used when creating a char for the first time
		//TODO: Tie this into char creation
		//TODO: confirm these values are the correct default ones of classic
		//TODO: make it save to the database
		void Client::SetRacialLanguages( PlayerProfile_Struct *pp )
		{
			switch(pp->race)
			{
			case BARBARIAN:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					pp->languages[LANGUAGE_BARBARIAN] = 100;
					break;
				}
			case DARK_ELF:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					pp->languages[LANGUAGE_DARK_ELVISH] = 100;
					pp->languages[LANGUAGE_DARK_SPEECH] = 100;
					pp->languages[LANGUAGE_ELDER_ELVISH] = 100;
					pp->languages[LANGUAGE_ELVISH] = 25;
					break;
				}
			case DWARF:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					pp->languages[LANGUAGE_DWARVISH] = 100;
					pp->languages[LANGUAGE_GNOMISH] = 25;
					break;
				}
			case ERUDITE:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					pp->languages[LANGUAGE_ERUDIAN] = 100;
					break;
				}
			case GNOME:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					pp->languages[LANGUAGE_DWARVISH] = 25;
					pp->languages[LANGUAGE_GNOMISH] = 100;
					break;
				}
			case HALF_ELF:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					pp->languages[LANGUAGE_ELVISH] = 100;
					break;
				}
			case HALFLING:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					pp->languages[LANGUAGE_HALFLING] = 100;
					break;
				}
			case HIGH_ELF:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					pp->languages[LANGUAGE_DARK_ELVISH] = 25;
					pp->languages[LANGUAGE_ELDER_ELVISH] = 25;
					pp->languages[LANGUAGE_ELVISH] = 100;
					break;
				}
			case HUMAN:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					break;
				}
			case IKSAR:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 95;
					pp->languages[LANGUAGE_DARK_SPEECH] = 100;
					pp->languages[LANGUAGE_LIZARDMAN] = 100;
					break;
				}
			case OGRE:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 95;
					pp->languages[LANGUAGE_DARK_SPEECH] = 100;
					pp->languages[LANGUAGE_OGRE] = 100;
					break;
				}
			case TROLL:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 95;
					pp->languages[LANGUAGE_DARK_SPEECH] = 100;
					pp->languages[LANGUAGE_TROLL] = 100;
					break;
				}
			case WOOD_ELF:
				{
					pp->languages[LANGUAGE_COMMON_TONGUE] = 100;
					pp->languages[LANGUAGE_ELVISH] = 100;
					break;
				}
			}
		}
	}
}