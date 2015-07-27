// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef CLIENT_H
#define CLIENT_H
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "Clientnetwork.h"

#include "EQPacketManager.h"
#include "linked_list.h"
#include "timer.h"
#include "zoneserver.h"
#include "config.h"
#include "APPlayer.h"

#define CLIENT_TIMEOUT 30000

using namespace EQC::Common::Network;
using namespace EQC::World::Network;

namespace EQC
{
	namespace World
	{
		class Client
		{
		public:
			Client(int32 ip, int16 port, int send_socket);
			~Client();
			
			bool	Process();
			void	ReceiveData(uchar* buf, int len);
			void	SendCharInfo();
			void	EnterWorld(bool TryBootup = true);
			void	SendZoneUnavail();

			int32 GetIP()
			{
				return ip;
			}

			int16 GetPort()		
			{
				return port;
			}

			char* GetZoneName()	
			{
				return zone_name;
			}

			int	GetAdmin()
			{
				return admin; 
			}

			bool WaitingForBootup()
			{ 
				return pwaitingforbootup; 
			}

			int32 GetClientID()
			{
				return account_id;
			}

			bool GetIgnoreLogOut() 
			{
				return ignoreLogOut;
			}

			void SetIgnoreLogOut(bool set)
			{
				ignoreLogOut = set;
			}

			ClientNetwork* network;
		private:
			

			int32	ip;
			int16	port;
			int		send_socket;
			int32	account_id;
			char	char_name[30];
			char	zone_name[MAX_ZONE_NAME];
			Timer*	timeout_timer;
			Timer*	autobootup_timeout;
			int32	pwaitingforbootup;
			int		admin;
			bool	ignoreLogOut;
			CharWeapon_Struct weapons;
			int32	ls_account;

			bool ProcessOP_CharacterCreate(APPLAYER* inpacket);
			void ProcessOP_NameApproval(APPLAYER* inpacket);
			void ProcessOP_EnterWorld(APPLAYER* inpacket);
			bool ProcessOP_SendLoginInfo(APPLAYER* inpacket);
			bool ProcessOP_DeleteCharacter(APPLAYER* inpacket);
			void ProcessOP_WearChange(APPLAYER* inpacket);
			void SendExpansionInfo();
			void SendNameApproval(char* name, uchar race, uchar clas);
			void SendServerMOTD();
			void SendGuildsList();
			void CloseClientPackageManager();

			APPLAYER *CreateNameApprovalPacket();
			GuildsListEntry_Struct CreateBlankGuildsListEntry_Struct();

			void SetRacialLanguages( PlayerProfile_Struct *pp ); // Dark-Prince 21/12/2007
			bool CheckCharCreateInfo(PlayerProfile_Struct *cc); // Dark-Prince 22/12/2007
		};
	}
}


#endif
