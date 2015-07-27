// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef CLIENTNETWORK_H
#define CLIENTNETWORK_H

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "zoneserver.h"
#include "APPlayer.h"
#include "EQPacketManager.h"
#include "database.h"

using namespace std;
using namespace EQC::Common;

namespace EQC
{
	namespace World
	{
		namespace Network
		{
			class ClientNetwork
			{
			public:
				void QueuePacket(APPLAYER* packetforqueue, bool ack_req = true);
				MySendPacketStruct*	PMSendQueuePop();
				APPLAYER* PMOutQueuePop();
				EQPacketManager	packet_manager;	
				Mutex MPacketManager;

				void SendEnterWorld();
				void SendLoginApproved();
				void SendCreateCharFailed();
				void SendZoneServerInfo(ZoneServer* zs, char* zone_name);
				void SendZoneUnavail(char* zone_name);
				void SendNameApproval(bool Valid);
				void SendServerMOTD(ServerMOTD_Struct* smotd);
				void SendExpansionInfo(int flag);
				void SendTimeOfDay();
				void SendCharInfo(CharacterSelect_Struct* cs_struct);
				void SendGuildsList(GuildsList_Struct* gs_struct);
				void SendWearChangeRequestSlot(WearChange_Struct* wc,int16 itemid);

			private:
			};

		}
	}
}

#endif