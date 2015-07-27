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
#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H
/*
  Parent classes for interserver TCP Communication.
  -Quagmire
*/

#ifdef WIN32
	#define snprintf	_snprintf
	//#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp

	#include <process.h>
#else
	#include <pthread.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>
	#include <fcntl.h>
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#include "../common/unix.h"

#endif

#include "../common/types.h"
#include "../common/Mutex.h"
#include "../common/linked_list.h"
#include "../common/queue.h"
#include "../common/servertalk.h"
#include "../common/timer.h"
#include "../common/MiscFunctions.h"

class TCPServer;

#define TCPConnection_ErrorBufferSize	1024
#define MaxTCPReceiveBuffferSize		524288

#define TCPS_Ready			0
#define TCPS_Connecting		1
#define TCPS_Connected		100
#define TCPS_Disconnecting	200
#define TCPS_Disconnected	201
#define TCPS_Closing		250
#define TCPS_Error			255

#ifndef DEF_eConnectionType
#define DEF_eConnectionType
enum eConnectionType {Incomming, Outgoing};
#endif

#ifdef WIN32
	void TCPServerLoop(void* tmp);
	void TCPConnectionLoop(void* tmp);
#else
	void* TCPServerLoop(void* tmp);
	void* TCPConnectionLoop(void* tmp);
#endif

enum eTCPMode { modeConsole, modeTransition, modePacket };
class TCPConnection {
public:
#pragma pack(1)
	struct TCPNetPacket_Struct {
		int32	size;
		struct {
			int8
				compressed : 1,
				destination : 1,
				flag3 : 1,
				flag4 : 1,
				flag5 : 1,
				flag6 : 1,
				flag7 : 1,
				flag8 : 1;
		} flags;
		int16	opcode;
		uchar	buffer[0];
	};
#pragma pack()

	static TCPNetPacket_Struct* MakePacket(const ServerPacket* pack, int32 iDestination = 0);
	static SPackSendQueue* TCPConnection::MakeOldPacket(const ServerPacket* pack);

	TCPConnection(TCPServer* iServer, SOCKET iSock, int32 irIP, int16 irPort, bool iOldFormat = false);
	TCPConnection(bool iOldFormat = false, TCPServer* iRelayServer = 0, eTCPMode iMode = modePacket);	// for outgoing connections
	TCPConnection(TCPServer* iServer, TCPConnection* iRelayLink, int32 iRemoteID, int32 irIP, int16 irPort);				// for relay connections
	virtual ~TCPConnection();

	// Functions for outgoing connections
	bool			Connect(const char* irAddress, int16 irPort, char* errbuf = 0);
	bool			Connect(int32 irIP, int16 irPort, char* errbuf = 0);
	void			AsyncConnect(const char* irAddress, int16 irPort);
	void			AsyncConnect(int32 irIP, int16 irPort);
	virtual void	Disconnect(bool iSendRelayDisconnect = true);

	virtual bool	SendPacket(const ServerPacket* pack, int32 iDestination = 0);
	virtual bool	SendPacket(const TCPNetPacket_Struct* tnps);
	bool			Send(const uchar* data, sint32 size);

	char*			PopLine();
	ServerPacket*	PopPacket(); // OutQueuePop()
	inline int32	GetrIP()			{ return rIP; }
	inline int16	GetrPort()			{ return rPort; }
	virtual int8	GetState();
	eTCPMode		GetMode()			{ return TCPMode; }
	inline bool		Connected()		{ return (GetState() == TCPS_Connected); }
	inline bool		ConnectReady()	{ return (bool) (GetState() == TCPS_Ready && ConnectionType == Outgoing); }
	void			Free();		// Inform TCPServer that this connection object is no longer referanced

	inline int32	GetID()			{ return id; }
	inline bool		IsRelayServer() { return RelayServer; }
	inline int32	GetRemoteID()	{ return RemoteID; }
	inline TCPConnection* GetRelayLink()	{ return RelayLink; }

