#ifndef SPDAT_H
#define SPDAT_H
#include "types.h"
#include "config.h"
#include <cstring>

typedef int16 TSpellID;
#define SPDat_Location	"spdat.eff"
#define SPDAT_SIZE		1824000
#define SPDAT_RECORDS	3700
#define EFFECT_COUNT	12
#define BUFF_COUNT		15
#define SPELL_UNKNOWN	0xFFFF

// Cofruben: better in an enumerated way.
enum TTargetType : int8 {
	ST_LineOfSight		= 1, // Target is used if present, but not required. ex: Flare, Fireworks
	ST_GroupSong		= 3, // Melinko: renamed from GroupTeleport to GroupSong, type appears when bard casts group song
	ST_AECaster			= 4, // ae centered around caster
	ST_Target			= 5, // single targetted
	ST_Self				= 6, // self only
	ST_AETarget			= 8, // ae around target
	ST_Animal           = 9,
	ST_Undead			= 10,
	ST_Summoned			= 11,
	ST_Tap				= 13,
	ST_Pet				= 14,
	ST_Corpse			= 15,
	ST_Plant            = 16,
	ST_Giant            = 17,
	ST_Dragon           = 18,
	ST_UndeadAE			= 24,
	ST_AEBard			= 40, // ae friendly around self (ae bard song)
	ST_Group			= 41 // group spell
};


