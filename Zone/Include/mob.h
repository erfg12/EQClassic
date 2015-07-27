#ifndef MOB_H
#define MOB_H

#include "EQPacketManager.h"
#include "entity.h"
#include "hate_list.h"
#include <vector>
#include "../../Utils/azone/map.h"
#include "zone.h"
#include "../../common/include/types.h"
#include "spdat.h" // Pinedepain // We need it!
#include "Spell.hpp"
//#include "eq_packet_structs.h"
#include "nodes.h"
#include "priorityqueue.h"
#include "spawngroup.h"
class Node;

//LOS Parameters:
#define HEAD_POSITION 0.9f	//ratio of GetSize() where NPCs see from
#define SEE_POSITION 0.5f	//ratio of GetSize() where NPCs try to see for LOS
#define CHECK_LOS_STEP 1.0f
#define LOS_DEFAULT_HEIGHT 6.0f

const int StandingAppearance	= 0;
const int SittingAppearance		= 1;
const int DuckingAppearance		= 2;
const int WalkingAppearance		= 5;
const int UNKNOWN3Appearance	= 3;
const int UNKNOWN5Appearance	= 5;

enum PATH_STATUS : int16
{
	PATH_FOUND = 0,
	PATH_NOT_FOUND = 1,
	TELEPORTATION_NEEDED = 2
};

enum {	
	FeignDeathReuseTime = 9,
	SneakReuseTime = 7,
	HideReuseTime = 8,
	TauntReuseTime = 5,
	InstillDoubtReuseTime = 9,
	FishingReuseTime = 11,
	ForagingReuseTime = 50,
	MendReuseTime = 290,
	TrackingReuseTime = 9,
	BashReuseTime = 12,
	BackstabReuseTime = 9,
	KickReuseTime = 5,
	TailRakeReuseTime = 6,
	EagleStrikeReuseTime = 5,
	RoundKickReuseTime = 9,
	TigerClawReuseTime = 6,
	FlyingKickReuseTime = 7,
	SenseTrapsReuseTime = 9,
	DisarmTrapsReuseTime = 9,
	HarmTouchReuseTime = 4300,
	LayOnHandsReuseTime = 4300,
	FrenzyReuseTime = 10
};

struct Buffs_Struct {
	Buffs_Struct() { memset(this, 0, sizeof(this)); }
	Spell*	spell;
	int8	casterlevel;
	int16	casterid;		// Maybe change this to a pointer sometime, but gotta make sure it's 0'd when it no longer points to anything
	TBuffDurationFormula
			durationformula;
	int32	ticsremaining;
	sint16	diseasecounters;
	sint16	poisoncounters;
	sint16	damageRune;
};

struct StatsStruct  //struct StatBonuses 
{
	sint16	AC;
	sint32	HP;
	sint16	Mana;
	sint16	HPRegen;
	sint16	ManaRegen;
	sint16	ATK;
	sint16	STR;
	sint16	STA;
	sint16	DEX;
	sint16	AGI;
	sint16	INT;
	sint16	WIS;
	sint16	CHA;
	sint16	MR;
	sint16	FR;
	sint16	CR;
	sint16	PR;
	sint16	DR;
	sint16	DamageShield;
	sint16	ReverseDamageShield;//Yeahlight: Let's seperate normal from reverse damage shields since a single reverse DS completely cancel out normal ones
	int8	DamageShieldType;	//Yeahlight: This is stored in resist_type: 1)Thorns, 2)Fire, 3)Magic
	sint16	MovementSpeed;
	int16	DoTer_id;			//Cofruben: Who casted a DoT spell on this mob? Needed for experience.
	sint16	FrenzyRadius;		//Cofruben: Frenzy radius should be aggro radius, I think.
	sint16	Harmony;			//Cofruben: Another radius to avoid assisting to npc friends of same faction. It's TODO still.
	sint16	AttackSpeed;		//Yeahlight: Amount of haste (40 = 40% haste)
	sint16	StackingHaste;		//Yeahlight: Amount of stacking haste (10 = 10% stacking haste)
	sint16	OverHaste;			//Yeahlight: Amount of overhaste (5 = 5% overhaste). I do not believe we have any overhaste spells in our era, though
	sint16	DebuffAttackSpeed;	//Yeahlight: Amount of slow (60 = 60% slow)
};

