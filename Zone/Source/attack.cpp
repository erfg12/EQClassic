///////////////////////////////////////////////////
#include <logger.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include "EQCException.hpp"
#include "EQCUtils.hpp"
#include "client.h"
#include "config.h"
#include "npc.h"
#include "eq_packet_structs.h"
#include "skills.h"
#include "PlayerCorpse.h"
#include "spdat.h"
#include "zone.h"
#include "config.h"
#include "groups.h"
#include "mob.h"
#include "SpellsHandler.hpp"
#include "NpcAI.h"
#include "itemtypes.h"

#ifdef EMBPERL
	#include "embparser.h"
#endif

using namespace std;
using namespace EQC::Common;
using namespace EQC::Common::Network;;

extern Database			database;
extern EntityList		entity_list;
extern SpellsHandler	spells_handler;
extern Zone*			zone;


///////////////////////////////////////////////////
// Reorganised a little this function (Cofruben).
void Client::Attack(Mob* other, int hand, bool procEligible, bool riposte)
{
	//Yeahlight: PC is stunned
	if(stunned)
		return;

	//Yeahlight: PC is currently cloaked by an invisibility spell
	if(GetSpellInvis())
		BuffFadeByEffect(SE_Invisibility);
	if(GetInvisibleUndead())
		BuffFadeByEffect(SE_InvisVsUndead);
	if(GetInvisibleAnimal())
		BuffFadeByEffect(SE_InvisVsAnimals);

	// Some useful variables.
	Item_Struct*	attacking_weapon	= NULL;
	Client*			attacker			= this;
	Mob*			defender			= other;
	int8			skillinuse			= 0,
					attack_skill		= 0,
					defender_level		= 0,
					attacker_level		= 0;
	sint32			damage				= 0;
	int16			hitChance			= 0;

	// 1. Can we attack target? First check!
	if (!this->CanAttackTarget(defender)) return;

	// 2. Figure out attacker weapon/s, skill id's, levels and some other info.
	if(!this->CalculateAttackingSkills(hand, &skillinuse, &attack_skill, &attacking_weapon)) return;
	defender_level = (defender->GetLevel() > 0) ? defender->GetLevel() : 1;
	attacker_level = (attacker->GetLevel() > 0) ? attacker->GetLevel() : 1;

	// 3. Will we hit? Check for hit chance.
	hitChance = GetHitChance(attacker, defender, skillinuse);

	// 4. In case of hit, let's calculate the damage done to the defender.
	if (rand()%100 <= hitChance) {
		damage = this->CalculateAttackDamage(defender, attacking_weapon, skillinuse, hand, attacker_level, defender_level);
		//Yeahlight: Check for block, parry, riposte and dodge
		if(damage != 0)
			damage = defender->AvoidDamage(this, damage, riposte);
	}

	// 5. Send attack animation and damage information.
	this->DoAttackAnim(attacking_weapon, skillinuse, attack_skill, hand);  // Attacking animation
	//Melinko: Check for critical hit chance here
	damage = this->GetCriticalHit(defender, skillinuse, damage);
	defender->Damage(this, damage, 0xFFFF, attack_skill);
	//Yeahlight: Attack was riposted and the attack was not riposte itself
	if(damage == -3 && !riposte && defender)
		defender->Attack(this, 13, false, true);

	// 6. Proc checks (weapon and buff effect procs)
	if(procEligible)
	{
		//Yeahlight: Note: The redundant "defender" PTR checks below are necessary! The NPC may die between proc checks
		//Yeahlight: Weapon proc check
		if(defender)
			TryWeaponProc(attacking_weapon, defender, hand);

		//Yeahlight: Spell proc check (main hand only)
		if(defender && hand == 13 && GetBonusProcSpell())
			TryBonusProc(GetBonusProcSpell(), defender);
	}
}

//o--------------------------------------------------------------
//| GetHitChance; Yeahlight, Nov 24, 2008
//o--------------------------------------------------------------
//| Returns the chance to hit between attacker and defender
//o--------------------------------------------------------------
int16 Mob::GetHitChance(Mob* attacker, Mob* defender, int skill_num)
{
	bool debugFlag = true;

	//Yeahlight: If the target is sitting, the chance to hit them is 100%
	if(defender->GetAppearance() == SittingAppearance)
		return 999;

	sint16 ATKadjustment = (attacker->GetLevel() - defender->GetLevel()) * 3;
	sint16 hitRateAdjustment = (attacker->GetLevel() - defender->GetLevel());
	int16 hitChance = 70;
	int16 weaponSkill = 0;
	if(ATKadjustment < 0)
		ATKadjustment = 0;
	if(attacker->IsClient())
		weaponSkill = attacker->CastToClient()->GetSkill(skill_num);
	else
		weaponSkill = attacker->GetSkill(skill_num);
	int16 accuracyATK = (int)((float)weaponSkill * 2.70f) + 5 + ATKadjustment;
	if(accuracyATK == 0)
		accuracyATK = 1;
	int16 avoidanceAC = defender->GetAvoidanceAC();

	//Yeahlight: 5% bonus evasion for defensive monks
	if(defender->GetClass() == MONK || defender->GetClass() == MONKGM)
		hitChance = hitChance - 5;
	//Yeahlight: 5% bonus accuracy for offensive rogues
	if(attacker->GetClass() == ROGUE || attacker->GetClass() == ROGUEGM)
		hitChance = hitChance + 5;

	//Yeahlight: As the attacker falls further under the level of the defender, it becomes harder to land a hit
	//           Note: This will become a major tweak for class balancing! We must keep this on par with spell casting level penalties
	if(hitRateAdjustment < 0)
	{
		hitRateAdjustment = abs(hitRateAdjustment);
		if(hitRateAdjustment > 15)
			hitRateAdjustment = 15;
		float tempAdjustment = (float)hitRateAdjustment * 2.00f / 3.00f;
		hitRateAdjustment = (int)tempAdjustment;
		hitChance = hitChance - hitRateAdjustment;
	}

	//Yeahlight: Adjust hit rate based on the gap between accuracy and avoidance AC
	sint16 gapPercent = (sint16)(((float)(accuracyATK - avoidanceAC) / (float)(accuracyATK)) * 100.00f);
	sint16 gapAdjustment = ((float)gapPercent / 5.00f);
	if(gapAdjustment > 5)
		gapAdjustment = 5;
	else if(gapAdjustment < -5)
		gapAdjustment = -5;
	hitChance = hitChance + gapAdjustment;

	//Yeahlight: Debug messages
	if(debugFlag && defender->IsClient() && defender->CastToClient()->GetDebugMe())
		defender->Message(LIGHTEN_BLUE, "Debug: %s's ATK accuracy: %i; Your AC evasion: %i; Hit rate: %i%s", attacker->GetName(), accuracyATK, avoidanceAC, hitChance, "%");
	if(debugFlag && attacker->IsClient() && attacker->CastToClient()->GetDebugMe())
		attacker->Message(LIGHTEN_BLUE, "Debug: Your ATK accuracy: %i; %s's AC evasion: %i; Hit rate: %i%s", accuracyATK, defender->GetName(), avoidanceAC, hitChance, "%");
	CAST_CLIENT_DEBUG_PTR(attacker)->Log(CP_ATTACK, "Mob::GetHitChance: Your hit chance: %f", hitChance);
	CAST_CLIENT_DEBUG_PTR(defender)->Log(CP_ATTACK, "Mob::GetHitChance: %s's hit chance: %f", attacker->GetName(), hitChance);

	return hitChance;
}

///////////////////////////////////////////////////

void Client::Heal()
{
	SetMaxHP();
	APPLAYER app;
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

	entity_list.QueueCloseClients(this, &app);
	APPLAYER hp_app;
	CreateHPPacket(&hp_app);
	entity_list.QueueCloseClients(this, &hp_app, true);
	SendHPUpdate();

	cout << name << " healed via #heal" << endl; // Pyro: Why say damage?
}

///////////////////////////////////////////////////

void Client::Damage(Mob* attacker, sint32 damage, int16 spell_id, int8 attack_skill)
{
	//Yeahlight: PC is invulnerable
	if(IsInvulnerable())
	{
		damage = -5;
	}
	//Yeahlight: Damage from another entity always fades all forms of invisibility
	else
	{
		CancelAllInvisibility();
	}

	//Tazadar : things to ask the pet to help his master if he cans :) 05/31/08
	Mob* pet = GetPet();
    if(attacker && pet && attacker->GetID() != GetID())
    {
		//Yeahlight: Pet is an NPC, add the attacker to its hatelist
		if(pet->IsNPC())
		{
			if(pet->CastToNPC()->IsEngaged() == false)
				Message(WHITE,"%s tells you, 'Attacking %s, Master.'", pet->GetName(), attacker->IsNPC() ? attacker->CastToNPC()->GetProperName() : attacker->GetName());
			pet->CastToNPC()->AddToHateList(attacker);
		}
		//Yeahlight: Pet is a player
		else if(pet->IsClient())
		{
			//Yeahlight: Do nothing
		}
		//Tazadar : if the pet is sat he stands up 
		if(pet->GetPetOrder() == SPO_Sit)
		{
			pet->SetPetOrder(SPO_Follow);
		}
    }

	if (spell_id > 0 && spell_id < 0xFFFF && attacker)
	{
		entity_list.MessageClose(this, true, DEFAULT_MESSAGE_RANGE, DARK_BLUE, "%s hit %s for %d points of non-melee damage.", attacker->IsNPC() ? attacker->CastToNPC()->GetProperName() : attacker->GetName(), this->GetName(), damage);
		this->Message(DARK_BLUE, "%s hit you for %d points of non-melee damage.", attacker->IsNPC() ? attacker->CastToNPC()->GetProperName() : attacker->GetName(), damage);
	}

	//Yeahlight: Client has a magical rune on and some to all damage needs to be absorbed
	//           Note: The non-melee messages are kept above on purpose. We do not want to
	//                 broadcast messages such as "soandso hit soandso for 0 points of non-
	//                 melee damage."
	if(hasRuneOn)
	{
		sint16 actualDamage = damage;
		//Yeahlight: Iterate through all the buffs on the client
		for(int i = 0; i < 15; i++)
		{
			//Yeahlight: Buff exists and has an amount of rune on the spell
			if(buffs[i].spell && buffs[i].damageRune)
			{
				//Yeahlight: Damage exceeds the amount of rune on the spell
				if(actualDamage > buffs[i].damageRune)
				{
					actualDamage -= buffs[i].damageRune;
					BuffFadeBySlot(i, true);
				}
				//Yeahlight: Damage does not exceed the amount of rune on the spell
				else
				{
					buffs[i].damageRune -= actualDamage;
					actualDamage = -6;
				}
			}
		}
		if(actualDamage > 0)
			hasRuneOn = false;
		damage = actualDamage;
	}

	//Yeahlight: PC took damage
	if(damage > 0)
	{
		//Moraj: Break Bind Wound
		if(bindwound_target!=0&&spell_id)
			BindWound(bindwound_target,true);
		//TODO: Break sneaking
		SetHP(GetHP() - damage);
		//Yeahlight: Only skill up defense with melee attacks
		if(spell_id == 0xFFFF)
			CheckAddSkill(DEFENSE, -10);
	}

	//Yeahlight: Check for divine intervention save
	if(GetDeathSave() > 0 && GetHPRatio() <= 15)
	{
		//Yeahlight: Player passes the death save check
		if(rand()%100 + 1 <= GetDeathSave())
		{
			//Yeahlight: Fully restore the client's HP and remove the buff
			SetHP(GetMaxHP());
			//Yeahlight: TODO: This is not the correct message
			entity_list.MessageClose(this, false, DEFAULT_MESSAGE_RANGE, WHITE, "%s has been saved by divine intervention!", this->GetName());
		}
		//Yeahlight: Clear the buff icon
		BuffFadeByEffect(SE_DeathSave);
	}

	//Player has been killed, call death function.
	if(GetHP() < -9)
	{
		//Yeahlight: Lock the UI down
		SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Lose_Control, false);
		if(attacker)
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::Damage: You were killed by %s", attacker->GetName());
		Death(attacker, damage, spell_id, attack_skill);
		return;
	}

	APPLAYER app = APPLAYER(OP_Action, sizeof(Action_Struct));
	memset(app.pBuffer, 0, app.size);
	Action_Struct* a = (Action_Struct*)app.pBuffer;

	a->target = GetID();
	if(attacker == 0)
	{
		a->source = 0;
	}
	else if(attacker->IsClient() && attacker->CastToClient()->GMHideMe())
	{
		a->source = 0;
	}
	else
	{
		a->source = attacker->GetID();
		a->type = attack_skill;
		a->spell = spell_id;
		a->damage = damage;
	}

	entity_list.QueueCloseClients(this, &app);

	APPLAYER hp_app;
	CreateHPPacket(&hp_app);
	entity_list.QueueCloseClients(this, &hp_app);

	if(damage != 0)
	{
		//Yeahlight: Check for channeling and interruption
		if(this->GetCastingSpell())
		{
			int16 skill = 0;
			if(this->IsClient())
				skill = this->CastToClient()->GetSkill(CHANNELING);
			else
				skill = this->GetSkill(CHANNELING);
			int16 channelchance = (int16)(30+((float)skill/400)*100); //Taken from EQEMU 5.0
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::Damage: Interruption chance: %i", channelchance);
			if (((float)rand()/RAND_MAX)*100 > channelchance)
			{
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::Damage: Your spell were interrupted by %s", attacker->GetName());
				this->hitWhileCasting = true;
				this->InterruptSpell();
				//cout<<"Casting should be interrupted!"<<endl;
			}
			else
			{
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::Damage: Spell not interrupted", attacker->GetName());
				this->hitWhileCasting = true;
				//cout<<"Casting should NOT be interrupted!"<<endl;
			}
		}
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::Damage: You were damaged by %s for %i points of damage, %i HP left.", attacker->GetName(), damage, GetHP());
	}

	if(attack_skill != 0xFF && damage)
	{
		this->DamageShield(attacker);
	}
	else if(attack_skill == 0xFF)
	{
		if(spell_id != 0xFFFF && spell_id != 0 && attacker)
		{
			if(spells_handler.GetSpellPtr(spell_id)->GetSpellTargetType() == ST_Tap)
			{
				attacker->SetHP(GetHP() + damage);
			}
		}
	}
}

