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
#ifndef MOB_H
#define MOB_H

/* solar: macros for IsAttackAllowed, also used by spells.cpp */
#define _CLIENT(x) (x && x->IsClient() && !x->CastToClient()->IsBecomeNPC())
#define _NPC(x) (x && x->IsNPC() && !x->CastToMob()->GetOwnerID())
#define _BECOMENPC(x) (x && x->IsClient() && x->CastToClient()->IsBecomeNPC())
#define _CLIENTPET(x) (x && x->CastToMob()->GetOwner() && x->CastToMob()->GetOwner()->IsClient())
#define _NPCPET(x) (x && x->IsNPC() && x->CastToMob()->GetOwner() && x->CastToMob()->GetOwner()->IsNPC())
#define _BECOMENPCPET(x) (x && x->CastToMob()->GetOwner() && x->CastToMob()->GetOwner()->IsClient() && x->CastToMob()->GetOwner()->CastToClient()->IsBecomeNPC())

#include "entity.h"
#include "spdat.h"
#include "event_codes.h"
#include "hate_list.h"
#include "../common/Kaiyodo-LList.h"
#include "skills.h"

enum FindSpellType { SPELLTYPE_SELF, SPELLTYPE_OFFENSIVE, SPELLTYPE_OTHER };

struct Buffs_Struct {
	int16	spellid;
	int8	casterlevel;
	int16	casterid;		// Maybe change this to a pointer sometime, but gotta make sure it's 0'd when it no longer points to anything
	int8	durationformula;
	sint32	ticsremaining;
};

struct wplist {
	int   index;
	float x;
	float y;
	float z;
	int	  pause;
};


struct StatBonuses {
	sint16	AC;
	sint32	HP;
	sint32	HPRegen;
	sint16	Mana;
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
	int8	DamageShieldType;
    sint16  movementspeed;
    sint16  haste;
	float ArrgoRange; // when calculate just replace original value with this
	float AssistRange;
    int8 skillmod[76];
};

typedef struct
{
    int16 spellID;
    int8 chance;
    Timer *pTimer;
} tProc;

#define MAX_AISPELLS 16
class Mob : public Entity
{
public:
	enum eStandingPetOrder { SPO_Follow, SPO_Sit, SPO_Guard };
	struct AISpells_Struct {
		int16	type;			// 0 = never, must be one (and only one) of the defined values
		sint16	spellid;		// <= 0 = no spell
		sint16	manacost;		// -1 = use spdat, -2 = no cast time
		int32	time_cancast;	// when we can cast this spell next
		sint32	recast_delay;
		sint16	priority;
	};

	static int32	RandomTimer(int min, int max);
	static int8		GetDefaultGender(int16 in_race, int8 in_gender = 0xFF);
	static sint32	CalcPetHp(int8 levelb, int8 classb, int8 STA = 75);
	static void		CreateSpawnPacket(APPLAYER* app, NewSpawn_Struct* ns);
	static bool		IsBardSong(int16);
	static sint32	CalcSpellValue(int8 formula, sint16 base, sint16 max, int8 caster_level, int16 spell_id = 0xFFFF);
	static sint8	CheckEffectIDMatch(int8 effectindex, int16 spellid1, int8 caster_level1, int16 spellid2, int8 caster_level2);
	static int32	GetAppearanceValue(int8 iAppearance);
	//static int8		MaxSkill(int16 skillid, int16 class_, int16 level);
	int8 MaxSkill(int16 skillid, int16 class_, int16 level);
    inline int8 MaxSkill(int16 skillid) { return MaxSkill(skillid, GetClass(), GetLevel()); }
    // Util functions for MaxSkill
    int8 MaxSkill_weapon(int16 skillid, int16 class_, int16 level);
    int8 MaxSkill_offensive(int16 skillid, int16 class_, int16 level);
    int8 MaxSkill_defensive(int16 skillid, int16 class_, int16 level);
    int8 MaxSkill_arcane(int16 skillid, int16 class_, int16 level);
    int8 MaxSkill_class(int16 skillid, int16 class_, int16 level);
	
