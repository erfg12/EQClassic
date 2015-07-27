// Written by Cofruben.
#ifndef SPELL_HPP
#define SPELL_HPP
#include "spdat.h"


class Spell {
public:
	Spell();

	void						Set(SPDat_Spell_Struct* spell, TSpellID id);
	const SPDat_Spell_Struct&	Get() { return this->pSpell; }

	// Spell types.
	bool IsBuffSpell();
	bool IsSacrificeSpell();
	bool IsLifetapSpell();
	bool IsMezSpell();
	bool IsStunSpell();
	bool IsSlowSpell();
	bool IsHasteSpell();
	bool IsPercentalHealSpell();
	bool IsGroupOnlySpell();
	bool IsBeneficialSpell();
	bool IsDetrimentalSpell();
	bool IsIllusionSpell();
	bool IsSummonCorpseSpell();
	bool IsInvulnerabilitySpell();
	bool IsCHDurationSpell();
	bool IsPoisonCounterSpell();
	bool IsDiseaseCounterSpell();
	bool IsSummonItemSpell();
	bool IsSummonSkeletonSpell();
	bool IsSummonPetSpell();
	bool IsCharmSpell();
	bool IsBlindSpell();
	bool IsEffectHitpointsSpell();
	bool IsReduceCastTimeSpell();
	bool IsIncreaseDurationSpell();
	bool IsReduceManaSpell();
	bool IsExtRangeSpell();
	bool IsImprovedHealingSpell();
	bool IsImprovedDamageSpell();
	bool IsAEDurationSpell();
	bool IsPureNukeSpell();
	bool IsPartialCapableSpell();
	bool IsResistableSpell();
	bool IsGroupSpell();
	bool IsTGBCompatibleSpell();
	bool IsBardSong();
	bool IsEffectInSpell(TSpellEffect effect);
	bool IsBlankSpellEffect(int effect_index);
	bool IsValidSpell();
	bool IsSummonSpell();
	bool IsEvacSpell();
	bool IsDamageSpell();
	bool IsFearSpell();
	bool IsNoMerchantSpell();
	bool IsBindAffinitySpell();
	bool IsLevitationSpell();
	bool IsOutDoorSpell();
	bool IsAccelerationBuff();
	bool IsRootSpell();
	bool IsDecelerationBuff();
	bool IsCancelMagicSpell();
	bool IsMemoryBlurSpell();
	bool IsUtilitySpell();
	bool IsSelfTargetSpell();
	bool IsBindSightSpell();
	bool IsAttackSpeedSpell();

	// Get and such.
	TSpellID		GetSpellID()					{ return this->pID; }
	int32			GetSpellBuffDuration()			{ return this->pSpell.buffduration; }
	int32			GetSpellAEDuration()			{ return this->pSpell.AEDuration; }
	sint16			GetSpellBase(int8 index);
	void			SetSpellBase(int8 index, sint16 value);
	sint16			GetSpellMax(int8 index);
	TBuffDurationFormula
					GetSpellBuffDurationFormula()	{ return this->pSpell.buffdurationformula; }
	int8			GetSpellClass(int8 index);
	int8			GetSpellFormula(int8 index);
	int8			GetSpellSkill();
	int16			GetSpellComponent(int8 index);
	int16			GetSpellManaRequired();
	int32			GetSpellCastTime();
	int8			GetSpellBaseDiff()				{ return this->pSpell.basediff; }
	TSpellEffect	GetSpellEffectID(int8 element);
	void			SetSpellEffectID(int8 index, TSpellEffect value);
	TTargetType		GetSpellTargetType()			{ return this->pSpell.targettype; }
	int				GetMinLevel();
	int				GetSpellEffectIndex(int effect);
	int				CanUseSpell(int _class, int level);
	float			GetSpellAOERange()				{ return this->pSpell.aoerange; }
	const char*		GetSpellName()					{ return this->pSpell.name; }
	const char*		GetSpellTeleportZone()			{ return this->pSpell.teleport_zone; }
	float			GetSpellRange()					{ return this->pSpell.spell_range; }
	int8			GetSpellResistType()			{ return this->pSpell.resist_type; }
	int16			GetRuneAmount();
	int16			GetDiseaseCounters();
	int16			GetPoisonCounters();
	int32			GetSpellRecoveryTime()			{ return this->pSpell.recovery_time; }
	int32			GetSpellRecastTime()			{ return this->pSpell.recast_time * 1000; }

private:
	TSpellID			pID;
	SPDat_Spell_Struct	pSpell;
};




#endif