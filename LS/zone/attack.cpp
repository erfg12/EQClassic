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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <iostream.h>

#include "client.h"
#include "npc.h"
#include "NpcAI.h"
#include "../common/packet_dump.h"
#include "../common/eq_packet_structs.h"
#include "skills.h"
#include "PlayerCorpse.h"
#include "spdat.h"
#include "zone.h"
#include "groups.h"

#ifdef WIN32
#define snprintf	_snprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#endif

extern Database database;
extern EntityList entity_list;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif




#if defined(GOTFRAGS) || defined(_DEBUG)
	#define REDUCE_BATTLE_SPAM
#endif

extern Zone* zone;

bool Mob::AttackAnimation(int &attack_skill, int16 &skillinuse, int Hand, const Item_Struct* weapon)
{
	// Determine animation
	APPLAYER app;
	app.opcode = OP_Attack;
	app.size = sizeof(Attack_Struct);
	app.pBuffer = new uchar[app.size];
	memset(app.pBuffer, 0, app.size);
	Attack_Struct* a = (Attack_Struct*)app.pBuffer;
	
	a->spawn_id = GetID();
	if(weapon)
	{
		switch(weapon->common.skill)
		{
		case 0: // 1H Slashing
        {
		    attack_skill = 1;
			skillinuse = _1H_SLASHING;
			a->type = 5;
			break;
		}
		case 1: // 2H Slashing
		{
		    attack_skill = 1;
			skillinuse = _2H_SLASHING;
			a->type = 3;
			break;
		}
		case 2: // Piercing
		{
		    attack_skill = 36;
			skillinuse = PIERCING;
			a->type = 2;
			break;
		}
		case 3: // 1H Blunt
		{
		    attack_skill = 0;
			skillinuse = _1H_BLUNT;
			a->type = 5;
			break;
		}
		case 4: // 2H Blunt
		{

		    attack_skill = 0;
			skillinuse = _2H_BLUNT;
			a->type = 4;
			break;
		}
		case 35: // 2H Piercing
		{
		    attack_skill = 36;
			skillinuse = PIERCING;
			a->type = 4;
			break;
		}
		case 45:
		{
		    attack_skill = 4;
            skillinuse = HAND_TO_HAND;
			a->type = 8;
			break;
		}
		default:
		{
			attack_skill = 4;
			skillinuse = HIGHEST_SKILL+1;
			a->type = 8;
			break;
		}
		}// switch
	}
	else
	{
		attack_skill = 4;
		skillinuse = HAND_TO_HAND;
		a->type = 8;
	}
	// Kaiyodo - If we're attacking with the seconary hand, play the duel wield anim
	if(Hand == 14)	// DW anim
		a->type = 6;
	
	a->a_unknown2[5] = 0x80;
	a->a_unknown2[6] = 0x3f;
	app.priority = 1;
	entity_list.QueueCloseClients(this, &app);
    return true;
}

bool Mob::AvoidDamage(Mob* other, sint32 &damage, int16 spell_id, int8 attack_skill, int Hand)
{
    if(DEBUG>=5) LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) in damage=%i", GetName(), other->GetName(), damage);
	float skill = 0.0f;

	// if we fight against spell we cant avoid it
	if ( spell_id != 0xFFFF )
        	return false;

	////////////////////////////////////////////////////////
	// To hit calcs go here goes here
	////////////////////////////////////////////////////////
	if (other->IsNPC() && !other->CastToNPC()->IsInteractive()){
		// FIXME this should always base hit chance on level difference of target and self
		float chancetohit = 0;
		int8 otherlevel = this->GetLevel();
		int8 mylevel = other->GetLevel();
		otherlevel = otherlevel ? otherlevel : 1;
		mylevel = mylevel ? mylevel : 1;
		if (mylevel-otherlevel >= 0)
		{
			// 83% (+30) basechance, if target if lower then me
			chancetohit = 68;
		}
		else
		{
			// based on leveldiff
			chancetohit = 68-(float)(( otherlevel - mylevel )*( otherlevel - mylevel ))/4;
		}
		int16 targetagi = this->GetAGI();
		targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);
		chancetohit -= (float)targetagi*0.05;
		chancetohit += (float)(other->itembonuses->DEX + other->spellbonuses->DEX)/5;
		chancetohit = (chancetohit > 0) ? chancetohit+30:30;
		float tohit_roll = ((float)rand()/RAND_MAX)*100;
		if (tohit_roll > chancetohit){
		  if(DEBUG>=5) LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) NPC missed", GetName(), other->GetName());
		  damage = 0;
		  return false;
  		}
	}
	else if (other->IsNPC()){
		// FIXME this should always base hit chance on level difference of target and self
		float chancetohit = 0;
		int8 otherlevel = this->GetLevel();
		int8 mylevel = other->GetLevel();
		otherlevel = otherlevel ? otherlevel : 1;
		mylevel = mylevel ? mylevel : 1;
		if (mylevel-otherlevel >= 0)
		{
			// 83% (+30) basechance, if target if lower then me
			chancetohit = 68;
		}
		else
		{
			// based on leveldiff
			chancetohit = 68-(float)(( otherlevel - mylevel )*( otherlevel - mylevel ))/4;
		}
		int16 targetagi = this->GetAGI();
		targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);
		chancetohit -= (float)targetagi*0.05;
		chancetohit += (float)(other->itembonuses->DEX + other->spellbonuses->DEX)/5;
		chancetohit = (chancetohit > 0) ? chancetohit+30:30;
		float tohit_roll = ((float)rand()/RAND_MAX)*100;
		if (tohit_roll > chancetohit){
		  if(DEBUG>=5) LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) IPC missed", GetName(), other->GetName());
		  damage = 0;
		  return false;
  		}
	}
	else if (other->IsClient()){
		int8 otherlevel = this->GetLevel();
		int8 mylevel = other->GetLevel();
		const Item_Struct* weapon = 0;
		int16 skillinuse = HIGHEST_SKILL+1;
		int attack_skill=4;
		otherlevel = otherlevel ? otherlevel : 1;
		mylevel = mylevel ? mylevel : 1;
		if (!Hand)
			weapon = other->CastToClient()->weapon1;
		else
			weapon = other->CastToClient()->weapon2;
		// calculate attack_skill and skillinuse depending on hand and weapon
		// also send Packet to near clients
		other->AttackAnimation(attack_skill, skillinuse, Hand, weapon);
		float chancetohit = other->GetSkill(skillinuse) / 3.75;
		if ( mylevel - otherlevel < 0)
		{
		// based on leveldiff
			chancetohit -= (float)(( otherlevel - mylevel ) * ( otherlevel - mylevel ))/4;
		}
	
		int16 targetagi = this->GetAGI();
		targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);

		//Quagmire: Take into account offense/defense skill
		chancetohit += (float)other->GetSkill(OFFENSE) * 0.10;
		chancetohit -= (float)this->GetSkill(DEFENSE) * 0.09;
	
		chancetohit -= (float)targetagi*0.05;
		chancetohit += (float)(other->itembonuses->DEX + other->spellbonuses->DEX)/5;

		//Trumpcard:  Give a mild skew for characters below 20th level.
		if (mylevel <=10)
			chancetohit = (chancetohit > 0) ? chancetohit+50:50;
		else if (mylevel <=20)
			chancetohit = (chancetohit > 0) ? chancetohit+40:40;
		else
			chancetohit = (chancetohit > 0) ? chancetohit+30:30;

		chancetohit = chancetohit > 95 ? 95 : chancetohit; /* cap to 95% */
	
		//  char temp[100];
		//  snprintf(temp, 100, "Chance to hit %s: %f", other->GetName(), chancetohit);
		//  ChannelMessageSend(0,0,7,0,temp);
	
    		///Check if it's a hit or miss
		if (((float)rand()/RAND_MAX)*100 > chancetohit){
		  if(DEBUG>=5) LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) PC missed", GetName(), other->GetName());
		  damage = 0;
		  return false;
  		}
	}
	else {
	  LogFile->write(EQEMuLog::Error, "Unknown entity type: %s", this->GetName());
	}
	////////////////////////////////////////////////////////
	// Mitigation goes here
	////////////////////////////////////////////////////////
	if (damage > 1) {
	    if(this->IsClient()){
	    	if (damage > 1 && spell_id == 0xFFFF){
                	double acMod = GetAC()/100;
                	double acDam = (damage/100)*acMod;
                	damage = (sint32)damage-acDam;
                	if(damage <= 0)
				damage = 1;
		}
	    }
	    else {
	    	if (damage > 1 && spell_id == 0xFFFF){
			double acMod = GetAC()/100;
                	double acDam = (damage/100)*acMod;
                	damage = (sint32)damage-acDam;
                	if(damage <= 0)
				damage = 1;
		}
	    }
	}
	//////////////////////////////////////////////////////////
	// make enrage same as riposte
	/////////////////////////////////////////////////////////
	if (IsEnraged() && !other->BehindMob(this, other->GetX(), other->GetY()) )
		damage = -3;	

	/////////////////////////////////////////////////////////
	// riposte
	/////////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassRiposte() && !other->BehindMob(this, other->GetX(), other->GetY()) )
	{
		if (IsClient()) {
        		skill = CastToClient()->GetSkill(RIPOSTE);
        		this->CastToClient()->CheckIncreaseSkill(RIPOSTE);
		} else {
        		skill = this->MaxSkill(RIPOSTE);
		}
#if DEBUG >= 5
    LogFile->write(EQEMuLog::Debug,"%s::AvoidDamage(%s) Riposte skill is : %i", GetName(), other->GetName(),(int)skill);
#endif
		if(((float)rand()/RAND_MAX) < skill/2500.0)
			damage = -3;
	}
	
	///////////////////////////////////////////////////////	
	// block
	///////////////////////////////////////////////////////
	if (damage > 0 && (
			class_==MONK ||
			class_==BEASTLORD ||
			class_==MONKGM ||
			class_==BEASTLORDGM )
            && !other->BehindMob(this, other->GetX(), other->GetY()))
	{
		if (IsClient()) {
			skill = CastToClient()->GetSkill(BLOCK);
			this->CastToClient()->CheckIncreaseSkill(BLOCK);
		} else {
        		skill = this->MaxSkill(BLOCK);
		}
#if DEBUG >= 5
    LogFile->write(EQEMuLog::Debug,"%s::AvoidDamage(%s) Block skill is : %i", GetName(), other->GetName(), (int)skill);
#endif

		if (( (float)rand()/RAND_MAX ) < skill/2500.0)
			damage = -1;
	}
	
	//////////////////////////////////////////////////////		
	// parry
	//////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassParry() && !other->BehindMob(this, other->GetX(), other->GetY()) )
	{
        
		if (IsClient()) {
        		skill = CastToClient()->GetSkill(PARRY);
        		this->CastToClient()->CheckIncreaseSkill(PARRY); 
		} else {
        		skill = this->MaxSkill(PARRY);
		}
#if DEBUG >= 5
    LogFile->write(EQEMuLog::Debug,"%s::AvoidDamage(%s) Parry skill is : %i", GetName(), other->GetName(), (int)skill);
#endif
		if(( (float) rand()/RAND_MAX ) < skill/3500.0)
			damage = -2;
	}
	
	////////////////////////////////////////////////////////
	// dodge
	////////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassDodge() && !other->BehindMob(this, other->GetX(), other->GetY()) )
	{
	
		if (IsClient()) {
        		skill = CastToClient()->GetSkill(DODGE);
        		this->CastToClient()->CheckIncreaseSkill(DODGE);
		} else {
        		skill = this->MaxSkill(DODGE);
		}
#if DEBUG >= 5
    LogFile->write(EQEMuLog::Debug,"%s::AvoidDamage(%s) Dodge skill is : %i", GetName(), other->GetName(), (int)skill);
#endif
		if(((float)rand()/RAND_MAX) < skill/3500.0)
			damage = -4;
	}
	
	////////////////////////////////////////////////////////
    if(DEBUG>=5) LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) out damage=%i", GetName(), other->GetName(), damage);
	if (damage < 0){
	    return true;
	}
	else {
	    return false;
	}
}