enum : int8 {	//type arguments to DoAnim
	animKick				= 1,
	animPiercing			= 2,	//might be piercing?
	anim2HSlashing			= 3,
	anim2HWeapon			= 4,
	anim1HWeapon			= 5,
	animDualWield			= 6,
	animTailRake			= 7,	//slam & Dpunch too
	animHand2Hand			= 8,
	animShootBow			= 9,
	animRoundKick			= 11,
	animSwarmAttack			= 20,	//dunno about this one..
	animFlyingKick			= 45,
	animTigerClaw			= 46,
	animEagleStrike			= 47,

};

struct damageInterval
{
	int16 interval;
	int16 chance;
};

using std::vector;

class Mob : public Entity
{
public:

	enum eStandingPetOrder { SPO_Follow=0, SPO_Sit=1, SPO_Guard=2 };//Tazadar  - 5/31/08 We need this for the pet management

	static int8 GetDefaultGender(int16 in_race, int16 in_gender = 0xFFFF);

	void RogueBackstab(Mob* other, Item_Struct *weapon1, int8 bs_skill);
	bool BehindMob(Mob* other, float playerx, float playery);
	static int32	RandomTimer(int min, int max); //implemented from 4.4 for use in pick pockets. May be useful elsewhere. -Wizzel

	Mob(char*   in_name,
	    char*   in_Surname,
	    sint32  in_cur_hp,
	    sint32  in_max_hp,
	    int8    in_gender,
	    int8    in_race,
	    int8    in_class,
	    int8    in_deity,
	    int8    in_level,
		TBodyType
				in_body_type,
		int32   in_npctype_id, // rembrant, Dec. 20, 2001
		int16*	in_skills, // socket 12-29-01
		float	in_size,
		float	in_walkspeed,
		float	in_runspeed,
	    float   in_heading,
	    float	in_x_pos,
	    float	in_y_pos,
	    float	in_z_pos,

	    int8    in_light,
	    int8*   in_equipment,
		int8	in_texture,
		int8	in_helmtexture,
		int16	in_ac,
		int16	in_atk,
		int8	in_str,
		int8	in_sta,
		int8	in_dex,
		int8	in_agi,
		int8	in_int,
		int8	in_wis,
		int8	in_cha,
		sint16	in_mr,
		sint16	in_cr,
		sint16	in_fr,
		sint16	in_dr,
		sint16	in_pr,
		int32	in_min_dmg,
		int32	in_max_dmg,
		char	in_my_special_attacks[20],
		sint16	in_attack_speed_modifier,
		int16	in_main_hand_texture,
		int16	in_off_hand_texture,
		sint16	in_hp_regen_rate,
		bool	 in_passive_see_invis,
		bool	 in_passive_see_invis_to_undead,
		SPAWN_TIME_OF_DAY in_time_of_day = Any_Time
	);
	virtual ~Mob();

	// Cofruben: reorganised a little the class...

