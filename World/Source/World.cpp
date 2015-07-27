// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>

#include <signal.h>
#include "queue.h"
#include "timer.h"
#include "EQPacket.h"
#include "EQPacketManager.h"
#include "servertalk.h"
#include "client.h"
#include "clientlist.h"
#include "database.h"
#include "seperator.h"
#include "zoneserver.h"
#include "console.h"
#include "LoginServer.h"
#include "ZSList.h"
#include "EQCUtils.hpp"
#include "World.h"
#include "WorldGuildManager.h"
#include "GuildDatabase.h"
#include "SharedMemory.hpp"
#include "BoatsManager.h"
#include "TimeOfDay.h"

using namespace std;


volatile bool RunLoops = true;
extern volatile bool ZoneLoopRunning;
extern volatile bool ConsoleLoopRunning;
extern volatile bool LoginLoopRunning;
extern volatile bool BoatLoopRunning;
extern EQC::Common::GuildDatabase gb;
extern int8 zoneStatusList[TOTAL_NUMBER_OF_ZONES];

HINSTANCE	SharedMemory::hSharedDLL = NULL;
HANDLE		SharedMemory::hSharedMapObject = NULL;
LPVOID		SharedMemory::lpvSharedMem = NULL;
EQC::World::LoginServer* loginserver;
EQC::World::ClientList client_list;
uint32 numclients = 0;

/*
* World Server Application Entry Point
*/
int main(int argc, char** argv) // This is going to be moved to the WorldServer.cpp file
{
	EQC::Common::PrintF(CP_WORLDSERVER, "EQC Alpha Server - World Version : %d\n", EQC_VERSION);

#ifdef WIN32
#ifdef WIN32_WORLD_SERVERONLY
	if(!EQC::Common::IsRunningOnWindowsServer())
	{
		cout << "EQC World Server requires to be running on a Server Platform." << endl;
		exit(0);
	}
#endif
#endif
	
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	if (signal(SIGINT, CatchSignal) == SIG_ERR)
	{
		EQC::Common::PrintF(CP_WORLDSERVER, "Could not set signal handler\n");
		return 0;
	}

	if (argc >= 2) 
	{
		HandleApplicationArguments(argc, argv);
	}
#ifdef EQC_SHAREDMEMORY
	// -Cofruben: Load Shared Memory.
	EQC::Common::PrintF(CP_SHAREDMEMORY, "Loading Shared Memory Core...");
	SharedMemory::Load();
#else
	EQC::Common::PrintF(CP_WORLDSERVER, "Loading items... ");
	Database::Instance()->LoadItems();
	EQC::Common::PrintF(CP_GENERIC, "Done.\n");
#endif
	// Load Items


	// Load Guilds
	EQC::Common::PrintF(CP_WORLDSERVER, "Loading guild ranks...");

	guild_mgr.LoadGuilds();

	EQC::Common::PrintF(CP_GENERIC, "Done.\n");

	// Load Boats

	EQC::Common::PrintF(CP_WORLDSERVER, "Loading boats data...");

	EQC::World::BoatsManager* boat_manager = EQC::World::BoatsManager::getInst();

	EQC::Common::PrintF(CP_GENERIC, "Done.\n");

	// Start World Network layer
	EQC::Common::PrintF(CP_WORLDSERVER, "Starting world network...");
	InitTCP();
	net.Init();
	EQC::Common::PrintF(CP_GENERIC, "Done.\n");

	//Yeahlight: Empty active_accounts table
	EQC::Common::PrintF(CP_WORLDSERVER, "Truncating active_accounts table...");
	Database::Instance()->TruncateActiveAccounts();
	EQC::Common::PrintF(CP_GENERIC, "Done.\n");

	char* WorldAddress = net.GetWorldAddress();
	
	EQC::Common::PrintF(CP_WORLDSERVER, "World server listening on Address : '%s' and port : %d\n", WorldAddress, WORLD_PORT);

#ifdef WIN32
	_beginthread(ConsoleLoop, 0, NULL);
	_beginthread(ZoneServerLoop, 0, NULL);
#else
	pthread_t thread1, thread2;
	pthread_create(&thread1, NULL, &ConsoleLoop, NULL);
	pthread_create(&thread2, NULL, &ZoneServerLoop, NULL);
#endif

	// does MySQL pings and auto-reconnect
	Timer InterserverTimer(INTERSERVER_TIMER); 
	InterserverTimer.Trigger();
	//Yeahlight: Mass world kick timer
	Timer PurgeStuckAccountsTimer(WORLD_KICK_TIMER); 
	PurgeStuckAccountsTimer.Trigger();
	//Yeahlight: Mass PC corpse rot update timer
	Timer playerCorpseDecay_timer(PC_CORPSE_INCREMENT_DECAY_TIMER);
	playerCorpseDecay_timer.Trigger();
	//Yeahlight: Zone status variable and timer
	for(int i = 1; i < TOTAL_NUMBER_OF_ZONES; i++)
		zoneStatusList[i] = 1;
	Timer zoneStatus_timer(ZONE_STATUS_CHECK_DELAY * 2);
	zoneStatus_timer.Trigger();

	boat_manager->StartThread();

	while(RunLoops)
	{
		Timer::SetCurrentTime();
		EQC::World::TimeOfDay::Instance()->Process();
		net.ListenNewClients();

		if(InterserverTimer.Check()) 
		{
			InterserverTimer.Start();
			Database::Instance()->PingMySQL();

			if(net.LoginServerInfo && loginserver == 0) 
			{
#ifdef WIN32

				_beginthread(AutoInitLoginServer, 0, NULL);
#else
				pthread_t thread;
				pthread_create(&thread, NULL, &AutoInitLoginServer, NULL);
#endif
			}
		}

		//Yeahlight: Mass world kick request on stuck accounts
		if(PurgeStuckAccountsTimer.Check())
		{
			Database::Instance()->PurgeStuckAccounts();
		}

		//Yeahlight: Decrement the PC corpses of the world in bulk
		if(playerCorpseDecay_timer.Check())
		{
			queue<int32> rotList;
			int32 accountid;
			rotList = Database::Instance()->GetCorpseAccounts();
			while(rotList.empty() == false)
			{
				accountid = rotList.front();
				Database::Instance()->DecrementCorpseRotTimer(accountid);
				rotList.pop();
			}
		}

		//Yeahlight: Time to check the status of each zone
		if(CHECKING_ZONE_SERVER_STATUS && zoneStatus_timer.Check())
		{
			bool zoneFailure = false;
			//Yeahlight: Iterate through the zone status array looking for any zone ID with a value of 0
			for(int i = 1; i < TOTAL_NUMBER_OF_ZONES; i++)
			{
				//Yeahlight: This zone is not responding
				if(zoneStatusList[i] == 0)
				{
					char name[16] = "";
					if(Database::Instance()->GetZoneShortName(i, name))
					{
						if(zoneFailure == false)
						{
							cout<<"*************************************"<<endl;
							zoneFailure = true;
						}
						cout<<"Zone \'"<<name<<"\' is no longer responding!"<<endl;
						//Yeahlight: TODO: Kill the existing instance of the zone (if any) and boot up another version from an available sleeping zone
					}
				}
			}
			if(zoneFailure)
			{
				cout<<"*************************************"<<endl;
			}
			//Yeahlight: Clear the zone status array
			memset(zoneStatusList, 0, TOTAL_NUMBER_OF_ZONES);
		}

		if(numclients == 0)
		{
			Sleep(10);
			continue;
		}
	 
		client_list.Process();

		Sleep(1);
	}
	while(ZoneLoopRunning || ConsoleLoopRunning || LoginLoopRunning)
	{
		Sleep(1);
	}

	//Tazadar we delete boat data
	EQC::World::BoatsManager::KillInst();

#ifdef EQC_SHAREDMEMORY	
	// -Cofruben: Unload Shared Memory.
	SharedMemory::Unload();
#endif
	return 0;
}





