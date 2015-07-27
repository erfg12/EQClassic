// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include "Clientnetwork.h"
#include "client.h"
#include "config.h"
#include "eq_packet_structs.h"
#include "TimeOfDay.h"

using namespace EQC::Common::Network;

namespace EQC
{
	namespace World
	{
		namespace Network
		{
			MySendPacketStruct* ClientNetwork::PMSendQueuePop()
			{
				LockMutex lock(&MPacketManager);
				return packet_manager.SendQueue.pop();
			}

			APPLAYER* ClientNetwork::PMOutQueuePop()
			{
				LockMutex lock(&MPacketManager);
				return packet_manager.OutQueue.pop();
			}

			// Places a packet on the queue to be sent out to the client
			void ClientNetwork::QueuePacket(APPLAYER* packetforqueue, bool ack_req)
			{
				ack_req = true;	// It's broke right now, dont delete this line till fix it. =P

				if (packetforqueue != 0)
				{
					if (packetforqueue->size >= ABNORMAL_PACKET_SIZE)
					{
						cout << "WARNING: abnormal packet size. o=0x" << hex << packetforqueue->opcode << dec << ", s=" << packetforqueue->size << endl;
					}
				}
				LockMutex lock(&MPacketManager);
				packet_manager.MakeEQPacket(packetforqueue, ack_req);
			}


			// 20/10/2007 - froglok23 - Split into own method
			void ClientNetwork::SendEnterWorld()
			{
				APPLAYER *outpacket = new APPLAYER(OP_ENTERWORLD, 1);

				outpacket->pBuffer[0] = 0;

				this->QueuePacket(outpacket);
				safe_delete(outpacket);//delete outpacket;
			}

			// 20/10/2007 - froglok23 - Split into own method
			void ClientNetwork::SendLoginApproved()
			{
				APPLAYER *outpacket = new APPLAYER(WS_SEND_LOGIN_APPROVED, 1);

				outpacket->pBuffer[0] = 0;

				this->QueuePacket(outpacket);
				safe_delete(outpacket);//delete outpacket;
			}

			void ClientNetwork::SendCreateCharFailed()
			{
				APPLAYER *outpacket = new APPLAYER(OP_NAMEAPPROVAL, 1);

				outpacket->pBuffer[0] = 0;

				this->QueuePacket(outpacket);
				safe_delete(outpacket);//delete outpacket;
			}

			void ClientNetwork::SendZoneServerInfo(ZoneServer* zs, char* zone_name)
			{
				if(strlen(zone_name) > MAX_ZONE_NAME)
				{
					return;
				}

				APPLAYER* outpacket = new APPLAYER(OP_ZoneServerInfo, 130);


				strcpy((char*) outpacket->pBuffer,zs->GetCAddress());
				strcpy((char*)&outpacket->pBuffer[75], zone_name);

				int16 *temp = (int16*)&outpacket->pBuffer[128];
				*temp = ntohs(zs->GetCPort());

				this->QueuePacket(outpacket);
				safe_delete(outpacket);//delete outpacket;
			}


			void ClientNetwork::SendZoneUnavail(char* zone_name)
			{
				APPLAYER* outpacket = new APPLAYER(OP_ZoneUnavail, sizeof(ZoneUnavail_Struct));
				ZoneUnavail_Struct* ua = (ZoneUnavail_Struct*)outpacket->pBuffer;

				strcpy(ua->zonename, zone_name);

				this->QueuePacket(outpacket);
				safe_delete(outpacket);//delete outpacket;
			}

			
			void ClientNetwork::SendNameApproval(bool Valid)
			{
				APPLAYER *outpacket = new APPLAYER(OP_NAMEAPPROVAL, 1);
				outpacket->pBuffer[0] = Valid ? 1 : 0;

				this->QueuePacket(outpacket);
				safe_delete(outpacket);//delete outpacket;
			}

			/*
			//TODO: Dark-Prince 22/12/2007 - Finsih this
			void ClientNetwork::SendNameApproval(NameApproval_Struct *ns)
			{
				APPLAYER* outpacket = new APPLAYER(OP_NAMEAPPROVAL, sizeof(NameApproval_Struct));
				outpacket->pBuffer = new uchar[1];
				outpacket->size = 1;
				//outpacket->pBuffer = Valid;

				this->QueuePacket(outpacket);
				safe_delete(outpacket);//delete outpacket;
			}
			*/

			void ClientNetwork::SendServerMOTD(ServerMOTD_Struct* smotd)
			{
				APPLAYER* outpacket = new APPLAYER(OP_SERVERMOTD, sizeof(ServerMOTD_Struct));
				
				if(smotd)
				{
					int len = strlen(smotd->motd);


					if (len > 0)
					{
						outpacket->size = strlen(smotd->motd)+1;
						strcpy((char*)outpacket->pBuffer, smotd->motd);
					}
					else
					{
						// Null Message of the Day. :)
						outpacket->size = 1;
						outpacket->pBuffer[0] = 0;
					}
					this->QueuePacket(outpacket);
					//TODO: Causes nasty execption
					//safe_delete(outpacket);//delete outpacket;
				}
			}

