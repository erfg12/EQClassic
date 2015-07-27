// Written by Cofruben.
#include "Spell.hpp"
#include "classes.h"


///////////////////////////////////////////////////
///////////////////Constructors////////////////////
///////////////////////////////////////////////////

Spell::Spell(){
	memset(&this->pSpell, 0, sizeof(SPDat_Spell_Struct));
}

///////////////////////////////////////////////////

void Spell::Set(SPDat_Spell_Struct* spell, TSpellID id) {
	memcpy(&this->pSpell, spell, sizeof(SPDat_Spell_Struct));
	this->pID = id;
}

///////////////////////////////////////////////////
///////////////////////////////////////////////////

int8 Spell::GetSpellSkill(){
	return (this->Get().skill); // Pinedepain // Return the skill used by the spell
}

///////////////////////////////////////////////////

int16 Spell::GetSpellManaRequired(){
	return (this->Get().mana);
}

//////////////////////////////////////////////////

int32 Spell::GetSpellCastTime(){
	return (this->Get().cast_time);
}

//////////////////////////////////////////////////

TSpellEffect Spell::GetSpellEffectID(int8 index) {
	if(index >= EFFECT_COUNT) index = 0;
	return this->pSpell.effectid[index];
}

//////////////////////////////////////////////////

void Spell::SetSpellEffectID(int8 index, TSpellEffect value)
{
	if(index >= EFFECT_COUNT || value > SE_Blank)
		return;
	this->pSpell.effectid[index] = value;
}

//////////////////////////////////////////////////

sint16 Spell::GetSpellBase(int8 index) {
	if(index >= EFFECT_COUNT) index = 0;
	return this->pSpell.base[index];
}

///////////////////////////////////////////////////

void Spell::SetSpellBase(int8 index, sint16 value)
{
	if(index >= EFFECT_COUNT)
		return;
	this->pSpell.base[index] = value;
}

///////////////////////////////////////////////////

sint16 Spell::GetSpellMax(int8 index) {
	if(index >= EFFECT_COUNT) index = 0;
	return this->pSpell.max[index];
}

///////////////////////////////////////////////////

int8 Spell::GetSpellFormula(int8 index) {
	if(index >= EFFECT_COUNT) index = 0;
	return this->pSpell.formula[index];
}

///////////////////////////////////////////////////

int8 Spell::GetSpellClass(int8 index) {
	if(index >= 15) index = 0;
	return this->pSpell.classes[index];
}

///////////////////////////////////////////////////

int16 Spell::GetSpellComponent(int8 index) {
	if (index > 3) return 0xFFFF;
	return this->pSpell.components[index];
}

///////////////////////////////////////////////////

//Yeahlight: Returns number of disease counters on the spell
int16 Spell::GetDiseaseCounters()
{
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		TSpellEffect tid = this->GetSpellEffectID(i);
		if(tid == SE_DiseaseCounter)
			return this->Get().base[i];
	}
	return 0;
}

///////////////////////////////////////////////////

//Yeahlight: Returns number of poison counters on the spell
int16 Spell::GetPoisonCounters()
{
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		TSpellEffect tid = this->GetSpellEffectID(i);
		if(tid == SE_PoisonCounter)
			return this->Get().base[i];
	}
	return 0;
}

///////////////////////////////////////////////////

//Yeahlight: Returns the amount of rune absorbtion on the spell
int16 Spell::GetRuneAmount()
{
	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		TSpellEffect tid = this->GetSpellEffectID(i);
		if(tid == SE_Rune)
			return this->Get().base[i];
	}
	return 0;
}

///////////////////////////////////////////////////

bool Spell::IsIllusionSpell() {
	return this->IsEffectInSpell(SE_Illusion);
}

///////////////////////////////////////////////////

bool Spell::IsAttackSpeedSpell()
{
	return this->IsEffectInSpell(SE_AttackSpeed);
}

///////////////////////////////////////////////////

bool Spell::IsCancelMagicSpell() {
	return this->IsEffectInSpell(SE_CancelMagic);
}

///////////////////////////////////////////////////

//Yeahlight: These are spells that are "beneficial" but should otherwise be considered detrimental by the target
//Yeahlight: TODO: There may be more to add
bool Spell::IsUtilitySpell() {
	if(IsEffectInSpell(SE_CancelMagic) || 
		IsEffectInSpell(SE_WipeHateList) || 
		IsEffectInSpell(SE_Lull) || 
		IsEffectInSpell(SE_AddFaction) || 
		IsEffectInSpell(SE_ChangeFrenzyRad) || 
		IsEffectInSpell(SE_Harmony) || 
		IsEffectInSpell(SE_Calm))
		return true;
	return false;
}