bool Client::Attack(Mob* other, int Hand, bool bRiposte)
{
	SetAttackTimer();
	if (IsCasting() && class_ != 8)
		return false; // Only bards can attack while casting
	if(!other)
		return false;
	
	if((IsClient() && CastToClient()->dead) || (other->IsClient() && other->CastToClient()->dead))
		return false;
	
	if(GetHP() < 0)
		return false;
#ifdef GUILDWARS
	if(target->IsNPC() && target->CastToNPC()->IsCityController())
		return false;
#endif

	if(!IsAttackAllowed(other))
        return false;
	
	const Item_Struct* weapon = 0;
	int16 skillinuse = HIGHEST_SKILL+1;
	int min_hit = 0;
	int max_hit = 0;
	int weapon_damage = 0;

	int attack_skill=4;
	if (Hand==13)	// Kaiyodo - Pick weapon from the attacking hand
		weapon = weapon1;
	else
		weapon = weapon2;
    
	// calculate attack_skill and skillinuse depending on hand and weapon
	// also send Packet to near clients
	AttackAnimation(attack_skill, skillinuse, Hand, weapon);
	
	/// Now figure out damage
	int damage = 0;
	
	int8 otherlevel = other->GetLevel();
	int8 mylevel = this->GetLevel();
	int8 mydex = this->GetDEX();
	
	otherlevel = otherlevel ? otherlevel : 1;
	mylevel = mylevel ? mylevel : 1;
	
	// Determine players ability to hit based on:
	// mob level difference, skillinuse, randomness
	if (skillinuse == HIGHEST_SKILL+1) { // the fallthru, only do 1 damage
		damage = 1;
	}
	else
	{
		if ( damage >= 0 ) {
			CheckIncreaseSkill(skillinuse, -10);
			CheckIncreaseSkill(OFFENSE, -10);

			if(skillinuse == 28) // weapon is hand-to-hand
			{
				if(GetClass() == MONK || GetClass() == BEASTLORD)
					weapon_damage = GetMonkHandToHandDamage();	// Damage changes based on level
				else
					weapon_damage = 2; // This isn't quite right, something more like level/10 is more appropriate
			}
			else
			{
				weapon_damage = (int)weapon->common.damage;
				if(weapon_damage < 1)
					weapon_damage = 1;
			}
#if 1 // Elemental damage
			int elemental_damage = 0;
			if (weapon && weapon->common.ElemDmgType && weapon->common.ElemDmg) {
                		if(DEBUG>=11)
					LogFile->write(EQEMuLog::Debug, "%s::Attack(%s) Elemental damage in, type:%i damage:%i weapon_damage:%i", GetName(), other->GetName(), weapon->common.ElemDmgType, weapon->common.ElemDmg, weapon_damage);
                		int resist = 0;
                		// 1 Magic, 2 Fire, 3 Cold, 4 Poison, 5 Disease
                		switch((int)weapon->common.ElemDmgType) {
					case 1: resist = GetMR(); break;
					case 2: resist = GetFR(); break;
					case 3:	resist = GetCR(); break;
					case 4:	resist = GetPR(); break;
					case 5:	resist = GetDR(); break;
					default:
                                		LogFile->write(EQEMuLog::Normal, "Unknown Resist type: %i", (int)weapon->common.ElemDmgType);
					break;
				}
                        	bool partial = true;
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
                        	float chance = (resist / maxchance);
				// cap resists so it CAN be possible to land spells even if big difference
				// in levels resist
                        	if (chance < 0.01)
                                	chance = 0.01;
				if (chance > 0.99)
					chance = 0.99;

				float random = float(rand())/float(RAND_MAX);
        
				if (partial && (random > chance - 0.50)) {
					weapon_damage += (int) weapon->common.ElemDmg*0.5;
				}
				else if (partial && random > chance - 0.40) {
					weapon_damage += (int) weapon->common.ElemDmg*0.6;
				}
				else if (partial && random > chance - 0.30) {
					weapon_damage += (int) weapon->common.ElemDmg*0.7;
				}
				else if (partial && random > chance - 0.20) {
					weapon_damage += (int) weapon->common.ElemDmg*0.8;
				}
				else if (partial && random > chance - 0.10)
				{
					weapon_damage += (int) weapon->common.ElemDmg*0.9;
				}
				else if (random >= chance) { // Full
					weapon_damage += (int) weapon->common.ElemDmg;
				}
                        	if(DEBUG>=11) 
					LogFile->write(EQEMuLog::Debug, "%s::Attack(%s) Elemental damage out, type:%i damage:%i weapon_damage:%i",
						GetName(), other->GetName(), weapon->common.ElemDmgType, weapon->common.ElemDmg, weapon_damage);
			}
#endif // Elemental damage

#if 1 // Racial bane damage
			if (weapon && weapon->common.BaneDMG && weapon->common.BaneDMGRace && other && other->GetRace() == weapon->common.BaneDMGRace) {
				weapon_damage += weapon->common.BaneDMG;
			}
#endif // Racial bane damage


#if 1 // Body bane damage
			if (weapon && weapon->common.BaneDMG && weapon->common.BaneDMGBody && other && other->GetBodyType() == weapon->common.BaneDMGBody) {
				weapon_damage += weapon->common.BaneDMG;
			}
#endif // Body bane damage

			min_hit = 1;
			max_hit = weapon_damage * (( ((float)GetSTR()*2) + (float)GetSkill(skillinuse)+ (float)mylevel) / 100);	// Apply damage formula
#if 1 // Weighted MDF type damage
			int magic_number = 0;
			int weighted = 0;
			if (GetLevel() >= 25) {
				max_hit =  weapon_damage * (( ((float)GetSTR()) + (float)GetSkill(skillinuse)+ (float)mylevel) / 100);	// Apply damage formula
				min_hit = (GetLevel()-25)/3; // FIXME Brutal hack for Damage bonus this is here somewhere but
				if (Hand != 13)
					min_hit = 1;
				magic_number = 2* weapon_damage + (level-25)/3;
				weighted = (0.9 * (weapon_damage+min_hit) + 0.1 * max_hit);
			}
#endif // Weighted MDF type damage
			// Only apply the damage bonus to the main hand
			if(Hand == 13)		// Kaiyodo - If we're not using the DWDA stuff, will always be the primary hand
			{
				int damage_bonus = GetWeaponDamageBonus(weapon);	// Can be NULL, will then assume fists
				min_hit += damage_bonus;
				max_hit += damage_bonus;
			}
		
			if(max_hit <= min_hit)
				damage = min_hit;
			else
				damage = (int32)min_hit + (rand()%(max_hit-min_hit)+1);
#if 1 // Weighted MDF type damage
			float hml = (float) ((float)rand()/(float)RAND_MAX);
			if(GetLevel()>=25){
				if (hml <= 0.10f){ // Low
					damage = (int32) (min_hit + (rand()%(weighted-min_hit)));
					if(damage > min_hit || damage > weighted || damage < min_hit) {
						damage = min_hit;
					}
				}
				else if (hml >= 0.11f && hml <= 0.89f){ // Middle
					damage = (int32) (weighted + (rand()%(magic_number-weighted)+1));
					if(damage > magic_number || damage < weighted) {
						damage = magic_number;
					}
				}
				else { // High
					damage = (int32) (magic_number + (rand()%(max_hit-magic_number)+1));
					if(damage < magic_number || damage >max_hit) {
						damage = magic_number;
					}
				}
				if (DEBUG>=11) {
					LogFile->write(EQEMuLog::Debug,"%s::Attack(): min_hit:%i max_hit:%i weapon_damage:%i damage:%i mod:%f MN:%i WN:%i HML:%f",
						GetName(), min_hit, max_hit, weapon_damage, damage, (( ((float)GetSTR()) + (float)GetSkill(skillinuse)+ (float)mylevel) / 100), magic_number, weighted , hml);
				}
			}
#endif // Weighted MDF type damage
		} // End (damage >= 0)
		if (DEBUG>=11) {
			LogFile->write(EQEMuLog::Debug,"Client::Attack(): min_hit:%i max_hit:%i weapon_damage:%i damage:%i mod:%f",
				min_hit, max_hit, weapon_damage, damage, (( ((float)GetSTR()*2) + (float)GetSkill(skillinuse)+ (float)mylevel) / 100) );
		}
	} // End skill set?

	// } // <-- wtf

	if (damage > 0) {
        	if(other)
                	other->AvoidDamage(this, damage, 0xffff, attack_skill, Hand);
        	if (bRiposte && damage == -3)
                	return false;
        	if (damage <= 0) {
    			other->Damage(this, damage, 0xffff, attack_skill);
				return false;
		}

		//////////////////////////////////////////////////////////
		/////////	Finishing Blow
		/////////////////////////////////////////////////////////
		//kathgar: Made it so players cannot be finishing blowed.. something wanky was going on
		//			Added level limits and fixed the chances to the correct values?
		uint8 *aa_item = &(((uint8 *)&aa)[32]);
		if(damage > 0 && *aa_item>0 && other->GetHPRatio() < 10 && !other->IsClient()/*Don't finishing blow players.. at least for now*/) {
			int tempchancerand =rand()%100;
			if(*aa_item==1 && (tempchancerand<=2)&& other->GetLevel() <=50) {	  
				other->Damage(this,32000,0xffff,attack_skill); Message(0,"You inflict a finishing blow!");
				return true;
			}
			else if(*aa_item==2 && (tempchancerand<=5)&& other->GetLevel() <=52) {	  
				other->Damage(this,32000,0xffff,attack_skill); Message(0,"You inflict a finishing blow!");
				return true;
			}
			else if(*aa_item==3 && (tempchancerand<=7)&& other->GetLevel() <=54) {	  
				other->Damage(this,32000,0xffff,attack_skill); Message(0,"You inflict a finishing blow!");
				return true;
			}
			else {
				//Message(0,"Wtf? You can\'t have more than 3 points in finishing blow");
			}
		}

		///////////////////////////////////////////////////
		/////   Critical Hits
		//////////////////////////////////////////////////
		uint8 *aa_item2 = &(((uint8 *)&aa)[30]);
       		int amount1 = *aa_item2;
		if (damage > 0 && GetClass() == WARRIOR && mylevel >= 11) {
   			int amount = *aa_item2;
			amount = (amount*5)+5;
			int Critchance = mydex - amount;
			int Critical = 0;
			if (Critchance != 0) // Quag: Divide by zero protection
				Critical = rand()%Critchance;
			int FinalChance = Critchance-amount;
			if (Critical > FinalChance) {
				int RAND_CRIT = rand()%5;
				if (this->berserk)
					RAND_CRIT = rand()%10;
				damage += ((( mylevel / 4) + (weapon ? weapon->common.damage : 0)) * RAND_CRIT);
				if (this->berserk)
					entity_list.MessageClose(this, false, 200, 10, "%s lands a crippling blow!(%d)", name,damage);
				else
					entity_list.MessageClose(this, false, 200, 10, "%s scores a critical hit!(%d)", name,damage);
			}
		}
		else {
			if (amount1 != 0){
				int amount = amount1*5;
				int Critchance = mydex - amount;
				int Critical = 0;
				if (Critchance != 0) // Quag: Divide by zero protection
					Critical = rand()%Critchance;
				int FinalChance = Critchance-amount;
				if (Critical > FinalChance){
					int RAND_CRIT = rand()%5;
					damage += ((( mylevel / 4) + (weapon ? weapon->common.damage : 0)) * RAND_CRIT);
					entity_list.MessageClose(this, false, 200, 10, "%s scores a critical hit!(%d)", name,damage);
				}
			}
		}
	}
	else { // Make sure damage math doesn't give us a negative
		damage = 0;
	}
	///////////////////////////////////////////////////////////
	//////    Send Attack Damage
	///////////////////////////////////////////////////////////
	other->Damage(this, damage, 0xffff, attack_skill);

	////////////////////////////////////////////////////////////
	////////  PROC CODE
	////////  Kaiyodo - Check for proc on weapon based on DEX
	///////////////////////////////////////////////////////////
	if(HasProcs() || ( weapon && (weapon->common.spellId < 65535ul) && (weapon->common.effecttype == 0)) && other && other->GetHP() > 0)
	{
        	int16 usedspellID = 0xFFFF;
		float dexmod = (float) GetDEX() / 100;
        	for (int i = 0; i < MAX_PROCS; i++) {
			if (PermaProcs[i].spellID != 0xFFFF) {
				if (rand()%100 < (PermaProcs[i].chance * dexmod)) {
					usedspellID = PermaProcs[i].spellID;
					break;
				}
			}
			if (SpellProcs[i].spellID != 0xFFFF) {
				if (rand()%100 < (SpellProcs[i].chance * dexmod)) {
					usedspellID = SpellProcs[i].spellID;
					break;
				}
			}
		}

		if (usedspellID == 0xFFFF && weapon && (weapon->common.spellId < 65535ul) && (weapon->common.effecttype == 0)) {
			float ProcChance = (float)rand()/RAND_MAX;
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"ProcChance is : %f", ProcChance);
#endif
			if (ProcChance < ((float) mydex / 3020.0f)) {	// 255 dex = 0.084 chance of proc. No idea what this number should be really.
				usedspellID = weapon->common.spellId;
				if(weapon->common.level >  mylevel ) {
					Message(13, "Your will is not sufficient to command this weapon.");
					usedspellID = 0xFFFF;
				}
			}
		}


		// Trumpcard: Changed proc targets to look up based on the spells goodEffect flag.
		// This should work for the majority of weapons.
		if ( usedspellID != 0xFFFF && !(other->IsClient() && other->CastToClient()->dead) )
		{
			if ( IsBeneficial(usedspellID) )
				SpellFinished(usedspellID, GetID(), 10, 0);
			else
				SpellFinished(usedspellID, other->GetID(), 10, 0);
		}

	}
	if( (other->IsEnraged() || damage == -3) && !BehindMob(other, GetX(), GetY()) )
	{
		//other->CastToNPC()->FaceTarget(); //Causes too much lag?? Disabled. -image
		other->Attack(this, 13, true);
        	return false;
	}


	if (damage > 0)
        	return true;
	else
        	return false;
}