///////////////////////////////////////////////////

void Client::Death(Mob* attacker, sint32 damage, int16 spell, int8 attack_skill)
{
	//Yeahlight: Added to avoid multiple deaths from a double, triple or quad attack
	if(GetHP() == -123456789)
		return;

	SetHP(-123456789);
	entity_list.RemoveFromTargets(this);

	zonesummon_x = -3;
	zonesummon_y = -3;
	zonesummon_z = -3;

	//Gets rid of your pet
	SetPet(0);
	
	//Strips buffs off
	this->BuffFadeAll();
	//Remove memed spells
	for(int i = 0; i < 8; i++)
		pp.spell_memory[i] = 0xFFFF;

	pp.x = x_pos;
	pp.y = y_pos;
	pp.z = z_pos;
	pp.heading = heading;

	APPLAYER app = APPLAYER(OP_Death, sizeof(Death_Struct));
	memset(app.pBuffer, 0x00, app.size);
	Death_Struct* d = (Death_Struct*)app.pBuffer;
	d->attack_skill = attack_skill;
	d->spawn_id = GetID();
	d->corpseid = GetID();
	d->damage = damage;
	if(attacker == NULL)
		d->killer_id = 0;
	else
		d->killer_id = attacker->GetID();
	entity_list.QueueClients(this, &app);
	//Yeahlight: Only PCs level 6 and above lose exp
	if(GetLevel() > 5)
		ExpLost();
	else
		pendingExpLoss = 0;
	MakeCorpse();

	// Harakiri event for slaying a player
	#ifdef EMBPERL
	if (attacker->IsNPC())
			perlParser->Event(EVENT_SLAY, attacker->GetNPCTypeID(), 0, attacker->CastToNPC(), this);
	#endif
}

//Tazadar: Client can send a death request when we are dying from lava/water/dot
void Client::ProcessOP_Death(APPLAYER *app)
{
	//Yeahlight: Added to avoid multiple deaths from a double, triple or quad attack
	if(GetHP() == -123456789)
		return;

	SetHP(-123456789);
	entity_list.RemoveFromTargets(this);

	zonesummon_x = -3;
	zonesummon_y = -3;
	zonesummon_z = -3;

	//Gets rid of your pet
	SetPet(0);
	
	//Strips buffs off
	this->BuffFadeAll();
	//Remove memed spells
	for(int i = 0; i < 8; i++)
		pp.spell_memory[i] = 0xFFFF;
	pp.x = x_pos;
	pp.y = y_pos;
	pp.z = z_pos;
	pp.heading = heading;
	entity_list.QueueClients(this, app);

	//Yeahlight: Only PCs level 6 and above lose exp
	if(GetLevel() > 5)
		ExpLost();
	else
		pendingExpLoss = 0;
	MakeCorpse();
}

//Tazadar: We need that function for natural and mob death
void Client::ExpLost()
{
	//Yeahlight: You lose exp at all levels over 5
	if(GetLevel() > 5)
	{
		//Yeahlight: TODO: Check this formula
		int exploss = (int)(GetLevel() * (GetLevel() / 18.0) * 12000);
		if(exploss > 0)
		{
			if(exploss > GetEXP())
				exploss = GetEXP() - 1;
			int32 exp = GetEXP() - exploss;
			pendingExpLoss = exploss;
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::Death: %i experience lost, %i left", exploss, exp);
			SetEXP(exp);
		}
	}
	else
	{
		pendingExpLoss = 0;
	}
}

///////////////////////////////////////////////////

//Yeahlight: TODO: The logic and order of events for the following lines are CRUCIAL in stopping item duplications.
//                 We need to lock down this process and make it impossible for two of the same item to "exist" at once.
//                 This basically means NULL out the items in the PC's PP then we populate the corpse with items.
void Client::MakeCorpse()
{
	//Yeahlight: This PC does not have an entity ID; abort
	if(GetID() == 0)
		return;
	if(GetDebugMe())
		Message(LIGHTEN_BLUE, "Debug: Your EXP loss: %i", pendingExpLoss);
	entity_list.RemoveFromTargets(this);
	//Yeahlight: SAT_SendToBind > 0 creates a corpse for other PCs in the zone to interact with
	SendAppearancePacket(GetID(), SAT_SendToBind, 1, true);
	entity_list.AddCorpse(new Corpse(this, &pp, pendingExpLoss), GetID());
	SetID(0);
	GoToBind(true);
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::MakeCorpse: Corpse made at %f,%f,%f", GetX(), GetY(), GetZ());
}

///////////////////////////////////////////////////

void NPC::Attack(Mob* other, int Hand, bool procEligible, bool riposte)	 // Kaiyodo - base function has changed prototype, need to update overloaded version
{
	bool debugFlag = true;

	//Yeahlight: Return immediately if a target mob is not passed into the function
	if(!other)
		return;

	//Yeahlight: Target mob is already dead
	if((other->IsNPC() && other->GetHP() <= 0) || (other->IsClient() && other->GetHP() <= -10))
	{
		this->CastToNPC()->RemoveFromHateList(other);//Tazadar : added to avoid multikill ^^
		return;
	}

	//Yeahlight: NPC is out of melee range or they are casting
	if (fdistance(other->GetX(), other->GetY(), GetX(), GetY()) > GetMeleeReach() || this->GetCastingSpell())
	{
		//Yeahlight: Reserve the NPC's attack to fire once they are in melee range or done casting
		reserveMyAttack = true;
		return;
	}

	int skillinuse = 0;
	int max_hit_from_sitting = 0;
	int8 attack_skill = 0;
	Item_Struct* weaponInUse = NULL;
	APPLAYER app = APPLAYER(OP_Attack,sizeof(Attack_Struct));
	memset(app.pBuffer, 0, app.size);

	//Yeahlight: Pull equipment slot and available weapon data based on hand
	if(Hand == 13)
	{
		weaponInUse = GetMyMainHandWeapon();
	}
	else
	{
		weaponInUse = GetMyOffHandWeapon();
	}

	Attack_Struct* a = (Attack_Struct*)app.pBuffer;
	a->spawn_id = GetID();
	//Yeahlight: Grab the proper attack skill and animation based on available weapon's skill
	if(weaponInUse)
	{
		switch((int)weaponInUse->common.itemType)
		{
			case DB_1H_SLASHING:
				attack_skill = 0x01;
				skillinuse = _1H_SLASHING;
				a->type = 5;
				break;
			case DB_2H_SLASHING:
				attack_skill = 0x01;
				skillinuse = _2H_SLASHING;
				a->type = 3;
				break;
			case DB_PIERCING:
				attack_skill = 0x24;
				skillinuse = PIERCING;
				a->type = 2;
				break;
			case DB_1H_BLUNT:
				attack_skill = 0x00;
				skillinuse = _1H_BLUNT;
				a->type = 5;
				break;
			case DB_2H_BLUNT:
				attack_skill = 0x00;
				skillinuse = _2H_BLUNT;
				a->type = 4;
				break;
			case DB_2H_PIERCING:
				attack_skill = 0x24;
				skillinuse = PIERCING;
				a->type = 4;
				break;
			default:
				attack_skill = 0x04;
				skillinuse = HAND_TO_HAND;
				a->type = 8;
		}
	}
	else 
	{
		attack_skill = 0x04;
		skillinuse = HAND_TO_HAND;
		a->type = 8;
	}
	//Yeahlight: Dual wield animation
	if(Hand == 14)
		a->type = animDualWield;
	a->a_unknown2[5] = 0x80;
	a->a_unknown2[6] = 0x3f;
	entity_list.QueueCloseClients(this, &app);

	int damage = 0;
	float skillmodifier = 0.0;
	float hitmodifier = 0.0;
	int8 otherlevel = other->GetLevel();
	if (otherlevel == 0)
		otherlevel = 1;
	int8 mylevel = this->GetLevel();
	if (mylevel == 0)
		mylevel = 1;

	//Yeahlight: NPC has a default of one attack rounds
	int16 attackRounds = 1;

	//Yeahlight: Find the NPC's favored damage interval
	damageInterval chosenDI = GetRandomInterval(this, other);

	//Yeahlight: Find the NPC's chance to hit
	sint16 hitChance = GetHitChance(this, other, skillinuse);

	//Yeahlight: Flurry and rampage proc variables
	float procChance = 0.00;
	float hitsPerRound = 0.00;
	float dexBonus = 0.00;
	int16 flurryAndRampageRate = 0;
	int16 attackDelay = 0;
	bool hasRampaged = false;
	bool willFlurry = false;
	bool willRampage = false;

	//Yeahlight: Set appropriate flags for flurry and rampage on main hand attacks
	if((this->canFlurry || this->canRampage) && Hand == 13)
	{
		attackDelay = this->attack_timer->GetTimerTime();
		if(attackDelay < 1)
			attackDelay = 1;
		hitsPerRound = FLURRY_AND_RAMPAGE_PROC_INTERVAL / (float)attackDelay;
		if(hitsPerRound < 1)
			hitsPerRound = 1;
		if(GetDEX() > 75)
			dexBonus = (float)(GetDEX() - 75) / 1125.000f;
		procChance = (1.000f + dexBonus) / hitsPerRound;
		if(this->canFlurry && MakeRandomFloat(0.000, 1.000) < procChance)
			willFlurry = true;
		if(this->canRampage && MakeRandomFloat(0.000, 1.000) < procChance)
			willRampage = true;
	}

	//Yeahlight: Flurry NPC will flurry this round
	if(willFlurry)
		attackRounds = 2;

	if(debugFlag && other->IsClient() && other->CastToClient()->GetDebugMe())
		other->Message(LIGHTEN_BLUE, "Debug: %s's attack delay: %i", this->GetName(), this->attack_timer->GetTimerTime());

	for(int rounds = 0; rounds < attackRounds; rounds++)
	{
		//Yeahlight: Start off the attack attempts at one
		int16 potentialHits[4] = {1, 0, 0, 0};

		//Yeahlight: Calculate the maximum number of hit attempts (only main hand may quad and tripple attack)
		if(this->canQuadAttack && Hand == 13)
		{
			int16 sumChance = GetSkill(skillinuse);
			if(sumChance > 252)
				sumChance = 252;
			sumChance += mylevel;
			float BonusAttackProbability = (sumChance) / 500.0f;
			float random = (float)rand()/RAND_MAX;
			if(random < BonusAttackProbability)
				potentialHits[1] = 1;
			random = (float)rand()/RAND_MAX;
			if(random < BonusAttackProbability)
				potentialHits[2] = 1;
			random = (float)rand()/RAND_MAX;
			if(random < BonusAttackProbability)
				potentialHits[3] = 1;
		}
		else if(this->canTrippleAttack && Hand == 13)
		{
			int16 sumChance = GetSkill(skillinuse);
			if(sumChance > 252)
				sumChance = 252;
			sumChance += mylevel;
			float BonusAttackProbability = (sumChance) / 500.0f;
			float random = (float)rand()/RAND_MAX;
			if(random < BonusAttackProbability)
				potentialHits[1] = 1;
			random = (float)rand()/RAND_MAX;
			if(random < BonusAttackProbability)
				potentialHits[2] = 1;
		}
		//Yeahlight: NPC meets the requirement for a double attack
		else if(this->GetSkill(DOUBLE_ATTACK) > 0)
		{
			int16 sumChance = GetSkill(skillinuse);
			if(sumChance > 252)
				sumChance = 252;
			sumChance += mylevel;
			float BonusAttackProbability = (sumChance) / 500.0f;
			float random = (float)rand()/RAND_MAX;
			if(random < BonusAttackProbability)
				potentialHits[1] = 1;
		}

		//Yeahlight: NPC is executing a flurry of attacks
		if(rounds == 1)
			entity_list.MessageClose(this, true, DEFAULT_COMBAT_MESSAGE_RANGE, RED, "%s executes a FLURRY of attacks on %s", this->GetProperName(), other->GetName());

		//Yeahlight: Iterate through every potential hit
		for(int i = 0; i < 4; i++)
		{
			//Yeahlight: NPC can potential land this hit
			if(potentialHits[i] == 1)
			{
				damage = 0;
				//Yeahlight: NPC has at least one point in this skill
				if(GetSkill(skillinuse) != 0)
				{
					//Yeahlight: NPC hit its target
					if(rand()%100 <= hitChance)
					{
						//Yeahlight: Players sitting take maximum damage per hit
						if(other->GetAppearance() == SittingAppearance)
						{
							CAST_CLIENT_DEBUG_PTR(other)->Log(CP_ATTACK, "NPC::Attack: You're sitting, getting max damage.");
							//Yeahlight: Apply innate mitigation to sitting warriors
							if(other->GetClass() == WARRIOR)
							{
								int16 damageInterval = (this->GetMaxDmg() - this->GetMinDmg()) / 19;
								int16 damageBonus = this->GetMinDmg() - damageInterval;
								damage = this->GetMaxDmg() - damageInterval;
							}
							else
							{
								damage = this->GetMaxDmg();
							}
						}
						else
						{
							int32 max_hit = this->GetMaxDmg();
							int32 min_hit = this->GetMinDmg();
							//Yeahlight: NPC didn't have damage data in the DB; create it on the fly
							//           TODO: Query a small table or array for default damage values instead of all this overhead
							if(max_hit == 0 || min_hit == 0 || max_hit < min_hit)
							{
								int weapon_damage = 0;
								if (skillinuse == 28) // weapon is hand-to-hand
								{
									weapon_damage = abs((int)GetSkill(skillinuse)/100);
									if (weapon_damage < 1) 
									{
										weapon_damage = 1;
									}
								}
								else 
								{
									// TODO: Set NPC's weapon damage
									weapon_damage = (4*(int)mylevel)/3;
									if (weapon_damage < 1) 
									{
										weapon_damage = 1;
									}
								}
								int hit_modifier = (int)((int)mylevel - (int)otherlevel)/10;
								if(max_hit == 0)
									max_hit = (int)abs((((255 + (int)GetSkill(skillinuse) + (int)mylevel)/100) + hit_modifier)*(weapon_damage*mylevel/2));
								if(min_hit == 0)
									min_hit = (int)abs((((int)mylevel - 20)/3) + hit_modifier * mylevel/5);
							}
							damage = GetIntervalDamage(this, other, chosenDI, max_hit, min_hit);
							CAST_CLIENT_DEBUG_PTR(other)->Log(CP_ATTACK, "NPC::Attack: %s min hit: %i, max hit: %i, damage: %i.", GetName(), min_hit, max_hit, damage);
						}
					}
				}
				//Yeahlight: Rampage NPC has been flagged to rampage
				if(willRampage && !hasRampaged && damage > 0)
				{
					hasRampaged = true;
					Mob* rampageTarget = 0;
					int16 counter = 0;
					if(debugFlag)
						entity_list.MessageClose(this, true, DEFAULT_COMBAT_MESSAGE_RANGE, GREY3, "Debug: Evaluating the following entities on rampage list...");
					//Yeahlight: Iterate through the rampage list until a legit target is found
					do
					{
						rampageTarget = entity_list.GetMob(rampageList[counter]);
						counter++;
						if(rampageTarget)
						{
							if(debugFlag)
								entity_list.MessageClose(this, true, DEFAULT_COMBAT_MESSAGE_RANGE, GREY3, "            %s", rampageTarget->GetName());
							//Yeahlight: The current mob tanking (other) may not be the target of the rampage
							if(rampageTarget == other)
								rampageTarget = 0;
							//Yeahlight: Overlook FD'd targets for rampage
							else if(rampageTarget->IsClient() && rampageTarget->CastToClient()->GetFeigned())
								rampageTarget = 0;
							//Yeahlight: If the target is out of range, move on to the next in the list
							else if(fdistance(GetX(), GetY(), rampageTarget->GetX(), rampageTarget->GetY()) > 200)
								rampageTarget = 0;
							else if(debugFlag)
								entity_list.MessageClose(this, true, DEFAULT_COMBAT_MESSAGE_RANGE, GREY3, "Debug: Chosen rampage target: %s", rampageTarget->GetName());
						}
						//Yeahlight: Zone freeze debug
						if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
							EQC_FREEZE_DEBUG(__LINE__, __FILE__);
					}while(rampageTarget == 0 && counter < rampageListCounter);
					//Yeahlight: Attack target if one exists
					if(rampageTarget)
					{
						//Yeahlight: Check for block, parry, dodge
						damage = rampageTarget->AvoidDamage(this, damage, false);
						//Yeahlight: A rampage attack may not be riposted, so change it to a miss
						if(damage == -3)
							damage = 0;
						rampageTarget->Damage(this, damage, 0xffff, attack_skill);
					}
					//Yeahlight: Display this message even if there is no target to rampage upon
					entity_list.MessageClose(this, true, DEFAULT_COMBAT_MESSAGE_RANGE, RED, "%s goes on a RAMPAGE!", this->GetProperName());
				}

				//Yeahlight: Check for block, parry, riposte and dodge
				if(damage != 0)
					damage = other->AvoidDamage(this, damage, riposte);
				other->Damage(this, damage, 0xffff, attack_skill);
				//Yeahlight: Attack was riposted and the attack was not a riposte itself
				if(damage == -3 && !riposte && other)
					other->Attack(this, 13, false, true);
			}
			//Yeahlight: Check for weapon procs
			if(i == 0 && Hand == 13 && myMainHandWeapon && other)
				this->TryWeaponProc(myMainHandWeapon, other, Hand);
			else if(i == 0 && Hand == 14 && myOffHandWeapon && other)
				this->TryWeaponProc(myOffHandWeapon, other, Hand);
			//Yeahlight: Check for occasional (2%) melee stun from primary attack
			//           TODO: Research the real stun chance
			if(i == 0 && damage > 0 && rand()%50 == 33 && other)
			{
				//Yeahlight: Stun the target
				if(other->IsClient())
					other->CastToClient()->Stun(1250);
				else if(other->IsNPC())
					other->CastToNPC()->Stun(1250);
			}
		}
	}

	//Yeahlight: I pulled this out of the attack loop. Move this back in if you feel a pet can proc multiple times in the same swing.
	if (this->ownerid && target) {	// If we're a pet..
		//Yeahlight: TODO: A static 20% is not sufficient. This value needs to change based on attack speed like all other procs
		if ((float)rand()/RAND_MAX < 0.2) {
			printf("Trying to cast\n");
			printf("Trying to cast0\n");
			// No mana cost!
			if (this->typeofpet == 1) {
				printf("Trying to cast1\n");
				this->SpellFinished(spells_handler.GetSpellPtr(893), this->target->GetID());
			}
			else if (this->typeofpet == 2) {
				printf("Trying to cast2\n");
				this->SpellFinished(spells_handler.GetSpellPtr(1021), this->target->GetID());
			}
			else if (this->typeofpet == 3) {
				printf("Trying to cast3\n");
				this->SpellFinished(spells_handler.GetSpellPtr(1021), this->target->GetID());
			}
			else if (this->typeofpet == 4) {
				printf("Trying to cast4\n");
				this->SpellFinished(spells_handler.GetSpellPtr(1021), this->target->GetID());
			}
		}
	}
}

