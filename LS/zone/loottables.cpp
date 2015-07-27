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
#include "../common/debug.h"
#include <stdio.h>
#include <iostream.h>
#include <stdlib.h>
#include "npc.h"
#include "../common/database.h"
#ifdef WIN32
#define snprintf	_snprintf
#endif
class NPC;
extern Database database;
#ifdef SHAREMEM
	#include "../common/EMuShareMem.h"
	extern LoadEMuShareMemDLL EMuShareMemDLL;
	extern "C" bool extDBLoadLoot() { return database.DBLoadLoot(); }
#endif

//void Database::AddLootTableToNPC(int32 loottable_id, ItemList* itemlist, int32* copper, int32* silver, int32* gold, int32* plat) {
//}

bool Database::LoadLoot() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
#ifdef SHAREMEM
	int32 tmpLootTableCount = 0;
	int32 tmpLootTableEntriesCount = 0;
	int32 tmpLootDropCount = 0;
	int32 tmpLootDropEntriesCount = 0;
#endif
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT max(id), count(*) FROM loottable"), errbuf, &result)) {
		safe_delete(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (row[0])
				loottable_max = atoi(row[0]);
			else
				loottable_max = 0;
#ifdef SHAREMEM
			tmpLootTableCount = atoi(row[1]);
#endif
		}
		else {
			mysql_free_result(result);
			return false;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadLoot query, loottable part: '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return false;
	}
#ifdef SHAREMEM
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT count(*) FROM loottable_entries"), errbuf, &result)) {
		safe_delete(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			tmpLootTableEntriesCount = atoi(row[0]);
		}
		else {
			mysql_free_result(result);
			return false;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadLoot query, loottable2 part: '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return false;
	}

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT max(id), count(*) FROM lootdrop"), errbuf, &result)) {
		safe_delete(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (row[0])
				lootdrop_max = atoi(row[0]);
			else
				lootdrop_max = 0;
			tmpLootDropCount = atoi(row[1]);
		}
		else {
			mysql_free_result(result);
			return false;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadLoot query, lootdrop1 part: '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return false;
	}
#endif	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT max(lootdrop_id), count(*) FROM lootdrop_entries"), errbuf, &result)) {
		safe_delete(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
#ifndef SHAREMEM
			if (row[0])
				lootdrop_max = atoi(row[0]);
			else
				lootdrop_max = 0;
#endif
#ifdef SHAREMEM
			tmpLootDropEntriesCount = atoi(row[1]);
#endif
		}
		else {
			mysql_free_result(result);
			return false;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadLoot query, lootdrop part: '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return false;
	}
#ifdef SHAREMEM
	return EMuShareMemDLL.Loot.DLLLoadLoot(&extDBLoadLoot,
			 sizeof(LootTable_Struct), tmpLootTableCount, loottable_max,
			 sizeof(LootTableEntries_Struct), tmpLootTableEntriesCount,
			 sizeof(LootDrop_Struct), tmpLootDropCount, lootdrop_max,
			 sizeof(LootDropEntries_Struct), tmpLootDropEntriesCount);
#else
	return true;
#endif
}

