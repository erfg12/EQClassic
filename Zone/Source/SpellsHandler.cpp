// Reimplemented by Cofruben.

#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <windows.h>
#include "SpellsHandler.hpp"
#include "config.h"
#include "EQCUtils.hpp"
#include "EQCException.hpp"
#include "moremath.h"

/*
	solar:	General outline of spell casting process

	1.
		a)	Client clicks a spell bar gem, ability, or item.  client_process.cpp
		gets the op and calls CastSpell() with all the relevant info including
		cast time.

		b)  NPC does CastSpell() from AI

	2.
		a)	CastSpell() determines there is a cast time and sets some state keeping
		flags to be used to check the progress of casting and finish it later.

		b)	CastSpell() sees there's no cast time, and calls CastedSpellFinished()
		Go to step 4.

	3.
		SpellProcess() notices that the spell casting timer which was set by
		CastSpell() is expired, and calls CastedSpellFinished()

	4.
		CastedSpellFinished() checks some timed spell specific things, like
		wether to interrupt or not, due to movement or melee.  If successful
		SpellFinished() is called.

	5.
		SpellFinished() checks some things like LoS, reagents, target and
		figures out what's going to get hit by this spell based on its type.

	6.
		a)	Single target spell, SpellOnTarget() is called.

		b)	AE spell, Entity::AESpell() is called.

		c)	Group spell, Group::CastGroupSpell()/SpellOnTarget() is called as
		needed.

	7.
		SpellOnTarget() may or may not call SpellEffect() to cause effects to
		the target

	8.
		If this was timed, CastedSpellFinished() will restore the client's
		spell bar gems.


	Most user code should call CastSpell(), with a 0 casting time if needed,
	and not SpellFinished().
*/

SpellsHandler		spells_handler;
extern EntityList	entity_list;

///////////////////////////////////////////////////
///////////////////Constructors////////////////////
///////////////////////////////////////////////////
SpellsHandler::SpellsHandler() {
	memset(this->pSpells, 0, sizeof(this->pSpells));
	this->pSpellsLoaded = false;

}

///////////////////////////////////////////////////

SpellsHandler::~SpellsHandler() {

}


///////////////////////////////////////////////////
//////////////////////Members//////////////////////
///////////////////////////////////////////////////

bool SpellsHandler::LoadSpells(char* filename) {
	if (this->SpellsLoaded()) return true;

	FILE *fp;
	if (fp = fopen(filename, "rb"))
	{
		SPDat_Spell_Struct sp;
		SPDat_Spell_Struct* spp = &sp;
		int i = 0;
		for(i = 0; i < SPDAT_RECORDS; i++)
		{
			memset(&sp, 0, sizeof(SPDat_Spell_Struct));
			fread(&sp, sizeof(SPDat_Spell_Struct), 1, fp);
			//Yeahlight: For whatever reason, they used SE_CHA as a spacer in buff effect IDs.
			//           This will iterate through all the effect IDs of each spell and set all
			//           instances of this occurance to SE_Blank if the base is 0 (unused).
			//           Using SE_CHA as a spacer is resulting in many, many stacking issues.
			for(int j = 0; j < 12; j++)
			{
				//Yeahlight: A spacer has been found; change it
				if(spp->effectid[j] == SE_CHA && spp->base[j] == 0)
				{
					spp->effectid[j] = SE_Blank;
				}
			}
			this->pSpells[i].Set(spp, i);
			this->spellResistModArray[i] = 0;
		}

		this->SetSpellsLoaded(true);
		EQC::Common::PrintF(CP_SPELL, "Spells loaded from '%s'\n", filename);
		fclose(fp);
		LoadCustomSpells();
		LoadSpellResistMods();
		LoadSpellAdjustments();
		return true;
	}
	else
		EQC::Common::PrintF(CP_SPELL, "SPDat not found ('%s'), spells not loaded.\n", SPDat_Location);
	return false;
}

///////////////////////////////////////////////////

Spell* SpellsHandler::GetSpellPtr(TSpellID spell_id) {
	if (!this->SpellsLoaded() || spell_id > this->GetMaxSpell()) return NULL;
	return &this->pSpells[spell_id];
}

///////////////////////////////////////////////////

