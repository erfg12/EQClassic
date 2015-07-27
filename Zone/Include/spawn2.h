#ifndef SPAWN2_H
#define SPAWN2_H

#include "timer.h"
#include "spawngroup.h"

class Spawn2
{
public:
	Spawn2(int32 spawn2_id, int32 spawngroup_id, float x, float y, float z, float heading, int32 respawn, int32 variance, int32 timeleft = 0, int roamRange = 0, int16 pathgrid = 0);
	~Spawn2();

	void	Enable();
	bool	Process();
	void	Reset();
	void	Depop();
	void	Repop(int32 delay = 0);

	int32	GetID()		{ return spawn2_id; }
	float	GetX()		{ return x; }
	float	GetY()		{ return y; }
	float	GetZ()		{ return z; }
	int32   GetSpawnGroupId()       { return spawngroup_id_; } // maalanar: added for updating and deleting spawn info in spawn2
	bool	isTimerRunning(){return timer->Enabled(); } // Tazadar : Added for boats :)
	SPAWN_TIME_OF_DAY GetTimeOfDayFlag() { return time_of_day_flag; } // Kibanu - 8/10/2009
protected:
	friend class Zone;
	int32	spawn2_id;
	Timer*	timer;
private:
	int32	resetTimer();
	int32	spawngroup_id_;
	float	x;
	float	y;
	float	z;
	float	heading;
	int32	respawn_;
	int32	variance_;
	int		roamRange;
	int16	myRoamBox;
	int16	myPathGrid;
	int16	myPathGridStart;
	SPAWN_TIME_OF_DAY time_of_day_flag;
};

#endif
