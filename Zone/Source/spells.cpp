#include "client.h"
#include "spdat.h"
#include "packet_dump.h"
#include "moremath.h"
#include "MessageTypes.h"
#include "math.h"
#include "../include/playercorpse.h"
#include "../include/worldserver.h"
#include "../include/skills.h"
#include "SpellsHandler.hpp"
#include "groups.h" // Pinedepain // We need it for groups buff
#include "projectile.h"

#ifndef WIN32
//	#include <pthread.h>
	#include <cstdlib>
	#include "unix.h"
#endif
using namespace std;

extern Database				database;
extern Zone*				zone;
extern volatile bool		ZoneLoaded;
extern WorldServer			worldserver;
extern SpellsHandler		spells_handler;	// Better way of handling this.


//////////////////////////////////////////////////
// Spell Process.
void Mob::SpellProcess()
{
	if(this->GetCastingSpell() && spellend_timer->Check())
	{
		//Yeahlight: This is an NPC and their spell has finished, flag them as no longer casting
		if(this->IsNPC())
			this->CastToNPC()->SetIsCasting(false);
		spellend_timer->Disable();
		SpellFinished(this->GetCastingSpell(), this->GetCastingSpellTargetID(), this->GetCastingSpellSlot(), this->GetCastingSpellInventorySlot());
		//Yeahlight: Get the NPC moving as soon as possible if they are on a path
		if(this->IsNPC() && (this->CastToNPC()->GetOnPath() || this->CastToNPC()->GetOnRoamPath()))
		{
			this->CastToNPC()->setMoving(true);
			this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
		}
	}
}

//////////////////////////////////////////////////
//1. First function to be called.
void Mob::CastSpell(Spell* spell, TSpellID target_id, int16 slot, int32 cast_time, int16 inventorySlot)
{
	//Spells not loaded!
	if(!spells_handler.SpellsLoaded())
		return;

	//Yeahlight: Spell is not a valid spell
	if(spell == NULL || (spell && !spell->IsValidSpell()))
		return;

	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::CastSpell: Casting %s", spell->GetSpellName());

	//Yeahlight: Prevents a very well known exploit which allows a client to use an instant clicky item to bypass the recovery time of the global spellbar cooldown
	if(IsClient() && slot < SLOT_ITEMSPELL)
	{
		//Yeahlight: Cooldown timer has expired, so we need to reset it to 1ms to avoid a rare issue with fizzle reestablishing a repeat of an extended cooldown timer
		if(CastToClient()->spellCooldown_timer->Check())
		{
			CastToClient()->spellCooldown_timer->Start(1);
		}
		//Yeahlight: Client is not permitted to use a normal spell at this time
		else
		{
			Message(DARK_BLUE, "You haven't recovered yet...");
			EnableSpellBar(0);
			return;
		}
	}

	//Yeahlight: This spell is not ready to be used yet
	if(IsClient() && slot < SLOT_ITEMSPELL && CastToClient()->GetSpellRecastTimer(slot) > time(0))
	{
		Message(DARK_BLUE, "Your %s spell is not ready yet...", spell->GetSpellName());
		EnableSpellBar(0);
		return;
	}

	Mob* spellTarget = entity_list.GetMob(target_id);
	int32 mana_required	= spell->GetSpellManaRequired();
	int32 mana_available = GetMana();
	cast_time = cast_time == 0xFFFFFFFF ? spell->GetSpellCastTime() : cast_time;

	if(spellTarget == NULL)
	{
		spellTarget = this;
		target_id = GetID();
	}

	if(IsMesmerized())
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::CastSpell: You cannot cast while you do not have control of yourself!", spell->GetSpellName());
		if(IsClient())
			CastToClient()->Message(BLACK,"You cannot cast while you do not have control of yourself!");
		EnableSpellBar(0);
		return;
	}

	if(GetCastingSpell())
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::CastSpell: You're already casting another spell!", spell->GetSpellName());
		Message(RED, "You're already casting another spell!");
		return;
	}

	//Cofruben: good to check..
	if(IsClient() && mana_required > mana_available && slot < SLOT_ITEMSPELL)
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::CastSpell: insufficient mana to cast this spell!", spell->GetSpellName());
		Message(RED, "Insufficient mana to cast this spell!");
		EnableSpellBar(0);
		return;
	}

	if(!spell->IsSelfTargetSpell() && !spell->IsGroupSpell() && IsClient() && spellTarget && spellTarget->DistNoZ(this) > spell->GetSpellRange())
	{
		Message(RED, "Your target is out of range, get closer!");
		EnableSpellBar(0);
		return;
	}

	//Yeahlight: Assign this spell to the mob
	SetCastingSpell(spell, target_id, slot, GetX(), GetY(), inventorySlot);

	//We fizzled! Ouch.
	if(slot < SLOT_ITEMSPELL && IsClient() && CheckFizzle(spell))
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::CastSpell: Spell %s fizzled", spell->GetSpellName());
		InterruptSpell(true);
		SetMana(mana_available - mana_required);
		return;
	}

	//Yeahlight: Create and send the spell particle packet to nearby clients
	SendBeginCastPacket(spell->GetSpellID(), cast_time);
	
	//Yeahlight: If this NPC starts casting, stop their movement immediately and flag them as "casting"
	if(IsNPC())
	{
		CastToNPC()->FaceTarget(spellTarget);
		CastToNPC()->setMoving(false);
		CastToNPC()->SetIsCasting(true);
		SendPosUpdate(false, NPC_UPDATE_RANGE, false);
	}

	//Instant casting. No timer is needed so we get into SpellFinished directly.
	if(cast_time == 0)
	{
		SpellFinished(spell, target_id, slot,inventorySlot);
		return;
	}

	spellend_timer->Start(cast_time);
}