void Mob::Heal() {
	SetMaxHP();
/*	APPLAYER app;
	app.opcode = OP_Action;
	app.size = sizeof(Action_Struct);
	app.pBuffer = new uchar[app.size];
	memset(app.pBuffer, 0, app.size);
	Action_Struct* a = (Action_Struct*)app.pBuffer;
	
	a->target = GetID();
	a->source = GetID();
	a->type = 231; // 1
	a->spell = 0x000d; //spell_id
	a->damage = -10000;
	
	entity_list.QueueCloseClients(this, &app);*/
	APPLAYER hp_app;
	CreateHPPacket(&hp_app);
	hp_app.priority = 1;
	entity_list.QueueCloseClients(this, &hp_app, true);
	
    LogFile->write(EQEMuLog::Normal,"%s healed via #heal", name);

}

void Client::Damage(Mob* other, sint32 damage, int16 spell_id, int8 attack_skill, bool avoidable, sint8 buffslot, bool iBuffTic) {
	adverrorinfo = 411;
	
    if(other && this->IsAIControlled()) {
        if (attack_skill == 0x07)
	        AddToHateList(other, 1, damage, iBuffTic); // almost no aggro for archery
        else if (spell_id != 0xFFFF)
	        AddToHateList(other, damage / 2 , damage, iBuffTic); // half aggro for spells
        else
	        AddToHateList(other, damage, damage, iBuffTic);   // normal aggro for everything else
    }
    // if we got a pet, thats not already fighting something send it into battle
    Mob *pet = GetPet();
    if (other && pet && !pet->IsEngaged() && other->GetID() != GetID())
    {
        if (pet)
            pet->AddToHateList(other, 1, 0, iBuffTic);
        else
        {
            // todo: do whatever is necessary for clients as pets
        }
        pet->SetTarget(other);
        Message(10,"%s tells you, 'Attacking %s, Master.'", pet->GetName(), other->GetName());
    }
	
	if( spell_id != 0xFFFF || other == 0 )
        avoidable = false;
	
    if (invulnerable)
        damage=-5;
	
    // damage shield calls this function with spell_id set, so its unavoidable
	if (other && damage > 0 && spell_id == 0xFFFF) {
		this->DamageShield(other);
	}

#ifdef GUILDWARS
	if (spell_id != 0xFFFF && other && other->IsClient())
	{
		if(damage > 100)
		damage = damage*.6;
	}
	else if(spell_id == 0xFFFF && other && other->IsClient())
	{
		damage = damage*1.33;
	}

	if(other && other->IsClient() && other->CastToClient()->GuildDBID() != 0)
	{
	int8 total = database.TotalCitiesOwned(other->CastToClient()->GuildDBID());
	if(spell_id != 0xFFFF)
		damage += (damage*(total/80));
	else
		damage += (damage*(total/55));
	}
#endif

    if (spell_id != 0xFFFF || (attack_skill>200 && attack_skill<250)) {
        // todo: exchange that for EnvDamage-Packets when we know how to do it
        Message(4,"%s was hit by non-melee for %d points of damage.", GetName(), damage);
    }

	
	if (damage > 0 && GetRune() > 0) {
		damage = ReduceDamage(damage, GetRune());
	}
	if (damage > 0 && (GetHP() - damage) <= -10) {
		Death(other, damage, spell_id, attack_skill);
		return;
	}
	if (damage > 0)
		SetHP(GetHP()-damage);
	if (other && other->IsNPC())
		CheckIncreaseSkill(DEFENSE, -10);
	APPLAYER* outapp = new APPLAYER(OP_Action, sizeof(Action_Struct));
	Action_Struct* a = (Action_Struct*)outapp->pBuffer;
	adverrorinfo = 412;
	a->target = GetID();
	
	if (other == 0)
		a->source = 0;
	else if (other->IsClient() && other->CastToClient()->GMHideMe())
		a->source = 0;
	else
		a->source = other->GetID();
		
	a->type = attack_skill;
	a->spell = spell_id;
	a->damage = damage;
	outapp->priority = 4;
	entity_list.QueueCloseClients(this, outapp, false, 200, other);
	if (other && other->IsClient())
		other->CastToClient()->QueuePacket(outapp);
	delete outapp;
	adverrorinfo = 413;
	APPLAYER hp_app;
	CreateHPPacket(&hp_app);
	hp_app.priority = 1;
	entity_list.QueueCloseClients(this, &hp_app);
	SendHPUpdate();
	
	if (damage != 0)
	{ // Pyro: Why log this message if no damage is inflicted
		if (IsMezzed())
			this->BuffFadeByEffect(SE_Mez);
		if (attack_skill == BASH && GetLevel() < 56) {
			Stun(0);
		}
		if (IsRooted() && spell_id != 0xFFFF) { // neotoyko: only spells cancel root
			if ((float)rand()/RAND_MAX > 0.8f)
				this->BuffFadeByEffect(SE_Root, buffslot); // buff slot is passed through so a root w/ dam doesnt cancel itself
		}
		if(this->casting_spell_id != 0) {
			// client: 30 = basechance
			int16 channelchance = (int16)(30+((float)this->GetSkill(CHANNELING)/400)*100);
			if (((float)rand()/RAND_MAX)*100 > channelchance) {
				this->isattacked = true;
				this->isinterrupted = true;
			}
			else {
				this->isattacked = true;
			}
		}
		
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug," %s hit for %i, %i left", name, damage, GetHP());
#endif
		adverrorinfo = 41;
	}
}

