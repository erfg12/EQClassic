/*
  New class for handeling corpses and everything associated with them.
  Child of the Mob class.
  -Quagmire
*/

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "PlayerCorpse.h"
#include "packet_functions.h"
#include "config.h"
#include "packet_dump_file.h"
#include "groups.h"
using namespace std;

extern EntityList entity_list;
extern Zone* zone;
extern npcDecayTimes_Struct npcCorpseDecayTimes[100];

void Corpse::SendEndLootErrorPacket(Client* client) {
	APPLAYER* outapp = new APPLAYER(OP_LootComplete, 0);
	client->QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
}

void Corpse::SendLootReqErrorPacket(Client* client, int8 response) {
	APPLAYER* outapp = new APPLAYER(OP_MoneyOnCorpse, sizeof(MoneyOnCorpse_Struct));
	MoneyOnCorpse_Struct* d = (MoneyOnCorpse_Struct*) outapp->pBuffer;
	d->response		= response;
	d->unknown1		= 0x5a;
	d->unknown2		= 0x40;
	client->QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
}

Corpse* Corpse::LoadFromDBData(int32 in_dbid, int32 in_charid, char* in_charname, uchar* in_data, int32 in_datasize, float in_x, float in_y, float in_z, float in_heading, int8 rezed, int32 in_rotTime, int32 in_accountid, int32 in_rezExp) {
	if (in_datasize < sizeof(DBPlayerCorpse_Struct)) {
		cerr << "Error1 in LoadFromDBData query " << endl;
		return false;
	}
	DBPlayerCorpse_Struct* dbpc = (DBPlayerCorpse_Struct*) in_data;
	if (in_datasize != (sizeof(DBPlayerCorpse_Struct) + (dbpc->itemcount * sizeof(ServerLootItem_Struct)))) {
		return false;
		cerr << "Error2 in LoadFromDBData query " << endl;
	}
	ItemList* itemlist = new ItemList();
	ServerLootItem_Struct* tmp = 0;
	for (int i=0; i < dbpc->itemcount; i++) {
		tmp = new ServerLootItem_Struct;
		memcpy(tmp, &dbpc->items[i], sizeof(ServerLootItem_Struct));
		itemlist->Append(tmp);
	}
	Corpse* pc = new Corpse(in_dbid, in_charid, in_charname, itemlist, dbpc->copper, dbpc->silver, dbpc->gold, dbpc->plat, in_x, in_y, in_z, in_heading, dbpc->size, dbpc->gender, dbpc->race, dbpc->class_, dbpc->deity, dbpc->level, dbpc->texture, dbpc->helmtexture, rezed, in_rotTime, in_accountid, in_rezExp);
	//Tazadar No rez for u if you dont have anything?? :)
	//Yeahlight: TODO: This is crashing the zone; is this really needed?
	if(false)//pc->IsEmpty() && pc->IsRezzed()) 
	{
		safe_delete(pc);
		return 0;
	}
	else
	{
		return pc;
	}
}

// To be used on NPC death and ZoneStateLoad
Corpse::Corpse(NPC* in_npc, ItemList** in_itemlist, int32 in_npctypeid, NPCType** in_npctypedata, int32 in_decaytime)
 : Mob("Unnamed_Corpse","",0,0,in_npc->GetGender(),in_npc->GetRace(),in_npc->GetClass(),in_npc->GetDeity(),in_npc->GetLevel(), BT_Humanoid, in_npc->GetNPCTypeID(),0,in_npc->GetSize(),0,0,in_npc->GetHeading(),in_npc->GetX(),in_npc->GetY(),in_npc->GetZ(),0,0,in_npc->GetTexture(),in_npc->GetHelmTexture(),0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"",0,0,0,0,0,0)
{
	p_PlayerCorpse = false;
	BeingLootedBy = 0xFFFFFFFF;
	if (in_itemlist) {
		itemlist = *in_itemlist;
		*in_itemlist = 0;
	}
	else {
		itemlist = new ItemList();
	}
	AddCash(in_npc->GetCopper(), in_npc->GetSilver(), in_npc->GetGold(), in_npc->GetPlatinum());

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
	int32 decayTime = in_decaytime;
	//Yeahlight: The corpse is completely empty
	if(IsEmpty())
	{
		decayTime = DEFAULT_EMPTY_NPC_CORPSE_ROT_TIMER;
	}
	//Yeahlight: NPCs level 55 and above have thirty minute rot timers
	else if(in_npc->GetLevel() >= 55)
	{
		decayTime = EXTENDED_NPC_ROT_TIMER;
	}
	corpse_decay_timer = new Timer(decayTime);

	//Yeahlight: Build the hit and loot right lists for this corpse
	attackers = in_npc->GetHitList();
	lootrights = in_npc->GetLootRights();
	
	freeForAll = false;
	freeForAll_timer = new Timer(DEFAULT_FFA_LOOT_TIMER);

	save_timer = new Timer(0);

	updatePCDecay_timer = new Timer(0);

	SetX(in_npc->GetX());
	SetY(in_npc->GetY());
	SetZ(in_npc->GetZ());
}

