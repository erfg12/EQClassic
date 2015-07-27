// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
//
// ***************************************************************

#ifndef PACKET_DUMP_H
#define PACKET_DUMP_H

#include <iostream>

#include "EQPacketManager.h"
#include "APPlayer.h"

using namespace EQC::Common::Network;

class EQC::Common::Network::APPLAYER;
//class APPLAYER;

void DumpPacketAscii(uchar* buf, int32 size, int32 cols=16, int32 skip=0);
void DumpPacketHex(uchar* buf, int32 size, int32 cols=16, int32 skip=0);
void DumpPacketHex(APPLAYER* app);
void DumpPacketAscii(APPLAYER* app);
void DumpPacket(uchar* buf, int32 size);
void DumpPacket(APPLAYER* app);
void DumpPacketBin(APPLAYER* app);

#endif
