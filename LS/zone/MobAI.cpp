#include <iostream.h>
#include <iomanip.h>
#include <stdlib.h>
#include <math.h>
#include "mob.h"
#include "client.h"
#include "npc.h"
#include "NpcAI.h"
#include "map.h"
#include "../common/moremath.h"
#include "parser.h"
#include "groups.h"

extern EntityList entity_list;
extern Database database;
extern Zone *zone;
extern Parser * parse;

#define Z_AGGRO         10
#define THREATENLY_ARRGO_CHANCE		25 // % chance that a mob will arrgo on con Threatenly
#define MobAISpellRange				100 // max range of buffs
#define SpellType_Nuke		1
#define SpellType_Heal		2
#define SpellType_Root		4
#define SpellType_Buff		8
#define SpellType_Escape	16
#define SpellType_Pet		32
#define SpellType_Lifetap	64
#define SpellType_Snare		128
#define SpellType_DOT		256
#define SpellType_Any		0xFFFF
bool timercompleted=true;
#ifdef _DEBUG
	#define MobAI_DEBUG_Spells	-1
#else
	#define MobAI_DEBUG_Spells	-1
#endif

bool Mob::AICastSpell(Mob* tar, int8 iChance, int16 iSpellTypes) {
// Faction isnt checked here, it's assumed you wouldnt pass a spell type you wouldnt want casted on the mob
	if (!tar)
		return false;
	if (iChance < 100) {
		int8 tmp = rand()%100;
		if (tmp >= iChance)
			return false;
	}
	float dist = Dist(tar);
	if (iSpellTypes & SpellType_Escape) {
	    dist = Dist(this);
    }
	float manaR = GetManaRatio();
//	for (int i=0; i<MAX_AISPELLS; i++) {
	for (int i=MAX_AISPELLS-1; i >= 0; i--) {
		if (AIspells[i].spellid <= 0 || AIspells[i].spellid >= SPDAT_RECORDS) {
			// this is both to quit early to save cpu and to avoid casting bad spells
			// Bad info from database can trigger this incorrectly, but that should be fixed in DB, not here
			//return false;
			continue;
		}
		if (iSpellTypes & AIspells[i].type) {
			// manacost has special values, -1 is no mana cost, -2 is instant cast (no mana)
			sint32 mana_cost = AIspells[i].manacost;
			if (mana_cost == -1)
				mana_cost = spells[AIspells[i].spellid].mana;
			else if (mana_cost == -2)
				mana_cost = 0;
			if (
				dist <= spells[AIspells[i].spellid].range
				&& (mana_cost <= GetMana() || GetMana() == GetMaxMana())
				&& AIspells[i].time_cancast <= Timer::GetCurrentTime()
				) {
#if MobAI_DEBUG_Spells >= 21
				cout << "Mob::AICastSpell: Casting: spellid=" << AIspells[i].spellid
                    << ", tar=" << tar->GetName() 
                    << ", dist[" << dist << "]<=" << spells[AIspells[i].spellid].range 
                    << ", mana_cost[" << mana_cost << "]<=" << GetMana() 
                    << ", cancast[" << AIspells[i].time_cancast << "]<=" << Timer::GetCurrentTime()
                    << ", type=" << AIspells[i].type << endl;
#endif
				switch (AIspells[i].type) {
					case SpellType_Heal: {
						if (
							(spells[AIspells[i].spellid].targettype == ST_Target || tar == this)
							&& tar->DontHealMeBefore() < Timer::GetCurrentTime()
							) {
							int8 hpr = (int8)tar->GetHPRatio();
							if (
								hpr <= 35 
								|| (!IsEngaged() && hpr <= 50)
								|| (tar->IsClient() && hpr <= 99)
								) {
								AIDoSpellCast(i, tar, mana_cost, &tar->DontHealMeBefore());
								return true;
							}
						}
						break;
					}
					case SpellType_Root: {
						if (
							!tar->IsRooted() && dist >= 30 && (rand()%100) < 50
							&& tar->DontRootMeBefore() < Timer::GetCurrentTime()
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost, &tar->DontRootMeBefore());
							return true;
						}
						break;
					}
					case SpellType_Buff: {
						if (
							(spells[AIspells[i].spellid].targettype == ST_Target || tar == this)
							&& tar->DontBuffMeBefore() < Timer::GetCurrentTime()
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost, &tar->DontBuffMeBefore());
							return true;
						}
						break;
					}
					case SpellType_Escape: {
						if (GetHPRatio() <= 5 || (IsNPC() && CastToNPC()->IsInteractive() && tar != this) ) {
							AIDoSpellCast(i, tar, mana_cost);
							return true;
						}
						break;
					}
					case SpellType_Nuke: {
						if (
							manaR >= 40 && (rand()%100) < 50
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost);
							return true;
						}
						break;
					}
					case SpellType_Pet: {
						if (!GetPetID() && (rand()%100) < 25) {
							AIDoSpellCast(i, tar, mana_cost);
							return true;
						}
						break;
					}
					case SpellType_Lifetap: {
						if (GetHPRatio() <= 75
							&& (rand()%100) < 50
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost);
							return true;
						}
						break;
					}
					case SpellType_Snare: {
						if (
							!tar->IsRooted() && (rand()%100) < 50
							&& tar->DontSnareMeBefore() < Timer::GetCurrentTime()
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost, &tar->DontSnareMeBefore());
							return true;
						}
						break;
					}
					case SpellType_DOT: {
						if (
							tar->GetHPRatio() > 50 && (rand()%100) < 20
							&& tar->DontDotMeBefore() < Timer::GetCurrentTime()
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost, &tar->DontDotMeBefore());
							return true;
						}
						break;
					}
					default: {
						cout<<"Error: Unknown spell type in AICastSpell. caster:"<<this->GetName()<<" type:"<<AIspells[i].type<<" slot:"<<i<<endl;
						break;
					}
				}
			}
#if MobAI_DEBUG_Spells >= 21
			else {
				cout << "Mob::AICastSpell: NotCasting: spellid=" << AIspells[i].spellid << ", tar=" << tar->GetName() << ", dist[" << dist << "]<=" << spells[AIspells[i].spellid].range << ", mana_cost[" << mana_cost << "]<=" << GetMana() << ", cancast[" << AIspells[i].time_cancast << "]<=" << Timer::GetCurrentTime() << endl;
			}
#endif
		}
	}
	return false;
}

void Mob::AIDoSpellCast(int8 i, Mob* tar, sint32 mana_cost, int32* oDontDoAgainBefore) {
#if MobAI_DEBUG_Spells >= 1
	cout << "Mob::AIDoSpellCast: spellid=" << AIspells[i].spellid << ", tar=" << tar->GetName() << ", mana=" << mana_cost << ", Name: " << spells[AIspells[i].spellid].name << endl;
#endif
	casting_spell_AIindex = i;
	CastSpell(AIspells[i].spellid, tar->GetID(), 1, AIspells[i].manacost == -2 ? 0 : -1, mana_cost, oDontDoAgainBefore);
}

