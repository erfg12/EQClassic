// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef LOGINSERVER_H
#define LOGINSERVER_H

#include "servertalk.h"
#include "linked_list.h"
#include "timer.h"
#include "queue.h"
#include "eq_packet_structs.h"
#include "Mutex.h"
#include "ServerPacket.h"

#ifdef WIN32
	void LoginServerLoop(void *tmp);
	void AutoInitLoginServer(void *tmp);
#else
	void *LoginServerLoop(void *tmp);
	void *AutoInitLoginServer(void *tmp);
#endif
bool InitLoginServer();

namespace EQC
{
	namespace World
	{
		class LoginServer 
		{
		public:
			LoginServer(int32 ip, int16 port, int in_send_socket);
			~LoginServer();

			bool Process();
			bool ReceiveData();
			void SendPacket(ServerPacket* pack);

			void SendInfo();
			void SendStatus();

			int32 GetIP()		{ return ip; }
			int16 GetPort()		{ return port; }

			void AddAuth(LSAuth_Struct* newauth);
			LSAuth_Struct* CheckAuth(int32 in_lsaccount_id, char* key);
			bool RemoveAuth(int32 in_lsaccount_id);
			void CheckStale();
		private:
			Mutex	MAuthListLock;
			Mutex	MPackQueues;
			int32 ip;
			int16 port;

			int send_socket;

			SPackSendQueue* ServerSendQueuePop();
			MyQueue<ServerPacket> ServerOutQueue;
			MyQueue<SPackSendQueue> ServerSendQueue;

			Timer* timeout_timer;
			Timer* statusupdate_timer;
			Timer* staleauth_timer;

			LinkedList<LSAuth_Struct*> auth_list;

			void SendKeepAlive();

			void ProcessServerOP_LSClientAuth(ServerPacket* pack);
			void ProcessServerOP_LSFatalError(ServerPacket* pack);
			void ProcessServerOP_SystemwideMessage(ServerPacket* pack);
			void ProcessServerOP_Unknown(ServerPacket* pack);
			void ProcessServerOP_UsertoWorldReq(ServerPacket* pack);
		};
	}
}
#endif
