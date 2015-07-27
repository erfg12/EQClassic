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
#include "../common/debug.h"
#include "mob.h"
#include "client.h"
#include "groups.h"
#include "spdat.h"
#include "PlayerCorpse.h"
#include "../common/packet_dump.h"
#include "../common/moremath.h"
#include "worldserver.h"
#include "skills.h"
#include <math.h>
#include <assert.h>
#ifndef WIN32
//	#include <pthread.h>
#include <stdlib.h>
#include "../common/unix.h"
#endif
#ifdef _GOTFRAGS
	#include "../common/packet_dump_file.h"
#endif

extern Database database;
extern Zone* zone;
extern volatile bool ZoneLoaded;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern WorldServer worldserver;

void Mob::Spin() {
			APPLAYER* outapp = new APPLAYER(OP_Action,sizeof(Action_Struct));
			outapp->pBuffer[0] = 0x0B;
			outapp->pBuffer[1] = 0x0A;
			outapp->pBuffer[2] = 0x0B;
			outapp->pBuffer[3] = 0x0A;
			outapp->pBuffer[4] = 0xE7;
			outapp->pBuffer[5] = 0x00;
			outapp->pBuffer[6] = 0x4D;
			outapp->pBuffer[7] = 0x04;
			outapp->pBuffer[8] = 0x00;
			outapp->pBuffer[9] = 0x00;
			outapp->pBuffer[10] = 0x00;
			outapp->pBuffer[11] = 0x00;
			outapp->pBuffer[12] = 0x00;
			outapp->pBuffer[13] = 0x00;
			outapp->pBuffer[14] = 0x00;
			outapp->pBuffer[15] = 0x00;
			outapp->pBuffer[16] = 0x00;
			outapp->pBuffer[17] = 0x00;
			outapp->pBuffer[18] = 0xD4;
			outapp->pBuffer[19] = 0x43;
			outapp->pBuffer[20] = 0x00;
			outapp->pBuffer[21] = 0x00;
			outapp->pBuffer[22] = 0x00;
			outapp->pBuffer[23] = 0x00;
			outapp->priority = 5;
			CastToClient()->QueuePacket(outapp);
			delete outapp;
}

void Mob::SpellProcess() {
	if (casting_spell_id && !IsBardSong(casting_spell_id)) {
		float dist = (float)sqrt((float)pow(x_pos-GetSpellX(),2)+pow(y_pos-GetSpellY(),2));
		if (dist > 5) {
			isinterrupted = true;
		}
	}

	if(delaytimer == true && spellend_timer->Check()) {
		spellend_timer->Disable();
		delaytimer = false;
		return;
	}
	if (casting_spell_id != 0 && spellend_timer->Check()) {
		spellend_timer->Disable();

		Mob* tmob = entity_list.GetMob(casting_spell_targetid);
		if (tmob == 0) {
			InterruptSpell();
			return;
		}
		delaytimer = false;
		CastedSpellFinished(casting_spell_id, casting_spell_targetid,
                            casting_spell_slot, casting_spell_mana);
	}
}

// 0 = nomatch, 1=overwrite, -1 = stack failed
// spellid1 is the spell being cast, spellid2 is the buff being compared too
sint8 Mob::CheckEffectIDMatch(int8 effectindex, int16 spellid1,
                              int8 caster_level1, int16 spellid2,
                              int8 caster_level2) {
	// on a match we need to finish checking the rest of the for loops, on a fail theres no need
	if (effectindex >= 4)
		return 0;
	sint8 ret = 0;
	if (spells[spellid1].effectid[effectindex] == SE_StackingCommand1 || 
        spells[spellid1].effectid[effectindex] == SE_StackingCommand2) {
		// ok, it's a "do not stack" command, lets check all effectid's in spellid2
		for (int i=0; i<12; i++) {
			if (spells[spellid2].effectid[i] == spells[spellid1].base[effectindex]) {
				// it's a match, either overwrite or fail
				if (spells[spellid1].max[effectindex] >= 
                    CalcSpellValue(spells[spellid2].formula[i], 
                                   spells[spellid2].base[i], 
                                   spells[spellid2].max[i], 
                                   caster_level2, spellid2))
					ret = 1;
				else
					return -1;
			}
		}
	}
	else if (spells[spellid2].effectid[effectindex] == SE_StackingCommand1 ||
             spells[spellid2].effectid[effectindex] == SE_StackingCommand2) {
		// ok, it's a "do not stack" command, lets check all effectid's in spellid2
		for (int i=0; i<12; i++) {
			if (spells[spellid1].effectid[i] == spells[spellid2].base[effectindex]) {
				// it's a match, either overwrite or fail
				if (spells[spellid2].max[effectindex] < 
                    CalcSpellValue(spells[spellid1].formula[i], 
                                   spells[spellid1].base[i], 
                                   spells[spellid1].max[i], 
                                   caster_level1, spellid1))
					ret = 1;
				else
					return -1;
			}
		}
	}
	else {
//		if (effectindex >= 4)
//			return ret;
		if (spellid1 == spellid2 || 
            (spells[spellid1].effectid[effectindex] < 0xFE && 
             spells[spellid1].effectid[effectindex] == 
             spells[spellid2].effectid[effectindex] &&
             !(spells[spellid1].effectid[effectindex] == SE_CHA && 
               spells[spellid1].base[effectindex] == 0 && 
               spells[spellid1].formula[effectindex] == 100))) {
			if (CalcSpellValue(spells[spellid1].formula[effectindex], 
                               spells[spellid1].base[effectindex], 
                               spells[spellid1].max[effectindex], 
                               caster_level1, spellid1) >= 
                CalcSpellValue(spells[spellid2].formula[effectindex], 
                               spells[spellid2].base[effectindex], 
                               spells[spellid2].max[effectindex], 
                               caster_level2, spellid2))
				ret = 1;
			else
				return -1;
		}
	}
	return ret;
}

// returns -1 on stack failure, -2 if all slots full, the slot number if the buff should overwrite another buff, or a free buff slot
sint8 Mob::CanBuffStack(int16 spellid, int8 caster_level, bool iFailIfOverwrite) {
	if (CalcBuffDuration(caster_level, spells[spellid].buffdurationformula, 
                            spells[spellid].buffduration) <= 0) {
		LogFile->write(EQEMuLog::Debug,"CanBuffStack() CalcBuffDuration() <= 0");
		return BUFF_COUNT;
	}
	bool tmpBardSong = IsBardSong(spellid);
	sint8 firstfree = -2;
	int i, k;
	for (i=0; i < BUFF_COUNT; i++) {
		if (buffs[i].spellid == spellid) {
			if (caster_level >= buffs[i].casterlevel) {
				if (iFailIfOverwrite)
					return -1;
				else
					return i;
			}
			else
				return -1;
		}
		else if (buffs[i].spellid != 0xFFFF && 
                (IsBardSong(buffs[i].spellid) == tmpBardSong)) {
			sint8 ret = CheckEffectIDMatch(i, spellid, caster_level, 
                                           buffs[i].spellid, 
                                           buffs[i].casterlevel);
			if (ret == 1) {
			    return i;
			}
			else if (ret == -1)
			{
			    return -1;
			}
		}
		else if (firstfree == -2 && buffs[i].spellid == 0xFFFF)
		{
			firstfree = i;
		}
	}

	return firstfree;
}

void Mob::BuffFadeByEffect(int8 iEffectID, sint8 iButNotThisSlot, 
                           sint8 iBardSong, bool iRecalcBonuses) {
	bool tmpBardSong = (iBardSong == 1);
	for (int i=0; i < BUFF_COUNT; i++) {
		if (i != iButNotThisSlot && buffs[i].spellid != 0xFFFF && 
           (iBardSong == -1 || IsBardSong(buffs[i].spellid) == tmpBardSong)) {
			for (int k=0; k<12; k++) {
				if (spells[buffs[i].spellid].effectid[k] == iEffectID) {
					BuffFadeBySlot(i, false);
					break;
				}
			}
		}
	}
	if (iRecalcBonuses)
		CalcBonuses();
}

void Mob::SendSpellBarDisable(bool bEnable)
{
	if (!IsClient())
		return;

	if (!bEnable) {
		APPLAYER *outapp;

		outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
		MemorizeSpell_Struct* p = (MemorizeSpell_Struct*)outapp->pBuffer;
		p->slot = 0;
		p->spell_id = 0x2bc;
		p->scribing = 3;
		outapp->priority = 5;
		this->CastToClient()->QueuePacket(outapp);
		delete outapp;
	}
}

void Mob::CastSpell(int16 spell_id, int16 target_id, int16 slot,
                    sint32 cast_time, sint32 mana_cost, int32* oSpellWillFinish) {

	Mob* pMob = NULL;

	if (!spells_loaded)
	{
		Message(0, "Spells not loaded.");
		return;
	}
	if (spell_id >= SPDAT_RECORDS) {
		Message(0, "Invalid spell_id.");
		return;
	}

	if (bardsong) {
		LogFile->write(EQEMuLog::Debug, "BARD: disableing bardsong bardsong != 0");
		bardsong = 0;
		bardsong_timer->Disable();
		if (this->IsClient()) {
			SendSpellBarDisable(false);
			return;
		}
	}

	// better check such things in the beginning, not the end
	if (spellend_timer->Enabled()) {
		if(target_id == this->GetID()) {
			if (this->IsNPC()) {
//				this->CastToNPC()->AddQueuedSpell(casting_spell_id);
			}
		}
		if (this->IsClient()) Message(13, "You're already casting another spell!");
		//InterruptSpell();
		return;
	}

	if (spells[spell_id].targettype == ST_Self || 
        spells[spell_id].targettype == 3 || // (velns) what is 3?
        spells[spell_id].targettype == ST_AECaster || 
        spells[spell_id].targettype == ST_Group || 
        spells[spell_id].targettype == ST_AEBard || 
        (spells[spell_id].targettype == ST_TargetOptional && target_id == 0)) {

		target_id = GetID();
    } else if(target_id == 0) {

		if (this->IsClient()) Message(13, "Error: Spell requires a target.");
		InterruptSpell();
		return;
	}

	if (mana_cost == -1)
		mana_cost = spells[spell_id].mana;

	// neotokyo: 19-Nov-02
	// mana is checked for clients on the frontend. we need to recheck it for NPCs though
	// fix: items dont need mana :-/
	// Quagmire: If you're at full mana, let it cast even if you dont have enough mana
 	if (
		mana_cost != 0
		&& slot < 10
		&& !(
			this->GetMana() >= mana_cost
			|| (GetMana() == GetMaxMana() && (GetMaxMana() > 0 || IsAIControlled()))
			)
		) {
		Message(13, "Error: Insufficent Mana.");
		InterruptSpell();
		SetMana(GetMana());
		return;
	}
	if (IsBardSong(spell_id)) {
		LogFile->write(EQEMuLog::Normal, "BARD: starting bard song (1)");
		bardsong = spell_id;
		bardsong_timer->Start(6000);
	}

	pMob = entity_list.GetMob(target_id);
	if(!pMob) {
		if (this->IsClient()) Message(13, "Error aquiring target.");
		InterruptSpell();
		return;
	}

	int32 modrange =(int32)spells[spell_id].range;
	if (pMob != this && Dist(pMob) > spells[spell_id].range) {
	
		modrange = GetActSpellRange(spell_id, (int)(spells[spell_id].range));
	
		if (modrange < Dist(pMob) && 
            !(spells[spell_id].targettype == ST_AlterPlane && IsNPC())) {
		
			if(IsClient() && spells[spell_id].targettype == ST_AlterPlane) {
				CastToClient()->Message(MT_Spells, 
                    "You can only cast AlterPlane spells on yourself.");
            } else if (IsClient() && !(spells[spell_id].targettype == ST_AECaster)) {
				CastToClient()->Message(MT_Spells, 
                    "Your target is out of range(2)!");
            }

			InterruptSpell();
			return;
		}

	}

	if (this->IsAIControlled()) {
		SetRunAnimSpeed(0);
	    this->FaceTarget(pMob, true);
	}

	if (cast_time == -1)
	    cast_time = spells[spell_id].cast_time;

	int32 orgcasttime = cast_time;

	if (cast_time && IsClient()) {
		cast_time = GetActSpellCasttime(spell_id, cast_time);
	}

	if (cast_time == 0) {
		SpellFinished(spell_id, target_id, slot, mana_cost);
		return;
	}

	pDontCastBefore_casting_spell = oSpellWillFinish;
	if (oSpellWillFinish)
		*oSpellWillFinish = Timer::GetCurrentTime() + cast_time + 100;

/////////////////////////////////////////////////////////////
/////////////Client begins to cast a spell
////////////////////////////////////////////////////////////	
	APPLAYER* outapp = new APPLAYER;

	outapp->opcode = OP_BeginCast;
	outapp->size = sizeof(BeginCast_Struct);
	outapp->pBuffer = new uchar[outapp->size];
	memset(outapp->pBuffer, 0, outapp->size);
	BeginCast_Struct* begincast = (BeginCast_Struct*)outapp->pBuffer;
	begincast->caster_id = GetID();
	begincast->spell_id = spell_id;
	begincast->cast_time = orgcasttime; // client calculates reduced time by itself
	outapp->priority = 3;
	entity_list.QueueCloseClients(this, outapp);
	delete outapp;
	
	casting_spell_id = spell_id;
	casting_spell_targetid = target_id;
	casting_spell_slot = slot;

	casting_spell_mana = mana_cost;

	spellend_timer->Start(cast_time);
	
	if(slot < 10 && !CheckFizzle(spell_id)) {
        if ( IsBardSong(spell_id) ) {
		    InterruptSpell(0xB4, 0x121); /* wrong note */
		    bardsong = 0;
		    bardsong_timer->Disable();
        } else {
		    InterruptSpell(173, 0x121); /* 173 == Your spell fizzles */
        }
    } else if (IsClient()) {
		char itemname[65];
		int16 focusspell;
		sint32 dmg = 0;

		// Mana cost
		if (slot < 10 && 
            CastToClient()->GetReduceManaCostItem(focusspell, itemname))
			CastToClient()->Message(MT_Spells, "Your %s begins to glow.", itemname);

		// spell range
		if (modrange != spells[spell_id].range) {
			CastToClient()->GetExtendedRangeItem(focusspell, itemname);
			CastToClient()->Message(MT_Spells, "Your %s begins to glow.", itemname);
		}

		// buff duration
		int16 buffdur = CalcBuffDuration(GetLevel(), 
                                         spells[spell_id].buffdurationformula, 
                                         spells[spell_id].buffduration);
		int32 newdur = CastToClient()->GetActSpellDuration(spell_id, buffdur);
		if (newdur != buffdur) {
			CastToClient()->GetIncreaseSpellDurationItem(focusspell, itemname);
			CastToClient()->Message(MT_Spells, "Your %s begins to glow.", itemname);
		}

		// Improved Healing and Damage
		for (int i = 0; i < EFFECT_COUNT; i++) {
			if (spells[spell_id].effectid[i] == SE_CurrentHP) {
				dmg = CalcSpellValue(spells[spell_id].formula[i], 
                                     spells[spell_id].base[i], 
                                     spells[spell_id].max[i], 
                                     GetLevel(), spell_id);
				break;
			}   
		}
        
		if ( dmg < 0) {
			sint32 newdmg = CastToClient()->GetActSpellValue(spell_id, dmg);
			if (newdmg != dmg) {
				CastToClient()->GetImprovedDamageItem(focusspell, itemname);
				CastToClient()->Message(MT_Spells, "Your %s begins to glow.", itemname);
			}
		}

		else if (dmg > 0) {
			sint32 newdmg = CastToClient()->GetActSpellValue(spell_id, dmg);
			if (newdmg != dmg) {
				CastToClient()->GetImprovedHealingItem(focusspell, itemname);
				CastToClient()->Message(MT_Spells, "Your %s begins to glow.", itemname);
			}
		}

		// CastTime
		if (orgcasttime != cast_time) {
			CastToClient()->GetReduceCastTimeItem(focusspell, itemname);
			CastToClient()->Message(MT_Spells, "Your %s begins to glow.", itemname);
		}

		// spell hate
		// reagent cost
		// Check for any required items
		if (IsClient()){
		    if (DEBUG>=1&&CastToClient()->GetGM()) Message(0, "Checking for spell regents.");
		    // Check for consumables
		    for (uint8 t_count = 0; t_count <= 3 ; t_count++){
			if (spells[spell_id].components[t_count] != -1){
				// Consumed components
				if (CastToClient()->HasItemInInventory(
                        spells[spell_id].components[t_count], 
                        spells[spell_id].component_counts[t_count]) == -1) {
					// FIXME find items not all in one stack
					// needs new function or something
					if(DEBUG>=1 && CastToClient()->GetGM()) {
                        Message(0, "Missing item: %i", 
                                spells[spell_id].components[t_count]);
                    }

					Message(MT_Spells, 
                        "Sorry you don't have the required items, perhaps it's time you visited town.");
					if(!CastToClient()->GetGM()) {
						InterruptSpell(spell_id);
						return;
					}
				}
				else {
					// Components found Deleteing
					uint32 slot_id = CastToClient()->HasItemInInventory(
                            spells[spell_id].components[t_count], 
                            spells[spell_id].component_counts[t_count]);
					if(DEBUG>=1&&CastToClient()->GetGM()) 
                        Message(0, "Component found at slot:%i count:%i", slot_id, 
                                spells[spell_id].component_counts[t_count]);
				}
            } else if (spells[spell_id].NoexpendReagent[t_count] != -1){
			    // Focus item
				if ( !CastToClient()->HasItemInInventory(
                        spells[spell_id].NoexpendReagent[t_count])) {
					if(DEBUG>=1&&CastToClient()->GetGM()) 
                        Message(0, "Missing focus item: %i", 
                                spells[spell_id].NoexpendReagent[t_count]);
                } else {
				    // Handle focus item effects
				}
			}
		    }
		} // End of client Component checks
	}
	SaveSpellLoc();
}

/*
solar: returns true if spell is successful, false if it fizzled.
only works for clients, npcs shouldn't be fizzling..
neotokyo: new algorithm thats closer to live eq (i hope)
kathgar TODO: Add aa skills, item mods, reduced the chance to fizzle and gm's don't fizzle
*/
bool Mob::CheckFizzle(int16 spell_id) {
	if ( !this->IsClient() )
        return 1;
	if (this->CastToClient()->Admin() > 100) return 1;
	assert(spell_id < SPDAT_RECORDS);

	// neotokyo: this is my try to get something going
	int par_skill;
	int act_skill;
	
	par_skill = spells[spell_id].classes[GetClass()-1] * 5 - 10;//IIRC even if you are lagging behind the skill levels you don't fizzle much
	/*par_skill = spells[spell_id].classes[GetClass()-1] * 5 + 5;*/
	if (par_skill > 235)


		par_skill = 235;
	par_skill += spells[spell_id].classes[GetClass()-1]; // maximum of 270 for level 65 spell

	act_skill = GetSkill(spells[spell_id].skill);
	act_skill += this->CastToClient()->GetLevel(); // maximum of whatever the client can cheat

	// == 0 --> on par
	// > 0  --> skill is lower, higher chance of fizzle
	// < 0  --> skill is better, lower chance of fizzle
	// the max that diff can be is +- 235
	int diff = par_skill + spells[spell_id].basediff - act_skill;

	// if you have high int/wis you fizzle less, you fizzle more if you are stupid
	if (GetCasterClass() == 'W')
		diff -= (GetWIS() - 125) / 20;
	if (GetCasterClass() == 'I')
		diff -= (GetINT() - 125) / 20;

	// base fizzlechance is lets say 5%, we can make it lower for AA skills or whatever
	int basefizzle = 10;
	int fizzlechance = basefizzle + diff/5;

	if (fizzlechance < 5)
		fizzlechance = 5; // let there remain some chance to fizzle
	if (fizzlechance > 95)
		fizzlechance = 95; // and let there be a chance to succeed

	

	LogFile->write(EQEMuLog::Normal,
                   "Check Fizzle (%d) [%d]", fizzlechance, diff);
	if (rand()%100 > fizzlechance)
		return 1;
	return 0;
}

void Mob::InterruptSpell(int16 spellid) {
	if (spellid == 0xFFFF)
		spellid = casting_spell_id;
	int16 spelltype = 0x01b7;
	if (IsBardSong(spellid)) spelltype = 0x00B4;
	InterruptSpell(spelltype, 0x0121, spellid);
}

