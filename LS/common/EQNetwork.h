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
#ifndef EQNETWORK_H
#define EQNETWORK_H

#include "../common/debug.h"

#include <string.h>

#include "../common/types.h"
#include "../common/timer.h"
#include "../common/linked_list.h"
#include "../common/queue.h"
#include "../common/Mutex.h"
#include "../common/packet_functions.h"

#define EQNC_TIMEOUT	60000
#define KA_TIMER		500 /* keeps the lag bar constant */
#define MAX_HEADER_SIZE	39 // Quag: 39 is the max header + opcode + crc32 + unknowns size

class EQNetworkServer;
class EQNetworkConnection;
class EQNetworkPacket;
class EQNetworkFragmentGroupList;
class EQNetworkFragmentGroup;
typedef EQNetworkServer				EQNServer;
typedef EQNetworkConnection			EQNConnection;
typedef EQNetworkPacket				EQNPacket;
typedef EQNetworkFragmentGroupList	EQNFragmentGroupList;
typedef EQNetworkFragmentGroup		EQNFragmentGroup;

#define SAFE_DELETE(d) if(d){delete[] d; d=0;}

enum eappCompressed { appNormal, appInflated, appDeflated };
class APPLAYER {
public:
	APPLAYER(int16 in_opcode = 0, int32 in_size = 0) {
		size = in_size;
		opcode = in_opcode;
		priority = 0;
		compressed = appNormal;
		if (size == 0) {
			pBuffer = 0;
		}
		else {
			pBuffer = new uchar[size];
			memset(pBuffer, 0, size);
		}
	}
	~APPLAYER() { SAFE_DELETE(pBuffer); }
	APPLAYER* Copy() const {
		if (this == 0) {
			return 0;
		}
		APPLAYER* ret = new APPLAYER(this->opcode, this->size);
		if (this->size)
			memcpy(ret->pBuffer, this->pBuffer, this->size);
		ret->priority = this->priority;
		ret->compressed = this->compressed;
		return ret;
	}
	bool Deflate() {
		if ((!this) || (!this->pBuffer) || (!this->size))
			return false;
		if (compressed != appNormal)
			return false;
		uchar* tmp = new uchar[this->size + 128];
		int32 tmpsize = DeflatePacket(this->pBuffer, this->size, tmp, this->size + 128);
		if (!tmpsize) {
			safe_delete(tmp);
			return false;
		}
		compressed = appDeflated;
		this->size = tmpsize;
		uchar* tmpdel = this->pBuffer;
		this->pBuffer = tmp;
		safe_delete(tmpdel);
		return true;
	}
	bool Inflate(int32 bufsize = 0) {
		if ((!this) || (!this->pBuffer) || (!this->size))
			return false;
		if (compressed != appNormal)
			return false;
		if (!bufsize)
			bufsize = this->size * 20;
		uchar* tmp = new uchar[bufsize];
		int32 tmpsize = InflatePacket(this->pBuffer, this->size, tmp, bufsize);
		if (!tmpsize) {
			safe_delete(tmp);
			return false;
		}
		compressed = appInflated;
		this->size = tmpsize;
		uchar* tmpdel = this->pBuffer;
		this->pBuffer = tmp;
		safe_delete(tmpdel);
		return true;
	}


	int32  size;
	int16  opcode;
	uchar* pBuffer;
	int8 priority;
	eappCompressed compressed;
};

    /************ Contain ack stuff ************/
struct ACK_INFO {
	ACK_INFO() { dwARQ = 0; dbASQ_high = dbASQ_low = 0; dwGSQ = 0; }

	int16   dwARQ; //Current request ack
	int16   dbASQ_high : 8,
			dbASQ_low  : 8;
	int16   dwGSQ; //Main sequence number SHORT#2
};

#define ACK_ONLY_SIZE   6
#define ACK_ONLY    0x400

/************ PACKETS ************/
struct EQPACKET_HDR_INFO {
    int8    
        a0_Unknown  :   1,
        a1_ARQ      :   1,
        a2_Closing  :   1,
        a3_Fragment :   1,

        a4_ASQ      :   1,
        a5_SEQStart :   1,
        a6_Closing  :   1,
        a7_SEQEnd   :   1;
    
    int8
        b0_SpecARQ  :   1,
        b1_Unknown  :   1,
        b2_ARSP     :   1,
        b3_Unknown  :   1,

