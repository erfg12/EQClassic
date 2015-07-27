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
/*
New class for handeling corpses and everything associated with them.
Child of the Mob class.
-Quagmire
*/
#include "../common/debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream.h>
#ifdef WIN32
#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#endif

#include "PlayerCorpse.h"
#include "../common/packet_functions.h"
#include "../common/crc32.h"

extern Database database;
extern EntityList entity_list;
extern Zone* zone;
extern npcDecayTimes_Struct npcCorpseDecayTimes[100];

void Corpse::SendEndLootErrorPacket(Client* client) {
	APPLAYER* outapp = new APPLAYER(OP_LootComplete, 0);
	client->QueuePacket(outapp);
	delete outapp;
}

void Corpse::SendLootReqErrorPacket(Client* client, int8 response) {
	APPLAYER* outapp = new APPLAYER(OP_MoneyOnCorpse, sizeof(moneyOnCorpseStruct));
	moneyOnCorpseStruct* d = (moneyOnCorpseStruct*) outapp->pBuffer;
	d->response		= response;
	d->unknown1		= 0x5a;
	d->unknown2		= 0x40;
	client->QueuePacket(outapp);
	delete outapp;
}

Corpse* Corpse::LoadFromDBData(int32 in_dbid, int32 in_charid, char* in_charname, uchar* in_data, int32 in_datasize, float in_x, float in_y, float in_z, float in_heading, char* timeofdeath) {
	if (in_datasize < sizeof(DBPlayerCorpse_Struct)) {
		cout << "Corpse::LoadFromDBData: Corrupt data: in_datasize < sizeof(DBPlayerCorpse_Struct)" << endl;
		return 0;
	}
	DBPlayerCorpse_Struct* dbpc = (DBPlayerCorpse_Struct*) in_data;
	if (in_datasize != (sizeof(DBPlayerCorpse_Struct) + (dbpc->itemcount * sizeof(ServerLootItem_Struct)))) {
		cout << "Corpse::LoadFromDBData: Corrupt data: in_datasize != expected size" << endl;
		return 0;
	}
	if (dbpc->crc != CRC32::Generate(&((uchar*) dbpc)[4], in_datasize - 4)) {
		cout << "Corpse::LoadFromDBData: Corrupt data: crc failure" << endl;
		return 0;
	}
	ItemList* itemlist = new ItemList();
	ServerLootItem_Struct* tmp = 0;
	for (unsigned int i=0; i < dbpc->itemcount; i++) {
		tmp = new ServerLootItem_Struct;
		memcpy(tmp, &dbpc->items[i], sizeof(ServerLootItem_Struct));
		itemlist->Append(tmp);
	}
	Corpse* pc = new Corpse(in_dbid, in_charid, in_charname, itemlist, dbpc->copper, dbpc->silver, dbpc->gold, dbpc->plat, in_x, in_y, in_z, in_heading, dbpc->size, dbpc->gender, dbpc->race, dbpc->class_, dbpc->deity, dbpc->level, dbpc->texture, dbpc->helmtexture,dbpc->exp);
	if (dbpc->locked)
		pc->Lock();
	if (pc->IsEmpty()) {
		delete pc;
		return 0;
	}
	else {
		return pc;
	}
}

// To be used on NPC death and ZoneStateLoad
Corpse::Corpse(NPC* in_npc, ItemList** in_itemlist, int32 in_npctypeid, NPCType** in_npctypedata, int32 in_decaytime)
 : Mob("Unnamed_Corpse","",0,0,in_npc->GetGender(),in_npc->GetRace(),in_npc->GetClass(),0//bodytype added
       ,in_npc->GetDeity(),in_npc->GetLevel(),in_npc->GetNPCTypeID(),0,in_npc->GetSize(),0,0,in_npc->GetHeading(),in_npc->GetX(),in_npc->GetY(),in_npc->GetZ(),0,0,in_npc->GetTexture(),in_npc->GetHelmTexture(),0,0,0,0,0,0,0,0,0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,1,0,0)
{
	pIsChanged = false;
	p_PlayerCorpse = false;
	pLocked = false;
	BeingLootedBy = 0xFFFFFFFF;
	if (in_itemlist) {
		itemlist = *in_itemlist;
		*in_itemlist = 0;
	}
	else {
		itemlist = new ItemList();
	}
	AddCash(in_npc->GetCopper(), in_npc->GetSilver(), in_npc->GetGold(), in_npc->GetPlatinum());
	
	NPCTypedata = 0;
	npctype_id = in_npctypeid;
	if (in_npctypedata) {
		NPCTypedata = *in_npctypedata;
		*in_npctypedata = 0;
	}
	
	charid = 0;
	dbid = 0;
	p_depop = false;
	strcpy(orgname, in_npc->GetName());
	strcpy(name, in_npc->GetName());
	corpse_decay_timer = new Timer(in_decaytime);
	corpse_delay_timer = new Timer(in_decaytime/2);
	// Added By Hogie 
	for(int count = 0; count < 100; count++) {
		if ((level >= npcCorpseDecayTimes[count].minlvl) && (level <= npcCorpseDecayTimes[count].maxlvl)) {
			corpse_decay_timer->SetTimer(npcCorpseDecayTimes[count].seconds*1000);
			corpse_delay_timer->SetTimer(npcCorpseDecayTimes[count].seconds*100);
			break;
		}
	}
	// Added By Hogie -- End
	for (int i=0; i<MAX_LOOTERS; i++)
		memset(looters[i], 0, sizeof(looters[i]));
	this->rezzexp = 0;
	corpse_decay_timer->Start();
	corpse_delay_timer->Start();
}

