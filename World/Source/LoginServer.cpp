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
#include <iomanip>

#ifndef WIN32
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pthread.h>
	#include <errno.h>

	#include "unix.h"

	#include <unistd.h>

	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
	extern int errno;
#endif

#include "servertalk.h"
#include "LoginServer.h"
#include "eq_packet_structs.h"
#include "packet_dump.h"
//#include "net.h"
#include "zoneserver.h"
#include "ZSList.h"
#include "EQCUtils.hpp"

using namespace std;
using namespace EQC::World::Network;

extern EQC::World::Network::NetConnection net;
extern EQC::World::Network::ZSList zoneserver_list;
extern EQC::World::LoginServer* loginserver;
extern uint32 numzones;
extern volatile bool	RunLoops;
volatile bool LoginLoopRunning = false;

bool AttemptingConnect = false;

// this should always be called in a new thread
#ifdef WIN32
void AutoInitLoginServer(void *tmp)
#else
void *AutoInitLoginServer(void *tmp)
#endif
{
	if (loginserver == 0 && (!AttemptingConnect)) 
	{
		InitLoginServer();
	}
#ifndef WIN32
	return 0;
#endif
}

bool InitLoginServer() 
{
	if (AttemptingConnect || loginserver != 0) 
	{
		cout << "Error: InitLoginServer() while already attempting connect" << endl;
		return false;
	}
	if (!net.LoginServerInfo) 
	{
		cout << "Error: Login server info not loaded" << endl;
		return false;
	}

	AttemptingConnect = true;

	unsigned long nonblocking = 1;
	struct sockaddr_in	server_sin;
	struct in_addr	in;


#ifdef WIN32
	SOCKET tmpsock = INVALID_SOCKET;
	WORD version = MAKEWORD (1,1);
	WSADATA wsadata;
	PHOSTENT phostent = NULL;
	WSAStartup (version, &wsadata);

#else
	int tmpsock = INVALID_SOCKET;
	struct hostent *phostent = NULL;
#endif
	
	if ((tmpsock = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		cout << "LoginServer connect: Allocating socket failed. Error: " << EQC::Common::GetLastSocketError();
		AttemptingConnect = false;
		return false;
	}


	server_sin.sin_family = AF_INET;

	if ((phostent = gethostbyname(net.GetLoginAddress())) == NULL)
	{
		cout << "Unable to get the host name. Error: " << EQC::Common::GetLastSocketError();
		EQC::Common::CloseSocket(tmpsock);
		AttemptingConnect = false;
		return false;
	}
#ifdef WIN32
	memcpy ((char FAR *)&(server_sin.sin_addr), phostent->h_addr, phostent->h_length);
#else
	memcpy ((char*)&(server_sin.sin_addr), phostent->h_addr, phostent->h_length);
#endif
	server_sin.sin_port = htons(net.GetLoginPort());

	// Establish a connection to the server socket.
#ifdef WIN32
	if (connect (tmpsock, (PSOCKADDR) &server_sin, sizeof (server_sin)) == SOCKET_ERROR)
#else
	if (connect (tmpsock, (struct sockaddr *) &server_sin, sizeof (server_sin)) == SOCKET_ERROR)
#endif
	{
		cout << "LoginServer connect: Connecting to the server failed. Error: " << EQC::Common::GetLastSocketError() << endl;
		EQC::Common::CloseSocket(tmpsock);
		AttemptingConnect = false;
		return false;
	}	
#ifdef WIN32
	ioctlsocket (tmpsock, FIONBIO, &nonblocking);
#else
	fcntl(tmpsock, F_SETFL, O_NONBLOCK);
#endif

	in.s_addr = server_sin.sin_addr.s_addr;
	cout << "Connected to LoginServer: " << net.GetLoginAddress() << ":" << ntohs(server_sin.sin_port) << endl;
	loginserver = new EQC::World::LoginServer(in.s_addr, ntohs(server_sin.sin_port), tmpsock);

#ifdef WIN32
	_beginthread(LoginServerLoop, 0, NULL);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, LoginServerLoop, NULL);
#endif
	AttemptingConnect = false;
	return true;
}

// this should always be called in a new thread
#ifdef WIN32
void LoginServerLoop(void *tmp)
#else
void *LoginServerLoop(void *tmp)
#endif
{
	LoginLoopRunning = true;
	loginserver->SendInfo();
	loginserver->SendStatus();
	
	while(RunLoops && loginserver != 0)
	{
		if (!(loginserver->ReceiveData() && loginserver->Process()))
		{
			break;
		}
		Sleep(1);
	}

	safe_delete(loginserver);
	LoginLoopRunning = false;
#ifdef WIN32
	_endthread();
#else
	return 0;
#endif
}

namespace EQC
{
	namespace World
	{
		LoginServer::LoginServer(int32 in_ip, int16 in_port, int in_send_socket)
		{
			ip = in_ip;
			port = in_port;
			send_socket = in_send_socket;
			
			// Setup timers
			timeout_timer = new Timer(SERVER_TIMEOUT);
			statusupdate_timer = new Timer(LoginServer_StatusUpdateInterval);
			staleauth_timer = new Timer(LoginServer_AuthStale);
		}