//Spell Effect IDs
enum TSpellEffect : uint8 {
	SE_CurrentHP = 0,			// Heals and nukes, repeates every tic if in a buff
	SE_ArmorClass = 1,
	SE_ATK = 2,
	SE_MovementSpeed = 3,		// SoW, SoC, etc
	SE_STR = 4,
	SE_DEX = 5,
	SE_AGI = 6,
	SE_STA = 7,
	SE_INT = 8,
	SE_WIS = 9,
	SE_CHA = 10,				// Often used as a spacer, who knows why
	SE_AttackSpeed = 11,
	SE_Invisibility = 12,
	SE_SeeInvis = 13,
	SE_WaterBreathing = 14,
	SE_CurrentMana = 15,
	SE_Lull = 18,				// see SE_Harmony
	SE_AddFaction = 19,
	SE_Blind = 20,
	SE_Stun = 21,
	SE_Charm = 22,
	SE_Fear = 23,
	SE_Stamina = 24,
	SE_BindAffinity = 25,
	SE_Gate = 26,
	SE_CancelMagic = 27,
	SE_InvisVsUndead = 28,
	SE_InvisVsAnimals = 29,
	SE_ChangeFrenzyRad = 30,
	SE_Mez = 31,
	SE_SummonItem = 32,
	SE_SummonPet = 33,
	SE_DiseaseCounter = 35,
	SE_PoisonCounter = 36,
	SE_DivineAura = 40,
	SE_Destroy = 41,			// Disintegrate, Banishment of Shadows
	SE_ShadowStep = 42,
	SE_Lycanthropy = 44,
	SE_ResistFire = 46,
	SE_ResistCold = 47,
	SE_ResistPoison = 48,
	SE_ResistDisease = 49,
	SE_ResistMagic = 50,
	SE_SenseDead = 52,
	SE_SenseSummoned = 53,
	SE_SenseAnimals = 54,
	SE_Rune = 55,
	SE_TrueNorth = 56,
	SE_Levitate = 57,
	SE_Illusion = 58,
	SE_DamageShield = 59,
	SE_Identify = 61,
	SE_WipeHateList = 63,
	SE_SpinTarget = 64,
	SE_InfraVision = 65,
	SE_UltraVision = 66,
	SE_EyeOfZomm = 67,
	SE_ReclaimPet = 68,
	SE_TotalHP = 69,
	SE_NecPet = 71,
	SE_BindSight = 73,
	SE_FeignDeath = 74,
	SE_VoiceGraft = 75,
	SE_Sentinel = 76,
	SE_LocateCorpse = 77,
	SE_AbsorbMagicAtt = 78,		// rune for spells
	SE_CurrentHPOnce = 79,		// Heals and nukes, non-repeating if in a buff
	SE_Revive = 81,				// resurrect
	SE_CallOfHero = 82,
	SE_Teleport = 83,
	SE_TossUp = 84,				// Gravity Flux
	SE_WeaponProc = 85,			// i.e. Call of Fire
	SE_Harmony = 86,			// what is SE_Lull??
	SE_MagnifyVision = 87,		// Telescope
	SE_Succor = 88,				// Evacuate/Succor lines?
	SE_ModelSize = 89,			// Shrink, Growth
	SE_Cloak = 90,				// some kind of focus effect?
	SE_SummonCorpse = 91,
	SE_Calm = 92,				// Hate modifier. Enrageing blow
	SE_StopRain = 93,			// Wake of Karana
	SE_NegateIfCombat = 94,		// Component of Spirit of Scale
	SE_Sacrifice = 95,
	SE_Silence = 96,			// Cacophony
	SE_ManaPool = 97,
	SE_AttackSpeed2 = 98,		// Melody of Ervaj
	SE_Root = 99,
	SE_HealOverTime = 100,
	SE_CompleteHeal = 101,
	SE_Fearless = 102,			// Valiant Companion
	SE_CallPet = 103,			// Summon Companion
	SE_Translocate = 104,
	SE_AntiGate = 105,			// Translocational Anchor
	SE_SummonBSTPet = 106,		// neotokyo: added BST pet support
	SE_Familiar = 108,
	SE_SummonItemIntoBag = 109,	// Summon Jewelry Bag - summons stuff into container
	SE_ResistAll = 111,
	SE_CastingLevel = 112,
	SE_SummonHorse = 113,
	SE_ChangeAggro = 114,		// chanter spells Horrifying Visage ...
	SE_Hunger = 115,			// Song of Sustenance
	SE_CurseCounter = 116,
	SE_MagicWeapon = 117,		// Magical Monologue
	SE_SingingSkill = 118,		// Amplification
	SE_AttackSpeed3 = 119,		// Frenzied Burnout
	SE_HealRate = 120,			// Packmaster's Curse - not sure what this is
	SE_ReverseDS = 121,
	SE_Screech = 123,			// Form of Defense
	SE_ImprovedDamage = 124,
	SE_ImprovedHeal = 125,
	SE_IncreaseSpellHaste = 127,
	SE_IncreaseSpellDuration = 128,
	SE_IncreaseRange = 129,
	SE_ReduceSpellHate = 130,
	SE_ReduceReagentCost = 131,
	SE_ReduceManaCost = 132,
	SE_LimitMaxLevel = 134,
	SE_LimitResist = 135,
	SE_LimitTarget = 136,
	SE_LimitEffect = 137,
	SE_LimitSpellType = 138,
	SE_LimitSpell = 139,
	SE_LimitMinDur = 140,
	SE_LimitInstant = 141,
	SE_LimitMinLevel = 142,
	SE_LimitCastTime = 143,
	SE_Teleport2 = 145,			// Banishment of the Pantheon
	SE_PercentalHeal = 147,
	SE_StackingCommand_Block = 148,
	SE_StackingCommand_Overwrite = 149,
	SE_DeathSave = 150,
	SE_TemporaryPets = 152,		// Swarm of Fear III
	SE_BalanceHP = 153,			// Divine Arbitration
	SE_DispelDetrimental = 154,
	SE_IllusionCopy = 156,		// Deception
	SE_SpellDamageShield = 157,	// Petrad's Protection
	SE_Reflect = 158,
	SE_AllStats = 159,			// Aura of Destruction
	SE_MakeDrunk = 160,
	SE_MeleeMitigation = 168,
	SE_CriticalHitChance = 169,
	SE_CrippBlowChance = 171,
	SE_AvoidMeleeChance = 172,
	SE_RiposteChance = 173,
	SE_DodgeChance = 174,
	SE_ParryChance = 175,
	SE_DualWeildChance = 176,
	SE_DoubleAttackChance = 177,
	SE_MeleeLifetap = 178,
	SE_AllInstrunmentMod = 179,
	SE_ResistSpellChance = 180,
	SE_ResistFearChance = 181,
	SE_HundredHands = 182,
	SE_MeleeSkillCheck = 183,
	SE_HitChance = 184,
	SE_DamageModifier = 185,
	SE_MinDamageModifier = 186,
	SE_FadingMemories = 194,
	SE_StunResist = 195,
	SE_ProcChance = 200,
	SE_RangedProc = 201,			//not implemented
	SE_Rampage = 205,
	SE_AETaunt = 206,
	SE_ReduceSkillTimer = 227,		//not implemented
	SE_DivineSave = 232,			//not implemented (base == % chance on death to insta-res)
	SE_Blank = 254
/*	SE_ExtraAttackChance = 266,		//not implemented
	SE_CriticalDoTChance = 273,		//not implemented
	SE_CriticalSpellChance = 295,	//not implemented
	SE_ChangeHeight = 298,			//not implemented
	SE_WakeTheDead = 299,
	SE_Doppelganger = 300,
	SE_NoCombatSkills = 311,
	SE_DefensiveProc = 323,			//not implemented
	SE_CriticalDamageMob = 330,		//not implemented
	SE_BardAEDot = 334				//needs a better name (spell id 703 and 730)
	*/
};

