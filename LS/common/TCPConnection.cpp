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
#include "../common/debug.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>

using namespace std;

#include "TCPConnection.h"
#include "../common/servertalk.h"
#include "../common/timer.h"
#include "../common/packet_dump.h"

#ifdef FREEBSD //Timothy Whitman - January 7, 2003
	#define MSG_NOSIGNAL 0
#endif

#ifdef WIN32
InitWinsock winsock;
#endif

#define TCPN_DEBUG				0
#define TCPN_DEBUG_Console		0
#define TCPN_DEBUG_Memory		0
#define TCPN_LOG_PACKETS		0
#define TCPN_LOG_RAW_DATA_OUT	0
#define TCPN_LOG_RAW_DATA_IN	0

TCPConnection::TCPNetPacket_Struct* TCPConnection::MakePacket(const ServerPacket* pack, int32 iDestination) {
	sint32 size = sizeof(TCPNetPacket_Struct) + pack->size;
	if (pack->compressed) {
		size += 4;
	}
	if (iDestination) {
		size += 4;
	}
	TCPNetPacket_Struct* tnps = (TCPNetPacket_Struct*) new uchar[size];
	tnps->size = size;
	tnps->opcode = pack->opcode;
	*((int8*) &tnps->flags) = 0;
	uchar* buffer = tnps->buffer;
	if (pack->compressed) {
		tnps->flags.compressed = 1;
		*((sint32*) buffer) = pack->InflatedSize;
		buffer += 4;
	}
	if (iDestination) {
		tnps->flags.destination = 1;
		*((sint32*) buffer) = iDestination;
		buffer += 4;
	}
	memcpy(buffer, pack->pBuffer, pack->size);
	return tnps;
}

SPackSendQueue* TCPConnection::MakeOldPacket(const ServerPacket* pack) {
	SPackSendQueue* spsq = (SPackSendQueue*) new uchar[sizeof(SPackSendQueue) + pack->size + 4];
	if (pack->pBuffer != 0 && pack->size != 0)
		memcpy((char *) &spsq->buffer[4], (char *) pack->pBuffer, pack->size);
	memcpy((char *) &spsq->buffer[0], (char *) &pack->opcode, 2);
	spsq->size = pack->size+4;
	memcpy((char *) &spsq->buffer[2], (char *) &spsq->size, 2);
	return spsq;
}

TCPConnection::TCPConnection(bool iOldFormat, TCPServer* iRelayServer, eTCPMode iMode) {
	id = 0;
	Server = iRelayServer;
	if (Server)
		RelayServer = true;
	RelayLink = 0;
	RelayCount = 0;
	RemoteID = 0;
	pOldFormat = iOldFormat;
	ConnectionType = Outgoing;
	TCPMode = iMode;
	pState = TCPS_Ready;
	pFree = false;
	pEcho = false;
	sock = 0;
	rIP = 0;
	rPort = 0;
	keepalive_timer = new Timer(SERVER_TIMEOUT);
	timeout_timer = new Timer(SERVER_TIMEOUT * 2);
	recvbuf = 0;
	sendbuf = 0;
	pRunLoop = false;
	charAsyncConnect = 0;
	pAsyncConnect = false;
#if TCPN_DEBUG_Memory >= 7
	cout << "Constructor #1 on outgoing TCP# " << GetID() << endl;
#endif
}

TCPConnection::TCPConnection(TCPServer* iServer, SOCKET in_socket, int32 irIP, int16 irPort, bool iOldFormat) {
	Server = iServer;
	RelayLink = 0;
	RelayServer = false;
	RelayCount = 0;
	RemoteID = 0;
	id = Server->GetNextID();
	ConnectionType = Incomming;
	pOldFormat = iOldFormat;
	if (pOldFormat)
		TCPMode = modePacket;
	else
		TCPMode = modeConsole;
	pState = TCPS_Connected;
	pFree = false;
	pEcho = false;
	connection_socket = in_socket;
	rIP = irIP;
	rPort = irPort;
	keepalive_timer = new Timer(SERVER_TIMEOUT);
	timeout_timer = new Timer(SERVER_TIMEOUT * 2);
	recvbuf = 0;
	sendbuf = 0;
	pRunLoop = false;
	charAsyncConnect = 0;
	pAsyncConnect = false;
#if TCPN_DEBUG_Memory >= 7
	cout << "Constructor #2 on outgoing TCP# " << GetID() << endl;
#endif
}

TCPConnection::TCPConnection(TCPServer* iServer, TCPConnection* iRelayLink, int32 iRemoteID, int32 irIP, int16 irPort) {
	Server = iServer;
	RelayLink = iRelayLink;
	RelayServer = true;
	id = Server->GetNextID();
	RelayCount = 0;
	RemoteID = iRemoteID;
	if (!RemoteID)
		ThrowError("Error: TCPConnection: RemoteID == 0 on RelayLink constructor");
	pOldFormat = false;
	ConnectionType = Incomming;
	TCPMode = modePacket;
	pState = TCPS_Connected;
	pFree = false;
	pEcho = false;
	connection_socket = 0;
	rIP = irIP;
	rPort = irPort;
	keepalive_timer = 0;
	timeout_timer = 0;
	recvbuf = 0;
	sendbuf = 0;
	pRunLoop = false;
	charAsyncConnect = 0;
	pAsyncConnect = false;
#if TCPN_DEBUG_Memory >= 7
	cout << "Constructor #3 on outgoing TCP# " << GetID() << endl;
#endif
}

TCPConnection::~TCPConnection() {
	Disconnect();
	ClearBuffers();
	if (ConnectionType == Outgoing) {
		MRunLoop.lock();
		pRunLoop = false;
		MRunLoop.unlock();
		MLoopRunning.lock();
		MLoopRunning.unlock();
#if TCPN_DEBUG_Memory >= 6
		cout << "Deconstructor on outgoing TCP# " << GetID() << endl;
#endif
	}
#if TCPN_DEBUG_Memory >= 5
	else {
		cout << "Deconstructor on incomming TCP# " << GetID() << endl;
	}
#endif
	safe_delete(keepalive_timer);
	safe_delete(timeout_timer);
	safe_delete(recvbuf);
	safe_delete(sendbuf);
	safe_delete(charAsyncConnect);
}

void TCPConnection::SetState(int8 in_state) {
	MState.lock();
	pState = in_state;
	MState.unlock();
}

int8 TCPConnection::GetState() {
	int8 ret;
	MState.lock();
	ret = pState;
	MState.unlock();
	return ret;
}

void TCPConnection::Free() {
	if (ConnectionType == Outgoing) {
		ThrowError("TCPConnection::Free() called on an Outgoing connection");
	}
#if TCPN_DEBUG_Memory >= 5
	cout << "Free on TCP# " << GetID() << endl;
#endif
	Disconnect();
	pFree = true;
}

