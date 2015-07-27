#include "../common/debug.h"

#include <iostream>
#include <string.h>
#include <time.h>
#include <signal.h>

using namespace std;

#include "../common/queue.h"
#include "../common/timer.h"

#include "../common/EQNetwork.h"
#include "../common/seperator.h"

#include "net.h"
#include "client.h"
#include "../common/database.h"
#include "LWorld.h"
#include "../common/packet_functions.h"

#ifdef WIN32
	#define snprintf	_snprintf
	//#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
	#include <conio.h>
#else
	#include <stdlib.h>
	#include "../common/unix.h"
#endif

#if defined(LOGINCRYPTO) && !defined(MINILOGIN) && !defined(PUBLICLOGIN)
	#include "../Login/EQCrypto.h"
	extern EQCrypto	eq_crypto;
#endif

#ifndef MINILOGIN
	Database database;
#endif
EQNetworkServer eqns(5999);
NetConnection net;
ClientList client_list;
LWorldList world_list;
volatile bool RunLoops = true;
bool WriteXML_enabled = false;

bool ReadLoginConfig();

#ifdef PUBLICLOGIN
char version[200], consoletitle[200];
#endif

int main(int argc, char** argv)
{
#ifdef PUBLICLOGIN
				sprintf(version,"v1.01P");
				sprintf(consoletitle,"EQEmu Public Login %s",version);
				SetConsoleTitle(consoletitle);
				printf("============================\n");
				printf("EQEmu Login %s\n",version);
				printf("http://eqemu.sourceforge.net\n");
				printf("============================\n");
#endif

#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	if (signal(SIGINT, CatchSignal) == SIG_ERR) {
		cerr << "Could not set signal handler" << endl;
	}
	srand(time(NULL));

/*	if (argc >= 2) {
		if (strcasecmp(argv[1], "port") == 0) {
			if (argc == 3 && atoi(argv[2]) > 0 && atoi(argv[2]) < 0xFFFF) {
				net.SetPort(atoi(argv[2]));
			}
			else {
				cout << "Usage: Login port <number>" << endl;
				return 0;
			}
		}
		else {
			cout << "Error: Unknown command line option" << endl;
			return 0;
		}
	}*/
	net.ReadLoginConfig();
#ifdef MINILOGIN
	net.ReadMiniLoginAccounts();
#endif

#ifndef MINILOGIN
	database.LoadVariables();
//	if ((net.GetLoginMode() == Master) || (net.GetLoginMode() == Standalone)) {
//		database.ClearLSAuthChange();
//	}
#endif

	if (net.GetLoginMode() == Standalone) {
		cout << "Server mode: Standalone" << endl;
	}
	else if (net.GetLoginMode() == Master) {
		cout << "Server mode: Master" << endl;
	}
	else if (net.GetLoginMode() == Slave) {
		cout << "Server mode: Slave" << endl;
	}
	else if (net.GetLoginMode() == Mesh) {
		cout << "Server mode: Mesh" << endl;
	}
	else {
		cout << "Server mode: Unknown" << endl;
		return 1;
	}

	if (net.GetLoginMode() == Slave) {
		world_list.ConnectUplink();
	}
	world_list.Init();
	if (eqns.Open(net.GetPort())) {
		cout << "Login server listening on port:" << net.GetPort() << endl;
	}
	else {
		cout << "EQNetworkServer.Open() error" << endl;
		return 1;
	}

	Timer InterserverTimer(INTERSERVER_TIMER); // does MySQL pings and auto-reconnect
	EQNetworkConnection* eqnc;
	while(RunLoops) {
		Timer::SetCurrentTime();
		while (eqnc = eqns.NewQueuePop()) {
#ifdef CLIENT_JOINPART
			struct in_addr	in;
			in.s_addr = eqnc->GetrIP();
			cout << Timer::GetCurrentTime() << " New client from ip: " << inet_ntoa(in) << " port: " << ntohs(eqnc->GetrPort()) << endl;
#endif
			Client* client = new Client(eqnc);
			client_list.Add(client);
		}
		client_list.Process();
		world_list.Process();
#ifdef WIN32
		if(kbhit())
		{
			int hitkey = getch();
			net.HitKey(hitkey);
		}
#endif
		if (InterserverTimer.Check()) {
			InterserverTimer.Start();
			if (net.GetLoginMode() == Slave) {
				world_list.ConnectUplink();
			}
#ifndef MINILOGIN
			database.ping();
			database.LoadVariables();
#ifndef PUBLICLOGIN
			if (WriteXML_enabled) {
//				WriteXML_enabled = 
				world_list.WriteXML();
			}
#endif
#endif
		}
		Sleep(1);
	}
	world_list.Shutdown();
	return 0;
}
#ifdef WIN32
void NetConnection::HitKey(int keyhit)
{
	switch(keyhit)
	{
	case 'l':
	case 'L': {
		world_list.ListWorldsToConsole();
		break;
	}
#ifdef PUBLICLOGIN
	case 118: {
		printf("============================\n");
		printf("EQEmu Login: %s\n",version);
		printf("============================\n");
		break;
	}
#endif
// Quagmire: Cannot use GetNextID() here.
/*	case 48: {
		printf("Kick All World Servers (0 = No, 1 = Yes)\n");
		printf("Awaiting response:");
		int result = getch();
		printf("\n");
		int k = world_list.GetNextID();
		if(result==49) {
			for(int i=0; i < k; i++) {
				LWorld* world = world_list.FindByID(i);
				if(world != 0) {
					printf("Server: %s has been kicked.\n",world->GetName());
					world->Kick("Login server has kicked all world servers.");
				}
			}
		}
		break;
	}*/
	case 'H':
	case 'h': {
		printf("============================\n");
		printf("Available Commands:\n");
		printf("l = Listing of World Servers\n");
		printf("v = Login Version\n");
//		printf("0 = Kick all connected world servers\n");
		printf("============================\n");
		break;
	}
	default:
		printf("Invalid Command.\n");
		break;
	}
}
#endif
	