void Client::Death(Mob* other, sint32 damage, int16 spell, int8 attack_skill)
{
	SetHP(-100);
	entity_list.RemoveFromTargets(this);
	zonesummon_x = -3;
	zonesummon_y = -3;
	zonesummon_z = -3;
	
	SetPet(0);
	
	pp.x = x_pos;
	pp.y = y_pos;
	pp.z = z_pos;
	pp.heading = heading;
	int i;

	BuffFade(0xFFFe);

    //
    // Xorlac: A bad assumption here is that other will be set.
    //         For instance, spontaneous death due to lore items will not have set other
    //

    if (other != NULL)
    {
        if(other->IsClient() && other->CastToClient()->IsDueling() && other->CastToClient()->GetDuelTarget() == GetID())
        {
	        other->CastToClient()->SetDueling(false);
	        other->CastToClient()->SetDuelTarget(0);
        }
    }
	
    LogFile->write(EQEMuLog::Normal,"Player %s has died", name);
	//We are going to strip the spells off the player now....
	for(i=0; i<BUFF_COUNT; i++)
        buffs[i].spellid = 0xFFFF;
	APPLAYER app(OP_Death, sizeof(Death_Struct));
	Death_Struct* d = (Death_Struct*)app.pBuffer;
	d->corpseid = GetID();
	//  d->unknown011 = 0x05;
	d->spawn_id = GetID();
	if (other == 0)
		d->killer_id = 0;
	else
		d->killer_id = other->GetID();
	d->damage = damage;
	d->spell_id = spell;

	d->type = attack_skill;
	d->unknownds016 = pp.bind_point_zone;
	int32 exploss = (int32)(GetLevel()*((float)GetLevel()/18)*8000);
	if (GetLevel() > 9 && !IsBecomeNPC() && !GetGM() && GetEXP() >= 0 && (other == 0 || !other->IsClient()))
		SetEXP((int32)(GetEXP() - GetLevel()*((float)GetLevel()/18)*8000 > 0)? (int32)(GetEXP() - GetLevel()*((float)GetLevel()/18)*8000) : 1,GetAAXP());
	entity_list.QueueClients(this, &app);
#ifdef GUILDWARS
	if(other != 0 && other->IsClient() && IsAttackAllowed(other) && other != this)
	{
	if (GetLevel() > 9 && GetLevelCon(other->GetLevel()) == CON_GREEN)
		SetEXP((int32)(GetEXP() - GetLevel()*((float)GetLevel()/18)*12000 > 0)? (int32)(GetEXP() - GetLevel()*((float)GetLevel()/18)*8000) : 1,GetAAXP());

		char msg[400];
		sprintf(msg,"%s:%i:%i:%i",zone->GetShortName(),GuildDBID(),other->CastToClient()->GuildDBID(),other->GetHP());
		database.InsertNewsPost(1,name,other->GetName(),2,msg);
		if (other->CastToClient()->isgrouped && entity_list.GetGroupByClient(other->CastToClient()) != 0)
			entity_list.GetGroupByClient(other->CastToClient())->SplitExp((uint32)(level*level*325*3.5f), GetLevel());
		else if(other->GetLevelCon(GetLevel()) != CON_GREEN)
			other->CastToClient()->AddEXP((uint32)(level*level*350*3.5f));
	}
#endif
	if(IsBecomeNPC() == true)
	{
		if (other != NULL && other->IsClient())

		{
			if (other->CastToClient()->isgrouped && entity_list.GetGroupByMob(other) != 0)
				entity_list.GetGroupByMob(other->CastToClient())->SplitExp((uint32)(level*level*75*3.5f), GetLevel());
			else
				other->CastToClient()->AddEXP((uint32)(level*level*75*3.5f)); // Pyro: Comment this if NPC death crashes zone
			//hate_list.DoFactionHits(GetNPCFactionID());
		}
	}
	if (other && other->IsNPC())
		parse->Event(6, other->GetNPCTypeID(), 0, other, this->CastToMob());
	pp.cur_hp = GetMaxHP()*.2;
	if(GetMaxMana() > 0)
	pp.mana = GetMaxMana()*.2;
	if(IsLD())
	{
	pp.current_zone = pp.bind_point_zone;
	pp.x = pp.bind_location[0][0];
	pp.y = pp.bind_location[1][0];
	pp.z = pp.bind_location[2][0];
	}

	Save();
	MakeCorpse(exploss);
}

