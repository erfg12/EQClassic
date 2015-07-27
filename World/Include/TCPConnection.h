// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "types.h"

namespace EQC
{
	namespace World
	{
		namespace Network
		{
			class TCPConnection
			{
			public:
				TCPConnection() 
				{
				}

				~TCPConnection() 
				{
				}

				virtual void SendEmoteMessage(char* to, int32 to_guilddbid, int32 type, char* message, ...) { }
			};
		}
	}
}
#endif
