#ifndef NPC_H
#define NPC_H

#include "database.h"
#include "mob.h"
#include "spawn2.h"
#include "hate_list.h"
#include "loottable.h"
#include "zonedump.h"
#include "../../Utils/azone/map.h"
#include "zone.h"

class Group;

#ifdef WIN32
	#define  M_PI	3.141592
#endif

class NPC : public Mob
{
public:
	NPC(NPCType* data, Spawn2* respawn, float x, float y, float z, float heading, bool IsCorpse = false, int in_roamRange = 0, int16 in_myRoamBox = 0, int16 in_myPathGrid = 0, int16 in_myPathGridStart = 0);
	virtual ~NPC();

	virtual bool IsNPC() { return true; }
	virtual bool Process();

	void	AddToHateList(Mob* other, sint32 damage = 0, sint32 hate = 0xFFFFFFFF);
	// Kaiyodo - added attacking hand as a param
	void	Attack(Mob* other, int Hand = 13, bool procEligible = true, bool riposte = false);
	void	Damage(Mob* other, sint32 damage, int16 spell_id, int8 attack_skill = 0x04);
	void	Death(Mob* other, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04);
	void	setMoving(bool _moving = true);
	bool	isMoving()	{ return this->moving; }
	Mob*	GetHateTop()  { return hate_list.GetTop(this->CastToMob()); }
	Mob*	GetHateSecondFromTop()  { return hate_list.GetSecondFromTop(); }
	int16	GetHateSize() { return hate_list.GetHateListSize(); }
	bool	IsEngaged()   { return (hate_list.GetHateListSize() == 0) ? false:true; }
	inline bool	IsInteractive() { return interactive; }

	void	FaceTarget(Mob* MobToFace = 0);
	void	RemoveFromHateList(Mob* mob);
	void	WhipeHateList() { hate_list.Whipe(); }
	void	GoToBind()	{ GMMove(org_x, org_y, org_z, org_heading); }

