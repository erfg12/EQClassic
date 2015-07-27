#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "timer.h"
#include "eq_opcodes.h"
#include "eq_packet_structs.h"
#include "EQPacketManager.h"
#include "linked_list.h"
#include "object.h"
#include "database.h"
#include "errno.h"
#include "classes.h"
#include "races.h"
#include "deity.h"
#include "mob.h"
#include "npc.h"
#include "zone.h"
#include "seperator.h"
#include "faction.h"
#include "servertalk.h"
#include "ServerPacket.h"
#include "ZoneGuildManager.h"
#include "boats.h"
#include "languages.h"
#include "inventory.h"

using namespace std;

#define CLIENT_TIMEOUT 90000

#define CLIENT_CONNECTING1	0
#define CLIENT_CONNECTING2	1
#define CLIENT_CONNECTING3	2
#define CLIENT_CONNECTING4	3
#define CLIENT_CONNECTING5	4
#define CLIENT_CONNECTED	5
#define CLIENT_KICKED		6
#define CLIENT_DISCONNECTED	7

#define CAST_CLIENT(c) if(c && c->IsClient()) c->CastToClient()
#define CAST_CLIENT_DEBUG_PTR(c) if(c && c->IsClient()) c->CastToClient()->GetDebugSystemPtr()


//typedef void (Client::*fProcessOP_Code)(APPLAYER*);          

class Client : public Mob
{
public:
	Client(int32 ip, int16 port, int send_socket);
    ~Client();

	virtual bool IsClient() { return true; }
	virtual bool Process();
	void Disconnect();