//////////////////////////////////////////////////
//2. Second function to be called, just when timer's up or there's no casting time.
void Mob::SpellFinished(Spell* spell, int32 target_id, int16 slot,int16 inventoryslot, bool weaponProc)
{
	//Spells not loaded!
	if(!spells_handler.SpellsLoaded())
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::SpellFinished(): spells are not loaded! ");
		return;
	}

	if(spell == NULL || (spell && !spell->IsValidSpell()))
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::SpellFinished(): spell won't be casted, you're a bad guy!");
		InterruptSpell();
		return;
	}

	// Harakiri remove a charge from an item
	if(IsClient() && slot == SLOT_ITEMSPELL && inventoryslot > 0) {
		CastToClient()->RemoveOneCharge(inventoryslot,true);
	}

	Mob* target = (target_id > 0) ? entity_list.GetMob(target_id) : NULL;
	int16 mana_used = (slot == SLOT_ITEMSPELL) ? 0 : spell->GetSpellManaRequired();
	int8 skill2 = spell->GetSpellSkill();
	TTargetType TargetType = spell->GetSpellTargetType();

	if(false)
	{
		cout<<"A: "<<(int)spell->Get().activated<<endl;
		cout<<"B: "<<(int)spell->Get().AEDuration<<endl;
		cout<<"C: "<<(int)spell->Get().aoerange<<endl;
		cout<<"D: ";
		for(int i = 0; i < 12; i++)
		{
			cout<<(int)spell->Get().base[i]<<" ";
		}
		cout<<endl;
		cout<<"E: "<<(int)spell->Get().basediff<<endl;
		cout<<"F: "<<(int)spell->Get().cast_time<<endl;
		cout<<"G: ";
		for(int i = 0; i < 15; i++)
		{
			cout<<(int)spell->Get().classes[i]<<" ";
		}
		cout<<endl;
		cout<<"H: ";
		for(int i = 0; i < 4; i++)
		{
			cout<<(int)spell->Get().component_counts[i]<<" ";
		}
		cout<<endl;
		cout<<"I: ";
		for(int i = 0; i < 4; i++)
		{
			cout<<(int)spell->Get().components[i]<<" ";	
		}
		cout<<endl;
		cout<<"II:";
		for(int i = 0; i < 12; i++)
		{
			cout<<(int)spell->Get().effectid[i]<<" ";	
		}
		cout<<endl;
		cout<<"J: "<<(int)spell->Get().good_effect<<endl;
		cout<<"K: "<<(int)spell->Get().icon<<endl;
		cout<<"L: "<<(int)spell->Get().light_type<<endl;
		cout<<"M: "<<(int)spell->Get().mana<<endl;
		cout<<"N: ";
		for(int i = 0; i < 12; i++)
		{
			cout<<(int)spell->Get().max[i]<<" ";
		}
		cout<<endl;
		cout<<"NN:"<<spell->Get().player_1<<endl;
		cout<<"O: "<<(int)spell->Get().pushback<<endl;
		cout<<"P: "<<(int)spell->Get().pushup<<endl;
		cout<<"Q: "<<(int)spell->Get().spell_range<<endl;
		cout<<"R: "<<(int)spell->Get().recast_time<<endl;
		cout<<"S: "<<(int)spell->Get().recovery_time<<endl;
		cout<<"T: "<<(int)spell->Get().resist_type<<endl;
		cout<<"U: "<<(int)spell->Get().skill<<endl;
		cout<<"V: "<<(int)spell->Get().spell_icon<<endl;
		cout<<"W: "<<(int)spell->Get().targettype<<endl;
		cout<<"X: "<<spell->Get().teleport_zone<<endl;
		cout<<"Y: ";
		for(int i = 0; i < 3; i++)
		{
			cout<<(int)spell->Get().unknown_1[i]<<" ";
		}
		cout<<endl;
		cout<<"Z: ";
		for(int i = 0; i < 4; i++)
		{
			cout<<(int)spell->Get().unknown_3[i]<<" ";
		}
		cout<<endl;
		cout<<"1: ";
		for(int i = 0; i < 4; i++)
		{
			cout<<(int)spell->Get().unknown_5[i]<<" ";
		}
		cout<<endl;
		cout<<"2: ";
		for(int i = 0; i < 28; i++)
		{
			cout<<(int)spell->Get().unknown_6[i]<<" ";
		}
		cout<<endl;
	}

	if(target)
	{
		//Yeahlight: Restrictions for NPC casting (range)
		if(this->IsNPC() && this->CastToNPC()->fdistance(GetX(), GetY(), target->GetX(), target->GetY()) > spell->GetSpellRange())
		{
			InterruptSpell();
			return;
		}
		//Yeahlight: PC is out of range of its target (range to target is ignored for self and PBAoE spells)
		//Melinko: Added check for Group Spells, fixes target out of range bug for group songs/spells
		if(this->IsClient() && !spell->IsSelfTargetSpell() && !spell->IsGroupSpell() && target->DistNoZ(this) > spell->GetSpellRange())
		{
			this->Message(RED, "Your target is out of range, get closer!");
			InterruptSpell(false, false);
			return;
		}
	}

	//Yeahlight: Special check for bind affinity
	if(spell->IsBindAffinitySpell())
	{
		bool permit = false;
		//Yeahlight: Target of the spell is the caster and may be bound in this zone
		if((!target || this == target) && zone->GetBindCondition() >= 1)
		{
			permit = true;
		}
		//Yeahlight: Target of the spell may be bound in the this zone and the two entities are grouped together
		else if(target && this != target && zone->GetBindCondition() == 2 && this->IsClient() && target->IsClient() && this->CastToClient()->IsGrouped() && entity_list.GetGroupByClient(this->CastToClient()) == entity_list.GetGroupByClient(target->CastToClient()))
		{
			permit = true;
		}
		//Yeahlight: The bind is not permitted
		if(!permit)
		{
			//Yeahlight: TODO: Research the real messages below
			//Yeahlight: Target is not the caster
			if(target && target != this)
			{
				target->Message(RED, "You may not bind here.");
				this->Message(RED, "Your target may not be bound here.");
			}
			else
			{
				this->Message(RED, "You may not bind here.");
			}
			InterruptSpell(false, true);
			return;
		}
	}

	//Yeahlight: Special check for levitation spells
	if(spell->IsLevitationSpell())
	{
		bool permit = false;
		//Yeahlight: Target of the spell is the caster and may levitate in this zone
		if((!target || this == target) && zone->GetLevCondition() >= 1)
		{
			permit = true;
		}
		//Yeahlight: Target of the spell may levitate in this zone
		else if(target && zone->GetLevCondition() >= 1)
		{
			permit = true;
		}
		if(!permit)
		{
			//Yeahlight: Target is not the caster
			this->Message(RED, "You can't levitate in this zone.");
			InterruptSpell(false, true);
			return;
		}
	}

	//Yeahlight: Special check for outdoor spells (NPCs may cast outdoor spells indoors)
	if(this->IsClient() && spell->IsOutDoorSpell() && !zone->GetOutDoorZone())
	{
		this->Message(RED, "You can't cast this spell indoors.");
		InterruptSpell(false, true);
		return;
	}

	//Tazadar : added target!=0 cause it makes zone.exe crash if you cast a spell with no target + check if its a corpse spell :)
	if(target && target->IsCorpse())
	{
		if(spell->GetSpellTargetType() == ST_Corpse)
		{
			bool rezFlag = false;
			for(int element=0; element < 12; element++)
				if(spell->GetSpellEffectID(element) == SE_Revive)
					rezFlag = true;
			if(!rezFlag)
			{
				InterruptSpell();
				return;
			}
		}
		//Yeahlight: No other spells are allowed to land on corpses, interrupt and consume mana for the rest
		else
		{
			InterruptSpell(false, true);
			if(IsClient())
				this->Message(RED, "You may not target a corpse with this spell.");
			return;
		}
	}

	//Yeahlight: Caster has moved significantly (3 units for PCs, 8 units for NPCs)
	//           TODO: 8 units for raid bosses may not be sufficient (See: Vox)
	if(this->DistanceToLocation(casting_spell_x_location, casting_spell_y_location) > (IsClient() ? 3 : 8))
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::SpellFinished(spell_name = %s): interrupting spell because you moved.", spell->GetSpellName());
		//Yeahlight: Mob is not a bard
		if(this->GetClass() != BARD)
		{ 
			InterruptSpell();
			return;
		}
		//Yeahlight: Bard mob is using an item
		else if(slot >= SLOT_ITEMSPELL) 
		{ 
			InterruptSpell();
			return;
		}
	}

	SetCastingSpell(NULL);

	if(this->GetCastingSpell() && this->GetCastingSpell() != spell)
	{
		this->CastToClient()->Message(RED, "You're already casting another spell!");
	}
	else 
	{
		APPLAYER* outapp = NULL;
		if(this->IsClient() && !IsBard() && slot < SLOT_ITEMSPELL)
		{
			// Pinedepain: We check if it's not a bard to avoid interrupting a song
			// Spell casting, not item.
			/* Dunno why this goes here, but it does */
			// Quagmire - Think this is to grey the memorized spell icons?
			outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
			memset(outapp->pBuffer, 0, outapp->size);
			MemorizeSpell_Struct* memspell = (MemorizeSpell_Struct*)outapp->pBuffer;
			memspell->slot = slot;
			memspell->spell_id = spell->GetSpellID();
			memspell->scribing = 3;
			this->CastToClient()->QueuePacket(outapp);
			safe_delete(outapp);//delete outapp;
		}

		// Pinedepain: Moved this part of code here because there's no bard animation when the bard doesn't have the required instrument
		// Pinedepain: If we are a bard using a song, the song recast itself in 6 seconds, with the same spell_id and target_id
		if(IsClient() && IsBard() && spell->IsBardSong())
		{
			// Pinedepain // If we have the required instrument, it's ok!
			if(CastToClient()->HasRequiredInstrument(spell->GetSpellID()))
			{
				spellend_timer->Start(6000);
				SetCastingSpell(spell, target_id);
			}

			// Pinedepain // Else, the spell does not land
			else
			{
				// Pinedepain // So we tell the client he can cast again, and reset everything before returning
				EnableSpellBar(spell->GetSpellID());
				StopSong();
				hitWhileCasting = false;

				// Pinedepain // Before returning, we tell the client what's happening
				switch (skill2)	// CFB* GetSpellSkill()
				{
					case SS_PERCUSSION:
					{
						this->CastToClient()->Message(DARK_BLUE,"You need to play a percussion instrument for this song.");
						break;
					}
					case SS_BRASS:
					{
						this->CastToClient()->Message(DARK_BLUE,"You need to play a brass instrument for this song.");
						break;
					}
					case SS_WIND:
					{
						this->CastToClient()->Message(DARK_BLUE,"You need to play a wind instrument for this song.");
						break;
					}
					case SS_STRING:
					{
						this->CastToClient()->Message(DARK_BLUE,"You need to play a stringed instrument for this song.");
						break;
					}
					default:
					{
						cout << "Mob::SpellFinished >> unknown spell_skill = " << (int)skill2 << endl;
						break;
					}
				}
				return;
			}

		}

		/* Client animation */
		// Pinedepain: If we're not a bard, the animation is the normal one
		if(!IsBard() || (IsBard() && !spell->IsBardSong()))
		{
			outapp = new APPLAYER(OP_Attack, sizeof(Attack_Struct));
			memset(outapp->pBuffer, 0, outapp->size);
			Attack_Struct* a = (Attack_Struct*)outapp->pBuffer;
			a->spawn_id = GetID();
			a->type = 42;
			a->a_unknown2[5] = 0x80;      
			a->a_unknown2[6] = 0x3f;       
			entity_list.QueueCloseClients(this, outapp);
			safe_delete(outapp);//delete outapp;
		}

		// Pinedepain // Else, if we're a bard, we call the "DoBardSongAnim" function to send the right anim to the client
		/*else
		{
			// Pinedepain // We only do that if we are a client, because while being a mob I think it would crash (never tried though), more comments
						  // are added to the declaration of DoBardSongAnim
			if (IsClient())
			{
				//DoBardSongAnim(spell->GetSpellID());
			}
		}*/

		// Pinedepain // If we're a bard and not using a song or a weapon proc, we need to know that we can cast again
		if((IsClient() && !IsBard()) || (IsClient() && IsBard() && !spell->IsBardSong())) 
		{
			/* Tell client it can cast again */
			/*outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
			ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
			manachange->new_mana = this->GetMana()-mana_used;
			manachange->spell_id = spell->GetSpellID();
			this->CastToClient()->QueuePacket(outapp);
			safe_delete(outapp);//delete outapp;*/
			
			sint32 tmpmana = this->GetMana();
			// Set our Mana now to Current Mana minus how much mana was used
			SetMana(tmpmana - mana_used);
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::SpellFinished(spell_name = %s): %i mana used, %i left", spell->GetSpellName(), mana_used, tmpmana - mana_used);
			EnableSpellBar();
		}

		// Pinedepain // Bard can be hit while casting without any channeling issues
		if(hitWhileCasting && !IsBard()) 
		{
			if(IsClient()) 
			{
				//cout<<"Channeling was a success!"<<endl;
				CastToClient()->Message(CAST_SPELL, "You regain your concentration and continue your casting!");
				if(slot < 10)
				{
					this->CastToClient()->CheckAddSkill(CHANNELING);
					//cout<<"Checking skillup"<<endl;
				}
			}
			else
			{
				entity_list.MessageClose(this, true, DEFAULT_MESSAGE_RANGE, CAST_SPELL, "%s's regains concentration and continues casting!", this->GetName());
			}
		}

		hitWhileCasting = false;

		// Cofruben: let's check if that spell requires any reagent. //Yeahlight: Ignore checks for weapon procs
		//Melinko: removed check for bard songs, we already checked to see if they were using correct instrument
		if (IsClient() && !weaponProc && !spell->IsBardSong())
		{
			int8 i = 0;
			int16 item_id;
			int16 slots[4] = {0, 0, 0, 0};
			int16 itemid[4] = {0, 0, 0, 0};
			for (i = 0; i < 4; i++)
			{
				item_id = spell->GetSpellComponent(i);
				if(item_id != 0xFFFF)
				{
					slots[i] = CastToClient()->FindItemInInventory(item_id);
					itemid[i]=item_id;
					if(slots[i] == 0)
					{						
						this->CastToClient()->SendItemMissing(item_id,1);						
						return;
					}
				}
			}
			// Ok we have every reagent needed, let's remove them
			for(i = 0; i < 4; i++)
			{
				this->CastToClient()->RemoveOneCharge(slots[i],true);
			}
		}

		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::SpellFinished(spell_name = %s): spell target type: %i", spell->GetSpellName(), (int)TargetType);

		switch(TargetType)
		{
			case ST_Tap:
			case ST_Target: 
			{					
				if(target && target != this && this->IsClient() && !CheckCoordLos(GetX(), GetY(), GetZ(), target->GetX(), target->GetY(), target->GetZ()))
				{
					if(this->IsClient())
						this->CastToClient()->Message(RED,"You cannot see your target from here!");
					return;
				}
				if(target)
				{
					if(!spell->IsBeneficialSpell() && this != target && this->IsClient()
						&& target->IsClient() && !this->CastToClient()->CanAttackTarget(target)
						&& !this->CastToClient()->GetPlayerProfilePtr()->gm) // Of course GM's can cast on anything =)
						return; // Cofruben: avoid pvp without consent.
					SpellOnTarget(spell, target);
				}
				else
				{
					if(this->IsClient())
						this->CastToClient()->Message(RED, "Error: Spell requires a target");
				}
				break;
			}
			case ST_Self:
			{
				SpellOnTarget(spell, this);
				break;
			}
			case ST_AECaster:
			{
				entity_list.AESpell(this, GetX(), GetY(), GetZ(), spell->GetSpellAOERange(), spell);
				break;
			}
			case ST_AETarget:
			{
				if(target)
				{
					entity_list.AESpell(this, target->GetX(), target->GetY(), target->GetZ(), spell->GetSpellAOERange(), spell);
					//Yeahlight: Spell is a rain spell
					//			 TODO: I am assuming no NPCs in the game use rain spells
					//			 TODO: I am assuming all rain spells pulse exactly three times
					//			 TODO: 1000ms may not be appropriate for all spell checks
					if(IsClient() && spell->GetSpellAEDuration() > 1000)
					{
						//Yeahlight: Add this rain spell to the PC's rain list and if it was not successful then throw an error
						if(CastToClient()->AddSpellToRainList(spell, 2, target->GetX(), target->GetY(), target->GetZ()) == false && CastToClient()->GetDebugMe())
						{
							Message(RED, "Debug: An error occured while trying to add this rain spell to your rain list.");
						}
						else
						{
							CastToClient()->rain_timer->Start();
						}
					}
				}
				else if(IsClient())
				{
					CastToClient()->Message(RED, "Error: Spell requires a target");
				}
				break;
			}
			case ST_Group:
			{
				// Pinedepain // Working on group spell
				if(IsClient())
				{
					CastToClient()->SpellOnGroup(spell,target_id);
				}
				break;
			}
			case ST_Pet:
			{
				if (this->GetPetID() != 0) {
					SpellOnTarget(spell, entity_list.GetMob(this->GetPetID()));
					if (spell->GetSpellID() == 331) {
						Mob* mypet = entity_list.GetMob(this->GetPetID());
						if (mypet && mypet->IsNPC() && mypet->GetPetType() != 0xFF) {
							mypet->CastToNPC()->Depop();
							this->SetPet(0);
						}
					}
				}
				else {
					this->CastToClient()->Message(RED, "You don't have a pet to cast this on!");
				}
				break;
			}
			case ST_Corpse: 
			{
				if(!target->IsPlayerCorpse()) {
					if (this->IsClient())
						this->CastToClient()->Message(RED, "Target is not a corpse.");
					break;
				}
				SpellOnTarget(spell, this);  // Cofruben: is 'this' ok? shouldn't it be target? , Tazadar: We have a prob with our spell.dat so we do like that
				break;
			}
			// Pinedepain // If the target type is a group song one
			case ST_GroupSong: // Melinko: changed to GroupSong from GroupTeleport
			{
				// Pinedepain // Working on group spell
				if (IsClient())
				{
					CastToClient()->SpellOnGroup(spell, target_id);
				}
				break;
			}
			case ST_Undead: // Cofruben: added 16/08/08.
			{
				if(target != this && this->IsClient() && !CheckCoordLos(GetX(), GetY(), GetZ(), target->GetX(), target->GetY(), target->GetZ()))
				{
					this->CastToClient()->Message(RED,"You cannot see your target from here!");
					return;
				}
				if (target->GetBodyType() != BT_Undead) {
					this->CastToClient()->Message(RED, "Error: Target is not undead.");
					return;
				}
				if (target)
					SpellOnTarget(spell, target);
				else
					this->CastToClient()->Message(RED, "Error: Spell requires a target");
				break;
			}
			case ST_Animal: // Cofruben: added 16/08/08.
			{
				if(target != this && this->IsClient() && !CheckCoordLos(GetX(), GetY(), GetZ(), target->GetX(), target->GetY(), target->GetZ()))
				{
					this->CastToClient()->Message(RED,"You cannot see your target from here!");
					return;
				}
				if (target->GetBodyType() != BT_Animal) {
					this->CastToClient()->Message(RED, "Error: Target is not an animal.");
					return;
				}
				if (target)
					SpellOnTarget(spell, target);
				else
					this->CastToClient()->Message(RED, "Error: Spell requires a target");
				break;
			}
			case ST_Summoned:	// Cofruben: added 25/08/08.
			{
				if(target != this && this->IsClient() && !CheckCoordLos(GetX(), GetY(), GetZ(), target->GetX(), target->GetY(), target->GetZ()))
				{
					this->CastToClient()->Message(RED,"You cannot see your target from here!");
					return;
				}
				if (target->GetBodyType() != BT_Summoned) {
					this->CastToClient()->Message(RED, "Error: Target is not summoned.");
					return;
				}
				if (target)
					SpellOnTarget(spell, target);
				else
					this->CastToClient()->Message(RED, "Error: Spell requires a target");
				break;
			}
			//Yeahlight: Bolt spells
			case ST_LineOfSight:
			{
				if(target)
					SpawnProjectile(this, target, spell);
				else
					SpawnProjectile(this, this, spell);
				break;
			}
			default:
			{
				if (IsClient())
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::SpellFinished(spell_name = %s): unknown target type: %i", spell->GetSpellName(), TargetType);
				break;
			}
		}
	}

	//Yeahlight: If the target of the determental spell is currently FD'ed, reverse it for them!
	if(target && target->IsClient() && spell->IsDetrimentalSpell() && target->CastToClient()->GetFeigned())
		target->CastToClient()->SetFeigned(false);

	//Yeahlight: PC caster has used a normal spell slot for this spell
	if(IsClient() && slot < SLOT_ITEMSPELL)
	{
		int32 recoveryTime = spell->GetSpellRecoveryTime();
		int32 recastTime = spell->GetSpellRecastTime();

		//Yeahlight: Start the global spell bar cooldown timer
		CastToClient()->spellCooldown_timer->Start(recoveryTime);
		//Yeahlight: Start the individual spell recast timer
		CastToClient()->SetSpellRecastTimer(slot, recastTime);

		//Yeahlight: Recovery time is over the default expected value (2.25 seconds)
		//if(recoveryTime > 2250)
		//{
		//	
		//}
		////Yeahlight: Recast time is over the default expected value (2.25 seconds)
		//if(spell->GetSpellRecastTime() > 2250)
		//{
		//	APPLAYER* outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
		//	memset(outapp->pBuffer, 0, outapp->size);
		//	MemorizeSpell_Struct* memspell = (MemorizeSpell_Struct*)outapp->pBuffer;
		//	memspell->slot = slot;
		//	memspell->spell_id = spell->GetSpellID();
		//	memspell->scribing = 3;
		//	this->CastToClient()->QueuePacket(outapp);
		//	safe_delete(outapp);//delete outapp;
		//}
	}
}