// To be used on PC death
Corpse::Corpse(Client* client, PlayerProfile_Struct* pp, int32 in_ExpLoss)
 : Mob("Unnamed_Corpse","",0,0,client->GetGender(),client->GetRace(), client->GetClass(), client->GetDeity(),client->GetLevel(), BT_Humanoid,
		0, 0, client->GetSize(), 0, 0,client->GetHeading(),client->GetX(),client->GetY(),client->GetZ(),0,0,client->GetTexture(),client->GetHelmTexture(),0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"",0,0,0,0,0,0)
{
	int32 decayTime = 0;
	int i;

	p_PlayerCorpse = true;
	BeingLootedBy = 0xFFFFFFFF;
	itemlist = new ItemList();
	charid = client->CharacterID();
	accountid = client->AccountID();
	dbid = 0;
	p_depop = false;
	isrezzed = false;
	rezzexp = in_ExpLoss;
	strcpy(orgname, pp->name);
	strcpy(name, pp->name);
	AddCash(pp->copper, pp->silver, pp->gold, pp->platinum);
	pp->copper = 0;
	pp->silver = 0;
	pp->gold = 0;
	pp->platinum = 0;

	//Yeahlight: TODO: Major issue in either here or the save routein, as a PC will occasionally have duplicated items on their corpse
	for (i=0; i<30; i++) {
		AddItem(pp->inventory[i],pp->invItemProprieties[i].charges, i);
		pp->inventory[i] = 0xFFFF;
	}
	for (i=0; i<80; i++) {
		AddItem(pp->containerinv[i], pp->bagItemProprieties[i].charges, i+250);
		pp->containerinv[i] = 0xFFFF;
	}
	for	(i=0;i<10;i++) {
		AddItem(pp->cursorbaginventory[i], pp->cursorItemProprieties[i].charges, i+330);
		pp->cursorbaginventory[i]=0xFFFF;
	}
	
	//Yeahlight: PC corpse level 5 or below
	if(pp->level <= 5)
	{
		decayTime = PC_CORPSE_UNDER_SIX_ROT_TIMER;
	}
	//Yeahlight: PC corpse is over level 5 and naked
	else if(IsEmpty())
	{
		decayTime = PC_CORPSE_NAKED_OVER_SIX_ROT_TIMER;
	}
	//Yeahlight: PC corpse is over level 5 and not naked
	else
	{
		decayTime = PC_CORPSE_OVER_SIX_ROT_TIMER;
	}
	corpse_decay_timer = new Timer(decayTime);

	save_timer = new Timer(PC_CORPSE_SAVE_TIMER);

	freeForAll = false;
	freeForAll_timer = new Timer(DEFAULT_FFA_LOOT_TIMER);
	freeForAll_timer->Disable();

	SetX(client->GetX());
	SetY(client->GetY());
	SetZ(client->GetZ());

	updatePCDecay_timer = new Timer(PC_CORPSE_INCREMENT_DECAY_TIMER);

	client->Save();
	Save();
}

// To be called from LoadFromDBData
Corpse::Corpse(int32 in_dbid, int32 in_charid, char* in_charname, ItemList* in_itemlist, int32 in_copper, int32 in_silver, int32 in_gold, int32 in_plat, float in_x, float in_y, float in_z, float in_heading, float in_size, int8 in_gender, int8 in_race, int8 in_class, int8 in_deity, int8 in_level, int8 in_texture, int8 in_helmtexture, int8 rezed, int32 in_rotTime, int32 in_accountid, int32 in_rezzexp)
 : Mob("Unnamed_Corpse","",0,0,in_gender, in_race, in_class, in_deity, in_level, BT_Humanoid, 0 ,0, in_size, 0, 0, in_heading, in_x, in_y, in_z,0,0,in_texture,in_helmtexture,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"",0,0,0,0,0,0)
{
	p_PlayerCorpse = true;
	BeingLootedBy = 0xFFFFFFFF;
	dbid = in_dbid;
	p_depop = false;
	charid = in_charid;
	itemlist = in_itemlist;
	accountid = in_accountid;
	rezzexp = in_rezzexp;

	strcpy(orgname, in_charname);
	strcpy(name, in_charname);
	copper = in_copper;
	silver = in_silver;
	gold = in_gold;
	platinum = in_plat;
	if(rezed)
		isrezzed = true;
	else
		isrezzed = false;

	save_timer = new Timer(0);

	corpse_decay_timer = new Timer(in_rotTime);

	updatePCDecay_timer = new Timer(PC_CORPSE_INCREMENT_DECAY_TIMER);

	freeForAll = false;
	freeForAll_timer = new Timer(DEFAULT_FFA_LOOT_TIMER);
	freeForAll_timer->Disable();
	SetX(in_x);
	SetY(in_y);
	SetZ(in_z);
}

Corpse::~Corpse() {
	if (p_PlayerCorpse && itemlist) {
		if ((IsEmpty() && dbid != 0 && IsRezzed() )|| p_depop)
			Database::Instance()->DeletePlayerCorpse(dbid);
		else if (!IsEmpty() && !(p_depop && dbid == 0))
			Save();
	}
	safe_delete(itemlist);
	safe_delete(corpse_decay_timer);
	safe_delete(freeForAll_timer);
	safe_delete(save_timer);
	//Yeahlight: TODO: What does this mean and why do we do this?
	if(npctype_id == 0)
		safe_delete(NPCTypedata);
}

/*
  this needs to be called AFTER the entity_id is set
  the client does this too, so it's unchangable
*/
void Corpse::CalcCorpseName() {
	EntityList::RemoveNumbers(name);
	name[14] = 0; name[28] = 0; name[29] = 0;
#ifdef WIN32
	snprintf(name, sizeof(name), "%s's corpse%d", name, GetID());
#else
	//make a copy as glibc snprintf clears destination--misanthropicfiend
	char temp_name[30];
	strncpy(temp_name, name, 30);
	snprintf(name, sizeof(name), "%s's_corpse%d", temp_name, GetID());
#endif
}