	bool			GetEcho();
	void			SetEcho(bool iValue);
protected:
	friend class TCPServer;
	virtual bool	Process();
	void			SetState(int8 iState);
	inline bool		IsFree() { return pFree; }
	bool			CheckNetActive();

#ifdef WIN32
	friend void TCPConnectionLoop(void* tmp);
#else
	friend void* TCPConnectionLoop(void* tmp);
#endif
	SOCKET			sock;
	bool			RunLoop();
	Mutex			MLoopRunning;
	Mutex	MAsyncConnect;
	bool	GetAsyncConnect();
	bool	SetAsyncConnect(bool iValue);
	char*	charAsyncConnect;
	
#ifdef WIN32
	friend class TCPConnection;
#endif
	void	OutQueuePush(ServerPacket* pack);
	void	RemoveRelay(TCPConnection* relay, bool iSendRelayDisconnect);
private:
	void	ProcessNetworkLayerPacket(ServerPacket* pack);
	void	SendNetErrorPacket(const char* reason = 0);
	TCPServer*		Server;
	TCPConnection*	RelayLink;
	int32			RemoteID;
	sint32			RelayCount;

	bool pOldFormat;
	bool SendData(char* errbuf = 0);
	bool RecvData(char* errbuf = 0);
	bool ProcessReceivedData(char* errbuf = 0);
	bool ProcessReceivedDataAsPackets(char* errbuf = 0);
	bool ProcessReceivedDataAsOldPackets(char* errbuf = 0);
	void ClearBuffers();

	bool	pAsyncConnect;

	eConnectionType	ConnectionType;
	eTCPMode		TCPMode;
	bool	RelayServer;
	Mutex	MRunLoop;
	bool	pRunLoop;

	SOCKET	connection_socket;
	int32	id;
	int32	rIP;
	int16	rPort; // host byte order
	bool	pFree;

	Mutex	MState;
	int8	pState;

	void	LineOutQueuePush(char* line);
	MyQueue<char> LineOutQueue;
	MyQueue<ServerPacket> OutQueue;
	Mutex	MOutQueueLock;

	Timer*	keepalive_timer;
	Timer*	timeout_timer;

	uchar*	recvbuf;
	sint32	recvbuf_size;
	sint32	recvbuf_used;

	sint32	recvbuf_echo;
	bool	pEcho;
	Mutex	MEcho;

	void	InModeQueuePush(TCPNetPacket_Struct* tnps);
	MyQueue<TCPNetPacket_Struct> InModeQueue;
	Mutex	MSendQueue;
	uchar*	sendbuf;
	sint32	sendbuf_size;
	sint32	sendbuf_used;
	bool	ServerSendQueuePop(uchar** data, sint32* size);
	void	ServerSendQueuePushEnd(const uchar* data, sint32 size);
	void	ServerSendQueuePushEnd(uchar** data, sint32 size);
	void	ServerSendQueuePushFront(const uchar* data, sint32 size);
};

class TCPServer {
public:
	TCPServer(int16 iPort = 0, bool iOldFormat = false);
	virtual ~TCPServer();

	bool	Open(int16 iPort = 0, char* errbuf = 0);			// opens the port
	void	Close();						// closes the port
	bool	IsOpen();
	inline int16	GetPort()		{ return pPort; }

	TCPConnection* NewQueuePop();

	void	SendPacket(ServerPacket* pack);
	void	SendPacket(TCPConnection::TCPNetPacket_Struct** tnps);
protected:
#ifdef WIN32
	friend void TCPServerLoop(void* tmp);
#else
	friend void* TCPServerLoop(void* tmp);
#endif
	void	Process();
	bool	RunLoop();
	Mutex	MLoopRunning;

	friend class TCPConnection;
	inline int32	GetNextID() { return NextID++; }
	void			AddConnection(TCPConnection* con);
	TCPConnection*	GetConnection(int32 iID);
private:
	void	ListenNewConnections();

	int32	NextID;
	bool	pOldFormat;

	Mutex	MRunLoop;
	bool	pRunLoop;

	Mutex	MSock;
	SOCKET	sock;
	int16	pPort;

	Mutex	MNewQueue;
	MyQueue<TCPConnection>		NewQueue;

	void	CheckInQueue();
	Mutex	MInQueue;
	TCPConnection::TCPNetPacket_Struct*	InQueuePop();
	MyQueue<TCPConnection::TCPNetPacket_Struct>	InQueue;

	LinkedList<TCPConnection*>*	list;
};
#endif
