#include "EQEMuError.h"
#include "linked_list.h"
#include "Mutex.h"
#include "MiscFunctions.h"
#include <stdio.h>
#include <string.h>
#ifdef WIN32
	#include <conio.h>
#endif

void UpdateWindowTitle(char* iNewTitle = 0);
void CatchSignal(int sig_num);

char* EQEMuErrorText[EQEMuError_MaxErrorID] = { "ErrorID# 0, No Error",
	"MySQL Error #1405 or #2001 means your mysql server rejected the username and password you presented it.",
	"MySQL Error #2003 means you were unable to connect to the mysql server.",
	"MySQL Error #2005 means you there are too many connections on the mysql server. The server is overloaded.",
	"MySQL Error #2007 means you the server is out of memory. The server is overloaded.",
	};

LinkedList<char*>* EQEMuErrorList;
Mutex* MEQEMuErrorList;
AutoDelete<void> ADEQEMuErrorList((void**) &EQEMuErrorList);
AutoDelete<Mutex> ADMEQEMuErrorList(&MEQEMuErrorList);

char* GetErrorText(int32 iError) {
	if (iError >= EQEMuError_MaxErrorID)
		return "ErrorID# out of range";
	else
		return EQEMuErrorText[iError];
}

void AddEQEMuError(eEQEMuError iError, bool iExitNow) {
	if (!iError)
		return;
	if (!EQEMuErrorList) {
		EQEMuErrorList = new LinkedList<char*>;
		MEQEMuErrorList = new Mutex;
	}
	LockMutex lock(MEQEMuErrorList);

	LinkedListIterator<char*> iterator(*EQEMuErrorList);
	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()[0] == 1) {
			if (*((int32*) iterator.GetData()[1]) == iError)
				return;
		}
		iterator.Advance();
	}
	
	char* tmp = new char[6];
	tmp[0] = 1;
	tmp[5] = 0;
	*((int32*) &tmp[1]) = iError;
	EQEMuErrorList->Append(tmp);

	if (iExitNow)
		CatchSignal(0);
}

void AddEQEMuError(char* iError, bool iExitNow) {
	if (!iError)
		return;
	if (!EQEMuErrorList) {
		EQEMuErrorList = new LinkedList<char*>;
		MEQEMuErrorList = new Mutex;
	}
	LockMutex lock(MEQEMuErrorList);
	char* tmp = strcpy(new char[strlen(iError) + 1], iError);
	EQEMuErrorList->Append(tmp);

	if (iExitNow)
		CatchSignal(0);
}

int32 CheckEQEMuError() {
	if (!EQEMuErrorList)
		return 0;
	int32 ret = 0;
	char* tmp = 0;
	bool HeaderPrinted = false;
	LockMutex lock(MEQEMuErrorList);

	while (tmp = EQEMuErrorList->Pop()) {
		if (!HeaderPrinted) {
			fprintf(stdout, "===============================\nRuntime errors:\n\n");
			HeaderPrinted = true;
		}
		if (tmp[0] == 1) {
			fprintf(stdout, "%s\n", GetErrorText(*((int32*) &tmp[1])));
			fprintf(stdout, "For more information on this error, visit http://www.eqemu.net/eqemuerror.php?id=%u\n\n", *((int32*) &tmp[1]));
		}
		else {
			fprintf(stdout, "%s\n\n", tmp);
		}
		safe_delete(tmp);
		ret++;
	}
	return ret;
}

void CheckEQEMuErrorAndPause() {
#ifdef WIN32
	if (CheckEQEMuError()) {
		fprintf(stdout, "Hit any key to exit\n");
		UpdateWindowTitle("Error");
		getch();
	}
#endif
}

