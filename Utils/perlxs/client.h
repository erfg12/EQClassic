 
//Harakiri Sept. 23 2009
//Put method signatures into this file which should be accessable from perl.



	virtual bool IsClient() { return true; }	
	void Disconnect();

	void	Message(MessageFormat Type, char* pMessage, ...);
	
	void	AddToCursorBag(int16 item,int16 slot,int8 charges = 1) {pp.cursorbaginventory[slot] = item; pp.cursorItemProprieties[slot].charges=charges; }

	int32	GetIP()			{ return ip; }
	int16	GetPort()		{ return port; }

	bool	Save();

	bool	Connected()		{ return (client_state == CLIENT_CONNECTED); }
	void	Kick() { Save(); client_state = CLIENT_KICKED; }
	int8	GetAnon()		{ return pp.anon; }

	void	Attack(Mob* other, int hand = 13, bool procEligible = true, bool riposte = false);	// 13 = Primary (default), 14 = secondary
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
	bool	SummonItem(int16 item_id, sint8 charges = 1);
	// Harakiri summon an item from non blob db, charges initially 0 to take default value from db
	bool	SummonItemNonBlob(int16 item_id, sint8 charges = 0);
	void	ChangeSurname(char* in_Surname);

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

	int16	GetItemAt(int16 in_slot);	
	bool	PutItemInInventory(int16 slotid, int16 itemid, sint8 charges);			
	void	DeleteItemInInventory(uint32 slotid);
	void	SendClientMoneyUpdate(int8 type,int32 amount); //yay! -Wizzel
	bool	TakeMoneyFromPP(uint32 copper);	
	void	AddMoneyToPP(uint32 copper, uint32 silver, uint32 gold,uint32 platinum,bool updateclient);	
	void	SendClientQuestCompletedFanfare();
	// Harakiri for fanfare sound with money e.g. You receive 3 silver from Guard Elron.
	void	SendClientQuestCompletedMoney(Mob* sender, uint32 copper, uint32 silver, uint32 gold,uint32 platinum);
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
	bool  CheckLoreConflict(int16 itemid);
	void  SendStationItem(int32 oldPlayerID);
	int16 FindItemInInventory(int16 itemid);
	// Harakiri: finds a specific item type in the inventory i.e. baits see itemtypes.h
	int16 FindItemTypeInInventory(int16 itemtype);

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

	bool	IsAlive() { return cur_hp > -10; }

	bool	GetSpellImmunity() { return immuneToSpells; }
	void	SetSpellImmunity(bool val) {immuneToSpells = val; }

	int32	GetSpellRecastTimer(int16 slot) { return spellRecastTimers[slot]; }
	void	SetSpellRecastTimer(int16 slot, int32 recastDelay) { spellRecastTimers[slot] = time(0) + recastDelay; pp.spellSlotRefresh[slot] = time(0) + recastDelay; }
	uint32  GetEXPForLevel(int8 level);
	void	RemoveOneCharge(uint32 slotid, bool deleteItemOnLastCharge=false);