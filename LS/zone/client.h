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
#ifndef CLIENT_H
#define CLIENT_H
class Client;

#include "../common/timer.h"
#include "../common/eq_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "../common/EQNetwork.h"
#include "../common/linked_list.h"
#include "../common/database.h"
#include "errno.h"
#include "../common/classes.h"
#include "../common/races.h"
#include "../common/deity.h"
#include "mob.h"
#include "npc.h"
#include "zone.h"
#include "../common/seperator.h"
#include <assert.h>

#define CLIENT_TIMEOUT		90000
#define CLIENT_LD_TIMEOUT	30000 // length of time client stays in zone after LDing
extern Zone* zone;

class CLIENTPACKET
{
public:
    CLIENTPACKET();
    ~CLIENTPACKET();
    APPLAYER *app;
    bool ack_req;
};

class Client : public Mob
{
public:
    enum CLIENT_CONN_STATUS { CLIENT_CONNECTING1, CLIENT_CONNECTING2, CLIENT_CONNECTING2_5, CLIENT_CONNECTING3,
                          CLIENT_CONNECTING4, CLIENT_CONNECTING5, CLIENT_CONNECTED, CLIENT_LINKDEAD,
                          CLIENT_KICKED, DISCONNECTED, CLIENT_ERROR, CLIENT_CONNECTINGALL };

	Client(EQNetworkConnection* ieqnc);
    ~Client();

	void	AI_Init();
	void	AI_Start(int32 iMoveDelay = 0);
	void	AI_Stop();
	virtual bool IsClient() { return true; }
	virtual void DBAWComplete(int8 workpt_b1, DBAsyncWork* dbaw);
	bool	FinishConnState2(DBAsyncWork* dbaw);
	bool	IsOnBoat;
	void	FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho);
	virtual bool Process();
	void	ReceiveData(uchar* buf, int len);
	void	RemoveData();
	void    Commandlog(const Seperator* sep);
	void	LogMerchant(Client* player,Mob* merchant,Merchant_Purchase_Struct* mp,const Item_Struct* item,bool buying);
	void	SendPacketQueue(bool Block = true);
	void	QueuePacket(const APPLAYER* app, bool ack_req = true, CLIENT_CONN_STATUS = CLIENT_CONNECTINGALL);
	void	FastQueuePacket(APPLAYER** app, bool ack_req = true, CLIENT_CONN_STATUS = CLIENT_CONNECTINGALL);
	void	ChannelMessageReceived(int8 chan_num, int8 language, const char* message, const char* targetname);
	void	ChannelMessageSend(const char* from, const char* to, int8 chan_num, int8 language, const char* message, ...);
	void	Message(int32 type, const char* message, ...);
	void	operator<<(const char* message)		{ Message(0, "%s", message); }

	inline int32	GetIP()			{ return ip; }
	inline int16	GetPort()		{ return port; }
	bool	berserk;
	bool	dead;
	int32 tradecp;
	int32 tradesp;
	int32 tradegp;
	int32 tradepp;
	char tradeitems[500];
	virtual bool	Save() { return Save(0); }
			bool	Save(int8 iCommitNow); // 0 = delayed, 1=async now, 2=sync now
			void	SaveBackup();
	
	inline bool	Connected()		{ return (client_state == CLIENT_CONNECTED); }
	inline bool	InZone()		{ return (client_state == CLIENT_CONNECTED || client_state == CLIENT_LINKDEAD); }
	inline bool	IsState5()		{ return (client_state == CLIENT_CONNECTING5); }
	inline void	Kick()			{ client_state = CLIENT_KICKED; }
	inline void	Disconnect()	{ eqnc->Close(); client_state = DISCONNECTED; }
	inline bool IsLD()			{ return (bool) (client_state == CLIENT_LINKDEAD); }
	void	WorldKick();
	inline int8	GetAnon()		{ return pp.anon; }
	inline PlayerProfile_Struct GetPP()	{ return pp; }
	bool	CheckAccess(sint16 iDBLevel, sint16 iDefaultLevel);

	bool IsEnd(char* string);
	bool IsCommented(char* string);
	char * rmnl(char* nstring);
	void CheckQuests(const char* zonename, const char* message, int32 npc_id, uint16 item_id, Mob* other);
	char * strreplace(const char* searchstring, const char* searchquery, const char* replacement);
	void LogLoot(Client* player,Corpse* corpse,const Item_Struct* item);
	bool	Attack(Mob* other, int Hand = 13, bool = false);	// 13 = Primary (default), 14 = secondary