///////////////////////////////////////////////////
/* Called when NPC takes damage from another mob. */
void NPC::Damage(Mob* attacker, sint32 damage, int16 spell_id, int8 attack_skill)
{
	bool debugFlag = true;

	//Harakiri handle EVENT_ATTACK. Resets after we have not been attacked for 12 seconds
	#ifdef EMBPERL
		if(attacked_timer->Check()) 
		{		
			perlParser->Event(EVENT_ATTACK, this->GetNPCTypeID(), 0, this, attacker);
			attacked_timer->Start();
		}		
	#endif

	// Pinedepain // If the mob takes damage, he's not mesmerized anymore
	mesmerized = false;

	//Yeahlight: NPC will enrage if their HP drops below 15%, they are currently not enraged and they have not enraged yet this fight
	if(this->canEnrage && !this->IsEnraged() && this->GetHPRatio() <= 10 && !this->hasEnragedThisFight)
	{
		this->enraged = true;
		this->enrage_timer->Start(12000);
		this->hasEnragedThisFight = true;
		entity_list.MessageClose(this, true, DEFAULT_COMBAT_MESSAGE_RANGE, RED, "%s has become ENRAGED!", this->GetProperName());
	}

	//Yeahlight: A flee check is available for non-pet NPCs whom are not currently fleeing
	if(!this->IsFleeing() && !this->GetOwner() && emergencyFlee_timer->Check())
	{
		sint16 fleeRatio = 20;
		if(target != 0)
			fleeRatio = 20 - ((this->GetLevel() - this->target->GetLevel()) * 2);
		if(fleeRatio < 1)
			fleeRatio = 1;
		else if(fleeRatio > 20)
			fleeRatio = 20;

		//Yeahlight: Casting NPC is ready to attempt to save itself, is below 25 percent hp and is NOT currently casting or fleeing
		if(this->isCaster && this->GetHPRatio() <= fleeRatio && this->target != 0 && (float)((float)this->target->GetHPRatio() / (float)this->GetHPRatio()) > FLEE_RATIO && !this->isCasting)
		{
			//Yeahlight: Number of offensive spells left
			int16 numberOfSpellsLeft = (this->GetLevel() / 10 + 3) - this->mySpellCounter;
			int16 chanceToCast = 0;
			if(numberOfSpellsLeft == 0)
			{
				//Yeahlight: Grant NPCs who are out of spells a 70% chance to save their ass
				chanceToCast = 700;
			}
			else
			{
				//Yeahlight: Chance to cast (70% base chance)
				chanceToCast = (float)((float)numberOfSpellsLeft / (float)(this->GetLevel() / 10 + 3)) * 1000 + 700;
			}
			//Yeahlight: NPC may attempt to save itself
			if(rand()%1000 < chanceToCast)
			{
				int16 SpellId = NULL_SPELL;
				//Yeahlight: Heals are favored to gate 4:1 (80% chance to check for a heal over gate)
				if(rand()%5 < 4)
				{
					//Yeahlight: Check for a heal
					SpellId = myBuffs[HEAL];
				}
				//Yeahlight: Check for gate over heal
				//           TODO: NPC will not gate if they are too close to their spawn or if they have allies in the area
				else
				{
					//Yeahlight: Check for gate if above level 35 (we don't want nooby mobs gating).
					//           TODO: I only remember enchanters and wizards gating, look into this
					if(this->GetLevel() >= 35 && (this->GetClass() == ENCHANTER || this->GetClass() == WIZARD))
						SpellId = myBuffs[GATE];
					//Yeahlight: If this NPC does not have a gate spell, then check for a heal
					if(SpellId == NULL_SPELL)
						SpellId = myBuffs[HEAL];
				}
				//Yeahlight: A heal or a gate is available
				if(SpellId != NULL_SPELL)
				{
					this->CastSpell(spells_handler.GetSpellPtr(SpellId), this->GetID());
					this->mySpellCounter++;
				}
				//Yeahlight: NPC has nothing to save itself with, so flee if the NPC is not flagged to bypass flee checks
				else if(!this->isImmuneToFleeing && this->GetBodyType() != BT_Undead)
				{
					bool flee = true;
					for(int i = 0; i < zone->zoneAgroCounter; i++)
					{
						Mob* mob = entity_list.GetMob(zone->zoneAgro[i]);
						//Yeahlight: Make sure we don't compare the NPC to itself
						if(mob != 0 && mob->GetID() == this->GetID())
							continue;
						//Yeahlight: NPC is on the same pFaction as this mob
						if(mob != 0 && mob->IsNPC() && mob->CastToNPC()->GetPrimaryFactionID() == this->GetPrimaryFactionID())
						{
							//Yeahlight: These two NPCs are in "antiflee, assist range" of each other; prevent the flee
							if(fdistance(GetX(), GetY(), mob->GetX(), mob->GetY()) < NPC_ANTIFLEE_DISTANCE)
							{
								flee = false;
								break;
							}
						}
					}
					this->fleeing = flee;
				}
			}
			//Yeahlight: NPC failed the chance to save itself, so flee if the NPC is not flagged to bypass flee checks
			else if(!this->isImmuneToFleeing && this->GetBodyType() != BT_Undead)
			{
				bool flee = true;
				for(int i = 0; i < zone->zoneAgroCounter; i++)
				{
					Mob* mob = entity_list.GetMob(zone->zoneAgro[i]);
					//Yeahlight: Make sure we don't compare the NPC to itself
					if(mob != 0 && mob->GetID() == this->GetID())
						continue;
					//Yeahlight: NPC is on the same pFaction as this mob
					if(mob != 0 && mob->IsNPC() && mob->CastToNPC()->GetPrimaryFactionID() == this->GetPrimaryFactionID())
					{
						//Yeahlight: These two NPCs are in "antiflee, assist range" of each other; prevent the flee
						if(fdistance(GetX(), GetY(), mob->GetX(), mob->GetY()) < NPC_ANTIFLEE_DISTANCE)
						{
							flee = false;
							break;
						}
					}
				}
				this->fleeing = flee;
			}
		}
		//Yeahlight: NPC is most likely a melee class and is ready to flee
		else if (!this->isImmuneToFleeing && this->GetBodyType() != BT_Undead && this->GetHPRatio() <= fleeRatio && this->target != 0 && (float)((float)this->target->GetHPRatio() / (float)this->GetHPRatio()) > FLEE_RATIO && !this->isCasting)
		{
			bool flee = true;
			for(int i = 0; i < zone->zoneAgroCounter; i++)
			{
				Mob* mob = entity_list.GetMob(zone->zoneAgro[i]);
				//Yeahlight: Make sure we don't compare the NPC to itself
				if(mob != 0 && mob->GetID() == this->GetID())
					continue;
				//Yeahlight: NPC is on the same pFaction as this mob
				if(mob != 0 && mob->IsNPC() && mob->CastToNPC()->GetPrimaryFactionID() == this->GetPrimaryFactionID())
				{
					//Yeahlight: These two NPCs are in "antiflee, assist range" of each other; prevent the flee
					if(fdistance(GetX(), GetY(), mob->GetX(), mob->GetY()) < NPC_ANTIFLEE_DISTANCE)
					{
						flee = false;
						break;
					}
				}
			}
			this->fleeing = flee;
		}
	}

	//Yeahlight: NPC is damaged while fleeing and passes an occasional check
	if(this->IsFleeing() && this->isMoving() && rand()%10 == 1)
	{
		//Yeahlight: Call this function to recalculate the HP snare portion of the calculation
		this->setMoving(true);
	}

	if(spell_id > 0 && spell_id < 0xFFFF)
	{
		Spell* spell = spells_handler.GetSpellPtr(spell_id);
		if(spell)
		{
			entity_list.MessageClose(attacker, true, DEFAULT_MESSAGE_RANGE, DARK_BLUE, "%s hit %s for %d points of non-melee damage.", attacker->IsNPC() ? attacker->CastToNPC()->GetProperName() : attacker->GetName(), this->GetProperName(), damage);
			if(attacker->IsClient())
				attacker->Message(DARK_BLUE, "You hit %s for %d points of non-melee damage.", this->GetProperName(), damage);
		}
	}

	//Yeahlight: NPC has a magical rune on and some to all damage needs to be absorbed
	//           Note: The non-melee messages are kept above on purpose. We do not want to
	//                 broadcast messages such as "soandso hit soandso for 0 points of non-
	//                 melee damage."
	if(hasRuneOn)
	{
		sint16 actualDamage = damage;
		//Yeahlight: Iterate through all the buffs on the NPC
		for(int i = 0; i < 15; i++)
		{
			//Yeahlight: Buff exists and has an amount of rune on the spell
			if(buffs[i].spell && buffs[i].damageRune)
			{
				//Yeahlight: Damage exceeds the amount of rune on the spell
				if(actualDamage > buffs[i].damageRune)
				{
					actualDamage -= buffs[i].damageRune;
					BuffFadeBySlot(i, true);
				}
				//Yeahlight: Damage does not exceed the amount of rune on the spell
				else
				{
					buffs[i].damageRune -= actualDamage;
					actualDamage = -6;
				}
			}
		}
		if(actualDamage > 0)
			hasRuneOn = false;
		damage = actualDamage;
	}

	APPLAYER app = APPLAYER(OP_Action, sizeof(Action_Struct));
	memset(app.pBuffer, 0, app.size);
	Action_Struct* a = (Action_Struct*)app.pBuffer;
	a->target = GetID();
	if(attacker == 0)
		a->source = 0;
	else if(attacker->IsClient() && attacker->CastToClient()->GMHideMe())
		a->source = 0;
	else
		a->source = attacker->GetID();
	a->type = attack_skill;
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

	//Yeahlight: NPC is eligible to be pushed if their hate list has at least 7 entities and the NPC is not fleeing
	if(damage > 0 && (spell_id == 0xFFFF || spell_id == 0) && hate_list.GetHateListSize() > 6 && this->IsFleeing() == false && rand()%10 == 1)
		this->CombatPush(attacker);

	//Yeahlight: Damage was dealt and it came from a spell or a melee attack, so flag this NPC as getting hit at least once by a legitmate skill
	//           Note: We do this so that NPCs that die to pure damage shield feedback do not reward the PC with anything
	if(GetHitOnceOrMore() == false && damage && (attack_skill != 0xFF || (spell_id != 0xFFFF && spell_id != 0)))
		SetHitOnceOrMore(true);

	//Yeahlight: NPC is now dead
	if(damage >= GetHP())
	{
		SetHP(0);
		//Still count this damage towards hate list for loot rights.
		AddToHateList(attacker, damage);				
		Death(attacker, damage, spell_id, attack_skill);
		return;
	}
	else
	{
		//Yeahlight: Attack came from a PC
		if(attacker->IsClient())
		{
			//Yeahlight: Server is flagged to send PC damage packets to other clients
			if(SEND_PC_DAMAGE_PACKETS)
			{
				entity_list.QueueCloseClients(this, &app);
			}
			//Yeahlight: Server is flagged to not send PC damage packets, but we still need to send one to the attacker
			else
			{
				attacker->CastToClient()->QueuePacket(&app);
			}
		}
		//Yeahlight: Attack came from an NPC
		else
		{
			entity_list.QueueCloseClients(this, &app);
		}
		int8 inHPRatio = GetHPRatio();
		SetHP(GetHP() - damage);
		//Yeahlight: We definitely do not want to issue an HP update packet every single time the NPC takes damage; only when their HP ratio changes
		if(abs(inHPRatio - GetHPRatio()) > 0)
		{
			APPLAYER hp_app;
			CreateHPPacket(&hp_app);
			entity_list.QueueCloseClients(this, &hp_app);
		}
		//Yeahlight: Drop current pathing commitments if this NPC is not agro'ed
		if(IsEngaged() == false)
		{								
			this->onPath = false;
			this->onRoamPath = false;
		}
		AddToHateList(attacker, damage);
	}

	if(attack_skill != 0xFF && damage)
	{
		this->DamageShield(attacker);
	}
	else if(attack_skill == 0xFF)
	{
		if(spell_id != 0xFFFF && spell_id != 0)
		{
			if(spells_handler.GetSpellPtr(spell_id)->GetSpellTargetType() == ST_Tap)
			{
				attacker->SetHP(attacker->GetHP() + damage);
			}
		}
	}
}

