#ifndef EVENT_CODES_H
#define EVENT_CODES_H

typedef enum {
	EVENT_SAY = 0,
	EVENT_ITEM,			//being given an item
	EVENT_DEATH,		//being killed
	EVENT_SPAWN,		//triggered when we first spawn
	EVENT_ATTACK,		//being attacked (resets after an interval of not being attacked)
	EVENT_COMBAT,		//being attacked or attacking (resets after an interval of not being attacked)
	EVENT_AGGRO,		//entering combat mode due to a PC attack
	EVENT_SLAY,			//killing a PC
	EVENT_NPC_SLAY,		//killing an NPC
	EVENT_WAYPOINT,		//reaching a waypoint on a grid
	EVENT_TIMER,
	EVENT_SIGNAL,
	EVENT_HP,
	EVENT_ENTER,		//PC entering your set proximity
	EVENT_EXIT,			//PC leaving your set proximity
	EVENT_ENTERZONE,		//PC only, you enter zone
	EVENT_CLICKDOOR,		//pc only, you click a door
	EVENT_LOOT,			//pc only
	EVENT_ZONE,			//pc only
	EVENT_LEVEL_UP,		//pc only
	EVENT_KILLED_MERIT, //killed by a PC or group, gave experience; will repeat several times for groups
	EVENT_CAST_ON,		//pc casted a spell on npc
	EVENT_TASKACCEPTED,	//pc accepted a task
	EVENT_TASK_STAGE_COMPLETE,
	EVENT_AGGRO_SAY,
	EVENT_PLAYER_PICKUP,
	EVENT_POPUPRESPONSE,
	EVENT_PROXIMITY_SAY,
	EVENT_CAST,
	EVENT_SCALE_CALC,
	
	_LargestEventID
} QuestEventID;

extern const char *QuestEventSubroutines[_LargestEventID];

#endif