void CatchSignal(int sig_num) {
	cout << "Got signal " << sig_num << endl;
	RunLoops = false;
}

bool NetConnection::ReadLoginConfig() {
	char buf[201], type[201];
	int8 items[5] = {0, 0, 0, 0, 0};
	FILE *f;

	if (!(f = fopen ("LoginServer.ini", "r"))) {
		printf ("Couldn't open the LoginServer.ini file.\n");
		return false;
	}
	do {
		fgets (buf, 200, f);
		if (feof (f))
		{
#ifndef PUBLICLOGIN
			printf ("[LoginConfig] block not found in LoginServer.ini.\n");
#endif
			fclose (f);
			return false;
		}
	}
	while (strncasecmp (buf, "[LoginConfig]\n", 14) != 0 && strncasecmp (buf, "[LoginConfig]\r\n", 15) != 0);

	while (!feof (f))
	{
#ifdef WIN32
		if (fscanf (f, "%[^=]=%[^\n]\r\n", type, buf) == 2)
#else
		if (fscanf (f, "%[^=]=%[^\r\n]\n", type, buf) == 2)
#endif
		{
#if defined(PUBLICLOGIN) || defined(MINILOGIN)
items[0] = 1;
LoginMode = Standalone;
items[1] = 1;
items[2] = 1;
items[3] = 1;
#else
			if (!strncasecmp (type, "servermode", 10)) {
				items[0] = 1;
				if (strcasecmp(buf, "Standalone") == 0) {
					LoginMode = Standalone;
				}
				else if (strcasecmp(buf, "Master") == 0) {
					LoginMode = Master;
				}
				else if (strcasecmp(buf, "Slave") == 0) {
					LoginMode = Slave;
				}
				else if (strcasecmp(buf, "Mesh") == 0) {
					LoginMode = Mesh;
				}
				else {
					items[0] = 0;
				}
			}
			if (!strncasecmp (type, "uplinkpeer", 10)) {
				if (net.GetLoginMode() == Mesh) {
					world_list.AddPeer(buf, true);
					items[4] = 1;
				}
				else {
					cout << "Error: 'uplinkpeer' found in LoginServer.ini before mode set to Mesh." << endl;
					cout << "       Make sure your 'ServerMode' line is the first in the [LoginConfig] block" << endl;
				}
			}
			if (!strncasecmp (type, "uplinkaddress", 13)) {
				memset(masteraddress, 0, sizeof(masteraddress));
				strncpy(masteraddress, buf, sizeof(masteraddress) - 1);
				items[1] = 1;
			}
			if (!strncasecmp (type, "uplinkport", 10)) {
				if (Seperator::IsNumber(buf) && atoi(buf) > 0 && atoi(buf) < 0xFFFF) {
					uplinkport = atoi(buf);
				}
			}
			if (!strncasecmp (type, "uplinkaccount", 13)) {
				memset(uplinkaccount, 0, sizeof(uplinkaccount));
				strncpy(uplinkaccount, buf, sizeof(uplinkaccount) - 1);
				items[2] = 1;
			}
			if (!strncasecmp (type, "uplinkpassword", 14)) {
				memset(uplinkpassword, 0, sizeof(uplinkpassword));
				strncpy(uplinkpassword, buf, sizeof(uplinkpassword) - 1);
				items[3] = 1;
			}
			if (!strncasecmp (type, "enablestats", 11)) {
				WriteXML_enabled = atobool(buf);
				if (WriteXML_enabled)
					cout << "Stats writing enabled" << endl;
			}
#endif
			if (!strncasecmp(type, "serverport", 10)) {
				if (Seperator::IsNumber(buf) && atoi(buf) > 0 && atoi(buf) < 0xFFFF) {
					port = atoi(buf);
				}
			}
		}
	}

	fclose (f);
	if (!items[0] || (LoginMode == Slave && !(items[1] && items[2] && items[3])) || (items[4] && !(items[2] && items[3])))
	{
		cout << "Incomplete LoginServer.INI file." << endl;
		return false;
	}

/*	if (strcasecmp(worldname, "Unnamed server") == 0) {
		cout << "LoginServer.ini: server unnamed, disabling uplink" << endl;
		return false;
	}*/
	
	cout << "LoginServer.ini read." << endl;
	return true;
}

