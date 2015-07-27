// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include "config.h"

#include "client.h"
#include "eq_packet_structs.h"
#include "packet_dump.h"
#include "database.h"
#include "LoginServer.h"
#include "zoneserver.h"
#include "EQCException.hpp"
#include "ZSList.h"
#include "races.h"
#include "EQCUtils.hpp"
#include "WorldGuildManager.h"
#include "BoatsManager.h"
#include "TimeOfDay.h"

//Conflict resolution
#include "packet_dump_file.h"

using namespace std;
using namespace EQC::World::Network;
using namespace EQC::Common::Network;

extern const char* ZONE_NAME;
extern ZSList zoneserver_list;
extern EQC::World::LoginServer* loginserver;

namespace EQC
{
	namespace World
	{
		// Constructor
		Client::Client(int32 in_ip, int16 in_port, int in_send_socket)
		{
			// Set network information
			this->ignoreLogOut = false;
			this->ip = in_ip;
			this->port = in_port;
			this->send_socket = in_send_socket;

			// Setup Timers
			this->timeout_timer = new Timer(CLIENT_TIMEOUT); // TODO: Whats this timer for?
			this->autobootup_timeout = new Timer(10000); // TODO: Whats this timer for? Shoudl this timer be configurable?
			this->autobootup_timeout->Disable(); // Set the zone bootup timer to sdisabled

			this->account_id = 0; // Initialize the acocount ID

			// Create the network lager
			this->network = new EQC::World::Network::ClientNetwork();

			// Set up the Debug level of the page manager
			this->network->packet_manager.SetDebugLevel(0); // should this be configurable?

			// Zero out our current zone name
			memset(this->zone_name, 0, sizeof(this->zone_name));
			
			// Zero out our char name
			this->char_name[0] = 0;

			// Set it so we initially are not waiting for a zone bootup
			this->pwaitingforbootup = 0;


		}


		// Destructor
		Client::~Client() 
		{
			// Cleanup timers
			safe_delete(this->timeout_timer);
			safe_delete(this->autobootup_timeout);

			// clean up network connections / network layer
			safe_delete(this->network);
		}


		// Sends the Server Message of the Day (MOTD) to the client
		void Client::SendServerMOTD()
		{
			// crate the buffer & strut
			uchar* pBuffer = new uchar[sizeof(ServerMOTD_Struct)];
			ServerMOTD_Struct* smotd = (ServerMOTD_Struct*)pBuffer;

			char servermotd[SERVER_MOTD_LENGTH];

			// get the server motd from the database and put in buffer
			if (Database::Instance()->GetVariable("MOTD", servermotd, SERVER_MOTD_LENGTH))
			{
				// if we sucessfully go the server motd, copy it to the struct
				strncpy(smotd->motd, servermotd, SERVER_MOTD_LENGTH);
			}

			// pass to network to send it off
			this->network->SendServerMOTD(smotd); //TODO: Dark-Prince - 11/05/2008 - Shoudl we make it, so if it cant get MOTD, it wont pass it to the network layer?
		}

		// Sends what expansions the client is enabled for (from server side)
		// 20/10/2007 - froglok23 - Split into own method
		// This  way this was handled before broke the Velious UI. 
		// Please do not edit this unless you have tested it. It isn't broken.
		// -Wizzel

		void Client::SendExpansionInfo()
		{
			//Yeahlight: Bitwise flag ID calculation
			//  0: Antonica
			//  1: Kunark
			//  2: Velious
			//  4: SoL
			//  0 + 1 + 2 + 4 = 7 -> Antoica + Kunark + Velious + SoL

			//Yeahlight: Leave this at 3 (Antonica - Velious) to prevent any exploits with our SoL client (people getting the newUI to function properly).
			int flag = 3;
			char tmp[4];

			//if (Database::Instance()->GetVariable("Expansions", tmp, 3))
			//{
			//	flag = atoi(tmp);
			//}

			this->network->SendExpansionInfo(flag);
		}

		// Sends OP_SendCharInfo to the client.
		// The Char Info is returned from Database::Instance()->GetCharSelectInfos
		void Client::SendCharInfo()
		{
			// has to ber a bete way of doing this!


			// Create the buffer & struct
			uchar* pBuffer = new uchar[sizeof(CharacterSelect_Struct)];
			CharacterSelect_Struct* cs_struct = (CharacterSelect_Struct*)pBuffer;

			// get the char info from the database and place in cs_struct
			Database::Instance()->GetCharSelectInfo(account_id, cs_struct,&weapons); //TODO: Add validation if the command succeeded

			// pas to network to send it off
			this->network->SendCharInfo(cs_struct);
		}

