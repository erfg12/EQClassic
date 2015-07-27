#include "unix.h"
#include <cstring>
#include <ctype.h>

void Sleep(unsigned int x) {
	if (x > 0)
		usleep(x*1000);
}

char* _strupr(char* tmp) {
	int l = strlen(tmp);
	for (int x = 0; x < l; x++) {
		tmp[x] = toupper(tmp[x]);
	}
	return tmp;
}