Mob* EntityList::AICheckCloseArrgo(Mob* sender, float iArrgoRange, float iAssistRange) {
	if (sender->GetPrimaryFaction() == 0 && (sender->CastToNPC()->GetGuildOwner() == 0 || sender->CastToNPC()->GetGuildOwner() == 3))
		return 0; // well, if we dont have a faction set, we're gonna be indiff to everybody
    LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	int8 RandRoll = rand()%100;
	float dist, distZ;
    while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob() && ! iterator.GetData()->IsDoor() && !iterator.GetData()->IsCorpse() && iterator.GetData() != sender) {
			Mob* mob = iterator.GetData()->CastToMob();
			FACTION_VALUE fv = FACTION_INDIFFERENT;
			fv = mob->GetFactionCon(sender);
			dist = mob->Dist(sender);
            distZ = mob->Dist(sender)-mob->DistNoZ(sender);
			// Check If it's invisible and if we can see invis
			// Check if it's a client, and that the client is connected and not linkdead,
			//   and that the client isn't Playing an NPC, with thier gm flag on
			// Check if it's not a Interactive NPC
			if(   ( mob->IsInvisible(sender))
			   || ( mob->IsClient() && (!mob->CastToClient()->Connected() || mob->CastToClient()->IsLD()) )
			   || ( mob->IsClient() && mob->CastToClient()->IsBecomeNPC() )
			   || ( mob->IsClient() && (mob->CastToClient()->GetGM() || mob->CastToClient()->Admin() > 0) )
               || ( mob->IsNPC()    && !mob->CastToNPC()->IsInteractive() )
               || ( dist > (iAssistRange*2) && dist > iArrgoRange )
               || ( (distZ <= 0 && distZ < (Z_AGGRO-Z_AGGRO-Z_AGGRO)) || (distZ >= 0 && distZ > (Z_AGGRO)) )
               )
            {
            // Skip it
                     //if (   DEBUG >= 5
                     //    && mob->IsClient()
                     //    && mob->CastToClient()->Connected()
                     //    && !mob->CastToClient()->IsLD()
                     //    && dist <= iArrgoRange
                     //   )
                     //     LogFile->write(EQEMuLog::Debug, "Check aggro for %s skipping client %s.", sender->GetName(), mob->GetName());
                     //if (   DEBUG >= 5
                     //    && mob->IsNPC()
                     //    && mob->CastToNPC()->IsInteractive()
                     //   )
                     //     LogFile->write(EQEMuLog::Debug, "Check aggro for %s skipping IPC %s.", sender->GetName(), mob->GetName());
                     iterator.Advance();
                     continue;
            }

#ifdef GUILDWARS
if(sender->IsNPC() && sender->CastToNPC()->GetGuildOwner() != 0 && dist <= iAssistRange)
{
if(mob->IsNPC() && mob->CastToNPC()->GetGuildOwner() != 0 && mob->IsEngaged() && mob->GetTarget())
{
if((mob->CastToNPC()->GetGuildOwner() == sender->CastToNPC()->GetGuildOwner()) || (database.GetGuildAlliance(sender->CastToNPC()->GetGuildOwner(),mob->CastToNPC()->GetGuildOwner())))
return mob->GetTarget();
}
}
if(sender->IsNPC() && sender->CastToNPC()->GetGuildOwner() != 0 && dist <= iArrgoRange && mob->IsClient() && mob->CastToClient()->GuildDBID() != 0 && sender->CastToNPC()->cityfactionstanding == 0)
{
if((mob->CastToClient()->GuildDBID() != sender->CastToNPC()->GetGuildOwner()) && (!database.GetGuildAlliance(sender->CastToNPC()->GetGuildOwner(),mob->CastToClient()->GuildDBID())))
return mob;
}
if(sender->IsNPC() && sender->CastToNPC()->GetGuildOwner() != 0 && dist <= iArrgoRange && mob->IsNPC() && mob->CastToNPC()->GetOwner() != 0 && mob->CastToNPC()->GetOwner()->IsClient() != 0 && mob->CastToNPC()->GetOwner()->CastToClient()->GuildDBID() != 0 && sender->CastToNPC()->cityfactionstanding == 0)
{
if((mob->CastToClient()->GuildDBID() != sender->CastToNPC()->GetGuildOwner()) && (!database.GetGuildAlliance(sender->CastToNPC()->GetGuildOwner(),mob->CastToNPC()->GetOwner()->CastToClient()->GuildDBID())))
return mob;
}
if(sender->IsNPC() && sender->CastToNPC()->GetGuildOwner() != 0)
{
//Do nothing
}
#endif
        // Assist check
        // Check faction amiable or better (friend)
        // Is friend engaged
        // Does friend have a target
        // Is friend in range
        // Are we stupid, or is friends target green
        // Is friends target in range
		if (fv <= FACTION_AMIABLE
				&& mob->IsEngaged()
				&& mob->GetTarget()
				&& dist <= iAssistRange
				&& (mob->GetINT() <= 100 || mob->GetTarget()->GetLevelCon(sender->GetLevel()) != CON_GREEN)
				&& sender->Dist(mob->GetTarget()) <= (iAssistRange * 2)
				) {
				// Assist friend
                LogFile->write(EQEMuLog::Debug, "Check aggro for %s assisting %s, target %s.", sender->GetName(), mob->GetName(), mob->GetTarget()->GetName());
				return mob->GetTarget();
		}
		// Make sure they're still in the zone
		// Are they in range?
		// Are they kos?
		// Are we stupid or are they green
		// and they don't have thier gm flag on
		if ( 
				mob->InZone()
				&& dist <= iArrgoRange
				&& (fv == FACTION_SCOWLS || (fv == FACTION_THREATENLY && RandRoll < THREATENLY_ARRGO_CHANCE))

				&& (sender->GetINT() <= 75 || mob->GetLevelCon(sender->GetLevel()) != CON_GREEN)
	   ) {
	       if(  (mob->IsClient()&&!mob->CastToClient()->GetGM()) || (mob->IsNPC()&& mob->CastToNPC()->IsInteractive()) ){
				// Aggro
				LogFile->write(EQEMuLog::Debug, "Check aggro for %s target %s.", sender->GetName(), mob->GetName());
				return mob;
		   } else {
		   }
		} else  if (DEBUG >= 6){
		  cout<<"In zone:"<<mob->InZone()<<endl;
		  cout<<"Dist:"<<dist<<endl;
		  cout<<"Range:"<<iArrgoRange<<endl;
		  cout<<"Faction:"<<fv<<endl;
		  cout<<"Int:"<<sender->GetINT()<<endl;
		  cout<<"Con:"<<mob->GetLevelCon(sender->GetLevel())<<endl;
		  //In zone:1
		  //Dist:55.3376
		  //Range:85
		  //Faction:5
		  //Int:75
		  //Con:4
		}
		} // No data
		iterator.Advance();
	}
	//LogFile->write(EQEMuLog::Debug, "Check aggro for %s no target.", sender->GetName());
	return 0;
}

void EntityList::AIYellForHelp(Mob* sender, Mob* attacker) {
	if (sender->GetPrimaryFaction() == 0 && sender->CastToNPC()->GetGuildOwner() == 0)
		return; // well, if we dont have a faction set, we're gonna be indiff to everybody
    LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
    while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob()) {
			Mob* mob = iterator.GetData()->CastToMob();

#ifdef GUILDWARS
if(mob != sender && mob != attacker && mob->Dist(sender) <= mob->GetAssistRange() && sender->IsNPC() && sender->CastToNPC()->GetGuildOwner() != 0)
{
if(mob->IsNPC() && mob->CastToNPC()->GetGuildOwner() != 0 && mob->IsEngaged() && mob->GetTarget())
{
if((mob->CastToNPC()->GetGuildOwner() == sender->CastToNPC()->GetGuildOwner()) || (database.GetGuildAlliance(sender->CastToNPC()->GetGuildOwner(),mob->CastToNPC()->GetGuildOwner())))
{
if((mob->GetTarget()->IsClient() && mob->GetTarget()->CastToClient()->Admin() == 0) || (mob->GetTarget()->IsNPC()))
mob->AddToHateList(attacker,0,0,false);
else if(mob->GetTarget()->IsNPC() && mob->GetTarget()->CastToNPC()->GetOwner() != 0 && mob->GetTarget()->CastToNPC()->GetOwner()->IsClient() && mob->CastToNPC()->GetTarget()->GetOwner()->CastToClient()->GuildDBID() != 0)
mob->AddToHateList(attacker,0,0,false);

}
}
}
	else
#endif
		 if (mob != sender
				&& mob != attacker
				&& !mob->IsCorpse()
				&& mob->IsAIControlled()
				&& mob->Dist(sender) <= mob->GetAssistRange()
				&& ( ((sender->GetZ()+mob->GetZ()) <= 0 && (sender->GetZ()+mob->GetZ()) > (Z_AGGRO-Z_AGGRO-Z_AGGRO)) || ((sender->GetZ()+mob->GetZ()) >= 0 && (sender->GetZ()+mob->GetZ()) < Z_AGGRO) )
				) {
				FACTION_VALUE fv = sender->GetFactionCon(mob);
				if (fv <= FACTION_AMIABLE && !mob->IsEngaged() && (mob->GetINT() <= 100 || attacker->GetLevelCon(mob->GetLevel()) != CON_GREEN)) {
					if (fv == 1) {
					    if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "AIYellForHelp(\"%s\",\"%s\") %s attacking %s Dist %f Z %f", 
                           sender->GetName(), attacker->GetName(), mob->GetName(), attacker->GetName(), mob->Dist(sender), (sender->GetZ()+mob->GetZ()));
						mob->AddToHateList(attacker, 0, 0, false);
					}
					else {
                        if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "AIYellForHelp(\"%s\",\"%s\") %s attacking %s Dist %f Z %f", 
                           sender->GetName(), attacker->GetName(), mob->GetName(), sender->GetName(), mob->Dist(sender), (sender->GetZ()+mob->GetZ()));
						mob->AddToHateList(sender, 0, 0, false);
					}
				}
			}
		}
		iterator.Advance();
	}
}

bool EntityList::AICheckCloseSpells(Mob* caster, int8 iChance, float iRange, int16 iSpellTypes) {
	if (iChance < 100) {
		int8 tmp = rand()%100;
		if (tmp >= iChance)
			return false;
	}
	iRange *= iRange;
    LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
    while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob()) {
			Mob* mob = iterator.GetData()->CastToMob();
			if (mob != caster && mob->DistNoRoot(caster) <= iRange) {
				if (caster->GetPetID() == mob->GetID() || caster->GetOwnerID() == mob->GetID() || mob->GetFactionCon(caster) <= FACTION_AMIABLE) {
					// we have a winner!
					if (caster->AICastSpell(mob, 100, iSpellTypes))
						return true;
				}
			}
		}
		iterator.Advance();
	}
	return false;
}

