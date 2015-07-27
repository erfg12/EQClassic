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
#include <cstdlib>
#include <ctime>
#include <iomanip>

#include <signal.h>
#include "queue.h"
#include "timer.h"
#include "EQPacket.h"
#include "EQPacketManager.h"
#include "servertalk.h"
#include "client.h"
#include "clientlist.h"
#include "database.h"
#include "seperator.h"
#ifndef WIN32
	#include <pthread.h>
	#include "unix.h"
#endif
#include "zoneserver.h"
#include "console.h"
#include "LoginServer.h"
#include "ZSList.h"
#include "EQCUtils.hpp"
#include "World.h"

using namespace std;
using namespace EQC::World::Network;

NetConnection net;

EQC::World::Network::ZSList zoneserver_list;

uint32 numzones = 0;

extern EQC::World::ClientList client_list;
extern uint32 numclients;

namespace EQC
{
	namespace World
	{
		namespace Network
		{
			// Constructor
			NetConnection::NetConnection() 
			{
				world_locked = false; // by default, the world is NOT locked

				memset(loginaddress, 0, sizeof(loginaddress));
				memset(worldname, 0, sizeof(worldname));
				memset(worldaccount, 0, sizeof(worldaccount));
				memset(worldpassword, 0, sizeof(worldpassword));
				memset(worldaddress, 0, sizeof(worldaddress));
				LoginServerInfo = ReadLoginINI();
			}

			
			NetConnection::~NetConnection()
			{
			}

			bool NetConnection::Init()
			{
				struct sockaddr_in address;
				int reuse_addr = 1;
				
			#ifdef WIN32
				unsigned long nonblocking = 1;
				WORD version = MAKEWORD (1,1);
				WSADATA wsadata;
				WSAStartup (version, &wsadata);
			#endif


				// Setup internet address information.  
				// This is used with the bind() call 
				memset((char *) &address, 0, sizeof(address));
				address.sin_family = AF_INET;
				address.sin_port = htons(WORLD_PORT);
				address.sin_addr.s_addr = htonl(INADDR_ANY);

				/* Setting up UDP port for new clients */
				listening_socket = socket(AF_INET, SOCK_DGRAM, 0);
				
				if (listening_socket < 0) 
 				{	
					return false;
				}

			#ifndef WIN32
				setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
			#else
				setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse_addr, sizeof(reuse_addr));
			#endif

				if (bind(listening_socket, (struct sockaddr *) &address, sizeof(address)) < 0) 
				{
					EQC::Common::CloseSocket(listening_socket);
					return false;
				}


			#ifndef WIN32
				fcntl(listening_socket, F_SETFL, O_NONBLOCK);
			#else
				ioctlsocket (listening_socket, FIONBIO, &nonblocking);
			#endif

				return true;
			}

			void NetConnection::ListenNewClients()
			{
				
				uchar			buffer[1024];
				int				status;
				unsigned short	port;

				struct sockaddr_in	from;
				struct in_addr		in;
				unsigned int		fromlen;

				from.sin_family = AF_INET;
				fromlen = sizeof(from);

			#ifndef WIN32
				status = recvfrom(listening_socket, &buffer, sizeof(buffer), 0,(struct sockaddr*) &from, &fromlen);
			#else
				status = recvfrom(listening_socket, (char *) &buffer, sizeof(buffer), 0,(struct sockaddr*) &from, (int *) &fromlen);
			#endif

				if (status > 1)
				{
					EQC::World::Client* client = 0;
					port = from.sin_port;
					in.s_addr = from.sin_addr.s_addr;

					client = client_list.Get(in.s_addr, from.sin_port);
					if (client == 0)
					{
						// If it is a new client make sure it has the starting flag set. Ignore otherwise
						if (buffer[0] & 0x20)
						{
							EQC::Common::PrintF(CP_CLIENT, "New client from %s:%i\n", inet_ntoa(in), ntohs(port));
							client = new EQC::World::Client(in.s_addr, port, listening_socket);
							client_list.Add(client);
							numclients++;
							//Yeahlight: Part I or III to log client into the DB
							Database::Instance()->LogAccountInPartI(in.s_addr, this->GetWorldAccount());
						}
						else 
						{
							return;
						}
					}
					else {
			//			cout << Timer::GetCurrentTime() << " Data from ip: " << inet_ntoa(in) << " port:" << ntohs(port) << " length: " << status << endl;
					}
					client->ReceiveData(buffer, status);
				}
			}


			bool NetConnection::ReadLoginINI()
			{
				char buf[201], type[201];
				int items[2] = {0, 0};
				FILE *f;

				if (!(f = fopen ("LoginServer.ini", "r")))
				{
					EQC::Common::PrintF(CP_WORLDSERVER, "Couldn't open the LoginServer.ini file.\n");
					return false;
				}

				do 
				{
					fgets (buf, 200, f);

					if (feof (f))
					{
						EQC::Common::PrintF(CP_WORLDSERVER,"[LoginServer] block not found in LoginServer.ini.\n");
						
						fclose (f);
						return false;
					}
				}
				while (strncasecmp (buf, "[LoginServer]\n", 14) != 0 && strncasecmp (buf, "[LoginServer]\r\n", 15) != 0);

				while (!feof (f))
				{
			#ifdef WIN32
					if (fscanf (f, "%[^=]=%[^\n]\r\n", type, buf) == 2)
			#else
					if (fscanf (f, "%[^=]=%[^\r\n]\n", type, buf) == 2)
			#endif
					{
						if (!strncasecmp (type, "loginserver", 11))
						{
							strncpy (loginaddress, buf, 100);
							items[0] = 1;
						}
						if (!strncasecmp (type, "worldname", 9))
						{
							strncpy (worldname, buf, 200);
							items[1] = 1;
						}
						if (!strncasecmp (type, "account", 7)) 
						{
							strncpy(worldaccount, buf, 30);
						}
						if (!strncasecmp (type, "password", 8)) 
						{
							strncpy (worldpassword, buf, 30);
						}
						if (!strncasecmp (type, "locked", 6)) 
						{
							if (strcasecmp(buf, "true") == 0 || (buf[0] == '1' && buf[1] == 0))
							{
								world_locked = true;
							}
						}
						if (!strncasecmp (type, "worldaddress", 12))
						{
							strncpy (worldaddress, buf, 250);
						}
						if (!strncasecmp(type, "loginport", 9)) 
						{
							if (Seperator::IsNumber(buf) && atoi(buf) > 0 && atoi(buf) < 0xFFFF) 
							{
								loginport = atoi(buf);
							}
						}
					}
				}

				if (!items[0] || !items[1])
				{
					EQC::Common::PrintF(CP_WORLDSERVER, "Incomplete LoginServer.INI file.\n");
					fclose (f);
					return false;
				}
				
				fclose (f);
				EQC::Common::PrintF(CP_WORLDSERVER, "LoginServer.ini read.\n");
				return true;
			}
		}
	}
}