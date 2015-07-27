// big update by Daniel Brown / froglok23 22/09/2007
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <assert.h>
#include <math.h>
#include <fstream>

#ifndef WIN32
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "client.h"
#include "database.h"
#include "EQCUtils.hpp"
#include "packet_functions.h"
#include "packet_dump.h"
#include "worldserver.h"
#include "packet_dump_file.h"
#include "PlayerCorpse.h"
#include "spdat.h"
#include "petitions.h"
#include "NpcAI.h"
#include "skills.h"
#include "EQCException.hpp"
#include "MessageTypes.h"
#include "forage.h"
#include "npc.h"
#include "Clientlist.h"
#include "packet_dump_file.h"
#include "groups.h"
#include "SpellsHandler.hpp"
#include "ZoneGuildManager.h"
#include "itemtypes.h"
#include "projectile.h"
#include "Item.h"

using namespace std;
using namespace EQC::Zone;
extern Database			database;
extern Zone*			zone;
extern volatile bool	ZoneLoaded;
extern WorldServer		worldserver;
extern SpellsHandler	spells_handler;
extern PetitionList		petition_list;
extern ClientList		client_list;


extern PetitionList petition_list;
extern ZoneGuildManager zgm;


bool Client::Process()
{
	bool ret = true;

	if(Connected())
	{
		//Melinko: Check if warrior is in berserker frenzy
		if(!this->CastToClient()->IsBerserk())
		{
			if(GetClass() == WARRIOR && GetHPRatio() < 40)
			{
				//Melinko: TODO: increase STR of warrior
				SetBerserk(true);
				entity_list.MessageClose(this, false, DEFAULT_MESSAGE_RANGE, WHITE, "%s goes into a berserker frenzy!", this->GetName());
			}
		}else{
			if(GetClass() == WARRIOR && GetHPRatio() > 44)
			{
				//Melinko: TODO: increase STR of warrior
				SetBerserk(false);
				entity_list.MessageClose(this, false, DEFAULT_MESSAGE_RANGE, WHITE, "%s is no longer berserk.", this->GetName());
			}
		}

		//Yeahlight: Check for stun's end duration
		if(stunned && stunned_timer->Check())
		{
			stunned_timer->Disable();
			stunned = false;
			//Yeahlight: PC is feared, so get their velocity going again
			if(GetFeared())
				GetFearDestination(GetX(), GetY(), GetZ());
		}

		//Yeahlight: PC is charmed
		if(IsCharmed() && charmPositionUpdate_timer->Check())
		{
			//Yeahlight: PC's master no longer exists
			if(GetOwner() == NULL)
			{
				this->BuffFadeByEffect(SE_Charm);
			}
			//Yeahlight: PC's master exists
			else
			{
				Mob* myOwner = GetOwner();
				//Yeahlight: PC's target is itself or NULL
				if(target == this || target == NULL)
					target = myOwner;
				//Yeahlight: This charmed PC's target is not its own master OR the target is its master and has been ordered to follow him/her
				if(target != myOwner || (target == myOwner && GetPetOrder() != SPO_Guard))
				{
					//Yeahlight: Update the PC's heading while attacking
					if(target && target != myOwner)
					{
						float inHeading = GetHeading();
						float outHeading = 999;
						this->faceDestination(target->GetX(), target->GetY());
						outHeading = GetHeading();
						//Yeahlight: A change in heading is required
						if(inHeading != outHeading)
						{
							this->SendPosUpdate(true, PC_UPDATE_RANGE, true);
						}
					}
					float distanceToTargetNoRoot = 10*10 + HIT_BOX_MULTIPLIER * target->GetHitBox();
					//Yeahlight: PC is out of range of their target
					if(DistNoRootNoZ(target) > distanceToTargetNoRoot)
					{
						float inX = x_dest;
						float inY = y_dest;
						x_dest = target->GetX();
						y_dest = target->GetY();
						//Yeahlight: PC is not currently moving
						if(this->animation == 0)
						{
							this->GetCharmDestination(x_dest, y_dest);
						}
						//Yeahlight: Update the PC's position manually while moving
						else
						{
							UpdateCharmPosition(x_dest, y_dest, sqrt(distanceToTargetNoRoot));
						}
						//Yeahlight: A change in heading is required
						if(inX != x_dest && inY != y_dest)
						{
							this->faceDestination(x_dest, y_dest);
							this->SendPosUpdate(true, PC_UPDATE_RANGE, false);
						}
					}
					//Yeahlight: PC is out of range of its target and is currently moving
					else if(this->animation > 0)
					{
						//Yeahlight: Pet is following its master and its master is not currently moving
						if(target == myOwner && myOwner->GetAnimation() == 0 && myOwner)
							this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Standing_To_Sitting, true);
						this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Lose_Control, true);
						this->animation = 0;
						this->delta_heading = 0;
						this->delta_x = 0;
						this->delta_y = 0;
						this->delta_z = 0;
						this->SendPosUpdate(true, PC_UPDATE_RANGE, false);
					}
				}
				//Yeahlight: PC's target is its own master and its current order is to guard position
				else
				{
					float distanceToTarget = fdistance(GetX(), GetY(), guard_x, guard_y);
					//Yeahlight: PC is out of range of its guarding destination
					if(distanceToTarget > 8.00)
					{
						//Yeahlight: PC is not currently moving
						if(this->animation == 0)
						{
							this->GetCharmDestination(guard_x, guard_y);
						}
						//Yeahlight: Update the PC's position manually while moving
						else
						{
							UpdateCharmPosition(guard_x, guard_y, distanceToTarget);
						}
					}
					//Yeahlight: PC is out of range of its guarding destination and is currently moving
					else if(this->animation > 0)
					{
						this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Standing_To_Sitting, true);
						this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Lose_Control, true);
						this->animation = 0;
						this->delta_heading = 0;
						this->delta_x = 0;
						this->delta_y = 0;
						this->delta_z = 0;
						this->SendPosUpdate(true, PC_UPDATE_RANGE, false);
					}
				}
			}
		}

		//static int skipList[50] = {0x2020, 0x2021, 0x2022, 0x2520, 0x2720, 0x2A20, 0x2C21, 0x2D20, 0x3120, 0x3421, 0x3E20, 0x3E21, 0x3E22, 0x3F20, 0x3F21, 0x3F22, 0x4021, 0x4022, 0x4120, 0x4121, 0x4122, 0x4221, 0x6D21};
		//static int hackCounter = -3000;
		//static int x = 0x9F20;
		//static int y = 0;
		//static int h = 0;
		//static int i = 0;
		//static int j = 0;
		//static int k = 0;
		//if((hackCounter++) == 2048)
		//{
		//	//if(i == 0 && j == 4 && k == 4)
		//	//{
		//	//	if(h == 1)
		//	//	{
		//	//		Message(WHITE, "Debug: Starting projectile hack");
		//	//		cout<<"Debug: Starting projectile hack"<<endl;
		//	//	}
		//	//	Message(WHITE, "Debug: On %i", h);
		//	//	cout<<"Debug: On "<<h<<endl;
		//	//}
		//	bool permit = true;
		//	hackCounter = 0;
		//	//for(int i = 0; i < sizeof(skipList); i++)
		//	//{
		//	//	if(x == skipList[i])
		//	//	{
		//	//		permit = false;
		//	//		break;
		//	//	}
		//	//}
		//	if(permit)
		//	{
		//		//Message(WHITE, "Debug: On %x", x);
		//		//cout<<"Debug: On "<<hex<<x<<endl;
		//		APPLAYER* outapp = new APPLAYER(OP_SpawnProjectile, sizeof(SpawnProjectile_Struct));
		//		SpawnProjectile_Struct* hs = (SpawnProjectile_Struct*)outapp->pBuffer;
		//		memset(outapp->pBuffer, 0x00, sizeof(SpawnProjectile_Struct));
		//		strcpy(hs->texture, "GENC00");
		//		hs->sourceID = GetID();
		//		hs->targetID = target ? target->GetID() : 0x00;
		//		hs->tilt = 256;
		//		hs->pitch = -60;
		//		hs->yaw = 0;
		//		hs->velocity = 0.333333;
		//		hs->x = GetX();
		//		hs->y = GetY();
		//		hs->z = GetZ();
		//		hs->heading = GetHeading() * 2;
		//		hs->projectileType = 9;
		//		hs->spellID = 477;
		//		entity_list.QueueClients(this, outapp);
		//		safe_delete(outapp);

		//		//APPLAYER* outapp = new APPLAYER(x, sizeof(Test_Struct));
		//		//memset(outapp->pBuffer, 0x00, sizeof(Test_Struct));
		//		//Test_Struct* ts = (Test_Struct*)outapp->pBuffer;
		//		//for(int kk = 0; kk < 3; kk++)
		//		//{
		//		//	for(int jj = 0; jj < 3; jj++)
		//		//	{
		//		//		for(int mm = 0; mm < 3; mm++)
		//		//		{
		//		//			if(kk == jj || kk == mm || jj == mm)
		//		//				continue;
		//		//			ts->test[jj] = 0x01;
		//		//			ts->test[kk] = 0x02;
		//		//			ts->test[mm] = 0x26;
		//		//			QueuePacket(outapp);
		//		//		}
		//		//	}
		//		//}
		//		//safe_delete(outapp);
		//	}
		//	//y = y + 1;
		//	//if(y < 3)
		//	//{
		//	//	x = x + 1;
		//	//}
		//	//else
		//	//{
		//	//	y = 0;
		//	//	x = x + 0x00FE;
		//	//}
		//	j++;
		//	//if(j == 12)
		//	//	j = 60;
		//	//if(j == 64)
		//	//	j = 72;
		//	//if(j == 85)
		//	//	j = 91;
		//	//if(j > 107)
		//	//{
		//	//	j = 4;
		//	//	k++;
		//	//	if(k == 12)
		//	//		k = 60;
		//	//	if(k == 64)
		//	//		k = 72;
		//	//	if(k == 85)
		//	//		k = 91;
		//	//	if(k > 107)
		//	//	{
		//	//		k = 4;
		//	//		i++;
		//	//		if(i == 12)
		//	//			i = 60;
		//	//		if(i == 64)
		//	//			i = 72;
		//	//		if(i == 85)
		//	//			i = 91;
		//	//		if(i > 107)
		//	//		{
		//	//			i = 0;
		//	//			h++;
		//	//			hackCounter = -5000;
		//	//		}
		//	//	}
		//	//}
		//}

		//static int hackCounter = 0;
		//static int x = 0;
		//if(target && (hackCounter++) == 128)
		//{
		//	hackCounter = 0;
		//	Message(WHITE, "On: %i", x);
		//	APPLAYER* outapp = new APPLAYER(OP_Action, sizeof(Action_Struct));
		//	Action_Struct* action = (Action_Struct*)outapp->pBuffer;
		//	action->damage = 0;
		//	action->spell = 300;
		//	action->source = (IsClient() && CastToClient()->GMHideMe()) ? 0 : GetID();
		//	action->target = target->GetID();
		//	action->type = x;
		//	QueuePacket(outapp);
		//	safe_delete(outapp);
		//	x++;
		//}

		//Yeahlight: Projectile stress test code
		if(projectileStressTest)
		{
			static int countDown = 0;
			static int j = 1;
			if(countDown++ == 3)
			{
				countDown = 0;
				//Yeahlight: Create the new projectile object
				Projectile* p = new Projectile();
				p->SetX(GetX());
				p->SetY(GetY());
				p->SetZ(GetZ());
				p->SetVelocity(0.50);
				p->SetHeading(heading * 2 + projectileStressTestCounter - 50);
				p->SetSpellID(projectileStressTestID);
				p->SetType(9);
				p->SetSourceID(GetID());
				p->SetTexture("GENW00");
				p->SetLightSource(4);
				p->SetTargetID(0);
				p->SetYaw(0.001);
				p->BuildCollisionList(NULL, 125);
				entity_list.AddProjectile(p);
				p->BuildProjectile(this);
				projectileStressTestCounter--;
				if(projectileStressTestCounter == 0)
					projectileStressTest = false;
			}
		}

		//Yeahlight: PC is feared
		if(GetFeared())
		{
			//Yeahlight: PC is in range of their destination
			if(DistanceToLocation(fear_walkto_x, fear_walkto_y, fear_walkto_z) <= 15)
			{
				GetFearDestination(GetX(), GetY(), GetZ());
			}
		}
		//Yeahlight: Time for this PC to attack with its primary weapon
		else if(attack_timer->Check() || reservedPrimaryAttack)
		{
			//Yeahlight: Client has a target to attack
			if(target && (auto_attack || (IsCharmed() && GetOwner() && GetOwner()->GetID() != target->GetID())))
			{
				//Yeahlight: Drop the reserved attack
				reservedPrimaryAttack = false;

				//Yeahlight: PC is using their fist or an item with damage
				if(weapon1 == NULL || (weapon1 && weapon1->common.damage))
				{
					float hitBoxModifier = 1.00;
					//Yeahlight: Mob is mobile, give the PC a hitbox bonus
					if(target->GetAnimation())
					{
						hitBoxModifier = 3.00;
					}
					//Yeahlight: PC is out of melee range
					if(DistNoRootNoZ(target) > (10*10 + HIT_BOX_MULTIPLIER * target->GetHitBox()) * hitBoxModifier)
					{
						Message(RED, "Your target is too far away, get closer!");
					}
					//Yeahlight: PC is targeting themself
					else if(target == this)
					{
						//Yeahlight: Client handles this message
					}
					//Yeahlight: PC is not looking at their target
					else if(CanNotSeeTarget(target->GetX(), target->GetY()))
					{
						Message(RED, "You can't see your target from here.");                              
					}
					//Yeahlight: PC may attack target and the target is still alive
					else if(target->IsAlive())
					{
						Attack(target, 13);

						// Kaiyodo - support for double attack. Chance based on formula from Monkly business
						if(CanThisClassDoubleAttack())
						{
							float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max

							// Check for double attack with main hand assuming maxed DA Skill (MS)
							float random = (float)rand()/RAND_MAX;

							if(random < DoubleAttackProbability) // Max 62.4 % chance of DA
							{
								if(target && target->IsAlive())
								{
									Attack(target, 13, false);
								}
							}
						}
					}
				}
				else if(GetDebugMe())
				{
					Message(LIGHTEN_BLUE, "Debug: You are attempting to attack with a non-weapon in your primary slot.");
				}
			}
			//Yeahlight: Client does not have a target to attack
			else
			{
				//Yeahlight: Attack did not happen, so reserve the attack
				reservedPrimaryAttack = true;
			}
		}
		//Yeahlight: Time for this PC to attack with its secondary weapon
		else if(attack_timer_dw->Check() || reservedSecondaryAttack)
		{
			//Yeahlight: Client has a target to attack
			if(CanThisClassDuelWield() && target && (auto_attack || (IsCharmed() && GetOwner() && GetOwner()->GetID() != target->GetID())))
			{
				//Yeahlight: Drop the reserved attack
				reservedSecondaryAttack = false;

				//Yeahlight: PC is using their fist or an item with damage
				if(weapon2 == NULL || (weapon2 && weapon2->common.damage))
				{
					float hitBoxModifier = 1.00;
					//Yeahlight: Mob is mobile, give the PC a hitbox bonus
					if(target->GetAnimation())
					{
						hitBoxModifier = 3.00;
					}
					//Yeahlight: PC is out of melee range
					if(DistNoRootNoZ(target) > (10*10 + HIT_BOX_MULTIPLIER * target->GetHitBox()) * hitBoxModifier)
					{
						Message(RED, "Your target is too far away, get closer!");
					}
					//Yeahlight: PC is targeting themself
					else if (target == this)
					{
						//Yeahlight: Client handles this message
					}
					//Yeahlight: PC is not looking at their target
					else if(CanNotSeeTarget(target->GetX(), target->GetY()))
					{
						Message(RED, "You can't see your target from here.");
					}
					//Yeahlight: PC may attack target and the target is still alive
					else if(target->IsAlive())
					{
						float DualWieldProbability = (GetSkill(DUEL_WIELD) + GetLevel()) / 400.0f; // 78.0 max

						float random = (float)rand()/RAND_MAX;

						if(random < DualWieldProbability)		// Max 78% of DW
						{
							Attack(target, 14);	// Single attack with offhand

							float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max

							// Check for double attack with off hand assuming maxed DA Skill
							random = (float)rand()/RAND_MAX;

							if(random <= DoubleAttackProbability)	// Max 62.4% chance of DW/DA
							{
								if(target && target->IsAlive())
								{
									Attack(target, 14, false);
								}
							}
						}
					}
				}
				else if(GetDebugMe())
				{
					Message(LIGHTEN_BLUE, "Debug: You are attempting to attack with a non-weapon in your secondary slot.");
				}
			}
			//Yeahlight: Client does not have a target to attack
			else
			{
				reservedSecondaryAttack = true;
			}
		}

		//Yeahlight: TODO: Double check this
		// maalanar 2008-02-05: no need for the server to refresh player position, this only causes jumpyness for the viewing clients
		//if (position_timer->Check())
		//{
		//	SendPosUpdate();

		//	// Send position updates, whole zone every 3 seconds, close mobs every 150ms
		//	if (position_timer_counter > 36) 
		//	{
		//		entity_list.SendPositionUpdates(this, pLastUpdate);
		//		position_timer_counter = 0;
		//	}
		//	else
		//	{
		//		entity_list.SendPositionUpdates(this, pLastUpdate, 400, target);
		//	}

		//	pLastUpdate = Timer::GetCurrentTime();
		//	position_timer_counter++;
		//}

		SpellProcess();

		//Yeahlight: PC has rain spells going
		if(rain_timer->Check())
		{
			//Yeahlight: Execute the rain pulses; if false, then the array is empty, so stop the timer
			if(DoRainWaves() == false)
			{
				rain_timer->Disable();
			}
		}

		//The 6 second tic timer. Still need to add mana regen to the DoRegen();
		if(mana_timer->Check())
		{
			//If you are not very thirsty/hungry
			if(pp.hungerlevel != 0 && pp.thirstlevel != 0)
			{
				this->DoManaRegen();
			}
		}

		//Endurace check
		if(endu_timer->Check())
		{
			this->DoEnduRegen();
		}

		//Yeahlight: Tic process timer (HP regen tics, spell tics)
		if(ticProcess_timer->Check())
		{
			DoHPRegen();
		}
		//Moraj - Uncommented for Bind Wound functionality, Tested
		if (bindwound_timer->Check() && bindwound_target != 0) {
			BindWound(bindwound_target, false);
		}

		//Harakiri: check fishing timer		
		if (fishing_timer->Check()) {
			GoFish();
		}
		//Harakiri: check instilldoubt timer
		if (instilldoubt_timer->Check()) {
			DoInstillDoubt();
		}

	}

	/************ Get all packets from packet manager out queue and process them ************/
	APPLAYER *app = 0;
	while(ret && (app = PMOutQueuePop()))
	{
		if (app->opcode == 0)
		{
			safe_delete(app);//delete app;
			continue;
		}

		switch(client_state)
		{
		case CLIENT_CONNECTING1:
			{
				this->Process_ClientConnection1(app);
				break;
			}
		case CLIENT_CONNECTING2: 
			{
				if (app->opcode == OP_ZoneEntry)
				{
					if(!this->QuagmireGhostCheck(app))
					{
						ret = false;
					}
					else
					{
					this->Process_ClientConnection2(app);
					}
				}
				else if (app->opcode == OP_SetDataRate)
				{
					//We don't use datarate yet.
				}
				//Recycle packet until client is connected...
				else if (app->opcode == OP_WearChange)
				{
					//QueuePacket(app);
				}
				else
				{
					cout << "Unexpected packet during CLIENT_CONNECTING2: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
					DumpPacket(app);
				}
				break;
			}
		case CLIENT_CONNECTING3:
			{
				this->Process_ClientConnection3(app);
				break;
			}
		case CLIENT_CONNECTING4:
			{
				this->Process_ClientConnection4(app);
				break;
			}
		case CLIENT_CONNECTING5:
			{
				this->Process_ClientConnection5(app);
				break;
			}
		case CLIENT_CONNECTED:
			{
				this->ProcessOpcode(app);
				break;
			}
		case CLIENT_KICKED:
		case CLIENT_DISCONNECTED:
			{
				continue; // addign the below code into these switch/case blocks, causesa loop of client disconnects
				// for the same user over and over and over and over.... -Dark-Prince
			}
			break;
		default:
			{
				cerr << "Unknown client_state:" << (int16) client_state << endl;
				break; 
			}
		}
		safe_delete(app);//delete app;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}   

	if (this->client_state == CLIENT_KICKED)
	{
		EQC::Common::PrintF(CP_CLIENT, "Client Kicked : %s\n", this->GetName());

		this->Disconnect();

		PMClose();
		//QueuePacket(0);
	}

	if (this->client_state == CLIENT_DISCONNECTED)
	{
		EQC::Common::PrintF(CP_CLIENT, "Client disconnected : %s\n", this->GetName());
		if(!this->isZoning) 
		{
			this->Disconnect();
		}
		return false;
	}


	if (timeout_timer->Check())
	{
		EQC::Common::PrintF(CP_CLIENT, "Client timeout: %s\n", this->GetName());

		this->Disconnect();

		return false;
	}

	return ret;
}

// Handle Disconnection of the clinet smoothly - Dark-Prince
void Client::Disconnect()
{
	// Is this Client in a Group?
	if(this->IsGrouped())
	{
		// Get the Group Instance
		Group* g = entity_list.GetGroupByClient(this);

		// Verifty this client is a group member
		if(g && g->IsGroupMember(this))
		{
			// Remove this client from the group
			g->DelMember(this->GetName(), true, true);
		}
	}

	// Should we be calling a Save() here?
}

void Client::SetAttackTimer() 
{
	bool debugFlag = true;
	float attackSpeedModifier = 0;
	Timer *TimerToUse;
	Item_Struct *ItemToUse;
	
	//Yeahlight: Spell haste and slow require two distinct formulas
	//           TODO: I do not believe the slow calculation is correct
	if(GetHaste() >= 0)
		attackSpeedModifier = 100.0f / (100.0f + GetHaste());
	else
		attackSpeedModifier = 1.00f + (float)((float)(abs(GetHaste()))) / 100.00f;

	//Yeahlight: Loop through each hand
	for(int i = 13 ; i <= 14 ; i++)
	{
		//Yeahlight: Currently on the main hand
		if(i == 13)
			TimerToUse = attack_timer;
		else
			TimerToUse = attack_timer_dw;

		//Yeahlight: Grab the item from the database
		ItemToUse = Database::Instance()->GetItem(pp.inventory[i]);

		if(debugFlag && ItemToUse && GetDebugMe())
			Message(LIGHTEN_BLUE, "Debug: Calculating delay in %s %s", i > 13 ? "off-hand:  " : "main-hand:", ItemToUse->name);
		else if(debugFlag && !ItemToUse && GetDebugMe())
			Message(LIGHTEN_BLUE, "Debug: Calculating delay in %s Fist", i > 13 ? "off-hand:  " : "main-hand:", ItemToUse->name);

		//Yeahlight: No item was found, client is using their fist
		if(!ItemToUse)
		{
			int8 tmpClass = GetClass();

			//Work out if we're a monk
			if(tmpClass == MONK)
			{
				int speed = (int)(GetMonkHandToHandDelay() * 100.0f * attackSpeedModifier);
				//Jester: Check for epic fists.
				if(pp.inventory[12] == 10652) 
					speed = (int)(16 * 100.0f * attackSpeedModifier);
				TimerToUse->SetTimer(speed);
				if(GetDebugMe())
					Message(LIGHTEN_BLUE, "Debug: Your new attack delay............ %ims", speed);
			}
			else
			{
				int newDelay = 3600 * attackSpeedModifier;
				//Hand to hand, non-monk 2/36
				TimerToUse->SetTimer(newDelay);
				if(GetDebugMe())
					Message(LIGHTEN_BLUE, "Debug: Your new attack delay............ %ims", newDelay);
			}
		}
		else
		{
			int newDelay = (int)ItemToUse->common.delay * (100.0f * attackSpeedModifier);
			if(GetDebugMe())
				Message(LIGHTEN_BLUE, "Debug: Your new attack delay............ %ims", newDelay);
			TimerToUse->SetTimer(newDelay);
		}
	}
}

void Client::SendInventoryItems() 
{
	int i;
	Item_Struct* item = 0;
	Item_Struct* outitem = 0;
	for (i = 0; i < 30; i++) 
	{
		item = Database::Instance()->GetItem(pp.inventory[i]);

		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.inventory[i]=0xFFFF;
				if(item->type==0x01){//If its a bag we delete all the items in it
					if(i!=0){//if its not the cursor bag
						for(int j=0;j<10;j++){
							pp.containerinv[(22-i)*10+j]=0xFFFF;
						}
					}
					else{//it is the cursor bag
						for(int j=0;j<10;j++){
							pp.cursorbaginventory[j]=0xFFFF;
						}
					}
				}
				item=NULL;
			}
		}

		if (item) 
		{
			//cout << "Sending inventory slot:" << i << " Item:" << item->name <<endl;

			if(item->common.stackable==1){//Tazadar load the number in each stack for inventory (not container)
				//cout << "number in stack is " << (int) pp.invItemProprieties[i].charges << "of Item:" << item->name <<endl;
				item->common.charges=pp.invItemProprieties[i].charges;
			}
			APPLAYER* app = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));
			memcpy(app->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app->pBuffer;
			outitem->equipSlot = i;
			QueuePacket(app);
			safe_delete(app);//delete app;
		}
	}

	// Cursor bag slots are 330->339 (10 slots)
	for (i=0;i<10; i++) {
		item = Database::Instance()->GetItem(pp.cursorbaginventory[i]);

		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.cursorbaginventory[i]=0xFFFF;
				item=NULL;
			}
		}

		if (item) {


			//cout << "Sending inventory slot:" << i << " Item:" << item->name <<endl;

			if(item->common.stackable==1){//Tazadar load the number in each stack for inventory (not container)
				//cout << "number in stack is " << (int) pp.invItemProprieties[i].charges << "of Item:" << item->name <<endl;
				item->common.charges=pp.cursorItemProprieties[i].charges;
			}
			APPLAYER* app = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));
			memcpy(app->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app->pBuffer;
			outitem->equipSlot = 330+i;
			QueuePacket(app);
			safe_delete(app);//delete app;
		}
	}

	// Coder_01's container code
	for (i=0; i < 80; i++) 
	{
		item = Database::Instance()->GetItem(pp.containerinv[i]);
		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.containerinv[i]=0xFFFF;
				item=NULL;
			}
		}
		if (item) 
		{
			if(item->common.stackable==1){//Tazadar load the number in each stack for inventory (not container)
				//cout << "number in stack in the bag " << (int) pp.bagItemProprieties[i].charges << "of Item:" << item->name <<endl;
				item->common.charges=pp.bagItemProprieties[i].charges;
			}

			APPLAYER* app = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));

			memcpy(app->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app->pBuffer;
			outitem->equipSlot = 250 + i;
			QueuePacket(app);
			safe_delete(app);//delete app;
		}
	}

	// Quagmire - Bank code, these should be the proper pp locs too
	for (i=0; i < 8; i++) 
	{
		item = Database::Instance()->GetItem(pp.bank_inv[i]);
		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.bank_inv[i]=0xFFFF;
				if(item->type==0x01){//If its a bag we delete all the items in it
					for(int j=0;j<10;j++){
						pp.bank_cont_inv[i*10+j]=0xFFFF;
					}
				}
				item=NULL;
			}
		}
		if (item) 
		{

			if(item->common.stackable==1){//Tazadar load the number in each stack for bag (not container)
				//cout << "number in stack is " << (int) pp.bankinvitemproperties[i].charges << "of Bank Item:" << item->name <<endl;
				item->common.charges=pp.bankinvitemproperties[i].charges;
			}

			APPLAYER* app = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));

			memcpy(app->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app->pBuffer;
			outitem->equipSlot = 2000 + i;
			QueuePacket(app);
			safe_delete(app);//delete app;

		}
	}

	for (i=0; i < 80; i++) 
	{
		item = Database::Instance()->GetItem(pp.bank_cont_inv[i]);
		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.bank_cont_inv[i]=0xFFFF;
				item=NULL;
			}
		}
		if (item) 
		{
			if(item->common.stackable==1){//Tazadar load the number in each stack for inventory (not container)
				//cout << "number in stack is " << (int) pp.bankbagitemproperties[i].charges << "of Bank Item:" << item->name <<endl;
				item->common.charges=pp.bankbagitemproperties[i].charges;
			}
			APPLAYER* app = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));

			memcpy(app->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app->pBuffer;
			outitem->equipSlot = 2030 + i;
			QueuePacket(app);
			safe_delete(app);//delete app;
		}
	}
}