	void	FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho);
	void	ReceiveData(uchar* buf, int len);
	void	SendPacketQueue(bool Block = true);
	void	QueuePacket(APPLAYER* app, bool ack_req = true);
	
	void			InitProcessArray();
	//fProcessOP_Code process_opcode_array[0xFFFF];
	void	(Client::*process_opcode_array[0xFFFF])(APPLAYER*); 
	void	ProcessOP_Default(APPLAYER* pApp);

	void	ChannelMessageReceived(int8 chan_num, int8 language, char* message, char* targetname);
	void	ChannelMessageSend(char* from, char* to, int8 chan_num, int8 language, char* message, ...);
	void	Message(MessageFormat Type, char* pMessage, ...);
	void	Message(MessageFormat Type, const string& rMessage);
	// Harakiri - chance to skillup language from party or say
	void	TeachLanguage(Client* student, int8 language, int8 chan_num); 
	//inline bool	GetPVP()	{ return zone->GetZoneID() == 77 ? true : pp.pvp; } //77 is arena i believe -Wizzel

	void	AddToCursorBag(int16 item,int16 slot,int8 charges = 1) {pp.cursorbaginventory[slot] = item; pp.cursorItemProprieties[slot].charges=charges; }

	int32	GetIP()			{ return ip; }
	int16	GetPort()		{ return port; }

	bool	Save();

	bool	Connected()		{ return (client_state == CLIENT_CONNECTED); }
	void	Kick() { Save(); client_state = CLIENT_KICKED; }
	int8	GetAnon()		{ return pp.anon; }

	bool IsEnd(char* string);
	bool IsCommented(char* string);
	char * rmnl(char* nstring);
	char * strreplace(char* searchstring, char* searchquery, char* replacement);
	
	void	Attack(Mob* other, int hand = 13, bool procEligible = true, bool riposte = false);	// 13 = Primary (default), 14 = secondary
	void	SetAttackTimer();
	void	Heal();
	void	Damage(Mob* other, sint32 damage, int16 spell_id, int8 attack_skill = 0x04);
	void	Death(Mob* other, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04);
	void	MakeCorpse();
	sint32	SetMana(sint32 amount);

	void	Duck(bool send_update_packet = true);
	void	Sit(bool send_update_packet = true);
	void	Stand(bool send_update_packet = true);

	void	SetMaxHP();
	void	SetGM(bool toggle);

	int16	GetBaseRace()	{ return pp.race; }
	int8	GetBaseGender()	{ return pp.gender; }

	int8	GetBaseSTR()	{ return pp.STR; }
	int8	GetBaseSTA()	{ return pp.STA; }
	int8	GetBaseCHA()	{ return pp.CHA; }
	int8	GetBaseDEX()	{ return pp.DEX; }
	int8	GetBaseINT()	{ return pp.INT; }
	int8	GetBaseAGI()	{ return pp.AGI; }
	int8	GetBaseWIS()	{ return pp.WIS; }
	int8	GetLanguageSkill(int8 n)	{ return pp.languages[n]; }
	virtual void SetLanguageSkill(int languageid, int8 value);


	DebugSystem* GetDebugSystemPtr() { return this->mpDbg; }	// Cofruben: 17/08/08.

	sint16	GetSTR();
	sint16	GetSTA();
	sint16	GetDEX();
	sint16	GetAGI();
	sint16	GetINT();
	sint16	GetWIS();
	sint16	GetCHA();

	uint32	GetEXP()		{ return pp.exp; }

	sint32	GetHP()			{ return cur_hp; }
	sint32	GetMaxHP()		{ return max_hp; }
	sint32	GetBaseHP()		{ return base_hp; }
	sint32	CalcMaxHP();
	sint32	CalcBaseHP();
	uint32	GetTotalLevelExp();

	void	AddEXP(uint32 add_exp, bool resurrected = false);
	void	SetEXP(uint32 set_exp);
	virtual void SetLevel(uint8 set_level, bool show_message = true);
	void	GoToBind(bool death = false);
	void	SetBindPoint();
	void	MovePC(const char* zonename, float x, float y, float z, bool ignorerestrictions = false, bool useSummonMessage = true);
	void	ZonePC(char* zonename, float x, float y, float z);
	// Harakiri the client will automatically decide if he needs to zone or just moved if he is already in the zone
	void	TeleportPC(char* zonename, float x, float y, float z, float heading = 0);
	// Harakiri teleport and translocate is bit different in the client, the translocate seems only be used for the spell
	// whereas TeleportPC should be used for everything else, teleportPC also has heading
	void	TranslocatePC(char* zonename, float x, float y, float z);	
	void	SendTranslocateConfirmation(Mob* caster, Spell* spell);
	void	WhoAll();
	bool	SummonItem(int16 item_id, sint8 charges = 1);
	// Harakiri summon an item from non blob db, charges initially 0 to take default value from db
	bool	SummonItemNonBlob(int16 item_id, sint8 charges = 0);
	void	ChangeSurname(char* in_Surname);

	bool	CanAttackTarget(Mob* target);
	bool	CalculateAttackingSkills(int hand, int8* skill_in_use, int8* attack_skill, Item_Struct** attacking_weapon);
	sint32	CalculateAttackDamage(Mob* defender, Item_Struct* attacking_weapon, int8 skill, int hand, int8 attacker_level, int8 defender_level);
	sint32	GetCriticalHit(Mob* defender, int8 skill, sint32 damage);
	
	void	SetFactionLevel(int32 char_id, int32 npc_id, int8 char_class, int8 char_race, int8 char_deity);
	void    SetFactionLevel2(int32 char_id, sint32 faction_id, int8 char_class, int8 char_race, int8 char_deity, sint32 value);

	int8	GetSkill(int skillid);
	virtual void SetSkill(int skillid, int8 value);

	int32	CharacterID()	{ return character_id; }
	int32	AccountID()		{ return account_id; }
	char*	AccountName()	{ return account_name; }
	int8	Admin()			{ return admin; }
	void	UpdateAdmin();
	void	UpdateWho(bool remove = false);
	bool	GMHideMe(Client* client = 0);

	int32	GuildEQID()		{ return guildeqid; }
	int32	GuildDBID()		{ return guilddbid; }
	int8	GuildRank()		{ return guildrank; }
	bool	SetGuild(int32 in_guilddbid, int8 in_rank);
	void	SendManaUpdatePacket();


    // Disgrace: currently set from database.CreateCharacter. 
	// Need to store in proper position in PlayerProfile...
	int8	GetFace()		{ return pp.face; } 
	int32	PendingGuildInvite; // Used for /guildinvite
	void WhoAll(Who_All_Struct* whom);

	int16	GetItemAt(int16 in_slot);
	int16	FindFreeInventorySlot(int16** pointer = 0, bool ForBag = false, bool TryCursor = true);
	bool	PutItemInInventory(int16 slotid, int16 itemid, sint8 charges);
	bool	PutItemInInventory(int16 slotid, Item_Struct* item);
	bool	PutItemInInventory(int16 slotid, Item_Struct* item, sint8 charges);
	int16   AutoPutItemInInventory(Item_Struct* item, sint8 charges,int tradeslot = -1);
	int16	FindBestEquipmentSlot(Item_Struct* item);
	bool	CanEquipThisItem(Item_Struct* item);
    int16	TradeList[90];
	int8	TradeCharges[90];
	int16	stationItems[10];
	ItemProperties_Struct stationItemProperties[10];
	void	DeleteItemInInventory(uint32 slotid);
	void	DeleteItemInStation();
	void	SetCraftingStation(Object* station){craftingStation=station;};
	Object* GetCraftingStation(void){return craftingStation;};
	int16	TradeWithEnt;
	sint8	InTrade;
	void	Stun(int16 duration, Mob* stunner = NULL, bool melee = false);
	void	Client::ReadBook(char txtfile[8]);

	void	SendClientMoneyUpdate(int8 type,int32 amount); //yay! -Wizzel
	bool	TakeMoneyFromPP(uint32 copper);
	void	AddMoneyToPP(uint32 copper,bool updateclient);
	void	AddMoneyToPP(uint32 copper, uint32 silver, uint32 gold,uint32 platinum,bool updateclient);	
	void	SendClientQuestCompletedFanfare();
	// Harakiri for fanfare sound with money e.g. You receive 3 silver from Guard Elron.
	void	SendClientQuestCompletedMoney(Mob* sender, uint32 copper, uint32 silver, uint32 gold,uint32 platinum);
	// Harakiri trading with mob for quests
	void	FinishTrade(Mob* with);
	void	FinishTrade(Client* with);	

	bool	IsGrouped(){return (pp.GroupMembers[0][0] != '\0' && pp.GroupMembers[1][0] != '\0');}
	bool	GroupInvitePending(){return group_invite_pending;}
	void	SetGroupInvitePending(bool status){group_invite_pending = status;}
	void	ProcessChangeLeader(char* member);
	void	RefreshGroup(int32 groupid, int8 action);

	bool	IsMedding()	{return medding;}

	int16	GetDuelTarget() { return duel_target; }
	bool	IsDueling() { return duelaccepted; }
	void	SetDuelTarget(int16 set_id) { duel_target=set_id; }
	void	SetDueling(bool duel) { duelaccepted = duel; }

	void	SetFeigned(bool in_feigned);
	inline bool GetFeigned() { return feigned; }
	void	SetHide(bool bHidden);
	inline bool GetHide() { return bHide; }
	inline void SetSneaking(bool bSneaking) { sneaking = bSneaking; }  
    inline bool	GetSneaking() { return sneaking; }  

	FACTION_VALUE	GetFactionLevel(int32 char_id, int32 npc_id, int32 p_race, int32 p_class, int32 p_deity, sint32 pFaction, Mob* tnpc);

	uint8 GetWornInstrumentType(void); // Pinedepain // Return the instrument type ID currently worn, 0xFF if none
	uint8 GetInstrumentModByType(uint8 instruType); // Pinedepain // Return the instrument modifier according to the type instruType
	void  SetInstrumentModByType(uint8 instruType,uint8 mod); // Pinedepain // Set the instrument modifier to mod for the type instruType
	void  ResetAllInstrumentMod(void); // Pinedepain // Reset all the instrument modifiers (set everything to 0)
	void  UpdateInstrumentMod(void); // Pinedepain // Update the client's instrument modifier according to weapon1 and weapon2 (Item_Struct*)
	bool  HasRequiredInstrument(int16 spell_id); // Pinedepain // Return true if the bard has the correct instrument to sing the current song, else false
	int8  GetInstrumentModAppliedToSong(Spell* spell); // Pinedepain // Return the instrument mod applied to this song (spell_id)
	bool  CheckLoreConflict(int16 itemid);
	void  SendStationItem(int32 oldPlayerID);
	int16 FindItemInInventory(int16 itemid);
	// Harakiri: finds a specific item type in the inventory i.e. baits see itemtypes.h
	int16 FindItemTypeInInventory(int16 itemtype);
	PlayerProfile_Struct GetPlayerProfile(){return pp;}
	PlayerProfile_Struct* GetPlayerProfilePtr(){return &pp;}

	void	CreateZoneLineNode(char* zoneName, char* connectingZoneName, int16 range);
	void	ScanForZoneLines();

	bool	FearMovement(float x, float y, float z);

	char*	GetAccountName(){return account_name;}
	int16	GetAccountID(){return account_id;}

	bool	IsStunned() { return stunned; }

	bool	GetDebugMe() { return debugMe; }
	void	SetDebugMe(bool choice) { debugMe = choice; }

	int16	GetMyEyeOfZommID() { if(myEyeOfZomm) return myEyeOfZomm->GetID(); else return 0; }
	void	SetMyEyeOfZomm(NPC* npc) { myEyeOfZomm = npc; }

	void	FeignDeath(int16 skillValue);

	int16	GetDeathSave() { return mySaveChance; }
	void	SetDeathSave(int16 SuccessChance) { mySaveChance = SuccessChance; }

	bool	GetFeared() {return feared;}
	void	SetFeared(bool in) {feared = in;}

	bool	GetFearDestination(float x, float y, float z);
	bool	GetCharmDestination(float x, float y);
	void	UpdateCharmPosition(float targetX, float targetY, int16 meleeReach);

	float	fdistance(float x1, float y1, float x2, float y2);
	float	square(float number);

	void	SetCharmed(bool in) {charmed = in;}
	bool	IsCharmed() {return charmed;}

	void	SetBerserk(bool in)	{ berserk = in;};
	bool    IsBerserk();

	// Harakiri Tradeskill related methods
	bool TradeskillExecute(DBTradeskillRecipe_Struct *spec);
	void CheckIncreaseTradeskill(sint16 bonusstat, sint16 stat_modifier, float skillup_modifier, uint16 success_modifier, SkillType tradeskill);

	// Harakiri check LOS to water
	bool	CanFish();
	// Harakiri get some random item from zone or common food id
	void	GoFish();
	void	SendCharmPermissions();
	// Harakiri returns if the player is currently in water
	bool	IsInWater();
	bool	IsInLava();

	// Harakiri Function when timer is up for Instill Doubt
	void DoInstillDoubt();
	
	// Moraj: Bind Wound
	bool BindWound(Mob* bindwound_target, bool fail);

	

	void	SetIsZoning(bool in) { isZoning = in; }
	void	SetZoningX(float in) { zoningX = in; }
	void	SetZoningY(float in) { zoningY = in; }
	void	SetZoningZ(float in) { zoningZ = in; }
	void	SetZoningHeading(int8 in) { tempHeading = in; }
	void	SetUsingSoftCodedZoneLine(bool in) { usingSoftCodedZoneLine = in; }
	void	SetTempHeading(int8 in) { tempHeading = in; }

	Item_Struct* GetPrimaryWeapon() { return weapon1; }
	Item_Struct* GetSecondaryWeapon() { return weapon2; }

	bool	GetVoiceGrafting() { return voiceGrafting; }
	void	SetVoiceGrafting(bool in) { voiceGrafting = in; }

	void	SetPendingRezExp(int32 in) { pendingRezExp = in; }
	int32	GetPendingRezExp() { return pendingRezExp; }
	void	SetPendingRez(bool in) { pendingRez = in; }
	bool	GetPendingRez() { return pendingRez; }
	void	SetPendingCorpseID(int32 in) { pendingCorpseID = in; }
	int32	GetPendingCorpseID() { return pendingCorpseID; }
	void	SetPendingRezX(float in) { pendingRezX = in; }
	float	GetPendingRezX() { return pendingRezX; }
	void	SetPendingRezY(float in) { pendingRezY = in; }
	float	GetPendingRezY() { return pendingRezY; }
	void	SetPendingRezZ(float in) { pendingRezZ = in; }
	float	GetPendingRezZ() { return pendingRezZ; }
	void	SetPendingRezZone(char* in) { strcpy(pendingRezZone, in); }
	char*	GetPendingRezZone() { return pendingRezZone; }
	void	SetPendingSpellID(int16 in) { pendingSpellID = in;}
	int16	GetPendingSpellID() { return pendingSpellID; }
	void	StartPendingRezTimer() { pendingRez_timer->Start(PC_RESURRECTION_ANSWER_DURATION); }
	void	StopPendingRezTimer() { pendingRez_timer->Disable(); }

	void	GetRelativeCoordToBoat(float x,float y,float angle,float &xout,float &yout);

	bool	AddSpellToRainList(Spell* spell, int8 waveCount, float x, float y, float z);
	bool	DoRainWaves();

	bool	IsAlive() { return cur_hp > -10; }

	bool	GetSpellImmunity() { return immuneToSpells; }
	void	SetSpellImmunity(bool val) {immuneToSpells = val; }

	int32	GetSpellRecastTimer(int16 slot) { return spellRecastTimers[slot]; }
	void	SetSpellRecastTimer(int16 slot, int32 recastDelay) { spellRecastTimers[slot] = time(0) + recastDelay; pp.spellSlotRefresh[slot] = time(0) + recastDelay; }
	uint32  GetEXPForLevel(int8 level);
	APPLAYER* CreateTimeOfDayPacket(int8 Hour = 9, int8 Minute = 0, int8 Day = 1, int8 Month = 1, long Year = 1);
	void	RemoveOneCharge(uint32 slotid, bool deleteItemOnLastCharge=false);	
	void	SendItemMissing(int32 itemID, int8 itemType);
