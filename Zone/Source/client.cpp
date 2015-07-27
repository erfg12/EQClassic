#include <cstdarg>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <signal.h>
#include <cmath>

#include "EQCUtils.hpp"
#include "client.h"
#include "worldserver.h"
#include "skills.h"
#include "database.h"
#include "spdat.h"
#include "ClientList.h"
#include "packet_functions.h"
#include "PlayerCorpse.h"
#include "petitions.h"
#include "EQCException.hpp"
#include "groups.h"
#include "ZoneGuildManager.h"
#include "packet_dump_file.h"
#include "SpellsHandler.hpp"
#include "watermap.h"
#include "spawngroup.h"
#include "SharedMemory.hpp"
#include "questmgr.h"

#ifdef EMBPERL
	#include "embparser.h"
#endif


using namespace std;
using namespace EQC::Common;
using namespace EQC::Common::Network;
using namespace EQC::Zone;

extern EntityList		entity_list;
extern Zone*			zone;
extern volatile bool	ZoneLoaded;
extern WorldServer		worldserver;
extern ZoneGuildManager zgm;
extern SpellsHandler	spells_handler;
extern ClientList		client_list;
extern int32			numclients;
extern PetitionList		petition_list;



Client::Client(int32 in_ip, int16 in_port, int in_send_socket)
: Mob("No name",	// Firstname
	  "",	// Surname
	  0,	// Current HitPoints
	  0,	// Maximum Hitpoints
	  0,	// Gender
	  0,	// Race
	  0,	// Class
	  0,	// Deity
	  0,	// Level
	  BT_Humanoid,	// Body type.
	  0,	// npctypeid
	  0,	// skills
	  0,	// Size
	  0.46,	// Walking Speed
	  0.7,	// Running Speed
	  0,	// Heading
	  0,	// X Coord
	  0,	// Y Coord
	  0,	// Z Coord
	  0,	// light
	  0,	// equip
	  0xFF,	// Texture
	  0xFF,	// helmtexture
	  0,	// ac
	  0,	// atk
	  0,	// str
	  0,	// sta
	  0,	// dex
	  0,	// agi
	  0,	// int
	  0,	// wis
	  0,	// cha
	  0,    // mr
	  0,	// cr
	  0,	// fr
	  0,	// dr
	  0,	// pr
	  0,	// min_dmg
	  0,	// max_dmg
	  "",	// special_attacks
	  0,    // attack_speed_modifier
	  0,    // main_hand_texture
	  0,    // off_hand_texture
	  0,	// hp_my_regen_rate
	  0,	// passiveSeeInvis
	  0,	// passiveSeeInvisToUndead
	  Any_Time	// time_of_day
	  )
{

	// Set Networking Detials of this client
	ip = in_ip;
	port = in_port;
	send_socket = in_send_socket;

	// Set State to Connecting1
	client_state = CLIENT_CONNECTING1;

	// Create Timeout Timer
	timeout_timer = new Timer(CLIENT_TIMEOUT);
	// Start Timer
	timeout_timer->Start();

	account_id = 0;
	admin = 0;
	strcpy(account_name, "");
	
	packet_manager.SetDebugLevel(0);
	
	tellsoff = false;
	gmhideme = false;
	LFG = false;
	bHide = false;
	feigned = false;
	sneaking = false; 
	hitWhileCasting = false;
	isZoning = false;
	InWater = false;
	berserk = false;
	SetCraftingStation(0);

	target = 0;
	auto_attack = false;
	PendingGuildInvite = 0;
	zonesummon_x = -2;
	zonesummon_y = -2;
	zonesummon_z = -2;
	zonesummon_ignorerestrictions = false;
	SetCastingSpell(NULL);

	// Start it once to keep first beg as flagging as hack...
	beg_timer = new Timer(25);
	beg_timer->Start(25);

	// Harakiri timer for fishing, client seems to have 10sec lock out	
	fishing_timer = new Timer(10000);
	fishing_timer->Disable(); // dont want to start upon zoning

	// Harakiri timer for InstillDoubt, clients high kick animation is after about 5sec
	instilldoubt_timer = new Timer(5000);
	instilldoubt_timer->Disable(); // dont want to start upon zoning


	position_timer = new Timer(250);
	position_timer->Disable();
	position_timer_counter = 0;
	pLastUpdate = 0;
	pLastUpdateWZ = 0;
	
	// Kaiyodo - initialise haste variable
	HastePercentage = 0;
	group_invite_pending = false;

	pendingRezExp = 0;

	// Pinedepain - Initialize the instrument modifiers vars
	singingInstrumentMod = 0;
	percussionInstrumentMod = 0;
	stringedInstrumentMod = 0;
	windInstrumentMod = 0;
	brassInstrumentMod = 0;

	PickLock_timer = new Timer(50);

	immuneToSpells = false; 

	myMitigationAC = 0;
	myAvoidanceAC = 0;

	stunned_timer = new Timer(0);
	stunned_timer->Disable();
	stunned = false;

	myNetworkFootprint = 0;
	debugMe = false; // Harakiri default false, YL debug is annoying =p
	myEyeOfZomm = NULL;
	mySaveChance = 0;
	feared = false;

	ticProcess_timer = new Timer(6000);
	charmPositionUpdate_timer = new Timer(200);
	charmPositionUpdate_timer->Disable();

	voiceGrafting = false;

	SetPendingRez(false);
	SetPendingRezExp(0);
	SetPendingCorpseID(0);

	pendingExpLoss = 0;

	pendingRez_timer = new Timer(PC_RESURRECTION_ANSWER_DURATION);
	pendingRez_timer->Disable();

	pendingTranslocate = false;
	pendingTranslocateX = 0;
	pendingTranslocateY = 0;
	pendingTranslocateZ = 0;
	pendingTranslocateHeading = 0;
	pendingTranslocateSpellID = 0;
	pendingTranslocate_timer = new Timer(PC_TRANSLOCATE_ANSWER_DURATION);
	pendingTranslocate_timer->Disable();

	//Yeahlight: Set the HP and mana to ten thousand during the load process
	max_hp = 10000;
	max_mana = 10000;
	cur_hp = max_hp;
	cur_mana = max_mana;

	rain_timer = new Timer(4000);
	rain_timer->Disable();
	for(int i = 0; i < 10; i++)
	{
		rainList[i].spellID = 0;
		rainList[i].wavesLeft = 0;
		rainList[i].x = 0;
		rainList[i].y = 0;
		rainList[i].z = 0;
	}

	InitProcessArray();

	projectileStressTest = false;
	projectileStressTestCounter = 0;

	spellCooldown_timer = new Timer(1);

	reservedPrimaryAttack = false;
	reservedSecondaryAttack = false;

	for(int i = 0; i < 8; i++)
		spellRecastTimers[i] = 0;//pp.spellSlotRefresh[i];
}