	void RogueBackstab(Mob* other, const Item_Struct *weapon1, int8 bs_skill);
	void RogueAssassinate(Mob* other); // solar
	bool BehindMob(Mob* other = 0, float playerx = 0.0f, float playery = 0.0f);

	Mob(const char*   in_name,
	    const char*   in_lastname,
	    sint32  in_cur_hp,
	    sint32  in_max_hp,
	    int8    in_gender,
	    uint16    in_race,
	    int8    in_class,
        int8    in_bodytype,
	    int8    in_deity,
	    int8    in_level,
		int32   in_npctype_id, // rembrant, Dec. 20, 2001
		const int8*	in_skills, // socket 12-29-01
		float	in_size,
		float	in_walkspeed,
		float	in_runspeed,
	    float   in_heading,
	    float	in_x_pos,
	    float	in_y_pos,
	    float	in_z_pos,

	    int8    in_light,
	    const int8*   in_equipment,
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
		int8	in_haircolor,
		int8	in_beardcolor,
		int8	in_eyecolor1, // the eyecolors always seem to be the same, maybe left and right eye?
		int8	in_eyecolor2,
		int8	in_hairstyle,
		int8	in_title, //Face Overlay? (barbarian only)
		int8	in_luclinface, // and beard);
		float	in_fixedZ,
		int16	in_d_meele_texture1,
		int16	in_d_meele_texture2
	);
	virtual ~Mob();

	inline virtual bool IsMob() { return true; }
	inline virtual bool InZone() { return true; }
	MyList <wplist> Waypoints;
	void	TicProcess();
	virtual void SetLevel(uint8 in_level, bool command = false) { level = in_level; }

	virtual inline sint32 GetPrimaryFaction() { return 0; }
	inline virtual void SetSkill(int16 in_skill_num, int8 in_skill_id) { // socket 12-29-01
		if (in_skill_num <= HIGHEST_SKILL) { skills[in_skill_num + 1] = in_skill_id; } }
	inline virtual int8 GetSkill(int16 skill_num) { if (skill_num <= HIGHEST_SKILL) { return skills[skill_num + 1]; } return 0; } // socket 12-29-01
	inline int8 GetEquipment(int16 item_num) { return equipment[item_num]; } // socket 12-30-01

	virtual void GoToBind() = 0;
	virtual bool Attack(Mob* other, int Hand = 13, bool FromRiposte = false) = 0;		// 13 = Primary (default), 14 = secondary
	virtual void Damage(Mob* from, sint32 damage, int16 spell_id, int8 attack_skill = 0x04, bool avoidable = true, sint8 buffslot = -1, bool iBuffTic = false) {};
	virtual void Heal();
	virtual void SetMaxHP() { cur_hp = max_hp; }
	virtual void Death(Mob* killer, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04) = 0;
	int32	GetLevelCon(int8 iOtherLevel);
	
	inline virtual void SetHP(sint32 hp) { if (hp >= max_hp) cur_hp = max_hp; else cur_hp = hp;} 
	int16   equipment[9];
	bool ChangeHP(Mob* other, sint32 amount, int16 spell_id = 0, sint8 buffslot = -1, bool iBuffTic = false);
	void MonkSpecialAttack(Mob* other, int8 type);
	void DoAnim(const int animnum, bool ackreq = true);
	bool IsLifetapSpell(int16 spell_id);

	void ChangeSize(float in_size, bool bNoRestriction = false);
	virtual void GMMove(float x, float y, float z, float heading = 0.01);
	void SendPosUpdate(int8 iSendToSelf = 0);
	void MakeSpawnUpdate(SpawnPositionUpdate_Struct* spu);
	
