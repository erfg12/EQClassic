// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include "MiscFunctions.h"
#include "timer.h"
#include "../../zone/include/entity.h"
#include "../../zone/include/entitylist.h"
#include <cstring>
#ifdef WIN32
	#include <io.h>
#endif

#ifndef WIN32
	#include <stdarg.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif
#include "EQCException.hpp"

EntityList	entity_list;


int MakeAnyLenString(char** ret, const char* format, ...) {
	if (!format) EQC_EXCEPT("int MakeAnyLenString()", "format = NULL");

	int buf_len = 128;
    int chars = -1;
	va_list argptr;
	va_start(argptr, format);
	while (chars == -1 || chars >= buf_len) {
		safe_delete(*ret);
		if (chars == -1)
			buf_len *= 2;
		else
			buf_len = chars + 1;
		*ret = new char[buf_len];
		chars = vsnprintf(*ret, buf_len, format, argptr);
	}
	va_end(argptr);
	return chars;
}

int32 AppendAnyLenString(char** ret, int32* bufsize, int32* strlen, const char* format, ...) {
	if (!format) EQC_EXCEPT("int32 AppendAnyLenString()", "format = NULL");

	if (*bufsize == 0)
		*bufsize = 256;
    int chars = -1;
	char* oldret = 0;
	va_list argptr;
	va_start(argptr, format);
	while (chars == -1 || chars >= (*bufsize-*strlen)) {
		if (chars == -1)
			*bufsize *= 2;
		else
			*bufsize = *strlen + chars + 1;
		oldret = *ret;
		*ret = new char[*bufsize];
		if (oldret) {
			memcpy(*ret, oldret, *strlen);
			safe_delete(*ret);
		}
		chars = vsnprintf(&(*ret)[*strlen], (*bufsize-*strlen), format, argptr);
	}
	va_end(argptr);
	*strlen += chars;
	return *strlen;
}

int32 hextoi(char* num) {
	int len = strlen(num);
	if (len < 3)
		return 0;

	if (num[0] != '0' || (num[1] != 'x' && num[1] != 'X'))
		return 0;

	int32 ret = 0;
	int mul = 1;
	for (int i=len-1; i>=2; i--) {
		if (num[i] >= 'A' && num[i] <= 'F')
			ret += ((num[i] - 'A') + 10) * mul;
		else if (num[i] >= 'a' && num[i] <= 'f')
			ret += ((num[i] - 'a') + 10) * mul;
		else if (num[i] >= '0' && num[i] <= '9')
			ret += (num[i] - '0') * mul;
		else
			return 0;
		mul *= 16;
	}
	return ret;
}

//Random formulas for certain calculations to be added in the next few days. -Wizzel
int MakeRandomInt(int low, int high)
{
	if(low == high)
		return(low);
	return (int)MakeRandomFloat((double)low, (double)high + 0.999);
}

double MakeRandomFloat(double low, double high)
{
	_CP(MakeRandomFloat);
	if(low == high)
		return(low);
	
	static bool seeded=0;
	double diff = high - low;
  
	if(!diff) return low;
	if(diff < 0)
		diff = 0 - diff;

	if(!seeded)
	{
		srand(time(0) * (time(0) % (int)diff));
		seeded = true;
	}
  
	return (rand() / (double)RAND_MAX * diff + (low > high ? high : low));
}


sint32 filesize(FILE* fp) {
#ifdef WIN32
	return _filelength(_fileno(fp));
#else
	struct stat file_stat;
	fstat(fileno(fp), &file_stat);
	return (sint32) file_stat.st_size;
#endif
}

// normal strncpy doesnt put a null term on copied strings, this one does
// ref: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcecrt/htm/_wcecrt_strncpy_wcsncpy.asp
char* strn0cpy(char* dest, const char* source, int32 size) {
	if (!dest)
		return 0;
	if (size == 0 || source == 0) {
		dest[0] = 0;
		return dest;
	}
	strncpy(dest, source, size);
	dest[size - 1] = 0;
	return dest;
}


const char * itoa(int num) {
		static char temp[_ITOA_BUFLEN];
		memset(temp,0,_ITOA_BUFLEN);
		snprintf(temp,_ITOA_BUFLEN,"%d",num);
		return temp;
}

float EQ13toFloat(int d)
{
	return ( float(d)/float(1<<2));
}

float NewEQ13toFloat(int d)
{
	return ( float(d)/float(1<<6));
}

float EQ19toFloat(int d)
{
	return ( float(d)/float(1<<3));
}

int FloatToEQ13(float d)
{
	return int(d*float(1<<2));
}

int NewFloatToEQ13(float d)
{
	return int(d*float(1<<6));
}

int FloatToEQ19(float d)
{
	return int(d*float(1<<3));
}

/*
	Heading of 0 points in the pure positive Y direction

*/
int FloatToEQH(float d)
{
	return(int((360.0f - d) * float(1<<11)) / 360);
}

float EQHtoFloat(int d)
{
	return(360.0f - float((d * 360) >> 11));
}
