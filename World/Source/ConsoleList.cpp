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

#include "config.h"

#ifndef WIN32
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

namespace EQC
{
	namespace World
	{
		namespace Network
		{
		void ConsoleList::SendConsoleWho(char* to, int8 admin, EQC::World::Network::TCPConnection* connection)
		{
			LinkedListIterator<EQC::World::Network::Console*> iterator(list);
			iterator.Reset();
			struct in_addr  in;
			int x = 0;

			while(iterator.MoreElements())
			{
				in.s_addr = iterator.GetData()->GetIP();
				connection->SendEmoteMessage(to, 0, 10, "  Console: %s:%i AccID: %i AccName: %s", inet_ntoa(in), iterator.GetData()->GetPort(), iterator.GetData()->AccountID(), iterator.GetData()->AccountName());
				x++;
				iterator.Advance();
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
				connection->SendEmoteMessage(to, 0, 10, "%i consoles connected", x);
		}

		EQC::World::Network::Console* ConsoleList::FindByAccountName(char* accname) {
			LinkedListIterator<EQC::World::Network::Console*> iterator(list);
			iterator.Reset();

			while(iterator.MoreElements()) {
				if (strcasecmp(iterator.GetData()->AccountName(), accname) == 0)
					return iterator.GetData();

				iterator.Advance();
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
			return 0;
		}

		void ConsoleList::Add(EQC::World::Network::Console* con) {
			list.Insert(con);
		}

		void ConsoleList::ReceiveData() {
			LinkedListIterator<Console*> iterator(list);

			iterator.Reset();
			while(iterator.MoreElements())
			{
				if (!iterator.GetData()->ReceiveData()) {
					iterator.GetData()->Die();
					iterator.RemoveCurrent();
				}
				else {
					iterator.Advance();
				}
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
		}

	}
}
}