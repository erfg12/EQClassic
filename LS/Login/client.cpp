#include "../common/debug.h"
#include <iostream>
#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
	#include <process.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif
#include <string.h>
#include <iomanip>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

using namespace std;

#include "../common/EQNetwork.h"
#include "net.h"
#include "client.h"
#include "../common/eq_packet_structs.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "../common/database.h"
#include "login_opcodes.h"
#include "login_structs.h"
#include "LWorld.h"

//#define LS_VERSION "12-10-2001 1750"
//Yeahlight: This is our version for the EQTrilogy client; thanks Secrets =P
#define LS_VERSION "8-09-2001 14:25"

#ifndef LS_VERSION
//#define LS_VERSION "09-05-2001 9:05"
//#define LS_VERSION "11-04-2001 18:00"
//#define LS_VERSION "12-10-2001 1750"
//#define LS_VERSION "2-4-2001 1230"
//#define LS_VERSION "5-28-2002 1140"
#ifndef NEWDLLSTRUCT
	#define LS_VERSION "4-9-2002 1640"
#else
	#define LS_VERSION "6-6-2003 1200" //5.0
//#define LS_VERSION "12-4-2002 1800" //4.4
#endif // #ifndef NEWDLLSTRUCT
#endif // #ifndef LS_VERSION

#ifndef MINILOGIN
	extern Database			database;
#endif
extern NetConnection	net;
extern LWorldList		world_list;
#if defined(LOGINCRYPTO) && !defined(MINILOGIN) && !defined(PUBLICLOGIN)
	#include "EQCrypto.h"
	EQCrypto eq_crypto;
#endif
Client::Client(EQNetworkConnection* ieqnc) {
	eqnc = ieqnc;
	ip = eqnc->GetrIP();
	port = ntohs(eqnc->GetrPort());
	account_id = 0;
	memset(account_name, 0, sizeof(account_name));
	lsadmin = 0;
	worldadmin = 0;
	lsstatus = 0;
	kicked = false;
	verified = false;
	memset(bannedreason, 0, sizeof(bannedreason));
	worldresponse_timer = new Timer(10000);
	worldresponse_timer->Disable();

	LoginMode = None;
}

Client::~Client() {
	safe_delete(worldresponse_timer);
	eqnc->Free();
}