			void ClientNetwork::SendExpansionInfo(int flag)
			{
				// Quagmire - Enabling expansions. Pretty sure this is bitwise
				APPLAYER *outpacket = new APPLAYER(OP_ExpansionInfo, 4);

				memset(outpacket->pBuffer, 0, outpacket->size);
				outpacket->pBuffer[0] = flag;

				this->QueuePacket(outpacket);
				safe_delete(outpacket);//delete outpacket;
			}

			void ClientNetwork::SendTimeOfDay()
			{
				APPLAYER *outpacket = new APPLAYER(OP_TimeOfDay, sizeof(TimeOfDay_Struct));
				TimeOfDay_Struct* tods = (TimeOfDay_Struct*)outpacket->pBuffer;
				memset(outpacket->pBuffer, 0, sizeof(TimeOfDay_Struct));
				
				tods->hour = EQC::World::TimeOfDay::Instance()->GetHour();
				tods->minute = TimeOfDay::Instance()->GetMinute();					
				tods->day = TimeOfDay::Instance()->GetDay();
				tods->month = TimeOfDay::Instance()->GetMonth();
				tods->year = TimeOfDay::Instance()->GetYear();

				this->QueuePacket(outpacket);
				safe_delete(outpacket);//delete outpacket;
			}

			void ClientNetwork::SendCharInfo(CharacterSelect_Struct* cs_struct)
			{
				APPLAYER *outpacket = new APPLAYER(OP_SendCharInfo, sizeof(CharacterSelect_Struct));

				if (outpacket->size != sizeof(CharacterSelect_Struct)) // 20/10/2007 - froglok23 - added size checking sanity check
				{
					cout << "Wrong size on CharacterSelect_Struct. Got: " << outpacket->size << ", Expected: " << sizeof(CharacterSelect_Struct) << endl;
				} 
				else
				{
					memset(outpacket->pBuffer, 0, outpacket->size);
					outpacket->pBuffer = (uchar*)cs_struct;

					this->QueuePacket(outpacket);
				}
				safe_delete(outpacket);//delete outpacket;
			}

			void ClientNetwork::SendGuildsList(GuildsList_Struct* gs_struct)
			{
				// Quagmire - tring to send list of guilds
				APPLAYER *outpacket = new APPLAYER(OP_GuildsList, sizeof(GuildsList_Struct));

				if (outpacket->size != sizeof(GuildsList_Struct)) // 20/10/2007 - froglok23 - added size checking sanity check
				{
					cout << "Wrong size on GuildsList_Struct. Got: " << outpacket->size << ", Expected: " << sizeof(GuildsList_Struct) << endl;
				}
				else
				{
					memset(outpacket->pBuffer, 0, outpacket->size);
					outpacket->pBuffer = (uchar*)gs_struct;

					this->QueuePacket(outpacket);
				}
				safe_delete(outpacket);//delete outpacket;
			}

			void ClientNetwork::SendWearChangeRequestSlot(WearChange_Struct* wc,int16 itemid)
			{
				APPLAYER* outpacket = new APPLAYER(OP_WearChange, sizeof(WearChange_Struct));

				memset(outpacket->pBuffer, 0, outpacket->size);

				WearChange_Struct* wc_out = (WearChange_Struct*) outpacket->pBuffer;

				wc_out->wear_slot_id = wc->wear_slot_id;

				if(wc->wear_slot_id == 7)
				{
					Item_Struct* item=Database::Instance()->GetItem(itemid);
					int number=0;
					int i=1;
					if(item){
						for(int8 j=0;j<3;j++){
							if(item->idfile[4-j]!=0){//Tazadar : We turn char into int ^^
								number+=((int)item->idfile[4-j]-48)*i;
								i=i*10;
							}
						}
						//cout<<"item->idfile "<< item->idfile << endl;
						//cout<<"number "<< (int)number << endl;
					}
					wc_out->slot_graphic = number; 
				}
				if(wc->wear_slot_id == 8)
				{
					int number=0;
					int i=1;
					Item_Struct* item=Database::Instance()->GetItem(itemid);
					if(item){
						for(int8 j=0;j<3;j++){
							if(item->idfile[4-j]!=0){//Tazadar : We turn char into int ^^
								number+=((int)item->idfile[4-j]-48)*i;
								i=i*10;
							}
						}
						//cout<<"item->idfile "<< item->idfile << endl;
						//cout<<"number "<< (int)number << endl;
					}
					wc_out->slot_graphic = number;
				}

				if(wc->slot_graphic != 0){
					//strcpy(wc_out->slot_graphic,wc->slot_graphic);
				}
				
				this->QueuePacket(outpacket);

				safe_delete(outpacket);//delete outpacket;
			}
		}
	}
}