//o--------------------------------------------------------------
//| SpawnProjectile; Yeahlight, Aug 20, 2009
//o--------------------------------------------------------------
//| Spawns a spell based projectile
//o--------------------------------------------------------------
void Mob::SpawnProjectile(Mob* inflictor, Mob* inflictee, Spell* spell)
{
	//Yeahlight: A target, source or spell was not supplied; abort
	if(inflictor == NULL || inflictee == NULL || spell == NULL)
		return;

	//Yeahlight: Create the new projectile object
	Projectile* p = new Projectile();
	p->SetX(inflictor->GetX());
	p->SetY(inflictor->GetY());
	p->SetZ(inflictor->GetZ());
	p->SetVelocity(0.50);
	p->SetHeading(inflictor->GetHeading() * 2);
	p->SetSpellID(spell->GetSpellID());
	p->SetType(9);
	p->SetSourceID(inflictor->GetID());
	p->SetTexture("GENW00");
	p->SetLightSource(4);
	//Yeahlight: The source and destination are the same
	if(inflictor == inflictee)
	{
		p->SetTargetID(0);
		p->SetYaw(0.001);
		p->BuildCollisionList(NULL, 75);
	}
	else
	{
		p->SetTargetID(inflictee->GetID());
		p->SetYaw(0);
		p->BuildCollisionList(inflictee, 500);
	}
	entity_list.AddProjectile(p);
	p->BuildProjectile(inflictor);
}

//o--------------------------------------------------------------
//| SpawnProjectile; Yeahlight, Aug 20, 2009
//o--------------------------------------------------------------
//| Spawns an item based projectile
//o--------------------------------------------------------------
void Mob::SpawnProjectile(Mob* inflictor, Mob* inflictee, Item_Struct* item)
{
	//Yeahlight: A target or source was not supplied; abort
	if(inflictor == NULL || inflictee == NULL)
		return;

	//Yeahlight: The source and destination are the same
	if(inflictor == inflictee)
	{
		
	}
	//Yeahlight: The destination target differs from the source
	else
	{
		
	}
}

//////////////////////////////////////////////////

void Mob::SpellOnTarget(Spell* spell, Mob* spelltar)
{
	//Spells not loaded!
	if(spells_handler.SpellsLoaded() == false)
		return;

	//Yeahlight: No valid spell was supplied; abort
	if(spell == NULL || (spell && !spell->IsValidSpell()))
		return;

	//Yeahlight: Never target the caster with their own PBAoE spell
	if(spelltar == this && spell->GetSpellTargetType() == ST_AECaster)
		return;

	//Yeahlight: No spell target was supplied
	if(spelltar == NULL)
	{
		//Yeahlight: Caster is a PC
		if(IsClient())
		{
			CastToClient()->Message(RED, "You must have a target for this spell.");
		}
		return;
	}

	//Cofruben: Here we check if spell is allowed to land.
	if(spells_handler.WillSpellLand(this, spelltar, spell) == false)
	{
		//Yeahlight: Caster is a PC in debug mode
		if(IsClient() && CastToClient()->GetDebugMe())
		{
			CastToClient()->Message(YELLOW, "Debug: Your %s spell is not permitted to land on %s", spell->GetSpellName(), spelltar->GetName());
		}
		return;
	}

	//Yeahlight: Check for stacking conflicts (-1 is a conflict)
	if(spelltar->AddBuff(this, spell) == -1)
		return;

	//Yeahlight: GM is currently immune to spells
	if(spelltar->IsClient() && spelltar->CastToClient()->GetSpellImmunity() && (spell->IsDetrimentalSpell() || spell->IsUtilitySpell()))
	{
		HandleResistedSpell(spell, this, spelltar);
		return;
	}

	bool partialResist = false;
	//Yeahlight: Check for spell resists on detrimental/utility resistable spells
	if(spell->IsResistableSpell() && (spell->IsDetrimentalSpell() || spell->IsUtilitySpell()))
	{
		int16 resistValue = spells_handler.CalcResistValue(spell, spelltar, this);
		//Yeahlight: Spell was resisted
		if(rand()%100 + 1 < resistValue)
		{
			//Yeahlight: 20% chance to grant the caster a partial nuke on direct damage spells if the resist value was 50% or less
			//           TODO: I made this formula up; research it
			if(resistValue <= 50 && spell->IsPureNukeSpell() && rand()%100 < 20)
			{
				partialResist = true;
			}
			//Yeahlight: Full resist
			else
			{
				HandleResistedSpell(spell, this, spelltar, RM_None, resistValue == IMMUNE_RESIST_FLAG);
				return;
			}
		}
	}

	//Yeahlight: Check the level restriction on this charm spell
	if(spell->IsCharmSpell() && GetCharmLevelCap(spell) < spelltar->GetLevel())
	{
		HandleResistedSpell(spell, this, spelltar, RM_CharmTargetTooHigh);
		return;
	}

	//Yeahlight: Check for level restriction on this mez spell
	if(spell->IsMezSpell() && GetMezLevelCap(spell) < spelltar->GetLevel())
	{
		HandleResistedSpell(spell, this, spelltar, RM_MezTargetTooHigh);
		return;
	}

	//Yeahlight: Display the casting message for all nearby clients
	CreateStartingCastingPacket(spell, this, spelltar);

	//Yeahlight: Send the icon and animation packets to all nearby clients
	CreateEndingCastingPacket(spell, this, spelltar);

	//Yeahlight: Apply the spell's effects to the target
	spelltar->SpellEffect(this, spell, this->GetLevel(), partialResist);
}

//o--------------------------------------------------------------
//| CreateStartingCastingPacket; Yeahlight, Aug 25, 2009
//o--------------------------------------------------------------
//| Creates the starting spell casting packet
//o--------------------------------------------------------------
void Mob::CreateStartingCastingPacket(Spell* spell, Mob* caster, Mob* spelltar)
{
	//Yeahlight: No valid spell, target or caster was supplied; abort
	if(spell == NULL || (spell && !spell->IsValidSpell()) || caster == NULL || target == NULL)
		return;

	APPLAYER* outapp = new APPLAYER(OP_Action, sizeof(Action_Struct));
	Action_Struct* action = (Action_Struct*)outapp->pBuffer;
	action->damage = 0;
	action->spell = spell->GetSpellID();
	action->source = (caster->IsClient() && caster->CastToClient()->GMHideMe()) ? 0 : caster->GetID();
	action->target = spelltar->GetID();
	action->type = 0xE7;
	entity_list.QueueCloseClients(spelltar, outapp, false, 250, caster);
	if(caster->IsClient())
		caster->CastToClient()->QueuePacket(outapp);
	safe_delete(outapp);
}

//o--------------------------------------------------------------
//| CreateEndingCastingPacket; Yeahlight, Aug 25, 2009
//o--------------------------------------------------------------
//| Creates the starting spell casting packet
//o--------------------------------------------------------------
void Mob::CreateEndingCastingPacket(Spell* spell, Mob* caster, Mob* spelltar)
{
	//Yeahlight: No valid spell, target or caster was supplied; abort
	if(spell == NULL || (spell && !spell->IsValidSpell()) || caster == NULL || target == NULL)
		return;

	APPLAYER* outapp = new APPLAYER(OP_CastOn, sizeof(CastOn_Struct));
	memset(outapp->pBuffer, 0, sizeof(CastOn_Struct));
	CastOn_Struct* caston = (CastOn_Struct*) outapp->pBuffer;
	if(caster->IsClient() && caster->CastToClient()->GMHideMe())
		caston->source_id = 0;
	else
		caston->source_id = caster->GetID();
	caston->target_id = spelltar->GetID();
	// Harakiri Bind Sight needs to switch targets
	// target is now source
	// because we are watching through the eyes of the target
	// The client "remembers" the original target somehow
	// Also, you cannot click this off when using #castspell
	// it works fine, if its cast from a mem spell slot
	if(spell->IsEffectInSpell(SE_BindSight))
	{	
		caston->source_id = caster->GetID();
		caston->target_id = caster->GetID();		
	}
	caston->action = 231;
	caston->spell_id = spell->GetSpellID();
	caston->heading = caster->GetHeading() * 2;
	caston->source_level = caster->GetLevel();
	caston->unknown1[1] = 0x41;
	// Pinedepain // If we'r a bard using a song, we have to get the applied instrument mod
	if(caster->IsBard() && spell->IsBardSong() && caster->IsClient())
		caston->unknown1[3] = caster->CastToClient()->GetInstrumentModAppliedToSong(spell);
	// Pinedepain // Else, it's 0x0A
	else
		caston->unknown1[3] = 0x0A;
	caston->unknown2[0] = 0x00;
	caston->unknown2[1] = 0x04; //Cofruben: this makes the buff to appear.
	entity_list.QueueCloseClients(spelltar, outapp, false, 250, caster); // send target and people near target
	if(caster->IsClient())
		caster->CastToClient()->QueuePacket(outapp); // send to caster of the spell
	safe_delete(outapp);
}

