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
#ifndef SPDAT_H
#define SPDAT_H

//#define SPDAT_SIZE		1824000
/* 
   solar: look at your spells_en.txt and find the id of the last spell.
   this number has to be 1 more than that.  if it's higher, your zone will
   NOT start up.  gonna autodetect this later..
*/
#define NEW_LoadSPDat

#define EFFECT_COUNT 12

enum RESISTTYPE {
		RESIST_NONE    = 0,
		RESIST_MAGIC   = 1,
		RESIST_FIRE    = 2,
		RESIST_COLD    = 3,
		RESIST_POISON  = 4,
        RESIST_DISEASE = 5 };

//Target Type IDs
#define ST_TargetOptional	0x01 // Target is used if present, but not required. ex: Flare, Fireworks
#define ST_AECaster			0x04 // ae centered around caster
#define ST_Target			0x05 // single targetted
#define ST_Self				0x06 // self only
#define ST_AETarget			0x08 // ae around target
#define ST_AEBard			0x28 // ae friendly around self (ae bard song)
#define ST_Group			0x29 // group spell
#define ST_GroupTeleport	0x03
#define ST_Undead			0x0a
#define ST_Tap				0x0d
#define ST_Pet				0x0e
#define ST_Animal           0x09
#define ST_Plant            0x10
#define ST_Dragon           0x12
#define ST_Giant            0x11
//#define ST_Unknown1			0x14
//#define ST_Unknown			0x18
//#define ST_Summoned			0x19
#define ST_Summoned			0x0b // NEOTOKYO: see spells_en.txt -> seems to be value 11 not 25
#define ST_Corpse			0x0f
#define ST_AlterPlane		0x3

#define RegenTwo			0xfe
//Spell Effect IDs
#define SE_CurrentHP		0x00 // Heals and nukes, repeates every tic if in a buff
#define SE_ArmorClass		0x01
#define SE_ATK				0x02
#define SE_MovementSpeed	0x03 // SoW, SoC, etc
#define SE_STR				0x04
#define SE_DEX				0x05
#define SE_AGI				0x06
#define SE_STA				0x07
#define SE_INT				0x08
#define SE_WIS				0x09

#define SE_CHA				0x0a // Often used as a spacer, who knows why
#define SE_AttackSpeed		0x0b
#define SE_Invisibility		0x0c
#define SE_SeeInvis			0x0d
#define SE_WaterBreathing	0x0e
#define SE_CurrentMana		0x0f
#define SE_Lull				0x12 // see SE_Harmony
#define SE_AddFaction		0x13 // Alliance line
#define SE_Stun				0x15
#define SE_Charm			0x16
#define SE_Fear				0x17
#define SE_Stamina			0x18 // Invigor and such
#define SE_BindAffinity		0x19
#define SE_Gate				0x1a // Gate to bind point
#define SE_CancelMagic		0x1b
#define SE_InvisVsUndead	0x1c
#define SE_InvisVsAnimals	0x1d
#define SE_ChangeFrenzyRad  0x1e
#define SE_Mez				0x1f
#define SE_SummonItem		0x20
#define SE_SummonPet		0x21
#define SE_DiseaseCounter   0x23
#define SE_PoisonCounter    0x24
#define SE_DivineAura		0x28
#define SE_ShadowStep		0x2a
#define SE_ResistFire		0x2e
#define SE_ResistCold		0x2f
#define SE_ResistPoison		0x30
#define SE_ResistDisease	0x31
#define SE_ResistMagic		0x32
#define SE_SenseDead		0x34
#define SE_SenseSummoned	0x35
#define SE_SenseAnimals		0x36
#define SE_Rune				0x37
#define SE_TrueNorth		0x38
#define SE_Levitate			0x39
#define SE_Illusion			0x3a
#define SE_DamageShield		0x3b
#define SE_Identify			0x3d
#define SE_WhipeHateList	0x3f
#define SE_SpinTarget		0x40
#define SE_InfaVision		0x41
#define SE_UltraVision		0x42
#define SE_EyeOfZomm		0x43
#define SE_ReclaimPet		0x44
#define SE_TotalHP			0x45
#define SE_NecPet			0x47
#define SE_BindSight		0x49
#define SE_FeignDeath		0x4a
#define SE_VoiceGraft		0x4b
#define SE_Sentinel			0x4c
#define SE_LocateCorpse		0x4d
#define SE_AbsorbMagicAtt   0x4e    // rune for spells
#define SE_CurrentHPOnce	0x4f // Heals and nukes, non-repeating if in a buff
#define SE_Revive			0x51
#define SE_Teleport			0x53
#define SE_WeaponProc		0x55 // i.e. Call of Fire
#define SE_Harmony			0x56 // what is SE_Lull??
#define SE_Succor			0x58 // Evacuate/Succor lines?
#define SE_ModelSize		0x59 // Shrink, Growth
#define SE_SummonCorpse		0x5b
#define SE_Calm		0x5c // Hate modifier. Enrageing blow
#define SE_NegateIfCombat   0x5e // Component of Spirit of Scale
#define SE_ManaPool         0x61
#define SE_Root				0x63
#define SE_HealOverTime		0x64
#define SE_CompleteHeal     0x65
#define SE_Translocate		0x68
#define SE_SummonBSTPet		0x6a // neotokyo: added BST pet support
#define SE_Familiar         0x6c
#define SE_ResistAll        0x6f
#define	SE_SummonHorse		0x71
#define SE_ChangeAggro		0x72 // chanter spells Horrifying Visage ...
#define SE_ModCastSkill     0x70
#define SE_ReverseDS        0x79