// To be used on PC death
Corpse::Corpse(Client* client, PlayerProfile_Struct* pp, sint32 in_rezexp, int8 iCorpseLevel)
 : Mob("Unnamed_Corpse","",0,0,client->GetGender(),client->GetRace(),client->GetClass(), 0, // bodytype added
        client->GetDeity(),client->GetLevel(),0,0, client->GetSize(), 0, 0,client->GetHeading(),client->GetX(),client->GetY(),client->GetZ(),0,0,client->GetTexture(),client->GetHelmTexture(),0,0,0,0,0,0,0,0,0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,1,0,0)
{
	pIsChanged = true;
	NPCTypedata = 0;
	rezzexp = in_rezexp;
	p_PlayerCorpse = true;
	pLocked = false;
	BeingLootedBy = 0xFFFFFFFF;
	itemlist = new ItemList();
	charid = client->CharacterID();
	dbid = 0;
	p_depop = false;
	strcpy(orgname, pp->name);
	strcpy(name, pp->name);
	if (iCorpseLevel >= 2) {
		AddCash(pp->copper, pp->silver, pp->gold, pp->platinum);
		pp->copper = 0;
		pp->silver = 0;
		pp->gold = 0;
		pp->platinum = 0;
	}
	int i;
	if (iCorpseLevel >= 3) {
		client->RepairInventory();
		for (i=0; i<30; i++) {
			AddItem(pp->inventory[i], pp->invitemproperties[i].charges, i);
			pp->inventory[i] = 0xFFFF;
			pp->invitemproperties[i].charges = 0;
		}
		for (i=0; i<pp_containerinv_size; i++) {
			AddItem(pp->containerinv[i], pp->bagitemproperties[i].charges, i+250);
			pp->containerinv[i] = 0xFFFF;
			pp->bagitemproperties[i].charges = 0;
		}
	}
	for (i=0; i<MAX_LOOTERS; i++)
		memset(looters[i], 0, sizeof(looters[i]));
	Save();
	client->Save();
	corpse_decay_timer = 0;
	corpse_delay_timer = 0;
	
}

// To be called from LoadFromDBData
Corpse::Corpse(int32 in_dbid, int32 in_charid, char* in_charname, ItemList* in_itemlist, int32 in_copper, int32 in_silver, int32 in_gold, int32 in_plat, float in_x, float in_y, float in_z, float in_heading, float in_size, int8 in_gender, int16 in_race, int8 in_class, int8 in_deity, int8 in_level, int8 in_texture, int8 in_helmtexture,int32 in_rezexp)
 : Mob("Unnamed_Corpse","",0,0,in_gender, in_race, in_class, 0, in_deity, in_level,0,0, in_size, 0, 0, in_heading, in_x, in_y, in_z,0,0,in_texture,in_helmtexture,0,0,0,0,0,0,0,0,0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,1,0,0)
{
	pIsChanged = false;
	NPCTypedata = 0;
	p_PlayerCorpse = true;
	pLocked = false;
	BeingLootedBy = 0xFFFFFFFF;
	dbid = in_dbid;
	p_depop = false;
	charid = in_charid;
	itemlist = in_itemlist;
	
	strcpy(orgname, in_charname);
	strcpy(name, in_charname);
	this->copper = in_copper;
	this->silver = in_silver;
	this->gold = in_gold;
	this->platinum = in_plat;
	corpse_decay_timer = 0;
	corpse_delay_timer = 0;
	rezzexp = in_rezexp;
	for (int i=0; i<MAX_LOOTERS; i++)
		memset(looters[i], 0, sizeof(looters[i]));
}

Corpse::~Corpse() {
	if (p_PlayerCorpse && itemlist) {
		if (IsEmpty() && dbid != 0)
			database.DeletePlayerCorpse(dbid);
		else if (!IsEmpty() && !(p_depop && dbid == 0))
			Save();
	}
	safe_delete(itemlist);
	safe_delete(corpse_decay_timer);
	safe_delete(corpse_delay_timer);
	safe_delete(NPCTypedata);
}

/*
this needs to be called AFTER the entity_id is set
the client does this too, so it's unchangable
*/
void Corpse::CalcCorpseName() {
	EntityList::RemoveNumbers(name);
	char tmp[64];
	snprintf(tmp, sizeof(tmp), "'s corpse%d", GetID());
	name[(sizeof(name) - 1) - strlen(tmp)] = 0;
	strcat(name, tmp);
}

