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
#ifndef DEBUG
#define DEBUG 0
#endif
#ifdef _DEBUG
	#ifndef _CRTDBG_MAP_ALLOC
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
		#define new \
        new(_NORMAL_BLOCK, __FILE__, __LINE__) 
	#endif
#endif

#ifndef ThrowError
	void CatchSignal(int);
	#if defined(CATCH_CRASH) || defined(_DEBUG)
		#define ThrowError(errstr)	{ cout << "Fatal error: " << errstr << " (" << __FILE__ << ", line " << __LINE__ << ")" << endl; LogFile->write(EQEMuLog::Error, "Thown Error: %s (%s:%i)", errstr, __FILE__, __LINE__); throw errstr; }
	#else
		#define ThrowError(errstr)	{ cout << "Fatal error: " << errstr << " (" << __FILE__ << ", line " << __LINE__ << ")" << endl; LogFile->write(EQEMuLog::Error, "Thown Error: %s (%s:%i)", errstr, __FILE__, __LINE__); CatchSignal(0); }
	#endif
#endif

#ifndef DEBUG_H
#define DEBUG_H
#ifdef WIN32
//	#define getpid()	_getpid()
#endif
#include "../common/Mutex.h"
#include <stdio.h>

class EQEMuLog {
public:
	EQEMuLog();
	~EQEMuLog();

	enum LogIDs { Status, Normal, Error, Debug, MaxLogID };

	bool write(LogIDs id, const char *fmt, ...);
	bool Dump(LogIDs id, int8* data, int32 size, int32 cols=16, int32 skip=0);
private:
	bool open(LogIDs id);
	bool writeNTS(LogIDs id, bool dofile, const char *fmt, ...); // no error checking, assumes is open, no locking, no timestamp, no newline

	Mutex*	MOpen;
	Mutex**	MLog;
	FILE**	fp;
/* LogStatus: bitwise variable
	1 = output to file
	2 = output to stdout
	4 = fopen error, dont retry
	8 = use stderr instead (2 must be set)
*/
	int8*	pLogStatus;
};

extern EQEMuLog* LogFile;

#ifdef _DEBUG
class PerformanceMonitor {
public:
	PerformanceMonitor(sint64* ip) {
		p = ip;
		QueryPerformanceCounter(&tmp);
	}
	~PerformanceMonitor() {
		LARGE_INTEGER tmp2;
		QueryPerformanceCounter(&tmp2);
		*p += tmp2.QuadPart - tmp.QuadPart;
	}
	LARGE_INTEGER tmp;
	sint64* p;
};
#endif
#endif

