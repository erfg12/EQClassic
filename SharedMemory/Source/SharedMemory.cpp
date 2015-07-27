// ***************************************************************
//  EQCException   ·  date: 31/08/2009
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// -Cofruben: initial release on 31/08/09.
// ***************************************************************
#include <iostream>
#include "EQCUtils.hpp"
#include "SharedMemory.hpp"

using namespace EQC::Common;
using namespace std;

HINSTANCE	SharedMemory::hSharedDLL = NULL;
HANDLE		SharedMemory::hSharedMapObject = NULL;
LPVOID		SharedMemory::lpvSharedMem = NULL;

////////////////////////////////////////////////////////////

SharedMemory::SharedMemory()
{
	hSharedDLL = NULL;
	hSharedMapObject = NULL;
	lpvSharedMem = NULL;
}

////////////////////////////////////////////////////////////

SharedMemory::~SharedMemory()
{
	Unload();
}
////////////////////////////////////////////////////////////

bool SharedMemory::Load()
{
	bool			firstInit;
	int				errn = NULL;
	const LPCWSTR	SHMEMNAME		= L"EQCSharedMem";
	const LPCWSTR	SHMEMDLLNAME	= L"SharedMemory.dll";
	const uint32	SHMEMSIZE		= SharedMemory::CalcSMSize();

	if (isLoaded())
		return true;
	hSharedDLL = LoadLibrary(SHMEMDLLNAME);
	if (hSharedDLL == NULL) {
		errn = GetLastError();
		EQC::Common::PrintF(CP_SHAREDMEMORY, "Error loading DLL: %i\n", errn);
		return false;
	}

	// 1. We create/open the "file".
	hSharedMapObject = CreateFileMapping (
			INVALID_HANDLE_VALUE,			// We use system paging file.
			NULL,							// Default.
			PAGE_READWRITE,					// Read & Write access.
			0,								// Size (64bits): higher 32 bits.
			SHMEMSIZE,						// Size (64bits): lower 32 bits.
			SHMEMNAME);						// File name.
	firstInit = GetLastError() != ERROR_ALREADY_EXISTS;
	if (!hSharedMapObject) {
		errn = firstInit;
		EQC::Common::PrintF(CP_SHAREDMEMORY, "Error creating filemapping: %i\n", errn);
		return false;
	}
	// 2. We now map the file into our process address space.
	lpvSharedMem = MapViewOfFile(
			hSharedMapObject,		// Our file.
			FILE_MAP_WRITE,			// Read & write.
			0,						// Offset(64bits): higher 32bits.
			0,						// Offset(64bits): lower 32bits, beggining.
			0);						// Size to map: all.
	if (!lpvSharedMem) {
		errn = GetLastError();
		EQC::Common::PrintF(CP_SHAREDMEMORY, "Error mapping file: %i\n", errn);
		return false;
	}
	// 3. If we've just created it, clean the memory and load stuff.
	if (firstInit) {
		memset(lpvSharedMem, 0, SHMEMSIZE);
		LoadItems();
	}

	if (isLoaded())
	{
		EQC::Common::PrintF(CP_SHAREDMEMORY,"SharedMemory.dll loaded. First init: %i\n", firstInit);
		return true;
	}
	else
	{
		EQC::Common::PrintF(CP_SHAREDMEMORY,"SharedMemory.dll failed to load.\n");
		return false;
	}
}

////////////////////////////////////////////////////////////

uint32 SharedMemory::CalcSMSize(){
	const uint32 step = 4096;
	uint32 size = 0;

	while (size < sizeof(ShMemData_Struct))
		size += step;
	return size;
}

////////////////////////////////////////////////////////////

bool SharedMemory::isLoaded()
{
	return hSharedDLL != NULL && hSharedMapObject != NULL && lpvSharedMem != NULL;
}

////////////////////////////////////////////////////////////

void SharedMemory::Unload()
{
	if (isLoaded()) return;

	UnloadItems();
	// Unmap shared memory from the process's address space
	UnmapViewOfFile(lpvSharedMem); 
	lpvSharedMem = NULL;

	// Close the process's handle to the file-mapping object
	CloseHandle(hSharedMapObject); 
	hSharedMapObject = NULL;

	// Close the DLL handle.
	FreeLibrary(hSharedDLL);
	safe_delete(hSharedDLL);
}

////////////////////////////////////////////////////////////

SharedMemory::ShMemData_Struct* SharedMemory::getPtr()
{
	return (ShMemData_Struct*)SharedMemory::lpvSharedMem;
}