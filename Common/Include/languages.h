//o--------------------------------------------------------------
//| languages.h; Harakiri, August 10, 2009
//o--------------------------------------------------------------
//| taken from the eqgame.exe sub_4C8FCE .text:004C8FC
//o--------------------------------------------------------------

#ifndef LANGUAGES_H
#define LANGUAGES_H

/* Harakiri - All language Ids have been confimed by me, see offset in eqgame.exe */

#define LANGUAGE_COMMON_TONGUE   0		
#define LANGUAGE_BARBARIAN       1		
#define LANGUAGE_ERUDIAN         2		
#define LANGUAGE_ELVISH          3		
#define LANGUAGE_DARK_ELVISH     4		
#define LANGUAGE_DWARVISH        5		
#define LANGUAGE_TROLL           6		
#define LANGUAGE_OGRE            7		
#define LANGUAGE_GNOMISH         8		
#define LANGUAGE_HALFLING        9		
#define LANGUAGE_THIEVES_CANT   10		
#define LANGUAGE_OLD_ERUDIAN    11		
#define LANGUAGE_ELDER_ELVISH   12		
#define LANGUAGE_FROGLOK        13		
#define LANGUAGE_GOBLIN         14		
#define LANGUAGE_GNOLL          15		
#define LANGUAGE_COMBINE_TONGUE 16		
#define LANGUAGE_ELDER_TEIRDAL  17		
#define LANGUAGE_LIZARDMAN      18		
#define LANGUAGE_ORCISH         19		
#define LANGUAGE_FAERIE         20		
#define LANGUAGE_DRAGON         21		
#define LANGUAGE_ELDER_DRAGON   22		
#define LANGUAGE_DARK_SPEECH    23		

namespace EQC
{
	namespace Common
	{
		char* GetLanguageName(int8 language);
	}
}

#endif