//o--------------------------------------------------------------
//| HandleResistedSpell; Yeahlight, Aug 25, 2009
//o--------------------------------------------------------------
//| Handles all things associated with resisted spells
//o--------------------------------------------------------------
void Mob::HandleResistedSpell(Spell* spell, Mob* caster, Mob* spelltar, ResistMessage type, bool immunityMessage)
{
	//Yeahlight: Spell, caster or target are null; abort
	if(spell == NULL || caster == NULL || target == NULL)
		return;

	//Yeahlight: The PC caster requires a resist message (immunity messages are handled elsewhere)
	if(caster->IsClient() && immunityMessage == false)
	{
		//Yeahlight: A special message flag was supplied
		if(type)
		{
			//Yeahlight: Switch based on the resist message type
			//           TODO: These messages are wrong; research the correct ones
			switch(type)
			{
				case RM_CharmTargetTooHigh:
				{
					caster->Message(RED, "Your target cannot be charmed with this spell!");
					break;
				}
				case RM_MezTargetTooHigh:
				{
					caster->Message(RED, "Your target cannot be mesmerized with this spell!");
					break;
				}
				default:
				{
					caster->Message(RED, "ERROR: No resist message defined for ResistMessage(%i)", type);
					break;
				}
			}
		}
		else
		{
			caster->Message(RED, "Your target has resisted your %s spell!", spell->GetSpellName());
		}
	}
	
	//Yeahlight: The PC target requires a resist message
	if(spelltar->IsClient())
		spelltar->Message(RED, "You resist the %s spell!", spell->GetSpellName());
	//Yeahlight: The NPC target requires a hate addition
	else if(spelltar->IsNPC())
		spelltar->CastToNPC()->AddToHateList(caster, 0, GetSpellHate(spell, true));

	//Yeahlight: False landing packet
	APPLAYER* outapp = new APPLAYER(OP_CastOn, sizeof(CastOn_Struct));
	memset(outapp->pBuffer, 0, sizeof(CastOn_Struct));
	CastOn_Struct* caston = (CastOn_Struct*) outapp->pBuffer;
	caston->source_id = caster->GetID();
	caston->target_id = spelltar->GetID();
	caston->action = 231;
	caston->spell_id = spell->GetSpellID();
	caston->heading = caster->GetHeading() * 2;
	caston->source_level = caster->GetLevel();
	caston->unknown1[1] = 0x41;
	caston->unknown1[3] = 0x0A;
	caston->unknown2[0] = 0x00;
	//Yeahlight: Anything but 0x04 will prevent the client from placing a debuff icon on the player
	caston->unknown2[1] = 0x00;
	entity_list.QueueCloseClients(spelltar, outapp, false);
	safe_delete(outapp);
}

//////////////////////////////////////////////////
// Hogie - Stuns "this"
void Client::Stun(int16 duration, Mob* stunner, bool melee)
{
	if(this->IsInvulnerable())
		return;
	//Yeahlight: Ogres are immune to frontal, melee stuns
	if(GetBaseRace() == OGRE && melee && stunner)
	{
		//Yeahlight: Ogre PC can see its target
		if(!this->CanNotSeeTarget(stunner->GetX(), stunner->GetY()))
			return;
	}
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::Stun(duration = %i): ", duration);
	APPLAYER* outapp = new APPLAYER(OP_Stun, sizeof(Stun_Struct));
	Stun_Struct* stunon = (Stun_Struct*) outapp->pBuffer;
	stunon->duration = duration;
	this->CastToClient()->QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
	stunned_timer->Start(duration);
	stunned = true;
}

//////////////////////////////////////////////////

void NPC::Stun(int16 duration, Mob* stunner, bool melee)
{
	//Yeahlight: NPCs level 55 and above are immune to stun
	if(this->IsInvulnerable() || this->GetLevel() >= 55)
		return;
	//Yeahlight: Ogres are immune to frontal, melee stuns
	if(GetBaseRace() == OGRE && melee && stunner)
	{		
		//Yeahlight: Ogre NPC can see its target
		if(!this->CanNotSeeTarget(stunner->GetX(), stunner->GetY()))
			return;
	}
	Stunned(duration);
}

//////////////////////////////////////////////////

void NPC::Stunned(int16 duration)
{
	this->stunned = true;
	stunned_timer->Start(duration);
}

//////////////////////////////////////////////////

void Corpse::CastRezz(APPLAYER* pApp, int16 spellid, Mob* caster)
{
	//Yeahlight: Caster was not supplied; abort
	if(caster == NULL)
		return;

	if(caster->IsClient() && caster->CastToClient()->GetDebugMe())
		caster->Message(YELLOW, "Debug: Attempting to contact %s for the resurrection", orgname);
	worldserver.RezzPlayer(pApp, orgname, GetRezExp(), OP_RezzRequest, dbid);
}

//////////////////////////////////////////////////
//Tazadar: Fixed several things on this and now it seems to work ! :)
void Client::MakeBuffFadePacket(Spell* spell, int8 slot_id)
{
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::MakeBuffFadePacket(spell_name = %s)", spell->GetSpellName());
	APPLAYER* outapp = new APPLAYER(OP_Buff, sizeof(SpellBuffFade_Struct));
	memset(outapp->pBuffer, 0, outapp->size);
	SpellBuffFade_Struct* sbf = (SpellBuffFade_Struct*) outapp->pBuffer;
	sbf->playerid=this->GetID();
    sbf->unknown000[0] = 0x02;
	sbf->unknown000[1] = 0x07;
	sbf->unknown000[2] = 0x0A;
	sbf->fade = 0x01;
	sbf->spellid = spell->GetSpellID();
	sbf->slotid = slot_id;
	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
	Save();
}

//////////////////////////////////////////////////

void Client::SetBindPoint()
{
	strcpy(pp.bind_point_zone, zone->GetShortName());
	memset(pp.bind_location, 0, sizeof(pp.bind_location));
	//Yeahlight: Client location is backwards! (Y, X, Z)
	pp.bind_location[1][1] = (sint32)GetX();
	pp.bind_location[0][1] = (sint32)GetY();
	pp.bind_location[2][1] = (float)GetZ();
	Save();
	if(GetDebugMe())
		Message(LIGHTEN_BLUE, "Debug: Binding you to (%f, %f, %f) in %s", pp.bind_location[1][1], pp.bind_location[0][1], pp.bind_location[2][1], pp.bind_point_zone);
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Mob::SetBindPoint(): (%f, %f, %f) in %s", pp.bind_location[1][1], pp.bind_location[0][1], pp.bind_location[2][1], pp.bind_point_zone);
}

//////////////////////////////////////////////////

//Yeahlight: Surprise! Gate is resolved client side, but we still must prepare the PC for potential cross-zone transit
void Client::GoToBind(bool death)
{
	char* zone_name = this->GetPlayerProfilePtr()->bind_point_zone;
	//Yeahlight: Cross-zone transit; prepare the PC for zoning
	if(death || strcmp(zone_name, zone->GetShortName()) != 0)
	{
		//Yeahlight: Client location is backwards! (Y, X, Z)
		this->zoningX = (sint32)GetPlayerProfilePtr()->bind_location[1][1];
		this->zoningY = (sint32)GetPlayerProfilePtr()->bind_location[0][1];
		this->zoningZ = (float)GetPlayerProfilePtr()->bind_location[2][1];
		this->tempHeading = GetHeading();
		this->usingSoftCodedZoneLine = true;
		this->isZoning = true;
	}
}

//////////////////////////////////////////////////

const char* Mob::GetRandPetName()
{
	char* petreturn = 0;
	char petnames[77][32] = { "Gabeker","Gann","Garanab","Garn","Gartik","Gebann","Gebekn","Gekn","Geraner","Gobeker","Gonobtik","Jabantik","Jasarab","Jasober","Jeker","Jenaner","Jenarer","Jobantik","Jobekn","Jonartik","Kabann","Kabartik","Karn","Kasarer","Kasekn","Kebekn","Keber","Kebtik","Kenantik","Kenn","Kentik","Kibekab","Kobarer","Kobobtik","Konaner","Konarer","Konekn","Konn","Labann","Lararer","Lasobtik","Lebantik","Lebarab","Libantik","Libtik","Lobn","Lobtik","Lonaner","Lonobtik","Varekab","Vaseker","Vebobab","Venarn","Venekn","Vener","Vibobn","Vobtik","Vonarer","Vonartik","Xabtik","Xarantik","Xarar","Xarer","Xeber","Xebn","Xenartik","Xeratik","Xesekn","Xonartik","Zabantik","Zabn","Zabeker","Zanab","Zaner","Zenann","Zonarer","Zonarn" };
	petreturn = petnames[rand() % 77];
	//printf("Using %s\n", petreturn);
	return petreturn;
}

