
#include <logger.h>
#include <cmath>
#include <queue>

#include "Config.h"
#include "skills.h"
#include "mob.h"
#include "client.h"
#include "PlayerCorpse.h"
#include "zone.h"
#include "spdat.h"
#include "map.h"
#include "moremath.h"
#include "SpellsHandler.hpp"
#include <fstream>

#include "eq_opcodes.h"
#include "eq_packet_structs.h"
#include "packet_functions.h"
#include "PlayerProfile.h"
#include "EQCException.hpp"
#include "itemtypes.h"
#include <cstdio>

using namespace EQC::Common::Network;
using namespace std;
extern EntityList		entity_list;
extern SpellsHandler	spells_handler;
extern Database			database;
extern Zone*			zone;

void DumpPacketHex(APPLAYER*);

Mob::Mob(char*   in_name, 
         char*   in_Surname, 
         sint32  in_cur_hp, 
         sint32  in_max_hp,
         int8    in_gender,
         int8    in_race,
         int8    in_class,
         int8    in_deity,
         int8    in_level,
		 TBodyType
				in_body_type,
         int32	 in_npctype_id, // rembrant, Dec. 20, 2001
         int16*	 in_skills, // socket 12-29-01
		 float	in_size,
		 float	in_walkspeed,
		 float	in_runspeed,
         float	in_heading,
         float	in_x_pos,
         float	in_y_pos,
         float	in_z_pos,

         int8    in_light,
         int8*   in_equipment,
		 int8	 in_texture,
		 int8	 in_helmtexture,
		 int16	 in_ac,
		 int16	 in_atk,
		 int8	 in_str,
		 int8	 in_sta,
		 int8	 in_dex,
		 int8	 in_agi,
		 int8	 in_int,
		 int8	 in_wis,
		 int8	 in_cha,
		 sint16	 in_mr,
		 sint16	 in_cr,
		 sint16	 in_fr,
		 sint16	 in_dr,
		 sint16	 in_pr,
		 int32	 in_min_dmg,
		 int32	 in_max_dmg,
		 char    in_special_attacks[20],
		 sint16	 in_attack_speed_modifier,
		 int16	 in_main_hand_texture,
		 int16   in_off_hand_texture,
		 sint16	 in_my_regen_rate,
		 bool	 in_passive_see_invis,
		 bool	 in_passive_see_invis_to_undead,
		 SPAWN_TIME_OF_DAY in_time_of_day
		)
{
	strncpy(name, in_name, 30);
	strncpy(Surname, in_Surname, 20);
	this->SetBodyType(in_body_type);
	cur_hp		= in_cur_hp;
	max_hp		= in_max_hp;
	base_hp		= in_max_hp;
	gender		= in_gender;
	race		= in_race;
	base_gender	= in_gender;
	base_race	= in_race;
	class_		= in_class;
	deity		= in_deity;
	level		= in_level;
	npctype_id	= in_npctype_id; // rembrant, Dec. 20, 2001
	size		= in_size;
	base_size	= in_size;
	walkspeed	= in_walkspeed;
	runspeed	= in_runspeed;
	heading		= in_heading;
	animation	= 0;
	
	x_pos		= in_x_pos;
	y_pos		= in_y_pos;
	z_pos		= in_z_pos;

	light		= in_light;
	texture		= in_texture;
	helmtexture	= in_helmtexture;

	BaseStats.AC		= in_ac;
	BaseStats.ATK		= in_atk;
	BaseStats.STR		= in_str;
	BaseStats.STA		= in_sta;
	BaseStats.DEX		= in_dex;
	BaseStats.AGI		= in_agi;
	BaseStats.INT		= in_int;
	BaseStats.WIS		= in_wis;
	BaseStats.CHA		= in_cha;
	BaseStats.MR		= in_mr;
	BaseStats.CR		= in_cr;
	BaseStats.FR		= in_fr;
	BaseStats.DR		= in_dr;
	BaseStats.PR		= in_pr;
	baseAttackSpeedModifier = 100 + in_attack_speed_modifier;

	NPCTypedata = 0;

	cur_mana = 0;
	max_mana = 0;
	this->SetInvisible(false);
	this->SetInvisibleUndead(false);
	this->SetInvisibleAnimal(false);

	// Kibanu
	time_of_day = in_time_of_day;

	int i = 0;
	for (i=0; i<9; i++)
	{
		if (in_equipment == 0)
		{
			equipment[i] = 0;
		}
		else
		{
			equipment[i] = in_equipment[i];
		}
	}
	for (i=0; i<74; i++) { // socket 12-29-01
		if (in_skills == 0) {
			skills[i] =0;
		}
		else {
			skills[i] = in_skills[i];
		}
	}
	for (int j = 0; j < 15; j++) {
		buffs[j].spell = NULL;
	}

	//Yeahlight: Load default armor / weapon textures from the database
	if(in_texture)
		equipment[1] = in_texture;
	if(in_main_hand_texture)
		equipment[7] = in_main_hand_texture;
	if(in_off_hand_texture)
		equipment[8] = in_off_hand_texture;

	this->delta_heading = 0;
	this->delta_x = 0;
	this->delta_y = 0;
	this->delta_z = 0;

	this->SetInvulnerable(false);
	this->appearance = StandingAppearance;
	this->guildeqid = 0xFFFFFFFF;

	int16 attackDelay = 2000 * (float)((float)(baseAttackSpeedModifier)/100.00f);
	attack_timer = new Timer(attackDelay);
	attack_timer_dw = new Timer(attackDelay);
	tic_timer = NULL;
	mana_timer = new Timer(6000);
	endu_timer = new Timer(5760);//Tazadar:Food formula shows the timer must be that !
	mana_timer->Start();
	endu_timer->Start();
	spellend_timer = new Timer(0);
	clicky_lockout_timer = new Timer(200);
	this->SetCastingSpell(NULL);
	target = 0;
	temptarget = 0;

	memset(&itembonuses, 0, sizeof(StatsStruct));
	memset(&spellbonuses, 0, sizeof(StatsStruct));

	pLastChange = 0;
	this->SetPetID(0);
	this->SetOwnerID(0, false);
	typeofpet = 0xFF;

	// Bind wound
	bindwound_timer = new Timer(10000);
	bindwound_timer->Disable();
	bindwound_target = 0;

	dx = 0; dy = 0; dz = 0;
	x_pos0 = 0;
	y_pos0 = 0;
	z_pos0 = 0; 

	//Init quest inventory to empty.
	quest_item_inventory = new vector<float>;

	hitWhileCasting = false;

	//Yeahlight: hitBoxExtension calculations
	hitBoxExtension = 2;
	if(size > 8)
		hitBoxExtension = size - 8 + 2;
	//Yeahlight: Dragons don't have a size per se, so give them a generic extension
	if(base_race == 49)	
		hitBoxExtension = 40;

	SetCurfp(false);
	fear_walkto_x = 0;
    fear_walkto_y = 0;
    fear_walkto_z = 0;

	mesmerized = false; // Pinedepain // We initialize mesmerized to false. When a mob enters the world, he's not mezzed.

	nodeReporter = false;
	InWater = false;

	mySpawnNode = NULL;

	for(int i = 0; i < 7; i++)
		equipmentColor[i] = 0;

	myMainHandWeapon = NULL;
	myOffHandWeapon = NULL;

	hasRuneOn = false;

	bonusProcSpell = NULL;

	SetPetOrder(SPO_Follow);

	SetCanSeeThroughInvis(false);
	SetCanSeeThroughInvisToUndead(false);
}     

Mob::~Mob()
{
	SetPet(0);
	safe_delete(spellend_timer);
	safe_delete(tic_timer);
	safe_delete(clicky_lockout_timer);
	safe_delete(attack_timer);//delete attack_timer;
	safe_delete(mana_timer);//delete mana_timer;
	safe_delete(endu_timer);
	safe_delete(attack_timer_dw);//delete attack_timer_dw;
	APPLAYER app;
	CreateDespawnPacket(&app);
	entity_list.QueueClients(this, &app, true);
	entity_list.RemoveFromTargets(this);
}

void Mob::CreateSpawnPacket(APPLAYER* app, Mob* ForWho)
{
	
	app->opcode = OP_NewSpawn;
	app->pBuffer = new uchar[sizeof(NewSpawn_Struct)];
	app->size = sizeof(NewSpawn_Struct);

	memset(app->pBuffer, 0, sizeof(NewSpawn_Struct));
	NewSpawn_Struct* ns = (NewSpawn_Struct*)app->pBuffer;
	FillSpawnStruct(ns, ForWho);
	EncryptSpawnPacket(app);
	
}

void Mob::FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho)
{
	ns->spawn.size = size;
	ns->spawn.walkspeed = walkspeed;
	ns->spawn.runspeed = runspeed;
	ns->spawn.heading = (int8) heading;
	ns->spawn.x_pos = (sint16) x_pos;
	ns->spawn.y_pos = (sint16) y_pos;
	if(this->IsClient())
		ns->spawn.z_pos = (sint16) (z_pos * 100);
	else
		ns->spawn.z_pos = (sint16) (z_pos * 10);
	ns->spawn.spawn_id = GetID();
	ns->spawn.cur_hp = GetHPRatio();
	ns->spawn.race = race;
	ns->spawn.body_type	= this->GetBodyType();	// Cofruben: 16/08/08.

	ns->spawn.GuildID = 0xffff;
	if(GetInvisible() == false)
		ns->spawn.invis = 0;
	else
		ns->spawn.invis = 1;
	ns->spawn.NPC = 1;
	ns->spawn.class_ = class_;
	ns->spawn.gender = gender;
	ns->spawn.level = level;
	ns->spawn.anim_type = 0x64;
	ns->spawn.light = light;

	ns->spawn.LD = 0;
	//Yeahlight: I don't know why, but out database does not differentiate between model texture and armor texture. We cannot use "texture" to fill out
	//           armor textures, such as robes on the PoD. In order to use these armor textures, we must pull the textures from equipment[], which is
	//           done by setting the main race texture to 0xFF. I don't know of any races with more than 7 textures, so we should be ok setting the
	//           value at 7
	//           TODO: Look into the maximum number of textures for models
	if(!texture || texture > 7)
		ns->spawn.npc_armor_graphic = 0xff;
	else
		ns->spawn.npc_armor_graphic = texture;
	ns->spawn.npc_helm_graphic = helmtexture;

	strcpy(ns->spawn.name, name);
	strcpy(ns->spawn.Surname, Surname);
	ns->spawn.deity = deity;

	ns->spawn.pet_owner_id = ownerid;
	if(ownerid && this->IsNPC())
		this->CastToNPC()->StartTaunting();
	for(int i = 0; i < 9; i++)
		ns->spawn.equipment[i] = equipment[i];
	for(int i = 0; i < 7; i++)
		ns->spawn.equipcolors[i] = equipmentColor[i];
}

void Mob::CreateDespawnPacket(APPLAYER* app)
{
	app->opcode = OP_DeleteSpawn;
	app->size = sizeof(DeleteSpawn_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, app->size);
	DeleteSpawn_Struct* ds = (DeleteSpawn_Struct*)app->pBuffer;
	ds->spawn_id = GetID();
}

//o--------------------------------------------------------------
//| TryWeaponProc; Yeahlight, Dec 6, 2008
//o--------------------------------------------------------------
//| Attemps to proc the effect of the weapon held by the mob
//o--------------------------------------------------------------
void Mob::TryWeaponProc(const Item_Struct* weapon_g, Mob *on, int hand)
{
	bool debugFlag = true;

	//Yeahlight: Weapon is null, proc effect is outside the bounds of the spell records, item is a clicky or the target is dead; abort
	if(!(weapon_g && (weapon_g->common.spell_effect_id < SPDAT_RECORDS + NUMBER_OF_CUSTOM_SPELLS) && (weapon_g->common.effecttype == 0) && on && on->IsAlive()))
		return;

	float procChance = 0.00;
	float hitsPerRound = 0.00;
	float dexBonus = 0.00;
	sint16 weaponDelay = 0;

	//Yeahlight: Evaluate primary weapon's proc chance
	if(hand == 13)
	{
		weaponDelay = this->attack_timer->GetTimerTime();
		if(weaponDelay < 1)
			weaponDelay = 1;
		hitsPerRound = MAIN_HAND_PROC_INTERVAL / (float)weaponDelay;
		if(hitsPerRound < 1)
			hitsPerRound = 1;
		if(GetDEX() > 75)
			dexBonus = (float)(GetDEX() - 75) / 1125.000f;
		procChance = (1.000f + dexBonus) / hitsPerRound;
		if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
			this->Message(LIGHTEN_BLUE, "Debug: Main-hand weapon's proc chance: %f", procChance);
		if(debugFlag && on->IsClient() && on->CastToClient()->GetDebugMe())
			on->Message(LIGHTEN_BLUE, "Debug: %s's main-hand weapon's proc chance: %f", this->GetName(), procChance);
	}
	//Yeahlight: Evaluate secondary weapon's proc chance
	else
	{
		weaponDelay = this->attack_timer_dw->GetTimerTime();
		if(weaponDelay < 1)
			weaponDelay = 1;
		hitsPerRound = OFF_HAND_PROC_INTERVAL / (float)weaponDelay;
		if(hitsPerRound < 1)
			hitsPerRound = 1;
		if(GetDEX() > 75)
			dexBonus = (float)(GetDEX() - 75) / 1125.000f;
		procChance = (1.000f + dexBonus) / hitsPerRound;
		if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
			this->Message(LIGHTEN_BLUE, "Debug: Off-hand weapon's proc chance: %f", procChance);
		if(debugFlag && on->IsClient() && on->CastToClient()->GetDebugMe())
			on->Message(LIGHTEN_BLUE, "Debug: %s's off-hand weapon's proc chance: %f", this->GetName(), procChance);
	}

	//Yeahlight: Chance for the weapon to proc
	if(MakeRandomFloat(0.000, 1.000) < procChance)
	{
		//Yeahlight: PC is under the item's level requirement
		if(this->IsClient() && weapon_g->common.level > this->GetLevel())
		{
			this->Message(RED, "You lack sufficient will to command this weapon");
			return;
		}

		//Yeahlight: Load spell
		Spell* spell = spells_handler.GetSpellPtr(weapon_g->common.spell_effect_id);

		//Yeahlight: Spell does not exist
		if(!spell)
			return;

		//Yeahlight: Record location for spell casting
		this->casting_spell_x_location = GetX();
		this->casting_spell_y_location = GetY();

		//Yeahlight: Weapon has a beneficial buff, so cast it on self
		//           TODO: There are exceptions to this, e.g. detrimental spells on target which should be on self
		if(spell->IsBeneficialSpell())
			this->SpellFinished(spell, this->GetID(), SLOT_ITEMSPELL, 0 , true);
		else
			this->SpellFinished(spell, on->GetID(), SLOT_ITEMSPELL, 0 , true);
	}
}

//o--------------------------------------------------------------
//| TryBonusProc; Yeahlight, Feb 18, 2009
//o--------------------------------------------------------------
//| Attemps to proc the buff's proc effect on the mob
//| See: Spell: Boon of the Garou
//o--------------------------------------------------------------
void Mob::TryBonusProc(Spell* spell, Mob *on)
{
	bool debugFlag = true;

	//Yeahlight: Bonus proc spell is null, target is null or the target is dead; abort
	if(!(spell && on && on->IsAlive()))
		return;

	float procChance = 0.00;
	float hitsPerRound = 0.00;
	float dexBonus = 0.00;
	sint16 weaponDelay = 0;

	weaponDelay = this->attack_timer->GetTimerTime();
	if(weaponDelay < 1)
		weaponDelay = 1;
	hitsPerRound = MAIN_HAND_PROC_INTERVAL / (float)weaponDelay;
	if(hitsPerRound < 1)
		hitsPerRound = 1;
	if(GetDEX() > 75)
		dexBonus = (float)(GetDEX() - 75) / 1125.000f;
	procChance = (1.000f + dexBonus) / hitsPerRound;
	if(debugFlag && this->IsClient() && this->CastToClient()->GetDebugMe())
		this->Message(LIGHTEN_BLUE, "Debug: Spell effect's proc chance: %f", procChance);
	if(debugFlag && on->IsClient() && on->CastToClient()->GetDebugMe())
		on->Message(LIGHTEN_BLUE, "Debug: %s's spell effect's proc chance: %f", this->GetName(), procChance);

	//Yeahlight: Chance for the buff's proc effect to trigger
	if(MakeRandomFloat(0.000, 1.000) < procChance)
	{
		//Yeahlight: Record location for spell casting
		this->casting_spell_x_location = GetX();
		this->casting_spell_y_location = GetY();

		//Yeahlight: Weapon has a beneficial buff, so cast it on self
		//           TODO: There are exceptions to this, e.g. detrimental spells on target which should be on self
		if(spell->IsBeneficialSpell())
			this->SpellFinished(spell, this->GetID(), SLOT_ITEMSPELL, 0, true);
		else
			this->SpellFinished(spell, on->GetID(), SLOT_ITEMSPELL, 0, true);
	}
}

int32 Mob::GetWeaponDamage(int8 Skill, const Item_Struct* Weapon, Mob* Target)
{
	int32 weapon_damage = 0;
	if(Weapon) 
	{
		weapon_damage = (int)Weapon->common.damage;
		if(weapon_damage < 1) 
			weapon_damage = 1;
	}
	else if(Skill == HAND_TO_HAND)
	{
		int tmpClass = this->GetClass();
		if(tmpClass == MONK)
			weapon_damage = this->GetMonkHandToHandDamage();
		else
			weapon_damage = 2;
	}

	return weapon_damage;
}

void Mob::CreateHPPacket(APPLAYER* app)
{
	app->opcode = OP_HPUpdate;
	app->size = sizeof(SpawnHPUpdate_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, sizeof(SpawnHPUpdate_Struct));
	SpawnHPUpdate_Struct* ds = (SpawnHPUpdate_Struct*)app->pBuffer;
	ds->spawn_id = GetID();
	
	if (IsNPC())
	{
		ds->cur_hp = cur_hp;
		ds->max_hp = max_hp;
	}
	else if (IsClient())
	{
		if (GetHP() > 30000)
		{
			ds->cur_hp = 30000;
		}
		else
		{
			ds->cur_hp = GetHP() - itembonuses.HP; //Tazadar : Client adds the HP item bonuses .Yeah its wierd but its like that , if you are not happy of that tell the old verant devs !!
			//ds->cur_hp = cur_hp;
		}

		if (max_hp > 30000)
		{
			ds->max_hp = 30000;
		}
		else
		{
			ds->max_hp = max_hp - itembonuses.HP;
		}
	}
	else 
	{
		ds->cur_hp = cur_hp;
		ds->max_hp = max_hp;
	}
}

void Mob::SendPosUpdate(bool SendToSelf, sint32 range, bool CheckLoS)
{
	APPLAYER* app = new APPLAYER(OP_MobUpdate,sizeof(SpawnPositionUpdates_Struct) + sizeof(SpawnPositionUpdate_Struct));
	SpawnPositionUpdates_Struct* spu = (SpawnPositionUpdates_Struct*)app->pBuffer;
	
	spu->num_updates = 1;
	MakeSpawnUpdate(&spu->spawn_update[0]);

	if (range > 0)
	{
		entity_list.QueueCloseClients(this, app, !SendToSelf, range, 0, CheckLoS);
	}
	else
		entity_list.QueueClients(this, app, !SendToSelf);
	safe_delete(app);//delete app;
}

