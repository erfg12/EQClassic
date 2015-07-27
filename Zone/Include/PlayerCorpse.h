#include "database.h"
#include "entity.h"
#include "mob.h"
#include "client.h"
#include "hate_list.h"

class Corpse : public Mob
{
public:
	static void SendEndLootErrorPacket(Client* client);
	static void SendLootReqErrorPacket(Client* client, int8 response = 2);
	static Corpse* LoadFromDBData(int32 in_dbid, int32 in_charid, char* in_charname, uchar* in_data, int32 in_datasize, float in_x, float in_y, float in_z, float in_heading, int8 rezed, int32 in_rotTime, int32 in_accountid, int32 in_rezExp);

	Corpse(NPC* in_npc, ItemList** in_itemlist, int32 in_npctypeid, NPCType** in_npctypedata, int32 in_decaytime = 480000);
	Corpse(Client* client, PlayerProfile_Struct* pp, int32 in_ExpLoss);
	Corpse(int32 in_dbid, int32 in_charid, char* in_charname, ItemList* in_itemlist, int32 in_copper, int32 in_silver, int32 in_gold, int32 in_plat, float in_x, float in_y, float in_z, float in_heading, float in_size, int8 in_gender, int8 in_race, int8 in_class, int8 in_deity, int8 in_level, int8 in_texture, int8 in_helmtexture, int8 rezed, int32 in_rotTime, int32 in_accountid, int32 in_rezexp);
	~Corpse();

	bool	IsCorpse()			{ return true; }
	bool	IsPlayerCorpse()	{ return p_PlayerCorpse; }
	bool	IsNPCCorpse()		{ return !p_PlayerCorpse; }
	void	Summon(Client* client,Client * caster ,bool spell);
	bool	Process();
	bool	Save();
	int32	GetCharID()			{ return charid; }
	void	SetDBID(int32 dbid1);
	int32	GetDBID()			{ return dbid; }
	int32	GetDecayTime()		{ if (corpse_decay_timer == 0) return 0xFFFFFFFF; else return corpse_decay_timer->GetRemainingTime(); }
	void	CalcCorpseName();

	void	SetDecayTimer(int32 decaytime);
	bool	IsEmpty();
	void	AddItem(int16 itemnum, int8 charges, int16 slot = 0);
	int16	GetWornItem(int16 equipSlot);
	ServerLootItem_Struct* GetItem(int16 lootslot);
	ServerLootItem_Struct* GetItemWithEquipSlot(int32 equipslot);
	void	RemoveItem(int16 lootslot);
	void	AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum);
	void	RemoveCash();
	void	QueryLoot(Client* to);
	int32	CountItems();
	void	Delete();
	virtual void	Depop(bool StartSpawnTimer = true);

	uint32	GetCopper()		{ return copper; }
	uint32	GetSilver()		{ return silver; }
	uint32	GetGold()		{ return gold; }
	uint32	GetPlatinum()	{ return platinum; }

	void	FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho);
	void	MakeLootRequestPackets(Client* client, APPLAYER* app);
	void	LootItem(Client* client, APPLAYER* app);
	void	EndLoot(Client* client, APPLAYER* app);
	void	Summon(Client* client);
	void	CastRezz(APPLAYER* pApp, int16 spellid, Mob* Caster);
	void	CompleteRezz();
	bool	IsRezzed() { return isrezzed; }
	void	setRezzed(bool rez){ isrezzed = rez; }
	inline char* GetOwnerName()	{ return orgname;}
	
	void	SendLootMessages(float corpseX, float corpseY, Mob* looter, Item_Struct* item);
	bool	MayLootCorpse(char* clientName);
	bool	GetFreeForAll() { return freeForAll; }
	void	SetFreeForAll(bool in) { freeForAll = in; }
	void	CreateDecayTimerMessage(Client* client);

	float	GetX() { return x_pos; }
	float	GetY() { return y_pos; }
	float	GetZ() { return z_pos; }
	void	SetX(float in) { x_pos = in; }
	void	SetY(float in) { y_pos = in; }
	void	SetZ(float in) { z_pos = in; }

	int32	GetRezExp();
	bool	GetRezzed() { return isrezzed; }
	void	SetRezzed(bool in) { isrezzed = in; }

	char*	GetOrgname() { return orgname; }

protected:
private:
	queue<char*> attackers;
	queue<char*> lootrights;
	bool		p_PlayerCorpse;
	char		orgname[30];
	int32		dbid;
	int32		charid;
	int32		accountid;
	ItemList*	itemlist;
	uint32		copper;
	uint32		silver;
	int32		rezzexp;
	bool		isrezzed;
	uint32		gold;
	uint32		platinum;
	bool		p_depop;
	int32		BeingLootedBy;
	Timer*		corpse_decay_timer;
	Timer*		freeForAll_timer;
	Timer*		save_timer;
	Timer*		updatePCDecay_timer;
	float		x_pos;
	float		y_pos;
	float		z_pos;
	bool		freeForAll;
};