//////////////////////////////////////////////////
//Tazadar 06/01/08 modified this function in order to use the Database to load the pets infos
void Mob::MakePet(const char* pettype) {
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Mob::MakePet(pettype = %s)", pettype);
	Make_Pet_Struct petstruct;

	if (strncmp(pettype, "SumEarthR", 9) == 0) {
		int8 tmp = atoi(&pettype[9]);
		//cout << "number of the earth pet "<< (int)tmp << endl;
		if (tmp >= 2 && tmp <= 14) {
			switch (tmp) { 
				 case  2:
					Database::Instance()->MakePet(&petstruct,74,1);
					break;
				 case  3:
					Database::Instance()->MakePet(&petstruct,75,1);
					break;
				 case  4:
					Database::Instance()->MakePet(&petstruct,76,1);
					break;
				 case  5:
					Database::Instance()->MakePet(&petstruct,77,1);
					break;
				 case  6:
					Database::Instance()->MakePet(&petstruct,78,1);
					break;
				 case  7:
					Database::Instance()->MakePet(&petstruct,79,1);
					break;
				 case  8:
					Database::Instance()->MakePet(&petstruct,80,1);
					break;
				 case  9:
					Database::Instance()->MakePet(&petstruct,81,1);
					break;
				 case 10:
					Database::Instance()->MakePet(&petstruct,82,1);
					break;
				 case 11:
					Database::Instance()->MakePet(&petstruct,83,1);
					break;
				 case 12:
					Database::Instance()->MakePet(&petstruct,84,1);
					break;
				 case 13:
					Database::Instance()->MakePet(&petstruct,85,1);
					break;
				 case 14:
					Database::Instance()->MakePet(&petstruct,86,1);
					break;
			}
		}
		else {
			this->CastToClient()->Message(BLACK, "Error: Unknown Earth Pet formula");
		}
		MakePet(petstruct.level,petstruct.class_, petstruct.race, petstruct.texture, petstruct.pettype , petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg,petstruct.max_hp);
	}
	else if (strncmp(pettype, "SumFireR", 8) == 0) {
		int8 tmp = atoi(&pettype[8]);
		//cout << "number of the fire pet "<< (int)tmp << endl;
		if (tmp >= 2 && tmp <= 14) {
			switch (tmp) { 
			 case  2:
				Database::Instance()->MakePet(&petstruct,88,1);
				break;
			 case  3:
				Database::Instance()->MakePet(&petstruct,89,1);
				break;
			 case  4:
				Database::Instance()->MakePet(&petstruct,90,1);
				break;
			 case  5:
				Database::Instance()->MakePet(&petstruct,91,1);
				break;
			 case  6:
				Database::Instance()->MakePet(&petstruct,92,1);
				break;
			 case  7:
				Database::Instance()->MakePet(&petstruct,93,1);
				break;
			 case  8:
				Database::Instance()->MakePet(&petstruct,94,1);
				break;
			 case  9:
				Database::Instance()->MakePet(&petstruct,95,1);
				break;
			 case 10:
				Database::Instance()->MakePet(&petstruct,96,1);
				break;
			 case 11:
				Database::Instance()->MakePet(&petstruct,97,1);
				break;
			 case 12:
				Database::Instance()->MakePet(&petstruct,98,1);
				break;
			 case 13:
				Database::Instance()->MakePet(&petstruct,99,1);
				break;
			 case 14:
				Database::Instance()->MakePet(&petstruct,100,1);
				break;
		}
		}
		else {
			this->CastToClient()->Message(BLACK, "Error: Unknown Fire Pet formula");
		}
		MakePet(petstruct.level,petstruct.class_, petstruct.race, petstruct.texture, petstruct.pettype , petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg,petstruct.max_hp);
	}
	else if (strncmp(pettype, "SumAirR", 7) == 0) {
		int8 tmp = atoi(&pettype[7]);
		//cout << "number of the air pet "<< (int)tmp << endl;
		if (tmp >= 2 && tmp <= 14) {
			switch (tmp) {
			 case  2:
				Database::Instance()->MakePet(&petstruct,60,1);
				break;
			 case  3:
				Database::Instance()->MakePet(&petstruct,61,1);
				break;
			 case  4:
				Database::Instance()->MakePet(&petstruct,62,1);
				break;
			 case  5:
				Database::Instance()->MakePet(&petstruct,63,1);
				break;
			 case  6:
				Database::Instance()->MakePet(&petstruct,64,1);
				break;
			 case  7:
				Database::Instance()->MakePet(&petstruct,65,1);
				break;
			 case  8:
				Database::Instance()->MakePet(&petstruct,66,1);
				break;
			 case  9:
				Database::Instance()->MakePet(&petstruct,67,1);
				break;
			 case 10:
				Database::Instance()->MakePet(&petstruct,68,1);
				break;
			 case 11:
				Database::Instance()->MakePet(&petstruct,69,1);
				break;
			 case 12:
				Database::Instance()->MakePet(&petstruct,70,1);
				break;
			 case 13:
				Database::Instance()->MakePet(&petstruct,71,1);
				break;
			 case 14:
				Database::Instance()->MakePet(&petstruct,72,1);
				break;

		}
		}
		else {
			this->CastToClient()->Message(BLACK, "Error: Unknown Air Pet formula");
		}
		MakePet(petstruct.level,petstruct.class_, petstruct.race, petstruct.texture, petstruct.pettype , petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg,petstruct.max_hp);
	}
	else if (strncmp(pettype, "SumWaterR", 9) == 0) {
		int8 tmp = atoi(&pettype[9]);
		//cout << "number of the water pet "<< (int)tmp << endl;
		if (tmp >= 2 && tmp <= 14) {
			switch (tmp) {
			 case  2:
				Database::Instance()->MakePet(&petstruct,102,1);
				break;
			 case  3:
				Database::Instance()->MakePet(&petstruct,103,1);
				break;
			 case  4:
				Database::Instance()->MakePet(&petstruct,104,1);
				break;
			 case  5:
				Database::Instance()->MakePet(&petstruct,105,1);
				break;
			 case  6:
				Database::Instance()->MakePet(&petstruct,106,1);
				break;
			 case  7:
				Database::Instance()->MakePet(&petstruct,107,1);
				break;
			 case  8:
				Database::Instance()->MakePet(&petstruct,108,1);
				break;
			 case  9:
				Database::Instance()->MakePet(&petstruct,109,1);
				break;
			 case 10:
				Database::Instance()->MakePet(&petstruct,110,1);
				break;
			 case 11:
				Database::Instance()->MakePet(&petstruct,111,1);
				break;
			 case 12:
				Database::Instance()->MakePet(&petstruct,112,1);
				break;
			 case 13:
				Database::Instance()->MakePet(&petstruct,113,1);
				break;
			 case 14:
				Database::Instance()->MakePet(&petstruct,114,1);
				break;
		}
		}
		else {
			this->CastToClient()->Message(BLACK, "Error: Unknown Water Pet formula");
		}
		MakePet(petstruct.level,petstruct.class_, petstruct.race, petstruct.texture, petstruct.pettype , petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg,petstruct.max_hp);
	}
	else if (strncmp(pettype, "SpiritWolf", 10) == 0) {
		int8 tmp = atoi(&pettype[10]);
		//cout << "number of the spirit pet "<< (int)tmp << endl;
		switch (tmp) {
		case 242:
			Database::Instance()->MakePet(&petstruct,45,3);
			break;
		case 237:
			Database::Instance()->MakePet(&petstruct,44,3);
			break;
		case 234:
			Database::Instance()->MakePet(&petstruct,43,3);
			break;
		case 230:
			Database::Instance()->MakePet(&petstruct,42,3);
			break;
		case 227:
			Database::Instance()->MakePet(&petstruct,41,3);
			break;
		case 224:
			Database::Instance()->MakePet(&petstruct,40,3);
			break;
		}
		MakePet(petstruct.level,petstruct.class_, petstruct.race, petstruct.texture, petstruct.pettype , petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg,petstruct.max_hp);
	}
	else if (strncmp(pettype, "Animation", 9) == 0) {
		int8 ptype = atoi(&pettype[9]);

		switch ( ptype ) {
		case 14:
			Database::Instance()->MakePet(&petstruct,59,2);
			break;
		case 13:
			Database::Instance()->MakePet(&petstruct,58,2);
			break;
		case 12:
			Database::Instance()->MakePet(&petstruct,57,2);
			break;
		case 11:
			Database::Instance()->MakePet(&petstruct,56,2);
			break;
		case 10:
			Database::Instance()->MakePet(&petstruct,55,2);
			break;
		case 9:
			Database::Instance()->MakePet(&petstruct,54,2);
			break;
		case 8:
			Database::Instance()->MakePet(&petstruct,53,2);
			break;
		case 7:
			Database::Instance()->MakePet(&petstruct,52,2);
			break;
		case 6:
			Database::Instance()->MakePet(&petstruct,51,2);
			break;
		case 5:
			Database::Instance()->MakePet(&petstruct,50,2);
			break;
		case 4:
			Database::Instance()->MakePet(&petstruct,49,2);
			break;
		case 3:
			Database::Instance()->MakePet(&petstruct,48,2);
			break;
		case 2:
			Database::Instance()->MakePet(&petstruct,47,2);
			break;
		case 1:
			Database::Instance()->MakePet(&petstruct,46,2);
			break;
		}
		MakePet(petstruct.level,petstruct.class_, petstruct.race, petstruct.texture, petstruct.pettype , petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg,petstruct.max_hp);
	}
	else if (strncmp(pettype, "Skeleton", 8) == 0) {
		int8 tmp = atoi(&pettype[8]);
		
		//cout << "number of the skelee pet "<< (int)tmp << " named "<< pettype << endl;

		switch ( tmp ) {
		case 1:
			Database::Instance()->MakePet(&petstruct,22,4);
			break;
		case 104:
			Database::Instance()->MakePet(&petstruct,23,4);
			break;
		case 108:
			Database::Instance()->MakePet(&petstruct,24,4);
			break;
		case 110:
			Database::Instance()->MakePet(&petstruct,25,4);
			break;
		case 214:
			Database::Instance()->MakePet(&petstruct,26,4);
			break;
		case 217:
			Database::Instance()->MakePet(&petstruct,27,4);
			break;
		case 220:
			Database::Instance()->MakePet(&petstruct,28,4);
			break;
		case 223:
			Database::Instance()->MakePet(&petstruct,29,4);
			break;
		case 226:
			Database::Instance()->MakePet(&petstruct,30,4);
			break;
		case 229:
			Database::Instance()->MakePet(&petstruct,31,4);
			break;
		case 232:
			Database::Instance()->MakePet(&petstruct,32,4);
			break;
		case 237:
			Database::Instance()->MakePet(&petstruct,33,4);
			break;
		case 240:
			Database::Instance()->MakePet(&petstruct,34,4);
			break;
		case 241:
			Database::Instance()->MakePet(&petstruct,35,4);
			break;
		case 245:
			Database::Instance()->MakePet(&petstruct,36,4);
			break;
		}
		MakePet(petstruct.level,petstruct.class_, petstruct.race, petstruct.texture, petstruct.pettype , petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg,petstruct.max_hp);
	}
	/*	else if (strncmp(pettype, "MonsterSum", 9) == 0) {
	}
	else if (strncmp(pettype, "Mistwalker", 10) == 0) {
	}
	else if (strncmp(pettype, "SumEarthYael", 12) == 0) {
	}
	else if (strncmp(pettype, "DALSkelGolin", 12) == 0) {
	}
	else if (strncmp(pettype, "SUMHammer", 9) == 0) {
	}
	else if (strncmp(pettype, "TunareBane", 10) == 0) {
	}*/
	else if (strncmp(pettype, "DruidPet", 8) == 0){
		Database::Instance()->MakePet(&petstruct,11,11);
		MakePet(petstruct.level,petstruct.class_, petstruct.race, petstruct.texture, petstruct.pettype , petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg,petstruct.max_hp);
		//MakePet(25, 0, 43, 1,11,2,11);
	}
	/*
	else if (strncmp(pettype, "SumHammer", 9) == 0) {
	}
	else if (strncmp(pettype, "SumSword", 8) == 0) {
	}
	else if (strncmp(pettype, "SumDecoy", 8) == 0) {
	}
	else if (strncmp(pettype, "Burnout", 7) == 0) {
	}
	else if (strncmp(pettype, "ValeGuardian", 12) == 0) {
	}*/
	else if (strncmp(pettype, "SumMageMultiElement", 19) == 0){
		Database::Instance()->MakePet(&petstruct,4,15);
		MakePet(petstruct.level,petstruct.class_, petstruct.race, petstruct.texture, petstruct.pettype , petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg,petstruct.max_hp);
		//MakePet(50,0,75,3,0,8,15);
	}
	else {
		this->CastToClient()->Message(RED, "Unknown pet type: %s", pettype);
	}
}

//o--------------------------------------------------------------
//| MakeEyeOfZomm; Yeahlight, Feb 20, 2009
//o--------------------------------------------------------------
//| Creates an eye of zomm for the caster
//o--------------------------------------------------------------
void Mob::MakeEyeOfZomm(Mob* caster)
{
	//Yeahlight: Caster no longer exists or the caster is a non-PC
	if(!caster || (caster && !caster->IsClient()))
		return;

	NPCType* npc_type = new NPCType;
	memset(npc_type, 0, sizeof(NPCType));
	char eye_name[64];
	snprintf(eye_name, sizeof(eye_name), "Eye_of_%s", caster->GetName());
	strcpy(npc_type->name, eye_name);
	npc_type->gender = 2;
	npc_type->level = 1;
	npc_type->race = EYE_OF_ZOMM;
	npc_type->class_ = WARRIOR;
	npc_type->texture = 0;
	npc_type->helmtexture = 0;
	npc_type->size = 0;
	npc_type->max_hp = 1;
	npc_type->cur_hp = 1;
	npc_type->min_dmg = 1;
	npc_type->max_dmg = 1;
	npc_type->walkspeed = 0.7f;
	npc_type->runspeed = 1.25f;
	npc_type->npc_id = 0;
	NPC* npc = new NPC(npc_type, 0, this->GetX(), this->GetY(), this->GetZ(), this->GetHeading());
	entity_list.AddNPC(npc);
	npc->SetMyEyeOfZommMasterID(this->GetID());

	if(this->IsClient())
	{
		this->CastToClient()->myEyeOfZomm = npc;
		this->CastToClient()->myEyeOfZommX = GetX();
		this->CastToClient()->myEyeOfZommY = GetY();
	}
}