void Mob::MakeSpawnUpdate(SpawnPositionUpdate_Struct* spu) {
	memset(spu, 0, sizeof(SpawnPositionUpdate_Struct));

	spu->spawn_id = this->id;
	spu->anim_type = animation;
	spu->heading = heading;
	//Yeahlight: Exception for spin stun spells
	if(this->IsNPC() && this->CastToNPC()->GetSpin())
		spu->delta_heading = 100;
	else
		spu->delta_heading = delta_heading;
	spu->x_pos = (sint16) x_pos;
	spu->y_pos = (sint16) y_pos;
	if(this->IsClient())
		spu->z_pos = (sint16) z_pos;  //Yeahlight: This formula is perfect, do not touch! Ok, I lied; NOW it's perfect!
	else
		spu->z_pos = (sint16) z_pos * 10;  //Yeahlight: This is pretty close to perfect
	spu->delta_y = delta_y/125;  //Yeahlight: These magic numbers seem to do the trick
	spu->delta_x = delta_x/125;
	spu->delta_z = delta_z/40; //Yeahlight: TODO: This is very wrong
}

float Mob::Dist(Mob* other)
{
	//if(this->IsClient() && this != other)
	//	return sqrt((double)(pow(other->x_pos-GetX(), 2) + pow(other->y_pos-GetY(), 2) + pow(other->z_pos-GetZ(), 2)));
	//else

	float x_cord_this = this->GetX();
	float y_cord_this = this->GetY();
	float z_cord_this = this->GetZ();
	float x_cord_that = other->GetX();
	float y_cord_that = other->GetY();
	float z_cord_that = other->GetZ();
	
	return sqrt((double)(pow(x_cord_that - x_cord_this, 2) + pow(y_cord_that - y_cord_this, 2) + pow(z_cord_that - z_cord_this, 2)));
//	return pow((other->x_pos-x_pos)*(other->x_pos-x_pos)+(other->y_pos-y_pos)*(other->y_pos-y_pos)+(other->z_pos-z_pos)*(other->z_pos-z_pos), 1/3);
}

float Mob::DistanceToLocation(float x, float y, float z)
{
	//if(this->IsClient() && this != other)
	//	return sqrt((double)(pow(other->x_pos-GetX(), 2) + pow(other->y_pos-GetY(), 2) + pow(other->z_pos-GetZ(), 2)));
	//else

	float x_cord_this = this->GetX();
	float y_cord_this = this->GetY();
	float z_cord_this = this->GetZ();
	float x_cord_that = x;
	float y_cord_that = y;
	float z_cord_that = z;
	
	return sqrt((double)(pow(x_cord_that - x_cord_this, 2) + pow(y_cord_that - y_cord_this, 2) + pow(z_cord_that - z_cord_this, 2)));
//	return pow((other->x_pos-x_pos)*(other->x_pos-x_pos)+(other->y_pos-y_pos)*(other->y_pos-y_pos)+(other->z_pos-z_pos)*(other->z_pos-z_pos), 1/3);
}

float Mob::DistanceToLocation(float x, float y)
{
	float x_cord_this = this->GetX();
	float y_cord_this = this->GetY();
	float x_cord_that = x;
	float y_cord_that = y;

	return sqrt((double)pow(x_cord_that - x_cord_this, 2) + pow(y_cord_that - y_cord_this, 2));
//	return pow((other->x_pos-x_pos)*(other->x_pos-x_pos)+(other->y_pos-y_pos)*(other->y_pos-y_pos)+(other->z_pos-z_pos)*(other->z_pos-z_pos), 1/3);
}

float Mob::DistNoZ(Mob* other)
{
	float x_cord_this = this->GetX();
	float y_cord_this = this->GetY();
	float x_cord_that = other->GetX();
	float y_cord_that = other->GetY();

	return sqrt((double)pow(x_cord_that - x_cord_this, 2) + pow(y_cord_that - y_cord_this, 2));
//	return pow((other->x_pos-x_pos)*(other->x_pos-x_pos)+(other->y_pos-y_pos)*(other->y_pos-y_pos)+(other->z_pos-z_pos)*(other->z_pos-z_pos), 1/3);
}

float Mob::DistNoRoot(Mob* other)
{
	//if(this->IsClient() && this != other)
	//	return pow(other->x_pos-GetX(), 2) + pow(other->y_pos-GetY(), 2) + pow(other->z_pos-GetZ(), 2);
	//else

	float x_cord_this = this->GetX();
	float y_cord_this = this->GetY();
	float z_cord_this = this->GetZ();
	float x_cord_that = other->GetX();
	float y_cord_that = other->GetY();
	float z_cord_that = other->GetZ();

	return pow(x_cord_that - x_cord_this, 2) + pow(y_cord_that - y_cord_this, 2) + pow(z_cord_that - z_cord_this, 2);
//	return (other->x_pos-x_pos)*(other->x_pos-x_pos)+(other->y_pos-y_pos)*(other->y_pos-y_pos)+(other->z_pos-z_pos)*(other->z_pos-z_pos);
}

float Mob::DistNoRootNoZ(Mob* other)
{
	float x_cord_this = this->GetX();
	float y_cord_this = this->GetY();
	float x_cord_that = other->GetX();
	float y_cord_that = other->GetY();

	return (x_cord_that - x_cord_this)*(x_cord_that - x_cord_this)+(y_cord_that - y_cord_this)*(y_cord_that - y_cord_this);
} 

sint32 Mob::SetHP(sint32 hp)
{
	if (hp >= max_hp)
		cur_hp = max_hp;
	else
		cur_hp = hp;
	return cur_hp;
}

void Mob::ShowStats(Client* client) {
	client->Message(BLACK, "Name: %s %s", GetName(), Surname);
	client->Message(BLACK, "  Level: %i  Class: %i  MaxHP: %i  CurHP: %i  AC: %i  EvasionAC: %i  MitigationAC: %i", GetLevel(), (int)GetClass(), GetMaxHP(), GetHP(), CalculateACBonuses(), GetAvoidanceAC(), GetMitigationAC());
	int16 efficiencyATK = 0;
	int16 accuracyATK = 0;
	int16 attackerSTRBonus = 0;
	int16 skillinuse = 0;
	if ((int)GetEquipment(7) != 0) 
	{
		// 1h Slashing weapons
		if ((int)GetEquipment(7) == 1 || (int)GetEquipment(7) == 3)
		{
			skillinuse = 0;
		}
		// 2h Slashing weapons
		else if ((int)GetEquipment(7) == 2)
		{
			skillinuse = 1;
		}
		// Piercing
		else if ((int)GetEquipment(7) == 5)
		{
			skillinuse = 2;
		}
		// 1h Blunt
		else if ((int)GetEquipment(7) == 7)
		{
			skillinuse = 3;
		}
		// 2h Blunt
		else if ((int)GetEquipment(7) == 8)
		{
			skillinuse = 4;
		}
		else 
		{
			skillinuse = 80;
		}
	}
	else 
	{
		skillinuse = 28;
	}
	if(GetSTR() > 75)
		attackerSTRBonus = GetSTR() - 75;
	else
		attackerSTRBonus = 0;
	if(this->IsClient())
	{
		efficiencyATK = (this->CastToClient()->GetSkill(OFFENSE) * 4 / 3) + (attackerSTRBonus * 9 / 10) + 5;
		accuracyATK = (this->CastToClient()->GetSkill(skillinuse) * 2.70f) + 5;
	}
	else
	{
		efficiencyATK = (this->GetSkill(OFFENSE) * 4 / 3) + (attackerSTRBonus * 9 / 10) + 5;
		accuracyATK = (this->GetSkill(skillinuse) * 2.70f) + 5;
	}
	client->Message(BLACK, "  MaxMana: %i  CurMana: %i  Size: %1.1f  ATK: %i  EfficiencyATK: %i  AccuracyATK: %i", GetMaxMana(), GetMana(), GetSize(), GetATK(), efficiencyATK, accuracyATK);
	client->Message(BLACK, "  STR: %i  STA: %i  DEX: %i  AGI: %i  INT: %i  WIS: %i  CHA: %i", GetSTR(), GetSTA(), GetDEX(), GetAGI(), GetINT(), GetWIS(), GetCHA());
	client->Message(BLACK, "  SVM: %i  SVC: %i  SVF: %i  SVD: %i  SVP: %i", GetMR(), GetCR(), GetFR(), GetDR(), GetPR());
	client->Message(BLACK, "  Race: %i  BaseRace: %i  Texture: %i  HelmTexture: %i  Gender: %i  BaseGender: %i", GetRace(), GetBaseRace(), GetTexture(), GetHelmTexture(), GetGender(), GetBaseGender());
	if (client->Admin() >= 100) {
		if (this->IsClient()) {
			client->Message(BLACK, "  EntityID: %i  CharID: %i  PetID: %i", this->GetID(), this->CastToClient()->CharacterID(), this->GetPetID());
		}
		else if (this->IsCorpse()) {
			if (this->IsPlayerCorpse()) {
				client->Message(BLACK, "  EntityID: %i  CharID: %i  PlayerCorpse: %i", this->GetID(), this->CastToCorpse()->GetCharID(), this->CastToCorpse()->GetDBID());
			}
			else {
				client->Message(BLACK, "  EntityID: %i  NPCCorpse", this->GetID());
			}
		}
		else if (this->IsNPC()) {
			client->Message(BLACK, "  EntityID: %i  NPCID: %i  LootTable: %i  PetID: %i  OwnerID: %i", this->GetID(), this->CastToNPC()->GetNPCTypeID(), this->CastToNPC()->GetLoottableID(), this->GetPetID(), this->GetOwnerID());
		}
	}
	if(this->IsNPC())
		client->Message(BLACK, "  MaxHit: %i  MinHit: %i  BlockSkill: %i", this->CastToNPC()->GetMaxDmg(), this->CastToNPC()->GetMinDmg(), this->GetSkill(SKILL_BLOCK));
}
void Mob::DoAttackAnim(Item_Struct* AttackerWeapon, int8& SkillInUse, int8& AttackSkill, int Hand){
	int8 AnimationType = anim1HWeapon;
	if (AttackerWeapon && AttackerWeapon->flag != 0x7669) {
		if (AttackerWeapon->common.itemType == SKILLTYPE_1HSLASHING){
			SkillInUse = _1H_SLASHING;
			AttackSkill = 0x01;
			AnimationType = anim1HWeapon;
		}
		if (AttackerWeapon->common.itemType == SKILLTYPE_2HSLASHING) {
			SkillInUse = _2H_SLASHING;
			AttackSkill = 0x01;
			AnimationType = anim2HSlashing;
		}
		if (AttackerWeapon->common.itemType == SKILLTYPE_PIERCING) {
			SkillInUse = PIERCING;
			AttackSkill = 0x24;
			AnimationType = animPiercing;
		}
		if (AttackerWeapon->common.itemType == SKILLTYPE_1HBLUNT) {
			SkillInUse = _1H_BLUNT;
			AttackSkill = 0x00;
			AnimationType = anim1HWeapon;
		}
		if (AttackerWeapon->common.itemType == SKILLTYPE_2HBLUNT) {
			SkillInUse = _2H_BLUNT;
			AttackSkill = 0x00;
			AnimationType = anim2HWeapon;
		}
		if (AttackerWeapon->common.itemType == SKILLTYPE_2HPIERCING) {
			SkillInUse = PIERCING;
			AttackSkill = 0x24;
			AnimationType = anim2HWeapon;
		}
	}
	else {
		SkillInUse = HAND_TO_HAND;
		AttackSkill = 0x04;
		AnimationType = 8;
	}
	if(Hand == 14) //Dual Wield
		AnimationType = animDualWield;
	this->DoAnim(AnimationType);
}

void Mob::DoAnim(const int8 animnum) 
{
	APPLAYER* outapp = new APPLAYER(OP_Attack, sizeof(Attack_Struct));
	memset(outapp->pBuffer, 0, outapp->size);
	Attack_Struct* AttackStruct = (Attack_Struct*)outapp->pBuffer;
	AttackStruct->spawn_id = GetID();
	AttackStruct->type = animnum;


	AttackStruct->a_unknown2[5] = 0x80;
	AttackStruct->a_unknown2[6] = 0x3f;



	entity_list.QueueCloseClients(this, outapp);
	safe_delete(outapp);
}

void Mob::ShowBuffs(Client* client) {
	if (!spells_handler.SpellsLoaded())
		return;
	client->Message(BLACK, "Buffs on: %s", this->GetName());
	for (int i=0; i < 15; i++) {
		if (buffs[i].spell && buffs[i].spell->IsValidSpell()) {
			const char* name = buffs[i].spell->GetSpellName();
			if (buffs[i].durationformula == BDF_Permanent)
				client->Message(BLACK, "  %i: %s: Permanent", i, name);
			else
				client->Message(BLACK, "  %i: %s: %i tics left, formula: %i", i, name, buffs[i].ticsremaining, (int)buffs[i].durationformula);
		}
	}
}

sint32 Mob::SetMana(sint32 amount)
{
	if (amount < 0)
	{
		this->cur_mana = 0;
	}
	else if (amount > this->GetMaxMana())
	{
		this->cur_mana = this->GetMaxMana();
	}
	else
	{
		this->cur_mana = amount;
	}

	return cur_mana;
}

void Mob::GMMove(float x, float y, float z, float heading) {
	this->x_pos = x;
	this->y_pos = y;
	this->z_pos = z;
	if (heading != 0.01)
		this->heading = heading;
	SendPosUpdate(true);
}

void Mob::SendIllusionPacket(int16 in_race, int16 in_gender, int16 in_texture, int16 in_helmtexture) {
	if (in_race == 0) {
		this->race = GetBaseRace();
		if (in_gender == 0xFFFF)
			this->gender = GetBaseGender();
		else
			this->gender = in_gender;
	}
	else {
		this->race = in_race;
		if (in_gender == 0xFFFF) {
			int8 tmp = Mob::GetDefaultGender(this->race, gender);
			if (tmp == 2)
				gender = 2;
			else if (gender == 2 && GetBaseGender() == 2)
				gender = tmp;
			else if (gender == 2)
				gender = GetBaseGender();
		}
		else
			gender = in_gender;
	}
	if (in_texture == 0xFFFF) {
		if ((race == 0 || race > 12) && race != 128 && race != 130) {
			if (GetTexture() == 0xFFFF)
				this->texture = 0;
		}
		else if (in_race == 0)
			this->texture = 0xFFFF;
	}
	else if (in_texture != 0xFFFF || this->IsClient() || this->IsPlayerCorpse()) {
		this->texture = in_texture;
	}
	else
		this->texture = 0;
	if (in_helmtexture == 0xFFFF) {
		if (in_texture != 0xFFFF)
			this->helmtexture = this->texture;
		else if ((race == 0 || race > 12) && race != 128 && race != 130) {
			if (GetHelmTexture() == 0xFFFF)
				this->helmtexture = 0;
		}
		else if (in_race == 0)
			this->helmtexture = 0xFFFF;
		else
			this->helmtexture = 0;
	}
	else if (in_helmtexture != 0xFFFF || this->IsClient() || this->IsPlayerCorpse()) {
		this->helmtexture = in_helmtexture;
	}
	else
		this->helmtexture = 0;

	APPLAYER* outapp = new APPLAYER(OP_Illusion, sizeof(Illusion_Struct));
	Illusion_Struct* is = (Illusion_Struct*) outapp->pBuffer;
	//is->spawnid = GetID();
	is->gender = this->gender;
	is->race = this->race;
	is->texture = this->texture;
	is->helm = this->helmtexture;
	strcpy(is->name, this->GetName());
	strcpy(is->target, this->GetName());
	//strcpy(is->target, this->target->GetName());
	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);//delete outapp;
}

int8 Mob::GetDefaultGender(int16 in_race, int16 in_gender) 
{
	
	// Checks for male/female combo races and then
	// applies the race from your player profile
	// packet if a a combo race is found - Wizzel.

	if ((in_race >= HUMAN && in_race <= GNOME ) || 
		in_race == IKSAR || 
		in_race == VAHSHIR ||
		in_race == BROWNIE || 
		in_race == LION || 
		in_race == DRACNID || 
		in_race == ZOMBIE || 
		in_race == ELF_VAMPIRE || 
		in_race == ERUDITE_GHOST) 
	{
		// No change in gender.
		return in_gender;
	}

	// Checks for default male races and
	// sets the gender to male.

	else if (in_race == FREEPORT_GUARDS || 
		in_race == MIMIC || 
		in_race == HUMAN_BEGGAR || 
		in_race == VAMPIRE || 
		in_race == HIGHPASS_CITIZEN || 
		in_race == CLOCKWORK_GNOME || 
		in_race == GHOST_DWARF || 
		in_race == INVISIBLE_MAN ||
		in_race == NERIAK_CITIZEN || 
		in_race == ERUDIN_CITIZEN || 
		in_race == RIVERVALE_CITIZEN || 
		in_race == HALAS_CITIZEN || 
		in_race == GROBB_CITIZEN || 
		in_race == OGGOK_CITIZEN || 
		in_race == KALADIM_CITIZEN || 
		in_race == FELGUARD || 
		in_race == FAYGUARD)
	{
		// Set race to male
		return 0;
	}

	// Checks for default female races and
	// sets the gender to female.

	else if (in_race == FAIRY ||
		in_race == PIXIE ||
		in_race == KERRA) 
	{
		// Set race to female
		return 1;
	}

	// All other races are set to default gender of 2.

	else
	{
		// Neutral default for NPC Races
		return 2;
	}
}

void Mob::SendAppearancePacket(int16 Spawn_ID, TSpawnAppearanceType Type, int32 Parameter, bool WholeZone)
{
	APPLAYER* SpawnPacket = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
	SpawnAppearance_Struct* appearance = (SpawnAppearance_Struct*)SpawnPacket->pBuffer;
	appearance->spawn_id = Spawn_ID;
	appearance->type = Type;
	appearance->parameter = Parameter;

	if(WholeZone)
		entity_list.QueueClients(this, SpawnPacket, false);
	else if(this->IsClient())
		this->CastToClient()->QueuePacket(SpawnPacket);

	safe_delete(SpawnPacket);
}

void Mob::ChangeSize(float in_size = 0)
{
	this->size = in_size;
	this->SendAppearancePacket(this->GetID(), SAT_Size, (int32)in_size, true);
}

Mob* Mob::GetPet() {
	Mob* tmp = entity_list.GetMob(this->GetPetID());
	if (tmp) {
		if (tmp->GetOwnerID() == this->GetID()) {
			return tmp;
		}
		else {
			this->SetPetID(0);
			return 0;
		}
	}
	return 0;
}

void Mob::SetPet(Mob* newpet) {
	Mob* oldpet = GetPet();
	if (oldpet) {
		oldpet->SetOwnerID(0);
	}
	if (newpet == 0) {
		SetPetID(0);
	}
	else {
		SetPetID(newpet->GetID());
		Mob* oldowner = entity_list.GetMob(newpet->GetOwnerID());
		if (oldowner)
			oldowner->SetPetID(0);
		newpet->SetOwnerID(this->GetID());
	}
}

// Harakiri get the ower of this mob/pet
Mob* Mob::GetOwnerOrSelf() {
	if (!GetOwnerID())
		return this;
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (!owner) {
		SetOwnerID(0);
		return(this);
	}
	if (owner->GetPetID() == this->GetID()) {
		return owner;
	}

	SetOwnerID(0);
	return this;
}