		bool Client::Process()
		{
			bool ret = true;

			// Setup connecting details
			sockaddr_in to;
			memset((char *) &to, 0, sizeof(to));
			to.sin_family = AF_INET;	// Set what family the client is using? - Dark-Prince - 11/05/2008) - needs confirming
			to.sin_port = port;			// Set what port the client is using? - Dark-Prince - 11/05/2008) - needs confirming
			to.sin_addr.s_addr = ip;	// Set what IP the  client is using? - Dark-Prince - 11/05/2008) - needs confirming


			//TOSO: confirm this - Is this the timeout for world to if a zone is up or not?
			if (this->autobootup_timeout->Check())
			{
				// We couldnt get/bootup a zone, so say that no zones are available.
				this->SendZoneUnavail();
			}

			// Get all packets from packet manager out queue and process them
			APPLAYER *inpacket = 0;

			while(ret && (inpacket = this->network->PMOutQueuePop()))
			{
				//TODO: document this entire switch statement
				switch(inpacket->opcode)
				{
				case 0:
					{
						break; // Ignore the packet
					}
				case OP_SendLoginInfo:
					{
						this->ProcessOP_SendLoginInfo(inpacket);
						break;
					}
				case OP_NAMEAPPROVAL:
					{
						this->ProcessOP_NameApproval(inpacket);
						break;			
					}
				case OP_CharacterCreate:
					{
						ret = this->ProcessOP_CharacterCreate(inpacket);
						break;
					}
				case OP_ENTERWORLD:
					{
						this->ProcessOP_EnterWorld(inpacket);
						break;
					}
				case OP_DeleteCharacter:
					{
						ret = this->ProcessOP_DeleteCharacter(inpacket);
						break;
					}
				case OP_WearChange:
					{
						// Used by the sever when a user changes the selected character.
						// The client sends a WearChange packet requesting the model number
						// for what the character is holding in it's hands. -neorab
						//DumpPacket(inpacket->pBuffer, inpacket->size);
						this->ProcessOP_WearChange(inpacket);
						break;
					}
				case OP_GuildsList: // 12th of october 2007 - froglok23 - this so needs way way way more testing!
					{
						guild_mgr.SendGuildsList(this);
						break;
					}
					// 12th of October 2007 - froglok23 - WTF is these ones?  all are size 0
				case 0x2320: // UNKNOWN
				case 0xa980: // UNKNOWN
				case 0x00ab: // UNKNOWN
				case 0x00ac: // UNKNOWN
				case 0x00ad: // UNKNOWN
					{
						this->network->QueuePacket(inpacket);
						break;
					}
				case 0x3521:
				case 0x3921:
					break;
				//case 0x5900:
				//	{
				//		APPLAYER* outapp = new APPLAYER(0x4600, sizeof(0));
				//		this->network->QueuePacket(outapp);
				//		break;
				//	}

				default:
					{
						//Unknown packet was passed in, lets output it as it is unknown and something the client sent us
						cout << "Unknown opcode: 0x" << hex << setfill('0') << setw(4) << inpacket->opcode << dec;
						cout << " size:" << inpacket->size << endl;
						FileDumpPacketHex("UnknownOpcodes.txt", inpacket->pBuffer, inpacket->size);
						//DumpPacket(inpacket->pBuffer, inpacket->size);
						break;
					}
				}
				safe_delete(inpacket);//delete inpacket;
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}    
			/************ Get first send packet on queue and send it! ************/
			MySendPacketStruct* p = 0;    
			while(p = this->network->PMSendQueuePop())
			{
#ifndef WIN32
				sendto(send_socket, p->buffer, p->size, 0, (sockaddr*)&to, sizeof(to));
#else
				sendto(send_socket, (const char *) p->buffer, p->size, 0, (sockaddr*)&to, sizeof(to));
#endif
				safe_delete(p->buffer);
				safe_delete(p);//delete p;
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
			/************ Processing finished ************/
			LockMutex lock(&this->network->MPacketManager);

			// Check if the client is still connected (aka ping it)
			if (!this->network->packet_manager.CheckActive())
			{
				//cout << "Client disconnected" << endl;
				return false; // bail out, we cant ping the client
			}

			// check if the client has timed out
			if (this->timeout_timer->Check())
			{
				//cout << "Client timeout" << endl;
				return false; return false; // bail out, timer has expired
			}

			// Check packet managers timers
			//TODO: redo comments for this
			this->network->packet_manager.CheckTimers();

			return ret;
		}

		// The Client is entering the world
		void Client::EnterWorld(bool TryBootup)
		{
			// Check if the zone name length is 0
			if (strlen(this->zone_name) == 0)
			{
				// OK, we are tyring to zone into a empty zone... bail out!
				return;
			}

			// Get the zone server in which we are being passed to
			ZoneServer* zs = zoneserver_list.FindByName(this->zone_name);

			if (zs)
			{
				// if we can get the zone server, notifiy world we're comming, so it knows not to shutdown
				zs->TriggerBootup();
			}
			else
			{
				if (TryBootup) // check the trybootup flag
				{
					// Start the timer for the timeout of zone bootup
					this->autobootup_timeout->Start();

					cout << "Attempting autobootup of '" << this->zone_name << "' for " << this->char_name << endl;

					if (!(this->pwaitingforbootup = zoneserver_list.TriggerBootup(this->zone_name)))
					{
						// We cant boot up
						cout << "Error: No zoneserver to bootup '" << this->zone_name << "' for " << this->char_name << endl;

						// let the client know the zone is unavaialble
						this->SendZoneUnavail();

					}

					return; // bail out of passing this client to zone
				}
				else
				{
					// we have ben told not to attempt a bootup

					cout << "Error: Player '" << this->char_name << "' requested zone status for '" << this->zone_name << "' but it's not up." << endl;
					
					// let the client know the zone is unavaialble
					this->SendZoneUnavail();
					
					return; // bail out
				}
			}

			// reset so we knwo we arnt waiting for a bootup
			this->pwaitingforbootup = 0;

			//Yeahlight: If client is entering world, we do not remove them from the active_accounts table; flag them to prevent it.
			this->SetIgnoreLogOut(true);

			cout << "Enter world: " << this->char_name << ": " << this->zone_name << endl;

			// Add a record into the database to let knwo zone that this client is now authorized to enter the zone
			Database::Instance()->SetAuthentication(this->account_id, this->char_name, this->zone_name, this->ip);

			// Send Time of Day packet - Kibanu - It saves to client even if only connected to the world server.
			this->network->SendTimeOfDay();

			// Send what zone server and what zone we are passing the client to
			this->network->SendZoneServerInfo(zs, this->zone_name);

		}

		// Notifies the client that the zone they have requested is unavilable
		void Client::SendZoneUnavail()
		{
			// Zero out the zone name
			memset(this->zone_name, 0, sizeof(this->zone_name));

			// reset it so we arnt waiting for a zone bootup
			this->pwaitingforbootup = 0;

			// Disable the auto bootup timer
			this->autobootup_timeout->Disable();

			// pass information to network layer
			this->network->SendZoneUnavail(this->zone_name);
		}

		// Handles reciving of data
		void Client::ReceiveData(uchar* buf, int len) 
		{
			// This does what?
			LockMutex lock(&this->network->MPacketManager);

			// Start a timer for a timeout of reciving data
			this->timeout_timer->Start();

			// pass the data to the packet manager
			this->network->packet_manager.ParceEQPacket(len, buf);
		}

		// returns true if the request is ok, false if there's an error
		bool Client::CheckCharCreateInfo(PlayerProfile_Struct *cc)
		{
			if(!cc) // check if the player profile struct is null
			{
				return false; // its null, bail out with a false
			}

			int bSTR, bSTA, bAGI, bDEX, bWIS, bINT, bCHA, bTOTAL, cTOTAL, stat_points;
			int classtemp, racetemp;
			int Charerrors = 0;


			static const int BaseRace[TOTAL_RACES][7] =
			{				 /* STR  STA  AGI  DEX  WIS  INT  CHR */
				{ /*Human*/      75,  75,  75,  75,  75,  75,  75},
				{ /*Barbarian*/ 103,  95,  82,  70,  70,  60,  55},
				{ /*Erudite*/    60,  70,  70,  70,  83, 107,  70},
				{ /*Wood Elf*/   65,  65,  95,  80,  80,  75,  75},
				{ /*High Elf*/   55,  65,  85,  70,  95,  92,  80},
				{ /*Dark Elf*/   60,  65,  90,  75,  83,  99,  60},                
				{ /*Half Elf*/   70,  70,  90,  85,  60,  75,  75},
				{ /*Dwarf*/      90,  90,  70,  90,  83,  60,  45},
				{ /*Troll*/     108, 109,  83,  75,  60,  52,  40},
				{ /*Ogre*/      130, 122,  70,  70,  67,  60,  37},
				{ /*Halfling*/   70,  75,  95,  90,  80,  67,  50},
				{ /*Gnome*/      60,  70,  85,  85,  67,  98,  60},
				{ /*Iksar*/      70,  70,  90,  85,  80,  75,  55} // Iksar Support - Dark-Prince 22/12/2007
			};


			static const int BaseClass[TOTAL_CLASSES][8] =
			{					/* STR  STA  AGI  DEX  WIS  INT  CHR  ADD*/
				{ /*Warrior*/      10,  10,   5,   0,   0,   0,   0,  25},
				{ /*Cleric*/        5,   5,   0,   0,  10,   0,   0,  30},
				{ /*Paladin*/      10,   5,   0,   0,   5,   0,  10,  20},
				{ /*Ranger*/        5,  10,  10,   0,   5,   0,   0,  20},
				{ /*ShadowKnight*/ 10,   5,   0,   0,   0,   10,  5,  20},
				{ /*Druid*/         0,  10,   0,   0,  10,   0,   0,  30},
				{ /*Monk*/          5,   5,  10,  10,   0,   0,   0,  20},                
				{ /*Bard*/          5,   0,   0,  10,   0,   0,  10,  25},
				{ /*Rouge*/         0,   0,  10,  10,   0,   0,   0,  30},
				{ /*Shaman*/        0,   5,   0,   0,  10,   0,   5,  30},
				{ /*Necromancer*/   0,   0,   0,  10,   0,  10,   0,  30},
				{ /*Wizard*/        0,  10,   0,   0,   0,  10,   0,  30},
				{ /*Magician*/      0,  10,   0,   0,   0,  10,   0,  30},
				{ /*Enchanter*/     0,   0,   0,   0,   0,  10,  10,  30},
				{ /*Beastlord*/     0,  10,   5,   0,  10,   0,   5,  20},
			};

			static const bool ClassRaceLookupTable[TOTAL_CLASSES][TOTAL_RACES]= 
			{						/*Human  Barbarian Erudite Woodelf Highelf Darkelf Halfelf Dwarf  Troll  Ogre   Halfling Gnome  Iksar  Vahshir Froglok*/
				{ /*Warrior*/         true,  true,     false,  true,   false,  true,   true,   true,  true,  true,  true,    true},
				{ /*Cleric*/          true,  false,    true,   false,  true,   true,   true,   true,  false, false, true,    true},
				{ /*Paladin*/         true,  false,    true,   false,  true,   false,  true,   true,  false, false, true,    true},
				{ /*Ranger*/          true,  false,    false,  true,   false,  false,  true,   false, false, false, true,    false},
				{ /*ShadowKnight*/    true,  false,    true,   false,  false,  true,   false,  false, true,  true,  false,   true},
				{ /*Druid*/           true,  false,    false,  true,   false,  false,  true,   false, false, false, true,    false},
				{ /*Monk*/            true,  false,    false,  false,  false,  false,  false,  false, false, false, false,   false},
				{ /*Bard*/            true,  false,    false,  true,   false,  false,  true,   false, false, false, false,   false},
				{ /*Rogue*/           true,  true,     false,  true,   false,  true,   true,   true,  false, false, true,    true},
				{ /*Shaman*/          false, true,     false,  false,  false,  false,  false,  false, true,  true,  false,   false},
				{ /*Necromancer*/     true,  false,    true,   false,  false,  true,   false,  false, false, false, false,   true},
				{ /*Wizard*/          true,  false,    true,   false,  true,   true,   false,  false, false, false, false,   true},
				{ /*Magician*/        true,  false,    true,   false,  true,   true,   false,  false, false, false, false,   true},
				{ /*Enchanter*/       true,  false,    true,   false,  true,   true,   false,  false, false, false, false,   true},
				{ /*Beastlord*/       false, true,     false,  false,  false,  false,  false,  false, true,  true,  false,   false}
			};//Initial table by kathgar, editted by Wiz for accuracy, solar too, addeed to EQC by Dark-Prince

			EQC::Common::PrintF(CP_WORLDSERVER, "Validating char creation info for '%s'...\n", cc->name);

			classtemp = cc->class_ - 1;
			racetemp = cc->race - 1;

			// Iksar support.... might as well keep it in, its not hurting -- Dark-Prince 22/12/2007
			if (cc->race == IKSAR) 
			{
				racetemp = 12;
			}

			// Added a level check, dont want to be able to phoney this one :P - Dark-Prince 22/12/2007
			if((cc->level <1) || (cc->level > MAX_PLAYERLEVEL))
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : Level is out of range for '%s'!\n", cc->name);
				return false;
			}

			// if out of range looking it up in the table would crash stuff
			// so we return from these
			if(classtemp >= TOTAL_CLASSES)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : Class is out of range for '%s'!\n", cc->name);
				return false;
			}
			if(racetemp >= TOTAL_RACES)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : Race is out of range for '%s'!\n", cc->name);
				return false;
			}