void Client::SendInventoryItems2() 
{
	int i;
	Item_Struct* item = 0;
	Item_Struct* outitem = 0;

	BulkedItem_Struct tmpitems[250];
	int8 numberofitems = 0;

	for (i = 0; i < 30; i++) 
	{
		item = Database::Instance()->GetItem(pp.inventory[i]);

		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.inventory[i]=0xFFFF;
				if(item->type==0x01){//If its a bag we delete all the items in it
					if(i!=0){//if its not the cursor bag
						for(int j=0;j<10;j++){
							pp.containerinv[(22-i)*10+j]=0xFFFF;
						}
					}
					else{//it is the cursor bag
						for(int j=0;j<10;j++){
							pp.cursorbaginventory[j]=0xFFFF;
						}
					}
				}
				item=NULL;
			}
		}

		if (item) 
		{
			//cout << "Sending inventory slot:" << i << " Item:" << item->name <<endl;

			if(item->common.stackable==1){//Tazadar load the number in each stack for inventory (not container)
				//cout << "number in stack is " << (int) pp.invItemProprieties[i].charges << "of Item:" << item->name <<endl;
				item->common.charges=pp.invItemProprieties[i].charges;
			}
			if(item->type==0x01)
				tmpitems[numberofitems].opcode = 0x6621;
			else if(item->type==0x02)
				tmpitems[numberofitems].opcode = 0x6521;
			else
				tmpitems[numberofitems].opcode = 0x6421;
			item->equipSlot = i;
			memcpy(&tmpitems[numberofitems].item, item, sizeof(Item_Struct));
			numberofitems++;
		}
	}

	// Cursor bag slots are 330->339 (10 slots)
	for (i=0;i<10; i++) {
		item = Database::Instance()->GetItem(pp.cursorbaginventory[i]);

		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.cursorbaginventory[i]=0xFFFF;
				item=NULL;
			}
		}

		if (item) {


			//cout << "Sending inventory slot:" << i << " Item:" << item->name <<endl;

			if(item->common.stackable==1){//Tazadar load the number in each stack for inventory (not container)
				//cout << "number in stack is " << (int) pp.invItemProprieties[i].charges << "of Item:" << item->name <<endl;
				item->common.charges=pp.cursorItemProprieties[i].charges;
			}

			if(item->type==0x02)
				tmpitems[numberofitems].opcode = 0x6521;
			else
				tmpitems[numberofitems].opcode = 0x6421;

			item->equipSlot = 330+i;
			memcpy(&tmpitems[numberofitems].item, item, sizeof(Item_Struct));
			numberofitems++;
		}
	}

	// Coder_01's container code
	for (i=0; i < 80; i++) 
	{
		item = Database::Instance()->GetItem(pp.containerinv[i]);
		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.containerinv[i]=0xFFFF;
				item=NULL;
			}
		}
		if (item) 
		{
			if(item->common.stackable==1){//Tazadar load the number in each stack for inventory (not container)
				//cout << "number in stack in the bag " << (int) pp.bagItemProprieties[i].charges << "of Item:" << item->name <<endl;
				item->common.charges=pp.bagItemProprieties[i].charges;
			}
			if(item->type==0x02)
				tmpitems[numberofitems].opcode = 0x6521;
			else
				tmpitems[numberofitems].opcode = 0x6421;

			item->equipSlot = 250 + i;
			memcpy(&tmpitems[numberofitems].item, item, sizeof(Item_Struct));
			numberofitems++;
		}
	}

	// Quagmire - Bank code, these should be the proper pp locs too
	for (i=0; i < 8; i++) 
	{
		item = Database::Instance()->GetItem(pp.bank_inv[i]);
		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.bank_inv[i]=0xFFFF;
				if(item->type==0x01){//If its a bag we delete all the items in it
					for(int j=0;j<10;j++){
						pp.bank_cont_inv[i*10+j]=0xFFFF;
					}
				}
				item=NULL;
			}
		}
		if (item) 
		{
			if(item->type==0x01)
				tmpitems[numberofitems].opcode = 0x6621;
			else if(item->type==0x02)
				tmpitems[numberofitems].opcode = 0x6521;
			else
				tmpitems[numberofitems].opcode = 0x6421;

			if(item->common.stackable==1){//Tazadar load the number in each stack for bag (not container)
				//cout << "number in stack is " << (int) pp.bankinvitemproperties[i].charges << "of Bank Item:" << item->name <<endl;
				item->common.charges=pp.bankinvitemproperties[i].charges;
			}

			item->equipSlot = 2000 + i;
			memcpy(&tmpitems[numberofitems].item, item, sizeof(Item_Struct));
			numberofitems++;

		}
	}

	for (i=0; i < 80; i++) 
	{
		item = Database::Instance()->GetItem(pp.bank_cont_inv[i]);
		if(item && item->norent==0){ //If we have a no rent item and loged of for more than 30 mins we remove item
			if((time(0)-pp.logtime)>1800){
				pp.bank_cont_inv[i]=0xFFFF;
				item=NULL;
			}
		}
		if (item) 
		{
			if(item->common.stackable==1){//Tazadar load the number in each stack for inventory (not container)
				//cout << "number in stack is " << (int) pp.bankbagitemproperties[i].charges << "of Bank Item:" << item->name <<endl;
				item->common.charges=pp.bankbagitemproperties[i].charges;
			}
			if(item->type==0x02)
				tmpitems[numberofitems].opcode = 0x6521;
			else
				tmpitems[numberofitems].opcode = 0x6421;
			item->equipSlot = 2030 + i;
			memcpy(&tmpitems[numberofitems].item, item, sizeof(Item_Struct));
			numberofitems++;
		}
	}
	APPLAYER* app = new APPLAYER(0xf621, sizeof(int16)+numberofitems*sizeof(BulkedItem_Struct));

	int16* packetNum = (int16*) app->pBuffer;
	packetNum[0] = numberofitems ;

	memcpy(app->pBuffer+sizeof(int16),tmpitems,numberofitems*sizeof(BulkedItem_Struct));

	DumpPacket(app);
	QueuePacket(app);
	safe_delete(app);
}

bool Client::CanThisClassDuelWield(void)
{
	bool bresult = false;

	// Kaiyodo - Check the classes that can DW, and make sure we're not using a 2 hander
	switch(GetClass())
	{
	case WARRIOR:
	case MONK:
	case RANGER:
	case ROGUE:
	case BARD:
	case BEASTLORD:
		{
			// Check the weapon in the main hand, see if it's a 2 hander
			uint16 item_id = pp.inventory[13];
			Item_Struct* item = Database::Instance()->GetItem(item_id);

			// 2HS, 2HB or 2HP
			if(item && (item->common.itemType == ItemType2HS || item->common.itemType == ItemType2HB || item->common.itemType == ItemType2HPierce))
			{
				break;
			}
		}
		bresult = (GetSkill(DOUBLE_ATTACK) != 0);	// No skill = no chance
		break;
	default:
		bresult = false;
		break;
	}
	return bresult;
}

bool Client::CanThisClassDoubleAttack(void)
{
	bool bresult = false;
	// Kaiyodo - Check the classes that can DA
	switch(GetClass())
	{
	case WARRIOR:
	case MONK:
	case RANGER:
	case PALADIN:
	case SHADOWKNIGHT:
	case ROGUE:
		bresult =  (GetSkill(DUEL_WIELD) > 0);//DB - changed from != to > // return(GetSkill(DUEL_WIELD) != 0);	// No skill = no chance
		break;
	default:
		bresult = false;
		break;
	}
	return bresult;
}

//Yeahlight: TODO: This is a god damn mess. I know all of this is not necessary; clean this up.
//Harakiri: Im 99% certain the client never sents the x,y,z locs on a zonechange, read my ZoneChange_Struct
//i spent 10h debugging the translocate/teleport process - there was no evidence for it
// what that means is, our server always has to remember the x,y,z coord the clients wants to zone to
void Client::ProcessOP_ZoneChange(APPLAYER* pApp)
{
	if(pApp->size != sizeof(ZoneChange_Struct))
	{
		cout << "Wrong size on ZoneChange: " << pApp->size << ", should be " << sizeof(ZoneChange_Struct) << " on 0x" << hex << setfill('0') << setw(4) << pApp->opcode << dec << endl;
		return;
	}


	//EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Dumping ProcessOP_ZoneChange_Struct, packet size: %d", pApp->size);
	//DumpPacket(pApp);

	ZoneChange_Struct* zc = (ZoneChange_Struct*)pApp->pBuffer;

	if(strcasecmp(zc->zone_name, "zonesummon") == 0)
	{
		strcpy(zc->zone_name, zonesummon_name);
	}

	float tarx = 0, tary = 0, tarz = 0;
	int8 minstatus = 0;
	int8 minlevel = 0;
	char tarzone[16] = "";
	cout << "Zone request for:" << zc->char_name << " to:" << zc->zone_name << endl;

	if(Database::Instance()->GetSafePoints(zc->zone_name, &tarx, &tary, &tarz, &minstatus, &minlevel))
	{
		strcpy(tarzone, zc->zone_name);
	}

	if(zonesummon_ignorerestrictions) 
	{
		minstatus = 0;
		minlevel = 0;
	}

	zonesummon_ignorerestrictions = false;

	cout << "Player at x:" << GetX() << " y:" << GetY() << " z:" << GetZ() << endl;
	ZonePoint* zone_point = zone->GetClosestZonePoint(GetY(), GetX(), GetZ(), zc->zone_name);

	//Yeahlight: First, check if our client is zoning via ZonePC
	if(this->usingSoftCodedZoneLine)
	{
		
		tarx = zoningX;
		tary = zoningY;
		tarz = zoningZ;
		EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Client::ProcessOP_ZoneChange(Zoning with soft coded values zone name = %s, x = %f, y = %f, z = %f, heading = %f)", zc->zone_name, tarx, tary, tarz);

	}
	// -1, -1, -1 = code for zone safe point
	else if(zonesummon_x == -1 && zonesummon_y == -1 && (zonesummon_z == -1 || zonesummon_z == -10)) 
	{
		cout << "Zoning to safe coords: " << tarzone << ", x=" << tarx << ", y=" << tary << ", z=" << tarz << endl;
		zonesummon_x = -2;
		zonesummon_y = -2;
		zonesummon_z = -2;
	}
	// -3 -3 -3 = bind point
	else if(zonesummon_x == -3 && zonesummon_y == -3 && (zonesummon_z == -3 || zonesummon_z == -30)) 
	{
		strcpy(tarzone, pp.bind_point_zone); //copy bindzone to target
		//Yeahlight: Client location is backwards! (Y, X, Z)
		tarx = pp.bind_location[1][0]; //copy bind cords to target
		tary = pp.bind_location[0][0];
		tarz = pp.bind_location[2][0];
		cout << "Zoning: Death, zoning to bind point: " << tarzone << ", x=" << tarx << ", y=" << tary << ", z=" << tarz << endl;
		zonesummon_x = -2;
		zonesummon_y = -2;
		zonesummon_z = -2;
		minstatus = 0;
		minlevel = 0;
		//Yeahlight: I moved this process elsewhere
		//SetMaxHP();// Tazadar :If you dont add that you often spawn with no hp !!!
		//this->SetMana(this->CalcMaxMana()); // Tazadar same for mana !!!
	}
	// if not -2 -2 -2, zone to these coords. -2, -2, -2 = not a zonesummon zonerequest
	else if(!(zonesummon_x == -2 && zonesummon_y == -2 && (zonesummon_z == -2 || zonesummon_z == -20))) 
	{
		tarx = zonesummon_x;
		tary = zonesummon_y;
		tarz = zonesummon_z;
		cout << "Zoning to specified cords: " << tarzone << ", x=" << tarx << ", y=" << tary << ", z=" << tarz << endl;
		zonesummon_x = -2;
		zonesummon_y = -2;
		zonesummon_z = -2;
	}
	else if(zone_point != 0)
	{
		cout << "Zone point found at x:" << zone_point->x << " y:" << zone_point->y << " z:" << zone_point->z << endl;
		tarx = zone_point->target_x;
		tary = zone_point->target_y;
		tarz = zone_point->target_z;
		this->isZoningZP = true;
		//Yeahlight: If we are not using a custom coordinate, then use the current heading
		if(tarx != 0 && tary != 0 && tarz != 0)
			this->tempHeading = zone_point->heading;
		else
			this->tempHeading = heading;
		this->zoningX = tarx;
		this->zoningY = tary;
		this->zoningZ = tarz;
	}
	//Yeahlight: Succor / escape spell
	else if(strcmp(zc->zone_name, zone->GetShortName()) == 0)
	{
		cout<<"Zoning with escape spell"<<endl;
		strcpy(tarzone, zc->zone_name);
	}
	else if(pendingTranslocate)
	{		
		cout<<"Zoning with translocate"<<endl;
		strcpy(tarzone, zc->zone_name);
		tarx = pendingTranslocateX;
		tary = pendingTranslocateY;
		tarz = pendingTranslocateZ;
		//translocate was accepted and we are zoning them so set the pending translocate to false
		pendingTranslocate = false;
	}
	else
	{
		cout<<"Zoning with invalid protocol"<<endl;
		tarzone[0] = 0;
	}

	if(admin < minstatus)
	{
		Message(RED, "You are not awesome enough to enter %s.", tarzone);
	}
	else if(GetLevel() < minlevel)
	{
		Message(RED, "Your will is not strong enough to enter %s", tarzone);
	}

	APPLAYER* outapp;
	if(tarzone[0] != 0 && this->admin >= minstatus && this->GetLevel() >= minlevel)
	{
		if(this->IsGrouped())
		{
			Group* g = entity_list.GetGroupByClient(this);
			if(g)
				g->MemberZonedOut(this);
		}
		//cout << "Zone target:" << tarzone << " x:" << tarx << " y:" << tary << " z:" << tarz << endl;

		outapp = new APPLAYER(OP_CancelTrade, 0);
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;

		outapp = new APPLAYER;
		outapp->opcode = 0x1020;
		outapp->size = 0;
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;

		outapp = new APPLAYER(OP_ZoneChange, sizeof(ZoneChange_Struct));
		memset(outapp->pBuffer, 0, sizeof(ZoneChange_Struct));
		ZoneChange_Struct *zc2 = (ZoneChange_Struct*)outapp->pBuffer;
		strcpy(zc2->char_name, zc->char_name);
		strcpy(zc2->zone_name, tarzone);
		//								memcpy(zc2->zc_unknown1, zc->zc_unknown1, sizeof(zc->zc_unknown1));

		zc2->zc_unknown1[0] = 0x10;
		zc2->zc_unknown1[1] = 0x00; 
		zc2->zc_unknown1[2] = 0x00;
		zc2->zc_unknown1[3] = 0x00;
		zc2->zc_unknown1[4] = 0x04;
		zc2->zc_unknown1[5] = 0xb5; 
		zc2->zc_unknown1[6] = 0x01;
		zc2->zc_unknown1[7] = 0x02; 
		zc2->zc_unknown1[8] = 0x43; 
		zc2->zc_unknown1[9] = 0x58; 
		zc2->zc_unknown1[10] = 0x4f;
		zc2->zc_unknown1[11] = 0x00; 
		zc2->zc_unknown1[12] = 0xb0; 
		zc2->zc_unknown1[13] = 0xa5; 
		zc2->zc_unknown1[14] = 0xc7; 
		zc2->zc_unknown1[15] = 0x0d; 
		zc2->zc_unknown1[16] = 0x01;
		zc2->zc_unknown1[17] = 0x00;
		zc2->zc_unknown1[18] = 0x00;
		zc2->zc_unknown1[19] = 0x00;


		// The client seems to dump this profile on us, but I ignore it for now. Saving is client initiated?
		x_pos = tarx; // Hmm, these coordinates will now be saved when ~client is called
		y_pos = tary;
		z_pos = tarz;

		strcpy(pp.current_zone, tarzone);
		this->Save();

		Database::Instance()->SetAuthentication(account_id, zc->char_name, tarzone, ip); // We have to tell the world server somehow?
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;
	}
	else
	{
		//cerr << "Zone '" << zc->zone_name << "' is not available" << endl;

		outapp = new APPLAYER(OP_CancelTrade, 0);
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;

		outapp = new APPLAYER;
		outapp->opcode = 0x1020;
		outapp->size = 0;
		QueuePacket(outapp);					
		safe_delete(outapp);//delete outapp;

		outapp = new APPLAYER(OP_ZoneChange, sizeof(ZoneChange_Struct));
		memset(outapp->pBuffer, 0, sizeof(ZoneChange_Struct));
		ZoneChange_Struct *zc2 = (ZoneChange_Struct*)outapp->pBuffer;
		strcpy(zc2->char_name, zc->char_name);
		strcpy(zc2->zone_name, zc->zone_name);
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;

		isZoning = false;
		isZoningZP = false;
		usingSoftCodedZoneLine = false;
		//MovePC(0, zone->GetSafeX(), zone->GetSafeY(), zone->GetSafeZ(), false, false);
	}
}

//Yeahlight: TODO: What?? What does any of this mean? We only PMClose() GMs? Why?
void Client::ProcessOP_Camp(APPLAYER* pApp)
{
	Save();

	if(admin < 100)
		return;

	//APPLAYER* SpawnApperance = this->SendAppearancePacket(this->GetID(), 0x10, 0);
	this->SendAppearancePacket(0, SAT_Camp, 0, false); // The orginal code did not set spawn_id.... so i;ve left this one here

	PMClose();
	//QueuePacket(0);
}

void Client::ProcessOP_YellForHelp(APPLAYER* pApp)
{
	cout << name << " yells for help." << endl;
	entity_list.QueueCloseClients(this, pApp, true, 100.0);
	// TODO: write a msg send routine
}

void Client::ProcessOP_ClientTarget(APPLAYER* pApp)
{
	if (pApp->size == sizeof(ClientTarget_Struct))
	{
		Mob* inTarget = GetTarget();
		ClientTarget_Struct* ct=(ClientTarget_Struct*)pApp->pBuffer;	
		target = entity_list.GetMob(ct->new_target);
		temptarget = entity_list.GetMob(ct->new_target);
		//Yeahlight: Our client does not support decay timers, so we will tell the client a decay time when they target a corpse
		if(target && inTarget != target && target->IsCorpse())
			target->CastToCorpse()->CreateDecayTimerMessage(this);
	} 
	else
	{
		cout << "Wrong size on OP_ClientTarget. Got: " << pApp->size << ", Expected: " << sizeof(ClientTarget_Struct) << endl;
	}
}
/********************************************************************
*                        Tazadar - 8/22/08	                        *
********************************************************************
*                      ProcessOP_CraftingStation	                *
********************************************************************
*  + When the client leaves the station we save the items		    *
*	+ We reopen the station										    *
********************************************************************/

void Client::ProcessOP_CraftingStation(void){

	Object* station=this->GetCraftingStation();
	memcpy(station->stationItems,this->stationItems,sizeof(this->stationItems));
	memcpy(station->stationItemProperties,this->stationItemProperties,sizeof(this->stationItemProperties));
	station->SetOpen(1);
	this->SetCraftingStation(0);

}
/*******************************************************************
*                 Tazadar   : ProcessOP_MoveItem                  *
*******************************************************************
* When you move item in your inventory the serv save the changes  *
*     +Save stack amounts when moving item                        *
*                                                                 *
*******************************************************************/
void Client::ProcessOP_MoveItem(APPLAYER* pApp)
{
	if (pApp->size != sizeof(MoveItem_Struct)) {
		cout << "Wrong size on OP_MoveItem. Got: " << pApp->size << ", Expected: " << sizeof(MoveItem_Struct) << endl;
		return;
	}
	MoveItem_Struct* mi = (MoveItem_Struct*)pApp->pBuffer;

	if (mi->from_slot == mi->to_slot)
		return; // Item summon, no further proccessing needed

	cout << "Moving item from slot " << (int)mi->from_slot << " Moving item to slot "<< (int)mi->to_slot<< endl;
	
	ItemProperties_Struct cursorProp[10];
	uint16 cursorId[10];
	bool bagInCursor=false;

	uint16* to_slot = 0;
	uint16* to_bag_slots = 0;
	uint16* from_slot = 0;
	uint16* from_bag_slots = 0;
	sint8*	from_charges = 0;
	sint8*	to_charges	= 0;
	ItemProperties_Struct*	from_bag_charges = 0;
	ItemProperties_Struct*	to_bag_charges	= 0;
	sint8	chargesmoved = 0;
	sint8	chargesreplaced	= 0;
	uint16 itemmoved = 0xFFFF;
	uint16 itemreplaced = 0xFFFF;
	
	if (mi->to_slot >= 22 && mi->to_slot <= 29){
		Item_Struct* item =Database::Instance()->GetItem(pp.inventory[mi->to_slot]);
		if(item && item->type == 0x01){
			memset(cursorProp,0,10*sizeof(ItemProperties_Struct));
			memset(cursorId,0xFFFF,10*sizeof(uint16));
			bagInCursor=true;
			for(int i=0;i<10;i++){
				cursorId[i]=pp.containerinv[((mi->to_slot-22)*10)+i];
				cursorProp[i].charges=pp.bagItemProprieties[((mi->to_slot-22)*10)+i].charges;
			}
		}
	}

	if (mi->to_slot == 0 || (mi->to_slot >= 22 && mi->to_slot <= 29)) {
		to_slot = &(pp.inventory[mi->to_slot]);
		to_charges =&(pp.invItemProprieties[mi->to_slot].charges);
		if (mi->to_slot == 0){
			to_bag_slots = &pp.cursorbaginventory[0];
			to_bag_charges =&(pp.cursorItemProprieties[0]);
		}
		else{
			to_bag_slots = &pp.containerinv[(mi->to_slot-22)*10];
			to_bag_charges =&(pp.bagItemProprieties[(mi->to_slot-22)*10]);
		}
	}
	else if (mi->to_slot >= 1 && mi->to_slot <= 21){ // Worn items and main inventory
		to_charges =&(pp.invItemProprieties[mi->to_slot].charges);
		to_slot = &(pp.inventory[mi->to_slot]);
	}
	else if (mi->to_slot >= 250 && mi->to_slot <= 329){ // Main inventory's containers
		to_slot = &(pp.containerinv[mi->to_slot-250]);
		to_charges =&(pp.bagItemProprieties[mi->to_slot-250].charges);
	}
	else if (mi->to_slot >= 2000 && mi->to_slot <= 2007) { // Bank slots
		to_slot = &(pp.bank_inv[mi->to_slot-2000]);
		to_bag_slots = &pp.bank_cont_inv[(mi->to_slot-2000)*10];
		to_bag_charges = &pp.bankbagitemproperties[(mi->to_slot-2000)*10];
		to_charges =&(pp.bankinvitemproperties[mi->to_slot-2000].charges);
	}
	else if (mi->to_slot >= 2030 && mi->to_slot <= 2109){ // Bank's containers
		to_slot = &(pp.bank_cont_inv[mi->to_slot-2030]);
		to_charges =&(pp.bankbagitemproperties[mi->to_slot-2030].charges);
	}
	else if (mi->to_slot >= 4000 && mi->to_slot <= 4009){ // Crafting Station
		to_slot = &(this->stationItems[mi->to_slot-4000]);
		to_charges =&(this->stationItemProperties[mi->to_slot-4000].charges);
	}
	else if (mi->to_slot >= 3000 && mi->to_slot < 3016) {
		//cout << "trade item" << endl;
		TradeList[mi->to_slot-3000] = GetItemAt(0);
		TradeCharges[mi->to_slot-3000] = pp.invItemProprieties[0].charges;
		for (int j = 0;j != 10;j++)
		{
			TradeList[((mi->to_slot-2999)*10)+j] = pp.cursorbaginventory[j];

			TradeCharges[((mi->to_slot-2999)*10)+j] = pp.cursorItemProprieties[j].charges;

		}
		to_slot = 0;
		if (this->TradeWithEnt != 0 && entity_list.GetID(this->TradeWithEnt)->IsClient()){
			APPLAYER* outapp = new APPLAYER(OP_ItemToTrade,sizeof(ItemToTrade_Struct));
			ItemToTrade_Struct* ti = (ItemToTrade_Struct*)outapp->pBuffer;
			memset(outapp->pBuffer,0,outapp->size);
			Item_Struct* item = 0;
			item=Database::Instance()->GetItem(TradeList[mi->to_slot-3000]);
			memcpy(&ti->item,item,sizeof(Item_Struct));
			ti->item.common.charges = pp.invItemProprieties[0].charges;
			ti->to_slot = mi->to_slot - 3000;
			ti->playerid = this->TradeWithEnt;
			entity_list.GetID(this->TradeWithEnt)->CastToClient()->QueuePacket(outapp);
			safe_delete(outapp);//delete outapp;
			if(item->type == 0x01){
				//cout << "bag item" << endl;
				for(int j=0; j<10 ;j++){
					Item_Struct* item = 0;
					item=Database::Instance()->GetItem(TradeList[((mi->to_slot-2999)*10)+j]);
					if(item){
						//cout << " showing item in bag " << endl;
						APPLAYER* outapp = new APPLAYER(OP_ItemToTrade,sizeof(ItemToTrade_Struct));
						ItemToTrade_Struct* ti = (ItemToTrade_Struct*)outapp->pBuffer;
						memset(outapp->pBuffer,0,outapp->size);
						memcpy(&ti->item,item,sizeof(Item_Struct));
						ti->item.common.charges = TradeCharges[((mi->to_slot-2999)*10)+j];
						ti->to_slot =30+10*(mi->to_slot-3000)+j;
						ti->playerid = this->TradeWithEnt;
						entity_list.GetID(this->TradeWithEnt)->CastToClient()->QueuePacket(outapp);
						safe_delete(outapp);//delete outapp;
					}
				}
			}
		}

		//cout << "Item is " << TradeList[mi->to_slot-3000] << endl;

	}
	else if (mi->to_slot == 0xFFFFFFFF) // destroy button
		to_slot = 0;
	else {
		Message(BLACK, "Error: OP_MoveItem: Unknown to_slot: 0x%04x", mi->to_slot);
		to_slot = 0;
	}

	if (mi->from_slot == 0 || (mi->from_slot >= 22 && mi->from_slot <= 29)) {
		from_slot = &(pp.inventory[mi->from_slot]);
		from_charges =&(pp.invItemProprieties[mi->from_slot].charges);
		if (mi->from_slot == 0)
		{
			from_bag_slots = &pp.cursorbaginventory[0];
			from_bag_charges =&(pp.cursorItemProprieties[0]);
		}
		else {
			from_bag_slots = &pp.containerinv[(mi->from_slot-22)*10];
			from_bag_charges =&(pp.bagItemProprieties[(mi->from_slot-22)*10]);
		}
	}
	else if (mi->from_slot >= 1 && mi->from_slot <= 21){ // Worn items and main inventory
		from_slot = &(pp.inventory[mi->from_slot]);
		from_charges =&(pp.invItemProprieties[mi->from_slot].charges);
	}
	else if (mi->from_slot >= 250 && mi->from_slot <= 329){ // Main inventory's containers
		from_slot = &(pp.containerinv[mi->from_slot-250]);
		from_charges =&(pp.bagItemProprieties[mi->from_slot-250].charges);
	}
	else if (mi->from_slot >= 2000 && mi->from_slot <= 2007) { // Bank slots
		from_slot = &(pp.bank_inv[mi->from_slot-2000]);
		from_bag_slots = &pp.bank_cont_inv[(mi->from_slot-2000)*10];
		from_bag_charges = &pp.bankbagitemproperties[(mi->from_slot-2000)*10];
		from_charges =&(pp.bankinvitemproperties[mi->from_slot-2000].charges);
	}
	else if (mi->from_slot >= 2030 && mi->from_slot <= 2109){ // Bank's containers
		from_slot = &(pp.bank_cont_inv[mi->from_slot-2030]);
		from_charges =&(pp.bankbagitemproperties[mi->from_slot-2030].charges);
	}
	else if (mi->from_slot >= 4000 && mi->from_slot <= 4009){ // Crafting Station
		from_slot = &(this->stationItems[mi->from_slot-4000]);
		from_charges =&(this->stationItemProperties[mi->from_slot-4000].charges);
	}
	else {
		Message(BLACK, "Error: OP_MoveItem: Unknown from_slot: 0x%04x", mi->from_slot);
		from_slot = 0;
	}

	int i = 0;
	const Item_Struct* tmp = Database::Instance()->GetItem(*from_slot);

	/*if (tmp != 0 && tmp->common.stackable == 1)
	cout << "Item is stackable" << endl;
	else if (tmp != 0)
	cout << "Item is not stackable" << endl;
	else
	cout << "Item is invalid!" << endl;*/

	if (tmp != 0 && tmp->common.stackable == 1 && from_slot != 0 && to_slot != 0 && from_slot != to_slot && *from_slot==*to_slot) {
		//stacks
		*to_charges += (mi->number_in_stack == 0) ? *from_charges:mi->number_in_stack;
		if (*to_charges <= 20){
			*from_slot = 0xFFFF;
			*from_charges -= (mi->number_in_stack == 0) ? 1:mi->number_in_stack;
		}
		else
			*from_charges = *to_charges-20;
	}
	else
	{
		if (from_slot != 0)
		{
			itemmoved = *from_slot;
			chargesmoved = (mi->number_in_stack == 0) ? 1:mi->number_in_stack;
		}
		if (to_slot != 0)
		{
			itemreplaced = *to_slot;
			*to_slot = itemmoved;
			if (to_charges != 0){
				chargesreplaced = *to_charges;
				*to_charges = (mi->number_in_stack == 0) ? *from_charges:mi->number_in_stack;
			}
		}

		if (from_slot != 0)
		{
			if (from_charges != 0)
			{
				*from_charges = (mi->number_in_stack == 0) ? chargesreplaced:*from_charges-mi->number_in_stack;
				if (*from_charges == 0 || itemreplaced != 0xFFFF)
					*from_slot = itemreplaced;
			}
		}
	}
	const Item_Struct* item;
	Item_Struct* outitem = 0;
	for (i=0; i<10; i++) {
		itemmoved = 0xFFFF;
		itemreplaced = 0xFFFF;
		chargesmoved = 0;
		chargesreplaced = 0;

		if (from_bag_charges != 0)
			chargesmoved = from_bag_charges[i].charges;
		if (to_bag_charges != 0)
			chargesreplaced = to_bag_charges[i].charges;
		if (to_bag_charges != 0)
			to_bag_charges[i].charges = chargesmoved;
		if (from_bag_charges != 0)
			from_bag_charges[i].charges = chargesreplaced;

		if (from_bag_slots != 0)
			itemmoved = from_bag_slots[i];
		if (to_bag_slots != 0)
			itemreplaced = to_bag_slots[i];
		if (to_bag_slots != 0)
			to_bag_slots[i] = itemmoved;
		if (from_bag_slots != 0)
			from_bag_slots[i] = itemreplaced;

		if (pp.cursorbaginventory[i] != 0xFFFF && mi->to_slot == 0 && mi->from_slot == 0) {
			item = Database::Instance()->GetItem(pp.cursorbaginventory[i]);
			APPLAYER* app2 = new APPLAYER;
			app2->opcode = 0x3120;
			app2->size = sizeof(Item_Struct);
			app2->pBuffer = new uchar[app2->size];
			memcpy(app2->pBuffer, item, sizeof(Item_Struct));
			outitem = (Item_Struct*) app2->pBuffer;
			outitem->equipSlot = (mi->to_slot-22)*10+250 + i;
			outitem->common.charges = pp.cursorItemProprieties[i].charges;
			QueuePacket(app2);
			safe_delete(app2);//delete app2;

		}
		if (!bagInCursor && pp.cursorbaginventory[i] != 0xFFFF && mi->to_slot != 0){
			pp.cursorbaginventory[i] = 0xFFFF;
			pp.cursorItemProprieties[i].charges =0;
		}
		
		if(bagInCursor){
			pp.cursorbaginventory[i] = cursorId[i];
			pp.cursorItemProprieties[i].charges = cursorProp[i].charges;
		}
		// end
	}

	/*	if (mi->to_slot == 0xFFFFFFFF)
	cout << "Moved " << itemmoved << " from: " << mi->from_slot << " to: destroy button" << endl;
	else
	cout << "Moved " << itemmoved << " from: " << mi->from_slot << " to: " << mi->to_slot << " qty: " << mi->number_in_stack << endl;
	*/
	sint16 tx1 = (to_charges == 0) ? 0:(sint8)*to_charges;
	sint16 tx2 = (from_charges == 0) ? 0:(sint8)*from_charges;
	//cout << "toCharges: " << tx1 << " from_charges: " << tx2 << endl;
	if(pp.inventory[0] == 0xFFFF && this->summonedItems.size() > 0){
		 SummonedItemWaiting_Struct tmp;
		 tmp = this->summonedItems.back();
		 this->summonedItems.pop_back();
		 SummonItem(tmp.itemID,tmp.charge); 
	}
	if ((mi->from_slot >= 1 && mi->from_slot <= 21) || (mi->to_slot >= 1 && mi->to_slot <= 21))
		this->CalcBonuses(true, true); // Update item/spell bonuses
	if (mi->from_slot == 13 || mi->to_slot == 14 || mi->to_slot == 13 || mi->from_slot == 14) {
		//this->CalcBonuses(); //Yeahlight: This is handled else where now
	}
	if(mi->to_slot == 13 || mi->from_slot == 13) {
		uint16 item_id = pp.inventory[13];
		weapon1 = Database::Instance()->GetItem(item_id);

		// Pinedepain // If there's any movement concerning the primary or secondary hand, we update the instrument mod
		if (IsBard())
		{
			UpdateInstrumentMod();
		}
	}
	if (mi->to_slot == 14 || mi->from_slot == 14) {
		uint16 item_id = pp.inventory[14];
		weapon2 = Database::Instance()->GetItem(item_id);

		// Pinedepain // If there's any movement concerning the primary or secondary hand, we update the instrument mod
		if (IsBard())
		{
			UpdateInstrumentMod();
		}
	}
	if( (mi->from_slot>0 && mi->from_slot<21) || ( mi->to_slot>0 && mi->to_slot<21 )){
		SendHPUpdate();
	}
	this->Save();
}


