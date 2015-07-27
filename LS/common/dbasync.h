#ifndef DBASYNC_H
#define DBASYNC_H
#include "../common/dbcore.h"

extern DBAsync*					dbasync;
extern DBAsyncFinishedQueue*	MTdbafq;

void AsyncLoadVariables();
#endif