Client::~Client()
{
	if(!isZoning && !isZoningZP && this->IsGrouped()) {
		Group* g = entity_list.GetGroupByClient(this);
		if (g) g->DelMember(this->GetName(), false, true);
	}

	if(IsDueling() && GetDuelTarget() != 0)
	{
		Entity* entity = entity_list.GetID(GetDuelTarget());
		if(entity && entity->IsClient())
		{
			entity->CastToClient()->SetDueling(false);
			entity->CastToClient()->SetDuelTarget(0);
		}
	}

	client_list.Remove(this);
	this->UpdateWho(true);
	this->Save(); // This fails when database destructor is called first on shutdown
	
	safe_delete(timeout_timer);
	safe_delete(position_timer);
	safe_delete(PickLock_timer);
	safe_delete(this->mpDbg); //Cofruben.
	safe_delete(stunned_timer);
	safe_delete(ticProcess_timer);
	safe_delete(pendingRez_timer);
	safe_delete(pendingTranslocate_timer);
	safe_delete(spellCooldown_timer);

	// is this needed? Im not sure but it better have it in than not - Merkur
	LinkedListIterator<FactionValue*> iterator(factionvalue_list);
	iterator.Reset();

	while(iterator.MoreElements())
	{
		iterator.RemoveCurrent(false);
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

//Save the current client state to the database (normal char save)
//Yeahlight: TODO: Several other things here to save (items mainly)
bool Client::Save() 
{
	pp.logtime = time(0); // We save the logtime to use that for no rent items

	if(Connected())
	{
		//Yeahlight: Player hit hardcoded (*.s3d) zoneline
		if(this->isZoningZP)
		{
			pp.heading = this->tempHeading;
			//Yeahlight: Database is supplying custom x, y, z coordinates
			if(zoningX != 0 && zoningY != 0 && zoningZ != 0)
			{
				pp.y = zoningX;
				pp.x = zoningY;
				pp.z = zoningZ;
			}
			else
			{
				pp.x = GetX();
				pp.y = GetY();
				pp.z = GetZ();
			}
		}
		//Yeahlight: Player hit softcoded zoneline
		else if(this->isZoning)
		{
			pp.heading = tempHeading;
			if(this->usingSoftCodedZoneLine)
			{
				pp.x = zoningX;
				pp.y = zoningY;
				pp.z = zoningZ;
			}
			else
			{
				pp.x = GetX();
				pp.y = GetY();
				pp.z = GetZ();
			}
		}
		else
		{
			pp.heading = heading;
			pp.x = GetX();
			pp.y = GetY();
			pp.z = GetZ();
		}

		if(GetDebugMe())
			Message(LIGHTEN_BLUE, "Debug: Saving your position at %.2f, %.2f, %.2f", pp.x, pp.y, pp.z);

		sint32 tmphp = GetHP();

		if (tmphp <= 0)
		{
			if (GetMaxHP() > 30000)
			{
				pp.cur_hp = 30000;
			}
			else
			{
				pp.cur_hp = GetMaxHP();
			}
		}
		else if (tmphp > 30000)
		{
			pp.cur_hp = 30000;
		}
		else
		{
			pp.cur_hp = tmphp;
		}
		pp.mana = cur_mana;

		for (int i = 0; i < 15; i++)
		{
			if (buffs[i].spell && buffs[i].spell->IsValidSpell())
			{
				pp.buffs[i].spellid = buffs[i].spell->GetSpellID();
				pp.buffs[i].duration = buffs[i].ticsremaining;
				pp.buffs[i].level = buffs[i].casterlevel;
				pp.buffs[i].visable = 2;
			}
			else
			{
				pp.buffs[i].spellid = 0xFFFF;
				pp.buffs[i].duration = 0;
				pp.buffs[i].level = 0;
			}
		}

 		if (!Database::Instance()->SetPlayerProfile(account_id, name, &pp))
		{
			cerr << "Failed to update player profile" << endl;
			return false;
		}
	}
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::Save()");
	return true;
}

void Client::SendPacketQueue(bool Block)
{
	// Get first send packet on queue and send it!
	MySendPacketStruct* p = 0;    
	sockaddr_in to;	
	memset((char *) &to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_port = port;
	to.sin_addr.s_addr = ip;

	if (Block)
	{
		MPacketManager.lock();
	}
	else if (!MPacketManager.trylock()) 
	{
		return;
	}

	while(p = packet_manager.SendQueue.pop())
	{
		sendto(send_socket, (char *) p->buffer, p->size, 0, (sockaddr*)&to, sizeof(to));


		//Yeahlight: TODO: These deletes appear backwards to me. Why do we apply "delete" to a uchar* but a "delete[]" to a single structure?
		//safe_delete(p->buffer);
		//safe_delete_array(p);//delete[] p;
		safe_delete(p->buffer);
		safe_delete(p);//delete[] p;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	/************ Processing finished ************/

	//Yeahlight: TODO: If a client gets flooded with too many packets, this loop may become infinite or close to it
	if (!packet_manager.CheckActive())
	{
		EQC::Common::PrintF(CP_CLIENT, "Client disconnected (!pm.CA): %s\n", GetName());
		client_state = CLIENT_DISCONNECTED;
		if(this->GetCraftingStation()!=0){//Tazadar: If Client goes LD we close the station
			this->ProcessOP_CraftingStation();
		}
		this->Save();
	}

	packet_manager.CheckTimers();
	MPacketManager.unlock();
}

APPLAYER* Client::PMOutQueuePop() 
{
	APPLAYER* tmp;
	MPacketManager.lock();
	tmp = packet_manager.OutQueue.pop();
	MPacketManager.unlock();
	return tmp;
}

void Client::PMClose()
{
	MPacketManager.lock();
	packet_manager.Close();
	MPacketManager.unlock();
}

void Client::QueuePacket(APPLAYER* app, bool ack_req) 
{	
	int16 breakOutCounter = 0;
	//Yeahlight: TODO: CRITICAL: HUGE issue here. When two or more PCs attempt to zone in together, one will get stuck in an infinite loop at this stage.
	//                           The breakout counter is a temporary "fix" to get around the issue.
	
	
	/* // remmed out - Dark-Prince
	while(packet_manager.IsTooMuchPending() && (client_state != CLIENT_DISCONNECTED) && breakOutCounter < 100)
	{ 
		// Merkur experimental testcode
		MPacketManager.lock();				// will this slow down the whole zone if someone has lag? it should and seems not, but it needs to be tested out :)
		packet_manager.CheckTimers();
		MPacketManager.unlock();
		Sleep(5); 
		breakOutCounter++;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	*/


	while(packet_manager.IsTooMuchPending())
	{ 
		// Check if the Yeahlights Break out Counter is greather than 100
		if( breakOutCounter > 100) // YeahLight's Breakout Counter Check
		{
			break; // if it is, break out of the loop
		}

		if(client_state != CLIENT_DISCONNECTED)
		{
			// Merkur experimental testcode
			MPacketManager.lock();				// will this slow down the whole zone if someone has lag? it should and seems not, but it needs to be tested out :)
			packet_manager.CheckTimers();
			MPacketManager.unlock();
			
			breakOutCounter++; // YeahLight's breakout Counter Check - Increase it by 1
			
			Sleep(5);
		}
	}


	ack_req = true;	// It's broke right now, dont delete this line till fix it. =P
	if (app != 0)
	{
		//Yeahlight: Keep track of each client's network footprint
		myNetworkFootprint += app->size;
		if (app->size >= 31500)
		{
			cout << "WARNING: abnormal packet size. n='" << this->GetName() << "', o=0x" << hex << app->opcode << dec << ", s=" << app->size << endl;
		}
	}
	MPacketManager.lock();
	packet_manager.MakeEQPacket(app, ack_req);
	MPacketManager.unlock();
	//cout<<"Sending out packet with an opcode of "<<app->opcode<<" and size of "<<app->size<<endl;
}

void Client::ReceiveData(uchar* buf, int len)
{
	timeout_timer->Start();
	MPacketManager.lock();
	packet_manager.ParceEQPacket(len, buf);
	MPacketManager.unlock();
}

void Client::SetMaxHP()
{
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Setting max HP.", this->GetName());
	this->SetHP(CalcMaxHP());
	this->SendHPUpdate();
	this->Save();
}

void Client::AddEXP(uint32 add_exp, bool resurrected)
{
	//Yeahlight: No exp was supplied; abort
	if(add_exp == 0)
		return;

	bool debugFlag = true;
	uint32 maxExpGain = 0;
	
	//Yeahlight: The PC is being resurrected, therefor there are no exp restrictions
	if(resurrected)
	{
		maxExpGain = 999999999;
	}
	//Yeahlight: A PC may not gain more than 10% of their level in one kill
	else
	{
		maxExpGain = GetTotalLevelExp() / 10.00f;
	}

	if(resurrected == false && debugFlag && GetDebugMe())
	{
		this->Message(LIGHTEN_BLUE, "Debug: Your experience gain from this kill: %i", add_exp);
		this->Message(LIGHTEN_BLUE, "Debug: It will take approximately %i occurances of this experience scenario to fill your level", maxExpGain * 10 / add_exp);
	}
	if(add_exp > maxExpGain)
	{
		add_exp = maxExpGain;
		if(debugFlag && GetDebugMe())
			this->Message(LIGHTEN_BLUE, "Debug: Exp gain reduced to %i since the previous value was greater than 10 percent of your current level", add_exp);
	}
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Adding %i experience points.", add_exp);
	this->SetEXP(GetEXP() + add_exp);
}

///////////////////////////////////////////////////
// Cofruben: SetEXP rewritten.
void Client::SetEXP(int32 set_exp)
{
	int32	actual_exp		= this->GetEXP();
	int8	actual_level	= this->GetLevel();	
	int32	target_exp		= set_exp;
	int8	target_level	= this->GetLevelForEXP(target_exp);
	
	if(target_level < 1 || target_level > MAX_PLAYERLEVEL || actual_exp == target_exp) 
		return;

	// 1. Update player profile.
	this->GetPlayerProfilePtr()->exp = target_exp;

	// 2. Are we winning or loosing exp? Show messages.
	if(target_exp > actual_exp)
	{
		if(this->IsGrouped())
		{
			Message(YELLOW, "You gain party experience!!");
		}
		else
		{
			Message(YELLOW, "You gain experience!!");
		}
	}
	else
	{
		Message(YELLOW, "You have lost experience.");
	}

	// 3. Are we gaining or loosing a level?
	bool UpdateLevel = true;

	if(target_level > actual_level)	
	{
		Message(YELLOW, "You have gained a level! Welcome to level %i!", target_level);
	}
	else if(target_level < actual_level)
	{
		Message(YELLOW, "You have lost a level! Welcome to level %i!", target_level);
	}
	else
	{
		UpdateLevel = false;
	}

	if(UpdateLevel)
	{
		SetLevel(target_level, false);
		UpdateWho();
	}

	// 4. Send the update packet
	SendExpUpdatePacket(target_exp); // Moved into own method - Dark-Prince
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Setting %i experience points.", set_exp);
}

// Sends a OP_ExpUpdate to the client notifying them of the experiance update - Dark-Prince
void Client::SendExpUpdatePacket(int32 set_exp)
{
	// Check if the update is greater than or equal to the maximum PC experiance allowed in 1 hit
	if(set_exp >= MAX_PC_EXPERIANCE)
	{
		cout << "Possible experiance hack attempt by \"" << this->GetName() << "\" . Experiance Value to high and is greater than : "  << MAX_PC_EXPERIANCE << endl;
		return;
	}

	// Create the Packet
	APPLAYER app = APPLAYER(OP_ExpUpdate, sizeof(ExpUpdate_Struct));

	// Allocate Buffer
	app.pBuffer = new uchar[app.size];

	// Map to Struct
	ExpUpdate_Struct* eu = (ExpUpdate_Struct*)app.pBuffer;

	// Set Experiance Update
	eu->exp = set_exp;
	
	// Send out Packet
	this->QueuePacket(&app);
}

void Client::MovePC(const char* zonename, float x, float y, float z, bool ignorerestrictions, bool useSummonMessage)
{
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::MovePC(zonename = %s, x = %f, y = %f, z = %f, ignore_restrictions = %i)", zonename, x, y, z, ignorerestrictions);
	zonesummon_ignorerestrictions = ignorerestrictions;

	//Yeahlight: We are using the GM summon struct, thus displaying, "You have been summoned!"
	if(useSummonMessage)
	{
		APPLAYER* outapp = new APPLAYER(OP_GMSummon, sizeof(GMSummon_Struct));
		memset(outapp->pBuffer, 0, outapp->size);
		GMSummon_Struct* gms = (GMSummon_Struct*) outapp->pBuffer;
		strcpy(gms->charname, this->GetName());
		strcpy(gms->gmname, this->GetName());

		if (zonename == 0 || strcmp(zone->GetShortName(), zonename) == 0)
		{
			strcpy(gms->zonename, zone->GetFileName());
		}
		else
		{
			strcpy(gms->zonename, "zonesummon");
			if (strcasecmp(zonename, zone->GetShortName()) != 0)
			{
				strcpy(zonesummon_name, zonename);
				zonesummon_x = x;
				zonesummon_y = y;
				zonesummon_z = z;
			}
		}
		gms->x = (sint32) x;
		gms->y = (sint32) y;
		gms->z = (sint32) z;
		int8 tmp[16] = { 0x7C, 0xEF, 0xFC, 0x0F, 0x80, 0xF3, 0xFC, 0x0F, 0xA9, 0xCB, 0x4A, 0x00, 0x7C, 0xEF, 0xFC, 0x0F };
		memcpy(gms->unknown2, tmp, 16);
		int8 tmp2[4] = { 0xE0, 0xE0, 0x56, 0x00 };
		memcpy(gms->unknown3, tmp2, 4);
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;
	}
	//Yeahlight: We will not be using the GM summon struct, so force a change locally
	else
	{
		APPLAYER* app = new APPLAYER(OP_ClientUpdate, sizeof(SpawnPositionUpdate_Struct));
		SpawnPositionUpdate_Struct* spu = (SpawnPositionUpdate_Struct*)app->pBuffer;
		memset(spu, 0, sizeof(SpawnPositionUpdate_Struct));
		spu->x_pos = x;
		spu->y_pos = y;
		spu->z_pos = z;
		spu->heading = GetHeading();
		spu->spawn_id = this->GetID();
		QueuePacket(app);
		safe_delete(app);
	}
	if(GetDebugMe())
		Message(LIGHTEN_BLUE, "Debug: Moving you to %.2f, %.2f, %.2f", x, y, z);
	SendPosUpdate(false, PC_UPDATE_RANGE, false);
}

// Sends out a OP_LevelUpdate packet -Dark-Prince 
void Client::SendLevelUpdate(int8 SetLevel)
{
	// Check if SetLevel is less than 1 or greater than MAX_PLAYERLEVEL
	if(SetLevel < 1 || SetLevel > MAX_PLAYERLEVEL)
		return;

	APPLAYER* outapp = new APPLAYER(OP_LevelUpdate, sizeof(LevelUpdate_Struct));
	LevelUpdate_Struct* lu = (LevelUpdate_Struct*)outapp->pBuffer;
	lu->level = SetLevel;
	lu->can_delevel = 1;
		
	QueuePacket(outapp);
	safe_delete(outapp);
}

void Client::SetLevel(int8 set_level, bool show_message)
{
	if(show_message)
		Message(YELLOW, "Welcome to level %i!", set_level);

	//update training points
	int8 diff = set_level - level;
	
	if(diff > 0) {
		GetPlayerProfilePtr()->trainingpoints=(5*diff);
	}
	
	SendLevelUpdate(set_level);
	SendAppearancePacket(this->GetID(), SAT_LevelChange, set_level, true);
	GetPlayerProfilePtr()->level = set_level;
	level = set_level;
	SetHP(CalcMaxHP());
	SendHPUpdate();
	SetMana(CalcMaxMana());
	UpdateWho();
	Save();
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SetLevel(set_level = %i, show_message = %i)", (int32)set_level, show_message);
}

//                            hum     bar     eru     elf     hie     def     hef     dwa     tro     ogr     hal    gno     iks,    vah
float  race_modifiers[14] =  {100.0f, 105.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 120.0f, 115.0f, 95.0f, 100.0f, 120.0f, 120.0f}; // Quagmire - Guessed on iks and vah
//                            war   cle    pal    ran    shd    dru    mnk    brd    rog    shm    nec    wiz    mag    enc
float class_modifiers[14] =  {9.0f, 10.0f, 14.0f, 14.0f, 14.0f, 10.0f, 12.0f, 14.0f, 9.05f, 10.0f, 11.0f, 11.0f, 11.0f, 11.0f};
/*
Note: The client calculates exp separatly, we cant change this function
7th of the 10th, 2007 - froglok23 made this method way simpler, now the calculation is only defined once
and reused.

Also made it so GetClass() is only called once and not 3 times.
*/
uint32 Client::GetEXPForLevel(int8 check_level)
{
	int8 tmprace = this->GetBaseRace();

	if (tmprace == IKSAR)
	{
		tmprace = 13;
	}
	else if (tmprace == VAHSHIR)
	{
		tmprace = 14;
	}

	tmprace -= 1;
	int8 tmpclass = GetClass();

	if (tmprace >= 14 || tmpclass < 1 || tmpclass > 14)
	{
		return 0xFFFFFFFF;
	}

	float modifier = 1.1;

	uint32 calc = (uint32)((check_level-1)*(check_level-1)*(check_level-1)*class_modifiers[tmpclass-1]*race_modifiers[tmprace]);

	if (check_level < 31)
	{
		return calc;
	}
	else if (check_level < 36)
	{
		modifier = 1.1;
	}
	else if (check_level < 41)
	{
		modifier = 1.2;
	}
	else if (check_level < 46)
	{
		modifier = 1.3;
	}
	else if (check_level < 52)
	{
		modifier = 1.4;
	}
	else if (check_level < 53)
	{
		modifier = 1.5;
	}
	else if (check_level < 54)
	{
		modifier = 1.6;
	}
	else if (check_level < 55)
	{
		modifier = 1.7;
	}
	else if (check_level < 56)
	{
		modifier = 1.9;
	}
	else if (check_level < 57)
	{
		modifier = 2.1;
	}
	else if (check_level < 58)
	{
		modifier = 2.3;
	}
	else if (check_level < 59)
	{
		modifier = 2.5;
	}
	else if (check_level < 60)
	{
		modifier = 2.7;
	}
	else if (check_level < 61)
	{
		modifier = 3.0;
	}
	else
	{
		modifier = 3.1;
	}

	return (calc * modifier);
}

int8 Client::GetLevelForEXP(uint32 exp)
{
	int8 lvl = 1;
	int32 exp_needed[MAX_PLAYERLEVEL + 1];

	for(lvl = 1; lvl <= MAX_PLAYERLEVEL; lvl++)
	{
		exp_needed[lvl] = GetEXPForLevel(lvl);
	}

	for(lvl = MAX_PLAYERLEVEL; lvl > 0; lvl--)
	{
		if(exp_needed[lvl] <= exp)
		{
			return lvl;
		}
	}
	return 1;
}

sint32 Client::CalcMaxHP() 
{
	max_hp = (CalcBaseHP() + itembonuses.HP + spellbonuses.HP);	

	if (cur_hp > max_hp)
	{
		cur_hp = max_hp;
	}

	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CalcMaxHP() returned %i", max_hp);

	return max_hp;
}

/*
Note: The client calculates max hp separatly, we cant change this function
7th of the 10th 2007 - froglok23 - added tmplevel = GetLevel() and reused the variable
*/
sint32 Client::CalcBaseHP()
{
	int8 multiplier = 0;
	int8 tmplevel = GetLevel();

	switch(GetClass())
	{
	case WARRIOR:
		if (tmplevel < 20)
		{
			multiplier = 22;
		}
		else if (tmplevel < 30)
		{
			multiplier = 23;
		}
		else if (tmplevel < 40)
		{
			multiplier = 25;
		}
		else if (tmplevel < 53)
		{
			multiplier = 27;
		}
		else if (tmplevel < 57)
		{
			multiplier = 28;
		}
		else
		{
			multiplier = 30;
		}
		break;

	case DRUID:
	case CLERIC:
	case SHAMAN:
		multiplier = 15;
		break;

	case PALADIN:
	case SHADOWKNIGHT:
		if (tmplevel < 35)
		{
			multiplier = 21;
		}
		else if (tmplevel < 45)
		{
			multiplier = 22;
		}
		else if (tmplevel < 51)
		{
			multiplier = 23;
		}
		else if (tmplevel < 56)
		{
			multiplier = 24;
		}
		else if (tmplevel < 60)
		{
			multiplier = 25;
		}
		else
		{
			multiplier = 26;
		}
		break;

	case MONK:
	case BARD:
	case ROGUE:
	case BEASTLORD:
		if (tmplevel < 51)
		{
			multiplier = 18;
		}
		else if (tmplevel < 58)
		{
			multiplier = 19;
		}
		else
		{
			multiplier = 20;
		}
		break;

	case RANGER:
		if (tmplevel < 58)
		{
			multiplier = 20;
		}
		else
		{
			multiplier = 21;			
		}
		break;

	case MAGICIAN:
	case WIZARD:
	case NECROMANCER:
	case ENCHANTER:
		multiplier = 12;
		break;

	default:
		//cerr << "Unknown/invalid class in Client::CalcBaseHP" << endl;
		if (tmplevel < 35)
		{
			multiplier = 21;
		}
		else if (tmplevel < 45)
		{
			multiplier = 22;
		}
		else if (tmplevel < 51)
		{
			multiplier = 23;
		}
		else if (tmplevel < 56)
		{
			multiplier = 24;
		}
		else if (tmplevel < 60)
		{
			multiplier = 25;
		}
		else
		{
			multiplier = 26;
		}
		break;
	}

	if (multiplier == 0)
	{
		cerr << "Multiplier == 0 in Client::CalcBaseHP" << endl;
	}

	base_hp = 5+multiplier*tmplevel+multiplier*tmplevel*GetSTA()/300;
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CalcBaseHP() returned %i", base_hp);
	return base_hp;
}


int8 Client::GetSkill(int skill_num)
{
	if (skill_num > sizeof(pp.skills)/sizeof(pp.skills[0]))
	{
		skill_num = 0;
	}
	return pp.skills[skill_num];
}

// Sends OP_SkillUpdate - Dark-Prince
void Client::SendSkillUpdate(int SkillID, int8 Value)
{
	if(SkillID < _1H_BLUNT || SkillID > TAUNT)
	{
		return;
	}

	// Create the Packet
	APPLAYER* outapp = new APPLAYER(OP_SkillUpdate, sizeof(SkillUpdate_Struct));

	// Set out the Buffer
	memset(outapp->pBuffer, 0, sizeof(outapp->size));

	// Map to Struct
	SkillUpdate_Struct* skill = (SkillUpdate_Struct*)outapp->pBuffer;

	skill->skillId = SkillID;
	skill->value = Value;
	QueuePacket(outapp);
	safe_delete(outapp);
}

void Client::SetSkill(int skillid, int8 value)
{
	pp.skills[skillid] = value;
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SetSkill(skillid = %i, value = %i", skillid, (int8)value);

	this->SendSkillUpdate(skillid, value);

	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SetSkill(skillid = %i, value = %i)", skillid, (int32)value);
}

//o--------------------------------------------------------------
//| SendLanguageSkillUpdate; Harakiri, August 10, 2009
//o--------------------------------------------------------------
//| Send Skill Update for a Language to Client
//o--------------------------------------------------------------
void Client::SendLanguageSkillUpdate(int languageID, int8 Value)
{
	if(languageID < LANGUAGE_COMMON_TONGUE || languageID > LANGUAGE_DARK_SPEECH)
	{
		return;
	}

	// Harakiri
	// Note there is no update packet defined for languages, im 99.99% sure, the OP_SkillUpdate
	// just prints out the message you have become better at ..., but only for skills below 100
	// there is no print method for languages, i disasm the eqgame.exe to be certain
	// furthermore, if you use this skill for party chat to learn new languages
	// you have to zone for the skill update to take a effect in the client gui and when reading unknown
	// languages , this is the same as in vanilla, FYI as long as a language skill is below 24 (not 25)
	// the client will see "says in an unknown tongue" when somebody speaks to him, this is hardwired in the
	// client

	// Harakiri edit 26 Aug 2009
	// i found multiple confirmations that you didnt see the You have become better at
	// but just Your language skills have improved., the current eq live string table confirms this
	//Message(BLUE,"You have become better at %s! (%d)",GetLanguageName(languageID),Value);
	Message(BLUE,"Your language skills have improved.");

}

//o--------------------------------------------------------------
//| SetLanguageSkill; Harakiri, August 10, 2009
//o--------------------------------------------------------------
//| Increase language Skill
//o--------------------------------------------------------------
void Client::SetLanguageSkill(int languageID, int8 value)

{
	if(languageID < LANGUAGE_COMMON_TONGUE || languageID > LANGUAGE_DARK_SPEECH)
	{
		return;
	}

	if(value <= 100) {
		pp.languages[languageID] = value;
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SetLanguageSkill(languageid = %i, value = %i", languageID, (int8)value);

		this->SendLanguageSkillUpdate(languageID, value);

		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SetLanguageSkill(languageid = %i, value = %i)", languageID, (int32)value);
	}
}
//o--------------------------------------------------------------
//| TeachLanguage; Harakiri, August 11, 2009
//o--------------------------------------------------------------
//| Increase language Skill through party/say
//o--------------------------------------------------------------
void Client::TeachLanguage(Client* student, int8 language, int8 chan_num) {

	// no self teaching 
	if(student == 0 || (student->GetID() == this->GetID()) ) {
		return;
	}

	// only say or party can increase the lang
	if( (chan_num == MessageChannel_SAY && this->Dist(student) < 100) || chan_num == MessageChannel_GSAY) {			
			int langSkillTeacher = GetLanguageSkill(language);
			int langSkillStudent = student->GetLanguageSkill(language);

			Message(BLACK,"Debug: Channel = %i TeachLanguage skillTeacher = %i skillStudent = %i",chan_num,langSkillTeacher,langSkillStudent);

			// a student can get better as a teacher, by exactly + 1
			if((langSkillTeacher >= langSkillStudent) && (langSkillStudent < 100)) {

				int chance = MakeRandomInt(1,100);
				// Harakiri 5% should be fine i guess
				if(chance > 95) {
					student->SetLanguageSkill(language,langSkillStudent+1);
				}
			}		
	}
}

sint16	Client::GetSTR()		
{ 
	sint16 STR=GetBaseSTR() + itembonuses.STR + spellbonuses.STR;
	if(STR<1)
		return 1;
	else if(STR<STAT_CAP)
		return STR;
	else
		return STAT_CAP;
}
sint16	Client::GetSTA()		
{ 
	sint16 STA=GetBaseSTA() + itembonuses.STA + spellbonuses.STA; 
	if(STA<1)
		return 1;
	else if(STA<STAT_CAP)
		return STA;
	else
		return STAT_CAP;
}
sint16	Client::GetDEX()		
{ 
	sint16 DEX= GetBaseDEX() + itembonuses.DEX + spellbonuses.DEX;
	if(DEX<1)
		return 1;
	else if(DEX<STAT_CAP)
		return DEX;
	else
		return STAT_CAP;
}
sint16	Client::GetAGI()		
{ 
	sint16 AGI= GetBaseAGI() + itembonuses.AGI + spellbonuses.AGI;
	if(AGI<1)
		return 1;
	else if(AGI<STAT_CAP)
		return AGI;
	else
		return STAT_CAP;
}
sint16	Client::GetINT()		
{ 
	sint16 INT= GetBaseINT() + itembonuses.INT + spellbonuses.INT;
	if(INT<1)
		return 1;
	else if(INT<STAT_CAP)
		return INT;
	else
		return STAT_CAP;
}
sint16	Client::GetWIS()		
{ 
	sint16 WIS= GetBaseWIS() + itembonuses.WIS + spellbonuses.WIS;
	if(WIS<1)
		return 1;
	else if(WIS<STAT_CAP)
		return WIS;
	else
		return STAT_CAP;
}
sint16	Client::GetCHA()		
{
	sint16 CHA= GetBaseCHA() + itembonuses.CHA + spellbonuses.CHA;
	if(CHA<1)
		return 1;
	else if(CHA<STAT_CAP)
		return CHA;
	else
		return STAT_CAP;
}

/********************************************************************
*                      Tazadar - 07/28/08                          *
********************************************************************
*                         UpdateGoods                              *
********************************************************************
*  + Show the goods to the player						            *
*									                                *
********************************************************************/
void Client::UpdateGoods(int merchantid){
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_MERCHANT, "Client::UpdateGoods(merchantid = %i)", merchantid);
	this->startingslot=1;
	memset(&this->merchantgoods[0],0xFFFF,sizeof(merchantgoods));
	int itemdisplayed=0;
	int bugeditems=0;
	int x=0;
	bool first=true;
	int32 maxvendoritems=Database::Instance()->GetMerchantListNumb(merchantid);
	while(itemdisplayed < maxvendoritems && itemdisplayed-bugeditems <30)
	{
		//cout << "item displayed " << (int)itemdisplayed << " total item: "<<(int)maxvendoritems << endl;
		Item_Struct* item = 0;
		x++;
		int32 item_nr = Database::Instance()->GetMerchantData(merchantid,x);
		
		if(item_nr!=0 && item_nr!=-1)
		{
			 item = Database::Instance()->GetItem(item_nr);
		}

		if(item!=0)
		{
			if(first)
			{
				this->startingslot=x;
				first=false;
			}
			//Remove after Alfa !!
			if(this->admin>4){
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_MERCHANT, "Client::UpdateGoods(): %i: %s", (int) item->item_nr, item->name);
			}
			//endromove
			this->merchantgoods[itemdisplayed-bugeditems]=item_nr;
			itemdisplayed++;
			//		cout << "merchant is selling: " << item->name << "  -> Weight: " << (int)item->weight<< endl;
			
		item->equipSlot = itemdisplayed - 1 - bugeditems;			// this needs to be incremented in loop.
		SendShopItem(DEFAULT_ITEM_CHARGES, merchantid, item);

			
		}
		if(item==0 && item_nr!=-1){//Tazadar : We have a buggy item
			//Remove after Alfa !!
			if(this->admin>4){
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_MERCHANT, "Client::UpdateGoods(): Bugged item id = %i !.",(int)item_nr);
			}
			//end remove
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_MERCHANT, "Client::UpdateGoods(): There is no item found in database for the current merchantid, slot request.",(int)startingslot);
			cout << "There is no item found in database for the current merchantid, slot request." << endl;
			itemdisplayed++;
			bugeditems++;
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	this->totalitemdisplayed=itemdisplayed-bugeditems;
}

void Client::SendShopItem(sint8 ItemCharges, uint32 MerchantID, Item_Struct* Item)
{
	// Create Packet
	APPLAYER* outapp = new APPLAYER(OP_ShopItem, sizeof(Item_Shop_Struct));

	// Map to Struct
	Item_Shop_Struct* iss = (Item_Shop_Struct*)outapp->pBuffer;

	// Zero Out Buffer
	memset(outapp->pBuffer, 0, sizeof(Item_Shop_Struct));

	// Set Charges (default is 1)
	Item->common.charges=ItemCharges;

	// Set the Merchant ID
	iss->merchantid = MerchantID;

	// Set the Item Type
	iss->itemtype = Item->type;

	// Set Unknown
	iss->iss_unknown001[0] = 0;
	
	// Copy the Item to the ShopItem Items property (Item_Shop_Struct->item)
	memcpy(&iss->item, Item, sizeof(Item_Struct));
	
	// Queue Packet
	QueuePacket(outapp);

	// Delete it
	safe_delete(outapp);
}

/********************************************************************
 *                      Tazadar - 07/28/08                          *
 ********************************************************************
 *                         UpdateGoodsBuy                           *
 ********************************************************************
 *  + Show the goods to the player after he buys			        *
 *									                                *
 ********************************************************************/
void Client::UpdateGoodsBuy(int8 startingslot){
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_MERCHANT, "Client::UpdateGoodsBuy(startingslot = %i)",(int)startingslot);
	for(int i=startingslot;i<this->totalitemdisplayed;i++){
		this->merchantgoods[i]=this->merchantgoods[i+1];
	}
	this->merchantgoods[this->totalitemdisplayed]=0xFFFF;
	this->totalitemdisplayed--;

	for(int i=0;i<this->totalitemdisplayed;i++)
	{
		Item_Struct* item = 0;
		if(this->merchantgoods[i]!=0xFFFF){
			 item = Database::Instance()->GetItem(this->merchantgoods[i]);
		}
		if(item!=0)
		{				
			item->equipSlot = i;
			this->SendShopItem(DEFAULT_ITEM_CHARGES, merchantid, item);

		}
		else
		{
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_MERCHANT, "Client::UpdateGoodsBuy(): There is no item found in database for the current merchantid, slot request.");
			cout << "There is no item found in database for the current merchantid, slot request." << endl;
			return;
		}
	}

	APPLAYER* outapp = new APPLAYER(OP_ShopDelItem, sizeof(Merchant_DelItem_Struct));
	Merchant_DelItem_Struct* del = (Merchant_DelItem_Struct*)outapp->pBuffer;
	memset(outapp->pBuffer, 0, sizeof(Merchant_DelItem_Struct));
	del->npcid=merchantid;			
	del->playerid=this->GetID();		
	del->itemslot=this->totalitemdisplayed;

	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;

}

/********************************************************************
 *                      Tazadar - 07/28/08                          *
 ********************************************************************
 *                         UpdateGoodsSell                          *
 ********************************************************************
 *  + Vendor shows the good to the player after he sells it 	    *
 *									                                *
 ********************************************************************/
void Client::UpdateGoodsSell(int32 item_nr){
	if(this->totalitemdisplayed == 30){//we dont show the item we reached max amount of viewable items
		return;
	}
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_MERCHANT, "Client::UpdateGoodsSell(item_nr = %i)",(int)item_nr);

	this->totalitemdisplayed++;
	this->merchantgoods[this->totalitemdisplayed-1]=item_nr;


	Item_Struct* item = Database::Instance()->GetItem(item_nr);

	if(item!=0){
		APPLAYER* outapp = new APPLAYER(OP_ShopItem, sizeof(Item_Shop_Struct));
		Item_Shop_Struct* iss = (Item_Shop_Struct*)outapp->pBuffer;
		memset(outapp->pBuffer, 0, sizeof(Item_Shop_Struct));
		item->common.charges=1;
		iss->merchantid = this->merchantid;
		iss->itemtype = item->type;			
		item->equipSlot = this->totalitemdisplayed-1;
		memcpy(&iss->item, item, sizeof(Item_Struct));

		iss->iss_unknown001[0] = 0;
		QueuePacket(outapp);
		safe_delete(outapp);
	}else{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_MERCHANT, "Client::UpdateGoodsSell(): There is no item found in database for the current merchantid, slot request.");
		cout << "There is no item found in database for the current merchantid, slot request." << endl;
		return;
	}
}