bool TCPConnection::SendPacket(const ServerPacket* pack, int32 iDestination) {
	LockMutex lock(&MState);
	if (!Connected())
		return false;
	eTCPMode tmp = GetMode();
	if (tmp != modePacket && tmp != modeTransition)
		return false;
	if (RemoteID)
		return RelayLink->SendPacket(pack, RemoteID);
	else if (pOldFormat) {
		#if TCPN_LOG_PACKETS >= 1
			if (pack && pack->opcode != 0) {
				struct in_addr	in;
				in.s_addr = GetrIP();
				CoutTimestamp(true);
				cout << ": Logging outgoing TCP OldPacket. OPCode: 0x" << hex << setw(4) << setfill('0') << pack->opcode << dec << ", size: " << setw(5) << setfill(' ') << pack->size << " " << inet_ntoa(in) << ":" << GetrPort() << endl;
				#if TCPN_LOG_PACKETS == 2
					if (pack->size >= 32)
						DumpPacket(pack->pBuffer, 32);
					else
						DumpPacket(pack);
				#endif
				#if TCPN_LOG_PACKETS >= 3
					DumpPacket(pack);
				#endif
			}
		#endif
		SPackSendQueue* spsq = MakeOldPacket(pack);
		ServerSendQueuePushEnd(spsq->buffer, spsq->size);
		delete spsq;
	}
	else {
		TCPNetPacket_Struct* tnps = MakePacket(pack, iDestination);
		if (tmp == modeTransition) {
			InModeQueuePush(tnps);
		}
		else {
			#if TCPN_LOG_PACKETS >= 1
				if (pack && pack->opcode != 0) {
					struct in_addr	in;
					in.s_addr = GetrIP();
					CoutTimestamp(true);
					cout << ": Logging outgoing TCP packet. OPCode: 0x" << hex << setw(4) << setfill('0') << pack->opcode << dec << ", size: " << setw(5) << setfill(' ') << pack->size << " " << inet_ntoa(in) << ":" << GetrPort() << endl;
					#if TCPN_LOG_PACKETS == 2
						if (pack->size >= 32)
							DumpPacket(pack->pBuffer, 32);
						else
							DumpPacket(pack);
					#endif
					#if TCPN_LOG_PACKETS >= 3
						DumpPacket(pack);
					#endif
				}
			#endif
			ServerSendQueuePushEnd((uchar**) &tnps, tnps->size);
		}
	}
	return true;
}

bool TCPConnection::SendPacket(const TCPNetPacket_Struct* tnps) {
	LockMutex lock(&MState);
	if (RemoteID)
		return false;
	if (!Connected())
		return false;
	eTCPMode tmp = GetMode();
	if (tmp == modeTransition) {
		TCPNetPacket_Struct* tnps2 = (TCPNetPacket_Struct*) new uchar[tnps->size];
		memcpy(tnps2, tnps, tnps->size);
		InModeQueuePush(tnps2);
		return true;
	}
	if (GetMode() != modePacket)
		return false;
	#if TCPN_LOG_PACKETS >= 1
		if (tnps && tnps->opcode != 0) {
			struct in_addr	in;
			in.s_addr = GetrIP();
			CoutTimestamp(true);
			cout << ": Logging outgoing TCP NetPacket. OPCode: 0x" << hex << setw(4) << setfill('0') << tnps->opcode << dec << ", size: " << setw(5) << setfill(' ') << tnps->size << " " << inet_ntoa(in) << ":" << GetrPort();
			if (pOldFormat)
				cout << " (OldFormat)";
			cout << endl;
			#if TCPN_LOG_PACKETS == 2
				if (tnps->size >= 32)
					DumpPacket((const uchar*) tnps, 32);
				else
					DumpPacket((const uchar*) tnps, tnps->size);
			#endif
			#if TCPN_LOG_PACKETS >= 3
				DumpPacket((const uchar*) tnps, tnps->size);
			#endif
		}
	#endif
	ServerSendQueuePushEnd((const uchar*) tnps, tnps->size);
	return true;
}

bool TCPConnection::Send(const uchar* data, sint32 size) {
	if (!Connected())
		return false;
	if (GetMode() != modeConsole)
		return false;
	if (!size)
		return true;
	ServerSendQueuePushEnd(data, size);
	return true;
}

void TCPConnection::InModeQueuePush(TCPNetPacket_Struct* tnps) {
	MSendQueue.lock();
	InModeQueue.push(tnps);
	MSendQueue.unlock();
}

void TCPConnection::ServerSendQueuePushEnd(const uchar* data, sint32 size) {
	MSendQueue.lock();
	if (sendbuf == 0) {
		sendbuf = new uchar[size];
		sendbuf_size = size;
		sendbuf_used = 0;
	}
	else if (size > (sendbuf_size - sendbuf_used)) {
		sendbuf_size += size + 1024;
		uchar* tmp = new uchar[sendbuf_size];
		memcpy(tmp, sendbuf, sendbuf_used);
		delete sendbuf;
		sendbuf = tmp;
	}
	memcpy(&sendbuf[sendbuf_used], data, size);
	sendbuf_used += size;
	MSendQueue.unlock();
}

void TCPConnection::ServerSendQueuePushEnd(uchar** data, sint32 size) {
	MSendQueue.lock();
	if (sendbuf == 0) {
		sendbuf = *data;
		sendbuf_size = size;
		sendbuf_used = size;
		MSendQueue.unlock();
		*data = 0;
		return;
	}
	if (size > (sendbuf_size - sendbuf_used)) {
		sendbuf_size += size;
		uchar* tmp = new uchar[sendbuf_size];
		memcpy(tmp, sendbuf, sendbuf_used);
		delete sendbuf;
		sendbuf = tmp;
	}
	memcpy(&sendbuf[sendbuf_used], *data, size);
	sendbuf_used += size;
	MSendQueue.unlock();
	safe_delete(*data);
}

void TCPConnection::ServerSendQueuePushFront(const uchar* data, sint32 size) {
	MSendQueue.lock();
	if (sendbuf == 0) {
		sendbuf = new uchar[size];
		sendbuf_size = size;
		sendbuf_used = 0;
	}
	else if (size > (sendbuf_size - sendbuf_used)) {
		sendbuf_size += size;
		uchar* tmp = new uchar[sendbuf_size];
		memcpy(&tmp[size], sendbuf, sendbuf_used);
		delete sendbuf;
		sendbuf = tmp;
	}
	memcpy(sendbuf, data, size);
	sendbuf_used += size;
	MSendQueue.unlock();
}

