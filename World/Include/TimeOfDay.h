#ifndef TIMEOFDAY_H
#define TIMEOFDAY_H

#include <vector>
#include <map>
#include <iostream>
#include "mutex.h"
#include "servertalk.h"

namespace EQC
{
	namespace World
	{
		class TimeOfDay
		{
			public:
				TimeOfDay();
				~TimeOfDay();
				bool Process();
				int8 GetMinute();
				int8 GetHour();
				int8 GetDay();
				int8 GetMonth();
				int16 GetYear();
				void UpdateAllClients();
				void SendTimeOfDayToServer(char* zonename);
				void SendTimeOfDayToServer(ZoneServer* zs);
				static TimeOfDay* Instance();
				
			private:
				static TimeOfDay* pinstance;
				Timer* tod;
				int8 tod_hour;
				int8 tod_day;
				int8 tod_month;
				int16 tod_year;
				void DayTime();
				void NightTime();
				void AdvanceDay();
				void AdvanceMonth();
				void AdvanceYear();
		};
	}
}

#endif