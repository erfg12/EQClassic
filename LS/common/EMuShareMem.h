#ifndef EMuShareMem_H
#define EMuShareMem_H
#ifdef WIN32
#include <windows.h>
#else
#include "../common/unix.h"
#endif
#include "../common/eq_packet_structs.h"
#include "../zone/zonedump.h"
#include "../zone/loottable.h"

////////////
// Items //
///////////
typedef bool(*CALLBACK_DBLoadItems)(sint32, int32);

typedef bool(*DLLFUNC_DLLLoadItems)(const CALLBACK_DBLoadItems, int32, sint32*, int32*);
typedef const Item_Struct*(*DLLFUNC_GetItem)(uint32);
typedef const Item_Struct*(*DLLFUNC_IterateItems)(uint32*);
typedef bool(*DLLFUNC_AddItem)(int32, const Item_Struct*);

struct ItemsDLLFunc_Struct {
	DLLFUNC_DLLLoadItems DLLLoadItems;
	DLLFUNC_GetItem GetItem;
	DLLFUNC_IterateItems IterateItems;
	DLLFUNC_AddItem cbAddItem;
};

typedef bool(*CALLBACK_DBLoadNPCTypes)(sint32, int32);

typedef bool(*DLLFUNC_DLLLoadNPCTypes)(const CALLBACK_DBLoadNPCTypes, int32, sint32*, int32*);
typedef const NPCType*(*DLLFUNC_GetNPCType)(int32);
typedef bool(*DLLFUNC_AddNPCType)(int32, const NPCType*);
struct NPCTypesDLLFunc_Struct {
	DLLFUNC_DLLLoadNPCTypes DLLLoadNPCTypes;
	DLLFUNC_GetNPCType GetNPCType;
	DLLFUNC_AddNPCType cbAddNPCType;
};

typedef bool(*CALLBACK_DBLoadDoors)(sint32, int32);

typedef bool(*DLLFUNC_DLLLoadDoors)(const CALLBACK_DBLoadDoors, int32, sint32*, int32*);
typedef const Door*(*DLLFUNC_GetDoor)(int32);
typedef bool(*DLLFUNC_AddDoor)(int32, const Door*);
struct DoorsDLLFunc_Struct {
	DLLFUNC_DLLLoadDoors DLLLoadDoors;
	DLLFUNC_GetDoor GetDoor;
	DLLFUNC_AddDoor cbAddDoor;
};

typedef bool(*CALLBACK_FileLoadSPDat)(void*, sint32);

typedef bool(*DLLFUNC_DLLLoadSPDat)(const CALLBACK_FileLoadSPDat, const void**, sint32*, int32);
struct SpellsDLLFunc_Struct {
	DLLFUNC_DLLLoadSPDat DLLLoadSPDat;
};

typedef bool(*CALLBACK_DBLoadNPCFactionLists)(sint32, int32);

typedef bool(*DLLFUNC_DLLLoadNPCFactionLists)(const CALLBACK_DBLoadNPCFactionLists, int32, sint32*, int32*, int8);
typedef const NPCFactionList*(*DLLFUNC_GetNPCFactionList)(int32);
typedef bool(*DLLFUNC_AddNPCFactionList)(int32, const NPCFactionList*);
typedef bool(*DLLFUNC_SetFaction)(int32, uint32*, sint32*);
struct NPCFactionListDLLFunc_Struct {
	DLLFUNC_DLLLoadNPCFactionLists DLLLoadNPCFactionLists;
	DLLFUNC_GetNPCFactionList GetNPCFactionList;
	DLLFUNC_AddNPCFactionList cbAddNPCFactionList;
	DLLFUNC_SetFaction cbSetFaction;
};

typedef bool(*CALLBACK_DBLoadLoot)();

typedef bool(*DLLFUNC_DLLLoadLoot)(const CALLBACK_DBLoadLoot, int32, int32, int32, int32, int32, int32, int32, int32, int32, int32);
typedef bool(*DLLFUNC_AddLootTable)(int32, const LootTable_Struct*);
typedef bool(*DLLFUNC_AddLootDrop)(int32, const LootDrop_Struct*);
typedef const LootTable_Struct*(*DLLFUNC_GetLootTable)(uint32);
typedef const LootDrop_Struct*(*DLLFUNC_GetLootDrop)(uint32);
struct LootDLLFunc_Struct {
	DLLFUNC_DLLLoadLoot DLLLoadLoot;
	DLLFUNC_AddLootTable cbAddLootTable;
	DLLFUNC_AddLootDrop cbAddLootDrop;
	DLLFUNC_GetLootTable GetLootTable;
	DLLFUNC_GetLootDrop GetLootDrop;
};

class LoadEMuShareMemDLL {
public:
	LoadEMuShareMemDLL();
	~LoadEMuShareMemDLL();

	inline bool	Loaded() { return ((hDLL != NULL) && !(hDLL <= 0)); }
	bool	Load();
	void	Unload();

	ItemsDLLFunc_Struct				Items;
	NPCTypesDLLFunc_Struct			NPCTypes;
	DoorsDLLFunc_Struct				Doors;
	SpellsDLLFunc_Struct			Spells;
	NPCFactionListDLLFunc_Struct	NPCFactionList;
	LootDLLFunc_Struct				Loot;
private:
	void ClearFunc();

#ifdef WIN32
	HINSTANCE hDLL;
#else
	void* hDLL;
	static int32  refCount;
	static int32  refCountU() { return ++refCount; };
	static int32  refCountD() { return --refCount; };
#endif
};
#endif