		LoginServer::~LoginServer()
		{
			// delete timers
			safe_delete(timeout_timer);//delete timeout_timer;
			safe_delete(statusupdate_timer);//delete statusupdate_timer;
			safe_delete(staleauth_timer);//delete staleauth_timer;

			// Send out shutdown
			shutdown(send_socket, 0x01);
			shutdown(send_socket, 0x00);

			// Close the socket
			EQC::Common::CloseSocket(send_socket);
		}

		bool LoginServer::Process()
		{
#ifdef WIN32
			SOCKADDR_IN to;
#else
			struct sockaddr_in to;
#endif

			memset((char *) &to, 0, sizeof(to));
			to.sin_family = AF_INET;
			to.sin_port = port;
			to.sin_addr.s_addr = ip;

			if (statusupdate_timer->Check())
			{
				this->SendStatus();
			}

			if (staleauth_timer->Check())
			{
				staleauth_timer->Start();
				loginserver->CheckStale();
			}
		    
			/************ Get all packets from packet manager out queue and process them ************/
			ServerPacket *pack = 0;

			while(pack = ServerOutQueue.pop())
			{
				switch(pack->opcode) 
				{
					case 0:
					break;
					case ServerOP_KeepAlive: 
					{
						// ignore this -- froglok23 -- WHY?
					}
					break;
					case ServerOP_LSClientAuth:
					{
						this->ProcessServerOP_LSClientAuth(pack);
					}
					break;
				case ServerOP_LSFatalError: 
					{
						this->ProcessServerOP_LSFatalError(pack);          
					}
					break;
				case ServerOP_SystemwideMessage:
					{
						this->ProcessServerOP_SystemwideMessage(pack);
						break;
					}
				case ServerOP_UsertoWorldReq:
					{
						this->ProcessServerOP_UsertoWorldReq(pack);
						break;
					}
				case 0x0109:
				case 0x010a:
				case 0x0106:
				case 0x0107:
					{
						//Yeahlight: These opcodes occasionally appear with a packet size of 18 when a player attempts to enter a zone. Leave these packets unresolved is freezing the client
						this->ProcessServerOP_Unknown(pack);
						this->SendPacket(pack);
						break;
					}
				default:
					{
						this->ProcessServerOP_Unknown(pack);
						break;
					}
				}

				safe_delete(pack);//delete pack;
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}

			/************ Get first send packet on queue and send it! ************/
			SPackSendQueue* p = 0;    
			int status = 0;
			while(p = ServerSendQueuePop())
			{

#ifdef WIN32
				status = send(send_socket, (const char *) p->buffer, p->size, 0);
#else
				status = send(send_socket, p->buffer, p->size, 0);
#endif
				safe_delete(p);//delete p;
				if (status == SOCKET_ERROR)
				{
					cout << "Loginserver send(): status=" << status  << ", Errorcode: " << EQC::Common::GetLastSocketError() << endl;	
					return false;
				}
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
			/************ Processing finished ************/
			if (timeout_timer->Check())
			{
				this->SendKeepAlive();
			}

			return true;
		}

		void LoginServer::SendKeepAlive()
		{
			// Keepalive packet doesnt actually do anything
			// just makes sure that send() still works, if not connection is dead
			ServerPacket* pack = new ServerPacket(ServerOP_KeepAlive, 0);
			SendPacket(pack);
			safe_delete(pack);//delete pack;
		}

		void LoginServer::SendPacket(ServerPacket* pack) 
		{
			SPackSendQueue* spsq = (SPackSendQueue*) new uchar[sizeof(SPackSendQueue) + pack->size + 4];
			
			if (pack->pBuffer != 0 && pack->size != 0)
			{
				memcpy((char *) &spsq->buffer[4], (char *) pack->pBuffer, pack->size);
			}

			memcpy((char *) &spsq->buffer[0], (char *) &pack->opcode, 2);
			spsq->size = pack->size+4;
			memcpy((char *) &spsq->buffer[2], (char *) &spsq->size, 2);
			LockMutex lock(&MPackQueues);
			ServerSendQueue.push(spsq);
		}

		SPackSendQueue* LoginServer::ServerSendQueuePop()
		{
			LockMutex lock(&MPackQueues);
			return ServerSendQueue.pop();
		}

		bool LoginServer::ReceiveData() 
		{
			uchar		buffer[10240];
			
			int			status;
			unsigned short	port;

			struct sockaddr_in	from;
			struct in_addr	in;
			unsigned int	fromlen;

			from.sin_family = AF_INET;
			fromlen = sizeof(from);

		#ifdef WIN32
			status = recvfrom(send_socket, (char *) &buffer, sizeof(buffer), 0, (struct sockaddr*) &from, (int *) &fromlen);
		#else
			status = recvfrom(send_socket, (char *) &buffer, sizeof(buffer), 0, (struct sockaddr*) &from, &fromlen);
		#endif

			if (status > 1)
			{
				// cout << "TCPData from ip: " << inet_ntoa(in) << " port:" << GetPort() << " length: " << status << endl;
				// DumpPacket(buffer, status);
				port = from.sin_port;
				in.s_addr = from.sin_addr.s_addr;

				timeout_timer->Start();
				int16 base = 0;
				int16 size = 0;
				while (base < status)
				{
					size = 4;
					if ((status - base) < 4)
					{
						cout << "LoginServer: packet too short for processing" << endl;
					}
					else
					{
						ServerPacket* pack = new ServerPacket;
						memcpy((char*) &size, (char*) &buffer[base + 2], 2);

						memcpy((char*) &pack->opcode, (char*) &buffer[base + 0], 2);
						pack->size = size - 4;
						
						if (size > 4) 
						{
							pack->pBuffer = new uchar[pack->size];
							memcpy((char*) pack->pBuffer, (char*) &buffer[base + 4], pack->size);
						} 
						else
						{
							pack->pBuffer = 0;
						}
						// cout << "Packet from LoginServer: OPcode=0x" << hex << setw(4) << setfill('0') << pack->opcode << dec << " size=" << pack->size << endl;
						ServerOutQueue.push(pack);
					}
					base += size;
				}
			}
			else if (status == SOCKET_ERROR) 
			{
#ifdef WIN32
				if (!(WSAGetLastError() == WSAEWOULDBLOCK))
#else
				if (!(errno == EWOULDBLOCK))
#endif
				{
					struct in_addr  in;
					in.s_addr = GetIP();
					cout << "LoginServer connection lost: " << inet_ntoa(in) << ":" << GetPort() << endl;
					return false;
				}
			}
			return true;
		}

		void LoginServer::SendInfo()
		{
			ServerPacket* pack = new ServerPacket(ServerOP_LSInfo, sizeof(ServerLSInfo_Struct));

			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, pack->size);
			ServerLSInfo_Struct* lsi = (ServerLSInfo_Struct*) pack->pBuffer;
			strcpy(lsi->name, net.GetWorldName());
			strcpy(lsi->address, net.GetWorldAddress());
			strcpy(lsi->account, net.GetWorldAccount());
			strcpy(lsi->password, net.GetWorldPassword());
			strcpy(lsi->protocolversion, EQEMU_PROTOCOL_VERSION); //newage: added
			strcpy(lsi->version, EQC_VERSION);
			SendPacket(pack);
			safe_delete(pack);//delete pack;
		}