bool Corpse::Save() {
	if (IsEmpty()) {
		Delete();
		return true;
	}	
	if (!p_PlayerCorpse)
		return true;
	if (!pIsChanged)
		return true;
	
	int32 tmp = this->CountItems();
	int32 tmpsize = sizeof(DBPlayerCorpse_Struct) + (tmp * sizeof(ServerLootItem_Struct));
	DBPlayerCorpse_Struct* dbpc = (DBPlayerCorpse_Struct*) new uchar[tmpsize];
	memset(dbpc, 0, tmpsize);
	dbpc->itemcount = tmp;
	dbpc->size = this->size;
	dbpc->locked = pLocked;
	dbpc->copper = this->copper;
	dbpc->silver = this->silver;
	dbpc->gold = this->gold;
	dbpc->plat = this->platinum;
	dbpc->race = race;
	dbpc->class_ = class_;
	dbpc->gender = gender;
	dbpc->deity = deity;
	dbpc->level = level;
	dbpc->texture = this->texture;
	dbpc->helmtexture = this->helmtexture;
	dbpc->exp = rezzexp;
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	iterator.Reset();
	int32 x = 0;
	while(iterator.MoreElements()) {
		memcpy((char*) &dbpc->items[x++], (char*) iterator.GetData(), sizeof(ServerLootItem_Struct));
		iterator.Advance();
	}

	dbpc->crc = CRC32::Generate(&((uchar*) dbpc)[4], tmpsize - 4);

	if (dbid == 0)
		dbid = database.CreatePlayerCorpse(charid, orgname, zone->GetZoneID(), (uchar*) dbpc, tmpsize, x_pos, y_pos, z_pos, heading);
	else
		dbid = database.UpdatePlayerCorpse(dbid, charid, orgname, zone->GetZoneID(), (uchar*) dbpc, tmpsize, x_pos, y_pos, z_pos, heading);
	delete dbpc;
	if (dbid == 0) {
		cout << "Error: Failed to save player corpse '" << this->GetName() << "'" << endl;
		return false;
	}
	return true;
}

void Corpse::Delete() {
	if (IsPlayerCorpse() && dbid != 0)
		database.DeletePlayerCorpse(dbid);
	dbid = 0;
	p_depop = true;
}

void Corpse::Depop(bool StartSpawnTimer) {
	if (IsNPCCorpse())
		p_depop = true;
}

int32 Corpse::CountItems() {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	int32 x = 0;
	while(iterator.MoreElements())
	{
		x++;
		iterator.Advance();
	}
	
	return x;
}

void Corpse::AddItem(int16 itemnum, int8 charges, int16 slot) {
	if (!database.GetItem(itemnum))
		return;
	pIsChanged = true;
	ServerLootItem_Struct* item = new ServerLootItem_Struct;
	memset(item, 0, sizeof(ServerLootItem_Struct));
	item->item_nr = itemnum;
	item->charges = charges;
	item->equipSlot = slot;
	(*itemlist).Append(item);
}

ServerLootItem_Struct* Corpse::GetItem(int16 lootslot, ServerLootItem_Struct** bag_item_data) {
	ServerLootItem_Struct* ret = 0;
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->lootslot == lootslot) {
			ret = iterator.GetData();
			break;
		}
		iterator.Advance();
	}
	if (bag_item_data) {
		if (ret == 0)
			return 0;
		int16 bagstart = ((ret->equipSlot-22)*10) + 250;
		iterator.Reset();
		while(iterator.MoreElements()) {
			if (iterator.GetData()->equipSlot >= bagstart && iterator.GetData()->equipSlot < bagstart + 10) {
				bag_item_data[iterator.GetData()->equipSlot - bagstart] = iterator.GetData();
			}
			iterator.Advance();
		}
	}
	
	return ret;
}

int16 Corpse::GetWornItem(int16 equipSlot) {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->equipSlot == equipSlot)
		{
			return iterator.GetData()->item_nr;
		}
		iterator.Advance();
	}
	
	return 0;
}

void Corpse::RemoveItem(int16 lootslot) {
	if (lootslot == 0xFFFF)
		return;
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->lootslot == lootslot) {
			pIsChanged = true;
			ItemRemoved(iterator.GetData());
			iterator.RemoveCurrent();
			return;
		}
		iterator.Advance();
	}
}

void Corpse::RemoveItem(ServerLootItem_Struct* item_data) {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData() == item_data) {
			pIsChanged = true;
			ItemRemoved(iterator.GetData());
			iterator.RemoveCurrent();
			return;
		}
		iterator.Advance();
	}
}

