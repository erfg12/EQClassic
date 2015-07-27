/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef MISCFUNCTIONS_H
#define MISCFUNCTIONS_H

#include "types.h"
#include <stdio.h>
#include <ctype.h>


#ifndef ERRBUF_SIZE
#define ERRBUF_SIZE		1024
#endif

//////////////////////////////////////////////////////////////////////
//
//  MakeUpperString
//   i     : source - allocated null-terminated string
//   return: pointer to static buffer with the target string
const char *MakeUpperString(const char *source);
//////////////////////////////////////////////////////////////////////
//
//  MakeUpperString
//   i : source - allocated null-terminated string
//   io: target - allocated buffer, at least of size strlen(source)+1
void MakeUpperString(const char *source, char *target);


int		MakeAnyLenString(char** ret, const char* format, ...);
int32	AppendAnyLenString(char** ret, int32* bufsize, int32* strlen, const char* format, ...);
int32	hextoi(char* num);
int64	hextoi64(char* num);
bool	atobool(char* iBool);
sint32	filesize(FILE* fp);
int32	ResolveIP(const char* hostname, char* errbuf = 0);
bool	ParseAddress(const char* iAddress, int32* oIP, int16* oPort, char* errbuf = 0);
void	CoutTimestamp(bool ms = true);
char*	strn0cpy(char* dest, const char* source, int32 size);
		// return value =true if entire string(source) fit, false if it was truncated
bool	strn0cpyt(char* dest, const char* source, int32 size);

#ifndef WIN32

const char *itoa(int num);
const char *itoa(int num, char* a,int b);

#endif

class InitWinsock {
public:
	InitWinsock();
	~InitWinsock();
};

template<class T> class AutoDelete {
public:
	AutoDelete(T** iVar, T* iSetTo = 0) {
		pVar = iVar;
		if (iSetTo)
			*pVar = iSetTo;
	}
	~AutoDelete() {
		safe_delete(*pVar);
	}
private:
	T** pVar;
};

#endif