//////////////////////////////////////////////////
//Tazadar 06/02/08 modified this function in order to use the Database to load the pets infos
void Mob::MakePet(int8 in_level, int8 in_class, int16 in_race, int8 in_texture, int8 in_pettype, float in_size, int8 type,int32 min_dmg,int32 max_dmg, sint32  petmax_hp) {
	if (this->GetPetID() != 0) {
		return;
	}

	NPCType* npc_type = new NPCType;
	memset(npc_type, 0, sizeof(NPCType));
	npc_type->gender = 2;
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
	npc_type->size = in_size;
	npc_type->max_hp = petmax_hp;
	npc_type->cur_hp = npc_type->max_hp;
	//cout << "hp of the pet "<< (int)petmax_hp << endl;
	
	npc_type->min_dmg = min_dmg;
	npc_type->max_dmg = max_dmg;
	npc_type->walkspeed = 0.7f;
	npc_type->runspeed = 1.25f;
	npc_type->npc_id=type;
	switch(type) Tazadar : //Since we cannot see pet weapons we remove them for now :)
	{
	case 2: // Enchanter Pet
		{
			//cout << "chanter pet? " << endl;
			npc_type->gender = 0;
			//npc_type->equipment[7] = 34;
			//npc_type->equipment[8] = 202;
			
			if (in_pettype >=57 && in_pettype<=59) {
			//npc_type->equipment[7] = 26;
			//npc_type->equipment[8] = 26;
		}
		else if (in_pettype >= 49 && in_pettype<=50){
			//npc_type->equipment[7] = 3;
		}
		else{
			printf("Unknown pet number of %i\n",in_pettype);
		}
			break;
		}
	default:
		break;
	}
	NPC* npc = new NPC(npc_type, 0, this->GetX(), this->GetY(), this->GetZ(), this->GetHeading());
	//safe_delete(npc_type);//delete npc_type;
	npc->SetPetType(in_pettype);
	npc->SetOwnerID(this->GetID());
	entity_list.AddNPC(npc);
	this->SetPetID(npc->GetID());
}


//////////////////////////////////////////////////

void Mob::CheckPet() {
	int16 buffid = 0;
	if (this->GetPetID() == 0 && (this->GetClass() == 11 || this->GetClass() == 13)) {
		if (this->GetClass() == 13) {
			buffid = FindSpell(this->class_, this->level, SE_SummonPet);
		}
		else if (this->GetClass() == 11) {
			buffid = FindSpell(this->class_, this->level, SE_NecPet);
		}
		if (buffid != 0) {
			this->CastSpell(spells_handler.GetSpellPtr(buffid), this->GetID());
		}
	}
}

//////////////////////////////////////////////////

int16 Mob::FindSpell(int16 classp, int16 level, int8 type, int8 spelltype) {
	if (!spells_handler.SpellsLoaded()) return 0;
	if (this->GetCastingSpell())
		return 0;

	if (spelltype == 2) // for future use
		spelltype = 0;

	int16 bestsofar = 0;
	int16 bestspellid = 0;
	for (int i = 0; i < SPDAT_RECORDS + NUMBER_OF_CUSTOM_SPELLS; i++) {
		Spell* s = spells_handler.GetSpellPtr(i);
		TTargetType target_type = s->GetSpellTargetType();
		const char* sp_name = s->GetSpellName();
		if ((target_type == ST_Tap && spelltype == 1) || (target_type != ST_Group && target_type != ST_Undead 
			&& target_type != ST_Summoned && target_type != ST_Pet && strstr(sp_name, "Summoning") == NULL)) {
			int Canuse = s->CanUseSpell(classp, level);
			if (Canuse != 0) {
				for (int z=0; z < 12; z++) {
					TSpellEffect effect_id = s->GetSpellEffectID(z);
					int8 formula = s->GetSpellFormula(z);
					sint16 base  = s->GetSpellBase(z);
					sint16 max   = s->GetSpellMax(z);
					int32 buff_duration = s->GetSpellBuffDuration();
					int spfo = spells_handler.CalcSpellValue(s, z, this->GetLevel());
					if (effect_id == SE_ArmorClass && type == SE_ArmorClass && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_TotalHP && type == SE_TotalHP && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_STR && type == SE_STR && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_DEX && type == SE_DEX && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}

					if (effect_id == SE_AGI && type == SE_AGI && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}

					if (effect_id == SE_WIS && type == SE_WIS && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}

					if (effect_id == SE_INT && type == SE_INT && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_CHA && type == SE_CHA && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}

					if (effect_id == SE_MovementSpeed && type == SE_MovementSpeed && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}

					if (effect_id == SE_AttackSpeed && type == SE_AttackSpeed && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_ResistFire && type == SE_ResistFire && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_ResistCold && type == SE_ResistCold && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_ResistMagic && type == SE_ResistMagic && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_ResistDisease && type == SE_ResistDisease && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_ResistPoison && type == SE_ResistPoison && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_DamageShield && type == SE_DamageShield && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_CurrentHPOnce && type == SE_CurrentHPOnce && !FindBuff(s)) {
						if (spfo > 0 && (spfo + buff_duration) > bestsofar) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}
					}
					if (effect_id == SE_SummonPet && type == SE_SummonPet && !FindBuff(s)) {
						if (Canuse > bestsofar) {
							bestsofar = Canuse;
							bestspellid = i;
						}
					}
					if (effect_id == SE_NecPet && type == SE_NecPet && !FindBuff(s)) {
						if (Canuse > bestsofar) {
							bestsofar = Canuse;
							bestspellid = i;
						}
					}
					if (effect_id == SE_CurrentHP && type == SE_CurrentHP && !FindBuff(s)) {
						if (spfo < 0 && (buff_duration + spfo) < bestsofar && spelltype == 1) {
							bestsofar = ((buff_duration * -1) + spfo);
							bestspellid = i;
						}
						if ((spfo + buff_duration) > bestsofar && spfo > 0 && spelltype == 0) {
							bestsofar = spfo + buff_duration;
							bestspellid = i;
						}

					}
				}
			}
		}
	}
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::FindSpell(classp %i, level %i, type %i, spelltype %i): returns %i", classp, level, type, spelltype, bestspellid);
	return bestspellid;
}

//////////////////////////////////////////////////

bool Mob::FindBuff(Spell* spell) {
	if (!spell) return false;
	for (int i=0;i<15;i++) {
		if (buffs[i].spell == spell && buffs[i].spell->GetSpellID() != 0xFFFF) {
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::FindBuff(spell_name = %s): true", spell->GetSpellName());
			return true;
		}
	}
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::FindBuff(spell_name = %s): false", spell->GetSpellName());
	return false;
}

//////////////////////////////////////////////////