Mob* Mob::GetOwner() {
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (owner && owner->GetPetID() == this->GetID()) {
		return owner;
	}
	this->SetOwnerID(0, false);
	return 0;
}

void Mob::SetOwnerID(int16 NewOwnerID, bool despawn) {
	ownerid = NewOwnerID;
	if(ownerid == 0 && this->IsNPC() && despawn)
		this->Depop();
}

//heko: for backstab
bool Mob::BehindMob(Mob* other, float playerx, float playery) {
	//see if player is behind mob
	float angle, lengthb, vectorx, vectory;
	float mobx = -(other->GetX());	// mob xlocation (inverse because eq is confused)
	float moby = other->GetY();		// mobylocation
	float heading = (int8)other->GetHeading();	// mob heading
	heading = (heading * 360.0)/256.0;	// convert to degrees
	if (heading < 270)
		heading += 90;
	else
		heading -= 270;
	heading = heading*3.1415/180.0;	// convert to radians
	vectorx = mobx + (10.0 * cosf(heading));	// create a vector based on heading
	vectory = moby + (10.0 * sinf(heading));	// of mob length 10
	
	//length of mob to player vector
	lengthb = (float)sqrt(pow((-playerx-mobx),2) + pow((playery-moby),2));

	// calculate dot product to get angle
	angle = acosf(((vectorx-mobx)*(-playerx-mobx)+(vectory-moby)*(playery-moby)) / (10 * lengthb));
	angle = angle * 180 / 3.1415;
	if (angle > 90.0) //not sure what value to use (90*2=180 degrees is front)
		return true;
	else
		return false;
}

void Mob::Kill() {
	Death(this, 0);
}

int32 Mob::GetMonkHandToHandDamage(void)
{
	// Kaiyodo - Determine a monk's fist damage. Table data from www.monkly-business.com
	// saved as static array - this should speed this function up considerably
	static int damage[66] = {
	//   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
        99, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,
         8, 8, 8, 8, 8, 9, 9, 9, 9, 9,10,10,10,10,10,11,11,11,11,11,
        12,12,12,12,12,13,13,13,13,13,14,14,14,14,14,14,14,14,14,14,
        15,15,16,16,17,18 };
	
	// Have a look to see if we have epic fists on
	int Level = GetLevel();
    if (Level > 65)
	    return(19);
    else
        return damage[Level];
}

int32 Mob::GetMonkHandToHandDelay(void)
{
	// Kaiyodo - Determine a monk's fist delay. Table data from www.monkly-business.com
	// saved as static array - this should speed this function up considerably
	static int delayshuman[66] = {
	//  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
        99,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,
        36,36,36,36,36,35,35,35,35,35,34,34,34,34,34,33,33,33,33,33,
        32,32,32,32,32,31,31,31,31,31,30,30,30,29,29,29,28,28,28,27,
        27,26,26,25,25,25 };
	static int delaysiksar[66] = {
	//  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
        99,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,
        36,36,36,36,36,36,36,36,36,36,35,35,35,35,35,34,34,34,34,34,
        33,33,33,33,33,32,32,32,32,32,31,31,31,30,30,30,29,29,29,28,
        28,27,27,26,26,26 };
	
	// Have a look to see if we have epic fists on
	int Level = GetLevel();
	if (GetRace() == HUMAN)
	{
        if (Level > 65)
		    return(24);
        else
            return delayshuman[Level];
	}
	else	//heko: iksar table
	{
        if (Level > 65)
		    return(25);
        else
            return delaysiksar[Level];
	}
}

int32 Mob::GetWeaponDamageBonus(Item_Struct *Weapon)
{
	// Kaiyodo - Calculate the damage bonus for a weapon on the main hand

	int8 tmplevel = GetLevel();

	if(tmplevel < 28)
	{
		return(0);
	}

	// Check we're on of the classes that gets a damage bonus
	switch(GetClass())
	{
	case WARRIOR:
	case MONK:
	case ROGUE:
	case PALADIN:
	case SHADOWKNIGHT:
	case BEASTLORD:
	case BARD:
		break;
	default:
		return(0);
	}

	int BasicBonus = ((tmplevel - 25) / 3) + 1;

	// If we have no weapon, or only a single handed weapon, just return the default
	// damage bonus of (Level - 25) / 3
	if(!Weapon || Weapon->common.itemType == ItemType1HS || Weapon->common.itemType == ItemTypePierce || Weapon->common.itemType == ItemType1HB)
	{
		return(BasicBonus);
	}

	// Things get more complicated with 2 handers, the bonus is based on the delay of
	// the weapon as well as a number stored inside the weapon.
	int WeaponBonus = 0;	// How do you find this out?

	// Data for this, again, from www.monkly-business.com
	if(Weapon->common.delay <= 27)
	{
		return(WeaponBonus + BasicBonus + 1);
	}

	if(Weapon->common.delay <= 39)
	{
		return(WeaponBonus + BasicBonus + ((tmplevel-27) / 4));
	}

	if(Weapon->common.delay <= 42)
	{
		return(WeaponBonus + BasicBonus + ((tmplevel-27) / 4) + 1);
	}
	// Weapon must be > 42 delay
	return(WeaponBonus + BasicBonus + ((tmplevel-27) / 4) + ((Weapon->common.delay-34) / 3));
}

