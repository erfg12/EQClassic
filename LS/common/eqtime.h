#ifndef EQTIME_H
#define EQTIME_H

#include "../common/eq_packet_structs.h"
#include "../common/database.h"

//Struct
struct eqTimeOfDay
{
	TimeOfDay_Struct start_eqtime;
	time_t	start_realtime;
};

//Class Def
class EQTime
{
public:
	//Constructor/destructor
	EQTime(TimeOfDay_Struct start_eq, time_t start_real);
	EQTime();
	~EQTime();

	//Get functions
	int getEQTimeOfDay( time_t timeConvert, TimeOfDay_Struct *eqTimeOfDay );
	TimeOfDay_Struct getStartEQTime() { return eqTime.start_eqtime; }
	time_t getStartRealTime() { return eqTime.start_realtime; }
	int32 getEQTimeZone() { return timezone; }
	int32 getEQTimeZoneHr() { return timezone/60; }
	int32 getEQTimeZoneMin() { return timezone%60; }

	//Set functions
	int setEQTimeOfDay(TimeOfDay_Struct start_eq, time_t start_real);
	void setEQTimeZone(sint32 in_timezone) { timezone=in_timezone; }

	//Database functions
	//bool loadDB(Database q);
	//bool setDB(Database q);
	bool loadFile(const char filename[255]);
	bool saveFile(const char filename[255]);

private:
	//This is our reference clock.
	eqTimeOfDay eqTime;
	//This is our tz offset
	sint32 timezone;
};

#endif