/********************************************************************
*                      Tazadar - 07/29/08                          *
********************************************************************
*                        ShowedVendorItem                          *
********************************************************************
*  + Tell us if the item is displayed on vendor window	            *
*									                                *
********************************************************************/
bool Client::ShowedVendorItem(int32 item_nr){
	for(int i=0;i<30;i++){
		if(this->merchantgoods[i]==item_nr){
			return true;
		}
	}
	return false;
}

void Client::UpdateWho(bool remove) 
{
	if (account_id == 0)
	{
		return;
	}
	if (!worldserver.Connected())
	{
		return;
	}
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::UpdateWho(remove = %i)", remove);

	ServerPacket* pack = new ServerPacket(ServerOP_ClientList, sizeof(ServerClientList_Struct));

	//pack->pBuffer = new uchar[pack->size]; //Taz : Memory leak !!
	memset(pack->pBuffer, 0, pack->size);
	ServerClientList_Struct* scl = (ServerClientList_Struct*) pack->pBuffer;
	scl->remove = remove;
	strcpy(scl->name, this->GetName());
	scl->Admin = this->Admin();
	scl->AccountID = this->AccountID();
	strcpy(scl->AccountName, this->AccountName());
	strcpy(scl->zone, zone->GetShortName());
	scl->race = this->GetRace();
	scl->class_ = GetClass();
	scl->level = GetLevel();

	if (pp.anon == 0)
	{
		scl->anon = 0;
	}
	else if (pp.anon == 1)
	{
		scl->anon = 1;
	}
	else if (pp.anon >= 2)
	{
		scl->anon = 2;
	}

	scl->tellsoff = tellsoff;
	scl->guilddbid = guilddbid;
	scl->guildeqid = guildeqid;
	scl->LFG = LFG;

	worldserver.SendPacket(pack);
	safe_delete(pack);//delete pack;
}

// Zone client to spot specified, cords all = 0xFFFF for safe point
/*void Client::MovePC(char* zonename, sint16 x, sint16 y, sint16 z) {
zonesummon_x = x;
zonesummon_y = y;
zonesummon_z = z;
APPLAYER* outapp = new APPLAYER;
outapp->opcode = OP_Translocate;
outapp->size = 88;
outapp->pBuffer = new uchar[outapp->size];
memset(outapp->pBuffer, 0, outapp->size);
if (zonename == 0)
strcpy((char*) outapp->pBuffer, zone->GetShortName());
else
strcpy((char*) outapp->pBuffer, zonename);
int8 tmp[68] = {
0x10, 0xd2, 0x3f, 0x04, 0x86, 0xf3, 0xc4, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x46, 0x00, 0xa0, 0xa0, 0x6d, 0x0d,
0x80, 0x15, 0xd5, 0x06, 0xf4, 0xd2, 0xd2, 0x0c, 0xf5, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
0x04, 0x00, 0x00, 0x00, 0xf4, 0xca, 0x84, 0x08, 0x00, 0x00, 0xfa, 0xc6, 0x00, 0x00, 0xfa, 0xc6,
0x00, 0x00, 0xfa, 0xc6 };
memcpy(&outapp->pBuffer[16], tmp, 68);
outapp->pBuffer[84] = 1;
QueuePacket(outapp);
safe_delete(outapp);//delete outapp;
}*/



void Client::WhoAll(Who_All_Struct* whom)
{
	if (!worldserver.Connected())
	{
		Message(BLACK, "Error: World server disconnected");
	}
	else
	{
		ServerPacket* pack = new ServerPacket(ServerOP_Who, sizeof(ServerWhoAll_Struct));

		//pack->pBuffer = new uchar[pack->size]; // Taz : Memory leak !!
		memset(pack->pBuffer, 0, sizeof(pack->pBuffer));
		ServerWhoAll_Struct* whoall = (ServerWhoAll_Struct*) pack->pBuffer;
		int16 adminLevel = 0;
		//Yeahlight: Support for low end testers to use #worldkick
		if(admin >= EQC_Alpha_Tester)
			adminLevel = 255;
		else
			adminLevel = 0;
		whoall->admin = adminLevel;
		strcpy(whoall->from, this->GetName());
		strcpy(whoall->whom, whom->whom);
		whoall->firstlvl = whom->firstlvl;
		whoall->gmlookup = whom->gmlookup;
		whoall->secondlvl = whom->secondlvl;
		whoall->wclass = whom->wclass;
		whoall->wrace = whom->wrace;
		worldserver.SendPacket(pack);
		safe_delete(pack);//delete pack;
	}
}


void Client::UpdateAdmin()
{
	admin = Database::Instance()->CheckStatus(account_id);
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::UpdateAdmin(): admin: %i", (int)admin);
	UpdateWho();
	
	int32 parameter = 0;
	
	if (admin >= 100)
	{
		parameter = 1;
	}

	this->SendAppearancePacket(this->GetID(), SAT_GM_Tag, parameter, true);
}

void Client::FindItem(char* search_criteria)
{
	int count=0;
	int iSearchLen = strlen(search_criteria)+1;
	char sName[36];
	char sCriteria[255];

	strcpy(sCriteria, search_criteria);
	_strupr(sCriteria);
	Item_Struct* item = 0;
	char* pdest;

	// -Cofruben: changing to shared memory method.
#ifndef EQC_SHAREDMEMORY
	for (int i=0; i < Database::Instance()->max_item; i++)
	{
		if (Database::Instance()->item_array[i] != 0)
		{
			item = (Item_Struct*)Database::Instance()->item_array[i];
#else
	for (int i=0; i < SharedMemory::getMaxItem(); i++) {
		{
			item = SharedMemory::getItem(i);
#endif
			strcpy(sName, item->name);
			_strupr(sName);

			pdest = strstr(sName, sCriteria);
			if (pdest != NULL) {
				Message(BLACK, "  %i: %s", (int) item->item_nr, item->name);
				count++;
			}
			if (count == 20) {
				break;
			}
		}
	}
	if (count == 20)
		Message(BLACK, "20 items shown...too many results.");
	else
		Message(BLACK, "%i items found", count);
}


bool Client::SummonItem(int16 item_id, sint8 charges) {
	Item_Struct* item = Database::Instance()->GetItem(item_id);		
	if (item == 0) {
		Message(BLACK, "No such item: %i", item_id);
		return false;
	}
	else if(CheckLoreConflict(item_id)){// Tazadar : Lore item that we already have
		return false;
	}
	else if(pp.inventory[0] != 0xFFFF){ // Tazadar : Item in hands
		cout << " hands are full " << endl; 
		SummonedItemWaiting_Struct tmp;
		tmp.charge = charges;
		tmp.itemID = item_id;
		this->summonedItems.push_back(tmp);
		return true;
	}
	else {
		pp.inventory[0] = item_id;
		
		ItemInst *i = new ItemInst(item, 0);
		
		// Harakiri only apply charges when its a consumable item, else use DB defaults
		if(charges > 0 && i->IsConsumable()) {
			pp.invItemProprieties[0].charges = charges;
			item->common.charges = charges;
		} else {
			pp.invItemProprieties[0].charges = item->common.charges;			
		}

		APPLAYER* outapp = new APPLAYER(OP_SummonedItem, sizeof(SummonedItem_Struct));
		memcpy(outapp->pBuffer, item, sizeof(Item_Struct));
		Item_Struct* item2 = (Item_Struct*) outapp->pBuffer;
		item2->equipSlot = 0;
		
		QueuePacket(outapp);
		safe_delete(outapp);
		return true;
	}
}

bool Client::SummonItemNonBlob(int16 item_id, sint8 charges) {
	Item_Struct* item = Database::Instance()->GetItemNonBlob(item_id);
	if (item == 0) {
		Message(BLACK, "No such item: %i", item_id);
		return false;
	}
	else if(CheckLoreConflict(item_id)){// Tazadar : Lore item that we already have
		return false;
	}
	else if(pp.inventory[0] != 0xFFFF){ // Tazadar : Item in hands
		cout << " hands are full " << endl; 
		SummonedItemWaiting_Struct tmp;
		tmp.charge = charges;
		tmp.itemID = item_id;
		this->summonedItems.push_back(tmp);
		return true;
	}
	else {
		pp.inventory[0] = item_id;

		ItemInst *i = new ItemInst(item, 0);
			
		// Harakiri only apply charges when its a consumable item, else use DB defaults
		if(charges > 0 && i->IsConsumable()) {
			pp.invItemProprieties[0].charges = charges;
			item->common.charges = charges;
		} else {
			pp.invItemProprieties[0].charges = item->common.charges;			
		}
					
		APPLAYER* outapp = new APPLAYER(OP_SummonedItem, sizeof(SummonedItem_Struct));
		memcpy(outapp->pBuffer, item, sizeof(Item_Struct));
		Item_Struct* item2 = (Item_Struct*) outapp->pBuffer;
		item2->equipSlot = 0;						

		QueuePacket(outapp);
		safe_delete(outapp);
		return true;
	}
}



sint32 Client::SetMana(sint32 amount) {
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SetMana(amount = %i)", amount);
	Mob::SetMana(amount);
	SendManaUpdatePacket();
	return cur_mana;
}



//I reverted this to an older one to get #mana working again. If this breaks anything or causes
//an undesirable result, delete the first part and uncomment out the part below it.
// - Wizzel
void Client::SendManaUpdatePacket()
{
	if (!Connected())
		return;
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SendManaUpdatePacket()");
	APPLAYER* outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
	ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
	manachange->new_mana = cur_mana;
	manachange->spell_id = 0;
	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
}

/*{
if (!Connected())
{
return;
}

<<<<<<< .mine
=======
/*APPLAYER* outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
manachange->new_mana = this->cur_mana;
manachange->spell_id = 0;
this->QueuePacket(outapp);
safe_delete(outapp);//delete outapp;*/
/*
>>>>>>> .r198
this->SendManaChange(this->cur_mana, 0);
}
*/


void Client::FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho)
{
	Mob::FillSpawnStruct(ns, ForWho);

	if (guildeqid == 0xFFFFFFFF)
	{
		ns->spawn.GuildID = 0xFFFF;
		ns->spawn.guildrank = GuildUnknown;
	}
	else
	{
		zgm.LoadGuilds();
		Guild_Struct* guilds = zgm.GetGuildList();

		ns->spawn.GuildID = guildeqid;

		if (guilds[guildeqid].rank[guildrank].warpeace || guilds[guildeqid].leader == account_id)
		{
			ns->spawn.guildrank = GuildLeader;
		}
		else if (guilds[guildeqid].rank[guildrank].invite || guilds[guildeqid].rank[guildrank].remove || guilds[guildeqid].rank[guildrank].motd)
		{
			ns->spawn.guildrank = GuildOffice;
		}
		else
		{
			ns->spawn.guildrank = GuildMember;
		}
	}

	if (ForWho == this)
		ns->spawn.NPC = 10;
	else
		ns->spawn.NPC = 0;


	/*if (GetPVP())
	ns->spawn.pvp = 1;
	*/
	if (pp.gm==1)
		ns->spawn.GM = 1;
	ns->spawn.anon = pp.anon;

	ns->spawn.x_pos = this->GetX();
	ns->spawn.y_pos = this->GetY();
	ns->spawn.z_pos = this->GetZ() * 10;
	ns->spawn.heading = this->GetHeading();

	ns->spawn.npc_armor_graphic = texture;
	ns->spawn.npc_helm_graphic = helmtexture;

	Item_Struct* item = 0;
	item = Database::Instance()->GetItem(pp.inventory[2]);
	if (item != 0) {
		ns->spawn.equipment[0] = item->common.material;
		ns->spawn.equipcolors[0] = item->common.color;
	}
	item = Database::Instance()->GetItem(pp.inventory[17]);
	if (item != 0) {
		ns->spawn.equipment[1] = item->common.material;
		ns->spawn.equipcolors[1] = item->common.color;
	}
	item = Database::Instance()->GetItem(pp.inventory[7]);
	if (item != 0) {
		ns->spawn.equipment[2] = item->common.material;
		ns->spawn.equipcolors[2] = item->common.color;
	}
	item = Database::Instance()->GetItem(pp.inventory[10]);
	if (item != 0) {
		ns->spawn.equipment[3] = item->common.material;
		ns->spawn.equipcolors[3] = item->common.color;
	}
	item = Database::Instance()->GetItem(pp.inventory[12]);
	if (item != 0) {
		ns->spawn.equipment[4] = item->common.material;
		ns->spawn.equipcolors[4] = item->common.color;
	}
	item = Database::Instance()->GetItem(pp.inventory[18]);
	if (item != 0) {
		ns->spawn.equipment[5] = item->common.material;
		ns->spawn.equipcolors[5] = item->common.color;
	}
	item = Database::Instance()->GetItem(pp.inventory[19]);
	if (item != 0) {
		ns->spawn.equipment[6] = item->common.material;
		ns->spawn.equipcolors[6] = item->common.color;
	}

	item = Database::Instance()->GetItem(pp.inventory[13]);
	if (item != 0)
	{
		if (strlen(item->idfile) >= 3)
			ns->spawn.equipment[7] = (int8) atoi(&item->idfile[2]);
		else
			ns->spawn.equipment[7] = 0;
	}
	item = Database::Instance()->GetItem(pp.inventory[14]);
	if (item != 0)
	{
		if (strlen(item->idfile) >= 3) 
			ns->spawn.equipment[8] = (int8) atoi(&item->idfile[2]);
		else 
			ns->spawn.equipment[8] = 0;
	}
}



int16 Client::GetItemAt(int16 in_slot)
{
	if (in_slot <= 29) // Worn items and main inventory
	{
		return pp.inventory[in_slot];
	}
	else if (in_slot >= 250 && in_slot <= 329) // Main inventory's containers
	{
		return pp.containerinv[in_slot-250];
	}
	else if (in_slot >= 2000 && in_slot <= 2007) // Bank slots
	{
		return pp.bank_inv[in_slot-2000];
	}
	else if (in_slot >= 2030 && in_slot <= 2109) // Bank's containers
	{
		return pp.bank_cont_inv[in_slot-2030];
	}
	else
	{
		cerr << "Error: " << GetName() << ": GetItemAt(): Unknown slot: 0x" << hex << setw(4) << setfill('0') << in_slot << dec << endl;
		Message(BLACK, "Error: GetItemAt(): Unknown slot: 0x%04x", in_slot);
	}
	return 0;
}


/********************************************************************
*                      Tazadar - 12/17/08                          *
********************************************************************
*                         RemoveOneCharge		                    *
********************************************************************
*  + Remove One charge from an item when using right click         *
*									                                *
********************************************************************/
void Client::RemoveOneCharge(uint32 slotid, bool deleteItemOnLastCharge)
{
	
	int16 itemID =  GetItemAt(slotid);

	
	ItemInst *itemInst = new ItemInst(Database::Instance()->GetItem(itemID), 0);
	// Harakiri imported signed int to not let overlap to 255
	sint8 charges=-1;
	// Harakiri - check if we remove a charge from inventory or bags
	if (slotid <= 29) {
		charges = pp.invItemProprieties[slotid].charges;
	} else if (slotid >= 250 && slotid <= 329) {
		charges = pp.bagItemProprieties[slotid-250].charges;
	}
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT, "RemoveOneCharge(slotid = %i) (itemid = %i) (current charges = %i)", slotid,itemID,charges);		

	if(charges == -1){//Tazadar:We have unlimited charge !!
		return;
	// Harakiri last charge, should be delete?
	} if(charges == 1 && deleteItemOnLastCharge && itemInst->IsConsumable()) {
		EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Item has 0 charges left and is consumable, deleting");
		DeleteItemInInventory(slotid);
	} else{//Tazadar: We have limited charge
		// Worn items and main inventory
		if (slotid <= 29) {
			pp.invItemProprieties[slotid].charges=charges-1;
			PutItemInInventory(slotid,pp.inventory[slotid],charges-1);
		// bags ?
		} else if (slotid >= 250 && slotid <= 329) {
			pp.bagItemProprieties[slotid-250].charges=charges-1;
			PutItemInInventory(slotid,pp.containerinv[slotid-250],charges-1);
		}
	}
}