void CatchSignal(int sig_num)
{
	EQC::Common::PrintF(CP_WORLDSERVER, "Get signal : '%d'\n", sig_num);
	RunLoops = false;
}

void HandleApplicationArguments(int argc, char** argv)
{
	char tmp[2];

	if (strcasecmp(argv[1], "help") == 0 || strcasecmp(argv[1], "?") == 0 || strcasecmp(argv[1], "/?") == 0 || strcasecmp(argv[1], "-?") == 0 || strcasecmp(argv[1], "-h") == 0 || strcasecmp(argv[1], "-help") == 0) {
		cout << "Worldserver command line commands:" << endl;
		cout << "adduser username password flag    - adds a user account" << endl;
		cout << "flag username flag    - sets GM flag on the account" << endl;
		cout << "startzone zoneshortname    - sets the starting zone" << endl;
		exit(0);
	}
	else if (Database::Instance()->GetVariable("disablecommandline", tmp, 2)) 
	{
		if (strlen(tmp) == 1) 
		{
			if (tmp[0] == '1') 
			{
				cout << "Command line disabled in database... exiting" << endl;
				exit(0);
			}
		}
	}

	if (strcasecmp(argv[1], "adduser") == 0) 
	{
		if (argc == 5) 
		{
			if (Seperator::IsNumber(argv[4])) 
			{
				if (atoi(argv[4]) >= 0 && atoi(argv[4]) <= 255) 
				{
					if (Database::Instance()->CreateAccount(argv[2], argv[3], atoi(argv[4])) == 0)
					{
						cout << "database.CreateAccount failed." << endl;
					}
					else
					{
						cout << "Account created: Username='" << argv[2] << "', Password='" << argv[3] << ", status=" << argv[4] << endl;
					}
					exit(0);
				}
			}
		}
		cout << "Usage: world adduser username password flag" << endl;
		cout << "flag = 0, 1 or 2" << endl;
		exit(0);
	}
	else if (strcasecmp(argv[1], "flag") == 0) 
	{
		if (argc == 4) 
		{
			if (Seperator::IsNumber(argv[3])) 
			{
				if (atoi(argv[3]) >= 0 && atoi(argv[3]) <= 255) 
				{
					if (Database::Instance()->SetGMFlag(argv[2], atoi(argv[3])))
					{
						cout << "Account flagged: Username='" << argv[2] << "', status=" << argv[3] << endl;
					}
					else
					{
						cout << "database.SetGMFlag failed." << endl;
					}
					exit(0);
				}
			}
		}
		cout << "Usage: world flag username flag" << endl;
		cout << "flag = 0-9" << endl;
		exit(0);
	}
	else if (strcasecmp(argv[1], "startzone") == 0) 
	{
		if (argc == 3) 
		{
			if (strlen(argv[2]) < 3) 
			{
				cout << "Error: zone name too short" << endl;
			}
			else if (strlen(argv[2]) > 15) 
			{
				cout << "Error: zone name too long" << endl;
			}
			else 
			{
				if (Database::Instance()->SetVariable("startzone", argv[2]))
				{
					cout << "Starting zone changed: '" << argv[2] << "'" << endl;
				}
				else
				{
					cout << "database.SetVariable failed." << endl;
				}
			}
			exit(0);
		}
		cout << "Usage: world startzone zoneshortname" << endl;
		exit(0);
	}
	cout << "Error, unknown command line option" << endl;
	exit(0);
}
