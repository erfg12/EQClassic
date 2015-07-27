// ***************************************************************
//  EQCUtils   ·  date: 26/09/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifndef EQC_PACKET_UTIL_HPP
#define EQC_PACKET_UTIL_HPP

#include <cstring>
#include "types.h"
#include "ServerPacket.h"

using namespace std;
using namespace EQC::Common::Network;

namespace EQC 
{
	namespace Common
	{
		ServerPacket* CreateServerPacket(int16 in_OpCode, int16 in_Size);
	}
}


#endif