//o--------------------------------------------------------------
//| VisibleToMob; Yeahlight, June 25, 2009
//o--------------------------------------------------------------
//| Checks to see if 'other' can see 'this'
//o--------------------------------------------------------------
bool Mob::VisibleToMob(Mob* other)
{
	//Yeahlight: No mob was supplied; abort
	if(other == NULL)
		return false;

	//Yeahlight: Mob can see itself
	if(this == other)
		return true;

	//Yeahlight: Other mob is undead
	if(other->GetBodyType() == BT_Undead)
	{
		//Yeahlight: Undead mob sees through invis versus undead, therefore this NPC will *always* see its target
		if(other->GetSeeInvisibleUndead())
		{
			return true;
		}
		else
		{
			//Yeahlight: This mob is currently invis to undead
			if(GetInvisibleUndead())
			{
				return false;
			}
			else
			{
				return true;
			}
		}
	}
	//Yeahlight: Other mob is an animal (Note: There are no NPCs that see through invis vs animal, but they are animals that see through normal invis!)
	else if(other->GetBodyType() == BT_Animal)
	{
		//Yeahlight: This mob is currently invis to animals OR this mob is a PC and currently sneaking behind it OR this mob is using standard invis and this animal NPC cannot see through it
		if(GetInvisibleAnimal() || ((GetInvisible() || (this->IsClient() && this->CastToClient()->GetSneaking() && BehindMob(other, GetX(), GetY()))) && other->GetSeeInvisible() == false))
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	//Yeahlight: Other mob is not undead or an animal
	else
	{
		//Yeahlight: Regular mob sees through invis, therefore this NPC will *always* see its target
		if(other->GetSeeInvisible())
		{
			return true;
		}
		else
		{
			//Yeahlight: This mob is currently invis or sneaking behind the other mob
			if(GetInvisible() || (this->IsClient() && this->CastToClient()->GetSneaking() && BehindMob(other, GetX(), GetY())))
			{
				return false;
			}
			else
			{
				return true;
			}
		}
	}
}


//o--------------------------------------------------------------
//| GetInvisible; Yeahlight, June 25, 2009
//o--------------------------------------------------------------
//| Returns true if the mob is currently using standard invis
//o--------------------------------------------------------------
bool Mob::GetInvisible()
{
	//Yeahlight: Mob is a PC
	if(this->IsClient())
	{
		//Yeahlight: PC has a standard invis spell on OR is currently using the hide ability
		if(GetSpellInvis() || this->CastToClient()->GetHide())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	//Yeahlight: Mob is an NPC
	else
	{
		//Yeahlight: NPC currently has a standard invis spell on
		if(GetSpellInvis())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

void Mob::SetInvisible(bool toggle)
{
	SendAppearancePacket(GetID(), SAT_Invis, (int32)toggle, true);
	this->invisible = toggle;
}

void Mob::SetInvisibleUndead(bool toggle)
{
	this->invisible_undead = toggle;
}

void Mob::SetInvisibleAnimal(bool toggle)
{
	this->invisible_animal = toggle;
}

//o--------------------------------------------------------------
//| Spin; Yeahlight, Feb 18, 2009
//o--------------------------------------------------------------
//| Spins the mob! Weeeeeeeeeeee!!
//| TODO: Whirl Till you Hurl is more of a mez, it fades on hit
//o--------------------------------------------------------------
void Mob::Spin(Mob* caster, Spell* spell)
{
	//Yeahlight: Victim is a PC
	if(this->IsClient())
	{
		//Yeahlight: Instead of stunning the PC, just lock their UI
		SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Lose_Control, false);
		APPLAYER* app = new APPLAYER(OP_ClientUpdate, sizeof(SpawnPositionUpdate_Struct));
		SpawnPositionUpdate_Struct* spu = (SpawnPositionUpdate_Struct*)app->pBuffer;
		memset(spu, 0, sizeof(SpawnPositionUpdate_Struct));
		spu->x_pos = (sint16)GetX();
		spu->y_pos = (sint16)GetY();
		spu->z_pos = (sint16)(GetZ() * 10);
		spu->heading = (int8)GetHeading();
		spu->delta_heading = 100;
		spu->spawn_id = this->GetID();
		entity_list.QueueCloseClients(this,app);
		safe_delete(app);
	}
	//Yeahlight: Victim is an NPC
	else if(this->IsNPC())
	{
		//Yeahlight: NPC is immune to stun or greater than level 55
		if(this->CastToNPC()->GetCannotBeStunned() || this->GetLevel() > 55)
		{
			if(caster && caster->IsClient())
				caster->Message(RED, "Your target is immune to the stun portion of this effect");
		}
		//Yeahlight: Spin stun will land on this NPC
		else
		{
			//Yeahlight: A spell was not supplied; abort
			if(!spell)
				return;

			//Yeahlight: Set the stun duration for each spin stun type spell
			//			 TODO: Fill out the rest of this switch
			switch(spell->GetSpellID())
			{
				//Yeahlight: Whirl Till You Hurl
				case 303:
					this->CastToNPC()->Stun(3000);
					break;
				//Yeahlight: Spin the Bottle
				case 1101:
					this->CastToNPC()->Stun(12000);
					break;
				//Yeahlight: One Hundred Blows
				case 1809:
					this->CastToNPC()->Stun(6000);
					break;
				default:
					this->CastToNPC()->Stun(1500);
			}
			this->CastToNPC()->SetSpin(true);
			this->SendPosUpdate(false, 200);
		}
	}
}

int32 Mob::RandomTimer(int min,int max) {
    int r = 14000;
	if(min != 0 && max != 0 && min < max)
	{
	    r = (rand()  % (max - min)) + min;
	}
	return r;
}

//o--------------------------------------------------------------
//| GetSpecialFactionCon; Yeahlight, Nov 26, 2008
//o--------------------------------------------------------------
//| Returns a default faction value for NPCs with a primary
//| faction of 0.
//o--------------------------------------------------------------
FACTION_VALUE Mob::GetSpecialFactionCon(Mob* iOther)
{
	switch(iOther->GetRace())
	{
		case 14:case 16:case 17:case 18:case 21:
		case 28:case 29:case 31:case 32:case 33:
		case 37:case 39:case 40:case 41:case 42:
		case 43:case 45:case 46:case 47:case 48:
		case 49:case 50:case 51:case 52:case 53:
		case 54:case 55:case 57:case 58:case 60:
		case 61:case 62:case 63:case 64:case 65:
		case 66:case 68:case 70:case 74:case 75:
		case 76:case 80:case 84:case 85:case 86:
		case 89:case 91:case 96:case 97:case 98:
		case 99:case 100:case 101:case 104:case 105:
		case 109:case 111:case 116:case 117:case 118:
		case 119:case 120:case 121:case 122:case 123:
		case 124:case 125:case 126:case 129:
		case 131:case 132:case 133:case 134:case 135:
		case 136:case 137:case 138:case 140:case 144:
		case 145:case 146:case 147:case 148:case 149:
		case 155:case 156:case 157:case 158:case 159:
		case 160:case 161:case 162:case 163:case 164:
		case 165:case 166:case 168:case 169:case 170:
		case 171:case 172:case 173:case 174:case 175:
		case 178:case 181:case 185:case 186:case 187:
		case 188:
		case 189: return FACTION_SCOWLS; break;
		default: return FACTION_INDIFFERENT; break;
	}
	return FACTION_INDIFFERENT;
}

///////////////////////////////////////////////////
// Cofruben: more secure.
void Mob::SetCastingSpell(Spell* spell, int16 target_id, int16 slot, float casting_x, float casting_y, int16 inventorySlot)
{
	if(spell)
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_SPELL, "Mob::SetCastingSpell(spell_name = %s, slot = %i, target_id = %i)", spell->GetSpellName(), slot, target_id);
	casting_spell = spell;
	casting_spell_targetid = (target_id > 0 && !entity_list.GetID(target_id)) ? 0 : target_id;
	casting_spell_slot = slot;
	casting_spell_inventory_slot = inventorySlot;
	casting_spell_x_location = casting_x;
	casting_spell_y_location = casting_y;
}

///////////////////////////////////////////////////

//o--------------------------------------------------------------
//| GetSeeInvisible; Yeahlight, June 25, 2009
//o--------------------------------------------------------------
//| Returns true if the mob can see through invis
//o--------------------------------------------------------------
bool Mob::GetSeeInvisible()
{
	//Yeahlight: Mob is an NPC
	if(this->IsNPC())
	{
		//Yeahlight: NPC has a see invis buff or can passively see through invis at all times
		if(GetCanSeeThroughInvis() || this->CastToNPC()->GetPassiveCanSeeThroughInvis())
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
		//Yeahlight: PC has a see invis buff
		if(GetCanSeeThroughInvis())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

//o--------------------------------------------------------------
//| GetSeeInvisibleUndead; Yeahlight, June 25, 2009
//o--------------------------------------------------------------
//| Returns true if the mob can see through invis to undead
//o--------------------------------------------------------------
bool Mob::GetSeeInvisibleUndead()
{
	//Yeahlight: Mob is an NPC
	if(this->IsNPC())
	{
		//Yeahlight: NPC has a see invis to undead buff or can passively see through invis to undead at all times
		if(GetCanSeeThroughInvisToUndead() || this->CastToNPC()->GetPassiveCanSeeThroughInvisToUndead())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	//Yeahlight: Mob is a PC and PCs may never see through invis to undead
	else
	{
		return false;
	}
}

/********************************************************************
 *                        Tazadar  - 5/31/08                        *
 ********************************************************************
 *                          SaveGuardSpot()                         *                        
 ********************************************************************
 *  + Save the location of the Guard Spot                           *
 *                                                                  *
 ********************************************************************/
void Mob::SaveGuardSpot() {
		guard_x = x_pos;
		guard_y = y_pos;
		guard_z = z_pos;
		guard_heading = heading;
}

///////////////////////////////////////////////////

//o--------------------------------------------------------------
//| CheckLosFN; Yeahlight, June 11, 2008
//o--------------------------------------------------------------
//| Returns true if line of sight is present between two mobs
//o--------------------------------------------------------------
bool Mob::CheckLosFN(Mob* other)
{	
	if(other == NULL || zone->map == NULL)
		return(false);

	VERTEX myloc;
	VERTEX oloc;
		
	myloc.x = GetX();
	myloc.y = GetY();
	myloc.z = (GetZ() * 10 + (GetSize()==0.0?LOS_DEFAULT_HEIGHT:GetSize())/2 * HEAD_POSITION) / 10;
	
	oloc.x = other->GetX();
	oloc.y = other->GetY();
	oloc.z = other->GetZ() + (other->GetSize()==0.0?LOS_DEFAULT_HEIGHT:other->GetSize())/2 * SEE_POSITION;

	FACE *onhit;
	NodeRef mynode;
	NodeRef onode;
	
	VERTEX hit;
	
	//see if anything in our node is in the way
	mynode = zone->map->SeekNode( zone->map->GetRoot(), myloc.x, myloc.y);
	if(mynode != NODE_NONE) {
		if(zone->map->LineIntersectsNode(mynode, myloc, oloc, &hit, &onhit)) {
			return(false);
		}
	}
	
	//see if they are in a different node.
	//if so, see if anything in their node is blocking me.
	if(! zone->map->LocWithinNode(mynode, oloc.x, oloc.y)) {
		onode = zone->map->SeekNode( zone->map->GetRoot(), oloc.x, oloc.y);
		if(onode != NODE_NONE && onode != mynode) {
			if(zone->map->LineIntersectsNode(onode, myloc, oloc, &hit, &onhit)) {
				return(false);
			}
		}
	}
	return(true);
}

///////////////////////////////////////////////////

void Mob::CalculateNewFearpoint()
{
	int loop = 0;
	float ranx, rany, ranz;
	SetCurfp(false);
	while (loop < 100) //Max 100 tries
	{
		int ran = 250 - (loop*2);
		loop++;
		ranx = GetX()+rand()%ran-rand()%ran;
		rany = GetY()+rand()%ran-rand()%ran;
		ranz = FindGroundZ(ranx,rany);
		if (ranz == -999999)
			continue;
		float fdist = ranz - GetZ();
		if (fdist >= -12 && fdist <= 12 && CheckCoordLosNoZLeaps(GetX(),GetY(),GetZ(),ranx,rany,ranz))
		{
			SetCurfp(true);
			break;
		}
	}
	if (GetCurfp())
	{
		fear_walkto_x = ranx;
		//this->x_dest = ranx;
		fear_walkto_y = rany;
		//this->y_dest = rany;
		fear_walkto_z = ranz;
		//this->z_dest = ranz;
	}
	else //Break fear
	{
		//BuffFadeByEffect(SE_Fear);
	}
	faceDestination(fear_walkto_x, fear_walkto_y);	
	SendPosUpdate(false, NPC_UPDATE_RANGE, false);
}

float Mob::FindGroundZ(float new_x, float new_y, float z_offset)
{
	float ret = -999999;
	if (zone->map != 0)
	{
		NodeRef pnode = zone->map->SeekNode( zone->map->GetRoot(), new_x, new_y );
		if (pnode != NODE_NONE)
		{
			VERTEX me;
			me.x = new_x;
			me.y = new_y;
			me.z = GetZ() + z_offset;
			VERTEX hit;
			FACE *onhit;
			float best_z = zone->map->FindBestZ(pnode, me, &hit, &onhit);
			if (best_z != -999999)
			{
				ret = best_z;
			}
		}
	}
	return ret;
}

float Mob::FindGroundZWithZ(float new_x, float new_y, float new_z, float z_offset)
{
	float ret = -999999;
	if (zone->map != 0)
	{
		NodeRef pnode = zone->map->SeekNode( zone->map->GetRoot(), new_x, new_y );
		if (pnode != NODE_NONE)
		{
			VERTEX me;
			me.x = new_x;
			me.y = new_y;
			me.z = new_z + z_offset;
			VERTEX hit;
			FACE *onhit;
			float best_z = zone->map->FindBestZ(pnode, me, &hit, &onhit);
			if (best_z != -999999)
			{
				ret = best_z;
			}
		}
	}
	return ret;
}

bool Map::LineIntersectsZoneNoZLeaps(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on) {
	float z = -999999;
	VERTEX step;
	VERTEX cur;
	cur.x = start.x;
	cur.y = start.y;
	cur.z = start.z;
	
	step.x = end.x - start.x;
	step.y = end.y - start.y;
	step.z = end.z - start.z;
	float factor = step_mag / sqrt(step.x*step.x + step.y*step.y + step.z*step.z);

	step.x *= factor;
	step.y *= factor;
	step.z *= factor;

	int steps = 0;

	if (step.x > 0 && step.x < 0.001f)
		step.x = 0.001f;
	if (step.y > 0 && step.y < 0.001f)
		step.y = 0.001f;
	if (step.z > 0 && step.z < 0.001f)
		step.z = 0.001f;
	if (step.x < 0 && step.x > -0.001f)
		step.x = -0.001f;
	if (step.y < 0 && step.y > -0.001f)
		step.y = -0.001f;
	if (step.z < 0 && step.z > -0.001f)
		step.z = -0.001f;
	
	NodeRef cnode, lnode;
	lnode = NULL;
	int i = 0;
	//while we are not past end
	//always do this once, even if start == end.
	while(cur.x != end.x || cur.y != end.y || cur.z != end.z)
	{
		steps++;
		cnode = SeekNode(GetRoot(), cur.x, cur.y);
		if (cnode == NODE_NONE)
		{
			return(true);
		}		
		VERTEX me;
		me.x = cur.x;
		me.y = cur.y;
		me.z = cur.z;
		VERTEX hit;
		FACE *onhit;
		float best_z = zone->map->FindBestZ(cnode, me, &hit, &onhit);
		float diff = best_z-z;
		diff *= sign(diff);
		if (z == -999999 || best_z == -999999 || diff < 12.0)
			z = best_z;
		else
			return(true);
		//look at current location
		if(cnode != NODE_NONE && cnode != lnode) {
			if(LineIntersectsNode(cnode, start, end, result, on))
			{
				return(true);
			}
			lnode = cnode;
		}
		
		//move 1 step
		if (cur.x != end.x)
			cur.x += step.x;
		if (cur.y != end.y)
			cur.y += step.y;
		if (cur.z != end.z)
			cur.z += step.z;
		
		//watch for end conditions
		if ( (cur.x > end.x && end.x >= start.x) || (cur.x < end.x && end.x <= start.x) || (step.x == 0) ) {
			cur.x = end.x;
		}
		if ( (cur.y > end.y && end.y >= start.y) || (cur.y < end.y && end.y <= start.y) || (step.y == 0) ) {
			cur.y = end.y;
		}
		if ( (cur.z > end.z && end.z >= start.z) || (cur.z < end.z && end.z < start.z) || (step.z == 0) ) {
			cur.z = end.z;
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	
	//walked entire line and didnt run into anything...
	return(false);
}

void Mob::faceDestination(float x, float y){
	float angle;

	if (x-GetX() > 0)
		angle = - 90 + atan((double)(y-GetY()) / (double)(x-GetX())) * 180 / M_PI;
	else if (x-GetX() < 0)
		angle = + 90 + atan((double)(y-GetY()) / (double)(x-GetX())) * 180 / M_PI;
	else
	{
		if (y-GetY() > 0)
			angle = 0;
		else
			angle = 180;
	}
	if (angle < 0)
		angle += 360;
	if (angle > 360)
		angle -= 360;
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_PATHING, "Client::faceDestination(x = %f, y = %f): angle = %f", x, y, angle);
	heading = (sint8) (256*(360-angle)/360.0f);
}

/********************************************************************
 *                      Pinedepain  - 6/10/08                       *
 ********************************************************************
 *                             Mesmerize                            *                        
 ********************************************************************
 *  Mesmerize this mob. It interrupts the casting process if the    *
 *  mob is casting.                                                 *
 ********************************************************************/
void Mob::Mesmerize(void)
{
	//Pinedepain: If we are casting, it interrupts the casting (but not songs)
	if(this->GetCastingSpell())
		InterruptSpell();

	//Yeahlight: Freeze the PC's UI
	if(IsClient())
		SendAppearancePacket(this->GetID(), SAT_Position_Update, SAPP_Lose_Control, false);

	// Pinedepain // We update the mesmerized var
	mesmerized = true;
}

/********************************************************************
 *                      Pinedepain  - 6/10/08                       *
 ********************************************************************
 *                          EnableSpellBar                          *                        
 ********************************************************************
 *  Enable this mob's spell bar									    *
 ********************************************************************/
void Mob::EnableSpellBar(int16 spell_id)
{
	//Pinedepain // We do this only if we are a client! AI mobs don't need this.
	if(IsClient())
	{
		//Pinedepain // We just send the right packet to notify the mob that his spell bar is enabled
		APPLAYER* outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
		ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
		manachange->new_mana = GetMana();
		manachange->spell_id = spell_id ? spell_id : 1;
		this->CastToClient()->QueuePacket(outapp);
		safe_delete(outapp);//delete outapp;
	}
}

//o--------------------------------------------------------------
//| EnableSpellBarWithManaConsumption; Yeahlight, Jan 23, 2009
//o--------------------------------------------------------------
//| Enables the mob's spell bar after an interruption and after
//| the spell's mana has been taken from the caster's mana pool
//o--------------------------------------------------------------
void Mob::EnableSpellBarWithManaConsumption(Spell* spell)
{
	if(IsClient())
	{
		APPLAYER* outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
		ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
		sint32 newMana = 0;
		//Yeahlight: A spell is supplied
		if(spell)
		{
			newMana = GetMana() - spell->GetSpellManaRequired();
			if(newMana < 0)
				newMana = 0;
			CastToClient()->SetMana(newMana);
		}
		//Yeahlight: No spell supplied, so do not reduce caster's mana
		else
		{
			newMana = GetMana();
		}
		manachange->new_mana = newMana;
		manachange->spell_id = spell ? spell->GetSpellID() : 1;
		this->CastToClient()->QueuePacket(outapp);
		safe_delete(outapp);
	}
}

//o--------------------------------------------------------------
//| findMyPath; Yeahlight, June 30, 2008
//o--------------------------------------------------------------
//| Utilizing the data stored in memory from the directed
//| graph, we use the A* algorithm to find the optimal path
//| between two nodes
//o--------------------------------------------------------------
PATH_STATUS Mob::findMyPath(Node* start, Node* end, bool doNotMove)
{
	bool debugFlag = false;

	//Yeahlight: TODO: If start is the same as end, pass a single path back of the start node

	if(debugFlag){
		cout<<"Start Node's Data"<<endl;
		cout<<"NodeID: "<<start->nodeID<<" Loc: "<<start->x<<", "<<start->y<<", "<<start->z<<", "<<endl;
		cout<<"End Node's Data"<<endl;
		cout<<"NodeID: "<<end->nodeID<<" Loc: "<<end->x<<", "<<end->y<<", "<<end->z<<", "<<endl;
	}

	Node* root = new Node();		//Root of state tree;
	Node* currentNode;				//Holds current node taken from queue;
	Node* tempNode;					//Used to iterate through parent nodes;
	Node* bogusNode = new Node();
	Node tempNode2;
	root = start->Clone();
	root->myParent = NULL;
	root->fValue = sqrt((double)(pow(start->x - end->x, 2) + pow(start->y - end->y, 2) + pow(start->z - end->z, 2)));
	root->pathCost = 0;
	bool solution_found = false;
	Node* solution_node;
	int nodeStack[5000] = {0};
	int nodeStackIterator = 0;
	int16 bailCounter = 0;
	bool bailCondition = false;
	ofstream debugFile;

	//Yeahlight: Clear our previous path, if it exists
	while(!this->CastToNPC()->GetMyPath().empty())
	{
		this->CastToNPC()->GetMyPath().pop();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	Queue* myQ = new Queue();
	int16 nodeStore[100] = {0};
	int16 nodeStoreCounter = 0;
	//Yeahlight: Queue to keep track of our nodes that need deallocated
	queue<Node*> deletion;

	//Yeahlight: Send a "bogus" node to the queue so all other nodes pushed onto the queue have a maximum to reference
	bogusNode->fValue = 999999;
	myQ->add(bogusNode);
	deletion.push(bogusNode);
	
	//Yeahlight Send our start node to the queue
	myQ->add(root);
	deletion.push(root);

	if(debugFlag){
		debugFile.open("pathingDebug.txt");
		debugFile<<"Root "<<endl;
		debugFile<<"NodeID: "<<root->nodeID<<endl;
		debugFile<<"Coords: "<<root->x<<" "<<root->y<<" "<<root->z<<endl;
		debugFile<<"Parent: "<<root->myParent<<endl;
		debugFile<<"G(x): "<<root->pathCost<<endl;
		debugFile<<"F(x): "<<root->fValue<<endl;
		debugFile<<"************************"<<endl;
	}

	while(myQ->getSize() > 0 && !solution_found){
		//Removes node with lowest f(n) from queue
		currentNode = myQ->remove();

		//Yeahlight: If the server has spent too much time in this loop, bail and just teleport the NPC to the player
		if(bailCounter++ > 200 && !doNotMove)
		{
			bailCondition = true;
			solution_found = false;
			break;
		}

		if(debugFlag){
			debugFile<<"   Node "<<endl;
			debugFile<<"   NodeID: "<<currentNode->nodeID<<endl;
			debugFile<<"   Coords: "<<currentNode->x<<" "<<currentNode->y<<" "<<currentNode->z<<endl;
			debugFile<<"   Parent: "<<currentNode->myParent<<endl;
			debugFile<<"   G(x): "<<currentNode->pathCost<<endl;
			debugFile<<"   F(x): "<<currentNode->fValue<<endl;
			debugFile<<"   ************************"<<endl;
			nodeStack[nodeStackIterator] = currentNode->nodeID;
			nodeStackIterator++;
		}
 
		if((solution_found == false) || (currentNode->fValue < solution_node->fValue)){
			//Always goal test before expanding current node
			if(currentNode->x == end->x && currentNode->y == end->y && currentNode->z == end->z){
				solution_found = true;
				solution_node = currentNode;
			}else{
				//Grab current node's children
				for(int i = 0; i < currentNode->childCount; i++){
					//Yeahlight: Create a new node to be passed to the queue
					Node* tempChild = zone->thisZonesNodes[currentNode->myChildren[i].nodeID]->Clone();
					//f(x) = g(x) + h(x), where g(x) is distance already traveled to CURRENT point from START & h(x) is estimated distance to the END from CURRENT point
					//h(x) below
					float tempDistance = sqrt((double)(pow(tempChild->x - end->x, 2) + pow(tempChild->y - end->y, 2) + pow(tempChild->z - end->z, 2)));
					//g(x) below
					tempChild->pathCost = currentNode->pathCost + currentNode->myChildren[i].cost;
					//f(x) below
					tempChild->fValue = tempDistance + tempChild->pathCost;
					tempChild->myParent = currentNode;
					if(debugFlag){
						debugFile<<"      Child Node"<<endl;
						debugFile<<"      NodeID: "<<tempChild->nodeID<<endl;
						debugFile<<"      Coords: "<<tempChild->x<<" "<<tempChild->y<<" "<<tempChild->z<<endl;
						debugFile<<"      Parent: "<<tempChild->myParent<<endl;
						debugFile<<"      G(x): "<<tempChild->pathCost<<endl;
						debugFile<<"      F(x): "<<tempChild->fValue<<endl;
						debugFile<<"      ************************"<<endl;
					}
					myQ->add(tempChild);
					deletion.push(tempChild);
				}
			}
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	if(debugFlag){
		if(solution_found){
			debugFile<<"Destination "<<endl;
			debugFile<<"NodeID: "<<solution_node->nodeID<<endl;
			debugFile<<"Coords: "<<solution_node->x<<" "<<solution_node->y<<" "<<solution_node->z<<endl;
			debugFile<<"Parent: "<<solution_node->myParent<<endl;
			debugFile<<"G(x): "<<solution_node->pathCost<<endl;
			debugFile<<"F(x): "<<solution_node->fValue<<endl;
			debugFile<<"************************"<<endl;
			cout<<"Final Node Info"<<endl;
			cout<<"Parent: "<<solution_node->myParent<<endl;
			cout<<"fValue: "<<solution_node->fValue<<endl;
			cout<<"PathCost: "<<solution_node->pathCost<<endl;
		}
		debugFile.close();

		cout<<"Complete Node Stack: "<<endl;
		for(int i = 0; i < nodeStackIterator; i++)
			cout<<nodeStack[i]<<" ";
		cout<<endl;
	}

	if(!solution_found){
		if(debugFlag)
			cout<<"No possible path..."<<endl;
		this->CastToNPC()->SetOnPath(false);
	}else{
		if(debugFlag)
			cout<<"Path found!"<<endl;
		tempNode = solution_node;
		//Yeahlight: Follow the "parent" pointers starting from the solution node back to the start node to get our optimal path
		int16 tempParent = tempNode->myParent->nodeID;
		nodeStore[0] = solution_node->nodeID;
		nodeStore[1] = tempParent;
		nodeStoreCounter = 1;
		while(tempParent != start->nodeID)
		{
			nodeStoreCounter++;
			tempNode = tempNode->myParent;
			tempParent = tempNode->myParent->nodeID;
			nodeStore[nodeStoreCounter] = tempParent;
			//Yeahlight: Zone freeze debug
			if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
				EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		}
		for(int i = nodeStoreCounter; i >= 0; i--)
			this->CastToNPC()->GetMyPath().push(nodeStore[i]);
		this->CastToNPC()->SetMyPathNode(zone->thisZonesNodes[this->CastToNPC()->GetMyPath().front()]);
		this->CastToNPC()->SetOnPath(true);
		if(!doNotMove)
		{
			this->CastToNPC()->faceDestination(this->CastToNPC()->GetMyPathNode()->x, this->CastToNPC()->GetMyPathNode()->y);
			this->CastToNPC()->SetDestination(this->CastToNPC()->GetMyPathNode()->x, this->CastToNPC()->GetMyPathNode()->y, this->CastToNPC()->GetMyPathNode()->z);
			this->CastToNPC()->StartWalking();
			this->CastToNPC()->SetNWUPS(this->GetAnimation() * 2.4 / 5);
			this->CastToNPC()->MoveTowards(x_dest, y_dest, this->CastToNPC()->GetNWUPS());
			this->CastToNPC()->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
		}
	}

	while(!deletion.empty()){
		currentNode = deletion.front();
		safe_delete(currentNode);//delete currentNode;
		deletion.pop();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	safe_delete(myQ);//delete myQ;

	if(!solution_found)
	{
		if(bailCondition)
			return TELEPORTATION_NEEDED;
		else
			return PATH_NOT_FOUND;
	}
	return PATH_FOUND;
}

//o--------------------------------------------------------------
//| findMyPreprocessedPath; Yeahlight, October 10, 2008
//o--------------------------------------------------------------
//| Sets the path for an NPC between two nodes using the zone's
//| preprocessed path sets
//o--------------------------------------------------------------
bool Mob::findMyPreprocessedPath(Node* start, Node* end, bool parsing)
{
	queue <int16> pathToTake;
	if(start == end)
	{
		pathToTake.push(start->nodeID);
	}
	else
	{
		int16 pathArray[MAX_PATH_SIZE];
		for(int i = 0; i < MAX_PATH_SIZE; i++)
			pathArray[i] = zone->zonePaths[start->nodeID][end->nodeID].path[i];
		int16 nodeID = 0;
		int16 counter = 0;
		nodeID = pathArray[counter];
		counter++;
		if(nodeID == NULL_NODE)
			return false;
		while(nodeID != NULL_NODE && counter < MAX_PATH_SIZE)
		{
			pathToTake.push(nodeID);
			nodeID = pathArray[counter];
			counter++;
			//Yeahlight: Zone freeze debug
			if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
				EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		}
	}
	this->CastToNPC()->SetMyPath(pathToTake);
	if(!parsing)
	{
		this->CastToNPC()->SetMyPathNode(zone->thisZonesNodes[this->CastToNPC()->GetMyPath().front()]);
		this->CastToNPC()->SetOnPath(true);
		this->CastToNPC()->faceDestination(this->CastToNPC()->GetMyPathNode()->x, this->CastToNPC()->GetMyPathNode()->y);
		this->CastToNPC()->SetDestination(this->CastToNPC()->GetMyPathNode()->x, this->CastToNPC()->GetMyPathNode()->y, this->CastToNPC()->GetMyPathNode()->z);
		this->CastToNPC()->StartWalking();
		this->CastToNPC()->SetNWUPS(this->GetAnimation() * 2.4 / 5);
		this->CastToNPC()->MoveTowards(x_dest, y_dest, this->CastToNPC()->GetNWUPS());
		this->CastToNPC()->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
	}
	return true;
}

//o--------------------------------------------------------------
//| findClosestNode; Yeahlight, October 13, 2008
//o--------------------------------------------------------------
//| Returns the first node within 300 range which has LoS
//| and satisfies the no z-leap restriction
//o--------------------------------------------------------------
Node* Mob::findClosestNode(Mob* mob, bool parse)
{
	//Yeahlight: If this mob is an NPC, there's a good chance they are standing at their stored node already; check for it
	if(mob->IsNPC() && !parse)
		if(mob->mySpawnNode != NULL)
			if(!mob->CastToNPC()->isMoving())
				if(strcmp(mob->CastToNPC()->GetFeignMemory(), "0") != 0)
					if(CheckCoordLosNoZLeaps(mob->GetX(), mob->GetY(), mob->GetZ(), mob->mySpawnNode->x, mob->mySpawnNode->y, mob->mySpawnNode->z))
						return mob->mySpawnNode;

	int16 closestNode = 50000;
	int16 closestDistance = 50000;
	int16 maxZoneNodes = zone->numberOfNodes;
	if(maxZoneNodes == 0)
		return NULL;
	int16 bailCounter = 0;
	float distance = 0;
	float x = mob->GetX();
	float y = mob->GetY();
	float z = mob->GetZ();
	if(this->IsClient())
		z = FindGroundZ(x, y, 2);
	int16 distanceFromCenter = sqrt((float)(pow(x - 0, 2) + pow(y - 0, 2) + pow(z - 0, 2)));
	int aid = 0;
	int deviation = 0;
	int guessMax = maxZoneNodes;
	int guessGap = 0;
	int guessCounter = 1;
	int clientDistance = 0;
	int16 tempNodeID = 0;
	queue<int16> searchList;

	//Yeahlight: This routein is not used for parsing data
	if(!parse)
	{
		aid = (int)(((float)guessMax) * (float)(1/(pow(2, (double)guessCounter))));
		guessCounter++;
		guessGap = (int)(((float)guessMax) * (float)(1/(pow(2, (double)guessCounter))));
		while(bailCounter++ < 6)
		{
			distance = distanceFromCenter - zone->thisZonesNodes[aid]->distanceFromCenter;
			//Yeahlight: Our aid is close enough, bail out
			if(abs(distance) <= 75)
				break;
			if(distance < 0)
				aid = aid - guessGap;
			else
				aid = aid + guessGap;
			if(aid < 0)
			{
				aid = 0;
				break;
			}
			else if(aid >= maxZoneNodes)
			{
				aid = maxZoneNodes - 1;
				break;
			}
			guessCounter++;
			guessGap = (int)(((float)guessMax) * (float)(1/(pow(2, (double)guessCounter))));
			//Yeahlight: Zone freeze debug
			if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
				EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		}

		while((aid - (deviation * 10) >= 0) || (aid + (deviation * 10) < maxZoneNodes))
		{
			for(int i = 10 + (deviation * 10); i > 0 + (deviation * 10); i--)
			{
				if(aid - i >= 0)
					searchList.push(aid - i);
				if(aid + i < maxZoneNodes)
					searchList.push(aid + i);
			}
			deviation++;
			while(!searchList.empty())
			{
				tempNodeID = searchList.front();
				searchList.pop();
				distance = sqrt((double)(pow(zone->thisZonesNodes[tempNodeID]->x - x, 2) + pow(zone->thisZonesNodes[tempNodeID]->y - y, 2) + pow(zone->thisZonesNodes[tempNodeID]->z - z, 2)));
				if(distance <= 300 && CheckCoordLosNoZLeaps(x, y, z, zone->thisZonesNodes[tempNodeID]->x, zone->thisZonesNodes[tempNodeID]->y, zone->thisZonesNodes[tempNodeID]->z))
				{
					if(distance < closestDistance)
					{
						closestNode = tempNodeID;
						closestDistance = distance;
						if(mob->IsNPC() && mob->target != 0)
						{
							clientDistance = sqrt((double)(pow(x - mob->target->GetX(), 2) + pow(y - mob->target->GetY(), 2) + pow(z - mob->target->GetZ(), 2))) - 
											 sqrt((double)(pow(zone->thisZonesNodes[tempNodeID]->x - mob->target->GetX(), 2) + pow(zone->thisZonesNodes[tempNodeID]->y - mob->target->GetY(), 2) + pow(zone->thisZonesNodes[tempNodeID]->z - mob->target->GetZ(), 2)));            
						}
						else
						{
							clientDistance = 0;
						}
						//Yeahlight: An ideal node has been found, leave the routein with this node
						if(closestDistance <= 50 && clientDistance >= 0)
						{
							return zone->thisZonesNodes[closestNode];
						}
					}
				}
				//Yeahlight: Zone freeze debug
				if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
			//Yeahlight: A decent node has been found, leave the routein with this node
			if(closestDistance <= 50)
			{
				return zone->thisZonesNodes[closestNode];
			}
			//Yeahlight: Zone freeze debug
			if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
				EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		}
		//Yeahlight: An acceptable node has been found, leave the routein with this node
		if(closestDistance <= 300)
		{
			return zone->thisZonesNodes[closestNode];
		}
	}
	//Yeahlight: This routein is used for parsing data
	else
	{
		for(int i = 0; i < zone->numberOfNodes; i++)
		{
			distance = sqrt((double)(pow(zone->thisZonesNodes[i]->x - x, 2) + pow(zone->thisZonesNodes[i]->y - y, 2) + pow(zone->thisZonesNodes[i]->z - z, 2)));
			if(distance < 500 && CheckCoordLosNoZLeaps(x, y, z, zone->thisZonesNodes[i]->x, zone->thisZonesNodes[i]->y, zone->thisZonesNodes[i]->z))
			{
				if(distance < closestDistance)
				{
					closestDistance = distance;
					closestNode = i;
				}
			}
		}
		if(closestDistance != 50000)
			return zone->thisZonesNodes[closestNode];
	}
	return NULL;
}

///////////////////////////////////////////////////

float Mob::GetRunSpeed() {
	return this->runspeed + (((float)spellbonuses.MovementSpeed + (float)itembonuses.MovementSpeed)/100.0f * this->runspeed);
}

///////////////////////////////////////////////////

float Mob::GetWalkSpeed() {
	return this->walkspeed + (((float)spellbonuses.MovementSpeed + (float)itembonuses.MovementSpeed)/100.0f * this->walkspeed);
}

///////////////////////////////////////////////////

// setBoatSpeed (15/06/09)
// Tazadar : We only use the runspeed for a boat
void Mob::setBoatSpeed(float speed){
	this->walkspeed = speed/0.64;
	this->runspeed = speed/0.64;
}

///////////////////////////////////////////////////

sint8 Mob::GetAnimation() {
	return this->animation;
}

///////////////////////////////////////////////////

Buffs_Struct& Mob::GetBuff(int8 index) {
	if (index >= BUFF_COUNT)
		index = 0;
	return this->buffs[index];
}

///////////////////////////////////////////////////

//o--------------------------------------------------------------
//| GetLevelRegen; Yeahlight, Dec 19, 2008
//o--------------------------------------------------------------
//| Returns the amount of regen for the mob based on level, 
//| race and appearance
//o--------------------------------------------------------------
int32 Mob::GetLevelRegen()
{
	//Yeahlight: Data gathered from http://www.midnightskywalker.com/everquest/regenchart.html
	int16 myLevel = GetLevel();
	int16 myRace = GetBaseRace();
	int16 myLevelBracket = 0;
	int16 myRegenRank = 0;
	int32 HP_Regen = 0;

	//Yeahlight: Iksar / Troll             <21  <51  <52  <57  <60  =60
	static int16 bonusRegenArray[3][6] = {
	/*                         Sitting*/     4,   6,   8,  12,  16,  18,
	/*                         FD'ed*/       3,   3,   4,   8,  12,  14,
	/*                         Standing*/    2,   2,   2,   6,  10,  12};
	
	//Yeahlight: All other races           <21  <51  <52  <57  <60  =60
	static int16 regenArray[3][6]      = {
	/*                         Sitting*/     2,   3,   4,   5,   6,   7,
	/*                         FD'ed*/       1,   2,   2,   3,   4,   5,
	/*                         Standing*/    1,   1,   1,   2,   3,   4};

	//Yeahlight: Find the level bracket for the mob
	if(myLevel <= REGEN_LEVEL_FIRST)
		myLevelBracket = 0;
	else if(myLevel <= REGEN_LEVEL_SECOND)
		myLevelBracket = 1;
	else if(myLevel <= REGEN_LEVEL_THIRD)
		myLevelBracket = 2;
	else if(myLevel <= REGEN_LEVEL_FOURTH)
		myLevelBracket = 3;
	else if(myLevel <= REGEN_LEVEL_FIFTH)
		myLevelBracket = 4;
	else
		myLevelBracket = 5;

	//Yeahlight: Find the regen rank for the mob
	if(appearance == SittingAppearance)
		myRegenRank = 0;
	else if(this->IsClient() && this->CastToClient()->GetFeigned())
		myRegenRank = 1;
	else
		myRegenRank = 2;

	//Yeahlight: Iksar and Troll use the bonus regen array
	if(myRace == IKSAR || myRace == TROLL)
		return bonusRegenArray[myRegenRank][myLevelBracket];
	else
		return regenArray[myRegenRank][myLevelBracket];
}

///////////////////////////////////////////////////

//Yeahlight: TODO: I recall something in EQ along the lines of "If your target is diseased (disease based DoT), your target no longer gets their regen bonus"
void Mob::DoHPRegen(int32 expected_new_hp)
{
	CalcMaxHP();
	CalcMaxMana();
	TicProcess();
	const sint32	current_HP		= GetHP();
	const sint32	max_HP			= GetMaxHP();
	int32           base_regen		= GetLevelRegen();
	//Yeahlight: The NPC has a higher predetermined regen rate in the DB
	if(IsNPC() && CastToNPC()->GetMyRegenRate() > base_regen)
		base_regen = CastToNPC()->GetMyRegenRate();
	const sint32	spells_regen	= spellbonuses.HPRegen;
	const sint32	items_regen		= itembonuses.HPRegen;
	sint32			HP_regen		= base_regen + spells_regen + items_regen;
	sint32			new_HP			= current_HP + HP_regen;

	if(new_HP > max_HP)
		new_HP = max_HP;
	//if(current_HP < this->GetMaxHP() || (expected_new_hp != new_HP && expected_new_hp > 0))
	//{
	//	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Mob::DoHPRegen(): New HP: %i. Base: %i, spells: %i, items: %i", new_HP, base_regen, spells_regen, items_regen);
	//	if (this->IsClient() && new_HP != expected_new_hp)
	//		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Mob::DoHPRegen(): Error: I was expecting new HP to be %i, but it is %i", expected_new_hp, new_HP);
	//Yeahlight: Only issue updates when their current hp is not equal to their max hp
	if(new_HP < max_HP || current_HP != max_HP)
	{
		//Yeahlight: PCs may dip down to -9 hp
		if(this->IsClient() && new_HP > -10)
		{
			SetHP(new_HP);
			SendHPUpdate();
		}
		//Yeahlight: NPCs may only dip dow to 1hp
		else if(this->IsNPC() && new_HP > 0)
		{
			SetHP(new_HP);
			SendHPUpdate();
		}
		//Yeahlight: Mob is a PC below -9hp or an NPC below 1hp
		else if(new_HP <= 0)
		{
			Mob* killer = NULL;
			int16 DoTDamage = 0;
			Spell* spell = NULL;
			//Yeahlight: Iterate through all the available buffs on the mob
			for(int i = 0; i < 15; i++)
			{
				//Yeahlight: Search for a detrimental debuff
				if(buffs[i].spell && buffs[i].spell->IsDetrimentalSpell())
				{
					//Yeahlight: Iterate through the debuff's effect IDs
					for(int j = 0; j < 12; j++)
					{
						//Yeahlight: Debuff is a DoT
						if(buffs[i].spell->GetSpellEffectID(j) == SE_CurrentHP)
						{
							//Yeahligth: Killer will be on the first debuff with a DoT portion
							killer = entity_list.GetMob(buffs[i].casterid);
							//Yeahlight: If the killer is present, calculate the damage responsible for the death blow
							if(killer)
							{
								DoTDamage = spells_handler.CalcSpellValue(buffs[i].spell, j, killer->GetLevel());
								spell = buffs[i].spell;
							}
							//Yeahlight: "Break out" of the loops
							i = 15;
							j = 12;
						}
					}
				}
			}
			//Yeahlight: We have a mob to credit for the kill
			if(killer)
			{
				if(IsClient())
					CastToClient()->Death(killer, DoTDamage, spell->GetSpellID(), 0xFF);
				else if(IsNPC())
					CastToNPC()->Death(killer, DoTDamage, spell->GetSpellID(), 0xFF);
			}
			//Yeahlight: This mob died on its own
			else
			{
				if(IsClient())
					CastToClient()->Death(killer, 0);
				else if(IsNPC())
					CastToNPC()->Death(killer, 0);
			}
		}
	}
}

/********************************************************************
 *                        Tazadar  - 8/29/08                        *
 ********************************************************************
 *                          DoEnduRegen()				            *                        
 ********************************************************************
 *  + Manages the Endu Regeneration		                            *
 *                                                                  *
 ********************************************************************/
void Mob::DoEnduRegen(){

	if(this->CastToClient()->pp.hungerlevel==0 || this->CastToClient()->pp.thirstlevel==0){//if we are hungry ir thirst => fatigue
		this->CastToClient()->pp.fatigue++;
		if(this->CastToClient()->pp.fatigue>100){
			this->CastToClient()->pp.fatigue=100;
		}
	}
	else{
		if(this->CastToClient()->pp.fatigue!=0 && !InWater){
			this->CastToClient()->pp.fatigue-=10;
			if(this->CastToClient()->pp.fatigue<0 || this->CastToClient()->pp.fatigue>200){
				this->CastToClient()->pp.fatigue=0;
			}
			if(this->CastToClient()->pp.fatigue>100){
				this->CastToClient()->pp.fatigue=100;
			}
		}
		if(InWater){//If we are in water we lose stamina !
			this->CastToClient()->pp.fatigue++;
			if(this->CastToClient()->pp.fatigue>100){
				this->CastToClient()->pp.fatigue=90; // Verified Client side (Tazadar)
			}
		}
	}

	sint32 hunger=-2;//We get more hungry :)
	hunger+=this->CastToClient()->pp.hungerlevel;
	if(hunger < 0){
		this->CastToClient()->pp.hungerlevel=0;
	}
	else{
		this->CastToClient()->pp.hungerlevel=hunger;
	}
	sint32 thirst=-2;//We get more thirsty :)
	thirst+=this->CastToClient()->pp.thirstlevel;
	if(thirst < 0){
		this->CastToClient()->pp.thirstlevel=0;
	}
	else{
		this->CastToClient()->pp.thirstlevel=thirst;
	}

	
	//We send the packet with new hunger/thirst/endu level 
	APPLAYER* outapp = new APPLAYER(OP_Stamina, sizeof(Stamina_Struct));
	Stamina_Struct* sta = (Stamina_Struct*)outapp->pBuffer;
	sta->food = this->CastToClient()->pp.hungerlevel;
	sta->water = this->CastToClient()->pp.thirstlevel;
	sta->fatigue = this->CastToClient()->pp.fatigue;
	int totalendu=100-this->CastToClient()->pp.fatigue;
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Mob::DoEnduRegen(): Endu: %i, Hunger: %i ,Thirst: %i ", totalendu,this->CastToClient()->pp.hungerlevel,this->CastToClient()->pp.thirstlevel);
	this->CastToClient()->QueuePacket(outapp);
	safe_delete(outapp);

}

///////////////////////////////////////////////////

/*
Consolidated GetCasterClass() with CalcMaxMana.
Get rid of a method returning stupid magic strings 'W' for WIS based classes, 'I' for INT based classes and 'N' for malee
*/

sint32 Mob::CalcMaxMana()
{
	//Yeahlight: TODO: I specifically recall mana gains slowing down at 200, thus a simple formula cannot be correct
	int8 tmplevel = GetLevel();

	switch(class_)
	{
	case CLERIC:
	case PALADIN:
	case RANGER:
	case DRUID:
	case SHAMAN:
	case BEASTLORD:
		max_mana = (((GetWIS()/5)+2) * tmplevel) + spellbonuses.Mana + itembonuses.Mana;
		break;

	case SHADOWKNIGHT:
	case BARD:
	case NECROMANCER:
	case WIZARD:
	case MAGICIAN:
	case ENCHANTER:
		max_mana = (((GetINT()/5)+2) * tmplevel) + spellbonuses.Mana + itembonuses.Mana;
		break;

	default:
		max_mana = 0;
		break;
	}

	if (this->cur_mana > max_mana)
	{
		this->cur_mana = max_mana;
		if (this->IsClient())
			this->CastToClient()->SendManaUpdatePacket();
	}
	return max_mana;
}

///////////////////////////////////////////////////

void Mob::SendHPUpdate()	
{
	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Mob::SendHPUpdate()");
	APPLAYER* app = new APPLAYER;
	CreateHPPacket(app);

	if (IsClient() && CastToClient()->GMHideMe())
	{
		entity_list.QueueClientsStatus(this, app, false, CastToClient()->admin);
	}
	else if (IsNPC())
	{
		entity_list.QueueCloseClients(this, app, true);
	}
	else
	{
		entity_list.QueueCloseClients(this, app);
	}

	safe_delete(app);//delete app;
}

///////////////////////////////////////////////////

void Mob::DoManaRegen() {
	int32 level=GetLevel();
	int32 oldmana=GetMana();
	int32 newmana=0;
	if (GetMana() < max_mana) {
		if (GetAppearance() == SittingAppearance) {
			int16 meditate_skill = IsClient() ? CastToClient()->GetSkill(MEDITATE) : GetLevel()*5;
			if(meditate_skill > 0){
				newmana=oldmana+(((meditate_skill/10)+(level-(level/4)))/4)+4 + spellbonuses.ManaRegen + itembonuses.ManaRegen;
				//Message(0,"GetMana: %i,GetSkill: %i, GetLevel(): %i, Total: %i",GetMana(),GetSkill(MEDITATE)/10,(GetLevel()-(GetLevel()/4)),newmana);
				CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Mob::DoManaRegen(): old mana: %i, new mana: %i", oldmana, newmana);
				SetMana(newmana);
				if (IsClient())
					CastToClient()->CheckAddSkill(MEDITATE);
			}
			if(((newmana-oldmana)<=0) || ((newmana-oldmana)>(max_mana/6)))
				SetMana(GetMana()+2+(level/5) + spellbonuses.ManaRegen + itembonuses.ManaRegen);
		}
		else {
			newmana = GetMana()+2+(level/5);
			CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Mob::DoManaRegen(): old mana: %i, new mana: %i", oldmana, newmana);
			SetMana(newmana);
		}
	}
}

///////////////////////////////////////////////////
void Mob::Message(MessageFormat Type, char* pMessage, ...) {
	if (!IsClient()) return;
	//-Cofruben: attempt to secure this.
	const int32 TAM_MAX = 2056;
	if (!pMessage)
	{
		EQC_MOB_EXCEPT("void Client::Message()", "pText = NULL");
	}

	int32 Len = strlen(pMessage);

	if (Len > TAM_MAX)
	{
		EQC_MOB_EXCEPT("void Client::Message()", "Length is too big");
	}

	va_list Args;
	char* Buffer = new char[Len*10+512];
	memset(Buffer, 0, Len*10+256);
	va_start(Args, pMessage);
	vsnprintf(Buffer, Len*10 + 256, pMessage, Args);
	va_end(Args);
	string NewString = Buffer;

	APPLAYER* app = new APPLAYER(OP_SpecialMesg, 4+strlen(Buffer)+1);
	//app->opcode = OP_SpecialMesg;
	//app->size = 4+strlen(Buffer)+1;
	//app->pBuffer = new uchar[app->size];
	SpecialMesg_Struct* sm=(SpecialMesg_Struct*)app->pBuffer;
	sm->msg_type = Type;
	strcpy(sm->message, Buffer);
	CastToClient()->QueuePacket(app);
	safe_delete(app);//delete app;
	safe_delete_array(Buffer);//delete[] Buffer;
}

//o--------------------------------------------------------------
//| CanThisClassDodge; Yeahlight, Nov 24, 2008
//o--------------------------------------------------------------
//| Returns true if the Client/NPC skill is available
//o--------------------------------------------------------------
bool Mob::CanThisClassDodge()
{
	if(this->IsClient())
	{
		//Yeahlight: 254 (0xFE) is reserved for "not yet trained" and 255 (0xFF) is reserved for "cannot use."
		//           Only PCs are subjected to these values
		if(this->CastToClient()->GetSkill(DODGE) > 0 && this->CastToClient()->GetSkill(DODGE) <= 252)
			return true;
	}
	else
	{
		if(this->GetSkill(DODGE) > 0)
			return true;
	}
	return false;
}

//o--------------------------------------------------------------
//| CanThisClassParry; Yeahlight, Nov 24, 2008
//o--------------------------------------------------------------
//| Returns true if the Client/NPC skill is available
//o--------------------------------------------------------------
bool Mob::CanThisClassParry()
{
	if(this->IsClient())
	{
		//Yeahlight: 254 (0xFE) is reserved for "not yet trained" and 255 (0xFF) is reserved for "cannot use."
		//           Only PCs are subjected to these values
		if(this->CastToClient()->GetSkill(PARRY) > 0 && this->CastToClient()->GetSkill(PARRY) <= 252)
			return true;
	}
	else
	{
		if(this->GetSkill(PARRY) > 0)
			return true;
	}
	return false;
}

//o--------------------------------------------------------------
//| CanThisClassBlock; Yeahlight, Nov 24, 2008
//o--------------------------------------------------------------
//| Returns true if the Client/NPC skill is available
//o--------------------------------------------------------------
bool Mob::CanThisClassBlock()
{
	if(this->IsClient())
	{
		//Yeahlight: 254 (0xFE) is reserved for "not yet trained" and 255 (0xFF) is reserved for "cannot use."
		//           Only PCs are subjected to these values
		if(this->CastToClient()->GetSkill(SKILL_BLOCK) > 0 && this->CastToClient()->GetSkill(SKILL_BLOCK) <= 252)
			return true;
	}
	else
	{
		if(this->GetSkill(SKILL_BLOCK) > 0)
			return true;
	}
	return false;
}

//o--------------------------------------------------------------
//| CanThisClassRiposte; Yeahlight, Nov 24, 2008
//o--------------------------------------------------------------
//| Returns true if the Client/NPC skill is available
//o--------------------------------------------------------------
bool Mob::CanThisClassRiposte()
{
	if(this->IsClient())
	{
		//Yeahlight: 254 (0xFE) is reserved for "not yet trained" and 255 (0xFF) is reserved for "cannot use."
		//           Only PCs are subjected to these values
		if(this->CastToClient()->GetSkill(RIPOSTE) > 0 && this->CastToClient()->GetSkill(RIPOSTE) <= 252)
			return true;
	}
	else
	{
		if(this->GetSkill(RIPOSTE) > 0)
			return true;
	}
	return false;
}

//o--------------------------------------------------------------
//| CanNotSeeTarget; Yeahlight, May 21, 2008
//o--------------------------------------------------------------
//| Returns true if player is outside the bounds of the frontal
//| attack cone (140 degrees).
//o--------------------------------------------------------------
bool Mob::CanNotSeeTarget(float mobx, float moby)
{  
	float angle, lengthb, vectorx, vectory;  
	float playerx = -(GetX());      // mob xlocation (inverse because eq is confused)  
	float playery = GetY();         // mobylocation  
	float heading = (int8)GetHeading();     // your heading  
	heading = (heading * 360.0)/256.0;      // convert to degrees  
	if (heading < 270)  
		 heading += 90;  
	else  
		 heading -= 270;  
	heading = heading*3.1415/180.0; // convert to radians  
	vectorx = playerx + (10.0 * cosf(heading));     // create a vector based on heading  
	vectory = playery + (10.0 * sinf(heading));     // of mob length 10  

	//length of mob to player vector  
	lengthb = (float)sqrt(pow((-mobx-playerx),2) + pow((moby-playery),2));  

	// calculate dot product to get angle  
	angle = acosf(((vectorx-playerx)*(-mobx-playerx)+(vectory-playery)*(moby-playery)) / (10 * lengthb));  
	angle = angle * 180 / 3.1415;  
	if (angle > (FRONTAL_ATTACK_CONE / 2.0))  {
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_PATHING, "Client::CanNotSeeTarget(mobx = %f, moby = %f) returned true", mobx, moby);
		return true;
	}
	else {
		CAST_CLIENT_DEBUG_PTR(this)->Log(CP_PATHING, "Client::CanNotSeeTarget(mobx = %f, moby = %f) returned false", mobx, moby);
		return false;  
	}
}

//o--------------------------------------------------------------
//| CheckMaxSkill; Yeahlight, May 21, 2008
//o--------------------------------------------------------------
//| Calculates the max value for each skill based on class, race
//| and level. The EXCEL '07 file for this table is in the root
//| of the main SVN as Skills.xlsx!
//o--------------------------------------------------------------
int16 Mob::CheckMaxSkill(int skillid, int8 race, int8 eqclass, int8 level, bool gmRequest)
{
	//Yeahlight: Skip all this overhead if the skill has not yet been trained or cannot be trained
	if(this->IsClient() && !gmRequest)
		if((int)this->CastToClient()->GetSkill(skillid) == 254 || (int)this->CastToClient()->GetSkill(skillid) == 255)
			return 0;

	//Yeahlight: Special checks for NPCs
	if(this->IsNPC() && (level <= 40 || (eqclass == BARD && level <= 58)))
	{
		int16 levelRequired = 1;
		switch(skillid)
		{
			case BASH:
			{
				switch(eqclass)
				{
					case WARRIOR: case WARRIORGM: case BANKER: case MERCHANT: case PALADIN: case SHADOWKNIGHT: case PALADINGM: case SHADOWKNIGHTGM: 
						levelRequired = 6;
						break;
					case CLERIC:
						levelRequired = 15;
						break;
				}
				break;
			}
			case DODGE:
			{
				switch(eqclass)
				{
					case WARRIOR: case WARRIORGM: case BANKER: case MERCHANT: 
						levelRequired = 6;
						break;
					case CLERIC: case DRUID: case SHAMAN: case CLERICGM: case DRUIDGM: case SHAMANGM: 
						levelRequired = 15;
						break;
					case RANGER: case RANGERGM:
						levelRequired = 8;
						break;
					case ROGUE: case ROGUEGM:
						levelRequired = 4;
						break;
					case PALADIN: case PALADINGM: case SHADOWKNIGHT: case SHADOWKNIGHTGM: case BARD: case BARDGM: 
						levelRequired = 10;
						break;
					case NECROMANCER: case WIZARD: case ENCHANTER: case MAGICIAN: case NECROMANCERGM: case WIZARDGM: case ENCHANTERGM: case MAGICIANGM: 
						levelRequired = 22;
						break;
				}
				break;
			}
			case PARRY:
			{
				switch(eqclass)
				{
					case WARRIOR: case WARRIORGM: case BANKER: case MERCHANT:
						levelRequired = 10;
						break;
					case PALADIN: case PALADINGM: case SHADOWKNIGHT: case SHADOWKNIGHTGM:
						levelRequired = 17;
						break;
					case RANGER: case RANGERGM: 
						levelRequired = 18;
						break;
					case ROGUE: case ROGUEGM:
						levelRequired = 12;
						break;
					case BARD: case BARDGM:
						levelRequired = 53;
						break;
				}
				break;
			}
			case DUEL_WIELD:
			{
				switch(eqclass)
				{
					case WARRIOR: case WARRIORGM: case BANKER: case MERCHANT: case ROGUE: case ROGUEGM:
						levelRequired = 13;
						break;
					case RANGER: case RANGERGM: case BARD: case BARDGM:
						levelRequired = 17;
						break;
				}
				break;
			}
			case DOUBLE_ATTACK:
			{
				switch(eqclass)
				{
					case WARRIOR: case WARRIORGM: case BANKER: case MERCHANT: case MONK: case MONKGM: 
						levelRequired = 15;
						break;
					case PALADIN: case PALADINGM: case SHADOWKNIGHT: case SHADOWKNIGHTGM: case RANGER: case RANGERGM:
						levelRequired = 20;
						break;
					case ROGUE: case ROGUEGM:
						levelRequired = 16;
						break;
				}
				break;
			}
			case RIPOSTE:
			{
				switch(eqclass)
				{
					case WARRIOR: case WARRIORGM: case BANKER: case MERCHANT:
						levelRequired = 25;
						break;
					case RANGER: case RANGERGM: case MONK: case MONKGM: 
						levelRequired = 35;
						break;
					case PALADIN: case PALADINGM: case SHADOWKNIGHT: case SHADOWKNIGHTGM: case ROGUE: case ROGUEGM:
						levelRequired = 30;
						break;
					case BARD: case BARDGM:
						levelRequired = 58;
						break;
				}
				break;
			}
			case DISARM:
			{
				switch(eqclass)
				{
					case WARRIOR: case WARRIORGM: case BANKER: case MERCHANT: case RANGER: case RANGERGM:
						levelRequired = 35;
						break;
					case PALADIN: case PALADINGM: case SHADOWKNIGHT: case SHADOWKNIGHTGM: 
						levelRequired = 40;
						break;
					case MONK: case MONKGM: case ROGUE: case ROGUEGM:
						levelRequired = 27;
						break;
				}
				break;
			}
			case SKILL_BLOCK:
			{
				if(eqclass == MONK)
					levelRequired = 12;
				break;
			}
			case ROUND_KICK:
			{
				if(eqclass == MONK)
					levelRequired = 5;
				break;
			}
			case TIGER_CLAW:
			{
				if(eqclass == MONK)
					levelRequired = 10;
				break;
			}
			case EAGLE_STRIKE:
			{
				if(eqclass == MONK)
					levelRequired = 20;
				break;
			}
			case DRAGON_PUNCH:
			{
				if(eqclass == MONK)
					levelRequired = 25;
				break;
			}
			case FLYING_KICK:
			{
				if(eqclass == MONK)
					levelRequired = 30;
				break;
			}
		}
		if(levelRequired > level)
			return 0;
	}

	//Yeahlight: Note: Skills begin at 0, but classes begin at 1. Don't forget to subtract 1 from eqclass when using this array!
	//*******************************Warrior Cleric Paladin Ranger Shadow_Knight Druid Monk Bard Rogue Shaman Necromancer Wizard Magician Enchanter Beastlord Banker WarriorGM ClericGM PaladinGM RangerGM Shadow_KnightGM DruidGM MonkGM BardGM RogueGM ShamanGM NecromancerGM WizardGM MagicianGM EnchanterGM BeastlordGM Merchant
	static int8 maxSkills[74][32] = {
				/*_1H_BLUNT*/		 200, 175, 200, 200, 200, 175, 240, 200, 200, 200, 110, 110, 110, 110, 0, 0, 200, 175, 200, 200, 200, 175, 240, 200, 200, 200, 110, 110, 110, 110, 0, 0,
				/*_1H_SLASHING*/	 200, 0, 200, 200, 200, 175, 0, 200, 200, 0, 0, 0, 0, 0, 0, 0, 200, 0, 200, 200, 200, 175, 0, 200, 200, 0, 0, 0, 0, 0, 0, 0,
				/*_2H_BLUNT*/		 200, 175, 200, 200, 200, 175, 240, 0, 0, 200, 110, 110, 110, 110, 0, 0, 200, 175, 200, 200, 200, 175, 240, 0, 0, 200, 110, 110, 110, 110, 0, 0,
				/*_2H_SLASHING*/	 200, 0, 200, 200, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 200, 200, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*ABJURATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*ALTERATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*APPLY_POISON*/	 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0,
				/*ARCHERY*/			 200, 0, 75, 240, 75, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 200, 0, 75, 240, 75, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0,
				/*BACKSTAB*/		 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0,
				/*BIND_WOUND*/		 175, 200, 200, 150, 150, 200, 200, 150, 176, 200, 100, 100, 100, 100, 0, 0, 175, 200, 200, 150, 150, 200, 200, 150, 176, 200, 100, 100, 100, 100, 0, 0,
				/*BASH*/			 220, 0, 180, 0, 175, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 220, 0, 180, 0, 175, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*BLOCK*/			 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*BRASS_INSTR*/		 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*CHANNELING*/		 0, 200, 200, 200, 200, 200, 0, 0, 0, 200, 200, 200, 200, 200, 0, 0, 0, 200, 200, 200, 200, 200, 0, 0, 0, 200, 200, 200, 200, 200, 0, 0,
				/*CONJURATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*DEFENSE*/			 210, 200, 210, 200, 210, 200, 230, 200, 200, 200, 145, 145, 145, 145, 0, 0, 210, 200, 210, 200, 210, 200, 230, 200, 200, 200, 145, 145, 145, 145, 0, 0,
				/*DISARM*/			 200, 0, 70, 55, 70, 0, 200, 0, 200, 0, 0, 0, 0, 0, 0, 0, 200, 0, 70, 55, 70, 0, 200, 0, 200, 0, 0, 0, 0, 0, 0, 0,
				/*DISARM_TRAPS*/	 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0,
				/*DIVINATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*DODGE*/			 140, 75, 125, 137, 125, 75, 200, 125, 150, 75, 75, 75, 75, 75, 0, 0, 140, 75, 125, 137, 125, 75, 200, 125, 150, 75, 75, 75, 75, 75, 0, 0,
				/*DOUBLE_ATTACK*/	 205, 0, 200, 200, 200, 0, 210, 0, 200, 0, 0, 0, 0, 0, 0, 0, 205, 0, 200, 200, 200, 0, 210, 0, 200, 0, 0, 0, 0, 0, 0, 0,
				/*DRAGON_PUNCH*/	 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*DUEL_WIELD*/		 210, 0, 0, 210, 0, 0, 252, 210, 210, 0, 0, 0, 0, 0, 0, 0, 210, 0, 0, 210, 0, 0, 252, 210, 210, 0, 0, 0, 0, 0, 0, 0,
				/*EAGLE_STRIKE*/	 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*EVOCATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*FEIGN_DEATH*/		 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*FLYING_KICK*/		 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*FORAGE*/			 0, 0, 0, 200, 0, 200, 50, 55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 200, 50, 55, 0, 0, 0, 0, 0, 0, 0, 0,
				/*HAND_TO_HAND*/	 100, 75, 100, 100, 100, 75, 225, 100, 100, 75, 75, 75, 75, 75, 0, 0, 100, 75, 100, 100, 100, 75, 225, 100, 100, 75, 75, 75, 75, 75, 0, 0,
				/*HIDE*/			 0, 0, 0, 75, 75, 0, 0, 40, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 0, 0, 40, 200, 0, 0, 0, 0, 0, 0, 0,
				/*KICK*/			 149, 0, 0, 149, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 149, 0, 0, 149, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*MEDITATE*/		 0, 235, 185, 185, 235, 235, 0, 1, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 185, 185, 235, 235, 0, 1, 0, 235, 235, 235, 235, 235, 0, 0,
				/*MEND*/			 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*OFFENSE*/			 210, 200, 200, 210, 200, 200, 230, 200, 210, 200, 140, 140, 140, 140, 0, 0, 210, 200, 200, 210, 200, 200, 230, 200, 210, 200, 140, 140, 140, 140, 0, 0,
				/*PARRY*/			 200, 0, 175, 185, 175, 0, 0, 75, 200, 0, 0, 0, 0, 0, 0, 0, 200, 0, 175, 185, 175, 0, 0, 75, 200, 0, 0, 0, 0, 0, 0, 0,
				/*PICK_LOCK*/		 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0,
				/*PIERCING*/		 200, 0, 200, 200, 200, 0, 0, 200, 210, 200, 110, 110, 110, 110, 0, 0, 200, 0, 200, 200, 200, 0, 0, 200, 210, 200, 110, 110, 110, 110, 0, 0,
				/*RIPOSTE*/			 200, 0, 175, 150, 175, 0, 200, 75, 200, 0, 0, 0, 0, 0, 0, 0, 200, 0, 175, 150, 175, 0, 200, 75, 200, 0, 0, 0, 0, 0, 0, 0,
				/*ROUND_KICK*/		 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*SAFE_FALL*/		 0, 0, 0, 0, 0, 0, 200, 40, 94, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 40, 94, 0, 0, 0, 0, 0, 0, 0,
				/*SENSE_HEADING*/	 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*SINGING*/			 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*SNEAK*/			 0, 0, 0, 75, 0, 0, 113, 75, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 75, 0, 0, 113, 75, 200, 0, 0, 0, 0, 0, 0, 0,
				/*SPECIALIZE_ABJ*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*SPECIALIZE_ALT*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*SPECIALIZE_CON*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*SPECIALIZE_DIV*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*SPECIALIZE_EVO*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*PICK_POCKETS*/	 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0,
				/*STRINGED_INSTRU*/	 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*SWIMMING*/		 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*THROWING*/		 113, 0, 0, 113, 0, 0, 113, 113, 220, 0, 75, 75, 75, 75, 0, 0, 113, 0, 0, 113, 0, 0, 113, 113, 220, 0, 75, 75, 75, 75, 0, 0,
				/*TIGER_CLAW*/		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*TRACKING*/		 0, 0, 0, 200, 0, 125, 0, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 125, 0, 100, 0, 0, 0, 0, 0, 0, 0, 0,
				/*WIND_INSTRUMENTS*/ 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*SKILL_FISHING*/	 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*MAKE_POISON*/		 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0,
				/*TINKERING*/		 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*RESEARCH*/		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 200, 200, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 200, 200, 200, 0, 0,
				/*ALCHEMY*/			 0, 0, 0, 0, 0, 0, 0, 0, 0, 130, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 130, 0, 0, 0, 0, 0, 0,
				/*BAKING*/			 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*TAILORING*/		 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*SENSE_TRAPS*/		 0, 0, 0, 0, 0, 0, 0, 75, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 75, 200, 0, 0, 0, 0, 0, 0, 0,
				/*BLACKSMITHING*/	 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*FLETCHING*/		 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*BREWING*/			 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*ALCOHOL_TOL*/		 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*BEGGING*/			 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*JEWELRY_MAKING*/	 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*POTTERY*/			 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*PERCUSSION_INSTR*/ 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*INTIMIDATION*/	 0, 0, 0, 0, 0, 0, 200, 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 100, 200, 0, 0, 0, 0, 0, 0, 0,
				/*BERSERKING*/		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*TAUNT*/			 200, 0, 180, 150, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 180, 150, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	//*******************************Warrior Cleric Paladin Ranger Shadow_Knight Druid Monk Bard Rogue Shaman Necromancer Wizard Magician Enchanter Beastlord Banker WarriorGM ClericGM PaladinGM RangerGM Shadow_KnightGM DruidGM MonkGM BardGM RogueGM ShamanGM NecromancerGM WizardGM MagicianGM EnchanterGM BeastlordGM Merchant
	static int8 maxSkillsBeyond50[74][32] = { 
				/*_1H_BLUNT*/		 250, 175, 225, 250, 225, 175, 252, 225, 250, 200, 110, 110, 110, 110, 0, 0, 250, 175, 225, 250, 225, 175, 252, 225, 250, 200, 110, 110, 110, 110, 0, 0,
				/*_1H_SLASHING*/	 250, 0, 225, 250, 225, 175, 0, 225, 250, 0, 0, 0, 0, 0, 0, 0, 250, 0, 225, 250, 225, 175, 0, 225, 250, 0, 0, 0, 0, 0, 0, 0,
				/*_2H_BLUNT*/		 250, 175, 225, 250, 225, 175, 252, 0, 0, 200, 110, 110, 110, 110, 0, 0, 250, 175, 225, 250, 225, 175, 252, 0, 0, 200, 110, 110, 110, 110, 0, 0,
				/*_2H_SLASHING*/	 250, 0, 225, 250, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 250, 0, 225, 250, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*ABJURATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*ALTERATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*APPLY_POISON*/	 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0,
				/*ARCHERY*/			 240, 0, 75, 240, 75, 0, 0, 0, 240, 0, 0, 0, 0, 0, 0, 0, 240, 0, 75, 240, 75, 0, 0, 0, 240, 0, 0, 0, 0, 0, 0, 0,
				/*BACKSTAB*/		 0, 0, 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0,
				/*BIND_WOUND*/		 210, 201, 210, 200, 200, 200, 210, 200, 210, 200, 100, 100, 100, 100, 0, 0, 210, 201, 210, 200, 200, 200, 210, 200, 210, 200, 100, 100, 100, 100, 0, 0,
				/*BASH*/			 240, 0, 200, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 240, 0, 200, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*BLOCK*/			 0, 0, 0, 0, 0, 0, 230, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 230, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*BRASS_INSTR*/		 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*CHANNELING*/		 0, 220, 220, 215, 220, 220, 0, 0, 0, 220, 220, 220, 220, 220, 0, 0, 0, 220, 220, 215, 220, 220, 0, 0, 0, 220, 220, 220, 220, 220, 0, 0,
				/*CONJURATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*DEFENSE*/			 252, 200, 252, 200, 252, 200, 252, 252, 252, 200, 145, 145, 145, 145, 0, 0, 252, 200, 252, 200, 252, 200, 252, 252, 252, 200, 145, 145, 145, 145, 0, 0,
				/*DISARM*/			 200, 0, 70, 55, 70, 0, 200, 0, 200, 0, 0, 0, 0, 0, 0, 0, 200, 0, 70, 55, 70, 0, 200, 0, 200, 0, 0, 0, 0, 0, 0, 0,
				/*DISARM_TRAPS*/	 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0,
				/*DIVINATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*DODGE*/			 175, 75, 155, 170, 155, 75, 200, 155, 210, 75, 75, 75, 75, 75, 0, 0, 175, 75, 155, 170, 155, 75, 200, 155, 210, 75, 75, 75, 75, 75, 0, 0,
				/*DOUBLE_ATTACK*/	 245, 0, 235, 245, 235, 0, 250, 0, 240, 0, 0, 0, 0, 0, 0, 0, 245, 0, 235, 245, 235, 0, 250, 0, 240, 0, 0, 0, 0, 0, 0, 0,
				/*DRAGON_PUNCH*/	 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*DUEL_WIELD*/		 245, 0, 0, 245, 0, 0, 252, 210, 245, 0, 0, 0, 0, 0, 0, 0, 245, 0, 0, 245, 0, 0, 252, 210, 245, 0, 0, 0, 0, 0, 0, 0,
				/*EAGLE_STRIKE*/	 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*EVOCATION*/		 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*FEIGN_DEATH*/		 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*FLYING_KICK*/		 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*FORAGE*/			 0, 0, 0, 200, 0, 200, 50, 55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 200, 50, 55, 0, 0, 0, 0, 0, 0, 0, 0,
				/*HAND_TO_HAND*/	 100, 75, 100, 100, 100, 75, 252, 100, 100, 75, 75, 75, 75, 75, 0, 0, 100, 75, 100, 100, 100, 75, 252, 100, 100, 75, 75, 75, 75, 75, 0, 0,
				/*HIDE*/			 0, 0, 0, 75, 75, 0, 0, 40, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 75, 75, 0, 0, 40, 200, 0, 0, 0, 0, 0, 0, 0,
				/*KICK*/			 210, 0, 0, 205, 0, 0, 250, 0, 0, 0, 0, 0, 0, 0, 0, 0, 210, 0, 0, 205, 0, 0, 250, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*MEDITATE*/		 0, 252, 235, 226, 235, 252, 0, 1, 0, 252, 252, 252, 252, 252, 0, 0, 0, 252, 235, 226, 235, 252, 0, 1, 0, 252, 252, 252, 252, 252, 0, 0,
				/*MEND*/			 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*OFFENSE*/			 252, 200, 225, 252, 230, 200, 252, 225, 252, 200, 140, 140, 140, 140, 0, 0, 252, 200, 225, 252, 230, 200, 252, 225, 252, 200, 140, 140, 140, 140, 0, 0,
				/*PARRY*/			 230, 0, 205, 220, 205, 0, 0, 75, 230, 0, 0, 0, 0, 0, 0, 0, 230, 0, 205, 220, 205, 0, 0, 75, 230, 0, 0, 0, 0, 0, 0, 0,
				/*PICK_LOCK*/		 0, 0, 0, 0, 0, 0, 0, 100, 210, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 210, 0, 0, 0, 0, 0, 0, 0,
				/*PIERCING*/		 240, 0, 225, 240, 225, 0, 0, 225, 250, 200, 110, 110, 110, 110, 0, 0, 240, 0, 225, 240, 225, 0, 0, 225, 250, 200, 110, 110, 110, 110, 0, 0,
				/*RIPOSTE*/			 225, 0, 200, 150, 200, 0, 225, 75, 225, 0, 0, 0, 0, 0, 0, 0, 225, 0, 200, 150, 200, 0, 225, 75, 225, 0, 0, 0, 0, 0, 0, 0,
				/*ROUND_KICK*/		 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 225, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*SAFE_FALL*/		 0, 0, 0, 0, 0, 0, 200, 40, 94, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 40, 94, 0, 0, 0, 0, 0, 0, 0,
				/*SENSE_HEADING*/	 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*SINGING*/			 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*SNEAK*/			 0, 0, 0, 75, 0, 0, 113, 75, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 75, 0, 0, 113, 75, 200, 0, 0, 0, 0, 0, 0, 0,
				/*SPECIALIZE_ABJ*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*SPECIALIZE_ALT*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*SPECIALIZE_CON*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*SPECIALIZE_DIV*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*SPECIALIZE_EVO*/	 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 235, 0, 0, 0, 235, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0,
				/*PICK_POCKETS*/	 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 200, 0, 0, 0, 0, 0, 0, 0,
				/*STRINGED_INSTRU*/	 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*SWIMMING*/		 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*THROWING*/		 200, 0, 0, 113, 0, 0, 200, 113, 250, 0, 75, 75, 75, 75, 0, 0, 200, 0, 0, 113, 0, 0, 200, 113, 250, 0, 75, 75, 75, 75, 0, 0,
				/*TIGER_CLAW*/		 0, 0, 0, 0, 0, 0, 252, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 252, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*TRACKING*/		 0, 0, 0, 200, 0, 125, 0, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 125, 0, 100, 0, 0, 0, 0, 0, 0, 0, 0,
				/*WIND_INSTRUMENTS*/ 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*SKILL_FISHING*/	 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*MAKE_POISON*/		 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0,
				/*TINKERING*/		 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*RESEARCH*/		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 200, 200, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 200, 200, 200, 0, 0,
				/*ALCHEMY*/			 0, 0, 0, 0, 0, 0, 0, 0, 0, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 180, 0, 0, 0, 0, 0, 0,
				/*BAKING*/			 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*TAILORING*/		 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*SENSE_TRAPS*/		 0, 0, 0, 0, 0, 0, 0, 75, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 75, 200, 0, 0, 0, 0, 0, 0, 0,
				/*BLACKSMITHING*/	 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*FLETCHING*/		 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*BREWING*/			 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*ALCOHOL_TOL*/		 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*BEGGING*/			 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 0, 0,
				/*JEWELRY_MAKING*/	 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*POTTERY*/			 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0, 0,
				/*PERCUSSION_INSTR*/ 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 235, 0, 0, 0, 0, 0, 0, 0, 0,
				/*INTIMIDATION*/	 0, 0, 0, 0, 0, 0, 200, 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 100, 200, 0, 0, 0, 0, 0, 0, 0,
				/*BERSERKING*/		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				/*TAUNT*/			 200, 0, 180, 150, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 180, 150, 180, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	//Yeahlight: Check for racial skills first
	switch(race) 
	{
		case BARBARIAN:
		{
			//Melinko: Bash and slam are directly linked
			if(skillid == BASH){
				if(eqclass == WARRIOR){
					break;
				}else{
					return 1;	
				}
			}
		}
		case WOOD_ELF:
		{
			if(skillid == FORAGE)
			{
				if(eqclass == 4 || eqclass == 6 || eqclass == 8 || eqclass == 20 || eqclass == 22 || eqclass == 24)
					break;
				else 
				{
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CheckMaxSkill(skillid = %i, race = %i, eqclass = %i, level = %i) returned %i", skillid, race, eqclass, level, 50);
					return 50;
				}
			}
			if(skillid == HIDE)
			{
				if(eqclass == 4 || eqclass == 5 || eqclass == 9 || eqclass == 20 || eqclass == 21 || eqclass == 25)
					break;
				else 
				{
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CheckMaxSkill(skillid = %i, race = %i, eqclass = %i, level = %i) returned %i", skillid, race, eqclass, level, 50);
					return 50;
				}
			}
		}
		case DARK_ELF:
		{
			if(skillid == HIDE)
			{
				if(eqclass == 4 || eqclass == 5 || eqclass == 9 || eqclass == 20 || eqclass == 21 || eqclass == 25)
					break;
				else 
				{
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CheckMaxSkill(skillid = %i, race = %i, eqclass = %i, level = %i) returned %i", skillid, race, eqclass, level, 50);
					return 50;
				}
			}
		}
		case TROLL:
		{
			//Melinko: Bash and slam are directly linked
			if(skillid == BASH){
				if(eqclass == WARRIOR || eqclass == SHADOWKNIGHT){
					break;
				}else{
					return 1;	
				}
			}
		}
		case OGRE:
		{
			//Melinko: Bash and slam are directly linked
			if(skillid == BASH){
				if(eqclass == WARRIOR || eqclass == SHADOWKNIGHT){
					break;
				}else{
					return 1;	
				}
			}
		}
		case HALFLING:
		{
			if(skillid == HIDE)
			{
				if(eqclass == 4 || eqclass == 5 || eqclass == 9 || eqclass == 20 || eqclass == 21 || eqclass == 25)
					break;
				else 
				{
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CheckMaxSkill(skillid = %i, race = %i, eqclass = %i, level = %i) returned %i", skillid, race, eqclass, level, 50);
					return 50;
				}
			}
			if(skillid == SNEAK)
			{
				if(eqclass == 4 || eqclass == 7 || eqclass == 8 || eqclass == 9 || eqclass == 20 || eqclass == 23 || eqclass == 24 || eqclass == 25)
					break;
				else 
				{
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CheckMaxSkill(skillid = %i, race = %i, eqclass = %i, level = %i) returned %i", skillid, race, eqclass, level, 50);
					return 50;
				}
			}
		}
  		case IKSAR:
		{
			if(skillid == FORAGE)
			{
				if(eqclass == 4 || eqclass == 6 || eqclass == 8 || eqclass == 20 || eqclass == 22 || eqclass == 24)
					break;
				else {
					CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CheckMaxSkill(skillid = %i, race = %i, eqclass = %i, level = %i) returned %i", skillid, race, eqclass, level, 50);
					return 50;
				}
			}
		}
	}

	//Yeahlight: Next, special check for trade skills to make sure only one skill is above 200
	//           Note: There is no level requirement for trade skills. A level 1 can get 200+ in any trade skill
	if(this->IsClient() && (skillid == BLACKSMITHING || skillid == BREWING || skillid == BAKING || skillid == FLETCHING || skillid == JEWELRY_MAKING || skillid == POTTERY || skillid == TAILORING || skillid == TINKERING))
	{
		int16 skill = (int16)this->CastToClient()->GetSkill(skillid);
		//Yeahlight: Skill is already above 200, send back permission for 250
		if(skill > 200)
			return 250;
		if(this->CastToClient()->GetSkill(BLACKSMITHING) > 200 && skill != BLACKSMITHING)
			return 200;
		if(this->CastToClient()->GetSkill(BREWING) > 200 && skill != BREWING)
			return 200;
		if(this->CastToClient()->GetSkill(BAKING) > 200 && skill != BAKING)
			return 200;
		if(this->CastToClient()->GetSkill(FLETCHING) > 200 && skill != FLETCHING)
			return 200;
		if(this->CastToClient()->GetSkill(JEWELRY_MAKING) > 200 && skill != JEWELRY_MAKING)
			return 200;
		if(this->CastToClient()->GetSkill(POTTERY) > 200 && skill != POTTERY)
			return 200;
		if(this->CastToClient()->GetSkill(TAILORING) > 200 && skill != TAILORING)
			return 200;
		if(this->CastToClient()->GetSkill(TINKERING) > 200 && skill != TINKERING)
			return 200;
		return 250;
	}

	//Yeahlight: Now that we have determined the skill is not an innate ability or a trade skill, find the maximum value for this character
	int16 levelSkillCap = 0;
	int16 maximumSkillCap = 0;
	//Yeahlight: Grant merchants and bankers the warrior skill set
	if(eqclass == MERCHANT || eqclass == BANKER)
		eqclass = WARRIOR;
	//Yeahlight: Mob is level 50 and under
	if(level <= 50)
	{
		levelSkillCap = (int)level * 5 + 5;
		maximumSkillCap = maxSkills[skillid][eqclass-1];
	}
	//Yeahlight: Mob is level 60 and under
	else if(level <= 60)
	{
		float gap = ((int)level - 50) * 5;
		maximumSkillCap = maxSkillsBeyond50[skillid][eqclass-1];
		levelSkillCap = maxSkills[skillid][eqclass-1] + gap;
		//Yeahlight: This skill has more than 50 potential points between 50 and 60
		if((maximumSkillCap - maxSkills[skillid][eqclass-1]) > 50)
		{
			gap = (float)((float)(maximumSkillCap - maxSkills[skillid][eqclass-1]) / 10.00) * ((int)level - 50);
			levelSkillCap = maxSkills[skillid][eqclass-1] + gap;
		}
	}
	//Yeahlight: Mob is over level 60
	else
	{
		if(maxSkillsBeyond50[skillid][eqclass-1] > 0)
			return maxSkillsBeyond50[skillid][eqclass-1] + (((int)level - 60) * 5);
		else
			return 0;
	}

	//Yeahlight: Level formula exceeds the class maximum
	if(levelSkillCap >= maximumSkillCap)
		return maximumSkillCap;

	CAST_CLIENT_DEBUG_PTR(this)->Log(CP_UPDATES, "Client::CheckMaxSkill(skillid = %i, race = %i, eqclass = %i, level = %i) returned %i", skillid, race, eqclass, level, levelSkillCap);

	return levelSkillCap;
}

//o--------------------------------------------------------------
//| GetItemHaste; Yeahlight, Nov 25, 2008
//o--------------------------------------------------------------
//| Returns the mob's highest worn haste item
//o--------------------------------------------------------------
int16 Mob::GetItemHaste()
{
	int16 highestHasteValue = 0;

	//Yeahlight: Iterate through the NPC's loot table
	if(this->IsNPC())
	{
		highestHasteValue = this->CastToNPC()->GetItemHaste();
	}
	//Yeahlight: Iterate through the client's worn inventory
	else if(this->IsClient())
	{
		const Item_Struct* item = NULL;

		//Yeahlight: Iterate through each item in the PC's inventory
		for(int i = 1; i < 21; i++)
		{
			//Yeahlight: Query the DB for the item
			item = Database::Instance()->GetItem(CastToClient()->pp.inventory[i]);

			//Yeahlight: Item exists and has a passive spell effect
			if(item && item->common.spell_effect_id && item->common.effecttype == 2)
			{
				//Yeahlight: Item has the "haste" spell effect
				if(item->common.spell_effect_id == 998)
				{
					//Yeahlight: This item is currently the highest level of item haste (haste value is stored in the item's 'level' field)
					if(GetLevel() >= item->common.level && item->common.level > highestHasteValue)
					{
						highestHasteValue = item->common.level;
					}
				}
				//Yeahlight: Item does not have the "haste" spell effect
				else
				{
					//Yeahlight: Grab the item's spell effect
					Spell* spell = spells_handler.GetSpellPtr(item->common.spell_effect_id);
					
					//Yeahlight: This item has a valid attack speed spell effect
					if(spell && spell->IsValidSpell() && spell->IsAttackSpeedSpell())
					{
						//Yeahlight: Grab the spells's haste value
						int16 amount = GetSpellAttackSpeedModifier(spell, GetLevel()) - 100;

						//Yeahlight: This spell effect is currently the highest level of item haste
						if(amount > highestHasteValue)
						{
							highestHasteValue = amount;
						}
					}
				}
			}
		}
	}

	return highestHasteValue;
}

/********************************************************************
 *                      Tazadar - 08/16/08                          *
 ********************************************************************
 *                      CalculateACBonuses	                        *
 ********************************************************************
 *  + Return the total amount of AC						            *
 *									                                *
 ********************************************************************/
int16 Mob::CalculateACBonuses()
{
	int avoidance = 0;
	int mitigation = 0;
	int displayed = 0;
	int16 AC = displayed;
	int16 defenseSkill = 0;
	if(this->IsClient())
		defenseSkill = this->CastToClient()->GetSkill(DEFENSE);
	else
		defenseSkill = this->GetSkill(DEFENSE);

	avoidance = (acmod() + monkacmod() + iksaracmod() + rogueacmod() + ((defenseSkill*16)/9));
	if (avoidance < 0)
		avoidance = 0;

	//Yeahlight: NPCs are given a 15% boost to avoidance AC
	if(this->IsNPC())
		this->myAvoidanceAC = avoidance * 1.15;
	else
		this->myAvoidanceAC = avoidance;

	if(this->IsClient())
	{
		if (GetClass() == WIZARD || GetClass() == MAGICIAN || GetClass() == NECROMANCER || GetClass() == ENCHANTER)
		{
			mitigation = defenseSkill/2 + (itembonuses.AC);
		}
		else
		{
			mitigation = defenseSkill/3 + ((itembonuses.AC*4)/3);
		}
	}
	else
	{
		//Yeahlight: NPCs don't have gear, so we need to take a guess at a reasonable substitute per level.
		//           Currently set to 6 AC per level.
		if (GetClass() == WIZARD || GetClass() == MAGICIAN || GetClass() == NECROMANCER || GetClass() == ENCHANTER)
		{
			mitigation = defenseSkill/2 + (GetLevel() * 6.00f);
		}
		else
		{
			mitigation = defenseSkill/3 + ((GetLevel() * 6.00f) * 4 / 3);
		}
	}
	if (mitigation < 0)
		mitigation = 0;

	//Yeahlight: NPCs are given a 5% boost to mitigation AC
	if(this->IsNPC())
		this->myMitigationAC = (mitigation * 1.05) + spellbonuses.AC;
	else
		this->myMitigationAC = (mitigation) + spellbonuses.AC;

	displayed += ((avoidance + mitigation)*1000)/847;

	//spell AC bonuses are added directly to natural total
	displayed += spellbonuses.AC;

	AC = displayed;
	
	if(GetAGI()<70)
		AC++;
	if(AC<=0)
		AC=1;
	return AC;
}

/********************************************************************
 *                      Tazadar - 08/16/08                          *
 ********************************************************************
 *                           acmod			                        *
 ********************************************************************
 *  + Natural AC Mod 									            *
 *									                                *
 ********************************************************************/
sint16 Mob::acmod() {
	int agility = GetAGI();
	int level = GetLevel();
	if(agility < 1 || level < 1)
		return(0);
	
	if (agility <=74){
		if (agility == 1)
			return -24;
		else if (agility <=3)
			return -23;
		else if (agility == 4)
			return -22;
		else if (agility <=6)
			return -21;
		else if (agility <=8)
			return -20;
		else if (agility == 9)
			return -19;
		else if (agility <=11)
			return -18;
		else if (agility == 12)
			return -17;
		else if (agility <=14)
			return -16;
		else if (agility <=16)
			return -15;
		else if (agility == 17)
			return -14;
		else if (agility <=19)
			return -13;
		else if (agility == 20)
			return -12;
		else if (agility <=22)
			return -11;
		else if (agility <=24)
			return -10;
		else if (agility == 25)
			return -9;
		else if (agility <=27)
			return -8;
		else if (agility == 28)
			return -7;
		else if (agility <=30)
			return -6;
		else if (agility <=32)
			return -5;
		else if (agility == 33)
			return -4;
		else if (agility <=35)
			return -3;
		else if (agility == 36)
			return -2;
		else if (agility <=38)
			return -1;
		else if (agility <=65)
			return 0;
		else if (agility <=70)
			return 1;
		else if (agility <=74)
			return 5;
	}
	else if(agility <= 137) {
		if (agility == 75){
			if (level <= 6)
				return 9;
			else if (level <= 19)
				return 23;
			else if (level <= 39)
				return 33;
			else
				return 39;
		}
		else if (agility >= 76 && agility <= 79){
			if (level <= 6)
				return 10;
			else if (level <= 19)
				return 23;
			else if (level <= 39)
				return 33;
			else
				return 40;
		}
		else if (agility == 80){
			if (level <= 6)
				return 11;
			else if (level <= 19)
				return 24;
			else if (level <= 39)
				return 34;
			else
				return 41;
		}
		else if (agility >= 81 && agility <= 85){
			if (level <= 6)
				return 12;
			else if (level <= 19)
				return 25;
			else if (level <= 39)
				return 35;
			else
				return 42;
		}
		else if (agility >= 86 && agility <= 90){
			if (level <= 6)
				return 12;
			else if (level <= 19)
				return 26;
			else if (level <= 39)
				return 36;
			else
				return 42;
		}
		else if (agility >= 91 && agility <= 95){
			if (level <= 6)
				return 13;
			else if (level <= 19)
				return 26;
			else if (level <= 39)
				return 36;
			else
				return 43;
		}
		else if (agility >= 96 && agility <= 99){
			if (level <= 6)
				return 14;
			else if (level <= 19)
				return 27;
			else if (level <= 39)
				return 37;
			else 
				return 44;
		}
		else if (agility == 100 && level >= 7){
			if (level <= 19)
				return 28;
			else if (level <= 39)
				return 38;
			else
				return 45;
		}
		else if (agility >= 101 && agility <= 105){
			if (level <= 6) {
				return 15;
			}
			else if (level <= 19)
				return 29;
			else if (level <= 39)
				return 39;
			else
				return 45;
		}
		else if (agility >= 106 && agility <= 110){
			if (level <= 6)
				return 16;
			else if (level <= 19)
				return 29;
			else if (level <= 39)
				return 39;
			else
				return 46;
		}
		else if (agility >= 111 && agility <= 115){
			if (level <= 6)
				return 17;
			else if (level <= 19)
				return 30;
			else if (level <= 39)
				return 40;
			else
				return 47;
		}
		else if (agility >= 116 && agility <= 119){
			if (level <= 6)
				return 17;
			else if (level <= 19)
				return 31;
			else if (level <= 39)
				return 41;
			else
				return 47;
		}
		else if (agility == 120){
			if (level <= 6)
				return 18;
			else if (level <= 19)
				return 32;
			if (level <= 39)
				return 42;
			else
				return 48;
		}
		else if (agility <= 125){
			if (level <= 6)
				return 19;
			else if (level <= 19)
				return 32;
			if (level <= 39)
				return 42;
			else
				return 49;
		}
		else if (agility <= 130){
			if (level <= 6)
				return 20;
			else if (level <= 19)
				return 33;
			if (level <= 39)
				return 43;
			else
				return 50;
		}
		else if (agility <= 135){
			if (level <= 6)
				return 20;
			else if (level <= 19)
				return 34;
			if (level <= 39)
				return 44;
			else
				return 50;
		}
		else {
			if (level <= 6)
				return 21;
			else if (level <= 19)
				return 34;
			if (level <= 39)
				return 44;
			else
				return 51;
		}
	} else if(agility <= 300) {
		if(level <= 6) {
			if(agility <= 139)
				return(21);
			else if(agility == 140)
				return(22);
			else if(agility <= 145)
				return(23);
			else if(agility <= 150)
				return(23);
			else if(agility <= 155)
				return(24);
			else if(agility <= 159)
				return(25);
			else if(agility == 160)
				return(26);
			else if(agility <= 165)
				return(26);
			else if(agility <= 170)
				return(27);
			else if(agility <= 175)
				return(28);
			else if(agility <= 179)
				return(28);
			else if(agility == 180)
				return(29);
			else if(agility <= 185)
				return(30);
			else if(agility <= 190)
				return(31);
			else if(agility <= 195)
				return(31);
			else if(agility <= 199)
				return(32);
			else if(agility <= 219)
				return(33);
			else if(agility <= 239)
				return(34);
			else
				return(35);
		} else if(level <= 19) {
			if(agility <= 139)
				return(34);
			else if(agility == 140)
				return(35);
			else if(agility <= 145)
				return(36);
			else if(agility <= 150)
				return(37);
			else if(agility <= 155)
				return(37);
			else if(agility <= 159)
				return(38);
			else if(agility == 160)
				return(39);
			else if(agility <= 165)
				return(40);
			else if(agility <= 170)
				return(40);
			else if(agility <= 175)
				return(41);
			else if(agility <= 179)
				return(42);
			else if(agility == 180)
				return(43);
			else if(agility <= 185)
				return(43);
			else if(agility <= 190)
				return(44);
			else if(agility <= 195)
				return(45);
			else if(agility <= 199)
				return(45);
			else if(agility <= 219)
				return(46);
			else if(agility <= 239)
				return(47);
			else
				return(48);
		} else if(level <= 39) {
			if(agility <= 139)
				return(44);
			else if(agility == 140)
				return(45);
			else if(agility <= 145)
				return(46);
			else if(agility <= 150)
				return(47);
			else if(agility <= 155)
				return(47);
			else if(agility <= 159)
				return(48);
			else if(agility == 160)
				return(49);
			else if(agility <= 165)
				return(50);
			else if(agility <= 170)
				return(50);
			else if(agility <= 175)
				return(51);
			else if(agility <= 179)
				return(52);
			else if(agility == 180)
				return(53);
			else if(agility <= 185)
				return(53);
			else if(agility <= 190)
				return(54);
			else if(agility <= 195)
				return(55);
			else if(agility <= 199)
				return(55);
			else if(agility <= 219)
				return(56);
			else if(agility <= 239)
				return(57);
			else
				return(58);
		} else {	//lvl >= 40
			if(agility <= 139)
				return(51);
			else if(agility == 140)
				return(52);
			else if(agility <= 145)
				return(53);
			else if(agility <= 150)
				return(53);
			else if(agility <= 155)
				return(54);
			else if(agility <= 159)
				return(55);
			else if(agility == 160)
				return(56);
			else if(agility <= 165)
				return(56);
			else if(agility <= 170)
				return(57);
			else if(agility <= 175)
				return(58);
			else if(agility <= 179)
				return(58);
			else if(agility == 180)
				return(59);
			else if(agility <= 185)
				return(60);
			else if(agility <= 190)
				return(61);
			else if(agility <= 195)
				return(61);
			else if(agility <= 199)
				return(62);
			else if(agility <= 219)
				return(63);
			else if(agility <= 239)
				return(64);
			else
				return(65);
		}
	}
	cout<< "Error in Client::acmod(): Agility: "<<agility<<", Level: "<<level<<endl;
	return 0;
}

/********************************************************************
 *                      Tazadar - 08/16/08                          *
 ********************************************************************
 *                         monkacmod                                *
 ********************************************************************
 *  + Mod for monk AC									            *
 *									                                *
 ********************************************************************/
sint16 Mob::monkacmod() {
	if(class_!=MONK){
		return 0;
	}
	int level = GetLevel();
	switch (level)
	{
	case 1:
		{
			return 7;
		}
	case 2:
		{
			return 9;
		}
	case 3:
		{
			return 10;
		}
	case 4:
		{
			return 11;
		}
	case 5:
		{
			return 13;
		}
	case 6:
		{
			return 14;
		}
	case 7:
		{
			return 15;
		}
	case 8:
		{
			return 17;
		}
	case 9:
		{
			return 18;
		}
	case 10:
		{
			return 19;
		}
	case 11:
		{
			return 21;
		}
	case 12:
		{
			return 22;
		}
	case 13:
		{
			return 23;
		}
	case 14:
		{
			return 25;
		}
	case 15:
		{
			return 26;
		}
	case 16:
		{
			return 27;
		}
	case 17:
		{
			return 29;
		}
	case 18:
		{
			return 30;
		}
	case 19:
		{
			return 31;
		}
	case 20:
		{
			return 33;
		}
	case 21:
		{
			return 34;
		}
	case 22:
		{
			return 35;
		}
	case 23:
		{
			return 37;
		}
	case 24:
		{
			return 38;
		}
	case 25:
		{
			return 39;
		}
	case 26:
		{
			return 41;
		}
	case 27:
		{
			return 42;
		}
	case 28:
		{
			return 43;
		}
	case 29:
		{
			return 45;
		}
	case 30:
		{
			return 46;
		}
	case 31:
		{
			return 47;
		}
	case 32:
		{
			return 49;
		}
	case 33:
		{
			return 50;
		}
	case 34:
		{
			return 51;
		}
	case 35:
		{
			return 53;
		}
	case 36:
		{
			return 54;
		}
	case 37:
		{
			return 55;
		}
	case 38:
		{
			return 57;
		}
	case 39:
		{
			return 58;
		}
	case 40:
		{
			return 59;
		}
	case 41:
		{
			return 61;
		}
	case 42:
		{
			return 62;
		}
	case 43:
		{
			return 63;
		}
	case 44:
		{
			return 65;
		}
	case 45:
		{
			return 66;
		}
	case 46:
		{
			return 67;
		}
	case 47:
		{
			return 69;
		}
	case 48:
		{
			return 70;
		}
	case 49:
		{
			return 71;
		}
	case 50:
		{
			return 73;
		}
	case 51:
		{
			return 74;
		}
	case 52:
		{
			return 75;
		}
	case 53:
		{
			return 77;
		}
	case 54:
		{
			return 78;
		}
	case 55:
		{
			return 79;
		}
	case 56:
		{
			return 81;
		}
	case 57:
		{
			return 82;
		}
	case 58:
		{
			return 83;
		}
	case 59:
		{
			return 85;
		}
	case 60:
		{
			return 86;
		}
	default:
		{
			cout << "monkacmod() error : Wrong Level ! Level :" << (int)level << endl;
			return 0;
		}
	}
}

/********************************************************************
 *                      Tazadar - 08/16/08                          *
 ********************************************************************
 *                         rogueacmod                               *
 ********************************************************************
 *  + Mod for rogue AC									            *
 *									                                *
 ********************************************************************/
sint16 Mob::rogueacmod() {
	int level = GetLevel();
	if(class_!=ROGUE || level < 30){
		return 0;
	}
	int agility = GetAGI();
	if(agility <= 75)
		return 0;
	
	if(agility <= 79){
		if(level<=33)
			return 1;
		else if(level<=37)
			return 2;
		else if(level<=41)
			return 3;
		else if(level<=45)
			return 4;
		else if(level<=49)
			return 5;
		else if(level<=53)
			return 6;
		else if(level<=57)
			return 7;
		else
			return 8;
	}
	else if(agility <= 84){
		if(level<=31)
			return 2;
		else if(level<=33)
			return 3;
		else if(level<=35)
			return 4;
		else if(level<=37)
			return 5;
		else if(level<=39)
			return 6;
		else if(level<=41)
			return 7;
		else if(level<=43)
			return 8;
		else if(level<=45)
			return 9;
		else if(level<=47)
			return 10;
		else if(level<=49)
			return 11;
		else
			return 12;
	}
	else if(agility <= 89){
		if(level<=31)
			return 3;
		else if(level<=32)
			return 4;
		else if(level<=33)
			return 5;
		else if(level<=35)
			return 6;
		else if(level<=36)
			return 7;
		else if(level<=37)
			return 8;
		else if(level<=39)
			return 9;
		else if(level<=40)
			return 10;
		else if(level<=41)
			return 11;
		else
			return 12;
	}
	else if(agility <= 99){
		if(level<=30)
			return 4;
		else if(level<=31)
			return 5;
		else if(level<=32)
			return 6;
		else if(level<=33)
			return 7;
		else if(level<=34)
			return 8;
		else if(level<=35)
			return 9;
		else if(level<=36)
			return 10;
		else if(level<=37)
			return 11;
		else
			return 12;
	}
	else{
		if(level<=30)
			return 5;
		else if(level<=31)
			return 6;
		else if(level<=32)
			return 7;
		else if(level<=33)
			return 8;
		else if(level<=34)
			return 10;
		else if(level<=35)
			return 11;
		else
			return 12;
	}
	cout << "rogueacmod() error !"<< endl;
	return 0;
}

/********************************************************************
 *                      Tazadar - 08/16/08                          *
 ********************************************************************
 *                         iksaracmod                               *
 ********************************************************************
 *  + Mod for iksar AC									            *
 *									                                *
 ********************************************************************/
sint16 Mob::iksaracmod() {
	if(GetRace()!=IKSAR){
		return 0;
	}
	int level = GetLevel();
	if(level <= 10){
		return 10;
	}
	if(level >=10 && level<36){
		return level;
	}
	return 35;
}

//o--------------------------------------------------------------
//| GetMeleeReach; Yeahlight, Dec 13, 2008
//o--------------------------------------------------------------
//| Returns the melee reach of the mob
//o--------------------------------------------------------------
int16 Mob::GetMeleeReach()
{
	return (4 + sqrt(float(HIT_BOX_MULTIPLIER * this->GetHitBox())));
}

//o--------------------------------------------------------------
//| SendWearChange; Yeahlight, Dec 14, 2008
//o--------------------------------------------------------------
//| Updates the textuers for each visible slot on the mob
//o--------------------------------------------------------------
void Mob::SendWearChange(int8 material_slot)
{
	if(material_slot > 8)
	{
		for(int i = 0; i < 9; i++)
		{
			APPLAYER* outapp = new APPLAYER(OP_WearChange, sizeof(WearChange_Struct));
			WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

			wc->spawn_id = GetID();
			wc->slot_graphic = equipment[i];
			wc->wear_slot_id = i;
			wc->color = equipmentColor[i];

			entity_list.QueueCloseClients(this, outapp);
			safe_delete(outapp);
		}
	}
	else
	{
		APPLAYER* outapp = new APPLAYER(OP_WearChange, sizeof(WearChange_Struct));
		WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

		wc->spawn_id = GetID();
		wc->slot_graphic = equipment[material_slot];
		wc->wear_slot_id = material_slot;
		wc->color = equipmentColor[material_slot];

		entity_list.QueueCloseClients(this, outapp);
		safe_delete(outapp);
	}
}

//o--------------------------------------------------------------
//| GetSlotOfItem; Yeahlight, Dec 16, 2008
//o--------------------------------------------------------------
//| Returns the most likely visible slot for the provided item
//o--------------------------------------------------------------
int8 Mob::GetSlotOfItem(Item_Struct* item)
{
	if(!item)
		return 99;
	//Yeahlight: Rip the bitmask apart and seperate it into slots
	int16 slots[22] = {0};
	int itemSlots = item->equipableSlots;
	int16 counter = 0;
	int16 slotToUse = 99;
	//Yeahlight: Keep iterating until we shift all the bits off the end
	while(itemSlots > 0)
	{
		if(itemSlots % 2)
			slots[counter] = 1;
		itemSlots = itemSlots >> 1;
		counter++;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	//Yeahlight: Item fits in primary
	if(slots[13])
	{
		//Yeahlight: Item is a two-hand weapon, so the item will go in the primary slot
		if(item->common.itemType == DB_2H_SLASHING || item->common.itemType == DB_2H_BLUNT || item->common.itemType == DB_2H_PIERCING)
		{
			slotToUse = 7;
		}
		//Yeahlight: Mob is already wielding a one-hand primary weapon and this item fits in secondary
		//           Note: I put a level cap of 60 to give NPCs off hand weapons. We do not want players
		//                 giving raid bosses another weapon to enable dual wield
		else if(equipment[7] && slots[14] && GetLevel() <= 60 && GetMyMainHandWeapon()->common.itemType != DB_2H_SLASHING && GetMyMainHandWeapon()->common.itemType != DB_2H_BLUNT && GetMyMainHandWeapon()->common.itemType != DB_2H_PIERCING)
		{
			//Yeahlight: Secondary slot is empty
			if(!equipment[8])
				slotToUse = 8;
			else
				slotToUse = rand()%2 + 7;
		}
		else
		{
			slotToUse = 7;
		}
	}
	//Yeahlight: Item fits in secondary
	else if(slots[14])
	{
		slotToUse = 8;
	}
	//Yeahlight: Item fits on head
	else if(slots[2])
	{
		slotToUse = 0;
	}
	//Yeahlight: Item fits on chest
	else if(slots[17])
	{
		slotToUse = 1;
	}
	//Yeahlight: Item fits on arms
	else if(slots[7])
	{
		slotToUse = 2;
	}
	//Yeahlight: Item fits on wrist
	else if(slots[10])
	{
		slotToUse = 3;
	}
	//Yeahlight: Item fits on hands
	else if(slots[12])
	{
		slotToUse = 4;
	}
	//Yeahlight: Item fits on legs
	else if(slots[18])
	{
		slotToUse = 5;
	}
	//Yeahlight: Item fits on feet
	else if(slots[19])
	{
		slotToUse = 6;
	}
	return slotToUse;
}

//o--------------------------------------------------------------
//| GetDefaultSize; Yeahlight, Dec 19, 2008
//o--------------------------------------------------------------
//| Returns the default starting size of PC races
//o--------------------------------------------------------------
float Mob::GetDefaultSize()
{
	switch(GetRace())
	{
		case GNOME: case HALFLING: 
			return 3;
			break;
		case DWARF:
			return 4;
			break;
		case WOOD_ELF: case DARK_ELF: case HALF_ELF:
			return 5;
			break;
		case HUMAN: case ERUDITE: case HIGH_ELF: case IKSAR:
			return 6;
			break;
		case BARBARIAN: 
			return 7;
			break;
		case TROLL:
			return 8;
			break;
		case OGRE:
			return 9;
			break;
		default:
			return GetBaseSize();
	}
}

//o--------------------------------------------------------------
//| CancelAllInvisibility; Yeahlight, June 25, 2009
//o--------------------------------------------------------------
//| Handles fading all forms of invisibility on a mob
//o--------------------------------------------------------------
void Mob::CancelAllInvisibility()
{
	if(GetSpellInvis())
		BuffFadeByEffect(SE_Invisibility);
	if(GetInvisibleUndead())
		BuffFadeByEffect(SE_InvisVsUndead);
	if(GetInvisibleAnimal())
		BuffFadeByEffect(SE_InvisVsAnimals);
	if(IsClient() && CastToClient()->GetHide())
		CastToClient()->SetHide(false);
}

// Harakiri mob says something, used for quest
void Mob::Say(const char *pText, ...)
{
    char* data=0;
	MakeAnyLenString(&data,pText);
	va_list args;
	va_start(args, data);		
	entity_list.ChannelMessage(this, MessageChannel_SAY, LANGUAGE_COMMON_TONGUE, data,args);
	va_end(args);
}

// Harakiri mob shouts something, used for quest
void Mob::Shout(const char *pText, ...)
{
    char* data=0;
	MakeAnyLenString(&data,pText);
	va_list args;
	va_start(args, data);		
	entity_list.ChannelMessage(this, MessageChannel_SHOUT, LANGUAGE_COMMON_TONGUE, data,args);
	va_end(args);
}

// Harakiri mob emotes something, used for quest
void Mob::Emote(const char *pText, ...)
{
    char* data=0;
	MakeAnyLenString(&data,"%s %s", IsNPC() ? CastToNPC()->GetProperName() : GetName(),pText);
	va_list args;
	va_start(args, data);				
	entity_list.MessageClose(this,false,200,EMOTE, data,args);		
	va_end(args);
}
// Harakiri hp event 
void Mob::SetNextHPEvent( int hpevent ) 
{ 
	nexthpevent = hpevent;
}

void Mob::SetNextIncHPEvent( int inchpevent ) 
{ 
	nextinchpevent = inchpevent;
}