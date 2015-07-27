#include "client.h"
#include "skills.h"
#include "zone.h"
#include "SpellsHandler.hpp"

extern SpellsHandler	spells_handler;

//o--------------------------------------------------------------
//| Process_InstillDoubt; Harakiri, August 19, 2009
//o--------------------------------------------------------------
//| Handles client requests for install doubt
//| effective fear will be done after a few seconds
//o--------------------------------------------------------------
void Client::Process_InstillDoubt(APPLAYER *app)
{
	CancelAllInvisibility();
	
	
	// just to be safe
	if(this->GetTarget() == 0) {
		return;
	}


	// Harakiri: in Client_Process() Process it will be checked
	// if the timer is up, then it will call DoInstillDoubt();
	instilldoubt_timer->Start();				
}

//o--------------------------------------------------------------
//| Process_InstillDoubt; Harakiri, August 19, 2009
//o--------------------------------------------------------------
//| Handles client requests for install doubt
//| Remember, instill doubt could be skilled up on any target
//| (corpse, npc, player) without any range check
//| however the fear only went off while being in close melee range
//| Note: Instill Doubt == Intimidation
//o--------------------------------------------------------------
void Client::DoInstillDoubt() {

	instilldoubt_timer->Disable();

	// just to be safe
	if(this->GetTarget() == 0) {
		return;
	}

	int FEAR_SPELL_ID = 229;
	
	// Harakiri according to posts, and my own testing on eqmac
	// the chance to succeed at 100 skill is about 20% for a blue con
	// after the 20% check, the normal resist check for the spell
	// fear should apply, then instill doubt is nothing else
	// as casting a fear spell (hence the 3 sec kick animation)
	// using this simple formular we get
	// skill 10 = 2% chance
	// skill 50 = 10% chance
	// skill 100 = 20% chance
	// through a cap, you still have a higher chance at higher skill
	// points to reach that cap
	// http://everquest.allakhazam.com/db/skills.html?skill=55
	int chance = GetSkill(INTIMIDATION)*0.20;			
	
	// cap 1/3 chance to go off
	if(chance > 33) {
		chance = 33;
	}

	if(chance > MakeRandomInt(0,99)) {		

		// can we attack the target? 
		// is it in melee range?
		if (this->CanAttackTarget(this->GetTarget()) && (this->Dist(this->GetTarget()) <= 20) ) {
			SpellOnTarget(spells_handler.GetSpellPtr(FEAR_SPELL_ID), this->GetTarget());
		// we still make the fear spell animation, even if it is out of range
		// the animation is on itself so that we see it, even if the mob is 1000 yards away
		// this is how its on eqmac
		} else {			
			APPLAYER* outapp = new APPLAYER(OP_CastOn, sizeof(CastOn_Struct));
			memset(outapp->pBuffer, 0, sizeof(CastOn_Struct));
			CastOn_Struct* caston = (CastOn_Struct*) outapp->pBuffer;
			caston->source_id = this->GetID();
			// same ID to see fear
			caston->target_id = this->GetID();
			caston->action = 231;
			caston->spell_id = FEAR_SPELL_ID;
			caston->heading = this->GetHeading() * 2;
			caston->source_level = this->GetLevel();
			caston->unknown1[1] = 0x41;
			caston->unknown1[3] = 0x0A;
			caston->unknown2[0] = 0x00;
			//Yeahlight: Anything but 0x04 will prevent the client from placing a debuff icon on the player
			caston->unknown2[1] = 0x00; 
			entity_list.QueueCloseClients(this, outapp, false);
		}
		
	} else {
		Message(DARK_BLUE,"You're not scaring anyone.");		
	}

	// Harakiri chance to skill up
	// this skill was hardcore to train
	// i macroed on eqmac from skill 40 to 100, it took 8 hours
	// a full cycle of Instill Doubt takes 10sec so
	// 28800sec / 10sec = 2880 tries / 60 skillups = 48 tries for each skill up
	// this means on average a 2% chance
	if(MakeRandomInt(0,99) >= 98) {
		if(GetSkill(INTIMIDATION) < CheckMaxSkill(INTIMIDATION, GetRace(), GetClass(), GetLevel())) {
			SetSkill(INTIMIDATION,GetSkill(INTIMIDATION)+1);
		}
	}
}