bool Mob::FindType(int8 type) {
	for (int i=0;i<15;i++) {
		if (buffs[i].spell->GetSpellID() != 0xFFFF) {
			Spell* s = buffs[i].spell;
			for (int o=0; o<12; o++) {
				TSpellEffect effect_id = s->GetSpellEffectID(o);
				if (effect_id == SE_ArmorClass && type == SE_ArmorClass) {
					return true;
				}
				if (effect_id == SE_TotalHP && type == SE_TotalHP) {
					return true;
				}
				if (effect_id == SE_STR && type == SE_STR) {
					return true;
				}
				if (effect_id == SE_DEX && type == SE_DEX) {
					return true;
				}
				if (effect_id == SE_AGI && type == SE_AGI) {
					return true;
				}
				if (effect_id == SE_WIS && type == SE_WIS) {
					return true;
				}
				if (effect_id == SE_INT && type == SE_INT) {
					return true;
				}
				if (effect_id == SE_CHA && type == SE_CHA) {
					return true;
				}
				if (effect_id == SE_ResistFire && type == SE_ResistFire) {
					return true;
				}
				if (effect_id == SE_ResistCold && type == SE_ResistCold) {
					return true;
				}
				if (effect_id == SE_ResistMagic && type == SE_ResistMagic) {
					return true;
				}
				if (effect_id == SE_ResistPoison && type == SE_ResistPoison) {
					return true;
				}
				if (effect_id == SE_ResistDisease && type == SE_ResistDisease) {
					return true;
				}
				if (effect_id == SE_DamageShield && type == SE_DamageShield) {
					return true;
				}
				if (effect_id == SE_MovementSpeed && type == SE_MovementSpeed) {
					return true;
				}
				if (effect_id == SE_AttackSpeed && type == SE_AttackSpeed) {
					return true;
				}
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////
//******************** StopSong ********************//
//--------------------------------------------------//
// Reset the song the bard is playing, server side  //
// only, the client handles it on his own.          //
//--------------------------------------------------//
//                 - Pinedepain -                   //
//--------------------------------------------------//

void Mob::StopSong(void)
{
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::StopSong()");
	// Pinedepain // We reset the casting vars
	this->SetCastingSpell(NULL);
	spellend_timer->Disable();
}

//////////////////////////////////////////////////
//****************** SpellOnGroup ******************//
//--------------------------------------------------//
// Cast a spell on the caster's group               //
//--------------------------------------------------//
//                 - Pinedepain -                   //
//--------------------------------------------------//

void Mob::SpellOnGroup(Spell* spell,int32 target_id)
{
	// Pinedepain // I use a var to avoid calling CastToClient too many times
	Client* current_caster = CastToClient();

	// Pinedepain // We cast the spell on self and then on our group

	if (current_caster->IsGrouped())
	{
		entity_list.GetGroupByClient(current_caster)->CastGroupSpell(current_caster,spell);
	}

	else
	{
		SpellOnTarget(spell,this);
	}
}

//////////////////////////////////////////////////
//****************  DoBardSongAnim *****************//
//--------------------------------------------------//
// Send the right bard anim to the client and the   //
// surrounding players                              //
//--------------------------------------------------//
//                 - Pinedepain -                   //
//--------------------------------------------------//

// Pinedepain // Comments : The way I'm doing it won't probably work for mobs since I don't know yet how to get access
						 // to what they are wearing, and in particular what's in their offhand, and what skill it uses.

// Pinedepain // Notes : I made all my defines in spdat.h, feel free to move them if they aren't at their place!
					  

//////////////////////////////////////////////////

void Mob::DoBardSongAnim(int16 spell_id)
{
	// Pinedepain //


	/* 
       --------------
	   AnimationID
	   --------------
	   percussion = 0x27 (It's the only one not working, someone have an idea on how to make this animation a working one?)
	   string = 0x28
	   wind = brass = 0x29
	   singing = NaN


	   --------------
	   epic_itemID = 0x503e
	   --------------

	   --------------
	   SkillID, from the item struct
	   --------------
	   drum_skillID = 0x1a
	   brass_skillID = 0x19
	   wind_skillID = 0x17
	   string_skillID = 0x18

	   --------------
	   SkillID, from the spell struct
	   --------------
	   drum_skillID = 0x9c5 || 0x9d6
	   brass_skillID = 0x9c7
	   wind_skillID = 0x9ca
	   string_skillID = 0x9ce
	   sing_skillID = 0x9cf
	*/
	   
	// Pinedepain // First, we get the song's skill type
	Spell* spell = spells_handler.GetSpellPtr(spell_id);
	int16 SkillType = spell->GetSpellSkill();
	

	// Pinedepain // We need to get the instrument type the player is wearing
	uint8 InstruType = CastToClient()->GetWornInstrumentType();

	
	//cout << "SkillType = " << (int)SkillType << endl;
	//cout << " || " << dec << spell_id << endl;

	// Pinedepain // For each skill, we check if we have the correct instrument, if so, we do anim, else we don't
	// Pinedepain // If we have the epic, we do the animation
	switch (SkillType)
	{
	case SS_PERCUSSION:
		if (InstruType == IS_PERCUSSION || InstruType == 0xaa)
		{
			DoAnim(AN_PERCUSSION);
		}
		break;


	case SS_BRASS:
		if (InstruType == IS_BRASS || InstruType == 0xaa)
		{
			DoAnim(AN_BRASS);
		}
		break;

	case SS_WIND:
		if (InstruType == IS_WIND || InstruType == 0xaa)
		{
			DoAnim(AN_WIND);
		}
		break;

	case SS_STRING:
		if (InstruType == IS_STRING || InstruType == 0xaa)
		{
			DoAnim(AN_STRING);
		}
		break;

			
	// Pinedepain // Default should be singing, we don't do anything since there's no animation for the singing skill
	default:
		break;
	}

	//cout << "SkillID = " << hex << spells[spell_id].skill << endl;
}

//////////////////////////////////////////////////

void Mob::InterruptSpell(bool fizzle, bool consumeMana)
{
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::InterruptSpell(fizzle = %i)", fizzle);
	const int function_number = 301;
	const char* InterruptedMessage = !fizzle ? "Your spell was interrupted!" : "Your spell fizzles!";
	this->hitWhileCasting = false;
	spellend_timer->Disable();

	//Yeahlight: Interruption stems from a spell casting mistake (casting on a corpse, casting sow indoors, etc.)
	if(consumeMana)
	{
		if(this->IsClient()) 
		{
			//Yeahlight: Unlock UI, consume spell's cost in mana from the caster and allow the player to cast once again.
			EnableSpellBarWithManaConsumption(this->GetCastingSpell() ? this->GetCastingSpell() : 0);
		}
	}
	else
	{
		if(!fizzle)
			entity_list.MessageClose(this, true, DEFAULT_MESSAGE_RANGE, CAST_SPELL, "%s's casting has been interrupted.", this->GetName());
		else
			entity_list.MessageClose(this, true, DEFAULT_MESSAGE_RANGE, CAST_SPELL, "%s's fizzles.", this->GetName());
		if(this->IsClient()) 
		{
			APPLAYER* outapp = new APPLAYER(OP_InterruptCast, (sizeof(InterruptCast_Struct) + strlen(InterruptedMessage) + 4));
			memset(outapp->pBuffer, 0, outapp->size);
			InterruptCast_Struct* ic = (InterruptCast_Struct*) outapp->pBuffer;
			ic->spawnid = this->GetID();
			strcpy(ic->message, InterruptedMessage);
			this->CastToClient()->QueuePacket(outapp);
			safe_delete(outapp);
			//Yeahlight: Unlock UI and allow the player to cast once again.
			//Pinedepain: Changed the 2nd parameter to use the current casting_spell_id
			EnableSpellBar(this->GetCastingSpell() ? this->GetCastingSpell()->GetSpellID() : 1);
		}
	}

	// Pinedepain // I moved this here, it was before "if (this->IsClient())", and since we are using casting_spell_id
	// in this if, it has no sense to reset this var.
	this->SetCastingSpell(NULL);
}

//////////////////////////////////////////////////

void Mob::SendManaChange(sint32 in_mana, Spell* spell)
{
	const int function_number = 302;
	APPLAYER* outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
	ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
	manachange->new_mana = in_mana;
	manachange->spell_id = spell ? spell->GetSpellID() : 1; //Yeahlight: UI will freeze if this is set to 0
	this->CastToClient()->QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
}

//////////////////////////////////////////////////

void Mob::SendBeginCastPacket(int16 spellid, int32 cast_time)
{
	APPLAYER* outapp = new APPLAYER(OP_BeginCast, sizeof(BeginCast_Struct));	
	memset(outapp->pBuffer, 0, outapp->size);
	BeginCast_Struct* begincast = (BeginCast_Struct*)outapp->pBuffer;
	begincast->caster_id = this->GetID();
	begincast->spell_id = spellid;
	//Yeahlight: FUN: cast_time tells the client how long to display spell particles from the cast
	begincast->cast_time = cast_time;
	entity_list.QueueCloseClients(this, outapp);
	safe_delete(outapp);//delete outapp;
}

//////////////////////////////////////////////////
// true = no fizzle, false = fizzle. Copied from EQEmu 5.0 sources (neotokyo).
bool Mob::CheckFizzle(Spell* spell) {
	if (!this->IsClient() ) return false;
	if (!spell) return true;

	int par_skill;
	int act_skill;

	par_skill = spell->GetSpellClass(GetClass()-1) * 5 - 10;  //IIRC even if you are lagging behind the skill levels you don't fizzle much
	if (par_skill > 235) par_skill = 235;
	par_skill += spell->GetSpellClass(GetClass()-1); // maximum of 270 for level 65 spell

	int8 spell_skill = spell->GetSpellSkill();
	act_skill = CastToClient()->GetSkill(spell_skill);
	act_skill += this->CastToClient()->GetLevel(); // maximum of whatever the client can cheat

	// == 0 --> on par
	// > 0  --> skill is lower, higher chance of fizzle
	// < 0  --> skill is better, lower chance of fizzle
	// the max that diff can be is +- 235
	int diff = par_skill + spell->GetSpellBaseDiff() - act_skill;

	// if you have high int/wis you fizzle less, you fizzle more if you are stupid
	int8 stat = GetWIS() > GetINT() ? GetWIS() : GetINT();
	diff -= (stat - 125) / 20;

	// base fizzlechance is lets say 5%
	int basefizzle = 10;
	int fizzlechance = basefizzle + diff/5;

	if (fizzlechance < 5)
		fizzlechance = 5; // let there remain some chance to fizzle
	if (fizzlechance > 95)
		fizzlechance = 95; // and let there be a chance to succeed


	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::CheckFizzle: %s final fizzle chance: %i, base spell diff: %i", spell->GetSpellName(), fizzlechance, spell->GetSpellBaseDiff());
		
	int result = MakeRandomInt(0, 100);
	if (result > fizzlechance)
		return false;
	return true;
}

//////////////////////////////////////////////////

//o--------------------------------------------------------------
//| AddBuff; Yeahlight, Nov 16, 2008
//o--------------------------------------------------------------
//| Adapted from EQEMU 7.0: Returns the slot the buff was added 
//| to, -1 if it wasn't added due to stacking problems, and -2 
//| if this is not a buff.
//o--------------------------------------------------------------
int Mob::AddBuff(Mob *caster, Spell* spell, int duration)
{
	bool debugFlag = true;

	int buffslot, ret, caster_level, emptyslot = -1;
	bool will_overwrite = false;
	vector<int> overwrite_slots;
	
	caster_level = caster->GetLevel();// ? caster->GetCasterLevel(spell_id) : GetCasterLevel(spell_id);
    
	if(duration == 0)
		duration = spell->GetSpellBuffDuration();//CalcBuffDuration(caster, this, spell_id);

	if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
		this->Message(LIGHTEN_BLUE, "Debug: Spell's duration: ", spell->GetSpellBuffDuration());
	if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe() && this != caster)
		caster->Message(LIGHTEN_BLUE, "Debug: Spell's duration: ", spell->GetSpellBuffDuration());

	if(duration == 0) 
	{
		int duration2 = spells_handler.CalcSpellBuffTics(caster, this, spell);
		if(duration2 == 0)
		{
			if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe())
				caster->Message(LIGHTEN_BLUE, "Debug: Buff %d failed to add because its duration came back as 0.", spell->GetSpellID());
			if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe() && this != caster)
				this->Message(LIGHTEN_BLUE, "Debug: Buff %d failed to add because its duration came back as 0.", spell->GetSpellID());
			return -2;	// no duration? this isn't a buff
		}
	}
	
	if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
		this->Message(LIGHTEN_BLUE, "Debug: Trying to add buff %s cast by %s with duration %d", spell->GetSpellName(), caster->GetName(), duration);
	if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe() && this != caster)
		caster->Message(LIGHTEN_BLUE, "Debug: Trying to add buff %s cast by %s with duration %d", spell->GetSpellName(), caster->GetName(), duration);

	// solar: first we loop through everything checking that the spell
	// can stack with everything.  this is to avoid stripping the spells
	// it would overwrite, and then hitting a buff we can't stack with.
	// we also check if overwriting will occur.  this is so after this loop
	// we can determine if there will be room for this buff
	for(buffslot = 0; buffslot < BUFF_COUNT; buffslot++)
	{
		const Buffs_Struct &curbuf = buffs[buffslot];

		if(curbuf.spell != NULL)
		{
			// there's a buff in this slot
			ret = CheckStackConflict(curbuf.spell, curbuf.casterlevel, spell, caster_level, entity_list.GetMob(curbuf.casterid), caster);
			if(ret == -1) {	// stop the spell
				if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
					this->Message(LIGHTEN_BLUE, "Debug: Adding buff %d failed: stacking prevented by spell %d in slot %d with caster level %d", spell->GetSpellID(), curbuf.spell->GetSpellID(), buffslot, curbuf.casterlevel);
				if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe() && this != caster)
					caster->Message(LIGHTEN_BLUE, "Debug: Adding buff %d failed: stacking prevented by spell %d in slot %d with caster level %d", spell->GetSpellID(), curbuf.spell->GetSpellID(), buffslot, curbuf.casterlevel);
				return -1;
			}
			if(ret == 1) {	// set a flag to indicate that there will be overwriting
				if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
					this->Message(LIGHTEN_BLUE, "Debug: Adding buff %d will overwrite spell %d in slot %d with caster level %d", spell->GetSpellID(), curbuf.spell->GetSpellID(), buffslot, curbuf.casterlevel);
				if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe() && this != caster)
					caster->Message(LIGHTEN_BLUE, "Debug: Adding buff %d will overwrite spell %d in slot %d with caster level %d", spell->GetSpellID(), curbuf.spell->GetSpellID(), buffslot, curbuf.casterlevel);
				will_overwrite = true;
				overwrite_slots.push_back(buffslot);
			}
		}
		else
		{
			if(emptyslot == -1)
				emptyslot = buffslot;
		}
	}
	
	// we didn't find an empty slot to put it in, and it's not overwriting
	// anything so there must not be any room left.
 	if(emptyslot == -1 && !will_overwrite)
 	{
 		if(spell->IsDetrimentalSpell()) //Sucks to be you, bye bye one of your buffs
 		{
 			for(buffslot = 0; buffslot < BUFF_COUNT; buffslot++)
 			{
 				const Buffs_Struct &curbuf = buffs[buffslot];
 				if(curbuf.spell->IsBeneficialSpell())
 				{
					if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
						this->Message(LIGHTEN_BLUE, "Debug: No slot for detrimental buff %d, so we are overwriting a beneficial buff %d in slot %d", spell->GetSpellID(), curbuf.spell->GetSpellID(), buffslot);
 					if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe() && this != caster)
						caster->Message(LIGHTEN_BLUE, "Debug: No slot for detrimental buff %d, so we are overwriting a beneficial buff %d in slot %d", spell->GetSpellID(), curbuf.spell->GetSpellID(), buffslot);
					BuffFadeBySlot(buffslot,false);
 					emptyslot = buffslot;
					break;
 				}
 			}
 			if(emptyslot == -1) {
				if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
					this->Message(LIGHTEN_BLUE, "Debug: Unable to find a buff slot for detrimental buff %d", spell->GetSpellID());
				if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe() && this != caster)
					caster->Message(LIGHTEN_BLUE, "Debug: Unable to find a buff slot for detrimental buff %d", spell->GetSpellID());
				return(-1);
 			}
 		}
 		else {
			if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
				this->Message(LIGHTEN_BLUE, "Debug: Unable to find a buff slot for beneficial buff %d", spell->GetSpellID());
			if(debugFlag && caster->IsClient() && caster->CastToClient()->GetDebugMe() && this != caster)
				caster->Message(LIGHTEN_BLUE, "Debug: Unable to find a buff slot for beneficial buff %d", spell->GetSpellID());
 			return -1;
 		}
 	}

	// solar: at this point we know that this buff will stick, but we have
	// to remove some other buffs already worn if will_overwrite is true
	if(will_overwrite)
	{
		vector<int>::iterator cur, end;
		cur = overwrite_slots.begin();
		end = overwrite_slots.end();
		for(; cur != end; cur++) {
			// strip spell
			BuffFadeBySlot(*cur, false);

			// if we hadn't found a free slot before, or if this is earlier
			// we use it
			if(emptyslot == -1 || *cur < emptyslot)
				emptyslot = *cur;
		}
	}

	// now add buff at emptyslot
	//assert(buffs[emptyslot].spell == NULL);	// sanity check
	
	//buffs[emptyslot].spell = spell;
	//buffs[emptyslot].casterlevel = caster_level;
	//buffs[emptyslot].casterid = caster ? caster->GetID() : 0;
	//buffs[emptyslot].durationformula = spell->GetSpellBuffDurationFormula();
	//buffs[emptyslot].ticsremaining = duration;
	////buffs[emptyslot].diseasecounters = 0;
	////buffs[emptyslot].poisoncounters = 0;
	////buffs[emptyslot].cursecounters = 0;
	////buffs[emptyslot].numhits = spells[spell_id].numhits;
	////buffs[emptyslot].client = caster ? caster->IsClient() : 0;

	////if(buffs[emptyslot].ticsremaining > (1+CalcBuffDuration_formula(caster_level, pSpells[spell->GetSpellID()].buffdurationformula, pSpells[spell->GetSpellID()].buffduration)))
	////	buffs[emptyslot].UpdateClient = true;
	//	
	//if(debugFlag && this->IsClient())
	//	this->Message(LIGHTEN_BLUE, "Debug: Buff %d added to slot %d with caster level %d", spell->GetSpellID(), emptyslot, caster_level);
	//if(debugFlag && caster->IsClient())
	//	caster->Message(LIGHTEN_BLUE, "Debug: Buff %d added to slot %d with caster level %d", spell->GetSpellID(), emptyslot, caster_level);
	//
	//// recalculate bonuses since we stripped/added buffs
	//CalcBonuses(true, false);

	return emptyslot;
}

