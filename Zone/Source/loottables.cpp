
#include <cstdio>
#include <iostream>
#include <cstdlib>

#include "database.h"
#include "config.h"

using namespace std;

// Queries the loottable: adds item & coin to the npc
void Database::AddLootTableToNPC(int32 loottable_id, ItemList* itemlist, int32* copper, int32* silver, int32* gold, int32* plat, int8 EquipmentList[], int32 EquipmentColorList[]) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	*copper = 0;
	*silver = 0;
	*gold = 0;
	*plat = 0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, mincash, maxcash, avgcoin FROM loottable WHERE id=%i", loottable_id), errbuf, &result)) {
		safe_delete(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 mincash = atoi(row[1]);
			int32 maxcash = atoi(row[2]);
			if (mincash > maxcash) {
				cerr << "Error in loottable #" << row[0] << ": mincash > maxcash" << endl;
			}
			else if (maxcash != 0) {
				int32 cash = 0;
				if (mincash == maxcash)
					cash = mincash;
				else
					cash = (rand() % (maxcash - mincash)) + mincash;
				if (cash != 0) {
					int32 coinavg = atoi(row[3]);
					if (coinavg != 0) {
						int32 mincoin = (int32) (coinavg * 0.75 + 1);
						int32 maxcoin = (int32) (coinavg * 1.25 + 1);
						*copper = (rand() % (maxcoin - mincoin)) + mincoin - 1;
						*silver = (rand() % (maxcoin - mincoin)) + mincoin - 1;
						*gold = (rand() % (maxcoin - mincoin)) + mincoin - 1;
						cash -= *copper;
						cash -= *silver * 10;
						cash -= *gold * 10;
					}
					*plat = cash / 1000;
					cash -= *plat * 1000;
					int32 gold2 = cash / 100;
					cash -= gold2 * 100;
					int32 silver2 = cash / 10;
					cash -= silver2 * 10;
					*gold += gold2;
					*silver += silver2;
					*copper += cash;
				}
			}
		}
		else {
			mysql_free_result(result);
			return;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in AddLootTableToNPC get coin query '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return;
	}

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT loottable_id, lootdrop_id, multiplier, probability FROM loottable_entries WHERE loottable_id=%i", loottable_id), errbuf, &result)) {
		safe_delete(query);
		while (row = mysql_fetch_row(result)) {
			int multiplier = atoi(row[2]);
			for (int i = 1; i <= multiplier; i++) {
				if ((rand() % 100) < atoi(row[3])) {
					AddLootDropToNPC(atoi(row[1]), itemlist, EquipmentList, EquipmentColorList);
				}
			}
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in AddLootTableToNPC get items query '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return;
	}

	return;
}

// Called by AddLootTableToNPC
// maxdrops = size of the array npcd
void Database::AddLootDropToNPC(int32 lootdrop_id, ItemList* itemlist, int8 EquipmentList[], int32 EquipmentColorList[]) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT lootdrop_id, item_id, item_charges, equip_item, chance FROM lootdrop_entries WHERE lootdrop_id=%i", lootdrop_id), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
		int32 maxchance = 0;
		while (row = mysql_fetch_row(result)) {
			maxchance += atoi(row[4]);
		}
		mysql_data_seek(result, 0); // go back to the beginning
		if (maxchance != 0) {
			int32 x = 0;
			int32 roll = (rand() % maxchance);
			while (row = mysql_fetch_row(result)) {
				x += atoi(row[4]);
				if (x > roll) {
					int32 itemid = atoi(row[1]);
					Item_Struct* dbitem = Database::Instance()->GetItem(itemid);
					if (dbitem == 0) 
					{
						cerr << "Error in AddLootDropToNPC: dbitem=0, item#=" << itemid << ", lootdrop_id=" << lootdrop_id << endl;
					}
					else
					{
						ServerLootItem_Struct* item = new ServerLootItem_Struct;
						item->item_nr = dbitem->item_nr;
						item->charges = atoi(row[2]);
						//Yeahlight: Rip the bitmask apart and seperate it into slots
						int16 slots[22] = {0};
						int itemSlots = dbitem->equipableSlots;
						int16 counter = 0;
						int16 slotToUse = 0;
						//Yeahlight: Keep iterating until we shift all the bits off the end
						while(itemSlots > 0)
						{
							if(itemSlots % 2)
								slots[counter] = 1;
							itemSlots = itemSlots >> 1;
							counter++;
							//Yeahlight: Zone freeze debug
							if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
								EQC_FREEZE_DEBUG(__LINE__, __FILE__);
						}
						//Yeahlight: Item fits in primary
						if(slots[13])
						{
							if(strlen(dbitem->idfile) >= 3)
								EquipmentList[7] = (int8) atoi(&dbitem->idfile[2]);
							else
								EquipmentList[7] = 0;
							slotToUse = 7;
						}
						//Yeahlight: Item fits in secondary
						else if(slots[14])
						{
							if(strlen(dbitem->idfile) >= 3)
								EquipmentList[8] = (int8) atoi(&dbitem->idfile[2]);
							else
								EquipmentList[8] = 0;
							slotToUse = 8;
						}
						//Yeahlight: Item fits on head
						else if(slots[2])
						{
							EquipmentList[0] = dbitem->common.material;
							EquipmentColorList[0] = dbitem->common.color;
							slotToUse = 0;
						}
						//Yeahlight: Item fits on chest
						else if(slots[17])
						{
							EquipmentList[1] = dbitem->common.material;
							EquipmentColorList[1] = dbitem->common.color;
							slotToUse = 1;
						}
						//Yeahlight: Item fits on arms
						else if(slots[7])
						{
							EquipmentList[2] = dbitem->common.material;
							EquipmentColorList[2] = dbitem->common.color;
							slotToUse = 2;
						}
						//Yeahlight: Item fits on wrist
						else if(slots[10])
						{
							EquipmentList[3] = dbitem->common.material;
							EquipmentColorList[3] = dbitem->common.color;
							slotToUse = 3;
						}
						//Yeahlight: Item fits on hands
						else if(slots[12])
						{
							EquipmentList[4] = dbitem->common.material;
							EquipmentColorList[4] = dbitem->common.color;
							slotToUse = 4;
						}
						//Yeahlight: Item fits on legs
						else if(slots[18])
						{
							EquipmentList[5] = dbitem->common.material;
							EquipmentColorList[5] = dbitem->common.color;
							slotToUse = 5;
						}
						//Yeahlight: Item fits on feet
						else if(slots[19])
						{
							EquipmentList[6] = dbitem->common.material;
							EquipmentColorList[6] = dbitem->common.color;
							slotToUse = 6;
						}
						item->equipSlot = slotToUse;
						(*itemlist).Append(item);
					}
					mysql_free_result(result);
					return;
				}
			}
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in AddLootDropToNPC query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return;
	}

	return;
}