	void	AddItem(Item_Struct* item, int8 charges, uint8 slot);
	void	AddItem(int32 itemid, int8 charges, uint8 slot);
	void	AddLootTable();
	void	RemoveItem(uint16 item_id);
	void	ClearItemList();
	ServerLootItem_Struct*	GetItem(int slot_id);
	void	AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum);
	void	AddCash();
	void	RemoveCash();
	ItemList*	GetItemList() { return itemlist; }
	void	QueryLoot(Client* to);
	int32	CountLoot();
	void	DumpLoot(int32 npcdump_index, ZSDump_NPC_Loot* npclootdump, int32* NPCLootindex);
	int32	GetLoottableID()	{ return loottable_id; }
	void	SetPetType(int8 in_type)	{ typeofpet = in_type; } // put this here because only NPCs can be anything but charmed pets

	uint32	GetCopper()		{ return copper; }
	uint32	GetSilver()		{ return silver; }
	uint32	GetGold()		{ return gold; }
	uint32	GetPlatinum()	{ return platinum; }
	uint32	MerchantType;
	void	SendTo(float new_x, float new_y);
	void	MoveTo(float dest_x, float dest_y, float dest_z);

	void    MoveTowards(float x, float y, float d);
	void    SetDestination(float x, float y, float z);
	void	StartWalking();
	void	StartFacingTarget() {walking_timer->Trigger();}
	float	fdistance(float x1, float y1, float x2, float y2);
	float   square(float number);

	void	Depop(bool StartSpawnTimer = true);
	void	Stunned(int16 duration);
	void	Stun(int16 duration, Mob* stunner = NULL, bool melee = false);
	bool	IsStunned() { return stunned; }

	int32   GetFactionID()  { return faction_id; } // Merkur 03/11
	int32	GetPrimaryFactionID()	{ return primaryFaction; }
	Mob*	GetIgnoreTarget() { return ignore_target; }
	void	SetIgnoreTarget(Mob* mob) {ignore_target = mob; }
	sint32	GetNPCHate(Mob* in_ent)  {return hate_list.GetEntHate(in_ent);}

	void	SetFactionID(int32 newfactionid) { faction_id=newfactionid; }
	int32   GetSpawnGroupId()       { return respawn2->GetSpawnGroupId(); } // maalanar: added for updating and deleting spawn info in spawn2
	void    SetFeignMemory(const char* num) {feign_memory = num;}
	inline const char*    GetFeignMemory()	{ return feign_memory; }
	inline bool	CheckAggro(Mob* other) {return hate_list.IsOnHateList(other);}
	void	FaceGuardSpot();

	FACTION_VALUE CheckForAggression(Mob* this_mob, Mob* that_mob);

	sint16	GetRoamRange() {return roamRange;}
	int16	GetRoamBox() {return myRoamBox;}

	bool	TeleportToLocation(float x, float y, float z);

	void	SetNWUPS(float NWUPS){npc_walking_units_per_second = NWUPS;}
	float	GetNWUPS(){return npc_walking_units_per_second;}

	float	GetOrgX() {return org_x;}
	float	GetOrgY() {return org_y;}

	bool	GetMyGridPath();

	int16	GetPatrolID() {return myPathGrid;}

	void	SetIsCasting(bool casting) {isCasting = casting;}

	bool	IsFleeing() {return fleeing;}

	bool	GetOnRoamPath() {return onRoamPath;}

	const char*	GetLeashMemory() {return leashMemory;}
	void	SetLeashMemory(const char* victim) {leashMemory = victim;}

	bool	CombatPush(Mob* attacker);

	int32	GetMinDmg() {return myMinDmg;}
	int32	GetMaxDmg() {return myMaxDmg;}

	int16	GetMitigationAC() {return myMitigationAC;}
	int16	GetAvoidanceAC() {return myAvoidanceAC;}

	bool	SummonTarget(Mob* target);

	bool	IsEnraged() {return enraged;}

	bool	GetCanSummon() {return canSummon;}
	bool	GetCanEnrage() {return canEnrage;}
	bool	GetCanRampage() {return canRampage;}
	bool	GetCanFlurry() {return canFlurry;}
	bool	GetCanTrippleAttack() {return canTrippleAttack;}
	bool	GetCanQuadAttack() {return canQuadAttack;}
	bool	GetCannotBeSlowed() {return cannotBeSlowed;}
	bool	GetCannotBeMezzed() {return cannotBeMezzed;}
	bool	GetCannotBeCharmed() {return cannotBeCharmed;}
	bool	GetCannotBeStunned() {return cannotBeStunned;}
	bool	GetCannotBeSnaredOrRooted() {return cannotBeSnaredOrRooted;}
	bool	GetCannotBeFeared() {return cannotBeFeared;}
	bool	GetIsImmuneToMelee() {return isImmuneToMelee;}
	bool	GetIsImmuneToMagic() {return isImmuneToMagic;}
	bool	GetIsImmuneToFleeing() {return isImmuneToFleeing;}
	bool	GetIsImmuneToNonBaneDmg() {return isImmuneToNonBaneDmg;}
	bool	GetIsImmuneToNonMagicalDmg() {return isImmuneToNonMagicalDmg;}
	bool	GetWillNeverAgro() {return willNeverAgro;}

	void	DoClassAttacks(Mob *target);

	bool	GetMeleeWeapons();
	int16	GetItemHaste();

	void	CreateProperName(char* name);
	char*	GetProperName() {return properName;}

	sint16	GetMyRegenRate() {if(IsEngaged()) return myRegenRate; return myRegenRate * 2;}

	void	CheckMyFeignMemory(bool debugFlag);
	void	CheckMyLeashMemory(bool debugFlag);
	void	CheckMyDepopStatus();
	void	CheckMyEnrageStatus();
	void	DoMyTeleport();
	void	CheckMyNodeFocusedStatus();
	void	CheckMyDebuffRefundStatus(bool engaged);
	void	CheckMyFleeStatus(bool debugFlag);
	void	CheckMyPatrolStatus();
	void	CheckMyRoamStatus();
	void	CheckMyAgroStatus();
	void	CheckMyDefenseCastStatus();
	void	CheckMyOffenseCastStatus();
	void	CheckMyLosStatus(bool debugFlag);
	void	CheckMyEngagedStatus(bool debugFlag);
	void	CheckMyGoHomeStatus();
	void	CheckMyMovingStatus(bool debugFlag);
	void	CheckMyOwnerStatus();
	void	CheckMyWalkingStatus(bool engaged);
	void	CheckMySummonStatus();
	void	CheckMyTargetStatus();

	void	SetSpin(bool in) {spinning = in;}
	bool	GetSpin() {return spinning;}

	void	SetMyEyeOfZommMasterID(int16 id) {myEyeOfZommMasterID = id;}

	void	SetFeared(bool in) { feared = in; }
	bool	GetFeared() { return feared; }
	bool	GetFearDestination(float x, float y, float z);
	void	SetOnFearPath(bool in) { onFearPath = in; }

	bool	IsDisabled();

	bool	IsCharmed() { return charmed; }
	void	SetCharmed(bool in) { charmed = in; }

	void	DepopTimerStart() { depop_timer->Start(); }
	void	DepopTimerStop() { depop_timer->Disable(); }
	Spawn2* getRespawn(){ return respawn2;}

	bool	IsBoat() { return isBoat; }
	void	SetBoat(bool in) { isBoat = in; }
	bool	getBoatPath(); // Tazadar : Get the boat path from the .txt file
	void	setBoatDestinationNode(int16 from,int16 to){ targetnode = to; currentnode = from; orders = true;} // Tazadar : We change the current & the target node
	void	SendTravelDone(); // Tazadar : Sent when travel is done
	void	SetLineRoute(bool route){lineroute = route;};
	bool	GetTaunting() { return taunting; }
	void	StartTaunting() { taunt_timer->Trigger(); taunting = true; }
	void	StopTaunting() { taunt_timer->Disable(); taunting = false; }

	bool	GetPassiveCanSeeThroughInvis() { return passiveCanSeeThroughInvis; }
	bool	GetPassiveCanSeeThroughInvisToUndead() { return passiveCanSeeThroughInvisToUndead; }
	void	SetPassiveCanSeeThroughInvis(bool in) { passiveCanSeeThroughInvis = in; }
	void	SetPassiveCanSeeThroughInvisToUndead(bool in) { passiveCanSeeThroughInvisToUndead = in; }

	queue<char*> GetHitList();
	queue<char*> BuildLootRights(Group* group, Client* client);
	queue<char*> GetLootRights() { return lootrights; }

	bool	GetHitOnceOrMore() { return hitOnceOrMore; }
	void	SetHitOnceOrMore(bool in) { hitOnceOrMore = in; }

	bool	IsEyeOfZomm();

	bool	GetMovingToGuardSpot() { return movingToGuardSpot; }
	int32	GetPetMinDamage() { return Petmin_dmg; }
	int32	GetPetMaxDamage() { return Petmax_dmg; }

	bool	GetOnPath() { return onPath; }
	void	SetOnPath(bool in) { onPath = in; }
	queue<int16> GetMyPath() { return myPath; }
	void	SetMyPath(queue<int16> in) { myPath = in; }
	Node*	GetMyPathNode() { return myPathNode; }
	void	SetMyPathNode(Node* in) { myPathNode = in; }

	bool	IsAlive() { return cur_hp > 0; }

	// Harakiri quest functions for scripts
	inline void SignalNPC(int _signal_id) { signaled = true; signal_id = _signal_id; }
	void CheckSignal();

