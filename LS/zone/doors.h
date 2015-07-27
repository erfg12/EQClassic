#include "../common/types.h"
#include "../common/linked_list.h"
#include "../common/timer.h"
#include "../common/eq_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "entity.h"
#include "mob.h"
#include "zonedump.h"

class Doors : public Entity
{
public:
	Doors(const Door* door);
	~Doors();
	bool	IsDoor()			{ return true; }
	void	HandleClick(Client* sender);
	bool	Process();
	int8	GetDoorID() { return door_id; }
	int32	GetDoorDBID() { return db_id; }
	int32	GetGuildID() { return guildid; }
	int8	GetOpenType() { return opentype; }
	char*	GetDoorName() { return door_name; }

	float	GetX() { return pos_x; }
	float	GetY() { return pos_y; }
	float	GetZ() { return pos_z; }
	float	GetHeading() { return heading; }

	bool	IsDoorOpen() { return isopen; }

	int8	GetTriggerDoorID() { return trigger_door; }
	int8	GetTriggerType() { return trigger_type; }

	void	SetGuildID(int32 guild_id) { guildid = guild_id; }

	int32	GetEntityID() { return entity_id; }
	void	SetEntityID(int32 entity) { entity_id = entity; }
	
	void DumpDoor();
	float	GetDestX() { return dest_x; }
	float	GetDestY() { return dest_y; }
	float	GetDestZ() { return dest_z; }
	float	GetDestHeading() { return dest_heading; }
	
	
private:
int32	db_id;
int8	door_id;
char	zone_name[16];
char	door_name[16];
float	pos_x;
float	pos_y;
float	pos_z;
float	heading;
int8	opentype;
int32	guildid;
int16	lockpick;
int16	keyitem;
int8	trigger_door;
int8	trigger_type;
int16	liftheight;
int32	entity_id;
bool	isopen;
Timer*	close_timer;

char    dest_zone[16];
float   dest_x;
float   dest_y;
float   dest_z;
float   dest_heading;


};
