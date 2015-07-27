// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef WIN32
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>
	#include <fcntl.h>
#else
	#include <errno.h>
	#include <fcntl.h>
	#include <windows.h>
	#include <winsock.h>
#endif

#include "config.h"

namespace EQC
{
	namespace World
	{
		namespace Network
		{
			class NetConnection
			{
			public:

				NetConnection() ;
				~NetConnection();
				
				bool Init();
				void ListenNewClients();
				bool LoginServerInfo;

				// Accessors
				char* GetLoginAddress()		{ return loginaddress; }
				int16 GetLoginPort()		{ return loginport; }
				char* GetWorldName()		{ return worldname; }
				char* GetWorldAccount()		{ return worldaccount; }
				char* GetWorldPassword()	{ return worldpassword; }
				char* GetWorldAddress()		{ return worldaddress; }
				bool  GetWorldStatus()		{ return world_locked; }

				bool world_locked;

			private:
				bool	ReadLoginINI();
				int		listening_socket;
				char	loginaddress[255];
				int16	loginport;
				char	worldname[201];
				char	worldaccount[31];
				char	worldpassword[31];
				char	worldaddress[255];				
			};

		}
	}
}