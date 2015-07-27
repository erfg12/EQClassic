/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/* 
 * EQNetwork classes, by Quagmire
*/

#include "../common/debug.h"

#include <iostream>
#include <iomanip>

using namespace std;

#ifdef WIN32
	#include <process.h>
#else
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/socket.h>
#ifdef FREEBSD //Timothy Whitman - January 7, 2003
	#include <netinet/in.h>
#endif
	#include <fcntl.h>
	#include <stdlib.h>
	#include <string.h>
	#include <pthread.h>
	#include "../common/unix.h"
	#define SOCKET_ERROR -1
#endif
#include "EQNetwork.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "../common/MiscFunctions.h"
#include "../common/crc32.h"

#define EQN_DEBUG			0
#define EQN_DEBUG_Error		0
#define EQN_DEBUG_Packet	0
#define EQN_DEBUG_Fragment	0
#define EQN_DEBUG_ACK		0
#define EQN_DEBUG_Unknown	0
#define LOG_PACKETS			0
#define LOG_RAW_PACKETS_OUT	0
#define LOG_RAW_PACKETS_IN	0
//#define PRIORITYTEST

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
type  HI_LOSWAPlong (type x) {return (LO_WORD(a)<<16) | (HIWORD(a)>>16);}  

EQNetworkServer::EQNetworkServer(int16 iPort) {
	RunLoop = false;
	pPort = iPort;
	pOpen = false;
	list = new LinkedList<EQNetworkConnection*>;
#ifdef WIN32
	WORD version = MAKEWORD (1,1);
	WSADATA wsadata;
	WSAStartup (version, &wsadata);
#endif
	sock = 0;
}

EQNetworkServer::~EQNetworkServer() {
	Close();
	RunLoop = false;
	MLoopRunning.lock();
	MLoopRunning.unlock();
#ifdef WIN32
	WSACleanup();
#endif
	delete list;
	while (NewQueue.pop()); // they're deleted with the list, clear this queue so it doesnt try to delete them again
}

bool EQNetworkServer::Open(int16 iPort) {
	LockMutex lock(&MOpen);
	if (iPort && pPort != iPort) {
		if (pOpen)
			return false;
		pPort = iPort;
	}
	if (!RunLoop) {
		RunLoop = true;
#ifdef WIN32
		_beginthread(EQNetworkServerLoop, 0, this);
#else
		pthread_t thread;
		pthread_create(&thread, NULL, &EQNetworkServerLoop, this);
#endif
	}
	if (pOpen) {
		return true;
	}
	else {
		struct sockaddr_in address;
		int reuse_addr = 1;
		int bufsize = 64 * 1024; // 64kbyte send/recieve buffers, up from default of 8k
#ifdef WIN32
		unsigned long nonblocking = 1;
#endif

	  /* Setup internet address information.  
		 This is used with the bind() call */
		memset((char *) &address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(pPort);
		address.sin_addr.s_addr = htonl(INADDR_ANY);

		/* Setting up UDP port for new clients */
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0) {
			return false;
		}

//#ifdef WIN32
//		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse_addr, sizeof(reuse_addr));
		setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*) &bufsize, sizeof(bufsize));
		setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*) &bufsize, sizeof(bufsize));
//#else
//		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
//		setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
//		setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
//#endif

		if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
#ifdef WIN32
			closesocket(sock);
#else
			close(sock);
#endif
			return false;
		}

#ifdef WIN32
		ioctlsocket (sock, FIONBIO, &nonblocking);
#else
		fcntl(sock, F_SETFL, O_NONBLOCK);
#endif
		pOpen = true;
		return true;
	}
}

void EQNetworkServer::Close() {
	SetOpen(false);
	if (sock) {
#ifdef WIN32
		closesocket(sock);
#else
		close(sock);
#endif
	}
	sock = 0;
}

bool EQNetworkServer::IsOpen() {
	MOpen.lock();
	bool ret = pOpen;
	MOpen.unlock();
	return ret;
}

void EQNetworkServer::SetOpen(bool iOpen) {
	MOpen.lock();
	pOpen = iOpen;
	MOpen.unlock();
}

void EQNetworkServer::Process() {
	if (!IsOpen()) {
		if (sock) {
#ifdef WIN32
			closesocket(sock);
#else
			close(sock);
#endif
			sock = 0;
		}
		return;
	}

    uchar		buffer[1518];
	
    int			status;
    struct sockaddr_in	from;
    unsigned int	fromlen;

    from.sin_family = AF_INET;
    fromlen = sizeof(from);

	while (1) {
#ifdef WIN32
		status = recvfrom(sock, (char *) buffer, sizeof(buffer), 0,(struct sockaddr*) &from, (int *) &fromlen);
#else
		status = recvfrom(sock, buffer, sizeof(buffer), 0,(struct sockaddr*) &from, &fromlen);
#endif
		if (status >= 1) {
			RecvData(buffer, status, from.sin_addr.s_addr, from.sin_port);
		}
		else {
			break;
		}
	}

	LinkedListIterator<EQNetworkConnection*> iterator(*list);
	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->IsFree() && (!iterator.GetData()->CheckNetActive())) {
			#if EQN_DEBUG >= 4
				cout << "EQNetwork Connection deleted." << endl;
			#endif
			iterator.RemoveCurrent();
		}
		else {
			iterator.GetData()->Process(sock);
			iterator.Advance();
		}
	}
}

void EQNetworkServer::RecvData(uchar* data, int32 size, int32 irIP, int16 irPort) {
/*
	CHANGE HISTORY

	Version		Author		Date		Comment
	1			Unknown		Unknown		Initial Revision
	2			Joolz		05-Jan-2003	Optimised
	3			Quagmire	05-Feb-2003	Changed so 2 connection objects wouldnt be created for the same ip/port pair, often happened
*/

	// Check for invalid data
	if (!data || size <= 4) return;
	if (CRC32::Generate(data, size-4) != ntohl(*((int32*) &data[size-4]))) {
#if EQN_DEBUG_Error >= 1
		cout << "Incomming Packet failed checksum" << endl;
#endif
		return;
	}

	LinkedListIterator<EQNetworkConnection*> iterator(*list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->IsMine(irIP, irPort)) {
			iterator.GetData()->RecvData(data, size);
			return;
		}
		iterator.Advance();
	}
	if (data[0] & 0x20) { // check for SEQStart
#if EQN_DEBUG >= 4
		cout << "New EQNetwork Connection." << endl;
#endif
		EQNetworkConnection* tmp = new EQNetworkConnection(irIP, irPort);
		tmp->RecvData(data, size);
		(*list).Append(tmp);
		MNewQueue.lock();
		NewQueue.push(tmp);
		MNewQueue.unlock();
		return;
	}
#if EQN_DEBUG >= 4
	struct in_addr	in;
	in.s_addr = irIP;
	cout << "WARNING: Stray packet? " << inet_ntoa(in) << ":" << irPort << endl;
#endif
}

EQNetworkConnection* EQNetworkServer::NewQueuePop() {
	EQNetworkConnection* ret = 0;
	MNewQueue.lock();
	ret = NewQueue.pop();
	MNewQueue.unlock();
	return ret;
}

#ifdef WIN32
	void EQNetworkServerLoop(void* tmp) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#else
	void* EQNetworkServerLoop(void* tmp) {
#endif
	EQNetworkServer* eqns = (EQNetworkServer*) tmp;
	eqns->MLoopRunning.lock();
	while (eqns->RunLoop) {
		eqns->Process();
		Sleep(1);
	}
	eqns->MLoopRunning.unlock();
#ifdef WIN32
	_endthread();
#else
	return 0;
#endif
}