#define SE_ImprovedDamage   0x7C
#define SE_ImprovedHeal     0x7D
//                          0x7E??
#define SE_IncreaseSpellHaste 0x7F
#define SE_IncreaseSpellDuration 0x80
#define SE_IncreaseRange    0x81
#define SE_ReduceSpellHate  0x82
#define SE_ReduceReagentCost 0x83
#define SE_ReduceManaCost   0x84

#define SE_LimitMaxLevel    0x86
#define SE_LimitResist      0x87
#define SE_LimitTarget      0x88
#define SE_LimitEffect      0x89
#define SE_LimitSpellType   0x8A
#define SE_LimitSpell       0x8B
#define SE_LimitMinDur      0x8C
#define SE_LimitInstant     0x8D
#define SE_LimitMinLevel    0x8E
#define SE_LimitCastTime    0x8F
#define SE_PercentalHeal    0x93
// use the base as the effectid to not stack with, the max as the value to overwrite
#define SE_StackingCommand1	0x94
#define SE_StackingCommand2	0x95

#define DF_Permanent		50

struct SPDat_Spell_Struct
{
/* 000 */	char	name[32];					/* Name of the spell */
/* 032 */	char	player_1[32];				/* "PLAYER_1" */
/* 064 */	char	teleport_zone[32];			/* Teleport zone, or item summoned */
/* 096 */	char	you_cast[64];				/* Message when you cast */
/* 160 */	char	other_casts[64];			/* Message when other casts */
/* 224 */	char	cast_on_you[64];			/* Message when spell is cast on you */
/* 288 */	char	cast_on_other[64];			/* Message when spell is cast on someone else */
/* 352 */	char	spell_fades[64];			/* Spell fades */
/* 416 */	float	range;
/* 420 */	float	aoerange;
/* 424 */	float	pushback;
/* 428 */	float	pushup;
/* 432 */	int32	cast_time;					/* Cast time */
/* 436 */	int32	recovery_time;				/* Recovery time */
/* 440 */	int32	recast_time;				/* Recast same spell time */
/* 444 */	int32	buffdurationformula;
///* 445 */	int8	unknown_1[3];				/* Spacer */
/* 448 */	int32	buffduration;
/* 450 */	int32	ImpactDuration;				/* Spacer */
/* 456 */	int16	mana;						/* Mana Used */
/* 458 */	sint16	base[EFFECT_COUNT];
/* 482 */	sint16	max[EFFECT_COUNT];
/* 508 */	int16	icon;						/* Spell icon */
			int16	memicon;					/* Icon on membarthing */
/* 510 */	sint16	components[4];				// regents
/* 518 */	int32 component_counts[4];		// amount of regents used
/* 526 */	signed NoexpendReagent[4];				/* Spacer */
/* 530 */	int16	formula[EFFECT_COUNT];     			// Spell's value formula
			int	LightType;				// probaly another effecttype flag
			int	goodEffect;				// 1= very good ;) 2 = Translocate etc unknown4[1]
			int	Activated;				// probaly another effecttype flag	
			
			int	resisttype;
			//int8	badEffect;					//unknown4[3] - Spells can have good & bad effects
/* 546 */	int	effectid[EFFECT_COUNT];				/* Spell's effects */
/* 558 */	int	targettype;					/* Spell's Target */
/* 559 */	int	basediff;					/* base difficulty fizzle adjustment */
/* 560 */	int	skill;
			sint16	zonetype;
			int16	EnvironmentType;
			int	TimeOfDay;
/* 565 */	int8	classes[15];				/* Classes */
/* 580 *///	int16	unknown1[3];
		//	sint16 unknown2;
			int8	CastingAnim;
/*0598*/	int8	TargetAnim;
/*0600*/	int32	TravelType;
/*0604*/	int16	SpellAffectIndex;
/*0606*/	int16	Spacing2[23];
			sint16  ResistDiff;
			int16	Spacing3[2];
			int16 RecourseLink;
			int16 Spacing4[1];
			
};

#ifdef NEW_LoadSPDat
	extern const SPDat_Spell_Struct* spells; 
	extern sint32 SPDAT_RECORDS;
#else
	#define SPDAT_RECORDS	3602
#endif

bool IsMezSpell(int16 spell_id);
bool IsCharmSpell(int16 spell_id);
bool IsExtRangeSpell(int16 spell_id);
bool IsReduceManaSpell(int16 spell_id);
bool IsIncreaseDurationSpell(int16 spell_id);
bool IsReduceCastTimeSpell(int16 spell_id);
bool IsPercentalHeal(int16 spell_id);
bool IsImprovedHealingSpell(int16 spell_id);
bool IsImprovedDamageSpell(int16 spell_id);
bool IsDetrimental(int16 spell_id);
bool IsBeneficial(int16 spell_id);
bool IsEffectHitpoints(int16 spell_id);
bool IsInvulnerability(int16 spell_id);
bool IsCHDuration(int16 spell_id);
bool IsPoisonCounter(int16 spell_id);
bool IsDiseaseCounter(int16 spell_id);
bool IsSummonItem(int16 spell_id);
bool IsSummonSkeleton(int16 spell_id);
bool IsSummonPet(int16 spell_id);
#endif
