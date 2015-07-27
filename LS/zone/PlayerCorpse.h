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
#include "../common/database.h"
#include "entity.h"
#include "mob.h"
#include "client.h"

#define MAX_LOOTERS 6

class Corpse : public Mob
{
public:
	static void Corpse::SendEndLootErrorPacket(Client* client);
	static void Corpse::SendLootReqErrorPacket(Client* client, int8 response = 2);
	static Corpse* Corpse::LoadFromDBData(int32 in_corpseid, int32 in_charid, char* in_charname, uchar* in_data, int32 in_datasize, float in_x, float in_y, float in_z, float in_heading, char* timeofdeath);

	Corpse::Corpse(NPC* in_npc, ItemList** in_itemlist, int32 in_npctypeid, NPCType** in_npctypedata, int32 in_decaytime = 600000);
	Corpse::Corpse(Client* client, PlayerProfile_Struct* pp,sint32 in_rezexp, int8 iCorpseLevel);
	Corpse::Corpse(int32 in_corpseid, int32 in_charid, char* in_charname, ItemList* in_itemlist, int32 in_copper, int32 in_silver, int32 in_gold, int32 in_plat, float in_x, float in_y, float in_z, float in_heading, float in_size, int8 in_gender, int16 in_race, int8 in_class, int8 in_deity, int8 in_level, int8 in_texture, int8 in_helmtexture,int32 in_rezexp);
	Corpse::~Corpse();

    void GoToBind() {}
    bool Attack(Mob* other, int Hand = 13, bool b = false) {return false;}
    void Heal() {}
    void Death(Mob* killer, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04) {}

	bool	IsCorpse()			{ return true; }
	bool	IsPlayerCorpse()	{ return p_PlayerCorpse; }
	bool	IsNPCCorpse()		{ return !p_PlayerCorpse; }
	bool	Process();
	bool	Save();
	int32	GetCharID()			{ return charid; }
	int32	SetCharID(int32 iCharID) { if (IsPlayerCorpse()) { return (charid=iCharID); } return 0xFFFFFFFF; };
	int32	GetDecayTime()		{ if (corpse_decay_timer == 0) return 0xFFFFFFFF; else return corpse_decay_timer->GetRemainingTime(); }
	void	CalcCorpseName();
	inline void		Lock()			{ pLocked = true; }
	inline void		UnLock()		{ pLocked = false; }
	inline bool		IsLocked()		{ return pLocked; }
	inline void		ResetLooter()	{ BeingLootedBy = 0xFFFFFFFF; }
	inline int32	GetDBID()		{ return dbid; }
	inline char*	GetOwnerName()	{ return orgname;}

	void	SetDecayTimer(int32 decaytime);
	bool	IsEmpty();
	void	AddItem(int16 itemnum, int8 charges, int16 slot = 0);
	int16	GetWornItem(int16 equipSlot);
	ServerLootItem_Struct* GetItem(int16 lootslot, ServerLootItem_Struct** bag_item_data = 0);
	void	RemoveItem(int16 lootslot);
	void	RemoveItem(ServerLootItem_Struct* item_data);
	void	ItemRemoved(ServerLootItem_Struct* item_data);
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
	void	MakeLootRequestPackets(Client* client, const APPLAYER* app);
	void	LootItem(Client* client, const APPLAYER* app);
	void	EndLoot(Client* client, const APPLAYER* app);
	void	Summon(Client* client, bool spell);
	void	CastRezz(int16 spellid, Mob* Caster);
	void	CompleteRezz();

	bool CanMobLoot(const char* iName);
	void AllowMobLoot(const char*, int8 slot);

    void StartEnrage() {}
    bool IsEnraged() {return false;}
    bool Flurry() {return false;}
    bool Rampage() {return false;}

    sint16	GetMR() { return 0; }
	sint16	GetFR()	{ return 0; }
	sint16	GetDR()	{ return 0; }
	sint16	GetPR()	{ return 0; }
	sint16	GetCR() { return 0; }
	char		orgname[64];
	bool IsRezzed() { return isrezzed; }
protected:
private:
	bool		p_PlayerCorpse;
	bool		pIsChanged;
	bool		pLocked;
	int32		dbid;
	int32		charid;
	ItemList*	itemlist;
	uint32		copper;
	uint32		silver;
	uint32		gold;
	uint32		platinum;
	bool		p_depop;
	int32		BeingLootedBy;
	int32		rezzexp;
	bool		isrezzed;
	char		looters[MAX_LOOTERS][64]; // People allowed to loot the corpse
	Timer*	corpse_decay_timer;
	Timer*	corpse_delay_timer;
};