#ifdef WIN32
	void EQNetworkConnectionLoop(void* tmp) {
#else
	void* EQNetworkConnectionLoop(void* tmp) {
#endif
	EQNetworkConnection* eqnc = (EQNetworkConnection*) tmp;
#ifdef _DEBUG
	if (eqnc->ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnectionLoop: eqnc->ConnectionType != Outgoing");
	}
#endif
	eqnc->MLoopRunning.lock();
	while (eqnc->RunLoop) {
		eqnc->DoRecvData();
		eqnc->Process(eqnc->outsock);
		Sleep(1);
	}
	eqnc->MLoopRunning.unlock();
#ifdef WIN32
	_endthread();
#else
	return 0;
#endif
}
	
EQNetworkConnection::EQNetworkConnection(int32 irIP, int16 irPort) {
	ConnectionType = Incomming;
	rIP = irIP;
	rPort = irPort;
	dataflow = 0;
	pState = EQNC_Init;
	pFree = false;
	SACK.dwGSQ = 0;
    dwFragSeq = 0;
	outsock = 0;
	RunLoop = false;

	timeout_timer = new Timer(EQNC_TIMEOUT);
	no_ack_sent_timer = new Timer(500);
    no_ack_sent_timer->Disable();
	keep_alive_timer = new Timer(KA_TIMER);
	datarate_timer = new Timer(100, true);
//	datakeepalive_timer = new Timer(1000);

	SetDataRate(500);
	if (rIP && rPort)
		pState = EQNC_Active;
}

EQNetworkConnection::EQNetworkConnection() {
#ifdef WIN32
	WORD version = MAKEWORD (1,1);
	WSADATA wsadata;
	WSAStartup (version, &wsadata);
#endif
	ConnectionType = Outgoing;
	rIP = 0;
	rPort = 0;
	dataflow = 0;
	pState = EQNC_Init;
	pFree = false;
	SACK.dwGSQ = 0;
    dwFragSeq = 0;
	outsock = 0;
	RunLoop = false;

	timeout_timer = new Timer(EQNC_TIMEOUT);
	no_ack_sent_timer = new Timer(500);
    no_ack_sent_timer->Disable();
	keep_alive_timer = new Timer(KA_TIMER);
	queue_check_timer = new Timer(2000);
	datarate_timer = new Timer(100, true);

	SetDataRate(500);
}

EQNetworkConnection::~EQNetworkConnection() {
	if (ConnectionType == Outgoing) {
		CloseSock();
		RunLoop = false;
		MLoopRunning.lock();
		MLoopRunning.unlock();
#ifdef WIN32
		WSACleanup();
#endif
	}
	InQueue_Struct* iqs = 0;
	while ((iqs = InQueue.pop())) {
		safe_delete(iqs->app);
		delete iqs;
	}
	APPLAYER* app = 0;
	while ((app = OutQueue.pop())) {
		delete app;
	}
	safe_delete(timeout_timer);
	safe_delete(no_ack_sent_timer);
	safe_delete(keep_alive_timer);
	safe_delete(datarate_timer);
//	safe_delete(datakeepalive_timer);
}

int8 EQNetworkConnection::GetState() {
	MStateLock.lock();
	int8 ret = pState;
	MStateLock.unlock();
	return ret;
}

bool EQNetworkConnection::CheckActive() {
	int8 tmp = GetState();
	return (tmp == EQNC_Active);
}

bool EQNetworkConnection::CheckNetActive() {
	int8 tmp = GetState();
	return (tmp == EQNC_Active || tmp == EQNC_Closing || tmp == EQNC_Closed);
}

void EQNetworkConnection::SetState(int8 iState) {
	MStateLock.lock();
	pState = iState;
	MStateLock.unlock();
}

void EQNetworkConnection::Free() {
#ifdef _DEBUG
	if (ConnectionType == Outgoing) {
		ThrowError("EQNetworkConnection::Free() called on an outgoing connection!");
	}
#endif
	if (CheckActive())
		Close();
	pFree = true;
}

APPLAYER* EQNetworkConnection::PopPacket() {
	APPLAYER* ret = 0;
	MOutQueueLock.lock();
	ret = OutQueue.pop();
	MOutQueueLock.unlock();
	return ret;
}

void EQNetworkConnection::OutQueuePush(APPLAYER* app) {
	MOutQueueLock.lock();
	OutQueue.push(app);
	MOutQueueLock.unlock();
}

void EQNetworkConnection::QueuePacket(const APPLAYER* app, bool ackreq) {
	if (!app) {
		ThrowError("EQNetworkConnection::QueuePacket(): app = 0!");
	}
	if (!CheckActive())
		return;
	InQueue_Struct* iqs = new InQueue_Struct;
	iqs->app = app->Copy();
	iqs->ackreq = ackreq;
	MInQueueLock.lock();
	InQueue.push(iqs);
	MInQueueLock.unlock();
}

void EQNetworkConnection::FastQueuePacket(APPLAYER** app, bool ackreq) {
	if (!(*app)) {
		ThrowError("EQNetworkConnection::FastQueuePacket(): *app = 0!");
	}
	if (!CheckActive()) {
		safe_delete(*app);
		return;
	}
	InQueue_Struct* iqs = new InQueue_Struct;
	iqs->app = *app;
	*app = 0;
	iqs->ackreq = ackreq;
	MInQueueLock.lock();
	InQueue.push(iqs);
	MInQueueLock.unlock();
}

void EQNetworkConnection::RemoveData() {
	printf("Flushing Queue!\n");
	MOutQueueLock.lock();
	MInQueueLock.lock();
	OutQueue.clear();
	InQueue.clear();
	MOutQueueLock.unlock();
	MInQueueLock.unlock();

	printf("Queue Flushed!\n");
}

void EQNetworkConnection::Close() {
	if (GetState() != EQNC_Active)
		return;
	SetState(EQNC_Closing);
	InQueue_Struct* iqs = new InQueue_Struct;
	iqs->app = 0;
	iqs->ackreq = true;
	MInQueueLock.lock();
	InQueue.push(iqs);
	MInQueueLock.unlock();
}

bool EQNetworkConnection::OpenSock(char* irAddress, int16 irPort, char* errbuf) {
	if (ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnection::Connect() called on an incoming connection!");
		return false;
	}
	int32 tmp = ResolveIP(irAddress, errbuf);
	if (tmp == 0) {
		return false;
	}
	return OpenSock(tmp, irPort);
}

bool EQNetworkConnection::OpenSock(int32 irIP, int16 irPort) {
	if (ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnection::Connect() called on an incoming connection!");
		return false;
	}
	rIP = irIP;
	rPort = htons(irPort);
	if (!(rIP && rPort))
		return false;
	LockMutex lock(&MStateLock);
	if (pState != EQNC_Init)
		return false;

	LockMutex lock2(&MSocketLock);
	outsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (outsock <= 0) {
		outsock = 0;
		return false;
	}

    struct sockaddr_in	server_sin;
	server_sin.sin_family = AF_INET;
	server_sin.sin_addr.s_addr = rIP;
	server_sin.sin_port = rPort;
#ifdef WIN32
	if (connect(outsock, (SOCKADDR*) &server_sin, sizeof (server_sin)) == SOCKET_ERROR) {
		closesocket(outsock);
		outsock = 0;
		return false;
	}
#else
	if (connect(outsock, (struct sockaddr *) &server_sin, sizeof (server_sin)) == SOCKET_ERROR) {
		close(outsock);
		outsock = 0;
		return false;
	}
#endif
#ifdef WIN32
	unsigned long nonblocking = 1;
	ioctlsocket(outsock, FIONBIO, &nonblocking);
#else
	fcntl(outsock, F_SETFL, O_NONBLOCK);
#endif

	if (!RunLoop) {
		RunLoop = true;
#ifdef WIN32
		_beginthread(EQNetworkConnectionLoop, 0, this);
#else
		pthread_t thread;
		pthread_create(&thread, NULL, &EQNetworkConnectionLoop, this);
#endif
	}
	pState = EQNC_Active;
	return true;
}

void EQNetworkConnection::CloseSock() {
	if (ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnection::Connect() called on an incoming connection!");
		return;
	}
	
	LockMutex lock(&MStateLock);
	if (pState == EQNC_Init)
		return;

	InQueue_Struct* iqs = 0;
	while ((iqs = InQueue.pop())) {
		safe_delete(iqs->app);
		delete iqs;
	}
	APPLAYER* app = 0;
	while ((app = OutQueue.pop())) {
		delete app;
	}
	LockMutex lock2(&MSocketLock);
	if (outsock) {
#ifdef WIN32
		closesocket(outsock);
#else
		close(outsock);
#endif
	}
	outsock = 0;
	pState = EQNC_Init;
}

void EQNetworkConnection::DoRecvData() {
	if (ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnection::Connect() called on an incoming connection!");
		return;
	}
	LockMutex lock(&MSocketLock);
	if (!outsock)
		return;

	uchar		buffer[1518];
	int			status;

	while (1) {
#ifdef WIN32
		status = recv(outsock, (char*) buffer, sizeof(buffer), 0);
#else
		status = recv(outsock, buffer, sizeof(buffer), 0);
#endif
		if (status >= 1) {
			RecvData(buffer, status);
		}
		else {
			break;
		}
	}
}

void EQNetworkConnection::PacketPriority()
{
EQNetworkPacket* pack;

	LinkedListIterator<EQNetworkPacket*> iterator(SendQueue);
	iterator.Reset();
	EQNetworkPacket* enp[10];
	int i=0;
	while(iterator.MoreElements() && i < 10)
	{
	pack = iterator.GetData();
	if(pack != 0)
	{
	switch(pack->priority)
	{
	case 0:
		{
		//Well theres no status so we might as well leave it alone
		break;
		}
	case 1:
		{
		//Low priority, trash it
		i++;
		enp[i] = iterator.GetData();
		break;
		}
	case 2:
		{
		i++;
		//Medium-Low priority, will trash this over a Medium, but it depends on the result of how many are low priority
		enp[i] = iterator.GetData();
		break;
		}
	case 3:
		{
		//Medium priority, will trash it if there are not enough below
		i++;
		enp[i] = iterator.GetData();
		break;
		}
	case 4:
		{
		//Medium-High priority, will only trash if this is the only packet in queue
		i++;
		enp[i] = iterator.GetData();
		break;
		}
	case 5:
		{
		//HIGH PRIORITY -- NEVER TRASH!!
		break;
		}
	default:
		{
		break;
		}
	}
	}
	iterator.Advance();
	}

	int prioritize[5];
	prioritize[5] = 1;
	for(int z=0;z<i;z++)
	{
	if(enp[z] != 0)
	{
	if(enp[z]->priority == prioritize[prioritize[5]])
	{
	prioritize[prioritize[5]]++;
	RemovePacket(enp[z]);
	}
	}
	else
	{
	if(prioritize[prioritize[5]] < 4)
	{
		z = 0;

		if(prioritize[5] == 1 && prioritize[1] > 3)
		z = 11;
		if(prioritize[5] == 2 && prioritize[2] > 2  && prioritize[1] >= 1)
		z = 11;
		if(prioritize[5] == 3 && prioritize[3] >= 1  && (prioritize[2] >= 1 || prioritize[1] >= 1))
		z = 11;
		if(prioritize[5] == 4 && prioritize[4] >= 1)
		z = 11;

		prioritize[5]++;
		
	}
	else
	{
		z=11;
	}
	}
	}
}

InQueue_Struct* EQNetworkConnection::InQueuePop() {
	if (MInQueueLock.trylock()) {
		InQueue_Struct* ret = InQueue.pop();
		MInQueueLock.unlock();
		return ret;
	}
	else {
		return 0;
	}
}

#ifdef WIN32
void EQNetworkConnection::Process(SOCKET sock) {
#else
void EQNetworkConnection::Process(int sock) {
#endif
	if (!CheckNetActive())
		return;
	InQueue_Struct* iqs = 0;
	while ((iqs = InQueuePop())) {
		MakeEQPacket(iqs->app, iqs->ackreq);
		safe_delete(iqs->app);
		delete iqs;
	}

	if (timeout_timer->Check()) {
		#if EQN_DEBUG_Error >= 1
			cout << "Connection timeout." << endl;
		#endif
		Close();
//		SetState(EQNC_Error);
	}
	fraglist.CheckTimers();

	if (datarate_timer->Check()) {
		if (DataQueueFull()) {
			datahigh++;
			if (datahigh > 15)
				datahigh = 15;
		}
		else if (datahigh) {
			datahigh--;
		}
		dataflow -= datarate_tic;
		if (dataflow < 0)
			dataflow = 0;
	}

	EQNetworkPacket* pack;
	int32 size;
	uchar* data;
	sockaddr_in to;	
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_port = rPort;
	to.sin_addr.s_addr = rIP;

	LinkedListIterator<EQNetworkPacket*> iterator(SendQueue);
	iterator.Reset();
	if (iterator.MoreElements()) {
		keep_alive_timer->Start();
	}
	else if (IsFree()) {
		SetState(EQNC_Finished);
	}
#ifdef PRIORITYTEST
	if(DataQueueFull() && GetState() == EQNC_Active && queue_check_timer->Check())
	{
	PacketPriority();
	}
#endif

	if ((no_ack_sent_timer->Check() || keep_alive_timer->Check()) && GetState() == EQNC_Active) {
		APPLAYER outapp;
		outapp.opcode = 0xFFFF;
		outapp.priority = 5;
		MakeEQPacket(&outapp);
	}

	iterator.Reset();
	while (iterator.MoreElements() && (!DataQueueFull())) {
		pack = iterator.GetData();
//#ifdef WORLD
		// We are world, so we need to have the higher times or modem users can't get in
		// Fixed this, there's no datarate setting in world, so it was using the default in the constructor (500), now set to 5
//		if (((pack->LastSent + 750) <= Timer::GetCurrentTime()) || (datahigh < 5 && (pack->LastSent + 500) <= Timer::GetCurrentTime())) {
//#else
		if (((pack->LastSent + 500) <= Timer::GetCurrentTime()) || (datahigh < 5 && (pack->LastSent + 250) <= Timer::GetCurrentTime())) {
//#endif
#if EQN_DEBUG >= 8
			cout << "ARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec;
			cout << " pack->LastSent: " << setw(8) << setfill(' ') << pack->LastSent;
			cout << " Timer: " << setw(8) << setfill(' ') << Timer::GetCurrentTime();
			cout << " sentcount: " << setw(2) << setfill(' ') << (int) pack->SentCount;
			cout << " datahigh: "  << setw(3) << setfill(' ') << (int) datahigh << endl;
#endif
			size = pack->ReturnPacket(&data);
			#if LOG_RAW_PACKETS_OUT >= 1
				cout << setw(8) << setfill(' ') << Timer::GetCurrentTime() << " Outgoing RAW packet: opcode=0x" << hex << setw(4) << setfill('0') << pack->dwOpCode << dec << " size=" << setw(5) << setfill(' ') << size << " headers: ";
				cout << (int) pack->HDR.a0_Unknown;
				cout << (int) pack->HDR.a1_ARQ;
				cout << (int) pack->HDR.a2_Closing;
				cout << (int) pack->HDR.a3_Fragment;
				cout << " ";
				cout << (int) pack->HDR.a4_ASQ;
				cout << (int) pack->HDR.a5_SEQStart;
				cout << (int) pack->HDR.a6_Closing;
				cout << (int) pack->HDR.a7_SEQEnd;
				cout << "  ";
				cout << (int) pack->HDR.b0_SpecARQ;
				cout << (int) pack->HDR.b1_Unknown;
				cout << (int) pack->HDR.b2_ARSP;
				cout << (int) pack->HDR.b3_Unknown;
				cout << " ";
				cout << (int) pack->HDR.b4_Unknown;
				cout << (int) pack->HDR.b5_Unknown;
				cout << (int) pack->HDR.b6_Unknown;
				cout << (int) pack->HDR.b7_Unknown;
				cout << endl;
				#if LOG_RAW_PACKETS_OUT >= 2
					cout << "SEQ";
					if (pack->HDR.a5_SEQStart)
						cout << " Start";
					cout << ": 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << " ";
					if (pack->HDR.a1_ARQ)
						cout << "ARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
					if (pack->HDR.b2_ARSP)
						cout << "ARSP: 0x" << hex << setw(4) << setfill('0') << pack->dwARSP << dec << " ";
					if (pack->HDR.a4_ASQ) {
						cout << "ASQ_high: 0x" << hex << setw(2) << setfill('0') << pack->dbASQ_high << dec << " ";
						if (pack->HDR.a1_ARQ)
							cout << "ASQ_low: 0x" << hex << setw(2) << setfill('0') << pack->dbASQ_low << dec << " ";
					}
					cout << endl;
					if (pack->HDR.a3_Fragment)
						cout << "Frag: SEQ: " << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << " Cur: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwCurr << dec << " Total: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwTotal << dec << " " << endl;
					#if LOG_RAW_PACKETS_OUT >= 9
						DumpPacket(data, size);
					#elif LOG_RAW_PACKETS_OUT >= 3
						if (size >= 32)
							DumpPacket(data, 32);
						else
							DumpPacket(data, size);
					#endif
				#endif
			#endif
#if EQN_DEBUG_Unknown >= 10
if (rand() % 5 != 0)
#endif
			sendto(sock, (char*) data, size, 0, (sockaddr*) &to, sizeof(to));
			delete[] data;
			dataflow += size;
			if (!pack->HDR.a1_ARQ) {
				iterator.RemoveCurrent();
				continue;
			}
			pack->LastSent = Timer::GetCurrentTime();
			if (pack->SentCount++ > 15) {
				#if EQN_DEBUG_Error >= 1
					cout << "Dropping connection, SentCount > 15 on ARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << ", SEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << endl;
				#endif
				SetState(EQNC_Error);
//				MakeEQPacket(0);
			}
		}
		iterator.Advance();
	}
}

void EQNetworkConnection::RecvData(uchar* data, int32 size) {
	timeout_timer->Start();
	EQNetworkPacket* pack = new EQNetworkPacket(this);
	//dataflow += size; // Sony does not add to the bandwidth limit coming in, this fixes the /anon lag out from spamming
	if (!pack->DecodePacket(data, size)) {
		delete pack;
		return;
	}
	#if LOG_RAW_PACKETS_IN >= 1
		cout << setw(8) << setfill(' ') << Timer::GetCurrentTime() << " Incomming RAW packet: size=" << setw(5) << setfill(' ') << size << " headers: ";
		cout << (int) pack->HDR.a0_Unknown;
		cout << (int) pack->HDR.a1_ARQ;
		cout << (int) pack->HDR.a2_Closing;
		cout << (int) pack->HDR.a3_Fragment;
		cout << " ";
		cout << (int) pack->HDR.a4_ASQ;
		cout << (int) pack->HDR.a5_SEQStart;
		cout << (int) pack->HDR.a6_Closing;
		cout << (int) pack->HDR.a7_SEQEnd;
		cout << "  ";
		cout << (int) pack->HDR.b0_SpecARQ;
		cout << (int) pack->HDR.b1_Unknown;
		cout << (int) pack->HDR.b2_ARSP;
		cout << (int) pack->HDR.b3_Unknown;
		cout << " ";
		cout << (int) pack->HDR.b4_Unknown;
		cout << (int) pack->HDR.b5_Unknown;
		cout << (int) pack->HDR.b6_Unknown;
		cout << (int) pack->HDR.b7_Unknown;
		cout << endl;
		#if LOG_RAW_PACKETS_IN >= 2
			cout << "SEQ";
			if (pack->HDR.a5_SEQStart)
				cout << " Start";
			cout << ": 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << " ";
			if (pack->HDR.a1_ARQ)
				cout << "ARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
			if (pack->HDR.b2_ARSP)
				cout << "ARSP: 0x" << hex << setw(4) << setfill('0') << pack->dwARSP << dec << " ";
			if (pack->HDR.a4_ASQ) {
				cout << "ASQ_high: 0x" << hex << setw(2) << setfill('0') << pack->dbASQ_high << dec << " ";
				if (pack->HDR.a1_ARQ)
					cout << "ASQ_low: 0x" << hex << setw(2) << setfill('0') << pack->dbASQ_low << dec << " ";
			}
			cout << endl;
			if (pack->HDR.a3_Fragment)
				cout << "Frag: SEQ: " << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << " Cur: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwCurr << dec << " Total: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwTotal << dec << " " << endl;
			#if LOG_RAW_PACKETS_IN >= 9
				DumpPacket(data, size);
			#elif LOG_RAW_PACKETS_IN >= 3
				if (size >= 32)
					DumpPacket(data, 32);
				else
					DumpPacket(data, size);
			#endif
		#endif
	#endif
#if EQN_DEBUG_Unknown >= 7
	if (pack->dwARQ - dwLastCACK > 10) {
		cout << "Debug: pack->dwARQ - dwLastCACK > 10" << endl;
		if (size >= 32)
			DumpPacket(data, 32);
		else
			DumpPacket(data, size);
		cout << endl;
	}
#endif
	if (ProcessPacket(pack, false))
		delete pack;
	CheckBufferedPackets();
}

void EQNetworkConnection::RemovePacket(EQNetworkPacket* pack) {
	LinkedListIterator<EQNetworkPacket*> iterator(SendQueue);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData() == pack) {
			iterator.RemoveCurrent();
			break;
		}
		iterator.Advance();
	}
}