void Client::MakeCorpse(int32 exploss) {
	if (this->GetID() == 0)
		return;
	
	if ( (!GetGM() && pp.level > 9) || IsBecomeNPC())
	{
		// Check to see if we are suppose to make a corpse
		// Via the database setting (anything in leavecorpses)
		char tmp[20];
		memset(tmp,0,sizeof(tmp));
		database.GetVariable("leavecorpses",tmp, 20);
		int8 tmp2 = atoi(tmp);
		if (tmp2 >= 1) {
#if DEBUG >= 5
    LogFile->write(EQEMuLog::Debug,"Creating corpse for %s at x=%d y=%d z=%d h=%d", GetName(), GetX(), GetY(), GetZ(), GetHeading());
#endif
			entity_list.AddCorpse(new Corpse(this, &pp, exploss, tmp2), this->GetID());
			this->SetID(0);
		}
	}
}

bool NPC::Attack(Mob* other, int Hand, bool bRiposte)	 // Kaiyodo - base function has changed prototype, need to update overloaded version
{
	int damage = 0;

	if (!other)
	{
    	    SetTarget(NULL);
    	    return false;
	}

	if (!target)
    	    SetTarget(other);

	SetAttackTimer();
#ifdef GUILDWARS
	if((other->GetHP() <= -11) || (other->IsClient() && other->CastToClient()->dead) || ((CastToNPC()->IsCityController()) || (target->IsNPC() && target->CastToNPC()->IsCityController())))
#else
	if((other->GetHP() <= -11) || (other->IsClient() && other->CastToClient()->dead))
#endif
	{
		RemoveFromHateList(other);
		return false;
	}

	if (!IsAttackAllowed(other)) {
		if (this->GetOwnerID())
			entity_list.MessageClose(this, 1, 200, 10, "%s says, 'That is not a legal target master.'", this->GetName());
		this->WhipeHateList();
		return false;
	}
	else
	{
		int16 skillinuse;
		int attack_skill;
		
		const Item_Struct *weapon = NULL;
		if (Hand == 13 && equipment[7] > 0)
		    weapon = database.GetItem(equipment[7]);
		else if (equipment[8])
		    weapon = database.GetItem(equipment[8]);
		AttackAnimation(attack_skill, skillinuse, Hand, weapon);

		int8 otherlevel = other->GetLevel();
		int8 mylevel = this->GetLevel();
		
		otherlevel = otherlevel ? otherlevel : 1;
		mylevel = mylevel ? mylevel : 1;

		float dmgbonusmod = 0;
		float clmod = (float)GetClassLevelFactor()/22;
		float basedamage;
		float basedefend = 0;
		float currenthit = 0;


		// set min_dmg max_dmg here based on level if they are not set already
		// FIXME database cache lookup fancy like stuff needed here
		float level_mod = 1.5f;
		if (mylevel >= 66) {
		    level_mod = 4.5f; // mod4
		    if (min_dmg==0)
			min_dmg = 220;
		    if (max_dmg==0)
			max_dmg = (((220*level_mod)*(mylevel-64))/4.0f);
					 // 66 = 495, 67 = 742, 68 = 990, 69 = 1237, 70 = 1485
		}
		if (mylevel >= 60 && mylevel <= 65){
		    level_mod = 4.25f;
		    if(min_dmg==0);
		    min_dmg = (mylevel+(mylevel/3));
		    if(max_dmg==0);
		    max_dmg = (mylevel*3);
		    // 60 = 180, 65 = 195
		}
		if (mylevel >= 51 && mylevel <= 59){
		    level_mod = 3.75f;
		    if(min_dmg==0)
		    min_dmg = (mylevel+(mylevel/3));
		    // 51 = 68, 59 = 78
		    if(max_dmg==0)
		    max_dmg = (mylevel*3);
		    // 51 = 153, 59 = 177
		}
		if (mylevel >= 40 && mylevel <= 50) {
			if (min_dmg==0)
				min_dmg = mylevel;
			if(max_dmg==0)
				max_dmg = (mylevel*3);
		    // 40 = 120 , 50 = 150
		}
		if (mylevel >= 28 && mylevel <= 39) {
		    if (min_dmg==0)
			min_dmg = mylevel / 2; // 14-17
		    if (max_dmg==0)
			max_dmg = (mylevel*2)+2;
		    // 28 = 58, 39 = 80
		}
		if (mylevel <= 27) {
		    if (min_dmg==0)
			min_dmg=1;
		    if (max_dmg==0)
			max_dmg = mylevel*2;
		    // 1 = 2, 27 = 54
		}
		
		if(max_dmg != 0 && min_dmg <= max_dmg) {
			basedamage = RandomTimer(min_dmg,max_dmg)*((clmod >= 1.0f) ? clmod:1.0f);  // npc only get bonus for class no negatives
		}
		else if (other->GetOwnerID()!=0) {
			// FIXME Shouldn't nerf the damage of charmed pets
			basedamage = mylevel*1.9f*clmod;
		}
		else { // Default calculation
			basedamage = mylevel*level_mod*clmod;
		}
		


		dmgbonusmod += (float)(this->itembonuses->STR + this->spellbonuses->STR)/3;
		dmgbonusmod += (float)(this->spellbonuses->ATK + this->itembonuses->ATK)/5;
		basedamage += (float)basedamage/100*dmgbonusmod;

		damage = (int)basedamage;

		if(other->IsClient() && min_dmg != 0 && damage == min_dmg && dmgbonusmod > 0)
		    damage += dmgbonusmod;
		if(min_dmg != 0 && damage < min_dmg)
		    damage = min_dmg;
		if(max_dmg != 0 && damage > max_dmg)
		    damage = max_dmg;
		if (other)
		    other->AvoidDamage(this, damage, 0xFFFF, attack_skill);
		if (bRiposte && damage == -3)
		    return false;
		adverrorinfo = 1292;
		if(other != 0 && this != 0 && GetHP() > 0 && other->GetHP() >= -11){
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"NPC::Attack() basedamage:%f basedefend:%f dmgbonusmod:%f clmod:%f currenthit:%f damage:%i", basedamage, basedefend, dmgbonusmod, clmod, currenthit, damage);
#endif
		    other->Damage(this, damage, 0xffff, attack_skill, false); // Not avoidable client already had thier chance to Avoid
	    }
		adverrorinfo = 1293;
	}
	
	// Kaiyodo - Check for proc on weapon based on DEX
	if(HasProcs() && other && other->GetHP() > 0)
	{
	    int16 usedspellID = 0xFFFF;
	    float dexmod = (float) GetDEX() / 100;
	    for (int i = 0; i < MAX_PROCS; i++)
    	    {
        	if (PermaProcs[i].spellID != 0xFFFF)
        	{
            	    if (rand()%100 < (PermaProcs[i].chance * dexmod))
            	    {

                	usedspellID = PermaProcs[i].spellID;
                	break;
            	    }
        	}
        	if (SpellProcs[i].spellID != 0xFFFF)
        	{
            	    if (rand()%100 < (SpellProcs[i].chance * dexmod))
            	    {
                	usedspellID = SpellProcs[i].spellID;
                	break;
            	    }
        	}
    	    }

	    // Trumpcard: Changed proc targets to look up based on the spells goodEffect flag.
	    // This should work for the majority of weapons.
	    if ( usedspellID != 0xFFFF )
	    {
		if ( IsBeneficial(usedspellID) )
		    SpellFinished(usedspellID, GetID(), 10, 0);				
		else
		    SpellFinished(usedspellID, other->GetID(), 10, 0);
	    }

	}
	// now check ripostes
	if (other && damage == -3) // riposting
	{
	    other->Attack(this, 13, true);
    	    // todo: double riposte

    	    return false;
	}
	if (damage > 0)
    	    return true;
	else
    	    return false;
}