	void CreateDespawnPacket(APPLAYER* app);
	void CreateHorseSpawnPacket(APPLAYER* app, const char* ownername, uint16 ownerid, Mob* ForWho = 0);
	void CreateSpawnPacket(APPLAYER* app, Mob* ForWho = 0);
	virtual void FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho);
	void CreateHPPacket(APPLAYER* app);
    
	int16 CalcBuffDuration(int8 level, int16 formula, int16 duration);

    enum { SPECATK_NONE = 0, SPECATK_SUMMON = 1, SPECATK_ENRAGE = 2, SPECATK_RAMPAGE = 3, SPECATK_FLURRY = 4,
           SPECATK_TRIPLE = 5, SPECATK_QUAD = 6,
           SPECATK_MAXNUM = 7 /*need maxnum to be 1 greater than last special attack */ };

	bool AddProcToWeapon(int16 spell_id, bool bPerma = false, int8 iChance = 3);
	bool RemoveProcFromWeapon(int16 spell_id, bool bAll = false);
	bool HasProcs();

	bool SeeInvisible();
	bool SeeInvisibleUndead();

	bool IsInvisible(Mob* other = 0) {
       if (other && other->SeeInvisible() && !sneaking) { return false; }
	   if (sneaking && BehindMob(other, GetX(), GetY()) ){ return true; }
       else { return invisible; }
    }

	bool AttackAnimation(int &attack_skill, int16 &skillinuse, int Hand, const Item_Struct* weapon);
	bool AvoidDamage(Mob* other, sint32 &damage, int16 spell_id, int8 attack_skill, int Hand = 0);

	void			DamageShield(Mob* other);
	bool			FindBuff(int16 spellid);
	bool FindType(int8 type, bool bOffensive = false, int16 threshold = 100);
	bool			IsEffectInSpell(sint16 spellid, int8 type);
	sint8           GetBuffSlotFromType(int8 type);

	int16			CalcPetLevel(int16 nlevel, int16 nclass);
	void			MakePet(const char* pettype);
	void			MakePet(int8 in_level, int8 in_class, int16 in_race, int8 in_texture = 0, int8 in_pettype = 0, float in_size = 0, int8 type = 0);
	char*		GetRandPetName();

	bool			CombatRange(Mob* other);
	int8			flag[60];

	virtual inline int16	GetBaseRace()		{ return base_race; }
	virtual inline int8	GetBaseGender()		{ return base_gender; }
	virtual inline int8	GetDeity()			{ return deity; }
	inline const int16&	GetRace()			{ return race; }
	inline const int8&	GetGender()			{ return gender; }
	inline const int8&	GetTexture()		{ return texture; }
	inline const int8&	GetHelmTexture()	{ return helmtexture; }
	inline const int8&	GetClass()			{ return class_; }
	inline const int8&	GetLevel()			{ return level; }
	inline const char*	GetName()			{ return name; }
	inline Mob*			GetTarget()			{ return target; }
	virtual inline void	SetTarget(Mob* mob)	{ target = mob; }
	inline float		GetHPRatio()		{ return max_hp == 0 ? 0 : ((float)cur_hp/max_hp*100); }
	
	bool IsWarriorClass();
	bool IsAttackAllowed(Mob *target);
	
	virtual inline const sint32&	GetHP()			{ return cur_hp; }
	virtual inline const sint32&	GetMaxHP()		{ return max_hp; }
	virtual inline sint32			CalcMaxHP()		{ return max_hp = (base_hp  + itembonuses->HP + spellbonuses->HP); }
	// need those cause SoW or Snare didnt work for mobs
	virtual float GetWalkspeed();
	virtual float GetRunspeed();
	void ApplySpellsBonuses(int16 spell_id, int8 casterlevel, StatBonuses* newbon);


	virtual inline const sint32&	SetMana(sint32 amount)  { if (amount < 0)  cur_mana = 0; else if (amount > GetMaxMana()) cur_mana = GetMaxMana();else cur_mana = amount; return cur_mana; }
	virtual inline const sint32&	GetMaxMana()	{ return max_mana; }
	virtual inline const sint32&	GetMana()		{ return cur_mana; }
	virtual inline float			GetManaRatio()	{ return max_mana == 0 ? 100 : (((float)cur_mana/max_mana)*100); }
	void			SetZone(int32 zone_id);
	
	// neotokyo: moved from client to use in NPC too
	char GetCasterClass();
	virtual sint32 CalcMaxMana();

	inline virtual int16	GetAC()		{ return AC + itembonuses->AC + spellbonuses->AC; } // Quagmire - this is NOT the right math
	inline virtual int16	GetATK()	{ return ATK + itembonuses->ATK + spellbonuses->ATK; }
	inline virtual sint16	GetSTR()	{ return STR + itembonuses->STR + spellbonuses->STR; }
	inline virtual sint16	GetSTA()	{ return STA + itembonuses->STA + spellbonuses->STA; }
	inline virtual sint16	GetDEX()	{ return DEX + itembonuses->DEX + spellbonuses->DEX; }
	inline virtual sint16	GetAGI()	{ return AGI + itembonuses->AGI + spellbonuses->AGI; }
	inline virtual sint16	GetINT()	{ return INT + itembonuses->INT + spellbonuses->INT; }
	inline virtual sint16	GetWIS()	{ return WIS + itembonuses->WIS + spellbonuses->WIS; }
	inline virtual sint16	GetCHA()	{ return CHA + itembonuses->CHA + spellbonuses->CHA; }
	virtual sint16	GetMR()	= 0;
	virtual sint16	GetFR()	= 0;
	virtual sint16	GetDR()	= 0;
	virtual sint16	GetPR()	= 0;
	virtual sint16	GetCR()	= 0;

	inline virtual sint16  GetMaxSTR() { return GetSTR(); }
	inline virtual sint16  GetMaxSTA() { return GetSTA(); }
	inline virtual sint16  GetMaxDEX() { return GetDEX(); }
	inline virtual sint16  GetMaxAGI() { return GetAGI(); }
	inline virtual sint16  GetMaxINT() { return GetINT(); }
	inline virtual sint16  GetMaxWIS() { return GetWIS(); }
	inline virtual sint16  GetMaxCHA() { return GetCHA(); }

	virtual sint32 GetActSpellRange(int16 spell_id, sint32 range){ return range;}
	virtual sint32 GetActSpellValue(int16 spell_id, sint32 value){ return value;}
	virtual sint32 GetActSpellCost(int16 spell_id, sint32 cost){ return cost;}
	virtual sint32 GetActSpellDuration(int16 spell_id, sint32 duration){ return duration;}
	virtual sint32 GetActSpellCasttime(int16 spell_id, sint32 casttime){ return casttime;}
	sint16 ResistSpell(int16 spell_id, Mob* caster);

	void ShowStats(Client* client);
	void ShowBuffs(Client* client);
	int32 GetNPCTypeID()			{ return npctype_id; } // rembrant, Dec. 20, 2001
	const NPCType* GetNPCTypeData()	{ return NPCTypedata; }
	inline const int32& GetNPCSpellsID()	{ return npc_spells_id; }

	//Trumpcard:  Inlined these calculations.
	float Dist(Mob*);
	float DistNoZ(Mob* other);
	float DistNoRoot(Mob*);
	float DistNoRootNoZ(Mob*);

	inline const float&	GetX()				{ return x_pos; }
	inline const float&	GetY()				{ return y_pos; }
	inline const float&	GetZ()				{ return z_pos; }
	inline const float&	GetHeading()		{ return heading; }
	inline const float&	GetSize()			{ return size; }
	inline void			SetChanged()		{ pLastChange = Timer::GetCurrentTime(); }
	inline const int32&	LastChange()		{ return pLastChange; }

	void	SetFollowID(int32 id) { follow = id; }
	int32	GetFollowID()		  { return follow; }

	virtual void Message(int32 type, const char* message, ...) {} // fake so dont have to worry about typecasting

	void SpellProcess();
	bool CheckFizzle(int16 spell_id);
	void InterruptSpell(int16 spellid = 0xFFFF);
	void InterruptSpell(int16, int16, int16 spellid = 0xFFFF);
	virtual void	CastSpell(int16 spell_id, int16 target_id, int16 slot = 10, sint32 casttime = -1, sint32 mana_cost = -1, int32* oSpellWillFinish = 0);
	void	CastedSpellFinished(int16 spell_id, int32 target_id, int16 slot, int16 mana_used);
	void	SpellFinished(int16 spell_id, int32 target_id, int16 slot = 10, int16 mana_used = 0);
	void	SpellOnTarget(int16 spell_id, Mob* spelltar);
	sint8	CheckAddBuff(Mob* caster, const int16& spell_id, const int8& caster_level, int16* buffdur, sint16 ticsremaining = -1);
	bool	SpellEffect(Mob* caster, int16 spell_id, int8 caster_level, sint8 buffslot = -1, int16 buffdur = 0, int16 partial = 100);
	void	DoBuffTic(int16 spell_id, int32 ticsremaining, int8 caster_level, Mob* caster = 0);
	void	BuffFade(int16 spell_id);
	void	BuffFadeByEffect(int8 iEffectID, sint8 iButNotThisSlot = -1, sint8 iBardSong = -1, bool iRecalcBonuses = true);
	void	BuffFadeBySlot(sint8 slot, bool iRecalcBonuses = true);
	void	BuffFadeByStackCommand(int16 spellid, sint8 iButNotThisSlot);
	sint8	CanBuffStack(int16 spellid, int8 caster_level, bool iFailIfOverwrite = false);
	inline bool	IsCasting() { return (bool) (casting_spell_id != 0); }

	void	SendIllusionPacket(int16 in_race, int8 in_gender = 0xFF, int16 in_texture = 0xFFFF, int16 in_helmtexture = 0xFFFF, int8 in_haircolor = 0xFF, int8 in_beardcolor = 0xFF, int8 in_eyecolor1 = 0xFF, int8 in_eyecolor2 = 0xFF, int8 in_hairstyle = 0xFF, int8 in_title = 0xFF, int8 in_luclinface = 0xFF);
	void	SendAppearancePacket(int32 type, int32 value, bool WholeZone = true, bool iIgnoreSelf = false);
	void	SetAppearance(int8 app, bool iIgnoreSelf = true);
	inline const int8&	GetAppearance()				{ return appearance; }
	inline const sint8&	GetRunAnimSpeed()			{ return pRunAnimSpeed; }
	inline void			SetRunAnimSpeed(sint8 in)	{ if (pRunAnimSpeed != in) { pRunAnimSpeed = in; pLastChange = Timer::GetCurrentTime(); } }
	Mob*	GetPet();
	Mob*	GetFamiliar();
	void	SetPet(Mob* newpet);
	Mob*	GetOwner();
	Mob*	GetOwnerOrSelf();
					void	SetPetID(int16 NewPetID);
	inline const	int16&	GetPetID()						{ return petid;  }
					void	SetFamiliarID(int16 NewPetID);
	inline const	int16&	GetFamiliarID()					{ return familiarid;  }
					void	SetOwnerID(int16 NewOwnerID);
	inline const	int16&	GetOwnerID()					{ return ownerid; }
	inline const	int16&	GetPetType()					{ return typeofpet; }
    inline const	int8&	GetBodyType()					{ return bodytype; }
    int16   FindSpell(int16 classp, int16 level, int type, FindSpellType spelltype, float distance, sint32 mana_avail);
	int16	CanUse(int16 spellid, int16 classa, int16 level);
	void	CheckBuffs();
	bool	CheckSelfBuffs();
	void	CheckPet();

 	void    SendSpellBarDisable(bool);

	bool	invulnerable;
	bool	invisible, invisible_undead, sneaking;
	void	Spin();
	void	Kill();

	void	SetAttackTimer();
	inline void	SetHaste(int Haste) { HastePercentage = Haste; }
	inline int		GetHaste(void) { return HastePercentage + spellbonuses->haste; }
	inline void	SetItemHaste(int Haste) { ItemHastePercentage = Haste; }
	inline int		GetItemHaste(void) { return itembonuses->haste; }
	// Kaiyodo - new function prototypes for damage system
	int		GetWeaponDamageBonus(const Item_Struct *Weapon);
	int		GetMonkHandToHandDamage(void);
	

	bool	CanThisClassDoubleAttack(void);
	bool	CanThisClassDuelWield(void);
	bool	CanThisClassRiposte(void);
	bool	CanThisClassDodge(void);
	bool	CanThisClassParry(void);
	

	int	GetMonkHandToHandDelay(void);
	int8	GetClassLevelFactor();
	void	Mesmerize();
	bool    IsMezzable(int16 spell_id);
	inline bool	IsMezzed()	{ return mezzed;}
	inline bool	IsStunned() { return stunned; }
	inline int16	GetErrorNumber()	{return adverrorinfo;}
	
	inline int16	GetRune() { return rune; }
	inline void	SetRune(int16 in_rune) { rune = in_rune; }
	
	sint16	ReduceDamage(sint16 damage, int16 in_rune);
	sint16  ReduceMagicalDamage(sint16 damage, int16 in_rune);

   	inline const int16& GetMagicRune() { return magicrune; }
	void	SetMagicRune(int16 in_rune) { magicrune = in_rune; }

    bool SpecAttacks[SPECATK_MAXNUM];