//o--------------------------------------------------------------
//| WillSpellLand; Yeahlight, June 25, 2009
//o--------------------------------------------------------------
//| Returns true if caster may cast on the target
//o--------------------------------------------------------------
bool SpellsHandler::WillSpellLand(Mob* caster, Mob* target, Spell* spell)
{
	//Yeahlight: Caster, target or the spell was not supplied; abort
	if(caster == NULL || target == NULL || spell == NULL)
		return false;

	//Yeahlight: Entities may always cast spells on themselves
	if(caster == target)
		return true;

	//Yeahlight: Caster is a PC
	if(caster->IsClient())
	{
		//Yeahlight: The spell is detrimental or a utility spell
		if(spell->IsDetrimentalSpell() || spell->IsUtilitySpell())
		{
			//Yeahlight: Target exists and the PC may attack it
			if(caster->CastToClient()->CanAttackTarget(target))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		//Yeahlight: The spell is beneficial
		else if(spell->IsBeneficialSpell())
		{
			//Yeahlight: Target exists, is an NPC, is a pet and its owner is another client
			if(target->IsNPC() && target->GetOwner() && target->GetOwner()->IsClient())
			{
				return true;
			}
			//Yeahlight: Target exists and is another PC
			else if(target->IsClient())
			{
				return true;
			}
			//Harakiri: Bind Sight
			else if(spell->IsEffectInSpell(SE_BindSight))
			{
				return true;
			}
			//Yeahlight: Target may not be buffed by the PC
			else
			{
				return false;
			}
		}
	}
	//Yeahlight: Caster is an NPC and they may always land spells
	else if(caster->IsNPC())
	{
		return true;
	}
	return true;
}

///////////////////////////////////////////////////

sint32 SpellsHandler::CalcSpellValue(Spell* spell, int8 effect_number, int8 caster_level, int8 bardMod) {
	/*
		0x01 - 0x63 = level * formulaID

		0x64 = min
		0x65 = min + level / 2
		0x66 = min + level
		0x67 = min + level * 2
		0x68 = min + level * 3
		0x69 = min + level * 4
		0x6c = min + level / 3
		0x6d = min + level / 4
		0x6e = min + level / 5
		0x77 = min + level / 8
	*/
	if(!spell) return 0;
	int8 formula = spell->GetSpellFormula(effect_number);
	sint16 base = spell->GetSpellBase(effect_number);
	sint16 max = spell->GetSpellMax(effect_number);
	TSpellEffect effect = spell->GetSpellEffectID(effect_number);
	if (effect == SE_SummonItem) {
		base = 0;
		max = 20;
	}
	//cout << "Cast Spell: f=0x" << hex << (int) formula << dec << ", b=" << base << ", m=" << max << ", cl=" << (int) caster_level << endl;
	sint16 ubase = abs(base);
	sint16 result = 0;
	switch(formula) {
		case 0x64:
		case 0x00: {
			result = ubase;
			break;
		}
		case 0x65: {
			result = ubase + (caster_level / 2);
			break;
		}
		case 0x66: {
			result = ubase + caster_level;
			break;
		}
		case 0x67: {
			result = ubase + (caster_level * 2);
			break;
		}
		case 0x68: {
			result = ubase + (caster_level * 3);
			break;
		}
		case 0x69: {
			result = ubase + (caster_level * 4);
			break;
		}
		case 0x6c: {
			result = ubase + (caster_level / 3);
			break;
		}
		case 0x6d: {
			result = ubase + (caster_level / 4);
			break;
		}
		case 0x6e: {
			result = ubase + (caster_level / 5);
			break;
		}
		case 0x77: {
			result = ubase + (caster_level / 8);
			break;
		}

		// Pinedepain // Adding a new formula
		case 0x79:
			{
				result = ubase + (caster_level / 3);
				break;
			}
		default: {
			if (formula < 100) {
				result = ubase + (caster_level * formula);
				break;
			}
			else {
				cout << "Unknown spell formula. b=" << base << ", m=" << max << ", f=0x" << hex << (int) formula << dec << endl;
				break;
			}
		}
	}
	// Pinedepain // If there's a bard mod, obviously we're a bard so we can apply the bonus
	if (bardMod)
		result = (int)result*bardMod/10;

	sint32 res = (result >= max && max != 0) ? max * sign(base) : result * sign(base);

	if(effect == SE_ArmorClass && spell->IsBeneficialSpell()) {
		res = AproximateSpellAC(res);	// Cofruben: only beneficial spells?
	}

	return res;
}


///////////////////////////////////////////////////

Spell* SpellsHandler::GetSpellByName(const char* spell_name) {
	if (!this->SpellsLoaded()) return NULL;
	TSpellID min = 0, i = 0, max = this->GetMaxSpell();
	for (i = min; i < max; i++) {
		Spell* spell = this->GetSpellPtr(i);
		if (spell) {
			const char* i_name = spell->GetSpellName();
			if (strncasecmp(spell_name, i_name, strlen(spell_name)) == 0) return spell;
		}
	}
	return NULL;
}

///////////////////////////////////////////////////
// Cofruben: Given a spell, target and caster, calculates how many tics the
//			 buff will remain. Remember that there're evil buffs, like snare.
sint32 SpellsHandler::CalcSpellBuffTics(Mob* caster, Mob* target, Spell* spell) {
	if (!spell) return -1;
	uint8 caster_level = caster ? caster->GetLevel() : 1;
	const TBuffDurationFormula	buff_duration_formula	= spell->GetSpellBuffDurationFormula();
	int32						buff_duration			= spell->GetSpellBuffDuration();
	if (buff_duration > 0) return buff_duration;	// Time already given? ;)
	// Now we've to calculate the time it'll remain, since it's not given.
	switch (buff_duration_formula) {
		case BDF_Zero: {
			return 0;
		}
		case BDF_LevelDiv2:
		case BDF_LevelDiv2_: {
			return (sint32) (caster_level / 2);
		}
		case BDF_LevelDiv2Plus1: {
			return (sint32) (caster_level / 2 + 1);
		}
		case BDF_Duration1:
		case BDF_Duration2:
		case BDF_Duration3: {
			if (buff_duration == 0) {
				EQC::Common::PrintF(CP_CLIENT, "Spell \"%s\" has unknown duration for formula %i, using 10 tics as default\n", spell->GetSpellName(), buff_duration_formula);
				return 10;
			}
			return buff_duration;
		}
		case BDF_3: {
			return 3;
		}
		case BDF_LevelMult10: {
			return (sint32) (caster_level * 10);
		}
		case BDF_LevelMult10Plus10: {
			return (sint32) (caster_level * 10 + 10);
		}
		case BDF_LevelMult2Plus10: {
			return (sint32) (caster_level * 2 + 10);
		}
		case BDF_LevelMult3Plus10: {
			return (sint32) (caster_level * 3 + 10);
		}
		case BDF_Permanent: {
			return 32767;		// Doesn't matter anyways.
		}
		default: {
			if (caster && caster->IsClient())
				caster->CastToClient()->Message(BLACK, "Buff duration formula for spell \"%s\" unknown: %i", spell->GetSpellName(), buff_duration_formula);
		}

	}
	return buff_duration;
}

///////////////////////////////////////////////////

bool SpellsHandler::HandleBuffSpellEffects(Mob* caster, Mob* target, Spell* spell)
{
	if (!spell->IsBuffSpell()) return false;

	// 1. Get an empty buff slot, if exists.
	sint32 free_slot = this->GetFreeBuffSlot(target, spell);

	bool debugFlag = false;

	// 2. Empty slot?
	if (free_slot < 0 || free_slot >= 15) { // No empty slots, do nothing.
		if (debugFlag && caster && caster->IsClient() && caster->CastToClient()->GetDebugMe())
			caster->CastToClient()->Message(RED, "Error: All buff slots full.");
		return false;
	}

	// 3. Calculate buff effect time.
	sint32 tics = spells_handler.CalcSpellBuffTics(caster, target, spell);
	if (tics > 0) {
		target->GetBuff(free_slot).spell = spell;
		target->GetBuff(free_slot).casterlevel = caster ? caster->GetLevel() : 1;
		target->GetBuff(free_slot).casterid = caster ? caster->GetID() : 0;
		target->GetBuff(free_slot).durationformula = spell->GetSpellBuffDurationFormula();
		target->GetBuff(free_slot).ticsremaining = tics;
		target->GetBuff(free_slot).diseasecounters = spell->GetDiseaseCounters();
		target->GetBuff(free_slot).poisoncounters= spell->GetPoisonCounters();
		target->GetBuff(free_slot).damageRune = spell->GetRuneAmount();

		target->CalcBonuses(true, true);	// It's important to refresh our bonuses.
	}

	return true;
}

///////////////////////////////////////////////////

sint32 SpellsHandler::GetFreeBuffSlot(Mob* target, Spell* spell_target) {
	int i = 0;
	for (i = 0; i < BUFF_COUNT; i++) {
		Buffs_Struct buff = target->GetBuff(i);
		Spell* spell = buff.spell;
		if (!spell || (spell && (!spell->IsValidSpell() || spell == spell_target))) 
			return i;
	}
	return -1;
}

///////////////////////////////////////////////////

sint32 SpellsHandler::AproximateSpellAC(float server_ac) {
	// Cofruben: aproximating real AC by this nice formula.
	const double a0 = 1.515301796169375f;
	const double a1 = 0.17777381516761756f;
	const double a2 = 0.0024143132498674513f;
	const double a3 = -0.000018370683559159788f;
	const double a4 = 0.00000004610545751698899f;
	double amount_2 = server_ac * server_ac;
	double amount_3 = amount_2 * server_ac;
	double amount_4 = amount_3 * server_ac;
	double final = a0 + a1*server_ac + a2*amount_2 + a3*amount_3 + a4*amount_4;
	return (sint32)(final + 0.5f);
}

//o--------------------------------------------------------------
//| Name: CalcResistValue; Yeahlight, Nov 11, 2008
//o--------------------------------------------------------------
//| Returns the chance to resist the spell based on level and 
//| current resist values.
//| Note: This is strictly PC on NPC / NPC on NPC casting. This
//| will need a ton of work for PvP casting.
//o--------------------------------------------------------------
int16 SpellsHandler::CalcResistValue(Spell* spell, Mob* victim, Mob* caster)
{
	bool debugFlag = true;

	//Yeahlight: Victim, caster or spell was not supplied; abort
	if(!victim || !caster || !spell)
		return 0;

	//Yeahlight: Before we get started, check for NPC special resists
	if(victim->IsNPC())
	{
		if(victim->CastToNPC()->GetIsImmuneToMagic() && spell->GetSpellResistType() == 1)
		{
			return 100;
		}
		if(victim->CastToNPC()->GetCannotBeSlowed() && spell->IsSlowSpell())
		{
			if(caster->IsClient())
				caster->Message(RED, "Your target is immune to changes in its attack speed");
			return IMMUNE_RESIST_FLAG;
		}
		if(victim->CastToNPC()->GetCannotBeCharmed() && spell->IsCharmSpell())
		{
			if(caster->IsClient())
				//Yeahlight: TODO: Incorrect message
				caster->Message(RED, "Your target may not be charmed");
			return IMMUNE_RESIST_FLAG;
		}
		if(victim->CastToNPC()->GetCannotBeFeared() && spell->IsFearSpell())
		{
			if(caster->IsClient())
				//Yeahlight: TODO: Incorrect message
				caster->Message(RED, "Your target may not be feared");
			return IMMUNE_RESIST_FLAG;
		}
		if(victim->CastToNPC()->GetCannotBeMezzed() && spell->IsMezSpell())
		{
			if(caster->IsClient())
				caster->Message(RED, "Your target cannot be mesmerized");
			return IMMUNE_RESIST_FLAG;
		}
		if(victim->CastToNPC()->GetCannotBeSnaredOrRooted() && (spell->IsRootSpell() || spell->IsDecelerationBuff()))
		{
			if(caster->IsClient())
				caster->Message(RED, "Your target is immune to changes in its run speed");
			return IMMUNE_RESIST_FLAG;
		}
	}

	sint16 victimsSave = 0;
	sint16 baseSave = victim->GetLevel() * 3;
	sint16 saveDifference = 0;
	sint16 saveAdjustment = 0;
	int16 bonusResistance = 0;
	float resistChance = 0.00f;
	float levelModifier = 0.00f;
	int16 ret = 0.00f;
	sint16 spellResistModifier = this->spellResistModArray[spell->GetSpellID()];
	int16 victimsRace = victim->GetRace();

	if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
		caster->Message(LIGHTEN_BLUE, "Debug: Spells's resist type: %i", spell->GetSpellResistType());

	if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
		caster->Message(LIGHTEN_BLUE, "Debug: Spells's resist modifier: %i", spellResistModifier);

	//Yeahlight: Find the resist type and pull the associated save value
	switch(spell->GetSpellResistType())
	{
		//Yeahlight: All type zero spells appear to be unresistable
		case 0: 
			if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
				caster->Message(LIGHTEN_BLUE, "Debug: This spell is unresistable");
			return 0; 
			break;
		case 1: victimsSave = victim->GetMR(); break;
		case 2: victimsSave = victim->GetFR(); break;
		case 3: victimsSave = victim->GetCR(); break;
		case 4: victimsSave = victim->GetPR(); break;
		case 5: victimsSave = victim->GetDR(); break;
	}

	//Yeahlight: Dragons are "belly casters." You must stand directly under them to land a resistable spell
	if(victimsRace == LAVA_DRAGON || victimsRace == WURM || victimsRace == WATER_DRAGON || victimsRace == VELIOUS_DRAGONS || victimsRace == TRAKANON || victimsRace == SLEEPER)
	{
		//Yeahlight: Spell is not unresistable and caster is not within 35 range of the victim
		if(spell->GetSpellResistType() != 0 && caster->DistNoZ(victim) > 35)
		{
			if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
				caster->Message(LIGHTEN_BLUE, "Debug: Your target is a \"belly caster.\" You must stand directly under this NPC to land a resistable spell");
			return 100;
		}
	}

	if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
		caster->Message(LIGHTEN_BLUE, "Debug: Victim's base save value: %i", victimsSave);

	//Yeahlight: Associated penalties/bonuses for casting on a differently leveled target
	if(victim->GetLevel() > caster->GetLevel())
		saveAdjustment = (victim->GetLevel() - caster->GetLevel()) * 6;
	else
		saveAdjustment = (victim->GetLevel() - caster->GetLevel()) * 3;

	if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
		caster->Message(LIGHTEN_BLUE, "Debug: Victim's save adjustment value: %i", saveAdjustment);

	//Yeahlight: Target's current save value plus the adjustment calculated above minus the spell's resist Modifier (adding a negative)
	victimsSave += saveAdjustment + spellResistModifier;

	if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
		caster->Message(LIGHTEN_BLUE, "Debug: Victim's new save value: %i", victimsSave);

	//Yeahlight: Casting on an NPC above the caster's level grants the victim a resistance bonus, as long
	//           as the spell does not have a massive resist adjustment.
	//           Note: This will be the main tweak for casting raid DPS.
	if(victim->GetLevel() > caster->GetLevel() && spellResistModifier > -100)
	{
		switch(victim->GetLevel() - caster->GetLevel())
		{
			case 1: bonusResistance = 5; break;
			case 2: bonusResistance = 10; break;
			case 3: bonusResistance = 15; break;
			case 4: bonusResistance = 20; break;
			default: bonusResistance = 20; break;
		}
		if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
			caster->Message(LIGHTEN_BLUE, "Debug: Victim's bonus resistance: %i%s", bonusResistance, "%");
	}

	//Yeahlight: Target's adjusted save value minus the base save for its level (level * 3)
	saveDifference = victimsSave - baseSave;

	//Yeahlight: Base resist chance is 20%
	if(saveDifference == 0)
	{
		resistChance = 20.00f;
	}
	else if(saveDifference > 0)
	{
		resistChance = 20.00f * pow(1.10f, (saveDifference / 3));
	}
	else
	{
		//Yeahlight: saveDifference needs to be positive for the following calculation
		saveDifference = abs(saveDifference);
		resistChance = 20.00f * pow(0.90f, (saveDifference / 3));
	}

	//Yeahlight: NPCs and PCs alike will have at least a 3% chance to resist typical spells
	if(spellResistModifier > -100)
	{
		if(resistChance < 3)
			resistChance = 3;
		else if(resistChance > 100)
			resistChance = 100;
	}
	//Yeahlight: Spell is highly unresistable, give the victim a 1% chance to resist it
	else
	{
		if(resistChance < 1)
			resistChance = 1;
		else if(resistChance > 100)
			resistChance = 100;
	}

	//Yeahlight: Calculated resist chance + the base adjustment based on level
	ret = (int)(resistChance) + bonusResistance;

	if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
		caster->Message(LIGHTEN_BLUE, "Debug: %s's percent chance to resist this spell: %i%s", victim->GetName(), ret, "%");

	return ret;
}

//o--------------------------------------------------------------
//| Name: LoadSpellResistMods; Yeahlight, Nov 12, 2008
//o--------------------------------------------------------------
//| Loads the resist adjustment modifiers for all spells
//o--------------------------------------------------------------
bool SpellsHandler::LoadSpellResistMods()
{
	int16 spellID = 0;
	sint16 modifier = 0;
	ifstream myfile ("spellResistMods.txt");

	if (myfile.is_open())
	{
		while (! myfile.eof() )
		{
			myfile>>spellID>>modifier;
			if(spellID >= 0 && spellID < SPDAT_RECORDS)
				this->spellResistModArray[spellID] = modifier;
		}
	}
	else
	{
		myfile.close();
		return false;
	}
	myfile.close();
	return true;
}

//o--------------------------------------------------------------
//| Name: LoadSpellAdjustments; Yeahlight, Feb 28, 2009
//o--------------------------------------------------------------
//| Loads adjustments for spells after they are processed
//o--------------------------------------------------------------
bool SpellsHandler::LoadSpellAdjustments()
{
	pSpells[1546].SetSpellEffectID(2, SE_DeathSave);
	pSpells[1546].SetSpellBase(2, 2);
	pSpells[1547].SetSpellEffectID(2, SE_DeathSave);
	pSpells[1547].SetSpellBase(2, 1);
	return true;
}

//o--------------------------------------------------------------
//| Name: LoadCustomSpells; Yeahlight, Nov 21, 2008
//o--------------------------------------------------------------
//| Loads custom spells used in testing, parsing, etc.
//o--------------------------------------------------------------
bool SpellsHandler::LoadCustomSpells()
{
	SPDat_Spell_Struct sp;
	SPDat_Spell_Struct* spp = &sp;
	int16 spellID = SPDAT_RECORDS;

	//Yeahlight: Spell: Skin Like Christ; gives the player +32767hp
	memset(&sp, 0, sizeof(SPDat_Spell_Struct));
	spp->base[1] = 32767;
	spp->base[2] = 32767;
	spp->base[3] = 180;
	spp->cast_time = 14000;
	spp->recovery_time = 2500;
	spp->components[0] = 65535;
	spp->components[1] = 65535;
	spp->components[2] = 65535;
	spp->components[3] = 65535;
	spp->classes[0] = 61;
	spp->classes[1] = 61;
	spp->classes[2] = 61;
	spp->classes[3] = 61;
	spp->classes[4] = 61;
	spp->classes[5] = 61;
	spp->classes[6] = 61;
	spp->classes[7] = 61;
	spp->classes[8] = 61;
	spp->classes[9] = 61;
	spp->classes[10] = 61;
	spp->classes[11] = 61;
	spp->classes[12] = 61;
	spp->classes[13] = 61;
	spp->classes[14] = 61;
	strcpy(spp->player_1, "PLAYER_1");
	spp->effectid[0] = SE_CHA;
	spp->effectid[1] = SE_TotalHP;
	spp->effectid[2] = SE_CurrentHPOnce;
	spp->effectid[3] = SE_ArmorClass;
	spp->effectid[4] = SE_Blank;
	spp->effectid[5] = SE_Blank;
	spp->effectid[6] = SE_Blank;
	spp->effectid[7] = SE_Blank;
	spp->effectid[8] = SE_Blank;
	spp->effectid[9] = SE_Blank;
	spp->effectid[10] = SE_Blank;
	spp->effectid[11] = SE_Blank;
	spp->good_effect = 1;
	spp->icon = 2056;
	spp->spell_range = 250;
	spp->skill = 4;
	spp->spell_icon = 2511;
	spp->targettype = ST_Target;
	spp->buffduration = 5000;
	spp->unknown_3[0] = 255;
	spp->unknown_3[1] = 255;
	spp->unknown_3[2] = 255;
	spp->unknown_3[3] = 255;
	spp->unknown_5[0] = 255;
	spp->unknown_6[16] = 42;
	spp->unknown_6[24] = 2;
	strcpy(spp->name, "Skin Like Christ");
	this->pSpells[spellID].Set(spp, spellID);
	spellID++;

	return true;
}