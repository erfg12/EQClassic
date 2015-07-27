//#include "quests.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <signal.h>
#include <ctime>

#include "queue.h"
#include "timer.h"
#include "EQPacket.h"
#include "EQPacketManager.h"
#include "eq_packet_structs.h"
#include "Mutex.h"
#include "net.h"
#include "client.h"
#include "zone.h"
#include "worldserver.h"

#include "ClientList.h"
#include "EQCUtils.hpp"
#include "GuildDatabase.h"
#include "ZoneGuildManager.h"
#include "SpellsHandler.hpp"
#include "SharedMemory.hpp"

#include <process.h>

using namespace EQC::Common;
using namespace EQC::Zone;

#include "spdat.h"

#include "ZoneGuildManager.h"

using namespace std;

NetConnection			net;
extern SpellsHandler	spells_handler;
extern EntityList		entity_list;
WorldServer				worldserver;
ZoneGuildManager		zgm;
int32					numclients = 0;
extern Zone* 			zone;
extern volatile bool 	ZoneLoaded;
extern ClientList		client_list;
npcDecayTimes_Struct 	npcCorpseDecayTimes[100];
volatile bool 			RunLoops = true;
bool 					ProcessLoopRunning = false;
extern bool 			WorldLoopRunning;
HINSTANCE				SharedMemory::hSharedDLL = NULL;
HANDLE					SharedMemory::hSharedMapObject = NULL;
LPVOID					SharedMemory::lpvSharedMem = NULL;

void Shutdown();
void LoadSPDat();

Mutex MNetLoop;





#define SPDat_Location	"spdat.eff"




int main(int argc, char** argv)
{

#ifdef WIN32
#ifdef WIN32_WORLD_SERVERONLY
	if(!EQC::Common::IsRunningOnWindowsServer())
	{
		cout << "EQC Zone Server requires to be running on a Server Platform." << endl;
		exit(0);
	}
#endif
#endif

#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	if (argc != 5)
	{
		cerr << "Usage: zone zone_name address port worldaddress" << endl;
		exit(0);
	}
	
	char* zone_name = argv[1];
	char* address = argv[2];
	int32 port = atoi(argv[3]);
	char* worldaddress = argv[4];

	if (strlen(address) <= 0)
	{
		cerr << "Invalid address" << endl;
		exit(0);
	}
	if (port <= 0)
	{
		cerr << "Bad port specified" << endl;
		exit(0);
	}
	if (strlen(worldaddress) <= 0)
	{
		cerr << "Invalid worldaddress" << endl;
		exit(0);
	}
	if (signal(SIGINT, CatchSignal) == SIG_ERR)
	{
		cerr << "Could not set signal handler" << endl;
		return 0;
	}
	net.SaveInfo(address, port, worldaddress);

#ifdef EQC_SHAREDMEMORY
	// -Cofruben: Load Shared Memory
	EQC::Common::PrintF(CP_SHAREDMEMORY, "Loading Shared Memory Core...");
	SharedMemory::Load();
	EQC::Common::PrintF(CP_GENERIC, "Done.\n");
#else
	EQC::Common::PrintF(CP_ZONESERVER, "Loading items...\n");
	Database::Instance()->LoadItems();
#endif


	EQC::Common::PrintF(CP_SPELL, "Loading SP DAT...\n");
	spells_handler.LoadSpells(SPDat_Location);

	EQC::Common::PrintF(CP_ZONESERVER, "Loading guild and guild ranks...\n");
	zgm.LoadGuilds();

	EQC::Common::PrintF(CP_ZONESERVER, "Loading faction data...\n");
	Database::Instance()->LoadFactionData();

	EQC::Common::PrintF(CP_ZONESERVER, "Loading faction lists...\n");
	Database::Instance()->LoadNPCFactionLists();

	EQC::Common::PrintF(CP_ZONESERVER, "Loading Corpse decay Timers...\n");
	Database::Instance()->GetDecayTimes(npcCorpseDecayTimes);

	if (!worldserver.Init()) {
		cerr << "InitWorldServer failed" << endl;
	}

	//Yeahlight: Static zone support
	if(strcmp(zone_name, ".") == 0)
	{
		EQC::Common::PrintF(CP_ZONESERVER, "Entering sleep mode.\n");
	}
	else if(!Zone::Bootup(zone_name))
	{
		EQC::Common::PrintF(CP_ZONESERVER, "%s boot FAILED!\n", zone_name);
	}
	else
	{
		EQC::Common::PrintF(CP_ZONESERVER, "%s boot was successful.\n", zone_name);
	}

	Timer InterserverTimer(INTERSERVER_TIMER); // does MySQL pings and auto-reconnect
	net.StartProcessLoop();
	MNetLoop.lock();

	while(RunLoops)
	{
		Timer::SetCurrentTime();
		if (ZoneLoaded) {
			net.ListenNewClients();
			client_list.SendPacketQueues();
		}
		if (InterserverTimer.Check()) {
			InterserverTimer.Start();
			Database::Instance()->PingMySQL();
			entity_list.UpdateWho();
		}
		MNetLoop.unlock();
		Sleep(1);
		MNetLoop.lock();
	}
	MNetLoop.unlock();
	if (zone != 0)
		Zone::Shutdown(true);
	while (WorldLoopRunning || ProcessLoopRunning) {
		Sleep(1);
	}
#ifdef EQC_SHAREDMEMORY
	// -Cofruben: Unload Shared Memory.
	SharedMemory::Unload();
#endif
	return 0;
}