bool Client::Process() { //NOTE: This is where a player logs in
    if (!kicked) {
		/************ Get all packets from packet manager out queue and process them ************/
		APPLAYER *app = 0;
		while(app = eqnc->PopPacket())
		{
#define cout_ALL_PACKETS
#ifdef cout_ALL_PACKETS
if (app->opcode != 0) {
cout << Timer::GetCurrentTime() << " Received opcode: 0x" << hex << setfill('0') << setw(4) << app->opcode << dec << " size:" << app->size << endl;
if (app->size > 0) DumpPacket(app);
}
#endif
if(worldresponse_timer && worldresponse_timer->Check())
{
FatalError(WorldDownErrorMessage);
worldresponse_timer->Disable();
}
			switch(app->opcode)
			{
				case OP_Version:
				{
					char buf[20];
#ifdef MINILOGIN
					strcpy(buf, LS_VERSION);
#else
					if (!database.GetVariable("LoginVersion", buf, sizeof(buf)-1) || strlen(buf) <= 3) {
						strcpy(buf, LS_VERSION);
#ifdef PUBLICLOGIN
				printf("Database Timestamp: %s\n",LS_VERSION);
#endif
					}
#endif
					APPLAYER *outapp;
					outapp = new APPLAYER;
					outapp->opcode = OP_Version;
					outapp->size = strlen(buf) +1;
					outapp->pBuffer = new uchar[outapp->size];
					memset(outapp->pBuffer, 0, outapp->size);
					strcpy((char*) outapp->pBuffer, buf);
					QueuePacket(outapp);
					delete outapp;	
					break;
				}
				case OP_LoginBanner:
				{
					char buf[501];
					APPLAYER *outapp;
					outapp = new APPLAYER;
					outapp->opcode = OP_LoginBanner;
					outapp->size = 5;
					memset(buf, 0, sizeof(buf));
#ifdef MINILOGIN
					//outapp->size += snprintf(buf, 475, "Welcome: %s", account_name);
					outapp->size += snprintf(buf, 475, "Welcome To EverQuest!", account_name); //newage: This is always eqemu
#else
					if (database.GetVariable("LoginBanner", buf, 500)) {
						outapp->size += strlen(buf);
					}
					else {
						strcpy(buf, "Welcome to EQEmu!");
						outapp->size += strlen("Welcome to EQEmu!");
					}
#endif
					if (strlen(buf) == 0) {
						delete outapp;
						break;
					}
					outapp->pBuffer = new uchar[outapp->size];
					memset(outapp->pBuffer, 0, outapp->size);
					outapp->pBuffer[0] = 1;
					outapp->pBuffer[1] = 0;
					outapp->pBuffer[2] = 0;
					outapp->pBuffer[3] = 0;
					strcpy((char *)&outapp->pBuffer[4], buf);
					QueuePacket(outapp);
					delete outapp;
					break;
				}
				case OP_RequestServerStatus:
				{
					//Yeahlight: Before we send back the server request, check if this account is already logged in
#ifndef MINILOGIN
					if(database.AccountLoggedIn(this->account_id)){
						FatalError("Error 1018: You currently have an active character on that EverQuest Server, please allow a minute for synchronization and try again.");
						break;
					}
#endif

					APPLAYER *outapp;
					outapp = new APPLAYER;
					outapp->opcode = OP_SendServerStatus;
					outapp->size = 0;
//			        outapp->pBuffer = new uchar[outapp->size];
					QueuePacket(outapp);
					delete outapp;
					break;
				}
				case 0x8e00:{
					APPLAYER *outapp;
					outapp = new APPLAYER(OP_SessionId, sizeof(SessionId_Struct));
					SessionId_Struct* s_id = (SessionId_Struct*)outapp->pBuffer;
					// this is submitted to world server as "username"
					strcpy(s_id->session_id, this->account_name);
					strcpy(s_id->unused, "unused");
					s_id->unknown = 4;
					QueuePacket(outapp);
					delete outapp;
					break;
				}
				case OP_ServerList: {
					APPLAYER* outapp = new APPLAYER;
					world_list.MakeServerListPacket(outapp, lsadmin);
					FastQueuePacket(&outapp);
					break;
				}
				case OP_SessionKey:
				{
#ifdef MINILOGIN
					APPLAYER *outapp;
					outapp = new APPLAYER;
					outapp->opcode = OP_SessionKey;
					outapp->size = 17;
					outapp->pBuffer = new uchar[outapp->size];
					memset(outapp->pBuffer, 0, outapp->size);
					// this is submitted to world server as "password"
					strncpy((char*) &outapp->pBuffer[1], account_password, 15);
					QueuePacket(outapp);
					delete outapp;
#else
					if (account_id == 0) {
						cerr << "ERROR: OP_SessionKey while not logged in" << endl;
					}
					else {
						LWorld* world = world_list.FindByAddress((char*) app->pBuffer);
						if (world == 0) {
							FatalError(WorldEqualZeroErrorMessage);
						}
						else if (world->GetStatus() == -1) {
							FatalError(WorldDownErrorMessage);
						}
						else if (world->GetStatus() == -2 && !(lsadmin >= 1 || world->GetAdmin() == account_id)) {
							ServerPacket* pack = new ServerPacket(ServerOP_UsertoWorldReq, sizeof(UsertoWorldRequest_Struct));
							UsertoWorldRequest_Struct* ustwr = (UsertoWorldRequest_Struct*) pack->pBuffer;
							ustwr->lsaccountid = account_id;
							ustwr->worldid = world->GetID();
							if (world->GetRemoteID()) {
								ustwr->FromID = 0;
								ustwr->ToID = world->GetRemoteID();
								world->GetLink()->SendPacket(pack);
							}
							else {
								ustwr->FromID = 0;
								ustwr->ToID = 0;
								world->SendPacket(pack);
							}
							delete pack;

							worldresponse_timer->Start(2500,true);
						}
						else {
							// Make a 15 char random key out of printable characters
							char key[16]; memset(key, 0, sizeof(key));
							int8 y;
							for (int x = 0; x < 15; x++) {
								y = (rand() % 62) + 48;
								if (y > 57) y = y + 7;
								if (y > 90) y = y + 6;
								key[x] = y;
							}
							if(lsadmin >= 1 || world->GetAdmin() == account_id) {
								WorldResponse(world->GetID(),1);
								break;
							}
							ServerPacket* pack = new ServerPacket(ServerOP_UsertoWorldReq, sizeof(UsertoWorldRequest_Struct));
							UsertoWorldRequest_Struct* ustwr = (UsertoWorldRequest_Struct*) pack->pBuffer;
							ustwr->lsaccountid = account_id;
							ustwr->worldid = world->GetID();
							if (world->GetRemoteID()) {
								ustwr->FromID = 0;
								ustwr->ToID = world->GetRemoteID();
								world->GetLink()->SendPacket(pack);
							}
							else {
								ustwr->FromID = 0;
								ustwr->ToID = 0;
								world->SendPacket(pack);
							}
							delete pack;
							worldresponse_timer->Start(2500,true);
						}
					}
#endif
					break;

				}
				case OP_LoginInfo: {
					if (app->size < sizeof(LoginInfo_Struct)) {
						printf("Wrong size: OP_LoginInfo, size=%i, expected: %i",app->size,sizeof(LoginInfo_Struct));
						FatalError("Malformed OP_LoginInfo");
						break;
					}
#ifdef MINILOGIN
					//strcpy(account_name, "eqemu");
					//trcpy(account_password, "eqemu");
					// Lookup account info from MiniLoginAccounts.ini
					
					for (int i=0; i<MaxMiniLoginAccounts; i++) {
						if (!net.MiniLoginAccounts[i].IP)
						{
							cout << "Cant find IP:" << GetIP() << " expected:" << net.MiniLoginAccounts[i].IP << endl;
							break;
						}
						else if (net.MiniLoginAccounts[i].IP == GetIP()) {
							strcpy(account_name, net.MiniLoginAccounts[i].username);
							strcpy(account_password, net.MiniLoginAccounts[i].password);
							struct in_addr  in;
							in.s_addr = GetIP();
							cout << "Matched " << inet_ntoa(in) << " to '" << account_name << "'" << endl;
							break;
						}
						else
						{
							cout << "IP Doesn't match: " << GetIP() << " expected:" << net.MiniLoginAccounts[i].IP << endl;
							break;
						}
			
					}

					APPLAYER *outapp;
					outapp = new APPLAYER(OP_SessionId, sizeof(SessionId_Struct));

					SessionId_Struct* s_id = (SessionId_Struct*)outapp->pBuffer;
					// this is submitted to world server as "username"
					strcpy(s_id->session_id, account_name);
					strcpy(s_id->unused, "unused");
					s_id->unknown = 4;
					QueuePacket(outapp);
					delete outapp;
#else
//cout << "LoginInfo packet:" << endl;
//DumpPacket(app);
					
					LoginInfo_Struct* li = (LoginInfo_Struct*) app->pBuffer;
#if defined(LOGINCRYPTO) && !defined(MINILOGIN) && !defined(PUBLICLOGIN)
						
					account_id = database.CheckEQLogin(app, account_name, &lsadmin, &lsstatus, &verified, &worldadmin,ip);

#else
					account_id = CheckLoginInfo(li->crypt, this->ip);
					
					if (!database.GetLSAccountInfo(account_id, account_name, &lsadmin, &lsstatus, &verified, &worldadmin))
						account_id = 0;
					
					//I'm sorry sir...but your Credit Card...has been denied.
#endif
					if(account_id == -1){
						FatalError(ErrorAccountBanished);
						break;
					}
					else if (account_id <= 0) {
						// disconnect isnt needed here, since the person isnt logged in (and causes client to go funky)
						FatalError(PasswordErrorMessage);
						break;
					}
					else if (account_id == ACCOUNT_TEMP_SUSPENDED)
					{
						FatalError("Login requests from this IP address have been temporarily SUSPENDED for 15 minutes.");
						break;
					}
					else if (account_id == ACCOUNT_PERM_SUSPENDED)
					{
						FatalError("Login requests from this IP address have been SUSPENDED! Please contact a server administrator for assistance.");
						break;
					}
					//Bypass account status checks if the user is an admin.
					else if (lsadmin < 50) {
						// They don't have their email verified.  Kickem
						if (verified == false) {
							FatalError(ErrorAccountNotVerified);
							break;
						}
						// They're suspended...Tell them about it.
						if (lsstatus == 40) {
							FatalError(ErrorAccountSuspended);
							break;
						}
						// They're banished...Tell them about it.
						if (lsstatus == 50) {
							FatalError(ErrorAccountBanished);
							break;
						}
#ifdef PUBLICLOGIN
						if(account_id > 12)
						{
							printf("A account with id# %d attempted to connect, breaks restrictions. (Max accounts: 12)\n",account_id);
							FatalError(ErrorAccountBanished);
							break;
						}
#endif
					}
					LoginMode = Normal;
					APPLAYER *outapp;
					outapp = new APPLAYER(OP_SessionId, sizeof(SessionId_Struct));
					
					SessionId_Struct* s_id = (SessionId_Struct*)outapp->pBuffer;
					// this is submitted to world server as "username"
					sprintf(s_id->session_id, "LS#%i", account_id);
					strcpy(s_id->unused, "unused");
					s_id->unknown = 4;

					QueuePacket(outapp);
					delete outapp;
#endif
					break;
				}
				case 0x8800: {
					// Not a clue what this is, but lets keep the client happy
					APPLAYER* outapp = new APPLAYER(0x8800, 20);
					outapp->pBuffer[0] = 1;
					QueuePacket(outapp);
					delete outapp;
					break;
				}
#ifndef MINILOGIN
				case OP_Reg_ChangeAcctLogin: {
#if !defined(LOGINCRYPTO) || defined(MINILOGIN) | defined(PUBLICLOGIN)
					FatalError(ErrorRegUnavail);
					break;
#else
					if (app->size < sizeof(LoginInfo_Struct)) {
//						cout << << "Wrong size: OP_Reg_ChangeAcctLogin, size=" << app->size << ", expected >=" << sizeof(LoginInfo_Struct) << endl;
						FatalError("Malformed OP_Reg_ChangeAcctLogin");
						break;
					}
					
						char buf[40];
						account_id = database.CheckEQLogin(app, account_name, &lsadmin, &lsstatus, &verified, &worldadmin,ip);
						if (!database.GetLSAccountInfo(account_id, account_name, &lsadmin, &lsstatus, &verified, &worldadmin))
							account_id = 0;
						if (account_id == 0) {
							cout << "me" << endl;
							FatalError(PasswordErrorMessage);
							break;
						}
						else if (!verified) {
							FatalError(ErrorAccountNotVerified);
							break;
						}
						else {
							LoginMode = Registration;
							char tmp[30];
							sprintf(tmp, "LS#%i", account_id);
							APPLAYER *outapp = new APPLAYER(OP_Reg_ChangeAcctLogin, strlen(tmp) + 13); // sessionID + null + "unused" + null + 0x01 + 0x00000000
							strcpy((char*) outapp->pBuffer, tmp);
							strcpy((char*) &outapp->pBuffer[strlen(tmp)+1], "unused");
							outapp->pBuffer[strlen(tmp) + 8] = 0x01;
							DumpPacket(outapp);
							QueuePacket(outapp);
							delete outapp;
						}
					
#endif
					break;
				}
				case OP_ChangePassword: {
						FatalError("Error 31337: Can't change your password here!  Check the forums!");
						break;
					// Quag: packet format is oldpass + null + newpass + null (no encryption)
					if (account_id == 0) {
						FatalError("Error: OP_ChangePassword when not logged in");
						break;
					}
					else if (LoginMode != Registration) {
						FatalError("Error error: OP_ChangePassword when not in Registration mode");
						break;
					}
					else if (strlen((char*) app->pBuffer) > 15 || (strlen((char*) app->pBuffer) + 2) >= app->size || strlen((char*) &app->pBuffer[strlen((char*) app->pBuffer)+1]) > 15) {
//						cout << << "Corrupted OP_ChangePassword packet." << endl;
						FatalError("Error error: Corrupted OP_ChangePassword packet");
						break;
					}
					int32 tmp = database.ChangeLSPassword(account_id, (char*) &app->pBuffer[strlen((char*) app->pBuffer)+1], (char*) app->pBuffer);
					if (tmp == 2) {
						FatalError("Error error: Unable to change password (database error)");
						break;
					}
					else if (tmp == 1) {
						FatalError("Error error: Old password didnt match. Password not changed.");
						break;
					}
					else if (tmp == 0) {
						APPLAYER* outapp = new APPLAYER(OP_ChangePassword, strlen(PasswordChangeMessage) + 1);
						strcpy((char*) outapp->pBuffer, PasswordChangeMessage);
						QueuePacket(outapp);
						delete outapp;
					}
					else {
						FatalError("Error error: Unable to change password (unknown error)");
						break;
					}
					break;
				}
				case OP_Reg_GetPricing2:
				case OP_Reg_GetPricing:	{
#if !defined(LOGINCRYPTO) || defined(MINILOGIN) | defined(PUBLICLOGIN)
					FatalError(ErrorRegUnavail);
					break;
#else
					
//					FatalError(ErrorRegUnavail);
					// Quag: Packet format: int32 pricing options count + pricing 1 name + null + (pricing 2 name + null, ...) + Pricing 1 amount (ex: "$1.50") + null + (pricing2 amount + null, ...)
					APPLAYER* outapp = new APPLAYER(OP_Reg_SendPricing, 14);
					*((int32*) outapp->pBuffer) = 1;
					strcpy((char*) &outapp->pBuffer[4], "Free");
					strcpy((char*) &outapp->pBuffer[9], "Free");
					QueuePacket(outapp);
					delete outapp;
#endif
					break;
				}
				case OP_RegisterAccount: {
#if !defined(LOGINCRYPTO) || defined(MINILOGIN) | defined(PUBLICLOGIN)
					FatalError(ErrorRegUnavail);
					break;
#else
					break; // Just Temporary (by Hogie)
					uchar tmp[1660];
					eq_crypto.DoEQDecrypt(app->pBuffer, tmp, 1660);
					Registration_Struct* rs = (Registration_Struct*) tmp;
					if (!database.AddLoginAccount(rs->StationName, rs->Password, rs->ChatHandel, rs->AccountKey, rs->EMail))
						FatalError("Error error: Unable to Add Account!");
					else
						FatalError("Error: Account Creation Successful");
					break;
#endif
				}
#endif // MINILOGIN
				case OP_AllFinish: {
					//Bye bye
					break;
				}
				default: {
					cout << Timer::GetCurrentTime() << " Received unknown opcode: 0x" << hex << setfill('0') << setw(4) << app->opcode << dec;
					cout << " size:" << app->size << endl;
//					DumpPacket(app->pBuffer, app->size);
				}
			}
			delete app;
		}
	}

	if (!eqnc->CheckActive()) {
#ifdef CLIENT_JOINPART
		cout << "Client disconnected (eqnc->GetState()=" << (int) eqnc->GetState() << ")" << endl;
#endif
		return false;
	}

	return true;
}