			if(!ClassRaceLookupTable[classtemp][racetemp]) //Lookup table better than a bunch of ifs?
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : '%s' has an invalid race/class combo!\n", cc->name);
				// we return from this one, since if it's an invalid combination our table
				// doesn't have meaningful values for the stats
				return false;
			}

			// solar: add up the base values for this class/race
			// this is what they start with, and they have stat_points more
			// that can distributed
			bSTR = BaseClass[classtemp][0] + BaseRace[racetemp][0];
			bSTA = BaseClass[classtemp][1] + BaseRace[racetemp][1];
			bAGI = BaseClass[classtemp][2] + BaseRace[racetemp][2];
			bDEX = BaseClass[classtemp][3] + BaseRace[racetemp][3];
			bWIS = BaseClass[classtemp][4] + BaseRace[racetemp][4];
			bINT = BaseClass[classtemp][5] + BaseRace[racetemp][5];
			bCHA = BaseClass[classtemp][6] + BaseRace[racetemp][6];
			stat_points = BaseClass[classtemp][7];
			bTOTAL = bSTR + bSTA + bAGI + bDEX + bWIS + bINT + bCHA;
			cTOTAL = cc->STR + cc->STA + cc->AGI + cc->DEX + cc->WIS + cc->INT + cc->CHA;

			// solar: the first check makes sure the total is exactly what was expected.
			// this will catch all the stat cheating, but there's still the issue
			// of reducing CHA or INT or something, to use for STR, so we check
			// that none are lower than the base or higher than base + stat_points
			// NOTE: these could just be else if, but i want to see all the stats
			// that are messed up not just the first hit

			if(bTOTAL + stat_points != cTOTAL)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : '%s' stat points total doesn't match expected value: expecting %d got %d!\n", cc->name, bTOTAL + stat_points, cTOTAL);
				Charerrors++;
			}

			// Check is STR is acceptable
			if(cc->STR > bSTR + stat_points || cc->STR < bSTR)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : '%s' stat STR is out of range!\n", cc->name);		
				Charerrors++;
			}

			// Check is STA is acceptable
			if(cc->STA > bSTA + stat_points || cc->STA < bSTA)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : '%s' stat STA is out of range!\n", cc->name);
				Charerrors++;
			}

			// Check is AGI TR is acceptable
			if(cc->AGI > bAGI + stat_points || cc->AGI < bAGI)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : '%s' stat AGI is out of range!\n", cc->name);
				Charerrors++;
			}

			// Check is DEX is acceptable
			if(cc->DEX > bDEX + stat_points || cc->DEX < bDEX)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : '%s' stat DEX is out of range!\n", cc->name);
				Charerrors++;
			}

			// Check is WIS is acceptable
			if(cc->WIS > bWIS + stat_points || cc->WIS < bWIS)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : '%s' stat WIS is out of range!", cc->name);
				Charerrors++;
			}

			// Check is INT is acceptable
			if(cc->INT > bINT + stat_points || cc->INT < bINT)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : '%s' stat INT is out of range!\n", cc->name);
				Charerrors++;
			}

			// Check is CHA is acceptable
			if(cc->CHA > bCHA + stat_points || cc->CHA < bCHA)
			{
				EQC::Common::PrintF(CP_WORLDSERVER, "FAIL : '%s' stat CHA is out of range!\n", cc->name);
				Charerrors++;
			}

			//TODO: Should we... at this point, also check resists? - Dark-Prince

			//TODO: Check for deity/class/race.. it'd be nice, but probably of any real use to hack(faction, deity based items are all I can think of)
			//I am NOT writing those tables - kathgar
			// Ill see if i can set time out to work out the combo's - Dark-Prince 22/12/2007

			bool HasErrors = Charerrors > 0;
			if(HasErrors)
			{
				EQC::Common::PrintF(CP_GENERIC, "FAILURE\b");
				EQC::Common::PrintF(CP_WORLDSERVER,"'%s' did not pass char create validation and contains %d errors, aborting creation!\n", cc->name, Charerrors);
			}
			else
			{
				EQC::Common::PrintF(CP_GENERIC,"SUCCESS\b");
			}

			return !HasErrors; // Dark-Prince - 11/05/2008 - has to be a better way of doing this...
		}
	}
}