void CatchSignal(int sig_num)
{

	EQC::Common::PrintF(CP_ZONESERVER, "Got signal %i\n", sig_num);
	exit(0);
	//RunLoops = false;
}

void Shutdown()
{
	Zone::Shutdown(true);
	MNetLoop.lock();
	RunLoops = false;
	worldserver.Disconnect();
	MNetLoop.unlock();
	//	safe_delete(worldserver);
	EQC::Common::PrintF(CP_ZONESERVER, "Shutting down...\n");
}



bool NetConnection::Init()
{
	struct sockaddr_in address;
	int reuse_addr = 1;

#ifdef WIN32
	unsigned long nonblocking = 1;
	WORD version = MAKEWORD (1,1);
	WSADATA wsadata;
#endif

	/* Setup internet address information.  
	This is used with the bind() call */
	memset((char *) &address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(ZonePort);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Setting up UDP port for new clients */
	listening_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (listening_socket < 0) 
	{

		return false;
	}

	//	setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

	if (bind(listening_socket, (struct sockaddr *) &address, sizeof(address)) < 0) 
	{
		EQC::Common::CloseSocket(listening_socket);

		return false;
	}

#ifdef WIN32
	ioctlsocket (listening_socket, FIONBIO, &nonblocking);
#else
	fcntl(listening_socket, F_SETFL, O_NONBLOCK);
#endif


	return true;
}


void NetConnection::ListenNewClients()
{

	uchar		buffer[1024];

	int			status;
	unsigned short	port;

	struct sockaddr_in	from;
	struct in_addr	in;
	unsigned int	fromlen;

	from.sin_family = AF_INET;
	fromlen = sizeof(from);

#ifdef WIN32
	status = recvfrom(listening_socket, (char *) &buffer, sizeof(buffer), 0,(struct sockaddr*) &from, (int *) &fromlen);
#else
	status = recvfrom(listening_socket, &buffer, sizeof(buffer), 0,(struct sockaddr*) &from, &fromlen);
#endif


	if (status > 1)
	{
		Client* client = 0;

		port = from.sin_port;
		in.s_addr = from.sin_addr.s_addr;

		//cout << Timer::GetCurrentTime() << " Data from ip: " << inet_ntoa(in) << " port:" << ntohs(port) << " length: " << status << endl;
		if (!client_list.RecvData(in.s_addr, from.sin_port, buffer, status)) {
			// If it is a new client make sure it has the starting flag set. Ignore otherwise
			if (buffer[0] & 0x20)
			{
				EQC::Common::PrintF(CP_CLIENT, "New client from %s:%i\n", inet_ntoa(in), ntohs(port));
				client = new Client(in.s_addr, port, listening_socket);
				client->ReceiveData(buffer, status);
				entity_list.AddClient(client);
				client_list.Add(client);
				numclients++;
			}
			else
			{
				return;
			}
		}
	}
}

int32 NetConnection::GetIP()
{
	char     name[255+1];
	size_t   len = 0;
	hostent* host = 0;

	if (gethostname(name, len) < 0 || len <= 0)
	{
		return 0;
	}

	host = gethostbyname(name);
	if (host == 0)
	{
		return 0;
	}

	return inet_addr(host->h_addr);
}

int32 NetConnection::GetIP(char* name)
{
	hostent* host = 0;

	host = gethostbyname(name);
	if (host == 0)
	{
		return 0;
	}

	return inet_addr(host->h_addr);

}

void NetConnection::SaveInfo(char* address, int32 port, char* waddress) {
	ZoneAddress = new char[strlen(address)+1];
	strcpy(ZoneAddress, address);
	ZonePort = port;
	WorldAddress = new char[strlen(waddress)+1];
	strcpy(WorldAddress, waddress);
}

NetConnection::~NetConnection() {
	if (ZoneAddress != 0)
		safe_delete(ZoneAddress);//delete ZoneAddress;
	if (WorldAddress != 0)
		safe_delete(WorldAddress);//delete WorldAddress;
}

void NetConnection::Close()
{
	EQC::Common::CloseSocket(listening_socket);
}

void NetConnection::StartProcessLoop() {
#ifdef WIN32
	_beginthread(ProcessLoop, 0, NULL);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, ProcessLoop, NULL);
#endif
}

// this should always be called in a new thread
#ifdef WIN32
void ProcessLoop(void *tmp) {
#else
void *ProcessLoop(void *tmp) {
#endif
	srand(time(NULL));
	bool worldwasconnected = worldserver.Connected();
	ProcessLoopRunning = true;
	while(RunLoops) {
		if (worldserver.Connected()) {
			worldserver.Process();
			worldwasconnected = true;
		}
		else {
			if (worldwasconnected && ZoneLoaded)
				entity_list.ChannelMessageFromWorld(0, 0, 6, 0, 0, "WARNING: World server connection lost");
			worldwasconnected = false;
		}
		if (ZoneLoaded) {
			if (numclients > 0) // Don't run entity_list Process() unless there are clients inside.
				entity_list.Process();
			else
				entity_list.BoatProcessOnly(); // Tazadar : We move boats even if the zone is empty !
			if (!zone->Process()) {
				Zone::Shutdown();
			}
		}
		Sleep(1);
	}
	ProcessLoopRunning = false;
#ifndef WIN32
	return 0;
#endif
}