void Client::DeleteItemInInventory(uint32 slotid)	
{
	const ItemInst* itemToDelete =  new ItemInst( GetItemAt(slotid), 0);

	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::DeleteItemInInventory(slotid = %i)", slotid);
	int8 charges=1;
	if (slotid <= 29) // Worn items and main inventory
	{
		pp.inventory[slotid] = 0xFFFF;
		charges=pp.invItemProprieties[slotid].charges;
		pp.invItemProprieties[slotid].charges=0;
	}
	else if (slotid >= 250 && slotid <= 329) // Main inventory's containers
	{
		pp.containerinv[slotid-250] = 0xFFFF;
		charges=pp.bagItemProprieties[slotid-250].charges;
		pp.bagItemProprieties[slotid-250].charges=0;
	}
	else if (slotid >= 2000 && slotid <= 2007) // Bank slots
	{
		pp.bank_inv[slotid-2000] = 0xFFFF;
		charges=pp.bankinvitemproperties[slotid-2000].charges;
		pp.bankinvitemproperties[slotid-2000].charges=0;
	}
	else if (slotid >= 2030 && slotid <= 2109) // Bank's containers
	{
		pp.bank_cont_inv[slotid-2030] =0xFFFF;
		charges=pp.bankbagitemproperties[slotid-2030].charges;
		pp.bankbagitemproperties[slotid-2030].charges=0;
	}
	else if (slotid >= 4000 && slotid <= 4009) // Bank's containers
	{
		this->stationItems[slotid-4000] =0xFFFF;
		charges=this->stationItemProperties[slotid-4000].charges;
		this->stationItemProperties[slotid-4000].charges=0;
	}
	else 
	{
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::DeleteItemInInventory(): Error: unknown slot");
		EQC::Common::PrintF(CP_CLIENT, "Error in DeleteItemInInventory : Unknown slot\n");
		return;
	}

	// Harakiri Move Item Cannot be used to delete charges if the client already removed one charge for example when 
	// the clients consumes food or somebody right clicks on alchol - the client removes the charge already - so the item
	// is still there with 0 charges
	// Client will print MOVE ITEM FAILED IN CLIENT APPLICATION
	// trick is - sent him the item again - with 1 charge (clients think it has 0 before) - then really remove it
	// maybe there is a real delete item opcode - havent found it yet
	// Harakiri: Fool Client to have at least 1 charge left
	if(itemToDelete->IsConsumable() && charges == 1) {						
		APPLAYER* app = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));
		app->pBuffer = new uchar[app->size];
		memcpy(app->pBuffer, itemToDelete->GetItem(), sizeof(Item_Struct));
		Item_Struct* outitem = (Item_Struct*) app->pBuffer;
		outitem->common.charges=1;	
		outitem->equipSlot = slotid;
		QueuePacket(app);
		safe_delete(app);
	}

	APPLAYER* outapp = new APPLAYER(OP_MoveItem, sizeof(MoveItem_Struct));
	MoveItem_Struct* delitem = (MoveItem_Struct*)outapp->pBuffer;
	delitem->from_slot = slotid;
	delitem->to_slot = 0xFFFFFFFF;
	delitem->number_in_stack = charges;
							
	QueuePacket(outapp);
	safe_delete(outapp);
	Save();	
}


/********************************************************************  
*                          Tazadar - 7/16/08                       *  
********************************************************************  
*                          DeleteItemInStation                     *  
********************************************************************  
*  + Clear items in the station ! ^^							    *  
*											                        *  
********************************************************************/
void Client::DeleteItemInStation()
{
	memset(this->stationItems,0,sizeof(this->stationItems));
	memset(this->stationItemProperties,0,sizeof(this->stationItemProperties));
	APPLAYER* pApp = new APPLAYER(OP_CleanStation,0);
	QueuePacket(pApp);
	safe_delete(pApp);
}



bool Client::GMHideMe(Client* client)
{
	if (gmhideme)
	{
		if (client == 0)
		{
			return true;
		}
		else if (admin > client->Admin())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}



void Client::Duck(bool send_update_packet)
{
	SetFeigned(false);
	if (send_update_packet) this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Ducking, true);
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::Duck(send_update_packet = %i)", send_update_packet);
	appearance = DuckingAppearance;	
	if (this->GetCastingSpell() && !IsBard() || (IsBard() && this->GetCastingSpell() && !this->GetCastingSpell()->IsBardSong()))
		InterruptSpell();	// Pinedepain // If we are a bard, and using a song, the duck state doesn't interrupt our song.
}

void Client::Sit(bool send_update_packet)
{
	SetFeigned(false);
	if (send_update_packet) this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Standing_To_Sitting, true);
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::Sit(send_update_packet = %i)", send_update_packet);
	appearance = SittingAppearance;	
	if (this->GetCastingSpell() && !IsBard() || (IsBard() && this->GetCastingSpell() && !this->GetCastingSpell()->IsBardSong()))
		InterruptSpell();	// Pinedepain // If we are a bard, and using a song, the duck state doesn't interrupt our song.

	// Moraj: Cancel Bind Wound
	BindWound(this,true);
}


void Client::Stand(bool send_update_packet)
{
	SetFeigned(false);
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::Stand(send_update_packet = %i)", send_update_packet);
	if(send_update_packet) 
		this->SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Sitting_To_Standing, true);
	appearance = StandingAppearance;
	if(this->GetCastingSpell() && !IsBard() || (IsBard() && this->GetCastingSpell() && !this->GetCastingSpell()->IsBardSong()))
		InterruptSpell();	// Pinedepain // If we are a bard, and using a song, the duck state doesn't interrupt our song.
	Mob* myPet = this->GetPet();
	if(myPet)
	{
		if(myPet->IsNPC())
		{
			myPet->CastToNPC()->DepopTimerStop();
		}
	}

	// Moraj: Cancel Bind Wound
	BindWound(this,true);
}


int16 Client::AutoPutItemInInventory(Item_Struct* item, sint8 charges,int tradeslot){

	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::AutoPutItemInInventory(item_name = %s)", item->name);
	if(item->common.stackable==1){ // if item can stack

		Item_Struct* itemInBag;
		sint8 chargesRemaining=0;
		sint8 totalCharges=0;
		sint16 i;


		// Pass 1: (Inventory) Attempt to fill stacks that aren't yet full
		for (i = 22; i <= 29; i++)
		{
			itemInBag = Database::Instance()->GetItem(pp.inventory[i]);
			if(itemInBag && itemInBag->item_nr == item->item_nr && pp.invItemProprieties[i].charges < 20)
			{
				chargesRemaining= charges - (20-pp.invItemProprieties[i].charges);
				if(chargesRemaining > 0){
					PutItemInInventory(i,item,20);
					pp.invItemProprieties[i].charges=20;
					return AutoPutItemInInventory(item,chargesRemaining,tradeslot);
				}
				else{
					totalCharges=pp.invItemProprieties[i].charges+charges;
					pp.invItemProprieties[i].charges=totalCharges;
					PutItemInInventory(i,item,totalCharges);
				}
				return i;
			}
		}


		// Pass 2: (Inventory Bags) Attempt to fill stacks that aren't yet full
		for (i = 22; i <= 29; i++)
		{
			Item_Struct* bag;
			bag = Database::Instance()->GetItem(pp.inventory[i]);
			if (bag != 0 && bag->type == 0x01) { // make sure it's a container
				//cout << "number of slots " << (int) bag->common.container.BagSlots << endl;
				for (uint8 j = 0; j < bag->common.container.BagSlots; j++)
				{
					int16 slotid = 250 + ((i-22)*10) + j;
					itemInBag=Database::Instance()->GetItem(pp.containerinv[slotid-250]);
					if(itemInBag && itemInBag->item_nr == item->item_nr && pp.bagItemProprieties[((i-22)*10) + j].charges < 20)
					{
						chargesRemaining= charges - (20-pp.bagItemProprieties[((i-22)*10) + j].charges);
						if(chargesRemaining > 0){
							PutItemInInventory(slotid,item,20);
							pp.bagItemProprieties[((i-22)*10) + j].charges=20;
							return AutoPutItemInInventory(item,chargesRemaining,tradeslot);
						}
						else{
							totalCharges=pp.bagItemProprieties[((i-22)*10) + j].charges+charges;
							pp.bagItemProprieties[((i-22)*10) + j].charges=totalCharges;
							PutItemInInventory(slotid,item,totalCharges);
						}
						return slotid;
					}
				}
			}
		}
	}
	//Yeahlight: First, attempt to find the best equipable slot not currently in use
	int16 tmpslot = FindBestEquipmentSlot(item);
	//Yeahlight: Failed to find an inventory slot not in use, continue on with general inventory search
	if(tmpslot > 50)
		tmpslot = FindFreeInventorySlot(0, (item->type == 0x01), false);
	if (tmpslot == 0xFFFF) {
		if(tradeslot!=-1){
			char buffer[200];
			if(item){//Tazadar: We drop the item on the ground + message ^^
				this->DropTradeItem(item,charges,tradeslot);
				sprintf(buffer, "You just dropped your %s.", item->name);
				this->Message(YELLOW,buffer);
			}
			
			return -1;
		}
		else {// We put the item  on the cursor
			SummonItem(item->item_nr , charges);
			return 0;
		}
	}
	if (!PutItemInInventory(tmpslot, item->item_nr , charges)) {
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::AutoPutItemInInventory(): Error adding this item to your inventory, putting it back on the corpse");
		Message(RED, "Error adding this item to your inventory, putting it back on the corpse.");
		return -1;
	}
	return tmpslot;
}

int16 Client::FindFreeInventorySlot(int16** pointer, bool ForBag, bool TryCursor) {
	int i;
	for (i=22; i<=29; i++) {
		if (pp.inventory[i] == 0xFFFF) {
			if (pointer != 0)
				*pointer = &pp.inventory[i];
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::FindFreeInventorySlot() returned %i", i);
			return i;
		}
	}
	if (!ForBag) {
		Item_Struct* item;
		for (i=22; i<=29; i++) {
			item = Database::Instance()->GetItem(pp.inventory[i]);
			if (item != 0 && item->type == 0x01) { // make sure it's a container
				for (int k=0; k<item->common.container.BagSlots; k++) {
					if (pp.containerinv[((i-22)*10) + k] == 0xFFFF) {
						if (pointer != 0)
							*pointer = &pp.containerinv[((i-22)*10) + k];
						int32 slot = 250 + ((i-22)*10) + k;
						CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::FindFreeInventorySlot() returned %i", slot);
						return slot;
					}
				}
			}
		}
	}
	if (TryCursor) {
		if (pp.inventory[0] == 0xFFFF) {
			*pointer = &pp.inventory[0];
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::FindFreeInventorySlot() returned 0");
			return 0;
		}
	}
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::FindFreeInventorySlot() returned 0xFFFF");
	return 0xFFFF;
}



bool Client::PutItemInInventory(int16 slotid, int16 itemid, sint8 charges) {
	Item_Struct* item = Database::Instance()->GetItem(itemid);
	if (item == 0)
		return false;
	else
		return PutItemInInventory(slotid, item, charges);
}



bool Client::PutItemInInventory(int16 slotid, Item_Struct* item) {
	return PutItemInInventory(slotid, item, item->common.charges);
}



bool Client::PutItemInInventory(int16 slotid, Item_Struct* item, sint8 charges) {
	if (!item) return false;
	if (slotid <= 29){
		pp.inventory[slotid] = item->item_nr;
		pp.invItemProprieties[slotid].charges = charges;
	}
	else if (slotid >= 250 && slotid <= 329){
		pp.containerinv[slotid-250] = item->item_nr;
		pp.bagItemProprieties[slotid-250].charges =charges;
	}
	else if(slotid >= 330 && slotid <= 339){
		pp.cursorbaginventory[slotid-330] = item->item_nr;
		pp.cursorItemProprieties[slotid-330].charges =charges;
	}
	else if (slotid >= 2000 && slotid <= 2007){
		pp.bank_inv[slotid-2000] = item->item_nr;
		pp.bankinvitemproperties[slotid-2000].charges=charges;
	}
	else if (slotid >= 2030 && slotid <= 2109){
		pp.bank_cont_inv[slotid-2030] = item->item_nr;
		pp.bankbagitemproperties[slotid-2030].charges =charges;
	}
	else
		return false;
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::PutItemInInventory(slotid = %i, item name = %s, charges = %i)", slotid, item->name, charges);

	APPLAYER* outapp = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));
	memcpy(outapp->pBuffer, item, outapp->size);
	Item_Struct* outitem = (Item_Struct*) outapp->pBuffer;
	outitem->equipSlot = slotid;
	if (item->flag != 0x7669)
		outitem->common.charges = charges;
	QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;

	//Yeahlight: Refresh bonuses
	CalcBonuses(true, true);
	return true;
}
/********************************************************************  
*                          Tazadar - 7/16/08                       *  
********************************************************************  
*                          CheckLoreConflict                       *  
********************************************************************  
*  + Check if you already have a lore item in your inventory ^^    *  
*											                        *  
********************************************************************/  
bool Client::CheckLoreConflict(int16 itemid)
{
	Item_Struct* item = 0;
	item=Database::Instance()->GetItem(itemid);

	if(!item) //We dont have the item ?
	{
		//Message(RED, "Error : Checking the lore conflict value of an item that does not exist.");
		return false;
	}
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::CheckLoreConflict(item name = %s)", item->name);

	if(item->lore[0]!='*') //Its not a lore item !
	{
		return false;
	}

	// Check against Items in Inventory
	for(int i=0;i<30;i++)
	{
		if(pp.inventory[i]==itemid)
		{
			Message(RED, "Duplicate Lore Items are not allowed");
			return true;
		}
	}

	// Check against Items in Containers in Inventory
	for(int i=0;i<80;i++)
	{
		if(pp.containerinv[i]==itemid){
			Message(RED, "Duplicate Lore Items are not allowed");
			return true;
		}
	}

	for(int i=0;i<10;i++)
	{
		if(pp.cursorbaginventory[i]==itemid)
		{
			Message(RED, "Duplicate Lore Items are not allowed");
			return true;
		}
	}

	// Check against Items in Bank Inventory
	for(int i=0;i<8;i++)
	{
		if(pp.bank_inv[i]==itemid)
		{
			Message(RED, "Duplicate Lore Items are not allowed");
			return true;
		}
	}

	// Check against Items in Bank Container Inventory
	for(int i=0;i<80;i++)
	{
		if(pp.bank_cont_inv[i]==itemid)
		{
			Message(RED, "Duplicate Lore Items are not allowed");
			return true;
		}
	}

	return false;
}

/********************************************************************  
*                          Harakiri - 9/16/09                      *  
********************************************************************  
*                          FindItemTypeInInventory                 *  
********************************************************************  
*  Return the slot of an item (0 if not found), see itemtype.h	   *  
*  for types								                       *  
********************************************************************/  
int16 Client::FindItemTypeInInventory(int16 itemtype){

	Item_Struct* item = 0;

	for(int i=22;i<30;i++){
		int16 itemID = pp.inventory[i];
		
		if(itemID!=0 && itemID!=0xFFFF) {
			item = Database::Instance()->GetItem(itemID);

			if(item!=0 && item->common.itemType == itemtype) {
				return i;
			}
		}
		
	}

	for(int i=0;i<80;i++){	
		int16 itemID = pp.containerinv[i];

		if(itemID!=0 && itemID!=0xFFFF) {
			item = Database::Instance()->GetItem(itemID);

			if(item!=0 && item->common.itemType == itemtype) {
				return i+250;
			}
		}
	}
	return 0;
}