//o--------------------------------------------------------------
//| CheckStackConflict; Yeahlight, Nov 16, 2008
//o--------------------------------------------------------------
//| Adapted from EQEMU 7.0: Helper function for AddBuff to 
//| determine stacking.
//| Returns:
//|   0 if not the same type, no action needs to be taken
//|   1 if spellid1 should be removed (overwrite)
//|   -1 if they can't stack and spellid2 should be stopped
//| A spell will not land if it would overwrite a better spell 
//| on any effect. If all effects are better or the same, we 
//| overwrite, else we do nothing.
//o--------------------------------------------------------------
int Mob::CheckStackConflict(Spell* spell1, int caster_level1, Spell* spell2, int caster_level2, Mob* caster1, Mob* caster2)
{
	bool debugFlag = true;

	int16 spellid1 = spell1->GetSpellID();
	int16 spellid2 = spell2->GetSpellID();
	const SPDat_Spell_Struct &sp1 = spell1->Get();
	const SPDat_Spell_Struct &sp2 = spell2->Get();
	
	int i, effect1, effect2, sp1_value, sp2_value;
	int blocked_effect, blocked_below_value, blocked_slot;
	int overwrite_effect, overwrite_below_value, overwrite_slot;

	if(debugFlag && caster2 && caster2->IsClient() && caster2->CastToClient()->GetDebugMe())
		caster2->Message(LIGHTEN_BLUE, "Debug: Check Stacking on old %s (%d) @ lvl %d (by %s) vs. new %s (%d) @ lvl %d (by %s)", sp1.name, spellid1, caster_level1, (caster1==NULL)?"Nobody":caster1->GetName(), sp2.name, spellid2, caster_level2, (caster2==NULL)?"Nobody":caster2->GetName());

	//if(((spellid1 == spellid2) && (spellid1 == 2751)) || //special case spells that will block each other no matter what
	//	((spellid1 == spellid2) && (spellid1 == 2755)) //manaburn / lifeburn
	//	){
	//		mlog(SPELLS__STACKING, "Blocking spell because manaburn/lifeburn does not stack with itself");
	//		return -1;
	//	}

	//resurrection effects wont count for overwrite/block stacking
	switch(spellid1)
	{
	case 756:
	case 757:
	case 5249:
		return (0);
	}

	switch (spellid2)
	{
	case 756:
	case 757:
	case 5249:
		return (0);
	}
	
	/*
	One of these is a bard song and one isn't and they're both beneficial so they should stack.
	*/
	if(spell1->IsBardSong() != spell2->IsBardSong()) 
	{
		if(!spell1->IsDetrimentalSpell() && !spell2->IsDetrimentalSpell())
		{
			if(debugFlag && caster2 && caster2->IsClient() && caster2->CastToClient()->GetDebugMe())
				caster2->Message(WHITE, "%s and %s are beneficial, and one is a bard song, no action needs to be taken", sp1.name, sp2.name);
			return (0);
		}
	}


	// solar: check for special stacking block command in spell1 against spell2
	for(i = 0; i < EFFECT_COUNT; i++)
	{
		effect1 = sp1.effectid[i];
		if(effect1 == SE_StackingCommand_Block)
		{
			/*
			The logic here is if you're comparing the same spells they can't block each other
			from refreshing
			*/
			if(spellid1 == spellid2)
				continue;

			blocked_effect = sp1.base[i];
			//Yeahlight: TODO: Check this - 201 business
			blocked_slot = sp1.formula[i] - 201;	//they use base 1 for slots, we use base 0
			blocked_below_value = sp1.max[i];
			
			if(sp2.effectid[blocked_slot] == blocked_effect)
			{
				sp2_value = CalcSpellEffectValue(spell2, blocked_slot, caster_level2);
				
				if(debugFlag && caster2 && caster2->IsClient() && caster2->CastToClient()->GetDebugMe())
					caster2->Message(LIGHTEN_BLUE, "Debug: %s (%d) blocks effect %d on slot %d below %d. New spell has value %d on that slot/effect. %s.", sp1.name, spellid1, blocked_effect, blocked_slot, blocked_below_value, sp2_value, (sp2_value < blocked_below_value)?"Blocked":"Not blocked");

				if(sp2_value < blocked_below_value)
				{
					if(debugFlag && caster2 && caster2->IsClient() && caster2->CastToClient()->GetDebugMe())
						caster2->Message(LIGHTEN_BLUE, "Debug: Blocking spell because sp2_value < blocked_below_value");
					return -1;	// blocked
				}
			} else {
				if(debugFlag && caster2 && caster2->IsClient() && caster2->CastToClient()->GetDebugMe())
					caster2->Message(LIGHTEN_BLUE, "Debug: %s (%d) blocks effect %d on slot %d below %d, but we do not have that effect on that slot. Ignored.", sp1.name, spellid1, blocked_effect, blocked_slot, blocked_below_value);
			}
		}
	}

	// check for special stacking overwrite in spell2 against effects in spell1
	for(i = 0; i < EFFECT_COUNT; i++)
	{
		effect2 = sp2.effectid[i];
		if(effect2 == SE_StackingCommand_Overwrite)
		{
			overwrite_effect = sp2.base[i];
			overwrite_slot = sp2.formula[i] - 201;	//they use base 1 for slots, we use base 0
			overwrite_below_value = sp2.max[i];
			if(sp1.effectid[overwrite_slot] == overwrite_effect)
			{
				sp1_value = CalcSpellEffectValue(spell1, overwrite_slot, caster_level1);

				//mlog(SPELLS__STACKING, "%s (%d) overwrites existing spell if effect %d on slot %d is below %d. Old spell has value %d on that slot/effect. %s.", sp2.name, spellid2, overwrite_effect, overwrite_slot, overwrite_below_value, sp1_value, (sp1_value < overwrite_below_value)?"Overwriting":"Not overwriting");
				
				if(sp1_value < overwrite_below_value)
				{
					//mlog(SPELLS__STACKING, "Overwrite spell because sp1_value < overwrite_below_value");
					return 1;			// overwrite spell if its value is less
				}
			} else {
				//mlog(SPELLS__STACKING, "%s (%d) overwrites existing spell if effect %d on slot %d is below %d, but we do not have that effect on that slot. Ignored.", sp2.name, spellid2, overwrite_effect, overwrite_slot, overwrite_below_value);

			}
		}
	}
	
	bool sp1_detrimental = spell1->IsDetrimentalSpell();
	bool sp2_detrimental = spell2->IsDetrimentalSpell();
	bool sp_det_mismatch;

	if(sp1_detrimental == sp2_detrimental)
		sp_det_mismatch = false;
	else
		sp_det_mismatch = true;
	
	// now compare matching effects
	// arbitration takes place if 2 spells have the same effect at the same
	// effect slot, otherwise they're stackable, even if it's the same effect
	bool will_overwrite = false;
	for(i = 0; i < EFFECT_COUNT; i++)
	{
		if(spell1->IsBlankSpellEffect(i))
			continue;

		effect1 = sp1.effectid[i];
		effect2 = sp2.effectid[i];

		//Effects which really aren't going to affect stacking.
		if(effect1 == SE_CurrentHPOnce ||
			effect1 == SE_CurseCounter	||
			effect1 == SE_DiseaseCounter ||
			effect1 == SE_PoisonCounter){
			continue;
			}

		/*
		Quick check, are the effects the same, if so then
		keep going else ignore it for stacking purposes.
		*/
		if(effect1 != effect2)
			continue;

		/*
		If target is a npc and caster1 and caster2 exist
		If Caster1 isn't the same as Caster2 and the effect is a DoT then ignore it.
		*/
		if(IsNPC() && caster1 && caster2 && caster1 != caster2) {
			if(effect1 == SE_CurrentHP && sp1_detrimental && sp2_detrimental) {
				continue;
				//mlog(SPELLS__STACKING, "Both casters exist and are not the same, the effect is a detrimental dot, moving on");
			}
		}

		if(effect1 == SE_CompleteHeal){ //SE_CompleteHeal never stacks or overwrites ever, always block.
			//mlog(SPELLS__STACKING, "Blocking spell because complete heal never stacks or overwries");
			return (-1);
		}

		/*
		If the effects are the same and
		sp1 = beneficial & sp2 = detrimental or
		sp1 = detrimental & sp2 = beneficial
		Then this effect should be ignored for stacking purposes.
		*/
		if(sp_det_mismatch)
		{
			//mlog(SPELLS__STACKING, "The effects are the same but the spell types are not, passing the effect");
			continue;
		}
		
		/*
		If the spells aren't the same
		and the effect is a dot we can go ahead and stack it
		*/
		if(effect1 == SE_CurrentHP && spellid1 != spellid2 && sp1_detrimental && sp2_detrimental) {
			//mlog(SPELLS__STACKING, "The spells are not the same and it is a detrimental dot, passing");
			continue;
		}

		sp1_value = CalcSpellEffectValue(spell1, i, caster_level1);
		sp2_value = CalcSpellEffectValue(spell2, i, caster_level2);
		
		// some spells are hard to compare just on value.  attack speed spells
		// have a value that's a percentage for instance
		if
		(
			effect1 == SE_AttackSpeed ||
			effect1 == SE_AttackSpeed2 ||
			effect1 == SE_AttackSpeed3
		)
		{
			sp1_value -= 100;
			sp2_value -= 100;
		}
		
		if(sp1_value < 0)
			sp1_value = 0 - sp1_value;
		if(sp2_value < 0)
			sp2_value = 0 - sp2_value;
		
		if(sp2_value < sp1_value) {
			//mlog(SPELLS__STACKING, "Spell %s (value %d) is not as good as %s (value %d). Rejecting %s.",sp2.name, sp2_value, sp1.name, sp1_value, sp2.name);
			return -1;	// can't stack
		}
		//we dont return here... a better value on this one effect dosent mean they are
		//all better...

		//mlog(SPELLS__STACKING, "Spell %s (value %d) is not as good as %s (value %d). We will overwrite %s if there are no other conflicts.",sp1.name, sp1_value, sp2.name, sp2_value, sp1.name);
		will_overwrite = true;
	}
	
	//if we get here, then none of the values on the new spell are "worse"
	//so now we see if this new spell is any better, or if its not related at all
	if(will_overwrite) {
		//mlog(SPELLS__STACKING, "Stacking code decided that %s should overwrite %s.", sp2.name, sp1.name);
		return(1);
	}
	
	//mlog(SPELLS__STACKING, "Stacking code decided that %s is not affected by %s.", sp2.name, sp1.name);
	return 0;
}