void NPC::Damage(Mob* other, sint32 damage, int16 spell_id, int8 attack_skill, bool avoidable, sint8 buffslot, bool iBuffTic) {
    // now add done damage to the hate list
    if (!other)
      return;
	if (attack_event == 0)
	{
		parse->Event(EVENT_ATTACK, this->GetNPCTypeID(), 0, this, other->CastToMob());
	}
	attack_event = 1;
	attacked_timer = new Timer(12000);

	if(other) {
        if (attack_skill == 0x07)
	        AddToHateList(other, 1, damage, iBuffTic); // almost no aggro for archery
        else if (spell_id != 0xFFFF)
	        AddToHateList(other, damage / 2 , damage, iBuffTic); // half aggro for spells
        else
	        AddToHateList(other, damage, damage, iBuffTic);   // normal aggro for everything else
    }

    // only apply DS if physical damage (no spell damage)
    if (other && damage > 0 && spell_id == 0xFFFF) {
		this->DamageShield(other);
	}
	if (spell_id != 0xFFFF || (attack_skill>200 && attack_skill<250))
    {
        // todo: exchange that for EnvDamage-Packets when we know how to do it
		if (other && other->IsClient())
			other->CastToClient()->Message(4,"%s was hit by non-melee for %d points of damage.", this->GetName(), damage);
    }

#ifdef GUILDWARS
	if(other && other->IsClient() && other->CastToClient()->GuildDBID() != 0)
	{
	int8 total = database.TotalCitiesOwned(other->CastToClient()->GuildDBID());
	if(spell_id != 0xFFFF)
		damage += (damage*(total/75));
	else
		damage += (damage*(total/50));
	}
#endif		
	// if spell is lifetap add hp to the caster
	if (other && IsLifetapSpell( spell_id ))
	{
		int32 healedhp;

		// check if healing would be greater than max hp
		// use temp var to store actual healing value
		if ( other && other->GetHP() + damage > other->GetMaxHP())
		{
			healedhp = other->GetMaxHP() - other->GetHP();
			other->SetHP(other->GetMaxHP());
		}
		else
		{
			healedhp = damage;
			if(other != 0)
    			other->SetHP(other->GetHP() + damage);
		}

        // not sure if i need to send this or not. didnt hurt yet though ;-)
		APPLAYER hp_app;
		other->CreateHPPacket(&hp_app);

		// if client was casting the spell there need to be some messages
		if (other->IsClient())
		{
			other->CastToClient()->Message(4,"You have been healed for %d points of damage.", healedhp);
			other->CastToClient()->QueuePacket(&hp_app);
		}
			
		// emote goes with every one ... even npcs
		entity_list.MessageClose(this, true, 300, MT_Emote, "%s beams a smile at %s", other->GetName(), this->GetName() );
#ifndef REDUCE_BATTLE_SPAM
		entity_list.QueueCloseClients(this, &hp_app, false, 600, other);
#endif
	}

    if (damage > 0 && GetRune() > 0)
	{
		damage = ReduceDamage(damage, GetRune());
	}

	if (damage >= GetHP())
	{
		SetHP(-100);
		Death(other, damage, spell_id, attack_skill);
		return;
	}
		
	SetHP(GetHP() - damage);
	APPLAYER app;
	app.opcode = OP_Action;
	app.size = sizeof(Action_Struct);
	app.pBuffer = new uchar[app.size];
	memset(app.pBuffer, 0, app.size);
	Action_Struct* a = (Action_Struct*)app.pBuffer;
	a->target = GetID();
	if (other == 0)
		a->source = 0;
	else if (other->IsClient() && other->CastToClient()->GMHideMe())
		a->source = 0;
	else
		a->source = other->GetID();
		
    a->type = attack_skill; // was 0x1c
    if (attack_skill == 231)
   		a->spell = 0xFFFF;
    else
   		a->spell = spell_id;
	a->damage = damage;
			
	a->unknown4[0] = 0xcd;
	a->unknown4[1] = 0xcc;
	a->unknown4[2] = 0xcc;
	a->unknown4[3] = 0x3d;
	a->unknown4[4] = 0x71;
	a->unknown4[5] = 0x6b;
	a->unknown4[6] = 0x3d;
	a->unknown4[7] = 0x41;
			
	app.priority = 5;
#ifdef REDUCE_BATTLE_SPAM
	if (other && other->GetOwnerID()) { // let pet owners see their pet's damage
		Mob* owner = other->GetOwner();
		if (owner && owner->IsClient())
			owner->CastToClient()->QueuePacket(&app);
	}
#else
	entity_list.QueueCloseClients(this, &app, false, 200, other);
#endif
	if (other && other->IsClient())
		other->CastToClient()->QueuePacket(&app);
#ifndef REDUCE_BATTLE_SPAM
	APPLAYER hp_app;
	CreateHPPacket(&hp_app);
	hp_app.priority = 1;
	entity_list.QueueCloseClients(this, &hp_app, false, 200, 0, false);
#endif
	if (!IsEngaged())
        zone->AddAggroMob();
    if (other && damage > 0)
        AddRampage(other);
			
	if (damage > 0)
	{
		if (IsMezzed()) {

			this->BuffFadeByEffect(SE_Mez);

		}
		if (attack_skill == BASH && GetLevel() < 56) {
			Stun(0);
		}
		if (IsRooted() && spell_id != 0xFFFF) // neotoyko: only spells cancel root
		{
			if ((float)rand()/RAND_MAX > 0.8f)
				this->BuffFadeByEffect(SE_Root, buffslot);
		}
		if(this->casting_spell_id != 0)
		{
		    // neotokyo: made interrupting NPCs very hard (base chance > 80%)
            int16 channelling = GetSkill(CHANNELING);
            if (channelling < 225)
                channelling = 225;
			float channelchance = (float)channelling/255.0f;
			if (((float)rand()/RAND_MAX) > channelchance)
			{
				this->isattacked = true;
				this->isinterrupted = true;
			}
			else
				this->isattacked = true;
		}
	}
}

void NPC::Death(Mob* other, sint32 damage, int16 spell, int8 attack_skill)
{
	if (this->IsEngaged())
	{
		zone->DelAggroMob();
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"NPC::Death() Mobs currently Aggro %i", zone->MobsAggroCount());
#endif
	}
	SetHP(0);
	SetPet(0);
	Mob* killer = GetHateDamageTop(this);
	
	entity_list.RemoveFromTargets(this);
	
	APPLAYER app;
	app.opcode = OP_Death;
	
	BuffFade(0xFFFe);
	app.size = sizeof(Death_Struct);
	app.pBuffer = new uchar[app.size];
	memset(app.pBuffer, 0, app.size);
	Death_Struct* d = (Death_Struct*)app.pBuffer;
	d->corpseid = GetID();
	d->spawn_id = GetID();
	if (other == 0)
		d->killer_id = 0;
	else
		d->killer_id = other->GetID();
	d->spell_id = spell;
	d->type = attack_skill;
	d->damage = damage;
	entity_list.QueueCloseClients(this, &app, false, 600, other);
	if (other)
	{
		if (other->IsClient())
			other->CastToClient()->QueuePacket(&app);
		hate_list.Add(other, damage);
	}
	
	if (killer && (killer->IsClient() || killer->CastToNPC()->IsInteractive()) && !IsCorpse() )
	{
		if (killer->CastToClient()->isgrouped && entity_list.GetGroupByClient(killer->CastToClient()) != 0)
#ifdef GUILDWARS
#ifdef GUILDWARS2
			if(killer->CastToClient()->GetLevel() <= 49 || (killer->CastToClient()->GetLevel() >= 50 && GetLevel() >= 61))
#endif
			entity_list.GetGroupByClient(killer->CastToClient())->SplitExp((uint32)(level*level*175*3.5f), GetLevel());
#else
			entity_list.GetGroupByClient(killer->CastToClient())->SplitExp((uint32)(level*level*75*3.5f), GetLevel());
#endif
		else
        {
            if (killer->GetLevelCon(GetLevel()) != CON_GREEN && MerchantType == 0)
            {
#ifdef GUILDWARS
#ifdef GUILDWARS2
			if(killer->CastToClient()->GetLevel() <= 49 || (killer->CastToClient()->GetLevel() >= 50 && GetLevel() >= 61))
#endif
			    killer->CastToClient()->AddEXP((uint32)(level*level*200*3.5f));
#else
			    killer->CastToClient()->AddEXP((uint32)(level*level*75*3.5f)); // Pyro: Comment this if NPC death crashes zone
#endif
            }
		}
		hate_list.DoFactionHits(GetNPCFactionID());
	}
	
	if (respawn2 != 0)
	{
		respawn2->Reset();
	}
	
	if (class_ != 32 && this->ownerid == 0 && this->flag[3]!=3 && CastToNPC()->MerchantType == 0) {
		Corpse* corpse = new Corpse(this, &itemlist, GetNPCTypeID(), &NPCTypedata);
		entity_list.AddCorpse(corpse, this->GetID());
		this->SetID(0);
		if(killer != 0 && killer->IsClient()) {
			corpse->AllowMobLoot(killer->GetName(), 0);
			if(killer->CastToClient()->isgrouped) {
				Group* group = entity_list.GetGroupByClient(killer->CastToClient());
				if(group != 0) {
					for(int i=0;i<6;i++) { // Doesnt work right, needs work
						if(group->members[i] != NULL) {
							corpse->AllowMobLoot(group->members[i]->GetName(),i);
						}
					}
				}
			}
		}
	}
	// Parse quests even if we're killed by an NPC
	if (other)
		parse->Event(EVENT_DEATH, this->GetNPCTypeID(),0, this, other->CastToMob());
	this->WhipeHateList();
	p_depop = true;

}