/********************************************************************  
*                          Tazadar - 8/16/08                       *  
********************************************************************  
*                          FindItemInInventory                     *  
********************************************************************  
*  + Return the slot of an item ^^	(0 if not found)			    *  
*											                        *  
********************************************************************/  
int16 Client::FindItemInInventory(int16 itemid){
	for(int i=22;i<30;i++){
		if(pp.inventory[i]==itemid){
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::FindItemInInventory(item id = %i) returned %i", itemid, i);
			return i;
		}
	}
	for(int i=0;i<80;i++){
		if(pp.containerinv[i]==itemid){
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::FindItemInInventory(item id = %i) returned %i", itemid, 250 + i);
			return i+250;
		}
	}
	return 0;
}
void Client::ChangeSurname(char* in_Surname)
{
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ChangeSurname(surname = %s)", in_Surname);
	memset(pp.Surname, 0, sizeof(pp.Surname));

	if (strlen(in_Surname) >= sizeof(pp.Surname))
	{
		strncpy(pp.Surname, in_Surname, sizeof(pp.Surname) - 1);
	}
	else
	{
		strcpy(pp.Surname, in_Surname);
	}

	strcpy(Surname, pp.Surname);
	// Send name update packet here... once know what it is
}



void Client::SetGM(bool toggle)
{	
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SetGM(toggle = %i)", toggle);
	this->Save();

	/*
	7th of 10th 2007 - froglok23
	Made the common code in the if block more common.
	moved our the Saves(), entity_list and delete outapp
	out of the block as less code is better :D
	*/

	int32 parameter = 0;

	if(toggle)
	{
		pp.gm=1;
		parameter=1;
		Message(RED, "You are now a GM.");
	}
	else
	{	
		pp.gm=0;
		parameter=0;		
		Message(RED, "You are no longer a GM.");
	}

	this->SendAppearancePacket(this->GetID(), SAT_GM_Tag, parameter, true);

	this->Save();
}

/* Sends the requested text of a book to the cliend to display 
* I've only tested this with the tome of discord which is 8 bytes 
* but I'm pretty sure it can go up to 14 bytes, but haven't tested.*/
void Client::ReadBook(char txtfile[8]) {
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ReadBook(txtfile = %s)", txtfile);
	char booktxt[6000];						//Buffer to store book txt, 6000 is magic;
	Database::Instance()->GetBook(txtfile, booktxt);		//Retrieves txt from the database for key txtfile;

	if (booktxt != 0) {
		//cout << "Just Sent Book for: " << txtfile << " Text: " << booktxt << endl;
		APPLAYER* outapp = new APPLAYER(OP_ReadBook, strlen(booktxt));
		memcpy(outapp->pBuffer,booktxt,strlen(booktxt));
		QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;
	}
	else
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ReadBook(): Error: book not found");
}

bool Client::TakeMoneyFromPP(uint32 copper)
{
	sint32 copperpp,silver,gold,platinum;
	copperpp = pp.copper;
	silver = pp.silver*10;
	gold = pp.gold*100;
	platinum = pp.platinum*1000;

	sint32 clienttotal = pp.copper+pp.silver*10+pp.gold*100+pp.platinum*1000;
	clienttotal -= copper;
	if(clienttotal < 0)
		return false; // Not enough money!

	pp.platinum = clienttotal / 1000;
	clienttotal -= pp.platinum * 1000;
	pp.gold = clienttotal / 100;
	clienttotal -= pp.gold * 100;
	pp.silver = clienttotal / 10;
	clienttotal -= pp.silver * 10;
	pp.copper = clienttotal;
	Save();
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::TakeMoneyFromPP(): client should have: plat: %i, gold: %i, silver: %i, copper: %i\n", pp.platinum, pp.gold, pp.silver, pp.copper);
	return true;
}

/********************************************************************
 *                      Harakiri - 8/31/2009                        *
 ********************************************************************
 *		       SendClientQuestComplete							    *
 ********************************************************************   
   Plays the Quest Complete Fanfare sound
 ********************************************************************/
void	Client::SendClientQuestCompletedFanfare() {
	SendClientQuestCompletedMoney(0,0,0,0,0);
}
/********************************************************************
 *                      Harakiri - 8/31/2009                        *
 ********************************************************************
 *		       SendClientQuestCompletedMoney					    *
 ********************************************************************
   Adds the Amount of Money to the Clients PP
   Plays the Quest Complete Fanfare sound
   Prints e.g
   You receive 3 silver from Guard Elron.
   You receive 3 platinum from Guard Elron.
 ********************************************************************/
void	Client::SendClientQuestCompletedMoney(Mob* sender, uint32 copper, uint32 silver, uint32 gold,uint32 platinum) {	

	APPLAYER* outapp = new APPLAYER(OP_QuestCompletedMoney,sizeof(QuestCompletedMoney_Struct));
	QuestCompletedMoney_Struct* questMoney= (QuestCompletedMoney_Struct*)outapp->pBuffer;
	if(sender !=0) {
		questMoney->npcID=sender->GetID();
	} else {
		questMoney->npcID=0;
	}

	questMoney->copper = copper;
	questMoney->silver = silver;
	questMoney->gold = gold;
	questMoney->platinum = platinum;	
	QueuePacket(outapp);
	safe_delete(outapp);
}


void Client::AddMoneyToPP(uint32 copper,bool updateclient){
	uint32 tmp;
	uint32 tmp2;
	tmp = copper;

	// Add Amount of Platinum
	tmp2 = tmp/1000;
	pp.platinum = pp.platinum + tmp2;
	tmp-=tmp2*1000; 

	if (updateclient)
		SendClientMoneyUpdate(3,tmp2);

	// Add Amount of Gold
	tmp2 = tmp/100;
	pp.gold = pp.gold + tmp2;
	tmp-=tmp2*100; 
	if (updateclient)
		SendClientMoneyUpdate(2,tmp2);

	// Add Amount of Silver
	tmp2 = tmp/10;
	tmp-=tmp2*10; 
	pp.silver = pp.silver + tmp2;
	if (updateclient)
		SendClientMoneyUpdate(1,tmp2);

	// Add Copper
	//tmp	= tmp - (tmp2* 10);
	if (updateclient)
		SendClientMoneyUpdate(0,tmp);
	pp.copper = pp.copper + tmp;
	Save();
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::AddMoneyToPP(): client should have: plat: %i, gold: %i, silver: %i, copper: %i\n", pp.platinum, pp.gold, pp.silver, pp.copper);
}



void Client::AddMoneyToPP(uint32 copper, uint32 silver, uint32 gold, uint32 platinum, bool updateclient)
{
	if ((updateclient) && platinum!=0)
		SendClientMoneyUpdate(3,platinum);
	if ((updateclient) && gold!=0)
		SendClientMoneyUpdate(2,gold);
	if ((updateclient) && silver!=0)
		SendClientMoneyUpdate(1,silver);
	if ((updateclient) && copper!=0)
		SendClientMoneyUpdate(0,copper);
	pp.platinum = pp.platinum + platinum;
	pp.gold = pp.gold + gold;
	pp.silver = pp.silver + silver;
	pp.copper = pp.copper + copper;
	Save();	
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::AddMoneyToPP(): client should have: plat: %i, gold: %i, silver: %i, copper: %i\n", pp.platinum, pp.gold, pp.silver, pp.copper);

}

/********************************************************************
*                        Tazadar - 8/22/08	                        *
********************************************************************
*                        SendStationItem			                *
********************************************************************
*  + Display items that are currently in the station !			    *
********************************************************************/

void Client::SendStationItem(int32 oldPlayerID){

	bool sameplayer;

	if(this->GetID()==oldPlayerID)
		sameplayer=true;
	else
		sameplayer=false;

	int i;
	Item_Struct* item = 0;
	Item_Struct* outitem = 0;
	for (i = 0; i < 10; i++) 
	{	
		Item_Struct* item = 0;
		Item_Struct* outitem = 0;
		if(this->stationItems[i]!=0){
			item = Database::Instance()->GetItem(this->stationItems[i]);

			if (item && (item->nodrop||sameplayer)) 
			{
				if(item->common.stackable==1){//Tazadar load the number in each stack for inventory (not container)
					item->common.charges=this->stationItemProperties[i].charges;
				}
				//cout<<"trying to send the items"<<endl;

				APPLAYER* app = new APPLAYER(OP_StationItem, sizeof(Item_Struct));
				memcpy(app->pBuffer, item, sizeof(Item_Struct));
				outitem = (Item_Struct*) app->pBuffer;
				outitem->equipSlot = i;
				QueuePacket(app);
				safe_delete(app);//delete app;

			}
			else{
				this->stationItems[i]=0;
				this->stationItemProperties[i].charges=0;
			}
		}
	}

}


//======================== Group Processes ====================================================//
/* Right now this just relays the group invites and the auto declines will come 
* if someone who is not eligable for a group invite is invited.  Later this can be
* improved to create it's own decline packets so that the ineligable player never
* receives a 'blah invites you to a group' message since they can't join anyways
* and it's a wasted packet sent.*/
void Client::ProcessOP_GroupInvite(APPLAYER* pApp)
{
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ProcessOP_GroupInvite()");
	if (pApp->size == sizeof(GroupInvite_Struct))
	{
		if(this->GetTarget() && this->GetTarget()->IsClient())
		{

			/*int namesize = 30;						//Magic number for now.

			APPLAYER* outapp = new APPLAYER(OP_GroupDeclineInvite, sizeof(GroupInviteDecline_Struct));
			GroupInviteDecline_Struct* gid = (GroupInviteDecline_Struct*)pApp->pBuffer;

			memcpy(gid->leader, this->CastToClient()->GetName(), namesize);
			memcpy(gid->leaver, ((GroupInvite_Struct*)pApp->pBuffer)->invited , namesize);

			//Don't send invite to target, but still send decline message to inviter;
			if(this->GetTarget()->CastToClient()->GroupInvitePending())
			{		
			gid->action = (int8)1;								//Player considering another group invite;
			this->CastToClient()->QueuePacket(outapp);			//Send update to player who invited;

			}else if(this->GetTarget()->CastToClient()->IsGrouped()){
			gid->action = (int8)2;								//Player already grouped;
			this->CastToClient()->QueuePacket(outapp);			//Send update to player who invited;

			}else{*/
			this->GetTarget()->CastToClient()->QueuePacket(pApp);
			this->GetTarget()->CastToClient()->SetGroupInvitePending(true);
			//}

			//safe_delete(outapp);//delete outapp;
		}
	}
	else
	{
		EQC::Common::PrintF(CP_CLIENT, "Wrong size: OP_GroupInvite, size=%i, expected = %i\n", pApp->size, sizeof(GroupInvite_Struct));
	}
}

/* Relays group follow to leader and then sends out a group 
* update packet to all group members.							*/
void Client::ProcessOP_GroupFollow(APPLAYER* pApp){
	//cout << "Group Follow Entry" << endl;
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ProcessOP_GroupFollow()");
	if (pApp->size == sizeof(GroupFollow_Struct)){
		GroupFollow_Struct* gf = (GroupFollow_Struct*) pApp->pBuffer;
		//cout << "Checking Leader" << endl;
		Client* leader = entity_list.GetClientByName(gf->leader);
		//cout << "Checking Leader is grouped, if not, make a group" << endl;
		if (!leader->IsGrouped()){
			cout << "Leader not grouped, making one" << endl;
			Group* ng = new Group(leader);
			entity_list.AddGroup(ng);
			Database::Instance()->SetGroupID(leader->GetName(), ng->GetGroupID(), 1);
		}
		//cout << "Populating myGroup" << endl;
		Group* myGroup = entity_list.GetGroupByClient(leader);

		if(myGroup != 0){
			//cout << "Group DEBUG: Adding group member (" << this->GetName() << ") to " << leader->GetName() << "'s group (ID: " << myGroup->GetID() << ")." << endl;			
			myGroup->AddMember(this);
			Database::Instance()->SetGroupID(this->GetName(), myGroup->GetGroupID());
		}else{
			cout << "No group found." << endl;
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ProcessOP_GroupFollow(): Error, group not found");
		}
		leader->QueuePacket(pApp);
	}
	else{
		EQC::Common::PrintF(CP_CLIENT, "Wrong size: OP_GroupFollow, size=%i, expected=%i\n", pApp->size, sizeof(GroupFollow_Struct));
	}
}

/* Triggers when a member gets kicked or clicks disband (not when leader disbands)
* We still need to figure out how to distinguish between kick and drop.*/
void Client::ProcessOP_GroupQuit(APPLAYER* pApp){
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ProcessOP_GroupQuit()");
	GroupDisband_Struct* gds = (GroupDisband_Struct*)pApp->pBuffer;
	if ( pApp->size == sizeof(GroupDisband_Struct))
	{
		gds->member[sizeof(gds->member)-1] = '\0'; //Cofruben: security check.
		//DumpPacketHex(pApp);
		if (strlen(gds->member) < 4) return;

		int32 KickedGID = Database::Instance()->GetGroupID(gds->member);
		int32 KickerGID = Database::Instance()->GetGroupID(this->GetName());
		if (KickedGID == 0 || KickerGID == 0){
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ProcessOP_GroupQuit(): Error removing member from group: not grouped!");
			return;
		}
		if (KickerGID != KickedGID) {
			cout << this->GetName() << " is trying to kick a person who is not in his group, hacker?" << endl;
			return;
		}
		//cout << "Attempting to remove member: " <<  gds->member << " from group." << endl;
		Group* g = entity_list.GetGroupByID(KickedGID);

		if (!g)
		{
			cout << "Group ID not found, groupID is " << KickedGID << "." << endl;
			return;
		}

		if (strcmp(this->GetName(), gds->member) != 0 && g->GetLeader() != this) {
			cout << this->GetName() << " is trying to kick a person without being the leader, hacker?" << endl;
			return;
		}
		if(g)
			g->DelMember(gds->member, false);
	}
}


void Client::ProcessOP_GroupDeclineInvite(APPLAYER* pApp){
	//DumpPacketHex(pApp);
	if(pApp->size == sizeof(GroupInviteDecline_Struct)) {
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ProcessOP_GroupDeclineInvite()");
		GroupInviteDecline_Struct* gid = (GroupInviteDecline_Struct*)pApp->pBuffer;
		if ( entity_list.GetClientByName(gid->leader) != 0 )
			entity_list.GetClientByName(gid->leader)->SetGroupInvitePending(false);
		if ( entity_list.GetClientByName(gid->leaver) != 0 )
			entity_list.GetClientByName(gid->leaver)->SetGroupInvitePending(false);
		if ( entity_list.GetClientByName(gid->leader) != 0 )
			entity_list.GetClientByName(gid->leader)->QueuePacket(pApp);
	}else{
		cout << "GroupDeclineInvite packet is wrong size. Expected: " << sizeof(GroupInviteDecline_Struct)
			<< " but received: " << pApp->size << " ." << endl;
	}
	Database::Instance()->SetGroupID(GetName(), 0);
}

void Client::ProcessChangeLeader(char* member) {
	int32 gid = Database::Instance()->GetGroupID(this->GetName());
	Group* g = entity_list.GetGroupByID(gid);
	
	member[0] = toupper(member[0]);
	for (int i = 1; i < strlen(member); i++ )
	{
		if ( member[i] != '\0' )
			member[i] = tolower(member[i]);
	}

	if (!g)	{
		cout << "Group not found when trying to change leader." << endl;
		Message(BLACK, "Error: #makeleader: You aren't in a group.");
		return;
	}
	if (!g->IsLeader(this->GetName())) {
		Message(BLACK, "Error: #makeleader: You aren't the leader of the group.");
		return;
	}
	if (stricmp(member, this->GetName()) == 0)
	{
		Message(BLACK, "Error: #makeleader: You are already the leader.");
		return;
	}
	if (!g->IsGroupMember(member))
	{
		Message(BLACK, "Error: #makeleader: %s is not in your group.", member);
		return;
	}
	g->ChangeLeader(member);
}

void Client::RefreshGroup(int32 gid, int8 action) { //Kibanu
	Group* g = entity_list.GetGroupByID(gid);
	
	if (!g)
		return; // Not in a group

	if ( action == GROUP_QUIT )
		entity_list.RemoveEntity(g->GetID());
	else
		g->LoadFromDB(gid);
}

//======================= End Group Processes =================================================//


	// Harakiri Place items into inventory of NPC specified
void Client::FinishTrade(Mob* with){
	EQC::Common::Log(EQCLog::Debug,CP_ITEMS,"Client::FinishTrade(NPC name = %s)", with->GetName());

	bool did_quest = false;
	#ifdef EMBPERL
		if(perlParser->HasQuestSub(with->GetNPCTypeID(), "EVENT_ITEM")) {

			char *data = 0;
			char *data2 = 0;
			// Harakiri Handle Trade Items
			for ( int z=0; z < 4; z++ ) {
				Item_Struct* item = 0;
				if(this->TradeList[z]!=0)
					item=Database::Instance()->GetItem(this->TradeList[z]);
				
				// sanity check
				if(item){							
					MakeAnyLenString(&data,"item%d.%d", z+1,with->GetNPCTypeID());					
					MakeAnyLenString(&data2,"%d",this->TradeList[z]);					
					perlParser->AddVar(data,data2);		
									
					MakeAnyLenString(&data,"item%d.charges.%d", z+1,with->GetNPCTypeID());					
					MakeAnyLenString(&data2,"%d",this->TradeCharges[z]);		
					perlParser->AddVar(data,data2);	
				}				
			}		


			// Handle money			
			MakeAnyLenString(&data,"copper.%d",with->GetNPCTypeID());		
			MakeAnyLenString(&data2,"%i",this->npctradecp);					
			perlParser->AddVar(data,data2);		
				
			MakeAnyLenString(&data,"silver.%d",with->GetNPCTypeID());		
			MakeAnyLenString(&data2,"%i",this->npctradesp);					
			perlParser->AddVar(data,data2);		

			MakeAnyLenString(&data,"gold.%d",with->GetNPCTypeID());		
			MakeAnyLenString(&data2,"%i",this->npctradegp);					
			perlParser->AddVar(data,data2);		

			MakeAnyLenString(&data,"platinum.%d",with->GetNPCTypeID());		
			MakeAnyLenString(&data2,"%i",this->npctradepp);					
			perlParser->AddVar(data,data2);		
			
			perlParser->Event(EVENT_ITEM, with->GetNPCTypeID(), NULL, with->CastToNPC(), this);
			did_quest = true;
		}
	#endif

	for(int i=0;i<4;i++){
		Item_Struct* item = 0;
		if(this->TradeList[i]!=0)
			item=Database::Instance()->GetItem(this->TradeList[i]);
		if(item){		
			if(item->nodrop && item->norent && item->type!=1){
				with->CastToNPC()->AddItem(this->TradeList[i],this->TradeCharges[i],255);
			}
		}
		with->CastToNPC()->AddCash(this->npctradecp,this->npctradesp,this->npctradegp,this->npctradepp);
	}
}

