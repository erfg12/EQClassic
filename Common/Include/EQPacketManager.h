// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef EQPACKETMANAGER_H
#define EQPACKETMANAGER_H

#include <iostream>

#include "config.h"
#include "types.h"
#include "queue.h"
#include "timer.h"

#include "eq_opcodes.h"
#include "EQPacket.h"
#include "FragmentGroupList.h"
#include "FragmentGroup.h"
#include "Fragment.h"
#include "packet_dump.h"
#include "APPlayer.h"
#include "ACK_INFO.h"

// Added struct
typedef struct
{
	uchar*  buffer;
	int16   size;
}MySendPacketStruct;



/************ DEFINES ************/

#define PM_ACTIVE    0 // Comment: manager is up and running
#define PM_FINISHING 1 // Comment: manager received closing bits and is going to send final packet
#define PM_FINISHED  2 // Comment: manager has sent closing bits back to client


/************ STRUCTURES/CLASSES ************/





namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			class EQPacketManager  
			{
			public:
				EQPacketManager();
				virtual ~EQPacketManager();

				bool IsTooMuchPending()
				{
					return (packetspending > EQPACKETMANAGER_OUTBOUD_THRESHOLD) ? true : false;
				}

				int16 PacketsPending()
				{
					return packetspending;
				}

				void SetDebugLevel(int8 set_level)
				{
					debug_level = set_level;
				}

				void LogPackets(bool logging) 
				{
					LOG_PACKETS = logging; 
				}
				
				// parce/make packets
				void ParceEQPacket(int16 dwSize, uchar* pPacket);
				void MakeEQPacket(APPLAYER* app, bool ack_req=true); //Make a fragment eq packet and put them on the SQUEUE/RSQUEUE

				// Add ack to packet if requested
				void AddAck(EQC::Common::Network::EQPacket *pack)
				{
					if(CACK.dwARQ)
					{       
						pack->HDR.b2_ARSP = 1;          //Set ack response field
						pack->dwARSP = CACK.dwARQ;      //ACK current ack number.
						CACK.dwARQ = 0;
					}
				}
				// Timer Functions
				
				//Check all class timers and call proper functions
				void CheckTimers(void); 

				int CheckActive(void) 
				{ 
					if(pm_state == PM_FINISHED)
					{
						return(0);
					}
					else
					{
						return(1);
					}
				}

				void Close()
				{ 
					if (pm_state == PM_ACTIVE) 
					{
						pm_state = PM_FINISHING;
					}
				}

				// Incomming / Outgoing Ack's
				void IncomingARSP(int16 dwARSP); 
				void IncomingARQ(int16 dwARQ);
				void OutgoingARQ(int16 dwARQ);
				void OutgoingARSP();

				MyQueue<MySendPacketStruct> SendQueue;	//Store packets thats on the send que
				MyQueue<APPLAYER>           OutQueue;	//parced packets ready to go out of this class


			private:
				bool ProcessPacket(EQC::Common::Network::EQPacket* pack, bool from_buffer=false);
				void CheckBufferedPackets();

				FragmentGroupList fragment_group_list;
				MyQueue<EQC::Common::Network::EQPacket> ResendQueue; //Resend queue

				LinkedList<EQC::Common::Network::EQPacket*> buffered_packets; // Buffer of incoming packets

				ACK_INFO    SACK; //Server -> client info.
				ACK_INFO    CACK; //Client -> server info.
				int16       dwLastCACK;

				Timer* no_ack_received_timer;
				Timer* no_ack_sent_timer;
				Timer* keep_alive_timer;

				int    pm_state;  //manager state 
				int16  dwFragSeq;   //current fragseq
				int8 debug_level;
				bool LOG_PACKETS;
				int16	packetspending;
			};

		}
	}
}
#endif
