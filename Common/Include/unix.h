#ifndef __UNIX_H__
#define __UNIX_H__
	#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
		#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP {0, 0, 0, PTHREAD_MUTEX_RECURSIVE_NP, __LOCK_INITIALIZER}
	#endif
#include <unistd.h>

void Sleep(unsigned int x);
char* _strupr(char* tmp);
#endif
