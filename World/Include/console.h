// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>

using namespace std;


#define CONSOLE_TIMEOUT 600000
#define CONSOLE_STATE_USERNAME 0
#define CONSOLE_STATE_CONNECTED 2
#define CONSOLE_STATE_CLOSED 3
#define CONSOLE_STATE_ZS 4

#include "TCPConnection.h"

#include "linked_list.h"
#include "timer.h"
#include "queue.h"
#include "servertalk.h"
#include "Mutex.h"

bool InitTCP();
#ifdef WIN32
void ConsoleLoop(void *tmp);
#else
void *ConsoleLoop(void *tmp);
#endif

namespace EQC
{
	namespace World
	{
		namespace Network
		{
			class Console : public TCPConnection 
			{
				public:
					Console(int32 in_ip, int16 in_port, int in_send_socket);
					~Console();
					bool Process();
					bool ReceiveData();
					void Send(char* message);
					int8 Admin() { return admin; }
					int32 GetIP() { return ip; }
					int16 GetPort() { return port; }
					void ProcessCommand(char* command);
					void Die();

					void SendEmoteMessage(char* to, int32 to_guilddbid, int32 type, char* message, ...);
					void SendExtMessage(char* message, ...);
					void SendMessage(int8 newline, char* message, ...);

					char* AccountName() { return paccountname; }
					int32 AccountID() { return paccountid; }

				private:
					Mutex	MOutPacketLock;
					int send_socket;
					int32 ip;
					int16 port;

					Timer* timeout_timer;

					void ClearBuffer();
					void SendPrompt();
					void CheckBuffer();

					int32 paccountid;
					char paccountname[30];

					int8 state;

					int8 admin;
					uchar textbuf[1024];
					int bufindex;
			};
		}
	}
}

#endif