void Client::ProcessOP_MoveCoin(APPLAYER* pApp){
	if (pApp->size != sizeof(MoveCoin_Struct)) {
		cout << "Wrong size on OP_MoveCoin. Got: " << pApp->size << ", Expected: " << sizeof(MoveCoin_Struct) << endl;
		return;
	}
	MoveCoin_Struct* mc = (MoveCoin_Struct*)pApp->pBuffer;
	//cout << "A player is moving his coins from" << (int)mc->from_slot << " to " <<(int)mc->to_slot <<endl;
	if(mc->from_slot == 0){
		pp.platinum_cursor=0;
		pp.gold_cursor=0;
		pp.silver_cursor=0;
		pp.copper_cursor=0;
	}
	if (mc->from_slot == 1) {
		if (mc->cointype2!=mc->cointype1){
			if (mc->cointype2<mc->cointype1)
			{
				int amount2=mc->cointype1-mc->cointype2;
				amount2=(int)pow(10.0f,amount2);
				mc->amount=mc->amount*amount2;
			}
			else if (mc->cointype2>mc->cointype1){
				int amount2=mc->cointype2-mc->cointype1;
				amount2=(int)pow(10.0f,amount2);
				mc->amount=(int)((mc->amount)/(amount2));
			}
		}
		switch (mc->cointype2)
		{
		case 0:
			if ((pp.copper - mc->amount) < 0) {
				EQC_MOB_EXCEPT("void Client::ProcessOP_MoveCoin()", "Error in OP_MoveCoin: negative copper value" );
				return;
			}
			pp.copper = pp.copper - mc->amount;
			break;
		case 1:
			if ((pp.silver - mc->amount) < 0) {
				EQC_MOB_EXCEPT("void Client::ProcessOP_MoveCoin()", "Error in OP_MoveCoin: negative silver value" );
				return;
			}
			pp.silver = pp.silver - mc->amount;
			break;
		case 2:
			if ((pp.gold - mc->amount) < 0) {
				EQC_MOB_EXCEPT("void Client::ProcessOP_MoveCoin()", "Error in OP_MoveCoin: negative gold value" );
				return;
			}
			pp.gold = pp.gold - mc->amount;
			break;
		case 3:
			if ((pp.platinum - mc->amount) < 0) {
				EQC_MOB_EXCEPT("void Client::ProcessOP_MoveCoin()", "Error in OP_MoveCoin: negative platinum value" );
				return;
			}
			pp.platinum = pp.platinum - mc->amount;
			break;
		}
	}
	if (mc->to_slot == 1) {
		if (mc->cointype2!=mc->cointype1){
			if (mc->cointype2<mc->cointype1)
			{
				int amount2=mc->cointype1-mc->cointype2;

				amount2=(int)pow(10.0f,amount2);
				mc->amount=mc->amount*amount2;
			}
			else if (mc->cointype2>mc->cointype1){
				int amount2=mc->cointype2-mc->cointype1;
				amount2=(int)pow(10.0f,amount2);
				mc->amount=(int)((mc->amount)/(amount2));
			}
		}
		switch (mc->cointype2)
		{
		case 0:
			AddMoneyToPP(mc->amount,0,0,0,false);
			break;
		case 1:
			AddMoneyToPP(0,mc->amount,0,0,false);
			break;
		case 2:
			AddMoneyToPP(0,0,mc->amount,0,false);
			break;
		case 3:
			AddMoneyToPP(0,0,0,mc->amount,false);
			break;
		}
	}
	if(mc->to_slot == 0){
		switch (mc->cointype1)
		{
		case 0:
			pp.copper_cursor=mc->amount;
			break;
		case 1:
			pp.silver_cursor=mc->amount;
			break;
		case 2:
			pp.gold_cursor=mc->amount;
			break;
		case 3:
			pp.platinum_cursor=mc->amount;
			break;
		}
	}
	if (mc->to_slot==2) { // BANK Money
		cout << "Moving coins to bank" << endl;
		if (mc->cointype2!=mc->cointype1){
			if (mc->cointype2<mc->cointype1) {
				int amount2=mc->cointype1-mc->cointype2;
				amount2=(int)pow(10.0f,amount2);
				mc->amount=mc->amount*amount2;
			}
			else if (mc->cointype2>mc->cointype1){
				int amount2=mc->cointype2-mc->cointype1;
				amount2=(int)pow(10.0f,amount2);

				mc->amount=(int)((mc->amount)/(amount2));
			}
		}
		switch (mc->cointype2)
		{
		case 0:
			pp.copper_bank=pp.copper_bank + mc->amount;
			break;
		case 1:
			pp.silver_bank=pp.silver_bank + mc->amount;
			break;
		case 2:
			pp.gold_bank=pp.gold_bank + mc->amount;
			break;
		case 3:
			pp.platinum_bank=pp.platinum_bank + mc->amount;
			break;
		}
	}
	if (mc->from_slot == 2) {
		cout << "Moving coins from bank" << endl;
		if (mc->cointype2!=mc->cointype1){
			if (mc->cointype2<mc->cointype1) {
				int amount2=mc->cointype1-mc->cointype2;
				amount2=(int)pow(10.0f,amount2);
				mc->amount=mc->amount*amount2;
			}
			else if (mc->cointype2>mc->cointype1){
				int amount2=mc->cointype2-mc->cointype1;
				amount2=(int)pow(10.0f,amount2);
				mc->amount=(int)((mc->amount)/(amount2));
			}
		}

		switch (mc->cointype2)
		{
		case 0:
			if ((pp.copper_bank - mc->amount) < 0)
				pp.copper_bank = 0;
			else
				pp.copper_bank = pp.copper_bank - mc->amount;
			break;
		case 1:
			if ((pp.silver_bank - mc->amount) < 0)
				pp.silver_bank = 0;
			else
				pp.silver_bank = pp.silver_bank - mc->amount;
			break;
		case 2:
			if ((pp.gold_bank - mc->amount) < 0)
				pp.gold_bank = 0;
			else
				pp.gold_bank = pp.gold_bank - mc->amount;
			break;
		case 3:
			if ((pp.platinum_bank - mc->amount) < 0)

				pp.platinum_bank = 0;
			else
				pp.platinum_bank = pp.platinum_bank - mc->amount;
			break;
		}
	}
	if (mc->to_slot == 3) {
		Entity* ent = entity_list.GetID(this->TradeWithEnt);
		if (this->TradeWithEnt != 0 && ent && ent->IsClient()){
			switch (mc->cointype1) {
				case 0:
					ent->CastToClient()->tradecp = ent->CastToClient()->tradecp+mc->amount;
					break;
				case 1:
					ent->CastToClient()->tradesp = ent->CastToClient()->tradesp+mc->amount;
					break;
				case 2:
					ent->CastToClient()->tradegp = ent->CastToClient()->tradegp+mc->amount;
					break;
				case 3:
					ent->CastToClient()->tradepp = ent->CastToClient()->tradepp+mc->amount;
					break;
			}
			//ent->CastToClient()->Message(BLACK,"%s adds some coins to the trade.",this->GetName());
			//ent->CastToClient()->Message(BLACK,"The total trade is: %i PP %i GP %i SP %i CP", ent->CastToClient()->tradepp, ent->CastToClient()->tradegp, ent->CastToClient()->tradesp, ent->CastToClient()->tradecp);
			APPLAYER* outapp = new APPLAYER(OP_TradeCoins,sizeof(TradeCoin_Struct));
			TradeCoin_Struct* tcs = (TradeCoin_Struct*)outapp->pBuffer;
			memset(outapp->pBuffer,0,outapp->size);
			tcs->trader=this->TradeWithEnt;
			tcs->slot=mc->cointype1;
			tcs->unknown5=0x4fD2;
			tcs->amount=mc->amount;
			entity_list.GetID(this->TradeWithEnt)->CastToClient()->QueuePacket(outapp);
			safe_delete(outapp);
		}
		else if(ent->IsNPC())
		{
			switch (mc->cointype1) {
				case 0:
					this->npctradecp += mc->amount;
					break;
				case 1:
					this->npctradesp += mc->amount;
					break;
				case 2:
					this->npctradegp += mc->amount;

					break;
				case 3:
					this->npctradepp += mc->amount;
					break;
			}
		}
	}
	this->Save();
	//cout << "Saving money data..."<<endl;
}

//////////////////////
void Client::ProcessOP_SpawnAppearance(APPLAYER* pApp){


	//DumpPacket(pApp);

	if (pApp->size != sizeof(SpawnAppearance_Struct))
	{
		cout << "Wrong size on OP_SpawnAppearance. Got: " << pApp->size << ", Expected: " << sizeof(SpawnAppearance_Struct) << endl;
		return;
	}
	SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)pApp->pBuffer;
	if(sa->spawn_id != GetID()) {
		//EQC_MOB_EXCEPT("void Client::ProcessOP_SpawnAppearance()", "sa->spawn_id != GetID()  -  Probably a hacker?" );
		//return; 
	}

	if(sa->type == SAT_Position_Update)		//Player position update.	
	{
		entity_list.QueueClients(this, pApp, true);
		if(sa->parameter == SAPP_Standing_To_Sitting){				//Standing ---> Sitting
			this->Sit(false);
		}
		else if(sa->parameter == SAPP_Sitting_To_Standing)			//Sitting ---> Standing
		{
			this->Stand(false);
		}
		else if (sa->parameter == SAPP_Ducking)			//Ducking		(untested)
		{
			this->Duck(false);
		}
		else if (sa->parameter == SAPP_Unconscious)			//Unconscious	(untested)
		{
			appearance = UNKNOWN3Appearance;
			if (this->GetCastingSpell() && !IsBard() || (IsBard() && this->GetCastingSpell() && !this->GetCastingSpell()->IsBardSong()))
				InterruptSpell();
		}
		else if (sa->parameter == SAPP_Looting)			//Looting		(untested)
		{
			//Cofruben: I changed it to 5, if that's not correct change it again to 3.
			appearance = UNKNOWN5Appearance;

			// Pinedepain // If we are a bard singing, it doesn't interrupt our casting process
			if (this->GetCastingSpell() && !IsBard() || (IsBard() && this->GetCastingSpell() && !this->GetCastingSpell()->IsBardSong()))
			{
				InterruptSpell();
			}
		}
		else if (sa->parameter == SAPP_Lose_Control)				//Lost control
		{
			appearance = StandingAppearance;
			if (this->GetCastingSpell() && !IsBard() || (IsBard() && this->GetCastingSpell() && !this->GetCastingSpell()->IsBardSong()))
			{
				InterruptSpell();
			}
		}
		else
		{
			cerr << "Client " << name << " unknown apperance " << (int)sa->parameter << endl;
			DumpPacketHex(pApp);
			return;
		}
	}

	// 0x11:	This is a client hp update packet.
	else if (sa->type == SAT_HP_Regen) {
		//Yeahlight: I do not trust this. Why are we relying on the client to tell the server when to execute a regeneration tick?
		//           I disabled this for now and I have the server handle all regeneration requests; this just seems very exploitable
		//           and this process is very sloppy.
		//int32 expected_new_hp = sa->parameter;
		//if (DEBUG > 5) EQC::Common::PrintF(CP_CLIENT, "%s requesting regeneration process.\n", this->GetName());
		//if (!tic_timer || (tic_timer && tic_timer->Check()))
		//{
		//	if(!tic_timer)
		//		tic_timer = new Timer(TIC_TIME - 1500);	// less than 6 seconds timer as margin.
		//	//DoHPRegen(expected_new_hp); 
		//}
		//else {
		//	SendHPUpdate();
		//	if (DEBUG > 10)
		//		EQC::Common::PrintF(CP_CLIENT, "%s out of regen timer\n", this->GetName());
		//}
	}
	else if (sa->type == SAT_Autosplit) {						
		//Tazadar: We change the autosplit value
		pp.autosplit=sa->parameter;
		cout << "Client " << name << " Change Autosplit to " << (int)sa->parameter<< endl;
		this->Save();
	}
	else if (sa->type == SAT_NameColor)	// For Anon/Roleplay
	{ 
		if (sa->parameter == 1) {								// This is Anon
			cout << "Client " << name << " Going Anon" << endl;
			pp.anon = 1;
		}
		else if (sa->parameter == 2 || sa->parameter == 3) {	// This is Roleplay, or anon+rp
			cout << "Client " << name << " Going Roleplay" << endl;
			pp.anon = 2;
		}
		else if (sa->parameter == 0) {							// This is Non-Anon
			cout << "Client " << name << " Going Un-Anon/Roleplay" << endl;
			pp.anon = 0;
		}
		else {
			cerr << "Client " << name << " unknown Anon/Roleplay Switch " << (int)sa->parameter << endl;
			return;
		}
		entity_list.QueueClients(this, pApp, true);
		UpdateWho();
		this->Save();
	}
	else if(sa->type == SAT_Invis) {
		// Cofruben: It's handled in spell effects.
	}
	else if (sa->type == SAT_Levitate) {
		cout << "Client " << name << " levitating." << endl;
		entity_list.QueueClients(this, pApp, true);
	}
	else{
		cout << "Unknown spawn appearance packet--: ";
		DumpPacketHex(pApp);
	}

	/*	APPLAYER* outapp = new APPLAYER;;
	outapp->opcode = OP_SpawnAppearance;
	outapp->size = sizeof(SpawnAppearance_Struct);
	outapp->pBuffer = new uchar[outapp->size];
	memset(outapp->pBuffer, 0, outapp->size);
	SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
	sa_out->spawn_id = GetID();
	sa_out->type = 0x0e;
	sa_out->parameter = sa->parameter;
	entity_list.QueueClients(this, pApp, true);
	safe_delete(outapp);//delete outapp;
	}

	}
	else if (sa->type == 0x03)// This should be normal invisibility
	{ 
	if (sa->parameter == 0) // Visible
	cout << "Visible" << endl;
	else if (sa->parameter == 1) // Invisi
	cout << "Invisi" << endl;
	else 
	cout << "Unknown Invisi" << endl;
	}*/
}

//////////////////////

void Client::ProcessOP_Surname(APPLAYER* pApp)
{
	if (pApp->size == sizeof(Surname_Struct))
	{
		Surname_Struct* surname = (Surname_Struct*) pApp->pBuffer;

		//Fill the unknown part of surname struct.
		for(int i=0; i<20;i++)
		{
			surname->s_unknown1[i] = 0;
		}

		this->CastToClient()->ChangeSurname(surname->Surname); // Couldnt we just use this->ChangeSurname(surname->Surname); ? -Dark-Prince

		APPLAYER* outapp = new APPLAYER(OP_GMSurname, sizeof(GMSurname_Struct));
		GMSurname_Struct* lnc = (GMSurname_Struct*) outapp->pBuffer;
		strcpy(lnc->name, surname->name);
		strcpy(lnc->Surname, surname->Surname);
		strcpy(lnc->gmname, "SurnameOP");
		memset(&lnc->unknown[0],1,sizeof(lnc->unknown));//We show the surname !
		memset(&surname->s_unknown1[0],1,sizeof(surname->s_unknown1));//Tazadar : We accept the surname !

		entity_list.QueueClients(this, outapp, false);
		QueuePacket(pApp);
		this->Save();
	}
	else
	{
		cout << "Wrong size on OP_Surname. Got: " << pApp->size << ", Expected: " << sizeof(Surname_Struct) << endl;
		return;
	}

}

//////////////////////

void Client::ProcessOP_Consider(APPLAYER* pApp)
{
	bool debugFlag = true;

	if(pApp->size == sizeof(Consider_Struct))
	{
		Consider_Struct* conin = (Consider_Struct*)pApp->pBuffer;
		if ((conin->targetid == GetID()) || (conin->targetid < 1)) //Cofruben: preventing crash.
		{
			return;
		}

		Mob* tmob = entity_list.GetMob(conin->targetid);

		if(!tmob) 
		{
			return; //Cofruben: preventing another crash.
		}
		else
		{
			if(tmob->IsCorpse())
			{
				return;
			}

			APPLAYER* outapp = new APPLAYER(OP_Consider, sizeof(Consider_Struct));		

			Consider_Struct* con = (Consider_Struct*)outapp->pBuffer;
			con->playerid = GetID();
			con->targetid = conin->targetid;

			FACTION_VALUE FactionLevel = FACTION_INDIFFERENT;

			if(tmob->IsNPC()) // Faction is only for NPC's at this stage -Dark-Prince
			{
				int32 pFactionID = tmob->CastToNPC()->GetFactionID();
				FactionLevel = GetFactionLevel(character_id, tmob->GetNPCTypeID(), race, class_, deity, tmob->CastToNPC()->GetPrimaryFactionID(), tmob);
				//Yeahlight: Merchants and bankers bottom out at FACTION_DUBIOUS
				if((tmob->GetClass() == MERCHANT || tmob->GetClass() == BANKER) && (FactionLevel == FACTION_SCOWLS || FactionLevel == FACTION_THREATENLY))
					FactionLevel = FACTION_DUBIOUS;
			}
			int32 passValue = 0x00000000;
			//Yeahlight: Our new client seems to use a four byte integer shifted left twice. I cannot make much sense of it
			//           but these following values trigger the correct messages in the client.
			//           Note: Apprehensive(0xFFFFFFFF) is not a typo and yes it does fill all four bytes unlike the rest
			//0x00000500 - ally
			//0x00000300 - warmly
			//0x00000200 - kindly
			//0x00000100 - amiable
			//0x00000000 - indifferent
			//0xFFFFFFFF - apprehensive
			//0xFFFFFF00 - dubious
			//0xFFFFFE00 - threateningly
			//0xFFFFFD00 - scowls
			switch(FactionLevel)
			{
				case FACTION_ALLY: 
					passValue = 0x00000500;
					break;
				case FACTION_WARMLY: 
					passValue = 0x00000300;
					break;
				case FACTION_KINDLY: 
					passValue = 0x00000200;
					break;
				case FACTION_AMIABLE: 
					passValue = 0x00000100;
					break;
				case FACTION_INDIFFERENT: 
					passValue = 0x00000000;
					break;
				case FACTION_APPREHENSIVE:
					passValue = 0xFFFFFFFF;
					break;
				case FACTION_DUBIOUS:
					passValue = 0xFFFFFF00;
					break;
				case FACTION_THREATENLY:
					passValue = 0xFFFFFE00;
					break;
				case FACTION_SCOWLS:
					passValue = 0xFFFFFD00;
					break;
				default:
					passValue = 0x00000000;
			}
			if(debugFlag && GetDebugMe())
				Message(WHITE, "Debug: Your consider level on %s: %i", tmob->GetName(), FactionLevel);
			con->faction = passValue;
			QueuePacket(outapp);
			safe_delete(outapp);//delete outapp;
		}
	}
	else
	{
		cout << "Wrong size on OP_Consider. Got: " << pApp->size << ", Expected: " << sizeof(Consider_Struct) << endl;
		return;
	}
}

//////////////////////

void Client::ProcessOP_ClientUpdate(APPLAYER* pApp)
{	
	if(pApp->size == sizeof(SpawnPositionUpdate_Struct))
	{
		SpawnPositionUpdate_Struct* cu = (SpawnPositionUpdate_Struct*)pApp->pBuffer;
		//Yeahlight: PC currently has an eye of zomm, so we need to forward the movement changes to the NPC and not the PC
		if(myEyeOfZomm)
		{
			//Yeahlight: The eye and the caster share the same packets, so we only want to use the packets for the eye
			if(abs(cu->x_pos - myEyeOfZommX) > 2 || abs(cu->y_pos - myEyeOfZommY) > 2)
			{
				myEyeOfZomm->SetX(cu->x_pos);
				myEyeOfZomm->SetY(cu->y_pos);
				myEyeOfZomm->SetZ(cu->z_pos);
				myEyeOfZomm->SetDeltaX(cu->delta_x);
				myEyeOfZomm->SetDeltaY(cu->delta_y);
				myEyeOfZomm->SetDeltaZ(cu->delta_z);
				myEyeOfZomm->SetHeading(cu->heading);
				myEyeOfZomm->SetVelocity(cu->anim_type);
				myEyeOfZomm->SendPosUpdate(false, 350, true);
			}
			if((Timer::GetCurrentTime() - pLastChange) > 10000) 
			{
				SendPosUpdate(false, PC_UPDATE_RANGE, false);
				pLastChange = Timer::GetCurrentTime();
			}
		}
		else
		{
			bool sendUpdate = true;

			//Yeahlight: Do not update unless there is a shift in the deltas
			if(delta_x == cu->delta_x && delta_y == cu->delta_y && delta_z == cu->delta_z && delta_heading == cu->delta_heading)
				sendUpdate = false;

			//Yeahlight: Scan for zoneline nodes if our client has a change in location
			if(x_pos != cu->x_pos || y_pos != cu->y_pos)
				ScanForZoneLines();

			animation = cu->anim_type;
			//Yeahlight: TODO: Look over this, this makes no sense.
			//if(animation > 0) { // maalanar 2008-02-05: no idea why but this seems to work
			//	animation++; // if not the client isn't in the right spot on next update and it causes a warp
			//}
			//if(animation < 0) {
			//	animation--;
			//}

			x_pos = (float)cu->x_pos;
			y_pos = (float)cu->y_pos;
			z_pos = (float)cu->z_pos;

			delta_x = cu->delta_x;
			delta_y = cu->delta_y;
			delta_z = cu->delta_z;

			heading = cu->heading;
			delta_heading = cu->delta_heading;

			//if(delta_heading != 0)
			//	sendUpdate = true;

			// maalanar 2008-02-05: anim_type is 0 when strafing even when the deltas aren't
			if ((delta_x != 0 || delta_y != 0 || delta_z != 0) && cu->anim_type == 0)
			{
				sendUpdate = true;
				//cout << "anim: " << static_cast<int>(Appearance) << " x: " << x_pos << " y: " << y_pos << " z: " << z_pos << " heading: " << heading << " dx: " << delta_x << " dy: " << delta_y << " dz: " << delta_z << " d_heading: " << static_cast<int>(delta_heading) << endl;//" spacer1: " << cu->spacer1 << " spacer2: " << cu->spacer2 << endl;
				//cout << "in: " << endl;
				//DumpPacketHex(pApp);
			}
			//If we moved , and if we are are not currently sneaking as a Rogue, then remove hide.
			if(cu->delta_x || cu->delta_y || cu->delta_z)
			{
				if(!GetSneaking())
					SetHide(false);
				else if (GetSneaking() && (GetClass() != ROGUE))
					SetHide(false);
			}
			// maalanar 2008-02-05: only send updates on delta changes, strafing,
			// or every 10 seconds to keep the client from disappearing on the other clients
			if ((Timer::GetCurrentTime() - pLastChange) > 10000 || sendUpdate) 
			{
				SendPosUpdate(false, PC_UPDATE_RANGE, false);
				pLastChange = Timer::GetCurrentTime();

				// Harakiri check for swimming skillup
				if(IsInWater()) {
					uint16 current_raw_skill = GetSkill(SWIMMING);

					int maxskill = CheckMaxSkill(SWIMMING, this->race, this->class_, this->level);

					if(current_raw_skill < maxskill) {		

						int chance = MakeRandomInt(1,100);						
						// Harakiri tweak this chance, no idea 2% atm
						if(chance > 98) {
							SetSkill(SWIMMING, current_raw_skill + 1);
						}
					}
				}

				// debug
				/*Message(BLACK,"Client is in Water: %d",IsInWater());
				Message(BLACK,"Client is in Lava: %d",IsInLava());*/
			}
		}
	}
	else
	{
		cout << "Wrong size on OP_ClientUpdate. Got: " << pApp->size << ", Expected: " << sizeof(SpawnPositionUpdate_Struct) << endl;
		return;
	}

}

//////////////////////

void Client::ProcessOP_DeleteSpawn(APPLAYER* pApp)
{
	// The client will send this with his id when he zones, maybe when he disconnects too?
	// When zoning he seems to want 5921 and finish flag

	cout << "Player attempting to delete spawn: " << name << endl;
	APPLAYER* outapp = new APPLAYER;
	outapp->opcode = 0x5921;
	outapp->size = 0;
	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;

	PMClose();
	//QueuePacket(0); // Closing packet
}

//////////////////////