	// Is
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
	Buffs_Struct&	GetBuff(int8 index);
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
	virtual TBodyType
					GetBodyType()		{ return this->body_type; }	// Cofruben: 16/08/08.
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
	Spell*			GetCastingSpell()	{ return this->casting_spell; }
	int16			GetCastingSpellTargetID()
										{ return this->casting_spell_targetid; }
	int16			GetCastingSpellSlot()
										{ return this->casting_spell_slot; }
	int16			GetCastingSpellInventorySlot()
										{ return this->casting_spell_inventory_slot; }
	bool			GetCurfp()			{ return curfp; }
	const char*		GetRandPetName();
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
	eStandingPetOrder GetPetOrder()		{ return pStandingPetOrder; } //Tazadar  - 5/31/08 we put thoses 2 functions to know if the pet still follow us , guard etc...
	damageInterval	GetRandomInterval(Mob* attacker, Mob* defender);
	int16			GetHitChance(Mob* attacker, Mob* defender, int skill_num);
	int16			GetItemHaste();
	int				GetKickDamage();
	int				GetBashDamage();
	int16			GetIntervalDamage(Mob* attacker, Mob* defender, damageInterval chosenDI, int32 max_hit, int32 min_hit);
	int16			GetMeleeReach();
	int16			GetBaseSize()		{ return base_size; }
	sint32			GetEquipmentMaterial(int8 material_slot);
	Item_Struct*	GetMyMainHandWeapon() { return myMainHandWeapon; }
	Item_Struct*	GetMyOffHandWeapon()  { return myOffHandWeapon; }
	int8			GetSlotOfItem(Item_Struct* item);
	float			GetDefaultSize();
	int16			GetSpellAttackSpeedModifier(Spell* spell, int8 casterLevel);
	int16			GetCharmLevelCap(Spell* spell);
	int16			GetMezLevelCap(Spell* spell);
	
	// Set
	void	setBoatSpeed(float speed);
	void	SetOwnerID(int16 NewOwnerID, bool despawn = true);
	void	SetPet(Mob* newpet);
	void	SetPetID(int16 NewPetID)				{ petid = NewPetID; }
	void	SetTarget(Mob* mob)						{ target = mob; }
	void	SetPetOrder(eStandingPetOrder i)		{ pStandingPetOrder = i; } //Tazadar  - 5/31/08 we put thoses 2 functions to know if the pet still follow us , guard etc...
	void	SetCurfp(bool change)					{ curfp = change; }
	void	SetCastingSpell(Spell* spell, int16 target_id = 0, int16 slot = 0, float casting_x = 0.0f, float casting_y = 0.0f, int16 inventorySlot=0);	 //Cofruben: more secure.
	void	DoManaRegen();
	void	DoEnduRegen();
	void	SetInvisible(bool toggle);
	void	SetInvisibleUndead(bool toggle);
	void	SetInvisibleAnimal(bool toggle);
	virtual sint32	SetHP(sint32 hp);
	virtual void	SetLevel(uint8 in_level)		{ level = in_level; }
	virtual sint32	SetMana(sint32 amount);
	virtual void	SetSkill(int in_skill_num, int8 in_skill_id) { skills[in_skill_num] = in_skill_id; }
	virtual void	SetBodyType(TBodyType bt)		{ this->body_type = bt; }	// Cofruben: 16/08/08.

	
	// Actions
	virtual void	GoToBind() {}
	virtual void	Attack(Mob* other, int Hand = 13, bool procEligible = true, bool riposte = false) {}		// 13 = Primary (default), 14 = secondary
	virtual void	Damage(Mob* from, sint32 damage, int16 spell_id, int8 attack_skill = 0x04)  {}
	virtual void	Heal()  {}
	virtual void	Death(Mob* killer, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04) {}
	virtual void	GMMove(float x, float y, float z, float heading = 0.01);
	virtual void	Message(MessageFormat Type, char* pMessage, ...);
	virtual void	CastSpell(Spell* spell, int16 target_id, int16 slot = 10, int32 casttime = 0xFFFFFFFF, int16 inventorySlot = 0);
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
	void			SpellFinished(Spell* spell, int32 target_id, int16 slot = SLOT_ITEMSPELL, int16 inventorySlot = 0 , bool weaponProc = false);
	void			SpellOnTarget(Spell* spell, Mob* spelltar);
	void			SpellOnGroup(Spell* spell,int32 target_id); // Pinedepain // Cast a spell on the caster's group
	void			SpellEffect(Mob* caster, Spell* spell, int8 caster_level, bool partialResist = false);
	void			DoBuffTic(Spell* spell, int32 ticsremaining, int8 caster_level, Mob* caster = 0);
	void			BuffFade(Spell* spell = NULL); // NULL will nuke all buffs
	void			BuffFadeBySlot(int slot, bool iRecalcBonuses);
	void			BuffFadeByEffect(TSpellEffect effect);
	void			BuffFadeAll()					{ this->BuffFade(NULL); }
	int				AddBuff(Mob *caster, Spell* spell, int duration = 0);
	int				CheckStackConflict(Spell* spell1, int caster_level1, Spell* spell2, int caster_level2, Mob* caster1 = NULL, Mob* caster2 = NULL);
	int				CalcSpellEffectValue(Spell* spell, int effect_id, int caster_level = 1, Mob *caster = NULL, int ticsremaining = 0);
	int				CalcSpellEffectValue_formula(int formula, int base, int max, int caster_level, Spell* spell, int ticsremaining = 0);
	int				GetMinLevel(Spell* spell);
	void			Spin(Mob* caster, Spell* spell);
	void			Kill();
	void			SaveGuardSpot();
	void			StopSong(void); // Pinedepain // Stop the song (Server side only!), it resets song vars
	void			DoBardSongAnim(int16 spell_id); // Pinedepain // Send the right bard anim to the client
	void			EnableSpellBar(int16 spell_id = 0); // Pinedepain // Send a packet to this mod to enable his spell bar.
	void			EnableSpellBarWithManaConsumption(Spell* spell);
	void			Mesmerize(void); // Pinedepain // Mesmerize this mob
	void			ApplySpellsBonuses(Spell* spell, int8 casterlevel, StatsStruct* newbon, int8 bardMod = 0, int16 caster_id = 0, bool isOnItem = false);
	void			CalcBonuses(bool refresHaste = false, bool refreshItemBonuses = false);
	void			DoHPRegen(int32 expected_new_hp = 0);
	void			SendHPUpdate();
	void			DoSpecialAttackDamage(Mob *who, int16 skill, sint32 max_damage, sint32 min_damage = 0);