void Corpse::ItemRemoved(ServerLootItem_Struct* item_data) {
	APPLAYER* outapp = new APPLAYER;
	outapp->opcode = OP_WearChange;
	outapp->size = sizeof(WearChange_Struct);
	outapp->pBuffer = new uchar[outapp->size];
	memset(outapp->pBuffer, 0, outapp->size);
	WearChange_Struct* wc = (WearChange_Struct*) outapp->pBuffer;
	wc->spawn_id = this->GetID();
	
	if (item_data->equipSlot == 2)
		wc->wear_slot_id = 0;
	else if (item_data->equipSlot == 17)
		wc->wear_slot_id = 1;
	else if (item_data->equipSlot == 7)
		wc->wear_slot_id = 2;
	else if (item_data->equipSlot == 10)
		wc->wear_slot_id = 3;
	else if (item_data->equipSlot == 12)
		wc->wear_slot_id = 4;
	else if (item_data->equipSlot == 18)
		wc->wear_slot_id = 5;
	else if (item_data->equipSlot == 19)
		wc->wear_slot_id = 6;
	else if (item_data->equipSlot == 13)
		wc->wear_slot_id = 7;
	else if (item_data->equipSlot == 14)
		wc->wear_slot_id = 8;
	else
		safe_delete(outapp);
	if (outapp != 0) {
		entity_list.QueueClients(0, outapp);
		safe_delete(outapp);
	}
}

void Corpse::AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum) {
	this->copper = in_copper;
	this->silver = in_silver;
	this->gold = in_gold;
	this->platinum = in_platinum;
	pIsChanged = true;
}

void Corpse::RemoveCash() {
	this->copper = 0;
	this->silver = 0;
	this->gold = 0;
	this->platinum = 0;
	pIsChanged = true;
}

bool Corpse::IsEmpty() {
	if (copper != 0 || silver != 0 || gold != 0 || platinum != 0)
		return false;
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	iterator.Reset();
	return !iterator.MoreElements();
}

bool Corpse::Process() {
	if (p_depop)
		return false;
	if(corpse_delay_timer) {
		if(corpse_delay_timer->Check())
		{
	for (int i=0; i<MAX_LOOTERS; i++)
		memset(looters[i], 0, sizeof(looters[i]));
		corpse_delay_timer->Disable();
			return true;
		}
	}
	if (corpse_decay_timer) {
		if(corpse_decay_timer->Check()) {
			return false;
		}
	}
	
	return true;
}

void Corpse::SetDecayTimer(int32 decaytime) {
	if (corpse_decay_timer) {
		corpse_decay_timer = new Timer(1);
	}
	if (decaytime == 0)
		corpse_decay_timer->Trigger();
	else
		corpse_decay_timer->Start(decaytime);
}

bool Corpse::CanMobLoot(const char* iName) {
	int8 z=0;
	for(int i=0; i<MAX_LOOTERS; i++) {
		if(looters[i][0] != 0)
			z++;

		if (strcasecmp(looters[i], iName) == 0)
			return true;
	}
	if(z == 0)
		return true;
	else
		return false;
}

void Corpse::AllowMobLoot(const char* iName, int8 slot)
{
	if(slot >= MAX_LOOTERS)
		return;

	strcpy(looters[slot], iName);
}