// returns what Other thinks of this
FACTION_VALUE Client::GetFactionCon(Mob* iOther) {
	if (GetOwnerID()) {
		return GetOwnerOrSelf()->GetFactionCon(iOther);
	}
	iOther = iOther->GetOwnerOrSelf();
	if (iOther->GetPrimaryFaction() < 0)
		return GetSpecialFactionCon(iOther);
	if (iOther->GetPrimaryFaction() == 0)
		return FACTION_INDIFFERENT;

	return GetFactionLevel(this->CharacterID(), 0, this->GetRace(), this->GetClass(), this->GetDeity(), iOther->GetPrimaryFaction(), iOther);
}

FACTION_VALUE NPC::GetFactionCon(Mob* iOther) {
	iOther = iOther->GetOwnerOrSelf();
	if (iOther->GetPrimaryFaction() < 0)
		return GetSpecialFactionCon(iOther);
	if (iOther->GetPrimaryFaction() == 0)
		return FACTION_INDIFFERENT;
	if (GetOwnerID())
		return GetOwnerOrSelf()->GetFactionCon(iOther);

	sint8 tmp = CheckNPCFactionAlly(iOther->GetPrimaryFaction());
#ifdef GUILDWARS
if(iOther->IsNPC() && GetGuildOwner() == iOther->CastToNPC()->GetGuildOwner() || database.GetGuildAlliance(GetGuildOwner(),iOther->CastToNPC()->GetGuildOwner())) // target is an npc, target is of the same guild or has a guild alliance
tmp = 1;
else if(iOther->IsClient() && GetGuildOwner() == iOther->CastToClient()->GuildDBID() || database.GetGuildAlliance(GetGuildOwner(),iOther->CastToClient()->GuildDBID())) // target is client, they are the same guild id or have a guild alliance
tmp = 1;
else if(GetGuildOwner() == 3) // default guild, is neutral
tmp = -2;
else
tmp = -1;
#endif
	if (tmp == 1)
		return FACTION_ALLY;
	else if (tmp == -1)
		return FACTION_SCOWLS;
	return FACTION_INDIFFERENT;
}

FACTION_VALUE Mob::GetSpecialFactionCon(Mob* iOther) {
	if (!iOther)
		return FACTION_INDIFFERENT;
	iOther = iOther->GetOwnerOrSelf();
	Mob* self = this->GetOwnerOrSelf();
	if (self->GetPrimaryFaction() >= 0 && self->IsAIControlled())
		return FACTION_INDIFFERENT;
	if (iOther->GetPrimaryFaction() >= 0)
		return FACTION_INDIFFERENT;
/* special values:
	-2 = indiff to player, ally to AI on special values, indiff to AI
	-3 = dub to player, ally to AI on special values, indiff to AI
	-4 = atk to player, ally to AI on special values, indiff to AI
	-5 = indiff to player, indiff to AI
	-6 = dub to player, indiff to AI
	-7 = atk to player, indiff to AI
	-8 = indiff to players, ally to AI on same value, indiff to AI
	-9 = dub to players, ally to AI on same value, indiff to AI
	-10 = atk to players, ally to AI on same value, indiff to AI
	-11 = indiff to players, ally to AI on same value, atk to AI
	-12 = dub to players, ally to AI on same value, atk to AI
	-13 = atk to players, ally to AI on same value, atk to AI
*/
	switch (iOther->GetPrimaryFaction()) {
		case -2: // -2 = indiff to player, ally to AI on special values, indiff to AI
			if (self->IsAIControlled() && iOther->IsAIControlled())
				return FACTION_ALLY;
			else
				return FACTION_INDIFFERENT;
		case -3: // -3 = dub to player, ally to AI on special values, indiff to AI
			if (self->IsAIControlled() && iOther->IsAIControlled())
				return FACTION_ALLY;
			else
				return FACTION_DUBIOUS;
		case -4: // -4 = atk to player, ally to AI on special values, indiff to AI
			if (self->IsAIControlled() && iOther->IsAIControlled())
				return FACTION_ALLY;
			else
				return FACTION_SCOWLS;
		case -5: // -5 = indiff to player, indiff to AI
			return FACTION_INDIFFERENT;
		case -6: // -6 = dub to player, indiff to AI
			if (self->IsAIControlled() && iOther->IsAIControlled())
				return FACTION_INDIFFERENT;
			else
				return FACTION_DUBIOUS;
		case -7: // -7 = atk to player, indiff to AI
			if (self->IsAIControlled() && iOther->IsAIControlled())
				return FACTION_INDIFFERENT;
			else
				return FACTION_SCOWLS;
		case -8: // -8 = indiff to players, ally to AI on same value, indiff to AI
			if (self->IsAIControlled() && iOther->IsAIControlled()) {
				if (self->GetPrimaryFaction() == iOther->GetPrimaryFaction())
					return FACTION_ALLY;
				else
					return FACTION_INDIFFERENT;
			}
			else
				return FACTION_INDIFFERENT;
		case -9: // -9 = dub to players, ally to AI on same value, indiff to AI
			if (self->IsAIControlled() && iOther->IsAIControlled()) {
				if (self->GetPrimaryFaction() == iOther->GetPrimaryFaction())
					return FACTION_ALLY;
				else
					return FACTION_INDIFFERENT;
			}
			else
				return FACTION_DUBIOUS;
		case -10: // -10 = atk to players, ally to AI on same value, indiff to AI
			if (self->IsAIControlled() && iOther->IsAIControlled()) {
				if (self->GetPrimaryFaction() == iOther->GetPrimaryFaction())
					return FACTION_ALLY;
				else
					return FACTION_INDIFFERENT;
			}
			else
				return FACTION_SCOWLS;
		case -11: // -11 = indiff to players, ally to AI on same value, atk to AI
			if (self->IsAIControlled() && iOther->IsAIControlled()) {
				if (self->GetPrimaryFaction() == iOther->GetPrimaryFaction())
					return FACTION_ALLY;
				else
					return FACTION_SCOWLS;
			}
			else
				return FACTION_INDIFFERENT;
		case -12: // -12 = dub to players, ally to AI on same value, atk to AI
			if (self->IsAIControlled() && iOther->IsAIControlled()) {
				if (self->GetPrimaryFaction() == iOther->GetPrimaryFaction())
					return FACTION_ALLY;
				else
					return FACTION_SCOWLS;


			}
			else
				return FACTION_DUBIOUS;
		case -13: // -13 = atk to players, ally to AI on same value, atk to AI
			if (self->IsAIControlled() && iOther->IsAIControlled()) {
				if (self->GetPrimaryFaction() == iOther->GetPrimaryFaction())
					return FACTION_ALLY;
				else
					return FACTION_SCOWLS;
			}
			else
				return FACTION_SCOWLS;
		default:
			return FACTION_INDIFFERENT;
	}
}

void Mob::AI_Init() {
	pAIControlled = false;
	AIthink_timer = 0;
	AIwalking_timer = 0;
	AImovement_timer = 0;
	AIautocastspell_timer = 0;
	AIscanarea_timer = 0;
	pLastFightingDelayMoving = 0;
	minLastFightingDelayMoving = 10000;
	maxLastFightingDelayMoving = 20000;
	memset(AIspells, 0, sizeof(AIspells));
	casting_spell_AIindex = MAX_AISPELLS;
	npc_spells_id = 0;

	pDontHealMeBefore = 0;
	pDontBuffMeBefore = 0;
	pDontDotMeBefore = 0;
	pDontRootMeBefore = 0;
	pDontSnareMeBefore = 0;
	pDontCastBefore_casting_spell = 0;

	roambox_max_x = 0;
	roambox_max_y = 0;
	roambox_min_x = 0;
	roambox_min_y = 0;
	roambox_distance = 0;
	roambox_movingto_x = 0;
	roambox_movingto_y = 0;
	roambox_delay = 2500;
}

void NPC::AI_Init() {
	Mob::AI_Init();
}

void Client::AI_Init() {
	Mob::AI_Init();
	minLastFightingDelayMoving = CLIENT_LD_TIMEOUT;
	maxLastFightingDelayMoving = CLIENT_LD_TIMEOUT;
}