		void LoginServer::SendStatus() 
		{
			statusupdate_timer->Start();
			ServerPacket* pack = new ServerPacket(ServerOP_LSStatus, sizeof(ServerLSStatus_Struct));
			
			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, pack->size);
			ServerLSStatus_Struct* lss = (ServerLSStatus_Struct*) pack->pBuffer;
			if (net.world_locked)
			{
				lss->status = -2;
			}
			else if (numzones == 0)
			{
				lss->status = -2;
			}
			else
			{
				lss->status = numzones;
			}
			SendPacket(pack);
			safe_delete(pack);//delete pack;
		}

		void LoginServer::AddAuth(LSAuth_Struct* newauth)
		{
			LockMutex lock(&MAuthListLock);
			auth_list.Insert(newauth);
		}

		LSAuth_Struct* LoginServer::CheckAuth(int32 in_lsaccount_id, char* key)
		{
			LinkedListIterator<LSAuth_Struct*> iterator(auth_list);

			LockMutex lock(&MAuthListLock);
			iterator.Reset();

			while(iterator.MoreElements())
			{
				if (iterator.GetData()->lsaccount_id == in_lsaccount_id && strcmp(iterator.GetData()->key, key) == 0)
				{
					LSAuth_Struct* tmp = iterator.GetData();
					return tmp;
				}
				iterator.Advance();
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
			return 0;
		}

		bool LoginServer::RemoveAuth(int32 in_lsaccount_id) 
		{
			LinkedListIterator<LSAuth_Struct*> iterator(auth_list);

			LockMutex lock(&MAuthListLock);
			iterator.Reset();
			while(iterator.MoreElements())
			{
				if (iterator.GetData()->lsaccount_id == in_lsaccount_id)
				{
					iterator.RemoveCurrent();
					return true;
				}
				iterator.Advance();
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
			return false;
		}

		void LoginServer::CheckStale()
		{
			LinkedListIterator<LSAuth_Struct*> iterator(auth_list);

			LockMutex lock(&MAuthListLock);
			iterator.Reset();
			while(iterator.MoreElements())
			{
				if (!iterator.GetData()->inuse)
				{
					if (iterator.GetData()->stale) 
					{
						iterator.RemoveCurrent();
					}
					else 
					{
						iterator.GetData()->stale = true;
						iterator.Advance();
					}
				}
				else
				{
					iterator.Advance();
				}
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
		}
	}
}