//	void	Heal();
	void	Damage(Mob* other, sint32 damage, int16 spell_id, int8 attack_skill = 0x04, bool avoidable = true, sint8 buffslot = -1, bool iBuffTic = false);
	void	Death(Mob* other, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04);
	void	MakeCorpse(int32 exploss);

	void	ChangeFirstName(const char* oldname,const char* in_firstname,const char* gmname);

	void	MakeHorseSpawnPacket(int16 spell_id);

	void	Duck();
	void	Stand();

	void	SendHPUpdate();
	virtual void	SetMaxHP();
	sint32	LevelRegen();
	void	SetGM(bool toggle);
	void	SetPVP(bool toggle);
	inline bool	GetPVP()	{ return zone->GetZoneID() == 77 ? true : pp.pvp; }
	inline bool	GetGM()		{ return (bool) pp.gm; }

	inline int16	GetBaseRace()	{ return pp.race; }
	inline int8	GetBaseGender()	{ return pp.gender; }
	inline int8	GetBaseFace()	{ return pp.face;	}
	sint32	CalcMaxMana();
	const sint32&	SetMana(sint32 amount);

	bool	ServerFilter(APPLAYER *app);

	void	BulkSendMerchantInventory(int merchant_id, int16 npcid);

	inline int8	GetBaseSTR()	{ return pp.STR; }
	inline int8	GetBaseSTA()	{ return pp.STA; }
	inline int8	GetBaseCHA()	{ return pp.CHA; }
	inline int8	GetBaseDEX()	{ return pp.DEX; }
	inline int8	GetBaseINT()	{ return pp.INT; }
	inline int8	GetBaseAGI()	{ return pp.AGI; }
	inline int8	GetBaseWIS()	{ return pp.WIS; }
	inline int8	GetLanguageSkill(int16 n)	{ return pp.languages[n]; }


	inline int16	GetAC()			{ return GetCombinedAC_TEST() + itembonuses->AC + spellbonuses->AC; } // Quagmire - this is NOT the right math on this
	inline sint16	GetSTR()		{ int16 str = GetBaseSTR() + itembonuses->STR + spellbonuses->STR; if(str>255)return 255; else return str  /*+ aa.general_skills.named.innate_strength * 2*/; }//might be screwing up
	inline sint16	GetSTA()		{ int16 sta = GetBaseSTA() + itembonuses->STA + spellbonuses->STA; if(sta>255)return 255; else return sta/*+ aa.general_skills.named.innate_stamina * 2*/; }
	inline sint16	GetDEX()		{ int16 dex = GetBaseDEX() + itembonuses->DEX + spellbonuses->DEX; if(dex>255)return 255; else return dex/*+ aa.general_skills.named.innate_dexterity * 2*/; }
	inline sint16	GetAGI()		{ int16 agi = GetBaseAGI() + itembonuses->AGI + spellbonuses->AGI; if(agi>255)return 255; else return agi/*+ aa.general_skills.named.innate_agility * 2*/; }
	inline sint16	GetINT()		{ int16 int_ = GetBaseINT() + itembonuses->INT + spellbonuses->INT; if(int_>255)return 255; else return int_/*+ aa.general_skills.named.innate_intelligence * 2*/; }
	inline sint16	GetWIS()		{ int16 wis = GetBaseWIS() + itembonuses->WIS + spellbonuses->WIS; if(wis>255)return 255; else return wis/*+ aa.general_skills.named.innate_wisdom * 2*/; }
	inline sint16	GetCHA()		{ int16 cha = GetBaseCHA() + itembonuses->CHA + spellbonuses->CHA; if(cha>255)return 255; else return cha/*+ aa.general_skills.named.innate_charisma * 2*/; }

    sint16  GetMaxSTR();
    sint16  GetMaxSTA();
    sint16  GetMaxDEX();
    sint16  GetMaxAGI();
    sint16  GetMaxINT();
    sint16  GetMaxWIS();
    sint16  GetMaxCHA();

    sint16	GetMR();
	sint16	GetFR();
	sint16	GetDR();
	sint16	GetPR();
	sint16	GetCR();

    sint32  GetActSpellRange(int16 spell_id, sint32);
    sint32  GetActSpellValue(int16 spell_id, sint32);
    sint32  GetActSpellCost(int16 spell_id, sint32);
    sint32  GetActSpellDuration(int16 spell_id, sint32);
    sint32  GetActSpellCasttime(int16 spell_id, sint32);

    bool Flurry();
    bool Rampage();

	inline uint32	GetEXP()		{ return pp.exp; }

	inline const sint32&	GetHP()			{ return cur_hp; }
	inline const sint32&	GetMaxHP()		{ return max_hp; }
	inline const sint32&	GetBaseHP()		{ return base_hp; }
	sint32	CalcMaxHP();
	sint32	CalcBaseHP();
	
	int32	cursorcopper;
	int32	cursorsilver;
	int32	cursorgold;
	int32	cursorplatinum;

	void	AddEXP(uint32 add_exp);
	void	SetEXP(uint32 set_exp, uint32 set_aaxp, bool resexp=false);
	//void	SetEXP(uint32 set_exp, bool isrezzexp = false);
	virtual void SetLevel(uint8 set_level, bool command = false);

	void	GoToBind();
	void	SetBindPoint();
	void	MovePC(const char* zonename, float x, float y, float z, int8 ignorerestrictions = 0, bool summoned = false);
	void	MovePC(int32 zoneID, float x, float y, float z, int8 ignorerestrictions = 0, bool summoned = false);
	void	WhoAll();
	void	SummonItem(uint16 item_id, int8 charges = 0);
	int32	NukeItem(int16 itemnum);
	bool	HasItemInInventory(int16 item_id);
	uint32 HasItemInInventory(int16 item_id, uint16 quantity);
	bool	CheckLoreConflict(const Item_Struct* item);
	void	ChangeLastName(const char* in_lastname);

	FACTION_VALUE	GetFactionCon(Mob* iOther);
    FACTION_VALUE   GetFactionLevel(int32 char_id, int32 npc_id, int32 p_race, int32 p_class, int32 p_deity, sint32 pFaction, Mob* tnpc);
	
	void	SetFactionLevel(int32 char_id, int32 npc_id, int8 char_class, int8 char_race, int8 char_deity);
	void    SetFactionLevel2(int32 char_id, sint32 faction_id, int8 char_class, int8 char_race, int8 char_deity, sint32 value);
	virtual void SetSkill(int16 skill_num, int8 skill_id); // socket 12-29-01
	void	AddSkill(int16 skillid, int8 value);
	int16	GetRawItemAC();
	int16	GetCombinedAC_TEST();

	inline int32	LSAccountID()	{ return lsaccountid; }
	inline int32	GetWID()		{ return WID; }
	inline void		SetWID(int32 iWID) { WID = iWID; }
	inline int32	AccountID()		{ return account_id; }
	inline char*	AccountName()	{ return account_name; }
	inline sint16	Admin()			{ return admin; }
	inline int32	CharacterID()	{ return character_id; }
	void	UpdateAdmin(bool iFromDB = true);
	void	UpdateWho(int8 remove = 0);
	bool	GMHideMe(Client* client = 0);

	inline int32	GuildEQID()		{ return guildeqid; }
	inline int32	GuildDBID()		{ return guilddbid; }
	inline int8	GuildRank()		{ return guildrank; }
	bool	SetGuild(int32 in_guilddbid, int8 in_rank);

