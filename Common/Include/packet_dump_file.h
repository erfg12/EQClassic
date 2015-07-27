// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
//
// ***************************************************************

#ifndef PACKET_DUMP_FILE_H
#define PACKET_DUMP_FILE_H

#include <iostream>
#include "EQPacketManager.h"

void FileDumpPacketAscii(char* filename, uchar* buf, int32 size, int32 cols=16, int32 skip=0);
void FileDumpPacketHex(char* filename, uchar* buf, int32 size, int32 cols=16, int32 skip=0);
void FileDumpPacketHex(char* filename, APPLAYER* app);
void FileDumpPacketAscii(char* filename, APPLAYER* app);
void FileDumpPacket(char* filename, uchar* buf, int32 size);
void FileDumpPacket(char* filename, APPLAYER* app);
void FilePrintLine(char* filename, bool prefix_timestamp = false, char* text = 0, ...);
void FilePrint(char* filename, bool newline, bool prefix_timestamp, char* text, ...);
#endif