void Client::ProcessOP_PickPockets(APPLAYER* pApp)
{
	// Pick Pockets - Wizzel - 04/21/08
	// Basis of PickPockets.
	//1. Makes sure the person you targeted is an interactive NPC or a PC
	//2. Calculations based on your skill and dexterity.
	//3. Weighs in the fact a mob might catch you.
	//4. Adds money or item to on player.

	// Problems and bugs.
	//1. Even if a player suceeds, the client gives the message that the pick pocket has failed.
	//2. Player does not get attacked yet. Waiting on factions and npc movement.
	//3. Client money does not refresh itself. It is added but won't show up until zone/reboot.
	//4. Was told success messages were something like "You have stolen "x" "money type" in IRC. I will implement them.

	PickPockets_Struct* pick_in = (PickPockets_Struct*) pApp->pBuffer;
	//		     LogFile->write(EQEMuLog::Debug,
	//		               "PickPocket to:%i from:%i myskill:%i type:%i",
	//                                     pick_in->to, pick_in->from ,pick_in->myskill, pick_in->type);

	APPLAYER* outapp = new APPLAYER(OP_PickPockets, sizeof(PickPockets_Struct));
	PickPockets_Struct* pick_out = (PickPockets_Struct*) outapp->pBuffer;
	Mob* victim = entity_list.GetMob(pick_in->to);
	if (!victim)
		return;
	if (pick_in->myskill == 0) {
		//                         LogFile->write(EQEMuLog::Debug,
		//                           "Client pick pocket response");
		DumpPacket(pApp);
		delete(outapp);
		return;
	}
	uint8 success = 0;
	if (RandomTimer(1,900) >= (GetSkill(PICK_POCKETS)+GetDEX()))
		success = RandomTimer(1,5);
	if ( (victim->IsNPC() && victim->CastToNPC()->IsInteractive()) || (victim->IsClient()) ) {
		//                        if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "PickPocket picking ipc/client");
		cout << "Payer " << name << "is trying to pickpocket a player/npc" << endl;
		pick_out->to   = pick_in->from;
		pick_out->from = pick_in->to;
		if (   victim->IsClient()
			&& !success
			){
				victim->CastToClient()->QueuePacket(pApp);
		}
		else if (   victim->IsNPC()
			&& victim->CastToNPC()->IsInteractive()
			//                                && GetPVP() 
			//                              && (GetPVP() == victim->CastToClient()->GetPVP())
			&& !success
			) {
				int16 hate = RandomTimer(1,503);
				int16 tmpskill = (GetSkill(PICK_POCKETS)+GetDEX());
				if( hate >= tmpskill ){
					//                                      victim->AddToHateList(this, 1, 0);
					entity_list.MessageClose(victim, true, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, your not as quick as you thought you were thief!", victim->GetName());
				}
		}
		if(!success){
			// Failed
			QueuePacket(outapp);
		}
		else if (success) {
			cout << "Payer " << name << "succesfully pickpocketed an NPC/PC" << endl;
			pick_out->to   = pick_in->from;
			pick_out->from = pick_in->to;
			pick_out->type = success;
			pick_out->coin = (RandomTimer(1,RandomTimer(2,10)));
			switch (success){
								 case 1:
									 AddMoneyToPP(0,0,0,pick_out->coin,true);
									 //You have stolen x platinum.
									 break;
								 case 2:
									 AddMoneyToPP(0,0,pick_out->coin,0,true);
									 break;
								 case 3:
									 AddMoneyToPP(0,pick_out->coin,0,0,true);
									 break;
								 case 4:
									 AddMoneyToPP(pick_out->coin,0,0,0,true);
									 break;
								 case 5:
									 // Item
									 pick_out->type = 0;
									 break;
									 //                                 default: LogFile->write(EQEMuLog::Error,"Unknown success in OP_PickPocket %i", __LINE__); break;
			}
			this->Message(EMOTE, "You have successfully pick pocketed your target.");
			QueuePacket(outapp);
		}
		else {
			//                                LogFile->write(EQEMuLog::Error, "Unknown error in OP_PickPocket");
		}
	}
	else {
		//                         if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "PickPocket picking npc");
		if (success) {
			pick_out->to   = pick_in->from;
			pick_out->from = pick_in->to;
			pick_out->type = success;
			pick_out->coin = (RandomTimer(1,RandomTimer(2,10)));
			switch (success){
								 case 1:
									 AddMoneyToPP(0,0,0,pick_out->coin,true);
									 break;
								 case 2:
									 AddMoneyToPP(0,0,pick_out->coin,0,true);
									 break;
								 case 3:
									 AddMoneyToPP(0,pick_out->coin,0,0,true);
									 break;
								 case 4:
									 AddMoneyToPP(pick_out->coin,0,0,0,true);
									 break;
								 case 5:
									 // Item
									 pick_out->type = 0;
									 break;
									 //                                 default: LogFile->write(EQEMuLog::Error,"Unknown success in OP_PickPocket %i", __LINE__); break;
			}
			this->Message(EMOTE, "You have successfully pick pocketed your target.");
			QueuePacket(outapp);
		}
		else {
			int16 hate = RandomTimer(1,503);
			int16 tmpskill = (GetSkill(PICK_POCKETS)+GetDEX());
			if( hate >= tmpskill ){
				//                                      victim->AddToHateList(this, 1, 0);
				entity_list.MessageClose(victim, true, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, your not as quick as you thought you were thief!", victim->GetName());
			}
			QueuePacket(outapp);
		}
	}
	safe_delete(outapp);//delete outapp;
	return;
}

///////////////////////////////////////

void Client::ProcessOP_GMZoneRequest(APPLAYER* pApp)
{
	if (pApp->size == sizeof(GMZoneRequest_Struct))
	{
		if(this->Admin() < 10)
		{ 
			//-Cofruben: Maybe a hacker?
			EQC_MOB_EXCEPT("void Client::ProcessOP_GMZoneRequest()", "this->Admin() < 10  -  Probably a hacker?" );
			return;
		}

		GMZoneRequest_Struct* gmzr = (GMZoneRequest_Struct*) pApp->pBuffer;
		APPLAYER* outapp = new APPLAYER(OP_GMZoneRequest, sizeof(GMZoneRequest_Struct));
		memset(outapp->pBuffer, 0, outapp->size);
		GMZoneRequest_Struct* gmzr2 = (GMZoneRequest_Struct*) outapp->pBuffer;
		strcpy(gmzr2->charname, this->GetName());
		strcpy(gmzr2->zonename, gmzr->zonename);
		gmzr2->success = 1;
		int8 tmp[32] = { 0xe8, 0xf0, 0x58, 0x00, 0x70, 0xef, 0xad, 0x0e, 0x74, 0xf3, 0xad, 0x0e, 0xc7, 0x01, 0x4c, 0x00,
			0x00, 0xa0, 0x04, 0xc5, 0x00, 0x20, 0x5f, 0xc5, 0x00, 0x00, 0xba, 0xc2, 0x00, 0x00, 0x00, 0x00 };
		memcpy(gmzr2->unknown1, tmp, 32);
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;
	}
	else
	{
		cout << "Wrong size on OP_GMZoneRequest. Got: " << pApp->size << ", Expected: " << sizeof(GMZoneRequest_Struct) << endl;
		return;
	}

}

//////////////////////

void Client::ProcessOP_EndLootRequest(APPLAYER* pApp)
{
	if (pApp->size == sizeof(int32))
	{
		APPLAYER* outapp = 0;
		Entity* entity = entity_list.GetID(*((int32*)pApp->pBuffer));
		if (entity == 0)
		{
			if(GetDebugMe())
				Message(WHITE, "Debug: OP_EndLootRequest: Corpse not found (ent = 0)");
			Corpse::SendEndLootErrorPacket(this);
			return;
		}

		if (!entity->IsCorpse()) 
		{
			if(GetDebugMe())
				Message(WHITE, "Debug: OP_EndLootRequest: Corpse not found (!entity->IsCorpse())");
			Corpse::SendEndLootErrorPacket(this);
			return;
		}
		entity->CastToCorpse()->EndLoot(this, pApp);
	}
	else
	{
		cout << "Wrong size: OP_EndLootRequest, size=" << pApp->size << ", expected " << sizeof(int16) << endl;
		return;
	}
}

//////////////////////

void Client::ProcessOP_LootRequest(APPLAYER* pApp)
{
	if(pApp->size != sizeof(int32))
	{
		cout << "Wrong size: OP_LootRequest, size=" << pApp->size << ", expected " << sizeof(int16) << endl;
		return;
	}

	Entity* ent = entity_list.GetID(*((int32*)pApp->pBuffer));
	if(ent == NULL)
	{
		Message(RED, "Error: OP_LootRequest: Corpse not found (ent = 0)");
		Corpse::SendLootReqErrorPacket(this);
		return;
	}
	if(ent->IsCorpse())
	{
		//Yeahlight: Purge client's invisibility
		CancelAllInvisibility();

		if(ent->CastToCorpse()->IsPlayerCorpse()){
			cout << ent->CastToCorpse()->GetName() << endl;
			if(strstr(ent->CastToCorpse()->GetName(), this->GetName()) != NULL){
				cout << "You may loot this corpse (still need to check consent list)." << endl;
				ent->CastToCorpse()->MakeLootRequestPackets(this, pApp);
			}else{
				Message(RED,"You may NOT loot this corpse.");
				Corpse::SendLootReqErrorPacket(this);
				return;
			}
		}else{
			ent->CastToCorpse()->MakeLootRequestPackets(this, pApp);	
		}
	}
	else {
		Message(RED, "Error: OP_LootRequest: target not a corpse?");
		Corpse::SendLootReqErrorPacket(this);
	}
}

//////////////////////

void Client::ProcessOP_LootItem(APPLAYER* pApp){
	if (pApp->size != sizeof(LootingItem_Struct))
	{
		cout << "Wrong size: OP_LootItem, size=" << pApp->size << ", expected " << sizeof(LootingItem_Struct) << endl;
		return;
	}
	//Yeahlight: Purge client's invisibility
	CancelAllInvisibility();

	APPLAYER* outapp = 0;
	Entity* entity = entity_list.GetID(*((int16*)pApp->pBuffer));
	if(entity->IsPlayerCorpse() && strstr(entity->CastToCorpse()->GetName(), this->GetName()) == NULL){
		Message(RED, "You may NOT loot this corpse.");
		Corpse::SendEndLootErrorPacket(this);
		return;
	}
	if(entity == NULL)
	{
		Message(RED, "Error: OP_LootItem: Corpse not found (ent = 0)");
		outapp = new APPLAYER(OP_LootComplete, 0);
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;
		return;
	}

	if(entity->IsCorpse())
	{
		//Yeahlight: Only NPC corpses will return a loot message
		if(entity->IsPlayerCorpse() == false)
		{
			LootingItem_Struct* lootitem = (LootingItem_Struct*)pApp->pBuffer;
			ServerLootItem_Struct* item_data = entity->CastToCorpse()->GetItem(lootitem->slot_id);
			Item_Struct* item = 0;
			if(item_data != 0)
				item = Database::Instance()->GetItem(item_data->item_nr);
			if(item != 0)
				entity->CastToCorpse()->SendLootMessages(this->GetX(), this->GetY(), this->CastToMob(), item);
		}
		//Yeahlight: TODO: Loot event logging will be done here
		entity->CastToCorpse()->LootItem(this, pApp);
	}
	else 
	{
		Message(RED, "Error: Corpse not found! (!ent->IsCorpse())");
		Corpse::SendEndLootErrorPacket(this);
	}

}
void Client::ProcessOP_CastSpell(APPLAYER* pApp)
{
	if(pApp->size != sizeof(CastSpell_Struct)) 
	{
		cout << "Wrong size: OP_CastSpell, size=" << pApp->size << ", expected " << sizeof(CastSpell_Struct) << endl;
		return;
	}	

	CastSpell_Struct* castspell = (CastSpell_Struct*)pApp->pBuffer;
	
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT, "%s casting slot=%d, spell=%d, target=%d, inventoryslot=%lx", this->GetName(), castspell->slot, castspell->spell_id, castspell->target_id, (unsigned long)castspell->inventoryslot);
	
	//Yeahlight: PC is currently cloaked by an invisibility spell
	CancelAllInvisibility();

	//Right click item
	if(castspell->slot == SLOT_ITEMSPELL)
	{
		//Yeahlight: Insta-clicky lockout timer
		if(clicky_lockout_timer->Check())
		{
			clicky_lockout_timer->Start(200);
			if(castspell->inventoryslot < 30)
			{ 
				//Grab item from DB
				Item_Struct* item = Database::Instance()->GetItem(pp.inventory[castspell->inventoryslot]);				

				//Cofruben: this should not happen.
				if(item == NULL)
				{
					EQC_MOB_EXCEPT("void Client::ProcessOP_CastSpell()", "item == 0" );
					return;
				}
				else if(item->common.effecttype == ET_ClickEffect || item->common.effecttype == ET_Expendable || item->common.effecttype == ET_EquipClick || item->common.effecttype == ET_ClickEffect2) 
				{

					// Harakiri right click items mostly are self only
					if(castspell->target_id == 0) {
						castspell->target_id = GetID();
					}

					if(item->common.casttime == 0)
					{
						//Pinedepain // We just do a little check here to avoid the posibility of casting an with an instant right-click item
						//while being mezzed,charmed,stunned etc ... We check the same thing later in the CastSpell function.
						//I prefer to handle the non-instant-cast check in the CastSpell function, not here just to have a clear code. I could do it here
						//if it's better
						if(IsMesmerized())
						{
							EnableSpellBar(0);
							Message(BLACK,"You cannot cast while you do not have control of yourself!");
						}
						else
						{
							Spell* spell = spells_handler.GetSpellPtr(item->common.click_effect_id);
							this->casting_spell_x_location = GetX();
							this->casting_spell_y_location = GetY();
	
							// Harakiri RemoveOneCharge called at finished
							SpellFinished(spell, castspell->target_id, castspell->slot,castspell->inventoryslot);																			
							
						}

					}
					else
					{
						Spell* spell = spells_handler.GetSpellPtr(item->common.click_effect_id);
						// Harakiri RemoveOneCharge called at SpellFinished
						CastSpell(spell, castspell->target_id, castspell->slot, item->common.casttime,castspell->inventoryslot);
					}
				}
				else
				{
					Message(BLACK, "Error: unknown item->common.effecttype (0x%02x)", item->common.effecttype);
				}
			}
			else
			{
				Message(BLACK, "Error: castspell->inventoryslot >= 30 (0x%04x)", castspell->inventoryslot);
			}
		}
		else
		{
			EnableSpellBar(0);
			return;
		}
	}
	else
	{
		CastSpell(spells_handler.GetSpellPtr(castspell->spell_id), castspell->target_id, castspell->slot);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// Pinedepain // Called whenever a ManaChange packet is sent to the server
void Client::ProcessOP_ManaChange(APPLAYER* pApp)
{
	// Pinedepain // If the packet is zero sized, and the client is bard, it means he stopped the casting process
	// of his song
	if(pApp->size == 0 && IsBard())
	{
		StopSong();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// Pinedepain // Called whenever an InterruptCast packet is sent to the server. I don't know yet what we should do in this function?
void Client::ProcessOP_InterruptCast(APPLAYER* pApp)
{


}

/////////////////////////////////////////////////////////////////////////////////////////////////

void Client::ProcessOP_MemorizeSpell(APPLAYER* pApp)
{
	if (pApp->size != sizeof(MemorizeSpell_Struct)) 
	{
		cout << "Wrong size: OP_MemorizeSpell, size=" << pApp->size << ", expected " << sizeof(MemorizeSpell_Struct) << endl;
		return;
	}
	MemorizeSpell_Struct memspell; 
	MoveItem_Struct spellmoveitem; 

	memcpy(&memspell, pApp->pBuffer, sizeof(MemorizeSpell_Struct)); 
	QueuePacket(pApp); 
	if(memspell.scribing == 0) 
	{ 
		pp.spell_book[memspell.slot] = memspell.spell_id; 
		APPLAYER* outapp = new APPLAYER(OP_MoveItem, sizeof(MoveItem_Struct));
		spellmoveitem.from_slot = 0; 
		spellmoveitem.to_slot = 0xffffffff; 
		spellmoveitem.number_in_stack = 0; 
		memcpy(outapp->pBuffer, &spellmoveitem, sizeof(MoveItem_Struct)); 
		QueuePacket(outapp); 
		safe_delete(outapp);//delete outapp;
		pp.inventory[0] = 0xFFFF;
	} 
	else if(memspell.scribing == 1)
	{
		pp.spell_memory[memspell.slot] = memspell.spell_id;
		//Yeahlight: Reset this slot's recast timer
		SetSpellRecastTimer(memspell.slot, 0);
	}
	else if(memspell.scribing == 2)
	{
		pp.spell_memory[memspell.slot] = 0xffff;
	}
}

//////////////////////

void Client::ProcessOP_SwapSpell(APPLAYER* pApp){

	if (pApp->size != sizeof(SwapSpell_Struct)) 
	{
		cout << "Wrong size on OP_SwapSpell. Got: " << pApp->size << ", Expected: " << sizeof(SwapSpell_Struct) << endl;
		return;
	}
	SwapSpell_Struct swapspell; 
	short swapspelltemp; 

	memcpy(&swapspell, pApp->pBuffer, sizeof(SwapSpell_Struct)); 

	swapspelltemp = pp.spell_book[swapspell.from_slot]; 
	pp.spell_book[swapspell.from_slot] = pp.spell_book[swapspell.to_slot]; 
	pp.spell_book[swapspell.to_slot] = swapspelltemp; 

	QueuePacket(pApp);



}

//////////////////////

//Yeahlight: TODO: Holy mess, Batman! Clean this function up and get rid of all these magic numbers
void Client::ProcessOP_CombatAbility(APPLAYER* pApp){
	if (pApp->size != sizeof(CombatAbility_Struct)) 
	{
		cout << "Wrong size on OP_CombatAbility. Got: " << pApp->size << ", Expected: " << sizeof(CombatAbility_Struct) << endl;
		return;
	}

	//Yeahlight: Combat abilities fade any invisibility spells
	CancelAllInvisibility();

	CombatAbility_Struct* ca_atk = (CombatAbility_Struct*) pApp->pBuffer;
	//Yeahlight: This PC has a target and is permitted to attack it
	if(target && CanAttackTarget(target))
	{
		if ((ca_atk->m_atk == 100)&&(ca_atk->m_type==10)) {    // SLAM - Bash without a shield equipped
			DoAnim(7);
			sint32 dmg=(sint32) ((level/10)  * 3  * (GetSkill(BASH) + GetSTR() + level) / (700-GetSkill(BASH)));
			//this->Message(MT_Emote, "You Bash for a total of %d damage.",  dmg);
			target->Damage(this, dmg, 0xffff, BASH);

			CheckAddSkill(BASH);

			if(GetClass()==WARRIOR&&(GetRace()==BARBARIAN||GetRace()==TROLL||GetRace()==OGRE))  // large race warriors only *
			{
				float wisebonus =  (pp.WIS > 200) ? 20 + ((pp.WIS - 200) * 0.05) : pp.WIS * 0.1;

				if (((55-(GetSkill(BASH)*0.240))+wisebonus > (float)rand()/RAND_MAX*100)&& (GetSkill(BASH)<(pp.level+1)*5))
					this->SetSkill(BASH,GetSkill(BASH)+1);
			}
			this->Save();
			return;
		}

		if ((ca_atk->m_atk == 11)&&(ca_atk->m_type==51)) {

			const Item_Struct* Rangeweapon = 0;
			Rangeweapon = Database::Instance()->GetItem(pp.inventory[11]);
			if (!Rangeweapon) {
				//Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have nothing to throw!", pp.inventory[11]);
				return;
			}
			uint8 WDmg = Rangeweapon->common.damage;
			// Throw stuff
			DoAnim(5);
			sint32 TotalDmg = 0;

			// borrowed this from attack.cpp
			// chance to hit

			float chancetohit = GetSkill(51) / 3.75;    // throwing

			if (pp.level-target->GetLevel() < 0)

			{
				chancetohit -= (float)((target->GetLevel()-pp.level)*(target->GetLevel()-pp.level))/4;
			}

			int16 targetagi = target->GetAGI();
			int16 playerDex = (int16)(this->itembonuses.DEX + this->spellbonuses.DEX)/2;

			targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);
			chancetohit -= (float)targetagi*0.05;
			chancetohit += playerDex;
			chancetohit = (chancetohit > 0) ? chancetohit+30:30;
			chancetohit = chancetohit > 95 ? 95 : chancetohit; /* cap to 95% */

			uint8 levelBonus = (pp.STR+pp.level+GetSkill(51)) / 100;
			uint8 MaxDmg = (WDmg)*levelBonus;
			if (MaxDmg == 0)
				MaxDmg = 1;
			TotalDmg = 1 + rand()%MaxDmg;

			// Hit?
			if (((float)rand()/RAND_MAX)*100 > chancetohit)
			{
				target->CastToNPC()->Damage(this, 0, 0xffff, THROWING);
			}
			else
			{
				//										this->Message(MT_Emote, "You Hit for a total of %d damage.", TotalDmg);
				target->CastToNPC()->Damage(this, TotalDmg, 0xffff, THROWING);
			}

			if(pp.inventory[21]==0xFFFF){
				if(pp.invItemProprieties[11].charges==1)
					pp.inventory[11]=0xFFFF;
				if(pp.invItemProprieties[11].charges>1){
					pp.invItemProprieties[21].charges=pp.invItemProprieties[11].charges-2;
					pp.invItemProprieties[11].charges=1;
					if(pp.invItemProprieties[21].charges>0)
						pp.inventory[21]=pp.inventory[11];
				}
			}
			if(pp.inventory[21]!=0xFFFF && pp.invItemProprieties[21].charges==1){
				pp.inventory[21]=0xFFFF;
			}
			if(pp.inventory[21]!=0xFFFF && pp.invItemProprieties[21].charges>1){
				pp.invItemProprieties[21].charges--;
			}
			// See if the player increases their skill - with cap
			float wisebonus =  (pp.WIS > 200) ? 20 + ((pp.WIS - 200) * 0.05) : pp.WIS * 0.1;

			if (((55-(GetSkill(THROWING)*0.240))+wisebonus > (float)rand()/RAND_MAX*100)&& (GetSkill(THROWING)<(pp.level+1)*5))
				this->SetSkill(THROWING,GetSkill(THROWING)+1);
			this->Save();
			return;
		}

		if ((ca_atk->m_atk == 11)&&(ca_atk->m_type==7)) {
			cout<<"Using Archery Skill"<<endl;
			DoAnim(9);

			// This is a Ranged Weapon Attack / Bow
			const Item_Struct* Rangeweapon = 0;
			const Item_Struct* Ammo = 0;

			Rangeweapon = Database::Instance()->GetItem(pp.inventory[11]);
			Ammo = Database::Instance()->GetItem(pp.inventory[21]);
			if (!Rangeweapon) {
				return;
			}
			if (!Ammo) {
				return;
			}

			uint8 WDmg = Rangeweapon->common.damage;
			uint8 ADmg = Ammo->common.damage;

			// These dmg formulae were taken from all over the net.
			// No-one knows but Verant. This should be fairly close -BoB

			// ** ToDO: take into account trueshot disc (x2.0)

			// Note: Rangers have a chance of crit dmg with a bow

			uint8 levelBonus = (pp.STR+pp.level+GetSkill(ARCHERY)) / 100;
			uint8 MaxDmg = (WDmg+ADmg)*levelBonus;

			sint32 TotalDmg = 0;
			sint32 critDmg = 0;

			if(GetClass()==RANGER)
			{
				critDmg = (sint32)(MaxDmg * 1.72);
			}

			if (MaxDmg == 0)
				MaxDmg = 1;
			TotalDmg = 1 + rand()%MaxDmg;

			// TODO: post 50 dmg bonus
			// TODO: Tone down the PvP dmg

			// borrowed this from attack.cpp
			// chance to hit

			float chancetohit = GetSkill(ARCHERY) / 3.75;
			if (pp.level-target->GetLevel() < 0) {
				chancetohit -= (float)((target->GetLevel()-pp.level)*(target->GetLevel()-pp.level))/4;
			}

			int16 targetagi = target->GetAGI();
			int16 playerDex = (int16)(this->itembonuses.DEX + this->spellbonuses.DEX)/2;

			targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);
			chancetohit -= (float)targetagi*0.05;
			chancetohit += playerDex;
			chancetohit = (chancetohit > 0) ? chancetohit+30:30;
			chancetohit = chancetohit > 95 ? 95 : chancetohit; /* cap to 95% */

			// Hit?
			if (((float)rand()/RAND_MAX)*100 > chancetohit)
			{
				target->CastToNPC()->Damage(this, 0, 0xffff, 0x07);
			}
			else
			{
				// no crits before level 12 cap is maxed
				if((GetClass()==RANGER)&&(GetSkill(ARCHERY)>65)&&(rand()%255<(GetSkill(ARCHERY)+playerDex)/2)&&(chancetohit > 85))
				{
					this->Message(RED, "You score a critical hit!(%d)", critDmg);
					target->CastToNPC()->Damage(this, critDmg, 0xffff, 0x07);

				}
				else
				{
					//this->Message(RED, "You Hit for a total of %d non-melee damage.", TotalDmg);
					target->CastToNPC()->Damage(this, TotalDmg, 0xffff, 0x07);
				}
			}
			pp.invItemProprieties[21].charges--;
			if(pp.invItemProprieties[21].charges==0){
				pp.inventory[21]=0xFFFF;
			}
			// See if the player increases their skill - with cap
			float wisebonus =  (pp.WIS > 200) ? 20 + ((pp.WIS - 200) * 0.05) : pp.WIS * 0.1;

			if (((55-(GetSkill(ARCHERY)*0.240))+wisebonus > (float)rand()/RAND_MAX*100)&& (GetSkill(ARCHERY)<(pp.level+1)*5) && GetSkill(ARCHERY) < 252)
				this->SetSkill(ARCHERY,GetSkill(ARCHERY)+1);
			this->Save();
			return;
		}
		switch(GetClass())
		{
		case WARRIOR:
			//Warrior Kicking
			if (target!=this) {
				if (target->IsNPC()){ //Warriors should kick harder than hybrid classes
					float dmg=(((GetSkill(KICK) + GetSTR())/25) *4 +level)*(((float)rand()/RAND_MAX));
					dmg = GetCriticalHit(target, KICK, dmg);
					target->CastToNPC()->Damage(this, (int32)dmg, 0xffff, 0x1e);
					CheckAddSkill(KICK);
					//Yeahlight: TODO: Stun kick at 55
					DoAnim(1);
				}
				else if(target->IsClient()) {
					float dmg=(((GetSkill(KICK) + GetSTR())/25) *3 +level)*(((float)rand()/RAND_MAX));
					//cout << "Dmg:"<<dmg;
					dmg = GetCriticalHit(target, KICK, dmg);
					target->CastToNPC()->Damage(this, (int32)dmg, 0xffff, 0x1e);
					//Yeahlight: TODO: Stun kick at 55
					DoAnim(1);
				}
			}
			break;
		case RANGER:
		case PALADIN:
		case SHADOWKNIGHT:
			//Hybrid kicking
			if (target!=this) {
				if (target->IsNPC()){
					float dmg=(((GetSkill(KICK) + GetSTR())/25) *3 +level)*(((float)rand()/RAND_MAX));
					//cout << "Dmg:"<<dmg;
					target->CastToNPC()->Damage(this, (int32)dmg, 0xffff, 0x1e);
					CheckAddSkill(KICK);
					DoAnim(1);
				}
				else if(target->IsClient()) {
					float dmg=(((GetSkill(KICK) + GetSTR())/25) *2 +level)*(((float)rand()/RAND_MAX));
					//cout << "Dmg:"<<dmg;
					target->CastToNPC()->Damage(this, (int32)dmg, 0xffff, 0x1e);
					DoAnim(1);
				}
			}
			break;
		case MONK:
			MonkSpecialAttack(target->CastToMob(), ca_atk->m_type);
			break;
		case ROGUE:
			if (ca_atk->m_atk == 100)
			{
				if (BehindMob(target, GetX(), GetY())) // Player is behind target
				{
					RogueBackstab(target, weapon1, GetSkill(BACKSTAB));
					//Yeahlight: Double backstab was an AA as far as I know, commenting this out for now
					//if ((level > 54) && (target != 0))
					//{
					//	float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max

					//	// Check for double attack with main hand assuming maxed DA Skill (MS)
					//	float random = (float)rand()/RAND_MAX;
					//	if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
					//	{
					//		if(target && target->GetHP() > 0)
					//		{
					//			RogueBackstab(target, weapon1, GetSkill(BACKSTAB));	
					//		}
					//	}
					//}
				}
				else	// Player is in front of target
				{
					//Yeahlight: Proc rates are balanced on attack timer, not the backstab refresh timer; prevent procs from this attack
					Attack(target, 13, false);
					//Yeahlight: Frontal BS attempts results in a single attack, commenting this out for now
					//if ((level > 54) && (target != 0))
					//{
					//	float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max

					//	// Check for double attack with main hand assuming maxed DA Skill (MS)
					//	float random = (float)rand()/RAND_MAX;
					//	if(random < DoubleAttackProbability && target && target->GetHP() > 0)		// Max 62.4 % chance of DA
					//		Attack(target, 13, false);													
					//}
				}
			}
			break;
		}
	}
	return;
}

//////////////////////

void Client::ProcessOP_Taunt(APPLAYER* pApp)
{
	if (pApp->size != sizeof(ClientTaunt_Struct)) 
	{
		cout << "Wrong size on OP_Taunt. Got: " << pApp->size << ", Expected: "<< sizeof(ClientTaunt_Struct) << endl;
		return;
	}

	//Yeahlight: Purge client's invisibility
	CancelAllInvisibility();

	ClientTaunt_Struct* cts = (ClientTaunt_Struct*) pApp->pBuffer;
	Mob* tauntee = entity_list.GetMob(cts->tauntTarget);

	//Yeahlight: Target is null or target is not an NPC
	if(!tauntee || (tauntee && !tauntee->IsNPC()))
		return;

	//Yeahlight: Check for taunt skill up
	this->CheckAddSkill(TAUNT, -10);

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
		//           Note: If a PC hits taunt while they have a 300 point hate lead on the next highest entity,
		//                 they will actually lose 299 hate points.
		if(tauntee->CastToNPC()->GetHateTop() == this)
			newhate = tauntee->CastToNPC()->GetNPCHate(tauntee->CastToNPC()->GetHateSecondFromTop()) - tauntee->CastToNPC()->GetNPCHate(this) + 1;
		else
			newhate = tauntee->CastToNPC()->GetNPCHate(tauntee->CastToNPC()->GetHateTop()) - tauntee->CastToNPC()->GetNPCHate(this) + 1;
		tauntee->CastToNPC()->AddToHateList(this, 0, newhate);
	}							
}



//////////////////////

void Client::ProcessOP_GMSummon(APPLAYER* pApp){
	if (pApp->size != sizeof(GMSummon_Struct)) 
	{
		cout << "Wrong size on OP_GMSummon. Got: " << pApp->size << ", Expected: " << sizeof(GMSummon_Struct) << endl;
		return;
	}
	if(admin<100) 
	{
		Message(RED, "You're not awesome enough to use this command!");
		return;
	}
	GMSummon_Struct* gms = (GMSummon_Struct*) pApp->pBuffer;
	Mob* st = entity_list.GetMob(gms->charname);

	if (admin >= 100) 
	{
		int8 tmp = gms->charname[strlen(gms->charname)-1];
		if (st != 0)
		{
			this->Message(BLACK, "Local: Summoning %s to %i, %i, %i", gms->charname, gms->x, gms->y, gms->z);
			if (st->IsClient() && (st->CastToClient()->GetAnon() != 1 || this->Admin() >= st->CastToClient()->Admin()))
			{
				st->CastToClient()->MovePC(0, gms->x, gms->y, gms->z);
			}
			else
			{
				st->GMMove(gms->x, gms->y, gms->z);
			}
		}
		else if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server disconnected");
		}
		else if (tmp < '0' || tmp > '9') 
		{ // dont send to world if it's not a player's name
			ServerPacket* pack = new ServerPacket(ServerOP_ZonePlayer, sizeof(ServerZonePlayer_Struct));

			//pack->pBuffer = new uchar[pack->size]; //Taz : Memory leak again
			memset(pack->pBuffer, 0, pack->size);
			ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
			strcpy(szp->adminname, this->GetName());
			szp->adminrank = this->Admin();
			strcpy(szp->name, gms->charname);
			strcpy(szp->zone, zone->GetShortName());
			szp->x_pos = gms->x;
			szp->y_pos = gms->y;
			szp->z_pos = gms->z;
			szp->ignorerestrictions = true;
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
		else 
			Message(RED, "Error: '%s' not found.", gms->charname);
	}
	else if (st->IsCorpse()) {
		st->CastToCorpse()->Summon(this);
	}
}

//////////////////////

void Client::ProcessOP_GiveItem(APPLAYER* pApp){

	cout << "Requesting trade !" << endl;
	Trade_Window_Struct* msg = (Trade_Window_Struct*) pApp->pBuffer; 
	Mob* tmp = entity_list.GetMob(msg->fromid);
	for(int x=0; x <90 ; x++){
		TradeList[x]=0;
		TradeCharges[x]=0;
	}
	if (!tmp)
		return;
	if (tmp->IsClient()) {
		tmp->CastToClient()->tradepp=0;
		tmp->CastToClient()->tradegp=0;
		tmp->CastToClient()->tradesp=0;
		tmp->CastToClient()->tradecp=0;
	}
	this->tradepp=0;
	this->tradegp=0;
	this->tradesp=0;
	this->tradecp=0;

	if (tmp != 0) {
		if (tmp->IsClient()) {
			tmp->CastToClient()->QueuePacket(pApp);
		}
		else
		{	//npcs always accept

			this->npctradecp=0;
			this->npctradesp=0;
			this->npctradegp=0;
			this->npctradepp=0;

			APPLAYER* outapp= new APPLAYER(OP_TradeAccepted, sizeof(Trade_Window_Struct));
			memset(outapp->pBuffer, 0, outapp->size);
			Trade_Window_Struct* trade = (Trade_Window_Struct*) outapp->pBuffer;
			trade->fromid = msg->toid;
			trade->toid = msg->fromid;
			TradeWithEnt= msg->fromid;
			InTrade = 1;
			QueuePacket(outapp); 
		}
	}
}


void Client::ProcessOP_TradeAccepted(APPLAYER* pApp){

	cout << "trade accepted" << endl;
	Trade_Window_Struct* msg = (Trade_Window_Struct*) pApp->pBuffer; 
	InTrade= 1;
	this->TradeWithEnt = msg->fromid;
	Mob* tmp = entity_list.GetMob(msg->fromid);
	if (tmp != 0) {
		if (tmp->IsClient()) {
			for(int x=0; x < 90; x++){
				TradeList[x]=0;
				TradeCharges[x]=0;
			}
			tmp->CastToClient()->QueuePacket(pApp);
			tmp->CastToClient()->TradeWithEnt=GetID();
			tmp->CastToClient()->InTrade=1;
		}
	}

}


//////////////////////

void Client::ProcessOP_Click_Give(APPLAYER* pApp){

	this->InTrade = 2;
	Mob* tmp = entity_list.GetMob(TradeWithEnt);
	if (tmp != 0) {
		if (tmp->IsClient()) {

			if (tmp->CastToClient()->InTrade == 2)
			{
				bool loreitemcheck=false;
				bool loreitemcheck2=false;
				int i=0;
				//cout << "deal ended" << endl;
				while(!loreitemcheck && i<90){//We check the lore items
					if(tmp->CastToClient()->TradeList[i]!=0)
						loreitemcheck=this->CheckLoreConflict(tmp->CastToClient()->TradeList[i]);
					i++;
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				i=0;
				while(!loreitemcheck2 && i<90){//We check the lore items
					if(this->TradeList[i]!=0)
						loreitemcheck2=tmp->CastToClient()->CheckLoreConflict(this->TradeList[i]);
					i++;
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				if(!loreitemcheck2 && !loreitemcheck){
					if((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp) > 0)
						tmp->CastToClient()->AddMoneyToPP(tmp->CastToClient()->tradecp,tmp->CastToClient()->tradesp,tmp->CastToClient()->tradegp,tmp->CastToClient()->tradepp,true);
					if ((this->tradecp+this->tradesp+this->tradegp+this->tradepp)>0)
						this->AddMoneyToPP(this->tradecp,this->tradesp,this->tradegp,this->tradepp,true);
					tmp->CastToClient()->QueuePacket(pApp);
					this->FinishTrade(tmp->CastToClient());
					tmp->CastToClient()->FinishTrade(this);
				}
				if(loreitemcheck2 || loreitemcheck){
					if ((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp) > 0)
						this->AddMoneyToPP(tmp->CastToClient()->tradecp,tmp->CastToClient()->tradesp,tmp->CastToClient()->tradegp,tmp->CastToClient()->tradepp,true);
					if ((this->tradecp+this->tradesp+this->tradegp+this->tradepp)>0)
						tmp->CastToClient()->AddMoneyToPP(this->tradecp,this->tradesp,this->tradegp,this->tradepp,true);
					tmp->CastToClient()->QueuePacket(pApp);
					this->FinishTrade(this);
					tmp->CastToClient()->FinishTrade(tmp->CastToClient());
				}


				APPLAYER* outapp = new APPLAYER(OP_CloseTrade,0);
				tmp->CastToClient()->QueuePacket(outapp);
				QueuePacket(outapp);
				tmp->CastToClient()->InTrade = 0;
				InTrade = 0;
				tmp->CastToClient()->TradeWithEnt = 0;
				TradeWithEnt = 0;
				safe_delete(outapp);//delete outapp;

			}
			else
				tmp->CastToClient()->QueuePacket(pApp); 				
		}
		else {
			APPLAYER* outapp = new APPLAYER(OP_CloseTrade,0);
			Mob* tmp = entity_list.GetMob(TradeWithEnt);
		
			this->FinishTrade(tmp);

			QueuePacket(outapp);
			cout << "npc deal ended" << endl;
			InTrade = 0;
			TradeWithEnt = 0;
			safe_delete(outapp);//delete outapp;
			//CheckQuests(zone->GetShortName(), tmp, 0, TradeList[0]);

		}
	}
}



//////////////////////
void Client::ProcessOP_CancelTrade(APPLAYER* pApp){

	CancelTrade_Struct* msg = (CancelTrade_Struct*) pApp->pBuffer;
	if (pApp->size != sizeof(CancelTrade_Struct)){
		cout << "Wrong size on OP_CancelTrade. Got: " << pApp->size << ", Expected: " << sizeof(CancelTrade_Struct) << endl;
		return;
	}
	Mob* tmp = entity_list.GetMob(TradeWithEnt);
	if (tmp != 0) {
		if(tmp->IsClient()){
			if ((tmp->CastToClient()->tradecp+tmp->CastToClient()->tradesp+tmp->CastToClient()->tradegp+tmp->CastToClient()->tradepp) > 0)
				this->AddMoneyToPP(tmp->CastToClient()->tradecp,tmp->CastToClient()->tradesp,tmp->CastToClient()->tradegp,tmp->CastToClient()->tradepp,true);
			if ((this->tradecp+this->tradesp+this->tradegp+this->tradepp)>0)
				tmp->CastToClient()->AddMoneyToPP(this->tradecp,this->tradesp,this->tradegp,this->tradepp,true);
		}
		else{
			if ((this->npctradecp+this->npctradesp+this->npctradegp+this->npctradepp) > 0)
				this->AddMoneyToPP(this->npctradecp,this->npctradesp,this->npctradegp,this->npctradepp,true);
			this->npctradecp=0;
			this->npctradesp=0;
			this->npctradegp=0;
			this->npctradepp=0;
		}
		TradeWithEnt = 0;
		msg->fromid = this->GetID();
		QueuePacket(pApp);
		this->FinishTrade(this);
		cout << "The trade was cancelled." << endl;
		InTrade = 0;
		if (tmp->IsClient()) {	
			tmp->CastToClient()->QueuePacket(pApp);	
			tmp->CastToClient()->FinishTrade(tmp->CastToClient());									
			tmp->CastToClient()->InTrade = 0;										
			tmp->CastToClient()->TradeWithEnt = 0;									
		}
	}

	return;
}


void Client::ProcessOP_SplitMoney(APPLAYER* pApp){

	//FileDumpPacket("Splitmoney.txt", pApp);

	Split_Struct *split = (Split_Struct *)pApp->pBuffer;
	//cout<<"split :platinum = " <<(int)split->platinum<< " gold = "<<(int)split->gold << " silver = " <<(int)split->silver << " copper = " <<(int)split->copper <<endl;

	if(!this->IsGrouped())
		return;
	Group *cgroup = entity_list.GetGroupByClient(this);

	if(!cgroup) {
		//invalid group, not sure if we should say more...
		Database::Instance()->SetGroupID(this->GetName(), 0);
		memset(this->GetPlayerProfilePtr()->GroupMembers, 0, sizeof(this->GetPlayerProfilePtr()->GroupMembers));
		return;
	}
	if (cgroup->GetMembersInZone() <= 1) return;

	if(!TakeMoneyFromPP(split->copper + 10 * split->silver + 100 * split->gold + 1000 * split->platinum)) {
		return;
	}
	cgroup->SplitMoney(split->copper, split->silver, split->gold, split->platinum,this);
	return;
}


//////////////////////
void Client::ProcessOP_Random(APPLAYER* pApp)
{
	if (pApp->size != sizeof(Random_Struct)){
		cout << "Wrong size on OP_Random. Got: " << pApp->size << ", Expected: " << sizeof(OP_Random) << endl;
		return;
	}

	Random_Struct* rndq = (Random_Struct*) pApp->pBuffer;
	if (rndq->low == rndq->high)
	{
		int32 num = rndq->low; // just use low as its the same as high!
		entity_list.Message(0, LIGHTEN_GREEN, "%s rolled their dice and got a %d out of %d to %d!", this->GetName(), num, rndq->low, rndq->high);
		return; // This stops a Zone Crash.
	}

	unsigned int rndqa = 0;
	if (rndq->low > rndq->high) // They did Big number first.
	{
		rndqa = rndq->low;
		rndq->low = rndq->high;
		rndq->high = rndqa;
	}

	if (rndq->low == 0) 
	{
		rndqa = 2;
	}
	else
	{
		rndqa = rndq->low;
	}
	srand( (Timer::GetCurrentTime() * Timer::GetCurrentTime()) / rndqa);
	rndqa = (rand()  % (rndq->high - rndq->low)) + rndq->low;
	entity_list.Message(0, LIGHTEN_GREEN, "%s rolled their dice and got a %d out of %d to %d!", this->GetName(), rndqa, rndq->low, rndq->high);
}

//////////////////////

void Client::ProcessOP_GMHideMe(APPLAYER* pApp)
{
	//DumpPacket(pApp);
	if(admin<100) 
	{
		Message(RED, "You're not awesome enough to use this command!");
		return;
	}
	SetInvisible(!this->invisible);
}



//////////////////////

void Client::ProcessOP_GMNameChange(APPLAYER* pApp)
{
	if(admin < 100) 
	{
		Message(RED, "You're not awesome enough to use this command!");
		return;
	}

	if (pApp->size != sizeof(GMName_Struct)) 
	{
		cout << "Wrong size on OP_GMNameChange. Got: " << pApp->size << ", Expected: " << sizeof(GMName_Struct) << endl;
		return;
	}

	GMName_Struct* gmn = (GMName_Struct *)pApp->pBuffer;

	Client* client = entity_list.GetClientByName(gmn->oldname);

	bool usedname = Database::Instance()->CheckUsedName((char*) gmn->newname);

	if(client==0) 
	{
		Message(RED, "%s not found for name change. Operation failed!", gmn->oldname);
		return;
	}

	int len = strlen(gmn->newname);

	if((len > 15) || (len == 0)) 
	{
		Message(RED, "Invalid number of characters in new name (%s).", gmn->newname);
		return;
	}

	if (!usedname) 
	{
		Message(RED, "%s is already in use.  Operation failed!", gmn->newname);
		return;
	}

	UpdateWho(true);

	Database::Instance()->UpdateName(gmn->oldname, gmn->newname);

	strcpy(client->name, gmn->newname);

	client->Save();

	if(gmn->badname==1) 
	{
		Database::Instance()->AddToNameFilter(gmn->oldname);
	}

	gmn->unknown[1] = 1;

	entity_list.QueueClients(this, pApp, false);

	UpdateWho(false);
}


//////////////////////

void Client::ProcessOP_GMKill(APPLAYER* pApp){
	if(admin<100) 
	{
		Message(RED, "You're not awesome enough to use this command!");
		return;
	}
	GMKill_Struct* gmk = (GMKill_Struct *)pApp->pBuffer;
	if (pApp->size != sizeof(GMKill_Struct)) 
	{
		cout << "Wrong size on OP_GMKill. Got: " << pApp->size << ", Expected: " << sizeof(GMKill_Struct) << endl;
		return;
	}
	Mob* obj = entity_list.GetMob(gmk->name);
	if(obj!=0) 
	{
		obj->Kill();
	}
	else 
	{
		if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server disconnected");
		}
		else 
		{
			ServerPacket* pack = new ServerPacket(ServerOP_KillPlayer, sizeof(ServerKillPlayer_Struct));

			//pack->pBuffer = new uchar[pack->size]; //Tazadar : Memory leak !!
			ServerKillPlayer_Struct* skp = (ServerKillPlayer_Struct*) pack->pBuffer;
			strcpy(skp->gmname, gmk->gmname);
			strcpy(skp->target, gmk->name);
			skp->admin = this->Admin();
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
}

//////////////////////

void Client::ProcessOP_GMSurname(APPLAYER* pApp){
	//DumpPacket(pApp);
	if (pApp->size != sizeof(GMSurname_Struct)) 
	{
		cout << "Wrong size on OP_GMSurname. Got: " << pApp->size << ", Expected: " << sizeof(GMSurname_Struct) << endl;
		return;
	}
	GMSurname_Struct* gmln = (GMSurname_Struct*) pApp->pBuffer;
	if (strlen(gmln->Surname) >= 20) 
	{
		Message(RED, "/Surname: New last name too long. (max=19)");
	}
	else 
	{
		Client* client = entity_list.GetClientByName(gmln->name);
		if (client == 0) 
		{
			Message(RED, "/Surname: %s not found", gmln->name);
		}
		else 
		{
			client->ChangeSurname(gmln->Surname);
		}
		gmln->unknown[0] = 1;
		entity_list.QueueClients(this, pApp, false);
	}
}
//////////////////////

void Client::ProcessOP_GMToggle(APPLAYER* pApp){
	if (pApp->size != 36) 
	{
		cout << "Wrong size on OP_GMToggle. Got: " << pApp->size << ", Expected: " << 36 << endl;
		return;
	}
	if (pApp->pBuffer[32] == 0) 
	{
		Message(BLACK, "Turning tells OFF");
		tellsoff = true;
	}
	else if (pApp->pBuffer[32] == 1) 
	{
		Message(BLACK, "Turning tells ON");
		tellsoff = false;
	}
	else 
	{
		Message(BLACK, "Unkown value in /toggle packet");
	}
	UpdateWho();
}

//////////////////////

void Client::ProcessOP_LFG(APPLAYER* pApp){
	if (pApp->size != sizeof(LFG_Struct)) 
	{
		cout << "Wrong size on OP_LFG. Got: " << pApp->size << ", Expected: " << sizeof(LFG_Struct) << endl;
		return;
	}
	LFG_Struct* lfg = (LFG_Struct*) pApp->pBuffer;
	if (lfg->value == 1) 
	{
		//cout << this->GetName() << " turning LFG on." << endl;
		this->LFG = true;
	}
	else if (lfg->value == 0) 
	{
		//cout << this->GetName() << " turning LFG off." << endl;
		this->LFG = false;
	}
	else
	{
		Message(BLACK, "Error: unknown LFG value");
	}
	this->UpdateWho();
}


//////////////////////

void Client::ProcessOP_GMGoto(APPLAYER* pApp){
	if (pApp->size != sizeof(GMGoto_Struct)) 
	{
		cout << "Wrong size on OP_GMGoto. Got: " << pApp->size << ", Expected: " << sizeof(GMGoto_Struct) << endl;
		return;
	}
	if (admin < 100) 
	{
		Message(RED, "You're not awesome enough to use this command!");					
		return;
	}
	GMGoto_Struct* gmg = (GMGoto_Struct*) pApp->pBuffer;
	Mob* gt = entity_list.GetMob(gmg->gotoname);

	if (gt != 0) 
	{
		this->MovePC(0, gt->GetX(), gt->GetY(), gt->GetZ());
	}
	else if (!worldserver.Connected())
	{
		Message(BLACK, "Error: World server disconnected.");
	}
	else 
	{
		ServerPacket* pack = new ServerPacket(ServerOP_GMGoto, sizeof(ServerGMGoto_Struct));

		//pack->pBuffer = new uchar[pack->size]; //Tazadar : Memory leak !
		memset(pack->pBuffer, 0, pack->size);
		ServerGMGoto_Struct* wsgmg = (ServerGMGoto_Struct*) pack->pBuffer;
		strcpy(wsgmg->myname, this->GetName());
		strcpy(wsgmg->gotoname, gmg->gotoname);
		wsgmg->admin = admin;
		worldserver.SendPacket(pack);
		safe_delete(pack);//delete pack;
	}

}
void Client::ProcessOP_ShopRequest(APPLAYER* pApp){
	/* this works*/			
	Merchant_Click_Struct* mc=(Merchant_Click_Struct*)pApp->pBuffer;

	if (pApp->size != sizeof(Merchant_Click_Struct)){
		cout << "Packet size: " << pApp->size << "  Expected: " << sizeof(Merchant_Click_Struct) << endl;
		return;
	}
	// Send back opcode OP_ShopRequest - tells client to open merchant window.
	APPLAYER* outapp = new APPLAYER(OP_ShopRequest, sizeof(Merchant_Click_Struct));
	Merchant_Click_Struct* mco=(Merchant_Click_Struct*)outapp->pBuffer;
	Mob* tmp = entity_list.GetMob(mc->entityid);

	mco->entityid = mc->entityid;
	mco->playerid = mc->playerid;

	int8 action=0x01;

	if(tmp->CastToNPC()->IsEngaged()){
		this->Message(BLACK, "%s says, 'Can't you see I am busy here?'");
		action = 0;
	}

	//Yeahlight: Merchants ignore invis PCs
	if(this->GetInvisible())
		action = 0;
	
	if(action !=0){
		FACTION_VALUE FactionLevel = GetFactionLevel(character_id, tmp->GetNPCTypeID(), race, class_, deity, tmp->CastToNPC()->GetPrimaryFactionID(), tmp);
	
		if(FactionLevel == FACTION_DUBIOUS){
			action = 0;
			this->Message(BLACK, "%s says, 'Get out of here !'",tmp->CastToNPC()->GetName());
		}
	}

	mco->unknown[0] = action; // Open ( = 1) or Close ( = 0)
	mco->unknown[1] = 0x03;
	mco->unknown[2] = 0x00;
	mco->unknown[3] = 0x00;
	if(action){ // Tazadar : We calculate the pricemultiplier
		mco->pricemultiplier=2.5;
	}
	else{ // Tazadar : We do not need to calculate the pricemultiplier
		mco->pricemultiplier=200;
	}
	//DumpPacketHex(outapp);
	QueuePacket(outapp);
	safe_delete(outapp);

	if(action == 0) // Tazadar : We have nothing more to do, vendor hates you :)
		return;

	int merchantid = 0;
	this->pricemultiplier=mco->pricemultiplier;
	
	if (tmp != 0 && tmp->IsNPC())
	{
		merchantid=tmp->CastToNPC()->MerchantType;
	}
	else {
		EQC_MOB_EXCEPT("void Client::ProcessOP_ShopRequest()", "NPC not found" );
		return;
	}

	//cout << "merchantid: " << (int)merchantid << endl;

	UpdateGoods(merchantid);
	this->merchantid=merchantid;
}

//////////////////////
//Tazadar: It removes players sold items now
void Client::ProcessOP_ShopPlayerBuy(APPLAYER* pApp){						
	cout << name << " is attempting to purchase an item..  " << endl;
	Merchant_Purchase_Struct* mp=(Merchant_Purchase_Struct*)pApp->pBuffer;
	if (pApp->size != sizeof(Merchant_Purchase_Struct)) {
		cout << "Packet size: " << pApp->size << "  Expected: " << sizeof(Merchant_Purchase_Struct) << endl;
		return;
	}
	DumpPacket(pApp);
	int merchantid;
	bool update=false;//Tazadar : Update the vendor window?
	bool sold=false;// Tazadar : Item has been sold?
	Mob* tmp = entity_list.GetMob(mp->npcid);
	if (tmp != 0)
	{
		merchantid=tmp->CastToNPC()->MerchantType;
	}
	else 
	{
		return;
	}
	uint16 item_nr = this->merchantgoods[mp->itemslot];//Tazadar : We check if the item really exists
	if (item_nr == 0)
	{
		return;
	}
	int16 slot =Database::Instance()->MerchantHasItem(this->merchantid,item_nr);//Tazadar: We load the slot from the DB.
	//cout<<"slot of the item = " << (int) slot << endl;
	if (!slot){//Tazadar: If the item has been already sold while the player is browsing vendor goods(vendor still shows the item but no longer have it) 
		//Tazadar: Removing the item from the window before the end of transaction tells the client that the item is no longer avaible.
		//Tazadar: So we send a packet in order to remove it ;)
		APPLAYER* outapp = new APPLAYER(OP_ShopDelItem, sizeof(Merchant_DelItem_Struct));
		Merchant_DelItem_Struct* del = (Merchant_DelItem_Struct*)outapp->pBuffer;
		memset(outapp->pBuffer, 0, sizeof(Merchant_DelItem_Struct));
		del->npcid=this->merchantid;			
		del->playerid=this->GetID();
		del->itemslot=mp->itemslot;

		QueuePacket(outapp);
		safe_delete(outapp);

		sold=true;//Tazadar:The item has already been sold and trade is canceled !
	}
	if(sold==false){//Tazadar:If the vendor has the item ! :)
		int32 stack;
		stack=Database::Instance()->GetMerchantStack(this->merchantid,item_nr);//Tazdar:We count how many items he has
		if(stack!=0){//Tazadar:If we dont have unlimited amount of items !! :)
			if(stack<=mp->quantity){//Tazadar:If you want more(or the same amount) than the vendor has !
				Database::Instance()->RemoveMerchantItem(this->merchantid,item_nr);
				Database::Instance()->UpdateAfterWithdraw(this->merchantid,slot+1,Database::Instance()->GetLastSlot(this->merchantid,this->startingslot));
				update=true;
			}
			if(stack>mp->quantity){//Tazadar:Vendor has all you want :)
				Database::Instance()->UpdateStack(this->merchantid,stack-mp->quantity,item_nr);
				stack=mp->quantity;
			}
		}
		else{//Tazadar:We have unlimited amount of items !! :)
			stack=mp->quantity;
		}
		Item_Struct* item = Database::Instance()->GetItem(item_nr);				
		APPLAYER* outapp = new APPLAYER(OP_ShopPlayerBuy, sizeof(Merchant_Purchase_Struct));
		Merchant_Purchase_Struct* mpo=(Merchant_Purchase_Struct*)outapp->pBuffer;							
		mpo->quantity = stack;
		mpo->playerid = mp->playerid;
		mpo->npcid = mp->npcid;
		mpo->itemslot = mp->itemslot;
		mpo->unknown001 = 0x00;
		mpo->unknown002 = 0x00;
		mpo->unknown003 = 0x00;
		mpo->unknown004 = 0x00; 
		mpo->unknown005 = 0x00;
		int32 exactprice=floor(item->cost*this->pricemultiplier);
		if((item->cost*this->pricemultiplier-exactprice) > 0.49)
			exactprice++;
		mpo->itemcost =  exactprice * stack;
		cout << "Debug: client initially have: plat: " << pp.platinum<<" gold: " << pp.gold << " silver: " << pp.silver << " copper: " << pp.copper << endl;
		cout << "taking " << mpo->itemcost << " copper from " << name << "."<< endl;
		cout<< "Item cost " << item->cost<<endl;
		QueuePacket(outapp);
		if(item->cost <= 0){
			int tmp=AutoPutItemInInventory(item,stack,-1);
			if(tmp==0){
				this->Message(WHITE,"Your inventory appears full now!");
			}
		}else if(TakeMoneyFromPP(exactprice)){ //Cofruben: check if we can..
			int tmp=AutoPutItemInInventory(item,stack,-1);
			if(tmp==0){
				this->Message(WHITE,"Your inventory appears full now!");
			}
		}
		delete(outapp);
		this->Save();
		if(update){
			this->UpdateGoodsBuy(mp->itemslot);
		}
	}
	if(sold==true){//Tazadar:Even if its sold we send a packet to end the transaction(amounts have no meaning because the item is no longer in the window)
		Item_Struct* item = Database::Instance()->GetItem(item_nr);				
		APPLAYER* outapp = new APPLAYER(OP_ShopPlayerBuy, sizeof(Merchant_Purchase_Struct));
		Merchant_Purchase_Struct* mpo=(Merchant_Purchase_Struct*)outapp->pBuffer;							
		mpo->quantity = 1;
		mpo->playerid = mp->playerid;
		mpo->npcid = mp->npcid;
		mpo->itemslot = mp->itemslot;													
		mpo->unknown001 = 0x00;
		mpo->unknown002 = 0x00;
		mpo->unknown003 = 0x00;
		mpo->unknown004 = 0x00; 
		mpo->unknown005 = 0x00;
		mpo->itemcost = 0xFF;
		QueuePacket(outapp);
		delete(outapp);

		this->UpdateGoodsBuy(mp->itemslot);
	}
}


//////////////////////
//Tazadar: It adds players sold items now
void Client::ProcessOP_ShopPlayerSell(APPLAYER* pApp){
	Merchant_Purchase_Struct* mp=(Merchant_Purchase_Struct*)pApp->pBuffer;
	//FileDumpPacket("selling.txt", pApp);
	if (pApp->size != sizeof(Merchant_Purchase_Struct)) {
		cout << "Packet size: " << pApp->size << "  Expected: " << sizeof(Merchant_Purchase_Struct) << endl;
		return;
	}
	cout << name << " is trying to sell an item from the slot: " << (int)mp->itemslot <<endl;

	uint16 item_nr=0;
	bool itemhere=false;
	if(mp->itemslot<30){
		if(pp.inventory[mp->itemslot] != 0xFFFF){
			item_nr=pp.inventory[mp->itemslot];
			itemhere=true;
		}
	}
	if(mp->itemslot>249){
		if(pp.containerinv[mp->itemslot-250]!=0xFFFF){
			item_nr=pp.containerinv[mp->itemslot-250];
			itemhere=true;
		}
	}
	if (itemhere) 
	{
		Item_Struct* itm = Database::Instance()->GetItem(item_nr);
		int exactprice=floor(itm->cost-(itm->cost*(1-(1/this->pricemultiplier))));
		if((itm->cost-(itm->cost*(1-(1/this->pricemultiplier)))-exactprice) > 0.49)
			exactprice++;
		sint32 Cost = exactprice*((mp->quantity == 0) ? 1:mp->quantity);
		cout<<"cost = "<< Cost <<endl;
		int32 slotitem =Database::Instance()->MerchantHasItem(this->merchantid,item_nr);
		if(slotitem==0){//Tazadar:We add the item to the vendor list
			if(mp->quantity==0)
				mp->quantity=1;
			//Database::Instance()->AddMerchantItem(this->merchantid,item_nr,mp->quantity,Database::Instance()->GetLastSlot(this->merchantid,this->startingslot)+1); Tazadar : For now we do not add item in DB , since we are working on it !!
			//this->UpdateGoodsSell(item_nr);
		}
		if(slotitem!=0){
			int stack=Database::Instance()->GetMerchantStack(this->merchantid,pp.inventory[mp->itemslot]);
			if(stack!=0){//Tazadar:If we dont have unlimited amount of items !!
				Database::Instance()->UpdateStack(this->merchantid,stack+mp->quantity,item_nr);
				if(!ShowedVendorItem(item_nr)){//Tazadar:If vendor does not show the sold item we ask him to :)
					this->UpdateGoodsSell(item_nr);
				}
			}
		}

		AddMoneyToPP(Cost,false);
		if(mp->itemslot<30 ){
			pp.inventory[mp->itemslot] = 0xFFFF;
		}
		if(mp->itemslot>249){
			pp.containerinv[mp->itemslot-250]=0xFFFF;
		}
	}

	QueuePacket(pApp); // Just send it back to accept the deal
	this->Save();
	cout << "response from sell action.." << endl;
}

//////////////////////

void Client::ProcessOP_Petition(APPLAYER* pApp){
	if (pApp->size <= 1)
	{
		return;
	}

	pApp->pBuffer[pApp->size - 1] = 0;

	if (!worldserver.Connected())
	{
		Message(BLACK, "Error: World server disconnected");
	}
	else
	{
		Petition* pet = new Petition;
		pet->SetAName(this->AccountName());
		pet->SetClass(this->GetClass());
		pet->SetLevel(this->GetLevel());
		pet->SetCName(this->GetName());
		pet->SetRace(this->GetRace());
		pet->SetLastGM("");
		pet->SetCName(this->GetName());
		pet->SetPetitionText((char*) pApp->pBuffer);
		pet->SetZone(zone->GetShortName()); // Set the zone of the pet
		pet->SetUrgency(0);
		petition_list.AddPetition(pet);
		Database::Instance()->InsertPetitionToDB(pet);
		petition_list.UpdateGMQueue();
		petition_list.UpdateZoneListQueue();
	}
}




//////////////////////

void Client::ProcessOP_Social_Text(APPLAYER* pApp)
{
	//cout << "Social Text: " << pApp->size << endl;		
	//DumpPacketHex(pApp);

	APPLAYER *outapp = new APPLAYER(pApp->opcode, sizeof(Emote_Text));
	//Yeahlight: Old client method
	//outapp->pBuffer[0] = pApp->pBuffer[0];
	//outapp->pBuffer[1] = pApp->pBuffer[1];
	//uchar *cptr = outapp->pBuffer + 2;
	//cptr += sprintf((char *)cptr, "%s", GetName());
	//cptr += sprintf((char *)cptr, "%s", pApp->pBuffer + 2);

	//Yeahlight: New client method
	uchar *cptr = outapp->pBuffer;
	cptr += sprintf((char *)cptr, "%s", GetName());
	cptr += sprintf((char *)cptr, "%s", pApp->pBuffer);
	//cout << "Check target" << endl;

	if(target != NULL && target->IsClient() && target != this)
	{
		//cout << "Client targeted" << endl;
		entity_list.QueueCloseClients(this, outapp, true, 100, target);
		//cptr += sprintf((char *)cptr, " Special message for you, the target");
		cptr = outapp->pBuffer + 2;

		// not sure if live does this or not.  thought it was a nice feature, but would take a lot to clean up grammatical and other errors.  Maybe with a regex parser...
		/*
		replacestr((char *)cptr, target->GetName(), "you");
		replacestr((char *)cptr, " he", " you");
		replacestr((char *)cptr, " she", " you");
		replacestr((char *)cptr, " him", " you");
		replacestr((char *)cptr, " her", " you");
		*/
		entity_list.GetMob(target->GetName())->CastToClient()->QueuePacket(outapp);
	}
	else
	{
		entity_list.QueueCloseClients(this, outapp, true, 100);
	}
	safe_delete(outapp);//delete outapp;
}

//////////////////////

void Client::ProcessOP_PetitionCheckout(APPLAYER* pApp){
	if (pApp->size != sizeof(int32)) 
	{
		cout << "Wrong size: OP_PetitionCheckout, size=" << pApp->size << ", expected " << sizeof(int32) << endl;
		return;
	}
	if (!worldserver.Connected())
	{
		Message(BLACK, "Error: World server disconnected");
	}
	else
	{
		int32 getpetnum = *((int32*) pApp->pBuffer);
		Petition* getpet = petition_list.GetPetitionByID(getpetnum);
		if (getpet != 0) 
		{
			getpet->AddCheckout();
			getpet->SetCheckedOut(true);
			getpet->SendPetitionToPlayer(this->CastToClient());
			petition_list.UpdatePetition(getpet);
			petition_list.UpdateGMQueue();
			petition_list.UpdateZoneListQueue();
		}
		else 
		{
			Message(BLACK, "Automated System Message: Your client is bugged.  Please camp to the login server to fix!");
		}
	}
}

//////////////////////

void Client::ProcessOP_PetitionDelete(APPLAYER* pApp){
	APPLAYER* outapp = new APPLAYER(OP_PetitionClientUpdate,sizeof(PetitionClientUpdate_Struct));
	PetitionClientUpdate_Struct* pet = (PetitionClientUpdate_Struct*) outapp->pBuffer;
	pet->petnumber = *((int*) pApp->pBuffer);
	pet->color = 0x00;
	pet->senttime = 0xFFFFFFFF;
	pet->unknown[3] = 0x00;
	pet->unknown[2] = 0x00;
	pet->unknown[1] = 0x00;
	pet->unknown[0] = 0x00;
	strcpy(pet->accountid, "");
	strcpy(pet->gmsenttoo, "");
	pet->something = 0x00;
	strcpy(pet->charname, "");
	QueuePacket(outapp);

	if (petition_list.DeletePetition(pet->petnumber) == -1) 
	{
		cout << "Something is borked with: " << pet->petnumber << endl;
	}
	petition_list.ClearPetitions();
	petition_list.UpdateGMQueue();
	petition_list.ReadDatabase();
	petition_list.UpdateZoneListQueue();
	safe_delete(outapp);//delete outapp;
}

//////////////////////

void Client::ProcessOP_GMDelCorpse(APPLAYER* pApp){
	if(admin<100) 
	{
		Message(RED, "You're not awesome enough to use this command!");
		return;
	}		
	GMDelCorpse_Struct* dc = (GMDelCorpse_Struct *)pApp->pBuffer;
	Mob* corpse = entity_list.GetMob(dc->corpsename);

	if(corpse==0)
	{
		return;
	}

	if(corpse->IsCorpse() != true) 
	{
		return;
	}
	corpse->CastToCorpse()->Delete();
	cout << name << " deleted corpse " << dc->corpsename << endl;
	Message(RED, "Corpse %s deleted.", dc->corpsename);
}

//////////////////////

void Client::ProcessOP_GMKick(APPLAYER* pApp){
	if(admin<150)
	{
		Message(RED, "You're not awesome enough to use this command!");
		return;
	}
	GMKick_Struct* gmk = (GMKick_Struct *)pApp->pBuffer;
	Client* client = entity_list.GetClientByName(gmk->name);

	if(client==0) 
	{
		if (!worldserver.Connected())
		{
			Message(BLACK, "Error: World server disconnected");
		}
		else 
		{
			ServerPacket* pack = new ServerPacket(ServerOP_KickPlayer, sizeof(ServerKickPlayer_Struct));
			//pack->pBuffer = new uchar[pack->size]; // Memory leak !!
			ServerKickPlayer_Struct* skp = (ServerKickPlayer_Struct*) pack->pBuffer;
			strcpy(skp->adminname, gmk->gmname);
			strcpy(skp->name, gmk->name);
			skp->adminrank = this->Admin();
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
	client->Kick();
}

//////////////////////

void Client::ProcessOP_PetitionCheckIn(APPLAYER* pApp){
	Petition_Struct* inpet = (Petition_Struct*) pApp->pBuffer;
	Petition* pet = petition_list.GetPetitionByID(inpet->petnumber);
	if (inpet->urgency != pet->GetUrgency())
	{
		pet->SetUrgency(inpet->urgency);
	}
	pet->SetLastGM(this->GetName());
	pet->SetCheckedOut(false);
	petition_list.UpdatePetition(pet);
	petition_list.UpdateGMQueue();
	petition_list.UpdateZoneListQueue();
}


void Client::ProcessOP_BugReport(APPLAYER* pApp)
{
	if (pApp->size == sizeof(Bug_Report_Struct))
	{
		Bug_Report_Struct* newBugReport = new Bug_Report_Struct;
		memcpy(newBugReport, pApp->pBuffer, sizeof(Bug_Report_Struct));

		int crashes = (int)newBugReport->noncrashbug;
		int duplicate = (int)newBugReport->cannotuplicate;

		Database::Instance()->AddBugReport(crashes, duplicate, newBugReport->playername, newBugReport->bugdescription);
	}
	else
	{
		cout << "Wrong size: OP_BugReport, size=" << pApp->size << ", expected " << sizeof(Bug_Report_Struct) << endl;
	}
}


void Client::ProcessOP_GMServers(APPLAYER* pApp)
{
	if (!worldserver.Connected())
	{
		Message(BLACK, "Error: World server disconnected");
	}
	else 
	{
		ServerPacket* pack = new ServerPacket;
		pack->size = strlen(this->GetName())+2;
		//pack->pBuffer = new uchar[pack->size]; // Taz : Memory leak 
		memset(pack->pBuffer, 0, pack->size);
		pack->opcode = ServerOP_ZoneStatus;
		memset(pack->pBuffer, (int8) admin, 1);
		strcpy((char *) &pack->pBuffer[1], this->GetName());
		worldserver.SendPacket(pack);
		safe_delete(pack);//delete pack;
	}
}


void Client::ProcessOP_AutoAttack(APPLAYER* pApp)
{
	if (pApp->size == 4)
	{
		if (pApp->pBuffer[0] == 0)
		{
			auto_attack = false;
		}
		else if (pApp->pBuffer[0] == 1)
		{
			//Yeahlight: Purge client's invisibility
			CancelAllInvisibility();
			auto_attack = true;
		}
	}
	else
	{
		cerr << "Wrong size " << pApp->size << " on OP_AutoAttack, should be 4" << endl; 
	}
}

/* froglok23 - 11th of 10th 2007 - Moved methods into here and added opcoide size checks! */
//////////////////////

void Client::ProcessOP_ChannelMessage(APPLAYER* pApp)
{
	if (pApp->size >= sizeof(ChannelMessage_Struct)) // was ==, but needs to be >=
	{
		ChannelMessage_Struct* cm=(ChannelMessage_Struct*)pApp->pBuffer;
		ChannelMessageReceived(cm->chan_num, cm->language, &cm->message[0], &cm->targetname[0]);
	}
	else
	{
		cout << "Wrong size on OP_ChannelMessage" << pApp->size << ", should be " << sizeof(ChannelMessage_Struct) << "+ on 0x" << hex << setfill('0') << setw(4) << pApp->opcode << dec << endl;
	}
}

void Client::ProcessOP_WearChange(APPLAYER* pApp)
{
	if (pApp->size == sizeof(WearChange_Struct))
	{
		WearChange_Struct* wc=(WearChange_Struct*)pApp->pBuffer;
		entity_list.QueueClients(this, pApp, true);

	}
	else
	{
		cout << "Wrong size: OP_WearChange, size=" << pApp->size << ", expected " << sizeof(WearChange_Struct) << endl;
	}
}


void Client::ProcessOP_Save(APPLAYER* pApp)
{
	if(pApp->size == sizeof(PlayerProfile_Struct)) 
	{ 							
		EQC::Common::PrintF(CP_CLIENT, "Got a player save request (OP_Save)\n");
		this->Save();
		//PlayerProfile_Struct* in_pp = (PlayerProfile_Struct*) pApp->pBuffer;
		//x_pos = in_pp->x;
		//y_pos = in_pp->y;
		//z_pos = in_pp->z;

		///*for(int i = 0 ; i<30 ; i++){
		////cout << "i got a stack of " << (int)in_pp->invItemProprieties[i].charges << " for the item number" << (int) i<< endl;
		//pp.invItemProprieties[i]=in_pp->invItemProprieties[i]; //Save the charges of the items in the inventory !
		//}
		//for(int i = 0; i <80 ; i++){
		//pp.bagItemProprieties[i]=in_pp->bagItemProprieties[i];
		//}
		//for(int i =0; i<10 ;i++){
		//pp.cursorItemProprieties[i]=in_pp->cursorItemProprieties[i];
		//}
		//for(int i=0;i<8;i++){
		//pp.bank_inv[i]=in_pp->bank_inv[i];
		//}
		//for(int i=0;i<80;i++){
		//pp.bank_cont_inv[i]=in_pp->bank_cont_inv[i];
		//}*/
		//this->merchantid=0;
		//heading = in_pp->heading;
		//memcpy(&pp, in_pp, sizeof(PlayerProfile_Struct));
		///*for(int i=0;i<15;i++){
		//cout<<"duration of the buff number "<< (int)i << " = " << (int)in_pp->buffs[i].duration <<" level of the caster = "<<(int)in_pp->buffs[i].level <<" visable = " << (int)in_pp->buffs[i].visable <<" spell id = " <<(int)in_pp->buffs[i].spellid <<endl;
		//cout<<"duration of the buff number one on the serv = " << (int)this->buffs[i].ticsremaining<<endl;
		//}*/
	}
	else
	{
		cout << "Wrong size on OP_Save. Got: " << pApp->size << ", Expected: " << sizeof(PlayerProfile_Struct) << endl;
	}
}


void Client::ProcessOP_WhoAll(APPLAYER* pApp)
{
	if (pApp->size == sizeof(Who_All_Struct)) 
	{ 							
		Who_All_Struct* whoall = (Who_All_Struct*) pApp->pBuffer;
		WhoAll(whoall);
	}
	else
	{
		cout << "Wrong size on OP_Save. Got: " << pApp->size << ", Expected: " << sizeof(Who_All_Struct) << endl;
	}
}

/** This get's used for at least fishing, and possibilty other abilities and monk abilities. (One-Way) */
/* Cofruben: fishing is done by op_fish, I think, anyways the opcode was incorrect.

void Client::ProcessOP_UseAbility(APPLAYER* pApp)
{
if (pApp->size == sizeof(UseAbility_Struct)) 
{
UseAbility_Struct* uas = (UseAbility_Struct*)pApp->pBuffer;
//CheckAddSkill(SKILL_FISHING, 0);

//Use bait.
//Start fishing timer.
//Root.
//Unroot after timer is over.
//SummonItem(some_fishing_item,0);

}
else
{
cout << "Wrong size on OP_UseAbility. Got: " << pApp->size << ", Expected: " << sizeof(UseAbility_Struct) << endl;
}
}
*/

void Client::ProcessOP_Buff(APPLAYER* pApp)
{
	if (pApp->size != sizeof(Buff_Struct)) {
		cout << "Wrong size on OP_Buff. Got: " << pApp->size << ", Expected: " << sizeof(Buff_Struct) << endl;
		return;
	}
	SpellBuffFade_Struct* sbf = (SpellBuffFade_Struct*) pApp->pBuffer;
	if (sbf->spellid != 65535) {
		cout << "Client Buff_Fade! " << (int)sbf->spellid << endl;
		cout << "Player id " << (int)this->GetID() << endl;
		BuffFade(spells_handler.GetSpellPtr(sbf->spellid));
	}
}


void Client::ProcessOP_GMEmoteZone(APPLAYER* pApp)
{
	if(admin < 80) 
	{
		Message(RED, "You're not awesome enough to use this command!");
		return;
	}

	if(pApp->size == sizeof(GMEmoteZone_Struct))
	{
		GMEmoteZone_Struct* gmez = (GMEmoteZone_Struct*)pApp->pBuffer;
		entity_list.Message(0, BLACK, gmez->text);
	}
	else
	{
		cout << "Wrong size on OP_GMEmoteZone. Got: " << pApp->size << ", Expected: " << sizeof(OP_GMEmoteZone) << endl;
	}
}


/** Sent from client doing the Inspecting.  We then relay to target client so he gets =================================
the 'blabla is inspecting your equipment message, then that triggers the target
client to send us an InspectAnswer packet. */
void Client::ProcessOP_InspectRequest(APPLAYER* pApp)
{
	if(pApp->size == sizeof(InspectRequest_Struct))
	{
		InspectRequest_Struct* ins = (InspectRequest_Struct*) pApp->pBuffer; 
		Mob* tmp = entity_list.GetMob(ins->TargetID); 
		tmp->CastToClient()->QueuePacket(pApp); // Send request to target 
	}
	else
	{
		cout << "Wrong size on OP_InspectRequest. Got: " << pApp->size << ", Expected: " << sizeof(InspectRequest_Struct) << endl;
	}
}

/** This gets relayed from the sender to the target which is the one who did the original ================================
inspect request. Works perfectly, and profile works perfectly.*/
void Client::ProcessOP_InspectAnswer(APPLAYER* pApp)
{
	if(pApp->size == sizeof(InspectAnswer_Struct))
	{
		InspectAnswer_Struct* ins = (InspectAnswer_Struct*) pApp->pBuffer; 
		Mob* tmp = entity_list.GetMob(ins->TargetID); 
		tmp->CastToClient()->QueuePacket(pApp); // Send answer to requester 
	}
	else
	{
		cout << "Wrong size on OP_InspectAnswer. Got: " << pApp->size << ", Expected: " << sizeof(InspectAnswer_Struct) << endl;
	}
}

void Client::ProcessOP_ReadBook(APPLAYER* pApp)
{

	if(pApp->size <= sizeof(BookRequest_Struct))
	{
		BookRequest_Struct* book = (BookRequest_Struct*) pApp->pBuffer;
		ReadBook(book->txtfile);
	} 
	else
	{
		cout << "Wrong size on OP_ReadBook. Got: " << pApp->size << ", Expected: " << sizeof(BookRequest_Struct) << endl;
	}
}

//o--------------------------------------------------------------
//| ProcessOP_MBRetrieveMessages; Harakiri, August 5, 2009
//o--------------------------------------------------------------
//| Handles message board requests from player
//o--------------------------------------------------------------
void Client::ProcessOP_MBRetrieveMessages(APPLAYER* pApp) {

	bool debugFlag = true;

	if(debugFlag) {
		cout << "\nMessageBoard Retrieve Message request\n";
	}

	//DumpPacketHex(pApp);

	if(pApp->size == sizeof(MBRetrieveMessages_Struct)) {
	
		MBRetrieveMessages_Struct* board = (MBRetrieveMessages_Struct*) pApp->pBuffer;		
		
		if(debugFlag) {
			printf("Debug: You clicked on the message board category %i!",board->category);			
		}		
					  
	  vector<MBMessage_Struct*> messageList;
	  
        int resultSize = 0;

		// (1) first sent out all messages for this category one at a time
        if(Database::Instance()->GetMBMessages(board->category,&messageList)) {

			for (int i=0; i<messageList.size();i++) {   
				
                        APPLAYER* outapp = new APPLAYER(OP_MBSendMessageToClient, sizeof(MBMessage_Struct));                                                                                                                                    
						memcpy(outapp->pBuffer, messageList[i], sizeof(MBMessage_Struct));  

                        QueuePacket(outapp);
						safe_delete(outapp);
                }
        }
		
		/*
			int numMessages = 100;			

			for(int i=1; i < numMessages; i++) {
				APPLAYER* outapp = new APPLAYER(OP_MBSendMessageToClient, sizeof(MBMessage_Struct));					
				

				MBMessage_Struct* message = new MBMessage_Struct;	
				memset(message, 0, sizeof(MBMessage_Struct));

				message->id=(i);			
				strcpy(message->date,"1234567890");			
				strcpy(message->author,"Harakiri");			

				char buffer [50];				
				sprintf (buffer, "Test Message %i", (i));

				strcpy(message->subject,buffer);			
				message->language=MakeRandomInt(0,23);		
				
				memcpy(outapp->pBuffer, message, sizeof(MBMessage_Struct));			

				QueuePacket(outapp);
				
			} */						
			
			// (2) notify client that all messages are sent and open/refresh message board
			APPLAYER* outappShowMB = new APPLAYER(OP_MBDataSent_OpenMessageBoard, 1);
			
			QueuePacket(outappShowMB);

			safe_delete(outappShowMB);
	} else {
		cout << "Wrong size on MBRequestMessages_Struct. Got: " << pApp->size << ", Expected: " << sizeof(MBRetrieveMessages_Struct) << endl;
	}

}

//o--------------------------------------------------------------
//| ProcessOP_MBPostMessage; Harakiri, August 5, 2009
//o--------------------------------------------------------------
//| Handles post messages requests from player
//o--------------------------------------------------------------
void Client::ProcessOP_MBPostMessage(APPLAYER* pApp) {

	bool debugFlag = true;

	if(debugFlag) {
		cout << "\nMessageBoard Post request\n";
	}

	//DumpPacketHex(pApp);

		
	MBMessage_Struct* message = (MBMessage_Struct*) pApp->pBuffer;
		
	if(debugFlag) {
		printf("You posted a message on the message board! \n");			
		printf("PostMessage: Date: %s \n",message->date);		
		printf("PostMessage: Category: %i \n",message->category);		
		printf("PostMessage: Author: %s \n",message->author);		
		printf("PostMessage: Language: %i \n",message->language);		
		printf("PostMessage: Subject: %s \n",message->subject);
		printf("PostMessage: Message: %s \n",message->message);
		printf("PostMessage: Message Size: %i \n",strlen(message->message));
	}		

	Database::Instance()->AddMBMessage(message);

	// client will automatically request message list with an OK
	APPLAYER* outappShowMB = new APPLAYER(OP_MBDataSent_OpenMessageBoard, 1);
	QueuePacket(outappShowMB);		

	safe_delete(outappShowMB);
}

//o--------------------------------------------------------------
//| ProcessOP_MBEraseMessage; Harakiri, August 5, 2009
//o--------------------------------------------------------------
//| Handles erase post requests from player
//o--------------------------------------------------------------
void Client::ProcessOP_MBEraseMessage(APPLAYER* pApp) {

	bool debugFlag = true;

	if(debugFlag) {
		cout << "\nMessageBoard Erase request\n";
	}
	
	//DumpPacketHex(pApp);		

	if(pApp->size == sizeof(MBEraseMessage_Struct)) {

		MBEraseMessage_Struct* message = (MBEraseMessage_Struct*) pApp->pBuffer;

		if(debugFlag) {			
			printf("You deleted a message from the message board! \n");			
			printf("DeleteMessage: ID: %u \n",message->id);		
			printf("PostMessage: Category: %i \n",message->category);					
		}

		
		Database::Instance()->DeleteMBMessage(message->category,message->id);
						
		// client will automatically request message list again with an OK
		APPLAYER* outappShowMB = new APPLAYER(OP_MBDataSent_OpenMessageBoard, 1);
		QueuePacket(outappShowMB);	

		safe_delete(outappShowMB);
	} else {
		cout << "Wrong size on MBEraseMessage_Struct. Got: " << pApp->size << ", Expected: " << sizeof(MBEraseMessage_Struct) << endl;
	}

}

//o--------------------------------------------------------------
//| ProcessOP_MBRetrieveMessage; Harakiri, August 5, 2009
//o--------------------------------------------------------------
//| Handles message detail requests from player, player right clicks 
//| on a message in the message list
//o--------------------------------------------------------------
void Client::ProcessOP_MBRetrieveMessage(APPLAYER* pApp) {

	bool debugFlag = true;

	if(debugFlag) {
		cout << "\nMessageBoard Retrieve Message request\n";
	}
	
	//DumpPacketHex(pApp);		

	if(pApp->size == sizeof(MBRetrieveMessage_Struct)) {


		MBRetrieveMessage_Struct* retrieveMessage = (MBRetrieveMessage_Struct*) pApp->pBuffer;
		
		if(debugFlag) {			
			printf("You retrieve a message from the message board! \n");			
			printf("RetrieveMessage: ID: %u \n",retrieveMessage->id);		
			printf("RetrieveMessage: Category: %i \n",retrieveMessage->category);					
		}
						
		
		char messageText[2048] = "";		

		// sent out the message body
		if(Database::Instance()->GetMBMessage(retrieveMessage->category,retrieveMessage->id,messageText)) {
			//  Signal Client we sent the message body, he should popup the detail window
			APPLAYER* outappShowDetail = new APPLAYER(OP_MBMessageDetail, sizeof(messageText));						
			memcpy(outappShowDetail->pBuffer, messageText, sizeof(messageText));	
			QueuePacket(outappShowDetail);			
			safe_delete(outappShowDetail);
			
		// some error? dont let the client limbo and wait forever, sent empty body
		} else {
			APPLAYER* outappShowDetail = new APPLAYER(OP_MBMessageDetail, 1);
			QueuePacket(outappShowDetail);		
			safe_delete(outappShowDetail);
		}
		
	} else {
		cout << "Wrong size on MBRetrieveMessageStruct. Got: " << pApp->size << ", Expected: " << sizeof(MBRetrieveMessage_Struct) << endl;
	}

}

//o--------------------------------------------------------------
//| ProcessOP_ClickDoor; Yeahlight, July 17, 2008
//o--------------------------------------------------------------
//| Handles door requests from player
//o--------------------------------------------------------------
void Client::ProcessOP_ClickDoor(APPLAYER* pApp)
{
	bool debugFlag = true;

	LinkedListIterator<Door_Struct*> iterator(zone->door_list);

	iterator.Reset();

	while(iterator.MoreElements())	
	{
		Door_Struct* door = iterator.GetData();
		ClickDoor_Struct* clicked = (ClickDoor_Struct*) pApp->pBuffer;

		if(door->doorid == clicked->doorid) 
		{
			//Yeahlight: If the door has not been touched in twelve seconds, then return it to the closed state
			if(time(0) - door->pLastClick >= 12)
				door->doorIsOpen = false;
			//Yeahlight: Record the time this door was touched
			door->pLastClick = time(0);
			if(debugFlag && GetDebugMe())
				Message(WHITE,"Debug: You clicked a door: #%i, parameter: %i. Door was last clicked at %i", clicked->doorid, door->parameter, door->pLastClick);
			APPLAYER* outapp = new APPLAYER(OP_OpenDoor, sizeof(DoorOpen_Struct));
			APPLAYER* outapp2 = new APPLAYER(OP_OpenDoor, sizeof(DoorOpen_Struct));
			DoorOpen_Struct* od = (DoorOpen_Struct*)outapp->pBuffer;
			//Yeahlight: Door is a trigger for another door.
			//Yeahlight: TODO: Are any triggers "locked"?
			if(door->triggerID > 0)
			{
				DoorOpen_Struct* od2 = (DoorOpen_Struct*)outapp2->pBuffer;
				od2->doorid = door->triggerID;
				od2->action = 0x02;
				entity_list.QueueClients(this, outapp2, false);
			}
			//Yeahlight: Door is locked
			if(door->keyRequired > 0 || door->lockpick > 0)
			{
				//Yeahlight: Compare required key to item on cursor
				if(clicked->keyinhand == door->keyRequired)
				{
					//Yeahlight: Door is a teleporter
					if(strcmp(door->zoneName, "NONE") != 0)
					{
						if(debugFlag && GetDebugMe())
							Message(LIGHTEN_BLUE, "Debug: You touched a locked TP object: %s", door->zoneName);
						if(strcmp(zone->GetShortName(), door->zoneName) != 0)
						{
							//Yeahlight: Locked door TPs player to different zone
							MovePC(door->zoneName, door->destX, door->destY, door->destZ);
						}
						else
						{
							//Yeahlight: Locked door TPs player to different location in same zone
							MovePC(0, door->destX, door->destY, door->destZ);
						}
					}
					//Yeahlight Door is a legit "door" and needs to be opened for players
					else
					{
						if(!door->doorIsOpen == 1)
						{
							Message(DARK_BLUE,"The door swings open!");
							od->doorid = door->doorid;
							od->action = 0x02;
							door->doorIsOpen = 1;
						}
						else
						{
							od->doorid = door->doorid;
							od->action = 0x03;
							door->doorIsOpen = 0;
						}
						entity_list.QueueClients(this, outapp, false);
					}
				//Yeahlight: Lockpick attempt
				}else if(door->lockpick > 0 && clicked->keyinhand == 13010){
					if(PickLock_timer->Check())
					{
						//Yeahlight: 10 seconds is the lockout on the "Pick Lock" skill button
						PickLock_timer->Start(10000);
						if(GetSkill(PICK_LOCK) >= door->lockpick)
						{
							if(!door->doorIsOpen == 1){
								Message(DARK_BLUE,"The door swings open!");
								od->doorid = door->doorid;
								od->action = 0x02;
								door->doorIsOpen = 1;
							}
							else
							{
								od->doorid = door->doorid;
								od->action = 0x03;
								door->doorIsOpen = 0;
							}
							entity_list.QueueClients(this, outapp, false);
							//Yeahlight: TODO: Research the range for which skilling up is possible. Set at door trivial + 15 for now
							if((GetSkill(PICK_LOCK) - door->lockpick) <= 15)
								CheckAddSkill(PICK_LOCK, 10);
						}
						else
						{
							Message(DARK_BLUE,"You were unable to pick this lock");
						}
					}
				}
				else
				{
					Message(RED, "You do not have the required key in hand to open this door");
					if(debugFlag && GetDebugMe())
					{
						Message(LIGHTEN_BLUE, "Debug: Key Required: (%i)",door->keyRequired);
						if(door->lockpick > 0)
							Message(LIGHTEN_BLUE, "Debug: This door may be picked (%i)",door->lockpick);
					}
				}
			}
			//Yeahlight: Door is a teleporter
			else if(strcmp(door->zoneName, "NONE") != 0)
			{
				if(debugFlag && GetDebugMe())
					Message(LIGHTEN_BLUE, "Debug: You touched a TP object: %s", door->zoneName);
				if(strcmp(zone->GetShortName(), door->zoneName) != 0)
				{
					//Yeahlight: Door TPs player to different zone
					MovePC(door->zoneName, door->destX, door->destY, door->destZ);
				}
				else
				{
					//Yeahlight: Door TPs player to different location in same zone
					MovePC(0, door->destX, door->destY, door->destZ);
				}
			}
			//Yeahlight: Normal door, open for all other players
			else
			{
				if(!door->doorIsOpen == 1)
				{
					od->doorid = door->doorid;
					od->action = 0x02;
					door->doorIsOpen = 1;
				}
				else
				{
					od->doorid = door->doorid;
					od->action = 0x03;
					door->doorIsOpen = 0;
				}
				entity_list.QueueClients(this, outapp, false);
			}
			safe_delete(outapp);//delete outapp;
			safe_delete(outapp2);//delete outapp2;
			break;
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

//o--------------------------------------------------------------
//| ProcessOP_DropCoin; Yeahlight, Jan 04, 2009
//o--------------------------------------------------------------
//| Handles the request to drop a coin on the ground
//o--------------------------------------------------------------
void Client::ProcessOP_DropCoin(APPLAYER* pApp)
{
	if(pApp->size == sizeof(DropCoins_Struct))
	{
		//Yeahlight: Only clear the client's coin, cursor inventories
		pp.platinum_cursor = 0;
		pp.gold_cursor = 0;
		pp.silver_cursor = 0;
		pp.copper_cursor = 0;
		this->Save();

		//Yeahlight: Add the new object to the zone's object list
		DropCoins_Struct* dc = (DropCoins_Struct*)pApp->pBuffer;
		Object* no = new Object(dc, OT_DROPPEDITEM, true);
		zone->object_list.push_back(no);
		entity_list.AddObject(no,true);
	} 
	else
	{
		cout << "Wrong size on OP_DropCoin. Got: " << pApp->size << ", Expected: " << sizeof(Object_Struct) << endl;
	}
}

/** Client attempts to drop an item on the ground. (Not for coin). ========================================*/
void Client::ProcessOP_DropItem(APPLAYER* pApp)
{
	if(pApp->size == sizeof(Object_Struct))
	{
		Object_Struct* co = (Object_Struct*)pApp->pBuffer;
		Object* no = new Object(co, OT_DROPPEDITEM, true);

		//cout << "\nItem dropped on ground.\n";
		//printf("Player heading: %i  Item heading: %i\n",this->heading, co->heading);
		//printf("BagItem[0]: %i\n\n",co->itemsinbag[0]);
		//cout << "SpawnID: " << this->id << "  Weight: " << (int)co->weight << "  Cost: " << (int)co->cost << " Click Effect: " << (int)co->click_effect << "\nStack Size: " << (int)co->stack_size << "\n";
		DumpPacketHex(pApp);

		for (int i = 0; i < 10; i++)
		{
			if (co->itemsinbag[i] != 0xFFFF)
			{
				no->SetBagItems(i,co->itemsinbag[i]);
			}
		}

		//Yeahlight: Add the new object to the zone's object list
		zone->object_list.push_back(no);
		entity_list.AddObject(no,true);
		//Clears the clients cursor.
		for (int j = 0; j != 10; j++)
		{
			pp.cursorbaginventory[j] = 0xFFFF;
		}
		pp.inventory[0] = 0xFFFF;
		this->Save();
	} 
	else
	{
		cout << "Wrong size on OP_DropItem. Got: " << pApp->size << ", Expected: " << sizeof(Object_Struct) << endl;
	}
}

/** Client attempts to pick up an item that is on the ground. ===============================================*/
void Client::ProcessOP_PickupItem(APPLAYER* pApp)
{
	DumpPacket(pApp);
	if(pApp->size == sizeof(ClickObject_Struct))
	{
		ClickObject_Struct* co = (ClickObject_Struct*)pApp->pBuffer;
		Entity* current_entity = entity_list.GetID(co->objectID);
		if(current_entity != NULL)
		{
			if(current_entity->IsObject())
			{
				Object* cur_obj = current_entity->CastToObject();
				cur_obj->dropid = co->objectID;
				//Yeahlight: Object has been picked up; remove it from the object list
				if(cur_obj->HandleClick(this,pApp))
				{
					for(int i = 0; i < zone->object_list.size(); i++)
					{
						if(zone->object_list.at(i) == cur_obj)
						{
							zone->object_list.erase(zone->object_list.begin() + i);
						}
					}
				}
			}
		}
		else
		{
			cout << "\nObject click request for DROPID (" << co->objectID << ") was not found in entity_list.\n";
		}
	} 
	else
	{
		cout << "Wrong size on OP_PickupItem. Got: " << pApp->size << ", Expected: " << sizeof(ClickObject_Struct) << endl;
	}
}


//---------------------------------------------------------------
//| ProcessOP_ConsumeItem; Tazadar, Harakiri, 10-14-2009
//---------------------------------------------------------------
//| Client sents this when something is consumed (arrow , throwing weaps, Alcohol but NOT food&drink)
//---------------------------------------------------------------
void Client::ProcessOP_ConsumeItem(APPLAYER* pApp){

	if(pApp->size == sizeof(ConsumeItem_Struct))
	{
		ConsumeItem_Struct* cas = (ConsumeItem_Struct*)pApp->pBuffer;
		DumpPacketHex(pApp);

		EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Consume Item in Slot %i received",cas->slot);

		const ItemInst* itemToConsume =  new ItemInst( GetItemAt(cas->slot), 0);

		if(itemToConsume && itemToConsume->GetItem() && itemToConsume->IsConsumable()){
			EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Consuming Item %s",itemToConsume->GetItem()->name);

			if(itemToConsume->GetItem()->common.itemType == ItemTypeAlcohol) {				
				CheckAddSkill(ALCOHOL_TOLERANCE, 0);
			}

			RemoveOneCharge(cas->slot,true);
		} else {
			EQC::Common::Log(EQCLog::Error, CP_CLIENT, "Cannot Consume Item in Slot %i, item not consumable or does not exist",cas->slot);
		}

	} 
	else
	{
		EQC::Common::Log(EQCLog::Error, CP_CLIENT, "Incorrect size on ConsumeItem_Struct. Got: %i , Expected: %i ",pApp->size,sizeof(ConsumeItem_Struct));			
	}
}


void Client::ProcessOP_ConsentRequest(APPLAYER* pApp){
	//Ignore empty consent request.
	if(sizeof(pApp->pBuffer) == 1){
		return;
	}

	ConsentRequest_Struct* crs = (ConsentRequest_Struct*)pApp->pBuffer;
	cout << crs->name;

	APPLAYER* outapp = new APPLAYER(OP_ConsentResponse, sizeof(ConsentResponse_Struct));
	ConsentResponse_Struct* conres = (ConsentResponse_Struct*)outapp->pBuffer;
	memset(conres, 0, sizeof(ConsentResponse_Struct));

	strcpy(conres->consentee, "Consentee");
	strcpy(conres->consenter, "Consenter");
	strcpy(conres->corpseZoneName, "qeynos");

	QueuePacket(pApp);
	safe_delete(outapp);//delete outapp;

}

void Client::Process_UnknownOpCode(APPLAYER* pApp)
{
	cout << "[Client] Unknown opcode: 0x" << hex << setfill('0') << setw(4) << pApp->opcode << dec;
	cout << " size:" << pApp->size << endl;
	DumpPacket(pApp->pBuffer, pApp->size);
}
/* froglok23 - 11th of 10th 2007 - end of block */

//////////////////////////////////////////////////////////////////////////////////////////

void Client::ProcessOP_Forage(APPLAYER* pApp){
	uint32 food_id = ForageItem(pp.current_zone, GetSkill(FORAGE));
	const Item_Struct* food_item = Database::Instance()->GetItem(food_id);

	if (food_item && food_item->name!=0) {
		int32 stringid=0;
		switch(food_id){
			case 13044:
				stringid=FORAGE_WATER;
				break;
			case 13106:
				stringid=FORAGE_GRUBS;
				break;
			case 13045:
			case 13046:
			case 13047:
			case 13048:
			case 13419:
				stringid=FORAGE_FOOD;
				break;
			default:
				stringid=FORAGE_NOEAT;
		}
		this->Message(EMOTE, "You forage a %s",food_item->name);
		SummonItem(food_id);
		return;
	}
	else 
		this->Message(EMOTE, "You fail to find anything to forage.");

	//See if the player increases their skill
	float wisebonus =  (pp.WIS > 200) ? 20 + ((pp.WIS - 200) * 0.05) : pp.WIS * 0.1;
	//if ((55-(GetSkill(FORAGE)*0.236))+wisebonus > (float)rand()/RAND_MAX*100)
	//	this->SetSkill(FORAGE,GetSkill(FORAGE)+1);
	CheckAddSkill(FORAGE, (int16)wisebonus);
}

///////////////////////////////////////////////////////////////////////////////////////////

void Client::ProcessOP_FeignDeath(APPLAYER* pApp)
{
	//Yeahlight: Purge client's invisibility
	CancelAllInvisibility();
	FeignDeath(GetSkill(FEIGN_DEATH));
	CheckAddSkill(FEIGN_DEATH);
}

///////////////////////////////////////////////////////////////////////////////////////////

//o--------------------------------------------------------------
//| FeignDeath; Yeahlight, Feb 27, 2009
//o--------------------------------------------------------------
//| Attempts to FD the client based on a given skill level
//o--------------------------------------------------------------
void Client::FeignDeath(int16 skillValue)
{
	int16 primfeign = skillValue;
	int16 secfeign = skillValue;
	if (primfeign > 100)
	{
		primfeign = 100;
		secfeign = secfeign - 100;
		secfeign = secfeign / 2;
	}
	else
	{
		secfeign = 0;
	}

	int16 totalfeign = primfeign + secfeign;
	if (rand()%160 > totalfeign)
	{
		SetFeigned(false);
		entity_list.MessageClose(this, false, DEFAULT_MESSAGE_RANGE, WHITE, "%s has fallen to the ground!", this->GetName());
	}
	else
	{
		SetFeigned(true);
	}

	// Moraj: Break bind wound on Feign Death
	if(bindwound_target != 0)
		BindWound(this,true);

	//Yeahlight: Check to see if this PC currently has a pet to release
	Mob* myPet = this->GetPet();
	if(myPet)
	{
		if(myPet->IsNPC())
		{
			//Yeahlight: NPC is a charmed pet, release it
			if(myPet->CastToNPC()->IsCharmed())
			{
				myPet->BuffFadeByEffect(SE_Charm);
			}
			//Yeahlight: NPC is a legit pet, start the depop timer
			else
			{
				myPet->CastToNPC()->DepopTimerStart();
			}
		}
		else if(myPet->IsClient())
		{
			//Yeahlight: PC is a charmed pet, release it
			if(myPet->CastToClient()->IsCharmed())
			{
				myPet->BuffFadeByEffect(SE_Charm);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////

void Client::ProcessOP_RequestDuel(APPLAYER* pApp){
	if(pApp->size != sizeof(Duel_Struct)){
		cout << "Wrong size on OP_RequestDuel. Got: " << pApp->size << ", Expected: " << sizeof(Duel_Struct) << endl;
		return;
	}
	Duel_Struct* ds = (Duel_Struct*) pApp->pBuffer;
	int16 duel = ds->duel_initiator;
	ds->duel_initiator = ds->duel_target;
	ds->duel_target = duel;
	Entity* entity = entity_list.GetID(ds->duel_target);
	if(GetID() != ds->duel_target && entity->IsClient() && (entity->CastToClient()->IsDueling() && entity->CastToClient()->GetDuelTarget() != 0))
	{
		Message(YELLOW,"%s has already been challenged in a duel.",entity->GetName());
		return;
	}
	if(IsDueling())
	{
		Message(YELLOW,"You have already initiated a duel.");
		return;
	}
	if(GetID() != ds->duel_target && entity->IsClient() && GetDuelTarget() == 0 && !IsDueling() && !entity->CastToClient()->IsDueling() && entity->CastToClient()->GetDuelTarget() == 0)
	{
		SetDuelTarget(ds->duel_target);
		entity->CastToClient()->SetDuelTarget(GetID());
		ds->duel_target = ds->duel_initiator;
		entity->CastToClient()->QueuePacket(pApp);
		entity->CastToClient()->SetDueling(false);
		SetDueling(false);
	}
	return;
}
///////////////////////////////////////////////////////////////////////////////////////////

void Client::ProcessOP_DuelResponse(APPLAYER* pApp){
	if(pApp->size != sizeof(DuelResponse_Struct))
		return;
	DuelResponse_Struct* ds = (DuelResponse_Struct*) pApp->pBuffer;
	//	DumpPacket(app);
	Entity* entity = entity_list.GetID(ds->target_id);
	Entity* initiator = entity_list.GetID(ds->entity_id);
	if(!entity->IsClient() || !initiator->IsClient())
		return;

	entity->CastToClient()->SetDuelTarget(0);
	entity->CastToClient()->SetDueling(false);
	initiator->CastToClient()->SetDuelTarget(0);
	initiator->CastToClient()->SetDueling(false);
	if(GetID() == initiator->GetID())
		entity->CastToClient()->Message(YELLOW,"%s has declined the duel.",initiator->GetName());
	else
		initiator->CastToClient()->Message(YELLOW,"%s has declined the duel.",entity->GetName());

	return;
}

///////////////////////////////////////////////////////////////////////////////////////////
void Client::ProcessOP_DuelResponse2(APPLAYER* pApp){
	if(pApp->size != sizeof(Duel_Struct))
		return;

	Duel_Struct* ds = (Duel_Struct*) pApp->pBuffer;
	Entity* entity = entity_list.GetID(ds->duel_target);
	Entity* initiator = entity_list.GetID(ds->duel_initiator);

	if(entity->CastToMob() == this && initiator->IsClient())
	{
		pApp->opcode = OP_RequestDuel;
		ds->duel_initiator = entity->GetID();
		ds->duel_target = entity->GetID();
		initiator->CastToClient()->QueuePacket(pApp);
		pApp->opcode = OP_DuelResponse2;
		ds->duel_initiator = initiator->GetID();
		initiator->CastToClient()->QueuePacket(pApp);
		QueuePacket(pApp);
		SetDueling(true);
		initiator->CastToClient()->SetDueling(true);
		SetDuelTarget(ds->duel_initiator);
	}

	return;
}
///////////////////////////////////////////////////////////////////////////////////////////
void Client::ProcessOP_Hide(APPLAYER *pApp)
{
	/*LaZZer: This is pretty much grabbed from the eqemu source. Tweaked a bit to fit.*/
	float fCheckHide = (GetSkill(HIDE)/300.0f) + .25f;	//Get our skill
	float fRandom = (float)rand()/RAND_MAX;
	/*Check to see if we gained a skill point. This returns a boolean. I'm not sure if we need 
	to catch it and inform the player of their skill increase or not.*/
	CheckAddSkill(HIDE);
	if(fRandom < fCheckHide) //Was the hide a sucess?
	{
		this->SetHide(true);
	}
	if(GetClass() == ROGUE)//Check for rouge. I belive this has to do with their hide/sneak combo.
	{
		if(GetHide())
		{
			APPLAYER *pOutApp = new APPLAYER(0x0202,12);
			uint8 rawData0[12] = { 0x5A, 0x01, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			memcpy(pOutApp->pBuffer,rawData0,12);
			QueuePacket(pOutApp);
			safe_delete(pOutApp);
		}
		else
		{
			APPLAYER *pOutApp = new APPLAYER(0x0202,12);
			uint8 rawData0[12] = { 0x59, 0x01, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			memcpy(pOutApp->pBuffer,rawData0,12);
			QueuePacket(pOutApp);
			safe_delete(pOutApp);
		}
	}
}

//o--------------------------------------------------------------
//| ProcessOP_Sneak; Yeahlight, April 16, 2008
//o--------------------------------------------------------------
//| Sneak ability
//o--------------------------------------------------------------
void Client::ProcessOP_Sneak(APPLAYER* pApp){

	//Yeahlight: TODO: Using sneak and hide at the same time has a moderate chance to
	//                 break sneak perminately for the character; fix this!

	bool debugFlag = false;

	bool wasPreviouslySneaking = GetSneaking();
	int sneakChance = ((float)((float)GetSkill(SNEAK)/300.00 + 0.25)) * 100;
	int sneakRandom = (rand()%100);

	if (wasPreviouslySneaking) 
	{
		this->SetSneaking(false);
	}
	if(!wasPreviouslySneaking)
	{
		if(sneakRandom < sneakChance) 
		{
			Message(DARK_BLUE, "You are as quiet as a cat stalking its prey.");  
			this->SetSneaking(true);
		}
		else
		{
			Message(DARK_BLUE, "You are as quiet as a herd of stampeding elephants."); 
		}
	}

	CheckAddSkill(SNEAK);

	//Yeahlight: Why is this block of code located outside
	//           of a successful sneak attempt? Taken from EQEMU
	//           and this was how it was written. Shouldn't
	//           there be a 'return;' in the above 'else' statement?
	this->SendAppearancePacket(this->GetID(), SAT_Sneaking, sneaking, true);

	APPLAYER* outapp;

	if(GetClass() == ROGUE)
	{
		if(GetSneaking())
		{
			outapp = new APPLAYER(0x0202,12);
			uint8 rawData0[12] = { 0x5B, 0x01, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			memcpy(outapp->pBuffer,rawData0,12);
			QueuePacket(outapp);
			safe_delete(outapp);
		}
		else 
		{
			outapp = new APPLAYER(0x0202,12);
			uint8 rawData0[12] = { 0x5C, 0x01, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			memcpy(outapp->pBuffer,rawData0,12);
			QueuePacket(outapp);
			safe_delete(outapp);
		}
	}

	if (debugFlag && GetDebugMe())
		cout<<"Debug: Chance to sneak: "<<sneakChance<<", Random to pass: "<<sneakRandom<<", Currently sneaking: "<<GetSneaking()<<endl;
}

//////////////////////////////////////////////////////////////////////////////////////////

//o--------------------------------------------------------------
//| ProcessOP_BackSlashTarget; Yeahlight, April 23, 2008
//o--------------------------------------------------------------
//| Returns a target to a player when they issue the /target
//| command.
//o--------------------------------------------------------------
void Client::ProcessOP_BackSlashTarget(APPLAYER* pApp){

	if (pApp->size != sizeof(BackSlashTarget_Struct)) 
	{
		cout << "Wrong size: OP_BackSlashTarget, size=" << pApp->size << ", expected " << sizeof(BackSlashTarget_Struct) << endl;
		return;
	}
	BackSlashTarget_Struct* bst = (BackSlashTarget_Struct*)pApp->pBuffer;
	ClientSideTarget = bst->bst_target;			//Yeahlight: Not sure why this is necessary; could be useful later on?
	target = entity_list.GetMob(bst->bst_target);
	if (target && DistNoRoot(target) <= TARGETING_RANGE*TARGETING_RANGE) 
	{
		QueuePacket(pApp);
		//APPLAYER hp_app;						//Yeahlight: /target apparently works just fine w/out these four lines.
		//target->IsTargeted(true);				//			 Somewhere in these three lines is a bug that causes the zone
		//target->CreateHPPacket(&hp_app);		//			 to crash when /target'ing with an empty target box. /boggle
		//QueuePacket(&hp_app, false);
	}
	else
	{
		Message(RED,"You don't see anyone around here by that name.");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////

//o--------------------------------------------------------------
//| ProcessOP_AssistTarget; Yeahlight, April 23, 2008
//o--------------------------------------------------------------
//| Returns a target to a player when they issue the /assist
//| command.
//o--------------------------------------------------------------
void Client::ProcessOP_AssistTarget(APPLAYER* pApp){

	bool debugFlag = false;

	if (pApp->size != sizeof(BackSlashAssist_Struct)) 
	{
		cout << "Wrong size: BackSlashAssist_Struct, size=" << pApp->size << ", expected " << sizeof(BackSlashAssist_Struct) << endl;
		return;
	}

	int16 entid = 0;
	BackSlashAssist_Struct* bsa = (BackSlashAssist_Struct*)pApp->pBuffer;

	Entity* entity = entity_list.GetID(bsa->bsa_target);
	if(entity->CastToMob()->GetTarget() != 0) {
		if(DistNoRoot(entity->CastToMob()->GetTarget()) <= TARGETING_RANGE*TARGETING_RANGE)
			entid = entity->CastToMob()->GetTarget()->GetID();
		else
			Message(RED,"You don't see anyone around here by that name.");
	}
	else
	{
		//Yeahlight: Is there a message for assisting an entity with an empty target box?
	}

	bsa->bsa_target = entid;
	QueuePacket(pApp);

	if (debugFlag && GetDebugMe())
		cout<<"EntID: "<<entid<<endl;
}

/********************************************************************
*                        Wizzel - 8/11/08		                    *
*********************************************************************
*                       Class Training			                    *
*********************************************************************
*  + Brings up the training window and allows player to train and	*
*	  learn skills and languages.									*
*********************************************************************
*  Todo: Figure out how to get money costs involved. Also, add		*
*  'highest_level' in the character table to prevent class points	*
*  exploiting. Also, make sure to add class points upon leveling.	*
********************************************************************/

void Client::ProcessOP_ClassTraining(APPLAYER* pApp)
{
	//Yeahlight: GMs ignore invis PCs
	if(this->GetInvisible())
		return;

	if (pApp->size == sizeof(ClassTrain_Struct))
	{

		ClassTrain_Struct* classtrain = (ClassTrain_Struct*) pApp->pBuffer;


		// --- Class Training Documentation ---
		// 08/11/08 - Wizzel
		// Due to the testing I have done related to this, I am positive
		// that the highesttrain bytes are referenced for the highest
		// amount a class trainer can teach you for each specific skill.
		// It is 100 for now, but I can tweak it if we find that players
		// were not able to train say 1 Hand Slash above 75. Also, if no
		// number is sent, that skill does not appear. Also,if you send
		// a number to a skill a player does not have, nothing happens. 

		//Yeahlight: I updated this to use the max skill for the class,
		//           race, level combination.

		for(int train = 0; train < 74; train++) // Send all skills value of MAX_TRAIN_SKILL.
		{
			classtrain->highesttrain[train] = CheckMaxSkill(train, GetRace(), GetClass(), GetLevel(), true);
		}


		// Send a mock greeting message
		// TODO: More messages, I think there are 2 possible
		Entity * trainer = entity_list.GetID(classtrain->npcid);
		if(trainer)
			Message(BLACK, "%s says, 'Hello %s, up for some training?'", trainer->GetName(), name);


		// Harakiri: All trainiers can train to max 100
		for(int i=0; i<=LANGUAGE_DARK_SPEECH;i++) {
			classtrain->highesttrainLang[i] = 100;
		}

	
		// Harakiri: You can verify the highesttrainLang struct with modifying
		// a language to train i.e BARB can only train to max 5 skill points
		// classtrain->highesttrainLang[LANGUAGE_BARBARIAN] = 0x05;		

		// Harakiri: reset client packet information to 0
		for(int i=0; i < sizeof(classtrain->unknown2); i++) {
			classtrain->unknown2[i] = 0;
		}
		
		// Harakiri: one of these bits are important to show the train dialog
		for(int i=0; i < sizeof(classtrain->unknown); i++) {
			classtrain->unknown[i] = 1;
		}
		
		QueuePacket(pApp);
	}
	else
	{
		cout << "Wrong size on OP_ClassTraining. Got: " << pApp->size << ", Expected: " << sizeof(ClassTrain_Struct) << endl;
	}
}

///////////////////////////////////////////////////////////////////////////

void Client::ProcessOP_ClassEndTraining(APPLAYER* pApp){

	ClassTrain_Struct* classtrain = (ClassTrain_Struct*) pApp->pBuffer;

	// Send a mock parting message
	// TODO: More messages, I think there are 2 possible
	Entity * trainer = entity_list.GetID(classtrain->npcid);
	if(trainer)
		Message(BLACK, "%s says, 'Good to see you are working on your skills, %s'", trainer->CastToNPC()->GetName(),this->GetName());
	return;
}

///////////////////////////////////////////////////////////////////////////

void Client::ProcessOP_ClassTrainSkill(APPLAYER* pApp)
{
	DumpPacketHex(pApp);
	
	ClassSkillChange_Struct* gmskill = (ClassSkillChange_Struct*) pApp->pBuffer;

	// train a regular skill
	if(gmskill->skill_type == 0) {
		int16 skillLevel = GetSkill(gmskill->skill_id);
		//Yeahlight: Skill is untrained, so set the first value at level - 1
		if(skillLevel == 254) {
			skillLevel = GetLevel() - 1;
		}
		SetSkill(gmskill->skill_id, skillLevel + 1);
	// Harakiri - train a language
	} else if(gmskill->skill_type == 1) {
		int16 skillLevel = GetLanguageSkill(gmskill->skill_id);
		
		if(skillLevel == 254) {
			skillLevel = GetLevel() - 1;
		}

		SetLanguageSkill(gmskill->skill_id, skillLevel + 1);
	}
	pp.trainingpoints--;
	this->Save();
	return;
}


///////////////////////////////////////////////////////////////////////////

void Client::Process_ClientConnection5(APPLAYER *app)
{
	if (app->opcode == 0xd820)
	{
		EQC::Common::PrintF(CP_CLIENT, "Enterzone complete (Login 5)\n");

		//this->SendAppearancePacket(this->GetID(), 0x10, this->GetID(), false);
		this->SendAppearancePacket(0, SAT_Camp, this->GetID(), false);
		// With the code below, see how spawn_id of the spawnapperance struct is not being set? should this be as we are basicly into the zone?
		// i updated the new code above to include the GetID() for the spawn ID. - Dark-Prince - 01/06/2008

		//APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
		//memset(outapp->pBuffer, 0, outapp->size);
		//SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)outapp->pBuffer;
		//sa->type = 0x10;			// Is 0x10 used to set the player id?
		//sa->parameter = GetID();	// Four bytes for this parameter...

		APPLAYER* outapp = new APPLAYER;
		// Inform the world about the client
		CreateSpawnPacket(outapp);
		entity_list.QueueClients(this, outapp, false);
		safe_delete(outapp);//delete outapp;

		outapp = new APPLAYER(0xc321,8);	// Unknown
		memset(outapp->pBuffer, 0, 8);
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;

		outapp = new APPLAYER(0xd820,0);	// Unknown
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;



		// We are now fully connected and ready to play					
		client_state = CLIENT_CONNECTED;

		/*outapp = new APPLAYER(OP_Stamina, sizeof(Stamina_Struct));
		Stamina_Struct* sta = (Stamina_Struct*)outapp->pBuffer;
		sta->food = 0;
		sta->water = 6000;
		sta->fatigue = 50;
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;*/

		this->Handle_Connect5Guild();

		// Quagmire - Setting GM flag
		if (this->admin >= 80)
		{
			//PP GM flag doesn't work, setting it manually here.
			if(this->pp.gm == 1)
			{
				this->SendAppearancePacket(this->GetID(), SAT_GM_Tag, 1, true);
			}

			this->DeletePetitionsFromClient();
		}


		this->position_timer->Start();
		this->SetDuelTarget(0);
		this->SetDueling(false);
		this->UpdateWho(false);

		this->isZoning = false;
		this->isZoningZP = false;
		this->usingSoftCodedZoneLine = false;

		for(int i=0;i<15;i++)//Tazadar : this is to remove spells with timer<0 , it happens during long loading zone (i.e your timer goes under 0 while you are zoning and your client does not sent the buff fade request...)
		{
			//cout<<"duration of the buff number "<< (int)i << " = " << (int)pp.buffs[i].duration <<" level of the caster = "<<(int)pp.buffs[i].level <<" visable = " << (int)pp.buffs[i].visable <<" spell id = " <<(int)pp.buffs[i].spellid <<endl;
			//cout<<"duration of the buff number one on the serv = " << (int)this->buffs[i].ticsremaining<<endl;
			if(pp.buffs[i].spellid != 0 && pp.buffs[i].spellid != 0xFFFF)
			{
				Spell* spell = spells_handler.GetSpellPtr(pp.buffs[i].spellid);
				//Yeahlight: Spell is about to fade or the spell is a levitation spell and we are zoning into a zone that has lev restrictions
				if((int)pp.buffs[i].duration <= 0 || (spell && spell->IsLevitationSpell() && zone->GetLevCondition() == 0))
				{
					if(spell)
					{
						this->BuffFade(spell);
					}
				}
			}
		}

		//Yeahlight: Remove specific effects upon zoning
		BuffFadeByEffect(SE_Charm);
		BuffFadeByEffect(SE_Illusion);
		BuffFadeByEffect(SE_Fear);
		BuffFadeByEffect(SE_Stun);
		BuffFadeByEffect(SE_Mez);
		BuffFadeByEffect(SE_BindSight);
		BuffFadeByEffect(SE_VoiceGraft);
		BuffFadeByEffect(SE_Sentinel);
		BuffFadeByEffect(SE_ModelSize);
		BuffFadeByEffect(SE_AddFaction);
		BuffFadeByEffect(SE_SpinTarget);

		//Yeahlight: Call the AC function to load the seperate AC variables
		CalculateACBonuses();

		//Yeahlight: Assign a size for the PC
		size = GetDefaultSize();

		//Yeahlight: Set current HP and mana
		cur_hp = pp.cur_hp;
		cur_mana = pp.mana;
		SendHPUpdate();
		SendManaUpdatePacket();
	}
	else if(app->opcode == 0x4721)
	{
		QueuePacket(app);//Cofruben
	}
	//Recycle packet until client is connected...
	else if (app->opcode == OP_WearChange)
	{
		//QueuePacket(app);
	}
	else if (app->opcode == OP_SpawnAppearance)
	{
		QueuePacket(app);
	}
	else 
	{
		cout << "Unexpected packet during CLIENT_CONNECTING5: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
		DumpPacket(app);
	}
}


void Client::Process_ClientDisconnected(APPLAYER *app)
{
	EQC::Common::PrintF(CP_CLIENT, "Client disconnected : %s\n", this->GetName());
	PMClose();
	//QueuePacket(0);
}

void Client::Process_ClientKicked(APPLAYER *app)
{
	EQC::Common::PrintF(CP_CLIENT, "Client Kicked : %s\n", this->GetName());
	PMClose();
	//QueuePacket(0);
}

void Client::Process_ShoppingEnd(APPLAYER *app)
{
	APPLAYER* outapp = new APPLAYER(OP_ShopEndConfirm, 2);
	outapp->pBuffer[0] = 0x0a;
	outapp->pBuffer[1] = 0x66;
	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
}

void Client::Process_Jump(APPLAYER *app)
{
	// Tazadar:Jumping effects stamina
	this->pp.fatigue+=10;
	// Cofruben: trick to send the jump animation to other players.
	this->DoAnim(20);
}

void Client::Process_SafePoints(APPLAYER *app)
{
	// This is handled clientside (But.. should we be updating the database with this? - Dark-Prince 03/06/2008?)
	cout << name << " moved to safe point." << endl;
}

void Client::Process_SenseTraps(APPLAYER *app)
{
	ChannelMessageSend(0,0,7,0,"Sense Traps not implemented...yet");
	//Size: 0
	CheckAddSkill(SENSE_TRAPS);
}

void Client::Process_DisarmTraps(APPLAYER *app)
{
	ChannelMessageSend(0,0,7,0,"Disable Traps not implemented...yet");
	//Size: 0
	CheckAddSkill(DISARM_TRAPS);
}


void Client::ProcessOP_SafeFallSuccess(APPLAYER* pApp)
{
	// --- Safe Fall Documentation ---
	// 08/11/08 - Wizzel
	// The opcode is recieved when a player who has learned safe fall
	// absorbs some damage from falling. It is not sent if no damage
	// is absorbed. The packet size is 0 and the client handles it 
	// correctly without any extra needed code.

	// Debug messages for testing. Remove after alpha.
	cout << "Player " << this->GetName() << " has absorbed damage from safe fall." << endl;
	if(GetDebugMe())
		Message(BLACK, "Debug: You have absorbed damage from safe fall.");

	// Check to see if skill up is achieved.
	CheckAddSkill(SAFE_FALL);
}

void Client::Process_Track(APPLAYER* app)
{
	//FileDumpPacket("Process_Track.txt", app);
	//Yeahlight: This is mysteriously working correctly now (for the most part, still need to remove triggers and traps)
	//ChannelMessageSend(0,0,7,0,"Track still allows player to track every mob in the zone.");
	CheckAddSkill(TRACKING);
}

/********************************************************************
*                        Tazadar  - 12/08/08                        *
*********************************************************************
*                          Process_Action				            *
*********************************************************************
*  + Manages drowning/burning/falling	                            *
*                                                                   *
*********************************************************************/
void Client::Process_Action(APPLAYER* app)
{
	//Yeahlight: TODO: Do these break sneak? Falls would seem to break it....
	if (app->size == sizeof(Action_Struct))
	{
		Action_Struct *s=(Action_Struct*)app->pBuffer;
		if(s->type == 0xFA){
			cout <<"[Client]"<< name << " is burning" << endl;
			this->SetHP(this->GetHP() - s->damage);
		}
		else if(s->type == 0xFB){
			cout <<"[Client]"<< name << " is drowning" << endl;
			this->SetHP(this->GetHP() - s->damage);
		}
		else if(s->type == 0xFC){
			cout <<"[Client]"<< name << " is getting falling damage" << endl;
			this->SetHP(this->GetHP() - s->damage);
		}
		else{
			cout <<"[Client]"<< name << " is trying to do an unknown Process_Action " << endl;
			DumpPacket(app);
		}
	}
	else
	{
		cout << "Wrong size on OP_Action. Got: " << app->size << ", Expected: " << sizeof(Action_Struct) << endl;
	}
}
/********************************************************************
*                        Tazadar  - 8/29/08                        *
********************************************************************
*                          Process_FoodDrinkConsume                *
********************************************************************
*  + Manages the food/drink consumption                            *
*  Harakiri 10-14-2009 - cleaned up the method                     *
********************************************************************/
void Client::Process_ConsumeFoodDrink(APPLAYER* app)
{	
	if (app->size == sizeof(Consume_Struct))
	{
		
		Consume_Struct* cas = (Consume_Struct*) app->pBuffer;
		
		EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Consume Food/Drink Item in Slot %i received",cas->slot);
				
		const ItemInst* itemToConsume =  new ItemInst( GetItemAt(cas->slot), 0);
		
		if(itemToConsume && itemToConsume->GetItem() && itemToConsume->IsConsumable()){

			uint32 foodDuration = itemToConsume->GetItem()->common.casttime;

			EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Consuming Food/Drink Item %s",itemToConsume->GetItem()->name);

			
			if(cas->Type == CONSUMEFOOD)
			{
				// Harakiri, the client has this formula at 0046AB71 (rightclickitem):
				// *((_DWORD *)playerProfile + 702) += 100 * *((_DWORD *)foodItem + 58) / 2;
				// i.e. hungerlevel+= 100 * (item.castime/2) so the below one is correct
				pp.hungerlevel+=50*foodDuration;//Tazadar: Seems to be the correct formula
			}
			else if(cas->Type == CONSUMEDRINK)
			{
				pp.thirstlevel+=50*foodDuration;//Tazadar: Seems to be the correct formula
			}
			else
			{
				EQC::Common::Log(EQCLog::Error, CP_CLIENT, "Unknown Food/Drink Item %i",cas->Type );
			}

			RemoveOneCharge(cas->slot,true);

			//Tazadar : We send new infos to the client.			
			//Harakiri, this is actually not needed because the client already does this locally, i leave it in tho
			SendStaminaUpdate(pp.hungerlevel, pp.thirstlevel, pp.fatigue);
		}
		else
		{
			EQC::Common::Log(EQCLog::Error, CP_CLIENT, "Cannot Consume Food/Drink Item in Slot %i, item not consumable or does not exist",cas->slot);
		}
	}
	else
	{
		EQC::Common::Log(EQCLog::Error, CP_CLIENT, "Incorrect size on Consume_Struct. Got: %i , Expected: %i ",app->size,sizeof(Consume_Struct));
	}
}

// Added metyhod to contina packet sending only - Dark-Prince
void Client::SendStaminaUpdate(int32 hungerlevel, int32 thirstlevel, int8 fatigue)
{
	APPLAYER* outapp = new APPLAYER(OP_Stamina, sizeof(Stamina_Struct));
	Stamina_Struct* sta = (Stamina_Struct*)outapp->pBuffer;

	// Should these have validation checks? (i.e min & max levels) - Dark-Prince
	sta->food = hungerlevel;
	sta->water = thirstlevel;
	sta->fatigue = fatigue;

	QueuePacket(outapp);
	safe_delete(outapp);
}

/******************************************************************
*                 Author?   : Process_ClientConnection1           *
*                 Date?     : ?                                   *
*******************************************************************
*																  *
*                                                                 *
*******************************************************************/
void Client::Process_ClientConnection1(APPLAYER *app)
{
	if (app->opcode == OP_SetDataRate)
	{
		EQC::Common::PrintF(CP_CLIENT, "Login packet 1\n");

		// Cofruben.
		EQC::Common::PrintF(CP_CLIENT, "Starting Debugging System for %s...\n", name);
		this->mpDbg = new DebugSystem(this);

		this->client_state = CLIENT_CONNECTING2;
	}
	else
	{
		cout << "Unexpected packet during CLIENT_CONNECTING1: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
		DumpPacket(app);
	}
}

// Quagmire - Antighost code
// Dark-Prince - Move it into its own method, formatted and commented it
bool Client::QuagmireGhostCheck(APPLAYER *app)
{
	// tmp var is so the search doesnt find this object
	char tmp[32];
	tmp[31] = 0;

	// Copy out 16 bytes
	strncpy(tmp, (char*)&app->pBuffer[4], 16);


	// Copy tmp to this->name
	strcpy(name, tmp);

	// Get the Authentication for 'name'
	account_id = Database::Instance()->GetAuthentication(name, zone->GetShortName(), ip);

	if (account_id == 0) // Is the account_id 0? if it is, drop the client as they are not authenticated
	{
		EQC::Common::PrintF(CP_CLIENT, "Client dropped: GetAuthentication = 0, n=%s\n", tmp);

		return false; // TODO: Can we tell the client to get lost in a good way

	}

	// Search for the name
	Client* client = entity_list.GetClientByName(tmp);
	// If we found the client, kick it
	if (client != 0)
	{
		client->Kick();  // Bye Bye ghost
	}

	return true;
}




/******************************************************************
*                 Author?   : Process_ClientConnection2           *
*                 Date?     : ?                                   *
*******************************************************************
*																  *
*                                                                 *
*******************************************************************/
void Client::Process_ClientConnection2(APPLAYER *app)
{

	EQC::Common::PrintF(CP_CLIENT, "Login packet 2\n");

	admin = Database::Instance()->CheckStatus(account_id);
	Database::Instance()->GetAccountName(account_id, account_name);
	Database::Instance()->GetCharacterInfo(name, &character_id, &guilddbid, &guildrank);
	Database::Instance()->LoadFactionValues(character_id, &factionvalue_list);

	memset(&pp, 0, sizeof(PlayerProfile_Struct));

	long pplen=Database::Instance()->GetPlayerProfile(account_id, name, &pp);

	if(pplen==0)
	{
		EQC::Common::PrintF(CP_CLIENT, "Client dropped: !GetPlayerProfile, name=%s\n", name);
		this->Kick();
	}

	EQC::Common::PrintF(CP_CLIENT, "Loaded playerprofile for %s\n", name);

	// Try to find the EQ ID for the guild, if doesnt exist, guild has been deleted.
	// Clear memory, but leave it in the DB (no reason not to, guild might be restored?)
	guildeqid = Database::Instance()->GetGuildEQID(guilddbid);

	if (guildeqid == 0xFFFFFFFF)
	{
		guilddbid = 0;
		guildrank = NotInaGuild;
		pp.guildid = 0xFFFF;
	}
	else
	{
		pp.guildid = guildeqid;
	}

	strcpy(name, pp.name);
	strcpy(Surname, pp.Surname); 
	x_pos = pp.x;
	y_pos = pp.y;
	z_pos = pp.z;
	heading = pp.heading * 2;
	race = pp.race;
	base_race = pp.race;
	class_ = pp.class_;
	gender = pp.gender;
	base_gender = pp.gender;
	level = pp.level;
	deity = DEITY_AGNOSTIC;

	for(int i=0; i< 24; i++)  
		pp.unknown3888[i] = 0;  	

	// Harakiri need to clear these, or the client things he got some items already as pointers
	for(int i=0; i< 8; i++)  
		pp.bankinvitemPointers[i] = 0;  	
	// Harakiri need to clear these, or the client things he got some items already as pointers
	for(int i=0; i< 30; i++)  
		pp.inventoryitemPointers[i] = 0;  
	
	for(int i=0; i< 12; i++)  
		pp.unknown3944[i] = 0;  
	
	pp.unknown4178 =  0;
	pp.unknown4179 =  0;
	
	// Harakiri for each spell slot in ms the refresh time left
	//pp.spellSlotRefresh[0] = 10000;
	//pp.spellSlotRefresh[1] = 10000;
	//pp.spellSlotRefresh[2] = 10000;
	//pp.spellSlotRefresh[3] = 10000;
	//pp.spellSlotRefresh[4] = 10000;
	//pp.spellSlotRefresh[5] = 10000;
	//pp.spellSlotRefresh[6] = 10000;
	//pp.spellSlotRefresh[7] = 10000;

	// Harakiri discpline always available
	pp.discplineAvailable = 1;

	// Harakiri SK and Paladin, ms left for deathtouch/LOH
	pp.abilityCooldown = 0;
	
	for(int i=0; i < 4; i++) {
		pp.unknown4180[i] = 0;
	}

	
	memset(pp.GroupMembers, 0, sizeof(pp.GroupMembers));  
	for(int i=0; i<3592; i++)  
		pp.unknown4532[i] = 0;

	this->LoadSpells();

	strcpy(pp.current_zone, zone->GetFileName());

	int GID = Database::Instance()->GetGroupID(this->GetName());
	if(GID > 0) {
		Group* g = entity_list.GetGroupByID(GID);
		if(!g) {	//nobody from our group is here... start a new group
			g = new Group(GID);
		}
		if (g)
			g->MemberZonedIn(this);
		else
			Database::Instance()->SetGroupID(this->GetName(), 0);
	}
	else
		memset(this->GetPlayerProfilePtr()->GroupMembers, 0, sizeof(this->GetPlayerProfilePtr()->GroupMembers));

	SetEQChecksum((unsigned char*)&pp, sizeof(PlayerProfile_Struct)); 
	APPLAYER *outapp;
	outapp = new APPLAYER;				
	outapp->opcode = OP_PlayerProfile;		
	outapp->pBuffer = new uchar[10000];
	outapp->size = DeflatePacket((unsigned char*)&pp, sizeof(PlayerProfile_Struct), outapp->pBuffer, 10000);
	EncryptProfilePacket(outapp);
	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
	strcpy(pp.current_zone, zone->GetShortName());

	outapp = new APPLAYER(OP_ZoneEntry, sizeof(ServerZoneEntry_Struct));
	ServerZoneEntry_Struct *sze = (ServerZoneEntry_Struct*)outapp->pBuffer;
	memset(sze, 0, sizeof(ServerZoneEntry_Struct));
	strcpy(sze->name, name);
	strcpy(sze->Surname, Surname);
	strcpy(sze->zone, zone->GetShortName());
	sze->class_ = pp.class_;
	sze->race = pp.race;
	sze->gender = pp.gender;
	sze->level = pp.level;
	sze->x = pp.x;
	sze->y = pp.y;
	sze->z = pp.z;
	sze->heading = pp.heading * 2;
	sze->walkspeed = walkspeed;
	sze->runspeed = runspeed;			
	sze->anon = pp.anon;

	// Disgrace: proper face selection
	sze->face = GetFace();

	// Quagmire - added coder?'s code from messageboard
	Item_Struct* item = Database::Instance()->GetItem(pp.inventory[2]);
	if (item)
	{
		sze->helmet = item->common.material;
		sze->helmcolor = item->common.color;
	}
	sze->npc_armor_graphic = 0xff;  // tell client to display PC's gear

	// Quagmire - Found guild id =)
	// havent found the rank yet tho
	sze->guildeqid = (int16) guildeqid;

	//Grass clipping radius.
	//					sze->skyradius=500;
	//*shrug*
	//			sze->unknown304[0]=255;

	//This might be the fRadius bug... Setting it to 500.

	//sze->unknownfloat2 = 0x0000000F;
	SetEQChecksum((unsigned char*)sze, sizeof(ServerZoneEntry_Struct));
	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;

	outapp = new APPLAYER(OP_Weather, 8);
	if (zone->zone_weather == 1)
	{
		outapp->pBuffer[6] = 0x31; // Rain
	}
	if (zone->zone_weather == 2)
	{
		outapp->pBuffer[0] = 0x01;
		outapp->pBuffer[4] = 0x02; 
	}
	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
	//cout << "Sent addplayer... (EQ disconnects here on error)" << endl;

	SetAttackTimer();

	//Yeahlight: Last stage of account login process to the active_accounts table
	Database::Instance()->LogAccountInPartIV(this->account_id, this->ip);

	client_state = CLIENT_CONNECTING3;

}

/******************************************************************
*                 Author?   : Process_ClientConnection3           *
*                 Date?     : ?                                   *
*******************************************************************
*																  *
*                                                                 *
*******************************************************************/
void Client::Process_ClientConnection3(APPLAYER *app)
{
	if (app->opcode == 0x5d20)
	{
		EQC::Common::PrintF(CP_CLIENT, "Login packet 3\n"); // Here the client sends tha character name again.. 
		// not sure why, nothing else in this packet
		SendInventoryItems();

		weapon1 = Database::Instance()->GetItem(pp.inventory[13]);
		weapon2 = Database::Instance()->GetItem(pp.inventory[14]);

		// Pinedepain // Once we first have weapon1 and weapon2, we update the instrument mod (if we're a bard)
		if(IsBard())
		{
			UpdateInstrumentMod();
		}

		//Yeahlight: Now that we have our gear, let's calculate the max hp/mana
		CalcBonuses(true, true);
		CalcMaxHP();
		CalcMaxMana();

		client_state = CLIENT_CONNECTING4;
	}
	//Recycle packet until client is connected...
	else if (app->opcode == OP_WearChange)
	{
		QueuePacket(app);
	}
	else {
		cout << "Unexpected packet during CLIENT_CONNECTING3: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
		DumpPacket(app);
	}
}

void Client::SendNewZone(NewZone_Struct newzone_data)
{
	APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
	memcpy(outapp->pBuffer, &newzone_data, outapp->size);
	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;				
}

/******************************************************************
*                 Author?   : Process_ClientConnection4           *
*                 Date?     : ?                                   *
*******************************************************************
*																  *
*                                                                 *
*******************************************************************/
void Client::Process_ClientConnection4(APPLAYER *app)
{
	if (app->opcode == 0x0a20)
	{
		EQC::Common::PrintF(CP_CLIENT, "Zhdr request (Login 4)\n"); // This packet was empty...

		/*
		APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
		memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;				
		*/
		this->SendNewZone(zone->newzone_data);

		APPLAYER* outapp = new APPLAYER;
		outapp->opcode = 0xd820; // Unknown
		outapp->size = 0;
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;

		//Tazadar: Send Bulk Spawns
		entity_list.SendZoneSpawnsBulk(this);

		// We'll Just send the doors right here, seems like as good as place as any.  We really should
		// evaluate if this is a good spot or not, but it should work for now. - neo
		this->Handle_Connect5GDoors();
		this->Handle_Connect5Objects();

		client_state = CLIENT_CONNECTING5;
	}
	else if(app->opcode == 0x4721)
	{
		QueuePacket(app);//Cofruben
	}
	//else if (app->opcode == 0xf520) {
	//	QueuePacket(app);
	//}

	//Recycle packet until client is connected...
	else if (app->opcode == OP_WearChange)
	{
		QueuePacket(app);
	}
	else
	{
		cout << "Unexpected packet during CLIENT_CONNECTING4: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
		DumpPacket(app);
	}
}

void Client::ProcessOP_Default(APPLAYER* app){
	switch(app->opcode){	
		case OP_Illusion:
			{
				DumpPacketHex(app); //hex workaround testing -Wizzel
				break;
			}
		case OP_GMBecomeNPC:
			{
				this->ProcessOP_GMBecomeNPC(app);
				break;
			}

		case OP_MovementUpdate: // DB - Noticed this was going unknown opcode between run/walk
			{
				break;
			}

		case OP_ENVDAMAGE2:
			{
				//DumpPacket(app);
				//cout << "OP_ENVDAMAGE2 size: " << app->size << endl;
				entity_list.QueueClients(this, app, false);
			}
			break;
		case OP_CraftingStation:
			{
				DumpPacket(app);
				this->ProcessOP_CraftingStation();
				break;
			}
		case OP_RezzRequest:
			{
				this->ProcessOP_RezzRequest(app);
				break;
			}
		case OP_RezzAnswer:
			{
				this->ProcessOP_RezzAnswer(app);
				break;
			}
		case 0x4121:
			{
				QueuePacket(app); // console says shoudl be size 2
				break;
			}		
		case OP_AutoAttack2: // Why 2
			{
				break;
			}
		
		case OP_GMZoneRequest2: 
			{
				if (admin < 100 && !strcasecmp((char*) app->pBuffer, "gmhome"))
				{
					Message(BLACK, "GMHome is for GMs only");
				}
				else
				{
					this->MovePC((char*) app->pBuffer, -1, -1, -1);
				}
				break;
			}
								
		case OP_GuildMOTD:
		case OP_GuildInvite:
		case OP_GuildInviteAccept:
		case OP_GuildRemove:
		case OP_GuildPeace:
		case OP_GuildWar:

		case OP_GuildLeader: 
			{
				zgm.Process(app, this);
				break;
			}
		
		case OP_Medding:
			{
				if (app->pBuffer[0])
					medding = true;
				else
					medding = false;
				break;
			}
			//heko: replaced the MonkAtk, renamed Monk_Attack_Struct
		case OP_CombatAbility:
			{
				this->ProcessOP_CombatAbility(app);
				break;
			}
			//heko: 
			//Packet length 4
			//0000: 00 00		NPC ID
		
		case OP_SenseHeading:
			{
				// Nothing needs to be done here AFAIK. Client handles sense heading correctly. - Wizzel
				CheckAddSkill(SENSE_HEADING);
				break;
			}
		case OP_GMGoto:
			{
				this->ProcessOP_GMGoto(app);
				break;
			}
			/*
			0 is e7 from 01 to
			1 is 03
			2 is 00
			3 is 00
			4 is ??
			5 is ??
			6 is 00 from a0 to
			7 is 00 from 3f to */			
		
		case OP_GroupDelete:					//Only comes when a group leader disbands himself;
			{
				if (this->IsGrouped() && entity_list.GetGroupByClient(this) != 0){
					entity_list.GetGroupByClient(this)->DisbandGroup();
				}
				break;
			}
		
		case OP_PetitionRefresh: 
			{
				// This is When Client Asks for Petition Again and Again...
				// break is here because it floods the zones and causes lag if it
				// Were to actually do something:P  We update on our own schedule now.
				break;
			}
		
		case OP_Social_Action: 
			{
				cout << "Social Action:  " << app->size << endl;
				//DumpPacket(app);
				entity_list.QueueCloseClients(this, app, true);
				break;
			}
		case OP_SetServerFilter: 
			{
				/*	APPLAYER* outapp = new APPLAYER;
				outapp = new APPLAYER;
				outapp->opcode = OP_SetServerFilterAck;
				outapp->size = sizeof(SetServerFilterAck_Struct);
				outapp->pBuffer = new uchar[outapp->size];
				memset(outapp->pBuffer, 0, outapp->size);
				QueuePacket(outapp);
				safe_delete(outapp);//delete outapp;*/
				break;
			}
				
		default:
			{
				this->Process_UnknownOpCode(app);
				break;
			}
		}
	
	return;
}

//o--------------------------------------------------------------
//| ProcessOP_GMBecomeNPC; Yeahlight, Aug 19, 2008
//o--------------------------------------------------------------
//| GM issues /becomeNPC command. This function sets the 
//| associated rules for becomming an "NPC"
//o--------------------------------------------------------------
void Client::ProcessOP_GMBecomeNPC(APPLAYER* pApp)
{
	//Yeahlight: TODO: Set temporary PVP flags and max level flags for the GM here and
	//                 set conditions in attack.cpp. The GM should also be indifferent
	//                 to other NPCs.
	BecomeNPC_Struct* bnpc = (BecomeNPC_Struct*)pApp->pBuffer;
	Mob* client = (Mob*) entity_list.GetMob(bnpc->entityID);
	if(client == NULL)
		return;
	client->CastToClient()->QueuePacket(pApp);
	client->SendAppearancePacket(client->GetID(), SAT_NPC_Name, 1, true);
	client->CastToClient()->tellsoff = true;
}

//o--------------------------------------------------------------
//| ProcessOP_RezzRequest; Yeahlight, June 28, 2009
//o--------------------------------------------------------------
//| Handles the initial resurrection request on a corpse
//o--------------------------------------------------------------
void Client::ProcessOP_RezzRequest(APPLAYER* pApp)
{
	DumpPacketHex(pApp);
	if(pApp->size == sizeof(Resurrect_Struct))
	{
		Resurrect_Struct* rez = (Resurrect_Struct*)pApp->pBuffer;
		Mob* corpse = entity_list.GetMob((int16)rez->corpseEntityID);
		Mob* caster = entity_list.GetMob(rez->casterName);
		int16 spell_id = rez->spellID;
		if(corpse && caster && corpse->IsCorpse() && corpse->CastToCorpse()->IsPlayerCorpse())
		{
			rez->x = corpse->GetX();
			rez->y = corpse->GetY();
			rez->z = corpse->GetZ();
			strcpy(rez->targetName, corpse->CastToCorpse()->GetOrgname());
			corpse->CastToCorpse()->CastRezz(pApp, spell_id, caster);
		}
	}
	else
	{
		cout << "Wrong size on ProcessOP_RezzRequest. Got: " << pApp->size << ", Expected: " << sizeof(Resurrect_Struct) << endl;
	}
}

//o--------------------------------------------------------------
//| ProcessOP_RezzAnswer; Yeahlight, July 2, 2009
//o--------------------------------------------------------------
//| Handles the resurrection request answer by the client
//o--------------------------------------------------------------
void Client::ProcessOP_RezzAnswer(APPLAYER* pApp)
{
	DumpPacketHex(pApp);
	if(pApp->size == sizeof(Resurrect_Struct))
	{
		//Yeahlight: PC's time to respond to the resurrection request has expired
		if(pendingRez_timer->Check())
		{
			Message(BLACK, "Your time to answer the resurrection request has expired.");
		}
		//Yeahlight: PC's still has time to respond to the resurrection request
		else
		{
			Resurrect_Struct* rez = (Resurrect_Struct*)pApp->pBuffer;
			//Yeahlight: The PC accepting the rez matches the PC described in the rez packet
			if(strcmp(GetName(), rez->targetName) == 0)
			{
				//Yeahlight: PC declines the resurrection request
				if(rez->action == 0)
				{
					ServerPacket* pack = new ServerPacket(ServerOP_RezzPlayer, sizeof(RezzPlayer_Struct));
					RezzPlayer_Struct* sem = (RezzPlayer_Struct*) pack->pBuffer;
					memset(pack->pBuffer, 0, sizeof(RezzPlayer_Struct));
					memcpy(&sem->rez, pApp->pBuffer, sizeof(Resurrect_Struct));
					sem->rezzopcode = OP_RezzAnswer;
					sem->exp = 0;
					strcpy(sem->owner, this->GetName());
					sem->action = 0;
					worldserver.SendPacket(pack);
					safe_delete(pack);
				}
				//Yeahlight: PC accepts the resurrection request
				else if(rez->action == 1)
				{
					/*ServerPacket* pack = new ServerPacket(ServerOP_RezzPlayer, sizeof(RezzPlayer_Struct));
					RezzPlayer_Struct* sem = (RezzPlayer_Struct*) pack->pBuffer;
					memset(pack->pBuffer, 0, sizeof(RezzPlayer_Struct));
					memcpy(&sem->rez, pApp->pBuffer, sizeof(Resurrect_Struct));
					sem->rezzopcode = OP_RezzAnswer;
					sem->exp = GetPendingRezExp();
					strcpy(sem->owner, this->GetName());
					sem->action = 1;
					worldserver.SendPacket(pack);*/

					DumpPacket(pApp);					
					APPLAYER* outapp = new APPLAYER(OP_RezzComplete, sizeof(RezzPlayer_Struct)); 
					rez->z = 0;
					memcpy(outapp->pBuffer,rez, sizeof(RezzPlayer_Struct));
					
					
					QueuePacket(outapp);
					safe_delete(outapp);
				}
			}
		}
		pendingRez_timer->Disable();
	}
	else
	{
		cout << "Wrong size on ProcessOP_RezzAnswer. Got: " << pApp->size << ", Expected: " << sizeof(Resurrect_Struct) << endl;
	}
}

//---------------------------------------------------------------
//| ProcessOP_UseDiscipline; Harakiri, 8-25-2009
//---------------------------------------------------------------
//| Handles the /discipline command from the client
//---------------------------------------------------------------
void Client::ProcessOP_UseDiscipline(APPLAYER* pApp)
{	
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"UseDiscipline Request");
	
	if(pApp->size == sizeof(UseDiscipline_Struct))
	{

		UseDiscipline_Struct* disc = (UseDiscipline_Struct*)pApp->pBuffer;
		// 5 = ashen hand 31 = fearless 0 = without param
		EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Using Discipline %i", disc->discipline);
	}
	else
	{
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Wrong size on ProcessOP_UseDiscipline. Got: %i , Expected: %i",pApp->size, sizeof(UseDiscipline_Struct));		
	}
}

//---------------------------------------------------------------
//| ProcessOP_ClientError; Harakiri, 8-25-2009
//---------------------------------------------------------------
//| Whenever the client encounters an error (i.e. bogus item) it will sent this opcode
//---------------------------------------------------------------
void Client::ProcessOP_ClientError(APPLAYER* pApp)
{	
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"OPClientError Received");
	DumpPacketHex(pApp);
}

//---------------------------------------------------------------
//| ProcessOP_ApplyPoison; Harakiri, 8-25-2009
//---------------------------------------------------------------
//| Handles the apply poison request from a client
//| good read for recipes
//| http://web.archive.org/web/20010215012243/members.home.net/humblebee/eq/poison/recipes.html
//| http://web.archive.org/web/20001109195100/http://eq.castersrealm.com/poison/
//---------------------------------------------------------------
void Client::ProcessOP_ApplyPoison(APPLAYER* pApp)
{	
	// Harakiri the correct SLOT ID will only be sent by the client
	// when  the unique item id is set for each client
	// currently, only the command #si2 will do this, because its taken the items from the nonblob table
	// if the unique id is not set, the client assumes ID=0 for all items, when it searches for the poison
	// the user clicked on (there is a 7sec local client timeout till it sents this opcode), it memories the
	// uniqueID of the item - since it would all be 0, it will sent the SLOT of the first item found (even head slot etc)
	
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"ApplyPoison Received");
	DumpPacketHex(pApp);

	struct ApplyPoison_Struct {
	uint32 inventorySlot;
	uint32 success;
	};

	uint32 ApplyPoisonSuccessResult = 0;
	ApplyPoison_Struct* ApplyPoisonData = (ApplyPoison_Struct*)pApp->pBuffer;
		
	const ItemInst* PrimaryWeapon =  new ItemInst(GetItemAt(SLOT_PRIMARY), 0);
	const ItemInst* SecondaryWeapon =  new ItemInst(GetItemAt(SLOT_SECONDARY), 0);
			
	const ItemInst* Poison =  new ItemInst( GetItemAt(ApplyPoisonData->inventorySlot), 0);

	if(this->GetClass() == ROGUE) {
		
		if(Poison && Poison->GetItem() && Poison->GetItem()->common.itemType == ItemTypePoison) {
			if((PrimaryWeapon && PrimaryWeapon->GetItem() && PrimaryWeapon->GetItem()->common.itemType == ItemTypePierce) || (SecondaryWeapon && SecondaryWeapon->GetItem() && SecondaryWeapon->GetItem()->common.itemType == ItemTypePierce)) {						     			

				EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Using poison in slot id %i name %s",ApplyPoisonData->inventorySlot, Poison->GetItem()->name);
							

				float SuccessChance = (GetSkill(APPLY_POISON) + GetLevel()) / 400.0f;
				double ChanceRoll = MakeRandomFloat(0, 1);
			
				CheckAddSkill(APPLY_POISON, 10);
					
				if(ChanceRoll < SuccessChance) {
					ApplyPoisonSuccessResult = 1;					
					Spell* spell = spells_handler.GetSpellPtr(Poison->GetItem()->common.click_effect_id);
					SetBonusProcSpell(spell);
				} else {
					EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"ApplyPoison failed, success chance to low");
					
				}

				// Harakiri the charge was always removed, no matter if successful or not
				RemoveOneCharge(ApplyPoisonData->inventorySlot,true);
			} else {				
				EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Client does not have a piercing weapon equiped, cannot apply poison.");			
			}
		} else {
			EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Item in slot id %i not found or not of type poison",ApplyPoisonData->inventorySlot);			
		}
	}
	

	APPLAYER* outapp = new APPLAYER(OP_PoisonComplete, sizeof(ApplyPoison_Struct)); 	

	ApplyPoison_Struct* res = (ApplyPoison_Struct*)outapp->pBuffer;
	memset(res, 0, sizeof(ApplyPoison_Struct));

	res->success = ApplyPoisonSuccessResult;
	res->inventorySlot = ApplyPoisonData->inventorySlot;
	QueuePacket(outapp);
	safe_delete(outapp);
	
}

//---------------------------------------------------------------
//| ProcessOP_TranslocateResponse; Enraged, 8-25-2009
//---------------------------------------------------------------
//| Handles the translocate response sent by the client
//| Harakiri: Sept. 17. 2009 fixed and identified all Translocate_Struct bytes
//---------------------------------------------------------------
void Client::ProcessOP_TranslocateResponse(APPLAYER* pApp)
{
	if(pApp->size == sizeof(Translocate_Struct))
	{

		Translocate_Struct *trs = (Translocate_Struct*)pApp->pBuffer;
				   

		//make sure a translocate request is pending
		if(!pendingTranslocate) {
			EQC::Common::Log(EQCLog::Error, CP_CLIENT, "TranslocateResponse but no active timer!");
			return;
		}

		//allotted translocate time has expired
		if(pendingTranslocate_timer->Check())
		{
			Message(BLACK, "Your time to accept the translocation request has expired.");
		}
		else
		{			

			if(trs->confirmed == 1)
			{

				EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Translocate response accepted.");							
				TranslocatePC(pendingTranslocateZone, pendingTranslocateX, pendingTranslocateY, pendingTranslocateZ);							
			}
			else 
			{
				
				EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Translocate response refused.");	
				pendingTranslocate = false;
				pendingTranslocateSpellID = 0;
			}			
		}
	}
	else
	{
		EQC::Common::Log(EQCLog::Error, CP_CLIENT, "Incorrect size on TranslocateResponse_Struct. Got: %i , Expected: %i ",pApp->size,sizeof(Translocate_Struct));
	}
}


//---------------------------------------------------------------
//| ProcessOP_PlayerDeath; Harakiri, 10-14-2009
//---------------------------------------------------------------
//| Client sents this when being slain, before sending zone change struct at offset 0048048E, afterwards clears all the vars in the playerProfile like memorized spells and money
//---------------------------------------------------------------
void Client::ProcessOP_PlayerDeath(APPLAYER * pApp) {

	struct PlayerDeath_Struct {	
		int8 unknown[8];	
	};

	EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Received Player Death OP");

	DumpPacketHex(pApp);
	if(pApp->size == sizeof(PlayerDeath_Struct))
	{
	
	
	} else
	{
		EQC::Common::Log(EQCLog::Error, CP_CLIENT, "Incorrect size on PlayerDeath_Struct. Got: %i , Expected: %i ",pApp->size,sizeof(PlayerDeath_Struct));
	}
}
//---------------------------------------------------------------
//| ProcessOP_PlayerSave; Harakiri, 10-14-2009
//---------------------------------------------------------------
//| Client sents this before zoning or every couple minutes
//| Client seems to only sent items with item_nr >=1000 in inventory,bags,bank according to a client decompile at offset 004C71A3
//---------------------------------------------------------------
void Client::ProcessOP_PlayerSave(APPLAYER * pApp) {

	EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Received Player Save (%X)",pApp->opcode);
	
	// Harakiri Client does not associated checksum wit playerprofile struct, we should fix that too
	if (pApp->size == (sizeof(PlayerProfile_Struct) - PLAYERPROFILE_CHECKSUM_LENGTH)) 
	{ 		
		// Harakiri we currently do not trust the clients pp, so we dont store its values, but ours
		PlayerProfile_Struct* in_pp = (PlayerProfile_Struct*) (pApp->pBuffer - PLAYERPROFILE_CHECKSUM_LENGTH);
		//FileDumpPacketHex("playerProfile.txt",app);
		
		// Harakiri  some values are set client side generated which we cannot catch

		// Harakiri Drunkness is a random value in the client at offset 0046AB71
		// algorithm is : 
		// rand = random(1,100); 
		// if (rand <= currentAlcoholSkill && rand < 96) 
		// drunkness = (40*rand)/currentAlcoholSkill;
      
		EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Saving Drunkess Value %i",in_pp->drunkeness);
		GetPlayerProfilePtr()->drunkeness = in_pp->drunkeness;
		
	
	} else
	{
		EQC::Common::Log(EQCLog::Error, CP_CLIENT, "Incorrect size on PlayerProfile_Struct. Got: %i , Expected: %i ",pApp->size,sizeof(PlayerProfile_Struct));
	}
	
	Save();
	
}
//---------------------------------------------------------------
//| ProcessOP_ItemMissing; Harakiri, 10-14-2009
//---------------------------------------------------------------
//| Client sents this under specific conditions when he cant use something like a spell, because a component is needed
//---------------------------------------------------------------
void Client::ProcessOP_ItemMissing(APPLAYER *pApp) {
	
	EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Received ItemMissing from Client");

	DumpPacketHex(pApp);

}