void Mob::AI_Start(int32 iMoveDelay) {
	if (iMoveDelay)
		pLastFightingDelayMoving = Timer::GetCurrentTime() + iMoveDelay;
	else
		pLastFightingDelayMoving = 0;
	if (pAIControlled)
		return;
	pAIControlled = true;
	AIthink_timer = new Timer(50);
	AIthink_timer->Trigger();
	AIwalking_timer = new Timer(0);
	AImovement_timer = new Timer(100);
	AIautocastspell_timer = new Timer(750);
	AIautocastspell_timer->Start(RandomTimer(0, 15000), false);
	AIscanarea_timer = new Timer(500);
	for (int i=0; i<MAX_AISPELLS; i++) {
		AIspells[i].spellid = 0xFFFF;
		AIspells[i].type = 0;
	}

	if (GetArrgoRange() == 0)
		pArrgoRange = 70;
	if (GetAssistRange() == 0)
		pAssistRange = 70;
	hate_list.Wipe();

	delta_heading = 0;
	delta_x = 0;
	delta_y = 0;
	delta_z = 0;
	pRunAnimSpeed = 0;
	pLastChange = Timer::GetCurrentTime();
}

void Client::AI_Start(int32 iMoveDelay) {
	Mob::AI_Start(iMoveDelay);
	if (!pAIControlled)
		return;
	// copy memed spells to the spells struct here

	Message(13, "You no longer feel in control of yourself!");
/*	APPLAYER *app = new APPLAYER(OP_Charm, sizeof(Charm_Struct));
	Charm_Struct *ps = (Charm_Struct*)app->pBuffer;
	ps->owner_id = GetOwnerOrSelf()->GetID();
	ps->pet_id = this->GetID();
	ps->command = 1;
	FastQueuePacket(&app);*/
	if (this->isgrouped && entity_list.GetGroupByClient(this) != NULL)
    {
		Group* group = entity_list.GetGroupByClient(this);
		group->DelMember(CastToMob(),true);
    }

	if (AIspells[0].spellid == 0)
		AIautocastspell_timer->Disable();
	SaveSpawnSpot();
	pClientSideTarget = target ? target->GetID() : 0;
	SendAppearancePacket(14, 102);
	SendAppearancePacket(18, 1); // Sending LD packet so *LD* appears by the player name when charmed/feared -Kasai
	attack_timer->Enable();
	attack_timer_dw->Enable();
	SetAttackTimer();
}

void NPC::AI_Start(int32 iMoveDelay) {
	Mob::AI_Start(iMoveDelay);
	if (!pAIControlled)
		return;
	if (NPCTypedata) {
		AI_AddNPCSpells(NPCTypedata->npc_spells_id);
		NPCSpecialAttacks(NPCTypedata->npc_attacks,0);
	}
	if (AIspells[0].spellid == 0)
		AIautocastspell_timer->Disable();
	SendTo(GetX(), GetY(), GetZ());
	SetChanged();
	SaveSpawnSpot();
	SaveGuardSpot();
}

void Mob::AI_Stop() {
	if (!IsAIControlled())
		return;
	pAIControlled = false;
	safe_delete(AIthink_timer);
	safe_delete(AIwalking_timer);
	safe_delete(AImovement_timer);
	safe_delete(AIautocastspell_timer);
	safe_delete(AIscanarea_timer);
	hate_list.Wipe();
}

void Client::AI_Stop() {
	Mob::AI_Stop();
	Message(13, "You are in control of yourself again.");
	APPLAYER *app = new APPLAYER(OP_Charm, sizeof(Charm_Struct));
	Charm_Struct *ps = (Charm_Struct*)app->pBuffer;
	ps->owner_id = 0;
	ps->pet_id = this->GetID();
	ps->command = 0;
	FastQueuePacket(&app);
	target = entity_list.GetMob(pClientSideTarget);
	SendAppearancePacket(14, GetAppearanceValue(appearance));
	SendAppearancePacket(18, 0); // Removing LD packet so *LD* no longer appears by the player name when charmed/feared -Kasai
	if (!auto_attack) {
		attack_timer->Disable();
		attack_timer_dw->Disable();
	}
	if (IsLD())
		Disconnect();
}