void NPC::Death(Mob* attacker, sint32 damage, int16 spell, int8 attack_skill)
{
	//Yeahlight: This function will occasionally be called more than once if an NPC is killed
	//           by another NPC with double, tripple or quad attack, so prevent the redundant
	//           execution if this NPC has already been flagged to depop
	if(p_depop)
		return;

	//if(attacker != 0 && attacker->IsClient())
	//	attacker->CastToClient()->CheckQuests(zone->GetShortName(), "%%DEATH%%", this->GetNPCTypeID());

	//Yeahlight: NPC had a PC as a pet
	if(this->GetPet() && this->GetPet()->IsClient())
		this->GetPet()->CastToClient()->BuffFadeByEffect(SE_Charm);

	if(this->IsEngaged()) 
	{
		zone->DelAggroMob();
		//Yeahlight: Special consideration for only one agroed NPC
		if(zone->zoneAgroCounter <= 1)
		{
			//Yeahlight: Clear the list of bogus entries
			for(int i = 0; i < MAX_ZONE_AGRO; i++)
				zone->zoneAgro[i] = NULL;
			zone->zoneAgroCounter = 0;
		}
		//Yeahlight: More than one NPC agroed in the zone
		else
		{
			bool permit = false;
			//Yeahlight: Iterate through all the agroed NPCs
			for(int i = 0; i < zone->zoneAgroCounter; i++)
			{
				//Yeahlight: Found the NPC we need to remove from the list
				if(zone->zoneAgro[i] == this->GetID())
				{
					permit = true;
				}
				//Yeahlight: Replace element 'i' with the NPC in element 'i+1'
				if(permit)
				{
					//Yeahlight: If we are NOT on the last element, proceed
					if(zone->zoneAgro[i+1] != NULL)
					{
						zone->zoneAgro[i] = zone->zoneAgro[i+1];
					}
					//Yeahlight: We are on the last element, NULL it out
					else
					{
						zone->zoneAgro[i] = NULL;
					}
				}
			}
			//Yeahlight: Decrease the agro counter by one if the NPC was on the list
			if(permit)
			{
				zone->zoneAgroCounter--;
				this->checkLoS_timer->Disable();
				this->faceTarget_timer->Disable();
				this->emergencyFlee_timer->Disable();
			}
		}
	}

	SetHP(0);
	SetPet(0);

	//Yeahlight: See if this NPC was an eye of zomm
	if(myEyeOfZommMasterID)
	{
		//Yeahlight: Grab the owner from the eye of zomm ID
		Mob* owner = entity_list.GetMob(myEyeOfZommMasterID);
		//Yeahlight: Owner is in the zone and is a client
		if(owner && owner->IsClient())
		{
			owner->CastToClient()->SetMyEyeOfZomm(NULL);
		}
	}

	entity_list.RemoveFromTargets(this);
	//Tazadar: Modified Packet it was wrong in several points, in it you have last dmg + death.
	APPLAYER app = APPLAYER(OP_Death,sizeof(Death_Struct));
	memset(app.pBuffer, 0x00, app.size);
	Death_Struct* d = (Death_Struct*)app.pBuffer;
	d->attack_skill = attack_skill;
	d->spawn_id = GetID();
	if (attacker == 0)
		d->killer_id = 0;
	else
		d->killer_id = attacker->GetID();
	d->damage = damage;
	
	//Yeahlight: Everyone in the zone needs to know about an NPC's death
	entity_list.QueueClients(this, &app, false);

	bool preventCorpse = false;

	//Yeahlight: NPC is not a merchant/banker, has a hatelist established and the attacker is a client or is a pet of a client
	//           Note: We do not reward anyone with anything if the killing blow comes from a non-pet NPC or the NPC died to 100% damage shield damage
	if(GetHitOnceOrMore() && GetClass() != MERCHANT && GetClass() != BANKER && hate_list.GetTop(this->CastToMob()) != 0 && attacker && (attacker->IsClient() || (attacker->GetOwner() && attacker->GetOwner()->IsClient())))
	{
		//Yeahlight: For whatever reason, eye of zomm NPCs *always* grant the killer one experience point
		if(IsEyeOfZomm())
		{
			Client* client = NULL;
			//Yeahlight: Killing blow came from a pet, credit the owner
			if(attacker->IsNPC())
				client = attacker->GetOwner()->CastToClient();
			else
				client = attacker->CastToClient();
			//Yeahlight: Grant the rewarding client the experience
			client->AddEXP(1);
			preventCorpse = true;
		}
		else
		{
			//Yeahlight: Base exp is the level of the NPC squared times the zone's exp modifier
			//           TODO: Implement zone exp modifiers
			//           Note: 75 is the default zone exp modifier
			sint32 baseExp = GetLevel() * GetLevel() * 75;// * zone->GetZoneExpModifier();
			float expAdjustment = 1.00;
			//Yeahlight: Hate list has more than one entity, so we need to find the rewarding group or rewarding entity
			if(hate_list.GetHateListSize() > 1)
			{
				returnGroup rewardingGroup;
				rewardingGroup.damage = 0;
				rewardingGroup.group = NULL;
				//Yeahlight: Grab the rewarding group data
				rewardingGroup = hate_list.GetRewardingGroup();
				//Yeahlight: A group has been found to possibly reward
				if(rewardingGroup.group)
				{
					//Yeahlight: Now that we have established a group exists on the hate list, we need to cross reference
					//           their total damage against the damage of each individual entity on the list as well.
					//           If a single entity has more damage than the total damage of the rewardingGroup, then the
					//           single entity is granted all the rewards
					Mob* entity = hate_list.GetRewardingEntity(rewardingGroup.damage);
					//Yeahlight: A single entity exists, which means they did the most damage to the NPC overall
					if(entity)
					{
						Client* client = NULL;
						//Yeahlight: An NPC did the majority of the damage
						if(entity->IsNPC())
						{
							//Yeahlight: NPC is a pet of a client; credit the owner
							if(entity->GetOwner() && entity->GetOwner()->IsClient())
							{
								client = entity->GetOwner()->CastToClient();
								//Yeahlight: The exp reward will be adjusted (cut in half) since a pet did most of the damage
								expAdjustment = 0.50;
							}
							//Yeahlight: NPC is not a pet
							else
							{
								//Yeahlight: A non-pet NPC did most of the work, so no rewards will be granted to any party
								preventCorpse = true;
								expAdjustment = 0.00;
							}
						}	
						else if(entity->IsClient())
						{
							client = entity->CastToClient();
						}
						//Yeahlight: A client exists to reward
						if(client)
						{
							//Yeahlight: The NPC does not con green to the rewarding client
							if(GetLevelCon(client->GetLevel(), GetLevel()) != 0x02)
							{
								//Yeahlight: Grant the rewarding client the experience
								client->AddEXP(baseExp * expAdjustment);
							}
							lootrights = BuildLootRights(NULL, client);
						}
					}
					//Yeahlight: A single entity does not exist, so we need to reward a group of players
					else
					{
						rewardingGroup.group->SplitExp(GetLevel());
						lootrights = BuildLootRights(rewardingGroup.group, NULL);
					}
				}
				//Yeahlight: No groups exists on the hate list, so we need to reward a single entity
				else
				{
					Mob* entity = hate_list.GetRewardingEntity();
					if(entity)
					{
						Client* client = NULL;
						//Yeahlight: An NPC did the majority of the damage
						if(entity->IsNPC())
						{
							//Yeahlight: NPC is a pet of a client; credit the owner
							if(entity->GetOwner() && entity->GetOwner()->IsClient())
							{
								client = entity->GetOwner()->CastToClient();
								//Yeahlight: The exp reward will be adjusted (cut in half) since a pet did most of the damage
								expAdjustment = 0.50;
							}
							//Yeahlight: NPC is not a pet
							else
							{
								//Yeahlight: A non-pet NPC did most of the work, so no rewards will be granted to any party
								preventCorpse = true;
								expAdjustment = 0.00;
							}
						}	
						else if(entity->IsClient())
						{
							client = entity->CastToClient();
						}
						//Yeahlight: A client exists to reward
						if(client)
						{
							//Yeahlight: The NPC does not con green to the rewarding client
							if(GetLevelCon(client->GetLevel(), GetLevel()) != 0x02)
							{
								//Yeahlight: Grant the rewarding client the experience
								client->AddEXP(baseExp * expAdjustment);
							}
							lootrights = BuildLootRights(NULL, client);
						}
					}
				}
			}
			//Yeahlight: Hate list is size (1), so just give the attacker all the rewards
			else
			{
				Client* client = NULL;
				Mob* entity = hate_list.GetRewardingEntity();
				//Yeahlight: Quick check to see if a pet did most of the work
				if(entity && entity->IsNPC())
					expAdjustment = 0.50;
				//Yeahlight: Killing blow came from a pet, credit the owner
				if(attacker->IsNPC())
					client = attacker->GetOwner()->CastToClient();
				else
					client = attacker->CastToClient();
				//Yeahlight: We do not reward exp for green con NPCs
				if(GetLevelCon(client->GetLevel(), GetLevel()) != 0x02)
				{
					//Yeahlight: Grant the rewarding client the experience
					client->AddEXP(baseExp * expAdjustment);
				}
				lootrights = BuildLootRights(NULL, client);
			}

			//Yeahlight: Finish up with faction hits to all players involved as long as a non-pet NPC
			//           did not do the majority of the work
			if(!preventCorpse)
				hate_list.DoFactionHits(npctype_id);
		}
	}
	//Yeahlight: An NPC got the killing blow or the NPC did not have a hate list established
	else
	{
		preventCorpse = true;
	}

	if (respawn2 != 0)
		respawn2->Reset();

	//Yeahlight: NPC is not a merchant, banker or a pet and its corpse has not been prevented
	if(GetClass() != MERCHANT && GetClass() != BANKER && this->ownerid == 0 && !preventCorpse)
	{
		entity_list.AddCorpse(new Corpse(this, &itemlist, GetNPCTypeID(), &NPCTypedata), this->GetID());
		this->SetID(0);
	}

	// Harakiri death event
	#ifdef EMBPERL
	// do we have a reference ?
	if(attacker) {
		Mob *oos = attacker->GetOwnerOrSelf();
		perlParser->Event(EVENT_DEATH, this->GetNPCTypeID(),0, this, oos);
		if(oos->IsNPC())
			perlParser->Event(EVENT_NPC_SLAY, this->GetNPCTypeID(), 0, oos->CastToNPC(), this);
	}
	#endif
	p_depop = true;
}

