#ifndef EMuShareMem_H
#define EMuShareMem_H

#include <windows.h>
#include "types.h"
#include "eq_packet_structs.h"


typedef bool(*CALLBACK_DBLoadItems)(int16, int16);

typedef bool(*DLLFUNC_DLLLoadItems)(const CALLBACK_DBLoadItems, int32, int16, int16);
typedef const Item_Struct*(*DLLFUNC_GetItem)(uint16);
typedef const Item_Struct*(*DLLFUNC_IterateItems)(uint16*);
typedef bool(*DLLFUNC_AddItem)(int16, const Item_Struct*);
struct ItemsDLLFunc_Struct {
	DLLFUNC_DLLLoadItems DLLLoadItems;
	DLLFUNC_GetItem GetItem;
	DLLFUNC_IterateItems IterateItems;
	DLLFUNC_AddItem cbAddItem;
};

class LoadEMuShareMemDLL {
public:
	LoadEMuShareMemDLL();
	~LoadEMuShareMemDLL();

	inline bool	Loaded() { return (hDLL != NULL); }
	bool	Load();
	void	Unload();

	ItemsDLLFunc_Struct		Items;
private:
	HINSTANCE hDLL;
};

#endif