void Mob::AI_Process() {
	if (!IsAIControlled())
		return;
	if (!(AIthink_timer->Check() || attack_timer->Check(false)))
		return;
	if (AIwalking_timer->Check()){
		timercompleted=true;
		AIwalking_timer->Disable();
	}
	if (IsCasting())
		return;
	if (IsEngaged()) {
		if (IsRooted())
			SetTarget(hate_list.GetClosest(this));
		else
			SetTarget(hate_list.GetTop());
		if (!target)
			return;
        if (GetHPRatio() < 15)
            StartEnrage();
        if (CombatRange(target)) {
			if (AImovement_timer->Check()) {
				SetRunAnimSpeed(0);
			}
			if (GetAppearance() == 0 && GetRunAnimSpeed() == 0 && !IsStunned())
				FaceTarget(target, true);
			if (attack_timer->Check()) {
    			Attack(target, 13);
				if (CanThisClassDoubleAttack()) {
					sint32 RandRoll = rand()%100;
					if (RandRoll < (GetLevel() + 20))  {
						if (Attack(target, 13)) {
							// lets see if we can do a triple attack with the main hand
							if (SpecAttacks[SPECATK_TRIPLE]) {
								if (!GetOwner() && RandRoll < (GetLevel()))  {
									if (Attack(target, 13)) {
										// now lets check the quad attack
										if (SpecAttacks[SPECATK_QUAD]) {
											if (!GetOwner() && RandRoll < (GetLevel() - 20))  {
            									Attack(target, 13);
											}
										} // if (SpecAttacks[SPECATK_QUAD])
									}
								}
							} // if (SpecAttacks[SPECATK_TRIPLE])
						}
					}
				} // if (CanThisClassDoubleAttack())
                
				if (SpecAttacks[SPECATK_FLURRY])
					Flurry();
				if (SpecAttacks[SPECATK_RAMPAGE])
					Rampage();
			}
			if (attack_timer_dw->Check() && CanThisClassDuelWield()) {
    			Attack(target, 14);
				if (CanThisClassDoubleAttack()) {
					sint32 RandRoll = rand()%100;
					if (RandRoll < (GetLevel() + 20))  {
						if (Attack(target, 14));
					}
				} // if (CanThisClassDoubleAttack())
			}
// TODO: Check special attacks (backstab, etc) here
			if (AIautocastspell_timer->Check()) {
#if MobAI_DEBUG_Spells >= 25
				cout << "Engaged autocast check triggered: " << this->GetName() << endl;
#endif
				if (!AICastSpell(this, 100, SpellType_Heal | SpellType_Escape)) // try casting a heal or gate
					if (!entity_list.AICheckCloseSpells(this, 25, MobAISpellRange, SpellType_Heal)) // try casting a heal on nearby
						AICastSpell(target, 20, SpellType_Nuke | SpellType_Lifetap | SpellType_DOT);
			}
		}
		else {
			// See if we can summon the mob to us
			if (!HateSummon()) {
// TODO: Check here for another person on hate list with close hate value
				if (AIautocastspell_timer->Check()) {
#if MobAI_DEBUG_Spells >= 25
					cout << "Engaged (pursing) autocast check triggered: " << this->GetName() << endl;
#endif
					AICastSpell(target, 90, SpellType_Root | SpellType_Nuke | SpellType_Lifetap | SpellType_Snare);
				}
				else if (AImovement_timer->Check() && !IsRooted()) {
					CalculateNewPosition(target->GetX(), target->GetY(), target->GetZ(), GetRunspeed());
				}
			}
		}
	}
	else { // not engaged
		if (pStandingPetOrder == SPO_Follow && GetOwnerID() && !IsStunned())
			FaceTarget(GetOwner(), true);
		if (AIautocastspell_timer->Check()) {
#if MobAI_DEBUG_Spells >= 25
			cout << "Non-Engaged autocast check triggered: " << this->GetName() << endl;
#endif
			AIautocastspell_timer->Start(2500, false);
			if (!AICastSpell(this, 100, SpellType_Heal | SpellType_Buff | SpellType_Pet))
				entity_list.AICheckCloseSpells(this, 33, MobAISpellRange, SpellType_Heal | SpellType_Buff);
		}
        else if (AIscanarea_timer->Check()) {
			Mob* tmptar = entity_list.AICheckCloseArrgo(this, GetArrgoRange(), GetAssistRange());
			if (tmptar) {
				AddToHateList(tmptar);
			}
		}
		else if (AImovement_timer->Check() && !IsRooted()) {
			SetRunAnimSpeed(0);
			if (GetOwnerID()) {
				// we're a pet, do as we're told
				switch (pStandingPetOrder) {
					case SPO_Follow: {
						Mob* owner = GetOwnerOrSelf();
						float dist = Dist(owner);
						if (dist >= 10) {
							float speed = GetWalkspeed();
							if (dist >= 25)
								speed = GetRunspeed();
							CalculateNewPosition(owner->GetX(), owner->GetY(), owner->GetZ(), speed);
						}
						break;
					}
					case SPO_Sit: {
						SetAppearance(1, false);
						break;
					}
					case SPO_Guard: {
						if (!CalculateNewPosition(GetGuardX(), GetGuardY(), GetGuardZ(), GetWalkspeed())) {
							SetHeading(GetGuardHeading());
						}
						break;
					}
				}
			}
			else if (GetFollowID()) {
				Mob* follow = entity_list.GetMob(GetFollowID());
				if (!follow) SetFollowID(0);
				else {
				float dist = Dist(follow);
				if (dist >= 10) {
					float speed = GetWalkspeed();
					if (dist >= 25)
						speed = GetRunspeed();
					CalculateNewPosition(follow->GetX(), follow->GetY(), follow->GetZ(), speed);
				}
				}
			}

			else {
				// dont move till a bit after you last fought
				if (pLastFightingDelayMoving < Timer::GetCurrentTime()) {
					if (this->IsClient()) {
						// LD timer expired, drop out of world
						if (this->CastToClient()->IsLD())
							this->CastToClient()->Disconnect();
						return;
					}
					if (roambox_distance) {
						if (
							roambox_movingto_x > roambox_max_x
							|| roambox_movingto_x < roambox_min_x
							|| roambox_movingto_y > roambox_max_y
							|| roambox_movingto_y < roambox_min_y
							) {
							float movedist = roambox_distance*roambox_distance;
							float movex = movedist * ((float)rand()/RAND_MAX);
							float movey = movedist - movex;
							movex = sqrt(movex);
							movey = sqrt(movey);
//cout << "1: MoveDist: " << roambox_distance << " MoveX: " << movex << " MoveY: " << movey << " MaxX: " << roambox_max_x << " MinX: " << roambox_min_x << " MaxY: " << roambox_max_y << " MinY: " << roambox_min_y << endl;
							movex *= rand()%2 ? 1 : -1;
							movey *= rand()%2 ? 1 : -1;
							roambox_movingto_x = GetX() + movex;
							roambox_movingto_y = GetY() + movey;
//printf("Roambox: Moving to: %1.2f, %1.2f  Move: %1.2f, %1.2f\n", roambox_movingto_x, roambox_movingto_y, movex, movey);
//cout << "2: RoamBox: Moving to: " << roambox_movingto_x << ", " << roambox_movingto_y << "  Move: " << movex << ", " << movey << endl;
							if (roambox_movingto_x > roambox_max_x || roambox_movingto_x < roambox_min_x)
								roambox_movingto_x -= movex * 2;
							if (roambox_movingto_y > roambox_max_y || roambox_movingto_y < roambox_min_y)
								roambox_movingto_y -= movey * 2;
//cout << "3: RoamBox: Moving to: " << roambox_movingto_x << ", " << roambox_movingto_y << "  Move: " << movex << ", " << movey << endl;
							if (roambox_movingto_x > roambox_max_x || roambox_movingto_x < roambox_min_x)
								roambox_movingto_x = roambox_max_x;
							if (roambox_movingto_y > roambox_max_y || roambox_movingto_y < roambox_min_y)
								roambox_movingto_y = roambox_max_y;
//cout << "4: RoamBox: Moving to: " << roambox_movingto_x << ", " << roambox_movingto_y << "  Move: " << movex << ", " << movey << endl;
						}
						else if (!CalculateNewPosition(roambox_movingto_x, roambox_movingto_y, GetZ(), GetWalkspeed())) {
							roambox_movingto_x = roambox_max_x + 1; // force update
							pLastFightingDelayMoving = Timer::GetCurrentTime() + RandomTimer(roambox_delay, roambox_delay + 5000);
						}
					}
					else if (roamer) {	
							if (cur_wp_x == GetX() && cur_wp_y == GetY() && timercompleted==true)	{
							
							if (this->CastToNPC()->GetGrid()==1 && zone->GetZoneID()==68 && (cur_wp_x>3700 && cur_wp_y>950 && cur_wp_y<1200)){
								printf("Depoping Ship\n");
								this->CastToNPC()->Depop();
							}
							else if (this->CastToNPC()->GetGrid()==2 && zone->GetZoneID()==69 && (x_pos<-8900 && y_pos<-4000)){
								printf("Depoping Ship\n");
								this->CastToNPC()->Depop();
							}
							else if (this->CastToNPC()->GetGrid()==4 && zone->GetZoneID()==69 && (x_pos>7310 && y_pos>1670)){
								printf("Depoping Ship\n");
								this->CastToNPC()->Depop();
							}
							else if (this->CastToNPC()->GetGrid()==3 && zone->GetZoneID()==10 && (x_pos<-1250 && x_pos>-1500 && y_pos>350)){
								printf("Depoping Ship\n");
								this->CastToNPC()->Depop();
							}
							else{
								if (((this->CastToNPC()->GetGrid()>0 &&  this->CastToNPC()->GetGrid()<5) && this->CastToNPC()->passengers==true) || this->CastToNPC()->GetGrid()>5){
									timercompleted=false;
									char temp[100];
									parse->Event(7,this->GetNPCTypeID(), itoa(cur_wp,temp,10), this->CastToMob(), 0);
									CalculateNewWaypoint();
									SetAppearance(0, false);
								}
							}
						}
						CalculateNewPosition(cur_wp_x, cur_wp_y, cur_wp_z, GetWalkspeed());
					}
					else if (!(GetGuardX() == 0 && GetGuardY() == 0 && GetGuardZ() == 0)) {
						if (!CalculateNewPosition(GetGuardX(), GetGuardY(), GetGuardZ(), GetWalkspeed())) {
							if (!GetTarget() || (GetTarget() && CalculateDistance(GetTarget()->GetX(),GetTarget()->GetY(),GetTarget()->GetZ()) >= 5) ) SetHeading(GetGuardHeading());
							else { FaceTarget(GetTarget(), true); }
						}
					}
				}
			}
		} // else if (AImovement_timer->Check())
	}
}

// Note: Mob that caused this may not get added to the hate list until after this function call completes
void Mob::AI_Event_Engaged(Mob* attacker, bool iYellForHelp) {
	if (!IsAIControlled())
		return;
	if (iYellForHelp)
		entity_list.AIYellForHelp(this, attacker);
}

// Note: Hate list may not be actually clear until after this function call completes
void Mob::AI_Event_NoLongerEngaged() {
	if (!IsAIControlled())
		return;
	this->AIwalking_timer->Start(RandomTimer(3000,20000));
	pLastFightingDelayMoving = Timer::GetCurrentTime();
	if (minLastFightingDelayMoving == maxLastFightingDelayMoving)
		pLastFightingDelayMoving += minLastFightingDelayMoving;
	else
		pLastFightingDelayMoving += (rand() % (maxLastFightingDelayMoving-minLastFightingDelayMoving)) + minLastFightingDelayMoving;
}

void Mob::AI_Event_SpellCastFinished(bool iCastSucceeded, int8 slot) {
	if (!IsAIControlled())
		return;
	if (slot == 1) {
		if (pDontCastBefore_casting_spell) {
			*pDontCastBefore_casting_spell = 0;
			pDontCastBefore_casting_spell = 0;
		}
		int32 recovery_time = 0;
		if (iCastSucceeded) {
			if (casting_spell_AIindex < MAX_AISPELLS) {
					recovery_time += spells[AIspells[casting_spell_AIindex].spellid].recovery_time;
					if (AIspells[casting_spell_AIindex].recast_delay >= 0){
						if (AIspells[casting_spell_AIindex].recast_delay <1000)
							AIspells[casting_spell_AIindex].time_cancast = Timer::GetCurrentTime() + (AIspells[casting_spell_AIindex].recast_delay*1000);
}
					else
						AIspells[casting_spell_AIindex].time_cancast = Timer::GetCurrentTime() + spells[AIspells[casting_spell_AIindex].spellid].recast_time;
			}
			if (!IsEngaged())
				recovery_time += 2500;
			if (recovery_time < AIautocastspell_timer->GetSetAtTrigger())
				recovery_time = AIautocastspell_timer->GetSetAtTrigger();
			AIautocastspell_timer->Start(recovery_time, false);
		}
		else
			AIautocastspell_timer->Start(800, false);
		casting_spell_AIindex = MAX_AISPELLS;
	}
}

void Mob::AI_SetRoambox(float iDist, float iRoamDist, int32 iDelay) {
	AI_SetRoambox(iDist, GetX()+iRoamDist, GetX()-iRoamDist, GetY()+iRoamDist, GetY()-iRoamDist, iDelay);
}

void Mob::AI_SetRoambox(float iDist, float iMaxX, float iMinX, float iMaxY, float iMinY, int32 iDelay) {
	roambox_distance = iDist;
	roambox_max_x = iMaxX;
	roambox_min_x = iMinX;
	roambox_max_y = iMaxY;
	roambox_min_y = iMinY;
	roambox_movingto_x = roambox_max_x + 1; // this will trigger a recalc
	roambox_delay = iDelay;
}