void Corpse::MakeLootRequestPackets(Client* client, const APPLAYER* app) {
	// Added 12/08.  Started compressing loot struct on live.
	int cpisize = sizeof(ItemOnCorpse_Struct) + (50 * sizeof(MerchantItemD_Struct));
	ItemOnCorpse_Struct* cpi = (ItemOnCorpse_Struct*) new uchar[cpisize];
	memset(cpi, 0, cpisize);

	if (IsPlayerCorpse() && dbid == 0) {
//		SendLootReqErrorPacket(client, 0);
		client->Message(13, "Warning: Corpse's dbid = 0! Corpse will not survive zone shutdown!");
		cout << "Error: PlayerCorpse::MakeLootRequestPackets: dbid = 0!" << endl;
//		return;
	}
	if (pLocked && client->Admin() < 100) {
		SendLootReqErrorPacket(client, 0);
		client->Message(13, "Error: Corpse locked by GM.");
		return;
	}
	if (this->BeingLootedBy != 0xFFFFFFFF) {
		// lets double check....
		Entity* looter = entity_list.GetID(this->BeingLootedBy);
		if (looter == 0)
			this->BeingLootedBy = 0xFFFFFFFF;
	}
	int8 tCanLoot = 2;
	if (this->BeingLootedBy != 0xFFFFFFFF && this->BeingLootedBy != client->GetID()) {
		// ok, now we tell the client to fuck off
		// Quagmire - i think this is the right packet, going by pyro's logs
		SendLootReqErrorPacket(client, 0);
		tCanLoot = 0;
//		cout << "Telling " << client->GetName() << " corpse '" << this->GetName() << "' is busy..." << endl;
	}
	else if (IsPlayerCorpse() && this->charid != client->CharacterID()) {
		// Not their corpse... get lost
		tCanLoot = 1;
		if (client->Admin() < 100) {
			SendLootReqErrorPacket(client, 2);
		}
//		cout << "Telling " << client->GetName() << " corpse '" << this->GetName() << "' is busy..." << endl;
	}
	else if (IsNPCCorpse() && !CanMobLoot(client->GetName())) {
		tCanLoot = 1;
		if (client->Admin() < 100) {
			SendLootReqErrorPacket(client, 2);
		}
	}
	if (tCanLoot == 2 || (tCanLoot == 1 && client->Admin() >= 100)) {
		this->BeingLootedBy = client->GetID();
		APPLAYER* outapp = new APPLAYER(OP_MoneyOnCorpse, sizeof(moneyOnCorpseStruct));
		moneyOnCorpseStruct* d = (moneyOnCorpseStruct*) outapp->pBuffer;
		
		d->response		= 1;
		d->unknown1		= 0x5a;
		d->unknown2		= 0x40;
		if (tCanLoot == 2) { // dont take the coin off if it's a gm peeking at the corpse
			if (zone->lootvar!=0){
				int admin=client->Admin();
				if (zone->lootvar==7){
						client->LogLoot(client,this,0);
				}
				else if ((admin>=10) && (admin<20)){
					if ((zone->lootvar<8) && (zone->lootvar>5))
						client->LogLoot(client,this,0);
				}
				else if (admin<=20){
					if ((zone->lootvar<8) && (zone->lootvar>4))
						client->LogLoot(client,this,0);
				}
				else if (admin<=80){
					if ((zone->lootvar<8) && (zone->lootvar>3))
						client->LogLoot(client,this,0);
				}
				else if (admin<=100){
					if ((zone->lootvar<9) && (zone->lootvar>2))
						client->LogLoot(client,this,0);
				}
				else if (admin<=150){
					if (((zone->lootvar<8) && (zone->lootvar>1)) || (zone->lootvar==9))
						client->LogLoot(client,this,0);
				}
				else if (admin<=255){
					if ((zone->lootvar<8) && (zone->lootvar>0))
						client->LogLoot(client,this,0);	
				}
			}
			#ifdef GUILDWARS
				if (this->GetPlatinum()>10000)
					this->RemoveCash();
			#endif
			d->copper		= this->GetCopper();
			d->silver		= this->GetSilver();
			d->gold			= this->GetGold();
			d->platinum		= this->GetPlatinum();

			client->AddMoneyToPP(this->GetCopper(),this->GetSilver(),this->GetGold(),this->GetPlatinum(),false);
			this->RemoveCash();
		}
		client->QueuePacket(outapp); 
		delete outapp;
		
		LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
		iterator.Reset();
		int i;
		const Item_Struct* item = 0;
		bool IsBagThere[8];
		for (i=0; i<8; i++) {
			item = database.GetItem(this->GetWornItem(22+i));
			if (item && item->type == 0x01)
				IsBagThere[i] = true;
			else
				IsBagThere[i] = false;
		}
		i = 0;
		while(iterator.MoreElements()) {
			ServerLootItem_Struct* item_data = iterator.GetData();
			item_data->lootslot = 0xFFFF;

			// Dont display the item if it's in a bag
			if (!IsPlayerCorpse() || item_data->equipSlot < 250 || item_data->equipSlot >= 330 || !IsBagThere[(item_data->equipSlot - 250) / 10]) {
				if (i >= 30) {
					if (cpi->count == 30)
						Message(13, "Warning: Too many items to display. Loot some then re-loot the corpse to see the rest");
				}
				else {
					item = database.GetItem(item_data->item_nr);
					
					if (item) {
						memcpy(&cpi->item[cpi->count].item, item, sizeof(Item_Struct));
						cpi->item[cpi->count].item.equipSlot = cpi->count;
						item_data->lootslot = cpi->count;
                
						if(item->flag != 0x7669)
							cpi->item[cpi->count].item.common.charges = item_data->charges;
						cpi->count++;
					}
				}
				i++;
			}
			iterator.Advance();
		}
	}	
	APPLAYER* outapp = new APPLAYER(OP_ItemOnCorpse, 10000);
	outapp->size = 2 + DeflatePacket((uchar*) cpi->item, cpi->count * sizeof(MerchantItemD_Struct), &outapp->pBuffer[2], 10000-2);
	ItemOnCorpse_Struct* cpi2 = (ItemOnCorpse_Struct*) outapp->pBuffer;
	cpi2->count = cpi->count;
	safe_delete(cpi);
	client->QueuePacket(outapp);
	//DumpPacket(outapp);
	//printf("Merchant has %i items available.\n",cpi->count);
	delete outapp;
	// Disgrace: Client seems to require that we send the packet back...
	client->QueuePacket(app);
}

