// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef PACKET_FUNCTIONS_H
#define PACKET_FUNCTIONS_H
#include "types.h"
#include "EQPacketManager.h"

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			void SetEQChecksum(uchar* in_data, int32 in_length);
			void EncryptProfilePacket(APPLAYER* app);
			void EncryptSpawnPacket(APPLAYER* app);
			void EncryptSpawnPacket(uchar* pBuffer, int16 size);
			void EncryptZoneSpawnPacket(uchar* pBuffer, int16 size);
			void EncryptZoneSpawnPacket(APPLAYER* app);
			int DeflatePacket(unsigned char* in_data, int in_length, unsigned char* out_data, int max_out_length);
			uint32 InflatePacket(uchar* indata, uint32 indatalen, uchar* outdata, uint32 outdatalen);
		}
	}
}
#endif
