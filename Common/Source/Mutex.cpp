// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include "Mutex.h"
//#define DEBUG_MUTEX_CLASS
#ifdef DEBUG_MUTEX_CLASS
#include <iostream>
#endif

Mutex::Mutex() {
#ifdef DEBUG_MUTEX_CLASS
	cout << "Constructing Mutex" << endl;
#endif
#ifdef WIN32
	InitializeCriticalSection(&CSMutex);
#else
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&CSMutex, &attr);
	pthread_mutexattr_destroy(&attr);
	//	pthread_mutex_t CSMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	//	CSMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	pthread_mutex_lock(&CSMutex);
	pthread_mutex_unlock(&CSMutex);
#endif
}

Mutex::~Mutex() {
#ifdef DEBUG_MUTEX_CLASS
	cout << "Deconstructing Mutex" << endl;
#endif
#ifdef WIN32
	DeleteCriticalSection(&CSMutex);
#else
	//	pthread_mutex_destroy(&CSMutex);
#endif
}

void Mutex::lock() {
#ifdef DEBUG_MUTEX_CLASS
	cout << "Locking Mutex" << endl;
#endif
#ifdef WIN32
	EnterCriticalSection(&CSMutex);
#else
	pthread_mutex_lock(&CSMutex);
#endif
}

bool Mutex::trylock() {
#ifdef DEBUG_MUTEX_CLASS
	cout << "TryLocking Mutex" << endl;
#endif
#ifdef WIN32
#if(_WIN32_WINNT >= 0x0400)
	return TryEnterCriticalSection(&CSMutex);
#else
	EnterCriticalSection(&CSMutex);
	return true;
#endif
#else
	return (pthread_mutex_trylock(&CSMutex) == 0);
#endif
}

void Mutex::unlock() {
#ifdef DEBUG_MUTEX_CLASS
	cout << "Unlocking Mutex" << endl;
#endif
#ifdef WIN32
	LeaveCriticalSection(&CSMutex);
#else
	pthread_mutex_unlock(&CSMutex);
#endif
}