void Corpse::LootItem(Client* client, const APPLAYER* app) {
	if (this->BeingLootedBy != client->GetID()) {
		client->Message(13, "Error: Corpse::LootItem: BeingLootedBy != client");
		SendEndLootErrorPacket(client);
		return;
	}
	if (IsPlayerCorpse() && (this->charid != client->CharacterID() && client->Admin() < 150)) {
		client->Message(13, "Error: This is a player corpse and you dont own it.");
		SendEndLootErrorPacket(client);
		return;
	}
	if (pLocked && client->Admin() < 100) {
		SendLootReqErrorPacket(client, 0);
		client->Message(13, "Error: Corpse locked by GM.");
		return;
	}

	LootingItem_Struct* lootitem = (LootingItem_Struct*)app->pBuffer;
	ServerLootItem_Struct* bag_item_data[10];
	memset(bag_item_data, 0, sizeof(bag_item_data));
	ServerLootItem_Struct* item_data = this->GetItem(lootitem->slot_id, bag_item_data);
	const Item_Struct* item = 0;
	if (item_data != 0)
		item = database.GetItem(item_data->item_nr);
	if (item!=0) {
		if (lootitem->type == 1) {
/*			int16 tmpslot = client->FindFreeInventorySlot(0, (item->type == 0x01), false);
			if (tmpslot == 0xFFFF) {
				client->Message(13, "Error: No room in your inventory, putting the item back onto the corpse.");
			}
			else if (!client->PutItemInInventory(tmpslot, item_data->item_nr, item_data->charges)) {
				client->Message(13, "Error adding this item to your inventory, putting it back on the corpse.");
			}
			else
				this->RemoveItem(item_data->lootslot);
			*/
			ServerLootItem_Struct** tmp = 0;
			if (IsPlayerCorpse())
				tmp = bag_item_data;
			if (client->CheckLoreConflict(item))
				client->Message(13, "Error: Lore item conflict");
			else if (!client->AutoPutItemInInventory(item, &item_data->charges, false, tmp, true))
				client->Message(13, "Error adding this item to your inventory, putting it back on the corpse.");
			else {
				this->RemoveItem(item_data->lootslot);
				if (zone->lootvar!=0){
					int admin=client->Admin();
					if (zone->lootvar==7){
							client->LogLoot(client,this,item);
					}
					else if ((admin>=10) && (admin<20)){
						if ((zone->lootvar<8) && (zone->lootvar>5))
							client->LogLoot(client,this,item);
					}
					else if (admin<=20){
						if ((zone->lootvar<8) && (zone->lootvar>4))
							client->LogLoot(client,this,item);
					}
					else if (admin<=80){
						if ((zone->lootvar<8) && (zone->lootvar>3))
							client->LogLoot(client,this,item);
					}
					else if (admin<=100){
						if ((zone->lootvar<9) && (zone->lootvar>2))
							client->LogLoot(client,this,item);
					}
					else if (admin<=150){
						if (((zone->lootvar<8) && (zone->lootvar>1)) || (zone->lootvar==9))
							client->LogLoot(client,this,item);
					}
					else if (admin<=255){
						if ((zone->lootvar<8) && (zone->lootvar>0))
							client->LogLoot(client,this,item);	
					}
				}
				if (IsPlayerCorpse()) {
					for (int i=0; i<10; i++) {
						if (bag_item_data[i] && bag_item_data[i]->looted)
							this->RemoveItem(bag_item_data[i]);
					}
				}
				if (IsPlayerCorpse()) {
				if (item->item_nr >=10000)
					client->Message(0,"You have looted a %c00%i%s%c",0x12, item->item_nr, item->name, 0x12);
				else
					client->Message(0,"You have looted a %c00%i %s%c",0x12, item->item_nr, item->name, 0x12);
				}
				else{
					if (item->item_nr >=10000){
						entity_list.MessageClose(client->CastToMob(),true,100,0,"%s has looted a %c00%i%s%c",client->GetName(),0x12, item->item_nr, item->name, 0x12);
						client->Message(0,"You have looted a %c00%i%s%c",0x12, item->item_nr, item->name, 0x12);
					}
					else{
						entity_list.MessageClose(client->CastToMob(),true,100,0,"%s has looted a %c00%i %s%c",client->GetName(),0x12, item->item_nr, item->name, 0x12);
						client->Message(0,"You have looted a %c00%i %s%c",0x12, item->item_nr, item->name, 0x12);
					}
				}
			}
		}
		else {
			if (client->CheckLoreConflict(item))
				client->Message(13, "Error: Lore item conflict");
			else if (client->PutItemInInventory(0, item_data->item_nr, item_data->charges)) {
				this->RemoveItem(item_data);
				if (zone->lootvar!=0){
					int admin=client->Admin();
					if (zone->lootvar==7){
							client->LogLoot(client,this,item);
					}
					else if ((admin>=10) && (admin<20)){
						if ((zone->lootvar<8) && (zone->lootvar>5))
							client->LogLoot(client,this,item);
					}
					else if (admin<=20){
						if ((zone->lootvar<8) && (zone->lootvar>4))
							client->LogLoot(client,this,item);
					}
					else if (admin<=80){
						if ((zone->lootvar<8) && (zone->lootvar>3))
							client->LogLoot(client,this,item);
					}
					else if (admin<=100){
						if ((zone->lootvar<9) && (zone->lootvar>2))
							client->LogLoot(client,this,item);
					}
					else if (admin<=150){
						if (((zone->lootvar<8) && (zone->lootvar>1)) || (zone->lootvar==9))
							client->LogLoot(client,this,item);
					}
					else if (admin<=255){
						if ((zone->lootvar<8) && (zone->lootvar>0))
							client->LogLoot(client,this,item);	
					}
				}
				if (IsPlayerCorpse() && item->IsBag()) {
					for (int i=0; i<10; i++) {
						if (bag_item_data[i]) {
							if (client->PutItemInInventory(330 + i, bag_item_data[i]->item_nr, bag_item_data[i]->charges))
								this->RemoveItem(bag_item_data[i]);
						}
					}
				}
				if (IsPlayerCorpse()) {
				if (item->item_nr >=10000)
					client->Message(0,"You have looted a %c00%i%s%c",0x12, item->item_nr, item->name, 0x12);
				else
					client->Message(0,"You have looted a %c00%i %s%c",0x12, item->item_nr, item->name, 0x12);
				}
				else{
					if (item->item_nr >=10000){
						entity_list.MessageClose(client->CastToMob(),true,100,0,"%s has looted a %c00%i%s%c",client->GetName(),0x12, item->item_nr, item->name, 0x12);
						client->Message(0,"You have looted a %c00%i%s%c",0x12, item->item_nr, item->name, 0x12);
					}
					else{
						entity_list.MessageClose(client->CastToMob(),true,100,0,"%s has looted a %c00%i %s%c",client->GetName(),0x12, item->item_nr, item->name, 0x12);
						client->Message(0,"You have looted a %c00%i %s%c",0x12, item->item_nr, item->name, 0x12);
					}
				}

			}			
		}
	}
	else {
		client->Message(15, "Error: Item not found. slot=%i", lootitem->slot_id);
	}
	client->QueuePacket(app);
}

