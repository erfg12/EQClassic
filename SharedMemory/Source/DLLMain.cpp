// ***************************************************************
//  EQCException   ·  date: 31/08/2009
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// -Cofruben: initial release on 31/08/09.
// ***************************************************************
#include <windows.h>
#include "SharedMemory.hpp"

BOOL WINAPI DllMain(
					HINSTANCE hinstDLL,  // handle to DLL module
					DWORD fdwReason,     // reason for calling function
					LPVOID lpReserved )
{
	switch( fdwReason ) 
	{ 
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		SharedMemory::Unload();
		break;
	}
	return TRUE;
}