void Client::QueuePacket(APPLAYER* app) {
#ifdef cout_ALL_PACKETS
if (app) {
cout << " Sent opcode: 0x" << hex << setfill('0') << setw(4) << app->opcode << dec << " size:" << app->size << endl;
if (app->size > 0) DumpPacket(app);
}
#endif
	eqnc->QueuePacket(app);
}

void Client::FastQueuePacket(APPLAYER** app) {
#ifdef cout_ALL_PACKETS
if (*app) {
cout << " Sent opcode: 0x" << hex << setfill('0') << setw(4) << (*app)->opcode << dec << " size:" << (*app)->size << endl;
if ((*app)->size > 0) DumpPacket(*app);
}
#endif
	eqnc->FastQueuePacket(app);
}

void Client::WorldResponse(int32 worldid,sint8 response)
{
	LWorld* world = world_list.FindByID(worldid);

	char msg[200];


	if(world == 0) {
		FatalError(WorldDownErrorMessage);
		return;
	}

	switch(response){
		case 0: {
			FatalError(WorldLockedErrorMessage);
			break;
		}
		case -1: {
			snprintf(msg, sizeof(msg),"You have been suspended from the world server %s, please try another server.",world->GetName());
			break;
		}
		case -2: {
			snprintf(msg, sizeof(msg),"You have been banned from the world server %s, please try another server.",world->GetName());
			break;
		}
		case -3: {
			snprintf(msg, sizeof(msg),"The world server %s is full and you do not have a high enough status to bypass the limit, please try again later.",world->GetName());
			break;
		}
	}

	if(response <= -1)
		FatalError(msg);

	if(response <= 0)
		return;


	// Make a 15 char random key out of printable characters
	char key[16]; memset(key, 0, sizeof(key));
	int8 y;
	for (int x = 0; x < 15; x++) {
		y = (rand() % 62) + 48;
		if (y > 57) y = y + 7;
		if (y > 90) y = y + 6;
		key[x] = y;
	}
#ifdef _DEBUG
	cout << "Issueing key: LS#" << this->account_id << ", Key=" << key << endl;
#endif

	// Send the accountid, username and key to the worldserver
	ServerPacket* pack = new ServerPacket(ServerOP_LSClientAuth, sizeof(ServerLSClientAuth));
	ServerLSClientAuth* slsca = (ServerLSClientAuth*) pack->pBuffer;
	slsca->lsaccount_id = account_id;
	strcpy(slsca->name, account_name);
	strncpy(slsca->key, key, 15);
	slsca->lsadmin = lsadmin;
	slsca->worldadmin = worldadmin;
	world->SendPacket(pack);
	delete pack;

	// Send the username and key to the client
	APPLAYER *outapp;
	outapp = new APPLAYER(OP_SessionKey, 17);
	// this is submitted to world server as "password"
	strncpy((char*) &outapp->pBuffer[1], key, 15);
	QueuePacket(outapp);
	delete outapp;
}