protected:
	friend class EntityList;
	HateList hate_list;
//	Spawn*	respawn;
	Spawn2*	respawn2;
	
	uint32	copper;
	uint32	silver;
	uint32	gold;
	uint32	platinum;
	ItemList*	itemlist;

	int32	faction_id;
	int32	primaryFaction;

	bool	isMovingHome;
	bool	moving;
	
	Timer*	attacked_timer;		//running while we are being attacked (damaged)
	Timer*	scanarea_timer; 
	Timer*	gohome_timer;
	Timer*	tod_despawn_timer;
	Mob*	ignore_target;

	Timer*	movement_timer;
	Timer*	walking_timer;
	float   org_x, org_y, org_z, org_heading;
	Timer*  stunned_timer;
	bool	stunned;
	const char*	feign_memory;
	Timer*	forget_timer;
	int8	forgetchance;

	Timer*  checkLoS_timer;
	Timer*  faceTarget_timer;

	float	myTargetsX;
	float	myTargetsY;
	float	myTargetsZ;

	Timer*	spawnUpdate_timer;

	sint16	roamRange;
	Timer*	roam_timer;
	bool	isRoamer;
	int16	myRoamBox;
	bool	onRoamPath;
	bool	blockFlee;

	float	npc_walking_units_per_second;

	Timer*  pathing_teleportation_timer;

	int16	myPathGrid;
	bool	isPatroller;
	bool	isPatrolling;
	Timer*	patrolPause_timer;
	bool	requiresNewPath;
	bool	preventPatrolling;

	int16	myBuffs[4];
	int16	myDebuffs[4];
	bool	isCaster;
	Timer*	offensiveCast_timer;
	bool	isCasting;
	sint16	mySpellCounter;
	Timer*  debuffCounterRegen_timer;
	Timer*  emergencyFlee_timer;
	Timer*	defensiveCast_timer;

	bool	fleeing;
	bool	onFleePath;
	Timer*	revertFlee_timer;

	bool	nodeFocused;
	Timer*	nodeFocused_timer;

	const char* leashMemory;
	Timer*	leash_timer;

	int32	myMinDmg;
	int32	myMaxDmg;
	char	mySpecialAttacks[20];

	//Yeahlight: Special attacks field of NPC table
	bool	canSummon;
	bool	canEnrage;
	bool	canRampage;
	bool	canFlurry;
	bool	canTrippleAttack;
	bool	canQuadAttack;
	bool	cannotBeSlowed;
	bool	cannotBeMezzed;
	bool	cannotBeCharmed;
	bool	cannotBeStunned;
	bool	cannotBeSnaredOrRooted;
	bool	cannotBeFeared;
	bool	isImmuneToMelee;
	bool	isImmuneToMagic;
	bool	isImmuneToFleeing;
	bool	isImmuneToNonBaneDmg;
	bool	isImmuneToNonMagicalDmg;
	bool	willNeverAgro;

	int16	myMitigationAC;
	int16	myAvoidanceAC;

	Timer*	summon_target_timer;
	Timer*  enrage_timer;
	bool	enraged;
	bool	hasEnragedThisFight;

	int16	rampageList[RAMPAGE_LIST_SIZE];
	int8	rampageListCounter;

	Timer*	get_target_timer;

	Timer*	taunt_timer;
	bool	taunting;
	Timer*	classattack_timer;
	Timer*	knightability_timer;
	Timer*	classAbility_timer;

	bool	reserveMyAttack;
	char    properName[30];

	sint16	myRegenRate;

	bool	spinning;
	
	int16	myEyeOfZommMasterID;

	bool	feared;
	bool	onFearPath;

	bool	charmed;

	Timer*	depop_timer;

	bool	isBoat;

	bool	passiveCanSeeThroughInvis;
	bool	passiveCanSeeThroughInvisToUndead;

	queue<char*> lootrights;

	bool	hitOnceOrMore;
	bool	interactive;

	bool	movingToGuardSpot; //Tazadar 03/06/08 for pets of course

	int32	Petmin_dmg; //Tazadar 06/01/08 we need to use this for pets dmg
	int32	Petmax_dmg;	//Tazadar 06/01/08 we need to use this for pets dmg

	bool	onPath;
	queue<int16> myPath;
	Node*	myPathNode;

	int		signal_id;
	bool	signaled;	// used by quest signal() command
private:
	int32	loottable_id;
	bool	p_depop;
	vector<ZoneBoatCommand_Struct> boatroute; // Tazadar : A vector with all the nodes
	int16	currentnode; // Tazadar : current node
	int16	targetnode; // Tazadar : go to the target node
	bool orders; // Tazadar : Boat recieved order?
	bool lineroute; // Tazadar : Is the route a line in one zone (like halas raft)
};

#endif