 
//Harakiri Sept. 23 2009
//Put method signatures into this file which should be accessable from perl.


	virtual bool	IsMob()		{ return true; }
	bool			VisibleToMob(Mob* other = 0);
	bool			IsFullHP()	{ return this->GetHP() == this->GetMaxHP(); }
	bool			IsSnared()	{ return (this->spellbonuses.MovementSpeed < 0 && this->spellbonuses.MovementSpeed > -100); }
	bool			IsRooted()	{ return (this->spellbonuses.MovementSpeed == -100); }
	virtual	bool	IsAlive()	{ return true; }

	// Get
	int16			GetSkill(int skill_num)		{ return skills[skill_num]; } // socket 12-29-01
	int8			GetEquipment(int item_num)	{ return equipment[item_num]; } // socket 12-30-01
	int16			GetRace()			{ return race; }
	int8			GetGender()			{ return gender; }
	virtual int16	GetBaseRace()		{ return base_race; }
	virtual	int8	GetBaseGender()		{ return base_gender; }
	int8			GetDeity()			{ return deity; }
	int8			GetTexture()		{ return texture; }
	int8			GetHelmTexture()	{ return helmtexture; }
	int8			GetClass()			{ return class_; }
	int8			GetLevel()			{ return level; }
	char*			GetName()			{ return name; }
	Mob*			GetTarget()			{ return target; }	
	int8			GetHPRatio()		{ return (int8)((float)cur_hp/max_hp*100); }
	virtual sint32	GetHP()				{ return cur_hp; }
	virtual sint32	GetMaxHP()			{ return max_hp; }
	virtual sint32	GetMaxMana()		{ return max_mana; }
	virtual sint32	GetMana()			{ return cur_mana; }
	virtual int16	GetATK()			{ return BaseStats.ATK + itembonuses.ATK + spellbonuses.ATK; }
	virtual sint16	GetSTR()			{ return BaseStats.STR + itembonuses.STR + spellbonuses.STR; }
	virtual sint16	GetSTA()			{ return BaseStats.STA + itembonuses.STA + spellbonuses.STA; }
	virtual sint16	GetDEX()			{ return BaseStats.DEX + itembonuses.DEX + spellbonuses.DEX; }
	virtual sint16	GetAGI()			{ return BaseStats.AGI + itembonuses.AGI + spellbonuses.AGI; }
	virtual sint16	GetINT()			{ return BaseStats.INT + itembonuses.INT + spellbonuses.INT; }
	virtual sint16	GetWIS()			{ return BaseStats.WIS + itembonuses.WIS + spellbonuses.WIS; }
	virtual sint16	GetCHA()			{ return BaseStats.CHA + itembonuses.CHA + spellbonuses.CHA; }
	virtual sint16	GetMR()				{ return BaseStats.MR + itembonuses.MR + spellbonuses.MR; }
	virtual sint16	GetCR()				{ return BaseStats.CR + itembonuses.CR + spellbonuses.CR; }
	virtual sint16	GetFR()				{ return BaseStats.FR + itembonuses.FR + spellbonuses.FR; }
	virtual sint16	GetDR()				{ return BaseStats.DR + itembonuses.DR + spellbonuses.DR; }
	virtual sint16	GetPR()				{ return BaseStats.PR + itembonuses.PR + spellbonuses.PR; }	
	float			GetX()				{ return x_pos; }
	float			GetY()				{ return y_pos; }
	float			GetZ()				{ if(this->IsClient()) return (z_pos/10); else return z_pos; }
	float			GetHeading()		{ return heading; }
	float			GetSize()			{ return size; }
	inline int32	GetLastChange()		{ return pLastChange; }
	int8			GetAppearance()		{ return appearance; }	
	Mob*			GetPet();
	int16			GetPetID()			{ return petid;  }
	Mob*			GetOwnerOrSelf();
	Mob*			GetOwner();
	int16			GetOwnerID()		{ return ownerid; }
	int8			GetPetType()		{ return typeofpet; }
	FACTION_VALUE	GetSpecialFactionCon( Mob* iOther);
	int16			GetHitBox()			{ return hitBoxExtension; }
	const float&	GetGuardX()			{ return guard_x; }
	const float&	GetGuardY()			{ return guard_y; }
	const float&	GetGuardZ()			{ return guard_z; }
	bool			IsBard(void)		{ return (class_ == 8); } // Pinedepain // Return true if the mob is a bard, otherwise false 
	bool			IsMesmerized(void)	{ return (mesmerized); } // Pinedepain // Tell if the mob is currently mezzed or not
	int16			GetCastingSpellTargetID()
										{ return this->casting_spell_targetid; }
	int16			GetCastingSpellSlot()
										{ return this->casting_spell_slot; }
	int16			GetCastingSpellInventorySlot()
										{ return this->casting_spell_inventory_slot; }
	bool			GetCurfp()			{ return curfp; }	
	bool			GetSeeInvisible();
	bool			GetSeeInvisibleUndead();
	float			GetRunSpeed();
	float			GetWalkSpeed();
	int32			GetNPCTypeID()		{ return npctype_id; } // rembrant, Dec. 20, 2001
	int32			GetDamageShield()	{ return this->spellbonuses.DamageShield + this->itembonuses.DamageShield; }
	int32			GetReverseDamageShield()	{ return this->spellbonuses.ReverseDamageShield + this->itembonuses.ReverseDamageShield; }
	int8			GetDamageShieldType() { return this->spellbonuses.DamageShieldType; }
	sint8			GetAnimation();
	int32			GetLevelRegen();		
	int16			GetHitChance(Mob* attacker, Mob* defender, int skill_num);
	int16			GetItemHaste();
	int				GetKickDamage();
	int				GetBashDamage();	
	int16			GetMeleeReach();
	int16			GetBaseSize()		{ return base_size; }	
	float			GetDefaultSize();	
	
	// Set
	void	setBoatSpeed(float speed);
	void	SetOwnerID(int16 NewOwnerID, bool despawn = true);
	void	SetPet(Mob* newpet);
	void	SetPetID(int16 NewPetID)				{ petid = NewPetID; }
	void	SetTarget(Mob* mob)						{ target = mob; }	
	void	SetCurfp(bool change)					{ curfp = change; }	
	void	DoManaRegen();
	void	DoEnduRegen();
	void	SetInvisible(bool toggle);
	void	SetInvisibleUndead(bool toggle);
	void	SetInvisibleAnimal(bool toggle);
	virtual sint32	SetHP(sint32 hp);
	virtual void	SetLevel(uint8 in_level)		{ level = in_level; }
	virtual sint32	SetMana(sint32 amount);
	virtual void	SetSkill(int in_skill_num, int8 in_skill_id) { skills[in_skill_num] = in_skill_id; }	

	
	// Actions
	virtual void	GoToBind() {}
	virtual void	Attack(Mob* other, int Hand = 13, bool procEligible = true, bool riposte = false) {}		// 13 = Primary (default), 14 = secondary
	virtual void	Damage(Mob* from, sint32 damage, int16 spell_id, int8 attack_skill = 0x04)  {}
	virtual void	Heal()  {}
	virtual void	Death(Mob* killer, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04) {}
	virtual void	GMMove(float x, float y, float z, float heading = 0.01);
	virtual void	Message(MessageFormat Type, char* pMessage, ...);	
	void			TicProcess();
	void			ChangeHP(Mob* other, sint32 amount, int16 spell_id = 0);
	int16			MonkSpecialAttack(Mob* other, int8 type);
	void			DoAnim(const int8 animnum);
	void			ChangeSize(float in_size);
	void			SendPosUpdate(bool SendToSelf = false, sint32 range = -1, bool CheckLoS = false);
	void			SendBeginCastPacket(int16 spellid, int32 cast_time);
	void			DamageShield(Mob* other);
	void			SpellProcess();
	void			InterruptSpell(bool fizzle = false, bool consumeMana = false);
	// Cofruben: By default (slot = SLOT_ITEMSPELL), SpellFinished does not require any mana.
	// Harakiri: inventory slot 0 by default, only needed when slot is slot_itemspell to remove a charge		
	void			BuffFadeBySlot(int slot, bool iRecalcBonuses);	
	void			BuffFadeAll()					{ this->BuffFade(NULL); }			
	void			Kill();
	void			SaveGuardSpot();
	void			StopSong(void); // Pinedepain // Stop the song (Server side only!), it resets song vars
	void			DoBardSongAnim(int16 spell_id); // Pinedepain // Send the right bard anim to the client
	void			EnableSpellBar(int16 spell_id = 0); // Pinedepain // Send a packet to this mod to enable his spell bar.	
	void			Mesmerize(void); // Pinedepain // Mesmerize this mob	
	void			CalcBonuses(bool refresHaste = false, bool refreshItemBonuses = false);
	void			DoHPRegen(int32 expected_new_hp = 0);
	void			SendHPUpdate();
	void			DoSpecialAttackDamage(Mob *who, int16 skill, sint32 max_damage, sint32 min_damage = 0);

	
	// Check
	void		CheckPet();
	inline bool	CheckAggro(Mob* other) {return hate_list.IsOnHateList(other);}
	bool		CheckLosFN(Mob* other);	
	int16		CheckMaxSkill(int skillid, int8 race, int8 eqclass, int8 level, bool gmRequest = false);

	// Make		
	void	MakePet(int8 in_level, int8 in_class, int16 in_race, int8 in_texture = 0, int8 in_pettype = 0, float in_size = 0, int8 type = 0,int32 min_dmg = 0,int32 max_dmg = 0,sint32  petmax_hp=0);
	void	MakeEyeOfZomm(Mob* caster);

	// Show
	void	ShowStats(Client* client);
	void	ShowBuffs(Client* client);

	
	// Find
	int16			FindSpell(int16 classp, int16 level, int8 type, int8 spelltype = 0); // 0=buff, 1=offensive, 2=helpful
	
	bool			FindType(int8 type);
	float			FindGroundZ(float new_x, float new_y, float z_offset = 0.0);
	float			FindGroundZWithZ(float new_x, float new_y, float new_z, float z_offset = 0.0);			


	// Other
	virtual sint32	CalcMaxHP()		{ return max_hp = (base_hp  + itembonuses.HP + spellbonuses.HP); }
	bool			HasOwner()		{ return(GetOwnerID() != 0); } //Tazadar 06/01/08 we need this in order to use pet dmg 
	float			Dist(Mob* other);
	float			DistNoZ(Mob* other);
	float			DistNoRoot(Mob* other);
	float			DistNoRootNoZ(Mob* other); 
	float			DistanceToLocation(float x, float y, float z);	
	void			SendIllusionPacket(int16 in_race, int16 in_gender = 0xFF, int16 in_texture = 0xFFFF, int16 in_helmtexture = 0xFFFF);	
	void			faceDestination(float x, float y);
	void			CalculateNewFearpoint();
	sint32			CalcMaxMana();	
	
	void	SetX(float x) {x_pos = x;}
	void	SetY(float y) {y_pos = y;}
	void	SetZ(float z) {z_pos = z;}
	void	SetDeltaX(sint16 dx) {delta_x = dx;}
	void	SetDeltaY(sint16 dy) {delta_x = dy;}
	void	SetDeltaZ(sint16 dz) {delta_x = dz;}
	void	SetHeading(int8 h) {heading = h;}
	void	SetVelocity(sint8 a) {animation = a;}	

	sint32	AvoidDamage(Mob* other, sint32 damage, bool riposte);
	bool	CanThisClassDodge();
	bool	CanThisClassParry();
	bool	CanThisClassBlock();
	bool	CanThisClassRiposte();

	bool	CanNotSeeTarget(float mobx, float moby);  

	int16	CalculateACBonuses();		//Tazadar:Return the total AC amount
	sint16	acmod();
	sint16	monkacmod();
	sint16	iksaracmod();
	sint16	rogueacmod();
	int16	GetMitigationAC() {return myMitigationAC;}
	int16	GetAvoidanceAC() {return myAvoidanceAC;}

	bool	IsInvulnerable() {return invulnerable;}
	void	SetInvulnerable(bool in) {invulnerable = in;}
	

	bool	GetInvisibleUndead() { return invisible_undead; }
	bool	GetInvisibleAnimal() { return invisible_animal; }
	bool	GetSpellInvis() { return invisible; }
	bool	GetInvisible();
	bool	GetCanSeeThroughInvis() { return canSeeThroughInvis; }
	bool	GetCanSeeThroughInvisToUndead() { return canSeeThroughInvisToUndead; }
	void	SetCanSeeThroughInvis(bool in) { canSeeThroughInvis = in; }
	void	SetCanSeeThroughInvisToUndead(bool in) { canSeeThroughInvisToUndead = in; }
	void	CancelAllInvisibility();
		
	
	
	// Harakiri Quest methods
	void	Say(const char *format, ...);
	void	Shout(const char *format, ...);
	void	Emote(const char *format, ...);
	inline bool GetQglobal() const {return qglobal;}
	inline int GetNextHPEvent() const { return nexthpevent; }
    void SetNextHPEvent( int hpevent );
	inline int& GetNextIncHPEvent() { return nextinchpevent; }
	void SetNextIncHPEvent( int inchpevent );
	void SetCastingSpellLocationX(float x) { casting_spell_x_location =x; }
	void SetCastingSpellLocationY(float y) { casting_spell_y_location =y; }	