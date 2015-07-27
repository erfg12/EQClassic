// Written by Cofruben.
#include "database.h"
#include "SpellsHandler.hpp"
#include "mob.h"
#include "classes.h"
#include "skills.h"
#include "../include/playercorpse.h"
#include "spdat.h"
#include "groups.h"

const bool	SPAM_SPELL_BONUSES = true;
extern		SpellsHandler		spells_handler;
extern		Database			database;
extern		Zone*				zone;

///////////////////////////////////////////////////
// Quagmire - that case above getting to long and spells are gonna have a lot of cases of their own
// Cofruben - Reorganised this a little.
void Mob::SpellEffect(Mob* caster, Spell* spell, int8 caster_level, bool partialResist)
{
	//Spells not loaded!
	if(!spells_handler.SpellsLoaded())
		return;
	//Spell not loaded!
	if(!spell)
		return;
	//Yeahlight: Caster was not supplied
	if(!caster)
		return;

	int i = 0;
	const int16	spell_id		= spell->GetSpellID();
	const char*	teleport_zone	= spell->GetSpellTeleportZone();
	
	// 1. Is it a buff? If so, handle its time based effects.
	if (spell->IsBuffSpell())
		spells_handler.HandleBuffSpellEffects(caster, this, spell);
	
	// 2. Handle its single-time effect.
	for (i = 0; i < EFFECT_COUNT; i++)
	{
		TSpellEffect effect_id = spell->GetSpellEffectID(i);
		if(effect_id == SE_Blank || effect_id == 0xFF)
			continue;
		int8   formula = spell->GetSpellFormula(i);
		sint16 base    = spell->GetSpellBase(i);
		sint16 max     = spell->GetSpellMax(i);	
		sint32 amount  = spells_handler.CalcSpellValue(spell, i, caster_level);
		//Yeahlight: This is an NPC and had a detremental spell casted upon it
		if(this->IsNPC() && (spell->IsDetrimentalSpell() || spell->IsUtilitySpell()))
		{
			CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): aggroing %s because of the spell effect!", spell->GetSpellName(), this->GetName());
			//Yeahlight: Generate hate based on the spells's effect type
			sint16 tempHate = GetSpellHate(effect_id, spell->GetMinLevel(), false, amount);
			if(tempHate)
			{
				this->CastToNPC()->AddToHateList(caster, 0, tempHate);
			}
		}
		switch(effect_id)
		{
			case SE_CurrentHP:
			case SE_CurrentHPOnce:
			{
				sint32 OldHP = this->GetHP();
				sint32 damage = amount;
				//Yeahlight: Partial resist calculations
				if(partialResist)
				{
					damage = damage / 2;
					damage = damage * (float)((float)(rand()%90 + 10) / 100.00f);
					if(caster->IsClient() && caster->CastToClient()->GetDebugMe())
						caster->Message(YELLOW, "Debug: Your direct damage spell resist has been upgrade to a partial resist.");
				}
				this->ChangeHP(caster, damage, spell_id);
				sint32 NewHP = this->GetHP();
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You changed %s's hp by %+i.", spell->GetSpellName(), this->GetName(), damage);
				break;
			}
			case SE_MovementSpeed:
			{
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a Movement Speed spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_AttackSpeed:
			{
				//Yeahlight: There should not be any work to be done here
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a Attack Speed spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Invisibility: 
			{
				this->SetInvisible(true);
				//Yeahlight: Castee has a pet; remove it
				if(GetPet())
				{
					Mob* myPet = GetPet();
					//Yeahlight: Castee's pet is an NPC
					if(myPet->IsNPC())
					{
						//Yeahlight: Castee's pet is a charmed NPC
						if(myPet->CastToNPC()->IsCharmed())
						{
							myPet->CastToNPC()->BuffFadeByEffect(SE_Charm);
						}
						//Yeahlight: Castee's pet is a summoned NPC
						else
						{
							myPet->Depop();
						}
					}
					//Yeahlight: Castee's pet is a charmed PC
					else if(myPet->IsClient() && myPet->CastToClient()->IsCharmed())
					{
						myPet->CastToClient()->BuffFadeByEffect(SE_Charm);
					}
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted an invisibility spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_CurrentMana:
			{
				SetMana(GetMana() + amount);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a mana recovery spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_AddFaction:
			{
				//Yeahlight: Only continue if the target is an NPC and the caster is a PC
				if(this->IsNPC() && caster->IsClient())
				{
					caster->CastToClient()->SetCharacterFactionLevelModifier(this->CastToNPC()->GetPrimaryFactionID(), amount);
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted an add faction spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Stun:
			{
				if (IsClient())
				{
					CastToClient()->Stun(base);
				}
				else if(IsNPC())
				{
					//Yeahlight: NPC is immune to stun effects
					if(CastToNPC()->GetCannotBeStunned())
					{
						if(caster->IsClient())
						{
							caster->Message(RED, "Your target is immune to the stun portion of this effect");
						}
					}
					else
					{
						CastToNPC()->Stun(base);
					}
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a stun spell, amount: %i.", spell->GetSpellName(), base);
				break;
			}
			case SE_Charm:
			{
				//Yeahlight: Can only charm a non-pet and the caster may only have one pet
				if(this->GetOwner() == NULL && caster->GetPet() == NULL && caster != this)
				{
					//Yeahlight: Flag the NPC as a pet
					if(this->IsNPC())
					{
						caster->SetPet(this);
						this->SetOwnerID(caster->GetID());
						this->CastToNPC()->SetCharmed(true);
						this->SetPetOrder(SPO_Follow);
						if(caster->IsClient())
							caster->CastToClient()->SendCharmPermissions();
						this->CastToNPC()->WhipeHateList();
						this->CastToNPC()->StartTaunting();
					}
					else if(this->IsClient())
					{
						if(caster->IsNPC())
						{
							caster->SetPet(this);
							this->SetOwnerID(caster->GetID());
							Mob* myTarget = caster->CastToNPC()->GetHateTop();
							if(!myTarget)
								myTarget = caster->CastToMob();
							this->SetTarget(myTarget);
							this->animation = 0;
							this->delta_heading = 0;
							this->delta_x = 0;
							this->delta_y = 0;
							this->delta_z = 0;
							this->SendPosUpdate(true, PC_UPDATE_RANGE, false);
							this->CastToClient()->charmPositionUpdate_timer->Start(200);
							this->CastToClient()->SetCharmed(true);
							this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Lose_Control, false);
							this->SetPetOrder(SPO_Follow);
						}
						else if(caster->IsClient())
						{
							caster->SetPet(this);
							this->SetOwnerID(caster->GetID());
							this->SetTarget(caster);
							this->animation = 0;
							this->delta_heading = 0;
							this->delta_x = 0;
							this->delta_y = 0;
							this->delta_z = 0;
							this->SendPosUpdate(true, PC_UPDATE_RANGE, false);
							this->CastToClient()->charmPositionUpdate_timer->Start(200);
							this->CastToClient()->SetCharmed(true);
							this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Lose_Control, false);
							this->SetPetOrder(SPO_Follow);
							caster->CastToClient()->SendCharmPermissions();
						}
					}
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a charm spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Fear:
			{
				//Yeahlight: Victim is a PC
				if(this->IsClient())
				{
					this->CastToClient()->SetFeared(true);
					this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Lose_Control, false);
					this->CastToClient()->GetFearDestination(GetX(), GetY(), GetZ());
				}
				//Yeahlight: Victim is an NPC
				else if(this->IsNPC())
				{
					this->CastToNPC()->SetFeared(true);
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a fear spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Stamina:
			{
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a stamina spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_BindAffinity:
			{
				//Yeahlight: Target of the bind affinity spell is a client
				if(this->IsClient())
				{
					this->CastToClient()->SetBindPoint();
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a Bind Affinity spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Gate:
			{
				if(IsClient())
					CastToClient()->GoToBind();
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a gate spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_CancelMagic:
			{
				for(int i = 0; i < 15; i++)
				{
					//Yeahlight: Buff must exist and the buff may not have any poison or disease counters
					if(buffs[i].spell && buffs[i].spell->IsValidSpell() && buffs[i].casterlevel <= (caster_level + base) && buffs[i].spell->GetDiseaseCounters() == 0 && buffs[i].spell->GetPoisonCounters() == 0)
					{	
						this->BuffFadeBySlot(i, true);
						break;
					}
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a cancel magic spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_InvisVsUndead:
			{
				this->SetInvisibleUndead(true);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted an Invis VS Undead spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_InvisVsAnimals:
			{
				this->SetInvisibleAnimal(true);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted an Invis VS Animal spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Mez:
			{
				// Pinedepain // When a mezz spell is casted, we mesmerize this mob
				Mesmerize();
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a mesmerize spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_SummonItem:
			{
				if(this->IsClient())
				{
					if(amount == 0)
						this->CastToClient()->SummonItem(base, 1);
					else
						this->CastToClient()->SummonItem(base, (amount > 20) ? 20 : amount);
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a summon item spell: %i.", spell->GetSpellName(), base);
				break;
			}
			case SE_NecPet:
			case SE_SummonPet:
			{
				if (this->GetPetID() != 0) {
					Message(RED, "You\'ve already got a pet.");
					break;
				}
				this->MakePet(teleport_zone);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a Summon pet / nec pet spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_DivineAura:
			{
				this->SetInvulnerable(true);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a Divine Aura, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_ShadowStep:
			{
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a Shadow Step spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Rune:
			{
				//Yeahlight: Flag entity with rune for damage calculations
				hasRuneOn = true;
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a Rune spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Levitate:
			{
				this->SendAppearancePacket(0, SAT_Levitate, 2, true);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a levitate spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_SummonCorpse:
			{
				bool permit = false;
				Mob* corpseOwner = target;
				//Yeahlight: The target of this spell is not a PC or the caster is targeting themself
				if(!target || (target && !target->IsClient()) || target == this)
				{
					corpseOwner = this;
					permit = true;
				}
				//Yeahlight: Can only summon a PC's corpse
				if(corpseOwner && corpseOwner->IsClient())
				{
					//Yeahlight: PCs must be grouped to summon a corpse
					if(!permit)
					{
						Group* targetGroup = entity_list.GetGroupByClient(corpseOwner->CastToClient());
						Group* myGroup = NULL;
						if(this->IsClient())
							myGroup = entity_list.GetGroupByClient(this->CastToClient());
						//Yeahlight: Caster is in a group and they share the same group as the target
						if(myGroup != NULL && myGroup == targetGroup)
							permit = true;
					}
					//Yeahlight: Caster may proceed with the summon
					if(permit)
					{
						Corpse *corpse = entity_list.GetCorpseByOwner(corpseOwner->CastToClient());
						//Yeahlight: Corpse has been located
						if(corpse)
						{
							corpse->Summon(corpseOwner->CastToClient(), caster->CastToClient(), true);
							CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a summon corpse spell: %s.", spell->GetSpellName(), this->GetName());
						}
						//Yeahlight: There is no corpse available
						else
						{
							//Yeahlight: Caster failed to locate his/her corpse
							if(caster == corpseOwner)
							{
								caster->Message(RED, "You do not have a corpse in this zone.");
							}
							//Yeahlight: Caster failed to locate their target's corpse
							else
							{
								caster->Message(RED, "Your target does not have a corpse in this zone.");
							}
							CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): Summon corpse: you can't sense the corpse: %i.", spell->GetSpellName());
						}
					}
					else
					{
						//Yeahlight: TODO: This is not the correct message
						Message(RED, "You and your target must be in the same group to perform this action.");
					}
				}
				break;
			}
			case SE_Illusion:
			{
				SendIllusionPacket(base, GetDefaultGender(base, GetBaseGender()), GetTexture(), GetHelmTexture());
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted an illusion spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Identify:
			{
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted an identify spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_WipeHateList:
			{
				//Yeahlight: NOTE: Do NOT wipe the rampage list here; that never goes away until the mob resets
				if(this->IsNPC())
				{
					//Yeahlight: TODO: I don't remember this message, look into this
					entity_list.MessageClose(this, true, DEFAULT_MESSAGE_RANGE, DARK_BLUE, "My mind fogs. Who are my friends? Who are my enemies?... it was all so clear a moment ago...");
					this->CastToNPC()->WhipeHateList();
					CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a whipe hate list spell, amount: %i.", spell->GetSpellName(), amount);
				}
				break;
			}
			case SE_SpinTarget:
			{
				Spin(caster, spell);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a spin target spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_EyeOfZomm:
			{
				//Yeahlignt: Only produce eyes of zomm for PCs
				if(this->IsClient())
				{
					if(this->CastToClient()->myEyeOfZomm == 0)
						MakeEyeOfZomm(this);
					else
						Message(RED, "You may only have one eye of zomm out at a time!");
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a Eye Of Zomm spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_ReclaimPet:
			{
				//Yeahlight: Target of the spell is an uncharmed NPC, has an owner and the owner is the caster of the spell
				if(IsNPC() && CastToNPC()->IsCharmed() == false && GetOwnerID() && caster->GetID() == GetOwnerID())
				{
					//Yeahlight: TODO: Research this formula
					caster->SetMana(caster->GetMana()+(GetLevel()*4));
					if(caster->IsClient())
					{
						caster->CastToClient()->SetPet(0);
					}
					SetOwnerID(0);
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a reclaim pet spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_FeignDeath:
			{
				if(this->IsClient())
					this->CastToClient()->FeignDeath(this->CastToClient()->GetSkill(ABJURATION));
				break;
			}
			case SE_VoiceGraft:
			{
				//Yeahlight: Only allow voice graft to be casted on NPCs (we don't want PCs griefing other charmed PCs with /say)
				if(IsNPC() && caster->IsClient() && CastToNPC()->GetOwner() == caster)
				{
					caster->CastToClient()->SetVoiceGrafting(true);
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a voice graft spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Revive:
			{
				//Yeahlight: Handled in client_process.cpp
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a revive spell, amount: %i, corpse: %s.", spell->GetSpellName(), amount, this->GetName());
				break;
			}
			case SE_Teleport:
			{
				if(this->IsClient())
					this->CastToClient()->MovePC(teleport_zone, spell->GetSpellBase(1), spell->GetSpellBase(0), spell->GetSpellBase(2));
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a teleport spell to %s (%f, %f, %f).", spell->GetSpellName(), teleport_zone, spell->GetSpellBase(1), spell->GetSpellBase(0), spell->GetSpellBase(2));
				break;
			}
			case SE_Translocate:
			{
				bool permit = false;
				Mob* translocatee = CastToMob();

				//Enraged: The target of this spell is an NPC.
				//if(translocatee && !translocatee->IsClient())
				//{
				//	//Enraged: TODO: This is not the correct message?
				//	Message(RED, "You cannot cast that spell on your current target.");
				//	break;
				//}

				//Enraged: The target of this spell is the caster.
				//TODO: Can players target themselves with translocate spells?
				if(translocatee && (translocatee == this))
					permit = true;

				//Enraged: Check if the targetted client is in the casters group.
				//TODO: Translocate only worked on group players, right?
				if(!permit && translocatee)
				{
					Group* translocateeGroup = entity_list.GetGroupByClient(translocatee->CastToClient());
					Group* casterGroup = NULL;
					if(this->IsClient())
						casterGroup = entity_list.GetGroupByClient(this->CastToClient());
					//Enraged: The translocatee is in a group and they share the same group as the target
					if(casterGroup != NULL && casterGroup == translocateeGroup)
						permit = true;
				}

				//Enraged: Target is clear to be translocated.
				if(permit)
				{
					CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a translocation spell on %s.", spell->GetSpellName(), translocatee->GetName());
					//TODO: Translocate code here
					translocatee->CastToClient()->SendTranslocateConfirmation(caster, spell);
				}
				else
				{
					//The translocatee was not in the casters group.
					//Enraged: TODO: This is not the correct message
					Message(RED, "You can only cast that spell on players in your group.");
				}
				break;
			}
			case SE_InfraVision:
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted an infravision spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_UltraVision:
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted an ultravision spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_BindSight:
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a bind sight spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_SeeInvis:
				SetCanSeeThroughInvis(true);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a see invis spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_WaterBreathing:
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted an water breathing spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_SenseDead:
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a sense dead spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_SenseSummoned:
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a sense summoned spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_TrueNorth: 
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a true north spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_SenseAnimals:
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a sense animals spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_DamageShield:
				//Yeahlight: There should not be any work to be done here
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a damage shield spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_Sentinel:
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a sentinel spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_LocateCorpse:
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a locate corpse spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_ModelSize:
			{
				//Yeahlight: Grow/Shrink
				float newSize = (GetSize() * (float)base) / 100.00f;
				//Yeahlight: Size of a gnome (minimum)
				if(newSize < 3)
					newSize = 3;
				//Yeahlight: Size of an ogre (maximum)
				else if(newSize > 9)
					newSize = 9;
				this->size = newSize;
				this->SendAppearancePacket(GetID(), SAT_Size, GetSize(), true);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a model size spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Root:
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a root spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_Blind:
				//Yeahlight: Handled client side
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a blind spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			case SE_DiseaseCounter:
			{
				//Yeahlight: Spell is a cure disease spell
				if(amount < 0)
				{
					//Yeahlight: Iterate through all the debuffs on the target and check for the chance to cure it
					for(int i = 0; i < 15; i++)
					{
						if(buffs[i].spell && buffs[i].diseasecounters)
						{
							buffs[i].diseasecounters = buffs[i].diseasecounters + amount;
							if(buffs[i].diseasecounters <= 0)
								BuffFadeBySlot(i, true);
							break;
						}
					}
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a disease counter spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_PoisonCounter:
			{
				//Yeahlight: Spell is a cure poison spell
				if(amount < 0)
				{
					//Yeahlight: Iterate through all the debuffs on the target and check for the chance to cure it
					for(int i = 0; i < 15; i++)
					{
						if(buffs[i].spell && buffs[i].poisoncounters)
						{
							buffs[i].poisoncounters = buffs[i].poisoncounters + amount;
							if(buffs[i].poisoncounters <= 0)
								BuffFadeBySlot(i, true);
							break;
						}
					}
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a poison counter spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_Calm:
			{
				//Yeahlight: Only add/remove hate from NPCs
				if(this->IsNPC())
				{
					this->CastToNPC()->AddToHateList(caster, 0, amount);
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): You casted a calm hate spell, amount: %i.", spell->GetSpellName(), amount);
				break;
			}
			case SE_WeaponProc:
			{
				Spell* spell = spells_handler.GetSpellPtr(base);
				//Yeahlight: Legit spell proc bonus found
				if(spell)
					SetBonusProcSpell(spell);
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell name = %s): Weapon proc bonus of spell ID %i.", spell->GetSpellName(), base);
				break;
			}
			case SE_StopRain:
			{
				zone->zone_weather = 0;
				zone->weatherSend();
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): You casted a stop rain spell, amount: %i.", spell->GetSpellName(), base);
				break;
			}
			case SE_CallOfHero:
			{
				//Yeahlight: Call of the Hero may only be used on PCs
				if(this->IsClient())
					this->CastToClient()->MovePC(0, caster->GetX(), caster->GetY(), caster->GetZ(), false, true);
				else
					caster->Message(RED, "This spell may only be cast on players.");
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): You casted a call of the hero spell, amount: %i.", spell->GetSpellName(), base);
				break;
			}
			case SE_CallPet:
			{
				//Yeahlight: This spell line may only be used on NPC pets
				if(GetPet() && GetPet()->IsNPC())
				{
					GetPet()->CastToNPC()->GMMove(GetX(), GetY(), GetZ(), GetHeading());
					GetPet()->pStandingPetOrder = SPO_Follow;
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): You casted a call pet spell, amount: %i.", spell->GetSpellName(), base);
				break;
			}
			case SE_DeathSave:
			{
				//Yeahlight: Only apply divine intervention to players
				if(this->IsClient())
				{
					sint16 successChance = 0;
					float baseChance = 0.00f;
					switch(base)
					{
						//Yeahlight: Death Pact (CLR: 51)
						case 1:
						{
							baseChance = 0.10f;
							break;
						}
						//Yeahlight: Divine Intervention (CLR: 60)
						case 2:
						{
							baseChance = 0.30f;
							break;
						}
						default:
						{
							baseChance = 0.10f;
						}
					}
					//Yeahlight: The target's CHA is calculated into the bonus save chance
					successChance = (((float)CastToClient()->GetCHA() * 0.0005f) + baseChance) * 100;
					//Yeahlight: The worst possible save chance is the spell's base chance
					if(successChance < baseChance)
						successChance = baseChance;
					else if(successChance > 100)
						successChance = 100;
					this->CastToClient()->SetDeathSave(successChance);
				}
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): You casted a death save spell, amount: %i.", spell->GetSpellName(), base);
				break;
			}
			case SE_Succor:
			{
				//Yeahlight: There should be nothing to do here
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::ApplySpellsBonuses(spell name = %s): You casted a succor spell, amount: %i.", spell->GetSpellName(), base);
				break;
			}
			case 0xFE:
			case 0xFF:
			case SE_Harmony:
			case SE_ChangeFrenzyRad:
			case SE_Lull:
			case SE_TotalHP:
			case SE_ArmorClass:
			case SE_MagnifyVision:
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
			{
				// Buffs are handeled elsewhere
				break;
			}
			default:
			{
				CAST_CLIENT_DEBUG_PTR(caster)->Log(CP_SPELL, "Mob::SpellEffect(spell_name = %s): unknown effect (%i) for amount: %i.", spell->GetSpellName(), effect_id, amount);
				break;
			}
		}
	}
	if(this->IsClient())
		this->CastToClient()->Save();
}

//o--------------------------------------------------------------
//| BuffFadeByEffect; Yeahlight, Feb 28, 2009
//o--------------------------------------------------------------
//| Fades all associated spells by effect
//o--------------------------------------------------------------
void Mob::BuffFadeByEffect(TSpellEffect effect)
{
	//Yeahlight: Iterate through each buff slot
	for(int i = 0; i < BUFF_COUNT; i++)
	{
		//Yeahlight: Buff exists and is a death save spell
		if(buffs[i].spell && buffs[i].spell->IsEffectInSpell(effect))
		{
			BuffFadeBySlot(i, true);
		}
	}
}

//o--------------------------------------------------------------
//| BuffFadeBySlot; Yeahlight, Nov 16, 2008
//o--------------------------------------------------------------
//| Adapted from EQEMU 7.0: Removes the buff in the supplied
//| buff slot 'slot'
//o--------------------------------------------------------------
void Mob::BuffFadeBySlot(int slot, bool iRecalcBonuses)
{
	bool debugFlag = true;

	if(slot < 0 || slot > BUFF_COUNT)
		return;

	if(!buffs[slot].spell || !buffs[slot].spell->IsValidSpell())
		return;

	if(IsClient())
		CastToClient()->MakeBuffFadePacket(buffs[slot].spell, slot);


	if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
		this->Message(LIGHTEN_BLUE, "Debug: Fading buff %d from slot %d", buffs[slot].spell->GetSpellID(), slot);

	for(int i = 0; i < EFFECT_COUNT; i++)
	{
		if(buffs[slot].spell->IsBlankSpellEffect(i))
			continue;
		switch(buffs[slot].spell->GetSpellEffectID(i))
		{
			case SE_WeaponProc:
			{
				SetBonusProcSpell(0);
				break;
			}
			case SE_Illusion: case SE_IllusionCopy:
			{
				SendIllusionPacket(GetBaseRace(), GetBaseGender(), GetTexture(), GetHelmTexture());
				SendAppearancePacket(this->GetID(), SAT_Size, GetDefaultSize(), true);
				break;
			}
			case SE_Levitate:
			{
				SendAppearancePacket(this->GetID(), SAT_Levitate, 0, true);
				break;
			}
			case SE_Invisibility:
			{
				SetInvisible(false);
				break;
			}
			case SE_InvisVsUndead:
			{
				SetInvisibleUndead(false);
				break;
			}
			case SE_InvisVsAnimals:
			{
				SetInvisibleAnimal(false);
				break;
			}
			case SE_Silence:
			{
				break;
			}
			case SE_DivineAura:
			{
				SetInvulnerable(false);
				break;
			}
			case SE_Rune:
			{
				break;
			}
			case SE_AbsorbMagicAtt:
			{
				break;
			}
			case SE_Mez:
			{
				//Yeahlight: Unfreeze the PC's UI
				if(IsClient())
					SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Sitting_To_Standing, true);
				this->mesmerized = false;
				break;
			}
			case SE_Charm:
			{
				Mob* charmer = entity_list.GetMob(this->GetOwnerID());
				if(charmer && charmer->IsClient())
				{
					char npcNameSuffix[] = "_CHARM00";
					char npcNamePrefix[100] = "";
					strcpy(npcNamePrefix, charmer->GetName());
					strcat(npcNamePrefix, npcNameSuffix);
					Mob* charmPH = entity_list.GetMob(npcNamePrefix);
					if(charmPH && charmPH->IsNPC())
					{
						charmPH->Depop();
					}
					//Yeahlight: Check for _CHARM01 NPC, too
					npcNamePrefix[strlen(npcNamePrefix) - 1] = '1';
					charmPH = entity_list.GetMob(npcNamePrefix);
					if(charmPH && charmPH->IsNPC())
					{
						charmPH->Depop();
					}
					//Yeahlight: Generate hate for towards the charmer if the charmer is not FD'ed
					if(this->IsNPC() && charmer->IsClient() && !charmer->CastToClient()->GetFeigned())
					{
						CastToNPC()->AddToHateList(charmer, 0, GetSpellHate(SE_Charm, this->GetLevel(), false));
					}
				}
				//Yeahlight: Unfreeze the PC's UI
				if(IsClient())
				{
					SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Sitting_To_Standing, true);
					CastToClient()->SetCharmed(false);
					CastToClient()->charmPositionUpdate_timer->Disable();
				}
				//Yeahlight: Deflag the NPC as charmed
				else if(IsNPC())
				{
					CastToNPC()->SetCharmed(false);
				}
				this->SetOwnerID(0, false);
				break;
			}
			case SE_Root:
			{
				break;
			}
			case SE_Fear:
			{
				//Yeahlight: Unfreeze the PC's UI
				if(IsClient())
				{
					CastToClient()->SetFeared(false);
					SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Sitting_To_Standing, true);
				}
				else if(IsNPC())
				{
					CastToNPC()->SetFeared(false);
					CastToNPC()->SetOnFearPath(false);
				}
				break;
			}
			case SE_SpinTarget:
			{
				//Yeahlight: Unfreeze the PC's UI
				if(IsClient())
					SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Sitting_To_Standing, true);
				break;
			}
			case SE_EyeOfZomm:
			{
				//Yeahlight: Clear the eye of zomm pointer and depop the eye
				if(IsClient() && CastToClient()->myEyeOfZomm)
				{
					CastToClient()->myEyeOfZomm->Depop();
					CastToClient()->myEyeOfZomm = NULL;
				}
				break;
			}
			case SE_DeathSave:
			{
				//Yeahlight: Clear the death save chance
				if(IsClient())
					CastToClient()->SetDeathSave(0);
				break;
			}
			case SE_VoiceGraft:
			{
				//Yeahlight: Drop the voice grafting flag from the client
				if(GetOwner() && GetOwner()->IsClient())
				{
					GetOwner()->CastToClient()->SetVoiceGrafting(false);
				}
				break;
			}
			case SE_SeeInvis:
			{
				//Yeahlight: Mob may no longer see through invis
				SetCanSeeThroughInvis(false);
				break;
			}
		}
	}

	buffs[slot].spell = NULL;

	if(iRecalcBonuses)
		CalcBonuses(true, true);
}

//o--------------------------------------------------------------
//| CalcSpellEffectValue; Yeahlight, Nov 16, 2008
//o--------------------------------------------------------------
//| Adapted from EQEMU 7.0
//o--------------------------------------------------------------
int Mob::CalcSpellEffectValue(Spell* spell, int effect_id, int caster_level, Mob *caster, int ticsremaining)
{
	int formula, base, max, effect_value;
	int16 spell_id = spell->GetSpellID();

	if(!spell->IsValidSpell() || effect_id < 0 || effect_id >= EFFECT_COUNT)
		return 0;

	formula = spell->GetSpellFormula(effect_id);
	base = spell->GetSpellBase(effect_id);
	max = spell->GetSpellMax(effect_id);

	if(spell->IsBlankSpellEffect(effect_id))
		return 0;

	effect_value = CalcSpellEffectValue_formula(formula, base, max, caster_level, spell, ticsremaining);

	if(caster && caster->IsClient() && spell->IsBardSong() &&
	(spell->Get().effectid[effect_id] != SE_AttackSpeed) &&
	(spell->Get().effectid[effect_id] != SE_AttackSpeed2) &&
	(spell->Get().effectid[effect_id] != SE_AttackSpeed3)) {
		int oval = effect_value;
		int mod = caster->CastToClient()->GetInstrumentModAppliedToSong(spell);
		effect_value = effect_value * mod / 10;
		//mlog(SPELLS__BARDS, "Effect value %d altered with bard modifier of %d to yeild %d", oval, mod, effect_value);
	}

	return(effect_value);
}

//o--------------------------------------------------------------
//| CalcSpellEffectValue_formula; Yeahlight, Nov 16, 2008
//o--------------------------------------------------------------
//| Adapted from EQEMU 7.0
//o--------------------------------------------------------------
int Mob::CalcSpellEffectValue_formula(int formula, int base, int max, int caster_level, Spell* spell, int ticsremaining)
{
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

	int result = 0, updownsign = 1, ubase = base;
	if(ubase < 0)
		ubase = 0 - ubase;

	// solar: this updown thing might look messed up but if you look at the
	// spells it actually looks like some have a positive base and max where
	// the max is actually less than the base, hence they grow downward
/*
This seems to mainly catch spells where both base and max are negative.
Strangely, damage spells  have a negative base and positive max, but
snare has both of them negative, yet their range should work the same:
(meaning they both start at a negative value and the value gets lower)
*/
	if (max < base && max != 0)
	{
		// values are calculated down
		updownsign = -1;
	}
	else
	{
		// values are calculated up
		updownsign = 1;
	}

	//mlog(SPELLS__EFFECT_VALUES, "CSEV: spell %d, formula %d, base %d, max %d, lvl %d. Up/Down %d", spell_id, formula, base, max, caster_level, updownsign);

	switch(formula)
	{
		case 60:	//used in stun spells..?
		case 70:
			result = ubase/100; break;
		case   0:
		case 100:	// solar: confirmed 2/6/04
			result = ubase; break;
		case 101:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level / 2)); break;
		case 102:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + caster_level); break;
		case 103:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level * 2)); break;
		case 104:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level * 3)); break;
		case 105:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level * 4)); break;

		case 107:
			//Used on Reckless Strength, I think it should decay over time
			result = updownsign * (ubase + (caster_level / 2)); break;
		case 108:
			result = updownsign * (ubase + (caster_level / 3)); break;
		case 109:	// solar: confirmed 2/6/04
			result = updownsign * (ubase + (caster_level / 4)); break;

		case 110:	// solar: confirmed 2/6/04
			//is there a reason we dont use updownsign here???
			result = ubase + (caster_level / 5); break;

		case 111:
            result = updownsign * (ubase + 6 * (caster_level - GetMinLevel(spell))); break;
		case 112:
            result = updownsign * (ubase + 8 * (caster_level - GetMinLevel(spell))); break;
		case 113:
            result = updownsign * (ubase + 10 * (caster_level - GetMinLevel(spell))); break;
		case 114:
            result = updownsign * (ubase + 15 * (caster_level - GetMinLevel(spell))); break;

        //these formula were updated according to lucy 10/16/04
		case 115:	// solar: this is only in symbol of transal
			result = ubase + 6 * (caster_level - GetMinLevel(spell)); break;
		case 116:	// solar: this is only in symbol of ryltan
            result = ubase + 8 * (caster_level - GetMinLevel(spell)); break;
		case 117:	// solar: this is only in symbol of pinzarn
            result = ubase + 12 * (caster_level - GetMinLevel(spell)); break;
		case 118:	// solar: used in naltron and a few others
            result = ubase + 20 * (caster_level - GetMinLevel(spell)); break;

		case 119:	// solar: confirmed 2/6/04
			result = ubase + (caster_level / 8); break;
		case 121:	// solar: corrected 2/6/04
			result = ubase + (caster_level / 3); break;
		case 122: {
			int ticdif = spell->Get().buffduration - (ticsremaining-1);
			if(ticdif < 0)
				ticdif = 0;
			result = -(11 + 11*ticdif);
			break;
		}
		case 123:	// solar: added 2/6/04
			result = MakeRandomInt(ubase, abs(max));
			break;

		//these are used in stacking effects... formula unknown
		case 201:
		case 203:
			result = max;
			break;
		default:
			if (formula < 100)
				result = ubase + (caster_level * formula);
			//else
			//	LogFile->write(EQEMuLog::Debug, "Unknown spell effect value forumula %d", formula);
	}

	int oresult = result;

	// now check result against the allowed maximum
	if (max != 0)
	{
		if (updownsign == 1)
		{
			if (result > max)
				result = max;
		}
		else
		{
			if (result < max)
				result = max;
		}
	}

	// if base is less than zero, then the result need to be negative too
	if (base < 0 && result > 0)
		result *= -1;

	//mlog(SPELLS__EFFECT_VALUES, "Result: %d (orig %d), cap %d %s", result, oresult, max, (base < 0 && result > 0)?"Inverted due to negative base":"");

	return result;
}

//o--------------------------------------------------------------
//| GetMinLevel; Yeahlight, Nov 16, 2008
//o--------------------------------------------------------------
//| Adapted from EQEMU 7.0: Returns the lowest level of any 
//| caster which can use the spell
//o--------------------------------------------------------------
int Mob::GetMinLevel(Spell* spell) {
	int r;
	int min = 255;
	const SPDat_Spell_Struct &use = spell->Get();
	for(r = 0; r < TOTAL_CLASSES; r++) {
		if(use.classes[r] < min)
			min = use.classes[r];
	}
	
	//if we can't cast the spell return 0
	//just so it wont screw up calculations used in other areas of the code
	if(min == 255)
		return 0;
	else
		return(min);
}