///////////////////////////////////////////////////

void Mob::ChangeHP(Mob* changer, sint32 amount, int16 spell_id)
{
	if (IsCorpse())
	{
		return;
	}

	if (spell_id == 982) 
	{
		SetHP(-100);
		this->Death(changer, abs(amount), spell_id, 0xFF);
		return;
	}
	else if (spell_id == 13) 
	{
		SetHP(GetMaxHP());
	}
	else if (!IsInvulnerable()) 
	{
		if (amount < 0 && changer) 
		{
			Mob* pet = changer->GetPet();
			if (!pet || (pet && pet->GetID() != this->GetID())) //We can't damage our pet.
				this->Damage(changer, abs(amount), spell_id, 0xFF);
			return;
		}
		else
		{
			SetHP(GetHP() + amount);
		}
	}

	APPLAYER hp_app;
	CreateHPPacket(&hp_app);
	entity_list.QueueCloseClients(this, &hp_app, false, 600);
}

///////////////////////////////////////////////////

//o--------------------------------------------------------------
//| DoClassAttacks; Yeahlight, Dec 8, 2008
//o--------------------------------------------------------------
//| Adapted from EQEMU 7.0: Handles all the NPC special attacks
//o--------------------------------------------------------------
void NPC::DoClassAttacks(Mob *target)
{	
	bool taunt_time = taunt_timer->Check();
	bool ca_time = classattack_timer->Check(false);
	bool ka_time = knightability_timer->Check(false);

	//Yeahlight: Return immediately if none of the timers are ready
	if(!taunt_timer && !ca_time && !ka_time)
		return;

	//Yeahlight: This is very unlikely to happen, thus located after the timer checks and not at the top
	if(!target)
		return;

	//Yeahlight: For whatever reason, the pet's owner is their current target while engaged
	if(this->GetOwner() == target)
		return;

	//Yeahlight: Do not proceed with special attacks if this NPC is fleeing or occupied
	if(IsDisabled() || this->isCasting)
		return;

	if(ka_time)
	{
		int knightreuse = 1000; //lets give it a small cooldown actually.
		switch(GetClass())
		{
			case SHADOWKNIGHT: case SHADOWKNIGHTGM:
			{
				//Yeahlight: For whatever reason, NPC harmtouch spell has a very small, almost melee range on it
				if(fdistance(target->GetX(), target->GetY(), GetX(), GetY()) <= GetMeleeReach())
				{
					Spell* spell = spells_handler.GetSpellPtr(929);
					CastSpell(spell, target->GetID());
					knightreuse = HarmTouchReuseTime * 1000;
				}
				break;
			}
			case PALADIN: case PALADINGM:
			{
				if(GetHPRatio() <= 30)
				{
					Spell* spell = spells_handler.GetSpellPtr(87);
					CastSpell(spell, GetID());
					knightreuse = LayOnHandsReuseTime * 1000;
				}
				else
				{
					knightreuse = 2000; //Check again in two seconds.
				}
				break;
			}
		}
		knightability_timer->Start(knightreuse);
	}

	//Yeahlight: Target may have died from the above harm touch spell
	if(!target)
		return;
	
	//Yeahlight: Time for the NPC to use taunt
	if(taunt_time && taunting && target->IsNPC())
	{
		//Yeahlight: Ensure proper distance and LoS is present
		if(fdistance(target->GetX(), target->GetY(), GetX(), GetY()) <= GetMeleeReach() && CheckCoordLos(GetX(), GetY(), GetZ() + 2, target->GetX(), target->GetY(), target->GetZ()))
		{
			NPC* tauntee = target->CastToNPC();
			Mob* owner = this->GetOwner();
			sint32 newhate;
			sint32 tauntvalue;

			//Yeahlight: TODO: These chances to successfully taunt the target are complete bullshit, research the real chances
			float tauntchance;
			int level_difference = level - tauntee->GetLevel();
			if (level_difference <= 5)
			{
				tauntchance = 25.0;	// minimum
				tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier

				if (tauntchance > 65.0)
				{
					tauntchance = 65.0;
				}
			}
			else if (level_difference <= 10)
			{
				tauntchance = 30.0;	// minimum
				tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier

				if (tauntchance > 85.0)
				{
					tauntchance = 85.0;
				}

			}
			else if (level_difference <= 15)
			{
				tauntchance = 40.0;	// minimum
				tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier

				if (tauntchance > 90.0)
				{
					tauntchance = 90.0;
				}
			}
			else
			{
				tauntchance = 50.0;	// minimum
				tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier

				if (tauntchance > 95.0)
				{
					tauntchance = 95.0;
				}
			}

			//Yeahlight: TODO: Research the taunt formula
			if(true)//tauntchance > ((float)rand()/(float)RAND_MAX)*100.0)
			{
				//Yeahlight: Taunter is currently not on the hate list, so add them
				if(tauntee->CastToNPC()->GetNPCHate(this) == 0)
					tauntee->CastToNPC()->AddToHateList(this, 0, 1);

				//Yeahlight: No adjustments are required if the size of the hatelist is (1)
				if(tauntee->CastToNPC()->GetHateSize() <= 1)
					return;

				//Yeahlight: Calculate the required hate adjustment necessary to put the taunter on top of the hate list
				//           Note: If an NPC hits taunt while they have a 300 point hate lead on the next highest entity,
				//                 they will actually lose 299 hate points.
				if(tauntee->CastToNPC()->GetHateTop() == this)
					newhate = tauntee->CastToNPC()->GetNPCHate(tauntee->CastToNPC()->GetHateSecondFromTop()) - tauntee->CastToNPC()->GetNPCHate(this) + 1;
				else
					newhate = tauntee->CastToNPC()->GetNPCHate(tauntee->CastToNPC()->GetHateTop()) - tauntee->CastToNPC()->GetNPCHate(this) + 1;
				tauntee->CastToNPC()->AddToHateList(this, 0, newhate);
			}
			if(owner && owner->IsClient())
			{
				owner->Message(WHITE, "%s tells you, 'Taunting attacker, Master.'", GetProperName());
				if(owner->CastToClient()->GetDebugMe())
					owner->Message(LIGHTEN_BLUE, "Debug: Your hate level: %i. Your pet's hate level: %i.", tauntee->CastToNPC()->GetNPCHate(owner), tauntee->CastToNPC()->GetNPCHate(this));
			}
		}
	}
	
	if(!ca_time)
		return;
	
	//Yeahlight: They are implying haste affects special attacks. I do not agree with this
	//float HasteModifier = 0;
	//if(GetHaste() > 0)
	//	HasteModifier = 10000 / (100 + GetHaste());
	//else if(GetHaste() < 0)
	//	HasteModifier = (100 - GetHaste());
	//else
	//	HasteModifier = 100;

	int level = GetLevel();
	int reuse = TauntReuseTime * 1000;	//make this very long since if they dont use it once, they prolly never will
	bool did_attack = false;
	//class specific stuff...
	switch(GetClass())
	{
		case WARRIOR: case WARRIORGM:
		{
			if(level >= 6){ //newage: level 6 minimum
			//Yeahlight: NPC has an 75% chance to kick and a 25% chance to bash
			//           Note: NPC always kicks if they cannot bash
			if(rand()%100 >= 25 || GetSkill(BASH) == 0)
			{
				//Yeahlight: Ensure proper distance and LoS is present
				if(fdistance(target->GetX(), target->GetY(), GetX(), GetY()) <= GetMeleeReach() && CheckCoordLos(GetX(), GetY(), GetZ() + 2, target->GetX(), target->GetY(), target->GetZ()))
				{
					DoAnim(animKick);
					sint16 hitChance = GetHitChance(this, target, KICK);
					sint32 dmg = 0;
					if(rand()%100 <= hitChance)
						dmg = GetKickDamage();
					DoSpecialAttackDamage(target, KICK, dmg);
					did_attack = true;
					//Yeahlight: Occasionally stun the target if this warrior NPC is 55+
					if(dmg > 0 && GetLevel() >= 55 && rand()%100 < 20)
					{
						//Yeahlight: Stun the target
						if(target->IsClient())
							target->CastToClient()->Stun(1000, this, true);
						else if(target->IsNPC())
							target->CastToNPC()->Stun(1000, this, true);
					}
				}
				reuse = KickReuseTime * 1000;
			}
			else
			{
				//Yeahlight: Ensure proper distance and LoS is present
				if(fdistance(target->GetX(), target->GetY(), GetX(), GetY()) <= GetMeleeReach() && CheckCoordLos(GetX(), GetY(), GetZ() + 2, target->GetX(), target->GetY(), target->GetZ()))
				{
					DoAnim(animTailRake);
					sint16 hitChance = GetHitChance(this, target, BASH);
					sint32 dmg = 0;
					if(rand()%100 <= hitChance)
						dmg = GetBashDamage();
					DoSpecialAttackDamage(target, BASH, dmg);
					did_attack = true;
					//Yeahlight: Stun the target
					if(dmg > 0 && target->IsClient())
						target->CastToClient()->Stun(2000, this, true);
					else if(dmg > 0 && target->IsNPC())
						target->CastToNPC()->Stun(2000, this, true);
				}
				//Yeahlight: No, using KickReuseTime is not a mistake here
				reuse = KickReuseTime * 1000;
			}
			}
			break;
		}
		case ROGUE: case ROGUEGM:
		{
			if(level >= 10)
			{
				//Yeahlight: NPC is behind target, proceed with backstab
				if(BehindMob(target, GetX(), GetY())) // Player is behind target
				{
					if(fdistance(GetX(), GetY(), target->GetX(), target->GetY()) <= GetMeleeReach())
					{
						RogueBackstab(target, 0, GetSkill(BACKSTAB));
					}
				}
				//Yeahlight: NPC is infront of target, use a standard attack
				else
				{
					if(fdistance(GetX(), GetY(), target->GetX(), target->GetY()) <= GetMeleeReach())
					{
						//Yeahlight: Proc rates are balanced on attack timer, not the backstab refresh timer; prevent procs from this attack
						Attack(target, 13, false);
					}
					
				}
				reuse = BackstabReuseTime * 1000;
				did_attack = true;
			}
			break;
		}
		case MONK: case MONKGM:
		{
			int8 satype = KICK;
			if(level > 29) {
				satype = FLYING_KICK;
			} else if(level > 24) {
				satype = DRAGON_PUNCH;
			} else if(level > 19) {
				satype = EAGLE_STRIKE;
			} else if(level > 9) {
				satype = TIGER_CLAW;
			} else if(level > 4) {
				satype = ROUND_KICK;
			}
			reuse = MonkSpecialAttack(target, satype);
			reuse *= 1000;
			did_attack = true;
			break;
		}
		case RANGER: case RANGERGM:
		case BEASTLORD: case BEASTLORDGM:
		{
			//Yeahlight: Ensure proper distance and LoS is present
			if(fdistance(target->GetX(), target->GetY(), GetX(), GetY()) <= GetMeleeReach() && CheckCoordLos(GetX(), GetY(), GetZ() + 2, target->GetX(), target->GetY(), target->GetZ()))
			{
				DoAnim(animKick);
				sint16 hitChance = GetHitChance(this, target, KICK);
				sint32 dmg = 0;
				if(rand()%100 <= hitChance)
					dmg = GetKickDamage();
				DoSpecialAttackDamage(target, KICK, dmg);
				did_attack = true;
			}
			reuse = KickReuseTime * 1000;
			break;
		}
		case CLERIC: case CLERICGM: //clerics can bash too.
		case SHADOWKNIGHT: case SHADOWKNIGHTGM:
		case PALADIN: case PALADINGM:
		{
			//Yeahlight: Ensure proper distance and LoS is present
			if(GetSkill(BASH) > 0 && fdistance(target->GetX(), target->GetY(), GetX(), GetY()) <= GetMeleeReach() && CheckCoordLos(GetX(), GetY(), GetZ() + 2, target->GetX(), target->GetY(), target->GetZ()))
			{
				DoAnim(animTailRake);
				sint16 hitChance = GetHitChance(this, target, BASH);
				sint32 dmg = 0;
				if(rand()%100 <= hitChance)
					dmg = GetBashDamage();
				DoSpecialAttackDamage(target, BASH, dmg);
				did_attack = true;
				//Yeahlight: Stun the target
				if(dmg > 0 && target->IsClient())
					target->CastToClient()->Stun(2000, this, true);
				else if(dmg > 0 && target->IsNPC())
					target->CastToNPC()->Stun(2000, this, true);
			}
			reuse = BashReuseTime * 1000;
			break;
		}
	}

	//if(did_attack)
	//{
	//	if(!combat_event) {
	//		mlog(COMBAT__HITS, "Triggering EVENT_COMBAT due to attack on %s", target->GetName());
	//		parse->Event(EVENT_COMBAT, this->GetNPCTypeID(), "1", this, target);
	//		combat_event = true;
	//	}
	//	combat_event_timer.Start(CombatEventTimer_expire);
	//}
	
	classattack_timer->Start(reuse/**HasteModifier/100*/);
}