void Mob::UpdateWaypoint(int wp_index)
{
	MyListItem <wplist> * Ptr = Waypoints.First;
	while (Ptr) {
		if ( (int32)Ptr->Data->index == wp_index) {
				cur_wp_x = Ptr->Data->x;
				cur_wp_y = Ptr->Data->y;
				cur_wp_z = Ptr->Data->z;
				cur_wp_pause = Ptr->Data->pause;
				break;
		}
		Ptr = Ptr->Next;
	}
	return;
}

void Mob::CalculateNewWaypoint()
{
//	int8 max_wp = wp_a[0];
//	int8 wandertype = wp_a[1];
//	int8 pausetype = wp_a[2];
//	int8 cur_wp = wp_a[3];

	int8 ranmax = cur_wp;
	int8 ranmax2 = max_wp - cur_wp;
	int old_wp = cur_wp;

	bool reached_end = false;
	bool reached_beginning = false;

	//Determine if we're at the last/first waypoint
	if (cur_wp == max_wp)
		reached_end = true;
	if (cur_wp == 0)
		reached_beginning = true;

	//Declare which waypoint to go to
	switch (wandertype)
	{
	case 0: //Circular
		if (reached_end)
			cur_wp = 0;
		else
			cur_wp = cur_wp + 1;
		break;
	case 1: //Random 5
		if (ranmax > 5)
			ranmax = 5;
		if (ranmax2 > 5)
			ranmax2 = 5;
		cur_wp = cur_wp + rand()%(ranmax+1) - rand()%(ranmax2+1);
		break;
	case 2: //Random
			cur_wp = (rand()%max_wp) + (rand()%2);
		break;
	case 3: //Patrol
		if (reached_end)
			patrol = 1;
		else if (reached_beginning)
			patrol = 0;
		if (patrol == 1)
			cur_wp = cur_wp - 1;
		else
			cur_wp = cur_wp + 1;
		break;
	}
	// Check to see if we need to update the waypoint. - Wes
	if (cur_wp != old_wp)
		UpdateWaypoint(cur_wp);

	//Declare time to wait on current WP
	if (cur_wp_pause == 0) {
		AIwalking_timer->Start(100);
	}
	else
	{
		
		switch (pausetype)
		{
		case 0: //Random Half
			AIwalking_timer->Start((cur_wp_pause - rand()%cur_wp_pause/2)*1000);
			break;
		case 1: //Full
			AIwalking_timer->Start(cur_wp_pause*1000);
			break;
		case 2: //Random Full
			AIwalking_timer->Start((rand()%cur_wp_pause)*1000);
			break;
		}
	}
}

float Mob::CalculateDistanceToNextWaypoint() {
    return CalculateDistance(cur_wp_x, cur_wp_y, cur_wp_z);
}

float Mob::CalculateDistance(float x, float y, float z) {
    return (float)sqrt((float)pow(x_pos-x,2)+pow(y_pos-x,2)+pow(z_pos-z,2));
}


int8 Mob::CalculateHeadingToNextWaypoint() {
    return CalculateHeadingToTarget(cur_wp_x, cur_wp_y);
}

sint8 Mob::CalculateHeadingToTarget(float in_x, float in_y) {
	float angle;

	if (in_x-x_pos > 0)
		angle = - 90 + atan((double)(in_y-y_pos) / (double)(in_x-x_pos)) * 180 / M_PI;
	else if (in_x-x_pos < 0)
		angle = + 90 + atan((double)(in_y-y_pos) / (double)(in_x-x_pos)) * 180 / M_PI;
	else // Added?
	{
		if (in_y-y_pos > 0)
			angle = 0;
		else
			angle = 180;
	}
	if (angle < 0)
		angle += 360;
	if (angle > 360)
		angle -= 360;
	return (sint8) (256*(360-angle)/360.0f);
}

bool Mob::CalculateNewPosition(float x, float y, float z, float speed) {
    float nx = this->x_pos;
    float ny = this->y_pos;
    float nz = this->z_pos;
    float vx, vy, vz;
    float vb;

    // if NPC is rooted
    if (speed == 0.0) {
        SetHeading(CalculateHeadingToTarget(x, y));
		SetRunAnimSpeed(0);
        return true;
    }

	if (zone->map) {
		// --------------------------------------------------------------------------
		// 1: get Vector AB (Vab = B-A)
		// --------------------------------------------------------------------------
		vx = x - nx;
		vy = y - ny;
		vz = z - nz;

		if (vx == 0 && vy == 0)
			return false;

		pRunAnimSpeed = (sint8)(speed*11);
		speed *= 3.6;

		// --------------------------------------------------------------------------
		// 2: get unit vector
		// --------------------------------------------------------------------------
		vb = speed / sqrt (vx*vx + vy*vy);
		heading = CalculateHeadingToTarget(x, y);

		if (vb >= 1.0) {
			SendTo(x, y, z);
		}
		else {
			// --------------------------------------------------------------------------
			// 3: destination = start plus movementvector (unitvektor*speed)
			// --------------------------------------------------------------------------
			SendTo(x_pos + vx*vb, y_pos + vy*vb, z_pos + vz*vb);
		}
	}
	else {
		// --------------------------------------------------------------------------
		// 1: get Vector AB (Vab = B-A)
		// --------------------------------------------------------------------------
		vx = x - nx;
		vy = y - ny;
		vz = z - nz;

		if (vx == 0 && vy == 0 && vz == 0)
			return false;

		pRunAnimSpeed = (sint8)(speed*11);
		speed *= 3.6;

		// --------------------------------------------------------------------------
		// 2: get unit vector
		// --------------------------------------------------------------------------
		vb = speed / sqrt (vx*vx + vy*vy + vz*vz);
		heading = CalculateHeadingToTarget(x, y);

		if (vb >= 1.0) {
			x_pos = x;
			y_pos = y;
			z_pos = z;
		}
		else {
			// --------------------------------------------------------------------------
			// 3: destination = start plus movementvector (unitvektor*speed)
			// --------------------------------------------------------------------------
			x_pos = x_pos + vx*vb;
			y_pos = y_pos + vy*vb;
			z_pos = z_pos + vz*vb;
		}
	}

    // now get new heading
	SetAppearance(0, false); // make sure they're standing
    pLastChange = Timer::GetCurrentTime();
    return true;
}

void Mob::AssignWaypoints(int16 grid)
{
	Waypoints.ClearListAndData();
#ifdef _DEBUG
	cout<<"Assigning waypoints for grid "<<grid<<" to "<<name<<"...";
#endif
	char wpstructempty[100];
	if (!database.GetWaypoints(grid, 1, wpstructempty))
		return;
	adverrorinfo = 7561;
	this->CastToNPC()->SetGrid(grid); //Assign grid number
	roamer = true; //This is a roamer
	for (int i=0; i < 52; i++) {
		adverrorinfo = 7562;
		char wpstruct[100];
		if (!database.GetWaypoints(grid, i, wpstruct))
			i=53;
		else {
			if (i < 2) {
				adverrorinfo = 7563;
				//wp_a[i+1] = atoi(wpstruct); //Assign wandering type and pause type
				((i==0) ? wandertype : pausetype) = atoi(wpstruct);
			}
			else { //Retrieve a waypoint
				wplist * newwp = new wplist;
				adverrorinfo = 7564;
				Seperator sep(wpstruct, ' ', 4);
				newwp->x	 = atof(sep.arg[0]);
				newwp->y	 = atof(sep.arg[1]);
				newwp->z	 = atof(sep.arg[2]);
				newwp->pause = atoi(sep.arg[3]);
				newwp->index = i-2;
//				printf("New Waypoint: X: %f - Y: %f - Z: %f - P: %d - Index: %d - MaxWp: %d\n", newwp->x, newwp->y, newwp->z, newwp->pause, newwp->index, max_wp);
				if (newwp->x && newwp->y && newwp->z) {
					max_wp		 = newwp->index;
					Waypoints.AddItem(newwp);
				}
			}
		}
	}
#ifdef _DEBUG
	cout<<" done."<<endl;
#endif
			adverrorinfo = 7565;
			UpdateWaypoint(0);
			this->SendTo(cur_wp_x, cur_wp_y, cur_wp_z);
	if (wandertype == 1 || wandertype == 2)
		CalculateNewWaypoint();
}

