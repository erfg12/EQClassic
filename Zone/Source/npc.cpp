
#include <cmath>
#include <fstream>
#include "moremath.h"
#include "stdio.h"

#ifndef WIN32
	#include <cstdlib>
	#include <pthread.h>
#endif

#include "npc.h"
#include "client.h"
#include "../../Utils/azone/map.h"
#include "entity.h"
#include "../../Zone/randomize.h"
#include "skills.h"
#include "SpellsHandler.hpp"

#ifdef EMBPERL
	#include "embparser.h"
#endif

using namespace std;

extern Zone* zone;
extern volatile bool ZoneLoaded;
extern SpellsHandler	spells_handler;
extern WorldServer		worldserver;

NPC::NPC(NPCType* d, Spawn2* in_respawn, float x, float y, float z, float heading, bool IsCorpse, int in_roamRange, int16 in_myRoamBox, int16 in_myPathGrid, int16 in_myPathGridStart)
 : Mob(d->name,
       d->Surname,
       d->max_hp,
       d->max_hp,
       d->gender,
       d->race,
       d->class_,
       d->deity,
       d->level,
	   d->body_type,
	   d->npc_id, // rembrant, Dec. 20, 2001
	   d->skills, // socket 12-29-01
	   d->size,
	   d->walkspeed,
	   d->runspeed,
       heading,
       x,
       y,
       z,
       d->light,
       d->equipment,
	   d->texture,
	   d->helmtexture,
	   d->AC,
	   d->ATK,
	   d->STR,
	   d->STA,
	   d->DEX,
	   d->AGI,
	   d->INT,
	   d->WIS,
	   d->CHA,
	   d->MR,
	   d->CR,
	   d->FR,
	   d->DR,
	   d->PR,
	   d->min_dmg,
	   d->max_dmg,
	   d->special_attacks,
	   d->attack_speed,
	   d->main_hand_texture,
	   d->off_hand_texture,
	   d->hp_regen_rate,
	   d->passiveSeeInvis,
	   d->passiveSeeInvisToUndead,
	   d->time_of_day
	   )
{
	NPCTypedata = d;
	respawn2 = in_respawn;

	itemlist = new ItemList();
	copper = 0;
	silver = 0;
	gold = 0;
	platinum = 0;
	MerchantType = d->merchanttype; // Yodason: merchant stuff

	// Harakiri for sending quest event attacked
	attacked_timer = new Timer(12000);
	
	//Mobs do not start out walking.
	walking_timer = new Timer(200);
	walking_timer->Disable();
	
	org_x = x;
	org_y = y;
	org_z = z;
	org_heading = heading;
	stunned = false;
	stunned_timer = new Timer(0);

	EntityList::RemoveNumbers(name);

	//Yeahlight: Grab the NPC's primary facton ID while their name is stripped of excess data
	primaryFaction = Database::Instance()->GetPrimaryFaction(name);
	//Yeahlight: Record the NPC's proper name while it is stripped of excees data
	CreateProperName(name);
	entity_list.MakeNameUnique(name);
	p_depop = false;
	loottable_id = d->loottable_id;

	sint32 tmp;
	Database::Instance()->GetNPCPrimaryFaction(this->npctype_id, &faction_id, &tmp);
	ignore_target = 0;
	tic_timer = new Timer(TIC_TIME);	// less than 6 seconds timer as margin.
	isMovingHome = false;
	gohome_timer = new Timer(5000);
	gohome_timer->Disable();
	tod_despawn_timer = new Timer(1000); // Kibanu
	tod_despawn_timer->Disable();

	x_dest = 0;
	y_dest = 0;
	z_dest = 0;
		
	feign_memory = "0";
	forget_timer = new Timer(500);
	forget_timer->Disable();
	SetPetOrder(SPO_Follow);//Tazadar : We initialise the pet movement
	movingToGuardSpot = false;

	onPath = false;
	myPathNode = NULL;
	SetCurfp(false);

	checkLoS_timer = new Timer(2250);
	checkLoS_timer->Disable();
	faceTarget_timer = new Timer(250);
	faceTarget_timer->Disable();

	//Yeahlight: We use a random starting timer so that ALL mobs do not udpate at once
	int32 randomTimer = (int32)(fRandomNum(0.125, 0.999) * 6000);
	spawnUpdate_timer = new Timer(randomTimer);

	//Yeahlight: We use a random starting timer so that ALL mobs do not scan their surroundings at once
	randomTimer = (int32)(fRandomNum(0.125, 0.999) * 1250);
	scanarea_timer = new Timer(randomTimer);
	
	this->setMoving(false);

	myRoamBox = in_myRoamBox;
	roamRange = in_roamRange;

	//Yeahlight: We use a random starting timer so that ALL mobs do not roam at the same time
	randomTimer = (int32)(fRandomNum(0.125, 0.999) * 60000);
	roam_timer = new Timer(randomTimer);

	//Yeahlight: Static mob; NPC does not need a roam timer
	if(roamRange == 0)
		roam_timer->Disable();
	else
		isRoamer = true;
	onRoamPath = false;
	blockFlee = false;

	pathing_teleportation_timer = new Timer(1000);
	pathing_teleportation_timer->Disable();

	if(in_myPathGridStart == 0)
	{
		mySpawnNode = findClosestNode(this);
	}
	else
	{
		mySpawnNode = zone->thisZonesNodes[in_myPathGridStart];
		myPathNode = mySpawnNode;
	}

	myPathGrid = in_myPathGrid;
	isPatrolling = false;
	randomTimer = (int32)(fRandomNum(0.125, 0.999) * 10000) + 5000;
	patrolPause_timer = new Timer(randomTimer);
	if(myPathGrid != 0)
	{
		if(GetMyGridPath())
			isPatroller = true;
		else
			isPatroller = false;
	}
	else
	{
		isPatroller = false;
	}
	if(!isPatroller)
		patrolPause_timer->Disable();

	requiresNewPath = false;
	preventPatrolling = false;

	offensiveCast_timer = new Timer(6000);
	defensiveCast_timer = new Timer(3000);
	debuffCounterRegen_timer = new Timer(6000);
	emergencyFlee_timer = new Timer(1500);
	emergencyFlee_timer->Disable();
	revertFlee_timer = new Timer(1000);
	revertFlee_timer->Disable();
	if(Database::Instance()->LoadNPCSpells(this->GetClass(), this->GetLevel(), myBuffs, myDebuffs))
	{
		isCaster = true;
	}
	else
	{
		isCaster = false;
		offensiveCast_timer->Disable();
		debuffCounterRegen_timer->Disable();
		defensiveCast_timer->Disable();
	}
	isCasting = false;
	mySpellCounter = 0;

	fleeing = false;
	onFleePath = false;

	nodeFocused = false;
	nodeFocused_timer = new Timer(3000);
	nodeFocused_timer->Disable();

	leash_timer = new Timer(6000);
	leash_timer->Disable();
	leashMemory = "0";

	myMinDmg = d->min_dmg;
	myMaxDmg = d->max_dmg;

	//Yeahlight: Load NPC special attacks
	strcpy(mySpecialAttacks, d->special_attacks);
	canSummon = false;
	canEnrage = false;
	canRampage = false;
	canFlurry = false;
	canTrippleAttack = false;
	canQuadAttack = false;
	cannotBeSlowed = false;
	cannotBeMezzed = false;
	cannotBeCharmed = false;
	cannotBeStunned = false;
	cannotBeSnaredOrRooted = false;
	cannotBeFeared = false;
	isImmuneToMelee = false;
	isImmuneToMagic = false;
	isImmuneToFleeing = false;
	isImmuneToNonBaneDmg = false;
	isImmuneToNonMagicalDmg = false;
	willNeverAgro = false;
	for(int i = 0; i < 20; i++)
	{
		char flag = mySpecialAttacks[i];
		if(flag == 'S')
			canSummon = true;
		else if(flag == 'E')
			canEnrage = true;
		else if(flag == 'R')
			canRampage = true;
		else if(flag == 'F')
			canFlurry = true;
		else if(flag == 'T')
			canTrippleAttack = true;
		else if(flag == 'Q')
			canQuadAttack = true;
		else if(flag == 'U')
			cannotBeSlowed = true;
		else if(flag == 'M')
			cannotBeMezzed = true;
		else if(flag == 'C')
			cannotBeCharmed = true;
		else if(flag == 'N')
			cannotBeStunned = true;
		else if(flag == 'I')
			cannotBeSnaredOrRooted = true;
		else if(flag == 'D')
			cannotBeFeared = true;
		else if(flag == 'A')
			isImmuneToMelee = true;
		else if(flag == 'B')
			isImmuneToMagic = true;
		else if(flag == 'f')
			isImmuneToFleeing = true;
		else if(flag == 'O')
			isImmuneToNonBaneDmg = true;
		else if(flag == 'W')
			isImmuneToNonMagicalDmg = true;
		else if(flag == 'H')
			willNeverAgro = true;
	}

	int16 highestMeleeSkill = 0;
	//Yeahlight: Set the NPC's skills by level
	for(int i = 0; i <= TAUNT; i++)
	{
		int16 skillValue = CheckMaxSkill(i, d->race, d->class_, d->level);
		//Yeahlight: Keep track of the highest melee skill so that PCs giving an NPC a weapon has no negative effects
		if(i == _1H_BLUNT || i == _2H_BLUNT || i == _1H_SLASHING || i == _2H_SLASHING || i == PIERCING || i == HAND_TO_HAND)
			if(skillValue > highestMeleeSkill)
				highestMeleeSkill = skillValue;
		skills[i] = skillValue;
	}
	//Yeahlight: Set all melee skills to the highest available for this class
	skills[_1H_BLUNT] = highestMeleeSkill;
	skills[_2H_BLUNT] = highestMeleeSkill;
	skills[_1H_SLASHING] = highestMeleeSkill;
	skills[_2H_SLASHING] = highestMeleeSkill;
	skills[PIERCING] = highestMeleeSkill;
	skills[HAND_TO_HAND] = highestMeleeSkill;

	int16 attackerSTRBonus = GetSTR();
	if(attackerSTRBonus > 75)
		attackerSTRBonus -= 75;
	else
		attackerSTRBonus = 0;
	//Yeahlight: Formula from monkly-business.net (Weapon_Skill*2.7 + Offense_Skill*4/3 + Worn_Atk*4/3 + Str>75*9/10 + 10)
	BaseStats.ATK = (highestMeleeSkill*(float)(2.7)) + (skills[OFFENSE]*(float)(4/3)) + (attackerSTRBonus * 9 / 10) + 10;

	//Yeahlight: Calculate AC portions
	this->CalcBonuses(false, false);

	summon_target_timer = new Timer(6000);
	if(!this->canSummon)
		summon_target_timer->Disable();

	enrage_timer = new Timer(12000);
	enrage_timer->Disable();
	hasEnragedThisFight = false;
	enraged = false;

	if(this->canRampage)
	{
		for(int i = 0; i < RAMPAGE_LIST_SIZE; i++)
			rampageList[i] = 0;
		rampageListCounter = 0;
	}

	get_target_timer = new Timer(200);
	taunt_timer = new Timer(5000);
	taunt_timer->Disable();
	taunting = false;
	classattack_timer = new Timer(1000);
	knightability_timer = new Timer(1000);
	if(GetClass() != SHADOWKNIGHT && GetClass() != SHADOWKNIGHTGM && GetClass() != PALADIN && GetClass() != PALADINGM)
		knightability_timer->Disable();
	classAbility_timer = new Timer(1000);
	//Yeahlight: The following classes do not get class abilities, so disable the timer
	if(GetClass() == NECROMANCER || GetClass() == MAGICIAN || GetClass() == WIZARD || GetClass() == ENCHANTER || GetClass() == SHAMAN || GetClass() == DRUID || GetClass() == BARD)
		classAbility_timer->Disable();

	reserveMyAttack = false;

	myRegenRate = d->hp_regen_rate;

	spinning = false;

	myEyeOfZommMasterID = 0;

	feared = false;
	onFearPath = false;

	charmed = false;

	depop_timer = new Timer(120000);
	depop_timer->Disable();

	SetBoat(false);
	if(race == SHIP || race == LAUNCH){
		SetBoat(true);
		currentnode = 0;
		targetnode = 0;
		orders = false;
		lineroute = false;
		getBoatPath();
	}

	SetPassiveCanSeeThroughInvis(false);
	SetPassiveCanSeeThroughInvisToUndead(false);
	if(d->passiveSeeInvis)
		SetPassiveCanSeeThroughInvis(true);
	if(d->passiveSeeInvisToUndead)
		SetPassiveCanSeeThroughInvisToUndead(true);
	
	SetHitOnceOrMore(false);

	// Harakiri for quest signal() command
	signaled = false;
	signal_id = 0;
}

NPC::~NPC()
{
	safe_delete(scanarea_timer);//delete scanarea_timer;
	safe_delete(gohome_timer);//delete gohome_timer;
	safe_delete(tod_despawn_timer);//delete tod_despawn_timer;
	safe_delete(stunned_timer);//delete stunned_timer;
	safe_delete(forget_timer);
	safe_delete(walking_timer);
	safe_delete(itemlist);
	safe_delete(checkLoS_timer);
	safe_delete(faceTarget_timer);
	safe_delete(roam_timer);
	safe_delete(patrolPause_timer);
	safe_delete(offensiveCast_timer);
	safe_delete(debuffCounterRegen_timer);
	safe_delete(emergencyFlee_timer);
	safe_delete(revertFlee_timer);
	safe_delete(nodeFocused_timer);
	safe_delete(summon_target_timer);
	safe_delete(enrage_timer);
	safe_delete(get_target_timer);
	safe_delete(classattack_timer);
	safe_delete(taunt_timer);
	safe_delete(knightability_timer);
	safe_delete(attacked_timer);
	//Yeahlight: Disabling this for now
	//if (npctype_id == 0)
	//{
	//	//NPCTypedata=0; // Added this in order to kill the pet and avoid a crash - Tazadar 31/05/08: Yeahlight: This just creates a memory leak
	//	safe_delete(NPCTypedata); // Added this in order to kill the pet and avoid a crash - Tazadar 31/05/08
	//}
}

