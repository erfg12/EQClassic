// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// Containes class ID's as used by both the server and client
// ***************************************************************

#ifndef CLASSES_CH
#define CLASSES_CH

#define WARRIOR			1
#define CLERIC			2
#define PALADIN			3
#define RANGER			4
#define SHADOWKNIGHT	5
#define DRUID			6
#define MONK			7
#define BARD			8
#define ROGUE			9
#define SHAMAN			10
#define NECROMANCER		11
#define WIZARD			12
#define MAGICIAN		13
#define ENCHANTER		14
#define BEASTLORD		15
#define BANKER			16
#define WARRIORGM		17
#define CLERICGM		18
#define PALADINGM		19
#define RANGERGM		20
#define SHADOWKNIGHTGM	21
#define DRUIDGM			22
#define MONKGM			23
#define BARDGM			24
#define ROGUEGM			25
#define SHAMANGM		26
#define NECROMANCERGM	27
#define WIZARDGM		28
#define MAGICIANGM		29
#define ENCHANTERGM		30
#define BEASTLORDGM		31
#define MERCHANT		32

#define PLAYER_CLASS_COUNT	15 // used for array defines, must be the count of playable classes

namespace EQC
{
	namespace Common
	{
		// Method header for GetClassName
		char* GetClassName(int8 class_);
	}
}

#endif

