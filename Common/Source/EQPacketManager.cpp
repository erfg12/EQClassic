// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include "EQPacketManager.h"

#include <iostream>
#include <cstdlib>
using namespace std;

template <typename type>                    // LO_BYTE
type  LO_BYTE (type a) {return (a&=0xff);}  
template <typename type>                    // HI_BYTE 
type  HI_BYTE (type a) {return (a&=0xff00);} 
template <typename type>                    // LO_WORD
type  LO_WORD (type a) {return (a&=0xffff);}  
template <typename type>                    // HI_WORD 
type  HI_WORD (type a) {return (a&=0xffff0000);} 
template <typename type>                    // HI_LOSWAPshort
type  HI_LOSWAPshort (type a) {return (LO_BYTE(a)<<8) | (HI_BYTE(a)>>8);}  
template <typename type>                    // HI_LOSWAPlong
type  HI_LOSWAPlong (type a) {return (LO_WORD(a)<<16) | (HIWORD(a)>>16);}  


namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			EQPacketManager::EQPacketManager()
			{
				pm_state = PM_ACTIVE;
				dwLastCACK = 0;
				dwFragSeq  = 0;
			    
				no_ack_received_timer = new Timer(500);
				no_ack_sent_timer = new Timer(500);

				/* 
				   on a normal server there is always data getting sent 
				   so the packetloss indicator in eq stays at 0%

				   This timer will send dummy data if nothing has been sent
				   in 1000ms. This is not needed. When the client doesnt
				   get any data it will send a special ack request to ask
				   if we are still alive. The eq client will show around 40%
				   packetloss at this time. It is not real packetloss. Its
				   just thinking there is packetloss since no data is sent.
				   The EQ servers doesnt have a timer like this one.

				   short version: This timer is not needed, it just keeps
				   the green bar green.
				*/
				keep_alive_timer = new Timer(1000);

				no_ack_received_timer->Disable();
				no_ack_sent_timer->Disable();

				debug_level = 0;
				LOG_PACKETS = false;

				packetspending = 0;
			}

			EQPacketManager::~EQPacketManager()
			{
				safe_delete(no_ack_received_timer);//delete no_ack_received_timer;
				safe_delete(no_ack_sent_timer);//delete no_ack_sent_timer;
				safe_delete(keep_alive_timer);//delete keep_alive_timer;
				MySendPacketStruct* p = 0;
				
				while (p = SendQueue.pop()) 
				{
					safe_delete(p->buffer);//delete p->buffer;
					safe_delete(p);//delete p;
				}
			}

			void EQPacketManager::IncomingARSP(int16 dwARSP) 
			{ 
				if (debug_level >= 4)
				{
					cout << "Incoming ack response: " << dwARSP << " SACK.dwARQ:" << SACK.dwARQ << endl;
				}
				EQC::Common::Network::EQPacket *pack;
				while (!ResendQueue.empty() && dwARSP - ResendQueue.top()->dwARQ >= 0)
				{
					if (debug_level >= 5)
					{
						cout << "Removing " << ResendQueue.top()->dwARQ << " from resendqueue" << endl;
					}
					pack = ResendQueue.pop();
					packetspending--;
					safe_delete(pack);
				}
				if (ResendQueue.empty())
				{
					no_ack_received_timer->Disable();
					if (debug_level >= 5)
					{
			//            cout << Timer::GetCurrentTime() << " no_ack_received_timer->Disable()";
			//            cout << " dwARSP:" << (unsigned short)dwARSP;
			//            cout << " SACK.dwARQ:" << (unsigned short)SACK.dwARQ << endl;
					}
				}
			}

			void EQPacketManager::IncomingARQ(int16 dwARQ) 
			{
				CACK.dwARQ = dwARQ;
				dwLastCACK = dwARQ;
			    
				if (!no_ack_sent_timer->Enabled())
				{
					no_ack_sent_timer->Start(500); // Agz: If we dont get any outgoing packet we can put an 
					// ack response in before 500ms has passed we send a pure ack response        if (debug_level >= 2)
			//            cout << Timer::GetCurrentTime() << " no_ack_sent_timer->Start(500)" << endl;
				}
			}

			void EQPacketManager::OutgoingARQ(int16 dwARQ)   //An ack request is sent
			{
				if(!no_ack_received_timer->Enabled())
				{
					no_ack_received_timer->Start(500);
			//        if (debug_level >= 2)
			//            cout << Timer::GetCurrentTime() << " no_ack_received_timer->Start(500)" << "ARQ:" << (unsigned short) dwARQ << endl;
				}
				if (debug_level >= 4)
					cout << "Outgoing ARQ:" << (unsigned short) dwARQ << endl;
			}

			void EQPacketManager::OutgoingARSP(void)
			{
				no_ack_sent_timer->Disable(); // Agz: We have sent the ack response
			//    if (debug_level >= 2)
			//        cout << Timer::GetCurrentTime() << " no_ack_sent_timer->Disable()" << endl;
			}

			/************ PARCE A EQPACKET ************/
			void EQPacketManager::ParceEQPacket(int16 dwSize, uchar* pPacket)
			{
				if(pm_state != PM_ACTIVE)
					return;
			        
				/************ DECODE PACKET ************/
				EQC::Common::Network::EQPacket* pack = new EQC::Common::Network::EQPacket;
				pack->DecodePacket(dwSize, pPacket);
				if (ProcessPacket(pack, false))
				{
					safe_delete(pack);//delete pack;
				}
				CheckBufferedPackets();
			}

			/*
				Return true if its safe to delete this packet now, if we buffer it return false
				this way i can skip memcpy the packet when buffering it
			*/
			bool EQPacketManager::ProcessPacket(EQC::Common::Network::EQPacket* pack, bool from_buffer)
			{
				/************ CHECK FOR ACK/SEQ RESET ************/ 
				if(pack->HDR.a5_SEQStart)
				{
					//      cout << "resetting SACK.dwGSQ1" << endl;
					//      SACK.dwGSQ      = 0;            //Main sequence number SHORT#2
					dwLastCACK      = pack->dwARQ-1;//0;
					//      CACK.dwGSQ = 0xFFFF; changed next if to else instead
				}
				// Agz: Moved this, was under packet resend before..., later changed to else statement...
				else if( (pack->dwSEQ - CACK.dwGSQ) <= 0 && !from_buffer)  // Agz: if from the buffer i ignore sequence number..
				{
					if (debug_level >= 6)
					{
						cout << Timer::GetCurrentTime() << " ";
						cout << "Invalid packet ";
						cout << "pack->dwSEQ:" << pack->dwSEQ << " ";
						cout << "CACK.dwGSQ;" << CACK.dwGSQ << " ";
						cout << "pack->dwARQ:" << pack->dwARQ << endl;
					}
					return true; //Invalid packet
				}
				else if (debug_level >= 9)
				{
					cout << Timer::GetCurrentTime() << " ";
					cout << "Valid packet ";
					cout << "pack->dwSEQ:" << pack->dwSEQ << " ";
					cout << "CACK.dwGSQ;" << CACK.dwGSQ << " ";
					cout << "pack->dwARQ:" << pack->dwARQ << endl;
				}

				CACK.dwGSQ = pack->dwSEQ; //Get current sequence #.

				/************ Process ack responds ************/
				// Quagmire: Moved this to above "ack request" checking in case the packet is dropped in there
				if(pack->HDR.b2_ARSP)
					IncomingARSP(pack->dwARSP);
				/************ End process ack rsp ************/

				// Does this packet contain an ack request?
				if(pack->HDR.a1_ARQ)
				{
					// Is this packet a packet we dont want now, but will need later?
					if(pack->dwARQ - dwLastCACK > 1 && pack->dwARQ - dwLastCACK < 16) // Agz: Added 16 limit
					{
						// Debug check, if we want to buffer a packet we got from the buffer something is wrong...
						if (from_buffer)
						{
							cerr << "ERROR: Rebuffering a packet in EQPacketManager::ProcessPacket" << endl;
						}
						LinkedListIterator<EQC::Common::Network::EQPacket*> iterator(buffered_packets);
						iterator.Reset();
						while(iterator.MoreElements())
						{
							if (iterator.GetData()->dwARQ == pack->dwARQ)
							{
								if (debug_level >= 6)
								{
									cout << Timer::GetCurrentTime() << " ";
									cout << "Tried to buffer this packet, but it was already buffered ";
									cout << "pack->dwARQ:" << pack->dwARQ << " ";
									cout << "dwLastCACK:" << dwLastCACK << " ";
									cout << "pack->dwSEQ:" << pack->dwSEQ << endl;
								}
								return true; // This packet was already buffered
							}
							iterator.Advance();
							//Yeahlight: Zone freeze debug
							if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
								EQC_FREEZE_DEBUG(__LINE__, __FILE__);
						}

						if (debug_level >= 1) {
							cout << Timer::GetCurrentTime() << " ";
							cout << "Buffering this packet ";
							cout << "pack->dwARQ:" << pack->dwARQ << " ";
							cout << "dwLastCACK:" << dwLastCACK << " ";
							cout << "pack->dwSEQ:" << pack->dwSEQ << endl;
						}

						buffered_packets.Insert(pack);
						return false;
					}
					// Is this packet a resend we have already processed?
					if(pack->dwARQ - dwLastCACK <= 0)
					{
						if (debug_level >= 6)
						{
							cout << Timer::GetCurrentTime() << " ";
							cout << "Duplicate packet received ";
							cout << "pack->dwARQ:" << pack->dwARQ << " ";
							cout << "dwLastCACK:" << dwLastCACK << " ";
							cout << "pack->dwSEQ:" << pack->dwSEQ << endl;
						}
						no_ack_sent_timer->Trigger(); // Added to make sure we send a new ack respond
						return true;
					}
				}

				/************ START ACK REQ CHECK ************/
				if (pack->HDR.a1_ARQ || pack->HDR.b0_SpecARQ)
					IncomingARQ(pack->dwARQ);   
				if (pack->HDR.b0_SpecARQ) // Send the ack reponse right away
					no_ack_sent_timer->Trigger();
				/************ END ACK REQ CHECK ************/

				/************ CHECK FOR THREAD TERMINATION ************/
				if(pack->HDR.a2_Closing && pack->HDR.a6_Closing)
				{
					if(!pm_state)
					{
						if (debug_level >= 1)
						{
							cout << "Closing bits received from client" << endl;
						}
						pm_state = PM_FINISHING;
						MakeEQPacket(0); // Agz: Added a close packet
					}
				}
				/************ END CHECK THREAD TERMINATION ************/

				/************ Get ack sequence number ************/
				if(pack->HDR.a4_ASQ)
					CACK.dbASQ_high = pack->dbASQ_high;

				if(pack->HDR.a1_ARQ)
					CACK.dbASQ_low = pack->dbASQ_low;
				/************ End get ack seq num ************/

				/************ START FRAGMENT CHECK ************/
				/************ IF - FRAGMENT ************/
				if(pack->HDR.a3_Fragment) 
				{
					if (debug_level >= 7)
					{
						cout << endl << Timer::GetCurrentTime() << " Fragment: ";
						cout << "pack->fraginfo.dwSeq:" << pack->fraginfo.dwSeq << " ";
						cout << "pack->fraginfo.dwCurr:" << pack->fraginfo.dwCurr << " ";
						cout << "pack->fraginfo.dwTotal:" << pack->fraginfo.dwTotal << endl;
					}
					FragmentGroup* fragment_group = 0;
					fragment_group = fragment_group_list.Get(pack->fraginfo.dwSeq);

					// If we dont have a fragment group with the right sequence number, create a new one
					if (fragment_group == 0)
					{
						fragment_group = new FragmentGroup(pack->fraginfo.dwSeq,pack->dwOpCode, pack->fraginfo.dwTotal);
						fragment_group_list.Add(fragment_group);
					}

					// Add this fragment to the fragment group
					fragment_group->Add(pack->fraginfo.dwCurr, pack->pExtra,pack->dwExtraSize);

					// If we have all the fragments to complete this group
					if(pack->fraginfo.dwCurr == (pack->fraginfo.dwTotal - 1) )
					{
						if (debug_level >= 8)
							cout << Timer::GetCurrentTime() << " Getting fragment opcode:" << fragment_group->GetOpcode() << endl;
						//Collect fragments and put them as one packet on the OutQueue
						APPLAYER *app = new APPLAYER;
						app->pBuffer = fragment_group->AssembleData(&app->size);
						app->opcode = fragment_group->GetOpcode();
						fragment_group_list.Remove(pack->fraginfo.dwSeq);
						if (debug_level >= 8)
							cout << "FRAGMENT_GROUP finished" << endl;

						if (LOG_PACKETS) {
							cout << "Logging incoming packet. OPCODE: 0x" << hex << app->opcode << endl;
							DumpPacketHex(app);
						}

						OutQueue.push(app);
						return true;
					}
					else
					{
						if (debug_level >= 8)
							cout << "FRAGMENT_GROUP not finished wait for more... " << endl;
						return true;
					}
				}
				/************ ELSE - NO FRAGMENT ************/
				else 
				{
					APPLAYER *app	= new APPLAYER;   
					app->pBuffer    = new uchar[pack->dwExtraSize]; memcpy(app->pBuffer,pack->pExtra, pack->dwExtraSize); // Agz: Added alloc
					app->size       = pack->dwExtraSize;
					app->opcode     = pack->dwOpCode;
					if (debug_level >= 9)
						cout << "Packet processed opcode:" << pack->dwOpCode << " length:" << app->size << endl;

					if (LOG_PACKETS) {
						cout << "Logging incoming packet. OPCODE: 0x" << hex << app->opcode << endl;
						DumpPacketHex(app);
					}

					OutQueue.push(app);
					return true;
				}
				/************ END FRAGMENT CHECK ************/

				cout << endl << "reached end of ParceEQPacket?!" << endl << endl;
				return true;
			}

			void EQPacketManager::CheckBufferedPackets()
			{
			// Should use a hash table or sorted list instead....
				int num=0; // Counting buffered packets for debug output
				LinkedListIterator<EQC::Common::Network::EQPacket*> iterator(buffered_packets);
				iterator.Reset();
				while(iterator.MoreElements())
				{
					num++;
					// Check if we have a packet we want already buffered
					if (iterator.GetData()->dwARQ - dwLastCACK == 1)
					{
						if (debug_level >= 8)
						{
							cout << Timer::GetCurrentTime() << " ";
							cout << "Found a packet we want in the incoming packet buffer";
							cout << "pack->dwARQ:" << iterator.GetData()->dwARQ << " ";
							cout << "dwLastCACK:" << dwLastCACK << " ";
							cout << "pack->dwSEQ:" << iterator.GetData()->dwSEQ << endl;
						}
						ProcessPacket(iterator.GetData(), true);
						iterator.RemoveCurrent(); // This will call delete on the packet
						iterator.Reset();         // Start from the beginning of the list again
						num=0;                    // Reset the counter
						continue;
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}

				if (debug_level >= 9)
					cout << "Number of packets in buffer:" << num << endl;
			}

			/************************************************************************/
			/************ Make an EQ packet and put it to the send queue ************/
			/* 
				APP->size == 0 && app->pBuffer == NULL if no data.

				Agz: set ack_req = false if you dont want this packet to require an ack
				response from the client, this menas this packet may get lost and not
				resent. This is used by the EQ servers for HP and position updates among 
				other things. WARNING: I havent tested this yet.
			*/
			void EQPacketManager::MakeEQPacket(APPLAYER* app, bool ack_req)
			{
				if (LOG_PACKETS) {
					cout << "Logging outgoing packet. OPCODE: 0x" << hex << app->opcode << endl;
					DumpPacketHex(app);
				}

				int16 restore_op;

				/************ PM STATE = NOT ACTIVE ************/
				if(pm_state == PM_FINISHING)
				{
					EQC::Common::Network::EQPacket *pack = new EQC::Common::Network::EQPacket;
					MySendPacketStruct *p = new MySendPacketStruct;

					pack->dwSEQ = SACK.dwGSQ++; // Agz: Added this commented rest        
			  
					pack->HDR.a6_Closing    = 1;// Agz: Lets try to uncomment this line again
					pack->HDR.a2_Closing    = 1;// and this
					pack->HDR.a1_ARQ        = 1;// and this
			//      pack->dwARQ             = 1;// and this, no that was not too good
					pack->dwARQ             = SACK.dwARQ;// try this instead

					//AddAck(pack);
					pm_state = PM_FINISHED;
			        
					p->buffer = pack->ReturnPacket(&p->size);
					SendQueue.push(p);
					SACK.dwGSQ++; 
					safe_delete(pack);//delete pack;

					return;
				}

				// Agz:Moved this to after finish check
				if(app == NULL)
				{
					cout << "EQPacketManager::MakeEQPacket app == NULL" << endl;
					return;
				}
				bool bFragment= false; //This is set later on if fragseq should be increased at the end.

				/************ IF opcode is == 0xFFFF it is a request for pure ack creation ************/
				if(app->opcode == 0xFFFF)
				{
					EQC::Common::Network::EQPacket *pack = new EQC::Common::Network::EQPacket;

					if(!SACK.dwGSQ)
					{
			//          pack->HDR.a5_SEQStart   = 1; // Agz: hmmm, yes commenting this makes the client connect to zone
													 //      server work and the world server doent seem to care either way
						SACK.dwARQ              = rand()%0x3FFF;//Current request ack
						SACK.dbASQ_high         = 1;            //Current sequence number
						SACK.dbASQ_low          = 0;            //Current sequence number
			  
						if (debug_level >= 1)
							cout << "New random SACK.dwARQ" << endl;
					}
					if (debug_level >= 3)
						cout << "SACK.dwGSQ:" << SACK.dwGSQ << endl;
					MySendPacketStruct *p = new MySendPacketStruct;
					pack->HDR.b2_ARSP    = 1;
					pack->dwARSP         = dwLastCACK;//CACK.dwARQ;
					pack->dwSEQ = SACK.dwGSQ++;
					p->buffer = pack->ReturnPacket(&p->size);

					if (debug_level >= 6)
					{
						cout << Timer::GetCurrentTime() << " Pure ack sent ";
						cout << "pack->dwARSP=dwLastCACK:" << dwLastCACK << " ";
						cout << "pack->dwSEQ=SACK.dwGSQ:" << SACK.dwGSQ << endl;
					}
					SendQueue.push(p);  

					no_ack_sent_timer->Disable();
					safe_delete(pack);//delete pack;

					return;
				}

				/************ CHECK PACKET MANAGER STATE ************/
				int fragsleft = (app->size >> 9) + 1;

				if (debug_level >= 8)
					cout << Timer::GetCurrentTime() << " fragsleft: " << fragsleft << endl;

				if(pm_state == PM_ACTIVE)
				/************ PM STATE = ACTIVE ************/
				{
					while(fragsleft--)
					{
						EQC::Common::Network::EQPacket *pack = new EQC::Common::Network::EQPacket;
						MySendPacketStruct *p = new MySendPacketStruct;
			    
						if(!SACK.dwGSQ)
						{
							pack->HDR.a5_SEQStart   = 1;
							SACK.dwARQ              = rand()%0x3FFF;//Current request ack
							SACK.dbASQ_high         = 1;            //Current sequence number
							SACK.dbASQ_low          = 0;            //Current sequence number   
							if (debug_level >= 1)
								cout << "New random SACK.dwARQ" << endl;
						}
						if (debug_level >= 5)
							cout << "SACK.dwGSQ:" << SACK.dwGSQ << endl;

						AddAck(pack);

						//IF NON PURE ACK THEN ALWAYS INCLUDE A ACKSEQ              // Agz: Not anymore... Always include ackseq if not a fragmented packet
						if ((app->size >> 9) == 0 || fragsleft == (app->size >> 9)) // If this will be a fragmented packet, only include ackseq in first fragment
							pack->HDR.a4_ASQ = 1;                                   // This is what the eq servers does

						/************ Caculate the next ACKSEQ/acknumber ************/
						/************ Check if its a static ackseq ************/
						if( HI_BYTE(app->opcode) == 0x2000)
						{
							if(app->size == 15)
								pack->dbASQ_low = 0xb2;
							else
								pack->dbASQ_low = 0xa1;

						}
						/************ Normal ackseq ************/
						else
						{
							//If not pure ack and opcode != 0x20XX then
							if (ack_req) // If the caller of this function wants us to put an ack request on this packet
							{
								pack->HDR.a1_ARQ = 1;
								pack->dwARQ      = SACK.dwARQ;
							}

							if(pack->HDR.a1_ARQ && pack->HDR.a4_ASQ)
							{
								pack->dbASQ_low  = SACK.dbASQ_low;
								pack->dbASQ_high = SACK.dbASQ_high;
							}
							else
							{
								if(pack->HDR.a4_ASQ)
								{
									pack->dbASQ_high = SACK.dbASQ_high;
								}
							}
						}

						/************ Check if this packet should contain op ************/
						if(app->opcode)
						{
							pack->dwOpCode = app->opcode;
							restore_op =  app->opcode; // Agz: I'm reusing messagees when sending to multiple clients.
							app->opcode = 0; //Only first fragment contains op
						}
						/************ End opcode check ************/

						/************ SHOULD THIS PACKET BE SENT AS A FRAGMENT ************/
						if((app->size >> 9))
						{
							bFragment = true;
							pack->HDR.a3_Fragment = 1;
						}
						/************ END FRAGMENT CHECK ************/

						if(app->size && app->pBuffer)
						{
							if(pack->HDR.a3_Fragment)
							{
								// If this is the last packet in the fragment group
								if(!fragsleft)
								{
									// Calculate remaining bytes for this fragment
									pack->dwExtraSize = app->size-510-512*((app->size/512)-1);
								}
								else
								{
									if(fragsleft == (app->size >> 9))
									{
										pack->dwExtraSize = 510; // The first packet in a fragment group has 510 bytes for data

									}
									else
									{
										pack->dwExtraSize = 512; // Other packets in a fragment group has 512 bytes for data
									}
								}
								pack->fraginfo.dwCurr = (app->size >> 9) - fragsleft;
								pack->fraginfo.dwSeq  = dwFragSeq;
								pack->fraginfo.dwTotal= (app->size >> 9) + 1;
							}
							else
							{
								pack->dwExtraSize = (int16)app->size;
							}
							pack->pExtra = new uchar[pack->dwExtraSize];
							memcpy((void*)pack->pExtra, (void*)app->pBuffer, pack->dwExtraSize);
							app->pBuffer += pack->dwExtraSize; //Increase counter
						}       

						/******************************************/
						/*********** !PACKET GENERATED! ***********/
						/******************************************/
			            
						/************ Update timers ************/
						if(pack->HDR.a1_ARQ)
						{
							if (debug_level >= 5)
							{
								cout << "OutgoingARQ pack->dwARQ:" << (unsigned short)pack->dwARQ << " SACK.dwARQ:" << (unsigned short)SACK.dwARQ << endl;
							}
							OutgoingARQ(pack->dwARQ);
							ResendQueue.push(pack);
							packetspending++;
							SACK.dwARQ++;
						}
			    
						if(pack->HDR.b2_ARSP)
						{
							OutgoingARSP();
						}

						keep_alive_timer->Start();
						/************ End update timers ************/
			                    
						pack->dwSEQ = SACK.dwGSQ++;
						p->buffer = pack->ReturnPacket(&p->size);
			            
						SendQueue.push(p);
			    
						if(pack->HDR.a4_ASQ)
							SACK.dbASQ_low++;
						
						if (!pack->HDR.a1_ARQ) 
						{
							// Quag: need to delete it since didnt get on the resend queue
							safe_delete(pack);//delete pack;
						}
						//Yeahlight: Zone freeze debug
						if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
							EQC_FREEZE_DEBUG(__LINE__, __FILE__);
					}//end while

					if(bFragment)
					{
						dwFragSeq++;
					}
					app->pBuffer -= app->size; //Restore ptr.
					app->opcode = restore_op; // Agz: Restore opcode
			        
				} //end if
			}

			void EQPacketManager::CheckTimers(void)
			{
				/************ Should packets be resent? ************/
				if (no_ack_received_timer->Check())
				{
					if (debug_level >= 1)
						cout << "Resending packets" << endl;
					EQC::Common::Network::EQPacket *pack;
					MyQueue<EQC::Common::Network::EQPacket> q;
					while(pack = ResendQueue.pop())
					{
						packetspending--;
						q.push(pack);
						MySendPacketStruct *p = new MySendPacketStruct;
						pack->dwSEQ = SACK.dwGSQ++;
						if (debug_level >= 4)
							cout << "Resending packet ARQ:" << pack->dwARQ << " with SEQ:" << pack->dwSEQ << ", count=" << (int16) pack->resend_count << endl;
						p->buffer = pack->ReturnPacket(&p->size);
						SendQueue.push(p);
						//Yeahlight: Zone freeze debug
						if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
							EQC_FREEZE_DEBUG(__LINE__, __FILE__);
					}
					while(pack = q.pop())
					{
						ResendQueue.push(pack);
						packetspending++;
						if(++pack->resend_count > 15) {
							if (debug_level >= 1) {
								cout << "Dropping client, resend_count > 15 on ARQ:" << pack->dwARQ << ", SEQ:" << pack->dwSEQ << endl;
							}
							pm_state = PM_FINISHING;
							MakeEQPacket(0);
						}
						//Yeahlight: Zone freeze debug
						if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
							EQC_FREEZE_DEBUG(__LINE__, __FILE__);
					}

					no_ack_received_timer->Start(1000);
				}
				/************ Should a pure ack be sent? ************/
				if (no_ack_sent_timer->Check() || keep_alive_timer->Check())
				{
					APPLAYER app;
					app.opcode = 0xFFFF;
					MakeEQPacket(&app);
				}
			}

		}
	}
}