void Client::FinishTrade(Client* with) {
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_ITEMS, "Client::FinishTrade(client name = %s)", with->GetName());
	Item_Struct* item = 0;
	Item_Struct* outitem = 0;
	int16* toppslot = 0;
	int16 toslot = 0;

	for (int i = 0;i < 8; i++){
		if (with->TradeList[i] != 0){
			item = 0;
			item = Database::Instance()->GetItem(with->TradeList[i]);					
			if(item != 0){
				if (item->type != 0x01){//if its not a bag
					this->AutoPutItemInInventory(item, with->TradeCharges[i],i);
				}
				else{
					toslot = FindFreeInventorySlot(&toppslot,(item->type == 1),true);
					if (toslot != 0xFFFF){
						this->PutItemInInventory(toslot,item,with->TradeCharges[i]);

						for (int j=0;j != 10; j++){
							if(with->TradeList[((i+1)*10)+j]!=0){
								this->PutItemInInventory(((toslot-22)*10) + j + 250,with->TradeList[((i+1)*10)+j],with->TradeCharges[((i+1)*10)+j]);
							}
						}
					}
					else{
						this->AutoPutItemInInventory(item, with->TradeCharges[i],i); //if its full we put it on the ground
					}
				}
			}
		}
	}
}

void Client::SetFeigned(bool in_feigned) {
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SetFeigned(%i)", in_feigned);
	if(in_feigned)
	{
		this->feigned = in_feigned;
		entity_list.ClearFeignAggro(this);
		if(GetDebugMe())
			this->Message(LIGHTEN_BLUE, "Debug: You are now feigning your own death");
	}
	else
	{
		if(this->feigned && GetDebugMe())
			this->Message(LIGHTEN_BLUE, "Debug: You are no longer feigning your own death");
		this->feigned = in_feigned;
	}
}

// LaZZer - CheckAddSkill, needs a few pieces. I'll get to those next. The TODOs are on my plate.
bool Client::CheckAddSkill(int skill_num, int16 skill_mod)
{
	/* TODO: If the player is charmed, don't even bother with the check.
	if(IsCharmed())
	return false;
	*/
	//Make sure the skill is in bounds.
	if(skill_num > TAUNT || skill_num < 0)
		return false;
	//Yeahlight: Current skill is lower than the maximum for this character combination
	if(GetSkill(skill_num) < CheckMaxSkill(skill_num, pp.race, pp.class_, pp.level)) 
	{
		sint16 nChance = 10 + skill_mod + ((252 - GetSkill(skill_num)) / 20);	//Grabbed this formula from eqemu 5.0
		//Check for a negative chance, change it to one so we still have a shot at a skill-up.
		if(nChance < 1)
			nChance = 1;
		if(MakeRandomInt(0, 99) < nChance)	
		{	//Award the player a skill point.
			SetSkill(skill_num,GetSkill(skill_num)+1);
			CalcBonuses();
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CheckAddSkill(skill = %i, mod = %i)", skill_num, skill_mod);
			return true;
		}
	}
	return false;
}

void Client::SendClientMoneyUpdate(int8 type,int32 amount){
	//cout<<"Sending Client Money Update"<<endl;
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::SendClientMoneyUpdate(type = %i, amount = %i)", (int)type, amount);
	APPLAYER* outapp = new APPLAYER(OP_TradeMoneyUpdate,sizeof(TradeMoneyUpdate_Struct));
	TradeMoneyUpdate_Struct* mus= (TradeMoneyUpdate_Struct*)outapp->pBuffer;
	mus->amount=amount;
	mus->trader=0;
	mus->type=type;
	QueuePacket(outapp);
	safe_delete(outapp);
}

/****************************************************************
*			This drops items on ground when you finish			* 
*				a trade and got no room on inv 					*
*																*
****************************************************************/
void Client::DropTradeItem(Item_Struct* item, sint8 charges, int8 slot)
{
	if(!item)
		return;
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::DropTradeItem(item name = %s, charges = %i, slot = %i)", item->name, (int)charges, (int)slot);
	Object* no = new Object(item, GetX(), GetY(), GetZ(), this, slot, OT_DROPPEDITEM, true);
	zone->object_list.push_back(no);
	entity_list.AddObject(no,true);
}


void Client::LoadSpells()
{
	if (!spells_handler.SpellsLoaded()) return;
	for (int i=0; i<15; i++)
	{
		for(int z=0;z<15;z++)
		{
			if(this->buffs[z].spell && this->buffs[z].spell->IsValidSpell()
				&& (this->buffs[z].spell->GetSpellID() == this->pp.buffs[i].spellid))
			{
				this->buffs[z].spell = NULL;
			}
		}
		Spell* pp_buff = spells_handler.GetSpellPtr(this->GetPlayerProfilePtr()->buffs[i].spellid);
		if (pp_buff && pp_buff->IsValidSpell() && this->pp.buffs[i].duration > 0)
		{
			//PP buff correct, so copy it into our buffs list.
			this->buffs[i].spell = pp_buff;
			this->buffs[i].ticsremaining = this->pp.buffs[i].duration+1;//Tazadar: we add 1 cause it decreases it by one when loging the char
			this->buffs[i].casterlevel = this->pp.buffs[i].level;
			this->buffs[i].casterid = this->GetID();
			this->buffs[i].durationformula = pp_buff->GetSpellBuffDurationFormula();
			this->buffs[i].diseasecounters = pp_buff->GetDiseaseCounters();
			this->buffs[i].poisoncounters = pp_buff->GetPoisonCounters();
			this->buffs[i].damageRune = pp_buff->GetRuneAmount();
			this->pp.buffs[i].visable = 2;
		}
		else
		{
			//That PP buff is incorrect, so "delete" it.
			this->buffs[i].spell = NULL;
			this->pp.buffs[i].visable = 0;
			this->pp.buffs[i].level = 0;			
			this->pp.buffs[i].bard_modifier = 0;
			this->pp.buffs[i].b_unknown3 = 0;
			this->pp.buffs[i].spellid = 0;
			this->pp.buffs[i].duration = 0;
		}
	}
}


APPLAYER* Client::CreateTimeOfDayPacket(int8 Hour, int8 Minute, int8 Day, int8 Month, long Year)
{
	APPLAYER* outapp = new APPLAYER(OP_TimeOfDay, sizeof(TimeOfDay_Struct));
	TimeOfDay_Struct* tod = (TimeOfDay_Struct*)outapp->pBuffer;

	//TODO: Make this set itself to worldtime.
	tod->hour = Hour;
	tod->minute = Minute;
	tod->day = Day;
	tod->month = Month;
	tod->year = Year;

	return outapp;
}

