// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <iomanip>
#include <cstdio>

#include "packet_dump.h"
using namespace std;

void DumpPacketAscii(uchar* buf, int32 size, int32 cols, int32 skip)
{
	// Output as ASCII
	for(int32 i=skip; i<size; i++)
	{
		if ((i-skip)%cols==0)
		{
			cout << endl << setw(3) << i-skip << ":";
		}
		else if ((i-skip)%(cols/2)==0)
		{
			cout << " - ";
		}
		if (buf[i] > 32 && buf[i] < 127)
		{
			cout << buf[i];
		}
		else
		{
			cout << '.';
		}
	}
	cout << endl << endl;
}

void DumpPacketHex(uchar* buf, int32 size, int32 cols, int32 skip)
{
	if (size == 0)
		return;
	// Output as HEX
	char output[4];
	int j = 0; char* ascii = new char[cols+1]; memset(ascii, 0, cols+1);
	int32 i;
    for(i=skip; i<size; i++)
    {
		if ((i-skip)%cols==0) {
			if (i != skip)
				cout << " | " << ascii << endl;
			cout << setw(4) << i-skip << ": ";
			memset(ascii, 0, cols+1);
			j = 0;
		}
		else if ((i-skip)%(cols/2) == 0) {
			cout << "- ";
		}
		sprintf(output, "%02X ", (unsigned char)buf[i]);
		cout << output;

		if (buf[i] >= 32 && buf[i] < 127) {
			ascii[j++] = buf[i];
		}
		else {
			ascii[j++] = '.';
		}
//		cout << setfill(0) << setw(2) << hex << (int)buf[i] << " ";
    }
	int k = ((i-skip)-1)%cols;
	if (k < 8)
		cout << "  ";
	for (int h = k+1; h < cols; h++) {
		cout << "   ";
	}
	cout << " | " << ascii << endl;
	safe_delete(ascii);//delete ascii;
}

void DumpPacketHex(APPLAYER* app)
{
	DumpPacketHex(app->pBuffer, app->size);
}

void DumpPacketAscii(APPLAYER* app)
{
	DumpPacketAscii(app->pBuffer, app->size);
}

void DumpPacket(uchar* buf, int32 size)
{
	DumpPacketHex(buf, size);
//	DumpPacketAscii(buf,size);
}

void DumpPacket(APPLAYER* app)
{
	DumpPacketHex(app->pBuffer, app->size);
//	DumpPacketAscii(app->pBuffer, app->size);
}

void DumpPacketBin(APPLAYER* app){
	uchar* buf = app->pBuffer;
	int32 size = app->size; 
	int32 cols = 16;
	int32 skip = 0;
	
	if (size == 0)
		return;
	// Output as HEX
	char output[4];
	int j = 0; char* ascii = new char[cols+1]; memset(ascii, 0, cols+1);
	int32 i;
	for(i=skip; i<size; i++)
	{
		if ((i-skip)%cols==0) {
			if (i != skip)
				cout << " | " << ascii << endl;
			cout << setw(4) << i-skip << ": ";
			memset(ascii, 0, cols+1);
			j = 0;
		}
		else if ((i-skip)%(cols/2) == 0) {
			cout << "- ";
		}
		sprintf(output, "%02X ", (unsigned char)buf[i]);
		cout << output;

		if (buf[i] >= 32 && buf[i] < 127) {
			ascii[j++] = buf[i];
		}
		else {
			ascii[j++] = '.';
		}
		//		cout << setfill(0) << setw(2) << hex << (int)buf[i] << " ";
	}
	int k = ((i-skip)-1)%cols;
	if (k < 8)
		cout << "  ";
	for (int h = k+1; h < cols; h++) {
		cout << "   ";
	}
	cout << " | " << ascii << endl;
	safe_delete(ascii);//delete ascii;
}


void PrintBinary(uchar* buf){
	const unsigned int bits = 8;
	
	int value = 0xF0; //11110000 in big-endian
	bool binary[ bits ];	

	//Convert:
	for ( unsigned int i = 0 ; i < bits ; ++i ) {
		binary[ i ] = (value >> i) & 1;
	}
	//Display:
	for ( unsigned int i = 0 ; i < bits ; ++i ) {
		cout << (binary[i] ? "1" : "0");
	}

}
