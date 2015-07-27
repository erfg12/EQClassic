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
#include <fstream>
#include <ctime>
#include "config.h"
#ifndef WIN32
	#include <stdarg.h>
#endif

#include "packet_dump_file.h"
using namespace std;

void FileDumpPacketAscii(char* filename, uchar* buf, int32 size, int32 cols, int32 skip)
{
	ofstream logfile(filename, ios::app);
	// Output as ASCII
	for(int i=skip; i<size; i++)
	{
		if ((i-skip)%cols==0)
		{
			logfile << endl << setw(3) << i-skip << ":";
		}
		else if ((i-skip)%(cols/2)==0)
		{
			logfile << " - ";
		}
		if (buf[i] > 32 && buf[i] < 127)
		{
			logfile << buf[i];
		}
		else
		{
			logfile << '.';
		}
	}
	logfile << endl << endl;
}

void oldFileDumpPacketHex(char* filename, uchar* buf, int32 size, int32 cols, int32 skip)
{
	ofstream logfile(filename, ios::app);
	// Output as HEX
	char output[4];
    for(int i=skip; i<size; i++)
    {
		if ((i-skip)%cols==0)
		{
			logfile << endl << setw(3) << i-skip << ": ";
		}
		else if ((i-skip)%(cols/2) == 0)
		{
			logfile << "- ";
		}
		sprintf(output, "%02X ",(unsigned char)buf[i]);
		logfile << output;
//		logfile << setfill(0) << setw(2) << hex << (int)buf[i] << " ";
    }	
	logfile << endl << endl;
}

void FileDumpPacketHex(char* filename, uchar* buf, int32 size, int32 cols, int32 skip)
{
	if (size == 0)
		return;
	ofstream logfile(filename, ios::app);
	// Output as HEX
	char output[4];
	int j = 0; char* ascii = new char[cols+1]; memset(ascii, 0, cols+1);
	int i;
    for(i=skip; i<size; i++)
    {
		if ((i-skip)%cols==0) {
			if (i != skip)
				logfile << " | " << ascii << endl;
			logfile << setw(4) << i-skip << ": ";
			memset(ascii, 0, cols+1);
			j = 0;
		}
		else if ((i-skip)%(cols/2) == 0) {
			logfile << "- ";
		}
		sprintf(output, "%02X ", (unsigned char)buf[i]);
		logfile << output;

		if (buf[i] >= 32 && buf[i] < 127) {
			ascii[j++] = buf[i];
		}
		else {
			ascii[j++] = '.';
		}
//		logfile << setfill(0) << setw(2) << hex << (int)buf[i] << " ";
    }
	int k = ((i-skip)-1)%cols;
	if (k < 8)
		logfile << "  ";
	for (int h = k+1; h < cols; h++) {
		logfile << "   ";
	}
	logfile << " | " << ascii << endl;
	safe_delete(ascii);//delete ascii;
}

void FileDumpPacketHex(char* filename, APPLAYER* app)
{
	FileDumpPacketHex(filename, app->pBuffer, app->size);
}

void FileDumpPacketAscii(char* filename, APPLAYER* app)
{
	FileDumpPacketAscii(filename, app->pBuffer, app->size);
}

void FileDumpPacket(char* filename, uchar* buf, int32 size)
{
	FilePrintLine(filename, true, "Size: %5i", size);
	FileDumpPacketHex(filename, buf, size);
//	FileDumpPacketAscii(filename, buf,size);
}

void FileDumpPacket(char* filename, APPLAYER* app)
{
	FilePrintLine(filename, true, "Size: %5i, OPCode: 0x%04x", app->size, app->opcode);
	FileDumpPacketHex(filename, app->pBuffer, app->size);
//	FileDumpPacketAscii(filename, app->pBuffer, app->size);
}

/*
	prints a line to the file. if text = 0, prints a blank line
	if prefix_timestamp specified, prints the current date/time to the file + ": " + text
*/
void FilePrintLine(char* filename, bool prefix_timestamp, char* text, ...) {
	ofstream logfile(filename, ios::app);
	if (prefix_timestamp) {
		time_t rawtime;
		struct tm* gmt_t;
		time(&rawtime);
		gmt_t = gmtime(&rawtime);
		logfile << (gmt_t->tm_year + 1900) << "/" << setw(2) << setfill('0') << (gmt_t->tm_mon + 1) << "/" << setw(2) << setfill('0') << gmt_t->tm_mday << " " << setw(2) << setfill('0') << gmt_t->tm_hour << ":" << setw(2) << setfill('0') << gmt_t->tm_min << ":" << setw(2) << setfill('0') << gmt_t->tm_sec << " GMT";
	}

	if (text != 0) {
		va_list argptr;
		char buffer[256];
		va_start(argptr, text);
		vsnprintf(buffer, 256, text, argptr);
		va_end(argptr);

		if (prefix_timestamp)
			logfile << ": ";
		logfile << buffer;
	}
	logfile << endl;
}

void FilePrint(char* filename, bool newline, bool prefix_timestamp, char* text, ...) {
	ofstream logfile(filename, ios::app);
	if (prefix_timestamp) {
		time_t rawtime;
		struct tm* gmt_t;
		time(&rawtime);
		gmt_t = gmtime(&rawtime);
		logfile << (gmt_t->tm_year + 1900) << "/" << setw(2) << setfill('0') << (gmt_t->tm_mon + 1) << "/" << setw(2) << setfill('0') << gmt_t->tm_mday << " " << setw(2) << setfill('0') << gmt_t->tm_hour << ":" << setw(2) << setfill('0') << gmt_t->tm_min << ":" << setw(2) << setfill('0') << gmt_t->tm_sec << " GMT";
	}

	if (text != 0) {
		va_list argptr;
		char buffer[256];
		va_start(argptr, text);
		vsnprintf(buffer, 256, text, argptr);
		va_end(argptr);

		if (prefix_timestamp)
			logfile << ": ";
		logfile << buffer;
	}

	if (newline)
		logfile << endl;
}

