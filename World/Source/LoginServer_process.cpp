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
#include "zoneserver.h"
#include "ZSList.h"
#include "EQCUtils.hpp"

using namespace std;
extern EQC::World::Network::NetConnection net;
extern EQC::World::Network::ZSList zoneserver_list;
extern EQC::World::LoginServer* loginserver;
extern uint32 numzones;
extern volatile bool	RunLoops;

namespace EQC
{
	namespace World
	{
		void LoginServer::ProcessServerOP_LSClientAuth(ServerPacket* pack)
		{
			if (pack->size != sizeof(ServerLSClientAuth)) 
			{
				DumpPacketHex(pack->pBuffer, pack->size);
				cout << "Wrong size on ServerOP_LSClientAuth. Got: " << pack->size << ", Expected: " << sizeof(ServerLSClientAuth) << endl;
			} 
			else
			{
				ServerLSClientAuth* slsca = (ServerLSClientAuth*)pack->pBuffer;
				LSAuth_Struct* lsa = new LSAuth_Struct;
				memset(lsa, 0, sizeof(LSAuth_Struct));
				lsa->lsaccount_id = slsca->lsaccount_id;
				strncpy(lsa->name, slsca->name, 18);
				strncpy(lsa->key, slsca->key, 15);
				lsa->stale = false;
				lsa->inuse = true;
				lsa->firstconnect = true;
				this->AddAuth(lsa);
			}
		}

		void LoginServer::ProcessServerOP_LSFatalError(ServerPacket* pack)
		{
			net.LoginServerInfo = false;
			
			cout << "Login server responded with FatalError. Disabling reconnect." << endl;

			if (pack->size > 1)
			{
				cout << "Error message: '" << (char*) pack->pBuffer << "'" << endl;
			}
		}

		void LoginServer::ProcessServerOP_SystemwideMessage(ServerPacket* pack)
		{
			ServerSystemwideMessage* swm = (ServerSystemwideMessage*) pack->pBuffer;
			zoneserver_list.SendEmoteMessage(0, 0, swm->type, swm->message);
		}

		void LoginServer::ProcessServerOP_UsertoWorldReq(ServerPacket* pack)
		{
			UsertoWorldRequest_Struct* utwr = (UsertoWorldRequest_Struct*) pack->pBuffer;	
			int32 id = Database::Instance()->GetAccountIDFromLSID(utwr->lsaccountid);
			sint16 status = Database::Instance()->CheckStatus(id);

			ServerPacket* outpack = new ServerPacket;
			outpack->opcode = ServerOP_UsertoWorldResp;
			outpack->size = sizeof(UsertoWorldResponse_Struct);
			outpack->pBuffer = new uchar[outpack->size];
			memset(outpack->pBuffer, 0, outpack->size);
			UsertoWorldResponse_Struct* utwrs = (UsertoWorldResponse_Struct*) outpack->pBuffer;
			utwrs->lsaccountid = utwr->lsaccountid;
			utwrs->ToID = utwr->FromID;

			if(false)//Config->Locked == true)
			{
				if((status == 0 || status < 100) && (status != -2 || status != -1))
					utwrs->response = 0;
				if(status >= 100)
					utwrs->response = 1;
			}
			else {
				utwrs->response = 1;
			}

			sint32 x = 5000;//Config->MaxClients;
			int32 numplayers = 0;//Yeahlight: This needs to be the actual number of clients logged in
			if( (sint32)numplayers >= x && x != -1 && x != 255 && status < 80)
				utwrs->response = -3;

			if(status == -1)
				utwrs->response = -1;
			if(status == -2)
				utwrs->response = -2;

			utwrs->worldid = utwr->worldid;
			SendPacket(outpack);
			safe_delete(outpack);//delete outpack;
		}

		void LoginServer::ProcessServerOP_Unknown(ServerPacket* pack)
		{
			//AB00 - opcode from LS to World
			cout << " Unknown LSOPcode:" << (int)pack->opcode << " size:" << pack->size << endl;
			DumpPacket(pack->pBuffer, pack->size);
		}
	}
}