bool EQNetworkConnection::ProcessPacket(EQNetworkPacket* pack, bool from_buffer) {
    /************ CHECK FOR ACK/SEQ RESET ************/ 
    if(pack->HDR.a5_SEQStart) {
		if (dwLastCACK == pack->dwARQ - 1)
			return true;
//		cout << "resetting SACK.dwGSQ1" << endl;
//		SACK.dwGSQ      = 0;            //Main sequence number SHORT#2
		dwLastCACK      = pack->dwARQ-1;//0;
//		CACK.dwGSQ = 0xFFFF; changed next if to else instead
	}
    // Agz: Moved this, was under packet resend before..., later changed to else statement...
	else if( (pack->dwSEQ - CACK.dwGSQ) <= 0 && !from_buffer) {  // Agz: if from the buffer i ignore sequence number..
#if EQN_DEBUG_Error >= 6
		cout << Timer::GetCurrentTime() << " ";
		cout << "Invalid packet ";
		cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << " ";
		cout << "CACK.dwGSQ: 0x" << hex << setw(4) << setfill('0') << CACK.dwGSQ << dec << " ";
		cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
		cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << endl;
#endif
		return true; //Invalid packet
	}
#if EQN_DEBUG_Packet >= 8
	cout << Timer::GetCurrentTime() << " ";
	cout << "Valid packet ";
	cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << " ";
	cout << "CACK.dwGSQ: 0x" << hex << setw(4) << setfill('0') << CACK.dwGSQ << dec << " ";
	cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
	cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << endl;
#endif

    CACK.dwGSQ = pack->dwSEQ; //Get current sequence #.

    /************ Process ack responds ************/
	// Quagmire: Moved this to above "ack request" checking in case the packet is dropped in there
	if(pack->HDR.b2_ARSP)
		IncomingARSP(pack->dwARSP);
    /************ End process ack rsp ************/

    // Does this packet contain an ack request?
	if(pack->HDR.a1_ARQ) {
		// Is this packet a packet we dont want now, but will need later?
		if(pack->dwARQ - dwLastCACK > 1 && pack->dwARQ - dwLastCACK < 16) { // Agz: Added 16 limit
			#ifdef _DEBUG
				// Debug check, if we want to buffer a packet we got from the buffer something is wrong...
				if (from_buffer) {
					cerr << "ERROR: Rebuffering a packet in EQNetworkConnection::ProcessPacket" << endl;
					return true;
				}
			#endif
			LinkedListIterator<EQNetworkPacket*> iterator(BufferedPackets);
			iterator.Reset();
			while(iterator.MoreElements()){
				if (iterator.GetData()->dwARQ == pack->dwARQ) {
					#if EQN_DEBUG_Packet >= 5
						cout << Timer::GetCurrentTime() << " ";
						cout << "Tried to buffer this packet, but it was already buffered ";
						cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
						cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
						cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << endl;
					#endif
					return true; // This packet was already buffered
				}
				iterator.Advance();
			}

			#if EQN_DEBUG_Packet >= 6
				cout << Timer::GetCurrentTime() << " ";
				cout << "Buffering this packet ";
				cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
				cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
				cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << endl;
			#endif

			BufferedPackets.Append(pack);
			return false;
		}
        // Is this packet a resend we have already processed?
		if(pack->dwARQ - dwLastCACK <= 0) {
			#if EQN_DEBUG_Packet >= 5
				cout << Timer::GetCurrentTime() << " ";
				cout << "Duplicate packet received ";
				cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
				cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
				cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << endl;
			#endif
// Quag: No need for this really
//			no_ack_sent_timer->Trigger(); // Added to make sure we send a new ack respond
			return true;
		}
	}
    
	/************ START ACK REQ CHECK ************/
	if (pack->HDR.a1_ARQ || pack->HDR.b0_SpecARQ) {
		IncomingARQ(pack->dwARQ);
	}
	if (pack->HDR.b0_SpecARQ) { // Send the ack reponse right away
		no_ack_sent_timer->Trigger();
		#if EQN_DEBUG_ACK >= 3
			cout << "Special ARQ received." << endl;
		#endif
	}
	/************ END ACK REQ CHECK ************/

	/************ CHECK FOR THREAD TERMINATION ************/
	if(pack->HDR.a2_Closing && pack->HDR.a6_Closing) {
		MStateLock.lock();
		if(pState == EQNC_Active) {
			#if EQN_DEBUG >= 1
				cout << "Closing bits received from client" << endl;
			#endif
			pState = EQNC_Closing;
			MStateLock.unlock();
			InQueue_Struct* iqs = new InQueue_Struct;
			iqs->app = 0;
			iqs->ackreq = true;
			MInQueueLock.lock();
			InQueue.push(iqs);
			MInQueueLock.unlock();
//			MakeEQPacket(0); // Agz: Added a close packet
		}
		else {
			MStateLock.unlock();
		}
	}
	/************ END CHECK THREAD TERMINATION ************/

	/************ Get ack sequence number ************/
	if(pack->HDR.a4_ASQ) {
		CACK.dbASQ_high = pack->dbASQ_high;
		if(pack->HDR.a1_ARQ)
			CACK.dbASQ_low = pack->dbASQ_low;
	}
    
	/************ End get ack seq num ************/

	/************ START FRAGMENT CHECK ************/
	/************ IF - FRAGMENT ************/
	if(pack->HDR.a3_Fragment) {
		#if EQN_DEBUG_Fragment >= 5
			cout << endl << Timer::GetCurrentTime() << " Fragment: ";
			cout << "pack->fraginfo.dwSeq: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << " ";
			cout << "pack->fraginfo.dwCurr: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwCurr << dec << " ";
			cout << "pack->fraginfo.dwTotal: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwTotal << dec << endl;
		#endif
		EQNetworkFragmentGroup* fragment_group = 0;
		fragment_group = fraglist.GotPacket(pack);

		// If we have all the fragments to complete this group
		if(fragment_group->Ready()) {
			#if EQN_DEBUG_Fragment >= 3
				cout << Timer::GetCurrentTime() << " Getting fragment opcode: " << hex << setw(4) << setfill('0') << fragment_group->GetOpcode() << dec << endl;
			#endif
			//Collect fragments and put them as one packet on the OutQueue
			APPLAYER *app = new APPLAYER;
			app->opcode = fragment_group->GetOpcode();
			app->size = fragment_group->Assemble(&app->pBuffer);
			fraglist.DeleteGroup(pack->fraginfo.dwSeq);
			#if EQN_DEBUG_Fragment >= 2
				cout << "Fragment_group 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << " finished" << endl;
			#endif

#if LOG_PACKETS >= 1
	if (app && app->opcode != 0 && app->opcode != 0xFFFF) {
		cout << "Logging incoming packet. OPCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << setw(5) << setfill(' ') << app->size << endl;
		#if LOG_PACKETS == 2
			if (app->size >= 32)
				DumpPacket(app->pBuffer, 32);
			else
				DumpPacket(app);
		#endif
		#if LOG_PACKETS >= 3
			DumpPacketHex(app);
		#endif
	}
#endif
			OutQueuePush(app);
            return true;
        }
        else {
			#if EQN_DEBUG_Fragment >= 4
                cout << "FRAGMENT_GROUP not finished wait for more... " << endl;
			#endif
			return true;
		}
	}
	/************ ELSE - NO FRAGMENT ************/
    else {
		if (!pack->dwOpCode) {
			// pure ack or pure ackreq, doesnt need to leave this class
			return true;
		}

		APPLAYER *app	= new APPLAYER(pack->dwOpCode);
		app->size       = pack->dwExtraSize;
		app->pBuffer    = new uchar[pack->dwExtraSize];
		memcpy(app->pBuffer, pack->pExtra, pack->dwExtraSize);

#if LOG_PACKETS >= 1
	if (app && app->opcode != 0 && app->opcode != 0xFFFF) {
		cout << "Logging incoming packet. OPCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << setw(5) << setfill(' ') << app->size << endl;
		#if LOG_PACKETS == 2
			if (app->size >= 32)
				DumpPacket(app->pBuffer, 32);
			else
				DumpPacket(app);
		#endif
		#if LOG_PACKETS >= 3
			DumpPacket(app);
		#endif
	}
#endif

		OutQueuePush(app);
		return true;
	}
	/************ END FRAGMENT CHECK ************/

	cout << endl << "reached end of ProcessPacket?!" << endl << endl;
	return true;
}

