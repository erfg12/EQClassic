#include "debug.h"

// Debug Levels
////// File/Console options
// 0 <= Quiet mode Errors to file Status and Normal ignored
// 1 >= Status and Normal to console, Errors to file
// 2 >= Status, Normal, and Error  to console and logfile
// 3 >= Lite debug
// 4 >= Medium debug
// 5 >= Debug release (Anything higher is not recommended for regular use)
// 6 == (Reserved for special builds) Login  opcode debug All packets dumped
// 7 == (Reserved for special builds) Chat Opcode debug All packets dumped
// 8 == (Reserved for special builds) World opcode debug All packets dumped
// 9 == (Reserved for special builds) Zone Opcode debug All packets dumped
// 10 >= More than you ever wanted to know
//
/////
// Add more below to reserve for file's functions ect.
/////
// Any setup code based on defines should go here
//

#include <iostream>
#include <time.h>
#include <string.h>

using namespace std;

#ifdef WIN32
	#include <process.h>

	#define snprintf	_snprintf
	//#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
#else
	#include <sys/types.h>
	#include <unistd.h>
	#include <stdarg.h>
#endif
#include "../common/MiscFunctions.h"

EQEMuLog* LogFile = new EQEMuLog;
AutoDelete<EQEMuLog> adlf(&LogFile);

static const char* FileNames[EQEMuLog::MaxLogID] = { "logs/eqemu", "logs/eqemu", "logs/eqemu_error", "logs/eqemu_debug" };
static const char* LogNames[EQEMuLog::MaxLogID] = { "Status", "Normal", "Error", "Debug" };

EQEMuLog::EQEMuLog() {
	MOpen = new Mutex;
	MLog = new Mutex*[MaxLogID];
	fp = new FILE*[MaxLogID];
	pLogStatus = new int8[MaxLogID];
	for (int i=0; i<MaxLogID; i++) {
		fp[i] = 0;
		MLog[i] = new Mutex;
#if DEBUG >= 2
		pLogStatus[i] = 1 | 2;
#else
		pLogStatus[i] = 0;
#endif
	}
// TODO: Make this read from an ini or something, everyone has different opinions on what it should be
#if DEBUG < 2
	pLogStatus[Status] = 2;
	pLogStatus[Error] = 2;
#endif
}

EQEMuLog::~EQEMuLog() {
	for (int i=0; i<MaxLogID; i++) {
		if (fp[i])
			fclose(fp[i]);
		safe_delete(MLog[i]);
	}
	safe_delete(fp);
	safe_delete(MLog);
	safe_delete(pLogStatus);
	safe_delete(MOpen);
}

bool EQEMuLog::open(LogIDs id) {
	if (id >= MaxLogID) {
		return false;
    }
	LockMutex lock(MOpen);
	if (pLogStatus[id] & 4) {
		return false;
    }
    if (fp[id]) {
        //cerr<<"Warning: LogFile already open"<<endl;
        return true;
    }

	char exename[200] = "";
#if defined(WORLD)
	snprintf(exename, sizeof(exename), "_world");
#elif defined(ZONE)
	snprintf(exename, sizeof(exename), "_zone");
#endif
	char filename[200];
#ifndef NO_PIDLOG
	snprintf(filename, sizeof(filename), "%s%s_%04i.log", FileNames[id], exename, getpid());
#else
	snprintf(filename, sizeof(filename), "%s%s.log", FileNames[id], exename);
#endif
    fp[id] = fopen(filename, "a");
    if (!fp[id]) {
		cerr << "Failed to open log file: " << filename << endl;
		pLogStatus[id] |= 4; // set file state to error
        return false;
    }
    fputs("---------------------------------------------\n",fp[id]);
    write(id, "Starting Log: %s", filename);
    return true;
}

bool EQEMuLog::write(LogIDs id, const char *fmt, ...) {
	if (!this) {
		return false;
    }
	if (id >= MaxLogID) {
		return false;
    }
	bool dofile = false;
	if (pLogStatus[id] & 1) {
		dofile = open(id);
	}
	if (!(dofile || pLogStatus[id] & 2))
		return false;
	LockMutex lock(MLog[id]);

    time_t aclock;
    struct tm *newtime;
    
    time( &aclock );                 /* Get time in seconds */
    newtime = localtime( &aclock );  /* Convert time to struct */

    if (dofile)
#ifndef NO_PIDLOG
		fprintf(fp[id], "[%02d.%02d. - %02d:%02d:%02d] ", newtime->tm_mon+1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
#else
		fprintf(fp[id], "%04i [%02d.%02d. - %02d:%02d:%02d] ", getpid(), newtime->tm_mon+1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
#endif

	va_list argptr;
	va_start(argptr, fmt);
    if (dofile)
		vfprintf( fp[id], fmt, argptr );
    if (pLogStatus[id] & 2) {
		if (pLogStatus[id] & 8) {
			fprintf(stderr, "[%s] ", LogNames[id]);
			vfprintf( stderr, fmt, argptr );
		}
		else {
			fprintf(stdout, "[%s] ", LogNames[id]);
			vfprintf( stdout, fmt, argptr );
		}
	}
	va_end(argptr);
    if (dofile)
	fprintf(fp[id], "\n");
    if (pLogStatus[id] & 2) {
		if (pLogStatus[id] & 8)
			fprintf(stderr, "\n");
		else
			fprintf(stdout, "\n");
	}
    if(dofile)
      fflush(fp[id]);
    return true;
};

bool EQEMuLog::writeNTS(LogIDs id, bool dofile, const char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	if (dofile)
		vfprintf( fp[id], fmt, argptr );
    if (pLogStatus[id] & 2) {
		if (pLogStatus[id] & 8)
			vfprintf( stderr, fmt, argptr );
		else
			vfprintf( stdout, fmt, argptr );
	}
	va_end(argptr);
    return true;
};

bool EQEMuLog::Dump(LogIDs id, int8* data, int32 size, int32 cols, int32 skip) {
	if (!this) {
#if DEBUG >= 10
    cerr << "Error: Dump() from null pointer"<<endl;
#endif
		return false;
    }
	if (size == 0)
		return true;
	if (!LogFile)
		return false;
	if (id >= MaxLogID)
		return false;
	bool dofile = false;
	if (pLogStatus[id] & 1) {
		dofile = open(id);
	}
	if (!(dofile || pLogStatus[id] & 2))
		return false;
	LockMutex lock(MLog[id]);
	write(id, "Dumping Packet: %i", size);
	// Output as HEX
	int j = 0; char* ascii = new char[cols+1]; memset(ascii, 0, cols+1);
	int32 i;
    for(i=skip; i<size; i++) {
		if ((i-skip)%cols==0) {
			if (i != skip)
				writeNTS(id, dofile, " | %s\n", ascii);
			writeNTS(id, dofile, "%4i: ", i-skip);
			memset(ascii, 0, cols+1);
			j = 0;
		}
		else if ((i-skip)%(cols/2) == 0) {
			writeNTS(id, dofile, "- ");
		}
		writeNTS(id, dofile, "%02X ", (unsigned char)data[i]);

		if (data[i] >= 32 && data[i] < 127)
			ascii[j++] = data[i];
		else
			ascii[j++] = '.';
    }
	int32 k = ((i-skip)-1)%cols;
	if (k < 8)
		writeNTS(id, dofile, "  ");
	for (int32 h = k+1; h < cols; h++) {
		writeNTS(id, dofile, "   ");
	}
	writeNTS(id, dofile, " | %s\n", ascii);
	if (dofile)
		fflush(fp[id]);
	delete ascii;
	return true;
}