///////////////////////////////////////////////////

int Mob::GetKickDamage()
{
	int multiple = (GetLevel()*100/5);
	multiple += 100;
	int16 kickSkill = 0;
	if(this->IsClient())
		kickSkill = this->CastToClient()->GetSkill(KICK);
	else
		kickSkill = this->GetSkill(KICK);
	int dmg = (((kickSkill + GetSTR() + GetLevel())*100 / 9000) * multiple) + 100;
	if(GetClass() == WARRIOR || GetClass() == WARRIORGM)
		dmg *= 12/10;//small increase for warriors
	dmg /= 100;
	return(dmg);
}

///////////////////////////////////////////////////

int Mob::GetBashDamage()
{
	int multiple = (GetLevel()*100/5);
	multiple += 100;
	int16 bashSkill = 0;
	if(this->IsClient())
		bashSkill = this->CastToClient()->GetSkill(BASH);
	else
		bashSkill = this->GetSkill(BASH);
	int dmg = ((((bashSkill + GetSTR())*100 + GetLevel()*100/2) / 10000) * multiple) + 100;
	dmg /= 100;		  
	return(dmg);
}

///////////////////////////////////////////////////

void Mob::DoSpecialAttackDamage(Mob *who, int16 skill, sint32 max_damage, sint32 min_damage) 
{
	if(!who)
		return;
	
	sint32 hate = max_damage;
	if(max_damage > 0)
	{
		who->AvoidDamage(this, max_damage, false);
		if(who->IsNPC())
		{
			if(max_damage != 0)
			{
				who->CastToNPC()->AddToHateList(this, hate);
			}
			else
			{
				who->CastToNPC()->AddToHateList(this, 0);
			}
		}
	}
	who->Damage(this, max_damage, SPELL_UNKNOWN, skill);
	if(max_damage == -3 && who)
		who->Attack(this, 13, false, true);
}

///////////////////////////////////////////////////

int16 Mob::MonkSpecialAttack(Mob* other, int8 type)
{
	if(!other)
		return 5;

	//Yeahlight: First, ensure LoS is present
	if(!CheckCoordLos(GetX(), GetY(), GetZ() + 2, other->GetX(), other->GetY(), other->GetZ()))
		return 5;

	//Yeahlight: Range check for NPCs
	//			 TODO: range check for PCs
	if(this->IsNPC() && this->CastToNPC()->fdistance(other->GetX(), other->GetY(), GetX(), GetY()) > this->GetMeleeReach())
		return 5;

	sint32 ndamage = 0;
	int16 reuse = 0;
	int16 skillValue = 0;

	if(this->IsClient())
		skillValue = this->CastToClient()->GetSkill(type);
	else
		skillValue = this->GetSkill(type);

	int8 tmplevel = other->GetLevel();
	float hitsuccess = (float)tmplevel - (float)level;
	float hitmodifier = 0.0f;
	float skillmodifier = 0.0f;
	if(level > tmplevel)
	{
		hitsuccess += 2.0f;
		hitsuccess *= 14.0f;
	}
	if (hitsuccess >= 40.0f)
	{
		hitsuccess *= 3.0f;
		hitmodifier = 1.0f;
	}
	if (hitsuccess >= 10.0f && hitsuccess <= 39.0f)
	{
		hitsuccess /= 4.0f;
		hitmodifier = 0.25f;
	}
	else if (hitsuccess < 10.0f && hitsuccess > -1.0f)
	{
		hitsuccess = 0.5f;
		hitmodifier = 1.5f;
	}
	else if (hitsuccess <= -1.0f)
	{
		hitsuccess = 0.1f;
		hitmodifier = 1.8f;
	}
	// cout << "2 - " << hitsuccess << endl;
	if (skillValue >= 100)
	{
		skillmodifier = 1.0f;
	}
	else if (skillValue >= 200)
	{
		skillmodifier = 2.0f;
	}

	hitsuccess -= ((float)skillValue/10000) + skillmodifier;
	// cout << "3 - " << hitsuccess << endl;
	hitsuccess += (float)rand()/RAND_MAX;
	// cout << "4 - " << hitsuccess << endl;
	float ackwardtest = 2.4f;
	float random = (float)rand()/RAND_MAX;
	if(random <= 0.2f)
		ackwardtest = 4.5f;
	else if(random > 85.0f && random < 400.0f)
		ackwardtest = 3.2f;
	else if(random > 400.0f && random < 800.0f)
		ackwardtest = 3.7f;
	else if(random > 900.0f && random < 1400.0)
		ackwardtest = 1.9f;
	else if(random > 1400.0f && random < 14000.0f)
		ackwardtest = 2.3f;
	else if(random > 14000.0f && random < 24000.0f)
		ackwardtest = 1.3f;
	else if(random > 24000.0f && random < 34000.0f)
		ackwardtest = 1.3f;
	else if(random > 990000.0f)
		ackwardtest = 1.2f;
	else if(random < 0.2f)
		ackwardtest = 0.8f;

	ackwardtest += (float)rand()/RAND_MAX;
	ackwardtest = (ackwardtest > 0) ? ackwardtest : -1.0f * ackwardtest;
	if (type == 0x1A) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (4 * ackwardtest) * (FLYING_KICK + GetSTR() + level) / 700);
		//Yeahlight: TODO: Thunderous Kicks are a /discipline, not a passive ability...
		//if ((float)rand()/RAND_MAX < 0.2) {
		//	ndamage = (sint32) (ndamage * 4.2f);
		//	if(ndamage <= 0) {
		//		entity_list.MessageClose(this, false, 200, WHITE, "%s misses at an attempt to thunderous kick %s!",name,other->name);
		//	}
		//	else {
		//		entity_list.MessageClose(this, false, 200, WHITE, "%s lands a thunderous kick!(%d)", name, ndamage);
		//	}
		//}
		other->Damage(this, ndamage, 0xffff, 0x1A);
		DoAnim(45);
		reuse = FlyingKickReuseTime;
	}
	else if (type == 0x34) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (2 * ackwardtest) * (TIGER_CLAW + GetSTR() + level) / 900);
		other->Damage(this, ndamage, 0xffff, 0x34);
		DoAnim(46);
		reuse = TigerClawReuseTime;
	}
	else if (type == 0x26) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (3 * ackwardtest) * (ROUND_KICK + GetSTR() + level) / 800);
		other->Damage(this, ndamage, 0xffff, 0x26);
		DoAnim(11);
		reuse = RoundKickReuseTime;
	}
	else if (type == 0x17) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (4 * ackwardtest) * (EAGLE_STRIKE + GetSTR() + level) / 1000);
		other->Damage(this, ndamage, 0xffff, 0x17);
		DoAnim(47);
		reuse = EagleStrikeReuseTime;
	}
	else if (type == 0x15) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (5 * ackwardtest) * (DRAGON_PUNCH + GetSTR() + level) / 800);
		other->Damage(this, ndamage, 0xffff, 0x15);
		DoAnim(7);
		reuse = TailRakeReuseTime;
	}
	else if (type == 0x1E) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (3 * ackwardtest) * (KICK + GetSTR() + level) / 1200);
		other->Damage(this, ndamage, 0xffff, 0x1e);
		DoAnim(1);
		reuse = KickReuseTime;
	}

	return reuse;
}

///////////////////////////////////////////////////

void NPC::AddToHateList(Mob* other, sint32 damage, sint32 hate) 
{
	bool debugFlag = true;
	int16 requiresAdding = false;
	bool wasengaged = IsEngaged();
	CAST_CLIENT_DEBUG_PTR(other)->Log(CP_ATTACK, "NPC::AddToHateList: adding %i damage, %i hate to %s", damage, hate, GetName());
	
	//Yeahlight: Flag this NPC to be considered at the end of the function
	if(!this->IsEngaged() && zone->zoneAgroCounter < 500)
	{
		requiresAdding = true;
	}

	if(other) 
	{
		//Yeahlight: Add "other" to rampage list if necessary
		if(this->canRampage && rampageListCounter < RAMPAGE_LIST_SIZE && !hate_list.IsOnHateList(other))
		{
			this->rampageList[rampageListCounter] = other->GetID();
			rampageListCounter++;
		}

		//Yeahlight: Never pass negative damage numbers to the hate list
		if(damage < 0)
			damage = 0;

		if(hate == 0xFFFFFFFF) 
		{
			Mob* owner = other->GetOwner();
			if(owner) 
			{
				hate_list.Add(other, damage, (damage > 0 ? damage : 1));
				hate_list.Add(owner, 0, damage / 2);
			}
			else 
			{
				hate_list.Add(other, damage, (damage > 0 ? damage : 1));
			}
			Mob* mypet = this->GetPet();
			if(mypet && mypet->IsNPC())
				mypet->CastToNPC()->AddToHateList(other, 0, (damage > 0 ? damage : 1));
		}
		else 
		{
			hate_list.Add(other, damage, hate);
		}
		if(debugFlag && other->IsClient() && other->CastToClient()->GetDebugMe())
			other->Message(LIGHTEN_BLUE, "Debug: %i hate points on %s", this->GetNPCHate(other), this->GetName());
	}
	else
	{
		return;
	}

	//Yeahlight: NPC is not on the list but is now fully agroed; add them to the agro list
	if(requiresAdding && this->IsEngaged())
	{
		zone->zoneAgro[zone->zoneAgroCounter] = this->GetID();
		zone->zoneAgroCounter++;
		checkLoS_timer->Trigger();
		faceTarget_timer->Trigger();
		emergencyFlee_timer->Trigger();
		this->onRoamPath = false;
		//Yeahlight: Turn moving off to refresh the animation velocity on path resolution
		this->setMoving(false);
	}

	// Harakiri quest event
	#ifdef EMBPERL
		if (!wasengaged) { 
			if(IsNPC() && other->IsClient() && other->CastToClient())
				perlParser->Event(EVENT_AGGRO, this->GetNPCTypeID(), 0, CastToNPC(), other); 

			perlParser->Event(EVENT_COMBAT, CastToNPC()->GetNPCTypeID(), "1", CastToNPC(), GetTarget());
		}
	#endif
}

///////////////////////////////////////////////////

//o--------------------------------------------------------------
//| DamageShield; Yeahlight, June 26, 2009
//o--------------------------------------------------------------
//| Handles the DS calculations and messages
//| TODO: Find the correct channel for damage shield messages
//o--------------------------------------------------------------
void Mob::DamageShield(Mob* other)
{
	//Yeahlight: Other mob was not supplied; abort
	if(other == NULL)
		return;

	int32 damageShield = this->GetDamageShield();
	int32 reverseDamageShield = this->GetReverseDamageShield();
	int8 damageShieldType = this->GetDamageShieldType();

	//Yeahlight: This mob has a reverse damage shield on, which overwrites normal damage shields
	if(reverseDamageShield)
	{
		if(other->IsClient())
			other->Message(BLUE, "You are healed for %i points of damage!", reverseDamageShield);
		other->SetHP(other->GetHP() + reverseDamageShield);
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Mob::DamageShield: Reverse DS of %i points of healing.", reverseDamageShield);
	}
	//Yeahlight: This mob has a normal damage shield on
	else if(damageShield)
	{
		//Yeahlight: Send the PC victim the correct message
		if(other->IsClient())
		{
			switch(damageShieldType)
			{
				case 1:
					other->Message(DARK_BLUE, "YOU are pierced by thorns!");
					break;
				case 2:
					other->Message(DARK_BLUE, "YOU are burned!");
					break;
				case 3:
					other->Message(DARK_BLUE, "YOU are tormented!");
					break;
			}
		}
		//Yeahlight: Send everyone else the damage message
		switch(damageShieldType)
		{
			case 1:
				entity_list.MessageClose(other, true, DEFAULT_MESSAGE_RANGE, DARK_BLUE, "%s is pierced by thorns!", other->IsNPC() ? other->CastToNPC()->GetProperName() : other->GetName());
				break;
			case 2:
				entity_list.MessageClose(other, true, DEFAULT_MESSAGE_RANGE, DARK_BLUE, "%s is burned!", other->IsNPC() ? other->CastToNPC()->GetProperName() : other->GetName());
				break;
			case 3:
				entity_list.MessageClose(other, true, DEFAULT_MESSAGE_RANGE, DARK_BLUE, "%s is tormented!", other->IsNPC() ? other->CastToNPC()->GetProperName() : other->GetName());
				break;
		}
		other->Damage(this, damageShield, 0, 0xFF);
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Mob::DamageShield: DS of %i points of damage.", damageShield);
	}
}