void Client::Handle_Connect5GDoors()
{
	EQC::Common::PrintF(CP_ZONESERVER, "Sending doors one at a time.\n");
	LinkedListIterator<Door_Struct*> iterator(zone->door_list);

	iterator.Reset();

	while(iterator.MoreElements())	
	{
		Door_Struct* door = iterator.GetData();

		//Yeahlight: TODO: Remove the excess, serverside data from the door struct
		APPLAYER* outapp = new APPLAYER(OP_SpawnDoor, sizeof(Door_Struct) - 40);
		memcpy(outapp->pBuffer, door, sizeof(Door_Struct) - 40);

		QueuePacket(outapp);

		safe_delete(outapp);//delete outapp;

		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

void Client::Handle_Connect5Objects()
{
	EQC::Common::PrintF(CP_ZONESERVER, "Sending objects one at a time.\n");

	for(int i = 0; i < (int)zone->object_list.size(); i++)
	{
		APPLAYER app;
		zone->object_list.at(i)->CreateSpawnPacket(&app);
		QueuePacket(&app);
	}
}

void Client::Handle_Connect5Guild()
{
	// Guild ID is in sze now, rank still using this tho
	if (guilddbid != 0)
	{
		int32 parameter = 0;

		zgm.LoadGuilds();
		Guild_Struct* guilds = zgm.GetGuildList();

		if (guilds[guildeqid].rank[guildrank].warpeace || guilds[guildeqid].leader == account_id)
		{
			parameter = 2;
		}
		else if (guilds[guildeqid].rank[guildrank].invite || guilds[guildeqid].rank[guildrank].remove || guilds[guildeqid].rank[guildrank].motd)
		{
			parameter = 1;
		}
		else
		{
			parameter = 0;
		}

		this->SendAppearancePacket(this->GetID(), SAT_Guild_Rank, parameter, false);
	}
}

void Client::DeletePetitionsFromClient()
{
	// Deleting all petitions off client
	APPLAYER* outapp = new APPLAYER(OP_PetitionClientUpdate,sizeof(PetitionClientUpdate_Struct));
	PetitionClientUpdate_Struct* pet = (PetitionClientUpdate_Struct*) outapp->pBuffer;
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

	for (int x = 0; x < 16; x++)
	{
		pet->petnumber = x;
		QueuePacket(outapp);
	}

	safe_delete(outapp);//delete outapp;
	petition_list.UpdateGMQueue();
}

//************** GetWornInstrumentType *************//
//--------------------------------------------------//
// Return the instrument type ID currently worn,    //
// 0xFF if none. (0xaa if it's the bard epic) 		//
//--------------------------------------------------//
//                 - Pinedepain -                   //
//--------------------------------------------------//

uint8 Client::GetWornInstrumentType(void)
{
	// Pinedepain // We check if we have an instrument in the second hand first.
	Item_Struct* item = Database::Instance()->GetItem(pp.inventory[14]);

	// Pinedepain // If we have something
	if (item)
	{
		// Pinedepain // We check if it's the epic. If it is, we return (0xaa)
		if (pp.inventory[14] == 0x503e)
		{
			return (0xaa);
		}

		// Pinedepain // We return the instrument skill ID
		else
		{
			return (item->common.itemType);
		}
	}

	// Pinedepain we haven't got anything in the off hand, we return 0xFF
	return (0xFF);
}

/********************************************************************
*                      Pinedepain  - 6/20/08                       *
********************************************************************
*                      GetInstrumentModByType                      *                        
********************************************************************
*  Return the instrument modifier for a specified instrument type  *
********************************************************************/

uint8 Client::GetInstrumentModByType(uint8 instruType)
{
	// Pinedepain // We return the right instrument mod
	switch (instruType)
	{
	case SS_WIND:
		{
			return (windInstrumentMod);
		}

	case SS_STRING:
		{
			return (stringedInstrumentMod);
		}

	case SS_BRASS:
		{
			return (brassInstrumentMod);
		}

	case SS_PERCUSSION:
		{
			return (percussionInstrumentMod);
		}

	case SS_SINGING:
		{
			return (singingInstrumentMod);
		}

	default:
		{
			cout << "Client::InstrumentModByType >> Tried to get an unknown instrument type (" << (int)instruType << ") !" << endl;
			return (0);
		}
	}

}

/********************************************************************
*                      Pinedepain  - 6/20/08                       *
********************************************************************
*                      SetInstrumentModByType                      *                        
********************************************************************
*  Set the instrument modifier for a specified instrument type     *
********************************************************************/

void Client::SetInstrumentModByType(uint8 instruType,uint8 mod)
{
	// Pinedepain // We set the right instrument mod
	switch (instruType)
	{
	case IS_WIND:
		{
			windInstrumentMod = mod;
			break;
		}

	case IS_STRING:
		{
			stringedInstrumentMod = mod;
			break;
		}

	case IS_BRASS:
		{
			brassInstrumentMod = mod;
			break;
		}

	case IS_PERCUSSION:
		{
			percussionInstrumentMod = mod;
			break;
		}

	case IS_SINGING:
		{
			singingInstrumentMod = mod;
			break;
		}

	case IS_ALLINSTRUMENT:
		{
			singingInstrumentMod = mod;
			percussionInstrumentMod = mod;
			stringedInstrumentMod = mod;
			windInstrumentMod = mod;
			brassInstrumentMod = mod;
			break;
		}

	default:
		{
			cout << "Tried to set an unknown instrument type (" << (int)instruType << ") !" << endl;
			break;
		}
	}

}

/********************************************************************
*                      Pinedepain  - 6/20/08                       *
********************************************************************
*                      ResetAllInstrumentMod                       *                        
********************************************************************
* Reset all the instrument modifiers. Set everything to 0   		*
********************************************************************/

void Client::ResetAllInstrumentMod(void)
{
	singingInstrumentMod = 0;
	percussionInstrumentMod = 0;
	stringedInstrumentMod = 0;
	windInstrumentMod = 0;
	brassInstrumentMod = 0;
}

/********************************************************************
*                      Pinedepain  - 6/20/08                       *
********************************************************************
*                        UpdateInstrumentMod                       *                        
********************************************************************
*  Update the client's instrument modifiers according to weapon1	*
*  and weapon2, which are respectively the primary-hand and		*
*  secondary-hand Item_Struct*.									*
********************************************************************/

void Client::UpdateInstrumentMod(void)
{

	// Pinedepain // Table order :
	/*
	instrumentModWeapon[0] = singing
	instrumentModWeapon[1] = percussion
	instrumentModWeapon[2] = stringed
	instrumentModWeapon[3] = wind
	instrumentModWeapon[4] = brass
	*/

	// Pinedepain // We need first to create and initialize the two weapon instrument mod table.
	uint8 instrumentModWeapon1[5];
	uint8 instrumentModWeapon2[5]; 

	for (int i = 0; i < 5; i++)
	{
		instrumentModWeapon1[i] = 0;
		instrumentModWeapon2[i] = 0;
	}


	/***************************** WEAPON1 *****************************/
	/*******************************************************************/

	// Pinedepain // If we have a weapon1 in the hand we check for its bardType (bard instrument type modifier)
	if (weapon1)
	{
		// Pinedepain // Checking bardType
		uint8 bardTypeWeapon1 = Database::Instance()->GetBardType(weapon1->item_nr);

		// If there's an instrument mod on this weapon
		if (bardTypeWeapon1)
		{
			// Pinedepain // We get its value
			uint8 bardValueWeapon1 = Database::Instance()->GetBardValue(weapon1->item_nr);

			// Pinedepain // According to its value, we update the weapon1 table
			switch (bardTypeWeapon1)
			{
			case IS_SINGING:
				{
					instrumentModWeapon1[0] = bardValueWeapon1;
					break;
				}

			case IS_PERCUSSION:
				{
					instrumentModWeapon1[1] = bardValueWeapon1;
					break;
				}

			case IS_STRING:
				{
					instrumentModWeapon1[2] = bardValueWeapon1;
					break;
				}

			case IS_WIND:
				{
					instrumentModWeapon1[3] = bardValueWeapon1;
					break;
				}

			case IS_BRASS:
				{
					instrumentModWeapon1[4] = bardValueWeapon1;
					break;
				}

			case IS_ALLINSTRUMENT:
				{
					for (int i = 0; i < 5; i++)
					{
						instrumentModWeapon1[i] = bardValueWeapon1;
					}
					break;
				}



			default:
				{
					cout << "Error, got a wrong bardType --> " << (int)bardTypeWeapon1 << endl;
					break;
				}
			}
		}
	}	

	/***************************** WEAPON2 *****************************/
	/*******************************************************************/


	// Pinedepain // If we have a weapon2 in the hand we check for its bardType (bard instrument type modifier)
	if (weapon2)
	{
		// Pinedepain // Checking bardType
		uint8 bardTypeWeapon2 = Database::Instance()->GetBardType(weapon2->item_nr);

		// If there's an instrument mod on this weapon
		if (bardTypeWeapon2)
		{
			// Pinedepain // We get its value
			uint8 bardValueWeapon2 = Database::Instance()->GetBardValue(weapon2->item_nr);

			// Pinedepain // According to its value, we update the weapon1 table
			switch (bardTypeWeapon2)
			{
			case IS_SINGING:
				{
					instrumentModWeapon2[0] = bardValueWeapon2;
					break;
				}

			case IS_PERCUSSION:
				{
					instrumentModWeapon2[1] = bardValueWeapon2;
					break;
				}

			case IS_STRING:
				{
					instrumentModWeapon2[2] = bardValueWeapon2;
					break;
				}

			case IS_WIND:
				{
					instrumentModWeapon2[3] = bardValueWeapon2;
					break;
				}

			case IS_BRASS:
				{
					instrumentModWeapon2[4] = bardValueWeapon2;
					break;
				}

			case IS_ALLINSTRUMENT:
				{
					for (int i = 0; i < 5; i++)
					{
						instrumentModWeapon2[i] = bardValueWeapon2;
					}
					break;
				}



			default:
				{
					cout << "Error, got a wrong bardType --> " << (int)bardTypeWeapon2 << endl;
					break;
				}
			}
		}
	}

	// Pinedepain // Now we set the correct client instrument mods. It's the max between the weapon1 mod and weapon2 mod
	singingInstrumentMod =		max(instrumentModWeapon1[0],instrumentModWeapon2[0]);
	percussionInstrumentMod =	max(instrumentModWeapon1[1],instrumentModWeapon2[1]);
	stringedInstrumentMod =		max(instrumentModWeapon1[2],instrumentModWeapon2[2]);
	windInstrumentMod =			max(instrumentModWeapon1[3],instrumentModWeapon2[3]);
	brassInstrumentMod =		max(instrumentModWeapon1[4],instrumentModWeapon2[4]);


	// Pinedepain // Debug
	/*cout << "-----------------------" << endl;
	cout << "singing =	" << (int)singingInstrumentMod << endl;
	cout << "percussion = " << (int)percussionInstrumentMod << endl;
	cout << "stringed = " << (int)stringedInstrumentMod << endl;
	cout << "wind = " << (int)windInstrumentMod << endl;
	cout << "brass = " << (int)brassInstrumentMod << endl;
	cout << "-----------------------" << endl;*/

}

/********************************************************************
*                      Pinedepain  - 6/27/08                       *
********************************************************************
*                       HasRequiredInstrument                      *                        
********************************************************************
* Return true if the bard has the correct instrument to sing the   *
* current song, else false.										*
********************************************************************/

bool Client::HasRequiredInstrument(int16 spell_id)
{
	Spell* spell = spells_handler.GetSpellPtr(spell_id);
	// Pinedepain // First, we check if the current song needs a component, if yes it means we have to have an instrument
	if (spell->GetSpellComponent(0) != 0xFFFF)
	{
		// Pinedepain // We get the current song skill
		int16 spell_skill = spell->GetSpellSkill();


		// Pinedepain // Now we check if we have an instrument mod for the current song skill
		switch (spell_skill)
		{

		case SS_PERCUSSION:
			{
				return (percussionInstrumentMod > 0);
				break;
			}

		case SS_BRASS:
			{
				return (brassInstrumentMod > 0);
				break;
			}

		case SS_WIND:
			{
				return (windInstrumentMod > 0);
				break;
			}

		case SS_STRING:
			{
				return (stringedInstrumentMod > 0);
				break;
			}

		case SS_SINGING:
			{
				return (singingInstrumentMod > 0);
				break;
			}

		default:
			{
				cout << "HasRequiredInstrument() >> Unknown spell skill : " << hex << spell_skill << endl;
				return (true);
				break;
			}
		}
	}

	// Pinedepain // The song doesn't need an instrument, we return true
	else
	{
		return (true);
	}
}

/********************************************************************
*                      Pinedepain  - 7/01/08                       *
********************************************************************
*                   GetInstrumentModAppliedToSong                  *                        
********************************************************************
* Return the instrument mod we can apply to the song corresponding *
* to spell_id.														*
********************************************************************/

int8 Client::GetInstrumentModAppliedToSong(Spell* spell)
{
	// Pinedepain // First of all, we have to check if the song is one of the selo's accelerando series, because
	// it seems they can't have an instrument mod higher than 24 otherwise it acts as if there was no bonus.

	// PinedepainDebug //
	//cout << "Client::GetInstrumentModAppliedToSong >> spell_id = " << spell_id << endl;
	if (!spell) return 0;
	TSpellID spell_id = spell->GetSpellID();
	switch (spell_id)
	{
		// Pinedepain // Selo's Accelerando, Selo's Accelerating Chorus, Selo's Song of Travel
	case 717:
		//case 7705:
		//case 19450:

		// Pinedepain // If we are in this case, we return the min between the instrument mod and 24 (highest possible
		// instrument mod for those songs).
		return (min(GetInstrumentModByType(SS_PERCUSSION),24));
		break;

		// Pinedepain // If we're in the normal case, we return the instrument mod corresponding to song skill
	default:
		{
			Spell* spell = spells_handler.GetSpellPtr(spell_id);
			int8 skill2 = spell->GetSpellSkill();
			return (GetInstrumentModByType(skill2)); //CFB*.  GetSpellSkill()
		}
	}
}

//o--------------------------------------------------------------
//| CreateZoneLineNode; Yeahlight, July 19, 2008
//o--------------------------------------------------------------
//| Creates a zoneline node on the fly via #createzoneline
//o--------------------------------------------------------------
void Client::CreateZoneLineNode(char* zoneName, char* connectingZoneName, int16 range)
{
	char fileNameSuffix[] = "ZoneLineNodes.sql";
	char fileNamePrefix[100] = "./Maps/ZoneLines/";
	strcat(fileNamePrefix, zone->GetShortName());
	strcat(fileNamePrefix, fileNameSuffix);
	ofstream myfile;
	myfile.open(fileNamePrefix, ios::app);
	if(range == 75)
		myfile<<"INSERT INTO `zone_line_nodes` ( `zone` , `x` , `y` , `z` , `target_zone` , `target_x` , `target_y` , `target_z`) VALUES ('"<<zoneName<<"','"<<GetX()<<"','"<<GetY()<<"','"<<GetZ()<<"','"<<connectingZoneName<<"','0','0','0');"<<endl;
	else
		myfile<<"INSERT INTO `zone_line_nodes` ( `zone` , `x` , `y` , `z` , `target_zone` , `target_x` , `target_y` , `target_z` , `range`) VALUES ('"<<zoneName<<"','"<<GetX()<<"','"<<GetY()<<"','"<<GetZ()<<"','"<<connectingZoneName<<"','0','0','0','"<<range<<"');"<<endl;
	myfile.close();
}


//o--------------------------------------------------------------
//| TeleportPC; Harakiri, September 16, 2009
//o--------------------------------------------------------------
//| Teleports the player, the client automatically issues a changezone
//| request if the player is not already in the zone
//| when the player is already in the zone, the client automatically 
//| moves the player
//| This method should now used for all moving/zoning EXCEPT translocate
//| The teleportPC function in the game client at offset 00495996
//| will call the same function as zonePC at offset 004DC1B5
//| when the player is not already in the zone
//| note: for negative x y the client always adds +1 to the coords when
//| moving in the same zone
//o--------------------------------------------------------------
void Client::TeleportPC(char* zonename, float x, float y, float z, float heading)
{	
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Client::TeleportPC(zone name = %s, x = %f, y = %f, z = %f, heading = %f)", zonename, x, y, z,heading);
				
	// if not in this zone, memorize coords, client will issue a ProcessOP_ZoneChange shortly
	if(strcmp(zone->GetShortName(), zonename) != 0)
	{
		this->usingSoftCodedZoneLine = true;
		this->isZoning = true;
		this->zoningX = (sint32)x;
		this->zoningY = (sint32)y;
		this->zoningZ = (sint32)z;
		this->tempHeading = (int8)heading;
	// reset
	} else {	
		this->isZoningZP = false;
		this->isZoning = false;
		this->usingSoftCodedZoneLine = false;
	}
	
	APPLAYER* outapp = new APPLAYER(OP_TeleportPC, sizeof(TeleportPC_Struct));

	TeleportPC_Struct* tls = (TeleportPC_Struct*)outapp->pBuffer;
	memset(outapp->pBuffer, 0, sizeof(TeleportPC_Struct));
	strcpy(tls->zone, zonename);			
	tls->xPos = x;
	tls->yPos = y;	
	
	// Harakiri - the client does not sometimes like going to 0 z at a specific point in a zone
	// example in qeynos2 standing infront of the gates and going to 0 0 0 will teleport to a random location sometimes
	// the closer you walk to 0 0 0 tho, teleport 0 0 0 will always work
	// however, i figured if z is not 0, like a small value, it will always work
	// translocate does not have this issue tho, only teleport
	if(z==0) {
		// workaround
		tls->zPos = 0.1f;
	} else {
		tls->zPos = z;
	}

	if(heading!=0) {
		tls->heading = heading*2; // Harakiri client will divide this by 2
	}

	QueuePacket(outapp);
	safe_delete(outapp);
	
}

//o--------------------------------------------------------------
//| TranslocatePC; Harakiri, September 16, 2009
//o--------------------------------------------------------------
//| Difference to TeleportPC, 
//| TranslocatePC does not seem to have a heading
//| some different setting on the client, but both ultimativly call
//| the same functions for local moving or zoning request
//| my guess is translocate was added for the translocate spells
//| they didnt bother with code reuse for whatever reason
//| note: for negative x y the client always adds +1 to the coords when
//| moving in the same zone
//o--------------------------------------------------------------
void Client::TranslocatePC(char* zonename, float x, float y, float z)
{			

	
	
	// if not in this zone, memorize coords, client will issue a ProcessOP_ZoneChange shortly
	if(strcmp(zone->GetShortName(), zonename) != 0)
	{
		this->usingSoftCodedZoneLine = true;
		this->isZoning = true;
		this->zoningX = (sint32)x;
		this->zoningY = (sint32)y;
		this->zoningZ = (sint32)z;
		this->tempHeading = (int8)heading;
	// reset
	} else {	
		this->isZoningZP = false;
		this->isZoning = false;
		this->usingSoftCodedZoneLine = false;
	}

	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Client::TranslocatePC(zone name = %s, x = %f, y = %f, z = %f heading = %f)", zonename, x, y, z,heading);

	APPLAYER* outapp = new APPLAYER(OP_Translocate, sizeof(Translocate_Struct));
	Translocate_Struct* tls = (Translocate_Struct*)outapp->pBuffer;
	memset(outapp->pBuffer, 0, sizeof(Translocate_Struct));
	strcpy(tls->zone, zonename);
	
	tls->confirmed = 1;
	tls->x = x;
	tls->y = y;
	tls->z = z;	

	QueuePacket(outapp);
	safe_delete(outapp);
	
}

//o--------------------------------------------------------------
//| ZonePC; Yeahlight, July 24, 2008
//o--------------------------------------------------------------
//| Moves a client from one zone to another using the TL
//| opcode/process.
//o--------------------------------------------------------------
void Client::ZonePC(char* zonename, float x, float y, float z)
{	
	if(GetDebugMe())
		Message(LIGHTEN_BLUE, "Debug: You are being sent to %.2f %.2f %.2f in %s", x, y, z, zonename);
	

	// Harakiri Client will automatically do local movepc if the client is already in the target zone
	TeleportPC(zonename,x,y,z);

	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::ZonePC(zone name = %s, x = %f, y = %f, z = %f)", zonename, x, y, z);
}

//o--------------------------------------------------------------
//| ScanForZoneLines; Yeahlight, July 20, 2008
//o--------------------------------------------------------------
//| Compares the player's location to that of each zoneline
//| node. Zones a player to a connecting zone if they are
//| close enough to a zoneline node.
//o--------------------------------------------------------------
void Client::ScanForZoneLines()
{
	bool debugFlag = true;

	//Yeahlight: Check to see if player is in range of each zoneline node
	for(int i = 0; i < zone->numberOfZoneLineNodes; i++)
	{
		int16 range = zone->thisZonesZoneLines[i]->range;
		int16 zDiff = zone->thisZonesZoneLines[i]->maxZDiff;
		if(zDiff == 0)
			zDiff = 50000;
		if(abs(this->GetX() - zone->thisZonesZoneLines[i]->x) > range || abs(this->GetY() - zone->thisZonesZoneLines[i]->y) > range || abs(this->GetZ() - zone->thisZonesZoneLines[i]->z) > zDiff || (GetZ() + 10) < zone->thisZonesZoneLines[i]->z)
		{
			continue;
		}
		else
		{
			if(CheckCoordLos(GetX(), GetY(), GetZ(), zone->thisZonesZoneLines[i]->x, zone->thisZonesZoneLines[i]->y, zone->thisZonesZoneLines[i]->z))
			{
				if(debugFlag && GetDebugMe())
					this->Message(WHITE,"Debug: You have come in contact with zoneline node: %i",zone->thisZonesZoneLines[i]->id);
				if(strcmp(zone->GetShortName(), zone->thisZonesZoneLines[i]->target_zone) == 0)
				{
					this->MovePC(0, zone->thisZonesZoneLines[i]->target_x, zone->thisZonesZoneLines[i]->target_y, zone->thisZonesZoneLines[i]->target_z, false, false);
				}
				else
				{
					this->tempHeading = zone->thisZonesZoneLines[i]->heading;
					this->usingSoftCodedZoneLine = true;
					this->isZoning = true;
					if(zone->thisZonesZoneLines[i]->keepX == 1)
						this->zoningX = x_pos;
					else
						this->zoningX = zone->thisZonesZoneLines[i]->target_x;
					if(zone->thisZonesZoneLines[i]->keepY == 1)
						this->zoningY = y_pos;
					else
						this->zoningY = zone->thisZonesZoneLines[i]->target_y;
					if(zone->thisZonesZoneLines[i]->keepZ == 1)
						this->zoningZ = z_pos;
					else
						this->zoningZ = zone->thisZonesZoneLines[i]->target_z;
					this->ZonePC(zone->thisZonesZoneLines[i]->target_zone, zone->thisZonesZoneLines[i]->target_x, zone->thisZonesZoneLines[i]->target_y, zone->thisZonesZoneLines[i]->target_z);
				}
				break;
			}
		}
	}
}

//o--------------------------------------------------------------
//| FearMovement; Yeahlight, Aug 17, 2008
//o--------------------------------------------------------------
//| Taken from EQEMU 7.0: Fears a PC towards x, y, z. This 
//| requires a ton of adeptation.
//o--------------------------------------------------------------
bool Client::FearMovement(float x, float y, float z)
{
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_PATHING, "Client::FearMovement(x = %f, y = %f, z = %f)", x, y, z);
	if (IsRooted())
	{
		this->animation = 0;
		return true;
	}
	float nx = this->x_pos;
	float ny = this->y_pos;
	float nz = this->z_pos;
	float vx, vy, vz;
	float vb;
	float speed = this->runspeed;
	float speed_mod = ((float)(speed*spellbonuses.MovementSpeed))/100.0f;
	speed += speed_mod;

	if (x_pos == x && y_pos == y)
	{
		this->animation = 0;
		return false;
	}

	bool ret = true;

	this->faceDestination(x, y);

	// --------------------------------------------------------------------------
	// 1: get Vector AB (Vab = B-A)
	// --------------------------------------------------------------------------
	vx = x - nx;
	vy = y - ny;
	vz = z - nz;

	speed *= 30; //First we recalc into clientside units. 1.2 = 36, etc.
	this->animation = speed;

	//Now we recalc it into units per second moved.
	speed *= 0.76;

	//Divide by ten to account for movement happening every 0.1
	speed /= 10;

	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_PATHING, "Client::FearMovement(): speed = %f", speed);

	// --------------------------------------------------------------------------
	// 2: get unit vector
	// --------------------------------------------------------------------------
	vb = speed / sqrt (vx*vx + vy*vy);

	if (vb >= 0.5) //Stop before we've reached the point in case it's too close to a wall
	{
		fear_walkto_x = x_pos;
		fear_walkto_y = y_pos;
		fear_walkto_z = z_pos;
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_PATHING, "Client::FearMovement(): stopping before getting to the wall");
		ret = false;
	}
	else
	{
		// --------------------------------------------------------------------------
		// 3: destination = start plus movementvector (unitvektor*speed)
		// --------------------------------------------------------------------------
		x_pos = x_pos + vx*vb;
		y_pos = y_pos + vy*vb;
		if (zone->map == 0)
		{
			z_pos = z_pos + vz*vb;
		}
		else
		{
			NodeRef pnode = zone->map->SeekNode( zone->map->GetRoot(), x_pos, y_pos );
			if (pnode == NODE_NONE)
			{
				z_pos = z_pos + vz*vb;
			}
			else
			{
				VERTEX me;
				me.x = x_pos;
				me.y = y_pos;
				me.z = z_pos + vz*vb + GetSize();
				VERTEX hit;
				FACE *onhit;
				float best_z = zone->map->FindBestZ(pnode, me, &hit, &onhit);
				if (best_z != -999999)
				{
					z_pos = best_z + 1;
				}
				else
				{
					z_pos = z_pos + vz*vb;
				}
			}
		}
	}
	APPLAYER* app = new APPLAYER(OP_ClientUpdate, sizeof(SpawnPositionUpdate_Struct));
	SpawnPositionUpdate_Struct* spu = (SpawnPositionUpdate_Struct*)app->pBuffer;
	MakeSpawnUpdate(spu);
	entity_list.QueueClients(this,app);
	safe_delete(app);
	return ret;
}

