#include <iostream.h>
#include "../common/types.h"

#include "EMuShareMem.h"

#ifdef WIN32
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp

	#define EmuLibName "EMuShareMem"
#else
	#define EmuLibName "libEMuShareMem.so"

	#include "../common/unix.h"
	#include <dlfcn.h>
    #define GetProcAddress(a,b) dlsym(a,b)
	#define LoadLibrary(a) dlopen(a, RTLD_NOW) 
	#define  FreeLibrary(a) dlclose(a)
	#define GetLastError() dlerror()
#endif

LoadEMuShareMemDLL EMuShareMemDLL;

#ifndef WIN32
int32 LoadEMuShareMemDLL::refCount = 0;
#endif

LoadEMuShareMemDLL::LoadEMuShareMemDLL() {
	hDLL = 0;
	ClearFunc();
#ifndef WIN32
    refCountU();
#endif
}

LoadEMuShareMemDLL::~LoadEMuShareMemDLL() {
#ifndef WIN32
    if (refCountD() <= 0) {
#endif
	Unload();
#ifndef WIN32
    }
#endif
}

bool LoadEMuShareMemDLL::Load() {
#ifdef WIN32
	DWORD load_error = 0;
	SetLastError(0);
#else
	const char* load_error = 0;
#endif
	if (Loaded())
	{
		return true;
	}
	hDLL = LoadLibrary(EmuLibName);
#ifdef WIN32
	if(!hDLL) {
		load_error = GetLastError();
#else
	if(!hDLL || ((load_error = GetLastError()) != NULL) ) {
#endif
		cerr << "LoadEMuShareMemDLL::Load() failed to load library error=" << load_error << endl;
	    return false;
	}
#ifdef WIN32
    else { SetLastError(0); } // Clear the win9x error
#endif
	if (Loaded()) {
		Items.GetItem = (DLLFUNC_GetItem) GetProcAddress(hDLL, "GetItem");
		Items.IterateItems = (DLLFUNC_IterateItems) GetProcAddress(hDLL, "IterateItems");
		Items.cbAddItem = (DLLFUNC_AddItem) GetProcAddress(hDLL, "AddItem");
		Items.DLLLoadItems = (DLLFUNC_DLLLoadItems) GetProcAddress(hDLL, "DLLLoadItems");
		NPCTypes.GetNPCType = (DLLFUNC_GetNPCType) GetProcAddress(hDLL, "GetNPCType");
		NPCTypes.cbAddNPCType = (DLLFUNC_AddNPCType) GetProcAddress(hDLL, "AddNPCType");
		NPCTypes.DLLLoadNPCTypes = (DLLFUNC_DLLLoadNPCTypes) GetProcAddress(hDLL, "DLLLoadNPCTypes");
		Doors.GetDoor = (DLLFUNC_GetDoor) GetProcAddress(hDLL, "GetDoor");
		Doors.cbAddDoor = (DLLFUNC_AddDoor) GetProcAddress(hDLL, "AddDoor");
		Doors.DLLLoadDoors = (DLLFUNC_DLLLoadDoors) GetProcAddress(hDLL, "DLLLoadDoors");
		Spells.DLLLoadSPDat = (DLLFUNC_DLLLoadSPDat) GetProcAddress(hDLL, "DLLLoadSPDat");
		NPCFactionList.DLLLoadNPCFactionLists = (DLLFUNC_DLLLoadNPCFactionLists) GetProcAddress(hDLL, "DLLLoadNPCFactionLists");
		NPCFactionList.GetNPCFactionList = (DLLFUNC_GetNPCFactionList) GetProcAddress(hDLL, "GetNPCFactionList");
		NPCFactionList.cbAddNPCFactionList = (DLLFUNC_AddNPCFactionList) GetProcAddress(hDLL, "AddNPCFactionList");
		NPCFactionList.cbSetFaction = (DLLFUNC_SetFaction) GetProcAddress(hDLL, "SetNPCFaction");
		Loot.DLLLoadLoot = (DLLFUNC_DLLLoadLoot) GetProcAddress(hDLL, "DLLLoadLoot");
		Loot.cbAddLootTable = (DLLFUNC_AddLootTable) GetProcAddress(hDLL, "AddLootTable");
		Loot.cbAddLootDrop = (DLLFUNC_AddLootDrop) GetProcAddress(hDLL, "AddLootDrop");
		Loot.GetLootTable = (DLLFUNC_GetLootTable) GetProcAddress(hDLL, "GetLootTable");
		Loot.GetLootDrop = (DLLFUNC_GetLootDrop) GetProcAddress(hDLL, "GetLootDrop");
		if ((!Items.GetItem)
			|| (!Items.IterateItems)
			|| (!Items.cbAddItem)
			|| (!Items.DLLLoadItems)
			|| (!NPCTypes.GetNPCType)
			|| (!NPCTypes.cbAddNPCType)
			|| (!NPCTypes.DLLLoadNPCTypes)
			|| (!Doors.GetDoor)
			|| (!Doors.cbAddDoor)
			|| (!Doors.DLLLoadDoors)
			|| (!Spells.DLLLoadSPDat)
			|| (!NPCFactionList.DLLLoadNPCFactionLists)
			|| (!NPCFactionList.GetNPCFactionList)
			|| (!NPCFactionList.cbAddNPCFactionList)
			|| (!NPCFactionList.cbSetFaction)
			|| (!Loot.DLLLoadLoot)
			|| (!Loot.cbAddLootTable)
			|| (!Loot.cbAddLootDrop)
			|| (!Loot.GetLootTable)
			|| (!Loot.GetLootDrop)
#ifndef WIN32
			|| ((load_error = GetLastError()) != NULL)
#else
			&& ((load_error = GetLastError()) != 0)
#endif
			) {
#ifdef WIN32
			load_error = GetLastError();
#endif
			Unload();
			cerr << "LoadEMuShareMemDLL::Load() failed to attach a function error=" << load_error << endl;
			return false;
		}
		cout << "EMuShareMem.dll loaded." << endl;
		return true;
	}
	else {
#ifdef WIN32
		if ((load_error = GetLastError()) != 0)
#else
		if ((load_error = GetLastError()) != NULL)
#endif 
			cout << "LoadLibrary() FAILED! error="<< load_error << endl;
		else
			cout << "LoadLibrary() FAILED! error= Unknown error" << endl;
	}
	return false;
}

void LoadEMuShareMemDLL::Unload() {
	ClearFunc();
	if (this->hDLL) {
		FreeLibrary(this->hDLL);
#ifndef WIN32
		const char* error;
		if ((error = GetLastError()) != NULL)
			cerr<<"FreeLibrary() error = "<<error<<endl;
#endif
	}
	hDLL = 0;
}

void LoadEMuShareMemDLL::ClearFunc() {
	Items.GetItem = 0;
	Items.IterateItems = 0;
	Items.cbAddItem = 0;
	Items.DLLLoadItems = 0;
	NPCTypes.GetNPCType = 0;
	NPCTypes.cbAddNPCType = 0;
	NPCTypes.DLLLoadNPCTypes = 0;
	Doors.GetDoor = 0;
	Doors.cbAddDoor = 0;
	Doors.DLLLoadDoors = 0;
	NPCFactionList.DLLLoadNPCFactionLists = 0;
	NPCFactionList.GetNPCFactionList = 0;
	NPCFactionList.cbAddNPCFactionList = 0;
	NPCFactionList.cbSetFaction = 0;
	Loot.DLLLoadLoot = 0;
	Loot.cbAddLootTable = 0;
	Loot.cbAddLootDrop = 0;
	Loot.GetLootTable = 0;
	Loot.GetLootDrop = 0;
}