ServerLootItem_Struct* NPC::GetItem(int slot_id)
{
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	iterator.Reset();
	ServerLootItem_Struct* temp = NULL;
	
	while(iterator.MoreElements())
	{
		ServerLootItem_Struct* item = iterator.GetData();
		if (item->equipSlot == slot_id)
		{
			//Yeahlight: We do not want to use the first item we come across, but rather the most recently added (bottom of the list)
			temp = item;
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	if(temp)
		return temp;
	
	return 0;
}

void NPC::AddItem(Item_Struct* item, int8 charges, uint8 slot)
{
	if(!item)
		return;
	ServerLootItem_Struct* item_data = new ServerLootItem_Struct;
	//Yeahlight: The supplied slot is always incorrect as far as I can tell, so get a new slot ID
	slot = GetSlotOfItem(item);
	item_data->charges = charges;
	item_data->equipSlot = slot;
	item_data->item_nr = item->item_nr;
	(*itemlist).Append(item_data);
	//Yeahlight: Fill equipment texture based on slot
	if(slot <= 8)
	{
		//Yeahlight: Item is not a weapon
		if(slot < 7)
		{
			equipment[slot] = item->common.material;
			equipmentColor[slot] = item->common.color;
		}
		//Yeahlight: Item is a weapon
		else
		{
			//Yeahlight: Item has a legit texture
			if(strlen(item->idfile) >= 3)
			{
				//Yeahlight: Assign new primary weapon
				if(slot == 7)
				{
					//Yeahlight: If the NPC is holding nodrop weapons, then prevent the removal of these items.
					//           We do not want players removing items from the hands of raid NPCs.
					if(myMainHandWeapon && myMainHandWeapon->nodrop == 0)
						return;
					myMainHandWeapon = item;
				}
				//Yeahlight: Assign new secondary weapon as long as it actually is a weapon
				else if(item->common.damage > 0)
				{
					//Yeahlight: Again, do not let PCs replace nodrop, off-hand weapons on the NPC
					if(myOffHandWeapon && myOffHandWeapon->nodrop == 0)
						return;
					myOffHandWeapon = item;
				}
				equipment[slot] = atoi(&item->idfile[2]);
				//Yeahlight: Remove offhand textures when a two hand weapon is recieved
				if(item->common.itemType == DB_2H_SLASHING || item->common.itemType == DB_2H_BLUNT || item->common.itemType == DB_2H_PIERCING)
				{
					equipment[8] = 0;
					myOffHandWeapon = 0;
					slot = 99;
				}
			}
		}			
	}
	if(GetMeleeWeapons())
		SendWearChange(slot);
	CalcBonuses(true, true);
}

void NPC::AddItem(int32 itemid, int8 charges, uint8 slot)
{
	ServerLootItem_Struct* item_data = new ServerLootItem_Struct;
	Item_Struct* item = Database::Instance()->GetItem(itemid);
	if(!item)
		return;
	//Yeahlight: The supplied slot is always incorrect as far as I can tell, so get a new slot ID
	slot = GetSlotOfItem(item);
	item_data->charges = charges;
	item_data->equipSlot = slot;
	item_data->item_nr = itemid;
	(*itemlist).Append(item_data);
	//Yeahlight: Fill equipment texture based on slot
	if(slot <= 8)
	{
		//Yeahlight: Item is not a weapon
		if(slot < 7)
		{
			equipment[slot] = item->common.material;
			equipmentColor[slot] = item->common.color;
		}
		//Yeahlight: Item is a weapon
		else
		{
			//Yeahlight: Item has a legit texture
			if(strlen(item->idfile) >= 3)
			{
				//Yeahlight: Assign new primary weapon
				if(slot == 7)
				{
					//Yeahlight: If the NPC is holding nodrop weapons, then prevent the removal of these items.
					//           We do not want players removing items from the hands of raid NPCs.
					if(myMainHandWeapon && myMainHandWeapon->nodrop == 0)
						return;
					myMainHandWeapon = item;
				}
				//Yeahlight: Assign new secondary weapon as long as it actually is a weapon
				else if(item->common.damage > 0)
				{
					//Yeahlight: Again, do not let PCs replace nodrop, off-hand weapons on the NPC
					if(myOffHandWeapon && myOffHandWeapon->nodrop == 0)
						return;
					myOffHandWeapon = item;
				}
				equipment[slot] = atoi(&item->idfile[2]);
				//Yeahlight: Remove offhand textures when a two hand weapon is recieved
				if(item->common.itemType == DB_2H_SLASHING || item->common.itemType == DB_2H_BLUNT || item->common.itemType == DB_2H_PIERCING)
				{
					equipment[8] = 0;
					myOffHandWeapon = 0;
					slot = 99;
				}
			}
		}			
	}
	if(GetMeleeWeapons())
		SendWearChange(slot);
	CalcBonuses(true, true);
}

void NPC::AddLootTable()
{
	if (npctype_id != 0)
	{
		Database::Instance()->AddLootTableToNPC(loottable_id, itemlist, &copper, &silver, &gold, &platinum, equipment, equipmentColor);
	}
}

void NPC::RemoveItem(uint16 item_id)
{
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();

	while(iterator.MoreElements())
	{
		if (iterator.GetData()->item_nr == item_id)
		{
			Item_Struct* item = Database::Instance()->GetItem(iterator.GetData()->item_nr);
			iterator.RemoveCurrent();
			return;
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	return;
}

void NPC::ClearItemList()
{
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.RemoveCurrent();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

void NPC::QueryLoot(Client* to) 
{
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	
	iterator.Reset();
	int x = 0;
	to->Message(BLACK, "Coin: %ip %ig %is %ic", platinum, gold, silver, copper);
	while(iterator.MoreElements())
	{
		Item_Struct* item = Database::Instance()->GetItem(iterator.GetData()->item_nr);
		to->Message(BLACK, "  %d: %s", item->item_nr, item->name);
		x++;
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	to->Message(BLACK, "%i items on %s.", x, this->GetName());
}

void NPC::AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum)
{
	copper = in_copper;
	silver = in_silver;
	gold = in_gold;
	platinum = in_platinum;
}

void NPC::AddCash()
{
	copper = (rand() % 100)+1;
	silver = (rand() % 50)+1;
	gold = (rand() % 10)+1;
	platinum = (rand() % 5)+1;
}

void NPC::RemoveCash()
{
	copper = 0;
	silver = 0;
	gold = 0;
	platinum = 0;
}

bool NPC::Process()
{
	bool debugFlag = true;

	// Harakiri Quest Trigger check
	if(IsNPC()) {
		CheckSignal();
	}

	//Yeahlight: NPC has been flagged to depop
	if(p_depop || depop_timer->Check()) 
	{
		CheckMyDepopStatus();
		return false;
	}

	bool engaged = false;

	//Yeahlight: This fixes the client's inability to target via F8 key and also restores each mob's random sound effects (flapping wings, growls, etc)
	if(this->spawnUpdate_timer->Check())
	{
		//Yeahlight: Reset the spawnUpdate_timer's duration to five seconds due to the random start time
		spawnUpdate_timer->Start(5000);
		if(this->IsBoat())
			SendPosUpdate(false, BOAT_UPDATE_RANGE, false);
		else
			SendPosUpdate(false, NPC_UPDATE_RANGE, true);
	}

	//Yeahlight: NPC is a boat
	if(IsBoat())
	{
		//Yeahlight: Tazadar's boat work here
		//cout << this->name << " Is a boat !! " << currentnode << " to " << targetnode << " at loc " << this->GetX() << "," << this->GetY() << "," << this->GetZ() <<endl;

		if(!isMoving() && (currentnode == targetnode) && orders){
			SendTravelDone();
			orders = false;
		}

		if(!isMoving() && (currentnode < targetnode)){
			//cout << "Current node changed " << currentnode <<" Z value "<< this->GetZ()  << "Heading " << this->GetHeading()<< endl;
			if(boatroute[currentnode].cmd == GOTO ){
				SetDestination(boatroute[currentnode].x,boatroute[currentnode].y,this->GetZ());
				setBoatSpeed(boatroute[currentnode].go_to.speed);
				//if(!lineroute)
					faceDestination(x_dest, y_dest);
				StartWalking();
				SendPosUpdate(false, BOAT_UPDATE_RANGE, false);
			}
			else if(boatroute[currentnode].cmd == ROTATE){
				this->GMMove(this->GetX(),this->GetY(),this->GetZ(),boatroute[currentnode].rotate.heading);
				SendPosUpdate(false, BOAT_UPDATE_RANGE, false);
			}
			currentnode++;
		}
	}
	//Yeahlight: NPC is not a boat
	else
	{
		//Yeahlight: NPC is currently stunned and is ready to be unstunned
		if(IsStunned() && stunned_timer->Check())
		{
			this->stunned = false;
			this->stunned_timer->Disable();
			if(this->GetSpin())
			{
				SetSpin(false);
				SendPosUpdate(false, 200);
			}
		}

		//Yeahlight: Check NPC's spell process
		SpellProcess();
		
		//Yeahlight: NPC is ready to regen
		if(tic_timer->Check())
		{
			DoHPRegen();
		}

		//Yeahlight: Return if this NPC is stunned or mesmerized
		if(IsStunned() || IsMesmerized())
		{
			//Yeahlight: Stop this NPC's movement if they currently have a velocity
			if(this->GetAnimation())
				setMoving(false);
			return true;
		}

		//Yeahlight: Eye of Zomms are controlled by PCs, so return immediately
		if(IsEyeOfZomm())
		{
			return true;
		}

		//Yeahlight: NPC is not charmed
		if(this->IsCharmed() == false)
		{
			//Yeahlight: Feign death memory
			if(forget_timer->Check() && strcmp(GetFeignMemory(), "0") != 0)
			{
				CheckMyFeignMemory(debugFlag);
			}

			//Yeahlight: Time for this leashed mob to consider reengaging the target in memory
			if(!this->nodeFocused && leash_timer->Check() && strcmp(GetLeashMemory(), "0") != 0)
			{
				CheckMyLeashMemory(debugFlag);
			}
		}

		//Yeahlight: Enraging NPC is ready to calm down
		if(enrage_timer->Check())
		{
			CheckMyEnrageStatus();
		}

		//Yeahlight: NPC has resorted to teleportation to reach its target
		if(pathing_teleportation_timer->Check())
		{
			DoMyTeleport();
		}

		//Yeahlight: NPC is focused on a node and it is time to consider breaking the focus
		if(this->nodeFocused && nodeFocused_timer->Check())
		{
			CheckMyNodeFocusedStatus();
		}

		//Yeahlight: NPC has been flagged as fleeing, but has not yet resolved a path and is not blocked from fleeing
		if(this->IsFleeing() && !this->onPath && !this->onFleePath && !this->blockFlee)
		{
			CheckMyFleeStatus(debugFlag);
		}

		//Yeahlight: NPC is currently fleeing and their HP ratio has been pushed above 25%
		if(this->revertFlee_timer->Check() && this->GetHPRatio() >= 26)
		{
			this->fleeing = false;
			this->onFleePath = false;
			this->revertFlee_timer->Disable();
		}

		//Yeahlight: Check to see if this NPC is currently engaged
		engaged = this->IsEngaged();

		//Yeahlight: Purge this NPC's target if they are not engaged
		if(engaged == false)
			SetTarget(NULL);

		//Yeahlight: Time for the engaged NPC / pet to retrieve their target from the hate list
		if(get_target_timer->Check() && (engaged || GetOwner()))
		{
			CheckMyTargetStatus();
		}

		//Yeahlight: This casting NPC has a chance to gain back a debuff credit
		if(this->debuffCounterRegen_timer->Check() && this->mySpellCounter > 0)
		{
			CheckMyDebuffRefundStatus(engaged);
		}
		
		//Yeahlight: NPC is feared but has not yet resolved a path
		if(GetFeared() && !onFearPath)
		{
			GetFearDestination(GetX(), GetY(), GetZ());
		}

		//Yeahlight: Update heading while not on a specific path or disabled
		if(faceTarget_timer->Check() && target && !onPath && !IsDisabled() && !isCasting)
		{
			FaceTarget(target);
		}

		//Yeahlight: NPC is engaged
		if(engaged)
		{
			//Yeahlight: Even after agro, undead will continue to scan its environment for entities to engage
			if(GetBodyType() == BT_Undead && scanarea_timer->Check())
			{
				//Yeahlight: NPC is not a pet
				if(GetOwner() == NULL)
				{
					CheckMyAgroStatus();
				}
			}

			//Yeahlight: NPC's target exists and the NPC is not currently disabled
			if(target && !IsDisabled())
			{
				//Yeahlight: Summoning NPC is engaged, not fleeing and ready to summon target if necessary
				if(this->canSummon && summon_target_timer->Check() && this->GetHPRatio() <= 97)
				{
					CheckMySummonStatus();
				}

				//Yeahlight: Mobs check for LoS while engaged to determine if creating a new path is necessary
				if(checkLoS_timer->Check() && !this->nodeFocused)
				{
					CheckMyLosStatus(debugFlag);
				}

				//Yeahlight: NPC is engaged, currently has a target and is not currently casting a spell
				if(!this->isCasting)
				{
					CheckMyEngagedStatus(debugFlag);
				}
			}

			//Yeahlight: NPC is engaged, is a caster and is ready to consider the option of a defensive spell (a heal) on an ally (NPC with the same pFaction) in the area
			if(this->isCaster && defensiveCast_timer->Check() && this->myBuffs[HEAL] != NULL_SPELL && !this->isCasting && !IsDisabled() && zone->zoneAgroCounter > 1)
			{
				CheckMyDefenseCastStatus();
			}

			//Yeahlight: Casting NPC is engaged and ready to cast an offensive spell (if it is not casting already)
			if(this->isCaster && offensiveCast_timer->Check() && !this->isCasting && !IsDisabled() && !this->nodeFocused)
			{
				CheckMyOffenseCastStatus();
			}

			//pet moves to the enemy
			if(movingToGuardSpot && GetPetOrder() == SPO_Guard)
			{
				movingToGuardSpot = false;
			}
		}
		//Yeahlight: NPC is not engaged
		else
		{
			//Yeahlight: NPC is not charmed
			if(this->IsCharmed() == false)
			{
				// Kibanu - Despawn now if they are close to home if day/night flags are set appropriately
				if ( zone && 
					(
						( zone->IsDaytime() && this->NPCTypedata->time_of_day == Night_Time_Only ) ||
						( !zone->IsDaytime() && this->NPCTypedata->time_of_day == Day_Time_Only )
					) 
				) {
					if ( !tod_despawn_timer->Enabled() )
					{
						int32 rnd_despawn = (rand()%300)*1000; // Despawn randomly between immediately and 5 minutes.
						tod_despawn_timer->Start(rnd_despawn,true);
						//cout << "NPC: " << this->GetName() << " despawning due to ToD in " << ( rnd_despawn / 1000 ) << " seconds." << endl;
					}
					if ( tod_despawn_timer->Check() )
					{
						// We're in distance of original x/y, let's despawn
						this->Depop(false);
						CheckMyDepopStatus();
						if ( respawn2 )
						{
							int32 rnd_respawn = ((rand()%10)+5)*1000;
							respawn2->Repop(rnd_respawn); // Respawn between 5 and 15 seconds
							//cout << "NPC: " << this->GetName() << " respawning due to ToD in " << ( rnd_respawn / 1000 ) << " seconds." << endl;
						}
						return false;
					}
				}
				//Yeahlight: Time for the NPC to go home while its not engaged
				if(gohome_timer->Check()) 
				{
					CheckMyGoHomeStatus();
				}

				//Yeahlight: NPC is moving home or is patrolling but is not currently casting a spell
				if((this->isMovingHome || this->isPatrolling) && !this->preventPatrolling && !this->isCasting)
				{
					CheckMyMovingStatus(debugFlag);
				}

				//Yeahlight: NPC is ready to roam and is not currently engaged or currently roaming (this may happen if the NPC is a zone wide roamer)
				if(!this->onRoamPath && this->roam_timer->Check() && rand()%2 == 1)
				{
					CheckMyRoamStatus();
				}

				//Yeahlight: NPC is a patroller and is ready to continue its grid path
				if(this->isPatroller && this->patrolPause_timer->Check())
				{
					CheckMyPatrolStatus();
				}
			}
			
			//Yeahlight: Time for the NPC to scan its surroundings as long as it is not flagged to never agro
			if(GetWillNeverAgro() == false && scanarea_timer->Check())
			{
				//Yeahlight: NPC is not a pet
				if(GetOwner() == 0)
				{
					//Yeahlight: Zone is not over its agro budget
					if(zone->AggroLimitReached() == false)
					{
						CheckMyAgroStatus();
					}
				}
				//Yeahlight: NPC is a pet
				else
				{
					//Yeahlight: Pet is currently guarding, face the closest entity that is not the owner
					if(GetPetOrder() == SPO_Guard && isMoving() == false)
					{
						entity_list.PetFaceClosestEntity(this);
					}
				}
			}
		}
	}

	//Yeahlight: NPC is moving and not casting a spell
	if(walking_timer->Check() && !this->isCasting)
	{
		CheckMyWalkingStatus(engaged);
	}
    return true;
}

void NPC::MoveTowards(float x, float y, float d)
{
	bool debugFlag = true;

	float stop_at_distance = 0;
	
	//Yeahlight: NPC is currently on a path or is a boat
	if(onPath || onRoamPath || onFleePath || onFearPath || IsBoat() || (isMovingHome && !this->IsEngaged()))
	{
		//Yeahlight: Set distance to 1 to make the transition from node to node smooth
		stop_at_distance = 1;
	}
	else
	{
		//Yeahlight: Set distance to max melee range minus three units to prevent "dead zones"
		stop_at_distance = GetMeleeReach() - 3;
	}
	
	float distance = fdistance(x,y, NPC::x_pos, NPC::y_pos);

	if(distance < d)
	{
		d = distance;
	}

	if(distance < stop_at_distance)
	{
		this->setMoving(false);
		//Yeahlight: NPC is a boat
		if(this->IsBoat())
		{
			SendPosUpdate(false, BOAT_UPDATE_RANGE, false);
			walking_timer->Disable();
		}
		//Yeahlight: NPC is not a boat
		else
		{
			//Yeahlight: Roamers are given such a large z-axis offset due to the randomness of the terrain for zone wide roamers
			if(onRoamPath)
				FindGroundZ(GetX(), GetY(), 75);
			else
				FindGroundZ(GetX(), GetY(), 6);
			//Yeahlight: If the NPC is roaming, pathing or patrolling, send a PoS update to EVERY client in the zone when their path finishes
			if(stop_at_distance == 1)
			{
				if(myPath.size() > 1)
					SendPosUpdate(false, NPC_UPDATE_RANGE, false);
				else
					SendPosUpdate(false);
			}
			//Yeahlight: NPC is most likely now in melee range of its target
			else
			{
				//Yeahlight: Now that the NPC may be in range of its target, attempt to use their reserved attack
				if(reserveMyAttack && this->IsEngaged() && target)
				{
					Attack(target, 13, true);
					pLastChange = Timer::GetCurrentTime();
					reserveMyAttack = false;
					//Yeahlight: Reset the attack timer
					attack_timer->Start(attack_timer->GetTimerTime());
				}
				SendPosUpdate(false, NPC_UPDATE_RANGE, false);
			}
			walking_timer->Disable();
			onRoamPath = false;
			onFleePath = false;
			onFearPath = false;
			//Yeahlight: Only break pathing focus on a node when the NPC hits the end of their path
			if(nodeFocused)
			{
				if(onPath && myPath.size() == 1)
					nodeFocused = false;
				else if(!onPath)
					nodeFocused = false;
			}
		}
		return;
	}
	
	float x1 = NPC::x_pos;	//Old position.
	float y1 = NPC::y_pos;
	float x2 = x;			//New destination.
	float y2 = y;			
	float delta_y = y1-y2;
	float delta_x =	x1-x2;
	float new_x = 0;
	float new_y = 0;
	float new_x2 = 0;
	float new_y2 = 0;


	if(delta_y == 0)
	{
		if(x1 < x2)
		{
			new_x = x1 + d;
		}
		else
		{
			new_x = x1 - d;
		}
		new_y = y1;
	}
	else if(delta_x == 0)
	{
		if(y1 < y2)
		{
			new_y = y1 + d;
		}
		else
		{
			new_y = y1 - d;
		}
		new_x = x1;
	}
	else
	{
		float m = delta_y/delta_x;
		float radical = sqrt(square(d)/(1+square(m)));

		new_x = x1 + radical;
		new_x2 = x1 - radical;
		new_y = m*new_x+(y1-m*x1);
		new_y2 = m*new_x2+(y1-m*x1);

		float d1 = fdistance(new_x,  new_y,  x2, y2);	//Verify distance.
		float d2 = fdistance(new_x2, new_y2, x2, y2);				

		if((d1 < d2) == false)
		{
			new_x = new_x2;
			new_y = new_y2;
		}
	}
	float x_step = (x_pos - new_x)/3;
	float y_step = (y_pos - new_y)/3;
	float old_x = x_pos;
	float old_y = y_pos;
	float old_z = z_pos;
	NPC::x_pos = new_x;
	NPC::y_pos = new_y;
	//Yeahlight: NPC is not a boat; we never change a boat's z-axis
	if(this->IsBoat() == false)
	{
		//Yeahlight: Update the mob's z-axis
		if(this->onPath == false)
		{
			float new_z = 0;
			//Yeahlight: Roamers are given such a large z-axis offset due to the randomness of the terrain for zone wide roamers
			if(this->onRoamPath)
				new_z = FindGroundZ(new_x, new_y, 150);
			else
				new_z = FindGroundZ(new_x, new_y, 6);
			//Yeahlight: If we cannot see our new location from our old, then we are dealing with a bogus z-axis value and need to derive a reliable position from interpolation
			if(CheckCoordLos(old_x, old_y, old_z + 3, new_x, new_y, new_z + 3) == false)
			{
				//Yeahlight: Occasionally, an NPC will attempt to follow a PC around a tight corner and get sent into the wall. If the NPC does not have LoS to its target,
				//           undo the location changes and find a path between the two points.
				if(target != 0)
				{
					this->x_pos = old_x;
					this->y_pos = old_y;
					this->z_pos = old_z;
					Node* myNearest = findClosestNode(this);
					//Yeahlight: NPC has at least one pathing node in range
					if(myNearest != NULL)
					{
						Node* theirNearest = findClosestNode(target);
						//Yeahlight: Target has at least one pathing node in range
						if(theirNearest != NULL)
						{
							if(zone->pathsLoaded)
							{
								if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
									target->Message(LIGHTEN_BLUE, "Debug: A path is required between you and %s; using a preprocessed path", this->GetName());
								this->findMyPreprocessedPath(myNearest, theirNearest);
							}
							else
							{
								//
							}
							this->myTargetsX = target->GetX();
							this->myTargetsY = target->GetY();
							this->myTargetsZ = target->GetZ();
						}
						//Yeahlight: Target does not have any pathing nodes in range
						else
						{
							if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
								target->Message(RED, "Debug: Using TP call I");
							this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
						}
					}
					//Yeahlight: NPC does not have any pathing nodes in range
					else
					{
						if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
							target->Message(RED, "Debug: Using TP call J");
						this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
					}
				}
				//Yeahlight: NPC does not have a target, so it must be roaming or returning home
				else
				{
					float test1, test2, test3, test4;
					//Yeahlight: Grab a sample from each cardinal direction with respect to the mob's heading
					test1 = FindGroundZ(new_x + x_step, new_y + y_step, 5);
					test2 = FindGroundZ(new_x + x_step, new_y - y_step, 5);
					test3 = FindGroundZ(new_x - x_step, new_y + y_step, 5);
					test4 = FindGroundZ(new_x - x_step, new_y - y_step, 5);
					float average = (test1 + test2 + test3 + test4) / 4.0;
					if(CheckCoordLos(old_x, old_y, old_z, new_x, new_y, average))
						NPC::z_pos = average;
					else
						TeleportToLocation(org_x, org_y, org_z);
				}
			}
			//Yeahlight: We can see our old location from our new
			else
			{
				NPC::z_pos = new_z;
			}
		}
		//Yeahlight: NPC is on a path
		else
		{
			//Yeahlight: If the NPC's destination is far away, update the z-axis where they stand so ramps and hills do not affect the NPC's vertical positioning
			if(fdistance(x_pos, y_pos, x, y) > 75 || abs(z_pos - (myPathNode->z - 3.7)) > 5)
				z_pos = FindGroundZ(x_pos, y_pos, 6);
		}
	}
}

void NPC::SetDestination(float x, float y, float z)
{
	x_dest = x;
	y_dest = y;
	z_dest = z;
}

void NPC::StartWalking()
{
	walking_timer->Start();
	this->setMoving(true);
}

void NPC::FaceTarget(Mob* MobToFace) 
{
	if (MobToFace == 0)
	{
		MobToFace = target;
	}

	if (MobToFace == 0 || MobToFace == this)
	{
		return;
	}

	float angle;

	if (MobToFace->GetX()-x_pos > 0)
	{
		angle = - 90 + atan((double)(MobToFace->GetY()-y_pos) / (double)(MobToFace->GetX()-x_pos)) * 180 / M_PI;
	}
	else if (MobToFace->GetX()-x_pos < 0)
	{
		angle = + 90 + atan((double)(MobToFace->GetY()-y_pos) / (double)(MobToFace->GetX()-x_pos)) * 180 / M_PI;
	}
	else // Added?
	{
		if (MobToFace->GetY()-y_pos > 0)
		{
			angle = 0;
		}
		else
		{
			angle = 180;
		}
	}

	if (angle < 0)
	{
		angle += 360;
	}
	if (angle > 360)
	{
		angle -= 360;
	}

	int8 startingHeading = heading;
	heading = (sint8) (256*(360-angle)/360.0f);
	//Yeahlight: Only send out updates if the change in heading is significant
	if(abs(startingHeading - heading) > 20)
	{
		SendPosUpdate(false, NPC_UPDATE_RANGE, true);
	}
}

void NPC::RemoveFromHateList(Mob* mob)
{
	bool debugFlag = true;

	appearance = StandingAppearance;

	if(this->IsEngaged())
	{
		hate_list.RemoveEnt(mob);

		// Harakiri check event to turn off combat
		#ifdef EMBPERL
			if(hate_list.IsEmpty())
			{
				perlParser->Event(EVENT_COMBAT, CastToNPC()->GetNPCTypeID(), "0", CastToNPC(), NULL);			
			}
		#endif

		if(mob->IsClient())
		{
			if(mob->CastToClient()->GetFeigned())
			{
				float forgetChance = 0;
				//Yeahlight: Minimum chance to drop agro with a low FD skill: 5%. Maximum chance to drop agro with a high FD skill: 15%
				forgetChance = (float)((float)mob->CastToClient()->GetSkill(FEIGN_DEATH) / (float)2000) + 0.05;
				forgetChance *= 100;
				if(debugFlag && mob && mob->IsClient() && mob->CastToClient()->GetDebugMe())
					mob->Message(LIGHTEN_BLUE, "Debug: Your chance to wipe agro on FD: %2.2f", forgetChance);
				if(rand()%100 <= forgetChance)
				{
					if(debugFlag && mob && mob->IsClient() && mob->CastToClient()->GetDebugMe())
						mob->Message(LIGHTEN_BLUE, "Debug: Your FD successfully wiped agro from %s", this->GetName());
				}
				else
				{
					this->forget_timer->Start(500);
					this->SetFeignMemory(mob->GetName());
					this->forgetchance = 0;
					if(debugFlag && mob && mob->IsClient() && mob->CastToClient()->GetDebugMe())
						mob->Message(LIGHTEN_BLUE, "Debug: Adding you to %s's feign memory", this->GetName());
				}
			}
		}

		if(!this->IsEngaged())
		{
			//Yeahlight: Clear enrage flag
			this->hasEnragedThisFight = false;
			//Yeahlight: Clear rampage list for rampage NPC
			if(this->canRampage)
			{
				for(int i = 0; i < RAMPAGE_LIST_SIZE; i++)
					this->rampageList[i] = 0;
				this->rampageListCounter = 0;
			}
			this->fleeing = false;
			//Yeahlight: Pets do not trigger the go home timer
			if(!this->GetOwner())
			{
				if(!this->isMovingHome)
				{
					//Yeahlight: TODO: Refine this random gohome_timer based on level, con, etc.
					this->gohome_timer->Start(fRandomNum(0.350, 0.999) * 20000);
					this->setMoving(false);
					this->preventPatrolling = true;
				}
				//Yeahlight: NPC was reengaged while it was moving home. NPCs in this scenario NEVER camp the player, 
				//           so send them home immediately.
				else
				{
					this->gohome_timer->Start(100);
					this->setMoving(false);
					this->preventPatrolling = true;
				}
			}
			SendPosUpdate(false, NPC_UPDATE_RANGE, false);
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
	}
}

int32 NPC::CountLoot() 
{
	if (itemlist == 0)
	{
		return 0;
	}
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	int32 count = 0;

	iterator.Reset();
	while(iterator.MoreElements())	
	{
		count++;
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return count;
}

void NPC::DumpLoot(int32 npcdump_index, ZSDump_NPC_Loot* npclootdump, int32* NPCLootindex) 
{
	if (itemlist == 0)
	{
		return;
	}

	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	int32 count = 0;

	iterator.Reset();
	while(iterator.MoreElements())	
	{
		npclootdump[*NPCLootindex].npc_dump_index = npcdump_index;
		npclootdump[*NPCLootindex].itemid = iterator.GetData()->item_nr;
		npclootdump[*NPCLootindex].charges = iterator.GetData()->charges;
		npclootdump[*NPCLootindex].equipSlot = iterator.GetData()->equipSlot;
		(*NPCLootindex)++;
		iterator.RemoveCurrent();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

void NPC::Depop(bool StartSpawnTimer) 
{
	p_depop = true;

	//Yeahlight: NPC had a PC as a pet
	if(this->GetPet() && this->GetPet()->IsClient())
		this->GetPet()->CastToClient()->BuffFadeByEffect(SE_Charm);

	if(this->IsEngaged())
	{
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
	
	if (StartSpawnTimer) 
	{
		if (respawn2 != 0) 
		{
			respawn2->Reset();
		}
	}
}

void NPC::SendTo(float new_x, float new_y) 
{

	float angle;
	float dx = new_x-x_pos;
	float dy = new_y-y_pos;

	// 0.09 is a perfect magic number for a human pnj's
	if(!walking_timer->Enabled()){
		walking_timer->Start((int32)sqrt( dx*dx + dy*dy ));
		x_dest = new_x;
		y_dest = new_y;
	}
	

	if (new_x-x_pos > 0)
	{
		angle = - 90 + atan((double)(new_y-y_pos) / (double)(new_x-x_pos)) * 180 / M_PI;
	}
	else 
	{
		if (new_x-x_pos < 0)	
		{
			angle = + 90 + atan((double)(new_y-y_pos) / (double)(new_x-x_pos)) * 180 / M_PI;
		}
		else
		{ // Added?
			if (new_y-y_pos > 0)
			{
				angle = 0;
			}
			else
			{
				angle = 180;
			}
		}
	}
	if (angle < 0)
	{
		angle += 360;
	}
	if (angle > 360	)
	{
		angle -= 360;
	}

	heading	= 256*(360-angle)/360.0f;
	appearance = UNKNOWN5Appearance;

	float slope = (new_x - x_pos)/(new_y - y_pos);

	x_pos = x_pos + slope;
	y_pos = y_pos + 1;
}

void NPC::MoveTo(float dest_x, float dest_y, float dest_z){

};

float NPC::fdistance(float x1, float y1, float x2, float y2){
	return sqrt(square(x2-x1)+square(y2-y1));
}

float NPC::square(float number){
	return number * number;
}

/********************************************************************
 *                        Tazadar  - 5/31/08                        *
 ********************************************************************
 *                          FaceGuardSpot()                         *                        
 ********************************************************************
 *  + Pet faces his guard spot when coming back to it after  a      *
 *    fight.                                                        *
 *                                                                  *
 ********************************************************************/
void NPC::FaceGuardSpot() 
{
	float angle;

	if (GetGuardX()-x_pos > 0)
	{
		angle = - 90 + atan((double)(GetGuardY()-y_pos) / (double)(GetGuardX()-x_pos)) * 180 / M_PI;
	}
	else if (GetGuardX()-x_pos < 0)
	{
		angle = + 90 + atan((double)(GetGuardY()-y_pos) / (double)(GetGuardX()-x_pos)) * 180 / M_PI;
	}
	else
	{
		if (GetGuardY()-y_pos > 0)
		{
			angle = 0;
		}
		else
		{
			angle = 180;
		}
	}

	if (angle < 0)
	{
		angle += 360;
	}
	if (angle > 360)
	{
		angle -= 360;
	}

	heading = (sint8) (256*(360-angle)/360.0f);
}

//o--------------------------------------------------------------
//| Name: CheckForAggression; Yeahlight, Aug 6, 2008
//o--------------------------------------------------------------
//| Checks for aggression between two NPC pFactions
//o--------------------------------------------------------------
FACTION_VALUE NPC::CheckForAggression(Mob* this_mob, Mob* that_mob)
{
	if(!this_mob || !that_mob)
		return FACTION_INDIFFERENT;

	if(this_mob == that_mob)
		return FACTION_INDIFFERENT;

	if(this_mob->GetOwnerID() || that_mob->GetOwnerID())
		return FACTION_INDIFFERENT;

	FACTION_VALUE factionReturn = FACTION_INDIFFERENT;

	int16 pFactionThis = this_mob->CastToNPC()->GetPrimaryFactionID();
	int16 pFactionThat = that_mob->CastToNPC()->GetPrimaryFactionID();

	if(pFactionThis == pFactionThat)
		return FACTION_ALLY;

	uint32 factionHash = pFactionThis * pFactionThat;

	switch(factionHash)
	{
		case 2737: //LOIO: Sarnak vs Goblin (23 * 119)
			factionReturn = FACTION_SCOWLS;
			break;
		default:
			factionReturn = FACTION_INDIFFERENT;
			break;
	}

	return factionReturn;
}

//o--------------------------------------------------------------
//| setMoving; Cofruben / Yeahlight, June 30, 2008
//o--------------------------------------------------------------
//| Changes an NPC's movement velocity
//o--------------------------------------------------------------
void NPC::setMoving(bool _moving)
{
	bool inMoving = moving;
	sint8 inAnimation = this->GetAnimation();

	//Yeahlight: NPC is a boat
	if(this->IsBoat())
	{
		this->animation = (sint8)(this->GetRunSpeed() * 7 * (int)_moving);
		SendPosUpdate(false, BOAT_UPDATE_RANGE, false);
		moving = _moving;
	}
	//Yeahlight: NPC is not a boat
	else
	{
		//Yeahlight: Last minute check for NPC casting / root
		if(this->isCasting || this->IsRooted())
		{
			moving = false;
			animation = 0;
		}
		//Yeahlight: NPC is not currently casting
		else
		{
			moving = _moving;
			//Yeahlight: NPC was not previously moving, so force a face update on target if necessary
			if(moving && !inMoving && !onPath && !IsDisabled() && !isCasting)
				this->FaceTarget(target);
			//Yeahlight: lowHPSnareRate reflects the speed decrease as a feeling mob loses more and more HP
			float lowHPSnareRate = 1.00f;
			//Yeahlight: NPC is fleeing
			if(this->IsFleeing())
			{
				lowHPSnareRate = 1.00f - (float)(((float)(25 - this->GetHPRatio()) * 4)/100.00f);
				//Yeahlight: The max penalty for low hp is 70%, thus snared mobs can actually stop moving and unsnared mobs may not
				if(lowHPSnareRate < 0.30f)
					lowHPSnareRate = 0.30f;
				else if(lowHPSnareRate > 1.00f)
					lowHPSnareRate = 1.00f;
			}
			//Yeahlight: NPC is engaged and is NOT fleeing (fleeing mobs base movespeed off of walkspeed, NOT runspeed) or the NPC
			//           is a pet and they are currently not moving home to their guard spot
			if((this->IsEngaged() && !this->IsFleeing()) || (this->GetOwner() && !this->movingToGuardSpot))
			{
				//Yeahlight: TODO: This needs tweaked (the 7)
				this->animation = (sint8)(this->GetRunSpeed()  * 7 * (int)_moving) * lowHPSnareRate;
			}
			else
			{
				//Yeahlight: TODO: This needs tweaked (the 4)
				this->animation = (sint8)(this->GetWalkSpeed() * 4 * (int)_moving) * lowHPSnareRate;
			}
			//Yeahlight: Refresh the animation to clients in a small area with the velocity change
			if(this->animation != inAnimation && this->IsFleeing())
				SendPosUpdate(false, NPC_UPDATE_RANGE / 5, false);
		}
	}

	//Yeahlight: Each NPC appearance speed requires 2.3 units per second. We divide by 5 because each update happens every 200ms (200ms * 5 = 1 second)
	this->npc_walking_units_per_second = this->animation * 2.3 / 5;

	//Yeahlight: This emulates the "skiing" of NPCs when they have very little HP remaining
	if(this->IsFleeing() && this->GetHPRatio() < 1 && this->npc_walking_units_per_second < 0.7f && this->appearance != DuckingAppearance && this->GetBodyType() != BT_Humanoid)
	{
		APPLAYER* SpawnPacket = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
		SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)SpawnPacket->pBuffer;
		sa->spawn_id = this->GetID();
		sa->type = SAT_Position_Update;
		sa->parameter = SAPP_Ducking;
		entity_list.QueueCloseClients(this, SpawnPacket, false);
		safe_delete(SpawnPacket);
		appearance = DuckingAppearance;
	}
}

//o--------------------------------------------------------------
//| TeleportToLocation; Yeahlight, Oct 12, 2008
//o--------------------------------------------------------------
//| Teleports an NPC to set of coordinates
//o--------------------------------------------------------------
bool NPC::TeleportToLocation(float x, float y, float z)
{
	bool debugFlag = true;
	int16 delay = 0;
	int16 distance = fdistance(GetX(), GetY(), x, y);
	sint16 unitsPerSecond = (sint8)(this->GetRunSpeed() * 7) * 2.4;
	if(unitsPerSecond < 1)
		unitsPerSecond = 1;
	delay = (distance / unitsPerSecond);
	if(delay * 1000 > 60000)
		delay = 60;
	delay *= 1000;

	this->pathing_teleportation_timer->Start(delay);
	this->stunned_timer->Start(delay);
	this->stunned = true;
	this->onPath = false;
	this->myTargetsX = x;
	this->myTargetsY = y;
	this->myTargetsZ = z;
	if(debugFlag)
		if(target && target->IsClient() && target->CastToClient()->GetDebugMe())
			target->Message(YELLOW, "Debug(WARNING): %s could not find a path to your location. It will teleport to your current location in %i (%i / %i) seconds.", this->GetName(), (delay / 1000), distance, unitsPerSecond);
		else
			cout<<"Debug(WARNING): "<<this->GetName()<<" could not find a path to its destination. It will teleport to its destination in "<<(delay / 1000)<<" ("<<distance<<" / "<<unitsPerSecond<<") seconds."<<endl;
	return true;
}

//o--------------------------------------------------------------
//| GetMyGridPath; Yeahlight, Oct 18, 2008
//o--------------------------------------------------------------
//| Selects a grid path for an NPC (either start to end or end
//| to start)
//o--------------------------------------------------------------
bool NPC::GetMyGridPath()
{
	//Yeahlight: This NPC's gridID does not reference a null grid path
	if(zone->gridsLoaded && zone->zoneGrids[myPathGrid].startID != NULL_NODE && zone->zoneGrids[myPathGrid].endID != NULL_NODE)
	{
		//Yeahlight: NPC is at the start of its grid
		if(myPathNode->nodeID == zone->zoneGrids[myPathGrid].startID)
		{
			if(zone->pathsLoaded)
				findMyPreprocessedPath(zone->thisZonesNodes[zone->zoneGrids[myPathGrid].startID], zone->thisZonesNodes[zone->zoneGrids[myPathGrid].endID]);
			//Yeahlight: NPC found a path, flag it as patrolling
			if(!myPath.empty())
			{
				int32 randomTimer = int32(fRandomNum(0.125, 0.999) * 10000) + 5000;
				this->patrolPause_timer->Start(randomTimer);
				this->isPatrolling = false;
			}
		}
		//Yeahlight: NPC is at the end of its grid
		else
		{
			if(zone->pathsLoaded)
				findMyPreprocessedPath(zone->thisZonesNodes[zone->zoneGrids[myPathGrid].endID], zone->thisZonesNodes[zone->zoneGrids[myPathGrid].startID]);
			//Yeahlight: NPC found a path, flag it as patrolling
			if(!myPath.empty())
			{
				int32 randomTimer = int32(fRandomNum(0.125, 0.999) * 10000) + 5000;
				this->patrolPause_timer->Start(randomTimer);
				this->isPatrolling = false;
			}
		}
		return true;
	}
	return false;
}

//o--------------------------------------------------------------
//| PushTarget; Yeahlight, Nov 14, 2008
//o--------------------------------------------------------------
//| Pushes a mob away from the attacker based on heading
//o--------------------------------------------------------------
bool NPC::CombatPush(Mob* attacker)
{
	sint16 xMod = 0;
	sint16 yMod = 0;
	if(attacker->GetHeading() >= 224 && attacker->GetHeading() <= 255)
		yMod = 1;
	else if(attacker->GetHeading() >= 0 && attacker->GetHeading() < 32)
		yMod = 1;
	else if(attacker->GetHeading() >= 32 && attacker->GetHeading() < 96)
		xMod = 1;
	else if(attacker->GetHeading() >= 96 && attacker->GetHeading() < 160)
		yMod = -1;
	else if(attacker->GetHeading() >= 160 && attacker->GetHeading() < 224)
		xMod = -1;
	if(this->CheckCoordLos(GetX(), GetY(), GetZ(), GetX() + xMod, GetY() + yMod, GetZ()))
	{
		this->x_pos = GetX() + xMod;
		this->y_pos = GetY() + yMod;
		SendPosUpdate(false, NPC_UPDATE_RANGE / 5, true);
		return true;
	}
	return false;
}

//o--------------------------------------------------------------
//| SummonTarget; Yeahlight, Dec 4, 2008
//o--------------------------------------------------------------
//| Summons the NPC's target
//o--------------------------------------------------------------
bool NPC::SummonTarget(Mob* target)
{
	if(!target)
		return false;

	if(target->IsClient())
	{
		target->CastToClient()->MovePC(0, this->GetX(), this->GetY(), this->GetZ() + 5);
	}
	else
	{
		target->SetX(this->GetX());
		target->SetY(this->GetY());
		target->SetZ(this->GetZ());
		target->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
	}
	entity_list.MessageClose(this, true, DEFAULT_COMBAT_MESSAGE_RANGE, WHITE, "You will not evade me, %s!", target->GetName());
	return true;
}

//o--------------------------------------------------------------
//| GetMeleeWeapons; Yeahlight, Dec 16, 2008
//o--------------------------------------------------------------
//| Searches the NPC's loot table for weapons to equip
//o--------------------------------------------------------------
bool NPC::GetMeleeWeapons()
{
	ServerLootItem_Struct* server_item = NULL;
	Item_Struct* item = NULL;
	for(int i = 7; i <= 8; i++)
	{
		server_item = GetItem(i);
		if(server_item)
		{
			item = Database::Instance()->GetItem(server_item->item_nr);
			if(item)
			{
				//Yeahlight: Item fits in the primary slot, equip it
				if(server_item->equipSlot == 7)
					myMainHandWeapon = item;
				//Yeahlight: Item fits in the secondary slot, try to equip it 
				else if(server_item->equipSlot == 8)
				{
					//Yeahlight: The primary slot is filled with a two-handed weapon, so do not allow this off hander to be equiped
					if(myMainHandWeapon && (myMainHandWeapon->common.itemType == DB_2H_SLASHING || myMainHandWeapon->common.itemType == DB_2H_BLUNT || myMainHandWeapon->common.itemType == DB_2H_PIERCING))
						continue;
					//Yeahlight: We do not want to grant raid NPCs dual wield just because they have two or more one-hand weapons
					//           in their loot table. Only allow nodrop shields and other nodrop, non-weapon secondary items to fill 
					//			 the off-hand slot.
					if(item->nodrop == 0 && item->common.damage > 0)
						continue;
					myOffHandWeapon = item;
				}
			}
		}
	}
	//Yeahlight: The order of the loot table may wrongfully fool the server into equiping both a shield and a two-handed weapon.
	//           This final check will remove the off-hand weapon and textures if this ever occurs.
	//			 Note: I also made it impossible for NPCs level 60 and over to dual wield weapons. The reasoning is this: We don't
	//                 want NPCs to spawn with two or more one-handers and have 25-100% more DPS than had they spawned with 1 or
	//                 less one-handers. Also, this makes it impossible for players to grief other guilds by giving allied raid
	//                 targets a vendor weapon to enable dual wield. I cannot think of any situation where we would want high level,
	//                 non-monk NPCs dual wielding.
	if(myMainHandWeapon && ((GetLevel() >= 60 && myOffHandWeapon && myOffHandWeapon->common.damage > 0) || (myMainHandWeapon->common.itemType == DB_2H_SLASHING || myMainHandWeapon->common.itemType == DB_2H_BLUNT || myMainHandWeapon->common.itemType == DB_2H_PIERCING)))
	{
		equipment[8] = 0;
		myOffHandWeapon = NULL;
		SendWearChange();
		return false;
	}
	return true;
}

//o--------------------------------------------------------------
//| GetItemHaste; Yeahlight, Dec 21, 2008
//o--------------------------------------------------------------
//| Searches the NPC's loot table for weapon haste
//o--------------------------------------------------------------
int16 NPC::GetItemHaste() 
{
	int16 highestHasteValue = 0;
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		Item_Struct* item = Database::Instance()->GetItem(iterator.GetData()->item_nr);
		if(item)
		{
			//Yeahlight: Effect ID is "Haste," effect is worn and it is currently the highest haste value
			if(item->common.spell_effect_id == 998 && item->common.effecttype == 2 && item->common.level > highestHasteValue)
			{
				highestHasteValue = item->common.level;
			}	
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return highestHasteValue;
}

//o--------------------------------------------------------------
//| CreateProperName; Yeahlight, Dec 31, 2008
//o--------------------------------------------------------------
//| Sets the NPC's proper name, such as 'a sarnak courier'
//| instead of '#a_sarnak_courier01'
//o--------------------------------------------------------------
void NPC::CreateProperName(char* name)
{
	char tmp[NPC_MAX_NAME_LENGTH] = "";
	int16 tmpCounter = 0;
	for(int i = 0; i < NPC_MAX_NAME_LENGTH; i++)
	{
		//Yeahlight: Do not pass the "special" NPC flag to the proper name
		if(name[i] == '#')
		{
			continue;
		}
		//Yeahlight: Replace underscore with spaces in the proper name
		else if(name[i] == '_')
		{
			tmp[tmpCounter] = ' ';
			tmpCounter++;
		}
		//Yeahlight: Reached the end of the original name; finish current string and bail
		else if(name[i] == '\0')
		{
			tmp[tmpCounter] = '\0';
			break;
		}
		//Yeahlight: Current character is fine, send it to the proper name
		else
		{
			tmp[tmpCounter] = name[i];
			tmpCounter++;
		}
	}
	//Yeahlight: Copy the new string name into the proper name
	strcpy(properName, tmp);
}

//o--------------------------------------------------------------
//| CheckMyFeignMemory; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's feign memory
//o--------------------------------------------------------------
void NPC::CheckMyFeignMemory(bool debugFlag)
{
	Client* remember_client = entity_list.GetClientByName(GetFeignMemory());
	if (remember_client != 0)
	{
		if (!remember_client->CastToClient()->GetFeigned())
		{
			AddToHateList(remember_client,1);
			SetFeignMemory("0");
			forgetchance = 0;
			forget_timer->Disable();
			if(hate_list.GetTop(this->CastToMob()) != 0)
			{
				if(strcmp(remember_client->GetName(), hate_list.GetTop(this->CastToMob())->GetName()) == 0)
				{
					target = hate_list.GetTop(this->CastToMob());
					this->requiresNewPath = true;
					this->onRoamPath = false;
					float distanceToTarget = GetMeleeReach();
					//Yeahlight: NPC is out of melee range of its target
					if(target != 0 && fdistance(target->GetX(), target->GetY(), GetX(), GetY()) > distanceToTarget)
					{
						//Yeahlight: For some reason, the NPC preserves their walkspeed in this situtation; force the runspeed
						this->animation = (sint8)(this->GetRunSpeed() * 7);
						this->npc_walking_units_per_second = this->animation * 2.3 / 5;
					}
				}
			}
		}
		//Yeahlight: A value of 8000 will take, on average, 126 consecutive "forgetchance" increases to pass the following condition
		else if (rand()%8000 <= forgetchance)
		{
			if(debugFlag && remember_client && remember_client->IsClient() && remember_client->GetDebugMe())
				remember_client->Message(LIGHTEN_BLUE, "Debug: %s has forgotten about you!", this->GetName());
			SetFeignMemory("0");
			forgetchance = 0;
			forget_timer->Disable();
		}
		else
		{
			//Yeahlight: 126 increases comes out to 63 seconds (126 * 500ms), which is not long enough. Make it harder to gain a "forgetchance" increase to extend this NPC's memory
			if(rand()%10 == 5)
				forgetchance += 1;
		}
	}
	else
	{
		SetFeignMemory("0");
		forget_timer->Disable();
	}
}

//o--------------------------------------------------------------
//| CheckMyLeashMemory; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's leash memory
//o--------------------------------------------------------------
void NPC::CheckMyLeashMemory(bool debugFlag)
{
	Client* leash_client = entity_list.GetClientByName(GetLeashMemory());
	//Yeahlight: Leashed target is still in the zone
	if(leash_client != 0)
	{
		//Yeahlight: Distance to leashed target is less than 750 units
		if(fdistance(GetX(), GetY(), leash_client->GetX(), leash_client->GetY()) < 750)
		{
			//Yeahlight: NPC is currently engaged, drop the leash agro
			if(hate_list.GetTop(this->CastToMob()) != 0)
			{
				if(debugFlag && leash_client && leash_client->IsClient() && leash_client->GetDebugMe())
					leash_client->Message(LIGHTEN_BLUE, "Debug: You are back in %s's territory, but it is currently engaged by another party; removing you from %s's memory", this->GetName(), this->GetName());
				SetLeashMemory("0");
				leash_timer->Disable();
			}
			//Yeahlight: NPC is not currently engaged, add it to hate list and get moving towards target
			else
			{
				if(debugFlag && leash_client && leash_client->IsClient() && leash_client->GetDebugMe())
					leash_client->Message(LIGHTEN_BLUE, "Debug: You are back in %s's territory; %s is persuing you once again", this->GetName(), this->GetName());
				AddToHateList(leash_client,1);
				SetLeashMemory("0");
				leash_timer->Disable();
				faceDestination(leash_client->GetX(), leash_client->GetY());
				SetDestination(leash_client->GetX(), leash_client->GetY(), leash_client->GetZ());
				StartWalking();
				walking_timer->Trigger();
				SendPosUpdate(false, NPC_UPDATE_RANGE, false);
			}
		}
	}
	else
	{
		SetLeashMemory("0");
		leash_timer->Disable();
	}
}

//o--------------------------------------------------------------
//| CheckMyDepopStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's depop status
//o--------------------------------------------------------------
void NPC::CheckMyDepopStatus()
{
	Mob* owner = entity_list.GetMob(this->ownerid);
	if (owner != 0)
	{
		owner->SetPetID(0);
		this->SetPetID(0);
		this->SetOwnerID(0);
	}
}

//o--------------------------------------------------------------
//| CheckMyEnrageStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's enraged status
//o--------------------------------------------------------------
void NPC::CheckMyEnrageStatus()
{
	this->enraged = false;
	enrage_timer->Disable();
	entity_list.MessageClose(this, true, DEFAULT_COMBAT_MESSAGE_RANGE, RED, "%s is no longer enraged", this->GetProperName());
}

//o--------------------------------------------------------------
//| DoMyTeleport; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| TPs the NPC to its target
//o--------------------------------------------------------------
void NPC::DoMyTeleport()
{
	this->x_pos = this->myTargetsX;
	this->y_pos = this->myTargetsY;
	this->z_pos = this->myTargetsZ;
	this->pathing_teleportation_timer->Disable();
	SendPosUpdate(false);
}

//o--------------------------------------------------------------
//| CheckMyNodeFocusedStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's node focused status
//o--------------------------------------------------------------
void NPC::CheckMyNodeFocusedStatus()
{
	if(!target)
	{
		nodeFocused = false;
		nodeFocused_timer->Disable();
	}
	if(target && CheckCoordLosNoZLeaps(GetX(), GetY(), GetZ(), target->GetX(), target->GetY(), target->GetZ()))
	{
		nodeFocused = false;
		nodeFocused_timer->Disable();
	}
}

//o--------------------------------------------------------------
//| CheckMyDebuffRefundStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's debuff refund status
//o--------------------------------------------------------------
void NPC::CheckMyDebuffRefundStatus(bool engaged)
{
	//Yeahlight: NPC is engaged
	if(engaged)
	{
		//Yeahlight: Engaged NPC has a 7.5% chance to regen debuff credit in battle
		if(rand()%1000 < 75)
		{
			if(--mySpellCounter < 0)
				mySpellCounter = 0;
		}
	}
	else
	{
		//Yeahlight: Unengaged NPC has a 15% chance to regen debuff credit out of battle
		if(rand()%1000 < 150)
		{
			if(--mySpellCounter < 0)
				mySpellCounter = 0;
		}
	}
}

//o--------------------------------------------------------------
//| CheckMyFleeStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's flee status
//o--------------------------------------------------------------
void NPC::CheckMyFleeStatus(bool debugFlag)
{
	Node* myNearest = findClosestNode(this);
	//Yeahlight: NPC has at least one pathing node in range
	if(myNearest != NULL)
	{
		int16 counter = 0;
		Node* myFleeingNode = NULL;
		//Yeahlight: Loop through this random choice until the start and end nodes do not match
		do
		{
			myFleeingNode = zone->thisZonesNodes[rand()%(zone->numberOfNodes)];
			//Yeahlight: Zone freeze debug
			if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
				EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		} 
		while(myNearest == myFleeingNode && counter++ < 6);
		//Yeahlight: Target has at least one pathing node in range
		if(myFleeingNode != NULL)
		{
			//Yeahlight: Find path between NPC and target
			if(zone->pathsLoaded)
			{
				if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
					target->Message(LIGHTEN_BLUE, "Debug: A path is required between %s and its flee destination; using a preprocessed path", this->GetName());
				this->findMyPreprocessedPath(myNearest, myFleeingNode);
				revertFlee_timer->Trigger();
				return;
			}
		}
	}
	//Yeahlight: Target has no pathing nodes in range and has not been blocked from finding paths, so use random location roaming mechanics
	int loop = 0;
	bool permit = false;
	float ranx, rany, ranz;
	float x = GetX();
	float y = GetY();
	float z = GetZ();
	float offset = 2;
	//Yeahlight: Roamers are given a large offset for hills and such
	if(this->isRoamer)
		offset = 150;
	while(loop < 100)
	{
		int ran = 250 - (loop*2);
		loop++;
		ranx = x + rand()%ran - rand()%ran;
		rany = y + rand()%ran - rand()%ran;
		ranz = FindGroundZ(ranx, rany, offset);
		if(ranz == -999999)
			continue;
		float fdist = abs(ranz - z);
		if(fdist <= 12 && CheckCoordLosNoZLeaps(x, y, z, ranx, rany, ranz))
		{
			permit = true;
			break;
		}
	}
	//Yeahlight: The NPC found reasonable values to flee towards
	if(permit)
	{
		//Yeahlight: Assign our new, approved values
		faceDestination(ranx, rany);
		SetDestination(ranx, rany, ranz);
		StartWalking();
		walking_timer->Trigger();
		SendPosUpdate(false, NPC_UPDATE_RANGE * 2.5, false);
		onFleePath = true;
		revertFlee_timer->Trigger();
	}
	//Yeahlight: The NPC did not find reasonable values to roam towards
	else
	{
		//Yeahlight: NPC is not a roamer and has failed to find a roam path, so block it from further roam path consideration
		blockFlee = true;
		onFleePath = false;
		//Yeahlight: TODO: Handle this situation
	}
}

//o--------------------------------------------------------------
//| CheckMyPatrolStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's patrol status
//o--------------------------------------------------------------
void NPC::CheckMyPatrolStatus()
{
	if(!myPathNode)
		return;
	this->patrolPause_timer->Disable();
	this->isPatrolling = true;
	faceDestination(myPathNode->x, myPathNode->y);
	SetDestination(myPathNode->x, myPathNode->y, myPathNode->z);
	StartWalking();
	walking_timer->Trigger();
	SendPosUpdate(false, NPC_UPDATE_RANGE * 1.5, false);
}

//o--------------------------------------------------------------
//| CheckMyRoamStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's roam status
//o--------------------------------------------------------------
void NPC::CheckMyRoamStatus()
{
	//Yeahlight: Reset timer to random range between 20 and 60 seconds
	int32 randomTimer = int32(fRandomNum(0.125, 0.999) * 40000) + 20000;
	this->roam_timer->Start(randomTimer);
	float tempX, tempY, tempZ;
	int abortCounter = 0;
	bool permit = false;
	//Yeahlight: NPC is currently outside of their roambox, so, instead of roaming, lets send it home
	if(this->GetX() > zone->roamBoxes[this->myRoamBox].max_x || this->GetX() < zone->roamBoxes[this->myRoamBox].min_x || this->GetY() > zone->roamBoxes[this->myRoamBox].max_y || this->GetY() < zone->roamBoxes[this->myRoamBox].min_y)
	{
		faceDestination(this->org_x, this->org_y);
		SetDestination(this->org_x, this->org_y, this->org_z);
		StartWalking();
		walking_timer->Trigger();
		SendPosUpdate(false, NPC_UPDATE_RANGE * 2.5, false);
		onRoamPath = true;
	}
	//Yeahlight: NPC is still inside their roambox; proceed
	else
	{
		//Yeahlight: While we have no reasonable roaming destination, continue to loop until we find one or we run out of tries
		do
		{
			//Yeahlight: Calculate new x, y & z values
			tempX = GetX() + ((rand() % (roamRange * 2)) - roamRange);
			tempY = GetY() + ((rand() % (roamRange * 2)) - roamRange);
			tempZ = FindGroundZ(tempX, tempY, 150);
			//Yeahlight: Temp values are within this NPC's roam box
			if(tempX < zone->roamBoxes[this->myRoamBox].max_x && tempX > zone->roamBoxes[this->myRoamBox].min_x && tempY < zone->roamBoxes[this->myRoamBox].max_y && tempY > zone->roamBoxes[this->myRoamBox].min_y)
			{
				//Yeahlight: Distance traveled is at least one third of their roam range. Any value is fine if this is a zone wide roamer
				if(roamRange == ZONE_WIDE_ROAM || fdistance(tempX, tempY, GetX(), GetY() >= (roamRange/3.0)))
				{
					//Yeahlight: Old location has elevated LoS to new location
					if(CheckCoordLos(tempX, tempY, tempZ + 5, GetX(), GetY(), GetZ() + 75))
					{
						permit = true;
					}
				}
			}
			abortCounter++;
			//Yeahlight: Zone freeze debug
			if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
				EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		} 
		while(!permit && abortCounter < 33);

		//Yeahlight: The NPC found reasonable values to roam towards
		if(permit)
		{
			//Yeahlight: Assign our new, approved values
			faceDestination(tempX, tempY);
			SetDestination(tempX, tempY, tempZ);
			StartWalking();
			walking_timer->Trigger();
			SendPosUpdate(false, NPC_UPDATE_RANGE * 2.5, false);
			onRoamPath = true;
		}
		//Yeahlight: The NPC did not find reasonable values to roam towards
		else
		{
			onRoamPath = false;
			//Yeahlight: TODO: Log this to find bugged/excluded spawns
		}
	}
}

//o--------------------------------------------------------------
//| CheckMyAgroStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's agro status
//o--------------------------------------------------------------
void NPC::CheckMyAgroStatus()
{
	//Yeahlight: Reset timer due to random start value
	scanarea_timer->Start(1250);
	//Yeahlight: Agro radius calculation (145 +/- spell mods)
	const float AggroRadius = BASE_AGRO_RANGE - this->spellbonuses.FrenzyRadius * this->spellbonuses.FrenzyRadius;
	if(entity_list.AddHateToCloseMobs(this, AggroRadius, 0))
	{
		//Yeahlight: Undead continue to agro after establishing agro
		if(this->IsEngaged() == false)
			zone->AddAggroMob();
	}
}

//o--------------------------------------------------------------
//| CheckMyDefenseCastStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's defensive cast status
//o--------------------------------------------------------------
void NPC::CheckMyDefenseCastStatus()
{
	//Yeahlight: Chance to cast a saving spell on an ally (base chance of 40%)
	int16 chance = (1000 - (1000 * mySpellCounter/(this->GetLevel() / 10 + 3))) + 400;
	//Yeahlight: NPC passed the condition to aid a potential ally
	if(rand()%1000 < chance)
	{
		bool permit = false;
		int16 myAlly = 0;
		//Yeahlight: First, make sure an NPC with the same pFaction is in the area
		for(int i = 0; i < zone->zoneAgroCounter; i++)
		{
			Mob* mob = entity_list.GetMob(zone->zoneAgro[i]);
			//Yeahlight: NPCs check to heal themselves in attack.cpp
			if(mob == NULL || !mob->IsNPC() || this->GetID() == mob->GetID())
				continue;
			//Yeahlight: The potential NPC exists and is in trouble
			if(mob->GetHPRatio() <= 25 && this->GetPrimaryFactionID() == mob->CastToNPC()->GetPrimaryFactionID())
			{
				//Yeahlight: NPC is in range to aid their ally (Note: NPCs do NOT require LoS to cast a heal; they may heal through walls)
				if(fdistance(GetX(), GetY(), mob->GetX(), mob->GetY()) <= NPC_BUFF_RANGE)
				{
					permit = true;
					myAlly = mob->GetID();
				}
			}
		}
		//Yeahlight: This NPC has found an ally to aid with a spell
		if(permit)
		{
			this->CastSpell(spells_handler.GetSpellPtr(this->myBuffs[HEAL]), myAlly);
			this->mySpellCounter++;
		}
	}
}

//o--------------------------------------------------------------
//| CheckMyOffenseCastStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's offensive cast status
//o--------------------------------------------------------------
void NPC::CheckMyOffenseCastStatus()
{
	int16 chance = (1000 - (1000 * mySpellCounter/(this->GetLevel() / 10 + 3)));
	//Yeahlight: If this NPC has an empty counter, give them an excellent chance to cast (most likely just pulled),
	//           otherwise, cut the chance in half (leaving the chance at 100% was triggering spells too often).
	if(mySpellCounter == 0)
		chance = chance * 0.85;
	else
		chance = chance * 0.50;
	//Yeahlight: Percent chance to cast a debuff is derived from the number of debuffs this NPC has casted already
	if(rand()%1000 < chance)
	{
		//Yeahlight: Grab a random target on the hate list
		Mob* victim = hate_list.GetRandom();
		//Yeahlight: Victim exists, is within 350 damage and the NPC can see the victim with z-axis restricted LoS
		if(victim && fdistance(GetX(), GetY(), victim->GetX(), victim->GetY()) <= 350 && CheckCoordLosNoZLeaps(GetX(), GetY(), GetZ(), victim->GetX(), victim->GetY(), victim->GetZ()))
		{
			bool spellFound = false;
			int16 randomDebuff = 0;
			int16 counter = 0;
			Spell* spellToCast = NULL;
			//Yeahlight: Search this NPC's debuffs
			while(!spellFound && counter++ <= 8)
			{
				randomDebuff = rand()%4;
				//Yeahlight: This debuff slot is "null"
				if(this->myDebuffs[randomDebuff] == NULL_SPELL)
					continue;
				spellToCast = spells_handler.GetSpellPtr(this->myDebuffs[randomDebuff]);
				bool permit = true;
				if(spellToCast)
				{
					//Yeahlight: Iterate through the buff slots of the victim
					for(int j = 0; j < 15; j++)
					{
						if(victim->buffs[j].spell && victim->buffs[j].spell->IsValidSpell() && victim->buffs[j].spell->GetSpellID() == spellToCast->GetSpellID())
						{
							permit = false;
							break;
						}
					}
					//Yeahlight: This is a valid spell to cast
					if(permit)
					{
						this->CastSpell(spellToCast, victim->GetID());
						spellFound = true;
						mySpellCounter++;
					}
				}
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
		}
	}
}

//o--------------------------------------------------------------
//| CheckMyLosStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's LoS status to target
//o--------------------------------------------------------------
void NPC::CheckMyLosStatus(bool debugFlag)
{
	if(!target)
		return;
	//Yeahlight: NPC is too far away from its target; drop agro
	if(fdistance(GetX(), GetY(), target->GetX(), target->GetY()) > NPC_LEASH_DISTANCE)
	{
		if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
			target->Message(LIGHTEN_BLUE, "Debug: You are more than 3000 range away from %s; dropping agro", this->GetName());
		this->RemoveFromHateList(target);
		//Yeahlight: Preventing a null target crash resulting from RemoveFromHateList()
		if(target)
		{
			if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
				target->Message(LIGHTEN_BLUE, "Debug: Adding you to %s's leash memory", this->GetName());
			this->SetLeashMemory(target->GetName());
			this->leash_timer->Trigger();
		}
	}
	//Yeahlight: NPC is on a specific path
	else if(this->onPath)
	{
		//Yeahlight: Target has moved a significant distance
		if(abs(this->myTargetsX - target->GetX()) > 65 || abs(this->myTargetsY - target->GetY()) > 65 || requiresNewPath)
		{
			if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
				target->Message(LIGHTEN_BLUE, "Debug: %s notices your change in location; updating its path", this->GetName());
			//Yeahlight: No need to scan the zone over again, use current pathing node
			Node* myNearest = this->myPathNode;
			//Yeahlight: NPC has at least one pathing node in range
			if(myNearest != NULL)
			{
				Node* theirNearest = findClosestNode(target);
				//Yeahlight: Target has at least one pathing node in range
				if(theirNearest != NULL)
				{
					//Yeahlight: Find path between NPC and target
					if(zone->pathsLoaded)
					{
						if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
							target->Message(LIGHTEN_BLUE, "Debug: A path is required between you and %s; using a preprocessed path", this->GetName());
						this->findMyPreprocessedPath(myNearest, theirNearest);
						requiresNewPath = false;
					}
					else
					{
						//
					}
					this->myTargetsX = target->GetX();
					this->myTargetsY = target->GetY();
					this->myTargetsZ = target->GetZ();
				}
				//Yeahlight: Target has no pathing nodes in range
				else
				{
					//Yeahlight: Preproceesed paths are loaded
					if(zone->pathsLoaded)
					{
						bool debug = true;
						if(debug)
							target->Message(LIGHTEN_BLUE, "Debug: %s is near a pathing node, but you are not", this->GetName());
						//Yeahlight: NPC is near a node, but the PC is not. This situation should only happen in outdoor zones
						//           when a PC leaves a minidungeon.
						if(myNearest != NULL)
						{
							//Yeahlight: First, check to see if this PC can see the same node the NPC sees
							if(CheckCoordLos(target->GetX(), target->GetY(), target->GetZ() + 100, myNearest->x, myNearest->y, myNearest->z))
							{
								if(debug)
									target->Message(LIGHTEN_BLUE, "Debug: You and %s share a common pathing node; running towards it now", this->GetName());
								this->faceDestination(myNearest->x, myNearest->y);
								this->SetDestination(myNearest->x, myNearest->y, myNearest->z);
								this->StartWalking();
								this->SetNWUPS(this->GetAnimation() * 2.3 / 5);
								this->MoveTowards(x_dest, y_dest, this->CastToNPC()->GetNWUPS());
								this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
								this->onPath = false;
								this->nodeFocused = true;
								nodeFocused_timer->Trigger();
							}
							else
							{
								if(debug)
									target->Message(LIGHTEN_BLUE, "Debug: You and %s do not share the same pathing node; finding another node", this->GetName());
								bool nodeFound = false;
								Node* nodeToUse = NULL;
								Node* tempZoneNode = NULL;
								int16 tempNode = 0;
								int16 myNodeID = myNearest->nodeID;
								int16 doNotRecheck[MAXZONENODES] = {0};
								int16 doNotRecheckCounter = 0;
								int16 distance = fdistance(GetX(), GetY(), target->GetX(), target->GetY()) * 2;
								if(distance > 650)
									distance = 650;
								//Yeahlight: We need to find a node that the PC can see
								for(int i = 0; i < zone->numberOfNodes; i++)
								{
									//Yeahlight: A Node has been found, break out of the loop
									if(nodeFound)
										break;
									//Yeahlight: Never examine identical nodes
									if(i == myNodeID)
										continue;
									//Yeahlight: Iterate through all the path nodes of each path
									for(int k = 0; k < MAX_PATH_SIZE; k++)
									{
										bool permit = true;
										tempNode = zone->zonePaths[myNodeID][i].path[k];
										//Yeahlight: At the end of the path data
										if(tempNode == NULL_NODE)
											break;
										//Yeahlight: We already checked this node above
										if(tempNode == myNodeID)
											continue;
										//Yeahlight: Iterate through the check array and continue if we have already considered this node
										for(int m = 0; m < doNotRecheckCounter; m++)
										{
											if(doNotRecheck[m] == tempNode)
											{
												permit = false;
												break;
											}
										}
										//Yeahlight: We have already considered this node; continue iterating
										if(!permit)
											continue;
										tempZoneNode = zone->thisZonesNodes[tempNode];
										//Yeahlight: Node is too far away from the NPC to consider or this PC has does not have elevated LoS to the node
										if(fdistance(GetX(), GetY(), tempZoneNode->x, tempZoneNode->y) > distance || !CheckCoordLos(tempZoneNode->x, tempZoneNode->y, tempZoneNode->z, target->GetX(), target->GetY(), target->GetZ() + 100))
										{
											bool permitTwo = true;
											//Yeahlight: Array is empty, get it started with this nodeID
											if(doNotRecheckCounter < 1)
											{
												doNotRecheck[0] = tempNode;
												doNotRecheckCounter++;
											}
											else
											{
												//Yeahlight: Iterate through the check array
												for(int j = 0; j < doNotRecheckCounter; j++)
												{
													//Yeahlight: This nodeID is already in the array; prevent it from getting added once more
													if(doNotRecheck[j] == tempNode)
													{
														permitTwo = false;
														break;
													}
												}
											}
											//Yeahlight: This nodeID is not in the array; add it
											if(permitTwo)
											{
												doNotRecheck[doNotRecheckCounter] = tempNode;
												doNotRecheckCounter++;
											}
											continue;
										}
										//Yeahlight: NPC passed all conditions; use this node
										nodeToUse = tempZoneNode;
										nodeFound = true;
										break;
									}
								}
								if(nodeFound)
								{
									if(debug)
										target->Message(LIGHTEN_BLUE, "Debug: A destination node has been found for %s; resolving a path to this NodeID: %i", this->GetName(), nodeToUse->nodeID);
									this->findMyPreprocessedPath(myNearest, nodeToUse);
									this->myTargetsX = target->GetX();
									this->myTargetsY = target->GetY();
									this->myTargetsZ = target->GetZ();
									this->nodeFocused = true;
									requiresNewPath = false;
									nodeFocused_timer->Trigger();
								}
								else
								{
									if(debug)
										target->Message(RED, "Debug: Using TP call A");
									this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
								}
							}
						}
					}
				}
			}
			//Yeahlight: NPC has no pathing nodes in range
			else
			{
				if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())				
					target->Message(RED, "Debug: Using TP call B");
				this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
			}
		}
		//Yeahlight: NPC is within a reasonable range of target to start looking for LoS
		if(fdistance(GetX(), GetY(), target->GetX(), target->GetY()) < NPC_LOS_SEARCH_RANGE)// && abs(GetZ() - target->GetZ()) <= 12)
		{
			//Yeahlight: NPC has sight of target
			if(CheckCoordLosNoZLeaps(GetX(), GetY(), GetZ(), target->GetX(), target->GetY(), target->GetZ()))
			{
				if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
					target->Message(LIGHTEN_BLUE, "Debug: %s can now see you; abandoning path and rushing you", this->GetName());
				this->onPath = false;
				faceDestination(target->GetX(), target->GetY());
				SetDestination(target->GetX(), target->GetY(), target->GetZ());
				StartWalking();
				walking_timer->Trigger();
				this->z_pos = FindGroundZ(GetX(), GetY(), 6);
				this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
			}
		}
	}
	//Yeahlight: NPC is not on a specific path, but is running towards the target
	else
	{
		//Yeahlight: We cannot see our target from our current location with a LoS check
		if(!CheckCoordLosNoZLeaps(GetX(), GetY(), GetZ() + 2, target->GetX(), target->GetY(), target->GetZ()))
		{
			if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
				target->Message(LIGHTEN_BLUE, "Debug: %s failed a basic LoS check; finding path", this->GetName());
			Node* myNearest = findClosestNode(this);
			//Yeahlight: NPC has at least one pathing node in range
			if(myNearest != NULL)
			{
				Node* theirNearest = findClosestNode(target);
				//Yeahlight: Target has at least one pathing node in range
				if(theirNearest != NULL)
				{
					//Yeahlight: Find path between NPC and target
					if(zone->pathsLoaded)
					{
						if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
							target->Message(LIGHTEN_BLUE, "Debug: A path is required between you and %s; using a preprocessed path", this->GetName());
						this->findMyPreprocessedPath(myNearest, theirNearest);
					}
					else
					{
						//
					}
					this->myTargetsX = target->GetX();
					this->myTargetsY = target->GetY();
					this->myTargetsZ = target->GetZ();
				}
				//Yeahlight: Target has no pathing nodes in range
				else
				{
					//Yeahlight: Preproceesed paths are loaded
					if(zone->pathsLoaded)
					{
						if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
							target->Message(LIGHTEN_BLUE, "Debug: %s is near a pathing node, but you are not", this->GetName());
						//Yeahlight: NPC is near a node, but the PC is not. This situation should only happen in outdoor zones
						//           when a PC leaves a minidungeon.
						if(myNearest != NULL)
						{
							//Yeahlight: First, check to see if this PC can see the same node the NPC sees
							if(CheckCoordLos(target->GetX(), target->GetY(), target->GetZ() + 100, myNearest->x, myNearest->y, myNearest->z))
							{
								if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
									target->Message(LIGHTEN_BLUE, "Debug: You and %s share a common pathing node; running towards it now", this->GetName());
								this->faceDestination(myNearest->x, myNearest->y);
								this->SetDestination(myNearest->x, myNearest->y, myNearest->z);
								this->StartWalking();
								this->SetNWUPS(this->GetAnimation() * 2.3 / 5);
								this->MoveTowards(x_dest, y_dest, this->CastToNPC()->GetNWUPS());
								this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
								this->onPath = false;
								this->nodeFocused = true;
								nodeFocused_timer->Trigger();
							}
							else
							{
								if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
									target->Message(LIGHTEN_BLUE, "Debug: You and %s do not share the same pathing node; finding another node", this->GetName());
								bool nodeFound = false;
								Node* nodeToUse = NULL;
								Node* tempZoneNode = NULL;
								int16 tempNode = 0;
								int16 myNodeID = myNearest->nodeID;
								int16 doNotRecheck[MAXZONENODES] = {0};
								int16 doNotRecheckCounter = 0;
								int16 distance = fdistance(GetX(), GetY(), target->GetX(), target->GetY()) * 2;
								if(distance > 650)
									distance = 650;
								//Yeahlight: We need to find a node that the PC can see
								for(int i = 0; i < zone->numberOfNodes; i++)
								{
									//Yeahlight: A Node has been found, break out of the loop
									if(nodeFound)
										break;
									//Yeahlight: Never examine identical nodes
									if(i == myNodeID)
										continue;
									//Yeahlight: Iterate through all the path nodes of each path
									for(int k = 0; k < MAX_PATH_SIZE; k++)
									{
										bool permit = true;
										tempNode = zone->zonePaths[myNodeID][i].path[k];
										//Yeahlight: At the end of the path data
										if(tempNode == NULL_NODE)
											break;
										//Yeahlight: We already checked this node above
										if(tempNode == myNodeID)
											continue;
										//Yeahlight: Iterate through the check array and continue if we have already considered this node
										for(int m = 0; m < doNotRecheckCounter; m++)
										{
											if(doNotRecheck[m] == tempNode)
											{
												permit = false;
												break;
											}
										}
										//Yeahlight: We have already considered this node; continue iterating
										if(!permit)
											continue;
										tempZoneNode = zone->thisZonesNodes[tempNode];
										//Yeahlight: Node is too far away from the NPC to consider or this PC has does not have elevated LoS to the node
										if(fdistance(GetX(), GetY(), tempZoneNode->x, tempZoneNode->y) > distance || !CheckCoordLos(tempZoneNode->x, tempZoneNode->y, tempZoneNode->z, target->GetX(), target->GetY(), target->GetZ() + 100))
										{
											bool permitTwo = true;
											//Yeahlight: Array is empty, get it started with this nodeID
											if(doNotRecheckCounter < 1)
											{
												doNotRecheck[0] = tempNode;
												doNotRecheckCounter++;
											}
											else
											{
												//Yeahlight: Iterate through the check array
												for(int j = 0; j < doNotRecheckCounter; j++)
												{
													//Yeahlight: This nodeID is already in the array; prevent it from getting added once more
													if(doNotRecheck[j] == tempNode)
													{
														permitTwo = false;
														break;
													}
												}
											}
											//Yeahlight: This nodeID is not in the array; add it
											if(permitTwo)
											{
												doNotRecheck[doNotRecheckCounter] = tempNode;
												doNotRecheckCounter++;
											}
											continue;
										}
										//Yeahlight: NPC passed all conditions; use this node
										nodeToUse = tempZoneNode;
										nodeFound = true;
										break;
									}
								}
								if(nodeFound)
								{
									if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
										target->Message(LIGHTEN_BLUE, "Debug: A destination node has been found for %s; resolving a path to this NodeID: %i", this->GetName(), nodeToUse->nodeID);
									this->findMyPreprocessedPath(myNearest, nodeToUse);
									this->myTargetsX = target->GetX();
									this->myTargetsY = target->GetY();
									this->myTargetsZ = target->GetZ();
									this->nodeFocused = true;
									nodeFocused_timer->Trigger();
								}
								else
								{
									if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
										target->Message(RED, "Debug: Using TP call C");
									this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
								}
							}
						}
					}
				}
			}
			//Yeahlight: NPC has no pathing nodes in range
			else
			{
				//Yeahlight: Preproceesed paths are loaded
				if(zone->pathsLoaded)
				{
					bool debug = true;
					Node* theirNearest = findClosestNode(target);
					//Yeahlight: Target is near a node, but the NPC is not. This situation should only happen in outdoor zones
					//           when a PC cuts a tight corner, goes into a mini dungeon, etc.
					if(theirNearest != NULL)
					{
						if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
							target->Message(LIGHTEN_BLUE, "Debug: You are near a pathing node, but %s is not", this->GetName());
						//Yeahlight: First, check to see if this NPC can see the same node the PC sees
						if(CheckCoordLos(GetX(), GetY(), GetZ() + 100, theirNearest->x, theirNearest->y, theirNearest->z))
						{
							if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
								target->Message(LIGHTEN_BLUE, "Debug: You and %s share a common pathing node; running towards it now", this->GetName());
							this->faceDestination(theirNearest->x, theirNearest->y);
							this->SetDestination(theirNearest->x, theirNearest->y, theirNearest->z);
							this->StartWalking();
							this->SetNWUPS(this->GetAnimation() * 2.3 / 5);
							this->MoveTowards(x_dest, y_dest, this->CastToNPC()->GetNWUPS());
							this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
							this->onPath = false;
							this->nodeFocused = true;
							nodeFocused_timer->Trigger();
						}
						else
						{
							if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
								target->Message(LIGHTEN_BLUE, "Debug: You and %s do not share the same pathing node; finding another node", this->GetName());
							bool nodeFound = false;
							Node* nodeToUse = NULL;
							Node* tempZoneNode = NULL;
							int16 tempNode = 0;
							int16 theirNodeID = theirNearest->nodeID;
							int16 doNotRecheck[MAXZONENODES] = {0};
							int16 doNotRecheckCounter = 0;
							int16 distance = fdistance(GetX(), GetY(), target->GetX(), target->GetY()) * 2;
							if(distance > 650)
								distance = 650;
							//Yeahlight: We need to find a node that the NPC can see
							for(int i = 0; i < zone->numberOfNodes; i++)
							{
								//Yeahlight: A Node has been found, break out of the loop
								if(nodeFound)
									break;
								//Yeahlight: Never examine identical nodes
								if(i == theirNodeID)
									continue;
								//Yeahlight: Iterate through all the path nodes of each path
								for(int k = 0; k < MAX_PATH_SIZE; k++)
								{
									bool permit = true;
									tempNode = zone->zonePaths[theirNodeID][i].path[k];
									//Yeahlight: At the end of the path data
									if(tempNode == NULL_NODE)
										break;
									//Yeahlight: We already checked this node above
									if(tempNode == theirNodeID)
										continue;
									//Yeahlight: Iterate through the check array and continue if we have already considered this node
									for(int m = 0; m < doNotRecheckCounter; m++)
									{
										if(doNotRecheck[m] == tempNode)
										{
											permit = false;
											break;
										}
									}
									//Yeahlight: We have already considered this node; continue iterating
									if(!permit)
										continue;
									tempZoneNode = zone->thisZonesNodes[tempNode];
									//Yeahlight: Node is too far away from the target to consider or this NPC has does not have elevated LoS to the node
									if(fdistance(target->GetX(), target->GetY(), tempZoneNode->x, tempZoneNode->y) > distance || !CheckCoordLos(tempZoneNode->x, tempZoneNode->y, tempZoneNode->z, GetX(), GetY(), GetZ() + 100))
									{
										bool permitTwo = true;
										//Yeahlight: Array is empty, get it started with this nodeID
										if(doNotRecheckCounter < 1)
										{
											doNotRecheck[0] = tempNode;
											doNotRecheckCounter++;
										}
										else
										{
											//Yeahlight: Iterate through the check array
											for(int j = 0; j < doNotRecheckCounter; j++)
											{
												//Yeahlight: This nodeID is already in the array; prevent it from getting added once more
												if(doNotRecheck[j] == tempNode)
												{
													permitTwo = false;
													break;
												}
											}
										}
										//Yeahlight: This nodeID is not in the array; add it
										if(permitTwo)
										{
											doNotRecheck[doNotRecheckCounter] = tempNode;
											doNotRecheckCounter++;
										}
										continue;
									}
									//Yeahlight: NPC passed all conditions; use this node
									nodeToUse = tempZoneNode;
									nodeFound = true;
									break;
								}
							}
							if(nodeFound)
							{
								if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
									target->Message(LIGHTEN_BLUE, "Debug: A new pathing node has been found for %s; running towards this NodeID: %i", this->GetName(), nodeToUse->nodeID);
								this->faceDestination(nodeToUse->x, nodeToUse->y);
								this->SetDestination(nodeToUse->x, nodeToUse->y, nodeToUse->z);
								this->StartWalking();
								this->SetNWUPS(this->GetAnimation() * 2.3 / 5);
								this->MoveTowards(x_dest, y_dest, this->CastToNPC()->GetNWUPS());
								this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
								this->onPath = false;
								this->nodeFocused = true;
								nodeFocused_timer->Trigger();
							}
							else
							{
								if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
									target->Message(RED, "Debug: Using TP call D");
								this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
							}
						}
					}
					//Yeahlight: Both NPC and target do not have pathing nodes in range
					else
					{
						int16 modifier = 1;
						if(target->IsClient())
							modifier = 10;
						//Yeahlight: The NPC and target cannot see each other on two elevated planes
						if(!CheckCoordLos(GetX(), GetY(), GetZ() + 2500, target->GetX(), target->GetY(), target->GetZ() * modifier + 2500))
						{
							if(!CheckCoordLos(GetX(), GetY(), GetZ() + 500, target->GetX(), target->GetY(), target->GetZ() * modifier + 500))
							{
								if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
									target->Message(RED, "Debug: Using TP call E");
								this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
							}
							else
							{
								if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
									target->Message(LIGHTEN_BLUE, "Debug: You and %s are not near pathing nodes but still have LoS. This NPC will rush your location", this->GetName());
							}
						}
						else
						{
							if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
								target->Message(LIGHTEN_BLUE, "Debug: You and %s are not near pathing nodes but still have LoS. This NPC will rush your location", this->GetName());
						}
					}
				}
				//Yeahlight: Both NPC and target do not have pathing nodes in range
				else
				{
					int16 modifier = 1;
					if(target->IsClient())
						modifier = 10;
					//Yeahlight: The NPC and target cannot see each other on two elevated planes
					if(!CheckCoordLos(GetX(), GetY(), GetZ() + 2500, target->GetX(), target->GetY(), target->GetZ() * modifier + 2500))
					{
						if(!CheckCoordLos(GetX(), GetY(), GetZ() + 500, target->GetX(), target->GetY(), target->GetZ() * modifier + 500))
						{
							if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
								target->Message(RED, "Debug: Using TP call F");
							this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
						}
						else
						{
							if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
								target->Message(LIGHTEN_BLUE, "Debug: You and %s are not near pathing nodes but still have LoS. This NPC will rush your location", this->GetName());
						}
					}
					else
					{
						if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
							target->Message(LIGHTEN_BLUE, "Debug: You and %s are not near pathing nodes but still have LoS. This NPC will rush your location", this->GetName());
					}
				}
			}
		}
		//Yeahlight: NPC can see its target
		else
		{
			//Yeahlight: Do nothing
		}
	}
}

//o--------------------------------------------------------------
//| CheckMyEngagedStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's engaged status
//o--------------------------------------------------------------
void NPC::CheckMyEngagedStatus(bool debugFlag)
{
	if(!target)
		return;
	//Yeahlight: Check for class attacks
	if(classAbility_timer->Check())
	{
		DoClassAttacks(target);
	}
	//Yeahlight: Main hand attacks
	else if(attack_timer->Check() && target && target->GetID() != this->ownerid && !onPath && CheckCoordLos(GetX(), GetY(), GetZ() + 2, target->GetX(), target->GetY(), target->GetZ()))
	{
		Attack(target, 13, true);
		pLastChange = Timer::GetCurrentTime();
	}
	//Yeahlight: Off hand attacks
	else if(attack_timer_dw->Check() && target && target->GetID() != this->ownerid && !onPath && CheckCoordLos(GetX(), GetY(), GetZ() + 2, target->GetX(), target->GetY(), target->GetZ()))
	{
		//Yeahlight: Monk NPCs, as well as pets over level 29, get dual wield. Any dual wielding class may dual wield if they have weapons in both hands and are under level 60
		if(this->GetSkill(DUEL_WIELD) > 0 && (this->GetClass() == MONK || this->GetClass() == MONKGM || (this->GetOwner() && this->GetLevel() >= 29) || (GetLevel() < 60 && GetMyMainHandWeapon() && GetMyOffHandWeapon() && GetMyOffHandWeapon()->common.damage > 0)))
		{
			Attack(target, 14, true);
			pLastChange = Timer::GetCurrentTime();
		}
	}
	//Yeahlight: NPC is not currently moving
	else if(!isMoving())
	{
		//Yeahlight: NPC is on a specific path
		if(onPath)
		{
			//Yeahlight: Animate NPC
			this->setMoving(true);
			float distanceToTarget = 2;
			//Yeahlight: NPC is out of range of their node destination
			if(fdistance(this->myPathNode->x, this->myPathNode->y, GetX(), GetY()) > distanceToTarget)
			{
				//Yeahlight: Do nothing
			}
			//Yeahlight: NPC is in range of their next node
			else
			{
				//Yeahlight: Set NPC's location to the node's location
				this->x_pos = this->myPathNode->x;
				this->y_pos = this->myPathNode->y;
				this->z_pos = this->myPathNode->z;
				//Yeahlight: NPC's path is over since the queue will be empty on the next pop()
				if(myPath.size() == 1)
				{
					//Yeahlight: NPC is currently feared
					if(GetFeared())
					{
						SetOnFearPath(false);
					}
					//Yeahlight: NPC is not currently feared or fleeing
					else if(!this->IsFleeing())
					{
						//Yeahlight: Target fails a z-restricted LoS check
						if(!CheckCoordLosNoZLeaps(GetX(), GetY(), GetZ(), target->GetX(), target->GetY(), target->GetZ()))
						{
							if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
								target->Message(LIGHTEN_BLUE, "Debug: %s finished its path, but you are not in sight; finding new path", this->GetName());
							Node* myNearest = findClosestNode(this);
							//Yeahlight: NPC has at least one pathing node in range
							if(myNearest != NULL)
							{
								Node* theirNearest = findClosestNode(target);
								//Yeahlight: Target has at least one pathing node in range
								if(theirNearest != NULL)
								{
									if(zone->pathsLoaded)
									{
										if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
											target->Message(LIGHTEN_BLUE, "Debug: A path is required between you and %s; using a preprocessed path", this->GetName());
										this->findMyPreprocessedPath(myNearest, theirNearest);
									}
									else
									{
										//
									}
									this->myTargetsX = target->GetX();
									this->myTargetsY = target->GetY();
									this->myTargetsZ = target->GetZ();
								}
								//Yeahlight: Target does not have any pathing nodes in range
								else
								{
									if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
										target->Message(RED, "Debug: Using TP call G");
									this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
								}
							}
							//Yeahlight: NPC does not have any pathing nodes in range
							else
							{
								if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
									target->Message(RED, "Debug: Using TP call H");
								this->TeleportToLocation(target->GetX(), target->GetY(), target->GetZ());
							}
						}
						//Yeahlight: NPC can see its target
						else
						{
							faceDestination(target->GetX(),target->GetY());
							SetDestination(target->GetX(), target->GetY(), target->GetZ());
							this->onPath = false;
							StartWalking();
							walking_timer->Trigger();
							this->z_pos = FindGroundZ(GetX(), GetY(), 6);
							SendPosUpdate(false, NPC_UPDATE_RANGE, false);
						}
					}
					//Yeahlight: NPC is fleeing
					else
					{
						//Yeahlight: Do nothing, NPC's path will be resolved in another block
						this->onPath = false;
					}
				}
				//Yeahlight: NPC still has nodes left in its path
				else
				{
					//Yeahlight: Grab the next pathing node from the queue
					myPath.pop();
					myPathNode = zone->thisZonesNodes[myPath.front()];
					faceDestination(this->myPathNode->x, this->myPathNode->y);
					SetDestination(this->myPathNode->x, this->myPathNode->y, this->myPathNode->z);
					StartWalking();
					walking_timer->Trigger();
					SendPosUpdate(false, NPC_UPDATE_RANGE, false);
				}
			}
		}
		//Yeahlight: NPC is not on a specific path and is also not focused on a pathing node
		else if(!this->nodeFocused)
		{	
			float distanceToTarget = GetMeleeReach();
			//Yeahlight: NPC is out of melee range of its target
			if(fdistance(target->GetX(), target->GetY(), GetX(), GetY()) > distanceToTarget)
			{
				//Yeahlight: Animate the NPC
				this->setMoving(true);
				SetDestination(target->GetX(), target->GetY(), target->GetZ());
				StartWalking();
				walking_timer->Trigger();
				this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
			}
		}
	}
	//Yeahlight: NPC is currently moving
	else
	{
		//Yeahlight: Do nothing
	}
}

//o--------------------------------------------------------------
//| CheckMyGoHomeStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's go home status
//o--------------------------------------------------------------
void NPC::CheckMyGoHomeStatus()
{
	gohome_timer->Disable();
	this->preventPatrolling = false;
	//Yeahlight: NPC is a patroller
	if(this->isPatroller)
	{
		isMovingHome = false;
		Node* nodeStart = zone->thisZonesNodes[zone->zoneGrids[myPathGrid].startID];
		Node* nodeEnd = zone->thisZonesNodes[zone->zoneGrids[myPathGrid].endID];
		//Yeahlight: NPC can see its starting grid node
		if(CheckCoordLosNoZLeaps(GetX(), GetY(), GetZ(), nodeStart->x, nodeStart->y, nodeStart->z))
		{
			Node* myNearest = findClosestNode(this);
			if(myNearest != NULL)
			{
				if(zone->pathsLoaded)
				{
					this->findMyPreprocessedPath(myNearest, nodeEnd);
				}
				else
				{
					//
				}
			}
			else
			{
				//Yeahlight: TODO: Handle this situation
			}
		}
		//Yeahlight: NPC can see its ending grid node
		else if(CheckCoordLosNoZLeaps(GetX(), GetY(), GetZ(), nodeEnd->x, nodeEnd->y, nodeEnd->z))
		{
			Node* myNearest = findClosestNode(this);
			if(myNearest != NULL)
			{
				if(zone->pathsLoaded)
				{
					this->findMyPreprocessedPath(myNearest, nodeStart);
				}
				else
				{
					//
				}
			}
			else
			{
				//Yeahlight: TODO: Handle this situation
			}
		}
		//Yeahlight: NPC cannot see either its start or end grid nodes
		else
		{
			Node* myNearest = findClosestNode(this);
			if(myNearest != NULL)
			{
				if(zone->pathsLoaded)
				{
					this->findMyPreprocessedPath(myNearest, nodeStart);
				}
				else
				{
					//
				}
			}
			else
			{
				//Yeahlight: TODO: Handle this situation
			}
		}
	}
	//Yeahlight: NPC is not a patroller and has a spawn node to reference, most likely a static mob
	else if(this->mySpawnNode != NULL)
	{
		isMovingHome = true;
		//Yeahlight: NPC cannot see its home location
		if(!CheckCoordLosNoZLeaps(GetX(), GetY(), GetZ(), this->org_x, this->org_y, this->org_z))
		{
			Node* myNearest = findClosestNode(this);
			//Yeahlight: NPC has at least one pathing node in range
			if(myNearest != NULL)
			{
				if(zone->pathsLoaded)
				{
					this->findMyPreprocessedPath(myNearest, mySpawnNode);
				}
				else
				{
					//
				}
			}
			//Yeahlight: NPC does not have any pathing nodes in range
			else
			{
				faceDestination(this->org_x, this->org_y);
				SetDestination(this->org_x, this->org_y, this->org_z);
				StartWalking();
				walking_timer->Trigger();
				SendPosUpdate(false, NPC_UPDATE_RANGE * 2.5, false);
				onRoamPath = true;
			}
		}
		//Yeahlight: NPC can see its spawn location
		else
		{
			this->faceDestination(this->org_x, this->org_y);
			this->SetDestination(this->org_x, this->org_y, this->org_z);
			this->StartWalking();
			this->SetNWUPS(this->GetAnimation() * 2.3 / 5);
			this->MoveTowards(x_dest, y_dest, this->CastToNPC()->GetNWUPS());
			this->SendPosUpdate(false, NPC_UPDATE_RANGE * 2.5, false);
			this->onPath = false;
		}
	}
	//Yeahlight: NPC does not have a spawn node, most likely a roamer or an outdoor static spawn
	else
	{
		isMovingHome = true;
		faceDestination(this->org_x, this->org_y);
		SetDestination(this->org_x, this->org_y, this->org_z);
		StartWalking();
		walking_timer->Trigger();
		SendPosUpdate(false, NPC_UPDATE_RANGE * 2.5, false);
		onRoamPath = true;
	}
}

//o--------------------------------------------------------------
//| CheckMyMovingStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's moving status
//o--------------------------------------------------------------
void NPC::CheckMyMovingStatus(bool debugFlag)
{
	//Yeahlight: NPC is not moving
	if(!isMoving())
	{
		//Yeahlight: NPC is on a specific path
		if(onPath)
		{
			//Yeahlight: Animate NPC
			this->setMoving(true);
			float distanceToTarget = 2;
			//Yeahlight: NPC is out of range of their node destination
			if(fdistance(this->myPathNode->x, this->myPathNode->y, GetX(), GetY()) > distanceToTarget)
			{
				//Yeahlight: Do nothing
			}
			//Yeahlight: NPC is in range of their next node
			else
			{
				//Yeahlight: NPC has a spawn node to reference
				if(mySpawnNode != NULL)
				{
					//Yeahlight: Set NPC's location to the node's location
					this->x_pos = this->myPathNode->x;
					this->y_pos = this->myPathNode->y;
					this->z_pos = this->myPathNode->z;
					//Yeahlight: NPC's path is over since the queue will be empty on the next pop()
					if(myPath.size() == 1)
					{
						//Yeahlight: NPC is patrolling and ran out of nodes to reference
						if(this->isPatrolling)
						{
							//Yeahlight: NPC is not at the tail or the head of the patrol path, which most likely means it was pulled away during a patrol.
							//           Find a path to the head or tail of the patrol from its current pathing node.
							if(myPathNode->nodeID != zone->zoneGrids[myPathGrid].startID && myPathNode->nodeID != zone->zoneGrids[myPathGrid].endID)
							{
								if(zone->pathsLoaded)
								{
									this->findMyPreprocessedPath(myPathNode, mySpawnNode);
								}
								else
								{
									//
								}
							}
							//Yeahlight: NPC is at the head or tail of its patrol, find a new patrol path
							else
							{
								this->GetMyGridPath();
							}
						}
						//Yeahlight: NPC is not patrolling as it moves home
						else
						{
							//Yeahlight: Home fails a z-restricted LoS check
							if(!CheckCoordLosNoZLeaps(GetX(), GetY(), GetZ(), this->org_x, this->org_y, this->org_z))
							{
								if(debugFlag)
									cout<<"Debug: "<<this->GetName()<<" finished its path, but its home is no where in sight; finding new path"<<endl;
								Node* myNearest = zone->thisZonesNodes[myPath.front()];
								//Yeahlight: NPC has at least one pathing node in range
								if(myNearest != NULL)
								{
									if(zone->pathsLoaded)
									{
										if(debugFlag)
											cout<<"Debug: A path is required between "<<this->GetName()<<" and its home; using a preprocessed path"<<endl;
										this->findMyPreprocessedPath(myNearest, mySpawnNode);
									}
									else
									{
										//
									}
								}
								//Yeahlight: NPC does not have any pathing nodes in range
								else
								{
									//Yeahlight: TODO: Handle this situation
								}
							}
							//Yeahlight: NPC can see its home location
							else
							{
								faceDestination(this->org_x,this->org_y);
								SetDestination(this->org_x, this->org_y, this->org_z);
								this->onPath = false;
								StartWalking();
								walking_timer->Trigger();
								SendPosUpdate(false, NPC_UPDATE_RANGE, false);
							}
						}
					}
					//Yeahlight: NPC still has nodes left in its path
					else
					{
						//Yeahlight: Grab the next pathing node from the queue
						myPath.pop();
						myPathNode = zone->thisZonesNodes[myPath.front()];
						faceDestination(this->myPathNode->x, this->myPathNode->y);
						SetDestination(this->myPathNode->x, this->myPathNode->y, this->myPathNode->z);
						StartWalking();
						walking_timer->Trigger();
						SendPosUpdate(false, NPC_UPDATE_RANGE, false);
					}
				}
				//Yeahlight: NPC does not have a spawnnode to reference
				else
				{
					//Yeahlight: TODO: Handle this situation
				}
			}
		}
		//Yeahlight: NPC is not on a path, but is moving home
		else
		{
			float distanceToTarget = 2;
			//Yeahlight: NPC is out of range of their node destination
			if(fdistance(org_x, org_y, GetX(), GetY()) > distanceToTarget)
			{
				//Yeahlight: Do nothing
			}
			//Yeahlight: NPC is in range of their node destination
			else
			{
				x_pos = org_x;
				y_pos = org_y;
				z_pos = org_z;
				heading = org_heading;
				isMovingHome = false;
				this->onRoamPath = false;
				this->onPath = false;
				this->setMoving(false);
				this->forget_timer->Disable();
				this->SetFeignMemory("0");
				this->hate_list.Whipe();
				SetHitOnceOrMore(false);
				//Yeahlight: NPC is home; update everyone in the zone
				SendPosUpdate(false);
			}
		}
	}
	//Yeahlight: NPC is moving home
	else
	{
		//Yeahlight: Do nothing
	}
}

//o--------------------------------------------------------------
//| CheckMyOwnerStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's owner status
//o--------------------------------------------------------------
void NPC::CheckMyOwnerStatus()
{
	Mob* obc = entity_list.GetMob(this->ownerid);
	SetTarget(obc);
	if(GetPetOrder() == SPO_Sit)
	{
		target = 0;
		moving = false;
		SetDestination(x_pos, y_pos, z_pos);
	}
	if(GetPetOrder() == SPO_Guard)
	{
		target = 0;
		if(!movingToGuardSpot && (fdistance(GetGuardX(), GetGuardY(), x_pos, y_pos) > GetMeleeReach()))
		{
			movingToGuardSpot = true;
			SetDestination(GetGuardX(), GetGuardY(), GetGuardZ());
			faceDestination(GetGuardX(), GetGuardY());
			StartWalking();
			walking_timer->Trigger();
			SendPosUpdate(false, NPC_UPDATE_RANGE, false);
		}		
	}
	if(!moving && target != 0)
	{
		if(fdistance(target->GetX(), target->GetY(), x_pos, y_pos) > GetMeleeReach())
		{
			//Yeahlight: Animate the pet
			this->setMoving(true);
			SetDestination(target->GetX(), target->GetY(), target->GetZ());
			faceDestination(target->GetX(), target->GetY());
			StartWalking();
			walking_timer->Trigger();
			this->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
			faceTarget_timer->Trigger();
		}
	}
}

//o--------------------------------------------------------------
//| CheckMyWalkingStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's walking status
//o--------------------------------------------------------------
void NPC::CheckMyWalkingStatus(bool engaged)
{
	//Yeahlight: NPC is a boat
	if(IsBoat())
	{
		MoveTowards(x_dest, y_dest, npc_walking_units_per_second);
	}
	//Yeahlight: NPC is not a boat
	else
	{
		//Yeahlight: NPC is an unengaged pet with /pet guard enabled
		if(!engaged && GetPetOrder() == SPO_Guard)
		{
			FaceGuardSpot();
		}
		//Yeahlight: TODO: Rewrite this to be far less confusing.
		if((engaged || movingToGuardSpot || target == entity_list.GetMob(this->ownerid)) && !this->onPath && !this->onRoamPath && !this->onFearPath && !this->IsFleeing() && !this->nodeFocused)
		{
			float inX = x_dest;
			float inY = y_dest;
			if(target != 0)
				SetDestination(target->GetX(), target->GetY(), target->GetZ());
			faceDestination(x_dest, y_dest);
			//Yeahlight: Only send out updates when a change in heading is present
			if((engaged || target == entity_list.GetMob(this->ownerid)) && this->isMoving() && (inX != x_dest || inY != y_dest))
				SendPosUpdate(false, NPC_UPDATE_RANGE, false);
			MoveTowards(x_dest, y_dest, npc_walking_units_per_second);
		}
		//Yeahlight: NPC is not engaged
		else
		{
			MoveTowards(x_dest, y_dest, npc_walking_units_per_second);
		}
	}
}

//o--------------------------------------------------------------
//| CheckMySummonStatus; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Checks the NPC's summon status
//o--------------------------------------------------------------
void NPC::CheckMySummonStatus()
{
	//Yeahlight: Summon target if they are out of melee range or they are not in LoS (this will prevent players from exploiting rooted mobs by standing under the NPC in melee range, but out of LoS range)
	if(fdistance(GetX(), GetY(), target->GetX(), target->GetY()) > GetMeleeReach() || !CheckCoordLos(GetX(), GetY(), GetZ(), target->GetX(), target->GetY(), target->GetZ()))
	{
		SummonTarget(target);
	}
}

//o--------------------------------------------------------------
//| CheckMySummonStatus; Yeahlight, June 11, 2009
//o--------------------------------------------------------------
//| Checks the NPC's target status
//o--------------------------------------------------------------
void NPC::CheckMyTargetStatus()
{
	Mob* newTarget = NULL;
	//Yeahlight: NPC is rooted; find closest target
	if(this->IsRooted())
	{
		newTarget = hate_list.GetClosest(this->CastToMob());
	}
	//Yeahlight: NPC is undead and near full health, thus it will target the closest entity
	else if(this->GetBodyType() == BT_Undead && this->GetHPRatio() >= 90)
	{
		newTarget = hate_list.GetClosest(this->CastToMob());
	}
	else
	{
		newTarget = hate_list.GetTop(this->CastToMob());
	}

	//Yeahlight: NPC's new target is a pet, let us make sure there are no other PCs in melee range to target
	if(newTarget && newTarget->GetOwner())
	{
		Mob* newTempTarget = NULL;
		newTempTarget = hate_list.GetTopInRangeNoPet(this->CastToMob());
		if(newTempTarget)
		{
			newTarget = newTempTarget;
		}
	}

	SetTarget(newTarget);

	//Yeahlight: This pet currently does not have a target
	if(newTarget == NULL && GetOwnerID() != 0) 
	{
		CheckMyOwnerStatus();
	}

	//Yeahlight: NPC has a pet
	if(GetPet())
	{
		//Yeahlight: NPC has a target
		if(newTarget)
		{
			GetPet()->SetTarget(newTarget);
		}
		//Yeahlight: NPC does not have a target; make the NPC the pet's target
		else
		{
			GetPet()->SetTarget(this->CastToMob());
		}
	}
}

//o--------------------------------------------------------------
//| GetFearDestination; Yeahlight, Mar 11, 2009
//o--------------------------------------------------------------
//| Assigns a fear destination for the NPC
//o--------------------------------------------------------------
bool NPC::GetFearDestination(float x, float y, float z)
{
	//Yeahlight: NPC is rooted; abort
	if(IsRooted())
		return false;

	bool debugFlag = true;

	Node* myNearest = findClosestNode(this);
	//Yeahlight: NPC has at least one pathing node in range
	if(myNearest != NULL)
	{
		int16 counter = 0;
		Node* myFearingNode = NULL;
		//Yeahlight: Loop through this random choice until the start and end nodes do not match
		do
		{
			myFearingNode = zone->thisZonesNodes[rand()%(zone->numberOfNodes)];
			//Yeahlight: Zone freeze debug
			if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
				EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		} 
		while(myNearest == myFearingNode && counter++ < 6);
		//Yeahlight: Target has at least one pathing node in range
		if(myFearingNode != NULL)
		{
			//Yeahlight: Find path between NPC and target
			if(zone->pathsLoaded)
			{
				if(debugFlag && target && target->IsClient() && target->CastToClient()->GetDebugMe())
					target->Message(LIGHTEN_BLUE, "Debug: A path is required between %s and its flee destination; using a preprocessed path", this->GetName());
				this->findMyPreprocessedPath(myNearest, myFearingNode);
				onFearPath = true;
				return true;
			}
		}
	}
	//Yeahlight: Target has no pathing nodes in range and has not been blocked from finding paths, so use random location roaming mechanics
	int loop = 0;
	bool permit = false;
	float ranx, rany, ranz;
	float offset = 2;
	//Yeahlight: Roamers are given a large offset for hills and such
	if(this->isRoamer)
		offset = 150;
	while(loop < 100)
	{
		int ran = 250 - (loop*2);
		loop++;
		ranx = x + rand()%ran - rand()%ran;
		rany = y + rand()%ran - rand()%ran;
		ranz = FindGroundZ(ranx, rany, offset);
		if(ranz == -999999)
			continue;
		float fdist = abs(ranz - z);
		if(fdist <= 12 && CheckCoordLosNoZLeaps(x, y, z, ranx, rany, ranz))
		{
			permit = true;
			break;
		}
	}
	//Yeahlight: A reasonable destination has been found
	if(permit)
	{
		fear_walkto_x = ranx;
		fear_walkto_y = rany;
		fear_walkto_z = ranz;
	}
	//Yeahlight: NPC is in an odd location, uncapable of locating a reasonable destination, so drop the debuff immediately
	else
	{
		BuffFadeByEffect(SE_Fear);
		return false;
	}
	faceDestination(ranx, rany);
	SetDestination(ranx, rany, ranz);
	StartWalking();
	walking_timer->Trigger();
	SendPosUpdate(false, NPC_UPDATE_RANGE * 2.5, false);
	onFearPath = true;
	return true;
}

//o--------------------------------------------------------------
//| IsDisabled; Yeahlight, Mar 11, 2009
//o--------------------------------------------------------------
//| Returns true if the NPC is currently disabled in any way
//o--------------------------------------------------------------
bool NPC::IsDisabled()
{
	//Yeahlight: NPC is mezzed, feared, fleeing or stunned, thus disabled
	if(IsFleeing() || GetFeared() || IsMesmerized() || IsStunned())
		return true;
	return false;
}


//o--------------------------------------------------------------
//| getBoatPath; Tazadar, June 20, 2009
//o--------------------------------------------------------------
//| Fill the boatroute vector
//o--------------------------------------------------------------
bool NPC::getBoatPath()
{
	char fileNameSuffix[] = ".txt";
	char fileNamePrefix[200] = "./Maps/Boats/Zone/";
	strcat(fileNamePrefix, this->name);
	strcat(fileNamePrefix, zone->GetShortName());
	strcat(fileNamePrefix, fileNameSuffix);

	ifstream myfile(fileNamePrefix);

	ZoneBoatCommand_Struct tmp;
	int tmp2;

	if (myfile.is_open())
	{
		while (! myfile.eof() )
		{
			myfile >> tmp2;
			tmp.cmd = (BOATCOMMANDS)tmp2;
			if(tmp.cmd == GOTO){
				myfile >> tmp.x >> tmp.y >> tmp.go_to.speed;
			}
			else if(tmp.cmd == ROTATE) {
				tmp.x = 0;
				tmp.y = 0;
				myfile >> tmp.rotate.heading;
			}
			else {
				std::cout << " Boat File corrupted : " << fileNamePrefix << std::endl;
				myfile.close();
				boatroute.clear();
				return false;
			}
			boatroute.push_back(tmp);
		}
	}
	else{
		std::cout << " Boat File not opened :" << fileNamePrefix << std::endl;
		return false;
	}

	/*for(int i = 0; i < boatroute.size() ; i++){
		std::cout << (int)boatroute[i].cmd << " " << boatroute[i].x << " " << boatroute[i].y << " " << boatroute[i].go_to.speed << std::endl; 
	}*/

	myfile.close();
	return true;
}

//o--------------------------------------------------------------
//| SendTravelDone; Tazadar, June 22, 2009
//o--------------------------------------------------------------
//| Send a packet to world when travel is done
//o--------------------------------------------------------------
void NPC::SendTravelDone(){

	// Tazadar : Make the packet for the World Serv
	ServerPacket* pack = new ServerPacket(ServerOP_TravelDone,sizeof(BoatName_Struct));
	
	BoatName_Struct* boatName = (BoatName_Struct*) pack->pBuffer;

	// Tazadar : Fill the packet

	memcpy(boatName->boatname,this->GetName(),NPC_MAX_NAME_LENGTH);

	// Tazadar : Send the packet
	worldserver.SendPacket(pack);
}

//o--------------------------------------------------------------
//| GetHitList; Yeahlight, June 26, 2009
//o--------------------------------------------------------------
//| Builds a list of attackers to be sent to the corpse
//o--------------------------------------------------------------
queue<char*> NPC::GetHitList()
{
	queue<char*> list;
	list = hate_list.BuildHitList();
	return list;
}

//o--------------------------------------------------------------
//| IsEyeOfZomm; Yeahlight, June 26, 2009
//o--------------------------------------------------------------
//| Returns true if the NPC is an eye of zomm spawn
//o--------------------------------------------------------------
bool NPC::IsEyeOfZomm()
{
	//Yeahlight: Check the NPC's name for the eye of zomm structure and verify it is level 1
	if(name[0] == 'E' && name[1] == 'y' && name[2] == 'e' && name[4] == 'o' && name[5] == 'f' && GetLevel() == 1)
		return true;
	else
		return false;
}

/********************************************************************
 *                      Harakiri - 8/31/2009                        *
 ********************************************************************
 *		       CheckSignal()									    *
 ********************************************************************
   Sent Event to perlParser if we got a signal from a quest script
 ********************************************************************/
void NPC::CheckSignal() {
	if (signaled) {		
		char buf[32];
		snprintf(buf, 31, "%d", signal_id);
		buf[31] = '\0';
		#ifdef EMBPERL
			perlParser->Event(EVENT_SIGNAL, GetNPCTypeID(), buf, this, NULL);
		#endif
		signaled=false;
	}
}