        b4_Unknown  :   1,
        b5_Unknown  :   1,
        b6_Unknown  :   1,
        b7_Unknown  :   1;
};

struct FRAGMENT_INFO {
    int16 dwSeq;
    int16 dwCurr;
    int16 dwTotal;
};

struct InQueue_Struct {
	APPLAYER* app;
	bool ackreq;
};

#ifndef DEF_eConnectionType
#define DEF_eConnectionType
enum eConnectionType {Incomming, Outgoing};
#endif

#ifdef WIN32
	void EQNetworkServerLoop(void* tmp);
	void EQNetworkConnectionLoop(void* tmp);
#else
	void* EQNetworkServerLoop(void* tmp);
	void* EQNetworkConnectionLoop(void* tmp);
#endif
class EQNetworkServer {
public:
	EQNetworkServer(int16 iPort = 0);
	virtual ~EQNetworkServer();

	bool	Open(int16 iPort = 0);			// opens the port
	void	Close();	// closes the port
	void	KillAll();						// kills all clients
	inline int16	GetPort()		{ return pPort; }

	EQNetworkConnection* NewQueuePop();
protected:
#ifdef WIN32
	friend void EQNetworkServerLoop(void* tmp);
#else
	friend void* EQNetworkServerLoop(void* tmp);
#endif
	void		Process();
	bool		IsOpen();
	void		SetOpen(bool iOpen);
	bool		RunLoop;
	Mutex		MLoopRunning;
private:
	void		RecvData(uchar* data, int32 size, int32 irIP, int16 irPort);
#ifdef WIN32
	SOCKET	sock;
#else
	int		sock;
#endif
	int16	pPort;
	bool	pOpen;
	Mutex	MNewQueue;
	Mutex	MOpen;

	LinkedList<EQNetworkConnection*>*	list;
	MyQueue<EQNetworkConnection>		NewQueue;
};

class EQNetworkFragmentGroupList {
public:
	EQNetworkFragmentGroupList();
	virtual ~EQNetworkFragmentGroupList();

	EQNetworkFragmentGroup*	GotPacket(EQNetworkPacket* pack);
	EQNetworkFragmentGroup*	FindGroup(int16 fragseq);
	void					DeleteGroup(int16 fragseq);
	void					CheckTimers();
private:
	LinkedList<EQNetworkFragmentGroup*>	list;
	Timer* timeout_check_timer;
};

#define EQNC_Init		0		// Just made, waiting for orders
#define EQNC_Active		100		// Up and running
#define	EQNC_Closing	101		// Received closing bits, about to die
#define	EQNC_Closed		102		// Closing bits sent, no more packets may be added to the queue
#define	EQNC_Finished	150		// Shutdown was graceful, last packets sent, ready to be deleted
#define EQNC_Error		250		// Error has occured, no longer functional (delete when free)
class EQNetworkConnection {
public:
	EQNetworkConnection(int32 irIP, int16 irPort);
	EQNetworkConnection();				// for outgoing connections
	virtual ~EQNetworkConnection();

	// Functions for outgoing connections
	bool			OpenSock(char* irAddress, int16 irPort, char* errbuf = 0);
	bool			OpenSock(int32 irIP, int16 irPort);
	void			CloseSock();

	void			QueuePacket(const APPLAYER* app, bool ackreq = true);	// InQueuePush()
	void			PacketQueueFlush(APPLAYER* app);						// QueueFlush() on zone
	
	void			FastQueuePacket(APPLAYER** app, bool ackreq = true);	// Faster than QueuePacket, but you cant call twice on the same app
	APPLAYER*		PopPacket(); // OutQueuePop()
	inline int32	GetrIP()		{ return rIP; }
	inline int16	GetrPort()		{ return rPort; }
	int8			GetState();
	bool			CheckActive();
	void			Close();
	void			Free();		// Inform EQNetworkServer that this connection object is no longer referanced
	void			RemoveData();