//o--------------------------------------------------------------
//| FindBestEquipmentSlot; Yeahlight, Dec 21, 2008
//o--------------------------------------------------------------
//| Returns the most likely used slot for the provided item
//o--------------------------------------------------------------
int16 Client::FindBestEquipmentSlot(Item_Struct* item)
{
	//Yeahlight: Item does not exist or the item is a book/container
	if(!item || (item && item->type != 0x00))
	{
		return 99;
	}

	int16 returnSlot = 99;

	//Yeahlight: Rip the bitmask apart and seperate it into slots
	int16 slots[22] = {0};
	int itemSlots = item->equipableSlots;
	int16 counter = 0;

	//Yeahlight: Keep iterating until we shift all the bits off the end
	while(itemSlots > 0)
	{
		if(itemSlots % 2)
		{
			slots[counter] = 1;
		}
		itemSlots = itemSlots >> 1;
		counter++;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	//Yeahlight: Primary slot is 1st priority
	if(slots[13] && pp.inventory[13] == 0xFFFF)
	{
		returnSlot = 13;
	}
	//Yeahlight: Secondary slot is 2nd priority
	else if(slots[14] && pp.inventory[14] == 0xFFFF)
	{
		returnSlot = 14;
	}
	else
	{
		//Yeahlight: Search the rest of the slots in no specific priority order
		for(int i = 0; i < 22; i++)
		{
			if(slots[i] && pp.inventory[i] == 0xFFFF)
			{
				returnSlot = i;
				i = 22;
			}
		}
	}

	//Yeahlight: A slot has been found. We need to make sure this race/class/deity combiniation may equip it, though
	if(returnSlot != 99)
	{
		if(CanEquipThisItem(item))
		{
			return returnSlot;
		}
		else
		{
			return 99;
		}
	}
	else
	{
		return 99;
	}
}

//o--------------------------------------------------------------
//| CanEquipThisItem; Yeahlight, May 18th, 2009
//o--------------------------------------------------------------
//| Checks to see if the PC may use the provided item
//o--------------------------------------------------------------
bool Client::CanEquipThisItem(Item_Struct* item)
{
	//Yeahlight: Item does not exist or the item is a book/container
	if(!item || (item && item->type != 0x00))
		return false;

	int16 clientClass = this->GetClass();
	int16 clientRace = this->GetRace();

	//Yeahlight: Special check for the iksar race
	if(clientRace == IKSAR)
		clientRace = 13;

	//Yeahlight: Our attributes start at one while our arrays start at zero
	clientClass--;
	clientRace--;

	//Yeahlight: Rip the bitmasks apart and seperate it into slots
	//Yeahlight: TODO: Check deities
	int16 classes[14] = {0};
	int16 races[13] = {0};
	uint16 itemClasses = item->common.classes;
	uint16 itemRaces = item->common.normal.races;
	int16 counter = 0;

	//Yeahlight: Keep iterating until we shift all the bits off the end
	while(itemClasses > 0)
	{
		if(itemClasses % 2)
		{
			classes[counter] = 1;
		}
		itemClasses = itemClasses >> 1;
		counter++;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	counter = 0;
	//Yeahlight: Keep iterating until we shift all the bits off the end
	while(itemRaces > 0)
	{
		if(itemRaces % 2)
		{
			races[counter] = 1;
		}
		itemRaces = itemRaces >> 1;
		counter++;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	//Yeahlight: This class and race may use this item
	if(classes[clientClass] && races[clientRace])
		return true;
	else
		return false;
}


//o--------------------------------------------------------------
//| GetTotalLevelExp; Yeahlight, Dec 29, 2008
//o--------------------------------------------------------------
//| Returns the total amount of exp required to move between
//| 'level' and 'level + 1'
//o--------------------------------------------------------------
uint32 Client::GetTotalLevelExp()
{
	int16 playersLevel = GetLevel();
	//Yeahlight: There is no level 61, so prevent the array from going out of bounds
	if(playersLevel >= 60)
		return 2140000000;
	uint32 exp_needed[61];
	for(int i = 1; i <= playersLevel + 1; i++)
		exp_needed[i] = GetEXPForLevel(i);
	return exp_needed[playersLevel + 1] - exp_needed[playersLevel];
}

//o--------------------------------------------------------------
//| GetFearDestination; Yeahlight, Mar 4, 2009
//o--------------------------------------------------------------
//| Assigns a fear destination for the client
//o--------------------------------------------------------------
bool Client::GetFearDestination(float x, float y, float z)
{
	int loop = 0;
	bool permit = false;
	float ranx, rany, ranz;
	while(loop < 100)
	{
		int ran = 250 - (loop*2);
		loop++;
		ranx = x + rand()%ran - rand()%ran;
		rany = y + rand()%ran - rand()%ran;
		ranz = FindGroundZ(ranx, rany, 2);
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
	//Yeahlight: PC is in an odd location, uncapable of locating a reasonable destination, so drop the debuff immediately
	else
	{
		BuffFadeByEffect(SE_Fear);
		return false;
	}
	//Yeahlight: PC is rooted, so they will not be given a velocity
	if(IsRooted())
		this->animation = 0;
	else
		this->animation = 6;
	this->delta_heading = 0;
	this->delta_z = 0;
	faceDestination(fear_walkto_x, fear_walkto_y);
	SendPosUpdate(true, PC_UPDATE_RANGE, false);
	return true;
}

//o--------------------------------------------------------------
//| GetCharmDestination; Yeahlight, Mar 17, 2009
//o--------------------------------------------------------------
//| Assigns a charm destination for the client
//o--------------------------------------------------------------
bool Client::GetCharmDestination(float x, float y)
{
	Mob* myOwner = GetOwner();
	//Yeahlight: PC is rooted, so they will not be given a velocity
	if(IsRooted())
	{
		this->animation = 0;
	}
	//Yeahlight: PC is following its master
	else if(myOwner && target == myOwner)
	{
		//Yeahlight: PC's master is an NPC, issue walking movespeed
		if(myOwner->IsNPC())
		{
			this->animation = 3;
		}
		//Yeahlight: PC's master is another PC
		else if(myOwner->IsClient())
		{
			//Yeahlight: PC is following a running PC, issue max movespeed
			if(GetPetOrder() == SPO_Follow)
			{
				this->animation = 6;
			}
			//Yeahlight: PC is moving to its gaurd spot, issue walking movespeed
			else
			{
				this->animation = 3;
			}
		}
	}
	//Yeahlight: PC is forced to engage, issue max movespeed
	else
	{
		this->animation = 6;
	}
	this->delta_heading = 0;
	this->delta_z = 0;
	faceDestination(x, y);
	SendPosUpdate(true, PC_UPDATE_RANGE, false);
	return true;
}

//o--------------------------------------------------------------
//| UpdateCharmPosition; Yeahlight, Mar 19, 2009
//o--------------------------------------------------------------
//| Updates the client's serverside position while charmed
//o--------------------------------------------------------------
void Client::UpdateCharmPosition(float targetX, float targetY, int16 meleeReach)
{
	//Yeahlight: Do NOT touch this formula; thanks!
	float d = this->animation * 2.4 / 5 * 1.57;
	
	float distance = fdistance(targetX, targetY, GetX(), GetY());

	if(distance < meleeReach)
	{
		this->animation = 0;
		this->SendPosUpdate(true, PC_UPDATE_RANGE, false);
		return;
	}
	
	float x1 = x_pos;
	float y1 = y_pos;
	float x2 = targetX;
	float y2 = targetY;			

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
		float d1 = fdistance(new_x, new_y, x2,y2);
		float d2 = fdistance(new_x2, new_y2, x2,y2);

		if(d1 < d2)
		{
			;
		}
		else
		{
			new_x = new_x2;
			new_y = new_y2;
		}
	}
	x_pos = new_x;
	y_pos = new_y;
}

float Client::fdistance(float x1, float y1, float x2, float y2)
{
	return sqrt(square(x2-x1)+square(y2-y1));
}

float Client::square(float number)
{
	return number * number;
}

bool Client::IsBerserk(){
	if(this->GetClass() != WARRIOR) return false;
	return berserk;
}

//o--------------------------------------------------------------
//| SendCharmPermissions; Yeahlight, June 2, 2009
//o--------------------------------------------------------------
//| Grants the PC pet permissions with a new untargetable spawn
//o--------------------------------------------------------------
void Client::SendCharmPermissions()
{
	//Yeahlight: Depop old charm PH if one exists
	char oldNpcNameSuffix[] = "_CHARM00";
	char oldNpcNamePrefix[100] = "";
	strcpy(oldNpcNamePrefix, this->GetName());
	strcat(oldNpcNamePrefix, oldNpcNameSuffix);
	Mob* charmPH = entity_list.GetMob(oldNpcNamePrefix);
	if(charmPH && charmPH->IsNPC())
	{
		charmPH->Depop();
	}
	//Yeahlight: Check for _CHARM01 NPC, too
	oldNpcNamePrefix[strlen(oldNpcNamePrefix) - 1] = '1';
	charmPH = entity_list.GetMob(oldNpcNamePrefix);
	if(charmPH && charmPH->IsNPC())
	{
		charmPH->Depop();
	}

	NPCType* npc_type = new NPCType;
	memset(npc_type, 0, sizeof(NPCType));
	npc_type->gender = 0;
	char npcNameSuffix[] = "_CHARM";
	char npcNamePrefix[100] = "";
	strcpy(npcNamePrefix, this->GetName());
	strcat(npcNamePrefix, npcNameSuffix);
	strcpy(npc_type->name, npcNamePrefix);
	npc_type->body_type = BT_Trigger;
	npc_type->level = 1;
	npc_type->race = 127;
	npc_type->class_ = 1;
	npc_type->texture = 1;
	npc_type->helmtexture = 0;
	npc_type->size = 0;
	npc_type->max_hp = 5;
	npc_type->cur_hp = 5;
	npc_type->min_dmg = 1;
	npc_type->max_dmg = 1;
	npc_type->walkspeed = 0.7f;
	npc_type->runspeed = 1.25f;
	npc_type->npc_id = 0;
	//Yeahlight: Give the NPC the "will never agro" special ability
	strcpy(npc_type->special_attacks, "H");
	NPC* npc = new NPC(npc_type, 0, this->GetX(), this->GetY(), this->GetZ(), this->GetHeading());
	npc->SetOwnerID(this->GetID());
	entity_list.AddNPC(npc);
}

//o--------------------------------------------------------------
//| SetHide; Yeahlight, June 25, 2009
//o--------------------------------------------------------------
//| Changes the clients hide status and appearance
//o--------------------------------------------------------------
void Client::SetHide(bool bHidden)
{
	 bool inHide = GetHide();
	 //Yeahlight: PC was previously hidden but is now visible, so change their appearance
	 if(inHide && bHidden == false)
	 {
		this->SendAppearancePacket(this->GetID(), SAT_Invis, 0, true);
	 }
	 //Yeahlight: PC was previously visible and is now hidden, so change their appearance
	 else if(inHide == false && bHidden)
	 {
		this->SendAppearancePacket(this->GetID(), SAT_Invis, 1, true);
	 }
	 bHide = bHidden;
}


//o--------------------------------------------------------------
//| GetRelativeCoordToBoat; Tazadar, July 01, 2009
//o--------------------------------------------------------------
//| Give the relative coords
//o--------------------------------------------------------------
void Client::GetRelativeCoordToBoat(float x,float y,float angle,float &xout,float &yout)
{
	xout = cos(angle)*x - sin(angle)*y;
	yout = sin(angle)*x + cos(angle)*y;
}

//o--------------------------------------------------------------
//| AddSpellToRainList; Yeahlight, July 09, 2009
//o--------------------------------------------------------------
//| Changes the clients hide status and appearance
//o--------------------------------------------------------------
bool Client::AddSpellToRainList(Spell* spell, int8 waveCount, float x, float y, float z)
{
	//Yeahlight: No spell was supplied; abort
	if(spell == NULL)
		return false;
	int16 counter = 0;
	//Yeahlight: Iterate through the array until we find an empty slot or run out of slots to check
	while(counter < 10)
	{
		//Yeahlight: Found an empty array element to fill
		if(rainList[counter].spellID == NULL)
		{
			rainList[counter].spellID = spell->GetSpellID();
			rainList[counter].wavesLeft = waveCount;
			rainList[counter].x = x;
			rainList[counter].y = y;
			rainList[counter].z = z;
			return true;
		}
		counter++;
	}
	return false;
}

//o--------------------------------------------------------------
//| DoRainWaves; Yeahlight, July 09, 2009
//o--------------------------------------------------------------
//| Sends out pulses for each rain wave in the rainList array
//o--------------------------------------------------------------
bool Client::DoRainWaves()
{
	bool ret = false;
	int16 counter = 0;
	//Yeahlight: While the client is still alive, iterate through the array and issue rain spell waves
	while(GetHPRatio() > 0 && counter < 10)
	{
		//Yeahlight: Client has a rain spell in their rainList array
		if(rainList[counter].spellID != NULL)
		{
			Spell* spell = spells_handler.GetSpellPtr(rainList[counter].spellID);
			//Yeahlight: Spell exists and is a valid AE spell, so cast it
			if(spell && (spell->GetSpellTargetType() == ST_AETarget || spell->GetSpellTargetType() == ST_AECaster))
			{
				entity_list.AESpell(this, rainList[counter].x, rainList[counter].y, rainList[counter].z, spell->GetSpellAOERange(), spell);
				//Yeahlight: There are waves left on this rain spell, decrease it by one
				if(rainList[counter].wavesLeft > 1)
				{
					rainList[counter].wavesLeft--;
				}
				//Yeahlight: Rain spell is out of waves, so zero out the reference
				else
				{
					rainList[counter].spellID = NULL;
				}
				ret = true;
			}
			//Yeahlight: Spell does not exist, so zero out this entry
			else
			{
				rainList[counter].spellID = NULL;
				rainList[counter].wavesLeft = NULL;
			}
		}
		counter++;
	}
	return ret;
}

/********************************************************************
 *                      Harakiri - 8/16/2009                        *
 ********************************************************************
 *		       IsInWater										    *
 ********************************************************************
	Checks if the player is in water, needs zone water files to work
 ********************************************************************/
bool	Client::IsInWater() {

	if(zone->watermap!=NULL) {

		bool val = zone->watermap->InWater(GetX(), GetY(),  GetZ());
		if(!val) {
			// Harakiri we need to tweak the Z coord a bit
			// sometimes when the client swims with the face to the sky
			// he is not in water
			// i added the GetSize()/2 to tweak that value
			// the below seems to work with levitate fine, as long as the feets
			// are not in the water =)
			return  zone->watermap->InWater(GetX(), GetY(),  GetZ()-(GetSize()/2));
		} else {
			return true;
		}
	}

	return false;
}

/********************************************************************
 *                      Harakiri - 8/16/2009                        *
 ********************************************************************
 *		       IsInLava											    *
 ********************************************************************
	Checks if the player is in lava, needs zone water files to work
 ********************************************************************/
bool	Client::IsInLava() {

	if(zone->watermap!=NULL) {

		return zone->watermap->InLava(GetX(), GetY(),  GetZ());
	}

	return false;
}

//o--------------------------------------------------------------
//| SendTranslocateConfirmation; Enraged - 8/25/2009
//o--------------------------------------------------------------
//| Send the translocate confirmation to the client and
//|		store all necessary information to translocate the client
//|		once confirmed.
//| ** based off of EQEmu 7 code
//| Harakiri: Sept. 17. 2009 fixed and identified all Translocate_Struct bytes and translocate to bind
//o--------------------------------------------------------------
void Client::SendTranslocateConfirmation(Mob* caster, Spell* spell)
{

	if(!caster || !spell)
		return;

	//get necessary translocate spell information
	pendingTranslocate = true;
	pendingTranslocateSpellID = spell->GetSpellID();

	//translocate to bind
	if(pendingTranslocateSpellID == 1422 /*Translocate*/ || pendingTranslocateSpellID ==  1334 /*GroupTranslocate*/)
	{
		pendingTranslocateX = (sint32)GetPlayerProfilePtr()->bind_location[1][1];
		pendingTranslocateY = (sint32)GetPlayerProfilePtr()->bind_location[0][1];
		pendingTranslocateZ = (float)GetPlayerProfilePtr()->bind_location[2][1];
		char* zone_name = this->GetPlayerProfilePtr()->bind_point_zone;
		strcpy(pendingTranslocateZone, zone_name);
		// Harakiri: is there a byte in the playerprofile for this?
		pendingTranslocateHeading = GetHeading();
	}
	else
	{
		pendingTranslocateX = spell->GetSpellBase(1);
		pendingTranslocateY = spell->GetSpellBase(0);
		pendingTranslocateZ = spell->GetSpellBase(2);
		pendingTranslocateHeading = spell->GetSpellBase(3);
		strcpy(pendingTranslocateZone, spell->GetSpellTeleportZone());
	}

	//create translocate request packet to the client
	APPLAYER* outapp = new APPLAYER(OP_Translocate, sizeof(Translocate_Struct));
	memset(outapp->pBuffer, 0, outapp->size);
	Translocate_Struct* ts = (Translocate_Struct*) outapp->pBuffer;

	strcpy(ts->caster, caster->GetName());

	strcpy(ts->zone, pendingTranslocateZone);

	// Harakiri : depending on the spell ID, the client will popup either you want to translocate to bind? or XYZ
	ts->spellID=pendingTranslocateSpellID;

	// Harakiri: initial request = ask client with popup
	ts->confirmed = 0;

	// Harkiri: the client will set the playerProfile values with these
	// but does not sent them back for zonechange for whatever reason
	ts->x = pendingTranslocateX;
	ts->y = pendingTranslocateY;
	ts->z = pendingTranslocateZ;
	
	pendingTranslocate_timer->Start(PC_TRANSLOCATE_ANSWER_DURATION);
	
	QueuePacket(outapp);
	safe_delete(outapp);

	return;
}


//---------------------------------------------------------------
//| SendInitiateConsumeItem; Harakiri, 10-14-2009
//---------------------------------------------------------------
//| Harakiri this can be used to tell the client that it should consume an item
//|	either set the itemType to i.e. food or drink, and itemID to 0xFFFF
//| or specify a specifc itemID, i have no idea for what it was used
void Client::SendInitiateConsumeItem(ItemTypes itemType, int16 itemID) {
	
	struct InitiateConsume_Struct
	{
		int8 itemType;
		int16 itemID;
	};

	APPLAYER* outapp = new APPLAYER(OP_InitiateConsume, sizeof(InitiateConsume_Struct));
	memset(outapp->pBuffer, 0, outapp->size);
	InitiateConsume_Struct* ic = (InitiateConsume_Struct*) outapp->pBuffer;
	ic->itemType=(int8)itemType;
	ic->itemID=itemID;

	QueuePacket(outapp);
	safe_delete(outapp);

	return;

}
//---------------------------------------------------------------
//| SendItemMissing; Harakiri, 10-14-2009
//---------------------------------------------------------------
//| Can be used to sent the item a message which items are missing
//|	up to 6 item names can be sent, method can be extended to list of itemids
void Client::SendItemMissing(int32 itemID, int8 itemType) {

	struct ItemMissing_Struct
	{
	 /* 0000*/  int8 unknown[12];
	 /* 0012*/	char   item[6][32]; // client can display up 6 missing items 
     /* 0204*/	int8 itemType; // 1 = "Missing spell components:" 2 = "For this song you must play a " 3 = "You are missing: "
	};
	
	Item_Struct* item = Database::Instance()->GetItem(itemID);

	if(item) {
		APPLAYER* outapp = new APPLAYER(OP_ItemMissing, sizeof(ItemMissing_Struct));
		memset(outapp->pBuffer, 0, outapp->size);
		ItemMissing_Struct* im = (ItemMissing_Struct*) outapp->pBuffer;

		im->itemType=itemType;	

		// instruments
		if(itemType == 2) {
			switch(item->common.itemType) {
				case ItemTypeWindInstr: strcpy(im->item[0],"wind instrument"); break;
				case ItemTypeStringInstr: strcpy(im->item[0],"stringed instrument"); break;
				case ItemTypeBrassInstr: strcpy(im->item[0],"brass instrument"); break;
				case ItemTypeDrumInstr: strcpy(im->item[0],"percussion instrument"); break;
				default: strcpy(im->item[0],"unknown instrument");				
			}
		} else {
			strcpy(im->item[0],item->name);
		}

		QueuePacket(outapp);
		safe_delete(outapp);
	} else {
		EQC::Common::Log(EQCLog::Error, CP_CLIENT, "SendItemMissing unknown item %i",itemID);
	}

	return;
}