bool Corpse::Save()
{
	//Yeahlight: We do not save NPC corpses
	if (p_PlayerCorpse == false)
		return true;

	int32 tmp = this->CountItems();
	int32 tmpsize = sizeof(DBPlayerCorpse_Struct) + (tmp * sizeof(ServerLootItem_Struct));
	int32 rotTime = 0;
	DBPlayerCorpse_Struct* dbpc = (DBPlayerCorpse_Struct*) new uchar[tmpsize];
	dbpc->itemcount = tmp;
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
	if(corpse_decay_timer->Enabled())
		rotTime = corpse_decay_timer->GetRemainingTime();
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	iterator.Reset();
	int32 x = 0;
	while(iterator.MoreElements())
	{
		memcpy((char*) &dbpc->items[x++], (char*) iterator.GetData(), sizeof(ServerLootItem_Struct));
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	if (dbid == 0)
		dbid = Database::Instance()->CreatePlayerCorpse(charid, orgname, zone->GetShortName(), (uchar*) dbpc, tmpsize, x_pos, y_pos, z_pos, heading, IsRezzed(), rotTime, accountid, rezzexp);
	else
		dbid = Database::Instance()->UpdatePlayerCorpse(dbid, charid, orgname, zone->GetShortName(), (uchar*) dbpc, tmpsize, x_pos, y_pos, z_pos, heading, IsRezzed(), GetRezExp());
	safe_delete(dbpc);
	if (dbid == 0) {
		cout << "Error: Failed to save player corpse '" << this->GetName() << "'" << endl;
		return false;
	}
	return true;
}

void Corpse::Delete()
{
	if(IsPlayerCorpse() && dbid)
		Database::Instance()->DeletePlayerCorpse(dbid);
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
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	return x;
}

void Corpse::AddItem(int16 itemnum, int8 charges, int16 slot) {
	if (!Database::Instance()->GetItem(itemnum))
		return;
	ServerLootItem_Struct* item = new ServerLootItem_Struct;
	memset(item, 0, sizeof(ServerLootItem_Struct));
	item->item_nr = itemnum;
	item->charges = charges;
	item->equipSlot = slot;
	(*itemlist).Append(item);
}

ServerLootItem_Struct* Corpse::GetItem(int16 lootslot) {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->lootslot == lootslot)
		{
			return iterator.GetData();
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	return 0;
}

/********************************************************************  
 *                          Tazadar - 7/16/08                       *  
 ********************************************************************  
 *							  GetItemWithEquipSlot                  *  
 ********************************************************************  
 *  + Return the ServerLootItem thanks to the equipslot			    *  
 *											                        *  
 ********************************************************************/  
ServerLootItem_Struct* Corpse::GetItemWithEquipSlot(int32 equipslot) {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	//cout << "equislot wanted = " << (int)equipslot <<endl;
	while(iterator.MoreElements())
	{
		//cout <<"equipslot =" <<(int)iterator.GetData()->equipSlot << endl;
		if (iterator.GetData()->equipSlot == equipslot)
		{
			return iterator.GetData();
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	return 0;
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
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	return 0;
}

void Corpse::RemoveItem(int16 lootslot) {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->lootslot == lootslot)
		{
			APPLAYER* outapp = new APPLAYER(OP_WearChange, sizeof(WearChange_Struct));
			memset(outapp->pBuffer, 0, outapp->size);
			WearChange_Struct* wc = (WearChange_Struct*) outapp->pBuffer;
			wc->spawn_id = this->GetID();

			if (iterator.GetData()->equipSlot == 2)
				wc->wear_slot_id = 0;
			else if (iterator.GetData()->equipSlot == 17)
				wc->wear_slot_id = 1;
			else if (iterator.GetData()->equipSlot == 7)
				wc->wear_slot_id = 2;
			else if (iterator.GetData()->equipSlot == 10)
				wc->wear_slot_id = 3;
			else if (iterator.GetData()->equipSlot == 12)
				wc->wear_slot_id = 4;
			else if (iterator.GetData()->equipSlot == 18)
				wc->wear_slot_id = 5;
			else if (iterator.GetData()->equipSlot == 19)
				wc->wear_slot_id = 6;
			else if (iterator.GetData()->equipSlot == 13)
				wc->wear_slot_id = 7;
			else if (iterator.GetData()->equipSlot == 14)
				wc->wear_slot_id = 8;
			else
				safe_delete(outapp);
			if (outapp != 0) {
				entity_list.QueueClients(0, outapp);
				safe_delete(outapp);
			}

			iterator.RemoveCurrent();
			return;
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

void Corpse::AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum) {
	this->copper = in_copper;
	this->silver = in_silver;
	this->gold = in_gold;
	this->platinum = in_platinum;
}

void Corpse::RemoveCash() {
	this->copper = 0;
	this->silver = 0;
	this->gold = 0;
	this->platinum = 0;
}

void Corpse::SetDBID(int32 dbid1) {
	this->dbid = dbid1;
}

bool Corpse::IsEmpty() {
	if (copper != 0 || silver != 0 || gold != 0 || platinum != 0)
		return false;
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	iterator.Reset();
	return !iterator.MoreElements();
}

bool Corpse::Process()
{
	if(p_depop)
	{
		return false;
	}

	//Yeahlight: Corpse is a player's corpse
	if(IsPlayerCorpse())
	{
		//Yeahlight: Time for the player's corpse to be saved to the database
		if(save_timer->Check())
		{
			Save();
		}

		//Yeahlight: Time to update's the player's corpse rot timer
		if(dbid && updatePCDecay_timer->Check())
		{
			int32 updateTime = 0;
			updateTime = Database::Instance()->GetPCCorpseRotTime(dbid);
			//Yeahlight: PC's corpse has expired
			if(updateTime == 0)
			{
				Delete();
				return false;
			}
			//Yeahlight: Update the corpse decay timer for the zone
			else
			{
				corpse_decay_timer->Start(updateTime);
			}
		}
	}
	//Yeahlight: Corpse is not a player's corpse
	else
	{
		//Yeahlight: Corpse depop timer check
		if(corpse_decay_timer->Check())
		{
			return false;
		}

		//Yeahlight: Free for all timer check
		if(GetFreeForAll() == false && freeForAll_timer->Check())
		{
			SetFreeForAll(true);
			freeForAll_timer->Disable();
		}
	}
	return true;
}

//Yeahlight: This doesn't look safe to me, commenting it out so no one uses it
void Corpse::SetDecayTimer(int32 decaytime)
{
	//if (corpse_decay_timer) {
	//	corpse_decay_timer = new Timer(1);
	//}
	//if (decaytime == 0)
	//	corpse_decay_timer->Trigger();
	//else
	//	corpse_decay_timer->Start(decaytime);
}

void Corpse::MakeLootRequestPackets(Client* client, APPLAYER* app)
{
	//Yeahlight: No client was supplied; abort
	if(client == NULL)
		return;

	if(IsPlayerCorpse() && dbid == 0)
	{
		SendLootReqErrorPacket(client, 0);
		client->Message(RED, "Error: Corpse's dbid = 0! Cannot get data.");
		cout << "Error: PlayerCorpse::MakeLootRequestPackets: dbid = 0!" << endl;
		return;
	}
	if(this->BeingLootedBy != 0xFFFFFFFF)
	{
		// lets double check....
		Entity* looter = entity_list.GetID(this->BeingLootedBy);
		if (looter == 0)
			this->BeingLootedBy = 0xFFFFFFFF;
	}
	//Yeahlight: Check loot rights for NPC corpses
	if(IsPlayerCorpse() == false && MayLootCorpse(client->GetName()) == false)
	{
		SendLootReqErrorPacket(client, 2);
	}
	else if(this->BeingLootedBy != 0xFFFFFFFF && this->BeingLootedBy != client->GetID())
	{
		// ok, now we tell the client to fuck off
		// Quagmire - i think this is the right packet, going by pyro's logs
		SendLootReqErrorPacket(client, 0);
		cout << "Telling " << client->GetName() << " corpse '" << this->GetName() << "' is busy..." << endl;
	}
	else if(IsPlayerCorpse() && (this->charid != client->CharacterID() && client->Admin() < 100))
	{
		// Not their corpse... get lost
		SendLootReqErrorPacket(client, 2);
		cout << "Telling " << client->GetName() << " corpse '" << this->GetName() << "' is busy..." << endl;
	}
	else
	{
		this->BeingLootedBy = client->GetID();
		APPLAYER* outapp = new APPLAYER(OP_MoneyOnCorpse, sizeof(MoneyOnCorpse_Struct));
		MoneyOnCorpse_Struct* d = (MoneyOnCorpse_Struct*) outapp->pBuffer;
		
		d->response		= 1;
		d->unknown1		= 0x5a;
		d->unknown2		= 0x40;
		if (!IsPlayerCorpse() || this->charid == client->CharacterID()) {
			//Tazadar : We check for autosplit , if its on and have a group we share the money
			Group* g = entity_list.GetGroupByClient(client);
			if(!IsPlayerCorpse() && client->GetPlayerProfilePtr()->autosplit == 1 && client->IsGrouped()
				&& g && g->GetMembersInZone() > 1){
				//cout << "Autospliting the money" <<endl;
				g->SplitMoney(this->GetCopper(),this->GetSilver(),this->GetGold(),this->GetPlatinum(),client);
			}
			else {
				d->copper		= this->GetCopper();
				d->silver		= this->GetSilver();
				d->gold			= this->GetGold();
				d->platinum		= this->GetPlatinum();
				client->AddMoneyToPP(this->GetCopper(),this->GetSilver(),this->GetGold(),this->GetPlatinum(),false);
			}
			this->RemoveCash();
		}
		client->QueuePacket(outapp); 
		safe_delete(outapp);//delete outapp;

		LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
		iterator.Reset();
		int counter = 1;
		while(iterator.MoreElements()) {
			ServerLootItem_Struct* item_data = iterator.GetData();
			Item_Struct* item = Database::Instance()->GetItem(item_data->item_nr);
			
			//cout <<"equipslot =" <<(int)item_data->equipSlot << endl;

			if (item && (item_data->equipSlot<30 || !IsPlayerCorpse())) {// Tazadar : we do not show items in bags of a player corpse
				
				item_data->lootslot = counter;
				outapp = new APPLAYER(OP_ItemOnCorpse, sizeof(ItemOnCorpse_Struct));
				memcpy(outapp->pBuffer, item, sizeof(Item_Struct));
				// modify the copy of the item
				item = (Item_Struct*)outapp->pBuffer;
				item->equipSlot = item_data->lootslot;
				if (item->flag != 0x7669)
					item->common.charges = item_data->charges;
				client->QueuePacket(outapp);
				counter++;
				safe_delete(outapp);
			}
			iterator.Advance();
			//Yeahlight: Zone freeze debug
			if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
				EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		}
	}							
	// Disgrace: Client seems to require that we send the packet back...
	client->QueuePacket(app);
	Save();
}

/********************************************************************  
 *                          Tazadar - 7/16/08                       *  
 ********************************************************************  
 *							  LootItem		                        *  
 ********************************************************************  
 *  + Check if you already have a lore item in your inventory ^^    *  
 *	+ Add items that are in the player corpse bag in the looted     *
 *	  bag									                        *  
 ********************************************************************/  
void Corpse::LootItem(Client* client, APPLAYER* app) {

	if (this->BeingLootedBy != client->GetID()) {
		client->Message(RED, "Error: Corpse::LootItem: BeingLootedBy != client");
		SendEndLootErrorPacket(client);
		return;
	}else if(this->IsPlayerCorpse() && strstr(this->CastToCorpse()->GetName(), client->GetName()) == NULL){
		client->Message(RED,"You may NOT loot this corpse.");
		return;
	}
	LootingItem_Struct* lootitem = (LootingItem_Struct*)app->pBuffer;
	ServerLootItem_Struct* item_data = this->GetItem(lootitem->slot_id);
	Item_Struct* item = 0;
	if (item_data != 0)
		item = Database::Instance()->GetItem(item_data->item_nr);
	if (item!=0) {
		if(client->CheckLoreConflict(item->item_nr)){//Tazadar: If we already have the lore item
			client->QueuePacket(app);
			return;
		}
		if(IsPlayerCorpse() && item->type==0x01){//Tazadar: We are looting a bag on a player corpse , we check lore conflicts before looting it, if it fails -> no loot !
			//Tazadar : We only check playerCorpses because mobs only drop empty bags !
			for(int i=0;i<item->common.container.BagSlots;i++){
				int32 bagslot=-1; // Tazadar : Slot of the item in our inventory when we died
				if(item_data->equipSlot==0){ // Tazadar : We can now have the slot of each item in the bag
					bagslot=330+i; 
				}
				else{
					bagslot=10*(item_data->equipSlot-22)+250+i;
				}

				ServerLootItem_Struct* bagitem_data = 0;
				bagitem_data=this->GetItemWithEquipSlot(bagslot); // Tazadar : We load item infos 
				if (bagitem_data != 0){
					if(client->CheckLoreConflict(bagitem_data->item_nr)){// Tazadar : If we have a lore conflict we can't loot
						client->QueuePacket(app);
						return;
					}
				}
			}
		}
		if (lootitem->type == 1) {//Tazadar : We can loot and we are using left click (AUTOLOOT)
			int32 slotnum=client->AutoPutItemInInventory(item, item_data->charges,-1);//Tazadar : We loot the bag or the item
			if(slotnum!=-1){
				if(IsPlayerCorpse() && item->type==0x01){// Tazadar : If its a bag we do not forget items in it.
					
					for(int i=0;i<item->common.container.BagSlots;i++){
						int32 bagslot=-1;//Tazadar : Slot of the item in our inventory when we died
						if(item_data->equipSlot==0){// Tazadar : Bag was on cursor (slot 0)
							bagslot=330+i;
						}
						else{// Tazadar : Bag was in main inventory (slot 22 -> 29)
							bagslot=10*(item_data->equipSlot-22)+250+i;
						}

						ServerLootItem_Struct* bagitem_data = this->GetItemWithEquipSlot(bagslot);
						Item_Struct* bagitem = 0;
						if (bagitem_data != 0){// Tazadar : We load the item from the DB in order to send it in the packet
							bagitem = Database::Instance()->GetItem(bagitem_data->item_nr);

						}
						if (bagitem!=0) {
							if(slotnum==0){//Tazadar : We put the item in cursor bag
								client->PutItemInInventory(330+i,bagitem,bagitem_data->charges);
							}
							else{//Tazadar : We put the item in inventory bag
								client->PutItemInInventory(10*(slotnum-22)+250+i,bagitem,bagitem_data->charges);
							}
							this->RemoveItem(bagitem_data->lootslot); //Tazadar: We remove the item which is in the bag
						}
					}
				}
				this->RemoveItem(item_data->lootslot);//Tazadar: We remove the item 
			}

		}
		else {//Tazadar : We can loot and we are using right click (item on cursor)
			client->SummonItem(item_data->item_nr, item_data->charges);
			if(IsPlayerCorpse() && item->type==0x01){
				//cout <<"looking for bag items"<< endl;
				for(int i=0;i<item->common.container.BagSlots;i++){
					int32 bagslot=-1;
					if(item_data->equipSlot==0){
						bagslot=330+i;
					}
					else{
						bagslot=10*(item_data->equipSlot-22)+250+i;
					}

					ServerLootItem_Struct* bagitem_data = this->GetItemWithEquipSlot(bagslot);
					Item_Struct* bagitem = 0;
					if (bagitem_data != 0){
						bagitem = Database::Instance()->GetItem(bagitem_data->item_nr);
		
					}
					if (bagitem!=0) {
						client->PutItemInInventory(330+i,bagitem,bagitem_data->charges);
						this->RemoveItem(bagitem_data->lootslot); //Tazadar: We remove the item which is in the bag
					}
				}
			}
			this->RemoveItem(item_data->lootslot);//Tazadar: We remove the item 
		}
	}
	else {
		client->Message(YELLOW, "Error: Item not found.");
	}
	client->QueuePacket(app);
	Save();
}

void Corpse::EndLoot(Client* client, APPLAYER* app) {
	APPLAYER* outapp = new APPLAYER(OP_LootComplete, 0);
	client->QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;

	client->Save();
	this->BeingLootedBy = 0xFFFFFFFF;
	if (this->IsEmpty()) {
		Delete();
	}
	else{
		this->Save();
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
		Item_Struct* item = 0;
		if (helmtexture == 0xFF) {
			item = Database::Instance()->GetItem(GetWornItem(2));
			if (item != 0) {
				ns->spawn.equipment[0] = item->common.material;
				ns->spawn.equipcolors[0] = item->common.color;
			}
		}
		if (texture == 0xFF) {
			item = Database::Instance()->GetItem(GetWornItem(17));
			if (item != 0) {
				ns->spawn.equipment[1] = item->common.material;
				ns->spawn.equipcolors[1] = item->common.color;
			}
			item = Database::Instance()->GetItem(GetWornItem(7));
			if (item != 0) {
				ns->spawn.equipment[2] = item->common.material;
				ns->spawn.equipcolors[2] = item->common.color;
			}
			item = Database::Instance()->GetItem(GetWornItem(10));
			if (item != 0) {
				ns->spawn.equipment[3] = item->common.material;
				ns->spawn.equipcolors[3] = item->common.color;
			}
			item = Database::Instance()->GetItem(GetWornItem(12));
			if (item != 0) {
				ns->spawn.equipment[4] = item->common.material;
				ns->spawn.equipcolors[4] = item->common.color;
			}
			item = Database::Instance()->GetItem(GetWornItem(18));
			if (item != 0) {
				ns->spawn.equipment[5] = item->common.material;
				ns->spawn.equipcolors[5] = item->common.color;
			}
			item = Database::Instance()->GetItem(GetWornItem(19));
			if (item != 0) {
				ns->spawn.equipment[6] = item->common.material;
				ns->spawn.equipcolors[6] = item->common.color;
			}
			item = Database::Instance()->GetItem(GetWornItem(13));
			if (item != 0) {
				if (strlen(item->idfile) >= 3) {
					ns->spawn.equipment[7] = (int8) atoi(&item->idfile[2]);
				}
				else {
					ns->spawn.equipment[7] = 0;
				}
				ns->spawn.equipcolors[7] = 0;
			}
			item = Database::Instance()->GetItem(GetWornItem(14));
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
	to->Message(BLACK, "Coin: %ip %ig %is %ic", platinum, gold, silver, copper);
	while(iterator.MoreElements())
	{
		Item_Struct* item = Database::Instance()->GetItem(iterator.GetData()->item_nr);
		if (item)
			to->Message(BLACK, "  %d: %s", item->item_nr, item->name);
		else
			to->Message(BLACK, "  Error: %04x", iterator.GetData()->item_nr);
		x++;
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	to->Message(BLACK, "%i items on %s.", x, this->GetName());
}

void Corpse::Summon(Client* client) {
	int32 dist = 10000; // pow(100, 2);
	// TODO: Check consent list
	if (this->GetCharID() == client->CharacterID()) {
		if (DistNoRootNoZ(client) <= dist)
			GMMove(client->GetX(), client->GetY(), client->GetZ());
		else
			client->Message(BLACK, "Corpse is too far away.");
	}
}

int32 Database::UpdatePlayerCorpse(int32 dbid, int32 charid, char* charname, char* zonename, uchar* data, int32 datasize, float x, float y, float z, float heading, bool rezed, int32 rezExp)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = new char[256+(datasize*2)];
	char* end = query;
	int32 affected_rows = 0;
	int8 rez = 0;
	
	if(rezed)
		rez = 1;

	end += sprintf(end, "Update player_corpses SET data=");
	*end++ = '\'';
	end += DoEscapeString(end, (char*)data, datasize);
	*end++ = '\'';
	end += sprintf(end,", charname='%s', zonename='%s', charid=%i, x=%1.1f, y=%1.1f, z=%1.1f, heading=%1.1f , rezed=%i, rezexp=%i WHERE id=%i", charname, zonename, charid, x, y, z, heading, rez, rezExp, dbid);

	if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);//delete[] query;
        cerr << "Error1 in UpdatePlayerCorpse query " << errbuf << endl;
		return 0;
    }
	safe_delete_array(query);//delete[] query;

	if (affected_rows == 0) {
        cerr << "Error2 in UpdatePlayerCorpse query: affected_rows = 0" << endl;
		return 0;
	}
	if(dbid != 0)
		return dbid;
	else
		return 1;
}

int32 Database::CreatePlayerCorpse(int32 charid, char* charname, char* zonename, uchar* data, int32 datasize, float x, float y, float z, float heading, bool rezed, int32 remainingTime, int32 accountid, int32 rezExp)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = new char[256+(datasize*2)];
	char* end = query;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int32 affected_rows = 0;
	int32 last_insert_id = 0;
	int8 rez = 0;
	int32 rezTimer = PC_CORPSE_REZ_DECAY_TIMER;
	
	if(rezed)
	{
		rez = 1;
		rezTimer = 0;
	}

	end += sprintf(end, "Insert into player_corpses SET data=");
	*end++ = '\'';
	end += DoEscapeString(end, (char*)data, datasize);
	*end++ = '\'';
	end += sprintf(end,", charname='%s', zonename='%s', charid=%i, x=%1.1f, y=%1.1f, z=%1.1f, heading=%1.1f, rezed=%i, time=%i, accountid=%i, reztime=%i, rezexp=%i", charname, zonename, charid, x, y, z, heading, rez, remainingTime, accountid, rezTimer, rezExp);

    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows, &last_insert_id)) {
		safe_delete_array(query);
        cerr << "Error1 in CreatePlayerCorpse query " << errbuf << endl;
		return 0;
	}
	

	if (affected_rows == 0) {
        cerr << "Error2 in CreatePlayerCorpse query: affected_rows = 0" << endl;
		return 0;
	}

	if (last_insert_id == 0) {
        cerr << "Error3 in CreatePlayerCorpse query: last_insert_id = 0" << endl;
		return 0;
	}
	//cerr << "insert_id= " << last_insert_id << endl;
    return last_insert_id;
}

