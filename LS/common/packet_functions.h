/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef PACKET_FUNCTIONS_H
#define PACKET_FUNCTIONS_H
#include "types.h"
class APPLAYER;

int32 roll(int32 in, int8 bits);
int64 roll(int64 in, int8 bits);
int32 rorl(int32 in, int8 bits);
int64 rorl(int64 in, int8 bits);

void SetEQChecksum(uchar* in_data, int32 in_length);
void EncryptProfilePacket(APPLAYER* app);
void EncryptProfilePacket(uchar* pBuffer, int32 size);

#define EncryptSpawnPacket EncryptZoneSpawnPacket
//void EncryptSpawnPacket(APPLAYER* app);
//void EncryptSpawnPacket(uchar* pBuffer, int32 size);

void EncryptZoneSpawnPacket(APPLAYER* app);
void EncryptZoneSpawnPacket(uchar* pBuffer, int32 size);

int DeflatePacket(unsigned char* in_data, int in_length, unsigned char* out_data, int max_out_length);
uint32 InflatePacket(uchar* indata, uint32 indatalen, uchar* outdata, uint32 outdatalen, bool iQuiet = false);
uint32 GenerateCRC(int32 b, int32 bufsize, uchar *buf);

#endif
