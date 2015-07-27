// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef CONSOLELIST_H
#define CONSOLELIST_H

#include "Console.h"
#include "TCPConnection.h"

namespace EQC
{
	namespace World
	{
		namespace Network
		{
		class ConsoleList
		{
		public:
			ConsoleList() {}
			~ConsoleList() {}
				
			void Add(Console* con);
			void ReceiveData();

			void SendConsoleWho(char* to, int8 admin, TCPConnection* connection);
			Console* FindByAccountName(char* accname);
		private:
			LinkedList<Console*> list;
		};
	}
}
}
#endif
