#include "client.h"
#include "skills.h"


void Client::ProcessOP_Beg(APPLAYER* pApp)
{
	if(pApp->size != sizeof(Beg_Struct))
	{
		cout << "Wrong size on OP_Beg. Got: " << pApp->size << ", Expected: " << sizeof(Beg_Struct) << endl;
		return;
	}

	//Yeahlight: Purge client's invisibility
	CancelAllInvisibility();
	
	Beg_Struct* beg_info = (Beg_Struct*)pApp->pBuffer;
	
	Mob* target = entity_list.GetMob(beg_info->target);
	Mob* player = entity_list.GetMob(beg_info->begger);
	
	//Beg_Struct* beg_info = (Beg_Struct*)pApp->pBuffer;
	// Validate Player
	// Added this check to keep people from fudging packets to make
	// mobs attack other through begging.  Flag as a hack also.
	if(this->GetID() != player->GetID())
	{
		cout << "[ hack ]" << this->GetName() << " is begging for another player: ";
		cout << player->GetName() << endl;
		
		// This will make the client not able to beg anymore.
		// Could we send qa packet back with 'beg_info->success = 0?
		// Nah, I'd rather just leave the hackers with a broken client. -neorab

		return;
	}

	// Validate Time
	// Should not be able to send two beg requests within 10 seconds.
	// Flag beg spammers as hackers.  Drop the packet and move on.
	int32 time_to_beg = beg_timer->GetRemainingTime();
	if(time_to_beg != 0) 
	{
		cout << "[ hack ]" << player->GetName() << " is begging to fast. ";
		cout << 10000 - time_to_beg << "ms since last beg." << endl;
		
		// This will make the client not able to beg anymore.
		return;
	}
	beg_timer->Start(10000);

	// Validate Target
	// Should not be able to beg from other players, corpses or pets.
	// Basiclly, the client will have to have the same thing targeted
	// as the packet says they do.  If they target a pet and send a beg
	// packet with the pet as the target, this won't catch it.  But it'll
	// stop the average dumbass forging packets.
	Mob* tmptar = this->GetTarget();
	if((tmptar->GetID() != target->GetID()) || target->IsNPC() != true)
	{
		cout << "[ hack ]" << player->GetName() << " is begging from: " << target->GetName();
		cout << "but has [" << tmptar->GetName() << "] targeted." << endl;
		
		// This will make the client not able to beg anymore.
		return;
	}
	
	// Validate Skill
	// Look the skill up, flag the account for hacks if they don't match.
	int8 beg_skill = this->GetSkill(BEGGING);
	if(beg_skill != beg_info->skill)
	{
		cout << "[ hack ]" << player->GetName() << " is trying to beg at " << beg_info->skill;
		cout << "but is [" << beg_skill << "] skill." << endl;
		
		// This will make the client not able to beg anymore.
		return;
	}


	// Pets.
	// You cannot succeed or crit fail on pets.
	if(target->CastToNPC()->GetOwner() == 0)
	{

		// Roll The Dice for Success
		// the threshold is the number you have to be under to have begged successfully
		//   skill level / 8000 (0 - 4% liner based on skill)
		// + Charisma Modifier (same as skill level) *  active charisma % (20% for ever 51 levels)
		double success_threshold = ((double)beg_skill / 8000) 
			+ (( (int)((double)beg_skill / 51) * 0.20) * ((double)player->GetCHA() / 8500));
		double the_dice = MakeRandomFloat(0.000, 1.000);
		
		if(the_dice <= success_threshold)
		{
			char message[255];
			sprintf(message, "%s says, \"Here %s, take this and LEAVE ME ALONE!\"", target->GetName(), player->GetName());
			Message(WHITE, message);
			beg_info->success = 4;
			if(the_dice == success_threshold)
			{
				beg_info->coins = 2;
			}
			else
			{
				beg_info->coins = 1;
			}
		}

		// Critical Failure =)
		else
		{
			beg_info->success = 0;
			// Random Attack (5% @ 0skill 1% @ 200 skill)
			the_dice = MakeRandomFloat(0.000, 1.000);
			if(the_dice <= (0.05 - (int)((double)beg_skill / 50) * 0.01))
			{
				char message[255];
				sprintf(message, "%s says, \"Beggers like you always bring out the worst in me.\"", target->GetName());
				target->CastToNPC()->AddToHateList(player);
				Message(DARK_RED, message);
			}
		}
	}

	// This is a pet, never allow them to succeed.
	else
	{
		beg_info->success = 0;
	}
	

	// Random Skill-Up
	// This was a bitch skill to learn.  Let's do ~1/2% change to skill up.
	// I'm not using the CheckAddSkill function here because that does not
	// let us control it quite like this.  I really don't believe every
	// skill uses the same formula. -neorab
	int dice = MakeRandomInt(0, 200);
	if(dice <= 1)
	{
		if(GetSkill(BEGGING) < CheckMaxSkill(BEGGING, this->race, this->class_, this->level))
		{
			SetSkill(BEGGING, GetSkill(BEGGING)+1);
		}
	}


	// Save Player Profile
	// The return told the client they got more money, we should save that
	// server side to reflect the extra cash.
	if(beg_info->success != 0)
	{
		AddMoneyToPP(beg_info->coins, 0, 0, 0, false);
	}
	
	QueuePacket(pApp);
}