void Client::FatalError(const char* message, bool disconnect) {
	APPLAYER *outapp;
	outapp = new APPLAYER;
	outapp->opcode = OP_FatalError;
	if (strlen(message) > 1) {
		outapp->size = 11 + strlen(message) + 1;
		outapp->pBuffer = new uchar[outapp->size];
		strncpy((char*) outapp->pBuffer, "Error: ", 11);
		strcpy((char*) &outapp->pBuffer[11], message);
	}
	else {
		outapp->size = strlen("Unknown Error") + 1;
		outapp->pBuffer = new uchar[outapp->size];
		strcpy((char*) outapp->pBuffer, "Unknown Error");
	}
	QueuePacket(outapp);
	delete outapp;
	if (disconnect) {
// ok, found out the client always disconnects itself on OP_FatalError, if we disconnect too the error messages get funky
// so just kill the client's ability to do anything and wait for disconnect
		kicked = true;
//		packet_manager.Close();
	}
}

void ClientList::Add(Client* client) {
	list.Insert(client);
}

Client* ClientList::Get(int32 ip, int16 port) {
	LinkedListIterator<Client*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetIP() == ip && iterator.GetData()->GetPort() == port)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}

Client* ClientList::FindByLSID(int32 lsaccountid) {
	LinkedListIterator<Client*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetAccountID() == lsaccountid)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}

void ClientList::Process() {
	LinkedListIterator<Client*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (!iterator.GetData()->Process()) {
#ifdef CLIENT_JOINPART
			struct in_addr  in;
			in.s_addr = iterator.GetData()->GetIP();
			cout << "Removing client from ip: " << inet_ntoa(in) << ", port: " << iterator.GetData()->GetPort() << endl;
#endif
			iterator.RemoveCurrent();
		}
		else {
			iterator.Advance();
		}
	}
}

#ifndef MINILOGIN
int32 Client::CheckLoginInfo(int8* auth, int32 ip) {
	int32 tmp = database.GetLSAuthentication(auth);
	if (tmp == 0) {
		struct in_addr  in;
		in.s_addr = ip;
		tmp = database.GetLSAuthChange(inet_ntoa(in));
		if (tmp == 0) {
			return 0;
		}
		else {
			database.UpdateLSAccountAuth(tmp, auth);
			database.RemoveLSAuthChange(tmp);
			return tmp;
		}
	}
	else {
		return tmp;
	}
}
#endif