	// Can
	int16	CanUse(int16 spellid, int16 classa, int16 level);

	// Check
	void		CheckPet();
	inline bool	CheckAggro(Mob* other) {return hate_list.IsOnHateList(other);}
	bool		CheckLosFN(Mob* other);
	bool		CheckFizzle(Spell* spell);
	int16		CheckMaxSkill(int skillid, int8 race, int8 eqclass, int8 level, bool gmRequest = false);

	// Make
	void	MakeSpawnUpdate(SpawnPositionUpdate_Struct* spu);
	void	MakePet(const char* pettype);
	void	MakePet(int8 in_level, int8 in_class, int16 in_race, int8 in_texture = 0, int8 in_pettype = 0, float in_size = 0, int8 type = 0,int32 min_dmg = 0,int32 max_dmg = 0,sint32  petmax_hp=0);
	void	MakeEyeOfZomm(Mob* caster);

	// Show
	void	ShowStats(Client* client);
	void	ShowBuffs(Client* client);

	// Create
	void	CreateDespawnPacket(APPLAYER* app);
    void	CreateSpawnPacket(APPLAYER* app, Mob* ForWho = 0);
	void	CreateHPPacket(APPLAYER* app);

	// Find
	int16			FindSpell(int16 classp, int16 level, int8 type, int8 spelltype = 0); // 0=buff, 1=offensive, 2=helpful
	virtual void	FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho);
	bool			FindBuff(Spell* spell);
	bool			FindType(int8 type);
	float			FindGroundZ(float new_x, float new_y, float z_offset = 0.0);
	float			FindGroundZWithZ(float new_x, float new_y, float new_z, float z_offset = 0.0);
	Node*			findClosestNode(Mob* mob, bool parse = false);
	PATH_STATUS		findMyPath(Node* start, Node* end, bool doNotMove = false);
	bool			findMyPreprocessedPath(Node* start, Node* end, bool parser = false);


	// Other
	virtual sint32	CalcMaxHP()		{ return max_hp = (base_hp  + itembonuses.HP + spellbonuses.HP); }
	bool			HasOwner()		{ return(GetOwnerID() != 0); } //Tazadar 06/01/08 we need this in order to use pet dmg 
	float			Dist(Mob*);
	float			DistNoZ(Mob* other);
	float			DistNoRoot(Mob*);
	float			DistNoRootNoZ(Mob*); 
	float			DistanceToLocation(float x, float y, float z);
	float			DistanceToLocation(float x, float y);
	void			SendIllusionPacket(int16 in_race, int16 in_gender = 0xFF, int16 in_texture = 0xFFFF, int16 in_helmtexture = 0xFFFF);
	void			SendAppearancePacket(int16 Spawn_ID, TSpawnAppearanceType Type, int32 Parameter, bool WholeZone);
	void			faceDestination(float x, float y);
	void			CalculateNewFearpoint();
	sint32			CalcMaxMana();
	void			SendWearChange(int8 material_slot = 250);

	vector<float>* quest_item_inventory;	//Keeps track of what npc has on hand for quests.

	void	SetX(float x) {x_pos = x;}
	void	SetY(float y) {y_pos = y;}
	void	SetZ(float z) {z_pos = z;}
	void	SetDeltaX(sint16 dx) {delta_x = dx;}
	void	SetDeltaY(sint16 dy) {delta_x = dy;}
	void	SetDeltaZ(sint16 dz) {delta_x = dz;}
	void	SetHeading(int8 h) {heading = h;}
	void	SetVelocity(sint8 a) {animation = a;}

	Buffs_Struct	buffs[BUFF_COUNT];

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

	Spell*	GetBonusProcSpell() {return bonusProcSpell;}
	void	SetBonusProcSpell(Spell* spell) {bonusProcSpell = spell;}

	sint16	GetSpellHate(TSpellEffect effect_id, int spell_level, bool resisted, sint32 amount = 0);
	sint16	GetSpellHate(Spell* spell, bool resisted);

	bool	GetInvisibleUndead() { return invisible_undead; }
	bool	GetInvisibleAnimal() { return invisible_animal; }
	bool	GetSpellInvis() { return invisible; }
	bool	GetInvisible();
	bool	GetCanSeeThroughInvis() { return canSeeThroughInvis; }
	bool	GetCanSeeThroughInvisToUndead() { return canSeeThroughInvisToUndead; }
	void	SetCanSeeThroughInvis(bool in) { canSeeThroughInvis = in; }
	void	SetCanSeeThroughInvisToUndead(bool in) { canSeeThroughInvisToUndead = in; }
	void	CancelAllInvisibility();

	void	SpawnProjectile(Mob* inflictor, Mob* inflictee, Spell* spell);
	void	SpawnProjectile(Mob* inflictor, Mob* inflictee, Item_Struct* item);

	void	HandleResistedSpell(Spell* spell, Mob* caster, Mob* target, ResistMessage type = RM_None, bool immunityMessage = false);
	void	CreateStartingCastingPacket(Spell* spell, Mob* caster, Mob* target);
	void	CreateEndingCastingPacket(Spell* spell, Mob* caster, Mob* target);
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