// Pinedepain // Spell skill ID
#define SS_PERCUSSION       0x46
#define SS_BRASS            0x0c
#define SS_WIND				0x36
#define SS_STRING			0x31
#define SS_SINGING			0x29

// Pinedepain // Instrument types (in the items database). So considering it as a part of the item struct (IS = Item Struct)
#define IS_PERCUSSION		0x1a
#define IS_BRASS			0x19
#define IS_WIND				0x17
#define IS_STRING			0x18
#define IS_SINGING			0x32
#define IS_ALLINSTRUMENT	0x33

// Pinedepain // Animation ID
#define AN_PERCUSSION		0x28 // Pinedepain // For now I use the same animation for the percussion as the string one because it's broken, normally the code is 0x27
#define AN_BRASS			0x29
#define AN_WIND				0x29
#define AN_STRING			0x28

enum TBuffDurationFormula : int8 {
	BDF_Zero				= 0,
	BDF_LevelDiv2			= 1,
	BDF_LevelDiv2Plus1		= 2,
	BDF_Duration1			= 3,
	BDF_Duration2			= 4,
	BDF_3					= 5,
	BDF_LevelDiv2_			= 6,
	BDF_LevelMult10			= 7,
	BDF_LevelMult10Plus10	= 8,
	BDF_LevelMult2Plus10	= 9,
	BDF_LevelMult3Plus10	= 10,
	BDF_Duration3			= 11,
	BDF_Permanent			= 50
};

//Yeahlight: Struct of our active rain spells
struct RainSpell_Struct
{
	int16	spellID;
	int8	wavesLeft;
	float	x;
	float	y;
	float	z;
};

enum ResistMessage : uint8
{
	RM_None				  = 0,
	RM_CharmTargetTooHigh = 1,
	RM_MezTargetTooHigh   = 2
};

struct SPDat_Spell_Struct
{
	SPDat_Spell_Struct() { memset(this, 0, sizeof(this)); }
/* 000 */	char	name[32];					/* Name of the spell */
/* 032 */	char	player_1[32];				/* "PLAYER_1" */
/* 064 */	char	teleport_zone[32];			/* Teleport zone, or item summoned */
/* 096 */	char	you_cast[64];				/* Message when you cast */
/* 160 */	char	other_casts[64];			/* Message when other casts */
/* 224 */	char	cast_on_you[64];			/* Message when spell is cast on you */
/* 288 */	char	cast_on_other[64];			/* Message when spell is cast on someone else */
/* 352 */	char	spell_fades[64];			/* Spell fades */
/* 416 */	float	spell_range;
/* 420 */	float	aoerange;
/* 424 */	float	pushback;
/* 428 */	float	pushup;
/* 432 */	int32	cast_time;					/* Cast time */
/* 436 */	int32	recovery_time;				/* Recovery time */
/* 440 */	int32	recast_time;				/* Recast same spell time */
/* 444 */	TBuffDurationFormula
					buffdurationformula;
/* 445 */	int8	unknown_1[3];				/* Spacer */
/* 448 */	int32	buffduration;
/* 452 */	int32	AEDuration;				/* Spacer */
/* 456 */	int16	mana;						/* Mana Used */
/* 458 */	sint16	base[EFFECT_COUNT];
/* 482 */	sint16	max[EFFECT_COUNT];
/* 506 */	int16	spell_icon;					// Pinedepain // I think this is the spell Icon, the actual icon member is the spell mem icon (the memorized spell icon), waiting for confirmation to update this
/* 508 */	int16	icon;						/* Spell icon */
/* 510 */	int16	components[4];				// regents
/* 518 */	int16	component_counts[4];		// amount of regents used
/* 526 */	int8	unknown_3[4];				/* Spacer */
/* 530 */	int8	formula[EFFECT_COUNT];		// Spell's value formula
			//  -- Cofruben: added the following ones, not sure if they're correct.
/* 542 */	int8	light_type;
/* 543 */	int8	good_effect;
/* 544 */	int8	activated;
/* 545 */	int8	resist_type;
			//  --
/* 546 */	TSpellEffect	effectid[EFFECT_COUNT];		/* Spell's effects */
/* 558 */	TTargetType		targettype;					/* Spell's Target */
/* 559 */	int8			basediff;					/* base difficulty fizzle adjustment, updated from 4.0, not sure if this is ok */
/* 560 */   int8			skill;						// Pinedepain // This is the actual exact skill ID used to cast
/* 561 */	int8			unknown_5[4];				/* Spacer */ // Pinedepain // Changed unknown_5[5] to unknown_5[4]
/* 565 */	int8			classes[15];				/* Classes */
/* 580 */	int8			unknown_6[28];				/* Spacer */
};
#endif
