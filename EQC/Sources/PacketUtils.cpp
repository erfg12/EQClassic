// ***************************************************************
//  EQCUtils   ·  date: 26/09/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#include <cstdarg>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "PacketUtils.hpp"

using namespace std;

namespace EQC 
{
	namespace Common
	{
		// Creates a Server Packet, Pretty Simple, nothing flash
		// Dark-Prince 10/05/2008 - Code Consolidation
		ServerPacket* CreateServerPacket(int16 in_OpCode, int16 in_Size)
		{
			// Create a new ServerPacket, Set Opcode & size
			ServerPacket* pack = new ServerPacket(in_OpCode, in_Size);

			// Initilize the buffer
			//pack->pBuffer = new uchar[pack->size]; Tazadar : this is a memory leak !

			// Zero out the buffer
			memset(pack->pBuffer, 0, pack->size);

			return pack;
		}
	}
}