bool Database::LoadPlayerCorpses(char* zonename) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	int char_num = 0;
	unsigned long* lengths;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, charid, charname, x, y, z, heading, data, time, rezed, accountid, rezexp FROM player_corpses WHERE zonename='%s'", zonename), errbuf, &result)) {
//                                                       0   1       2         3  4  5  6        7     8     9      10         11
		safe_delete_array(query);//delete[] query;
		while(row = mysql_fetch_row(result))
		{
			lengths = mysql_fetch_lengths(result);
			entity_list.AddCorpse(Corpse::LoadFromDBData(atoi(row[0]), atoi(row[1]), row[2], (uchar*) row[7], lengths[7], atof(row[3]), atof(row[4]), atof(row[5]), atof(row[6]), atoi(row[9]), atoi(row[8]), atoi(row[10]), atoi(row[11])));
			cerr << "loaded dbid= " << atoi(row[0])  << endl;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadPlayerCorpses query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	return true;
}

bool Database::DeletePlayerCorpse(int32 dbid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "Delete from player_corpses where id=%d", dbid), errbuf)) {
		cerr << "Error in DeletePlayerCorpse query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	safe_delete_array(query);//delete[] query;
	return true;
}

void Corpse::Summon(Client* client, Client * caster, bool spell)
{
	// TODO: Check consent list
	if(!spell) 
	{
		//GM stuff
	}
	else 
	{
		GMMove(caster->GetX(), caster->GetY(), caster->GetZ());
	}
	Save();
}