#define MAX_RAMPAGE_TARGETS 3
#define MAX_FLURRY_HITS 2
    char RampageArray[MAX_RAMPAGE_TARGETS][64];
    bool Flurry();
    bool Rampage();
    bool AddRampage(Mob*);

    void StartEnrage();
    bool IsEnraged();

	virtual void		AI_Init();
	virtual void		AI_Start(int32 iMoveDelay = 0);
	virtual void		AI_Stop();
	void				AI_Process();
	void				AI_Event_Engaged(Mob* attacker, bool iYellForHelp = true);
	void				AI_Event_NoLongerEngaged();
	void				AI_Event_SpellCastFinished(bool iCastSucceeded, int8 slot);
	bool				AI_AddNPCSpells(int32 iDBSpellsID);
	void				AI_SetRoambox(float iDist, float iRoamDist, int32 iDelay = 2500);
	void				AI_SetRoambox(float iDist, float iMaxX, float iMinX, float iMaxY, float iMinY, int32 iDelay = 2500);
	virtual FACTION_VALUE GetFactionCon(Mob* iOther) { return FACTION_INDIFFERENT; }
	FACTION_VALUE		GetSpecialFactionCon(Mob* iOther);
	inline const bool&	IsAIControlled() { return pAIControlled; }
    inline const float&	GetGuardX() { return guard_x; }
    inline const float&	GetGuardY() { return guard_y; }
    inline const float&	GetGuardZ() { return guard_z; }

	void	SetGuardXYZ(float x, float y, float z) { guard_x = x; guard_y = y; guard_z = z; }

	inline const float&	GetGuardHeading() { return guard_heading; }
    inline const float&	GetSpawnX() { return spawn_x; }
    inline const float&	GetSpawnY() { return spawn_y; }
    inline const float&	GetSpawnZ() { return spawn_z; }
    inline const float&	GetSpawnHeading() { return spawn_heading; }
	inline const float& GetArrgoRange() { return (spellbonuses->ArrgoRange == -1) ? pArrgoRange : spellbonuses->ArrgoRange; }
	inline const float& GetAssistRange() { return (spellbonuses->AssistRange == -1) ? pAssistRange : spellbonuses->AssistRange; }
    void				SaveGuardSpot(bool iClearGuardSpot = false);
    void				SaveSpawnSpot();

	void				UpdateWaypoint(int wp_index);
	bool				AICastSpell(Mob* tar, int8 iChance, int16 iSpellTypes);
	void				AIDoSpellCast(int8 i, Mob* tar, sint32 mana_cost, int32* oDontDoAgainBefore = 0);
	inline void			SetPetOrder(eStandingPetOrder i) { pStandingPetOrder = i; }
	inline const eStandingPetOrder& GetPetOrder() { return pStandingPetOrder; }
	inline const bool&	IsRoamer() { return roamer; }
	inline const bool&	IsRooted() { return rooted; }
	bool				RemoveFromHateList(Mob* mob);
    void				AddToHateList(Mob* other, sint32 hate = 0, sint32 damage = 0, bool iYellForHelp = true, bool bFrenzy = false, bool iBuffTic = false);
	void				SetHate(Mob* other, sint32 hate = 0, sint32 damage = 0) {hate_list.Set(other,hate,damage);}
	int32				GetHateAmount(Mob* tmob, bool is_dam = false)  {return hate_list.GetEntHate(tmob,is_dam);}
	int32				GetDamageAmount(Mob* tmob)  {return hate_list.GetEntHate(tmob, true);}
	Mob*				GetHateTop()  {return hate_list.GetTop();}
	Mob*				GetHateDamageTop(Mob* other)  {return hate_list.GetDamageTop(other);}
	Mob*				GetHateRandom()  {return hate_list.GetRandom();}
	bool				IsEngaged()   {return (hate_list.GetTop() == 0) ? false:true; }
	bool				HateSummon();
	void				FaceTarget(Mob* MobToFace = 0, bool update = false);
	void				SetHeading(float iHeading) { if (heading != iHeading) { pLastChange = Timer::GetCurrentTime(); heading = iHeading; } }
	void				WhipeHateList();

	int					GetMaxWp(){ return max_wp; }
	int					GetCurWp(){ return cur_wp; }

	inline bool			CheckAggro(Mob* other) {return hate_list.IsOnHateList(other);}
    sint8				CalculateHeadingToTarget(float in_x, float in_y);
    bool				CalculateNewPosition(float x, float y, float z, float speed);
    float				CalculateDistance(float x, float y, float z);
	void				CalculateNewWaypoint();
	int8				CalculateHeadingToNextWaypoint();
	float				CalculateDistanceToNextWaypoint();
	void				AssignWaypoints(int16 grid);
	void				NPCSpecialAttacks(const char* parse, int permtag);
	void				SendTo(float new_x, float new_y, float new_z);
	inline int32&		DontHealMeBefore() { return pDontHealMeBefore; }
	inline int32&		DontBuffMeBefore() { return pDontBuffMeBefore; }
	inline int32&		DontDotMeBefore() { return pDontDotMeBefore; }
	inline int32&		DontRootMeBefore() { return pDontRootMeBefore; }
	inline int32&		DontSnareMeBefore() { return pDontSnareMeBefore; }

	// calculate interruption of spell via movement of mob
	void SaveSpellLoc() {spell_x = x_pos; spell_y = y_pos; spell_z = z_pos; }
	inline float GetSpellX() {return spell_x;}
	inline float GetSpellY() {return spell_y;}
	inline float GetSpellZ() {return spell_z;}
	sint32 GetSpellHPRegen() { return spellbonuses->HPRegen; }
	inline bool	IsGrouped()	{ return isgrouped; }

	bool	isgrouped;
	bool	pendinggroup;
	int16	d_meele_texture1;
	int16	d_meele_texture2;
	int16	AC;
	int16	ATK;
	int8	STR;

	int8	STA;
	int8	DEX;
	int8	AGI;
	int8	INT;
	int8	WIS;
	int8	CHA;
