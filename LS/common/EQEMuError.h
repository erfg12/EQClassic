#ifndef EQEMuError_H
#define EQEMuError_H

#include "../common/types.h"

enum eEQEMuError { EQEMuError_NoError, 
	EQEMuError_Mysql_1405,
	EQEMuError_Mysql_2003,
	EQEMuError_Mysql_2005,
	EQEMuError_Mysql_2007,
	EQEMuError_MaxErrorID };

void AddEQEMuError(eEQEMuError iError, bool iExitNow = false);
void AddEQEMuError(char* iError, bool iExitNow = false);
int32 CheckEQEMuError();
void CheckEQEMuErrorAndPause();

#endif