//o--------------------------------------------------------------
//| SendLootMessages; Yeahlight, June 26, 2009
//o--------------------------------------------------------------
//| Issues loot messages to everyone on the hate list
//o--------------------------------------------------------------
void Corpse::SendLootMessages(float corpseX, float corpseY, Mob* looter, Item_Struct* item)
{
	//Yeahlight: Corpse, looter or item was not supplied; abort
	if(looter == NULL || item == NULL)
		return;

	queue<char*> names;
	char* name;
	names = attackers;
	while(names.empty() == false)
	{
		name = names.front();
		Mob* attacker = entity_list.GetMob(name);
		if(attacker && attacker->IsClient())
		{
			float distance = attacker->DistanceToLocation(corpseX, corpseY);
			distance = distance * distance;
			if(distance < DEFAULT_MESSAGE_RANGE)
			{
				//Yeahlight: This PC is the looter of the item
				if(attacker == looter)
				{
					attacker->Message(DARK_BLUE, "--You have looted a %s.--", item->name);
				}
				//Yeahlight: This PC is not the looter of the item
				else
				{
					attacker->Message(DARK_BLUE, "--%s has looted a %s.--", attacker->GetName(), item->name);
				}
			}
		}
		names.pop();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

//o--------------------------------------------------------------
//| MayLootCorpse; Yeahlight, June 26, 2009
//o--------------------------------------------------------------
//| Returns true of the supplied name may loot the corpse
//o--------------------------------------------------------------
bool Corpse::MayLootCorpse(char* clientName)
{
	//Yeahlight: No client name was supplied; abort
	if(clientName == NULL)
		return false;

	//Yeahlight: Corpse is free for all, so anyone may loot it
	if(GetFreeForAll())
		return true;

	queue<char*> list;
	list = lootrights;
	char* tempName = "";
	//Yeahlight: Keep iterating through the loot rights list and check for clientName
	while(list.empty() == false)
	{
		tempName = list.front();
		//Yeahlight: Found a match, give permission to loot this corpse
		if(strcmp(tempName, clientName) == 0)
		{
			return true;
		}
		list.pop();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return false;
}

//o--------------------------------------------------------------
//| CreateDecayTimerMessage; Yeahlight, June 26, 2009
//o--------------------------------------------------------------
//| Sends a rot timer messages to the supplied client
//o--------------------------------------------------------------
void Corpse::CreateDecayTimerMessage(Client* client)
{
	//Yeahlight: This corpse does not decay or a client was not supplied; abort
	if(corpse_decay_timer == NULL || client == NULL)
		return;

	int32 timeLeft = 0;
	int32 totalSeconds = 0;
	int16 seconds = 0;
	int16 minutes = 0;
	int16 hours = 0;
	int16 days = 0;

	//Yeahlight: Corpse is a PC's corpse and the accurate timer is stored in the database
	if(IsPlayerCorpse() && dbid)
	{
		//Yeahlight: The client requesting the rot timer is the owner of the corpse, so we may use the local timer
		if(accountid == (int32)client->GetAccountID())
		{
			timeLeft = corpse_decay_timer->GetRemainingTime();
		}
		//Yeahlight: The client requesting the rot timer is not the owner of the corpse, so we must query the database
		else
		{
			timeLeft = Database::Instance()->GetPCCorpseRotTime(dbid);
		}
	}
	//Yeahlight: Corpse is an NPC's corpse
	else
	{
		timeLeft = corpse_decay_timer->GetRemainingTime();
	}
	totalSeconds = timeLeft / 1000;

	//Yeahlight Rot timer is one minute or more
	if(totalSeconds > 59)
	{
		seconds = totalSeconds % 60;
		minutes = totalSeconds / 60;
		//Yeahlight Rot timer is one hour or more
		if(minutes > 59)
		{
			minutes = minutes % 60;
			hours = totalSeconds / 3600;
			//Yeahlight Rot timer is one day or more
			if(hours > 23)
			{
				hours = hours % 24;
				days = totalSeconds / 86400;
				client->Message(BLACK, "This corpse will decay in %i day(s) %i hour(s) %i minute(s) %i seconds.", days, hours, minutes, seconds);
			}
			//Yeahlight Rot timer is less than one day
			else
			{
				client->Message(BLACK, "This corpse will decay in %i hour(s) %i minute(s) %i seconds.", hours, minutes, seconds);
			}
		}
		//Yeahlight Rot timer is less than one hour
		else
		{
			client->Message(BLACK, "This corpse will decay in %i minute(s) %i seconds.", minutes, seconds);
		}
	}
	//Yeahlight Rot timer is less than one minute
	else
	{
		//Yeahlight: Corpse will decay in 1999ms or less; issue the expire warning
		if(timeLeft < 2000)
			client->Message(BLACK, "This corpse is waiting to expire.");
		else
			client->Message(BLACK, "This corpse will decay in 0 minute(s) %i seconds.", totalSeconds);
	}
}

//o--------------------------------------------------------------
//| GetRezExp; Yeahlight, June 27, 2009
//o--------------------------------------------------------------
//| Returns the total amount of rez exp available for the corpse
//o--------------------------------------------------------------
int32 Corpse::GetRezExp()
{
	//Yeahlight: Corpse has already been resurrected; abort with 0 exp
	if(GetRezzed() || rezzexp == 0)
		return 0;

	bool hasBeenRezzed = false;
	bool rezTimeOnCorpse = false;

	//Yeahlight: The corpse appears to have never been resurrected for exp, but we need to check the database next
	hasBeenRezzed = Database::Instance()->GetCorpseRezzed(dbid);
	
	//Yeahlight: Corpse has been resurrected for exp in the past
	if(hasBeenRezzed)
	{
		SetRezzed(true);
		return 0;
	}
	
	//Yeahlight: Query the player corpse table for the rez time remaining on the body
	rezTimeOnCorpse = Database::Instance()->CheckCorpseRezTime(dbid);
	
	//Yeahlight: Reztime still remains on the corpse
	if(rezTimeOnCorpse)
		return rezzexp;
	else
		return 0;
}

//o--------------------------------------------------------------
//| CheckCorpseRezTime; Yeahlight, June 27, 2009
//o--------------------------------------------------------------
//| Returns true if the corpse has a remaining reztime
//o--------------------------------------------------------------
bool Database::CheckCorpseRezTime(int32 dbid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	bool ret = false;
	//Yeahlight: See if this account is currently logged in
	if(RunQuery(query, MakeAnyLenString(&query, "SELECT reztime FROM player_corpses WHERE id = %i", dbid), errbuf, &result))
	{
		row = mysql_fetch_row(result);
		if(row)
		{
			//Yeahlight: A reztime exists on the corpse
			if(atoi(row[0]) > 0)
			{
				ret = true;
			}
		}
		mysql_free_result(result);
	}
	//Yeahlight: Query was not successful
	else
	{
		cerr << "Error in CheckCorpseRezTime query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);

	return ret;
}

//o--------------------------------------------------------------
//| SetCorpseRezzed; Yeahlight, July 1, 2009
//o--------------------------------------------------------------
//| Flags the corpse as "rezed" in the database
//o--------------------------------------------------------------
bool Database::SetCorpseRezzed(int32 dbid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	if(!RunQuery(query, MakeAnyLenString(&query, "UPDATE player_corpses SET rezed=1 WHERE id=%i", dbid), errbuf, 0, &affected_rows))
	{	
		cerr << "Error in SetCorpseRezzed query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);

	if(affected_rows == 0)
		return false;
	else
		return true;
}

//o--------------------------------------------------------------
//| GetCorpseRezzed; Yeahlight, July 1, 2009
//o--------------------------------------------------------------
//| Returns true if the corpse has been flagged as "rezed"
//o--------------------------------------------------------------
bool Database::GetCorpseRezzed(int32 dbid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	bool ret = false;
	//Yeahlight: Grab this corpse's "rezed" flag
	if(RunQuery(query, MakeAnyLenString(&query, "SELECT rezed FROM player_corpses WHERE id = %i", dbid), errbuf, &result))
	{
		row = mysql_fetch_row(result);
		if(row)
		{
			//Yeahlight: The corpse has been resurrected for exp before
			if(atoi(row[0]) != 0)
			{
				ret = true;
			}
		}
		mysql_free_result(result);
	}
	//Yeahlight: Query was not successful
	else
	{
		cerr << "Error in GetCorpseRezzed query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);
	return ret;
}