void Mob::InterruptSpell(int16 message, int16 color, int16 spellid) {
	if (spellid == 0xFFFF)
		spellid = casting_spell_id;
	if (spellid != 0) {
		AI_Event_SpellCastFinished(false, casting_spell_slot);
		spellend_timer->Disable();
		    if (IsBardSong(spellid)) {
			    entity_list.MessageClose(this, true, 600, MT_Spells, 
                        "A missed note brings %s's song to a close!",
                        this->GetName());
            } else {
			    entity_list.MessageClose(this, true, 600, MT_Spells,
                        "%s's casting has been interrupted!",
                        this->GetName());
            }

/* whatever this was good for ... dont do it anymore
		if(this->IsNPC() && spells[casting_spell_id].resisttype)
		{
			this->CastToNPC()->AddQueuedSpell(casting_spell_id);
		}
*/

		if (this->IsClient()) {
			APPLAYER* outapp = new APPLAYER(OP_InterruptCast, 
                                            sizeof(InterruptCast_Struct));
			InterruptCast_Struct* ic = (InterruptCast_Struct*) outapp->pBuffer;
			ic->message = message;
			ic->color = color;
			this->CastToClient()->QueuePacket(outapp);

//			DumpPacket(outapp);

			delete outapp;

//            SendSpellBarDisable(false);

			outapp = new APPLAYER;
			outapp->opcode = OP_ManaChange;
			outapp->size = sizeof(ManaChange_Struct);
			outapp->pBuffer = new uchar[outapp->size];
            memset(outapp->pBuffer, 0, outapp->size);
			ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
			manachange->new_mana = GetMana();
			manachange->spell_id = spellid ;//0x0120; // cancel spell
			outapp->priority = 5;
			this->CastToClient()->QueuePacket(outapp);
			delete outapp;
		}
		casting_spell_id = 0;
		delaytimer = false;
    } else {
				//LogFile->write(EQEMuLog::Error, "Interrupting casting while casting_spell_id == 0");
    }
    isattacked = false;
	isinterrupted = false;
}

bool Mob::IsBardSong(int16 spell_id) {
    if ( spell_id < SPDAT_RECORDS && spells[spell_id].classes[BARD - 1] < 255)
        return true;
    return false;
}

// only to be used from SpellProcess
void Mob::CastedSpellFinished(int16 spell_id, int32 target_id,
                              int16 slot, int16 mana_used) {
	if (spell_id >= SPDAT_RECORDS) {
		Message(0, "Invalid spell_id.");
		return;
	}
	if (spell_id == 0)
		return;
	if (!spells_loaded)
		Message(0, "Spells not loaded.");
	if(delaytimer == true && IsClient()) {
		Message(10,"You are unable to focus.");
		InterruptSpell();
		casting_spell_id = 0;
		return;
	}
	if(IsClient()) {
		delaytimer = true;
		spellend_timer->Start(800,true);
	}
	//APPLAYER* outapp = 0;
	if (this->isinterrupted) {
		this->InterruptSpell();
		return;
	}
	if (this->isattacked) {
		if (this->IsClient()){
			this->Message(MT_Spells, 
                    "You regain your concentration and continue your casting!");
			if (slot < 10)
				this->CastToClient()->CheckIncreaseSkill(CHANNELING);
        } else
			entity_list.MessageClose(this, true, 300, MT_Spells,
                    "%s's regains concentration and continues casting!",
                    this->GetName());
	}
	isattacked = false;
	if (casting_spell_id != spell_id) {
		Message(13, "You're already casting another spell!");
		this->InterruptSpell();
		return;

	}
		// Check for any required items
		if (IsClient()){
		    if (DEBUG>=1&&CastToClient()->GetGM())
                Message(0, "Checking for spell regents.");
		    // Check for consumables
		    for (uint8 t_count = 0; t_count <= 3 ; t_count++){
			if (spells[spell_id].components[t_count] != -1){
				// Consumed components
				if (CastToClient()->HasItemInInventory(
                        spells[spell_id].components[t_count],
                        spells[spell_id].component_counts[t_count]) == -1) {
					// FIXME find items not all in one stack
					// needs new function or something
					if(DEBUG>=1 && CastToClient()->GetGM()) {
                        Message(0, "Missing item: %i",
                                spells[spell_id].components[t_count]);
                    }

					Message(MT_Spells, 
                        "Sorry you don't have the required items, perhaps it's time you visited town.");
					if(!CastToClient()->GetGM()) {
						InterruptSpell(spell_id);
						return;
					}
                } else {
					// Components found Deleteing
					uint32 slot_id = CastToClient()->HasItemInInventory(
                            spells[spell_id].components[t_count], 
                            spells[spell_id].component_counts[t_count]);
					if(DEBUG>=1&&CastToClient()->GetGM()) {
                        Message(0, "Component found at slot:%i count:%i",
                                slot_id, 
                                spells[spell_id].component_counts[t_count]);
                    }

					if(!CastToClient()->GetGM()) {
						CastToClient()->DeleteItemInInventory(
                                slot_id,
                                spells[spell_id].component_counts[t_count],
                                true);
                    }
				}
            } else if (spells[spell_id].NoexpendReagent[t_count] != -1){
			    // Focus item
				if (!CastToClient()->HasItemInInventory(
                    spells[spell_id].NoexpendReagent[t_count])) {
					if(DEBUG>=1&&CastToClient()->GetGM()) {
                        Message(0, "Missing item: %i", 
                                spells[spell_id].NoexpendReagent[t_count]);
                    }

					//Message(MT_Spells, "Sorry you don't have the required items, perhaps it's time you visited town.");
					//if(!CastToClient()->GetGM()) {
					//	InterruptSpell(spell_id);
					//	return;
					//}
                    //
                } else {
				    // Handle focus item effects (TODO) (velns)
				}
			}
		    }
		} // End of client Component checks

	casting_spell_id = 0;
	SpellFinished(spell_id, target_id, slot, mana_used);
	if (this->IsClient()) 
		this->CastToClient()->CheckIncreaseSkill(spells[spell_id].skill);
}

// only used from CastedSpellFinished, unless the spell is a proc/ZeroCastTime
void Mob::SpellFinished(int16 spell_id, int32 target_id,
                        int16 slot, int16 mana_used) {
	AI_Event_SpellCastFinished(true, slot);
	if (spell_id >= SPDAT_RECORDS) {
		Message(0, "Invalid spell_id.");
		return;
	}

	if (spell_id == 0)
		return;
	if (!spells_loaded)

		Message(0, "Spells not loaded.");
	if (this->IsClient() && 
        zone->GetZoneID() == 184 && 
        CastToClient()->Admin() < 80) {
		if (IsEffectInSpell(spell_id, SE_Gate) || 
            IsEffectInSpell(spell_id, SE_Translocate) || 
            IsEffectInSpell(spell_id, SE_Teleport)) {
			Message(0, "The Gods brought you here, only they can send you away.");
			InterruptSpell(spell_id);
			return;
		}
	}
    if (slot < 10) {
		if (this->IsClient())
			mana_used = GetActSpellCost(spell_id, mana_used);
		SetMana(GetMana() - mana_used);
    }
	APPLAYER* outapp = 0;
	if (!IsBardSong(casting_spell_id) && !bardsong && this->IsClient()) {
		/* Tell client it can cast again */
//			SendSpellBarDisable(false);
		outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
		ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
		manachange->new_mana = GetMana();
		manachange->spell_id = spell_id;
		outapp->priority = 5;
		this->CastToClient()->QueuePacket(outapp);
		delete outapp;
	}
	if (this->IsClient()) {
		/* Dunno why this goes here, but it does */
		// Quagmire - Think this is to grey the memorized spell icons?
        // (TODO) Maybe delete spell from inventory here? (velns)
		if (slot <= 9) {
			outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
			MemorizeSpell_Struct* memspell = (MemorizeSpell_Struct*)outapp->pBuffer;
			memspell->slot = slot;
			memspell->spell_id = spell_id;
			memspell->scribing = 3;
			outapp->priority = 5;
			this->CastToClient()->FastQueuePacket(&outapp);
		}
	}
	if(!target_id)
		target_id = this->GetID();

	/* Client animation */
	outapp = new APPLAYER(OP_Attack, sizeof(Attack_Struct));
	Attack_Struct* a = (Attack_Struct*)outapp->pBuffer;
	a->spawn_id = GetID();
	a->a_unknown1 = spells[spell_id].TargetAnim; // if this is not zero, the animation will not occur.
	a->type = spells[spell_id].CastingAnim;
	a->a_unknown2[5] = 0x80;
	a->a_unknown2[6] = 0x3f;
	outapp->priority = 2;
	entity_list.QueueCloseClients(this, outapp);
	delete outapp;

	if (spell_id == 1948 && 
       (this->IsClient() && (this->CastToClient()->Admin() <= 79))) { // Destroy
		// This spell is lame!
		return;
	}

	//	if (target->CastToClient()->GetPVP() && /*TargetSelf*/(target->GetID() == this->GetID()) || /*TargetAlsoPVP*/(target->CastToClient()->pp.pvp == 1 && this->CastToClient()->pp.pvp == 1) || /*TargetPetAlsoPVP*/(target->GetOwner()->CastToClient()->pp.pvp == 1 && this->CastToClient()->pp.pvp == 1) || /*TargetNonPVP*/(target->CastToClient()->pp.pvp == 0 && this->CastToClient()->pp.pvp == 0) || /*TargetPetNonPVP*/(target->GetOwner()->CastToClient()->pp.pvp == 0 && this->CastToClient()->pp.pvp == 0) || /*TargetNPC*/(target->IsNPC())) {
	//Message(0, "I dont know that TargetType. 0x%x", spells[spell_id].targettype);
    //
	// neotokyo: 09-Nov-02
	// Recourse means there is a spell linked to that spell in that the recourse spell will
	// be automatically casted on the casters group or the caster only depending on Targettype
	if (spells[spell_id].RecourseLink != 0) {
		if (spells[spells[spell_id].RecourseLink].targettype != ST_Self && 
            this->IsClient() && this->CastToClient()->isgrouped && 
            entity_list.GetGroupByClient(this->CastToClient()) != 0) {

			//entity_list.GetGroupByClient(this->CastToClient())->CastGroupSpell(this->CastToMob(),spells[spell_id].RecourseLink);
 
			entity_list.AESpell(this, this, spells[spell_id].aoerange,
                                spells[spell_id].RecourseLink);

			//AESpell(Mob* caster, Mob* center, float dist, int16 spell_id, bool group)
           
		}
		if (spells[spells[spell_id].RecourseLink].targettype == ST_Self ||
           (this-IsClient() && !this->CastToClient()->isgrouped))
			SpellOnTarget(spells[spell_id].RecourseLink,this);
	}
	
	switch (spells[spell_id].targettype)
	{
		case ST_TargetOptional: {
			Mob* tar = entity_list.GetMob(target_id);
			if (tar)
				SpellOnTarget(spell_id, tar);
			else
				SpellOnTarget(spell_id, this);
			break;
		}
		case ST_Undead:
		case ST_Animal:
		case ST_Plant:
		case ST_Dragon:
		case ST_Giant:
		case ST_Summoned:
		case ST_Tap:
		case ST_Target: {
			Mob* tar = entity_list.GetMob(target_id);
			if (tar)
				SpellOnTarget(spell_id, tar);
			else
				Message(13, "Error: Spell requires a target");
			break;
		}
		case ST_Self: {

			SpellOnTarget(spell_id, this);
			break;
		}
		case ST_AECaster: {
			entity_list.AESpell(this, this, spells[spell_id].aoerange, spell_id);
			break;
		}
		case ST_AETarget: {
			Mob* tar = entity_list.GetMob(target_id);
			if (tar)
				entity_list.AESpell(this, tar,
                                    spells[spell_id].aoerange, spell_id);
			else
				Message(13, "Invalid target for an AETarget spell");
			break;
		}
		case ST_AEBard: {
			SpellOnTarget(spell_id,this);
			entity_list.AESpell(this, this,
                                spells[spell_id].aoerange, spell_id);
			break;
		}
		case ST_Group: {
#if 0
		    Mob* tar = entity_list.GetMob(target_id);
			if (this->IsClient() ||
               (this->IsNPC() && this->CastToNPC()->IsInteractive()) ) {
               // Spell cast by IPC or Client
			    // FIXME check MGB
			    if (  this->CastToClient()->isgrouped
                   && entity_list.GetGroupByClient(this->CastToClient()) != 0
                   && entity_list.GetGroupByClient(this->CastToClient())->IsGroupMember(tar)
                   ){
                   // Group spell cast by IPC/Client on thier group
 			        entity_list.AESpell(this, this, spells[spell_id].aoerange, spell_id, true);
 			        if (!this->IsNPC()) {
                        SpellOnTarget(spell_id,this);
                    } else if (this->IsNPC()) {
                        if (IsEffectInSpell(spell_id, SE_Gate)
                            || IsEffectInSpell(spell_id, SE_Translocate)
                            || IsEffectInSpell(spell_id, SE_Teleport)
                            || IsEffectInSpell(spell_id, SE_Succor)) {
                            this->CastToNPC()->Depop();
                        } else {
                            SpellOnTarget(spell_id,this);
                        }
                    }
				}
			    else if (   this->IsClient()
                         && this->CastToClient()->TGB()
                         ){
                         //Spell cast by client on another group
                         //AESpell(Mob* caster, Mob* center, float dist, int16 spell_id, bool group) {
                         entity_list.AESpell(this, tar, spells[spell_id].aoerange, spell_id, true);
                         SpellOnTarget(spell_id, this);
                }
			    if (this->IsClient() && !this->CastToClient()->isgrouped && !this->CastToClient()->TGB()) {
				    SpellOnTarget(spell_id,this);
                }
				if (tar != this && this->CastToClient()->TGB()){
				}
			}
			else { // Group spell cast by NPC
			   SpellOnTarget(spell_id, this);
			}
			break;
#else // Never
            Mob* trg = this; 
            if (IsClient() && CastToClient()->TGB() && target && target->IsClient() && target->CastToClient()->isgrouped) 
            { 
               trg = target; 
               SpellOnTarget(spell_id,this); 
            } 
            if (trg->IsClient() && trg->CastToClient()->isgrouped && entity_list.GetGroupByClient(trg->CastToClient()) != 0) 
               entity_list.GetGroupByClient(trg->CastToClient())->CastGroupSpell(this->CastToClient(),spell_id); 
            else 
               SpellOnTarget(spell_id,this); 
            break; 
#endif
		}
		case ST_Pet: {
			if (this->GetPetID() != 0) {
				SpellOnTarget(spell_id, entity_list.GetMob(this->GetPetID()));
				if (spell_id == 331) {
					Mob* mypet = entity_list.GetMob(this->GetPetID());
					if (mypet && mypet->IsNPC() && mypet->GetPetType() != 0xFF) {
						mypet->CastToNPC()->Depop();
						this->SetPet(0);
					}
				}
            } else {
				Message(13, "You don't have a pet to cast this on!");
			}
			break;
		}
		case ST_Corpse: {
			Mob* tar = entity_list.GetMob(target_id);
			if(!(tar->IsPlayerCorpse())) {
				Message(13, "Target is not a corpse.");
				break;
			}
			SpellOnTarget(spell_id, tar);
			break;
		}
		case ST_AlterPlane: {
			if (IsNPC() && !this->CastToNPC()->IsInteractive()) {
				SpellOnTarget(spell_id,target);
            } else if (IsNPC() && CastToNPC()->IsInteractive()) {
				entity_list.AESpell(this, this, spells[spell_id].aoerange,
                                    spell_id);
				//SpellOnTarget(spell_id,this);
				CastToNPC()->Depop();
            } else if (this->IsClient() &&
                       this->CastToClient()->isgrouped &&
                       entity_list.GetGroupByClient(this->CastToClient()) != 0) {
				//entity_list.GetGroupByClient(this->CastToClient())->CastGroupSpell(this->CastToMob(),spell_id);
				entity_list.AESpell(this, this, spells[spell_id].aoerange, spell_id);
				SpellOnTarget(spell_id,this);
            } else if (this-IsClient() && !this->CastToClient()->isgrouped) {
				SpellOnTarget(spell_id,this);
			}
			break;
		}
		default: {
			Message(0, "I dont know that TargetType. 0x%x", spells[spell_id].targettype);
			break;
		}
	}
	//	}
	//	else {
	//		Message(13, "Your spell was unable to take hold.");
	//	}
}

bool IsMezSpell(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ << ", IsMezSpell(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}
	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_Mez)
			return true;
	}
    return false;
}


bool IsPercentalHeal(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsPercentalHeal(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_PercentalHeal)
			return true;
	}
    return false;
}

bool IsDetrimental(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {

		cerr << "Error in " << __FILE__ <<
                ", IsDetrimental(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}
    return spells[spell_id].goodEffect == 0;
}

bool IsBeneficial(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsBeneficial(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}
    return spells[spell_id].goodEffect != 0;
}

bool IsInvulnerability(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsInvulnerability(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}
	for (int i = 0; i < EFFECT_COUNT; i++) {
	    if (spells[spell_id].effectid[i] == SE_DivineAura) {
		    return true;
        }
	}
    return false;
}

bool IsCHDuration(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ << ", IsCHDuration(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_CompleteHeal)
			return true;
	}
    return false;
}

bool IsPoisonCounter(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsPoisonCounter(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_PoisonCounter)
			return true;
	}
    return false;
}

bool IsDiseaseCounter(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsDiseaseCounter(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}



	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_DiseaseCounter)
			return true;
	}
    return false;
}

bool IsSummonItem(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsSummonItem(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_SummonItem)
			return true;
	}
    return false;
}


bool IsSummonSkeleton(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsSummonSkeleton(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_NecPet)
			return true;
	}
    return false;
}

bool IsReduceCastTimeSpell(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsReduceCastTimeSpell(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}
	
	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_IncreaseSpellHaste)
			return true;
	}

    return false;
}

bool IsIncreaseDurationSpell(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsIncreaseDurationSpell(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_IncreaseSpellDuration)
			return true;
	}

    return false;
}

bool IsSummonPet(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsSummonPet(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_SummonPet ||
            spells[spell_id].effectid[i] == SE_SummonBSTPet )
			return true;
	}

    return false;
}

bool IsEffectHitpoints(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsEffectHitpoints(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}
	
	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_CurrentHP)
			return true;
	}

    return false;
}

bool IsReduceManaSpell(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsReduceManaSpell(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_ReduceManaCost)
			return true;
	}

    return false;
}

bool IsExtRangeSpell(int16 spell_id) {
#ifdef _GOTFRAGS
	FilePrintLine("IsExtRangeSpell-Debug.txt", true, "spell_id=%d", spell_id);
#endif
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsExtRangeSpell(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_IncreaseRange)
			return true;
	}

return false;
}