///////////////////////////////////////////////////
//heko: backstab
void Mob::RogueBackstab(Mob* other, Item_Struct *weapon1, int8 bs_skill)
{
	if(!other)
		return;

	//Yeahlight: Check for LoS
	if(!CheckCoordLos(GetX(), GetY(), GetZ() + 2, other->GetX(), other->GetY(), other->GetZ()))
		return;

	sint32 ndamage = 0;
	sint32 max_hit, min_hit;

	float skillmodifier = 0.0;
	Item_Struct* primaryweapon = weapon1;
	int16 weaponDamage = 0;
	int damage = 0;

	//Yeahlight: Only use weapon's damage if it is a piercing weapon
	if(primaryweapon && primaryweapon->common.itemType == ItemTypePierce)
		weaponDamage = primaryweapon->common.damage;
	//Yeahlight: NPCs do not need a weapon to backstab
	//           TODO: (Level / 5) was a guess, research this
	else if(this->IsNPC())
		weaponDamage = this->GetLevel() / 5;
	//Yeahlight: PCs are not allowed to backstab w/out a piercing weapon
	else if(this->IsClient())
		return;

	skillmodifier = (float)((float)bs_skill/25.0);	//formula's from www.thesafehouse.org

	// formula is (weapon damage * 2) + 1 + (level - 25)/3 + (strength+skill)/100
	max_hit = (sint32)(((float)weaponDamage * 2.0) + 1.0 + ((level - 25)/3.0) + ((GetSTR()+GetSkill(0x08))/100));
	max_hit *= (sint32)skillmodifier;

	// determine minimum hits
	if (level < 51)
	{
		min_hit = 0;
	}
	else
	{
		switch (level)
		{
		case 51:
			min_hit = (sint32) ((float)level * 1.5);
			break;
		case 52:
			min_hit = (sint32) ((float)level * 1.55);
			break;
		case 53:
			min_hit = (sint32) ((float)level * 1.6);
			break;
		case 54:
			min_hit = (sint32) ((float)level * 1.65);
			break;
		case 55:
			min_hit = (sint32) ((float)level * 1.7);
			break;
		case 56:
			min_hit = (sint32) ((float)level * 1.75);
			break;
		case 57:
			min_hit = (sint32) ((float)level * 1.80);
			break;
		case 58:
			min_hit = (sint32) ((float)level * 1.85);
			break;
		case 59:
			min_hit = (sint32) ((float)level * 1.9);
			break;
		case 60:
			min_hit = (sint32) ((float)level * 2.0);
			break;
		default:
			min_hit = (sint32) ((float)level * 2.0);
			break;
		}
	}
	
	//Yeahlight: Find the PC's favored damage interval
	damageInterval chosenDI = GetRandomInterval(this, other);

	ndamage = GetIntervalDamage(this, other, chosenDI, max_hit, min_hit);
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Mob::RogueBackstab: Backstab min damage: %i, max damage: %i, final damage: %i", min_hit, max_hit, ndamage);
	if (this->IsNPC())CAST_CLIENT_DEBUG_PTR(other)->Log(CP_ATTACK, "Mob::RogueBackstab: %'s backstab min damage: %i, max damage: %i, final damage: %i", GetName(), min_hit, max_hit, ndamage);
	other->Damage(this, ndamage, 0xffff, 0x08);	//0x08 is backstab
	DoAnim(2);	//piercing animation
}

///////////////////////////////////////////////////

bool Client::CanAttackTarget(Mob* target) {
	if(target == NULL){ // No target?
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CanAttackTarget: Can't attack, no target");
		this->SetTarget(NULL);
		return false;
	}
	if(this == target){ // Self attack.
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CanAttackTarget: Can't attack, self target");
		return false;
	}
	if(target->IsCorpse()){ // Corpse attack.
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CanAttackTarget: Can't attack, corpse target");
		return false;
	}
	if(target->IsClient() == false && this->GetPet()
		&& target->GetID() == this->GetPet()->GetID()){ // Jester: own pet attack.
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CanAttackTarget: Can't attack, own pet target");
		return false;
	}
	if(target->IsClient() && target->GetPet()
		&& this->GetID() == target->GetPet()->GetID()){ // Kibanu: Pet cannot attack it's owner.
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CanAttackTarget: Can't attack, target is pet's master");
		return false;
	}
	if(target->IsClient() && target->GetID() != this->GetDuelTarget()){ // Player vs Player without dueling.
		//Yeahlight: PC is not currently charmed by an NPC
		if(!(GetOwner() && GetOwner()->IsNPC()))
		{
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CanAttackTarget: Can't attack, non-dueling player target");
			return false;
		}
	}
	//Yeahlight: Anti-ship attack
	if(target->GetBaseRace() == SHIP || target->GetBaseRace() == LAUNCH)
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CanAttackTarget: Can't attack, target is a ship");
		return false;
	}
	//Yeahlight: No more attacking while channeling a spell
	//Pinedepain: Bard can still attack while channeling :)
	if(this->GetCastingSpell() && this->IsBard() == false)
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CanAttackTarget: Can't attack, you're casting a spell");
		return false;
	}
	//Yeahlight: PC cannot see its target
	if(target->VisibleToMob(this) == false)
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CanAttackTarget: Can't attack, you cannot see your target");
		return false;
	}

	return true;
}

///////////////////////////////////////////////////

bool Client::CalculateAttackingSkills(int hand, int8* skill_in_use, int8* attack_skill, Item_Struct** attacking_weapon) {
	/* Beacuse the item skill types in our database are different then our client
	 * we have to to mathe DB skills to our actual skills.  Only the first 5 are wrong. */
	if (!skill_in_use || !attack_skill) return false;
	*attacking_weapon = (hand == 13) ? (this->weapon1) : (this->weapon2);
	*skill_in_use = (*attacking_weapon) ? (*attacking_weapon)->common.itemType : (DB_2H_BLUNT+15);
	switch(*skill_in_use){
		case DB_1H_SLASHING:	//=0
			{	
				*skill_in_use = _1H_SLASHING;
				*attack_skill = 0x01;
				break;
			}
		case DB_2H_SLASHING:	//=1
			{	
				*skill_in_use = _2H_SLASHING;
				*attack_skill = 0x01;
				break;
			}
		case DB_PIERCING:	//=2
			{	
				*skill_in_use = PIERCING;
				*attack_skill = 0x24;
				break;
			}
		case DB_1H_BLUNT:	//=3
			{	
				*skill_in_use = _1H_BLUNT;
				*attack_skill = 0x00;
				break;
			}
		case DB_2H_BLUNT:	//=4
			{	
				*skill_in_use = _1H_BLUNT;
				*attack_skill = 0x00;
				break;
			}
		case DB_2H_PIERCING:	//=35
			{	
				*skill_in_use = PIERCING;
				*attack_skill = 0x24;
				break;
			}
		default:
			{
				*skill_in_use = HAND_TO_HAND;
				*attack_skill = 0x04;
				break;
			}
	}
	// Skill level and leveling check.
	if (this->GetPlayerProfilePtr()->skills[*skill_in_use] == 0){
		this->SetSkill(*skill_in_use, 1);
	}
	this->CheckAddSkill(*skill_in_use, -10);
	this->CheckAddSkill(OFFENSE, -10);
	return true;
}

sint32 Client::GetCriticalHit(Mob* defender, int8 skill, sint32 damage){
	if(damage < 1) return damage;
	float critChance = 0.0f;
	//Melinko: Handle critical hits for the warrior class
	if(GetClass() == WARRIOR && GetLevel() > 11)
	{
		//Melinko: Not sure if there is a bonus to critical hit chance when you are in berserk mode
		critChance = (IsBerserk() ? 0.1f : 0.05f);
		//Melinko: let Dex have a maximum of a 1.25% to increase crit chance, need to find correct calculation of this
		critChance += GetDEX() * (0.0125f / 255);
	}

	if(critChance > 0)
	{
		if(MakeRandomFloat(0, 1) < critChance)
		{
			damage = damage * 2;
			if(IsBerserk())
			{
				//Melinko: 2Handers adjust crit hit by 75% and 1handers adjust by 90%
				if(skill == _2H_BLUNT || skill == _2H_SLASHING)
				{
					damage = damage * 1.75f;
				}
				else
				{
					damage = damage * 1.90f;
				}
				Message(WHITE, "YOU land a crippling blow!(%d)", damage);
				entity_list.MessageClose(this, true, 200, WHITE, "%s lands a crippling blow!(%d)", GetName(), damage);
			}
			else
			{
				Message(WHITE, "YOU score a critical hit!(%d)", damage);
				entity_list.MessageClose(this, true, 200, WHITE, "%s scores a critical hit!(%d)", GetName(), damage);
			}
		}
	}
	return damage;
}

///////////////////////////////////////////////////

sint32 Client::CalculateAttackDamage(Mob* defender, Item_Struct* attacking_weapon, int8 skill, int hand, int8 attacker_level, int8 defender_level)
{
	int weapon_damage = GetWeaponDamage(skill, attacking_weapon, defender);
	if(this->GetPlayerProfilePtr()->inventory[12] == 10652) // Jester: Check for epic fists.
		weapon_damage = 9;
	int min_hit = 1;
	int16 STRBonus = 0;
	if(GetSTR() > 75)
		STRBonus = GetSTR() - 75;
	else
		STRBonus = 0;
	int max_hit = (2 * weapon_damage) + (weapon_damage * (STRBonus + GetSkill(OFFENSE))/225);
	int bonus = 0;
	int damage = 0;

	//Yeahlight: Caps on early game weapon damage
	if(GetLevel() < 10 && max_hit > 20)
		max_hit = 20;
	else if(GetLevel() < 20 && max_hit > 40)
		max_hit = 40;

	//Yeahlight: Monks are granted a melee bonus over every other class
	if(GetClass() == MONK)
		bonus += 10;
	if(GetLevel() > 50)
		bonus += 15;
	if(GetLevel() >= 55)
		bonus += 15;
	if(GetLevel() >= 60)
		bonus += 15;
	
	//Yeahlight: Apply bonuses to each hit
	min_hit += (min_hit * bonus / 100);
	max_hit += (max_hit * bonus / 100);

	//Yeahlight: Damage bonus applies to main hand only
	if(hand == 13)
	{
		int damage_bonus = GetWeaponDamageBonus(attacking_weapon);
		min_hit += damage_bonus;
		max_hit += damage_bonus;
	}

	//Yeahlight: If max_hit is smaller than min_hit for some reason, use min_hit
	if(max_hit < min_hit)
		max_hit = min_hit;

	//Yeahlight: Find the PC's favored damage interval
	damageInterval chosenDI = GetRandomInterval(this, defender);

	//Yeahlight: Players sitting take maximum damage per hit
	if(defender->GetAppearance() == SittingAppearance)
	{
		CAST_CLIENT_DEBUG_PTR(defender)->Log(CP_ATTACK, "Client::CalculateAttackDamage: You're sitting, getting max damage.");
		//Yeahlight: Apply innate mitigation to sitting warriors
		if(defender->GetClass() == WARRIOR)
		{
			int16 damageInterval = (max_hit - min_hit) / 19;
			int16 damageBonus = min_hit - damageInterval;
			damage = max_hit - damageInterval;
		}
		else
		{
			damage = max_hit;
		}
	}
	else
	{
		damage = GetIntervalDamage(this, defender, chosenDI, max_hit, min_hit);
	}

	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ATTACK, "Client::CalculateAttackDamage: Weapon damage: %i, min hit: %i, max hit: %i, final damage: %i", weapon_damage, min_hit, max_hit, damage);
	return damage;
}

///////////////////////////////////////////////////