void Mob::SendTo(float new_x, float new_y, float new_z) {
	if (zone->map == 0)


		return;
	
//	float angle;
//	float dx = new_x-x_pos;
//	float dy = new_y-y_pos;
	// 0.09 is a perfect magic number for a human pnj's
//	AIwalking_timer->Start((int32) ( sqrt( dx*dx + dy*dy ) * 0.09f ) * 1000 );
	
/*	if (new_x-x_pos > 0)
		angle = - 90 + atan((double)(new_y-y_pos) / (double)(new_x-x_pos)) * 180 / M_PI;
	else {
		if (new_x-x_pos < 0)	
			angle = + 90 + atan((double)(new_y-y_pos) / (double)(new_x-x_pos)) * 180 / M_PI;
		else { // Added?
			if (new_y-y_pos > 0)
				angle = 0;
			else
				angle = 180;
		}
	}
	if (angle < 0)
		angle += 360;
	if (angle > 360	)
		angle -= 360;
	
	heading	= 256*(360-angle)/360.0f;
	SetRunAnimSpeed(5);*/
	//	SendPosUpdate();
	x_pos = new_x;
	y_pos = new_y;
	
	PNODE pnode  = zone->map->SeekNode( zone->map->GetRoot(), x_pos, y_pos );
	// Quagmire - Not sure if this is the right thing to do, but it stops the crashing
	if (pnode == 0) return;
	
	int  *iface  = zone->map->SeekFace( pnode, x_pos, y_pos );
	float tmp_z = 0;
	float best_z = 999999;
	while(*iface != -1) {
		tmp_z = zone->map->GetFaceHeight( *iface, x_pos, y_pos );
		if (abs((int)(tmp_z - new_z)) <= abs((int)(best_z - new_z)))
			best_z = tmp_z;
		iface++;
	}
	z_pos = best_z + 0.1f;
}

void Mob::StartEnrage()
{
    // dont continue if already enraged
    if (bEnraged)
        return;
    if (SpecAttackTimers[SPECATK_ENRAGE] && !SpecAttackTimers[SPECATK_ENRAGE]->Check())
        return;
    // see if NPC has possibility to enrage
    if (!SpecAttacks[SPECATK_ENRAGE])
        return;
    // check if timer exists (should be true at all times)
    if (SpecAttackTimers[SPECATK_ENRAGE])
    {
		safe_delete(SpecAttackTimers[SPECATK_ENRAGE]);
        SpecAttackTimers[SPECATK_ENRAGE] = NULL;
    }

    if (!SpecAttackTimers[SPECATK_ENRAGE])
    {
        SpecAttackTimers[SPECATK_ENRAGE] = new Timer(10000);
    }
    // start the timer. need to call IsEnraged frequently since we dont have callback timers :-/
    SpecAttackTimers[SPECATK_ENRAGE]->Start();
    bEnraged = true;
    entity_list.MessageClose(this, true, 600, 13, "%s has become ENRAGED.", GetName());
}

bool Mob::IsEnraged()
{
    // check the timer and set to false if time is up
    if (bEnraged && SpecAttackTimers[SPECATK_ENRAGE] && SpecAttackTimers[SPECATK_ENRAGE]->Check())
    {
        entity_list.MessageClose(this, true, 600, 13, "%s is no longer enraged.", GetName());
        safe_delete(SpecAttackTimers[SPECATK_ENRAGE]);
        SpecAttackTimers[SPECATK_ENRAGE] = new Timer(360000);
        SpecAttackTimers[SPECATK_ENRAGE]->Start();
        bEnraged = false;
    }
    return bEnraged;
}

bool Mob::Flurry()
{
    // perhaps get the values from the db?
    if (rand()%100 < 80)
        return false;
    // attack the most hated target, regardless of range or whatever
    Mob *target = GetHateTop();
	if (target) {
		entity_list.MessageClose(this, true, 600, 13, "%s executes a FLURRY of attacks on %s!", GetName(), target->GetName());
		for (int i = 0; i < MAX_FLURRY_HITS; i++)
			Attack(target);
	}
    return true;
}

bool Mob::AddRampage(Mob *mob)
{
    if (!SpecAttacks[SPECATK_RAMPAGE])
        return false;
    for (int i = 0; i < MAX_RAMPAGE_TARGETS; i++)
    {
        // if name is already on the list dont add it again
        if (strcasecmp(mob->GetName(), RampageArray[i]) == 0)
            return false;
        strcpy(RampageArray[i], mob->GetName());
        LogFile->write(EQEMuLog::Normal, "Adding %s to Rampage List in slot %d", RampageArray[i], i);
        return true;
    }
    return false;
}

bool Mob::Rampage()
{
    // perhaps get the values from the db?
    if (rand()%100 < 80)
        return false;

    entity_list.MessageClose(this, true, 600, 13, "%s goes on a RAMPAGE!", GetName());
    for (int i = 0; i < MAX_RAMPAGE_TARGETS; i++)
    {
        // range is important
        if (strlen(RampageArray[i]) > 0 && entity_list.GetMob(RampageArray[i]) )
        {
            Mob *target = entity_list.GetMob(RampageArray[i]);
            if (CombatRange(target))
                Attack(target);
        }
    }
    return true;
}