bool Mob::ChangeHP(Mob* other, sint32 amount, int16 spell_id, sint8 buffslot, bool iBuffTic) {
	if (IsCorpse())
		return false;
	if (spell_id == 982)		/* Cazic Touch */
	{
		SetHP(-100);
		this->Death(other, abs(amount), spell_id, 231);
		return true;
	}
	else if (spell_id == 13) {	/* Complete Healing */
		SetHP(GetMaxHP());
	}
	else if (!invulnerable)
	{
		if (amount < 0)
		{
			this->Damage(other, abs(amount), spell_id, 231, false, buffslot, iBuffTic);
			return true;
		}
		else
		{
#ifdef GUILDWARS
	if(other && other->IsClient() && other->CastToClient()->GuildDBID() != 0)
	{
	int8 total = database.TotalCitiesOwned(other->CastToClient()->GuildDBID());
		amount += (amount*(total/40));
	}
#endif

			SetHP(GetHP() + amount);
		}
	}

	if(GetHP() > GetMaxHP())
		SetHP(GetMaxHP());
	
	APPLAYER hp_app;
	CreateHPPacket(&hp_app);
	hp_app.priority = 2;
	entity_list.QueueCloseClients(this, &hp_app, false, 600, other);
	if (other != 0 && other->IsClient())
		other->CastToClient()->QueuePacket(&hp_app);
	return false;
}

void Mob::MonkSpecialAttack(Mob* other, int8 type)
{
	sint32 ndamage = 0;
	//PlayerProfile_Struct pp;
	float hitsuccess = (float)other->GetLevel() - (float)level;
	float hitmodifier = 0.0;
	float skillmodifier = 0.0;
	if(level > other->GetLevel())
	{
		hitsuccess += 2;
		hitsuccess *= 14;
	}
	if ((int)hitsuccess >= 40)
	{
		hitsuccess *= 3.0;
		hitmodifier = 1.1;
	}
	if ((int)hitsuccess >= 10 && hitsuccess <= 39)
	{
		hitsuccess /= 4.0;
		hitmodifier = 0.25;
	}
	else if ((int)hitsuccess < 10 && (int)hitsuccess > -1)
	{
		hitsuccess = 0.5;
		hitmodifier = 1.5;
	}
	else if ((int)hitsuccess <= -1)
	{
		hitsuccess = 0.1;
		hitmodifier = 1.8;
	}
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"MonkSpecialAttack() 2 - %d", hitsuccess);
#endif
	if ((int)GetSkill(type) >= 100)
	{
		skillmodifier = 1;
	}
	else if ((int)GetSkill(type) >= 200)
	{
		skillmodifier = 2;
	}
	
	hitsuccess -= ((float)GetSkill(type)/10000) + skillmodifier;
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"MonkSpecialAttack() 3 - %d", hitsuccess);
#endif
	hitsuccess += (float)rand()/RAND_MAX;
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"MonkSpecialAttack() 4 - %d", hitsuccess);
#endif
	float ackwardtest = 2.4;
	float random = (float)rand()/RAND_MAX;
	if(random <= 0.2)
	{
		ackwardtest = 4.5;
	}
	if(random > 85 && random < 400.0)
	{
		ackwardtest = 3.2;
	}
	if(random > 400 && random < 800.0)
	{
		ackwardtest = 3.7;

	}
	if(random > 900 && random < 1400.0)
	{
		ackwardtest = 1.9;
	}
	if(random > 1400 && random < 14000.0)
	{
		ackwardtest = 2.3;
	}

	if(random > 14000 && random < 24000.0)
	{
		ackwardtest = 1.3;
	}
	if(random > 24000 && random < 34000.0)
	{
		ackwardtest = 1.3;
	}
	if(random > 990000)
	{
		ackwardtest = 1.2;
	}
	if(random < 0.2)
	{
		ackwardtest = 0.8;
	}
	
	ackwardtest += (float)rand()/RAND_MAX;
	ackwardtest = abs((long)ackwardtest);
	if (type == 0x1A) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (4 * ackwardtest) * (FLYING_KICK + GetSTR() + level) / 700);
		if ((float)rand()/RAND_MAX < 0.2) {
			ndamage = (sint32) (ndamage * 4.2);
			if(ndamage <= 0) {
				entity_list.MessageClose(this, false, 200, 10, "%s misses at an attempt to thunderous kick %s!",name,other->name);
			}
			else {
				entity_list.MessageClose(this, false, 200, 10, "%s lands a thunderous kick!(%d)", name, ndamage);
			}
		}
		other->Damage(this, ndamage, 0xffff, 0x1A);
		DoAnim(45);
	}
	else if (type == 0x34) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (2 * ackwardtest) * (TIGER_CLAW + GetSTR() + level) / 900);
		other->Damage(this, ndamage, 0xffff, 0x34);
		DoAnim(46);
	}
	else if (type == 0x26) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (3 * ackwardtest) * (ROUND_KICK + GetSTR() + level) / 800);
		other->Damage(this, ndamage, 0xffff, 0x26);
		DoAnim(11);
	}
	else if (type == 0x17) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (4 * ackwardtest) * (EAGLE_STRIKE + GetSTR() + level) / 1000);
		other->Damage(this, ndamage, 0xffff, 0x17);
		DoAnim(47);
	}
	else if (type == 0x15) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (5 * ackwardtest) * (DRAGON_PUNCH + GetSTR() + level) / 800);
		other->Damage(this, ndamage, 0xffff, 0x15);
		DoAnim(7);
	}
	else if (type == 0x1E) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (3 * ackwardtest) * (KICK + GetSTR() + level) / 1200);
		other->Damage(this, ndamage, 0xffff, 0x1e);
		DoAnim(1);
	}
}

void Mob::AddToHateList(Mob* other, sint32 hate, sint32 damage, bool iYellForHelp, bool bFrenzy, bool iBuffTic) {
    assert(other != NULL);
    if (other == this)
        return;
    if(damage < 0){
        hate = 1;
    }
	bool wasengaged = IsEngaged();
	Mob* owner = other->GetOwner();
	Mob* mypet = this->GetPet();
    Mob* myowner = this->GetOwner();
    if (other == myowner)
			return;
	if (owner) { // Other has a pet, add him and it
			hate_list.Add(other, hate, 0, bFrenzy, !iBuffTic);
			hate_list.Add(owner, 1, damage, false, !iBuffTic);
	}
	else { // Other has no pet, add other
			hate_list.Add(other, hate, damage, false, !iBuffTic);
	}
	if (mypet) { // I have a pet, add other to it
			mypet->hate_list.Add(other, 1, 0, bFrenzy);
	}
    else if (myowner) { // I am a pet, add other to owner if it's NPC/LD
            if (myowner->IsAIControlled())
                myowner->hate_list.Add(other, 1, 0, bFrenzy);
    }
    if (!wasengaged) {
			AI_Event_Engaged(other, iYellForHelp);
			adverrorinfo = 8293;
//			other->CastToClient()->CheckQuests(zone->GetShortName(), "%%ATTACK%%", GetNPCTypeID(), 0, this); // This causes crashes
	}
}