void EQNetworkConnection::IncomingARSP(int16 arsp) { 
	#if EQN_DEBUG_ACK >= 3
        cout << "Incoming ack response: " << arsp << " SACK.dwARQ: " << hex << setw(4) << setfill('0') << SACK.dwARQ << dec << endl;
	#endif
	#if EQN_DEBUG_ACK >= 4
		bool MoreARQPackets = false;
	#endif
	LinkedListIterator<EQNetworkPacket*> iterator(SendQueue);
	iterator.Reset();
	while (iterator.MoreElements()) {
		if (arsp >= iterator.GetData()->dwARQ) {
			#if EQN_DEBUG_ACK >= 5
				cout << "Removing " << iterator.GetData()->dwARQ << " from resendqueue" << endl;
			#endif
			iterator.RemoveCurrent();
		}
		else {
			#if EQN_DEBUG_ACK >= 4
				if (iterator.GetData()->HDR.a1_ARQ) {
					MoreARQPackets = true;
				}
			#endif
			iterator.Advance();
		}
	}
	#if EQN_DEBUG_ACK >= 4
		if (!MoreARQPackets) {
			cout << Timer::GetCurrentTime() << " no more ARQ on SendQueue";
			cout << " dwARSP: 0x" << hex << setw(4) << setfill('0') << arsp << dec;
			cout << " SACK.dwARQ: 0x" << hex << setw(4) << setfill('0') << SACK.dwARQ << dec << endl;
		}
	#endif
}