int32 Mob::GetLevelCon(int8 iOtherLevel) {
    sint16 diff = iOtherLevel - GetLevel();
	int32 conlevel=0;

    if (diff == 0)
        return CON_WHITE;
    else if (diff >= 1 && diff <= 2)
        return CON_YELLOW;
    else if (diff >= 3)
        return CON_RED;

    if (GetLevel() <= 6)    // i didnt notice light blue mobs before level 6
    {
        if (diff <= -4)
            conlevel = CON_GREEN;
        else
            conlevel = CON_BLUE;
    }
    else if (GetLevel() <= 9)
	{
        if (diff <= -5)
            conlevel = CON_GREEN;
        else if (diff <= -4)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
    else if (GetLevel() <= 13)
	{
        if (diff <= -6)
            conlevel = CON_GREEN;
        else if (diff <= -5)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 18)
	{
        if (diff <= -7)
            conlevel = CON_GREEN;
        else if (diff <= -6)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 24)
	{
        if (diff <= -8)
            conlevel = CON_GREEN;
        else if (diff <= -7)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 35)
	{
        if (diff <= -9)
            conlevel = CON_GREEN;
        else if (diff <= -8)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 44)
	{
        if (diff <= -12)
            conlevel = CON_GREEN;
        else if (diff <= -11)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 50)
	{
        if (diff <= -14)

            conlevel = CON_GREEN;
        else if (diff <= -12)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 60)
	{
        if (diff <= -16)
            conlevel = CON_GREEN;
        else if (diff <= -14)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() >= 61)
    {
        if (diff <= -17)
            conlevel = CON_GREEN;
        else if (diff <= -15)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
    }
	return conlevel;

}

/*
alter table npc_types drop column usedspells;
alter table npc_types add column npc_spells_id int(11) unsigned not null default 0 after merchant_id;
Create Table npc_spells (
	id int(11) unsigned not null auto_increment primary key,
	name tinytext,
	parent_list int(11) unsigned not null default 0,
	attack_proc smallint(5) not null default -1,
	proc_chance tinyint(3) not null default 3
	);
create table npc_spells_entries (
	id int(11) unsigned not null auto_increment primary key,
	npc_spells_id int(11) not null,
	spellid smallint(5) not null default 0,
	type smallint(5) unsigned not null default 0,
	minlevel tinyint(3) unsigned not null default 0,
	maxlevel tinyint(3) unsigned not null default 255,
	manacost smallint(5) not null default '-1',
	recast_delay int(11) not null default '-1',
	priority smallint(5) not null default 0,
	index npc_spells_id (npc_spells_id)
	);
*/ 

bool IsSpellInList(DBnpcspells_Struct* list, sint16 iSpellID);
void AddSpellToNPCList(Mob::AISpells_Struct* AIspells, sint16 iPriority, sint16 iSpellID, uint16 iType, sint16 iManaCost, sint32 iRecastDelay);

bool Mob::AI_AddNPCSpells(int32 iDBSpellsID) {
	// ok, this function should load the list, and the parent list then shove them into the struct and sort
	npc_spells_id = iDBSpellsID;
	memset(AIspells, 0, sizeof(AIspells));
	if (iDBSpellsID == 0) {
		AIautocastspell_timer->Disable();
		return false;
	}
	DBnpcspells_Struct* list = database.GetNPCSpells(iDBSpellsID);
	if (!list) {
		AIautocastspell_timer->Disable();
		return false;
	}
	DBnpcspells_Struct* parentlist = database.GetNPCSpells(list->parent_list);
	int i;
#if MobAI_DEBUG_Spells >= 10
	cout << "Loading NPCSpells onto " << this->GetName() << ": dbspellsid=" << iDBSpellsID;
	if (list) {
		cout << " (found, " << list->numentries << "), parentlist=" << list->parent_list;
		if (list->parent_list) {
			if (parentlist) {
				cout << " (found, " << parentlist->numentries << ")";
			}
			else
				cout << " (not found)";
		}
	}
	else
		cout << " (not found)";
	cout << endl;
#endif
	sint16 attack_proc_spell = -1;
	sint8 proc_chance = 3;
	if (parentlist) {
		attack_proc_spell = parentlist->attack_proc;
		proc_chance = parentlist->proc_chance;
		for (i=0; i<parentlist->numentries; i++) {
			if (GetLevel() >= parentlist->entries[i].minlevel && GetLevel() <= parentlist->entries[i].maxlevel && parentlist->entries[i].spellid > 0) {
				if (!IsSpellInList(list, parentlist->entries[i].spellid))
					AddSpellToNPCList(AIspells, parentlist->entries[i].priority, parentlist->entries[i].spellid, parentlist->entries[i].type, parentlist->entries[i].manacost, parentlist->entries[i].recast_delay);
			}
		}
	}
	for (i=0; i<list->numentries; i++) {
		if (list->attack_proc >= 0) {
			attack_proc_spell = list->attack_proc;
			proc_chance = list->proc_chance;
		}
		if (GetLevel() >= list->entries[i].minlevel && GetLevel() <= list->entries[i].maxlevel && list->entries[i].spellid > 0) {
			AddSpellToNPCList(AIspells, list->entries[i].priority, list->entries[i].spellid, list->entries[i].type, list->entries[i].manacost, list->entries[i].recast_delay);
		}
	}
	if (attack_proc_spell > 0)
		AddProcToWeapon(attack_proc_spell, true, proc_chance);
#if MobAI_DEBUG_Spells >= 11
	i=0;
	for (int j=0; j<MAX_AISPELLS; j++) {
		if (AIspells[j].spellid > 0) {
			cout << "NPCSpells on " << this->GetName() << ": AIspells[" << j << "].spellid=" << setw(5) << AIspells[j].spellid << ": " << spells[AIspells[j].spellid].name << endl;
			i++;
		}
	}
	cout << i << " NPCSpells on " << this->GetName() << endl;
#endif
	if (AIspells[0].spellid == 0)
		AIautocastspell_timer->Disable();
	else
		AIautocastspell_timer->Trigger();
	return true;
}

bool IsSpellInList(DBnpcspells_Struct* list, sint16 iSpellID) {
	for (int i=0; i<list->numentries; i++) {
		if (list->entries[i].spellid == iSpellID)
			return true;
	}
	return false;
}

// adds a spell to the list, taking into account priority and resorting list as needed.
void AddSpellToNPCList(Mob::AISpells_Struct* AIspells, sint16 iPriority, sint16 iSpellID, uint16 iType, sint16 iManaCost, sint32 iRecastDelay) {
	if (iSpellID <= 0 || iSpellID > SPDAT_RECORDS) {
#if MobAI_DEBUG_Spells >= 1
		cout << "AddSpellToNPCList: Spell #" << iSpellID << " not added, out of bounds" << endl;
#endif
		return;
	}
#if MobAI_DEBUG_Spells >= 12
	cout << "Adding spell #" << iSpellID;
#endif
	for (int i=0; i<MAX_AISPELLS; i++) {
		if (AIspells[i].spellid <= 0) {
			AIspells[i].spellid = iSpellID;
			AIspells[i].priority = iPriority;
			AIspells[i].type = iType;
			AIspells[i].manacost = iManaCost;
			AIspells[i].recast_delay = iRecastDelay;
#if MobAI_DEBUG_Spells >= 12
			cout << " to slot " << i;
#endif
			break;
		}
		else if (AIspells[i].priority < iPriority) {
			for (int j=MAX_AISPELLS-1; j>i; j--) {
				AIspells[j].spellid = AIspells[j-1].spellid;
				AIspells[j].priority = AIspells[j-1].priority;
				AIspells[j].type = AIspells[j-1].type;
				AIspells[j].manacost = AIspells[j-1].manacost;
				AIspells[j].recast_delay = AIspells[j-1].recast_delay;
			}
			AIspells[i].spellid = iSpellID;
			AIspells[i].priority = iPriority;
			AIspells[i].type = iType;
			AIspells[i].manacost = iManaCost;
			AIspells[i].recast_delay = iRecastDelay;
#if MobAI_DEBUG_Spells >= 12
			cout << " to slot " << i;
#endif
			break;
		}
	}
#if MobAI_DEBUG_Spells >= 12
	cout << endl;
#endif
}


DBnpcspells_Struct* Database::GetNPCSpells(int32 iDBSpellsID) {
	if (iDBSpellsID == 0)
		return 0;
	if (!npc_spells_cache) {
		npc_spells_maxid = GetMaxNPCSpellsID();
		npc_spells_cache = new DBnpcspells_Struct*[npc_spells_maxid+1];
		npc_spells_loadtried = new bool[npc_spells_maxid+1];
		for (int i=0; i<=npc_spells_maxid; i++) {
			npc_spells_cache[i] = 0;
			npc_spells_loadtried[i] = false;
		}
	}
	if (iDBSpellsID > npc_spells_maxid)
		return 0;
	if (npc_spells_cache[iDBSpellsID]) { // it's in the cache, easy =)
		return npc_spells_cache[iDBSpellsID];
	}
	else if (!npc_spells_loadtried[iDBSpellsID]) { // no reason to ask the DB again if we have failed once already
		npc_spells_loadtried[iDBSpellsID] = true;
		char errbuf[MYSQL_ERRMSG_SIZE];
		char *query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;
		
		if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, parent_list, attack_proc, proc_chance from npc_spells where id=%d", iDBSpellsID), errbuf, &result)) {
			safe_delete(query);
			if (mysql_num_rows(result) == 1) {
				row = mysql_fetch_row(result);
				int32 tmpparent_list = atoi(row[1]);
				sint16 tmpattack_proc = atoi(row[2]);
				int8 tmpproc_chance = atoi(row[3]);
				mysql_free_result(result);
				if (RunQuery(query, MakeAnyLenString(&query, "SELECT spellid, type, minlevel, maxlevel, manacost, recast_delay, priority from npc_spells_entries where npc_spells_id=%d ORDER BY minlevel", iDBSpellsID), errbuf, &result)) {
					safe_delete(query);
					int32 tmpSize = sizeof(DBnpcspells_Struct) + (sizeof(DBnpcspells_entries_Struct) * mysql_num_rows(result));
					npc_spells_cache[iDBSpellsID] = (DBnpcspells_Struct*) new uchar[tmpSize];
					memset(npc_spells_cache[iDBSpellsID], 0, tmpSize);
					npc_spells_cache[iDBSpellsID]->parent_list = tmpparent_list;
					npc_spells_cache[iDBSpellsID]->attack_proc = tmpattack_proc;
					npc_spells_cache[iDBSpellsID]->proc_chance = tmpproc_chance;
					npc_spells_cache[iDBSpellsID]->numentries = mysql_num_rows(result);
					int j = 0;
					while ((row = mysql_fetch_row(result))) {
						npc_spells_cache[iDBSpellsID]->entries[j].spellid = atoi(row[0]);
						npc_spells_cache[iDBSpellsID]->entries[j].type = atoi(row[1]);
						npc_spells_cache[iDBSpellsID]->entries[j].minlevel = atoi(row[2]);
						npc_spells_cache[iDBSpellsID]->entries[j].maxlevel = atoi(row[3]);
						npc_spells_cache[iDBSpellsID]->entries[j].manacost = atoi(row[4]);
						npc_spells_cache[iDBSpellsID]->entries[j].recast_delay = atoi(row[5]);
						npc_spells_cache[iDBSpellsID]->entries[j].priority = atoi(row[6]);
						j++;
					}
					mysql_free_result(result);
					return npc_spells_cache[iDBSpellsID];
				}
				else {
					cerr << "Error in AddNPCSpells query1 '" << query << "' " << errbuf << endl;
					safe_delete(query);
					return 0;
				}
			}
			else {
				mysql_free_result(result);
			}
		}
		else {
			cerr << "Error in AddNPCSpells query1 '" << query << "' " << errbuf << endl;
			delete[] query;
			return 0;
		}
		
		return 0;	
	}
	return 0;
}

int32 Database::GetMaxNPCSpellsID() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT max(id) from npc_spells"), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 ret = 0;
			if (row[0])
				ret = atoi(row[0]);
			mysql_free_result(result);
			return ret;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetMaxNPCSpellsID query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	
	return 0;	
}

// returns 1 if they're allies, -1 if they're enimies, 0 if they dont care either way
sint8 NPC::CheckNPCFactionAlly(sint32 other_faction) {
	LinkedListIterator<struct NPCFaction*> fac_iteratorcur(faction_list);
	fac_iteratorcur.Reset();

	while(fac_iteratorcur.MoreElements()) {
		if (fac_iteratorcur.GetData()->factionID == other_faction) {
			if (fac_iteratorcur.GetData()->value_mod < 0)
				return 1;
			else if (fac_iteratorcur.GetData()->value_mod > 0)
				return -1;
			else
				return 0;
		}

		fac_iteratorcur.Advance();
	}
	return 0;
}