///////////////////////////////////////////////////

bool Spell::IsMemoryBlurSpell() {
	return this->IsEffectInSpell(SE_WipeHateList);
}

///////////////////////////////////////////////////

bool Spell::IsBuffSpell() {
	return (this->GetSpellBuffDurationFormula() > 0);
}

///////////////////////////////////////////////////

bool Spell::IsSacrificeSpell()
{
	return this->IsEffectInSpell(SE_Sacrifice);
}

///////////////////////////////////////////////////

bool Spell::IsSummonCorpseSpell() {
	return this->IsEffectInSpell(SE_SummonCorpse);
}

///////////////////////////////////////////////////

bool Spell::IsLifetapSpell()
{
	return
		(
		this->IsValidSpell() &&
		(
		this->Get().targettype == ST_Tap ||
		(
		this->GetSpellID() == 2115	// Ancient: Lifebane
		)
		)
		);
}

///////////////////////////////////////////////////

bool Spell::IsMezSpell()
{
	return this->IsEffectInSpell(SE_Mez);
}

///////////////////////////////////////////////////

bool Spell::IsStunSpell()
{
	return this->IsEffectInSpell(SE_Stun);
}

///////////////////////////////////////////////////

bool Spell::IsSummonSpell() {
	for (int o = 0; o < EFFECT_COUNT; o++)
	{
		TSpellEffect tid = this->GetSpellEffectID(o);
		if(tid == SE_SummonPet || tid == SE_SummonItem)
		{
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////

bool Spell::IsEvacSpell() {
	for (int o = 0; o < EFFECT_COUNT; o++)
	{
		TSpellEffect tid = this->GetSpellEffectID(o);
		if(tid == SE_DeathSave) {
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////

bool Spell::IsDamageSpell() {
	for (int o = 0; o < EFFECT_COUNT; o++)
	{
		TSpellEffect tid = this->GetSpellEffectID(o);
		if((tid == SE_CurrentHPOnce || tid == SE_CurrentHP) && this->GetSpellTargetType() != ST_Tap
			 && this->GetSpellBuffDuration() < 1 && this->GetSpellBase(o) < 0)
		{
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////

bool Spell::IsFearSpell() {
	return this->IsEffectInSpell(SE_Fear);
}

///////////////////////////////////////////////////

bool Spell::IsSlowSpell()
{
	int i;
	const SPDat_Spell_Struct &sp = this->Get();

	for(i = 0; i < EFFECT_COUNT; i++)
	{
		if
			(
			sp.effectid[i] == SE_AttackSpeed			// attack speed effect
			&& sp.base[i] < 100		// less than 100%
			)
			return true;
	}

	return false;
}

///////////////////////////////////////////////////

bool Spell::IsHasteSpell()
{
	int i;
	const SPDat_Spell_Struct &sp = this->Get();

	for(i = 0; i < EFFECT_COUNT; i++)
	{
		if(sp.effectid[i] == SE_AttackSpeed)
			return(sp.base[i] < 100);
	}

	return false;
}

///////////////////////////////////////////////////

bool Spell::IsAccelerationBuff()
{
	int i;
	const SPDat_Spell_Struct &sp = this->Get();

	for(i = 0; i < EFFECT_COUNT; i++)
	{
		//Yeahlight: Spell has a movement speed portion and the modifer is positive
		if(sp.effectid[i] == SE_MovementSpeed && sp.base[i] >= 0)
			return true;
	}
	return false;
}

///////////////////////////////////////////////////

bool Spell::IsDecelerationBuff()
{
	int i;
	const SPDat_Spell_Struct &sp = this->Get();

	for(i = 0; i < EFFECT_COUNT; i++)
	{
		//Yeahlight: Spell has a movement speed portion and the modifer is negative
		if(sp.effectid[i] == SE_MovementSpeed && sp.base[i] < 0)
			return true;
	}
	return false;
}

///////////////////////////////////////////////////

bool Spell::IsPercentalHealSpell()
{
	return this->IsEffectInSpell(SE_PercentalHeal);
}

///////////////////////////////////////////////////

bool Spell::IsRootSpell()
{
	return this->IsEffectInSpell(SE_Root);
}

///////////////////////////////////////////////////

bool Spell::IsGroupOnlySpell()
{
	return this->IsValidSpell() && this->Get().good_effect == 2;
}

///////////////////////////////////////////////////

bool Spell::IsBeneficialSpell()
{
	return this->Get().good_effect != 0 || IsGroupSpell();
}

///////////////////////////////////////////////////

bool Spell::IsDetrimentalSpell()
{
	return !IsBeneficialSpell();
}

///////////////////////////////////////////////////

bool Spell::IsInvulnerabilitySpell()
{
	return this->IsEffectInSpell(SE_DivineAura);
}

///////////////////////////////////////////////////

bool Spell::IsCHDurationSpell()
{
	return this->IsEffectInSpell(SE_CompleteHeal);
}

///////////////////////////////////////////////////

bool Spell::IsPoisonCounterSpell()
{
	return this->IsEffectInSpell(SE_PoisonCounter);
}

///////////////////////////////////////////////////

bool Spell::IsDiseaseCounterSpell()
{
	return this->IsEffectInSpell(SE_DiseaseCounter);
}

///////////////////////////////////////////////////

bool Spell::IsBindAffinitySpell()
{
	return this->IsEffectInSpell(SE_BindAffinity);
}

///////////////////////////////////////////////////

bool Spell::IsBindSightSpell()
{
	return this->IsEffectInSpell(SE_BindSight);
}

///////////////////////////////////////////////////

bool Spell::IsLevitationSpell()
{
	return this->IsEffectInSpell(SE_Levitate);
}

///////////////////////////////////////////////////

bool Spell::IsOutDoorSpell()
{
	//Yeahlight: TODO: Could you use root indoors?
	if(this->IsAccelerationBuff() || this->IsEffectInSpell(SE_Levitate) || this->IsEffectInSpell(SE_Harmony))
		return true;
	else
		return false;
}

///////////////////////////////////////////////////

bool Spell::IsSummonItemSpell()
{
	return this->IsEffectInSpell(SE_SummonItem);
}

///////////////////////////////////////////////////

bool Spell::IsSummonSkeletonSpell()
{
	return this->IsEffectInSpell(SE_NecPet);
}

///////////////////////////////////////////////////

bool Spell::IsSummonPetSpell()
{
	return
		(
		IsEffectInSpell(SE_SummonPet) ||
		IsEffectInSpell(SE_SummonBSTPet)
		);
}

///////////////////////////////////////////////////

bool Spell::IsCharmSpell()
{
	return this->IsEffectInSpell(SE_Charm);
}

///////////////////////////////////////////////////

bool Spell::IsBlindSpell()
{
	return this->IsEffectInSpell(SE_Blind);
}

///////////////////////////////////////////////////

bool Spell::IsEffectHitpointsSpell()
{
	return this->IsEffectInSpell(SE_CurrentHP);
}

///////////////////////////////////////////////////

bool Spell::IsReduceCastTimeSpell()
{
	return this->IsEffectInSpell(SE_IncreaseSpellHaste);
}

///////////////////////////////////////////////////

bool Spell::IsIncreaseDurationSpell()
{
	return this->IsEffectInSpell(SE_IncreaseSpellDuration);
}

///////////////////////////////////////////////////

bool Spell::IsReduceManaSpell()
{
	return this->IsEffectInSpell(SE_ReduceManaCost);
}

///////////////////////////////////////////////////

bool Spell::IsExtRangeSpell()
{
	return this->IsEffectInSpell(SE_IncreaseRange);
}

///////////////////////////////////////////////////

bool Spell::IsImprovedHealingSpell()
{
	return this->IsEffectInSpell(SE_ImprovedHeal);
}

///////////////////////////////////////////////////

bool Spell::IsImprovedDamageSpell()
{
	return this->IsEffectInSpell(SE_ImprovedDamage);
}

///////////////////////////////////////////////////

bool Spell::IsAEDurationSpell()
{
	return this->IsValidSpell() && this->Get().AEDuration !=0;
}

///////////////////////////////////////////////////

bool Spell::IsPureNukeSpell()
{
	int i, effect_count = 0;

	if(!IsValidSpell())
		return false;

	for(i = 0; i < EFFECT_COUNT; i++)
	{
		if(!IsBlankSpellEffect(i))
			effect_count++;
	}

	return
		(
		this->Get().effectid[0] == SE_CurrentHP &&
		effect_count == 1
		);
}

///////////////////////////////////////////////////

bool Spell::IsPartialCapableSpell()
{
	if(IsPureNukeSpell() || IsFearSpell())
		return true;

	return false;
}

///////////////////////////////////////////////////

bool Spell::IsResistableSpell()
{
	//Yeahlight: Resist type of 0 may never be resisted
	if(Get().resist_type == 0)
		return false;
	else
		return true;
}

///////////////////////////////////////////////////
// solar: checks if this spell affects your group
bool Spell::IsGroupSpell()
{
	return
		(
		IsValidSpell() &&
		(
		this->Get().targettype == ST_AEBard ||
		this->Get().targettype == ST_Group || 
		this->Get().targettype == ST_GroupSong
		)
		);
}

//Yeahlight: Checks to see if the spell is a "self" only targeted spell
bool Spell::IsSelfTargetSpell()
{
	if(this->Get().targettype == ST_Self || this->Get().targettype == ST_AECaster)
		return true;
	return false;
}

///////////////////////////////////////////////////
// solar: checks if this spell can be targeted
bool Spell::IsTGBCompatibleSpell()
{
	return
		(
		IsValidSpell() &&
		(
		!IsDetrimentalSpell() &&
		this->Get().buffduration != 0 &&
		!IsBardSong() &&
		!IsEffectInSpell(SE_Illusion)
		)
		);
}

///////////////////////////////////////////////////
// Pinedepain // Return true if the spell is a bardsong, otherwise false
bool Spell::IsBardSong()
{
	return
		(
		IsValidSpell() &&
		//Yeahlight: The condition needs to be '<= 60' because EVERY spell lists bard at 61, not 255
		this->Get().classes[BARD - 1] <= 60
		);
}

///////////////////////////////////////////////////

bool Spell::IsEffectInSpell(TSpellEffect effect)
{
	int j;

	if(!IsValidSpell())
		return false;

	for(j = 0; j < EFFECT_COUNT; j++)
		if(this->GetSpellEffectID(j) == effect) 
			return true;

	return false;
}

///////////////////////////////////////////////////

// solar: arguments are spell id and the index of the effect to check.
// this is used in loops that process effects inside a spell to skip
// the blanks
bool Spell::IsBlankSpellEffect(int effect_index)
{
	
	int base, formula;
	TSpellEffect effect = this->GetSpellEffectID(effect_index);
	base = this->GetSpellBase(effect_index);
	formula = this->GetSpellFormula(effect_index);

	return
		(
		effect == SE_Blank ||	// blank marker
		(	// spacer
		effect == SE_CHA && 
		base == 0 &&
		formula == 100
		)
		||
		effect == SE_StackingCommand_Block ||	// these are only used by stacking code
		effect == SE_StackingCommand_Overwrite
		);
}

///////////////////////////////////////////////////

// solar: checks some things about a spell id, to see if we can proceed
bool Spell::IsValidSpell()
{
	TSpellID spellid = this->GetSpellID();
	return
		(
		spellid != 0 &&
		spellid != 1 &&
		spellid != 0xFFFF &&
		spellid < SPDAT_RECORDS + 1 &&
		this->Get().player_1[0]
	);
}

///////////////////////////////////////////////////

//returns the lowest level of any caster which can use the spell
int Spell::GetMinLevel() {
	int r;
	int min = 60+1;
	const SPDat_Spell_Struct &spell = this->Get();
	for(r = 0; r < 15; r++) {
		if(spell.classes[r] < min)
			min = spell.classes[r];
	}

	return(min);
}

///////////////////////////////////////////////////

// solar: this will find the first occurance of effect.  this is handy
// for spells like mez and charm, but if the effect appears more than once
// in a spell this will just give back the first one.
int Spell::GetSpellEffectIndex(int effect)
{
	int i;

	if(!IsValidSpell())
		return -1;

	for(i = 0; i < EFFECT_COUNT; i++)
	{
		if(this->Get().effectid[i] == effect)
			return i;
	}

	return -1;
}

///////////////////////////////////////////////////

// solar: returns the level required to use the spell if that class/level
// can use it, 0 otherwise
// note: this isn't used by anything right now
int Spell::CanUseSpell(int _class, int level)
{
	int level_to_use;

	if(!IsValidSpell() || _class >= 15)
		return 0;

	level_to_use = this->GetSpellClass(_class - 1);

	if
		(
		level_to_use &&
		level_to_use != 255 &&
		level >= level_to_use
		)
		return level_to_use;

	return 0;
}

///////////////////////////////////////////////////

//from WR...
bool Spell::IsNoMerchantSpell()
{
	TSpellID id = this->GetSpellID();
	switch (id)
	{
	case 1196:
	case 1197:
	case 1900:
	case 1901:
	case 2100:
	case 2104:
	case 2107:
	case 2108:
	case 2109:
	case 2112:
	case 2115:
	case 2116:
	case 2145:
	case 2461:
	case 2467:
	case 2476:
	case 2320:
	case 1255:
	case 3018:
	case 652:
	case 1902:
	case 2101:
	case 2102:
	case 2103:
	case 2113:
	case 2114:
	case 2117:
	case 2120:
	case 2121:
	case 2122:
	case 2123:
	case 2125:
	case 2126:
	case 2474:
	case 2475:
	case 2479:
	case 2770:
	case 2632:
		return true;
	}
	return false;
}

///////////////////////////////////////////////////