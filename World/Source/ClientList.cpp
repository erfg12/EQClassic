// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#ifndef WIN32
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

#include "config.h"
#include "client.h"
#include "clientlist.h"
#include "eq_packet_structs.h"
#include "packet_dump.h"
#include "database.h"
#include "LoginServer.h"
#include "zoneserver.h"
#include "EQCException.hpp"


namespace EQC
{
	namespace World
	{
		ClientList::ClientList() 
		{
		}

		ClientList::~ClientList()
		{
		}

		void ClientList::Add(Client* client)
		{
			list.Insert(client);
		}

		Client* ClientList::Get(int32 ip, int16 port)
		{
			LinkedListIterator<Client*> iterator(list);

			iterator.Reset();
			while(iterator.MoreElements())
			{
				if (iterator.GetData()->GetIP() == ip && iterator.GetData()->GetPort() == port)
				{
					Client* tmp = iterator.GetData();
					return tmp;
				}
				iterator.Advance();
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
			return 0;
		}

		void ClientList::Process()
		{
			LinkedListIterator<Client*> iterator(list);

			iterator.Reset();
			while(iterator.MoreElements())
			{
				bool Res = false;
				try {
					Res = iterator.GetData()->Process();

				}
				catch (EQC::Common::EQCException Exception) { //Cofruben: Let's try to catch our exceptions.
					struct in_addr  in;
					in.s_addr = iterator.GetData()->GetIP();
					cout << "-EQC EXCEPTION caught from user " << inet_ntoa(in) << endl;
					Exception.FileDumpException("World_Exceptions.txt");
					iterator.RemoveCurrent();
				}
				catch (...) {
					struct in_addr  in;
					in.s_addr = iterator.GetData()->GetIP();
					cout << "-EXCEPTION caught from user  " << inet_ntoa(in) << endl;
					iterator.RemoveCurrent();
				}
				if (!Res)
				{
					struct in_addr in;
					in.s_addr = iterator.GetData()->GetIP();
					cout << "Removing client from ip:" << inet_ntoa(in) << " port:" << ntohs((int16)(iterator.GetData()->GetPort())) << endl;
					//Yeahlight: Log the account out of the database if they are not entering the world
					if(!iterator.GetData()->GetIgnoreLogOut())
					{
						Database::Instance()->LogAccountOut(iterator.GetData()->GetClientID());
					}
					iterator.GetData()->SetIgnoreLogOut(false);
					iterator.RemoveCurrent();
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

		void ClientList::ZoneBootup(ZoneServer* zs)
		{
			LinkedListIterator<Client*> iterator(list);

			iterator.Reset();

			while(iterator.MoreElements())
			{
				if (iterator.GetData()->WaitingForBootup())
				{
					if (strcasecmp(iterator.GetData()->GetZoneName(), zs->GetZoneName()) == 0) 
					{
						iterator.GetData()->EnterWorld(false);
					}
					else if (iterator.GetData()->WaitingForBootup() == (bool)zs->GetID())
					{
						iterator.GetData()->SendZoneUnavail();
					}
				}
				iterator.Advance();
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
		}
	}
}