bool TCPConnection::ServerSendQueuePop(uchar** data, sint32* size) {
	bool ret;
	if (!MSendQueue.trylock())
		return false;
	if (sendbuf) {
		*data = sendbuf;
		*size = sendbuf_used;
		sendbuf = 0;
		ret = true;
	}
	else {
		ret = false;
	}
	MSendQueue.unlock();
	return ret;
}

ServerPacket* TCPConnection::PopPacket() {
	ServerPacket* ret;
	if (!MOutQueueLock.trylock())
		return 0;
	ret = OutQueue.pop();
	MOutQueueLock.unlock();
	return ret;
}

char* TCPConnection::PopLine() {
	char* ret;
	if (!MOutQueueLock.trylock())
		return 0;
	ret = (char*) LineOutQueue.pop();
	MOutQueueLock.unlock();
	return ret;
}

void TCPConnection::OutQueuePush(ServerPacket* pack) {
	MOutQueueLock.lock();
	OutQueue.push(pack);
	MOutQueueLock.unlock();
}

void TCPConnection::LineOutQueuePush(char* line) {
	#if defined(GOTFRAGS) && 0
		if (strcmp(line, "**CRASHME**") == 0) {
			int i = 0;
			cout << (5 / i) << endl;
		}
	#endif
	if (strcmp(line, "**PACKETMODE**") == 0) {
		MSendQueue.lock();
		safe_delete(sendbuf);
		if (TCPMode == modeConsole)
			Send((const uchar*) "\0**PACKETMODE**\r", 16);
		TCPMode = modePacket;
		TCPNetPacket_Struct* tnps = 0;
		while ((tnps = InModeQueue.pop())) {
			SendPacket(tnps);
			delete tnps;
		}
		MSendQueue.unlock();
		delete line;
		return;
	}
	MOutQueueLock.lock();
	LineOutQueue.push(line);
	MOutQueueLock.unlock();
}

void TCPConnection::Disconnect(bool iSendRelayDisconnect) {
	if (connection_socket != INVALID_SOCKET && connection_socket != 0) {
		MState.lock();
		if (pState == TCPS_Connected || pState == TCPS_Disconnecting || pState == TCPS_Disconnected)
			SendData();
		pState = TCPS_Closing;
		MState.unlock();
		shutdown(connection_socket, 0x01);
		shutdown(connection_socket, 0x00);
#ifdef WIN32
		closesocket(connection_socket);
#else
		close(connection_socket);
#endif
		connection_socket = 0;
		rIP = 0;
		rPort = 0;
		ClearBuffers();
	}
	SetState(TCPS_Ready);
	if (RelayLink) {
		RelayLink->RemoveRelay(this, iSendRelayDisconnect);
		RelayLink = 0;
	}
}

bool TCPConnection::GetAsyncConnect() {
	bool ret;
	MAsyncConnect.lock();
	ret = pAsyncConnect;
	MAsyncConnect.unlock();
	return ret;
}

bool TCPConnection::SetAsyncConnect(bool iValue) {
	bool ret;
	MAsyncConnect.lock();
	ret = pAsyncConnect;
	pAsyncConnect = iValue;
	MAsyncConnect.unlock();
	return ret;
}

void TCPConnection::AsyncConnect(const char* irAddress, int16 irPort) {
	if (ConnectionType != Outgoing) {
		// If this code runs, we got serious problems
		// Crash and burn.
		ThrowError("TCPConnection::AsyncConnect() call on a Incomming connection object!");
		return;
	}
	if (GetState() != TCPS_Ready)
		return;
	MAsyncConnect.lock();
	if (pAsyncConnect) {
		MAsyncConnect.unlock();
		return;
	}
	pAsyncConnect = true;
	safe_delete(charAsyncConnect);
	charAsyncConnect = new char[strlen(irAddress) + 1];
	strcpy(charAsyncConnect, irAddress);
	rPort = irPort;
	MAsyncConnect.unlock();
	if (!pRunLoop) {
		pRunLoop = true;
#ifdef WIN32
		_beginthread(TCPConnectionLoop, 0, this);
#else
		pthread_t thread;
		pthread_create(&thread, NULL, TCPConnectionLoop, this);
#endif
	}
	return;
}

void TCPConnection::AsyncConnect(int32 irIP, int16 irPort) {
	if (ConnectionType != Outgoing) {
		// If this code runs, we got serious problems
		// Crash and burn.
		ThrowError("TCPConnection::AsyncConnect() call on a Incomming connection object!");
		return;
	}
	if (GetState() != TCPS_Ready)
		return;
	MAsyncConnect.lock();
	if (pAsyncConnect) {
		MAsyncConnect.unlock();
		return;
	}
	pAsyncConnect = true;
	safe_delete(charAsyncConnect);
	rIP = irIP;
	rPort = irPort;
	MAsyncConnect.unlock();
	if (!pRunLoop) {
		pRunLoop = true;
#ifdef WIN32
		_beginthread(TCPConnectionLoop, 0, this);
#else
		pthread_t thread;
		pthread_create(&thread, NULL, TCPConnectionLoop, this);
#endif
	}
	return;
}

bool TCPConnection::Connect(const char* irAddress, int16 irPort, char* errbuf) {
	if (errbuf)
		errbuf[0] = 0;
	int32 tmpIP = ResolveIP(irAddress);
	if (!tmpIP) {
		if (errbuf) {
#ifdef WIN32
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::Connect(): Couldnt resolve hostname. Error: %i", WSAGetLastError());
#else
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::Connect(): Couldnt resolve hostname. Error #%i: %s", errno, strerror(errno));
#endif
		}
		return false;
	}
	return Connect(tmpIP, irPort, errbuf);
}

