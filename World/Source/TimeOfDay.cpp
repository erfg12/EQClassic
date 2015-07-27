#include "zoneserver.h"
#include "ZSList.h"
#include "TimeOfDay.h"
#include "timer.h"
#include "eq_packet_structs.h"
#include "ClientListEntry.h"
#include "database.h"
#include <time.h>

extern EQC::World::Network::ZSList zoneserver_list;
int arrDaysInMonth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// Define Day / Night hours
#define DAYTIME_START 7
#define NIGHTTIME_START 21
#define NUM_SECONDS_PER_HOUR 180

namespace EQC
{
	namespace World
	{
		TimeOfDay* TimeOfDay::pinstance = 0;// initialize pointer

		TimeOfDay* TimeOfDay::Instance() 
		{
			// Check if pinstance has not been created previouly
			if (pinstance == 0)
			{  
				// pinstance is null, create a new instance
				pinstance = new TimeOfDay();
			}

			// return address of instance
			return pinstance;
		}

		TimeOfDay::TimeOfDay() 
		{	
			// Start timer
			tod = new Timer((1000 * NUM_SECONDS_PER_HOUR),false); // 3 Minute timer
			tod->Start();
			tod_hour = 0;
			tod_day = 0;
			tod_month = 0;
			tod_year = 0;
			
			int8 min = 60-(60*((tod->GetRemainingTime() / 1000)/NUM_SECONDS_PER_HOUR));
			
			// Retrieve current time from the database
			if ( Database::Instance()->LoadTimeOfDay(&tod_hour, &tod_day, &tod_month, &tod_year) )
			{	
				cout << "Time of Day started.  Current time is " << (int16)tod_hour << ":" << ( min < 10 ? "0" : "" ) << (int16)min << " on " << (int16)tod_month << "/" << (int16)tod_day << "/" << tod_year << "." << endl;
			}
			else
			{
				tod->Disable();
				cout << "Error setting Time of Day!" << endl;
			}

			if ( tod_hour >= DAYTIME_START && tod_hour < NIGHTTIME_START )
				Database::Instance()->SetDaytime(true);
			else
				Database::Instance()->SetDaytime(false);
		}

		TimeOfDay::~TimeOfDay() 
		{
			tod->Disable();
			safe_delete(tod);
		}

		void TimeOfDay::UpdateAllClients()
		{
			LinkedList<ClientListEntry*>* clientlist = zoneserver_list.GetClientList();
			LinkedListIterator<ClientListEntry*> iterator(*clientlist);

			iterator.Reset();
			while(iterator.MoreElements())
			{
				if ( iterator.GetData() != 0 )
				{
					// Send packet directly to client
					TimeOfDay_Struct* tods = new TimeOfDay_Struct();
					
					tods->hour = TimeOfDay::Instance()->GetHour();
					tods->minute = TimeOfDay::Instance()->GetMinute();					
					tods->day = TimeOfDay::Instance()->GetDay();
					tods->month = TimeOfDay::Instance()->GetMonth();
					tods->year = TimeOfDay::Instance()->GetYear();
									
					ServerPacket* ssp = new ServerPacket(ServerOP_SendPacket, sizeof(TimeOfDay_Struct) + sizeof(ServerSendPacket_Struct));
					memset( ssp->pBuffer, 0, ssp->size );
					ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*)ssp->pBuffer;
					strcpy(sss->charname, iterator.GetData()->name());
					sss->opcode = OP_TimeOfDay;
					memset(sss->packet, 0, sizeof(TimeOfDay_Struct));
					memcpy((TimeOfDay_Struct*)sss->packet, tods, sizeof(TimeOfDay_Struct));
	
					if ( iterator.GetData()->Server() == 0 )
						cout << "Error in Time of Day Update: Cannot find server for " << iterator.GetData()->name() << "." << endl;
					else
						iterator.GetData()->Server()->SendPacket(ssp);
					safe_delete(ssp);//delete packet;
				}
				iterator.Advance();
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
		}

		int8 TimeOfDay::GetMinute()
		{
			return (60-(60*((tod->GetRemainingTime() / 1000)/NUM_SECONDS_PER_HOUR)));
		}

		int8 TimeOfDay::GetHour()
		{
			return ( tod_hour == 0 ? 24 : tod_hour );
		}

		int8 TimeOfDay::GetDay()
		{
			return tod_day;
		}

		int8 TimeOfDay::GetMonth()
		{
			return tod_month;
		}

		int16 TimeOfDay::GetYear()
		{
			return tod_year;
		}

		bool TimeOfDay::Process()
		{
			if (tod->Check(true))
			{
				// 9:00 PM - 6:59 AM = Night
				// 7:00 AM - 8:59 PM = Day
				tod_hour++;
				if ( tod_hour >= 24 )
					AdvanceDay();
				else if ( tod_hour == NIGHTTIME_START )
					NightTime();
				else if ( tod_hour == DAYTIME_START )
					DayTime();
				Database::Instance()->UpdateTimeOfDay( tod_hour, tod_day, tod_month, tod_year );

				cout << "Time of Day: Hour Increase! Now " << (int16)tod_hour << ":00." << endl;
				// Update clients
				UpdateAllClients();
			}
			return true;
		}

		void TimeOfDay::AdvanceDay()
		{
			tod_hour = 0;
			tod_day++;
			if ( tod_day > arrDaysInMonth[tod_month] )
				AdvanceMonth();
		}

		void TimeOfDay::AdvanceMonth()
		{
			tod_day = 1;
			tod_month++;
			if ( tod_month > 12 )
				AdvanceYear();
		}
		
		void TimeOfDay::AdvanceYear()
		{
			tod_year++;
			tod_month = 1;
		}

		void TimeOfDay::NightTime()
		{
			// Update database first
			Database::Instance()->SetDaytime(false);

			// Send out packet to servers
			ServerPacket* pack = new ServerPacket( ServerOP_NightTime, 1);
			memset(pack->pBuffer, 0, pack->size);
			zoneserver_list.SendPacket(pack);
			safe_delete(pack);//delete pack;

		}

		void TimeOfDay::DayTime()
		{
			// Update database first
			Database::Instance()->SetDaytime(true);

			// Send out packet to servers
			ServerPacket* pack = new ServerPacket( ServerOP_DayTime, 1);
			memset(pack->pBuffer, 0, pack->size);
			zoneserver_list.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}

		void TimeOfDay::SendTimeOfDayToServer(char* zonename)
		{
			ZoneServer* zs = zoneserver_list.FindByName(zonename);
			if ( zs != 0 )
			{
				ServerPacket* pack = new ServerPacket( ( tod_hour >= DAYTIME_START && tod_hour < NIGHTTIME_START ? ServerOP_DayTime : ServerOP_NightTime ), 1);
				memset(pack->pBuffer, 0, pack->size);
				zs->SendPacket(pack);
				safe_delete(pack);//delete pack;				
			}
			else
			{
				cout << "SendTimeOfDayToServer: Unable to locate server for zone: " << zonename << endl;
			}
		}

		void TimeOfDay::SendTimeOfDayToServer(ZoneServer* zs)
		{
			if ( zs != 0 )
			{
				ServerPacket* pack = new ServerPacket( ( tod_hour >= DAYTIME_START && tod_hour < NIGHTTIME_START ? ServerOP_DayTime : ServerOP_NightTime ), 1);
				memset(pack->pBuffer, 0, pack->size);
				zs->SendPacket(pack);
				safe_delete(pack);//delete pack;				
			}
			else
			{
				cout << "SendTimeOfDayToServer: Bad zone server pointer." << endl;
			}
		}
	}
}
