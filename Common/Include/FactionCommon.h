#ifndef FACTIONCOMMON_H
#define FACTIONCOMMON_H


enum FactionLevels : int8
{
	FACTION_ALLY2			= 1,	// Comment: Ally
	FACTION_WARMLY2			= 2,	// Comment: Warmly
	FACTION_KINDLY2			= 3,	// Comment: Kindly
	FACTION_AMIABLE2		= 4,	// Comment: Ambiable
	FACTION_INDIFFERENT2	= 5,	// Comment: Indifferent
	FACTION_APPREHENSIVE2	= 9,	// Comment: Apprehensive
	FACTION_DUBIOUS2		= 8,	// Comment: Dubios
	FACTION_THREATENLY2		= 7,	// Comment: Threatenly
	FACTION_SCOWLS2			= 6		// Comment: Scrowls -> RUN!

};

// The Minimum value of Faction
#define MAX_FACTION	 1500

// The Maximum vaue for Faction
#define MIN_FACTION -1500

#endif