/////////////////////////////////////////////////////////////////////////////////////////////////////
protected:
	int32	GetMonkHandToHandDelay(void);
	int32	GetWeaponDamageBonus(Item_Struct *Weapon);
	int32	GetMonkHandToHandDamage(void);
	int32	GetWeaponDamage(int8 Skill, const Item_Struct* Weapon, Mob* Target);
	void	TryWeaponProc(const Item_Struct* weapon_g, Mob *on, int hand);
	void	TryBonusProc(Spell* spell, Mob *on);
	void	DoAttackAnim(Item_Struct* AttackerWeapon, int8& SkillInUse, int8& AttackSkill, int Hand);
	void	CalcSpellBonuses(StatsStruct* newbon);

	eStandingPetOrder pStandingPetOrder; //Tazadar: the stand of the pet(following , siting , guarding)

	int32	pLastChange;

	char    name[30];
	char    Surname[20];

	sint32  cur_hp;
	sint32  max_hp;
	sint32	base_hp;
	sint32	cur_mana;
	sint32	max_mana;

	StatsStruct		itembonuses;
	StatsStruct		spellbonuses;
	NPCType*		NPCTypedata;
	int16			petid;
	int16			ownerid;
	int8			typeofpet; // 0xFF = charmed

	StatsStruct BaseStats; // DB - Code reuse people, lets use it!

	int8	gender;
	int16   race;
	int8	base_gender;
	int16	base_race;
	int8    class_;
	TBodyType	body_type;		// Cofruben 16/08/08.
	int8    deity;
	int8    level;
	int32   npctype_id; // rembrant, Dec. 20, 2001
	int16   skills[74]; // socket 12-29-01 //Yeahlight: This needs to be int16 to accomidate NPCs
	bool	invulnerable;
	bool	InWater;

	float	x_pos0;
	float	y_pos0;
	float	z_pos0; 

	float	x_dest;
	float	y_dest;
	float	z_dest;

	float	dx, dy, dz;


	int8	heading;
	float	size;
	float	walkspeed;
	float	runspeed;