//o--------------------------------------------------------------
//| GetRandomInterval; Yeahlight, Nov 21, 2008
//o--------------------------------------------------------------
//| Returns a 'random' DI interval based on attacker's ATK and
//| defender's mitigation AC
//o--------------------------------------------------------------
damageInterval Mob::GetRandomInterval(Mob* attacker, Mob* defender)
{
	bool debugFlag = true;

	sint16 attackerSTRBonus = attacker->GetSTR();
	int16 efficiencyATK = 0;
	int16 offenseSkill = 0;
	int16 mitigationAC = defender->GetMitigationAC();
	sint16 favoredDI = 0;
	int16 favoredChance = 0;
	sint16 levelModifier = attacker->GetLevel() / 10;
	sint16 ATKadjustment = (attacker->GetLevel() - defender->GetLevel()) * 2;
	damageInterval ret;
	int16 softACCap = 0;
	int16 ATKBonus = spellbonuses.ATK + itembonuses.ATK;
	if(levelModifier < 1)
		levelModifier = 1;
	else if(levelModifier > 6)
		levelModifier = 6;
	if(attackerSTRBonus > 75)
		attackerSTRBonus -= 75;
	else
		attackerSTRBonus = 0;
	if(attacker->IsClient())
		offenseSkill = attacker->CastToClient()->GetSkill(OFFENSE);
	else
		offenseSkill = attacker->GetSkill(OFFENSE);
	if(ATKadjustment < 0)
		ATKadjustment = 0;
	efficiencyATK = (offenseSkill * 4 / 3) + (ATKBonus * 4 / 3) + (attackerSTRBonus * 9 / 10)  + 5 + ATKadjustment;

	//Yeahlight: Adjust client's mitigation AC with softcaps for each class
	//           Note: The numbers are just a guess, but the order of classes is accurate
	if(defender->IsClient())
	{
		switch(defender->GetClass())
		{
			case WARRIOR:
				softACCap = 400;
				break;
			case PALADIN:
			case SHADOWKNIGHT:
				softACCap = 375;
				break;
			case CLERIC:
				softACCap = 355;
				break;
			case SHAMAN:
			case RANGER:
			case BARD:
				softACCap = 315;
				break;
			case DRUID:
				softACCap = 300;
				break;
			case MONK:
			case ROGUE:
				softACCap = 290;
				break;
			case ENCHANTER:
			case NECROMANCER:
			case WIZARD:
			case MAGICIAN:
				softACCap = 225;
				break;
			default:
				softACCap = 400;
				break;
		}

		//Yeahlight: Mitigation requires adjusting
		if(mitigationAC > softACCap)
		{
			int16 baseAC = softACCap;
			int16 adjustedPortion = (mitigationAC - softACCap) / 3;
			mitigationAC = baseAC + adjustedPortion;
		}
	}

	//Yeahlight: Efficiency roughly equals mitigation, thus the attacker's favored DI (dmg. interval) will be 10. 
	//           Grant the favoredChance a 2% bonus since the attacker has the edge with a DI of 10 when it should 
	//           be 9.5.
	if(abs(efficiencyATK - mitigationAC) <= 6)
	{
		favoredDI = 10;
		favoredChance = 35 + 2;
	}
	//Yeahlight: Attacker will have an extremely favored DI if the ATK:AC ratio is seriously out of sync
	else if(efficiencyATK >= mitigationAC * 1.65)
	{
		favoredDI = 19 - (9 - (levelModifier * 3 / 2));
		favoredChance = 50;
	}
	else if(efficiencyATK < mitigationAC / 1.65)
	{
		favoredDI = 0 + (9 - (levelModifier * 3 / 2));
		favoredChance = 50;
	}
	else if(efficiencyATK >= mitigationAC * 1.45)
	{
		favoredDI = 19 - (9 - (levelModifier * 3 / 2));
		favoredChance = 45;
	}
	else if(efficiencyATK < mitigationAC / 1.45)
	{
		favoredDI = 0 + (9 - (levelModifier * 3 / 2));
		favoredChance = 45;
	}
	else if(efficiencyATK >= mitigationAC * 1.25)
	{
		favoredDI = 19 - (9 - (levelModifier * 3 / 2));
		favoredChance = 40;
	}
	else if(efficiencyATK < mitigationAC / 1.25)
	{
		favoredDI = 0 + (9 - (levelModifier * 3 / 2));
		favoredChance = 40;
	}
	//Yeahlight: No extreme intervals present, so we need to derive one from 1 and 18
	else
	{
		//Yeahlight: For every 2.5% from equal, add or subtract 1 interval from a default DI of 10
		sint16 gapPercent = (sint16)(((float)(efficiencyATK - mitigationAC) / (float)(efficiencyATK)) * 100.00f);
		//Yeahlight: Moving down the interval scale
		if(gapPercent < 0)
		{
			gapPercent = abs(gapPercent);
			int16 adjustment = (gapPercent / 2.5);
			favoredDI = 10 - adjustment;
			if(favoredDI < 1 + (9 - (levelModifier * 3 / 2)))
				favoredDI = 1 + (9 - (levelModifier * 3 / 2));
			favoredChance = 35;

		}
		//Yeahlight: Moving up the interval scale
		else
		{
			int16 adjustment = (gapPercent / 2.5);
			favoredDI = 10 + adjustment;
			if(favoredDI > 18 - (9 - (levelModifier * 3 / 2)))
				favoredDI = 18 - (9 - (levelModifier * 3 / 2));
			favoredChance = 35;
		}
	}

	if(debugFlag && defender->IsClient() && defender->CastToClient()->GetDebugMe())
		defender->Message(LIGHTEN_BLUE, "Debug: %s's ATK efficiency: %i; Your AC mitigation: %i; Chosen DI: %i at %i%s", attacker->GetName(), efficiencyATK, mitigationAC, favoredDI, favoredChance, "%");
	else if(debugFlag && attacker->IsClient() && attacker->CastToClient()->GetDebugMe())
		attacker->Message(LIGHTEN_BLUE, "Debug: Your ATK efficiency: %i; %s's AC mitigation: %i; Chosen DI: %i at %i%s", efficiencyATK, defender->GetName(), mitigationAC, favoredDI, favoredChance, "%");

	ret.interval = favoredDI;
	ret.chance = favoredChance;

	return ret;
}

//o--------------------------------------------------------------
//| AvoidDamage; Yeahlight, Nov 24, 2008
//o--------------------------------------------------------------
//| Adapted from EQEMU 7.0: Called when a mob is attacked, does 
//| the checks to see if it's a hit.
//| Special return values:
//|   -1: Block
//|   -2: Parry
//|   -3: Riposte
//|   -4: Dodge
//o--------------------------------------------------------------
sint32 Mob::AvoidDamage(Mob* other, sint32 damage, bool riposte)
{
	float skill;
	float bonus = 0.0;
	float RollTable[4] = {0,0,0,0};
	Mob *attacker = other;
	Mob *defender = this;

	//Yeahlight: Attacker was not supplied; abort
	if(!attacker)
		return damage;

	//Yeahlight: Disabled mobs may not evade damage
	if(defender->IsNPC() && defender->CastToNPC()->IsDisabled())
		return damage;
	if(defender->IsClient() && (defender->CastToClient()->IsStunned() || defender->CastToClient()->GetFeared()))
		return damage;

	//garunteed hit
	bool ghit = false;
	//if((attacker->spellbonuses.MeleeSkillCheck + attacker->itembonuses.MeleeSkillCheck) > 500)
	//	ghit = true;
	
	//////////////////////////////////////////////////////////
	// make enrage same as riposte
	//////////////////////////////////////////////////////////
	if(defender->IsNPC())
	{
		if (damage > 0 && defender->CastToNPC()->IsEnraged() && !CanNotSeeTarget(attacker->GetX(), attacker->GetY()))
		{
			damage = -3;
		}
	}

	/////////////////////////////////////////////////////////
	// riposte
	/////////////////////////////////////////////////////////
	if(damage > 0 && !riposte && CanThisClassRiposte() && !CanNotSeeTarget(attacker->GetX(), attacker->GetY()))
	{
		if(defender->IsClient())
			skill = defender->CastToClient()->GetSkill(RIPOSTE);
		else
			skill = defender->GetSkill(RIPOSTE);
		if (IsClient()) {
        	if (!attacker->IsClient() && GetLevelCon(GetLevel(), attacker->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckAddSkill(RIPOSTE);
		}
		
		if (!ghit) {	//if they are not using a garunteed hit discipline
			//bonus = defender->spellbonuses.RiposteChance + defender->itembonuses.RiposteChance);
			bonus = 2.0 + (float)skill/35.0 + ((float)GetDEX()/200);
			RollTable[0] = bonus;
		}
	}
	
	///////////////////////////////////////////////////////	
	// block
	///////////////////////////////////////////////////////
	if(damage > 0 && CanThisClassBlock() && !CanNotSeeTarget(attacker->GetX(), attacker->GetY()))
	{
		if(defender->IsClient())
			skill = defender->CastToClient()->GetSkill(SKILL_BLOCK);
		else
			skill = defender->GetSkill(SKILL_BLOCK);
		if (IsClient()) {
			if (!attacker->IsClient() && GetLevelCon(GetLevel(), attacker->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckAddSkill(SKILL_BLOCK);
		}
		
		if (!ghit) {	//if they are not using a garunteed hit discipline
			//Yeahlight: Block is more likely to fire than all the other avoidance skills, thus the (skill/20.0) check
			bonus = 2.0 + (float)skill/20.0 + ((float)GetDEX()/200);
			RollTable[1] = bonus;
		}
	}
	
	//////////////////////////////////////////////////////		
	// parry
	//////////////////////////////////////////////////////
	if(damage > 0 && CanThisClassParry() && !CanNotSeeTarget(attacker->GetX(), attacker->GetY()))
	{
        if(defender->IsClient())
			skill = defender->CastToClient()->GetSkill(PARRY);
		else
			skill = defender->GetSkill(PARRY);
		if (IsClient()) {
			if (!other->IsClient() && GetLevelCon(GetLevel(), other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckAddSkill(PARRY); 
		}
		
		//bonus = (defender->spellbonuses.ParryChance + defender->itembonuses.ParryChance) / 100.0f;
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus = 2.0 + (float)skill/35.0 + ((float)GetDEX()/200);
			RollTable[2] = bonus;
		}
	}
	
	////////////////////////////////////////////////////////
	// dodge
	////////////////////////////////////////////////////////
	if(damage > 0 && CanThisClassDodge() && !CanNotSeeTarget(attacker->GetX(), attacker->GetY()))
	{
        if(defender->IsClient())
			skill = defender->CastToClient()->GetSkill(DODGE);
		else
			skill = defender->GetSkill(DODGE);
		if (IsClient()) {
			if (!attacker->IsClient() && GetLevelCon(GetLevel(), attacker->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckAddSkill(DODGE);
		}
		
		//bonus = (defender->spellbonuses.DodgeChance + defender->itembonuses.DodgeChance) / 100.0f;
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus = 2.0 + (float)skill/35.0 + ((float)GetAGI()/200);
			RollTable[3] = bonus;
		}
	}

	//Yeahlight: The order of events below is very important, please do not reorder them!
	if(damage > 0)
	{
		//Yeahlight: Dodge check
		if(MakeRandomFloat(0.1, 100.0) <= RollTable[3])
		{
			damage = -4;
			return damage;
		}
		//Yeahlight: Parry check
		else if(MakeRandomFloat(0.1, 100.0) <= RollTable[2])
		{
			damage = -2;
			return damage;
		}
		//Yeahlight: Block check
		else if(MakeRandomFloat(0.1, 100.0) <= RollTable[1])
		{
			damage = -1;
			return damage;
		}
		//Yeahlight: Riposte check
		else if(MakeRandomFloat(0.1, 100.0) <= RollTable[0])
		{
			//Yeahlight: PC defender may not riposte if they are not wielding a primary weapon
			if(defender->IsClient())
			{
				//Yeahlight: PC is wielding a non-weapon in the primary slot
				if(defender->CastToClient()->GetPrimaryWeapon() && defender->CastToClient()->GetPrimaryWeapon()->common.damage == 0)
				{
					if(defender->CastToClient()->GetDebugMe())
					{
						Message(LIGHTEN_BLUE, "Debug: Your riposte FAILED due to wielding a non-weapon in your primary slot.");
					}
					return damage;
				}
			}
			damage = -3;
			return damage;
		}
	}
	return damage;
}

//o--------------------------------------------------------------
//| GetIntervalDamage; Yeahlight, Nov 24, 2008
//o--------------------------------------------------------------
//| Returns a random damage interval between 1 and 20
//o--------------------------------------------------------------
int16 Mob::GetIntervalDamage(Mob* attacker, Mob* defender, damageInterval chosenDI, int32 max_hit, int32 min_hit)
{
	if(!attacker || !defender)
		return 0;

	int damage = 0;

	//Yeahlight: Damage calculation: DB + X * DI, where DB is damage bonus (min_hit - DI), X is random integer (1 - 20) and
	//			 DI is damage interval ((max_hit - min_hit) / 19).
	//			 TODO: Warrior defensive disc multiplies DI by 0.5
	//           TODO: Monk stonestance disc multiplies DI by 0.1
	sint16 damageBonus = 0;
	int16 randomInterval = 0;
	int16 randomChance = chosenDI.chance;
	float damageInterval = 0.00;
	damageInterval = (max_hit - min_hit) / 19.00;
	damageBonus = min_hit - damageInterval;
	if(damageBonus < 1)
		damageBonus = 1;
	//Yeahlight: Grab a random interval with respect to randomChance
	if(rand()%100 <= randomChance)
	{
		randomInterval = chosenDI.interval;
	}
	else
	{
		if(chosenDI.interval == 0)
		{
			if(rand()%100 <= (randomChance / 2.25))
				randomInterval = 1;
			else
				randomInterval = rand()%20;
		}
		else if(chosenDI.interval == 19)
		{
			if(rand()%100 <= (randomChance / 2.25))
				randomInterval = 18;
			else
				randomInterval = rand()%20;
		}
		else
		{
			if(rand()%100 <= (randomChance / 4.5))
				randomInterval = chosenDI.interval + 1;
			else if(rand()%100 <= (randomChance / 4.5))
				randomInterval = chosenDI.interval - 1;
			else
				randomInterval = rand()%20;
		}
	}
	//Yeahlight: Final damage calculation (a warrior's 20th interval is everyone elses 19th interval)
	if(defender->GetClass() == WARRIOR)
		damage = damageBonus + randomInterval * damageInterval;
	else
		damage = damageBonus + (randomInterval + 1) * damageInterval;

	return damage;
}

//o--------------------------------------------------------------
//| BuildLootRights; Yeahlight, June 26, 2009
//o--------------------------------------------------------------
//| Builds a list of attackers who get first loot rights
//o--------------------------------------------------------------
queue<char*> NPC::BuildLootRights(Group* group, Client* client)
{
	queue<char*> list;
	//Yeahlight: A group will be given first loot rights
	if(group)
	{
		list = group->BuildGroupList();
	}
	//Yeahlight: A single entity will be given first loot rights
	else if(client)
	{
		list.push(client->GetName());
	}
	return list;
}