bool IsImprovedHealingSpell(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsImprovedHealingSpell(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	for (int i = 0; i < EFFECT_COUNT; i++) {
		if (spells[spell_id].effectid[i] == SE_ImprovedHeal)
			return true;
	}

    return false;
}

bool IsImprovedDamageSpell(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsImprovedDamageSpell(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

    for (int i = 0; i < EFFECT_COUNT; i++) {
        if (spells[spell_id].effectid[i] == SE_ImprovedDamage)
            return true;
    }

    return false;
}

bool IsCharmSpell(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ << ", IsCharmSpell(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

    for (int i = 0; i < EFFECT_COUNT; i++)
    {
        if (spells[spell_id].effectid[i] == SE_Charm)
            return true;
    }

    return false;
}

bool IsMesmerizeSpell(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ <<
                ", IsMesmerizeSpell(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

    for (int i = 0; i < EFFECT_COUNT; i++)
    {
        if (spells[spell_id].effectid[i] == SE_Mez)
            return true;
    }

    return false;
}

bool Mob::IsMezzable(int16 spell_id) {
	if (spell_id >= SPDAT_RECORDS) {
		cerr << "Error in " << __FILE__ << ", IsMezzable(): spell_id >= SPDAT_RECORDS" << endl;
		return false;
	}

	if(this->IsNPC() && this->CastToNPC()->HasBanishCapability() == 101)
		return false;

    for (int i = 0; i < EFFECT_COUNT; i++) {
        if (spells[spell_id].effectid[i] == SE_Mez) {
            // mez spells seem to have max level encoded into max[effect]
            if (spells[spell_id].max[i] < GetLevel())
                return false;
            else
                return true;
        }
    }
    return true;
}

// -1 = immune, -2 = immune and we already sent the message
sint16 Mob::ResistSpell(int16 spell_id, Mob* caster) 
{
	// assume same level as target, if we dont have a caster entity
	int level = GetLevel();
	if (caster)
		level = caster->GetLevel();
	float resist = 0;
		sint16 diff = GetLevel() - caster->GetLevel();
	if (IsCharmSpell(spell_id)) 
	{
		if(diff > 2 && caster && caster->IsClient() && IsNPC()) 
		{
			caster->Message(13, "This NPC is too high of level for you to charm.");
			return -2;
        } 
		else if (diff > 2 || (IsClient() && CastToClient()->GetGM() &&
                   !(caster && caster->IsClient() && 
                     caster->CastToClient()->GetGM()))) 
		{
			caster->Message(13, "This target is uncharmable.");
			return -2;
		}
	}
	if (IsMesmerizeSpell(spell_id)) 
	{
		if(diff > 4 && caster && caster->IsClient() && IsNPC()) 
		{
			caster->Message(13, "This NPC is too high of level for you to mesmerize.");
			return -2;
        } 
		else if (diff > 2 || (IsClient() && CastToClient()->GetGM() && 
                   !(caster && caster->IsClient() &&
                     caster->CastToClient()->GetGM()))) 
		{
			caster->Message(13, "This target cannot be mesmerized.");
			return -2;
		}
	}

	/* check resists for all "bad" spells */
	if (spells[spell_id].goodEffect == 0) 
	{
		switch(spells[spell_id].resisttype) 
		{
	
			case RESIST_NONE:
				break;
			case RESIST_MAGIC:
				resist = GetMR(); break;
			case RESIST_FIRE:
				resist = GetFR(); break;
			case RESIST_COLD:
				resist = GetCR(); break;
			case RESIST_POISON:
				resist = GetPR(); break;
			case RESIST_DISEASE:
				resist = GetDR(); break;
			default:
				LogFile->write(EQEMuLog::Normal, "Unknown Resist type: %d",
                           spells[spell_id].resisttype);
		}
        
		if (spells[spell_id].resisttype != RESIST_NONE) 
		{
			bool partial = true;
		/*{
			bool partial = false;//partial gets defined and set here and no where else.. so you never get a partial resist? half of the function is useless then */

		// 360 for a level 60 entity means 100% of resisting
		// npcs will by default have 1.5 * Level as resist if not defined otherwise in database
		// so in level 60 a default npc will have 90 resist thus a 25% chance to evade damage
			float maxchance = GetLevel() * 6;
            
		// we get additional resist if this->Level > other->Level
		// on the same side we lose resists if we are of lower level than caster
		// cap it on 30 levels below and above
			float leveladjust = 5.5f * (GetLevel() - level);
            
			if (leveladjust < -150)
				leveladjust = -150;
			if (leveladjust > 150)
				leveladjust = 150;

			resist += leveladjust;

		// now adjust spell resist rate (values from spells_en.txt)
		// lure spells have ResistDiff == -300, normal spells about +/- 10
			resist += spells[spell_id].ResistDiff;

			float chance = (resist / maxchance);

		// cap resists so it CAN be possible to land spells even if big difference
		// in levels resist
			if (chance < 0.01)
				chance = 0.01;
			if (chance > 0.99)
				chance = 0.99;

			float random = float(rand())/float(RAND_MAX);
	
			if (partial && (random > chance - 0.10)) 
			{
				LogFile->write(EQEMuLog::Normal, "%s casted %s on %s: 50%% Hit!",
                           caster?caster->GetName():"unknown", GetName(),
                           spells[spell_id].name);
				return 50;
			}
			else if (partial && random > chance - 0.08) 
			{
				LogFile->write(EQEMuLog::Normal, "%s casted %s on %s: 60%% Hit!",
                           caster?caster->GetName():"unknown", GetName(),
                           spells[spell_id].name);
				return 60;
			}
			else if (partial && random > chance - 0.06)
			{
				LogFile->write(EQEMuLog::Normal, "%s casted %s on %s: 70%% Hit!",
                           caster?caster->GetName():"unknown", GetName(),
                           spells[spell_id].name);
				return 70;
			}
			else if (partial && random > chance - 0.04) 
			{
				LogFile->write(EQEMuLog::Normal, "%s casted %s on %s: 80%% Hit!",
                           caster?caster->GetName():"unknown", GetName(),
                           spells[spell_id].name);
				return 80;
			}
			else if (partial && random > chance - 0.02)
			{
				LogFile->write(EQEMuLog::Normal, "%s casted %s on %s: 90%% Hit!",
                           caster?caster->GetName():"unknown", GetName(),
	                       spells[spell_id].name);
				return 90;
			}
			else if (random > chance) 
			{
				LogFile->write(EQEMuLog::Normal, "%s casted %s on %s: Full Hit!",
                           caster?caster->GetName():"unknown", GetName(),
                           spells[spell_id].name);
				return 100;
			}
			return 0;
		}
	}


return 100;
}

void Mob::SpellOnTarget(int16 spell_id, Mob* spelltar) {
	if (spelltar == 0) {
		Message(13, "You must have a target for this spell.");
		return;
	}
	if (spells[spell_id].targettype == ST_AECaster && spelltar == this)
        return;

	if (!spells[spell_id].goodEffect && spelltar->IsClient() &&
        spelltar->CastToClient()->invulnerable && spelltar != this) {
		return;
	}

	bool killspell=0;
	if (!(this->IsClient() && this->CastToClient()->GetGM())) {
		if(spelltar->GetID() != this->GetID()) { /* you can cast whatever you want on yourself */
			if(!spells[spell_id].goodEffect) { /* if it's harmful, pvp check it */
				if(spells[spell_id].targettype != ST_AEBard &&
                   spells[spell_id].targettype != ST_Group &&
                   spells[spell_id].targettype != ST_GroupTeleport &&
                   !IsAttackAllowed(spelltar))
					killspell=1;
			}

            if (!this->IsAIControlled() &&
                Dist(spelltar) > GetActSpellRange(spell_id, (int)(spells[spell_id].range)) &&
                !(spells[spell_id].targettype == ST_AlterPlane && IsNPC())) {
                if (this->IsClient() && spells[spell_id].targettype != ST_AEBard ) {
                    CastToClient()->Message(MT_Spells, "Your target is out of range(1)!");
                }
                killspell = true;
            }
			if(killspell) {
//				casting_spell_id=0;
//				this->InterruptSpell();
				return;
			}
		}
	}

	sint16 partial = spelltar->ResistSpell(spell_id, this);
	
	if (partial <= 0) {
		if (partial != -2) {
			if (IsClient())
				Message(MT_Spells, "Your target resisted the %s spell.", spells[spell_id].name);
		}
		if (spelltar->IsClient())
			spelltar->Message(MT_Shout, "You resist the %s spell.", spells[spell_id].name);
		// aggro even if resisted
		if (spelltar->IsAIControlled())
			spelltar->AddToHateList(this, 1);
		return;
	}

	if (spell_id == 982) { // Cazic Touch, hehe =P
		char tmp[64];
		strcpy(tmp, spelltar->GetName());
		strupr(tmp);
		if (this->IsClient() && spelltar->IsClient()) {
			if (spelltar->CastToClient()->Admin() > this->CastToClient()->Admin())
				return;
		}
		entity_list.Message(0, MT_Shout, "%s shouts, '%s!'", this->GetName(), tmp);
	}

	if (IsMezSpell(spell_id)) {
		if (!spelltar->IsMezzable(spell_id)) {
			if (IsClient()) {
				Message(MT_Shout,"You can not mesmerize this monster with that type of spell!");
			}
			return;
		}
	}
    
	if (IsCharmSpell(spell_id)) {
		if (this == spelltar) {
			Message(13, "You cannot charm yourself!");
			return; // charming yourself is very bad news =p
		}
		if (this->GetPetID() != 0) {
			Message(13, "You\'ve already have a pet.");
			return;
		}
    } else { // this sucks for charm spells todo: invent something better
		if (spelltar->IsAIControlled() && spells[spell_id].goodEffect==0)
			spelltar->AddToHateList(this,0,CalcSpellValue(spells[spell_id].formula[0],
                                                          spells[spell_id].base[0],
                                                          spells[spell_id].max[0],
                                                          GetLevel(), spell_id));
	}

	int16 tmpBuffdur = 0;
	sint8 tmpBuffSlot = spelltar->CheckAddBuff(this, spell_id, this->GetLevel(), &tmpBuffdur);
	if (tmpBuffSlot < -1)
		return;

	/*Spell resist code.. we need the resists for clients/mobs
	if(spells[spell_id].resisttype !=0 && spells[spell_id].goodEffect!=0)
	{
	if((spelltar->GetResist(spells[spell_id].resisttype) + spells[spell_id].ResistDiff + this->GetLevel()/spelltar->GetLevel() some formula with level)<(rand()%100)/100.0))
	//spell resisted.. or partially resisted..
	int temp=rand()%100;
	if(temp > 50) totalresist
	else spelltar->SpellPartialEffect(this,spell_id,this->GetLevel(),temp/50.0);
	}
	//else
	*/

	if (!spelltar->SpellEffect(this, spell_id, this->GetLevel(), tmpBuffSlot, tmpBuffdur)) {
		APPLAYER* outapp = 0;

		outapp = new APPLAYER(OP_Action, sizeof(Action_Struct));
		Action_Struct* action = (Action_Struct *)outapp->pBuffer;
		action->damage = 0;
		action->spell = spell_id;
		if (this->IsClient() && this->CastToClient()->GMHideMe())
			action->source = 0;
		else
			action->source = GetID();
		action->target = spelltar->GetID();
		action->type = 0xE7;
		outapp->priority = 3;
		entity_list.QueueCloseClients(spelltar, outapp, false, 200, this); // send target and people near target
		if (this->IsClient())
			this->CastToClient()->QueuePacket(outapp); // send to caster of the spell
		delete outapp;
	}
}

// Hogie - Stuns "this"
void Client::Stun(int16 duration) {
	if (duration > 0) {
		this->stunned = true;
		stunned_timer->Start(duration);
	}
	APPLAYER* outapp = new APPLAYER(OP_Stun, sizeof(Stun_Struct));
	Stun_Struct* stunon = (Stun_Struct*) outapp->pBuffer;
	stunon->duration = duration;
	outapp->priority = 5;
	this->CastToClient()->QueuePacket(outapp);
	delete outapp;
	InterruptSpell();
}


void NPC::Stun(int16 duration) {
	if(HasBanishCapability() == 101)
		return;

	InterruptSpell();

	SetRunAnimSpeed(0);
	pLastChange = Timer::GetCurrentTime();
	if (duration > 0) {

		this->stunned = true;
		stunned_timer->Start(duration);
	}
}

void Mob::Mesmerize() {
	this->mezzed = true;

	if (casting_spell_id) {
		isinterrupted = true;
		InterruptSpell();
	}

	if (this->IsClient()){
		APPLAYER* outapp = new APPLAYER(OP_Stun, sizeof(Stun_Struct));
		Stun_Struct* stunon = (Stun_Struct*) outapp->pBuffer;
		stunon->duration = 0xFFFF;
		this->CastToClient()->QueuePacket(outapp);
		delete outapp;
    } else {
		SetRunAnimSpeed(0);
	}

    return; // (velns) What is the point of this?
}

void Corpse::CastRezz(int16 spellid, Mob* Caster){
	if (!rezzexp) {
		Caster->Message(4, "You cannot resurrect this corpse");
		return;
	}

	APPLAYER* outapp = new APPLAYER(OP_RezzRequest, sizeof(Resurrect_Struct));
	Resurrect_Struct* rezz = (Resurrect_Struct*) outapp->pBuffer;
	memset(rezz,0,sizeof(Resurrect_Struct));
	memcpy(rezz->your_name,this->orgname,30);
	memcpy(rezz->corpse_name,this->name,30);
	memcpy(rezz->rezzer_name,Caster->GetName(),30);
	memcpy(rezz->zone,zone->GetShortName(),15);
	rezz->spellid = spellid;
	rezz->x = this->x_pos;
	rezz->y = this->y_pos;
	rezz->z = (float)this->z_pos;
	worldserver.RezzPlayer(outapp, rezzexp, OP_RezzRequest);
	//DumpPacket(outapp);
	delete outapp;
}

int16 Mob::CalcBuffDuration(int8 level, int16 formula, int16 duration) {
	switch(formula) {
	
	case 0:
		return 0;
	case 1:
		return level / 2;
	case 2:
		return level / 2 + 1;
	case 3:
		return duration;
	case 4:
		return duration;
	case 5:
        	return 3;
	case 6:
		return level / 2;
	case 7:
		return level * 10;
	case 8:
		return level * 10 + 10;
	case 9:
		return level * 2 + 10;
	case 10:
		return level * 3 + 10;
	case 11:
		return duration;
	case 50:    // ?? this is not correct, but leave it here for now
		return level * 10;
	case 3600:
		return 3600;
	default:
		cerr << "unknown formula: " << (int16)formula << endl;
	return 0;
	}
}

// returns -1 if not a buff, -2 if spell should fail, otherwise the buff slot
sint8 Mob::CheckAddBuff(Mob* caster, const int16& spellid,
                        const int8& caster_level, int16* buffdur,
                        sint16 ticsremaining) {
	sint8 tmpslot = -1;
    
	if (ticsremaining >= 0)
		*buffdur = ticsremaining;
	else {
		*buffdur = CalcBuffDuration(caster->GetLevel(),
                                    spells[spellid].buffdurationformula,
                                    spells[spellid].buffduration);
		
		if (IsClient()) {
		    *buffdur = CastToClient()->GetActSpellDuration(spellid, *buffdur);
		}
	}

	if (*buffdur > 0) {
		tmpslot = CanBuffStack(spellid, caster_level);
		if (tmpslot >= BUFF_COUNT)
			return -1;
		if (tmpslot < 0) {
            // if all slots full and NPC caster or negative effect
            // look for a positive buff to overwrite
            if (tmpslot == -2 && 
                ((caster && caster->IsAIControlled() && this->IsClient()) ||
                  spells[spellid].goodEffect <= 0)) {
				for (int i=0; i<BUFF_COUNT; i++) {
					if (spells[buffs[i].spellid].goodEffect == 1) {
						tmpslot = i;
						break;
					}
				}
				// no postitive effect, just overwrite a random buff
				if (tmpslot < 0)
					tmpslot = rand()%BUFF_COUNT;
            } else if (caster && caster->IsClient()) {
				if (tmpslot == -2) {
					caster->Message(13,
                        "Error: All buff slots full on %s.",
                        this->GetName());
                } else {
					caster->Message(13,
                        "Error: Unable to stack with existing buff on %s.",
                        this->GetName());
                }
                return -2;
            }
		}
	}
	return tmpslot;
}

// returns true if it sent the OP_Action, false if it didnt
bool Mob::SpellEffect(Mob* caster, int16 spell_id, int8 caster_level,
                      sint8 buffslot, int16 buffdur, int16 partial) {
	if (!spells_loaded) {
		Message(0, "Spells werent loaded on bootup.");
		return false;
	}
	if (GetHP() <= 0) {
	    return false; // Target is dead
	}

	bool ret = false;
	/*struct Buffs_Struct {
	int16	spellid;
	int8	casterlevel;
	int32	ticselapsed;
};*/

	int i = 0;

	if (buffslot >= 0 && buffslot < BUFF_COUNT) {
		BuffFadeByStackCommand(spell_id, buffslot);
		
		if (buffs[buffslot].spellid != 0xFFFF)
			BuffFadeBySlot(buffslot);
		
		buffs[buffslot].spellid = spell_id;
		buffs[buffslot].casterlevel = caster_level;
		
		if (caster == 0)
			buffs[buffslot].casterid = 0;
		else
			buffs[buffslot].casterid = caster->GetID();

		buffs[buffslot].durationformula = spells[spell_id].buffdurationformula;
		buffs[buffslot].ticsremaining = buffdur;
		CalcBonuses();
	}

	/* Actual cast action */
	if (caster) {
		APPLAYER* outapp = new APPLAYER(OP_CastOn, sizeof(CastOn_Struct));
		CastOn_Struct* caston = (CastOn_Struct*) outapp->pBuffer;
		if (caster->IsClient() && caster->CastToClient()->GMHideMe())
			caston->source_id = 0;
		else
			caston->source_id = caster->GetID();
		caston->target_id = this->GetID();
		caston->action = 231;
		caston->spell_id = spell_id;
		caston->heading = caster->GetHeading() * 2;
		caston->source_level = caster->GetLevel();
		caston->unknown2 = 0x0A;
		caston->unknown4[0] = 0x00;
		caston->unknown1[1] = 0x41;
		caston->unknown4[1] = 0x04;

		entity_list.QueueCloseClients(this, outapp, false, 600, caster); // send target and people near target
		if (caster->IsClient())
			caster->CastToClient()->QueuePacket(outapp); // send to caster of the spell
		//DumpPacket(outapp);
		delete outapp;
	}

	for (i=0; i < 12; i++) {
		if (!(spells[spell_id].effectid[i] == 0xFE  ||
              spells[spell_id].effectid[i] == 0xFF)) {
			switch(spells[spell_id].effectid[i]) {
				case SE_CurrentHP: {
				// SE_CurrentHP is calculated at first tick if its a dot/buff
				if (buffslot >= 0)
					break;
				case SE_CurrentHPOnce:
#ifdef SPELL_EFFECT_SPAM
				if (caster)
					caster->Message(0,
                        "Effect #%i: You changed %s's hp by %+i", i,
                        this->GetName(), CalcSpellValue(spells[spell_id].formula[i],
                                                        spells[spell_id].base[i],
                                                        spells[spell_id].max[i],
                                                        caster_level, spell_id));
#endif
		// for offensive spells check if we have a spell rune on
		sint32 dmg = CalcSpellValue(spells[spell_id].formula[i],
                                    spells[spell_id].base[i],
                                    spells[spell_id].max[i],
                                    caster_level, spell_id);
		if ( spells[spell_id].effectid[i] == SE_CurrentHP && dmg < 0) {
			// take partial damage into account
			dmg = (dmg * partial) / 100;
			if (caster && caster->IsClient()) {
				dmg = caster->CastToClient()->GetActSpellValue(spell_id, dmg);
				/*spell crits*/
				int chance =0;float ratio =1.0;
				/*normal spell crit*/
				if(caster_level > 12 && caster->GetClass() == 12)
				{	chance+= 3; ratio +=.15;}
				/*spell casting fury*/
                uint8 *aa_item = &(((uint8 *)&caster->CastToClient()->aa)[23]);
				if(*aa_item == 1) {chance+=2; ratio += .333;}
				if(*aa_item == 2) {chance+=5; ratio += .666;}
				if(*aa_item == 3) {chance+=7; ratio += 1.0;}
				
				if(rand()%100 <= chance) dmg*= ratio;	//message.. but I don't care atm			
			}

			sint32 origdmg = dmg;
			dmg = ReduceMagicalDamage(dmg, GetMagicRune());
			if (origdmg != dmg && caster && caster->IsClient()) {
				caster->Message(15,
                    "The Spellshield absorbed %d of %d points of damage",
                    abs(origdmg-dmg), abs(origdmg));
            }

			if (dmg == 0)
				break;
		}
		else if (spells[spell_id].effectid[i] == SE_CurrentHP && dmg > 0) {
			if (caster && caster->IsClient()) {
				dmg = caster->CastToClient()->GetActSpellValue(spell_id, dmg);
			}
		}

		ret = this->ChangeHP(caster, dmg, spell_id, buffslot);
				if(IsNPC() && CastToNPC()->IsInteractive())
					CastToNPC()->TakenAction(21,caster);
				break;
			}
			case SE_Translocate: {
				if(caster->IsNPC())
					CastToClient()->MovePC(spells[spell_id].teleport_zone,
                                           spells[spell_id].base[1],
                                           spells[spell_id].base[0],
                                           spells[spell_id].base[2]);
				else if(caster == this)
					CastToClient()->MovePC(spells[spell_id].teleport_zone,
                                           spells[spell_id].base[1],
                                           spells[spell_id].base[0],
                                           spells[spell_id].base[2]);
				else if(caster != 0) {
					Group* group = entity_list.GetGroupByClient(caster->CastToClient());
					if(group != 0 && group->IsGroupMember(this->CastToMob())) {
						CastToClient()->MovePC(spells[spell_id].teleport_zone,
                                               spells[spell_id].base[1],
                                               spells[spell_id].base[0],
                                               spells[spell_id].base[2]);
                    } else {
					    caster->Message(0,
                            "You cannot teleport this person unless you are grouped.");
                    }
				}
				break;
			}
			case SE_HealOverTime: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Heal over Time spell.", i);
#endif
				if(IsNPC() && CastToNPC()->IsInteractive())
					CastToNPC()->TakenAction(21,caster);
				break;
								  }

			case SE_MovementSpeed: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast an Movement Speed buff/debuff.", i);
#endif
				break;
								   }
			case SE_AttackSpeed: {
				if (spells[spell_id].goodEffect==0)
					this->SetHaste(0 - 
                        (int)CalcSpellValue(spells[spell_id].formula[i],
                                            spells[spell_id].base[i],
                                            spells[spell_id].max[i],
                                            caster_level, spell_id));
				else
					this->SetHaste(
                        (int)CalcSpellValue(spells[spell_id].formula[i],
                                            spells[spell_id].base[i],
                                            spells[spell_id].max[i],
                                            caster_level, spell_id));
#ifdef SPELL_EFFECT_SPAM

				Message(0, "Effect #%i:You cast an Attack Speed buff/debuff.", i);
#endif
				break;
								 }
			case SE_Invisibility: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast an Invisibility spell.", i);
#endif
				invisible = true;
				Mob *p = this->GetPet();
				if (p) {
					// todo: kill pet
					// or remove charm from charmed pets
				}
				break;
				}
			case SE_SeeInvis: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a See Invis buff.", i);
#endif
				break;
				}
			case SE_WaterBreathing: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Water Breathing buff.", i);
#endif
				break;
									}
			case SE_CurrentMana: {
				if (buffslot >= 0)
					break;
				SetMana(GetMana() +
                    CalcSpellValue(spells[spell_id].formula[i],
                                   spells[spell_id].base[i],
                                   spells[spell_id].max[i],
                                   caster_level, spell_id));
				break;
								 }
			case SE_AddFaction: {

#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast an Add Faction spell.", i);
#endif
				break;
								}
			case SE_Stun: {
				if (IsClient()) {
					CastToClient()->Stun(spells[spell_id].base[i]);
                } else if (IsNPC()) {
					CastToNPC()->Stun(spells[spell_id].base[i]);
                }
				break;
						  }
			case SE_Charm: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Charm spell.", i);
#endif
				if (!caster)
					break;

//				if (IsClient() && !caster->IsNPC() && !(caster->IsClient() && caster->CastToClient()->GetGM())) {
//					caster->Message(13, "You can only charm non-player-characters.");
//					break;
//				}
				SetPetOrder(SPO_Follow);
				WhipeHateList();
				this->SetOwnerID(caster->GetID());
				caster->SetPetID(this->GetID());
                
				if (caster->IsClient()) {
					APPLAYER *app = new APPLAYER(OP_Charm, sizeof(Charm_Struct));
					Charm_Struct *ps = (Charm_Struct*)app->pBuffer;
					ps->owner_id = caster->GetID();
					ps->pet_id = this->GetID();
					ps->command = 1;
					caster->CastToClient()->FastQueuePacket(&app);
				}
				if (this->IsClient()) {
					AI_Start();
				}
				buffs[buffslot].ticsremaining = (rand()%buffs[buffslot].ticsremaining);
				break;
			}
			case SE_Fear: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Fear spell.", i);