void EQNetworkConnection::IncomingARQ(int16 arq) {
	CACK.dwARQ = arq;
	dwLastCACK = arq;
    
	if (!no_ack_sent_timer->Enabled()) {
		no_ack_sent_timer->Start(500); // Agz: If we dont get any outgoing packet we can put an 
		// ack response in before 500ms has passed we send a pure ack response
		#if EQN_DEBUG_ACK >= 4
			cout << Timer::GetCurrentTime() << " no_ack_sent_timer->Start(500)" << endl;
		#endif
    }
}

void EQNetworkConnection::CheckBufferedPackets() {
	#if EQN_DEBUG_Packet >= 6
		int num=0; // Counting buffered packets for debug output
	#endif
	LinkedListIterator<EQNetworkPacket*> iterator(BufferedPackets);
	iterator.Reset();
	while(iterator.MoreElements()) {
		// Check if we have a packet we want already buffered
		if (iterator.GetData()->dwARQ - dwLastCACK == 1) {
			#if EQN_DEBUG_Packet >= 7
				cout << Timer::GetCurrentTime() << " ";
				cout << "Found a packet we want in the incoming packet buffer";
				cout << "pack->dwARQ:" << hex << setw(4) << setfill('0') << iterator.GetData()->dwARQ << dec << " ";
				cout << "dwLastCACK:" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
				cout << "pack->dwSEQ:" << hex << setw(4) << setfill('0') << iterator.GetData()->dwSEQ << dec << endl;
			#endif
			ProcessPacket(iterator.GetData(), true);
			iterator.RemoveCurrent();	// This will call delete on the packet
			iterator.Reset();			// Start from the beginning of the list again
			#if EQN_DEBUG_Packet >= 6
				num=0;					// Reset the counter
			#endif
			continue;
		}
		if (iterator.GetData()->dwARQ <= dwLastCACK) {
			#if EQN_DEBUG_Packet >= 1 || EQN_DEBUG_Error >= 1
				cout << Timer::GetCurrentTime() << " ";
				cout << "Found an old packet in the buffer, discarding. ";
				cout << "pack->dwARQ:" << hex << setw(4) << setfill('0') << iterator.GetData()->dwARQ << dec << " ";
				cout << "dwLastCACK:" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
				cout << "pack->dwSEQ:" << hex << setw(4) << setfill('0') << iterator.GetData()->dwSEQ << dec << endl;
			#endif
			iterator.RemoveCurrent();
			continue;
		}
		#if EQN_DEBUG_Packet >= 6
			num++;
		#endif
		iterator.Advance();
	}

	#if EQN_DEBUG_Packet >= 6
		if (num)
			cout << "Number of packets still in buffer: " << num << endl;
	#endif
}