//	virtual void	CastSpell(int16 spell_id, int16 target_id, int16 slot = 10, int32 casttime = 0xFFFFFFFF);
	void	SendManaUpdatePacket();

    // Disgrace: currently set from database.CreateCharacter. 
	// Need to store in proper position in PlayerProfile...
	int8	GetFace()		{ return pp.face; } 
	int32	PendingGuildInvite; // Used for /guildinvite
	void	WhoAll(Who_All_Struct* whom);

	int16	GetItemAt(int16 in_slot);
	ItemProperties_Struct GetItemPropAt(int16 in_slot);
	bool    MoveItem(int16 to_slot, int16 from_slot, uint8 quantity);
	int16	FindFreeInventorySlot(int16** pointer = 0, bool ForBag = false, bool TryCursor = true);
	bool	PutItemInInventory(int16 slotid, int16 itemid, int8 charges);
	bool	PutItemInInventory(int16 slotid, const Item_Struct* item);
	bool	PutItemInInventory(int16 slotid, const Item_Struct* item, int8 charges, bool update = false);
	void	SendLootItemInPacket(const Item_Struct* item, int16 slotid, int8 charges);
	bool	AutoPutItemInInventory(const Item_Struct* item, int8* charges, bool TryCursor = true, ServerLootItem_Struct** bag_item_data = 0, bool TryWornSlots = false);
	int16	TradeList[90];
	int8	TradeCharges[90];
	bool	DeleteItemInInventory(uint32 slotid, int16 quantity = 0, bool clientupdate = false);
	int16	TradeWithEnt;
	sint8	InTrade;
	void	Stun(int16 duration);
	void	ReadBook(char txtfile[14]);
	void	SendClientMoneyUpdate(int8 type,int32 amount);
	bool	TakeMoneyFromPP(uint32 copper);
	void	AddMoneyToPP(uint32 copper,bool updateclient);
	void	AddMoneyToPP(uint32 copper, uint32 silver, uint32 gold,uint32 platinum,bool updateclient);

	bool	CheckIncreaseSkill(int16 skillid,sint16 chancemodi = 0);
	bool	SimpleCheckIncreaseSkill(int16 skillid,sint16 chancemodi = 0);
	void	FinishTrade(Client* whith);
	
	bool   TGB() {return tgb;}  
	 


    inline int16	GetSkillPoints() {return pp.points;}
    inline void		SetSkillPoints(int16 inp) {pp.points = inp;}
    inline void		IncreaseSkill(int16 skill_id, int value = 1) { if (skill_id <= HIGHEST_SKILL) { pp.skills[skill_id + 1] += value; } }
    inline void		IncreaseLanguageSkill(int16 skill_id, int value = 1) { if (skill_id < 26) { pp.languages[skill_id] += value; } }
    inline int8		GetSkill(uint32 skill_id) { if (skill_id <= HIGHEST_SKILL) { return pp.skills[skill_id + 1]; } return 0; }

	void	ChangeTitle(int8 in_title) { this->pp.title = in_title; }
	void	SetZoneSummonCoords(float x, float y, float z) {zonesummon_x = x; zonesummon_y = y; zonesummon_z = z;}
	int32	pendingrezzexp;
	void	GMKill();
	void	RepairInventory();
	inline bool	IsMedding()	{return medding;}
	inline uint32  GetAAXP()   { return pp.expAA; } // FIXME! pp.expAA; }
	inline int16	GetDuelTarget() { return duel_target; }
	inline bool	IsDueling() { return duelaccepted; }
	inline bool	GetMount() { return hasmount; }
	inline void	SetMount(bool mount) { hasmount = mount; }
	inline void	SetDuelTarget(int16 set_id) { duel_target=set_id; }
	inline void	SetDueling(bool duel) { duelaccepted = duel; }
	void  SendAAStats();
	void  SendAATable();
	inline bool	IsSitting() {return (playeraction == 1);}
	inline bool	IsBecomeNPC() { return npcflag; }
	inline int8	GetBecomeNPCLevel() { return npclevel; }
	inline void	SetBecomeNPC(bool flag) { npcflag = flag; }
	inline void	SetBecomeNPCLevel(int8 level) { npclevel = level; }
	bool	LootToStack(int32 itemid);
	void	SetFeigned(bool in_feigned);
	inline bool    GetFeigned()	{ return feigned; }
	EQNetworkConnection* Connection() { return eqnc; }
	int8	guildchange;
	int16	otherleaderid;

	bool NormalUser(const Seperator* sep);
	bool PrivUser(const Seperator* sep);
	bool VeryPrivUser(const Seperator* sep);
	bool QuestTroupe(const Seperator* sep);
	bool NormalGM(const Seperator* sep);
	bool LeadGM(const Seperator* sep);
	bool ServerOP(const Seperator* sep);
	bool VHServerOP(const Seperator* sep);

    bool GetReduceManaCostItem(int16 &spell_id, char *itemname);
    bool GetExtendedRangeItem(int16 &spell_id, char *itemname);
    bool GetIncreaseSpellDurationItem(int16 &spell_id, char *itemname);
    bool GetReduceCastTimeItem(int16 &spell_id, char *itemname);
    bool GetImprovedHealingItem(int16 &spell_id, char *itemname);
    bool GetImprovedDamageItem(int16 &spell_id, char *itemname);
    sint32 GenericFocus(int16 spell_id, int16 modspellid);
	void SetHorseId(int16 horseid_in) { horseId = horseid_in; }
	int16 GetHorseId() { return horseId; }
	void SetHasMount(bool hasmount_in) { hasmount = hasmount_in; }
	bool GetHasMount() { return hasmount; }
	
	bool BindWound(Mob* bindmob, bool start, bool fail = false);