#endif			
				if (IsClient()) {
					CastToClient()->Stun((int)(rand()%(spells[spell_id].buffduration+10)));//kathgar: Its basicly fear, besides duration might not be right and they don't move
                } else if (IsNPC()) {
					CastToNPC()->Stun(((int)(rand()%(spells[spell_id].buffduration+10))));
                }
				Spin();
				break;
						  }
			case SE_Stamina: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Stamina add/subtract spell.", i);
#endif
				break;
							 }
			case SE_BindAffinity: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Bind Affinity spell.", i);
#endif
				if (this->IsClient()) {
					this->CastToClient()->SetBindPoint();
					this->CastToClient()->Save();
				}
				break;
			}
			case SE_Gate: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Gate spell.", i);
#endif
				if (this->IsAIControlled()) {
					this->WhipeHateList();
				}
				this->GoToBind();
				break;
			}
			case SE_CancelMagic: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Cancel Magic spell.", i);
#endif
				int countx; //for the win32 folks who scope wrong
				for(countx=0;countx< BUFF_COUNT;countx++)
				{
					if (buffs[countx].spellid != 0 && buffs[countx].spellid != 0xFFFF &&
					    (buffs[countx].casterlevel <= 
                        (caster_level + spells[spell_id].base[i])) &&
						!(buffs[countx].casterid == this->GetID() &&
                        this->IsClient() && this->CastToClient()->GetGM() &&
                        buffs[countx].durationformula == DF_Permanent)) {

						this->BuffFade(buffs[countx].spellid);
						countx=20;
					}
				}//kathgar: no longer takes randomly? just down the line that was annoying to code =/
				/*	bool BuffExists=false;
				int countx;
				for(countx =0; countx < 15;countx++)
				{
				if(buffs[countx].spellid!=0 && (buffs[countx].casterlevel <= (caster_level + spells[spell_id].base[i])))
				{
				BuffExists=true;
				countx=20;
				}
				}
				if(BuffExists)
				{
				int buffpos =0;
				do
				{
				buffpos=rand()%16;
				}
				while(buffs[buffpos].spellid != 0 && (buffs[buffpos].casterlevel <= (caster_level + spells[spell_id].base[i])));
				if(buffs[buffpos].spellid!=0)
				this->BuffFade(buffs[buffpos].spellid,0);
								 }*/
				break;
			}
			case SE_InvisVsUndead: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast an Invis Vs Undead spell.", i);
#endif
                Mob *p = this->GetPet();
                if (p)
                {
                    // todo: kill pet
                    // or remove charm from charmed pets
                }
				break;
								   }
			case SE_Mez:
            {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Mez spell.", i);
#endif
				this->Mesmerize();
				break;
			}
			case SE_SummonItem: {
				if (this->IsClient()) {
#ifdef SPELL_EFFECT_SPAM
					Message(0, "Effect #%i: You cast a Summon Item #%i.",
                            i, spells[spell_id].base[i]);
#endif
					int16 tmpcharges = 0;
					if (spells[spell_id].formula[i] < 100)
						tmpcharges = spells[spell_id].formula[i];
					else {
						int16 tmpclass = 0;
						int tmplevel = 1;
						if (caster) {
							tmpclass = caster->GetClass() - 1;
							if (tmpclass >= 15) {
								tmpclass = 0;
							}
							tmplevel = caster_level - spells[spell_id].classes[tmpclass];
							if (tmplevel < 0)
								tmplevel = 0;
							tmpcharges =
                                CalcSpellValue(spells[spell_id].formula[i],
                                               tmplevel,
                                               spells[spell_id].max[i],
                                               spells[spell_id].classes[tmpclass],
                                               spell_id);
						}
					}
					if (tmpcharges > 20)
						tmpcharges = 20;
					if (tmpcharges == 0)
						this->CastToClient()->SummonItem(spells[spell_id].base[i],
                                                         1);
					else
						this->CastToClient()->SummonItem(spells[spell_id].base[i],
                                                         tmpcharges);
				}
				break;
								}
			case SE_SummonBSTPet:
			case SE_NecPet:
			case SE_SummonPet: {
				if (this->GetPetID() != 0 || this->GetOwnerID() != 0) {
					Message(13, "You\'ve already have a pet or are a pet.");
					break;
				}
				this->MakePet(spells[spell_id].teleport_zone);
				break;
							   }
			case SE_DivineAura: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Divine Aura spell.", i);
#endif
				break;
								}
			case SE_ShadowStep: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Shadow Step spell.", i);
#endif
				break;
								}
			case SE_SenseDead: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Sense Dead spell.", i);
#endif
				break;
							   }
			case SE_SenseSummoned: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Sense Summoned spell.", i);
#endif
				break;
								   }
			case SE_SenseAnimals: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Sense Animals spell.", i);
#endif
				break;
								  }
			case SE_Rune: {
                LogFile->write(EQEMuLog::Normal, "Creating Rune");
				SetRune(CalcSpellValue(spells[spell_id].formula[i],
                                       spells[spell_id].base[i],
                                       spells[spell_id].max[i],
                                       caster_level, spell_id));
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Rune spell.", i);
#endif
				break;
						  }
            case SE_AbsorbMagicAtt:
            {
    			SetMagicRune(CalcSpellValue(spells[spell_id].formula[i],
                                            spells[spell_id].base[i],
                                            spells[spell_id].max[i],
                                            caster_level, spell_id));
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Magic Absorb Rune spell.", i);
#endif
				break;
            }
			case SE_TrueNorth: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a True North spell. UBER!", i);
#endif
				break;
							   }
			case SE_Levitate: {
				this->SendAppearancePacket(19, 2);
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Levitate spell.", i);
#endif
				break;
							  }
			case SE_Illusion:
            {
                int16 tex = 0;
                if (spell_id == 599||spell_id == 2798||spell_id == 2799||spell_id == 2800)
                    tex = 2;// water
                else if (spell_id == 598||spell_id == 2795||spell_id == 2796||spell_id == 2797)
                    tex = 1;// fire
                else if (spell_id == 584||spell_id == 2792||spell_id == 2793||spell_id == 2794)
                    tex = 0; //earth
                else if (spell_id == 597||spell_id == 2789|| spell_id == 2790|| spell_id == 2791)
                    tex = 3;// air
				SendIllusionPacket(spells[spell_id].base[i],
                                   Mob::GetDefaultGender(spells[spell_id].base[i],
                                   GetGender()), tex);
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast an Illusion spell.", i);
#endif
				break;
			}
			case SE_DamageShield: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Damage Shield spell.", i);
#endif
				break;
								  }
			case SE_Identify: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast an Identify spell.", i);
#endif
				break;
							  }
			case SE_WhipeHateList: {
				if (this->IsAIControlled()) {
					this->WhipeHateList();
				}
				Message(13, "Your mind fogs. Who are my friends? Who are my enimies?... it was all so clear a moment ago...");
				break;
								   }
			case SE_SpinTarget: {
				if (IsClient())
					Spin();
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Spin Target spell.", i);
#endif
				break;
								}
			case SE_InfaVision: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast an InfaVision buff.", i);
#endif
				break;
								}
			case SE_UltraVision: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast an UltraVision buff.", i);
#endif
				break;
								 }
			case SE_EyeOfZomm: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast an Eye of Zomm spell.", i);
#endif
				break;
							   }
			case SE_ReclaimPet: {
#ifdef SPELL_EFFECT_SPAM

				Message(0, "Effect #%i: You cast a Reclaim Pet spell.", i);
#endif
				break;
								}
			case SE_BindSight: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Bind Sight spell.", i);
#endif
				break;
							   }
			case SE_FeignDeath: {
				if (IsClient())
					this->CastToClient()->SetFeigned(true);
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Feign Death spell.", i);
#endif
				break;
								}
			case SE_VoiceGraft: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Voice Graft spell.", i);
#endif
				break;
								}
			case SE_Sentinel: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast Sentinel. Fuck You.", i);
#endif
				break;
							  }
			case SE_LocateCorpse: {
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Locate Corpse spell.", i);
#endif
				break;
								  }
			case SE_Revive: {
				if (this->IsCorpse() && this->CastToCorpse()->IsPlayerCorpse())
					this->CastToCorpse()->CastRezz(spell_id, caster);
				break;
							}
			case SE_Teleport: {
				if(caster->IsNPC()) {
					CastToClient()->MovePC(spells[spell_id].teleport_zone,
                                           spells[spell_id].base[1],
                                           spells[spell_id].base[0],
                                           spells[spell_id].base[2]);

                 } else if(this->IsClient()) {
					CastToClient()->MovePC(spells[spell_id].teleport_zone,
                                           spells[spell_id].base[1],
                                           spells[spell_id].base[0],
                                           spells[spell_id].base[2]);
                 }

				break;
			}
			case SE_ModelSize: {
				// Neotokyo's Size Code
				if (this->IsNPC())
					break;
				// End of Neotokyo's Size Code
				this->ChangeSize(this->GetSize()*(((float)(spells[spell_id].base[i]))/100));
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Shrink/Grow spell.", i);
#endif
				break;
							   }

			case SE_Root: {
				rooted = true;
//#ifdef SPELL_EFFECT_SPAM

//				Message(0, "Effect #%i: You cast a Root spell.", i);
//#endif
				break;
						  }
			case SE_SummonHorse: {
				if(IsClient())
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a SummonHorse spell.", i);
#endif
					CastToClient()->MakeHorseSpawnPacket(spell_id);
				break;
								 }
			case SE_SummonCorpse:{
				// (TODO) Take reagents from caster (velns)
				if (this->GetTarget() && this->GetTarget()->IsClient()){
				/*
				if (entity_list.GetGroupByClient(this->GetTarget()->CastToClient()) != entity_list.GetGroupByClient(this->CastToClient())){
				Message(13, "You can only summon corpses form groupmembers");
				break;
				}*/
					Corpse* corpse = entity_list.GetCorpseByOwner(
                            this->GetTarget()->CastToClient());
					if (!corpse) {
						Message(4, "There is no corpse from %s in this zone!",
                                this->GetTarget()->GetName());
                    } else {
						corpse->Summon(this->CastToClient(),true);
						Message(4, "You summon %s's corpse",
                                this->GetTarget()->GetName());
					}
                } else {
					Message(13, "You need to target a player!");
                }
				break;
								 }
            case SE_Familiar: {
				if (this->GetFamiliarID() != 0) {
					Message(13, "You\'ve already got a familiar.");
					break;
				}
				this->MakePet(spells[spell_id].teleport_zone);
				break;
            }
			case 0xFE:
			case 0xFF: {
				// this is the code for empty... i think
				break;
					   }
			case SE_Lull:
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a Lull spell.", i);
#endif
				// TODO: check vs. CHA when harmony effect failed, if caster is to be added to hatelist
				break;
			case SE_WeaponProc:
#ifdef SPELL_EFFECT_SPAM
				Message(0, "Effect #%i: You cast a WeaponProc spell.", i);
#endif
                AddProcToWeapon(spells[spell_id].base[i]);
				break;
			case SE_ChangeAggro:
#ifdef SPELL_EFFECT_SPAM

				Message(0, "Effect #%i: You cast a ChangeAggro spell.", i);
#endif
				// TODO: i really dont know how to use that spell :-((
				break;
			case SE_ChangeFrenzyRad:
			case SE_Harmony:
			case SE_TotalHP:
			case SE_ArmorClass:
			case SE_ATK:
			case SE_STR:
			case SE_DEX:
			case SE_AGI:
			case SE_STA:

			case SE_INT:
			case SE_WIS:

			case SE_CHA:
			case SE_ResistFire:
			case SE_ResistCold:
			case SE_ResistPoison:
			case SE_ResistDisease:
			case SE_ResistMagic:
            case SE_ResistAll:
            case SE_ManaPool:
            case SE_ModCastSkill:
			case SE_StackingCommand1:
			case SE_StackingCommand2: {
				// Buffs are handeled elsewhere
				break;
			}
			default: {
				Message(0, 
                        "Effect #%i: I dont know what this effect is: 0x%x.",
                        i, spells[spell_id].effectid[i]);
			}
		}
		}
	}
	return ret;
}

sint32 Mob::CalcSpellValue(int8 formula, sint16 base, sint16 max,
                           int8 caster_level, int16 spell_id) {
/*
neotokyo: i need those formulas checked!!!!

0 = base
1 - 99 = base + level * formulaID
100 = base
101 = base + level / 2
102 = base + level
103 = base + level * 2
104 = base + level * 3
105 = base + level * 4
106 ? base + level * 5
107 ? min + level / 2
108 = min + level / 3
109 = min + level / 4
110 = min + level / 5
119 ? min + level / 8
121 ? min + level / 4
122 = splurt
123 ?
203 = stacking issues ? max
205 = stacking issues ? 105


  0x77 = min + level / 8
	*/

    uint16 ubase = abs(base);
    sint32 result = 0;
    sint16 updownsign = 1;

    if (max < base && max != 0) {
        // values are calculated down
        updownsign = -1;
    } else {
        // values are calculated up
        updownsign = 1;
    }

    switch(formula) {
		case 100:
		case   0:
			result = ubase; break;
		case 101:
			result = ubase + updownsign * (caster_level / 2); break;
		case 102:
			result = ubase + updownsign * caster_level; break;
		case 103:
			result = ubase + updownsign * (caster_level * 2); break;
		case 104:
			result = ubase + updownsign * (caster_level * 3); break;
		case 105:
			result = ubase + updownsign * (caster_level * 4); break;
        case 108:
			result = ubase + updownsign * (caster_level / 3); break;
		case 109:
			result = ubase + (caster_level / 4); break;
		case 110:
			result = ubase + (caster_level / 5); break;
		case 111:
            result = ubase + 5 * (caster_level - 16); break;
		case 112:
            result = ubase + 8 * (caster_level - 24); break;
		case 113:
            result = ubase + 12 * (caster_level - 34); break;
		case 114:
            result = ubase + 15 * (caster_level - 44); break;
		case 119:
			result = ubase + (caster_level / 8); break;
		case 121:
			result = ubase + (caster_level / 4); break;
        case 122:
            // todo: we need the remaining tics here
            break;
		default: {
			if (formula < 100) {

    			result = ubase + (caster_level * formula);
	    		break;
            } else {
                LogFile->write(EQEMuLog::Normal,
                               "Unknown spell forumula at spell #%d",
                               spell_id);
			    break;
		    }
	    }
	}

    //if (result == 0)
        //g_LogFile.write("Strange: Result == 0");
        if (result == 0)
                LogFile->write(EQEMuLog::Debug, "Result = 0, spellid:%i", spell_id);

    // now check result against the allowed maximum
	if (max != 0) {
		if (updownsign == 1) {
			if (result > max)
				result = max;
        } else {
			if (result < max)
				result = max;
		}
    }

    // if base is less than zero, then the result need to be negativ too
    if (base < 0)
        result *= -1;

    return result;
}

void Mob::CalcBonuses() {
	//StatBonuses* newbon;
	if (this->IsClient()) {
		memset(itembonuses, 0, sizeof(StatBonuses));
		this->CastToClient()->CalcItemBonuses(itembonuses);
		this->CastToClient()->CalcEdibleBonuses(itembonuses);
	}
	CalcSpellBonuses(spellbonuses);

	CalcMaxHP();
	CalcMaxMana();
	rooted = FindType(SE_Root);
	if (GetMana() > GetMaxMana())
		SetMana(GetMaxMana());
}

void Client::CalcItemBonuses(StatBonuses* newbon) {
	const Item_Struct* item = 0;
	for (int i=1; i<21; i++) {
		item = database.GetItem(pp.inventory[i]);
		if (item != 0) {
			if (item->flag != 0x7669) {
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

                if (item->common.spellId == 998 &&
                    item->common.effecttype == 2) { // item haste
                    if (newbon->haste < item->common.level0)
                        newbon->haste = item->common.level0;
                } else if (item->common.spellId != 0xFFFF &&
                           item->common.effecttype == 2) { // latent effects
					ApplySpellsBonuses(item->common.spellId, item->common.level, newbon);
				}
			}
		}
	}
}

void Client::CalcEdibleBonuses(StatBonuses* newbon) {
#if DEBUG >= 11
    cout<<"Client::CalcEdibleBonuses(StatBonuses* newbon)"<<endl;
#endif
  // Search player slots for skill=14(food) and skill=15(drink)
  // pp.inventory[22] - pp.inventory[29]
  // pp.containerinv[0] - containerinv[79]


  // Find food
  uint32 food_inr = 0;
  int16 food_slot = 0;
  uint32 drink_inr = 0;
  int16 drink_slot = 0;

  const Item_Struct* search_item = NULL;
  for (int16 cur_i = 22; cur_i <= 29 ; cur_i++) {
    search_item = database.GetItem( GetItemAt(cur_i) );
    if (!search_item) {
      continue;
    }
    if (search_item->common.skill == 14 && !food_slot) {
      food_slot = cur_i;
      food_inr = GetItemAt(cur_i);
    } else if (search_item->common.skill == 15 && !drink_slot) {
      drink_slot = cur_i;
      drink_inr = GetItemAt(cur_i);
    }
    search_item = NULL;
  }
  for (int16 cur_b = 0+250; cur_b <= 79+250; cur_b++) {
    if ( !food_slot || !drink_slot ) {
      search_item = database.GetItem( GetItemAt(cur_b) );
      if (!search_item) {
        // Database error
        // FIXME    Log error function goes here
        continue;
      }
      if (search_item->common.skill == 14 && !food_slot) {
        food_slot = cur_b;
        food_inr = GetItemAt(cur_b);
      }
      if (search_item->common.skill == 15 && !drink_slot) {
        drink_slot = cur_b;
        drink_inr = GetItemAt(cur_b);
      }

      search_item = NULL;
    }
  }
  // End Find food

  if (food_slot) {
	search_item = database.GetItem( food_inr );
		if (search_item != NULL) {

#if DEBUG >= 11
    cout<<"Food_inr: "<< search_item->item_nr <<endl;
#endif

		newbon->AC += search_item->common.AC;
		newbon->HP += search_item->common.HP;
		newbon->Mana += search_item->common.MANA;
		newbon->STR += search_item->common.STR;
		newbon->STA += search_item->common.STA;
		newbon->DEX += search_item->common.DEX;
		newbon->AGI += search_item->common.AGI;
		newbon->INT += search_item->common.INT;
		newbon->WIS += search_item->common.WIS;
		newbon->CHA += search_item->common.CHA;
		newbon->MR += search_item->common.MR;
		newbon->FR += search_item->common.FR;
		newbon->CR += search_item->common.CR;
		newbon->PR += search_item->common.PR;
		newbon->DR += search_item->common.DR;
		search_item = NULL;

		}
	}

  if (drink_slot) {
	search_item = database.GetItem( drink_inr );
	if (search_item != NULL) {
#if DEBUG >= 11
	cout<<"Drink_inr: "<< search_item->item_nr <<endl;
#endif
	newbon->AC += search_item->common.AC;
	newbon->HP += search_item->common.HP;
	newbon->Mana += search_item->common.MANA;
	newbon->STR += search_item->common.STR;

	newbon->STA += search_item->common.STA;
	newbon->DEX += search_item->common.DEX;
	newbon->AGI += search_item->common.AGI;
	newbon->INT += search_item->common.INT;
	newbon->WIS += search_item->common.WIS;
	newbon->CHA += search_item->common.CHA;
	newbon->MR += search_item->common.MR;
	newbon->FR += search_item->common.FR;
	newbon->CR += search_item->common.CR;
	newbon->PR += search_item->common.PR;
	newbon->DR += search_item->common.DR;
	search_item = NULL;
	}
  }

    return;
}