bool TCPConnection::Connect(int32 in_ip, int16 in_port, char* errbuf) {
	if (errbuf)
		errbuf[0] = 0;
	if (ConnectionType != Outgoing) {
		// If this code runs, we got serious problems
		// Crash and burn.
		ThrowError("TCPConnection::Connect() call on a Incomming connection object!");
		return false;
	}
	MState.lock();
	if (pState == TCPS_Ready) {
		pState = TCPS_Connecting;
	}
	else {
		MState.unlock();
		SetAsyncConnect(false);
		return false;
	}
	MState.unlock();
	if (!pRunLoop) {
		pRunLoop = true;
#ifdef WIN32
		_beginthread(TCPConnectionLoop, 0, this);
#else
		pthread_t thread;
		pthread_create(&thread, NULL, TCPConnectionLoop, this);
#endif
	}

	connection_socket = INVALID_SOCKET;
    struct sockaddr_in	server_sin;
//    struct in_addr	in;

	if ((connection_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET || connection_socket == 0) {
#ifdef WIN32
		if (errbuf)
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::Connect(): Allocating socket failed. Error: %i", WSAGetLastError());
#else
		if (errbuf)
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::Connect(): Allocating socket failed. Error: %s", strerror(errno));
#endif
		SetState(TCPS_Ready);
		SetAsyncConnect(false);
		return false;
	}
	server_sin.sin_family = AF_INET;
	server_sin.sin_addr.s_addr = in_ip;
	server_sin.sin_port = htons(in_port);

	// Establish a connection to the server socket.
#ifdef WIN32
	if (connect(connection_socket, (PSOCKADDR) &server_sin, sizeof (server_sin)) == SOCKET_ERROR) {
		if (errbuf)
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::Connect(): connect() failed. Error: %i", WSAGetLastError());
		closesocket(connection_socket);
#else
	if (connect(connection_socket, (struct sockaddr *) &server_sin, sizeof (server_sin)) == SOCKET_ERROR) {
		if (errbuf)
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::Connect(): connect() failed. Error: %s", strerror(errno));
		close(connection_socket);
#endif
		connection_socket = 0;
		SetState(TCPS_Ready);
		SetAsyncConnect(false);
		return false;
	}	
	int bufsize = 64 * 1024; // 64kbyte recieve buffer, up from default of 8k
	setsockopt(connection_socket, SOL_SOCKET, SO_RCVBUF, (char*) &bufsize, sizeof(bufsize));
#ifdef WIN32
	unsigned long nonblocking = 1;
	ioctlsocket(connection_socket, FIONBIO, &nonblocking);
#else
	fcntl(connection_socket, F_SETFL, O_NONBLOCK);
#endif

	SetEcho(false);
	MSendQueue.lock();
	ClearBuffers();
	if (pOldFormat) {
		TCPMode = modePacket;
	}
	else if (TCPMode == modePacket || TCPMode == modeTransition) {
		TCPMode = modeTransition;
		sendbuf_size = 16;
		sendbuf_used = sendbuf_size;
		sendbuf = new uchar[sendbuf_size];
		memcpy(sendbuf, "\0**PACKETMODE**\r", 16);
	}
	MSendQueue.unlock();

	rIP = in_ip;
	rPort = in_port;
	SetState(TCPS_Connected);
	SetAsyncConnect(false);
	return true;
}

void TCPConnection::ClearBuffers() {
	LockMutex lock1(&MSendQueue);
	LockMutex lock2(&MOutQueueLock);
	LockMutex lock3(&MRunLoop);
	LockMutex lock4(&MState);
	safe_delete(recvbuf);
	safe_delete(sendbuf);
	ServerPacket* pack = 0;
	while ((pack = PopPacket()))
		delete pack;
	TCPNetPacket_Struct* tnps = 0;
	while ((tnps = InModeQueue.pop()))
		delete tnps;
	char* line = 0;
	while ((line = LineOutQueue.pop()))
		delete line;
	keepalive_timer->Start();
	timeout_timer->Start();
}

bool TCPConnection::CheckNetActive() {
	MState.lock();
	if (pState == TCPS_Connected || pState == TCPS_Disconnecting) {
		MState.unlock();
		return true;
	}
	MState.unlock();
	return false;
}

bool TCPConnection::Process() {
	char errbuf[TCPConnection_ErrorBufferSize];
	if (!CheckNetActive()) {
		if (ConnectionType == Outgoing) {
			if (GetAsyncConnect()) {
				if (charAsyncConnect)
					rIP = ResolveIP(charAsyncConnect);
				Connect(rIP, rPort);
			}
		}
		if (GetState() == TCPS_Disconnected) {
			Disconnect();
			return false;
		}
		else if (GetState() == TCPS_Connecting)
			return true;
		else
			return false;
	}
	if (!SendData(errbuf)) {
	    struct in_addr	in;
		in.s_addr = GetrIP();
		cout << inet_ntoa(in) << ":" << GetrPort() << ": " << errbuf << endl;
		return false;
	}
	if (!Connected())
		return false;
	if (!RecvData(errbuf)) {
	    struct in_addr	in;
		in.s_addr = GetrIP();
		cout << inet_ntoa(in) << ":" << GetrPort() << ": " << errbuf << endl;
		return false;
	}
	return true;
}

bool TCPConnection::RecvData(char* errbuf) {
	if (errbuf)
		errbuf[0] = 0;
	if (!Connected()) {
		return false;
	}

	int	status = 0;
	if (recvbuf == 0) {
		recvbuf = new uchar[5120];
		recvbuf_size = 5120;
		recvbuf_used = 0;
		recvbuf_echo = 0;
	}
	else if ((recvbuf_size - recvbuf_used) < 2048) {
		uchar* tmpbuf = new uchar[recvbuf_size + 5120];
		memcpy(tmpbuf, recvbuf, recvbuf_used);
		recvbuf_size += 5120;
		delete recvbuf;
		recvbuf = tmpbuf;
		if (recvbuf_size >= MaxTCPReceiveBuffferSize) {
			if (errbuf)
				snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::RecvData(): recvbuf_size >= MaxTCPReceiveBuffferSize");
			return false;
		}
	}

    status = recv(connection_socket, (char *) &recvbuf[recvbuf_used], (recvbuf_size - recvbuf_used), 0);

    if (status >= 1) {
#if TCPN_LOG_RAW_DATA_IN >= 1
		struct in_addr	in;
		in.s_addr = GetrIP();
		CoutTimestamp(true);
		cout << ": Read " << status << " bytes from network. (recvbuf_used = " << recvbuf_used << ") " << inet_ntoa(in) << ":" << GetrPort();
		if (pOldFormat)
			cout << " (OldFormat)";
		cout << endl;
	#if TCPN_LOG_RAW_DATA_IN == 2
		sint32 tmp = status;
		if (tmp > 32)
			tmp = 32;
		DumpPacket(&recvbuf[recvbuf_used], status);
	#elif TCPN_LOG_RAW_DATA_IN >= 3
		DumpPacket(&recvbuf[recvbuf_used], status);
	#endif
#endif
		recvbuf_used += status;
		timeout_timer->Start();
		if (!ProcessReceivedData(errbuf))
			return false;
    }
	else if (status == SOCKET_ERROR) {
#ifdef WIN32
		if (!(WSAGetLastError() == WSAEWOULDBLOCK)) {
			if (errbuf)
				snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::RecvData(): Error: %i", WSAGetLastError());
#else
		if (!(errno == EWOULDBLOCK)) {
			if (errbuf)
				snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::RecvData(): Error: %s", strerror(errno));
#endif
			return false;
		}
	}
	if ((TCPMode == modePacket || TCPMode == modeTransition) && timeout_timer->Check()) {
		if (errbuf)
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::RecvData(): Connection timeout");
		return false;
	}

	return true;
}


bool TCPConnection::GetEcho() {
	bool ret;
	MEcho.lock();
	ret = pEcho;
	MEcho.unlock();
	return ret;
}

void TCPConnection::SetEcho(bool iValue) {
	MEcho.lock();
	pEcho = iValue;
	MEcho.unlock();
}

bool TCPConnection::ProcessReceivedData(char* errbuf) {
	if (errbuf)
		errbuf[0] = 0;
	if (!recvbuf)
		return true;
	if (TCPMode == modePacket) {
		if (pOldFormat)
			return ProcessReceivedDataAsOldPackets(errbuf);
		else
			return ProcessReceivedDataAsPackets(errbuf);
	}
	else {
#if TCPN_DEBUG_Console >= 4
		if (recvbuf_used) {
			cout << "Starting Processing: recvbuf=" << recvbuf_used << endl;
			DumpPacket(recvbuf, recvbuf_used);
		}
#endif
		for (int i=0; i < recvbuf_used; i++) {
			if (GetEcho() && i >= recvbuf_echo) {
				Send(&recvbuf[i], 1);
				recvbuf_echo = i + 1;
			}
			switch(recvbuf[i]) {
			case 0: { // 0 is the code for clear buffer
					if (i==0) {
						recvbuf_used--;
						recvbuf_echo--;
						memcpy(recvbuf, &recvbuf[1], recvbuf_used);
						i = -1;
					} else {
						if (i == recvbuf_used) {
							safe_delete(recvbuf);
							i = -1;
						}
						else {
							uchar* tmpdel = recvbuf;
							recvbuf = new uchar[recvbuf_size];
							memcpy(recvbuf, &tmpdel[i+1], recvbuf_used-i);
							recvbuf_used -= i + 1;
							recvbuf_echo -= i + 1;
							delete tmpdel;
							i = -1;
						}
					}
#if TCPN_DEBUG_Console >= 5
					cout << "Removed 0x00" << endl;
					if (recvbuf_used) {
						cout << "recvbuf left: " << recvbuf_used << endl;
						DumpPacket(recvbuf, recvbuf_used);
					}
					else
						cout << "recbuf left: None" << endl;
#endif
					break;
				}
				case 10:
				case 13: // newline marker
				{
					if (i==0) { // empty line
						recvbuf_used--;
						recvbuf_echo--;
						memcpy(recvbuf, &recvbuf[1], recvbuf_used);
						i = -1;
					} else {
						char* line = new char[i+1];
						memset(line, 0, i+1);
						memcpy(line, recvbuf, i);
#if TCPN_DEBUG_Console >= 3
						cout << "Line Out: " << endl;
						DumpPacket((const uchar*) line, i);
#endif
						//line[i] = 0;
						uchar* tmpdel = recvbuf;
						recvbuf = new uchar[recvbuf_size];
						recvbuf_used -= i+1;
						recvbuf_echo -= i+1;
						memcpy(recvbuf, &tmpdel[i+1], recvbuf_used);
#if TCPN_DEBUG_Console >= 5
						cout << "i+1=" << i+1 << endl;
						if (recvbuf_used) {
							cout << "recvbuf left: " << recvbuf_used << endl;
							DumpPacket(recvbuf, recvbuf_used);
						}
						else
							cout << "recbuf left: None" << endl;
#endif
						delete tmpdel;
						if (strlen(line) > 0)
							LineOutQueuePush(line);
						else
							safe_delete(line);
						if (TCPMode == modePacket) {
							return ProcessReceivedDataAsPackets(errbuf);
						}
						i = -1;
					}
					break;
				}
				case 8: // backspace
				{
					if (i==0) { // nothin to backspace
						recvbuf_used--;
						recvbuf_echo--;
						memcpy(recvbuf, &recvbuf[1], recvbuf_used);
						i = -1;
					} else {
						uchar* tmpdel = recvbuf;
						recvbuf = new uchar[recvbuf_size];
						memcpy(recvbuf, tmpdel, i-1);
						memcpy(&recvbuf[i-1], &tmpdel[i+1], recvbuf_used-i);
						recvbuf_used -= 2;
						recvbuf_echo -= 2;
						delete tmpdel;
						i -= 2;
					}
					break;
				}
			}
		}
		if (recvbuf_used < 0)
			safe_delete(recvbuf);
	}
	return true;
}

bool TCPConnection::ProcessReceivedDataAsPackets(char* errbuf) {
	if (errbuf)
		errbuf[0] = 0;
	sint32 base = 0;
	sint32 size = 7;
	uchar* buffer;
	ServerPacket* pack = 0;
	while ((recvbuf_used - base) >= size) {
		TCPNetPacket_Struct* tnps = (TCPNetPacket_Struct*) &recvbuf[base];
		buffer = tnps->buffer;
		size = tnps->size;
		if (size >= MaxTCPReceiveBuffferSize) {
#if TCPN_DEBUG_Memory >= 1
			cout << "TCPConnection[" << GetID() << "]::ProcessReceivedDataAsPackets(): size[" << size << "] >= MaxTCPReceiveBuffferSize" << endl;
#endif
			if (errbuf)
				snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::ProcessReceivedDataAsPackets(): size >= MaxTCPReceiveBuffferSize");
			return false;
		}
		if ((recvbuf_used - base) >= size) {
			// ok, we got enough data to make this packet!
			pack = new ServerPacket;
			pack->size = size - sizeof(TCPNetPacket_Struct);
			// read headers
			pack->opcode = tnps->opcode;
			if (tnps->flags.compressed) {
				pack->compressed = true;
				pack->InflatedSize = *((sint32*)buffer);
				pack->size -= 4;
				buffer += 4;
			}
			if (tnps->flags.destination) {
				pack->destination = *((sint32*)buffer);
				pack->size -= 4;
				buffer += 4;
			}
			// end read headers
			if (pack->size > 0) {
				if (tnps->flags.compressed) {
					// Lets decompress the packet here
					pack->compressed = false;
					pack->pBuffer = new uchar[pack->InflatedSize];
					pack->size = InflatePacket(buffer, pack->size, pack->pBuffer, pack->InflatedSize);
				}
				else {
					pack->pBuffer = new uchar[pack->size];
					memcpy(pack->pBuffer, buffer, pack->size);
				}
			}
			if (pack->opcode == 0) {
				if (pack->size) {
					#if TCPN_DEBUG >= 2
						cout << "Received TCP Network layer packet" << endl;
					#endif
					ProcessNetworkLayerPacket(pack);
				}
				#if TCPN_DEBUG >= 5
					else {
						cout << "Received TCP keepalive packet. (opcode=0)" << endl;
					}
				#endif
				// keepalive, no need to process
				delete pack;
			}
			else {
				#if TCPN_LOG_PACKETS >= 1
					if (pack && pack->opcode != 0) {
						struct in_addr	in;
						in.s_addr = GetrIP();
						CoutTimestamp(true);
						cout << ": Logging incoming TCP packet. OPCode: 0x" << hex << setw(4) << setfill('0') << pack->opcode << dec << ", size: " << setw(5) << setfill(' ') << pack->size << " " << inet_ntoa(in) << ":" << GetrPort() << endl;
						#if TCPN_LOG_PACKETS == 2
							if (pack->size >= 32)
								DumpPacket(pack->pBuffer, 32);
							else
								DumpPacket(pack);
						#endif
						#if TCPN_LOG_PACKETS >= 3
							DumpPacket(pack);
						#endif
					}
				#endif
				if (RelayServer && Server && pack->destination) {
					TCPConnection* con = Server->GetConnection(pack->destination);
					if (!con) {
						#if TCPN_DEBUG >= 1
							cout << "Error relaying packet: con = 0" << endl;
						#endif
						delete pack;
					}
					else
						con->OutQueuePush(pack);
				}
				else
					OutQueuePush(pack);
			}
			base += size;
			size = 7;
		}
	}
	if (base != 0) {
		if (base >= recvbuf_used) {
			safe_delete(recvbuf);
		}
		else {
			uchar* tmpbuf = new uchar[recvbuf_size - base];
			memcpy(tmpbuf, &recvbuf[base], recvbuf_used - base);
			delete recvbuf;
			recvbuf = tmpbuf;
			recvbuf_used -= base;
			recvbuf_size -= base;
		}
	}
	return true;
}

bool TCPConnection::ProcessReceivedDataAsOldPackets(char* errbuf) {
	int32 base = 0;
	int32 size = 4;
	uchar* buffer;
	ServerPacket* pack = 0;
	while ((recvbuf_used - base) >= size) {
		buffer = &recvbuf[base];
		memcpy(&size, &buffer[2], 2);
		if (size >= MaxTCPReceiveBuffferSize) {
#if TCPN_DEBUG_Memory >= 1
			cout << "TCPConnection[" << GetID() << "]::ProcessReceivedDataAsPackets(): size[" << size << "] >= MaxTCPReceiveBuffferSize" << endl;
#endif
			if (errbuf)
				snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::ProcessReceivedDataAsPackets(): size >= MaxTCPReceiveBuffferSize");
			return false;
		}
		if ((recvbuf_used - base) >= size) {
			// ok, we got enough data to make this packet!
			pack = new ServerPacket;
			memcpy(&pack->opcode, &buffer[0], 2);
			pack->size = size - 4;
/*			if () { // TODO: Checksum or size check or something similar
				// Datastream corruption, get the hell outta here!
				delete pack;
				return false;
			}*/
			if (pack->size > 0) {
				pack->pBuffer = new uchar[pack->size];
				memcpy(pack->pBuffer, &buffer[4], pack->size);
			}
			if (pack->opcode == 0) {
				// keepalive, no need to process
				delete pack;
			}
			else {
				#if TCPN_LOG_PACKETS >= 1
					if (pack && pack->opcode != 0) {
						struct in_addr	in;
						in.s_addr = GetrIP();
						CoutTimestamp(true);
						cout << ": Logging incoming TCP OldPacket. OPCode: 0x" << hex << setw(4) << setfill('0') << pack->opcode << dec << ", size: " << setw(5) << setfill(' ') << pack->size << " " << inet_ntoa(in) << ":" << GetrPort() << endl;
						#if TCPN_LOG_PACKETS == 2
							if (pack->size >= 32)
								DumpPacket(pack->pBuffer, 32);
							else
								DumpPacket(pack);
						#endif
						#if TCPN_LOG_PACKETS >= 3
							DumpPacket(pack);
						#endif
					}
				#endif
				OutQueuePush(pack);
			}
			base += size;
			size = 4;
		}
	}
	if (base != 0) {
		if (base >= recvbuf_used) {
			safe_delete(recvbuf);
		}
		else {
			uchar* tmpbuf = new uchar[recvbuf_size - base];
			memcpy(tmpbuf, &recvbuf[base], recvbuf_used - base);
			delete recvbuf;
			recvbuf = tmpbuf;
			recvbuf_used -= base;
			recvbuf_size -= base;
		}
	}
	return true;
}

void TCPConnection::ProcessNetworkLayerPacket(ServerPacket* pack) {
	int8 opcode = pack->pBuffer[0];
	int8* data = &pack->pBuffer[1];
	switch (opcode) {
		case 0: {
			break;
		}
		case 1: { // Switch to RelayServer mode
			if (pack->size != 1) {
				SendNetErrorPacket("New RelayClient: wrong size, expected 1");
				break;
			}
			if (RelayServer) {
				SendNetErrorPacket("Switch to RelayServer mode when already in RelayServer mode");
				break;
			}
			if (RemoteID) {
				SendNetErrorPacket("Switch to RelayServer mode by a Relay Client");
				break;
			}
			if (ConnectionType != Incomming) {
				SendNetErrorPacket("Switch to RelayServer mode on outgoing connection");
				break;
			}
			#if TCPC_DEBUG >= 3
				struct in_addr	in;
				in.s_addr = GetrIP();
				cout << "Switching to RelayServer mode: " << inet_ntoa(in) << ":" << GetPort() << endl;
			#endif
			RelayServer = true;
			break;
		}
		case 2: { // New Relay Client
			if (!RelayServer) {
				SendNetErrorPacket("New RelayClient when not in RelayServer mode");
				break;
			}
			if (pack->size != 11) {
				SendNetErrorPacket("New RelayClient: wrong size, expected 11");
				break;
			}
			if (ConnectionType != Incomming) {
				SendNetErrorPacket("New RelayClient: illegal on outgoing connection");
				break;
			}
			TCPConnection* con = new TCPConnection(Server, this, *((int32*) data), *((int32*) &data[4]), *((int16*) &data[8]));
			Server->AddConnection(con);
			RelayCount++;
			break;
		}
		case 3: { // Delete Relay Client
			if (!RelayServer) {
				SendNetErrorPacket("Delete RelayClient when not in RelayServer mode");
				break;
			}
			if (pack->size != 5) {
				SendNetErrorPacket("Delete RelayClient: wrong size, expected 5");
				break;
			}
			TCPConnection* con = Server->GetConnection(*((int32*)data));
			if (con) {
				if (ConnectionType == Incomming) {
					if (con->GetRelayLink() != this) {
						SendNetErrorPacket("Delete RelayClient: RelayLink != this");
						break;
					}
				}
				con->Disconnect(false);
			}
			break;
		}
		case 255: {
			#if TCPC_DEBUG >= 1
				struct in_addr	in;
				in.s_addr = GetrIP();
				cout "Received NetError: '";
				if (pack->size > 1)
					cout << (char*) data;
				cout << "': " << inet_ntoa(in) << ":" << GetPort() << endl;
			#endif
			break;
		}
	}
}

void TCPConnection::SendNetErrorPacket(const char* reason) {
	#if TCPC_DEBUG >= 1
		struct in_addr	in;
		in.s_addr = GetrIP();
		cout "NetError: '";
		if (reason)
			cout << reason;
		cout << "': " << inet_ntoa(in) << ":" << GetPort() << endl;
	#endif
	ServerPacket* pack = new ServerPacket(0);
	pack->size = 1;
	if (reason)
		pack->size += strlen(reason) + 1;
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	pack->pBuffer[0] = 255;
	strcpy((char*) &pack->pBuffer[1], reason);
	SendPacket(pack);
	delete pack;
}

void TCPConnection::RemoveRelay(TCPConnection* relay, bool iSendRelayDisconnect) {
	if (iSendRelayDisconnect) {
		ServerPacket* pack = new ServerPacket(0, 5);
		pack->pBuffer[0] = 3;
		*((int32*) &pack->pBuffer[1]) = relay->GetRemoteID();
		SendPacket(pack);
		delete pack;
	}
	RelayCount--;
}

bool TCPConnection::SendData(char* errbuf) {
	if (errbuf)
		errbuf[0] = 0;
	/************ Get first send packet on queue and send it! ************/
	uchar* data = 0;
	sint32 size = 0;
	int status = 0;
	if (ServerSendQueuePop(&data, &size)) {
#ifdef WIN32
		status = send(connection_socket, (const char *) data, size, 0);
#else
		status = send(connection_socket, data, size, MSG_NOSIGNAL);
		if(errno==EPIPE) status = SOCKET_ERROR;
#endif
		if (status >= 1) {
#if TCPN_LOG_RAW_DATA_OUT >= 1
			struct in_addr	in;
			in.s_addr = GetrIP();
			CoutTimestamp(true);
			cout << ": Wrote " << status << " bytes to network. " << inet_ntoa(in) << ":" << GetrPort();
			if (pOldFormat)
				cout << " (OldFormat)";
			cout << endl;
	#if TCPN_LOG_RAW_DATA_OUT == 2
			sint32 tmp = status;
			if (tmp > 32)
				tmp = 32;
			DumpPacket(data, status);
	#elif TCPN_LOG_RAW_DATA_OUT >= 3
			DumpPacket(data, status);
	#endif
#endif
			keepalive_timer->Start();
			if (status < (signed)size) {
#if TCPN_LOG_RAW_DATA_OUT >= 1
				struct in_addr	in;
				in.s_addr = GetrIP();
				CoutTimestamp(true);
				cout << ": Pushed " << (size - status) << " bytes back onto the send queue. " << inet_ntoa(in) << ":" << GetrPort();
				if (pOldFormat)
					cout << " (OldFormat)";
				cout << endl;
#endif
				// If there's network congestion, the number of bytes sent can be less than
				// what we tried to give it... Push the extra back on the queue for later
				ServerSendQueuePushFront(&data[status], size - status);
			}
			else if (status > (signed)size) {
				ThrowError("TCPConnection::SendData(): WTF! status > size");
				return false;
			}
			// else if (status == size) {}
		}
		else {
			ServerSendQueuePushFront(data, size);
		}

		delete data;
		if (status == SOCKET_ERROR) {
#ifdef WIN32
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
			if (errno != EWOULDBLOCK) {
#endif
				if (errbuf) {
#ifdef WIN32
					snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::SendData(): send(): Errorcode: %i", WSAGetLastError());
#else
					snprintf(errbuf, TCPConnection_ErrorBufferSize, "TCPConnection::SendData(): send(): Errorcode: %s", strerror(errno));
#endif
				}
				return false;
			}
		}
	}
    if (TCPMode == modePacket && keepalive_timer->Check()) {
		ServerPacket* pack = new ServerPacket(0, 0);
		SendPacket(pack);
		delete pack;
		#if TCPN_DEBUG >= 5
			cout << "Sending TCP keepalive packet. (timeout=" << timeout_timer->GetRemainingTime() << " remaining)" << endl;
		#endif
    }
	return true;
}

#ifdef WIN32
void TCPConnectionLoop(void* tmp) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#else
void* TCPConnectionLoop(void* tmp) {
#endif
	if (tmp == 0) {
		ThrowError("TCPConnectionLoop(): tmp = 0!");
#ifdef WIN32
		return;
#else
		return 0;
#endif
	}
	TCPConnection* tcpc = (TCPConnection*) tmp;
	tcpc->MLoopRunning.lock();
	while (tcpc->RunLoop()) {
		Sleep(1);
		if (tcpc->GetState() != TCPS_Ready) {
			if (!tcpc->Process()) {
				tcpc->Disconnect();
			}
		}
		else if (tcpc->GetAsyncConnect()) {
			if (tcpc->charAsyncConnect)
				tcpc->Connect(tcpc->charAsyncConnect, tcpc->GetrPort());
			else
				tcpc->Connect(tcpc->GetrIP(), tcpc->GetrPort());
			tcpc->SetAsyncConnect(false);
		}
		else
			Sleep(10);
	}
	tcpc->MLoopRunning.unlock();
#ifdef WIN32
	_endthread();
#else
	return 0;
#endif
}

bool TCPConnection::RunLoop() {
	bool ret;
	MRunLoop.lock();
	ret = pRunLoop;
	MRunLoop.unlock();
	return ret;
}





TCPServer::TCPServer(int16 in_port, bool iOldFormat) {
	NextID = 1;
	pPort = in_port;
	sock = 0;
	pOldFormat = iOldFormat;
	list = new LinkedList<TCPConnection*>;
	pRunLoop = true;
#ifdef WIN32
	_beginthread(TCPServerLoop, 0, this);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, &TCPServerLoop, this);
#endif
}

TCPServer::~TCPServer() {
	MRunLoop.lock();
	pRunLoop = false;
	MRunLoop.unlock();
	MLoopRunning.lock();
	MLoopRunning.unlock();

	while (NewQueue.pop()); // the objects are deleted with the list, clear this queue so it doesnt try to delete them again
	delete list;
}

bool TCPServer::RunLoop() {
	bool ret;
	MRunLoop.lock();
	ret = pRunLoop;
	MRunLoop.unlock();
	return ret;
}

#ifdef WIN32
void TCPServerLoop(void* tmp) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#else
void* TCPServerLoop(void* tmp) {
#endif
	if (tmp == 0) {
		ThrowError("TCPServerLoop(): tmp = 0!");
#ifdef WIN32
		return;
#else
		return 0;
#endif
	}
	TCPServer* tcps = (TCPServer*) tmp;
	tcps->MLoopRunning.lock();
	while (tcps->RunLoop()) {
		Sleep(1);
		tcps->Process();
	}
	tcps->MLoopRunning.unlock();
#ifdef WIN32
	return;
#else
	return 0;
#endif
}

void TCPServer::Process() {
	CheckInQueue();
	ListenNewConnections();
	LinkedListIterator<TCPConnection*> iterator(*list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsFree() && (!iterator.GetData()->CheckNetActive())) {
			#if EQN_DEBUG >= 4
				cout << "EQNetwork Connection deleted." << endl;
			#endif
			iterator.RemoveCurrent();
		}
		else { 
			if (!iterator.GetData()->Process())
				iterator.GetData()->Disconnect();
			iterator.Advance();
		}
	}
}

void TCPServer::ListenNewConnections() {
    SOCKET tmpsock;
    struct sockaddr_in	from;
    struct in_addr	in;
    unsigned int	fromlen;
    unsigned short	port;
	
	TCPConnection* con;

    from.sin_family = AF_INET;
    fromlen = sizeof(from);
	LockMutex lock(&MSock);
	if (!sock)
		return;

	// Check for pending connects
#ifdef WIN32
	unsigned long nonblocking = 1;
	while ((tmpsock = accept(sock, (struct sockaddr*) &from, (int *) &fromlen)) != INVALID_SOCKET) {
		ioctlsocket (tmpsock, FIONBIO, &nonblocking);
#else
	while ((tmpsock = accept(sock, (struct sockaddr*) &from, &fromlen)) != INVALID_SOCKET) {
		fcntl(tmpsock, F_SETFL, O_NONBLOCK);
#endif
		int bufsize = 64 * 1024; // 64kbyte recieve buffer, up from default of 8k
		setsockopt(tmpsock, SOL_SOCKET, SO_RCVBUF, (char*) &bufsize, sizeof(bufsize));
		port = from.sin_port;
		in.s_addr = from.sin_addr.s_addr;

		// New TCP connection
		con = new TCPConnection(this, tmpsock, in.s_addr, ntohs(from.sin_port), pOldFormat);
		#if TCPN_DEBUG >= 1
			cout << "New TCP connection: " << inet_ntoa(in) << ":" << con->GetrPort() << endl;
		#endif
		AddConnection(con);
	}
}

bool TCPServer::Open(int16 in_port, char* errbuf) {
	if (errbuf)
		errbuf[0] = 0;
	LockMutex lock(&MSock);
	if (sock != 0) {
		if (errbuf)
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "Listening socket already open");
		return false;
	}
	if (in_port != 0) {
		pPort = in_port;
	}

#ifdef WIN32
	SOCKADDR_IN address;
	unsigned long nonblocking = 1;
#else
	struct sockaddr_in address;
#endif
	int reuse_addr = 1;

//	Setup internet address information.  
//	This is used with the bind() call
	memset((char *) &address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(pPort);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

//	Setting up TCP port for new TCP connections
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		if (errbuf)
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "socket(): INVALID_SOCKET");
		return false;
	}

// Quag: dont think following is good stuff for TCP, good for UDP
// Mis: SO_REUSEADDR shouldn't be a problem for tcp--allows you to restart
// without waiting for conns in TIME_WAIT to die
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse_addr, sizeof(reuse_addr));


	if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
