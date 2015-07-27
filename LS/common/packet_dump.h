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
#ifndef PACKET_DUMP_H
#define PACKET_DUMP_H

#include <iostream>

using namespace std;

#include "../common/types.h"

class APPLAYER;
class ServerPacket;

void DumpPacketAscii(const uchar* buf, int32 size, int32 cols=16, int32 skip=0);
void DumpPacketHex(const uchar* buf, int32 size, int32 cols=16, int32 skip=0);
void DumpPacketHex(const APPLAYER* app);
void DumpPacketAscii(const APPLAYER* app);
void DumpPacketBin(const void* data, int32 len);
void DumpPacket(const uchar* buf, int32 size);
void DumpPacket(const APPLAYER* app, bool iShowInfo = false);
void DumpPacket(const ServerPacket* pack, bool iShowInfo = false);
void DumpPacketBin(const APPLAYER* app);
void DumpPacketBin(const ServerPacket* pack);
void DumpPacketBin(int32 data);
void DumpPacketBin(int16 data);
void DumpPacketBin(int8 data);

#endif