#ifdef SHAREMEM
bool Database::DBLoadLoot() {
cout << "Loading Loot tables from database..." << endl;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
    MYSQL_RES *result2;
	int32 i, tmpid = 0, tmpmincash = 0, tmpmaxcash = 0, tmpavgcoin = 0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, mincash, maxcash, avgcoin FROM loottable"), errbuf, &result)) {
		safe_delete(query);
		LootTable_Struct* tmpLT = 0;
		while ((row = mysql_fetch_row(result))) {
			tmpid = atoi(row[0]);
			tmpmincash = atoi(row[1]);
			tmpmaxcash = atoi(row[2]);
			tmpavgcoin = atoi(row[3]);
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT loottable_id, lootdrop_id, multiplier, probability FROM loottable_entries WHERE loottable_id=%i", tmpid), errbuf, &result2)) {
				safe_delete(query);
				tmpLT = (LootTable_Struct*) new uchar[sizeof(LootTable_Struct) + (sizeof(LootTableEntries_Struct) * mysql_num_rows(result2))];
				memset(tmpLT, 0, sizeof(LootTable_Struct) + (sizeof(LootTableEntries_Struct) * mysql_num_rows(result2)));
				tmpLT->NumEntries = mysql_num_rows(result2);
				tmpLT->mincash = tmpmincash;
				tmpLT->maxcash = tmpmaxcash;
				tmpLT->avgcoin = tmpavgcoin;
				i=0;
				while ((row = mysql_fetch_row(result2))) {
					if (i >= tmpLT->NumEntries) {
						mysql_free_result(result);
						mysql_free_result(result2);
						cerr << "Error in Database::DBLoadLoot, i >= NumEntries" << endl;
						return false;
					}
					tmpLT->Entries[i].lootdrop_id = atoi(row[1]);
					tmpLT->Entries[i].multiplier = atoi(row[2]);
					tmpLT->Entries[i].probability = atoi(row[3]);
					i++;
				}
				if (!EMuShareMemDLL.Loot.cbAddLootTable(tmpid, tmpLT)) {
					mysql_free_result(result);
					mysql_free_result(result2);
					delete tmpLT;
					cout << "Error in Database::DBLoadLoot: !cbAddLootTable(" << tmpid << ")" << endl;
					return false;
				}
				delete tmpLT;
				mysql_free_result(result2);
			}
			else {
				mysql_free_result(result);
				cerr << "Error in LoadLoot (memshare) #1 query '" << query << "' " << errbuf << endl;
				safe_delete(query);
				return false;
			}
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadLoot (memshare) #2 query '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return false;
	}

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM lootdrop", tmpid), errbuf, &result)) {
		safe_delete(query);
		LootDrop_Struct* tmpLD = 0;
		while ((row = mysql_fetch_row(result))) {
			tmpid = atoi(row[0]);
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT lootdrop_id, item_id, item_charges, equip_item, chance FROM lootdrop_entries WHERE lootdrop_id=%i order by chance desc", tmpid), errbuf, &result2)) {
				safe_delete(query);
				tmpLD = (LootDrop_Struct*) new uchar[sizeof(LootDrop_Struct) + (sizeof(LootDropEntries_Struct) * mysql_num_rows(result2))];
				memset(tmpLD, 0, sizeof(LootDrop_Struct) + (sizeof(LootDropEntries_Struct) * mysql_num_rows(result2)));
				tmpLD->NumEntries = mysql_num_rows(result2);
				i=0;
				while ((row = mysql_fetch_row(result2))) {
					if (i >= tmpLD->NumEntries) {
						mysql_free_result(result);
						mysql_free_result(result2);
						cerr << "Error in Database::DBLoadLoot, i >= NumEntries" << endl;
						return false;
					}
					tmpLD->Entries[i].item_id = atoi(row[1]);
					tmpLD->Entries[i].item_charges = atoi(row[2]);
					tmpLD->Entries[i].equip_item = atoi(row[3]);
					tmpLD->Entries[i].chance = atoi(row[4]);
					i++;
				}
				if (!EMuShareMemDLL.Loot.cbAddLootDrop(tmpid, tmpLD)) {
					mysql_free_result(result);
					mysql_free_result(result2);
					delete tmpLD;
					cout << "Error in Database::DBLoadLoot: !cbAddLootDrop(" << tmpid << ")" << endl;
					return false;
				}
				delete tmpLD;
				mysql_free_result(result2);
			}
			else {
				cerr << "Error in LoadLoot (memshare) #3 query '" << query << "' " << errbuf << endl;
				safe_delete(query);
				return false;
			}
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadLoot (memshare) #4 query '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return false;
	}

	return true;
}
#endif