void Corpse::EndLoot(Client* client, const APPLAYER* app) {
	APPLAYER* outapp = new APPLAYER;
	outapp->opcode = OP_LootComplete;
	outapp->size = 0;
	client->QueuePacket(outapp);
	delete outapp;
	
	client->Save();
	this->Save();
	this->BeingLootedBy = 0xFFFFFFFF;
	if (this->IsEmpty()) {
		Delete();
	}
}

void Corpse::FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho) {
	Mob::FillSpawnStruct(ns, ForWho);
	
	if (IsPlayerCorpse())
		ns->spawn.NPC = 3;
	else
		ns->spawn.NPC = 2;
	
	ns->spawn.npc_armor_graphic = texture;
	ns->spawn.npc_helm_graphic = helmtexture;
	
	if (IsPlayerCorpse()) {
		const Item_Struct* item = 0;
		if (helmtexture == 0xFF) {
			item = database.GetItem(GetWornItem(2));
			if (item != 0) {
				ns->spawn.equipment[0] = item->common.material;
				ns->spawn.equipcolors[0] = item->common.color;
			}
		}
		if (texture == 0xFF) {
			item = database.GetItem(GetWornItem(17));
			if (item != 0) {
				ns->spawn.equipment[1] = item->common.material;
				ns->spawn.equipcolors[1] = item->common.color;
			}
			item = database.GetItem(GetWornItem(7));
			if (item != 0) {
				ns->spawn.equipment[2] = item->common.material;
				ns->spawn.equipcolors[2] = item->common.color;
			}
			item = database.GetItem(GetWornItem(10));
			if (item != 0) {
				ns->spawn.equipment[3] = item->common.material;
				ns->spawn.equipcolors[3] = item->common.color;
			}
			item = database.GetItem(GetWornItem(12));
			if (item != 0) {
				ns->spawn.equipment[4] = item->common.material;
				ns->spawn.equipcolors[4] = item->common.color;
			}
			item = database.GetItem(GetWornItem(18));
			if (item != 0) {
				ns->spawn.equipment[5] = item->common.material;
				ns->spawn.equipcolors[5] = item->common.color;
			}
			item = database.GetItem(GetWornItem(19));
			if (item != 0) {
				ns->spawn.equipment[6] = item->common.material;
				ns->spawn.equipcolors[6] = item->common.color;
			}
			item = database.GetItem(GetWornItem(13));
			if (item != 0) {
				if (strlen(item->idfile) >= 3) {
					ns->spawn.equipment[7] = (int8) atoi(&item->idfile[2]);
				}
				else {
					ns->spawn.equipment[7] = 0;
				}
				ns->spawn.equipcolors[7] = 0;
			}
			item = database.GetItem(GetWornItem(14));
			if (item != 0) {
				if (strlen(item->idfile) >= 3) {
					ns->spawn.equipment[8] = (int8) atoi(&item->idfile[2]);
				}
				else {
					ns->spawn.equipment[8] = 0;
				}
				ns->spawn.equipcolors[8] = 0;
			}
		}
	}
}

