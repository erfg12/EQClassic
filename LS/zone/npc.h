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
#ifndef NPC_H
#define NPC_H

class NPC;
#include "../common/database.h"
#include "mob.h"
//#include "spawn.h"


#include "spawn2.h"
#include "loottable.h"
#include "zonedump.h"

#ifdef WIN32
	#define  M_PI	3.141592
#endif
extern Database database;

//typedef LinkedList<Item_Struct*> ItemList;

class NPC : public Mob
{
public:
	static NPC* SpawnNPC(const char* spawncommand, float in_x, float in_y, float in_z, float in_heading = 0, Client* client = 0);
	static sint8 GetAILevel(bool iForceReRead = false);

	NPC(const NPCType* data, Spawn2* respawn, float x, float y, float z, float heading, bool IsCorpse = false);
	virtual ~NPC();

	virtual bool IsNPC() { return true; }

	virtual bool Process();
	void	AI_Init();
	void	AI_Start(int32 iMoveDelay = 0);

    void Heal() {} // todo: add some code
	virtual void SetTarget(Mob* mob);

	// neotokyo: added frenzy
	bool	Attack(Mob* other, int Hand = 13, bool = false);
	void	Damage(Mob* other, sint32 damage, int16 spell_id, int8 attack_skill = 0x04, bool avoidable = true, sint8 buffslot = -1, bool iBuffTic = false);
	void	Death(Mob* other, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04);
	bool	DatabaseCastAccepted(int spell_id);
	int32	spelllimit;

	bool	IsFactionListAlly(sint32 other_faction);
	sint8	CheckNPCFactionAlly(sint32 other_faction);
	FACTION_VALUE GetFactionCon(Mob* iOther);

	void	GoToBind()	{ GMMove(org_x, org_y, org_z, org_heading); }

	void	InteractiveChat(int8 chan_num, int8 language, const char * message, const char* targetname,Mob* sender);
	void	TakenAction(int8 action,Mob* actiontaker);

	void	AddItem(const Item_Struct* item, int8 charges, int8 slot = 0);
	void	AddItem(int32 itemid, int8 charges, int8 slot = 0);
	void	AddLootTable();
/*
	void	NPCSpecialAttacks(const char* parse, int permtag);
	void	NPCDatabaseSpells(const char* parse);
	void	NPCUnharmSpell(int spell_id);
	void	CheckFriendlySpellStatus();

	void	CheckEnemySpellStatus();
	void	NPCHarmSpell(int target,int type);
	void    HateSummon();
*/

	bool	IsRanger() { return rangerstance; }

	void    RemoveItem(uint16 item_id, int16 quantity = 0, int16 slot = 0);
	bool	AddNPCSpells(int32 iDBSpellsID, AISpells_Struct* AIspells);
//	void	RemoveItem(uint16 item_id);
	void	ClearItemList();
	ServerLootItem_Struct*	GetItem(int slot_id);
	void	AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum);
	void	AddCash();
	void	RemoveCash();
	ItemList*	GetItemList() { return itemlist; }
	void	QueryLoot(Client* to);
	int32	CountLoot();
	bool	passengers;
	void	DumpLoot(int32 npcdump_index, ZSDump_NPC_Loot* npclootdump, int32* NPCLootindex);
	inline int32	GetLoottableID()	{ return loottable_id; }
	void	SetPetType(int16 in_type)	{ typeofpet = in_type; } // put this here because only NPCs can be anything but charmed pets

	inline uint32	GetCopper()		{ return copper; }
	inline uint32	GetSilver()		{ return silver; }
	inline uint32	GetGold()		{ return gold; }
	inline uint32	GetPlatinum()	{ return platinum; }

	void SetGrid(int16 grid_){ grid=grid_; }
	void SetSp2(int32 sg2){ spawn_group=sg2; }
	void SetWaypointMax(int16 wp_){ wp_m=wp_; }

	int16 GetWaypointMax(){ return wp_m; }
	int16 GetGrid(){ return grid; }
	int32 GetSp2(){ return spawn_group; }

	uint32	MerchantType;
	void	Depop(bool StartSpawnTimer = true);
	void	Stun(int16 duration);
	inline bool	IsStunned() { return stunned; }


	inline bool	IsInteractive() { return interactive; }
	inline bool	IsPVP() { return pvp; }
	inline int8	CurrentPosition() { return position; }

	inline sint16	GetMR() { return m_MR + itembonuses->MR + spellbonuses->MR; }
	inline sint16	GetFR()	{ return m_FR + itembonuses->FR + spellbonuses->FR; }
	inline sint16	GetDR()	{ return m_DR + itembonuses->DR + spellbonuses->DR; }
	inline sint16	GetPR()	{ return m_PR + itembonuses->PR + spellbonuses->PR; }
	inline sint16	GetCR() { return m_CR + itembonuses->CR + spellbonuses->CR; }

	inline int8	HasBanishCapability() { return banishcapability; }

	inline const sint32&	GetNPCFactionID()	{ return npc_faction_id; }
	inline sint32			GetPrimaryFaction()	{ return primary_faction; }
	inline Mob*	GetIgnoreTarget() { return ignore_target; }
	inline void	SetIgnoreTarget(Mob* mob) {ignore_target = mob; }
	sint32	GetNPCHate(Mob* in_ent)  {return hate_list.GetEntHate(in_ent);}
    bool    IsOnHatelist(Mob*p) { return hate_list.IsOnHateList(p);}

	void	SetNPCFactionID(sint32 in) { npc_faction_id = in; database.GetFactionIdsForNPC(npc_faction_id, &faction_list, &primary_faction); }
	void	SetFeignMemory(const char* num) {feign_memory = num;}

	inline const char*    GetFeignMemory()	{ return feign_memory; }

	float   org_x, org_y, org_z, org_heading;

	bool	IsCityController() { return citycontroller; }
	bool	IsGuildBank() { return guildbank; }
	void	SetGuildOwner(int32 guild) { guildowner = guild; }
	int32	GetGuildOwner() { return guildowner; }
	void	SetGuildTerrainName(char* terrain) { strcpy(ownedterrain,terrain); }
	char*	GetGuildTerrainName() { return ownedterrain; }
	ItemList*	itemlist;

	bool	interactive;
	bool	citycontroller;
	bool	guildbank;

	int8	cityfactionstanding;

	bool	rangerstance;

	Spawn2*	respawn2;
protected:
	friend class EntityList;
	LinkedList<struct NPCFaction*> faction_list;
	Mob*	ignore_target;
	uint32	copper;
	uint32	silver;
	uint32	gold;
	uint32	platinum;
	int16   grid;
	int32   spawn_group;
	int16	wp_m;

	sint32	npc_faction_id;
	sint32	primary_faction;
	
	Timer*	forget_timer;
	Timer*	attacked_timer;
	int		attack_event;

    int16 m_MR;
    int16 m_CR;
    int16 m_DR;
    int16 m_FR;
    int16 m_PR;

    bool	evader;
	int8	position;	// 0 - Standing, 1 - Sitting, 2 - Crouching, 4 - Looting
	bool	pvp;
	int8	tired;
	int8	tiredmax;
	Timer*	interactive_timer;
	Timer*	sendhpupdate_timer;

	int8	banishcapability;
	int16	max_dmg;
	int16	min_dmg;
	const char*	feign_memory;
	int8    forgetchance;

	int32	guildowner;
	char*	ownedterrain;
private:
	int32	loottable_id;
	bool	p_depop;
};

#endif