void EQNetworkConnection::MakeEQPacket(APPLAYER* app, bool ackreq) {
#if LOG_PACKETS >= 1
	if (app && app->opcode != 0 && app->opcode != 0xFFFF) {
		cout << "Logging outgoing packet. OPCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << setw(5) << setfill(' ') << app->size << endl;
		#if LOG_PACKETS == 2
			if (app->size >= 32)
				DumpPacket(app->pBuffer, 32);
			else
				DumpPacket(app);
		#endif
		#if LOG_PACKETS >= 3
			DumpPacket(app);
		#endif
	}
#endif

	MStateLock.lock();
//	if(pState == EQNC_Closing) {
	if (!app) { // Quag: should fix packets not getting sent before the closing
//		SetState(EQNC_Closing);
/*		if (app) {
			MStateLock.unlock();
			return;
		}*/
		pState = EQNC_Closed;
		MStateLock.unlock();
		EQNetworkPacket* pack = new EQNetworkPacket(this);
		pack->HDR.a6_Closing    = 1;// Agz: Lets try to uncomment this line again
		pack->HDR.a2_Closing    = 1;// and this
		pack->HDR.a1_ARQ        = 1;// and this
//      pack->dwARQ             = 1;// and this, no that was not too good
		pack->dwARQ             = SACK.dwARQ++;// try this instead
		pack->priority = 0;

		SendQueue.Append(pack);
//		SetState(EQNC_Closed);
        return;
    }
	else {
		MStateLock.unlock();
	}

	if (!app) {
		#if EQN_DEBUG >= 2
			cout << "EQNetworkConnection::MakeEQPacket app == 0" << endl;
		#endif
		return;
	}

	//bool bFragment= false; //This is set later on if fragseq should be increased at the end.

	/************ IF opcode is == 0xFFFF it is a request for pure ack creation ************/
	if(app->opcode == 0xFFFF) {
		EQNetworkPacket* pack = new EQNetworkPacket(this);

		if(!SACK.dwGSQ) {
			SACK.dwGSQ++;
//			pack->HDR.a5_SEQStart	= 1;			// Agz: hmmm, yes commenting this makes the client connect to zone
													//      server work and the world server doent seem to care either way
			SACK.dwARQ				= rand()%0x3FFF;//Current request ack
			SACK.dbASQ_high			= 1;			//Current sequence number
			SACK.dbASQ_low			= 0;			//Current sequence number

			#if EQN_DEBUG_ACK >= 1
				cout << "New random SACK.dwARQ" << endl;
			#endif
		}

//		pack->dwOpCode = 0xFFFF;
		#if EQN_DEBUG_ACK >= 2
			cout << Timer::GetCurrentTime() << " Pure ack sent ";
			cout << "pack->dwARSP=dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
			cout << "pack->dwSEQ=SACK.dwGSQ: 0x" << hex << setw(4) << setfill('0') << SACK.dwGSQ << dec << endl;
		#endif
			pack->priority = app->priority;
		SendQueue.Append(pack);

		return;
	}


	int frags = (app->size >> 9);
	uchar* appBuffer = app->pBuffer;

	if (frags)
		ackreq = true; // Fragmented packets must have ackreq set

	#if EQN_DEBUG_Fragment >= 4
        cout << Timer::GetCurrentTime() << " frags: " << frags << endl;
	#endif

	int8 tmp = GetState();
	if (tmp == EQNC_Active || tmp == EQNC_Closing) {
	/************ PM STATE = ACTIVE ************/
		for (int i=0; i<=frags; i++) {
			EQNetworkPacket* pack = new EQNetworkPacket(this);
    
			if(!SACK.dwGSQ) {
				SACK.dwGSQ++;
				pack->HDR.a5_SEQStart   = 1;
				SACK.dwARQ              = rand()%0x3FFF;//Current request ack
				SACK.dbASQ_high         = 1;            //Current sequence number
				SACK.dbASQ_low          = 0;            //Current sequence number   
				#if EQN_DEBUG_ACK >= 1
					cout << "New random SACK.dwARQ: 0x" << hex << setw(4) << setfill('0') << SACK.dwARQ << dec << endl;
				#endif
			}

			//IF NON PURE ACK THEN ALWAYS INCLUDE A ACKSEQ              // Agz: Not anymore... Always include ackseq if not a fragmented packet
			if (i==0 && ackreq) // If this will be a fragmented packet, only include ackseq in first fragment
				pack->HDR.a4_ASQ = 1;                                   // This is what the eq servers does

			/************ Caculate the next ACKSEQ/acknumber ************/
			/************ Check if its a static ackseq ************/
			if( HI_BYTE(app->opcode) == 0x2000) {
				if(app->size == 15)
					pack->dbASQ_low = 0xb2;
				else
					pack->dbASQ_low = 0xa1;
			}
			/************ Normal ackseq ************/
			else {
				//If not pure ack and opcode != 0x20XX then
				if (ackreq) { // If the caller of this function wants us to put an ack request on this packet
					pack->HDR.a1_ARQ = 1;
					pack->dwARQ      = SACK.dwARQ++;
				}

				if(pack->HDR.a1_ARQ && pack->HDR.a4_ASQ) {
					pack->dbASQ_low  = SACK.dbASQ_low;
					pack->dbASQ_high = SACK.dbASQ_high;
				}
				else if(pack->HDR.a4_ASQ) {
					pack->dbASQ_high = SACK.dbASQ_high;
				}
				if(pack->HDR.a4_ASQ)
					SACK.dbASQ_low++;
			}

			/************ Check if this packet should contain op ************/
			if (app->opcode && i == 0) {
				pack->dwOpCode = app->opcode;
			}
			/************ End opcode check ************/

			/************ SHOULD THIS PACKET BE SENT AS A FRAGMENT ************/
			if(frags) {
				pack->HDR.a3_Fragment = 1;
			}
			/************ END FRAGMENT CHECK ************/

			if(app->size && app->pBuffer) {
				if(pack->HDR.a3_Fragment) {
					// If this is the last packet in the fragment group
					if(i == frags) {
						// Calculate remaining bytes for this fragment
						pack->dwExtraSize = app->size-510-512*((app->size/512)-1);
					}
					else if(i == 0) {
						pack->dwExtraSize = 510; // The first packet in a fragment group has 510 bytes for data
					}
					else {
						pack->dwExtraSize = 512; // Other packets in a fragment group has 512 bytes for data
					}
					pack->fraginfo.dwCurr	= i;
					pack->fraginfo.dwSeq	= dwFragSeq;
					pack->fraginfo.dwTotal	= frags + 1;
				}
				else {
					pack->dwExtraSize = (int16) app->size;
				}
				pack->pExtra = new uchar[pack->dwExtraSize];
				memcpy(pack->pExtra, appBuffer, pack->dwExtraSize);
				appBuffer += pack->dwExtraSize; //Increase counter
			}       

			/******************************************/
			/*********** !PACKET GENERATED! ***********/
			/******************************************/
			SendQueue.Append(pack);
		}//end for

		if(frags)
			dwFragSeq++;        
	} //end if
}