//	float	wp_x[50]; //X of waypoint
//	float	wp_y[50]; //Y of waypoint
//	float	wp_z[50]; //Z of waypoint
//	int32	wp_s[50]; //Pause of waypoint
//	int16	wp_a[6]; //0 = Amount of waypoints, 1 = Wandering Type, 2 = Pause Type, 3 = Current Waypoint, 4 = Grid Number, 5 = Used for patrol grids
	int8	texture;
	int8	helmtexture;
protected:
	sint32  cur_hp;
	sint32  max_hp;
	sint32	base_hp;
	sint32	cur_mana;
	sint32	max_mana;
	sint32	hp_regen;
	sint32	mana_regen;
	Buffs_Struct	buffs[15];
	StatBonuses*	itembonuses;
	StatBonuses*	spellbonuses;
	NPCType*		NPCTypedata;
	int16			petid;
    int16           familiarid;
	int16			ownerid;
	int16			typeofpet; // 0xFF = charmed

	int32			follow;

	int8    gender;
	int16	race;
	int8	base_gender;
	int16	base_race;
	int8    class_;
    int8    bodytype;
	int16	deity;
	int8    level;
	int32   npctype_id; // rembrant, Dec. 20, 2001
	int8    skills[75];
	float	x_pos;
	float	y_pos;
	float	z_pos;
	float	heading;
	float	size;
	float	walkspeed;
	float	runspeed;
	int32 pLastChange;
	void CalcSpellBonuses(StatBonuses* newbon);
	void CalcBonuses();

    enum {MAX_PROCS = 4};
    tProc PermaProcs[MAX_PROCS];
    tProc SpellProcs[MAX_PROCS];

	char    name[64];
	char    lastname[70];

    bool bEnraged;
    Timer *SpecAttackTimers[SPECATK_MAXNUM];