const LootTable_Struct* Database::GetLootTable(int32 loottable_id) {
#ifdef SHAREMEM
	return EMuShareMemDLL.Loot.GetLootTable(loottable_id);
#else
	int32 i;
	if (loottable_array == 0) {
		loottable_array = new LootTable_Struct*[loottable_max+1];
		loottable_inmem = new sint8[loottable_max+1];
		for (i=0; i<=loottable_max; i++) {
			loottable_array[i] = 0;
			loottable_inmem[i] = 0;
		}
	}
	if (loottable_id > loottable_max || loottable_id == 0)
		return 0;
	if (loottable_inmem[loottable_id] == -1)
		return 0;
	else if (loottable_inmem[loottable_id] == 1) {
		if (loottable_array[loottable_id])
			return loottable_array[loottable_id];
		else
			return 0;
	}
	loottable_inmem[loottable_id] = -1;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int32 tmpmincash = 0, tmpmaxcash = 0, tmpavgcoin = 0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, mincash, maxcash, avgcoin FROM loottable WHERE id=%i", loottable_id), errbuf, &result)) {
		safe_delete(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			tmpmincash = atoi(row[1]);
			tmpmaxcash = atoi(row[2]);
			tmpavgcoin = atoi(row[3]);
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in AddLootTableToNPC get coin query '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return 0;
	}

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT loottable_id, lootdrop_id, multiplier, probability FROM loottable_entries WHERE loottable_id=%i", loottable_id), errbuf, &result)) {
		safe_delete(query);
		loottable_array[loottable_id] = (LootTable_Struct*) new uchar[sizeof(LootTable_Struct) + (sizeof(LootTableEntries_Struct) * mysql_num_rows(result))];
		loottable_inmem[loottable_id] = 1;
		memset(loottable_array[loottable_id], 0, sizeof(LootTable_Struct) + (sizeof(LootTableEntries_Struct) * mysql_num_rows(result)));
		loottable_array[loottable_id]->NumEntries = mysql_num_rows(result);
		loottable_array[loottable_id]->mincash = tmpmincash;
		loottable_array[loottable_id]->maxcash = tmpmaxcash;
		loottable_array[loottable_id]->avgcoin = tmpavgcoin;
		i=0;
		while ((row = mysql_fetch_row(result))) {
			if (i >= loottable_array[loottable_id]->NumEntries) {
				mysql_free_result(result);
				cerr << "Error in Database::GetLootTable, i >= NumEntries" << endl;
				return 0;
			}
			loottable_array[loottable_id]->Entries[i].lootdrop_id = atoi(row[1]);
			loottable_array[loottable_id]->Entries[i].multiplier = atoi(row[2]);
			loottable_array[loottable_id]->Entries[i].probability = atoi(row[3]);
			i++;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in AddLootTableToNPC get items query '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return 0;
	}
	return loottable_array[loottable_id];
#endif
}

const LootDrop_Struct* Database::GetLootDrop(int32 lootdrop_id) {
#ifdef SHAREMEM
	return EMuShareMemDLL.Loot.GetLootDrop(lootdrop_id);
#else
	int32 i;
	if (lootdrop_array == 0) {
		lootdrop_array = new LootDrop_Struct*[lootdrop_max+1];
		lootdrop_inmem = new bool[lootdrop_max+1];
		for (i=0; i<=lootdrop_max; i++) {
			lootdrop_array[i] = 0;
			lootdrop_inmem[i] = false;
		}
	}
	if (lootdrop_id > lootdrop_max || lootdrop_id == 0)
		return 0;
	if (lootdrop_inmem[lootdrop_id]) {
		if (lootdrop_array[lootdrop_id])
			return lootdrop_array[lootdrop_id];
		else
			return 0;
	}
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT lootdrop_id, item_id, item_charges, equip_item, chance FROM lootdrop_entries WHERE lootdrop_id=%i order by chance desc", lootdrop_id), errbuf, &result)) {
		safe_delete(query);
		lootdrop_array[lootdrop_id] = (LootDrop_Struct*) new uchar[sizeof(LootDrop_Struct) + (sizeof(LootDropEntries_Struct) * mysql_num_rows(result))];
		memset(lootdrop_array[lootdrop_id], 0, sizeof(LootDrop_Struct) + (sizeof(LootDropEntries_Struct) * mysql_num_rows(result)));
		lootdrop_inmem[lootdrop_id] = true;
		lootdrop_array[lootdrop_id]->NumEntries = mysql_num_rows(result);
		i=0;
		while ((row = mysql_fetch_row(result))) {
			if (i >= lootdrop_array[lootdrop_id]->NumEntries) {
				mysql_free_result(result);
				cerr << "Error in Database::GetLootDrop, i >= NumEntries" << endl;
				return 0;
			}
			lootdrop_array[lootdrop_id]->Entries[i].item_id = atoi(row[1]);
			lootdrop_array[lootdrop_id]->Entries[i].item_charges = atoi(row[2]);
			lootdrop_array[lootdrop_id]->Entries[i].equip_item = atoi(row[3]);
			lootdrop_array[lootdrop_id]->Entries[i].chance = atoi(row[4]);
			i++;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in AddLootTableToNPC get items query '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return 0;
	}
	return lootdrop_array[lootdrop_id];
#endif
}