void Corpse::QueryLoot(Client* to) {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	int x = 0;
	to->Message(0, "Coin: %ip %ig %is %ic", platinum, gold, silver, copper);
	while(iterator.MoreElements())
	{
		const Item_Struct* item = database.GetItem(iterator.GetData()->item_nr);
		if (item)
			to->Message(0, "  %d: %s", item->item_nr, item->name);
		else
			to->Message(0, "  Error: 0x%04x", iterator.GetData()->item_nr);
		x++;
		iterator.Advance();
	}
	to->Message(0, "%i items on %s.", x, this->GetName());
}

void Corpse::Summon(Client* client,bool spell) {
	int32 dist = 10000; // pow(100, 2);
	// TODO: Check consent list
	if (!spell) {
		if (this->GetCharID() == client->CharacterID()) {
			if (this->IsLocked() && client->Admin() < 100) {
				client->Message(13, "Error: Corpse locked by GM.");
			}
			else if (DistNoRootNoZ(client) <= dist) {
				GMMove(client->GetX(), client->GetY(), client->GetZ());
				pIsChanged = true;
			}
			else
				client->Message(0, "Corpse is too far away.");
		}
		else {
			client->Message(0, "Error: You dont own the corpse");
		}
	}
	else {
		GMMove(client->GetX(), client->GetY(), client->GetZ());
		pIsChanged = true;
	}
	Save();
}

void Corpse::CompleteRezz(){
	rezzexp = 0;
	pIsChanged = true;
	this->Save();
}

int32 Database::UpdatePlayerCorpse(int32 dbid, int32 charid, const char* charname, int32 zoneid, uchar* data, int32 datasize, float x, float y, float z, float heading) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = new char[256+(datasize*2)];
	char* end = query;
	int32 affected_rows = 0;
	
	end += sprintf(end, "Update player_corpses SET data=");
	*end++ = '\'';
	end += DoEscapeString(end, (char*)data, datasize);
	*end++ = '\'';
	end += sprintf(end,", charname='%s', zoneid=%u, charid=%d, x=%1.1f, y=%1.1f, z=%1.1f, heading=%1.1f WHERE id=%d", charname, zoneid, charid, x, y, z, heading, dbid);
	
	if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
		delete[] query;
        cerr << "Error1 in UpdatePlayerCorpse query " << errbuf << endl;
		return 0;
    }
	delete[] query;
	
	if (affected_rows == 0) {
        cerr << "Error2 in UpdatePlayerCorpse query: affected_rows = 0" << endl;
		return 0;
	}
	
	return dbid;
}

int32 Database::CreatePlayerCorpse(int32 charid, const char* charname, int32 zoneid, uchar* data, int32 datasize, float x, float y, float z, float heading) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = new char[256+(datasize*2)];
	char* end = query;
    //MYSQL_RES *result;
    //MYSQL_ROW row;
	int32 affected_rows = 0;
	int32 last_insert_id = 0;
	
	end += sprintf(end, "Insert into player_corpses SET data=");
	*end++ = '\'';
	end += DoEscapeString(end, (char*)data, datasize);
	*end++ = '\'';
	end += sprintf(end,", charname='%s', zoneid=%u, charid=%d, x=%1.1f, y=%1.1f, z=%1.1f, heading=%1.1f, timeofdeath=Now()", charname, zoneid, charid, x, y, z, heading);
	
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows, &last_insert_id)) {
		delete[] query;
        cerr << "Error1 in CreatePlayerCorpse query " << errbuf << endl;
		return 0;
    }
	delete[] query;
	
	if (affected_rows == 0) {
        cerr << "Error2 in CreatePlayerCorpse query: affected_rows = 0" << endl;
		return 0;
	}

	if (last_insert_id == 0) {
        cerr << "Error3 in CreatePlayerCorpse query: last_insert_id = 0" << endl;
		return 0;
	}
	
	return last_insert_id;
}

bool Database::LoadPlayerCorpses(int32 iZoneID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	//	int char_num = 0;
	unsigned long* lengths;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, charid, charname, x, y, z, heading, data, timeofdeath FROM player_corpses WHERE zoneid='%u'", iZoneID), errbuf, &result)) {
		//                                               0   1       2         3  4  5  6        7     8
		delete[] query;
		while ((row = mysql_fetch_row(result))) {
			lengths = mysql_fetch_lengths(result);
			entity_list.AddCorpse(Corpse::LoadFromDBData(atoi(row[0]), atoi(row[1]), row[2], (uchar*) row[7], lengths[7], atof(row[3]), atoi(row[4]), atoi(row[5]), atoi(row[6]), row[8]));
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadPlayerCorpses query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return true;
}

bool Database::DeletePlayerCorpse(int32 dbid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "Delete from player_corpses where id=%d", dbid), errbuf)) {
		cerr << "Error in DeletePlayerCorpse query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	delete[] query;
	return true;
}