protected:
	friend class Mob;
	void CalcItemBonuses(StatBonuses* newbon);
	void CalcEdibleBonuses(StatBonuses* newbon);
	void MakeBuffFadePacket(int16 spell_id, int32 slot_id);
	bool tgb;
private:
	sint32	HandlePacket(const APPLAYER *app);
	void	OPRezzAnswer(const APPLAYER *app);
	void	OPMemorizeSpell(const APPLAYER *app);

	int32 pLastUpdate;
	int32 pLastUpdateWZ;
	int8  playeraction;
    void FindItem(const char* search_criteria);
    void FindNPCType(const char* search_criteria);
	void ViewNPCType(uint32 npctypeid);

	EQNetworkConnection* eqnc;

	int32				ip;
	int16				port;
    CLIENT_CONN_STATUS  client_state;
	int32				character_id;
	int32				WID;
	int32				account_id;
	char				account_name[30];
	int32				lsaccountid;
	char				lskey[30];
	sint16				admin;
	int32				guilddbid; // guild's ID in the database
	int8				guildrank; // player's rank in the guild, 0-GUILD_MAX_RANK
	int16				duel_target;
	bool				duelaccepted;
	bool				tellsoff;	// GM /toggle
	bool				gmhideme;
	bool				LFG;
	bool				auto_attack;
	bool				medding;
	bool				hasmount;
	int16				horseId;
	int32				pQueuedSaveWorkID;
	int16				pClientSideTarget;

	PlayerProfile_Struct pp;
	ServerSideFilters_Struct ssfs;
	Item_Struct cis;
	const Item_Struct* weapon1; // Primary Weapon ItemID.
	const Item_Struct* weapon2; // Secondary Weapon Item

	void GuildCommand(const Seperator* sep);
	bool GuildEditCommand(int32 dbid, int32 eqid, int8 rank, const char* what, const char* value);
	void NPCSpawn(const Seperator* sep);
	void CorpseCommand(const Seperator* sep);
	uint32 GetEXPForLevel(uint16 level);

    bool    AddPacket(const APPLAYER *, bool);
    bool    AddPacket(APPLAYER**, bool);
    bool    SendAllPackets();
	LinkedList<CLIENTPACKET *> clientpackets;

	char	zonesummon_name[32];
	float	zonesummon_x;
	float	zonesummon_y;
	float	zonesummon_z;
	int8	zonesummon_ignorerestrictions;

	Timer*	position_timer;
	int8	position_timer_counter;

	Timer*	hpregen_timer;

	Timer*	camp_timer;
	Timer*	process_timer;

	void	SendInventoryItems();
	void	BulkSendInventoryItems();

	LinkedList<FactionValue*> factionvalue_list;
	sint32	GetCharacterFactionLevel(sint32 faction_id);


	bool IsSettingGuildDoor;
	int16 SetGuildDoorID;

	int32       max_AAXP;
	int32		staminacount;
	PlayerAA_Struct aa; // Alternate Advancement!
	bool DecreaseCharges(int16 slotid);
	bool npcflag;
	int8 npclevel;
	bool feigned;
};

#include "parser.h"


#endif