void Mob::CalcSpellBonuses(StatBonuses* newbon) {
	memset(newbon, 0, sizeof(StatBonuses));
	newbon->ArrgoRange = -1;
	newbon->AssistRange = -1;
	for (int j=0; j<15; j++) {
		ApplySpellsBonuses(buffs[j].spellid, buffs[j].casterlevel, newbon);
	}
}

void Mob::ApplySpellsBonuses(int16 spell_id, int8 casterlevel, StatBonuses* newbon) {
	if (!spells_loaded) {
		return;
    }

	if (spell_id != 0xFFFF && spell_id != 0 && spell_id < SPDAT_RECORDS) {
		for (int i=0; i < 12; i++) {
			switch (spells[spell_id].effectid[i]) {
			case 0: // WTF hp regen for items?!?!?
			     break;
			case SE_HealOverTime: {
				if (this->IsClient() && spells[spell_id].base[i]> 0){
				    newbon->HPRegen +=
                        CalcSpellValue(spells[spell_id].formula[i],
                                       spells[spell_id].base[i],
                                       spells[spell_id].max[i],
                                       casterlevel,spell_id);
				}
				break;
			}
			case RegenTwo: {
				break; //No reason to have spell type 254 here as regen.
				/*
				if (this->IsClient()){
					if(i == 0)
					newbon->HPRegen +=
                        CalcSpellValue(spells[spell_id].formula[i],
                                       spells[spell_id].base[0],
                                       spells[spell_id].max[0],
                                       casterlevel,spell_id);
				}
				break;
				*/
			}
			case SE_Harmony: {
				// neotokyo: Harmony effect as buff - kinda tricky
				// harmony could stack with a lull spell, which has better aggro range
				// take the one with less range in any case
				if ((newbon->ArrgoRange == -1) ||
                    (spells[spell_id].base[i] < newbon->ArrgoRange)) {
					newbon->ArrgoRange = spells[spell_id].base[i];
                }
				//cout << "Effect: Aggroradius should be lowered" << newbon->aggroradius << endl;
				break;
			}
			case SE_ChangeFrenzyRad: {
				if ((newbon->AssistRange == -1) ||
                    (spells[spell_id].base[i] < newbon->AssistRange)) {
					newbon->AssistRange = spells[spell_id].base[i];
                }
				break;
			}
            case SE_AttackSpeed: {
                // TODO: figure out if stacking speed buffs are added/subtracted
                // or just subdued if slow is active (or not stacking?)
                sint32 haste = CalcSpellValue(spells[spell_id].formula[i],
                                              spells[spell_id].base[i],
                                              spells[spell_id].max[i],
                                              casterlevel, spell_id);
                if (haste < 100)
                    newbon->haste -= (100-haste);
                else
                    newbon->haste += (haste-100);

                // get live values, in respect of various bard songs and whatever
                if (haste < -120)
                    haste = -120;
                if (haste > 120)
                    haste = 120;


                //g_LogFile.write("Buff haste = %d",(int)newbon->haste);
                break;
            }
			case SE_TotalHP: {
					newbon->HP += CalcSpellValue(spells[spell_id].formula[i],
                                                 spells[spell_id].base[i],
                                                 spells[spell_id].max[i],
                                                 casterlevel, spell_id);
//					cout << "Effect: You cast an HP buff/debuff." << endl;
				break;
							 }
			case SE_ArmorClass: {
					newbon->AC += CalcSpellValue(spells[spell_id].formula[i],
                                                 spells[spell_id].base[i],
                                                 spells[spell_id].max[i],
                                                 casterlevel, spell_id);
				//					Message(0, "Effect #%i: You cast an AC buff/debuff.", i);
				break;
								}
			case SE_ATK: {
					newbon->ATK += CalcSpellValue(spells[spell_id].formula[i],
                                                  spells[spell_id].base[i],
                                                  spells[spell_id].max[i],
                                                  casterlevel, spell_id);
				//					Message(0, "Effect #%i: You cast an ATK buff/debuff.", i);
				break;
						 }
			case SE_STR: {
					newbon->STR += CalcSpellValue(spells[spell_id].formula[i],
                                                  spells[spell_id].base[i],
                                                  spells[spell_id].max[i],
                                                  casterlevel, spell_id);
				//					Message(0, "Effect #%i: You cast a STR buff/debuff.", i);
				break;
						 }
			case SE_DEX: {

					newbon->DEX += CalcSpellValue(spells[spell_id].formula[i],
                                                  spells[spell_id].base[i],
                                                  spells[spell_id].max[i],
                                                  casterlevel, spell_id);
				//					Message(0, "Effect #%i: You cast a DEX buff/debuff.", i);
				break;
						 }
			case SE_AGI: {
					newbon->AGI += CalcSpellValue(spells[spell_id].formula[i],
                                                  spells[spell_id].base[i],
                                                  spells[spell_id].max[i],
                                                  casterlevel, spell_id);
				//					Message(0, "Effect #%i: You cast an AGI buff/debuff.", i);
				break;

						 }
			case SE_STA: {
					newbon->STA += CalcSpellValue(spells[spell_id].formula[i],
                                                  spells[spell_id].base[i],
                                                  spells[spell_id].max[i],
                                                  casterlevel, spell_id);
				//					Message(0, "Effect #%i: You cast a STA buff/debuff.", i);
				break;
						 }
			case SE_INT: {
					newbon->INT += CalcSpellValue(spells[spell_id].formula[i],
                                                  spells[spell_id].base[i],
                                                  spells[spell_id].max[i],
                                                  casterlevel, spell_id);
				//					Message(0, "Effect #%i: You cast an INT buff/debuff.", i);
				break;
						 }
			case SE_WIS: {
					newbon->WIS += CalcSpellValue(spells[spell_id].formula[i],
                                                  spells[spell_id].base[i],
                                                  spells[spell_id].max[i],
                                                  casterlevel, spell_id);
				//					Message(0, "Effect #%i: You cast a WIS buff/debuff.", i);
				break;
						 }
			case SE_CHA: {
				if (spells[spell_id].base[i] != 0) {
						newbon->CHA += CalcSpellValue(spells[spell_id].formula[i],
                                                      spells[spell_id].base[i],
                                                      spells[spell_id].max[i],
                                                      casterlevel, spell_id);
					// Message(0, "Effect #%i: You cast a CHA buff/debuff.", i);
                } else {
					// this is used in a lot of spells as a spacer, dunno why
				}
				break;
			}
			case SE_ResistFire: {
					newbon->FR += CalcSpellValue(spells[spell_id].formula[i],
                                                 spells[spell_id].base[i],
                                                 spells[spell_id].max[i],
                                                 casterlevel, spell_id);
				// Message(0, "Effect #%i: You cast a Resist Fire spell.", i);
				break;
			}
			case SE_ResistCold: {
					newbon->CR += CalcSpellValue(spells[spell_id].formula[i],
                                                 spells[spell_id].base[i],
                                                 spells[spell_id].max[i],
                                                 casterlevel, spell_id);
				// Message(0, "Effect #%i: You cast a Resist Cold spell.", i);
				break;
			}
			case SE_ResistPoison: {
					newbon->PR += CalcSpellValue(spells[spell_id].formula[i],
                                                 spells[spell_id].base[i],
                                                 spells[spell_id].max[i],
                                                 casterlevel, spell_id);
				// Message(0, "Effect #%i: You cast a Resist Poison spell.", i);
				break;
			}
			case SE_ResistDisease: {
					newbon->DR += CalcSpellValue(spells[spell_id].formula[i], spells[spell_id].base[i], spells[spell_id].max[i], casterlevel, spell_id);
				// Message(0, "Effect #%i: You cast a Resist Disease spell.", i);
				break;
			}
			case SE_ResistMagic: {
					newbon->MR += CalcSpellValue(spells[spell_id].formula[i], spells[spell_id].base[i], spells[spell_id].max[i], casterlevel, spell_id);
				// Message(0, "Effect #%i: You cast a Resist Magic spell.", i);
				break;
			}
            case SE_ResistAll: {
			    sint32 spellValue =
                    CalcSpellValue(spells[spell_id].formula[i],
                                   spells[spell_id].base[i],
                                   spells[spell_id].max[i],
                                   casterlevel, spell_id);
			    newbon->MR += spellValue;
				newbon->DR += spellValue;
				newbon->PR += spellValue;
				newbon->CR += spellValue;
				newbon->FR += spellValue;
                break;
            }
            case SE_ManaPool: {
                newbon->Mana += CalcSpellValue(spells[spell_id].formula[i],
                                               spells[spell_id].base[i],
                                               spells[spell_id].max[i],
                                               casterlevel, spell_id);
                break;
            }
            case SE_ModCastSkill:
                // todo: add some values for skills
            case SE_MovementSpeed:
                newbon->movementspeed = CalcSpellValue(spells[spell_id].formula[i],
                                                       spells[spell_id].base[i],
                                                       spells[spell_id].max[i],
                                                       casterlevel, spell_id);
                break;
            default: {
				// do we need to do anyting here?
					 }
			}
		}
	}
}

void Mob::DoBuffTic(int16 spell_id, int32 ticsremaining, int8 caster_level, Mob* caster) {
	if (!spells_loaded)
		return;
	if (spell_id != 0xFFFF) {
		for (int i=0; i < 12; i++) {
			switch (spells[spell_id].effectid[i]) {
			case SE_CurrentHP: {
				if (this->IsClient() || caster != 0){
					adverrorinfo = 41;
					this->ChangeHP(caster,
                                   CalcSpellValue(spells[spell_id].formula[i],
                                                  spells[spell_id].base[i],
                                                  spells[spell_id].max[i],
                                                  caster_level, spell_id),
                                   spell_id, i, true);
					adverrorinfo = 4;
				}
				break;
			}
			case SE_HealOverTime: {
				if (this->IsClient() || caster != 0){
					adverrorinfo = 42;
					this->ChangeHP(caster,
                                   CalcSpellValue(spells[spell_id].formula[i],
                                                  spells[spell_id].base[i],
                                                  spells[spell_id].max[i],
                                                  caster_level, spell_id),
                                   spell_id, i, true);
					adverrorinfo = 4;
				}
				break;
			}
			case SE_CurrentMana: {
			    this->SetMana(GetMana() +
                              CalcSpellValue(spells[spell_id].formula[i],
                                             spells[spell_id].base[i],
                                             spells[spell_id].max[i],
                                             caster_level, spell_id));
				break;
            }
            case SE_Charm: {
                bool bBreak = false;

                // define spells with fixed duration
                // this is handled by the server, and not by the spell database
                switch(spell_id) {
                case 3371://call of the banshee
                case 1707://dictate
                    bBreak = true;
                }

                if (!bBreak && caster) {
                    int cha = caster->GetCHA();
                    float r1 = (float)rand()/(float)RAND_MAX;
                    float r2 = (float)cha  + (caster->GetLevel()/3) / 255.0f;

                    if (r1 > r2) {
                        BuffFadeByEffect(SE_Charm);
                    }
                }
                break;
            }
            case SE_Root: {
		        float r1 = (float)rand()/RAND_MAX;
		        float r2 = (float)(GetMR() - caster_level)/255.0f;
		        cout<<"Root:"<<(float)r1<<":"<<r2<<endl;
		        if ( r1 < r2 )
		            BuffFadeByEffect(SE_Root);
//		rooted = false;
                break;
            }
            // todo: add other random timer effects here
			default: {
				// do we need to do anyting here?
					 }
			}
		}
	}
}

void Mob::BuffFadeByStackCommand(int16 spellid, sint8 iButNotThisSlot) {
	if (spellid >= SPDAT_RECORDS)
		return;
	for (int i=0; i<12; i++) {
		if (spells[spellid].effectid[i] == SE_StackingCommand1 ||
            spells[spellid].effectid[i] == SE_StackingCommand2) {
			BuffFadeByEffect(spells[spellid].base[i],
                             iButNotThisSlot,
                             IsBardSong(spellid), false);
        }
	}
	CalcBonuses();
}

// neotokyo: we gonna need that for dispelling type of spells
void Mob::BuffFadeBySlot(sint8 slot, bool iRecalcBonuses) {
	if (slot < 0 || slot > BUFF_COUNT)
		return;
	if (this->IsClient() && buffs[slot].spellid < SPDAT_RECORDS)
		this->CastToClient()->MakeBuffFadePacket(buffs[slot].spellid, slot);
	if (buffs[slot].spellid == 0xFFFF)
		return;

	bool t_bardsong = (IsBardSong(buffs[slot].spellid) &&
                       spells[buffs[slot].spellid].targettype == ST_AEBard) ? true:false;

	if (buffs[slot].spellid < SPDAT_RECORDS) {
		for (int i=0; i < 12; i++) {
			switch (spells[buffs[slot].spellid].effectid[i]) {
			case SE_SummonHorse: {
				if (this->IsClient()) {
					Mob* horse = entity_list.GetMob(this->CastToClient()->GetHorseId());
					if (horse) horse->Depop();
					this->CastToClient()->SetHasMount(false);
				}
				break;
								 }
			case SE_Illusion: {
				SendIllusionPacket(0, GetBaseGender());
				break;
							  }
			case SE_Levitate: {
				this->SendAppearancePacket(19, 0);
				break;
							  }
			case SE_Invisibility: {
				invisible = false;
				break;
			}
			case SE_Rune: {
				SetRune(0);
                break;
			}
			case SE_AbsorbMagicAtt: {
				SetMagicRune(0);
                break;
			}
            case SE_Familiar: {
                Mob * myfamiliar = GetFamiliar();
                if (!myfamiliar)
                    break; // familiar already gone
    			myfamiliar->CastToNPC()->Depop();
                SetFamiliarID(0);
                break;
            }
            case SE_AttackSpeed: {
				this->SetHaste(0);//what about haste items? other spells?
				break;
			}
			case SE_Mez: {
				this->mezzed = false;
                break;
			}
			case SE_Charm: {
				Mob* tempmob = GetOwner();
                SetOwnerID(0);
				if(tempmob) {
					tempmob->SetPet(0);
                }
                if (IsAIControlled()) {
                    // clear the hate list of the mobs
                    entity_list.ReplaceWithTarget(this, tempmob);
                    WhipeHateList();
                    if (tempmob)
                        AddToHateList(tempmob, 1, 0);
					SendAppearancePacket(0x0e,0x64);
                }
                if (tempmob && tempmob->IsClient()) {
                    APPLAYER *app = new APPLAYER(OP_Charm, sizeof(Charm_Struct));
                    Charm_Struct *ps = (Charm_Struct*)app->pBuffer;
                    ps->owner_id = tempmob->GetID();
                    ps->pet_id = this->GetID();
                    ps->command = 0;
                    tempmob->CastToClient()->FastQueuePacket(&app);
                }
				if (this->IsClient()) {
					if (this->CastToClient()->IsLD())
						AI_Start(CLIENT_LD_TIMEOUT);
					else
						AI_Stop();
				}
    			break;
            }
			case SE_Root: {
				rooted = false;
				break;
			}
			}
		}
		Mob *p = entity_list.GetMob(buffs[slot].casterid);
		if (p && p->IsClient() && p!=this && !t_bardsong) {
			p->CastToClient()->Message(MT_Broadcasts,
                                       "Your %s spell has worn off.",
                                       spells[buffs[slot].spellid].name);
		}
	}
	buffs[slot].spellid = 0xFFFF;
	if (iRecalcBonuses)
		CalcBonuses();
}

void Mob::BuffFade(int16 spell_id) {
	if (!spells_loaded)
		return;
	// 0xFFFe is my code for nuke all buffs
	for (int j=0; j<BUFF_COUNT; j++) {
		if (buffs[j].spellid == spell_id || spell_id == 0xFFFe) {
			BuffFadeBySlot(j, false);
		}
	}
	CalcBonuses();
}

void Client::MakeBuffFadePacket(int16 spell_id, int32 slot_id) {
	//	return;
	APPLAYER* outapp = new APPLAYER;
	outapp->opcode = OP_Buff;
	outapp->size = sizeof(SpellBuffFade_Struct);
	outapp->pBuffer = new uchar[outapp->size];

	SpellBuffFade_Struct* sbf = (SpellBuffFade_Struct*) outapp->pBuffer;
	memset(sbf, 0, sizeof(SpellBuffFade_Struct));
	sbf->unknown000[0] = 0x02;
	sbf->unknown000[1] = 0x07;
	sbf->unknown000[2] = 0x0A;
	sbf->spellid = spell_id;

	sbf->slotid = slot_id;
    sbf->player_id = GetID();

	sbf->bufffade = 0x01;
	QueuePacket(outapp);
	//DumpPacket(outapp);
	delete outapp;
}

void Client::MakeHorseSpawnPacket(int16 spell_id) {
	if(!hasmount && ((spell_id >= 2862  && spell_id <= 2876) ||
                     (spell_id == 2919 || spell_id == 2917))) {
		// Spell: 2862 Tan Rope
		// Spell: 2863 Tan Leather
		// Spell: 2864 Tan Silken
		// Spell: 2865 Brown Chain
		// Spell: 2866 Tan Ornate Chain
		// Spell: 2867 White Rope
		// Spell: 2868 White Leather
		// Spell: 2869 White Silken
		// Spell: 2870 White Chain
		// Spell: 2871 White Ornate Chain
		// Spell: 2872 Black Rope
		// Spell: 2919 Tan Rope
		// Spell: 2917 Black Chain,		
		
		// No Horse, lets get them one.
		NPCType* npc_type = new NPCType;
		memset(npc_type, 0, sizeof(NPCType));
		char f_name[64];
		strcpy(f_name,this->GetName());
		strcat(f_name,"`s_Mount");
		strcpy(npc_type->name,f_name);
		npc_type->cur_hp = 1; 
		npc_type->max_hp = 1; 
		npc_type->race = 216; 
		npc_type->gender = 0; 
		npc_type->class_ = 1; 
		npc_type->deity= 1;
		npc_type->level = 1;
		npc_type->npc_id = 0;
		npc_type->loottable_id = 0;
		// Is it a White Horse?
		if (spell_id >= 2862 && spell_id <= 2866) {
			npc_type->texture = 0;
		// Is it a Tan/Brown Horse?
        } else if ((spell_id >= 2867 && spell_id <= 2871) || spell_id == 2919) {
			npc_type->texture = 1;
		// Must be a Black Horse then.
        } else {
		    npc_type->texture = 2;
        }
		npc_type->light = 0;
		npc_type->fixedZ = 1;
		npc_type->STR = 75;
		npc_type->STA = 75;
		npc_type->DEX = 75;
		npc_type->AGI = 75;
		npc_type->INT = 75;
		npc_type->WIS = 75;
		npc_type->CHA = 75;

		npc_type->walkspeed = 8;
		npc_type->runspeed = 10;
		
		NPC* horse = new NPC(npc_type, 0, GetX(), GetY(), GetZ(), GetHeading());
		delete npc_type;

		entity_list.AddNPC(horse, false);
		APPLAYER* outapp = new APPLAYER;
		horse->CreateHorseSpawnPacket(outapp,this->GetName(), this->GetID());
		entity_list.QueueClients(horse, outapp);
		safe_delete(outapp);
		// Okay, lets say he has a horse now.

		hasmount = true;
		int16 tmpID = horse->GetID();
		SetHorseId(tmpID);
    } else {
		if (hasmount)
			Message(13,"You already have a Horse.  Get off (or zone) Fatbutt!");
		else
			Message(13,"I dont know what horse spell this is! (%i)", spell_id);
	}
}

void Client::SetBindPoint() {
	pp.bind_point_zone = zone->GetZoneID();
	pp.bind_location[0][0] = x_pos;
	pp.bind_location[1][0] = y_pos;
	pp.bind_location[2][0] = z_pos;
}

void Client::GoToBind() {
	if (pp.bind_point_zone == zone->GetZoneID()) { //if same zone no reason to zone
		GMMove(pp.bind_location[0][0],
               pp.bind_location[1][0],
               pp.bind_location[2][0]);
    } else {
		MovePC(pp.bind_point_zone,
               pp.bind_location[0][0],
               pp.bind_location[1][0],
               pp.bind_location[2][0], 1); //lets zone
    }
}