EQNetworkPacket::EQNetworkPacket(EQNetworkConnection* inetcon) {
	netcon = inetcon;
	pExtra = 0;
	Clear();
}

EQNetworkPacket::~EQNetworkPacket() {
	safe_delete(pExtra);
}

void EQNetworkPacket::Clear() {
	*((int16*)&HDR) = 0;

	dwSEQ			= 0;
	dwARSP          = 0;
	dwARQ           = 0;
	dbASQ_low       = 0;        
	dbASQ_high      = 0;
	dwOpCode        = 0;    
	fraginfo.dwCurr = 0;
	fraginfo.dwSeq  = 0;
	fraginfo.dwTotal= 0;
	dwExtraSize     = 0;
	safe_delete(pExtra);

	LastSent		= 0;
	SentCount		= 0;
}

void EQNetworkPacket::AddData(uchar* data, int32 size) {
	safe_delete(pExtra);
	pExtra = new uchar[size];
	memcpy(pExtra, data, size);
}

bool EQNetworkPacket::DecodePacket(uchar* data, int32 size) {
    /************ LOCAL VARS ************/
    int32	o	= 0;
    
/************ START PROCESSING ************/

/*	if (size < 10) {	// Agz: Adding checks for illegal packets, so we dont read beyond the buffer
						//      Minimum normal packet length is 10
		cerr << "EQPacket.cpp: Illegal packet length" << endl;
		return false;      // TODO: Do we want to check crc checksum on the incoming packets?
    }*/

    memcpy(&HDR, data, 2);
    o += 2;

	dwSEQ = ntohs(*((int16*)&data[o]));
	o += 2;
    

    /************ CHECK ACK FIELDS ************/
    /**********/
    //Common ACK Response
    if(HDR.b2_ARSP) {
        dwARSP = ntohs(*((int16*)&data[o]));
        o += 2;
    }
    if (HDR.b3_Unknown) { // this is definately a SACK or NACK
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b3: 0x" << hex << setw(4) << setfill('0') << *((int16*)&data[o]) << dec << endl;
		#endif
		o += 2;
	}
    // Agz: Dont know what this HDR.b4_Unknown data is, gets sent when there is packetloss
    if (HDR.b4_Unknown) {
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b4: 0x" << hex << setw(2) << setfill('0') << (int) data[o] << dec << endl;
		#endif
		o++; // One unknown byte
    }
	/*
	  See Agz's comment above about HDR.b4.
	  Same story with b5, b6, b7, but they're 2, 4 and 8 bytes respectively.
	  -Quagmire
	*/
    if (HDR.b5_Unknown) {
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b5: 0x" << hex << setw(4) << setfill('0') << *((int16*)&data[o]) << dec << endl;
		#endif
        o += 2; // 2 unknown bytes
    }
	if (HDR.b6_Unknown) {
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b6: 0x" << hex << setw(8) << setfill('0') << *((int32*)&data[o]) << dec << endl;
		#endif
		o += 4; // 4 unknown bytes
	}
	if (HDR.b7_Unknown) {
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b7: 0x" << hex << setw(8) << setfill('0') << *((int32*)&data[o]) << dec << " 0x" << hex << setw(8) << setfill('0') << *((int32*)&data[o+4]) << dec << endl;
		#endif
		o += 8; // 8 unknown bytes
	}
#if EQN_DEBUG_Unknown >= 8
	if (HDR.b4_Unknown || HDR.b5_Unknown || HDR.b6_Unknown || HDR.b7_Unknown) {
		if (size >= 32)
			DumpPacket(data, 32);
		else
			DumpPacket(data, size);
	}
#endif
    
	/************/ 
    //Common  ACK Request
	if(HDR.a1_ARQ) {
		dwARQ = ntohs(*((int16*)&data[o]));
		o += 2;
	}
	/************ END CHECK ACK FIELDS ************/



	/************ CHECK FRAGMENTS ************/
	if(HDR.a3_Fragment) { 
		if (size < 16) { // Agz: Adding checks for illegal packets, so we dont read beyond the buffer
			cerr << "Illegal packet length" << endl;
			return false;
		}

		//Extract frag info.
		fraginfo.dwSeq   = ntohs(*((int16*)&data[o]));
		o += 2;
		fraginfo.dwCurr  = ntohs(*((int16*)&data[o]));
		o += 2;
		fraginfo.dwTotal = ntohs(*((int16*)&data[o]));
		o += 2;
	}
	/************ END CHECK FRAGMENTS ************/
    


	/************ CHECK ACK SEQUENCE ************/
	if(HDR.a4_ASQ) {
		dbASQ_high = data[o];
		o++;
		if(HDR.a1_ARQ) {
			dbASQ_low  = data[o];
			o++;
		}
	}
	/************ END CHECK ACK SEQUENCE ************/
    
	/************ GET OPCODE/EXTRACT DATA ************/
//  if( (size-4) != ACK_ONLY_SIZE && !(HDR.a2_Closing && HDR.a6_Closing))
//-4 (crc dont count) // Agz: Added closing checks...
	if(size-o > 4 && !(HDR.a2_Closing && HDR.a6_Closing)) { // Agz: Another bug fix. I got a arq&arsp packet with no data from the client.
		/************ Extract applayer ************/
		if(!fraginfo.dwCurr) { //OPCODE only included in first fragment!
			dwOpCode = ntohs(*((int16*)&data[o]));
			o += 2;
        }

        dwExtraSize = (size - o) - 4;
		if (size < (o + 4)) {
			#if EQN_DEBUG_Error >= 3
				cerr << "ERROR: Packet we couldnt handle, ExtraSize < 0" << endl;
				#if EQN_DEBUG_Error >= 9
					DumpPacket(data, size);
				#endif
			#endif
            return false;
		}
		if (dwExtraSize) {
			pExtra  = new uchar[dwExtraSize];
			memcpy(pExtra, &data[o], dwExtraSize);
		}
	}
	else {   /************ PURE ACK ************/
		dwOpCode    = 0;
		pExtra      = 0;
		dwExtraSize = 0;
	}
	/************ END GET OPCODE/EXTRA DATA ************/


/************ END PROCESSING ************/
	return true;
}

