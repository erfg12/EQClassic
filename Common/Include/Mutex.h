// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
//
// ***************************************************************

#ifndef MYMUTEX_H
#define MYMUTEX_H
#ifdef WIN32
	#include <windows.h>
#else
	#include <pthread.h>
#endif
#include "types.h"

class Mutex {
public:
	Mutex();
	~Mutex();

	void init() { }

	void lock();
	void unlock();
	bool trylock();
protected:
private:
#ifdef WIN32
	CRITICAL_SECTION CSMutex;
#else
	pthread_mutex_t CSMutex;
#endif
};

class LockMutex {
public:
	LockMutex(Mutex* in_mut) { mut = in_mut; mut->lock(); }
	~LockMutex() { mut->unlock(); }
private:
	Mutex* mut;
};

#endif

