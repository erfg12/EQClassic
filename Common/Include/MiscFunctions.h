// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
//
// ***************************************************************

#ifndef MISCFUNCTIONS_H
#define MISCFUNCTIONS_H

#include "types.h"
#include <cstdio>

int MakeAnyLenString(char** ret, const char* format, ...);
int32 AppendAnyLenString(char** ret, int32* bufsize, int32* strlen, const char* format, ...);
int32 hextoi(char* num);
int	MakeRandomInt(int low, int high);
double	MakeRandomFloat(double low, double high);
sint32 filesize(FILE* fp);

char*	strn0cpy(char* dest, const char* source, int32 size);

float EQ13toFloat(int d);
float NewEQ13toFloat(int d);
float EQ19toFloat(int d);
float EQHtoFloat(int d);
int FloatToEQ13(float d);
int NewFloatToEQ13(float d);
int FloatToEQ19(float d);
int FloatToEQH(float d);

#define _ITOA_BUFLEN	25
const char *itoa(int num);	//not thread safe

template<class T> class AutoDelete {
public:
	AutoDelete(T** iVar, T* iSetTo = 0) {
		init(iVar, iSetTo);
	}
	AutoDelete() { pVar = NULL; }
	void init(T** iVar, T* iSetTo = 0)
	{
		pVar = iVar;
		if (iSetTo)
			*pVar = iSetTo;
	}
	~AutoDelete() {
		if(pVar != NULL)
			safe_delete(*pVar);
	}
	void ReallyClearIt() {
		pVar = NULL;
	}
private:
	T** pVar;
};

#endif

