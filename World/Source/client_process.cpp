// ***************************************************************
//  client_process   ·  date: 21/12/2007
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
#include "EQCUtils.hpp"
#include "classes.h"
#include "races.h"
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
		// Handle the process of Creating a Character
		bool Client::ProcessOP_CharacterCreate(APPLAYER* inpacket)
		{
			bool ret = true;

			// Quag: This packet seems to be the PlayerProfile struct w/o the space for the checksum at the beginning
			// Scruffy: And some of the end data. *shrug*
			// Dark-Prince 22/12/2007 - movcd this as the first thing it checks, even before account id, whats the
			// point if checking if the accoutn is invalid if the packet size doesnt even match.
			if (inpacket->size != sizeof(PlayerProfile_Struct) - PLAYERPROFILE_CHECKSUM_LENGTH) 
			{
				cout << "Wrong size on OP_CharacterCreate. Got: " << inpacket->size << ", Expected: " << sizeof(PlayerProfile_Struct) - PLAYERPROFILE_CHECKSUM_LENGTH << endl;
			}
			else
			{
				// Validate the Account ID
				if (this->account_id == 0)
				{
					cerr << "Char create with no logged in account -> Expliot/Hacker?" << endl;
					ret = false;
				}
				else
				{
					// TODO: Sanity check in data
					PlayerProfile_Struct cc;
					FileDumpPacket("charcreation.txt", inpacket);
					memset(&cc, 0, sizeof(PlayerProfile_Struct));
					memcpy((char*) &cc.name, inpacket->pBuffer, sizeof(PlayerProfile_Struct)-PLAYERPROFILE_CHECKSUM_LENGTH);
					memset(cc.GroupMembers[0], 0, 6*sizeof(cc.GroupMembers[0])); // Tazadar :this makes Vista users crash with new chars
					/*memset(cc.bank_inv,0xFFFF,8*sizeof(cc.bank_inv[0]));
					memset(cc.containerinv,0xFFFF,80*sizeof(cc.containerinv[0]));
					memset(cc.inventory,0xFFFF,30*sizeof(cc.inventory[0]));*/
					/*cout << "checksum :" << cc.checksum << " ok?"<< endl;
					cout << "name is:" << cc.name << " ok?"<< endl;
					cout << "surname is :" << cc.Surname << " ok?"<< endl;
					cout << "gender is :" << (int)cc.gender << " ok?"<< endl;
					cout << "deity is :" << (int)cc.deity << " ok?"<< endl;
					cout << "race is :" << (int)cc.race << " ok?"<< endl;
					cout << "class is :" << (int)cc.class_ << " ok?"<< endl;
					cout << "pp_unknown3 is :" << (int)cc.pp_unknown3 << " ok?"<< endl;*/

					//PlayerProfile_NoChecksum_Struct* in;
					//in = (PlayerProfile_NoChecksum_Struct*)inpacket->pBuffer;

					/*cout << "name is :" << in->name << " ok?"<< endl;
					cout << "surname is :" << in->Surname << " ok?"<< endl;
					cout << "gender is :" << (int)in->gender << " ok?"<< endl;
					cout << "deity is :" << (int)in->deity << " ok?"<< endl;
					cout << "race is :" << (int)in->race << " ok?"<< endl;
					cout << "class is :" << (int)in->class_ << " ok?"<< endl;
					cout << "pp_unknown3 is :" << (int)in->pp_unknown3 << " ok?"<< endl;*/



					/*memcpy(cc.name,in->name,sizeof(in->name));
					memcpy(cc.Surname,in->Surname,sizeof(in->Surname));	
					cc.gender=in->gender;
					cc.deity=in->deity;					
					cc.race=in->race;			
					cc.class_=in->class_;			
					cc.level=in->level;					
					cc.exp=in->exp;					
					cc.mana=in->mana;						
					cc.face=in->face;
					cc.cur_hp=in->cur_hp;						
					cc.STR=in->STR;					
					cc.STA=in->STA;						
					cc.CHA=in->CHA;						
					cc.DEX=in->DEX;						
					cc.INT=in->INT;					
					cc.AGI=in->AGI;						
					cc.WIS=in->WIS;						
					memcpy(cc.languages,in->languages,sizeof(in->languages));
					memcpy(cc.inventory,in->inventory,sizeof(in->inventory));				
					memcpy(cc.containerinv,in->containerinv,sizeof(in->containerinv));		
					memcpy(cc.cursorbaginventory,in->cursorbaginventory,sizeof(in->cursorbaginventory));	
					memcpy(cc.buffs,in->buffs,sizeof(in->buffs));	
					memcpy(cc.spell_book,in->spell_book,sizeof(in->spell_book));			
					memcpy(cc.spell_memory,in->spell_memory,sizeof(in->spell_memory));		
					cc.y=in->y;							
					cc.x=in->x;							
					cc.z=in->z;							
					cc.heading=in->heading;
					memcpy(cc.current_zone,in->current_zone,sizeof(in->current_zone));
					cc.platinum=in->platinum;					
					cc.gold=in->gold;						 
					cc.silver=in->silver;						
					cc.copper=in->copper;					
					cc.platinum_bank=in->platinum_bank;
					cc.gold_bank=in->gold_bank;
					cc.silver_bank=in->silver_bank;
					cc.copper_bank=in->copper_bank;
					memcpy(cc.skills,in->skills,sizeof(in->skills));				
					cc.gm=in->gm;							
					memcpy(cc.bind_point_zone,in->bind_point_zone,sizeof(in->bind_point_zone));
					memcpy(cc.start_point_zone,in->start_point_zone,sizeof(in->start_point_zone));
					memcpy(cc.bind_location,in->bind_location,sizeof(in->bind_location));		
					cc.time1=in->time1;
					memcpy(cc.bank_inv,in->bank_inv,sizeof(in->bank_inv));		
					memcpy(cc.bank_cont_inv,in->bank_cont_inv,sizeof(in->bank_cont_inv));		
					cc.guildid=in->guildid; 
					cc.time2=in->time2;
					cc.anon=in->anon;						
					cc.guildrank=in->guildrank;	
					cc.eqbackground=in->eqbackground;				
					//memcpy(cc.GroupMembers,in->GroupMembers,sizeof(in->GroupMembers));	*/	

					
					
					
							
					//validate the char creation struct
					if(!CheckCharCreateInfo(&cc)) // if it faild, return false
					{
						//return false;
					}

					//TODO: Newer versions have this as a CharCreate_Struct which is different to the playerprofile
					// this is all confirmed of course if our structu si different, but it may make sense. so it needs investigating
					// if it is different, lets get another struct created and try and figure out what itall means.
					// but yeah, something which def needs confirming, however this method does already have struct size checkign on it, 
					// so it cant be *THAT* much different if at all, but still an idea. - Dark-Prince
					//PlayerProfile_Struct* in = (PlayerProfile_Struct*)inpacket->pBuffer; 

					//cc.unknown8104[0]=0x0f;  //Cofruben_TMP: cuidado con esto!! Si peta puede ser esto.

					//TODO: This crashes the client at the moment... no when the char is created,
					// but when the char tries to zone in.. my guess the struct is wrong in concern to languages - Dark-Prince 22/12/2007
					//SetRacialLanguages(&cc);

					// Add Starting items and locations to the newly created char
					Database::Instance()->SetStartingItems(&cc, (int8) cc.race, (int8) cc.class_, (char*) &cc.name);
					Database::Instance()->SetStartingLocations(&cc, (int8) cc.race, (int8) cc.class_, (char*) &cc.name);
					cc.invItemProprieties[22].charges=5;//we have 5 food
					cc.invItemProprieties[23].charges=5;//we have 5 drink

					//If the player has a 100+ admin level, flag the character GM automagically.
					if(this->admin >= 100)
					{
						cc.gm = 1; // set GM flag
					}

					// Set Defaults

					// Lets always start at level 1
					cc.level = 1;

					cc.eqbackground = 0;

					// Attempt to save the char to the database
					ret = Database::Instance()->CreateCharacter(this->account_id, &cc);

					if (ret)
					{
						// If creating the char was successful, send the struct to the client
						this->SendCharInfo();
					}
					else
					{
						// saving of the new char failed.

						// output the info to the console
						EQC::Common::PrintF(CP_WORLDSERVER, "Unable to save '%s'", cc.name);

						// If creating the char faild, send the struct to the client
						this->network->SendCreateCharFailed();
					}
				}
			}

			return ret;
		}


		// Handles name approval when a attempt to create a characture is made by the user
		void Client::ProcessOP_NameApproval(APPLAYER* inpacket)
		{
			// Validate the Account ID
			if (this->account_id == 0)
			{
				cerr << "Name approval with no logged in account. Expliot/Hacker?" << endl;
			}
			else
			{
				//TODO: Fix Size checking after confirming the struct size and data
				if (inpacket->size != sizeof(NameApproval_Struct))
				{
					cout << "Wrong size on OP_NameApproval. Got: " << inpacket->size << ", Expected: " << sizeof(NameApproval_Struct) << endl;
					cout << "However, ignoring as the struct is not entirly flushed out!" << endl;
				}

				// Crate the Name Approval Struct from the inpacket
				NameApproval_Struct* nas = (NameApproval_Struct*)inpacket->pBuffer;

				bool valid = false;

				if (Database::Instance()->CheckNameFilter(nas->charname)) // Check if the name is already in use
				{
					valid = false;
				}
				else if (Database::Instance()->ReserveName(this->account_id, nas->charname)) // check if the name has been reserved
				{
					valid = true;
				}
				else
				{
					valid = false;
				}

				// Show the result to the console
				EQC::Common::PrintF(CP_WORLDSERVER, "Name approval request for: %s race: %s class: %s...",nas->charname, EQC::Common::GetRaceName(nas->race), EQC::Common::GetClassName(nas->class_));

				if(valid)
				{
					EQC::Common::PrintF(CP_GENERIC, "SUCCESS!\n");
				}
				else
				{
					EQC::Common::PrintF(CP_GENERIC, "FALIURE!\n!");
				}

				// Send the network packet to the client
				this->network->SendNameApproval(valid);

			}
		}

		// Closed the packet manager and queues a packet of 0?
		void Client::CloseClientPackageManager()
		{
			// close the packet manager
			this->network->packet_manager.Close();

			// queue an empty packet?
			//this->network->QueuePacket(0); //TODO: What does queuing 0 do? - Dark-Prince
		}

		// Handles the client entering the world
		void Client::ProcessOP_EnterWorld(APPLAYER* inpacket)
		{
			// Validate the Account ID
			if (this->account_id == 0)
			{
				cerr << "Enter world with no logged in account" << endl;

				// Close the clinets packet manager
				this->CloseClientPackageManager();

				return; // bail out
			}
			else
			{
				// Added EnterWorld_Struct, appeats to just be char name with length based on that
				// see struct for more details - Dark-Prince 22/12/2007
				EnterWorld_Struct * ews = (EnterWorld_Struct*)inpacket->pBuffer;

				// copy the charname from the packet
				strncpy(this->char_name,(char*)inpacket->pBuffer,16);

				// Make sure this account owns this character
				if (Database::Instance()->GetAccountIDByChar(char_name) != this->account_id)
				{
					cerr << "This account does not own this character" << endl;

					// closes the clients packet manager
					this->CloseClientPackageManager();

					return; // bail out
				}

				// load up the Player Profile from the database
				PlayerProfile_Struct pp;
				if (Database::Instance()->GetPlayerProfile(this->account_id, char_name, &pp) == 0)
				{
					cerr << "Could not get PlayerProfile for " << char_name << endl;
					
					// closes the clients packet manager
					this->CloseClientPackageManager();

					return; // bail out
				}

				// Ensure that the player profile is correct still andone hasnt been injected! - Dark-Prince 22/12/2007
				//TODO: Ensure that this method isnt affected by such thinsg as item stats and thinsg which alter whatthe method checks... int/wis/dex/etc
				if(!CheckCharCreateInfo(&pp))
				{
					cout<<"BAD PLAYER PROFILE STRUCT!!"<<endl;
					//return;
				}
				


				// Check if we cant get the zones safe_points, if we cant, it means they are in an invalid zone, so lets move them to the arena
				if (!Database::Instance()->GetSafePoints(pp.current_zone)) 
				{
					// This is to save people in an invalid zone, once it's removed from the DB
					//TODO: Fix this hardcoding
					strcpy(pp.current_zone, "arena");
					pp.x = 1050;
					pp.y = -50;
					pp.z = 3.2;

					// Update the player profile
					Database::Instance()->SetPlayerProfile(this->account_id, char_name, &pp);
				}

				// update the clients object zone_name vairable to that of the player profile
				strcpy(this->zone_name, pp.current_zone);

				// Send the server MOTD
				this->SendServerMOTD();

				//Yeahlight: Part III of IV to log the account into the database
				Database::Instance()->LogAccountInPartIII(this->ls_account, this->ip);

				// Allow the client to enter the world
				this->EnterWorld();
			}
		}

		// Processes login information and details
		bool Client::ProcessOP_SendLoginInfo(APPLAYER* inpacket)
		{
			bool ret = true;
			char name[19];
			char password[16];

			// Zero out the username
			memset(name, 0, sizeof(name));

			// Zero out the password
			memset(password, 0, sizeof(password));

			// Copy the name from the in packet to the variable
			strncpy(name, (char*)inpacket->pBuffer, 18);

			//TODO: still flush this out so wecan make sure its all right. so far we have got account name and apssword of this struct
			LoginInfo_Struct* lis = (LoginInfo_Struct*)inpacket->pBuffer;

			if (inpacket->size < strlen(name)+2)
			{
				return false;
			}

			// Copy the password out
			strncpy(password, (char*)&inpacket->pBuffer[strlen(name)+1], 15);

			if (strncasecmp(name, "LS#", 3) == 0)
			{
				if (loginserver == 0)
				{
					cout << "Error: Login server login while not connected to login server." << endl;
					return false;
				}
				LSAuth_Struct* lsa = 0;

				if (lsa = loginserver->CheckAuth(atoi(&name[3]), password))
				{
					// gets the account of the user based on thier login account id? wtf - Dark-Prince
					this->account_id = Database::Instance()->GetAccountIDFromLSID(lsa->lsaccount_id);

					this->ls_account = lsa->lsaccount_id;

					// Validate the Account ID
					if (this->account_id == 0)
					{
						// create a loing account? - erm? wtf? Dark-Park-Prince
						Database::Instance()->CreateAccount(lsa->name, "", 0, lsa->lsaccount_id);

						// get the newly created accounts ID
						this->account_id = Database::Instance()->GetAccountIDFromLSID(lsa->lsaccount_id);

						// Validate the Account ID
						if (this->account_id == 0)
						{
							// TODO: Find out how to tell the client wrong username/password
							cerr << "Error adding local account for LS login: '" << lsa->name << "', duplicate name?" << endl;
							return false;
						}
					}

					// Set it so this is not our first connect?
					lsa->firstconnect = false;

					// loginserver->RemoveAuth(lsa->lsaccount_id);
					cout << "Logged in: LS#" << lsa->lsaccount_id << ": " << lsa->name << endl;
				}
				else 
				{
					// TODO: Find out how to tell the client wrong username/password
					cerr << "Bad/expired session key: " << name << endl;
					return false;
				}
			}
			else if (strlen(password) <= 1) 
			{
				// TODO: Find out how to tell the client wrong username/password
				cerr << "Login without a password" << endl;
				return false; // bail out, we must have a password
			}
			else 
			{
				// Check the login and get an account id back
				this->account_id = Database::Instance()->CheckLogin(name,password);

				// Validate the Account ID
				if (this->account_id == 0)
				{
					// TODO: Find out how to tell the client wrong username/password
					struct in_addr	in;
					in.s_addr = ip;
					cerr << inet_ntoa(in) << ": Wrong name/pass: name='" << name << "'" << endl;
					return false;
				}

				cout << "Logged in: " << name << endl;
			}

			// Set admin flag
			this->admin = Database::Instance()->CheckStatus(this->account_id);

			// tell the client its approved
			this->network->SendLoginApproved();

			// tell the client to enter world
			this->network->SendEnterWorld();

			// Send what expaions the client should have enabled
			this->SendExpansionInfo();

			// We are logging in and want to see character select
			this->SendCharInfo();

			Database::Instance()->LogAccountInPartII(this->ls_account, this->ip);

			return true;
		}

		// deletes a char at the users request
		bool Client::ProcessOP_DeleteCharacter(APPLAYER* inpacket)
		{
			// delete the char from the database
			bool ret = Database::Instance()->DeleteCharacter((char*)inpacket->pBuffer);

			if(ret)
			{
				// if the delete was successful, send the update to the client
				this->SendCharInfo();
			}

			return ret;
		}

		// Items have changed, is this at char select when flicking though chars? - Dark-Prince
		void Client::ProcessOP_WearChange(APPLAYER* inpacket)
		{
			if (inpacket->size != sizeof(WearChange_Struct))
			{
				cout << "Wrong size on OP_WearChange. Got: " << inpacket->size << ", Expected: " << sizeof(WearChange_Struct) << endl;
			}
			else
			{
				WearChange_Struct* wc = (WearChange_Struct*)inpacket->pBuffer;
				//DumpPacket(inpacket);
				switch(wc->sub_op) 
				{
				case SUB_ChangeChar:
					{
						
						// Not sure wtf this part is... I think it's to inform server that
						// a change in char select has happened.  I don't think it matters
						// though since it doesn't send any other info and requests the
						// pieces it needs anyway. -neorab
						break;
					}

				default://if its not a changechar its a Request to see weapons (the sub_op seems to change for every char)
					{
						//Yeahlight: Disabling this for now until I fix the struct
						//if(wc->flag==0x9D)//If the client wants to check the changes we break to avoid mass packet flood !
						//	break;
						//int16 itemid=0;
						//if(wc->wear_slot_id==7){
						//	itemid = weapons.righthand[wc->slot_graphic-1];
						//}
						//if(wc->wear_slot_id==8 ){
						//	itemid = weapons.lefthand[wc->slot_graphic-1];
						//}
						//this->network->SendWearChangeRequestSlot(wc,itemid);
						//break;
					}
				}
			}
		}
	}
}