	void			SetDataRate(float in_datarate)	{ datarate_sec = (sint32) (in_datarate * 1024); datarate_tic = datarate_sec / 10; dataflow = 0; datahigh = 0; } // conversion from kb/sec to byte/100ms, byte/1000ms
	float			GetDataRate()					{ return (float)datarate_sec / 1024; } // conversion back to kb/second
	inline bool		DataQueueFull()					{ return (dataflow > datarate_sec); }
	inline sint32	GetDataFlow()					{ return dataflow; }
	inline sint32	GetDataHigh()					{ return datahigh; }
	void RemovePacket(EQNetworkPacket* pack);
	void PacketPriority();
	void			RecvData(uchar* data, int32 size);
	void			OutQueuePush(APPLAYER* app);

protected:
	friend class EQNetworkServer;
#ifdef WIN32
	void			Process(SOCKET sock);
#else
	void			Process(int sock);
#endif
	void			SetState(int8 iState);
	inline bool		IsMine(int32 irIP, int16 irPort) { return (rIP == irIP && rPort == irPort); }
	inline bool		IsFree() { return pFree; }
	bool			CheckNetActive();

	friend class EQNetworkPacket;
	ACK_INFO		CACK;
	ACK_INFO		SACK;
	Timer*			no_ack_sent_timer;

#ifdef WIN32
	friend void EQNetworkConnectionLoop(void* tmp);
	SOCKET			outsock;
#else
	friend void* EQNetworkConnectionLoop(void* tmp);
	int				outsock;
#endif
	bool			RunLoop;
	Mutex			MLoopRunning;
	Mutex			MSocketLock;
	void			DoRecvData();

private:
	eConnectionType	ConnectionType;
	bool			ProcessPacket(EQNetworkPacket* pack, bool from_buffer);
	void			IncomingARSP(int16 arsp);
	void			IncomingARQ(int16 arq);
	void			CheckBufferedPackets();

	void			MakeEQPacket(APPLAYER* app, bool ackreq = true);
	InQueue_Struct*	InQueuePop();

	int32	rIP;
	int16	rPort; // network byte order

	sint32	dataflow;		// incremented by bytes sent with every outgoing packet, decremented by datarate_tic every 100ms
	sint32	datahigh;		// used to delay resends if dataflow has been near max
	int8	pState;
	bool	pFree;
	EQNetworkFragmentGroupList		fraglist;
	MyQueue<InQueue_Struct>			InQueue;
	MyQueue<APPLAYER>				OutQueue;
	LinkedList<EQNetworkPacket*>	SendQueue;
	LinkedList<EQNetworkPacket*>	BufferedPackets;
	Mutex	MInQueueLock;
	Mutex	MOutQueueLock;
	Mutex	MStateLock;

    int16	dwFragSeq;   //current fragseq
	int16	dwLastCACK;

	Timer*	keep_alive_timer;
	Timer*	timeout_timer;
	Timer*	datarate_timer;
	Timer*  datakeepalive_timer;
	Timer* queue_check_timer;

	sint32	datarate_sec;	// bytes/1000ms
	sint32	datarate_tic;	// bytes/100ms
};

class EQNetworkPacket {
public:
	EQNetworkPacket(EQNetworkConnection* inetcon);
	virtual ~EQNetworkPacket();

    bool			DecodePacket(uchar* data, int32 size);
	void			AddData(uchar* data, int32 size);
    int32			ReturnPacket(uchar** data);
	void			Clear();

    EQPACKET_HDR_INFO	HDR;

	int16				dwSEQ;			// Only used on incomming packets
    int16				dwARSP;			// Only used on incomming packets
    int16				dwARQ;
    int16				dbASQ_high	: 8,
						dbASQ_low	: 8;
    int16				dwOpCode;		//Not all packet have opcodes. 

    FRAGMENT_INFO		fraginfo;       //Fragment info
    int32				dwExtraSize;	//Size of additional info.
    uchar*				pExtra;			//Additional information
	int32				LastSent;
	int8				SentCount;
	int8				priority;
private:
	EQNetworkConnection* netcon;
};

class EQNetworkFragmentGroup {
public:
	EQNetworkFragmentGroup(int16 ifragseq, int16 inum_fragments);
	virtual ~EQNetworkFragmentGroup();

	void			Add(EQNetworkPacket* pack);
	bool			Ready();
	int32			Assemble(uchar** data);

	inline int16	GetFragSeq()		{ return fragseq; }
	inline int16	GetOpcode()			{ return opcode; }
protected:
	friend class EQNetworkFragmentGroupList;
	Timer*		timeout_timer;
private:
	int16		fragseq;		//Sequence number
	int16		opcode;			//Fragment group's opcode
	int16		num_fragments;
	uchar**		fragments;
	int32*		fragment_sizes;
};

#endif