//	sint8   delta_x;
//	sint8   delta_y;
//	sint8   delta_z;
	sint8   delta_heading;
    sint32 delta_y:10,
           spacer1:1,
           delta_z:10,
           spacer2:1,
           delta_x:10;
	int32	guildeqid; // guild's EQ ID, 0-511, 0xFFFFFFFF = none

	int8    light;

	float	fixedZ;
	int8    appearance; // 0 standing, 1 sitting, 2 ducking, 3 lieing down, 4 looting
	sint8	pRunAnimSpeed;

	Mob*	target;
	Timer*	attack_timer;
	Timer*	tic_timer;
	Timer*	mana_timer;

	// Kaiyodo - Timer added for dual wield
	Timer * attack_timer_dw;

	Timer*	spellend_timer;
	int16	casting_spell_id;
	int8	casting_spell_AIindex;

    float spell_x, spell_y, spell_z;

	bool	isinterrupted;
	bool	isattacked;
	bool	delaytimer;
	int16 casting_spell_targetid;
	int16 casting_spell_slot;
	int16 casting_spell_mana;
	int16	bardsong;
	int8	haircolor;
	int8	beardcolor;
	int8	eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
	int8	eyecolor2;
	int8	hairstyle;
	int8	title; //Face Overlay? (barbarian only)
	int8	luclinface; // and beard

	int16	rune;
	int16	magicrune;
	int HastePercentage;
	int ItemHastePercentage;
	bool	mezzed;
	bool	stunned;
	//bool	rooted;
	Timer* mezzed_timer;
	Timer*  stunned_timer;
	Timer*	bardsong_timer;
	int16	adverrorinfo;

	// MobAI stuff
	eStandingPetOrder pStandingPetOrder;
    float guard_x, guard_y, guard_z, guard_heading;
    float spawn_x, spawn_y, spawn_z, spawn_heading;
	int32	minLastFightingDelayMoving;
	int32	maxLastFightingDelayMoving;
	float	pArrgoRange;
	float	pAssistRange;
	Timer*	AIthink_timer;
	Timer*	AImovement_timer;
	Timer*	AIautocastspell_timer;
	Timer*	AIscanarea_timer;
	Timer*	AIwalking_timer;
	int32	pLastFightingDelayMoving;
	int32	npc_spells_id;
	AISpells_Struct	AIspells[MAX_AISPELLS]; // expected to be pre-sorted, best at low index
	HateList hate_list;

	bool	pAIControlled;
	bool	roamer;

	int		wandertype;
	int		pausetype;

	int		max_wp;
	int		cur_wp;

	int		cur_wp_x;
	int		cur_wp_y;
	int		cur_wp_z;
	int		cur_wp_pause;

	int		patrol;

	bool	rooted;
	int32	pDontHealMeBefore;
	int32	pDontBuffMeBefore;
	int32	pDontDotMeBefore;
	int32	pDontRootMeBefore;
	int32	pDontSnareMeBefore;
	int32*	pDontCastBefore_casting_spell;

	float roambox_max_x;
	float roambox_max_y;
	float roambox_min_x;
	float roambox_min_y;
	float roambox_distance;
	float roambox_movingto_x;

	float roambox_movingto_y;
	int32 roambox_delay;
	
	// Bind wound
	Timer*  bindwound_timer;
	Mob*    bindwound_target;
	
};

#endif

