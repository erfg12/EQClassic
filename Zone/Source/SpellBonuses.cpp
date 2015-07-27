// Written by Cofruben.
#include <cmath>
#include "database.h"
#include "SpellsHandler.hpp"
#include "mob.h"
#include "classes.h"


const bool	SPAM_SPELL_BONUSES = true;
extern		SpellsHandler		spells_handler;
extern		Database			database;

///////////////////////////////////////////////////

void Mob::ApplySpellsBonuses(Spell* spell, int8 casterlevel, StatsStruct* newbon, int8 bardMod, int16 caster_id, bool isOnItem)
{
	//Yeahlight: Spells are not loaded or a valid spell has not been supplied; abort
	if(!spells_handler.SpellsLoaded() || !spell || spell->GetSpellID() == 0xFFFF)
		return;

	//Yeahlight: Iterate through each effect slot of the spell
	for(int i = 0; i < 12; i++)
	{
		TSpellEffect effect_id = spell->GetSpellEffectID(i);
		if(effect_id == SE_Blank) continue;
		int8	formula	= spell->GetSpellFormula(i);
		sint16	base	= spell->GetSpellBase(i);
		sint16	max		= spell->GetSpellMax(i);
		sint32	amount	= spells_handler.CalcSpellValue(spell, i, casterlevel, bardMod);
		switch (effect_id) {
			case SE_TotalHP: {
				newbon->HP += amount;
				CalcMaxHP();
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): HP bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_ArmorClass: {
				newbon->AC += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): AC bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_ATK: {
				newbon->ATK += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): ATK bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_STR: {
				newbon->STR += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): STR bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_DEX: {
				newbon->DEX += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): DEX bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_AGI: {
				newbon->AGI += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): AGI bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_STA: {
				newbon->STA += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): STA bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_INT: {
				newbon->INT += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): INT bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_WIS: {
				newbon->WIS += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): WIS bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_CHA: {
				newbon->CHA += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): CHA bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_ResistFire: {
				newbon->FR += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Resist fire bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_ResistCold: {
				newbon->CR += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Resist cold bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_ResistPoison: {
				newbon->PR += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Resist poison bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_ResistDisease: {
				newbon->DR += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Resist disease bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_ResistMagic: {
				newbon->MR += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Resist magic bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			// -- Cofruben: new effects --
			case SE_MovementSpeed:
			{
				newbon->MovementSpeed += amount;
				//Yeahlight: Update the NPC's velocity immediately
				if(this->IsNPC())
				{
					if(this->CastToNPC()->isMoving())
					{
						this->CastToNPC()->setMoving(true);
						this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
					}
					else
					{
						this->CastToNPC()->setMoving(false);
					}
				}
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Movement speed bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Root:
			{
				newbon->MovementSpeed = (sint16)-100;
				//Yeahlight: Update the NPC's velocity immediately
				if(this->IsNPC())
				{
					if(this->CastToNPC()->isMoving())
					{
						this->CastToNPC()->setMoving(true);
						this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
					}
					else
					{
						this->CastToNPC()->setMoving(false);
					}
				}
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Root bonus.", spell->GetSpellName(), amount);
				break;
			}
			case SE_DivineAura: {
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Divine aura bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_HealOverTime: {
				newbon->HPRegen += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Heal Over Time bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_CurrentHPOnce: {
				break;
			}
			case SE_CurrentHP: {
				this->spellbonuses.HPRegen += amount;
				if (amount < 0)
					this->spellbonuses.DoTer_id = caster_id;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): HP regeneration bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_CurrentMana: {	// Breeze, clarity, etc..
				this->spellbonuses.ManaRegen += amount;
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Mana regeneration bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_DamageShield:
			{
				//Yeahlight: Normal damage shield
				if(amount < 0)
				{
					this->spellbonuses.DamageShield += abs(amount);
					this->spellbonuses.DamageShieldType = spell->GetSpellResistType();
				}
				//Yeahlight: Reverse damage shield
				else
				{
					this->spellbonuses.ReverseDamageShield += amount;
				}
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Damage Shield bonus of %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_InfraVision:
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Infravision bonus of %i.", spell->GetSpellName(), amount);
				break;
			case SE_UltraVision:
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Ultravision bonus of %i.", spell->GetSpellName(), amount);
				break;
			case SE_MagnifyVision:
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Magnify vision bonus of %i.", spell->GetSpellName(), amount);
				break;
			case SE_ChangeFrenzyRad:
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): Change Frenzy Radius bonus of %i.", spell->GetSpellName(), amount);
				this->spellbonuses.FrenzyRadius += amount;
				break;
			case SE_Harmony:
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): *Harmony bonus of %i.", spell->GetSpellName(), amount);
				this->spellbonuses.Harmony += amount;
				break;
			case SE_Lull:
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): *Lull bonus of %i.", spell->GetSpellName(), amount);
				break;
			case SE_AttackSpeed:
			{
				//Yeahlight: Item haste is handled in Mob::GetItemHaste()
				if(isOnItem == false)
				{
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): *Attack speed bonus of %i.", spell->GetSpellName(), amount);
					//Yeahlight: Grab the spell's attack speed modifier
					int16 speedModifier = GetSpellAttackSpeedModifier(spell, casterlevel);
					//Yeahlight: Spell is a slow
					if(speedModifier < 100)
					{
						int16 slowAmount = BASE_MOD - speedModifier;
						//Yeahlight: Only store the highest slow percent present
						if(slowAmount > spellbonuses.DebuffAttackSpeed)
							spellbonuses.DebuffAttackSpeed = slowAmount;
					}
					//Yeahlight: Spell is a haste
					else if(speedModifier > 100)
					{
						int16 hasteAmount = speedModifier - BASE_MOD;
						//Yeahlight: Only store the highest haste percent present
						if(hasteAmount > spellbonuses.AttackSpeed)
							spellbonuses.AttackSpeed = hasteAmount;
					}
					//Yeahlight: An error occured while fetching the attack speed modifier
					else if(IsClient())
					{
						Message(RED, "ERROR: Spell %s returned a zero attack speed modifier", spell->GetSpellName());
					}
				}
				break;
			}
			case SE_AttackSpeed2:
			{
				//Yeahlight: Item haste is handled in Mob::GetItemHaste()
				if(isOnItem == false)
				{
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): *Attack speed bonus of %i.", spell->GetSpellName(), amount);
					//Yeahlight: Grab the spell's attack speed modifier
					int16 speedModifier = GetSpellAttackSpeedModifier(spell, casterlevel);
					//Yeahlight: Spell is a stacking haste
					if(speedModifier > 100)
					{
						int16 hasteAmount = speedModifier - BASE_MOD;
						//Yeahlight: Only store the highest stacking haste percent present
						if(hasteAmount > spellbonuses.StackingHaste)
							spellbonuses.StackingHaste = hasteAmount;
					}
					//Yeahlight: An error occured while fetching the attack speed modifier
					else if(IsClient())
					{
						Message(RED, "ERROR: Spell %s returned a zero attack speed modifier", spell->GetSpellName());
					}
				}
				break;
			}
			case SE_AttackSpeed3:
			{
				//Yeahlight: Item haste is handled in Mob::GetItemHaste()
				if(isOnItem == false)
				{
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): *Attack speed bonus of %i.", spell->GetSpellName(), amount);
					//Yeahlight: Grab the spell's attack speed modifier
					int16 speedModifier = GetSpellAttackSpeedModifier(spell, casterlevel);
					//Yeahlight: Spell is a stacking haste
					if(speedModifier > 100)
					{
						int16 hasteAmount = speedModifier - BASE_MOD;
						//Yeahlight: Only store the highest overhaste percent present
						if(hasteAmount > spellbonuses.OverHaste)
							spellbonuses.OverHaste = hasteAmount;
					}
					//Yeahlight: An error occured while fetching the attack speed modifier
					else if(IsClient())
					{
						Message(RED, "ERROR: Spell %s returned a zero attack speed modifier", spell->GetSpellName());
					}
				}
				break;
			}
			default:
			{
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::ApplySpellsBonuses: Effect untreated: %i for amount %i", i, effect_id, amount);
			}
		}
	}
}

///////////////////////////////////////////////////

void Mob::CalcSpellBonuses(StatsStruct* newbon) 
{
	//Pinedepain // bardMod initialization
	int8 bardMod = 0;

	for (int j = 0; j < 15; j++) 
	{
		//Pinedepain // If we're a client and a bard using a song, we need to check for the instrument mod
		if(IsClient() && IsBard() && buffs[j].spell && buffs[j].spell->IsValidSpell() && buffs[j].spell->IsBardSong())
			bardMod = CastToClient()->GetInstrumentModAppliedToSong(buffs[j].spell);

		if(buffs[j].spell)
			this->ApplySpellsBonuses(buffs[j].spell, buffs[j].casterlevel, newbon, bardMod, buffs[j].casterid);
	}
}

///////////////////////////////////////////////////

void Client::CalcItemBonuses(StatsStruct* newbon)
{
	const Item_Struct* item = 0;

	//Yeahlight: Iterate through each item slot
	for(int i = 1; i < 21; i++)
	{
		//Yeahlight: Grab the item from the DB
		item = Database::Instance()->GetItem(pp.inventory[i]);

		//Yeahlight: Item exists
		if(item != 0)
		{
			//Yeahlight: TODO: What does this mean?
			if(item->flag != 0x7669)
			{
				char buffer[5000];
				memset(buffer, 0, sizeof(buffer));
				sprintf(buffer, "Client::CalcItemBonuses(): bonus for %s:  AC: %i, HP: %i, Mana: %i, STR: %i, STA: %i, \
							 DEX: %i, AGI: %i, INT: %i, WIS: %i, CHA: %i, MR: %i, FR: %i, CR: %i, PR: %i, DR", item->name,
							 item->common.AC, item->common.HP, item->common.MANA, item->common.STR, item->common.STA, item->common.DEX,
							 item->common.AGI, item->common.INT, item->common.WIS, item->common.CHA, item->common.MR, item->common.FR,
							 item->common.CR, item->common.PR, item->common.DR);
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, buffer);
				newbon->AC += item->common.AC;
				newbon->HP += item->common.HP;
				newbon->Mana += item->common.MANA;
				newbon->STR += item->common.STR;
				newbon->STA += item->common.STA;
				newbon->DEX += item->common.DEX;
				newbon->AGI += item->common.AGI;
				newbon->INT += item->common.INT;
				newbon->WIS += item->common.WIS;
				newbon->CHA += item->common.CHA;
				newbon->MR += item->common.MR;
				newbon->FR += item->common.FR;
				newbon->CR += item->common.CR;
				newbon->PR += item->common.PR;
				newbon->DR += item->common.DR;
				//Yeahlight: Passive effect on the item
				if(item->common.click_effect_id != 0xFFFF && item->common.effecttype == 2)
					this->ApplySpellsBonuses(spells_handler.GetSpellPtr(item->common.click_effect_id), item->common.level, newbon, 0, 0, true);
			}
		}
	}
}

///////////////////////////////////////////////////

void Mob::TicProcess()
{
	for(int buffs_i = 0; buffs_i < BUFF_COUNT; buffs_i++)
	{
		if(buffs[buffs_i].spell && buffs[buffs_i].spell->IsValidSpell())
		{
			Spell* buff_spell = buffs[buffs_i].spell;
			if(buff_spell->IsValidSpell())
				DoBuffTic(buff_spell, buffs[buffs_i].ticsremaining, buffs[buffs_i].casterlevel, entity_list.GetMob(buffs[buffs_i].casterid));
			if(buffs[buffs_i].durationformula == BDF_Permanent)
				continue; // We ignore permanent ones.
			buffs[buffs_i].ticsremaining--;
			if(this->IsClient()) // Cofruben: we forgot to store the tics remaining in pp!
				this->CastToClient()->GetPlayerProfilePtr()->buffs[buffs_i].duration = buffs[buffs_i].ticsremaining;
			if(buffs[buffs_i].ticsremaining < 1) 
			{
				BuffFadeBySlot(buffs_i, true);
			}
		}
	}
}

///////////////////////////////////////////////////

void Mob::DoBuffTic(Spell* spell, int32 ticsremaining, int8 caster_level, Mob* caster)
{
	if (!spells_handler.SpellsLoaded() || !spell)
		return;
	if (spell->GetSpellID() == 0xFFFF)
		return;

	bool debugFlag = true;

	for (int i=0; i < 12; i++)
	{
		TSpellEffect effect_id	= spell->GetSpellEffectID(i);
		if(effect_id == SE_Blank)
			continue;
		int8 formula			= spell->GetSpellFormula(i);
		sint16 base				= spell->GetSpellBase(i);
		sint16 max				= spell->GetSpellMax(i);
		sint32 amount			= spells_handler.CalcSpellValue(spell, i, caster_level);
		switch(effect_id)
		{
			case SE_HealOverTime:
			{
				//Yeahlight: TODO: I'm not sure HoT messages were in classic, either; look into this
				if(amount < 0)
					entity_list.MessageClose(this, false, DEFAULT_MESSAGE_RANGE, DARK_BLUE, "%s was healed by \"%s\" spell for %d hit points.", this->GetName(), spell->GetSpellName(), abs(amount));
				break;
			}
			case SE_CurrentHP:
			{
				if(amount < 0)
				{
					//Yeahlight: Generate hate per tick
					if(this->IsNPC() && caster)
						this->CastToNPC()->AddToHateList(caster, abs(amount), abs(amount));
					//Yeahlight: Keep in mind, classic did NOT have DoT messages
					if(debugFlag)
						entity_list.MessageClose(this, false, DEFAULT_MESSAGE_RANGE, LIGHTEN_BLUE, "Debug: %s was hit by \"%s\" spell for %d points of damage.", this->GetName(), spell->GetSpellName(), abs(amount));
				}
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::DoBuffTic(): Effect #%i: Adding hp regeneration by %+i.", i, amount);
				break;
			}
			default:
			{
				// do we need to do anyting here?
			}
		}
	}
}

//////////////////////////////////////////////////

void Mob::BuffFade(Spell* spell)
{
	if(!spells_handler.SpellsLoaded())
		return;

	for(int j = 0; j < 15; j++) 
	{
		Spell* buff = buffs[j].spell;
		if(!spell) 
		{
			BuffFadeBySlot(j, true);
			memset(&buffs[j], 0, sizeof(buffs[j]));
			if(IsClient())	
				memset(&CastToClient()->GetPlayerProfilePtr()->buffs[j], 0, sizeof(CastToClient()->GetPlayerProfilePtr()->buffs[j]));
		}
		else if(spell && buff == spell && buff->IsValidSpell()) 
		{
			//Cofruben: this will take care of faded buffs //Yeahlight: Using EQEMU 7.0's fade protocol now
			BuffFadeBySlot(j, true);
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::BuffFade(spell_name = %s)", spell->GetSpellName());
			memset(&buffs[j], 0, sizeof(buffs[j]));
			//Cofruben: clear the buff from player profile.
			if(IsClient())	
				memset(&CastToClient()->GetPlayerProfilePtr()->buffs[j], 0, sizeof(CastToClient()->GetPlayerProfilePtr()->buffs[j]));
		}
	}
}

///////////////////////////////////////////////////

void Mob::CalcBonuses(bool refreshHaste, bool refreshItemBonuses)
{
	bool debugFlag = true;
	//1. Clean bonuses.
	memset(&spellbonuses, 0, sizeof(StatsStruct));
	//2. Refresh item bonuses.
	if(refreshItemBonuses)
	{
		memset(&itembonuses, 0, sizeof(StatsStruct));
		if(IsClient())
		{
			CastToClient()->CalcItemBonuses(&this->itembonuses);
		}
		else if(IsNPC())
		{
			;//Yeahlight: TODO: Calculate this NPC's item stats from loot table
		}
	}
	//3. Refresh spell bonuses.
	CalcSpellBonuses(&spellbonuses);
	//4. Refresh AC calculations
	CalculateACBonuses();
	//5. Refresh haste
	if(refreshHaste)
	{
		if(IsClient())
		{
			sint16 spellHaste = spellbonuses.AttackSpeed;
			sint16 debuffHaste = spellbonuses.DebuffAttackSpeed;
			sint16 stackingHaste = spellbonuses.StackingHaste;
			sint16 overHaste = spellbonuses.OverHaste;
			sint16 accumulatedHaste = 0;
			sint16 finalHaste = 0;
			sint16 hasteCap = 0;

			//Yeahlight: Any one slow will override all spell haste
			if(debuffHaste)
			{
				spellHaste = 0;
				stackingHaste = 0;
				overHaste = 0;
			}

			//Yeahlight: Calculate base haste
			accumulatedHaste = GetItemHaste() + spellHaste + stackingHaste;

			//Yeahlight: If accumulated haste is greater than 50, then check for haste caps
			if(accumulatedHaste > 50)
			{
				if(GetLevel() <= 30)
					hasteCap = 50;
				else if(GetLevel() <= 50)
					hasteCap = 75;
				else if(GetLevel() <= 54)
					hasteCap = 85;
				else if(GetLevel() <= 59)
					hasteCap = 95;
				else
					hasteCap = 100;
				if(accumulatedHaste > hasteCap)
					accumulatedHaste = hasteCap;
			}
			
			//Yeahlight: Calculate final haste (overHaste may break the haste cap)
			finalHaste = accumulatedHaste + overHaste - debuffHaste;

			//Yeahlight: Set the new haste value and attack speed timer
			CastToClient()->SetHaste(finalHaste);
			CastToClient()->SetAttackTimer();

			//Yeahlight: Debug messages
			if(debugFlag && IsClient() && CastToClient()->GetDebugMe())
				Message(LIGHTEN_BLUE, "Debug: Your current haste formula: %i = %i + %i(v1) + %i(v2) + %i(v3) - %i", finalHaste, GetItemHaste(), spellHaste, stackingHaste, overHaste, debuffHaste);
		}
		else if(IsNPC())
		{
			sint16 spellHaste = spellbonuses.AttackSpeed;
			sint16 debuffHaste = spellbonuses.DebuffAttackSpeed;
			sint16 stackingHaste = spellbonuses.StackingHaste;
			sint16 accumulatedHaste = 0;
			float attackSpeedModifier = 1.00f;

			//Yeahlight: Any one slow will override all spell haste
			if(debuffHaste)
			{
				spellHaste = 0;
				stackingHaste = 0;
			}

			//Yeahlight: Add up all the NPC haste elements
			accumulatedHaste = CastToNPC()->GetItemHaste() + spellHaste + stackingHaste - debuffHaste;

			//Yeahlight: Spell haste and slow require two distinct formulas
			//           TODO: I do not believe the slow calculation is correct
			if(accumulatedHaste >= 0)
				attackSpeedModifier = 100.0f / (100.0f + accumulatedHaste);
			else
				attackSpeedModifier = 1.00f + (float)((float)(abs(accumulatedHaste))) / 100.00f;

			//Yeahlight: Apply final haste to the current attack speed
			attack_timer->SetTimer(2000 * (float)((float)(baseAttackSpeedModifier)/100.00f) * attackSpeedModifier);
			attack_timer_dw->SetTimer(2000 * (float)((float)(baseAttackSpeedModifier)/100.00f) * attackSpeedModifier);
		}
	}

	CalcMaxHP();
	CalcMaxMana();
}

//////////////////////////////////////////////////