#ifdef WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		sock = 0;
		if (errbuf)
			sprintf(errbuf, "bind(): <0");
		return false;
	}

	int bufsize = 64 * 1024; // 64kbyte recieve buffer, up from default of 8k
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*) &bufsize, sizeof(bufsize));
#ifdef WIN32
	ioctlsocket (sock, FIONBIO, &nonblocking);
#else
	fcntl(sock, F_SETFL, O_NONBLOCK);
#endif

	if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
#ifdef WIN32
		closesocket(sock);
		if (errbuf)
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "listen() failed, Error: %d", WSAGetLastError());
#else
		close(sock);
		if (errbuf)
			snprintf(errbuf, TCPConnection_ErrorBufferSize, "listen() failed, Error: %s", strerror(errno));
#endif
		sock = 0;
		return false;
	}

	return true;
}

void TCPServer::Close() {
	LockMutex lock(&MSock);
	if (sock) {
#ifdef WIN32
		closesocket(sock);
#else
		close(sock);
#endif
	}
	sock = 0;
}

bool TCPServer::IsOpen() {
	MSock.lock();
	bool ret = (bool) (sock != 0);
	MSock.unlock();
	return ret;
}

TCPConnection* TCPServer::NewQueuePop() {
	TCPConnection* ret;
	MNewQueue.lock();
	ret = NewQueue.pop();
	MNewQueue.unlock();
	return ret;
}

