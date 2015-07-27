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
#include <cstdarg>

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <errno.h>
	#include <cstdlib>
	#include <pthread.h>

	#include "unix.h"

	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1

	extern int errno;
#endif

#include "console.h"
#include "consolelist.h"
//#include "net.h"
#include "zoneserver.h"
#include "database.h"
#include "packet_dump.h"
#include "seperator.h"
#include "eq_packet_structs.h"
#include "LoginServer.h"
#include "EQCException.hpp"
#include "ZSList.h"
#include "EQCUtils.hpp"

using namespace std;

extern EQC::World::Network::NetConnection net;
extern EQC::World::Network::ZSList	zoneserver_list;
extern volatile bool RunLoops;
extern uint32	numzones;
extern EQC::World::LoginServer* loginserver;
volatile bool ConsoleLoopRunning = false;

int listening_socketTCP;
EQC::World::Network::ConsoleList console_list;

bool InitTCP()
{
#ifdef WIN32
	SOCKADDR_IN address;

	WORD version = MAKEWORD (1,1);
	WSADATA wsadata;

	WSAStartup (version, &wsadata);

#else
	struct sockaddr_in address;
#endif

	int reuse_addr = 1;
	unsigned long nonblocking = 1;


  /* Setup internet address information.  
     This is used with the bind() call */
	memset((char *) &address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(WORLD_PORT);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Setting up TCP port for new TCP connections */
	listening_socketTCP = socket(AF_INET, SOCK_STREAM, 0);
	if (listening_socketTCP == INVALID_SOCKET) 
 	{
		return false;
	}

// Quag: dont think following is good stuff for TCP, good for UDP
// Mis: SO_REUSEADDR shouldn't be a problem for tcp--allows you to restart
// without waiting for conns in TIME_WAIT to die
	setsockopt(listening_socketTCP, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse_addr, sizeof(reuse_addr));


	if (bind(listening_socketTCP, (struct sockaddr *) &address, sizeof(address)) < 0) 
	{
		EQC::Common::CloseSocket(listening_socketTCP);
		return false;
	}

#ifdef WIN32
	ioctlsocket (listening_socketTCP, FIONBIO, &nonblocking);
#else
	fcntl(listening_socketTCP, F_SETFL, O_NONBLOCK);
#endif

	if (listen (listening_socketTCP, SOMAXCONN) == SOCKET_ERROR) 
	{
		cout << "Listening TCP socket failed. Error: " << EQC::Common::GetLastSocketError() << endl;
		EQC::Common::CloseSocket(listening_socketTCP);
		return false;
	}

	return true;
}

void ListenNewTCP()
{
    int		tmpsock;
    struct sockaddr_in	from;
    struct in_addr	in;
    unsigned int	fromlen;
    unsigned short	port;
	EQC::World::Network::Console* con;

    from.sin_family = AF_INET;
    fromlen = sizeof(from);

	// Check for pending connects
#ifdef WIN32
	tmpsock = accept(listening_socketTCP, (struct sockaddr*) &from, (int *) &fromlen);
#else
	tmpsock = accept(listening_socketTCP, (struct sockaddr*) &from, &fromlen);
#endif
	if (!(tmpsock == INVALID_SOCKET))
	{
		port = from.sin_port;
		in.s_addr = from.sin_addr.s_addr;

		// New TCP connection
		con = new EQC::World::Network::Console(in.s_addr, ntohs(from.sin_port), tmpsock);
		cout << "New TCP connection: " << inet_ntoa(in) << ":" << con->GetPort() << endl;
		send(tmpsock, "Username: ", 10, 0);
		console_list.Add(con);
	}
}

#ifdef WIN32
void ConsoleLoop(void *tmp) {
#else
void *ConsoleLoop(void *tmp) {
#endif
	ConsoleLoopRunning = true;
	while(RunLoops) {
		ListenNewTCP();
		console_list.ReceiveData();
		Sleep(1);
	}
	ConsoleLoopRunning = false;
}



namespace EQC
{
	namespace World
	{
		namespace Network
		{
			Console::Console(int32 in_ip, int16 in_port, int in_send_socket) : TCPConnection() {
				ip = in_ip;
				port = in_port;
				send_socket = in_send_socket;
				timeout_timer = new Timer(CONSOLE_TIMEOUT);
				state = 0;
				ClearBuffer();
				paccountid = 0;
				memset(paccountname, 0, sizeof(paccountname));
				admin = 0;
				#ifndef WIN32
					fcntl(in_send_socket, F_SETFL, O_NONBLOCK);
				#endif
			}

			Console::~Console() {
				safe_delete(timeout_timer);//delete timeout_timer;
			}

			void Console::ClearBuffer() {
				bufindex = 0;
				memset(textbuf, 0, sizeof(textbuf));
			}

			void Console::Die() {
				if (state != CONSOLE_STATE_ZS) {
					state = CONSOLE_STATE_CLOSED;
					struct in_addr  in;
					in.s_addr = ip;
					cout << "Removing console from ip: " << inet_ntoa(in) << " port:" << port << endl;

					EQC::Common::CloseSocket(send_socket);
				}
			}

			void Console::SendEmoteMessage(char* to, int32 to_guilddbid, int32 type, char* message, ...) 
			{
				//-Cofruben: attempt to do this more secure
				if (!message) 
				{
					EQC_EXCEPT("void Console::SendEmoteMessage()", "message = NULL");
				}
				va_list argptr;
				char buffer[256];

				va_start(argptr, message);
				vsnprintf(buffer, 256, message, argptr);
				va_end(argptr);
				SendMessage(0, buffer);
			}

			void Console::SendExtMessage(char* message, ...)
			{
				//-Cofruben: attempt to do this more secure
				if (!message) EQC_EXCEPT("void Console::SendExtMessage()", "message = NULL");
				va_list argptr;
				char buffer[256];

				va_start(argptr, message);
				vsnprintf(buffer, 256, message, argptr);
				va_end(argptr);

				SendMessage(0, 0);
				SendMessage(0, buffer);
				ClearBuffer();
				SendPrompt();
			}

			void Console::SendMessage(int8 newline, char* message, ...)
			{
				LockMutex lock(&MOutPacketLock);
				if (message != 0) {
					va_list argptr;
					char buffer[256];

					va_start(argptr, message);
					vsnprintf(buffer, 256, message, argptr);
					va_end(argptr);

					send(send_socket, buffer, strlen(buffer), 0);
				}

				if (newline) {
					char outbuf[2];
					memset(&outbuf[0], 13, 1);
					memset(&outbuf[1], 10, 1);
					for (int i=0; i < newline; i++)
						send(send_socket, outbuf, 2, 0);
				}
			}


			bool Console::ReceiveData() {
				if (state == CONSOLE_STATE_ZS || state == CONSOLE_STATE_CLOSED)
					return false;

				uchar		buffer[1024];
				
				int			status;

				status = recv(send_socket, (char *) &buffer, sizeof(buffer), 0);

				if (status >= 1)
				{
					timeout_timer->Start();
			//DumpPacket(buffer, status);
					if ((bufindex + status) > 1020) {
						// buffer overflow, kill client
						SendMessage(0, 0);
						SendMessage(0, "Buffer overflow, disconnecting");
						return false;
					} else {
						memcpy(&textbuf[bufindex], &buffer, status);
						bufindex += status;
						MOutPacketLock.lock();
						send(send_socket, (char *) buffer, status, 0);
						MOutPacketLock.unlock();
						CheckBuffer();
					}
				} else if (status == SOCKET_ERROR) {
			#ifdef WIN32
					if (!(WSAGetLastError() == WSAEWOULDBLOCK)) {
			#else
					if (!(errno == EWOULDBLOCK)) {
			#endif
						struct in_addr  in;
						in.s_addr = GetIP();
						cout << "TCP connection lost: " << inet_ntoa(in) << ":" << GetPort() << endl;
						return false;
					}
				}

				if (timeout_timer->Check())
				{
					SendMessage(0, 0);
					SendMessage(0, "Timeout, disconnecting...");
					struct in_addr  in;
					in.s_addr = GetIP();
					cout << "TCP connection timeout: " << inet_ntoa(in) << ":" << GetPort() << endl;
					return false;
				}

				return true;
			}

			void Console::CheckBuffer() {
				for (int i=0; i < bufindex; i++) {
					if (textbuf[i] < 8 || textbuf[i] == 11 || textbuf[i] == 12 || (textbuf[i] > 13 && textbuf[i] < 32) || textbuf[i] >= 127) {
						cout << "Illegal character in TCP string, disconnecting at " << i << " 0x" << hex << textbuf[i] << dec << endl;
						SendMessage(0, 0);
						SendMessage(0, "Illegal character in stream, disconnecting.");
						state = CONSOLE_STATE_CLOSED;
						return;
					}
					switch(textbuf[i]) {
						case 10:
						case 13: // newline marker
						{
							if (i==0) { // empty line
								memcpy(textbuf, &textbuf[1], 1023);
								textbuf[1023] = 0;
								bufindex--;
								i = -1;
							} else {
								char* line = new char[i+1];
								memset(line, 0, i+1);
								memcpy(line, textbuf, i);
								line[i] = 0;
								uchar backup[sizeof(textbuf)];
								memcpy(backup, textbuf, sizeof(textbuf));
								memset(textbuf, 0, sizeof(textbuf));
								memcpy(textbuf, &backup[i+1], (sizeof(textbuf) - i) - 1);
								bufindex -= i+1;
								if (strlen(line) > 0)
									ProcessCommand(line);
								safe_delete(line);//delete line;
								i = -1;
							}
							break;
						}
						case 8: // backspace
						{
							if (i==0) { // nothin to backspace
								memcpy(textbuf, &textbuf[1], 1023);
								textbuf[1023] = 0;
								bufindex -= 1;
								i = -1;
							} else {
								uchar backup[1024];
								memcpy(backup, textbuf, 1024);
								memset(textbuf, 0, sizeof(textbuf));
								memcpy(textbuf, backup, i-1);
								memcpy(&textbuf[i-1], &backup[i+1], 1022-i);
								bufindex -= 2;
								i -= 2;
							}
							break;
						}
					}
				}
				if (bufindex < 0)
					bufindex = 0;
			}


			void Console::ProcessCommand(char* command) {
				switch(state)
				{
					case CONSOLE_STATE_USERNAME:
					{
						char Bf[64]; memset(Bf, 0, sizeof(Bf));
						sprintf(Bf, "**ZONESERVER*%s*", EQC_VERSION);
						if (strncmp(command, Bf, strlen(Bf)) == 0) {
							struct in_addr	in;
							in.s_addr = GetIP();
							if (!Database::Instance()->CheckZoneserverAuth(inet_ntoa(in))) {
								// not auth
								cout << "Zoneserver auth failed: " << inet_ntoa(in) << ":" << port << endl;
								SendMessage(BLACK, "Not Authorized.");
								state = CONSOLE_STATE_CLOSED;
							} else {
								ZoneServer* zs = new ZoneServer(ip, port, send_socket);
								cout << "New zoneserver: #" << zs->GetID() << " " << inet_ntoa(in) << ":" << port << endl;
								zoneserver_list.Add(zs);
								numzones++;
								SendMessage(BLACK, "ZoneServer Mode");
								state = CONSOLE_STATE_ZS;
							}
							return;
						}
						else {
							SendMessage(BLACK, "You're not allowed to log in.");
							this->Die();
						}
						break;
					}
					default: {
						break;
					}
				}
			}

			void Console::SendPrompt() {
				SendMessage(BLACK, "%s> ", paccountname);
			}

		}
	}
}

