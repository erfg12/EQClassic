// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef CLIENTLIST_H
#define CLIENTLIST_H

namespace EQC
{
	namespace World
	{
		class ClientList
		{
		public:
			ClientList();
			~ClientList();
			
			void	Add(Client* client);
			Client*	Get(int32 ip, int16 port);
			void	Process();

			void	ZoneBootup(ZoneServer* zs);
		private:
			LinkedList<Client*> list;
		};
	}
}
#endif