// Queries the loottable: adds item & coin to the npc
void Database::AddLootTableToNPC(NPC* npc,int32 loottable_id, ItemList* itemlist, int32* copper, int32* silver, int32* gold, int32* plat) {
//if (loottable_id == 178190)
//DebugBreak();
	const LootTable_Struct* lts = 0;
	*copper = 0;
	*silver = 0;
	*gold = 0;
	*plat = 0;

	lts = database.GetLootTable(loottable_id);
	if (!lts)
		return;

	// do coin
	if (lts->mincash > lts->maxcash) {
		cerr << "Error in loottable #" << loottable_id << ": mincash > maxcash" << endl;
	}
	else if (lts->maxcash != 0) {
		int32 cash = 0;
		if (lts->mincash == lts->maxcash)
			cash = lts->mincash;
		else
			cash = (rand() % (lts->maxcash - lts->mincash)) + lts->mincash;
		if (cash != 0) {
			if (lts->avgcoin != 0) {
				int32 mincoin = (int32) (lts->avgcoin * 0.75 + 1);
				int32 maxcoin = (int32) (lts->avgcoin * 1.25 + 1);
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

	// Do items
	for (int32 i=0; i<lts->NumEntries; i++) {
		for (int32 k = 1; k <= lts->Entries[i].multiplier; k++) {
			if ( (rand()%100) < lts->Entries[i].probability) {
				AddLootDropToNPC(npc,lts->Entries[i].lootdrop_id, itemlist);
			}
		}
	}
}

// Called by AddLootTableToNPC
// maxdrops = size of the array npcd
void Database::AddLootDropToNPC(NPC* npc,int32 lootdrop_id, ItemList* itemlist) {
	const LootDrop_Struct* lds = GetLootDrop(lootdrop_id);
	if (!lds)
		return;

// This is Wiz's updated Pool Looting functionality.  Eventually, the database format should be moved over to use this
// or implemented to support both methods.  (A unique identifier in lootable_entries indicates to roll for a pool item
// in another table.
#ifdef POOLLOOTING
	printf("POOL!\n");
	int32 chancepool = 0;
	int32 items[50];
	int32 itemchance[50];
	int16 itemcharges[50];
	int8 i = 0;

	for (int m=0;m < 50;m++) {
		items[m]=0;
		itemchance[m]=0;
		itemcharges[m]=0;
	}

	for (int k=0; k<lds->NumEntries; k++) {
		items[i] = lds->Entries[k].item_id;
		itemchance[i] = lds->Entries[k].chance + chancepool;
		itemcharges[i] = lds->Entries[k].item_charges;
		chancepool += lds->Entries[k].chance;
		i++;
	}
	int32 res;
	i = 0;

    if (chancepool!=0) { //avoid divide by zero if some mobs have 0 for chancepool
        res = rand()%chancepool;
    }
    else {
        res = 0;
    }

	while (items[i] != 0) {
		if (res <= itemchance[i])
			break;
		else
			i++;
	}
	const Item_Struct* dbitem = database.GetItem(items[i]);
	if (dbitem == 0) {
		cerr << "Error in AddLootDropToNPC: dbitem=0, item#=" << items[i] << ", lootdrop_id=" << lootdrop_id << endl;
	}
	else {
		cout << "Adding item to Mob" << endl;
		ServerLootItem_Struct* item = new ServerLootItem_Struct;
		item->item_nr = dbitem->item_nr;
		item->charges = itemcharges[i];
		item->equipSlot = 0;
		(*itemlist).Append(item);
	}
#else
	int x=0;
	int32 k;
	int32 totalchance = 0;
	for (k=0; k<lds->NumEntries; k++) {
		totalchance += lds->Entries[k].chance;
	}
	int32 thischance = 0;
	for (k=0; k<lds->NumEntries; k++) {
		x++;
		LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
		iterator.Reset();
		int itemon=0;
		while(iterator.MoreElements()){
		const Item_Struct* item = database.GetItem(iterator.GetData()->item_nr);
		if(item)
			if(iterator.GetData()->item_nr==lds->Entries[k].item_id)
				itemon=1;
		iterator.Advance();
		}
		thischance += lds->Entries[k].chance;
		if (totalchance == 0 || (lds->Entries[k].chance != 0 && rand()%totalchance < thischance && (lds->Entries[k].chance!=100 && itemon==0)) || (lds->Entries[k].chance==100 && itemon==0)) {
			int32 itemid = lds->Entries[k].item_id;
			const Item_Struct* dbitem = database.GetItem(itemid);
			if (dbitem == 0) {
				cerr << "Error in AddLootDropToNPC: dbitem=0, item#=" << itemid << ", lootdrop_id=" << lootdrop_id << endl;
			}
			else {				
				ServerLootItem_Struct* item = new ServerLootItem_Struct;
				item->item_nr = dbitem->item_nr;
				item->charges = lds->Entries[k].item_charges;
				if (lds->Entries[k].equip_item==1){
					const Item_Struct* item2= database.GetItem(item->item_nr);
					char tmp[20];
					char newid[20];
					memset(newid, 0, sizeof(newid));
					for(int i=0;i<7;i++){
						if (!isalpha(item2->idfile[i])){
							strncpy(newid, &item2->idfile[i],5);
							i=8;
						}
					}
					//printf("Npc Name: %s, Item: %i\n",npc->GetName(),item2->item_nr);
					if (((item2->equipableSlots==24576) || (item2->equipableSlots==8192)) && (npc->d_meele_texture1==0)) {
						npc->d_meele_texture1=atoi(newid);
						npc->equipment[7]=item2->item_nr;
						if (item2->common.spellId0!=0)
							npc->CastToMob()->AddProcToWeapon(item2->common.spellId0,true);
						npc->AC+=item2->common.AC;
						npc->STR+=item2->common.STR;
						npc->INT+=item2->common.INT;
					}
					else if (((item2->equipableSlots==24576) || (item2->equipableSlots==16384)) && (npc->d_meele_texture2 ==0) && ((npc->GetLevel()>=13) || (item2->common.damage==0)))
					{
						if (item2->common.spellId0!=0)
							npc->CastToMob()->AddProcToWeapon(item2->common.spellId0,true);
						npc->equipment[8]=item2->item_nr;
						npc->d_meele_texture2=atoi(newid);
						npc->AC+=item2->common.AC;
						npc->STR+=item2->common.STR;
						npc->INT+=item2->common.INT;
					}
					else if ((item2->equipableSlots==4) && (npc->equipment[0]==0)){
						npc->equipment[0]=item2->item_nr;
						npc->AC+=item2->common.AC;
						npc->STR+=item2->common.STR;
						npc->INT+=item2->common.INT;
					}
					else if ((item2->equipableSlots==131072) && (npc->equipment[1]==0)){
						npc->equipment[1]=item2->common.material;
						npc->texture=item2->common.material;
						npc->AC+=item2->common.AC;
						npc->STR+=item2->common.STR;
						npc->INT+=item2->common.INT;
					}
					else if ((item2->equipableSlots==128) && (npc->equipment[2]==0)){
						npc->equipment[2]=item2->common.material;
						npc->AC+=item2->common.AC;
						npc->STR+=item2->common.STR;
						npc->INT+=item2->common.INT;
					}
					else if ((item2->equipableSlots==1536) && (npc->equipment[3]==0)){
						npc->equipment[3]=item2->common.material;
						npc->AC+=item2->common.AC;
						npc->STR+=item2->common.STR;
						npc->INT+=item2->common.INT;
					}
					else if ((item2->equipableSlots==4096) && (npc->equipment[4]==0)){
						npc->equipment[4]=item2->common.material;
						npc->AC+=item2->common.AC;
						npc->STR+=item2->common.STR;
						npc->INT+=item2->common.INT;
					}
					else if ((item2->equipableSlots==262144) && (npc->equipment[5]==0)){
						npc->equipment[5]=item2->common.material;
						npc->AC+=item2->common.AC;
						npc->STR+=item2->common.STR;
						npc->INT+=item2->common.INT;
					}
					else if ((item2->equipableSlots==524288) && (npc->equipment[6]==0)){
						npc->equipment[6]=item2->common.material;
						npc->AC+=item2->common.AC;
						npc->STR+=item2->common.STR;
						npc->INT+=item2->common.INT;
					}
					item->equipSlot = dbitem->equipableSlots;
				}
				(*itemlist).Append(item);
			}
			break;
		}
	}
#endif
}


#if 0
// Queries the loottable: adds item & coin to the npc
void Database::AddLootTableToNPC(int32 loottable_id, ItemList* itemlist, int32* copper, int32* silver, int32* gold, int32* plat) {
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
		while ((row = mysql_fetch_row(result))) {
			int multiplier = atoi(row[2]);
			for (int i = 1; i <= multiplier; i++) {
				if ( ((rand()%1)*100) < atoi(row[3])) {
					AddLootDropToNPC(atoi(row[1]), itemlist);
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
void Database::AddLootDropToNPC(int32 lootdrop_id, ItemList* itemlist) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

// This is Wiz's updated Pool Looting functionality.  Eventually, the database format should be moved over to use this
// or implemented to support both methods.  (A unique identifier in lootable_entries indicates to roll for a pool item
// in another table.
#ifdef POOLLOOTING
	int32 chancepool = 0;
	int32 items[50];
	int32 itemchance[50];
	int16 itemcharges[50];
	int8 i = 0;

	for (int m=0;m < 50;m++)
	{
		items[m]=0;
		itemchance[m]=0;
		itemcharges[m]=0;
	}
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT lootdrop_id, item_id, item_charges, equip_item, chance FROM lootdrop_entries WHERE lootdrop_id=%i order by chance desc", lootdrop_id), errbuf, &result))
	{
		delete[] query;
		while (row = mysql_fetch_row(result))
		{
			items[i] = atoi(row[1]);
			itemchance[i] = atoi(row[4]) + chancepool;
			itemcharges[i] = atoi(row[2]);
			chancepool += atoi(row[4]);
			i++;
		}
		int32 res;
		i = 0;

        if (chancepool!=0) //avoid divide by zero if some mobs have 0 for chancepool
        {
            res = rand()%chancepool;
        }
        else
        {
            res = 0;
        }

		while (items[i] != 0)
		{
			if (res <= itemchance[i])
				break;
			else
				i++;
		}
		const Item_Struct* dbitem = database.GetItem(items[i]);
		if (dbitem == 0)
		{
			cerr << "Error in AddLootDropToNPC: dbitem=0, item#=" << items[i] << ", lootdrop_id=" << lootdrop_id << endl;
		}
		else
		{
			printf("Adding item2: %i",item->item_nr);
			cout << "Adding item to Mob" << endl;
			ServerLootItem_Struct* item = new ServerLootItem_Struct;
			item->item_nr = dbitem->item_nr;
			item->charges = itemcharges[i];
			item->equipSlot = 0;
			(*itemlist).Append(item);
		}
		mysql_free_result(result);
	}
#else
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT lootdrop_id, item_id, item_charges, equip_item, chance FROM lootdrop_entries WHERE lootdrop_id=%i order by chance desc", lootdrop_id), errbuf, &result))
	{
		delete[] query;
		while ((row = mysql_fetch_row(result)))
		{
			int8 LootDropMod=1;  // place holder till I put it in a database variable to make it configurable.
			if( (rand()%100) < ((atoi(row[4]) * LootDropMod)) )
			{
				int32 itemid = atoi(row[1]);
				const Item_Struct* dbitem = database.GetItem(itemid);
				if (dbitem == 0)
				{
					cerr << "Error in AddLootDropToNPC: dbitem=0, item#=" << itemid << ", lootdrop_id=" << lootdrop_id << endl;
				}
				else
				{
					printf("Adding item: %i",item->item_nr);
					ServerLootItem_Struct* item = new ServerLootItem_Struct;
					item->item_nr = dbitem->item_nr;
					item->charges = atoi(row[2]);
					item->equipSlot = 0;
					(*itemlist).Append(item);
				}
				
				//mysql_free_result(result);
				//return;
			}
		}
		mysql_free_result(result);
	}
#endif
	else
	{
		cerr << "Error in AddLootDropToNPC query '" << query << "' " << errbuf << endl;
		delete[] query;
		return;
	}
	
	return;
}
#endif