void Mob::DamageShield(Mob* other)
{
    int DS = 0;
    int DSRev = 0;
    int spellid = 0xFFFF;
	for (int i=0; i < BUFF_COUNT; i++)
	{
		if (buffs[i].spellid != 0xFFFF)
		{
			for (int z=0; z < 12; z++)
			{
				switch(spells[buffs[i].spellid].effectid[z])
				{
				case SE_DamageShield:
                {
                    int dmg = CalcSpellValue(spells[buffs[i].spellid].formula[z], spells[buffs[i].spellid].base[z], spells[buffs[i].spellid].max[z], GetLevel(), buffs[i].spellid);
                    DS += dmg;
                    spellid = buffs[i].spellid;
					break;
                }
				case SE_ReverseDS:
                    {
                    int dmg = CalcSpellValue(spells[buffs[i].spellid].formula[z], spells[buffs[i].spellid].base[z], spells[buffs[i].spellid].max[z], GetLevel(), buffs[i].spellid);
                    DSRev+=dmg;
                    spellid = buffs[i].spellid;
                    break;
                    }
				}
			}
		}
	}
    if (DS)
    {
        other->ChangeHP(this, DS, spellid);
        // todo: send EnvDamage packet to the other
		//entity_list.MessageClose(other, 0, 200, 10, "%s takes %d damage from %s's damage shield (%s)", other->GetName(), -spells[buffs[i].spellid].base[1], this->GetName(), spells[buffs[i].spellid].name);
    }
    if (DSRev)
    {
        this->ChangeHP(other, DSRev, spellid);
    }
}

int Mob::GetWeaponDamageBonus(const Item_Struct *Weapon)
{
	// Kaiyodo - Calculate the damage bonus for a weapon on the main hand
	if(GetLevel() < 28)
		return(0);
	
	// Check we're on of the classes that gets a damage bonus
	if ( !IsWarriorClass() )
		return 0;
	
	int BasicBonus = ((GetLevel() - 25) / 3) + 1;
	
	// If we have no weapon, or only a single handed weapon, just return the default
	// damage bonus of (Level - 25) / 3
	if(!Weapon || Weapon->common.skill == 0 || Weapon->common.skill == 2 || Weapon->common.skill == 3)
		return(BasicBonus);
	
	// Things get more complicated with 2 handers, the bonus is based on the delay of
	// the weapon as well as a number stored inside the weapon.
	int WeaponBonus = 0;	// How do you find this out?
	
	// Data for this, again, from www.monkly-business.com
	if(Weapon->common.delay <= 27)
		return(WeaponBonus + BasicBonus + 1);
	if(Weapon->common.delay <= 39)
		return(WeaponBonus + BasicBonus + ((GetLevel()-27) / 4));
	if(Weapon->common.delay <= 42)
		return(WeaponBonus + BasicBonus + ((GetLevel()-27) / 4) + 1);
	// Weapon must be > 42 delay
	return(WeaponBonus + BasicBonus + ((GetLevel()-27) / 4) + ((Weapon->common.delay-34) / 3));
}

int Mob::GetMonkHandToHandDamage(void)
{
	// Kaiyodo - Determine a monk's fist damage. Table data from www.monkly-business.com
    // saved as static array - this should speed this function up considerably
    static int damage[66] = {
    //   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
        99, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,
         8, 8, 8, 8, 8, 9, 9, 9, 9, 9,10,10,10,10,10,11,11,11,11,11,
        12,12,12,12,12,13,13,13,13,13,14,14,14,14,14,14,14,14,14,14,

        15,15,16,16,17,18 };
	
	// Have a look to see if we have epic fists on
	if (IsClient() && CastToClient()->pp.inventory[12] == 10652)
		return(9);
	else
	{
		int Level = GetLevel();
        if (Level > 65)
		    return(19);
        else
            return damage[Level];
	}
}

int Mob::GetMonkHandToHandDelay(void)
{
	// Kaiyodo - Determine a monk's fist delay. Table data from www.monkly-business.com
    // saved as static array - this should speed this function up considerably
    static int delayshuman[66] = {
    //  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
        99,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,
        36,36,36,36,36,35,35,35,35,35,34,34,34,34,34,33,33,33,33,33,
        32,32,32,32,32,31,31,31,31,31,30,30,30,29,29,29,28,28,28,27,
        27,26,26,25,25,25 };
    static int delaysiksar[66] = {
    //  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
        99,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,
        36,36,36,36,36,36,36,36,36,36,35,35,35,35,35,34,34,34,34,34,
        33,33,33,33,33,32,32,32,32,32,31,31,31,30,30,30,29,29,29,28,
        28,27,27,26,26,26 };

    // Have a look to see if we have epic fists on
	if (IsClient() && CastToClient()->pp.inventory[12] == 10652)
		return(16);
	else
	{
		int Level = GetLevel();
		if (GetRace() == HUMAN)
		{
            if (Level > 65)
			    return(24);
            else
                return delayshuman[Level];
		}
		else	//heko: iksar table
		{
            if (Level > 65)
			    return(25);
            else
                return delaysiksar[Level];
		}
	}

}

//heko: backstab
void Mob::RogueBackstab(Mob* other, const Item_Struct *weapon1, int8 bs_skill)
{
	int ndamage = 0;
	int max_hit, min_hit;
	float skillmodifier = 0.0;
	int8 primaryweapondamage;
	if (weapon1)
		primaryweapondamage = weapon1->common.damage; //backstab uses primary weapon
	else
		primaryweapondamage = this->GetLevel() % 10; // fallback incase it's a npc without a weapon
	
    // catch a divide by zero error
    if (!bs_skill)
        return;

	skillmodifier = (float)bs_skill/25.0;	//formula's from www.thesafehouse.org
	
	// formula is (weapon damage * 2) + 1 + (level - 25)/3 + (strength+skill)/100
	max_hit = (int)(((float)primaryweapondamage * 2.0) + 1.0 + ((level - 25)/3.0) + ((GetSTR()+GetSkill(BACKSTAB))/100));
	max_hit *= (int)skillmodifier;
	
	// determine minimum hits
	if (level < 51)
	{
		min_hit = 0;
	}
	else
	{
		// Trumpcard:  Replaced switch statement with formula calc.  This will give minhit increases all the way to 65.
		min_hit= (int)( level * ( 1.5 + ( (level - 51) * .05 ) ));
	}
	if (max_hit < min_hit)
		max_hit = min_hit;
	ndamage = (int)min_hit + (rand()%((max_hit-min_hit)+1));	// TODO: better formula, consider mob level vs player level, strength/atk
	other->Damage(this, ndamage, 0xffff, 0x08);	//0x08 is backstab
	DoAnim(2);	//piercing animation
}


// solar - assassinate
void Mob::RogueAssassinate(Mob* other)
{
	other->Damage(this, 32000, 0xffff, 0x08);	//0x08 is backstab
	DoAnim(2);	//piercing animation
	entity_list.Message(0, MT_Shout, "%s ASSASSINATES their victim.", this->GetName());
}

// neotokyo 14-Nov-02

// Helperfunction to check for Lifetaps
bool Mob::IsLifetapSpell(int16 spell_id)
{
	// filter out invalid spell_ids
	if (spell_id <= 0 || spell_id >= 0xffff)
		return false;
	
	// if database says its a tap, i am sure its right
	if (spells[spell_id].targettype == ST_Tap)
		return true;
	
	// now check some additional lifetaps just to make sure
	// i.e. lifebane isnt recognized since type == target not tap

	switch(spell_id)
	{
	case 1613:
	case 341:
	case 445:
	case 502:
	case 447:
	case 525:
	case 2115: // Ancient: Lifebane
	case 446:
	case 524:
	case 1618: 
	case 1393: // Gangrenous touch of zu'muul
	case 1735: // trucidation
		return true;
	}
	return false;
}

sint16 Mob::ReduceMagicalDamage(sint16 damage, int16 in_rune)

{
	if (in_rune >= abs(damage))
	{
		in_rune -= abs(damage);
		damage = 0;
	}
	else
	{
		damage += in_rune;
		in_rune = 0;
        int slot = GetBuffSlotFromType(SE_AbsorbMagicAtt);
        if (slot >= 0)
            BuffFadeBySlot(slot);
	}
	SetMagicRune(in_rune);
	return damage;
}

sint16 Mob::ReduceDamage(sint16 damage, int16 in_rune)
{
	if (in_rune >= damage)
	{
		in_rune -= damage;
		damage = -6;
	}
	else
	{
		damage -= in_rune;
		in_rune = 0;
        int slot = GetBuffSlotFromType(SE_Rune);
		LogFile->write(EQEMuLog::Normal, "Fading rune from slot %d",slot);
        if (slot >= 0)
            BuffFadeBySlot(slot);
	}
	SetRune(in_rune);
	return damage;
}

bool Mob::HasProcs()
{
    for (int i = 0; i < MAX_PROCS; i++)
        if (PermaProcs[i].spellID != 0xFFFF || SpellProcs[i].spellID != 0xFFFF)
            return true;
    return false;
}