protected:
	friend class Mob;
	void ProcessCommand(char* message);
	bool CheckAddSkill(int skill_num, int16 skill_mod = 0);
	void CalcItemBonuses(StatsStruct* newbon);
	void MakeBuffFadePacket(Spell* spell, int8 slot_id);
private:
	bool QuagmireGhostCheck(APPLAYER *app);
	void SendNewZone(NewZone_Struct newzone_data);
	void SendStaminaUpdate(int32 hungerlevel, int32 thirstlevel, int8 fatigue);
	void SendInitiateConsumeItem(ItemTypes itemType, int16 slotID);
	void SendLevelUpdate(int8 SetLevel);
	void SendSkillUpdate(int SkillID, int8 Value);
	void SendLanguageSkillUpdate(int languageID, int8 Value);
	void SendShopItem(sint8 ItemCharges, uint32 MerchantID, Item_Struct* Item);	
	void LoadSpells();
	void Process_ClientConnection1(APPLAYER *app);
	void Process_ClientConnection2(APPLAYER *app);
	void Process_ClientConnection3(APPLAYER *app);
	void Process_ClientConnection4(APPLAYER *app);
	void Process_ClientConnection5(APPLAYER *app);
	void Process_ClientKicked(APPLAYER *app);
	void Process_ClientDisconnected(APPLAYER *app);
	void Process_ShoppingEnd(APPLAYER *app);
	void Process_Jump(APPLAYER *app);
	void Process_SafePoints(APPLAYER *app);
	void Handle_Connect5Guild();
	void Handle_Connect5GDoors();
	void Handle_Connect5Objects();
	void ProcessOP_Death(APPLAYER *app);
	void DeletePetitionsFromClient();
	void Process_DisarmTraps(APPLAYER *app);
	void Process_SenseTraps(APPLAYER *app);
	void Process_InstillDoubt(APPLAYER *app);
	void Process_Track(APPLAYER* app);
	void Process_Action(APPLAYER* app);
	void Process_ConsumeFoodDrink(APPLAYER* app);	
	void ProcessOP_UseDiscipline(APPLAYER* app);
	void ProcessOP_ClientError(APPLAYER* app);
	void ProcessOP_ApplyPoison(APPLAYER* app);
	void ProcessOP_PlayerDeath(APPLAYER *app);
	void ProcessOP_PlayerSave(APPLAYER *app);
	void ProcessOP_ItemMissing(APPLAYER *app);
	
	int32 pLastUpdate;

	void DropTradeItem(Item_Struct* item, sint8 charges,int8 slot);
    void FindItem(char* search_criteria);
	void UpdateGoods(int merchantid);
	void UpdateGoodsBuy(int8 startingslot);
	void UpdateGoodsSell(int32 item_nr);
	bool ShowedVendorItem(int32 item_nr);

	APPLAYER*			PMOutQueuePop();
	void				PMClose();
	Mutex				MPacketManager;
	EQC::Common::Network::EQPacketManager	packet_manager;


	int32				ip;
	int16				port;
	int8				client_state;
	int					send_socket;
	Timer*				timeout_timer;
	int32				character_id;
	int32				account_id;
	char				account_name[30];
	int8				admin;
	int32				guilddbid; // guild's ID in the database
	int8				guildrank; // player's rank in the guild, 0-GUILD_MAX_RANK
	bool				tellsoff;	// GM /toggle
	bool				gmhideme;
	int16				duel_target;
	bool				duelaccepted;
	bool				LFG;
	bool				auto_attack;
	bool				group_invite_pending;
	bool				medding;
	PlayerProfile_Struct pp;
	
	std::vector<SummonedItemWaiting_Struct> summonedItems;

	int32 tradecp;
	int32 tradesp;
	int32 tradegp;
	int32 tradepp;
	int32 npctradecp;
	int32 npctradesp;
	int32 npctradegp;
	int32 npctradepp;
	int merchantid; //Tazadar :the id of the merchant we are trading with
	int32 startingslot; // Tazadar :the slot where the goods begin
	uint16 merchantgoods[30];//Tazadar :all the id of the items showed on merchant window
	int8 totalitemdisplayed;//Tazadar: items showed by the merchant
	float pricemultiplier; // Tazadar: current price multiplier Tazadar.

	Item_Struct* weapon1; // Primary Weapon ItemID.
	Item_Struct* weapon2; // Secondary Weapon Item

	Object* craftingStation; // Tazadar : The current crafting station	
	void	GuildCommand(Seperator* sep);
	bool	GuildEditCommand(int32 dbid, int32 eqid, int8 rank, char* what, char* value);
	void	GuildPCCommandHelp();
	void	GuildGMCommandHelp();
	bool	RemoveFromGuild(int32 in_guilddbid);
	void	GuildChangeRank(const char* name, int32 guildid,int32 oldrank,int32 newrank);
	
	int8	GetLevelForEXP(uint32 exp);
	void	ExpLost();

	void	ProcessOpcode(APPLAYER* pApp);
	void	ProcessOP_ZoneChange(APPLAYER* pApp);
	void	ProcessOP_Camp(APPLAYER* pApp=0);
	void	ProcessOP_MoveItem(APPLAYER* pApp);
	void    ProcessOP_DropItem(APPLAYER* pAPP);
	void    ProcessOP_DropCoin(APPLAYER* pAPP);
	void	ProcessOP_SplitMoney(APPLAYER* pApp);
	void	ProcessOP_PickupItem(APPLAYER* pAPP);
	void	ProcessOP_MoveCoin(APPLAYER* pApp);
	void	ProcessOP_SpawnAppearance(APPLAYER* pApp);
	void	ProcessOP_Surname(APPLAYER* pApp);
	void	ProcessOP_Consider(APPLAYER* pApp);
	void	ProcessOP_ClientUpdate(APPLAYER* pApp);
	void    ProcessOP_ClientTarget(APPLAYER* pApp);
	void    ProcessOP_YellForHelp(APPLAYER* pApp);
	void	ProcessOP_DeleteSpawn(APPLAYER* pApp=0);
	void	ProcessOP_GMZoneRequest(APPLAYER* pApp);
	void	ProcessOP_EndLootRequest(APPLAYER* pApp);
	void	ProcessOP_LootRequest(APPLAYER* pApp);
	void	ProcessOP_LootItem(APPLAYER* pApp);
	
	
	
	
	void    ProcessOP_CastSpell(APPLAYER* pApp);
	void    ProcessOP_ManaChange(APPLAYER* pApp); // Pinedepain // Added this
	void    ProcessOP_InterruptCast(APPLAYER* pApp); // Pinedepain // Added this
	void	ProcessOP_MemorizeSpell(APPLAYER* pApp);
	void	ProcessOP_SwapSpell(APPLAYER* pApp);
	void    ProcessOP_CombatAbility(APPLAYER* pApp);
	void    ProcessOP_Taunt(APPLAYER* pApp);
	void	ProcessOP_GMSummon(APPLAYER* pApp);
	void	ProcessOP_GiveItem(APPLAYER* pApp);
	void	ProcessOP_CancelTrade(APPLAYER *pApp);
	void    ProcessOP_TradeAccepted(APPLAYER* pApp);
	void	ProcessOP_Click_Give(APPLAYER* pApp);
	void	ProcessOP_Random(APPLAYER* pApp);
	void	ProcessOP_GMHideMe(APPLAYER* pApp);
	void	ProcessOP_GMNameChange(APPLAYER* pApp);
	void	ProcessOP_GMKill(APPLAYER* pApp);
	void	ProcessOP_GMSurname(APPLAYER* pApp);
	void	ProcessOP_GMToggle(APPLAYER* pApp);
	void	ProcessOP_LFG(APPLAYER* pApp);
	void	ProcessOP_GMGoto(APPLAYER* pApp);
	void	ProcessOP_ShopRequest(APPLAYER* pApp);
	void	ProcessOP_ShopPlayerBuy(APPLAYER* pApp);
	void	ProcessOP_ShopPlayerSell(APPLAYER* pApp);
	void	ProcessOP_Petition(APPLAYER* pApp);
	void	ProcessOP_TradeSkillCombine(APPLAYER* pApp);
	void	ProcessOP_Social_Text(APPLAYER* pApp);
	void	ProcessOP_PetitionCheckout(APPLAYER* pApp);
	void	ProcessOP_PetitionDelete(APPLAYER* pApp);
	void	ProcessOP_GMDelCorpse(APPLAYER* pApp);
	void	ProcessOP_GMKick(APPLAYER* pApp);
	void	ProcessOP_PetitionCheckIn(APPLAYER* pApp);
	void	ProcessOP_GMServers(APPLAYER* pApp);
	void	ProcessOP_AutoAttack(APPLAYER* pApp);
	void	ProcessOP_BugReport(APPLAYER* pAPP);
	void	ProcessOP_ChannelMessage(APPLAYER* pApp);
	void	ProcessOP_WearChange(APPLAYER* pApp);
	void	ProcessOP_Save(APPLAYER* pApp);
	void	ProcessOP_WhoAll(APPLAYER* pApp);
	void	ProcessOP_CraftingStation(void);
	//void	ProcessOP_UseAbility(APPLAYER* pApp);
	void	ProcessOP_Buff(APPLAYER* pApp);
	void	ProcessOP_GMEmoteZone(APPLAYER* pApp);
	void	ProcessOP_InspectRequest(APPLAYER* pApp);
	void	ProcessOP_InspectAnswer(APPLAYER* pApp);
	void	ProcessOP_ReadBook(APPLAYER* pApp);
	void	Process_UnknownOpCode(APPLAYER* pApp);
	void	ProcessOP_Mend(APPLAYER* pApp=0);
	void	ProcessOP_GroupQuit(APPLAYER* pApp);
	void	ProcessOP_GroupDeclineInvite(APPLAYER* pApp);
	void	ProcessOP_GroupInvite(APPLAYER* pApp);
	void	ProcessOP_GroupFollow(APPLAYER* pApp);
	void    ProcessOP_ClickDoor(APPLAYER* pApp);
	void    ProcessOP_MBRetrieveMessages(APPLAYER* pApp);	
	void    ProcessOP_MBRetrieveMessage(APPLAYER* pApp);	
	void    ProcessOP_MBPostMessage(APPLAYER* pApp);	
	void    ProcessOP_MBEraseMessage(APPLAYER* pApp);	
	void	ProcessOP_Beg(APPLAYER* pApp);
	void	ProcessOP_ConsumeItem(APPLAYER* pApp);
	void	ProcessOP_ConsentRequest(APPLAYER* pApp);
	void	ProcessOP_Forage(APPLAYER* pApp);
	void	ProcessOP_FeignDeath(APPLAYER* pApp);
	void	ProcessOP_RequestDuel(APPLAYER* pApp);
	void	ProcessOP_DuelResponse(APPLAYER* pApp);
	void	ProcessOP_DuelResponse2(APPLAYER* pApp);
	void	ProcessOP_Hide(APPLAYER* pApp);
	void	ProcessOP_Fishing(APPLAYER *pApp);
	void	ProcessOP_RezzRequest(APPLAYER *pApp);
	void	ProcessOP_RezzAnswer(APPLAYER *pApp);  
    void	ProcessOP_Sneak(APPLAYER* pApp);  
	void	ProcessOP_BackSlashTarget(APPLAYER* pApp);
	void	ProcessOP_PickPockets(APPLAYER* pApp);
	void	ProcessOP_AssistTarget(APPLAYER* pApp);
	void	ProcessOP_ClassTraining(APPLAYER* pApp);
	void	ProcessOP_ClassEndTraining(APPLAYER* pApp);
	void	ProcessOP_ClassTrainSkill(APPLAYER *pApp);
	void	ProcessOP_GetOnBoat(APPLAYER* pApp);
	void	ProcessOP_GetOffBoat(APPLAYER* pApp);
	void	ProcessOP_CommandBoat(APPLAYER* pApp);
	void	ProcessOP_SafeFallSuccess(APPLAYER* pApp);
	void	ProcessOP_Illusion(APPLAYER* pApp);
	void	ProcessOP_GMBecomeNPC(APPLAYER* pApp);
	void	ProcessOP_BindWound(APPLAYER* pApp);
	void	ProcessOP_SpawnProjectile(APPLAYER* pApp);
	void	ProcessOP_TranslocateResponse(APPLAYER* pApp);
	
	bool	IsValidGuild();// Need to find a better name for this!

	void SendExpUpdatePacket(int32 set_exp);

	//Disgrace: make due for now...
	char GetCasterClass();

	char	zonesummon_name[16];
	float	zonesummon_x;
	float	zonesummon_y;
	float	zonesummon_z;
	bool	zonesummon_ignorerestrictions;

	DebugSystem* mpDbg;	// Cofruben: added 17/08/08.

	Timer*  beg_timer;

	Timer*  fishing_timer;

	Timer*  instilldoubt_timer;

	Timer*	position_timer;
	int8	position_timer_counter;

	int32	pLastUpdateWZ;

	void	SendInventoryItems();
	void	SendInventoryItems2();

	bool	CanThisClassDoubleAttack(void);
	bool	CanThisClassDuelWield(void);

	// Kaiyodo - Haste support
	int		HastePercentage;
	void	SetHaste(int Haste) { HastePercentage = Haste; }
	int		GetHaste(void) { return(HastePercentage); }

	LinkedList<FactionValue*> factionvalue_list;
	sint32	GetCharacterFactionLevel(sint32 faction_id);
	void	SetCharacterFactionLevelModifier(sint32 faction_id, sint16 amount);

	bool	feigned;
	bool	bHide;
	bool	sneaking;
	int16	ClientSideTarget;
	bool	isZoning;
	int8	tempHeading;
	bool	isZoningZP; //Yeahlight: Client is zoning from a hardcoded zone_point
	bool	usingSoftCodedZoneLine;
	sint32	zoningX;
	sint32	zoningY;
	sint32	zoningZ;

	// Pinedepain // Start -- Instrument skill modifiers
	uint8	singingInstrumentMod;
	uint8	percussionInstrumentMod;
	uint8	stringedInstrumentMod;
	uint8	windInstrumentMod;
	uint8	brassInstrumentMod;
	// Pinedepain // End -- Instrument skill modifiers

	char currentBoat[NPC_MAX_NAME_LENGTH]; // Tazadar : name of the current boat

	Timer*	PickLock_timer;

	bool	immuneToSpells;

	Timer*	stunned_timer;
	bool	stunned;

	int32	myNetworkFootprint;

	bool	debugMe;

	NPC*	myEyeOfZomm;
	float	myEyeOfZommX;
	float	myEyeOfZommY;

	int16	mySaveChance;

	bool	feared;

	Timer*	ticProcess_timer;
	int32	pLastCharmUpdate;

	Timer*	charmPositionUpdate_timer;
	bool	charmed;

	bool	voiceGrafting;

	int32	pendingRezExp;
	bool	pendingRez;
	int32	pendingCorpseID;
	float	pendingRezX;
	float	pendingRezY;
	float	pendingRezZ;
	char	pendingRezZone[16];
	int16	pendingSpellID;
	int32	pendingExpLoss;
	Timer*	pendingRez_timer;

	Timer*	rain_timer;
	RainSpell_Struct rainList[10];

	sint16	projectileStressTestCounter;
	bool	projectileStressTest;
	int16	projectileStressTestID;

	bool	pendingTranslocate;
	int16	pendingTranslocateSpellID;
	float	pendingTranslocateX;
	float	pendingTranslocateY;
	float	pendingTranslocateZ;
	float	pendingTranslocateHeading;
	char	pendingTranslocateZone[16];
	Timer*	pendingTranslocate_timer;

	Timer*	spellCooldown_timer;

	bool	reservedPrimaryAttack;
	bool	reservedSecondaryAttack;

	bool	berserk;

	int32	spellRecastTimers[9];
};

#define STRING_FEIGNFAILED 1456
#define FORAGE_WATER 151
#define FORAGE_GRUBS 150
#define FORAGE_FOOD 152
#define FORAGE_DRINK 153
#define FORAGE_NOEAT 154
#define FORAGE_FAILED 155
#endif