#ifdef MINILOGIN
bool NetConnection::ReadMiniLoginAccounts() {
	char fbuf[10000];
	memset(fbuf, 0, sizeof(fbuf));
	sint32 fsize = 0;
	FILE *fp;

	if (!(fp = fopen ("MiniLoginAccounts.ini", "r"))) {
		printf ("Couldn't open the MiniLoginAccounts.ini file.\n");
		return false;
	}
	fsize = filesize(fp);
	if (fsize <= 0) {
		fclose (fp);
		return false;
	}
	if (fsize >= sizeof(fbuf)) {
		fclose (fp);
		return false;
	}

	fread(fbuf, 1, fsize, fp);

	Seperator Sep(fbuf, 10, MaxMiniLoginAccounts+1, 100, false, 13);

	if (Sep.argnum >= MaxMiniLoginAccounts) {
		cout << "Error in MiniLoginConfig.ini: File is more than " << MaxMiniLoginAccounts << " lines" << endl;
		fclose(fp);
		return false;
	}

	int32 arrayindex = 0;
	for (int i=0; i<Sep.argnum; i++) {
		if (Sep.arg[i][0] == '#')
			continue;
		if (Sep.arg[i][0] == 0)
			continue;
		Seperator LineSep(Sep.arg[i]);

		if (LineSep.argnum != 2) {
			cout << "Error in MiniLoginConfig.ini on line #" << (i+1) << ": Invalid format, should be \'IP username password\'" << endl;
			continue;
		}
		if (strlen(LineSep.arg[1]) >= 20) {
			cout << "Error in MiniLoginConfig.ini on line #" << (i+1) << ": Username too long" << endl;
			continue;
		}
		if (strlen(LineSep.arg[2]) >= 20) {
			cout << "Error in MiniLoginConfig.ini on line #" << (i+1) << ": Password too long" << endl;
			continue;
		}
		int32 tmpIP = ResolveIP(LineSep.arg[0]);
		cout << "test";
		if (tmpIP) {
//cout << LineSep.arg[0] << " " << LineSep.arg[1] << " " << LineSep.arg[2] << endl;
			MiniLoginAccounts[arrayindex].IP = tmpIP;
			strcpy(MiniLoginAccounts[arrayindex].username, LineSep.arg[1]);
			strcpy(MiniLoginAccounts[arrayindex].password, LineSep.arg[2]);
			arrayindex++;
		}
		else {
			cout << "Error in MiniLoginConfig.ini on line #" << (i+1) << ": \'" << LineSep.arg[0] << "\' isnt a recognizable ip or hostname." << endl;
			continue;
		}
	}


	fclose (fp);
	cout << "MiniLoginAccounts.ini read." << endl;
	return true;
}
#endif
