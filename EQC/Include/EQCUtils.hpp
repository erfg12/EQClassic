// ***************************************************************
//  EQCUtils   ·  date: 26/09/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifndef EQCUTILS_HPP
#define EQCUTILS_HPP
#include <vector>
#include <string>
#include <iostream>
#include <time.h>
#include "Logger.h"
#include "types.h"
using namespace std;

#define STRING_ERROR "EQCSTRING_ERROR"

enum CodePlace : int8{
	CP_DATABASE = 1,
	CP_CLIENT,
	CP_SPELL,
	CP_WORLDSERVER,
	CP_ZONESERVER,
	CP_GENERIC,
	CP_ATTACK,
	CP_UPDATES,
	CP_FACTION,
	CP_MERCHANT,
	CP_ITEMS,
	CP_PATHING,
	CP_SHAREDMEMORY,
	CP_QUESTS,
	CP_PERL
};

namespace EQC 
{
	namespace Common
	{
		std::string ReadKey(string FileName, string Key);
		std::string ToString(const char* pText, ...);
		std::string ToString(const double x);
		std::string ToString(const sint16 x);
		void CloseSocket(int socket);
		int GetLastSocketError();

		bool IsAlphaNumericStringOnly(char* str);
		string GetPlace(CodePlace Place);
		void PrintF(CodePlace Place, const char* pText, ...);
		void Log(EQCLog::LogIDs id, CodePlace Place, const char* pText, ...);
#ifdef WIN32
		const char* GetOSName();
		bool IsRunningOnWindowsServer();
#endif
		enum MessageFormat {
			BLACK = 0,
			DARK_RED = 1, DARK_GREEN = 2, DARK_YELLOW = 3, DARK_BLUE = 4, DARK_PINK = 5,
			EMERALD = 6, GREY = 7, LIGHTEN_GREEN = 8, LIGHTEN_BLUE = 9,
			WHITE = 10, GREY2 = 11, GREY3 = 12, 
			RED = 13, GREEN = 14, YELLOW = 15, BLUE = 16, BLUE2 = 18,
			SAY = 256, TELL = 257, GROUP = 258, GUILD = 259, OOC = 260, AUCTION = 261,
			SHOUT = 262, EMOTE = 263, CAST_SPELL = 264, YOUHITOTHER = 265, OTHERHITSYOU = 266,
			YOUMISSOTHER = 267, OTHERMISSESYOU = 268, BROADCASTS = 269, SKILLS = 270
		};
	}
}
#endif