int32 EQNetworkPacket::ReturnPacket(uchar** data) {
	*data = new uchar[dwExtraSize + MAX_HEADER_SIZE];
	uchar* buf = *data;
	int32 o = 4;
	this->dwSEQ = netcon->SACK.dwGSQ;
	*((int16*)&buf[2]) = ntohs(netcon->SACK.dwGSQ++);

	if (netcon->CACK.dwARQ) {
		dwARSP = netcon->CACK.dwARQ;
		HDR.b2_ARSP = 1;				//Set ack response field
		*((int16*)&buf[o]) = ntohs(netcon->CACK.dwARQ);
		o += 2;
		netcon->CACK.dwARQ = 0;
		netcon->no_ack_sent_timer->Disable();
	}
	else if (dwOpCode == 0xFFFF) {
		dwARSP = netcon->dwLastCACK;
		HDR.b2_ARSP = 1;				//Set ack response field
		*((int16*)&buf[o]) = ntohs(netcon->dwLastCACK);
		o += 2;
		netcon->CACK.dwARQ = 0;
		netcon->no_ack_sent_timer->Disable();
	}
	else {
		HDR.b2_ARSP = 0;
	}

	if (HDR.a1_ARQ) {
		*((int16*)&buf[o]) = ntohs(dwARQ);
		o += 2;
	}
	if(HDR.a3_Fragment) {
        *((int16*)&buf[o]) = ntohs(fraginfo.dwSeq);
		o += 2;
        *((int16*)&buf[o]) = ntohs(fraginfo.dwCurr);
		o += 2;
        *((int16*)&buf[o]) = ntohs(fraginfo.dwTotal);
		o += 2;
	}
    if(HDR.a4_ASQ && HDR.a1_ARQ) {
		*&buf[o++] = dbASQ_high;
		*&buf[o++] = dbASQ_low;
	}
	else if(HDR.a4_ASQ) {
		*&buf[o++] = dbASQ_high;
	}
	if (dwOpCode) {
        *((int16*)&buf[o]) = ntohs(dwOpCode);
		o += 2;
	}
	if(pExtra) {
		memcpy(&buf[o], pExtra, dwExtraSize);
		o += dwExtraSize;
	}
	memcpy(buf, &HDR, 2);
	*((int32*)&buf[o]) = htonl(CRC32::Generate(buf, o));
	o += 4;
	return o;
}

EQNetworkFragmentGroup::EQNetworkFragmentGroup(int16 ifragseq, int16 inum_fragments) {
	timeout_timer = new Timer(15000);
	fragseq = ifragseq;
	num_fragments = inum_fragments;
	fragments = new uchar*[num_fragments];
	fragment_sizes = new int32[num_fragments];
	for (int i=0; i<num_fragments; i++) {
		fragments[i] = 0;
		fragment_sizes[i] = 0;
	}
	opcode = 0xFFFF;
}

EQNetworkFragmentGroup::~EQNetworkFragmentGroup() {
	delete timeout_timer;
	for (int i=0; i<num_fragments; i++) {
		if (fragments[i])
			delete[] fragments[i];
	}
	delete[] fragments;
	delete[] fragment_sizes;
}

void EQNetworkFragmentGroup::Add(EQNetworkPacket* pack) {
#ifdef _DEBUG
	if (pack->fraginfo.dwSeq != fragseq) {
		ThrowError("EQNetworkFragmentGroup::Add(): pack->fraginfo.dwSeq != fragseq");
	}
	if (pack->fraginfo.dwCurr >= num_fragments) {
		ThrowError("EQNetworkFragmentGroup::Add(): pack->fraginfo.dwCurr >= num_fragments");
	}
#endif
	timeout_timer->Start();
	if (fragments[pack->fraginfo.dwCurr])
		return; // already got it, this one musta been a resend
	if (pack->fraginfo.dwCurr == 0)
		opcode = pack->dwOpCode;
	fragments[pack->fraginfo.dwCurr] = pack->pExtra;
	pack->pExtra = 0;
	fragment_sizes[pack->fraginfo.dwCurr] = pack->dwExtraSize;
}

int32 EQNetworkFragmentGroup::Assemble(uchar** data) {
	int32 size = 0;
	int i;
#ifdef _DEBUG
	if (!Ready()) {
		ThrowError("EQNetworkFragmentGroup::Assemble() when not ready");
	}
#endif
	for (i=0; i<num_fragments; i++) {
		size += fragment_sizes[i];
	}
	*data = new uchar[size];
	uchar* tmp = *data;
	for (i=0; i<num_fragments; i++) {
		memcpy(tmp, fragments[i], fragment_sizes[i]);
		tmp += fragment_sizes[i];
	}
	return size;
}

bool EQNetworkFragmentGroup::Ready() {
	for (int i=0; i<num_fragments; i++) {
		if (!fragments[i])
			return false;
#ifdef _DEBUG
		if (!fragment_sizes[i]) {
			ThrowError("EQNetworkFragmentGroup::Ready(): fragment_sizes[i] == 0");
		}
#endif
	}
	return true;
}

EQNetworkFragmentGroupList::EQNetworkFragmentGroupList() {
	timeout_check_timer = new Timer(5000);
}

EQNetworkFragmentGroupList::~EQNetworkFragmentGroupList() {
	delete timeout_check_timer;
}

void EQNetworkFragmentGroupList::CheckTimers() {
	if (!timeout_check_timer->Check())
		return;
	LinkedListIterator<EQNetworkFragmentGroup*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->timeout_timer->Check()) {
			#if EQN_DEBUG_Fragment >= 2
				cout << "Fragment_group 0x" << hex << setw(4) << setfill('0') << iterator.GetData()->GetFragSeq() << dec << " timed out." << endl;
			#endif
			iterator.RemoveCurrent();
		}
		else
			iterator.Advance();
	}
}

EQNetworkFragmentGroup* EQNetworkFragmentGroupList::GotPacket(EQNetworkPacket* pack) {
	LinkedListIterator<EQNetworkFragmentGroup*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->GetFragSeq() == pack->fraginfo.dwSeq) {
			iterator.GetData()->Add(pack);
			return iterator.GetData();
		}
		iterator.Advance();
	}
	#if EQN_DEBUG_Fragment >= 2
		cout << "New Fragment_group 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << endl;
	#endif
	EQNetworkFragmentGroup* fg = new EQNetworkFragmentGroup(pack->fraginfo.dwSeq, pack->fraginfo.dwTotal);
	fg->Add(pack);
	list.Insert(fg);
	return fg;
}

EQNetworkFragmentGroup* EQNetworkFragmentGroupList::FindGroup(int16 fragseq) {
	LinkedListIterator<EQNetworkFragmentGroup*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->GetFragSeq() == fragseq) {
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}

void EQNetworkFragmentGroupList::DeleteGroup(int16 fragseq) {
	LinkedListIterator<EQNetworkFragmentGroup*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->GetFragSeq() == fragseq) {
			iterator.RemoveCurrent();
			return;
		}
		else {
			iterator.Advance();
		}
	}
}