char* Mob::GetRandPetName() {
	char petnames[77][64] = { "Gabeker","Gann","Garanab","Garn","Gartik",
            "Gebann","Gebekn","Gekn","Geraner","Gobeker","Gonobtik","Jabantik",
            "Jasarab","Jasober","Jeker","Jenaner","Jenarer","Jobantik",
            "Jobekn","Jonartik","Kabann","Kabartik","Karn","Kasarer","Kasekn",
            "Kebekn","Keber","Kebtik","Kenantik","Kenn","Kentik","Kibekab",
            "Kobarer","Kobobtik","Konaner","Konarer","Konekn","Konn","Labann",
            "Lararer","Lasobtik","Lebantik","Lebarab","Libantik","Libtik",
            "Lobn","Lobtik","Lonaner","Lonobtik","Varekab","Vaseker","Vebobab",
            "Venarn","Venekn","Vener","Vibobn","Vobtik","Vonarer","Vonartik",
            "Xabtik","Xarantik","Xarar","Xarer","Xeber","Xebn","Xenartik",
            "Xeratik","Xesekn","Xonartik","Zabantik","Zabn","Zabeker","Zanab",
            "Zaner","Zenann","Zonarer","Zonarn" };
//	printf("Using %s\n", petreturn);
	int r = (rand()  % (77 - 1)) + 1;
	printf("Pet being created: %s\n",petnames[r]); // DO NOT COMMENT THIS OUT!
	if(r > 77) // Just in case
		r=77;
	return petnames[r];
}

int16 Mob::CalcPetLevel(int16 nlevel, int16 nclass) {
	//int plevel = 0;
	if (nclass == 13)
		return (nlevel - 10);
	else
		return (nlevel - 12);
}

sint32 Mob::CalcPetHp(int8 levelb, int8 classb, int8 STA) {
	int8 multiplier = 0;
	sint32 base_hp = 0;
	switch(classb) {
	case WARRIOR:
		if (levelb < 20)
			multiplier = 22;
		else if (levelb < 30)
			multiplier = 23;
		else if (levelb < 40)
			multiplier = 25;
		else if (levelb < 53)
			multiplier = 27;
		else if (levelb < 57)
			multiplier = 28;
		else
			multiplier = 30;
		break;
	case DRUID:
	case CLERIC:
	case SHAMAN:
		multiplier = 15;
		break;

	case PALADIN:
	case SHADOWKNIGHT:
		if (levelb < 35)
			multiplier = 21;
		else if (levelb < 45)
			multiplier = 22;
		else if (levelb < 51)
			multiplier = 23;
		else if (levelb < 56)
			multiplier = 24;
		else if (levelb < 60)
			multiplier = 25;
		else
			multiplier = 26;
		break;

	case MONK:
	case BARD:
	case ROGUE:
	case BEASTLORD:
		if (levelb < 51)
			multiplier = 18;
		else if (levelb < 58)
			multiplier = 19;
		else
			multiplier = 20;
		break;

	case RANGER:
		if (levelb < 58)
			multiplier = 20;
		else
			multiplier = 21;
		break;

	case MAGICIAN:
	case WIZARD:
	case NECROMANCER:
	case ENCHANTER:
		multiplier = 12;

		break;

	default:
		if (levelb < 35)
			multiplier = 21;
		else if (levelb < 45)
			multiplier = 22;
		else if (levelb < 51)
			multiplier = 23;
		else if (levelb < 56)
			multiplier = 24;
		else if (levelb < 60)
			multiplier = 25;
		break;
	}

	if (multiplier == 0)
	{
		cerr << "Multiplier == 0 in Client::CalcBaseHP" << endl;
	}

	//	cout << "m:" << (int)multiplier << " l:" << (int)levelb << 
    //	" s:" << (int)GetSTA() << endl;

	base_hp = 5 + (multiplier*levelb) + ((multiplier*levelb*STA) + 1)/300;
	return base_hp;
}