void TCPServer::AddConnection(TCPConnection* con) {
	list->Append(con);
	MNewQueue.lock();
	NewQueue.push(con);
	MNewQueue.unlock();
}

TCPConnection* TCPServer::GetConnection(int32 iID) {
	LinkedListIterator<TCPConnection*> iterator(*list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetID() == iID)
			return iterator.GetData();
		iterator.Advance();
	}
	return 0;
}

void TCPServer::SendPacket(ServerPacket* pack) {
	TCPConnection::TCPNetPacket_Struct* tnps = TCPConnection::MakePacket(pack);
	SendPacket(&tnps);
}

void TCPServer::SendPacket(TCPConnection::TCPNetPacket_Struct** tnps) {
	MInQueue.lock();
	InQueue.push(*tnps);
	MInQueue.unlock();
	tnps = 0;
}

void TCPServer::CheckInQueue() {
	LinkedListIterator<TCPConnection*> iterator(*list);	
	TCPConnection::TCPNetPacket_Struct* tnps = 0;

	while (( tnps = InQueuePop() )) {
		iterator.Reset();
		while(iterator.MoreElements()) {
			if (iterator.GetData()->GetMode() != modeConsole && iterator.GetData()->GetRemoteID() == 0)
				iterator.GetData()->SendPacket(tnps);
			iterator.Advance();
		}
		delete tnps;
	}
}

TCPConnection::TCPNetPacket_Struct* TCPServer::InQueuePop() {
	TCPConnection::TCPNetPacket_Struct* ret;
	MInQueue.lock();
	ret = InQueue.pop();
	MInQueue.unlock();
	return ret;
}