//	sint8   delta_x;
//	sint8   delta_y;
//	sint8   delta_z;
	sint8   delta_heading;
	// maalanar 2008-02-05: removed the bit field to be more thread safe
    sint16	delta_x;
    sint16	delta_z;
    sint16	delta_y;
	int32   velocity;
	int32	guildeqid; // guild's EQ ID, 0-511, 0xFFFFFFFF = none

	int8    light;
	int8    equipment[9];
	int32	equipmentColor[7];
	int8	texture;
	int8	helmtexture;
	int		appearance;

	// maalanar 2008-02-05: this seems to be the animation velocity on the client side
	sint8	animation;

	Mob*	target;
	Mob*	temptarget;
	Timer*	attack_timer;
	Timer*	tic_timer;
	Timer*	hp_timer;
	Timer*	mana_timer;
	Timer*	endu_timer;
	Timer*	forget_timer; // Jester - Added for feign death

	// Kaiyodo - Timer added for dual wield
	Timer * attack_timer_dw;

	Timer*	spellend_timer;
	Timer*	clicky_lockout_timer;	  //Yeahlight: Used to lockout players from spamming clickies (helps prevent crash exploits and lag).
	Spell*	casting_spell;
	int16	casting_spell_targetid;
	int16	casting_spell_slot;
	int16	casting_spell_inventory_slot;
	float	casting_spell_x_location; //Yeahlight: Mobs will not be subjected to a location check
	float	casting_spell_y_location; //           but are declared here for consistancy.

	int8	haircolor;
	int8	beardcolor;
	int8	eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
	int8	eyecolor2;
	int8	hairstyle;
	int8	wode; //Face Overlay? (barbarian only)
	int8	luclinface; // and beard

	// Bind wound
	Timer*  bindwound_timer;
	Mob*    bindwound_target;

	HateList hate_list;

	bool	hitWhileCasting; //Yeahlight: Used to track potential spell interruptions from attacks
	void	SendManaChange(sint32 in_mana, Spell* spell = NULL);
	int16	hitBoxExtension; //Yeahlight: Bonus hitbox extension (to AND from attacker) based on mob's size and race

	bool	mesmerized; // Pinedepain // True if the mob is mezzed, otherwise false

	bool	curfp;
	bool	nodeReporter;

	Node*	mySpawnNode;

	sint16	baseAttackSpeedModifier;

	int16	myMitigationAC;
	int16	myAvoidanceAC;

	int16	base_size;

	Item_Struct* myMainHandWeapon;
	Item_Struct* myOffHandWeapon;

	bool	hasRuneOn;

	Spell*	bonusProcSpell;

	bool	invisible;
	bool	invisible_undead;
	bool	invisible_animal;

	float	x_pos;
	float	y_pos;
	float	z_pos;

	float	guard_x;
	float	guard_y;
	float	guard_z;
	float	guard_heading;

	float	fear_walkto_x;
    float	fear_walkto_y;
    float	fear_walkto_z;

	bool	canSeeThroughInvis;
	bool	canSeeThroughInvisToUndead;

	SPAWN_TIME_OF_DAY time_of_day; // Kibanu

	// Harakiri Quests
	bool	qglobal;
	// hp event
	int nexthpevent;
	int nextinchpevent;

private:
		
};

#endif

//