void Mob::MakePet(const char* pettype) {
/*Baron-Sprite: Pet types were conflicting all over...
 * was rushed it appears.  I have corrected pet types and
 * assigned ranges for pet types. PLEASE follow these ranges 
 * if you ever add/modify pets: The Pettype ranges for each pet 
 * are shown next to their summoning name, take a minute and look. 
 * Since mage pets are not as percise as necromancers, I am going i
 * to reserve them a large range for future additions I will add.
 *
 * However their base values will still be reserved as 0-3 and epic 
 * will stay on 4. Also I may want to note that even though pet 
 * ranges and types are reserved, it doesn't mean they will be used, 
 * however please respect the reserved spots.  Thanks!
TODO: Define specifics for mage pets. - Make anything missing on this list ^_^
*/
	//Baron-Sprite:  Mage pets have been hax0red for now.
	if (strncmp(pettype, "SumEarthR", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 0. ALSO 74-87.
		int8 tmp = atoi(&pettype[9]);
		if (tmp >= 2 && tmp <= 15) {
                   switch (tmp) { 
                     case  2: MakePet(6, 1, 75, 0, 74,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  3: MakePet(9, 1, 75, 0, 75,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  4: MakePet(12, 1, 75, 0, 76,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  5: MakePet(15, 1, 75, 0, 77,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  6: MakePet(18, 1, 75, 0, 78,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  7: MakePet(21, 1, 75, 0, 79,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  8: MakePet(25, 1, 75, 0, 80,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  9: MakePet(29, 1, 75, 0, 81,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 10: MakePet(33, 1, 75, 0, 82,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 11: MakePet(37, 1, 75, 0, 83,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 12: MakePet(41, 1, 75, 0, 84,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 13: MakePet(45, 1, 75, 0, 85,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 14: MakePet(48, 1, 75, 0, 86,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 15: MakePet(60, 1, 75, 0, 87,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                  } //switch 
        } else {
			Message(0, "Error: Unknown Earth Pet formula");
		}
    } else if (strncmp(pettype, "SumFireR", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 1. ALSO 88-101.
		int8 tmp = atoi(&pettype[8]);
		if (tmp >= 2 && tmp <= 15) {
			switch (tmp) { 
                     case  2: MakePet(6, 1, 75, 1, 88,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  3: MakePet(9, 1, 75, 1, 89,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                             break; 
                     case  4: MakePet(12, 1, 75, 1, 90,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  5: MakePet(15, 1, 75, 1, 91,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  6: MakePet(18, 1, 75, 1, 92,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  7: MakePet(21, 1, 75, 1, 93,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  8: MakePet(25, 1, 75, 1, 94,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  9: MakePet(29, 1, 75, 1, 95,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 10: MakePet(33, 1, 75, 1, 96,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 11: MakePet(37, 1, 75, 1, 97,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 12: MakePet(41, 1, 75, 1, 98,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 13: MakePet(45, 12, 75, 1, 99,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 14: MakePet(48, 12, 75, 1, 100,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 15: MakePet(60, 12, 75, 1, 101,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                  }  
        } else {
			Message(0, "Error: Unknown Fire Pet formula");
		}
    } else if (strncmp(pettype, "SumAirR", 7) == 0) { //Baron-Sprite: This Pettype is reserved to 3. ALSO 60-73.
		int8 tmp = atoi(&pettype[7]);
        if (tmp >= 2 && tmp <= 15) {
                   switch (tmp) { 
                     case  2: MakePet(6, 1, 75, 3, 60,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  3: MakePet(9, 1, 75, 3, 61,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  4: MakePet(12, 1, 75, 3, 62,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  5: MakePet(15, 1, 75, 3, 63,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  6: MakePet(18, 1, 75, 3, 64,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  7: MakePet(21, 1, 75, 3, 65,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  8: MakePet(25, 1, 75, 3, 66,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  9: MakePet(29, 1, 75, 3, 67,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 10: MakePet(33, 1, 75, 3, 68,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 11: MakePet(37, 1, 75, 3, 69,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 12: MakePet(41, 1, 75, 3, 70,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 13: MakePet(45, 7, 75, 3, 71,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 14: MakePet(48, 7, 75, 3, 72,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 15: MakePet(60, 7, 75, 3, 73,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                  } 
        } else {
			Message(0, "Error: Unknown Air Pet formula");
		}
    } else if (strncmp(pettype, "SumWaterR", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 2. ALSO 102-115.
		int8 tmp = atoi(&pettype[9]);
		if (tmp >= 2 && tmp <= 15) {
			                   switch (tmp) { 
                     case  2: MakePet(6, 1, 75, 2, 102,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  3: MakePet(9, 1, 75, 2, 103,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  4: MakePet(12, 1, 75, 2, 104,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  5: MakePet(15, 1, 75, 2, 105,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  6: MakePet(18, 1, 75, 2, 106,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  7: MakePet(21, 1, 75, 2, 107,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  8: MakePet(25, 1, 75, 2, 108,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case  9: MakePet(29, 1, 75, 2, 109,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 10: MakePet(33, 1, 75, 2, 110,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 11: MakePet(37, 1, 75, 2, 111,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 12: MakePet(41, 1, 75, 2, 112,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 13: MakePet(45, 9, 75, 2, 113,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 14: MakePet(48, 9, 75, 2, 114,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                     case 15: MakePet(60, 9, 75, 2, 115,4 + (((float)(tmp - 2) / 14) * 3), 1); 
                              break; 
                  } 
        } else {
			Message(0, "Error: Unknown Water Pet formula");
		}
    } else if (strncmp(pettype, "Familiar1",9) == 0) { // neotokyo: reserved type 217 for familiars
        MakePet(27,1,46,1,120,3,217); //Baron-Sprite: This Pettype is reserved to 120-124.
    } else if (strncmp(pettype, "Familiar2",9) == 0) {
        MakePet(47,1,46,1,121,3,217);
    } else if (strncmp(pettype, "Familiar3",9) == 0) {
        MakePet(50,1,89,4,122,3,217);
    } else if (strncmp(pettype, "Familiar4",9) == 0) {
        MakePet(58,1,89,4,123,3,217);
    } else if (strncmp(pettype, "Familiar5",9) == 0) {
        MakePet(60,1,89,1,124,3,217);
    } else if (strncmp(pettype, "SpiritWolf", 10) == 0) { //Baron-Sprite: This Pettype is reserved to 40-45.  Looks sloppy, sorry.
		int8 tmp = atoi(&pettype[11]);
		
		switch (tmp) {
		case 42:
			MakePet(58, 1, 42, 0, 45, 7, 3);
			break;
		case 37:
			MakePet(37, 1, 42, 0, 44, 7, 3);
			break;
		case 34:
			MakePet(34, 1, 42, 0, 43, 7, 3);
			break;
		case 30:
			MakePet(30, 1, 42, 0, 42, 7, 3);
			break;
		case 27:
			MakePet(27, 1, 42, 0, 41, 7, 3);
			break;
		case 24:
			MakePet(24, 1, 42, 0, 40, 7, 3);
			break;
	    default:
			cout << "Unknown pettype: " << tmp<< " : Generating default type." << endl;
			MakePet(24, 1, 42, 0, 40, 7, 3);
			break;
		}
    } else if (strncmp(pettype, "BLpet", 5) == 0) { //Baron-Sprite: This Pettype is reserved to 125-137
		int8 ptype = atoi(&pettype[5]);
		int crace = this->CastToClient()->GetRace();
		int prace=0;

		int mat=0;
        float size_mod = 1;
		#ifdef DEBUG
			cout << "Setting stats for BL Pet for Race: " << crace << endl;
		#endif
		
		switch ( crace ) {
		case VAHSHIR:
			prace = 63;
			size_mod = 1.7f;
			break;
		case TROLL:
			prace=91;
			break;
		case OGRE:
			prace=43;
			mat=3;
			break;
		case IKSAR:
			prace=42;
			break;
		case BARBARIAN:
			prace=42;
			mat=2;
            size_mod = 1.5f;
			break;
		default:
			cout << "No pet type modifications defined for race: " << crace << endl;
			break;
		}
		#ifdef DEBUG
			cout << "Summoning BeastLord Pet: " << ptype << endl;
		#endif
		switch ( ptype ) {
		case 51:
			MakePet(60, 1, prace, mat, 136, 6*size_mod, 5);
			break;
		case 49:
			MakePet(60, 1, prace, mat, 135, 5.8*size_mod, 5);
			break;
		case 47: 
			MakePet(47, 1, prace, mat, 134, 5.6*size_mod, 5);
			break;
		case 45:
			MakePet(45, 1, prace, mat, 133, 5.4*size_mod, 5);
			break;
		case 43:
			MakePet(43, 1, prace, mat, 132, 5.2*size_mod, 5);
			break;
		case 41:
			MakePet(41, 1, prace, mat, 131, 5*size_mod, 5);
			break;
		case 39:
			MakePet(39, 1, prace, mat, 130, 4.5*size_mod, 5);
			break;
		case 31:
			MakePet(31, 1, prace, mat, 129, 4*size_mod, 5);
			break;
		case 26:
			MakePet(26, 1, prace, mat, 128, 3.5*size_mod, 5);
			break;
		case 22:
			MakePet(22, 1, prace, mat, 127, 3*size_mod, 5);
			break;
		case 16:
			MakePet(16, 1, prace, mat, 126, 2.6*size_mod, 5);
			break;
		case 9:
			MakePet(9, 1, prace, mat, 125, 2.3*size_mod, 5);
			break;
		default:
			MakePet(10, 1, prace, mat, 125, 2*size_mod, 5);
	        	cout << "ptype not found: Making default BL pet." << endl;
			break;	
		
		}
    } else if (strncmp(pettype, "BLBasePet", 9) == 0) { //Baron-Sprite: This Pettype does not need a reserve, it is only a warning message.
			Message(13, "Beastlord pets are summon via the Spirit of Sharik, Khaliz, Keshuval, Herikol, Yekan, Kashek, Omakin, Zehkes, Khurenz, Khati Sha, Arag, or Sorsha spell line.  The Summon Warder ability was taken out of live sometime ago and replaced with this method. ");
    } else if (strncmp(pettype, "Animation", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 46-59.
		int8 ptype = atoi(&pettype[9]);

		switch ( ptype ) {
		case 14:
			MakePet(58, 1, 127, 0, 59, 6, 2);
			break;
		case 13:
			MakePet(41, 1, 127, 0, 58, 6, 2);
			break;
		case 12:
			MakePet(37, 1, 127, 0, 57, 6, 2);
			break;
		case 11:
			MakePet(34, 1, 127, 0, 56, 6, 2);
			break;
		case 10:
			MakePet(32, 1, 127, 0, 55, 6, 2);
			break;
		case 9:
			MakePet(29, 1, 127, 0, 54, 6, 2);
			break;
		case 8:
			MakePet(26, 1, 127, 0, 53, 6, 2);
			break;
		case 7:
			MakePet(22, 1, 127, 0, 52, 6, 2);
			break;
		case 6:
			MakePet(18, 1, 127, 0, 51, 6, 2);
			break;
		case 5:
			MakePet(15, 1, 127, 0, 50, 6, 2);
			break;
		case 4:
			MakePet(11, 1, 127, 0, 49, 6, 2);
			break;
		case 3:
			MakePet(8, 1, 127, 0, 48, 6, 2);
			break;
		case 2:
			MakePet(4, 1, 127, 0, 47, 6, 2);
			break;
		case 1:
			MakePet(1, 0, 127, 0, 46, 6, 2);
			break;
		default:
			MakePet(1, 0, 127, 0, 46, 6, 2);
			cout << "ptype not found: Making default animation pet." << endl;
			break;
		}
    } else if (strncmp(pettype, "SumSword", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 18.
        // for testing make an chanter pet
		MakePet(59, 1, 127,0,46,0,2);
    } else if (strncmp(pettype, "skel_pet_", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 22-39.
		char sztmp[50];
		strcpy(sztmp, pettype);
		sztmp[11] = 0;
		int8 tmp = atoi(&sztmp[9]);
		//Baron-Sprite: No need for level algorithim - Since pet levels are now fixed in EQLive.
		//Baron-Sprite: MakePet(level, class, race, texture, pettype, size, type) 0 Can be a placeholder.
		if (tmp >= 65) {
			MakePet(60, 1, 85, 0, 39, 7, 4);
        } else if (tmp >= 63) {
			MakePet(60, 1, 85, 0, 38, 7, 4);
        } else if (tmp >= 61) {
			MakePet(60, 1, 85, 0, 37, 7, 4);
        } else if (tmp >= 47) {
			MakePet(47, 1, 85, 0, 36, 7, 4);
        } else if (tmp >= 44) {
			MakePet(44, 7, 60, 0, 35, 6, 4);
        } else if (tmp >= 43) {
			MakePet(43, 9, 60, 0, 34, 6, 4);
        } else if (tmp >= 41) {
			MakePet(41, 1, 60, 0, 33, 6, 4);
        } else if (tmp >= 37) {
			MakePet(37, 1, 60, 0, 32, 6, 4);
        } else if (tmp >= 33) {
			MakePet(33, 1, 60, 0, 31, 6, 4);
        } else if (tmp >= 29) {
			MakePet(29, 1, 60, 0, 30, 6, 4);
        } else if (tmp >= 25) {
			MakePet(25, 1, 60, 0, 29, 6, 4);
        } else if (tmp >= 22) {
			MakePet(22, 1, 60, 0, 28, 6, 4);
        } else if (tmp >= 19) {
			MakePet(19, 1, 60, 0, 27, 6, 4);
        } else if (tmp >= 16) {
			MakePet(16, 1, 60, 0, 26, 6, 4);
        } else if (tmp >= 11) {
			MakePet(11, 1, 60, 0, 25, 6, 4);
        } else if (tmp >= 9) {
			MakePet(9, 1, 60, 0, 24, 6, 4);
        } else if (tmp >= 5) {
			MakePet(5, 1, 60, 0, 23, 6, 4);
        } else if (tmp >= 1) {
			MakePet(1, 1, 60, 0, 22, 6, 4);
        } else {
			MakePet(1, 1, 60);
        }
	// in_level, in_class, in_race, in_texture, in_pettype, in_size, type
	/*	else if (strncmp(pettype, "MonsterSum", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 5-7.
	}
	*/
    } else if (strncmp(pettype, "Mistwalker", 10) == 0) { //Baron-Sprite: This Pettype is reserved to 8.
		MakePet(50, 1, 42, 0, 8, 7, 9);
		zone->AddAggroMob();
	    this->GetPet()->AddToHateList(target, 1);
		Mob* sictar = entity_list.GetMob(this->GetPetID());
		if (target)
			target->AddToHateList(sictar, 1, 0);
	/*
	else if (strncmp(pettype, "SumEarthYael", 12) == 0) { //Baron-Sprite: This Pettype is reserved to 9.
	}
	else if (strncmp(pettype, "DALSkelGolin", 12) == 0) { //Baron-Sprite: This Pettype is reserved to 10.
	}
	else if (strncmp(pettype, "SUMHammer", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 12. Unsure if this is same as 14-17... giving it it's own ID anyways.
	} */
    } else if (strncmp(pettype, "TunareBane", 10) == 0) { //Baron-Sprite: This Pettype is reserved to 13.
		MakePet(50, 1, 76, 3, 13 , 6, 12);
    } else if (strncmp(pettype, "DruidPet", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 11.
		MakePet(25, 0, 43, 1,11,2,11);
	/*	else if (strncmp(pettype, "SumHammer", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 14-17.
	}
	else if (strncmp(pettype, "SumDecoy", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 19.
	}
	else if (strncmp(pettype, "Burnout", 7) == 0) { //Baron-Sprite: This Pettype is reserved to 20.
	}

	else if (strncmp(pettype, "ValeGuardian", 12) == 0) { //Baron-Sprite: This Pettype is reserved to 21.
}*/
    } else if (strncmp(pettype, "SumMageMultiElement", 19) == 0) {
		MakePet(50,0,75,3,4,8,15); //Baron-Sprite: Mage Epic pet type should be 4
    } else {
		Message(13, "Unknown pet type: %s", pettype);
	}
}

void Mob::MakePet(int8 in_level, int8 in_class, int16 in_race,
                  int8 in_texture, int8 in_pettype, float in_size,
                  int8 type) {
	if (this->GetPetID() != 0) {
		return;
	}

	NPCType* npc_type = new NPCType;
	memset(npc_type, 0, sizeof(NPCType));
	if (in_level>1)
		npc_type->hp_regen = (int)(in_level/3);
	else
		npc_type->hp_regen = 1;

	if (in_race == 216) {
		npc_type->gender = 0;
	}
	else {
		npc_type->gender = 2;
	}

	if (this->IsClient())
		strcpy(npc_type->name, GetRandPetName());
	else {
		strcpy(npc_type->name, this->GetName());
		npc_type->name[29] = 0;
		npc_type->name[28] = 0;
		npc_type->name[19] = 0;
		strcat(npc_type->name, "'s_pet");
	}

	npc_type->level = in_level;
	npc_type->race = in_race;
	npc_type->class_ = in_class;
	npc_type->texture = in_texture;
	npc_type->helmtexture = in_texture;
	npc_type->runspeed = 1.25f;

	npc_type->walkspeed = 0.7f;
	npc_type->size = in_size;
	npc_type->npc_spells_id = this->GetNPCSpellsID();

	npc_type->max_hp = CalcPetHp(npc_type->level, npc_type->class_);
	npc_type->cur_hp = npc_type->max_hp;
	npc_type->fixedZ = 1;
	int pettype = in_pettype; //Baron-Sprite: Needed for necro pet types.
	int yourlevel = this->GetLevel();

	switch(type) {
	case 217: {
        	// wizards familiars
        	char f_name[50];
        	strcpy(f_name,this->GetName());
        	strcat(f_name,"'s Familiar");
        	strcpy(npc_type->name, f_name);
		npc_type->min_dmg = 0;  //Baron-Sprite: Naughty Familiar.  No Attack 4 u.

		npc_type->max_dmg = 0;
		npc_type->max_hp = 1000;
		break;
	}
	       case 1: { //Bentareth: Mage pets, as close live as I can find, need spell procs added 
                 //2 types of procs, last 3 in each category does a new type of proc 
                 //Air and earth do damage ~50 hp, water does double previous, and fire needs several wizard spells added 
                       npc_type->hp_regen = 6; //default case (true until lvl 39 pet) 
                       switch(pettype) { 
                         case 60: //Air pets begin 
                               npc_type->max_hp   = 75; 
                               npc_type->cur_hp   = 75; 
                               npc_type->min_dmg  = 6; 
                               npc_type->max_dmg  = 12; 
								break; 
						 case 61: 
                                npc_type->max_hp   = 175; 
                                npc_type->cur_hp   = 175; 
                                npc_type->min_dmg  = 9; 
                                npc_type->max_dmg  = 16; 
                                break; 
                          case 62: 
                                npc_type->max_hp   = 230; 
                                npc_type->cur_hp   = 230; 
                                npc_type->min_dmg  = 11; 
                                npc_type->max_dmg  = 18; 
                                break; 
                          case 63: 
                                npc_type->max_hp  = 360; 
                                npc_type->cur_hp  = 360; 
                                npc_type->min_dmg = 13; 
                                npc_type->max_dmg = 20; 
                                break; 
                          case 64: 
                                npc_type->max_hp  = 460; 
                                npc_type->cur_hp  = 460; 
                                npc_type->min_dmg = 15; 
                                npc_type->max_dmg = 22; 
                                break; 
                          case 65: 
                                npc_type->max_hp  = 580; 
                                npc_type->cur_hp  = 580; 
                                npc_type->min_dmg = 17; 
                                npc_type->max_dmg = 26; 
                                break; 
                          case 66: 
                                npc_type->max_hp  = 700; 
                                npc_type->cur_hp  = 700; 
                                npc_type->min_dmg = 20; 
                                npc_type->max_dmg = 28; 
                                break; 
                          case 67: 
                                npc_type->max_hp  = 800; 
                                npc_type->cur_hp  = 800; 
                                npc_type->min_dmg = 24; 
                                npc_type->max_dmg = 34; 
                               break; 
                          case 68: 
                                npc_type->max_hp   = 1015; 
                                npc_type->cur_hp   = 1015; 
                                npc_type->min_dmg  = 28; 
                                npc_type->max_dmg  = 40; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 69: 
                                npc_type->max_hp  = 1225; 
                                npc_type->cur_hp  = 1225; 
                                npc_type->min_dmg = 34; 
                                npc_type->max_dmg = 48; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 70: 
                                npc_type->max_hp  = 2205; 
                                npc_type->cur_hp  = 2205; 
                                npc_type->min_dmg = 38; 
                                npc_type->max_dmg = 56; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 71:  //from here in need to do damage proc 
                                npc_type->max_hp  = 2410; 
                                npc_type->cur_hp  = 2410; 
                                npc_type->min_dmg = 40; 
                                npc_type->max_dmg = 56; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 72: 
                                npc_type->max_hp  = 2700; 
                                npc_type->cur_hp  = 2700; 
                                npc_type->min_dmg = 50; 
                                npc_type->max_dmg = 68; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 73: 
                                sprintf(npc_type->npc_attacks, "E"); 
                               npc_type->max_hp  = 3800; 
                                npc_type->cur_hp  = 3800; 
                                npc_type->min_dmg = 70; 
                                npc_type->max_dmg = 83; 
                                npc_type->hp_regen = 100; 
                                break; 
                       //End of Air Pets, Begin Earth 
                          case 74: 
                                npc_type->max_hp   = 95; 
                                npc_type->cur_hp   = 95; 
                                npc_type->min_dmg  = 6; 
                                npc_type->max_dmg  = 12; 
                                break; 
                          case 75: 
                                npc_type->max_hp   = 250; 
                                npc_type->cur_hp   = 250; 
                                npc_type->min_dmg  = 9; 
                                npc_type->max_dmg  = 16; 
                                break; 
                          case 76: 
                                npc_type->max_hp   = 350; 
                                npc_type->cur_hp   = 350; 
                                npc_type->min_dmg  = 11; 
                                npc_type->max_dmg  = 18; 
                                break; 
                          case 77: 
                                npc_type->max_hp  = 520; 
                                npc_type->cur_hp  = 520; 
                                npc_type->min_dmg = 13; 
                                npc_type->max_dmg = 20; 
                                break; 
                          case 78: 
                                npc_type->max_hp  = 675; 
                                npc_type->cur_hp  = 675; 
                                npc_type->min_dmg = 15; 
                                npc_type->max_dmg = 22; 
                                break; 
                          case 79: 
                                npc_type->max_hp  = 830; 
                                npc_type->cur_hp  = 830; 
                                npc_type->min_dmg = 17; 
                                npc_type->max_dmg = 26; 
                                break; 
                          case 80: 
                                npc_type->max_hp  = 1000; 
                                npc_type->cur_hp  = 1000; 
                                npc_type->min_dmg = 20; 
                                npc_type->max_dmg = 28; 
                                break; 
                          case 81: 
                                npc_type->max_hp  = 1150; 
                                npc_type->cur_hp  = 1150; 
                                npc_type->min_dmg = 24; 
                                npc_type->max_dmg = 34; 
                               break; 
                          case 82: 
                                npc_type->max_hp   = 1450; 
                                npc_type->cur_hp   = 1450; 
                                npc_type->min_dmg  = 28; 
                                npc_type->max_dmg  = 40; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 83: 
                                npc_type->max_hp  = 1750; 
                                npc_type->cur_hp  = 1750; 
                                npc_type->min_dmg = 34; 
                                npc_type->max_dmg = 48; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 84: 
                                npc_type->max_hp  = 3150; 
                                npc_type->cur_hp  = 3150; 
                                npc_type->min_dmg = 38; 
                                npc_type->max_dmg = 56; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 85: //Damage proc from here on in 
                                npc_type->max_hp  = 3200; 
                                npc_type->cur_hp  = 3200; 
                                npc_type->min_dmg = 42; 
                                npc_type->max_dmg = 58; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 86: 
                                npc_type->max_hp  = 3300; 
                                npc_type->cur_hp  = 3300; 
                                npc_type->min_dmg = 52; 
                                npc_type->max_dmg = 70; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 87: 
                                sprintf(npc_type->npc_attacks, "E"); 
                                npc_type->max_hp  = 4800; 
                                npc_type->cur_hp  = 4800; 
                                npc_type->min_dmg = 72; 
                                npc_type->max_dmg = 85; 
                                npc_type->hp_regen = 100; 
                                break; 
                       // End of Earth Pets, Begin Fire, add flameshield effect 
                          case 88: 
                                npc_type->max_hp   = 50; 
                                npc_type->cur_hp   = 50; 
                                npc_type->min_dmg  = 6; 
                                npc_type->max_dmg  = 12; 
                                break; 
                          case 89: 
                                npc_type->max_hp   = 125; 
                                npc_type->cur_hp   = 125; 
                                npc_type->min_dmg  = 9; 
                                npc_type->max_dmg  = 16; 
                                break; 
                          case 90: 
                                npc_type->max_hp   = 180; 
                                npc_type->cur_hp   = 180; 
                                npc_type->min_dmg  = 11; 
                                npc_type->max_dmg  = 18; 
                                break; 
                          case 91: 
                                npc_type->max_hp  = 260; 
                                npc_type->cur_hp  = 260; 
                                npc_type->min_dmg = 13; 
                                npc_type->max_dmg = 20; 
                                break; 
                          case 92: 
                                npc_type->max_hp  = 340; 
                                npc_type->cur_hp  = 340; 
                                npc_type->min_dmg = 15; 
                                npc_type->max_dmg = 22; 
                                break; 
                          case 93: 
                                npc_type->max_hp  = 415; 
                                npc_type->cur_hp  = 415; 
                                npc_type->min_dmg = 17; 
                                npc_type->max_dmg = 26; 
                                break; 
                          case 94: 
                                npc_type->max_hp  = 500; 
                                npc_type->cur_hp  = 500; 
                                npc_type->min_dmg = 20; 
                                npc_type->max_dmg = 28; 
                                break; 
                          case 95: 
                                npc_type->max_hp  = 575; 
                                npc_type->cur_hp  = 575; 
                                npc_type->min_dmg = 24; 
                                npc_type->max_dmg = 34; 
                               break; 
                          case 96: 
                                npc_type->max_hp   = 725; 
                                npc_type->cur_hp   = 725; 
                                npc_type->min_dmg  = 28; 
                                npc_type->max_dmg  = 40; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 97: 
                                npc_type->max_hp  = 875; 
                                npc_type->cur_hp  = 875; 
                                npc_type->min_dmg = 34; 
                                npc_type->max_dmg = 48; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 98: 
                                npc_type->max_hp  = 1575; 
                                npc_type->cur_hp  = 1575; 
                                npc_type->min_dmg = 38; 
                                npc_type->max_dmg = 56; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 99: // Fire is a wizard from here on in, needs spells, 
                                npc_type->max_hp  = 1900; 
                                npc_type->cur_hp  = 1900; 
                                npc_type->min_dmg = 20; 
                                npc_type->max_dmg = 29; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 100: 
                                npc_type->max_hp  = 2080; 
                                npc_type->cur_hp  = 2080; 
                                npc_type->min_dmg = 24; 
                                npc_type->max_dmg = 36; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 101: 
                                sprintf(npc_type->npc_attacks, "E"); 
                                npc_type->max_hp  = 2400; 
                                npc_type->cur_hp  = 2400; 
                                npc_type->min_dmg = 30; 
                                npc_type->max_dmg = 45; 
                                npc_type->hp_regen = 100; 
                                break; 
                       // End of Fire Pets, Begin Water 
                          case 102: 
                                npc_type->max_hp   = 80; 
                                npc_type->cur_hp   = 80; 
                                npc_type->min_dmg  = 6; 
                                npc_type->max_dmg  = 12; 
                                break; 
                          case 103: 
                                npc_type->max_hp   = 200; 
                                npc_type->cur_hp   = 200; 
                                npc_type->min_dmg  = 9; 
                                npc_type->max_dmg  = 16; 
                                break; 
                          case 104: 
                                npc_type->max_hp   = 280; 
                                npc_type->cur_hp   = 280; 
                                npc_type->min_dmg  = 11; 
                                npc_type->max_dmg  = 18; 
                                break; 
                          case 105: 
                                npc_type->max_hp  = 420; 
                                npc_type->cur_hp  = 420; 
                                npc_type->min_dmg = 13; 
                                npc_type->max_dmg = 20; 
                                break; 
                          case 106: 
                                npc_type->max_hp  = 540; 
                                npc_type->cur_hp  = 540; 
                                npc_type->min_dmg = 15; 
                                npc_type->max_dmg = 22; 
                                break; 
                          case 107: 
                                npc_type->max_hp  = 660; 
                                npc_type->cur_hp  = 660; 
                                npc_type->min_dmg = 17; 
                                npc_type->max_dmg = 26; 
                                break; 
                          case 108: 
                                npc_type->max_hp  = 800; 
                                npc_type->cur_hp  = 800; 
                                npc_type->min_dmg = 20; 
                                npc_type->max_dmg = 28; 
                                break; 
                          case 109: 
                                npc_type->max_hp  = 920; 
                                npc_type->cur_hp  = 920; 
                                npc_type->min_dmg = 24; 
                                npc_type->max_dmg = 34; 
                          case 110: 
                                npc_type->max_hp   = 1160; 
                                npc_type->cur_hp   = 1160; 
                                npc_type->min_dmg  = 28; 
                                npc_type->max_dmg  = 40; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 111: 
                                npc_type->max_hp  = 1400; 
                                npc_type->cur_hp  = 1400; 
                                npc_type->min_dmg = 34; 
                                npc_type->max_dmg = 48; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 112: 
                                npc_type->max_hp  = 2520; 
                                npc_type->cur_hp  = 2520; 
                                npc_type->min_dmg = 38; 
                                npc_type->max_dmg = 56; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 113: //Rogue type now, should backstab, needs higher dmg proc 
                                npc_type->max_hp  = 2350; 
                                npc_type->cur_hp  = 2350; 
                                npc_type->min_dmg = 40; 
                                npc_type->max_dmg = 56; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 114: 
                                npc_type->max_hp  = 2450; 
                                npc_type->cur_hp  = 2450; 
                                npc_type->min_dmg = 50; 
                                npc_type->max_dmg = 58; 
                                npc_type->hp_regen = 30; 
                                break; 
                          case 115: 
                                sprintf(npc_type->npc_attacks, "E"); 
                                npc_type->max_hp  = 3300; 
                                npc_type->cur_hp  = 3300; 
                                npc_type->min_dmg = 70; 
                                npc_type->max_dmg = 81; 
                                npc_type->hp_regen = 100; 
                                break; 
                       } //switch (pettype) - End of Normal Mage pets 
       } 
       case 15: { // Mage Epic Pet fixed 
                       npc_type->max_hp  = 4300; 
                       npc_type->cur_hp  = 4300; 
                       npc_type->min_dmg = 50; 
                        npc_type->max_dmg = 80; 
                       npc_type->hp_regen=50; 					
						// TODO: NPCSPELLS
						//			sprintf(npc_type->npc_spells,"847 848 849");
						break;
	}
	case 2: { //Baron-Sprite: Enchanter Pets.  Some info from casters realm.
			npc_type->gender = 0;
			npc_type->equipment[7] = 34;
			npc_type->equipment[8] = 202;
			if (pettype == 59)
			{
			npc_type->max_hp = 2200;
			npc_type->cur_hp = 2200;
			npc_type->min_dmg = 53;
			npc_type->max_dmg = 60;
			npc_type->equipment[7] = 26;
			npc_type->equipment[8] = 26;
			}
			else if (pettype == 58)

			{
			npc_type->max_hp = 1400;
			npc_type->cur_hp = 1400;
			npc_type->min_dmg = 46;
			npc_type->max_dmg = 53;
			npc_type->equipment[7] = 26;
			npc_type->equipment[8] = 26;
			}
			else if (pettype == 57)
			{
			npc_type->max_hp = 1200;
			npc_type->cur_hp = 1200;
			npc_type->min_dmg = 44;
			npc_type->max_dmg = 50;
			npc_type->equipment[7] = 26;
			npc_type->equipment[8] = 26;
			}
			else if (pettype == 56)
			{
			npc_type->max_hp = 1000;
			npc_type->cur_hp = 1000;
			npc_type->min_dmg = 36;
			npc_type->max_dmg = 44;
			}
			else if (pettype == 55)
			{
			npc_type->max_hp = 900;
			npc_type->cur_hp = 900;
			npc_type->min_dmg = 29;
			npc_type->max_dmg = 36;
			}
			else if (pettype == 54)
			{
			npc_type->max_hp = 750;
			npc_type->cur_hp = 750;
			npc_type->min_dmg = 26;
			npc_type->max_dmg = 33;
			}
			else if (pettype == 53)
			{
			npc_type->max_hp = 600;
			npc_type->cur_hp = 600;
			npc_type->min_dmg = 21;
			npc_type->max_dmg = 28;
			}
			else if (pettype == 52)
			{
			npc_type->max_hp = 500;
			npc_type->cur_hp = 500;
			npc_type->min_dmg = 15;
			npc_type->max_dmg = 23;
			}

			else if (pettype == 51)
			{
			npc_type->max_hp = 350;
			npc_type->cur_hp = 350;
			npc_type->min_dmg = 10;
			npc_type->max_dmg = 18;
			}
			else if (pettype == 50)
			{
			npc_type->max_hp = 300;
			npc_type->cur_hp = 300;
			npc_type->min_dmg = 8;
			npc_type->max_dmg = 14;
			npc_type->equipment[7] = 3;
			}
			else if (pettype == 49)
			{
			npc_type->max_hp = 215;
			npc_type->cur_hp = 215;
			npc_type->min_dmg = 5;
			npc_type->max_dmg = 13;
			npc_type->equipment[7] = 3;

			}
			else if (pettype == 48)
			{
			npc_type->max_hp = 150;
			npc_type->cur_hp = 150;
			npc_type->min_dmg = 3;

			npc_type->max_dmg = 10;
			}
			else if (pettype == 47)
			{
			npc_type->max_hp = 75;
			npc_type->cur_hp = 75;
			npc_type->min_dmg = 2;
			npc_type->max_dmg = 7;
			}
			else if (pettype == 46)
			{
			npc_type->max_hp = 23;
			npc_type->cur_hp = 23;
			npc_type->min_dmg = 1;
			npc_type->max_dmg = 5;
			}
			break;
		}
	case 3: { //Baron-Sprite: Shaman pets.  Credits for information go mostly to eq.castersrealm.com.
			switch (pettype) {

			case 45:
				npc_type->max_hp = 2200;
				npc_type->cur_hp = 2200;
				npc_type->min_dmg = 52;
				npc_type->max_dmg = 60;
				sprintf(npc_type->npc_attacks, "E"); // should enrage, i guess
				break;
			case 44:
				npc_type->max_hp = 1600;
				npc_type->cur_hp = 1600;
				npc_type->min_dmg = 45;
				npc_type->max_dmg = 53;
				break;
			case 43:
				npc_type->max_hp = 1000;
				npc_type->cur_hp = 1000;
				npc_type->min_dmg = 40;
				npc_type->max_dmg = 47;
				break;
			case 42:
				npc_type->max_hp = 880;
				npc_type->cur_hp = 880;
				npc_type->min_dmg = 32;
				npc_type->max_dmg = 39;
				break;
			case 41:
				npc_type->max_hp = 800;
				npc_type->cur_hp = 800;
				npc_type->min_dmg = 25;
				npc_type->max_dmg = 31;
				break;
			case 40:
				npc_type->max_hp = 740;
				npc_type->cur_hp = 740;
				npc_type->min_dmg = 20;
				npc_type->max_dmg = 26;
				break;
			default:
				cout << "Fallthrough case for Shaman Pet." << endl;
				npc_type->max_hp = 25;
				npc_type->cur_hp = 25;
				npc_type->min_dmg = 1;
				npc_type->max_dmg = 3;
				break;
			}

		break;
		}
	case 4: { //Baron-Sprite: Necromancer pets.  Some of the info is from eqnecro.com
			npc_type->bodytype = 3;
			if(pettype == 39) //Baron-Sprite: This is defined above in the Makepet statement.  I use it to single out the individual pet spells.
			{
			npc_type->max_hp = 3800; //Max Life.
			npc_type->cur_hp = 3800; //Current Life.
			npc_type->min_dmg = 60;  //Minimum Damage.
			npc_type->max_dmg = 78;  //Maximum Damage.
// TODO: NPCSPELLS
//			sprintf(npc_type->npc_spells,"446 359"); //Spells allowed.
			sprintf(npc_type->npc_attacks, "E");
			} //Baron-Sprite:  You can also define things such as weapon and armor graphics/size, ect.  See the defines above.
			else if(pettype == 38)
			{
			npc_type->max_hp = 2200;
			npc_type->cur_hp = 2200;
			npc_type->min_dmg = 64;
			npc_type->max_dmg = 73;
// TODO: NPCSPELLS
//			sprintf(npc_type->npc_spells,"446");
			sprintf(npc_type->npc_attacks, "E");
			}
			else if(pettype == 37)
			{
			npc_type->max_hp = 2400;
			npc_type->cur_hp = 2400;
			npc_type->min_dmg = 60;
			npc_type->max_dmg = 73;
// TODO: NPCSPELLS
//			sprintf(npc_type->npc_spells,"446");
			sprintf(npc_type->npc_attacks, "E");

			}
			else if(pettype == 36)
			{
			npc_type->max_hp = 2300;
			npc_type->cur_hp = 2300;
			npc_type->min_dmg = 59;
			npc_type->max_dmg = 69;
// TODO: NPCSPELLS
//			sprintf(npc_type->npc_spells,"446 216");
			}
			else if(pettype == 35)
			{
			npc_type->max_hp = 1500;
			npc_type->cur_hp = 1500;
			npc_type->min_dmg = 52;
			npc_type->max_dmg = 59;
// TODO: NPCSPELLS
//			sprintf(npc_type->npc_spells,"445");

			}
			else if(pettype == 34)
			{
			npc_type->max_hp = 1400;
			npc_type->cur_hp = 1400;
			npc_type->min_dmg = 50;
			npc_type->max_dmg = 57;
// TODO: NPCSPELLS
//			sprintf(npc_type->npc_spells,"445");
			}
			else if(pettype == 33)
			{
			npc_type->max_hp = 2350;
			npc_type->cur_hp = 2350;
			npc_type->min_dmg = 48;
			npc_type->max_dmg = 55;
// TODO: NPCSPELLS
//			sprintf(npc_type->npc_spells,"445");
			}
			else if(pettype == 32)
			{
			npc_type->max_hp = 1300;
			npc_type->cur_hp = 1300;
			npc_type->min_dmg = 40;
			npc_type->max_dmg = 47;
// TODO: NPCSPELLS
//			sprintf(npc_type->npc_spells,"502");
			}
			else if(pettype == 31)
			{
			npc_type->max_hp = 1200;
			npc_type->cur_hp = 1200;
			npc_type->min_dmg = 32;
			npc_type->max_dmg = 39;
			}

			else if(pettype == 30)
			{
			npc_type->max_hp = 1000;
			npc_type->cur_hp = 1000;
			npc_type->min_dmg = 25;
			npc_type->max_dmg = 33;
			}
			else if(pettype == 29)
			{
			npc_type->max_hp = 775;
			npc_type->cur_hp = 775;
			npc_type->min_dmg = 21;
			npc_type->max_dmg = 28;
			}
			else if(pettype == 28)
			{
			npc_type->max_hp = 700;
			npc_type->cur_hp = 700;
			npc_type->min_dmg = 19;
			npc_type->max_dmg = 26;
			}
			else if(pettype == 27)
			{
			npc_type->max_hp = 375;
			npc_type->cur_hp = 375;
			npc_type->min_dmg = 15;
			npc_type->max_dmg = 22;
			}
			else if(pettype == 26)
			{
			npc_type->max_hp = 300;
			npc_type->cur_hp = 300;
			npc_type->min_dmg = 13;
			npc_type->max_dmg = 20;
			}
			else if(pettype == 25)
			{
			npc_type->max_hp = 250;
			npc_type->cur_hp = 250;
			npc_type->min_dmg = 11;
			npc_type->max_dmg = 16;
			}
			else if(pettype == 24)
			{
			npc_type->max_hp = 200;
			npc_type->cur_hp = 200;
			npc_type->min_dmg = 9;

			npc_type->max_dmg = 14;
			}
			else if(pettype == 23)
			{
			npc_type->max_hp = 100;
			npc_type->cur_hp = 100;
			npc_type->min_dmg = 7;

			npc_type->max_dmg = 12;
			}
			else if(pettype == 22)
			{
			npc_type->max_hp = 30;
			npc_type->cur_hp = 30;
			npc_type->min_dmg = 5;
			npc_type->max_dmg = 10;
			}
			else
			{
			npc_type->max_hp = 25;
			npc_type->cur_hp = 25;
			npc_type->min_dmg = 1;
			npc_type->max_dmg = 3;
			}
			break;
		}
	case 5: //Baron-Sprite: Beastlord pets.  Credits for information go mostly to eq.castersrealm.com.  What's new? :)
		{
		char f_name[50];
        strcpy(f_name,this->GetName());
        strcat(f_name,"`s warder");
        strcpy(npc_type->name, f_name);
			if (pettype == 136)
			{
			npc_type->max_hp = 4500;
			npc_type->cur_hp = 4500;
			npc_type->min_dmg = 71;
			npc_type->max_dmg = 78;
			}
			else if (pettype == 135)
			{
			npc_type->max_hp = 4000;
			npc_type->cur_hp = 4000;
			npc_type->min_dmg = 62;
			npc_type->max_dmg = 69;
			}
			else if (pettype == 134)

			{
			npc_type->max_hp = 3500;
			npc_type->cur_hp = 3500;
			npc_type->min_dmg = 53;
			npc_type->max_dmg = 60;
			}
			else if (pettype == 133)
			{
			npc_type->max_hp = 3100;
			npc_type->cur_hp = 3100;
			npc_type->min_dmg = 49;
			npc_type->max_dmg = 56;
			}
			else if (pettype == 132)
			{
			npc_type->max_hp = 2900;
			npc_type->cur_hp = 2900;
			npc_type->min_dmg = 47;
			npc_type->max_dmg = 54;
			}
			else if (pettype == 131)
			{
			npc_type->max_hp = 2700;
			npc_type->cur_hp = 2700;
			npc_type->min_dmg = 45;
			npc_type->max_dmg = 52;
			}
			else if (pettype == 130)
			{
			npc_type->max_hp = 1700;
			npc_type->cur_hp = 1700;
			npc_type->min_dmg = 42;
			npc_type->max_dmg = 49;
			}
			else if (pettype == 129)
			{
			npc_type->max_hp = 1400;
			npc_type->cur_hp = 1400;
			npc_type->min_dmg = 32;
			npc_type->max_dmg = 39;
			}
			else if (pettype == 128)
			{
			npc_type->max_hp = 1100;
			npc_type->cur_hp = 1100;
			npc_type->min_dmg = 23;
			npc_type->max_dmg = 30;
			}
			else if (pettype == 127)
			{
			npc_type->max_hp = 800;
			npc_type->cur_hp = 800;
			npc_type->min_dmg = 17;
			npc_type->max_dmg = 24;
			}
			else if (pettype == 126)
			{
			npc_type->max_hp = 500;
			npc_type->cur_hp = 500;
			npc_type->min_dmg = 11;
			npc_type->max_dmg = 18;
			}
			else if (pettype == 125)
			{
			npc_type->max_hp = 300;
			npc_type->cur_hp = 300;
			npc_type->min_dmg = 5;
			npc_type->max_dmg = 10;
			}
			else
			{
			npc_type->max_hp = 300;
			npc_type->cur_hp = 300;
			npc_type->min_dmg = 5;
			npc_type->max_dmg = 10;
			}
			if (in_race == 42 && in_texture == 0)
			{
			npc_type->gender = 1;
			}
			else if (in_race == 42 && in_texture == 2)
			{
			npc_type->gender = 2;
			}
			else {
			npc_type->gender = 2;
			}
			break;
		}
	case 9: { //Baron-Sprite:  Mistwalker :D
		char f_name[50];
        strcpy(f_name,this->GetName());

        strcat(f_name," pet");
        strcpy(npc_type->name, f_name);
			npc_type->max_hp = 400;
			npc_type->cur_hp = 400;
			npc_type->min_dmg = 1;
			npc_type->max_dmg = 35;
			break;
	}
	case 11: // Druid Pet...  Baron-Sprite: Info from www.eqdruids.com said about 100 hp.
		{
			npc_type->max_hp = 100;
			npc_type->cur_hp = 100;
			npc_type->min_dmg = 17;
			npc_type->max_dmg = 25;
			break;
		}
	case 12: //Baron-Sprite:  TunareBane.
		{
		char f_name[50];
        strcpy(f_name,this->GetName());
        strcat(f_name," pet");
        strcpy(npc_type->name, f_name);
			npc_type->max_hp = 2500;
			npc_type->cur_hp = 2500;
			npc_type->min_dmg = 71;
			npc_type->max_dmg = 78;
// TODO: NPCSPELLS
//			sprintf(npc_type->npc_spells,"1466");
			break;
		}
	default:
		break;
	}

	NPC* npc = new NPC(npc_type, 0,
                       this->GetX(), this->GetY(),
                       this->GetZ(), this->GetHeading());
	delete npc_type;
	npc->SetPetType(in_pettype);
	npc->SetOwnerID(this->GetID());
	entity_list.AddNPC(npc);
    if (type != 217)
	    this->SetPetID(npc->GetID());
    else
	    this->SetFamiliarID(npc->GetID());
	
}

void Mob::CheckBuffs() {
	return;
	if (this->casting_spell_id == 0) {
		this->CheckPet();
		int8 newtype[15] = { SE_ArmorClass, SE_STR, SE_DEX, SE_AGI, SE_WIS,
                             SE_INT, SE_CHA, SE_AttackSpeed, SE_MovementSpeed,
                             SE_DamageShield, SE_ResistFire, SE_ResistCold,
                             SE_ResistMagic, SE_ResistPoison, SE_ResistDisease };
		for (int h=0; h<15; h++) {
			if (!this->FindType(newtype[h])) {
				int16 buffid = FindSpell(this->class_, this->level,
                                         newtype[h], SPELLTYPE_SELF, 0,
                                         GetMana());
				if (buffid != 0) {
					this->CastSpell(buffid, this->GetID());
				}
			}
		}
	}
}

void Mob::CheckPet() {
	int16 buffid = 0;
	if (this->GetPetID() == 0 && 
       (this->GetClass() == 11 || this->GetClass() == 13)) {
		if (this->GetClass() == 13) {
			buffid = FindSpell(this->class_, this->level,
                               SE_SummonPet, SPELLTYPE_OTHER, 0,
                               GetMana());
        } else if (this->GetClass() == 11) {
			buffid = FindSpell(this->class_, this->level,
                               SE_NecPet, SPELLTYPE_OTHER, 0,
                               GetMana());
		}
		if (buffid != 0) {
			this->CastSpell(buffid, this->GetID());
		}
	}
}

int16 Mob::FindSpell(int16 classp, int16 level, int type,
                     FindSpellType spelltype, float distance,
                     sint32 mana_avail) {
    int i,j;

    int bestvalue = -1;
    int bestid = 0;

    if (classp < 1)
        return 0;
	classp = GetEQArrayEQClass(classp);
    if (level < 1)
        return 0;

    // purpose: find a suited spell for a class and level and type
    // the if's are here to filter out anything which isnt normal.
    // its possible that we miss some valid spells, but who cares.
    //  - neotokyo 19-Nov-02

	for (i = 0; i < SPDAT_RECORDS; i++) {
        // Filter all spells that should never be used
        if (spells[i].effectid[0] == SE_NegateIfCombat)
            continue;
        if (spells[i].targettype == ST_Group)
            continue;
        if (i == 2632)  // neotokyo: fix for obsolete BST pet summon spell
            continue;
        if (i == 1576)  // neotokyo: fix for torpor
            continue;
        if (spells[i].cast_time < 11)
            continue;
        if (spells[i].mana == 0)
            continue;

        // now for closer checks
        if (spelltype == SPELLTYPE_SELF) {
            if ( i == 357)  // fix for dark empathy
                continue;
            // check buffs 12 would be max, but 90% of all effects are in the first 4 slots
            for (j = 0; j < 5; j++) {
                // neotokyo: fix for pets
                if ( spells[i].effectid[j] == SE_Illusion &&
                     type != SE_Illusion)  // only let illusions thru if explicitly requested
                    continue;
                if ( spells[i].effectid[j] == type &&
                     spells[i].goodEffect != 0 &&
                     spells[i].classes[classp] <= level &&
                     spells[i].classes[classp] <= 65 &&
                     (spells[i].recast_time < 10000 ||
                      type == SE_SummonPet ||
                      type == SE_SummonBSTPet) && // neotokyo: fix for druid pets
                     (type == SE_AbsorbMagicAtt || type == SE_Rune ||
                      type == SE_NecPet || type == SE_SummonPet ||
                      spells[i].components[0] == -1 ) &&
                     spells[i].targettype != ST_Undead &&   // neotokyo: for  necro mend series
                     spells[i].targettype != ST_Group &&    // neotokyo: fix for group spells
                     spells[i].targettype != ST_Pet &&      // neotokyo: fix for beastlords casting pet heals on self
                     spells[i].targettype != ST_Summoned && // neotokyo: fix for vs. summoned spells on normal npcs
                     spells[i].targettype != ST_AETarget && // neotokyo: dont let em cast AEtarget spells
                     spells[i].mana <= mana_avail &&
                     spells[i].range >= distance) {
                    sint32 spellvalue;

                    // lets assume pet is always better if higher, so no formula needed
                    if (type == SE_NecPet ||
                        type == SE_SummonPet ||
                        type == SE_SummonBSTPet) {
                        spellvalue = spells[i].classes[classp];
                    } else {
                        spellvalue = CalcSpellValue(spells[i].formula[j],
                                                    spells[i].base[j],
                                                    spells[i].max[j],
                                                    level, i);
                    }

                    if (abs(spellvalue) > bestvalue) {
                        bestvalue = abs(spellvalue);
                        bestid = i;
                    }
                }
            }
        } else if (spelltype == SPELLTYPE_OFFENSIVE) {
            // check offensive spells
            for (j = 0; j < 5; j++) {
                if (spells[i].effectid[j] == SE_Illusion &&
                    type != SE_Illusion)  // only let illusions thru if explicitly requested
                    continue;
                if (spells[i].effectid[j] == type &&
                    spells[i].goodEffect == 0 &&
                    spells[i].classes[classp] <= level &&
                    spells[i].classes[classp] <= 65 &&
                    spells[i].recast_time < 10000 &&
                    spells[i].components[0] == -1 &&
                    spells[i].mana <= mana_avail &&
                    spells[i].targettype != ST_Undead &&   // neotokyo: thats for the necro mend series
                    spells[i].targettype != ST_Group &&    // neotokyo: fix for group spells
                    spells[i].targettype != ST_Pet &&      // neotokyo: fix for beastlords casting pet heals on self
                    spells[i].targettype != ST_Summoned && // neotokyo: fix for vs. summoned spells on normal npcs
                    spells[i].targettype != ST_AETarget && // neotokyo: dont let em cast AEtarget spells
                    spells[i].range >= distance) {
                    sint32 spellvalue = CalcSpellValue(spells[i].formula[j],
                                                       spells[i].base[j],
                                                       spells[i].max[j],
                                                       level, i);
                    if ( abs(spellvalue) > bestvalue ) {
                        bestvalue = abs(spellvalue);
                        bestid = i;
                    }
                }
            }
        } else if (spelltype == SPELLTYPE_OTHER) {
            if ( i == 357)  // fix for dark empathy
                continue;
            // healing and such
            for (j = 0; j < 5; j++) {
                if (spells[i].effectid[j] == SE_Illusion &&
                    type != SE_Illusion)  // only let illusions thru if explicitly requested
                    continue;
                if (spells[i].effectid[j] == type &&
                    spells[i].targettype != ST_Self &&
                    spells[i].goodEffect != 0 &&
                    spells[i].classes[classp] <= level &&
                    spells[i].classes[classp] <= 65 &&
                    spells[i].recast_time < 10000 &&
                    spells[i].components[0] == -1 &&
                    spells[i].targettype != ST_Undead &&   // neotokyo: thats for the necro mend series
                    spells[i].targettype != ST_Group &&    // neotokyo: fix for group spells
                    spells[i].targettype != ST_Pet &&      // neotokyo: fix for beastlords casting pet heals on self
                    spells[i].targettype != ST_Summoned && // neotokyo: fix for vs. summoned spells on normal npcs
                    spells[i].targettype != ST_AETarget && // neotokyo: dont let em cast AEtarget spells
                    spells[i].mana <= mana_avail &&
                    spells[i].range >= distance) {
                    sint32 spellvalue = CalcSpellValue(spells[i].formula[j],
                                                       spells[i].base[j],
                                                       spells[i].max[j],
                                                       level, i);
                    if ( abs(spellvalue) > bestvalue ) {
                        bestvalue = abs(spellvalue);
                        bestid = i;
                    }
                }
            }
        }
    } // for i

//    g_LogFile.write("for combination [class %02d][level %02d][SE_type %02d][type %02d] i selected the spell: %s",
//        classp, level, (int16)type, int16(spelltype), spells[bestid].name);
    return bestid;
}

#if 0
int16 Mob::FindSpell(int16 classp, int16 level, int8 type, int8 spelltype) {
	if (this->casting_spell_id != 0)
		return 0;

	if (spelltype == 2) // for future use
		spelltype = 0;

	//int count=0;
	int16 bestsofar = 0;
	int16 bestspellid = 0;
	for (int i = 0; i < SPDAT_RECORDS; i++) {
		if ((spells[i].targettype == ST_Tap && spelltype == 1) || (spells[i].targettype != ST_Group && spells[i].targettype != ST_Undead && spells[i].targettype != ST_Summoned && spells[i].targettype != ST_Pet && strstr(spells[i].name,"Summoning") == NULL)) {
			int Canuse = CanUse(i, classp, level);
			if (Canuse != 0) {
				for (int z=0; z < 12; z++) {
					int spfo = CalcSpellValue(spells[i].formula[z], spells[i].base[z], spells[i].max[z], this->GetLevel());
					if (spells[i].effectid[z] == SE_ArmorClass && type == SE_ArmorClass && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_TotalHP && type == SE_TotalHP && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_STR && type == SE_STR && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;

							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_DEX && type == SE_DEX && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_AGI && type == SE_AGI && !FindBuff(i)) {

						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {

							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_WIS && type == SE_WIS && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_INT && type == SE_INT && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_CHA && type == SE_CHA && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_MovementSpeed && type == SE_MovementSpeed && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_AttackSpeed && type == SE_AttackSpeed && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_ResistFire && type == SE_ResistFire && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_ResistCold && type == SE_ResistCold && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_ResistMagic && type == SE_ResistMagic && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_ResistDisease && type == SE_ResistDisease && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;

						}
					}
					if (spells[i].effectid[z] == SE_ResistPoison && type == SE_ResistPoison && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_DamageShield && type == SE_DamageShield && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_CurrentHPOnce && type == SE_CurrentHPOnce && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_SummonPet && type == SE_SummonPet && !FindBuff(i)) {
						if (Canuse > bestsofar) {
							bestsofar = Canuse;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_NecPet && type == SE_NecPet && !FindBuff(i)) {
						if (Canuse > bestsofar) {
							bestsofar = Canuse;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_CurrentHP && type == SE_CurrentHP && !FindBuff(i)) {
						if (spfo < 0 && (spells[i].buffduration + spfo) < bestsofar && spelltype == 1) {
							bestsofar = ((spells[i].buffduration * -1) + spfo);
							bestspellid = i;
						}
						if ((spfo + spells[i].buffduration) > bestsofar && spfo > 0 && spelltype == 0) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}

					}
				}
			}
		}
	}

	return bestspellid;
}
#endif

int16 Mob::CanUse(int16 spellid, int16 classa, int16 level) {
	classa = classa-1;
	for (int u=0; u<14; ++u) {
		if (u == classa && level >= spells[spellid].classes[u] &&
            spells[spellid].classes[u] != 0 &&
            spells[spellid].classes[u] != 255) {
			return spells[spellid].classes[u];
		}
	}
	return 0;
}

bool Mob::FindBuff(int16 spellid) {
	for (int i=0;i<15;i++) {
		if (buffs[i].spellid == spellid && buffs[i].spellid != 0xFFFF) {
			return true;
		}
	}
	return false;
}


bool Mob::IsEffectInSpell(sint16 spellid, int8 type) {
	if (spellid >= SPDAT_RECORDS)
		return false;
	for (int j = 0; j < 12; j++) {
		if (spells[spellid].effectid[j] == type) {
			return true;
        }
	}
	return false;
}

bool Mob::FindType(int8 type, bool bOffensive, int16 threshold) {
	for (int i = 0; i < BUFF_COUNT; i++) {
		if (buffs[i].spellid != 0xFFFF) {

			for (int j = 0; j < 12; j++) {
                // adjustments necessary for offensive npc casting behavior
                if (bOffensive) {
				    if (spells[buffs[i].spellid].effectid[j] == type) {
                        sint16 value = 
                                CalcSpellValue(buffs[i].durationformula,
                                               spells[buffs[i].spellid].base[j],
                                               spells[buffs[i].spellid].max[j],
                                               buffs[i].casterlevel,
                                               buffs[i].spellid);
                        LogFile->write(EQEMuLog::Normal, 
                                "FindType: type = %d; value = %d; threshold = %d",
                                type, value, threshold);
                        if (value < threshold)
                            return true;
                    }
                } else {
				    if (spells[buffs[i].spellid].effectid[j] == type )
					    return true;
                }
			}
		}
	}
	return false;
}

sint8 Mob::GetBuffSlotFromType(int8 type) {
	for (int i = 0; i < BUFF_COUNT; i++) {
		if (buffs[i].spellid != 0xFFFF) {
			for (int j = 0; j < 12; j++) {
				if (spells[buffs[i].spellid].effectid[j] == type )
					return i;
			}
		}
	}
    return -1;
}

bool Mob::AddProcToWeapon(int16 spell_id, bool bPerma, int8 iChance) {
	int i;
	if (bPerma) {
 		for (i = 0; i < MAX_PROCS; i++) {
			if (PermaProcs[i].spellID == 0xFFFF) {
				PermaProcs[i].spellID = spell_id;
				PermaProcs[i].chance = iChance;
				PermaProcs[i].pTimer = NULL;


				return true;
			}
		}
	cout << "Too many perma procs for " << GetName() << endl;
    } else {
		for (i = 0; i < MAX_PROCS; i++) {
			if (SpellProcs[i].spellID == 0xFFFF) {
				SpellProcs[i].spellID = spell_id;
				SpellProcs[i].chance = iChance;
				SpellProcs[i].pTimer = NULL;
				return true;
			}
		}
	cout << "Too many procs for " << GetName() << endl;
	}
    return false;
}

bool Mob::RemoveProcFromWeapon(int16 spell_id, bool bAll) {
	for (int i = 0; i < MAX_PROCS; i++) {
		if (bAll || SpellProcs[i].spellID == spell_id) {
			SpellProcs[i].spellID = 0xFFFF;
			SpellProcs[i].chance = 0;
			SpellProcs[i].pTimer = NULL;
		}
	}
    return true;
}
