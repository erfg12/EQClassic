//o--------------------------------------------------------------
//| languages.cpp; Harakiri, August 10, 2009
//o--------------------------------------------------------------
//| Translate language ID to string, taken from the eqgame.exe sub_4C8FCE .text:004C8FC
//o--------------------------------------------------------------

#include "eq_packet_structs.h"
#include "languages.h"

namespace EQC
{
	namespace Common
	{
		// Converts the int8 race to a text value
		char* GetLanguageName(int8 language) 
		{
			char* tmplang = "";
			
			switch(language) 
			{
				case LANGUAGE_COMMON_TONGUE:
					tmplang = "Common Tongue";
					break;
				case LANGUAGE_BARBARIAN:
					tmplang = "Barbarian";
					break;
				case LANGUAGE_ERUDIAN:
					tmplang = "Erudian";
					break;
				case LANGUAGE_ELVISH:
					tmplang = "Elvish";
					break;
				case LANGUAGE_DARK_ELVISH:
					tmplang = "Dark Elvish";
					break;
				case LANGUAGE_DWARVISH:
					tmplang = "Dwarvish";
					break;
				case LANGUAGE_TROLL:
					tmplang = "Troll";
					break;
				case LANGUAGE_OGRE:
					tmplang = "Ogre";
					break;
				case LANGUAGE_GNOMISH:
					tmplang = "Gnomish";
					break;
				case LANGUAGE_HALFLING:
					tmplang = "Halfling";
					break;
				case LANGUAGE_THIEVES_CANT:
					tmplang = "Thieves Cant";
					break;
				case LANGUAGE_OLD_ERUDIAN:
					tmplang = "Old Erudian";
					break;
				case LANGUAGE_ELDER_ELVISH:
					tmplang = "Eldar Elvish";
					break;
				case LANGUAGE_FROGLOK:
					tmplang = "Froglok";
					break;
				case LANGUAGE_GOBLIN:
					tmplang = "Goblin";
					break;
				case LANGUAGE_GNOLL:
					tmplang = "Gnoll";
					break;
				case LANGUAGE_COMBINE_TONGUE:
					tmplang = "Combine Tongue";
					break;
				case LANGUAGE_ELDER_TEIRDAL:
					tmplang = "Elder Teir'Dal";
					break;
				case LANGUAGE_LIZARDMAN:
					tmplang = "Lizardman";
					break;
				case LANGUAGE_ORCISH:
					tmplang = "Orcish";
					break;
				case LANGUAGE_FAERIE:
					tmplang = "Faerie";
					break;
				case LANGUAGE_DRAGON:
					tmplang = "Dragon";
					break;
				case LANGUAGE_ELDER_DRAGON:
					tmplang = "Elder Dragon";
					break;
				case LANGUAGE_DARK_SPEECH:
					tmplang = "Dark Speech";
					break;
				default:
					tmplang = "an unknown tongue"; 
					break;
			}

			return tmplang;
		}
	}
}