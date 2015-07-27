/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "../common/debug.h"
#include <math.h>
#include <stdlib.h>
#include "mob.h"
#include "client.h"
#include "PlayerCorpse.h"
#include "zone.h"
#include "spdat.h"
#include "skills.h"

#include "../common/eq_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "../common/database.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"

#include <stdio.h>
extern EntityList entity_list;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern Database database;
extern Zone* zone;

#define INTERACTIVE

Mob::Mob(const char*   in_name,
         const char*   in_lastname,
         sint32  in_cur_hp,
         sint32  in_max_hp,
         int8    in_gender,
         uint16    in_race,
         int8    in_class,
         int8    in_bodytype,   // neotokyo targettype support 17-Nov-02
         int8    in_deity,
         int8    in_level,
         int32	 in_npctype_id, // rembrant, Dec. 20, 2001
         const int8*	 in_skills, // socket 12-29-01
		 float	in_size,
		 float	in_walkspeed,
		 float	in_runspeed,
         float	in_heading,
         float	in_x_pos,
         float	in_y_pos,
         float	in_z_pos,

         int8    in_light,
         const int8*   in_equipment,
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
		 int8	in_haircolor,
		 int8	in_beardcolor,
		 int8	in_eyecolor1, // the eyecolors always seem to be the same, maybe left and right eye?
		 int8	in_eyecolor2,
		 int8	in_hairstyle,
		 int8	in_title, //Face Overlay? (barbarian only)
		 int8	in_luclinface, // and beard
		 float	in_fixed_z,
		 int16	in_d_meele_texture1,
		 int16	in_d_meele_texture2
		 )
{
	AI_Init();
	adverrorinfo = 0;
	strncpy(name,in_name,64);
	strncpy(lastname,in_lastname,64);
	cur_hp		= in_cur_hp;
	max_hp		= in_max_hp;
	base_hp		= in_max_hp;
	gender		= in_gender;
	race		= in_race;
	base_gender	= in_gender;
	base_race	= in_race;
	class_		= in_class;
    bodytype    = in_bodytype;
	deity		= in_deity;
	level		= in_level;
	npctype_id	= in_npctype_id; // rembrant, Dec. 20, 2001
	size		= in_size;
	walkspeed	= (in_walkspeed > 0)?in_walkspeed:0.7f;
	runspeed	= (in_runspeed > 0)?in_runspeed:1.25f;

    // neotokyo: sanity check
    if (walkspeed < 0 || walkspeed > 10)
        walkspeed = 0.7f;
    if (runspeed < 0 || runspeed > 20)
        runspeed = 1.25f;

	heading		= in_heading;
	x_pos		= in_x_pos;
	y_pos		= in_y_pos;
	z_pos		= in_z_pos;
//	fixedZ		= in_fixed_z;
	light		= in_light;
	texture		= in_texture;
	helmtexture	= in_helmtexture;
	d_meele_texture1 = in_d_meele_texture1;
	d_meele_texture2= in_d_meele_texture2;
	haircolor	= in_haircolor;
	beardcolor	= in_beardcolor;
	eyecolor1	= in_eyecolor1;
	eyecolor2	= in_eyecolor2;
	hairstyle	= in_hairstyle;
	title		= in_title;


	AC		= in_ac;
	ATK		= in_atk;
	STR		= in_str;
	STA		= in_sta;
	DEX		= in_dex;
	AGI		= in_agi;
	#ifdef GUILDWARS
	  if (in_int<=75)
	    INT 	= 105;
	  else
	    INT		= in_int;
	#else
	  INT		= in_int;
	#endif
	WIS		= in_wis;
	CHA		= in_cha;

	NPCTypedata = 0;
	HastePercentage = 0;
	bEnraged = false;

	cur_mana = 0;
	max_mana = 0;
	hp_regen = 0;
	mana_regen = 0;
	invisible = false;
	invisible_undead = false;
	sneaking = false;
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

	/*	for (i=0; i<74; i++) { // socket 12-29-01
	if (in_skills == 0) {
	skills[i] =0;
	}
	else {
	skills[i] = in_skills[i];
	}
		 }*/ // Quag: dont understand why a memcpy wont do the job here =P
	if (in_skills) {
		memcpy(skills, in_skills, sizeof(skills));
	}
	else {
		memset(skills, 0, sizeof(skills));
	}
	for (int j = 0; j < BUFF_COUNT; j++) {
		buffs[j].spellid = 0xFFFF;
	}

    // clear the proc array
    RemoveProcFromWeapon(0, true);
#ifdef WIN32
	for (j = 0; j < MAX_PROCS; j++)
#else
    for (int j = 0; j < MAX_PROCS; j++)
#endif
    {
        PermaProcs[j].spellID = 0xFFFF;
        PermaProcs[j].chance = 0;
        PermaProcs[j].pTimer = NULL;
    }

	delta_heading = 0;
	delta_x = 0;
	delta_y = 0;
	delta_z = 0;

	invulnerable = false;
	isgrouped = false;
	appearance = 0;
	pRunAnimSpeed = 0;
	guildeqid = 0xFFFFFFFF;

	attack_timer = new Timer(2000);
	attack_timer_dw = new Timer(2000);
	tic_timer = new Timer(6000);
	mana_timer = new Timer(5000);
	spellend_timer = new Timer(0);
    spellend_timer->Disable();
	bardsong_timer = new Timer(6000);
	bardsong_timer->Disable();
	bardsong = 0;
	casting_spell_id = 0;
	target = 0;

	itembonuses = new StatBonuses;
	spellbonuses = new StatBonuses;
	memset(itembonuses, 0, sizeof(StatBonuses));
	memset(spellbonuses, 0, sizeof(StatBonuses));
	spellbonuses->ArrgoRange = -1;
	spellbonuses->AssistRange = -1;
	pLastChange = 0;
	this->SetPetID(0);
	this->SetOwnerID(0);
	typeofpet = 0xFF;  // means is not a pet; 0xFF means charmed
    this->SetFamiliarID(0);
	SaveSpawnSpot();

	isattacked = false;
	isinterrupted = false;
	mezzed = false;
	stunned = false;
	stunned_timer = new Timer(0);
	rune = 0;
    magicrune = 0;
	for (int m = 0; m < 60; m++) {
		flag[m]=0;
	}
	for (i=0; i<SPECATK_MAXNUM ; i++) {
		SpecAttacks[i] = false;
		SpecAttackTimers[i] = 0;
	}
    memset( RampageArray, 0, sizeof(RampageArray));
	wandertype=0;
	pausetype=0;
	max_wp=0;
	cur_wp=0;
	patrol=0;
	follow=0;

	roamer = false;
	rooted = false;
    guard_x = 0;
	guard_y = 0;
	guard_z = 0;
	guard_heading = 0;
    spawn_x = 0;
	spawn_y = 0;
	spawn_z = 0;
	spawn_heading = 0;
	pStandingPetOrder = SPO_Follow;
	
	// Bind wound
	bindwound_timer = new Timer(10000);
	bindwound_timer->Disable();
	bindwound_target = 0;

}

Mob::~Mob()
{
	AI_Stop();
	delete stunned_timer;
	SetPet(0);
	safe_delete(itembonuses);
	safe_delete(spellbonuses);
	safe_delete(spellend_timer);
	safe_delete(tic_timer);
	safe_delete(bardsong_timer);
	for (int i=0; i<SPECATK_MAXNUM ; i++) {
		safe_delete(SpecAttackTimers[i]);
	}
	if (attack_timer != 0) {
		delete attack_timer;
		attack_timer = 0;
	}
	if (mana_timer != 0) {
		delete mana_timer;
		mana_timer = 0;
	}
	// Kaiyodo - destruction of dual wield attack timer
	if(attack_timer_dw != 0)
	{
		delete attack_timer_dw;
		attack_timer_dw = 0;
	}
	safe_delete(bindwound_timer);
	APPLAYER app;
	CreateDespawnPacket(&app);
	entity_list.QueueClients(this, &app, true);
	entity_list.RemoveFromTargets(this);
}

int32 Mob::GetAppearanceValue(int8 iAppearance) {
	switch (iAppearance) {
		// 0 standing, 1 sitting, 2 ducking, 3 lieing down, 4 looting
		case 0: {
			return 100;
		}
		case 1: {
			return 110;
		}
		case 2: {
			return 111;
		}
		case 3: {
			return 115;
		}
		case 4: {
			return 105;
		}
		default: {
			return 100;
		}
	}
}

bool Mob::SeeInvisible()
{
    if (IsNPC())
    {
		// TODO
        // NPC *thisnpc = this->CastToNPC();
        // TODO: define Undead here and return true
    }
    else
    {
        // TODO: check buffs
        // TODO: check items
    }
    return false;
}

bool Mob::SeeInvisibleUndead()
{
    if (IsNPC())
    {
		// TODO
        // NPC *thisnpc = this->CastToNPC();
        // TODO: define !Undead here and return true
    }
    return true;
}

float Mob::GetWalkspeed()
{
    float aa_speed = 1.0f;
    if (IsClient()){
        uint8 *aa_item = &(((uint8 *)&this->CastToClient()->aa)[13]);
        if (*aa_item > 0 && *aa_item < 4){
            aa_speed += (float) ((100+*aa_item)/100);
        }
    }
    if (IsRooted())
        return 0.0f;
    if (spellbonuses->movementspeed)
        return (walkspeed * (100+spellbonuses->movementspeed)) / 100;
    else
        return (walkspeed * aa_speed);
}

float Mob::GetRunspeed()
{
    float aa_speed = 1.0f;
    if (IsClient()){
        uint8 *aa_item = &(((uint8 *)&this->CastToClient()->aa)[13]);
        if (*aa_item > 0 && *aa_item < 4){
            aa_speed += (float) ((100+*aa_item)/100);
        }
    }
    if (IsRooted())
        return 0.0f;
    if (spellbonuses->movementspeed)
        return (runspeed * (100+spellbonuses->movementspeed)) / 100;
    else
        return (runspeed * aa_speed);
}

sint32 Mob::CalcMaxMana()
{
	switch (GetCasterClass()) {
		case 'I':
			max_mana = (GetINT() * GetLevel()) + spellbonuses->Mana;
			break;
		case 'W':
			max_mana = (GetWIS() * GetLevel()) + spellbonuses->Mana;
			break;
		case 'N':
		default:
			max_mana = 0;
			break;
	}
    return max_mana;
}

char Mob::GetCasterClass() {
	switch(class_)
	{
	case CLERIC:
	case PALADIN:
	case RANGER:
	case DRUID:
	case SHAMAN:
	case BEASTLORD:
	case CLERICGM:
	case PALADINGM:
	case RANGERGM:
	case DRUIDGM:
	case SHAMANGM:
	case BEASTLORDGM:
		return 'W';
		break;

	case SHADOWKNIGHT:
	case BARD:
	case NECROMANCER:
	case WIZARD:
	case MAGICIAN:
	case ENCHANTER:
	case SHADOWKNIGHTGM:
	case BARDGM:
	case NECROMANCERGM:
	case WIZARDGM:
	case MAGICIANGM:
	case ENCHANTERGM:
		return 'I';
		break;

	default:
		return 'N';
		break;
	}
}

void Mob::CreateSpawnPacket(APPLAYER* app, Mob* ForWho) {
	app->opcode = OP_NewSpawn;
	app->pBuffer = new uchar[sizeof(NewSpawn_Struct)];
	app->size = sizeof(NewSpawn_Struct);

	memset(app->pBuffer, 0, sizeof(NewSpawn_Struct));
	NewSpawn_Struct* ns = (NewSpawn_Struct*)app->pBuffer;
	FillSpawnStruct(ns, ForWho);
	int8* tmp = app->pBuffer;
	app->pBuffer = new int8[app->size+50];
	app->size = DeflatePacket(tmp, app->size, app->pBuffer, app->size + 50);
	safe_delete(tmp);
	EncryptSpawnPacket(app);
}

void Mob::CreateHorseSpawnPacket(APPLAYER* app, const char* ownername, uint16 ownerid, Mob* ForWho) {
	app->opcode = OP_NewSpawn;
	app->pBuffer = new uchar[sizeof(NewSpawn_Struct)];
	app->size = sizeof(NewSpawn_Struct);
	memset(app->pBuffer, 0, sizeof(NewSpawn_Struct));
	NewSpawn_Struct* ns = (NewSpawn_Struct*)app->pBuffer;
	FillSpawnStruct(ns, ForWho);
	// Spawn_ID is 0x00 on a horse
	app->pBuffer[72] = 0x00;
	app->pBuffer[73] = 0x00;
//	int j = 0;
//	int maxlen = 0;
//	if (strlen(ownername) > 8) maxlen = 16;
//	else maxlen = strlen(ownername) * 2;

	// On live they have the firstname Unicoded here for some reason,
	// But it only goes to character 8 of the name.
/* Quagmire: this is causing errors, writing beyond the end of the packet
	for (int i=0; i < maxlen; i++) {
		app->pBuffer[196+j]	= ownername[i];
		app->pBuffer[196+j+1] = 0x00;
		j = j+2;
	}*/
    // This might be the wrong way to do it, but I couldn't get
	// A straight answer from anybody in #eqemucoders.
	app->pBuffer[211] = ownerid;
	app->pBuffer[213] = GetID();
	app->pBuffer[215] = ownerid;
	app->pBuffer[217] = GetID();
#if (DEBUG >= 11)
	printf("Horse Spawn Packet - Owner: %s\n", ownername);
	DumpPacket(app);
#endif
	app->Deflate();
	EncryptSpawnPacket(app);
}


void Mob::CreateSpawnPacket(APPLAYER* app, NewSpawn_Struct* ns) {
	app->opcode = OP_NewSpawn;
	app->pBuffer = new uchar[sizeof(NewSpawn_Struct)+50];
	app->size = DeflatePacket((int8*) ns, sizeof(NewSpawn_Struct), app->pBuffer, sizeof(NewSpawn_Struct) + 50);
	EncryptSpawnPacket(app);
}

void Mob::FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho) {
    ns->spawn.traptype = bodytype;
	ns->spawn.size = size;
	ns->spawn.walkspeed = walkspeed;
	ns->spawn.runspeed = runspeed;
	ns->spawn.heading = (int8) heading;
	ns->spawn.x_pos = (sint16) x_pos;
	ns->spawn.y_pos = (sint16) y_pos;
	ns->spawn.z_pos = (sint16) z_pos * 10;
	ns->spawn.spawn_id = GetID();
	ns->spawn.cur_hp = (sint16)GetHPRatio();
	ns->spawn.race = race;
	ns->spawn.GuildID = 0xffff;
	ns->spawn.guildrank = -1;
	if (IsInvisible(ForWho) == false)
		ns->spawn.invis = 0;
	else
		ns->spawn.invis = 1;
	if(IsClient())
		ns->spawn.NPC = 0;
	else
		ns->spawn.NPC = 1;
#ifdef INTERACTIVE
	if(IsNPC() && CastToNPC()->IsInteractive())
		ns->spawn.NPC = 0;
	else
		ns->spawn.NPC = 1;
#endif
#ifdef GUILDWARS
	if (IsNPC())
    {
	int32 guild = CastToNPC()->GetGuildOwner();
	if(guild == 0)
	guild = database.GetCityGuildOwned(CastToNPC()->GetNPCTypeID());
	if(guild != 0)
	{
			//char *cityname;
			//strcpy(cityname,database.GetCityAreaName(CastToNPC()->GetNPCTypeID()));
			ns->spawn.NPC = 0;
			ns->spawn.anon = 2;
			ns->spawn.GuildID = database.GetGuildEQID(guild);
//			CastToNPC()->SetGuildTerrainName(cityname);
//			zone->SetCityName(cityname);
	}
	}
#endif	
	ns->spawn.class_ = class_;
	ns->spawn.gender = gender;
	ns->spawn.level = level;
	ns->spawn.anim_type = 0x64;
	ns->spawn.light = light;

	if (IsClient())
	        ns->spawn.LD = CastToClient()->IsLD();
    else
            ns->spawn.LD = 0;
	ns->spawn.npc_armor_graphic = texture;
	ns->spawn.npc_helm_graphic = helmtexture;
	strcpy(ns->spawn.name, name);
	strcpy(ns->spawn.lastname, lastname);
	ns->spawn.deity = deity;

	ns->spawn.pet_owner_id = ownerid;
	ns->spawn.haircolor = haircolor;

	ns->spawn.beardcolor = beardcolor;
	ns->spawn.eyecolor1 = eyecolor1;
	ns->spawn.eyecolor2 = eyecolor2;
	ns->spawn.hairstyle = hairstyle;
	ns->spawn.title = title;
	ns->spawn.equipment[7] = d_meele_texture1;
	ns->spawn.equipment[8] = d_meele_texture2;
	//	ns->spawn.luclinface = luclinface; See eq_packets_structs.h
	ns->spawn.runspeed = runspeed;
	ns->spawn.walkspeed = walkspeed;

	if(IsNPC() && CastToNPC()->IsInteractive())
	{
		for(int i=-1;i<9;i++)
		{
			const Item_Struct* item = 0;
			item = database.GetItem(equipment[i]);
			if(item != 0)
			{
				if(i <= 7)
					ns->spawn.equipcolors[i] = item->common.color;
				ns->spawn.equipment[i] = item->common.material;
			}
			if(item == 0)
			{
				if(i<=7)
					ns->spawn.equipcolors[i] = 0;
				ns->spawn.equipment[i] = 0;
			}
		}
	}

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

void Mob::CreateHPPacket(APPLAYER* app)
{
	app->opcode = OP_HPUpdate;
	app->size = sizeof(SpawnHPUpdate_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, sizeof(SpawnHPUpdate_Struct));
	SpawnHPUpdate_Struct* ds = (SpawnHPUpdate_Struct*)app->pBuffer;
	ds->spawn_id = GetID();
	if (IsNPC()) {
		ds->cur_hp = (sint32)GetHPRatio();
		ds->max_hp = 100;
	}
	else if (IsClient()) {

		if (GetHP() > 30000)
			ds->cur_hp = 30000;
		else
			ds->cur_hp = GetHP() - itembonuses->HP;

		if (GetMaxHP() > 30000)
			ds->max_hp = 30000;
		else
		    ds->max_hp = max_hp;
	}
	else {
		ds->cur_hp = cur_hp;
		ds->max_hp = max_hp;
	}
}

void Mob::SendPosUpdate(int8 iSendToSelf) {
	APPLAYER* app = new APPLAYER(OP_MobUpdate, sizeof(SpawnPositionUpdates_Struct) + sizeof(SpawnPositionUpdate_Struct));
	SpawnPositionUpdates_Struct* spu = (SpawnPositionUpdates_Struct*)app->pBuffer;



	spu->num_updates = 1;
	MakeSpawnUpdate(&spu->spawn_update[0]);
	app->priority = 2;
	if (iSendToSelf == 2) {
		if (this->IsClient())
			this->CastToClient()->FastQueuePacket(&app);
	}
	else {
		entity_list.QueueClients(this, app, iSendToSelf == 0);
	}
	safe_delete(app);
}

void Mob::MakeSpawnUpdate(SpawnPositionUpdate_Struct* spu) {
	spu->spawn_id = GetID();
	spu->anim_type = pRunAnimSpeed;
	spu->heading = (int8) heading;
	spu->delta_heading = delta_heading;
	spu->x_pos = (sint16) x_pos;

	spu->y_pos = (sint16) y_pos;
	spu->z_pos = (sint16) (z_pos * 10);
	if (size > 0)
		spu->z_pos -= (sint16)size;
	spu->delta_y = delta_y/125;
	spu->delta_x = delta_x/125;
	spu->delta_z = delta_z;
}

void Mob::ShowStats(Client* client) {
	client->Message(0, "Name: %s %s", GetName(), lastname);
	client->Message(0, "  Level: %i  MaxHP: %i  CurHP: %i  AC: %i  Class: %i", GetLevel(), GetMaxHP(), GetHP(), GetAC(), GetClass());
	client->Message(0, "  MaxMana: %i  CurMana: %i  ATK: %i  Size: %1.1f", GetMaxMana(), GetMana(), GetATK(), GetSize());
	client->Message(0, "  STR: %i  STA: %i  DEX: %i  AGI: %i  INT: %i  WIS: %i  CHA: %i", GetSTR(), GetSTA(), GetDEX(), GetAGI(), GetINT(), GetWIS(), GetCHA());
	client->Message(0, "  Race: %i  BaseRace: %i  Texture: %i  HelmTexture: %i  Gender: %i  BaseGender: %i", GetRace(), GetBaseRace(), GetTexture(), GetHelmTexture(), GetGender(), GetBaseGender());
	if (client->Admin() >= 100) {
		client->Message(0, "  EntityID: %i  PetID: %i  OwnerID: %i  AIControlled: %i", this->GetID(), this->GetPetID(), this->GetOwnerID(), this->IsAIControlled());
		if (this->IsClient()) {
			client->Message(0, "  CharID: %i  PetID: %i", this->CastToClient()->CharacterID(), this->GetPetID());
		}
		else if (this->IsCorpse()) {
			if (this->IsPlayerCorpse()) {
				client->Message(0, "  CharID: %i  PlayerCorpse: %i", this->CastToCorpse()->GetCharID(), this->CastToCorpse()->GetDBID());
			}
			else {
				client->Message(0, "  NPCCorpse", this->GetID());
			}
		}
		else if (this->IsNPC()) {
			int32 spawngroupid = 0;
			if(this->CastToNPC()->respawn2 != 0)
				spawngroupid = this->CastToNPC()->respawn2->SpawnGroupID();
			client->Message(0, "  NPCID: %u  SpawnGroupID: %u LootTable: %u  FactionID: %i  SpellsID: %u", this->GetNPCTypeID(),spawngroupid, this->CastToNPC()->GetLoottableID(), this->CastToNPC()->GetNPCFactionID(), this->GetNPCSpellsID());
		}
		if (this->IsAIControlled()) {
			client->Message(0, "  AIControlled: ArrgoRange: %1.0f  AssistRange: %1.0f", this->GetArrgoRange(), this->GetAssistRange());
		}
	}
}

void Mob::DoAnim(const int animnum, bool ackreq) {
	APPLAYER* outapp = new APPLAYER(OP_Attack, sizeof(Attack_Struct));
	Attack_Struct* a = (Attack_Struct*)outapp->pBuffer;
	a->spawn_id = GetID();
	a->type = animnum;
	a->a_unknown2[5] = 0x80;
	a->a_unknown2[6] = 0x3f;
	entity_list.QueueCloseClients(this, outapp, false, 200, 0, ackreq);
	delete outapp;
}

void Mob::ShowBuffs(Client* client) {
	if (!spells_loaded)
		return;
	client->Message(0, "Buffs on: %s", this->GetName());
	for (int i=0; i < BUFF_COUNT; i++) {
		if (buffs[i].spellid != 0xFFFF) {
			if (buffs[i].durationformula == DF_Permanent)
				client->Message(0, "  %i: %s: Permanent", i, spells[buffs[i].spellid].name);
			else
				client->Message(0, "  %i: %s: %i tics left", i, spells[buffs[i].spellid].name, buffs[i].ticsremaining);
		}
	}
}

void Mob::GMMove(float x, float y, float z, float heading) {
	this->x_pos = x;
	this->y_pos = y;
	this->z_pos = z;
	if (heading != 0.01)
		this->heading = heading;
	SendPosUpdate(1);
}

void Mob::SendIllusionPacket(int16 in_race, int8 in_gender, int16 in_texture, int16 in_helmtexture, int8 in_haircolor, int8 in_beardcolor, int8 in_eyecolor1, int8 in_eyecolor2, int8 in_hairstyle, int8 in_title, int8 in_luclinface) {
	if (in_race == 0) {
		this->race = GetBaseRace();
		if (in_gender == 0xFFFF)
			this->gender = GetBaseGender();
		else
			this->gender = in_gender;
	}
	else {
		this->race = in_race;
		if (in_gender == 0xFF) {

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
			if (GetTexture() == 0xFF)
				this->texture = 0;
		}
		else if (in_race == 0)
			this->texture = 0xFF;
	}
	else if (in_texture != 0xFF || this->IsClient() || this->IsPlayerCorpse()) {
		this->texture = in_texture;
	}
	else
		this->texture = 0;
	if (in_helmtexture == 0xFFFF) {
		if (in_texture != 0xFFFF)
			this->helmtexture = this->texture;
		else if ((race == 0 || race > 12) && race != 128 && race != 130) {
			if (GetHelmTexture() == 0xFF)
				this->helmtexture = 0;
		}
		else if (in_race == 0)
			this->helmtexture = 0xFF;
		else
			this->helmtexture = 0;
	}
	else if (in_helmtexture != 0xFF || this->IsClient() || this->IsPlayerCorpse()) {

		this->helmtexture = in_helmtexture;
	}
	else
		this->texture = 0;
	if ((race == 0 || race > 12) && race != 128 && race != 130) {
		this->haircolor = in_haircolor;
		this->beardcolor = in_beardcolor;
		this->eyecolor1 = in_eyecolor1;
		this->eyecolor2 = in_eyecolor2;
		this->hairstyle = in_hairstyle;
		this->title = in_title;
		this->luclinface = in_luclinface;
	}
	else {
		this->hairstyle = 0xFF;
		this->beardcolor = 0xFF;
		this->eyecolor1 = 0xFF;
		this->eyecolor2 = 0xFF;
		this->hairstyle = 0xFF;
		this->title = 0xFF;
		this->luclinface = 0xFF;
	}
	APPLAYER* outapp = new APPLAYER(OP_Illusion, sizeof(Illusion_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	Illusion_Struct* is = (Illusion_Struct*) outapp->pBuffer;
	is->spawnid = GetID();
	is->race = this->race;
	is->gender = this->gender;
	is->texture = this->texture;
	is->helmtexture = this->helmtexture;
	is->haircolor = this->haircolor;
	is->beardcolor = this->beardcolor;
	is->eyecolor1 = this->eyecolor1;
	is->eyecolor2 = this->eyecolor2;
	is->hairstyle = this->hairstyle;
	is->title = this->title;
	is->luclinface = this->luclinface;
	is->unknown_26 = 26;
	is->unknown016 = 0xffffffff;
	//DumpPacket(outapp);
	entity_list.QueueClients(this, outapp);
	delete outapp;
}

int8 Mob::GetDefaultGender(int16 in_race, int8 in_gender) {
//cout << "Gender in:  " << (int)in_gender << endl;
	if ((in_race > 0 && in_race <= 12 ) || in_race == 128 || in_race == 130 || in_race == 15 || in_race == 50 || in_race == 57 || in_race == 70 || in_race == 98 || in_race == 118) {
		if (in_gender >= 2) {
			// Female default for PC Races
			return 1;
		}
		else
			return in_gender;
	}
	else if (in_race == 44 || in_race == 52 || in_race == 55 || in_race == 65 || in_race == 67 || in_race == 88 || in_race == 117 || in_race == 127 ||
		in_race == 77 || in_race == 78 || in_race == 81 || in_race == 90 || in_race == 92 || in_race == 93 || in_race == 94 || in_race == 106 || in_race == 112) {
		// Male only races
		return 0;

	}
	else if (in_race == 25 || in_race == 56) {
		// Female only races
		return 1;
	}
	else {
		// Neutral default for NPC Races
		return 2;
	}
}

void Mob::TicProcess() {
	for (int buffs_i=0; buffs_i < BUFF_COUNT; buffs_i++) {
		if (buffs[buffs_i].spellid != 0xFFFF) {
			DoBuffTic(buffs[buffs_i].spellid, buffs[buffs_i].ticsremaining, buffs[buffs_i].casterlevel, entity_list.GetMob(buffs[buffs_i].casterid));
			if (buffs[buffs_i].durationformula != 50) {
				buffs[buffs_i].ticsremaining--;
				if (buffs[buffs_i].ticsremaining <= 0) {
					BuffFade(buffs[buffs_i].spellid);
					buffs[buffs_i].spellid = 0xFFFF;
				}
			}
		}
	}
}

void Mob::SendAppearancePacket(int32 type, int32 value, bool WholeZone, bool iIgnoreSelf) {
	if (!GetID())
		return;
	APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
	SpawnAppearance_Struct* appearance = (SpawnAppearance_Struct*)outapp->pBuffer;
	appearance->spawn_id = this->GetID();
	appearance->type = type;
	appearance->parameter = value;
	if (WholeZone)
		entity_list.QueueClients(this, outapp, iIgnoreSelf);
	else if (this->IsClient())
		this->CastToClient()->QueuePacket(outapp);
	delete outapp;
}

void Mob::SetAppearance(int8 app, bool iIgnoreSelf) {
	if (appearance != app) {
		appearance = app;
		SendAppearancePacket(14, GetAppearanceValue(app), true, iIgnoreSelf);
		if (this->IsClient() && this->IsAIControlled())
			SendAppearancePacket(14, 102, false, false);
	}
}

void Mob::ChangeSize(float in_size = 0, bool bNoRestriction) {
	//Neotokyo's Size Code
	if (!bNoRestriction)
	{
		if (this->IsClient() || this->petid != 0)
			if (in_size < 3.0)
				in_size = 3.0;


			if (this->IsClient() || this->petid != 0)
				if (in_size > 15.0)
					in_size = 15.0;
	}


	if (in_size < 1.0)
		in_size = 1.0;

	if (in_size > 255.0)
		in_size = 255.0;
	//End of Neotokyo's Size Code
	this->size = in_size;
	SendAppearancePacket(29, (int32) in_size);
}

Mob* Mob::GetFamiliar() {
	Mob* tmp = entity_list.GetMob(this->GetFamiliarID());

	if (tmp) {
		if (tmp->GetOwnerID() == this->GetID()) {
			return tmp;
		}
		else {
			this->SetFamiliarID(0);
			return 0;
		}
	}
	return 0;
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

void Mob::SetPetID(int16 NewPetID) {
	if (NewPetID == GetID() && NewPetID != 0)
		return;
	petid = NewPetID;
}

void Mob::SetFamiliarID(int16 NewPetID) {
	if (NewPetID == GetID() && NewPetID != 0)
		return;
	familiarid = NewPetID;
}



Mob* Mob::GetOwnerOrSelf() {
	if (!GetOwnerID())
		return this;
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (!owner) {
		SetOwnerID(0);
	}
	else if (owner->GetPetID() == this->GetID()) {
		return owner;
	}
    else if (owner->GetFamiliarID() == this->GetID()) {
        return owner;
    }
	else {
		SetOwnerID(0);
	}
	return this;
}

Mob* Mob::GetOwner() {
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (owner && owner->GetPetID() == this->GetID()) {

		return owner;
	}
    if (owner && owner->GetFamiliarID() == this->GetID())
    {
        return owner;
    }
	this->SetOwnerID(0);
	return 0;
}

void Mob::SetOwnerID(int16 NewOwnerID) {
	if (NewOwnerID == GetID() && NewOwnerID != 0) // ok, no charming yourself now =p
		return;
	ownerid = NewOwnerID;
	if (ownerid == 0 && this->IsNPC() && this->GetPetType() != 0xFF)
		this->Depop();
}

//heko: for backstab
bool Mob::BehindMob(Mob* other, float playerx, float playery) {
    if (!other)
        return true; // sure your behind your invisible friend?? (fall thru for sneak)
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

void Mob::SetZone(int32 zone_id)
{
if(IsClient())
CastToClient()->pp.current_zone = zone_id;
Save();
}

void Mob::Kill() {
	Death(this, 0, 0xffff, 0x04);
}

void Mob::SetAttackTimer() {
	// Quagmire - image's attack timer code
    float PermaHaste;
    if ((GetHaste()+GetItemHaste()) == -100)
        PermaHaste = 10.0f;   // 10x as slow as normal for 100% slowed mobs
    else
        PermaHaste = 100.0f / (100.0f + GetHaste()+GetItemHaste()); //PercentageHaste);  	// use #haste to set haste level
//    g_LogFile.write("Setting Haste to %f", PermaHaste);
//  neotokyo: dont understand this limitation
//    if(PermaHaste > .9)
//		PermaHaste = .9;

	Timer	*TimerToUse;
	const Item_Struct *ItemToUse;

	for(int i=13 ; i <= 14 ; i++) {
		ItemToUse = 0;
		if(i==13)
			TimerToUse = attack_timer;
		else
			TimerToUse = attack_timer_dw;
		if (IsClient())
			ItemToUse = database.GetItem(CastToClient()->pp.inventory[i]);
    		else if (IsNPC())
			ItemToUse = database.GetItem(CastToNPC()->equipment[i]);
		else
			ItemToUse = 0;
		if (i == 14 && IsClient()) {
			int8 tmp = this->CastToClient()->GetSkill(DUEL_WIELD);
			if (tmp == 0 || tmp > 252 || !ItemToUse || !ItemToUse->IsWeapon() ) {
				if (ItemToUse && !ItemToUse->IsWeapon()) {
					//if(DEBUG>=1 && CastToClient()->GetGM())
					//    Message(0, "Dual wield disabled secondary weapon is not a weapon");
					attack_timer_dw->Disable();
					break;
				}
				else if (ItemToUse && ItemToUse->common.skill != 45) {
					if(DEBUG>=1 && CastToClient()->GetGM())
					    Message(0, "Dual wield disabled secondary weapon is not a hand to hand weapon");
					attack_timer_dw->Disable();
					break;
				}
				else {
					const Item_Struct* MainWeapon = database.GetItem(CastToClient()->pp.inventory[13]);
					if (MainWeapon) {
						if (MainWeapon && (MainWeapon->common.skill == 1||MainWeapon->common.skill == 4||MainWeapon->common.skill == 35)) {
							//if (DEBUG >=1 && CastToClient()->GetGM())
							//    Message(0, "Dual wield disabled Primary weapon is a two handed weapon");
							attack_timer_dw->Disable();
							break;
						}
					}
				}
			}
		}
		//if (DEBUG>=1 && CastToClient()->GetGM())
		//    Message(0, "Dual wield enabled");

		if(!ItemToUse) {
			// Work out if we're a monk
			if(GetClass() == MONK || GetClass() == BEASTLORD) {
				int speed = (int)(GetMonkHandToHandDelay()*100.0f*PermaHaste);
				// neotokyo: 1200 seemed too much, with delay 10 weapons available
            			if(speed < 500)
					speed = 500;
				TimerToUse->SetAtTrigger(speed, true);	// Hand to hand, delay based on level or epic
			}
			else {
				int speed = (int)(3600*PermaHaste);
				if(speed < 1800)
					speed = 1800;
				TimerToUse->SetAtTrigger(speed, true); 	// Hand to hand, non-monk 2/36
			}
		}
		else {
			if(ItemToUse->common.skill > 4 && ItemToUse->common.skill != 45 && ItemToUse->common.skill != 23) {			// Check skill is valid
				// item info is invalid, but ignore that for npcs
				if (i == 13 || GetLevel() >= 13) { // make npcs auto have dual wield at lvl 13
					TimerToUse->SetAtTrigger(2000, true);
				}
				else
					TimerToUse->Disable();		// Disable timer if primary item uses a non-weapon skill

        		}
			else {
				int speed = (int)(ItemToUse->common.delay*(100.0f*PermaHaste));
				if(speed < 500)
					speed = 500;
				TimerToUse->SetAtTrigger(speed, true);	// Convert weapon delay to timer resolution (milliseconds)
			}
		}
	}
}

bool Mob::CanThisClassDuelWield(void)
{
    // All npcs over level 13 can dual wield
    if (this->IsNPC() && this->GetLevel() >= 13)
        return true;
    
	// Kaiyodo - Check the classes that can DW, and make sure we're not using a 2 hander
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
		{
		if(GetLevel() < 13)
			return false;
		}
		break;
	case ROGUE:
		{
		if(GetLevel() < 13)
			return false;
		}
		break;
	case BARD:
		{
		if(GetLevel() < 17)
			return false;
		}
		break;
	case RANGER:
		{
		if(GetLevel() < 17)
			return false;
		}
		break;
	case BEASTLORD:
		{
		if(GetLevel() < 17)
			return false;
		}
		break;
	case MONK:
		{
		}
		break;
	default:
		return false; // To stop other classes :P
	}
	if (IsClient()) {
		uint16 item_id = CastToClient()->pp.inventory[13];
		const Item_Struct* item = database.GetItem(item_id);

		// 2HS, 2HB or 2HP
		if(item && (item->common.skill == 0x01 || item->common.skill == 0x23 || item->common.skill == 0x04))
			return(false);
		return(this->CastToClient()->GetSkill(DUEL_WIELD) != 0);	// No skill = no chance
	}
	else
			return true;
}

bool Mob::CanThisClassDoubleAttack(void)
{
    // All npcs over level 26 can double attack
    if (this->IsNPC() && this->GetLevel() >= 26)
        return true;
	// Kaiyodo - Check the classes that can DA
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
		{
		if(GetLevel() < 15)
			return false;
		}
		break;
	case MONK:
		{
		if(GetLevel() < 15)
			return false;
		}
		break;
	case RANGER:
		{
		if(GetLevel() < 20)
			return false;
		}
		break;
	case PALADIN:
	case SHADOWKNIGHT:
		{
		if(GetLevel() < 20)
			return false;
		}
		break;
	case ROGUE:
		{
		if(GetLevel() < 16)
			return false;
		}
		break;
	default:
		return false;
		break;
	}
	if (this->IsClient())
		return(this->CastToClient()->GetSkill(DOUBLE_ATTACK) != 0);	// No skill = no chance
	else
		return true;
}

bool Mob::IsWarriorClass(void)
{
	switch(GetClass())
	{
	case WARRIOR:
	case WARRIORGM:
	case ROGUE:
	case ROGUEGM:
	case MONK:
	case MONKGM:
	case PALADIN:
	case PALADINGM:
	case SHADOWKNIGHT:
	case SHADOWKNIGHTGM:
	case RANGER:
	case RANGERGM:
	case BEASTLORD:
	case BEASTLORDGM:
	case BARD:
	case BARDGM:
		return true;
	default:
		return false;
	}

}

bool Mob::CanThisClassParry(void)
{
	// Trumpcard
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
		{
		if(GetLevel() < 10)
			return false;
		}
	case ROGUE:
		{
		if(GetLevel() < 12)
			return false;
		}
	case BARD:
		{
		if(GetLevel() < 53)
			return false;
		}
	case RANGER:
		{
		if(GetLevel() < 18)
			return false;
		}
	case SHADOWKNIGHT:
	case PALADIN:
		{
		if(GetLevel() < 17)
			return false;
		}
	
		if (this->IsClient())
			return(this->CastToClient()->GetSkill(PARRY) != 0);	// No skill = no chance
		else
			return true;
	}
return false;
}

bool Mob::CanThisClassDodge(void)
{
	// Trumpcard
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
		{
		if(GetLevel() < 6)
			return false;
		}
	case MONK:
		{
		if(GetLevel() < 1)
			return false;
		}
	case ROGUE:
		{
		if(GetLevel() < 4)
			return false;
		}
	case BARD:
		{
		if(GetLevel() < 10)
			return false;
		}
	case RANGER:
		{
		if(GetLevel() < 8)
			return false;
		}
	case BEASTLORD:
		{
		if(GetLevel() < 10)
			return false;
		}
	case SHADOWKNIGHT:
	case PALADIN:
		{
		if(GetLevel() < 10)
			return false;
		}
	default:
        {
		if (this->IsClient())
			return(this->CastToClient()->GetSkill(DODGE) != 0);	// No skill = no chance
		else
			return true;
		break;
		}
	}
return false;
}

bool Mob::CanThisClassRiposte(void)
{
	// Trumpcard
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
		{
		if(GetLevel() < 25)
			return false;
		}
	case MONK:
		{
		if(GetLevel() < 35)
			return false;
		}
	case ROGUE:
		{
		if(GetLevel() < 30)
			return false;
		}
	case BARD:
		{
		if(GetLevel() < 58)
			return false;
		}
	case RANGER:
		{
		if(GetLevel() < 30)
			return false;
		}
	case BEASTLORD:
		{
		if(GetLevel() < 40)
			return false;
		}
	case SHADOWKNIGHT:
	case PALADIN:
		{
		if(GetLevel() < 30)
			return false;
		}
	default:
	    {
		if (this->IsClient())
			return(this->CastToClient()->GetSkill(RIPOSTE) != 0);	// No skill = no chance
		else
			return true;
		break;
		}
	}
return false;
}

int8 Mob::GetClassLevelFactor(){
	int8 multiplier = 0;
	switch(GetClass())
	{
	case WARRIOR:
		if (GetLevel() < 20)
			multiplier = 22;
		else if (GetLevel() < 30)
			multiplier = 23;
		else if (GetLevel() < 40)
			multiplier = 25;
		else if (GetLevel() < 53)
			multiplier = 27;
		else if (GetLevel() < 57)
			multiplier = 28;
		else
			multiplier = 30;
		break;

	case DRUID:
	case CLERIC:
	case SHAMAN:
		multiplier = 15;
		break;

	case PALADIN:
	case SHADOWKNIGHT:
		if (GetLevel() < 35)
			multiplier = 21;
		else if (GetLevel() < 45)
			multiplier = 22;
		else if (GetLevel() < 51)
			multiplier = 23;
		else if (GetLevel() < 56)
			multiplier = 24;
		else if (GetLevel() < 60)
			multiplier = 25;

		else
			multiplier = 26;
		break;

	case MONK:
	case BARD:
	case ROGUE:
	case BEASTLORD:
		if (GetLevel() < 51)
			multiplier = 18;
		else if (GetLevel() < 58)
			multiplier = 19;
		else
			multiplier = 20;
		break;

	case RANGER:
		if (GetLevel() < 58)
			multiplier = 20;
		else
			multiplier = 21;
		break;

	case MAGICIAN:
	case WIZARD:
	case NECROMANCER:

	case ENCHANTER:

		multiplier = 12;
		break;

	default:
		//cerr << "Unknown/invalid class in Client::CalcBaseHP" << endl;
		if (GetLevel() < 35)
			multiplier = 21;
		else if (GetLevel() < 45)
			multiplier = 22;
		else if (GetLevel() < 51)
			multiplier = 23;
		else if (GetLevel() < 56)
			multiplier = 24;
		else if (GetLevel() < 60)
			multiplier = 25;
		else
			multiplier = 26;
		break;
	}
	return multiplier;
}

/*
solar: returns false if attack should not be allowed
I try to list every type of conflict that's possible here, so it's easy
to see how the decision is made.  Yea, it could be condensed and made
faster, but I'm doing it this way to make it readable and easy to modify
*/
bool Mob::IsAttackAllowed(Mob *target)
{
	if(!target)
        return 0;

	/* self */
#ifdef GUILDWARS
	if(this == target) {
		return false; // People exploiting to give others experience
	}
	if(target->IsNPC() && target->CastToNPC()->IsCityController())
		return false;
#else
	if(this == target) {
		return true; // Quag: sure, you can attack yourself, why not?
	}
#endif
	if (this->IsCorpse() || target->IsCorpse())
		return false;
#ifdef GUILDWARS
    	if(_CLIENT(this))
	    {
        	if(_CLIENT(target))
	        {
        		if(target->CastToClient()->GuildDBID() != 0 && CastToClient()->GuildDBID() != 0)
            	{
		            if(database.GetGuildAlliance(CastToClient()->GuildDBID(),target->CastToClient()->GuildDBID()) || target->CastToClient()->GuildDBID() == CastToClient()->GuildDBID())
            			return false;
		            else
			            return true;
		        }
	        }
	        else if(_NPC(target))
	        {
				if(CastToClient()->GuildDBID() == 0 && target->CastToNPC()->GetGuildOwner() != 0)
					return false;

		        if(CastToClient()->GuildDBID() != 0 && target->CastToNPC()->GetGuildOwner() != 0)
		        {
		            if(database.GetGuildAlliance(CastToClient()->GuildDBID(),target->CastToNPC()->GetGuildOwner()) || target->CastToNPC()->GetGuildOwner() == CastToClient()->GuildDBID())
			            return false;
		            else
			            return true;
		        }
	        }
		else if(_CLIENTPET(target)) {
        		if(target->GetOwner()->CastToClient()->GuildDBID() != 0 && CastToClient()->GuildDBID() != 0)
            	{
		            if(database.GetGuildAlliance(CastToClient()->GuildDBID(),target->GetOwner()->CastToClient()->GuildDBID()) || target->GetOwner()->CastToClient()->GuildDBID() == CastToClient()->GuildDBID())
            			return false;
		            else
			            return true;
		        }		
		}
		else if(_NPCPET(target))
		{
		        if(CastToClient()->GuildDBID() == 0 && target->GetOwner()->CastToNPC()->GetGuildOwner() != 0)
				return false;
				
		        if(CastToClient()->GuildDBID() != 0 && target->GetOwner()->CastToNPC()->GetGuildOwner() != 0)
		        {
		            if(database.GetGuildAlliance(CastToClient()->GuildDBID(),target->GetOwner()->CastToNPC()->GetGuildOwner()) || target->GetOwner()->CastToNPC()->GetGuildOwner() == CastToClient()->GuildDBID())
			            return false;
		            else
			            return true;
		        }
		}
    	} // End of Client
	else if(_CLIENTPET(this)) {
		if(_CLIENT(target)) {
        		if(target->CastToClient()->GuildDBID() != 0 && this->GetOwner()->CastToClient()->GuildDBID() != 0)
            	{
		            if(database.GetGuildAlliance(this->GetOwner()->CastToClient()->GuildDBID(),target->CastToClient()->GuildDBID()) || target->CastToClient()->GuildDBID() == this->GetOwner()->CastToClient()->GuildDBID())
            			return false;
		            else
			            return true;
		        }		
		}
		else if(_NPC(target)) {
				if(GetOwner()->CastToClient()->GuildDBID() == 0 && target->CastToNPC()->GetGuildOwner() != 0)
					return false;

		        if(GetOwner()->CastToClient()->GuildDBID() != 0 && target->CastToNPC()->GetGuildOwner() != 0)
		        {
		            if(database.GetGuildAlliance(GetOwner()->CastToClient()->GuildDBID(),target->CastToNPC()->GetGuildOwner()) || target->CastToNPC()->GetGuildOwner() == GetOwner()->CastToClient()->GuildDBID())
			            return false;
		            else
			            return true;
		        }
		}
		else if(_CLIENTPET(target)) {
        		if(target->GetOwner()->CastToClient()->GuildDBID() != 0 && this->GetOwner()->CastToClient()->GuildDBID() != 0)
            	{
		            if(database.GetGuildAlliance(this->GetOwner()->CastToClient()->GuildDBID(),target->GetOwner()->CastToClient()->GuildDBID()) || target->GetOwner()->CastToClient()->GuildDBID() == this->GetOwner()->CastToClient()->GuildDBID())
            			return false;
		            else
			            return true;
		        }		
		}
		else if(_NPCPET(target))
		{
		        if(GetOwner()->CastToClient()->GuildDBID() != 0 && target->GetOwner()->CastToNPC()->GetGuildOwner() != 0)
		        {
		            if(database.GetGuildAlliance(GetOwner()->CastToClient()->GuildDBID(),target->GetOwner()->CastToNPC()->GetGuildOwner()) || target->GetOwner()->CastToNPC()->GetGuildOwner() == GetOwner()->CastToClient()->GuildDBID())
			            return false;
		            else
			            return true;
		        }
		}
		}// end of clientpet
    	else if(_NPC(this))
    	{
        	if(_CLIENT(target))
	        {
				if(target->CastToClient()->GuildDBID() == 0 && CastToNPC()->GetGuildOwner() != 0)
					return false;

		        if(target->CastToClient()->GuildDBID() != 0 && CastToNPC()->GetGuildOwner() != 0)
		        {
		        if(database.GetGuildAlliance(CastToNPC()->GetGuildOwner(),target->CastToClient()->GuildDBID()) || target->CastToClient()->GuildDBID() == CastToNPC()->GetGuildOwner())
			        return false;
		        else
    			    return true;
    		    }
	        }
	        else if(_NPC(target))
	        {
                // allow attacking if the mob is a pet
                if (this->GetOwner())
                    return true;

        		if(CastToNPC()->GetGuildOwner() != 0 && target->CastToNPC()->GetGuildOwner() != 0)
	        	{
    		        if(database.GetGuildAlliance(CastToNPC()->GetGuildOwner(),target->CastToNPC()->GetGuildOwner()) || target->CastToNPC()->GetGuildOwner() == CastToNPC()->GetGuildOwner())
	    		        return false;
        	    	else
	        	    	return true;
		        }
	        }
	    } // End of NPC
#endif // Guildwar

	if(_CLIENT(this))
	{
		if(_CLIENT(target))
		{
			if(this->CastToClient()->GetPVP() != target->CastToClient()->GetPVP())
				return false;
			else if(this->CastToClient()->GetPVP() && target->CastToClient()->GetPVP())
				return true;
			else if(this->CastToClient()->GetDuelTarget() == target->GetID() && target->CastToClient()->GetDuelTarget() == GetID() && target->CastToClient()->IsDueling() && this->CastToClient()->IsDueling())
				return true;
		}
		else if(_NPC(target))
		{
			if(target->CastToNPC()->IsInteractive() && (!target->CastToNPC()->IsPVP() || !this->CastToClient()->GetPVP()))
				return false;
			else
				return true;
		}
		else if(_BECOMENPC(target))
		{
			if(this->CastToClient()->GetLevel() > target->CastToClient()->GetBecomeNPCLevel())
				return false;
			else
				return true;
		}
		else if(_CLIENTPET(target))
		{
			if(this->CastToClient()->GetDuelTarget() == target->CastToMob()->GetOwner()->GetID() && target->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == GetID() && target->CastToMob()->GetOwner()->CastToClient()->IsDueling() && this->CastToClient()->IsDueling())
				return true;
			else if(this->CastToClient()->GetPVP() != target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return false;
			else if(this->CastToClient()->GetPVP() && target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return true;
		}
		else if(_NPCPET(target))
		{
			return true;
		}
		else if(_BECOMENPCPET(target))
		{
			if(this->CastToClient()->GetLevel() > target->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return false;
			else
				return true;
		}
		else
		{
			goto nocase;
		}
	}
	else if(_NPC(this))
	{
		if(_CLIENT(target))
		{
			if(CastToNPC()->IsInteractive() && (!CastToNPC()->IsPVP() || !target->CastToClient()->GetPVP()))
				return false;
			return true;
		}
		else if(_NPC(target))
		{
				if(CastToNPC()->GetPrimaryFaction() != 0 && target->CastToNPC()->GetPrimaryFaction() != 0 && CastToNPC()->GetPrimaryFaction() == target->CastToNPC()->GetPrimaryFaction() || CastToNPC()->IsFactionListAlly(target->CastToNPC()->GetPrimaryFaction()))
					return false;


			return 1;
		}
		else if(_BECOMENPC(target))
		{

			return 1;
		}
		else if(_CLIENTPET(target))
		{
			return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;
		}
		else if(_BECOMENPCPET(target))
		{
			return 1;
		}
		else
		{
			goto nocase;
		}
	}
	else if(_BECOMENPC(this))
	{
		if(_CLIENT(target))
		{
			if(target->CastToClient()->GetLevel() > this->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPC(target))
		{
			return 1;
		}
		else if(_BECOMENPC(target))
		{
			return 1;
		}
		else if(_CLIENTPET(target)) {
			if(target->CastToMob()->GetOwner()->CastToClient()->GetLevel() > this->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPCPET(target)) {
			return 1;
		}
		else if(_BECOMENPCPET(target)) {
			return 1;
		}
		else {
			goto nocase;
		}
	}
	else if(_CLIENTPET(this)) {
		if(_CLIENT(target)) {
			if(this->CastToMob()->GetOwner()->CastToClient()->IsDueling() && this->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == target->GetID() && target->CastToClient()->IsDueling() && target->CastToClient()->GetDuelTarget() == this->CastToMob()->GetOwner()->GetID())
				return 1;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() != target->CastToClient()->GetPVP())
				return 0;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() && target->CastToClient()->GetPVP())
				return 1;
		}
		else if(_NPC(target)) {
			return 1;

		}
		else if(_BECOMENPC(target)) {
			if(this->CastToMob()->GetOwner()->CastToClient()->GetLevel() > target->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_CLIENTPET(target)) {
			if(this->CastToMob()->GetOwner()->CastToClient()->IsDueling() && this->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == target->CastToMob()->GetOwner()->GetID() && target->CastToMob()->GetOwner()->CastToClient()->IsDueling() && target->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == this->CastToMob()->GetOwner()->GetID())
				return 1;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() != target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return 0;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() && target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;
		}
		else if(_BECOMENPCPET(target))
		{
			if(this->CastToMob()->GetOwner()->CastToClient()->GetLevel() > target->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else
		{
			goto nocase;
		}
	}
	else if(_NPCPET(this))
	{
		if(_CLIENT(target))
		{
			return 1;
		}
		else if(_NPC(target))
		{
			return 1;
		}
		else if(_BECOMENPC(target))
		{
			return 1;
		}
		else if(_CLIENTPET(target))
		{
			return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;

		}
		else if(_BECOMENPCPET(target))
		{
			return 1;
		}



		else
		{
			goto nocase;
		}
	}
	else if(_BECOMENPCPET(this))
	{
		if(_CLIENT(target))
		{
			if(target->CastToClient()->GetLevel() > this->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPC(target))
		{
			return 1;
		}
		else if(_BECOMENPC(target))
		{
			return 1;
		}
		else if(_CLIENTPET(target))
		{
			if(target->CastToMob()->GetOwner()->CastToClient()->GetLevel() > this->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;
		}
		else if(_BECOMENPCPET(target))
		{
			return 1;
		}
		else
		{
			goto nocase;
		}
	}

nocase:
	printf("Mob::IsAttackAllowed: don't have a rule for this - %s vs %s\n", this->GetName(), target->GetName());
	return 0;
}

bool Mob::CombatRange(Mob* other)
{
    // neotokyo: some mobs have set size == -1; this caused a signed/unsigned overflow
	sint32 size_mod = (sint32)GetSize();
	if(GetRace() == 49 || GetRace() == 158 || GetRace() == 196) //For races with a fixed size
 		size_mod = 60;
	else if (size_mod < 6)
		size_mod = 8;
	if (other->GetSize() > size_mod)
		size_mod = (sint32)other->GetSize();
	if (DistNoZ(other) <= size_mod*2)
		return true;
	return false;
}

float Mob::Dist(Mob* other) {
	return sqrt((double)(pow(other->x_pos-x_pos, 2) + pow(other->y_pos-y_pos, 2) + pow(other->z_pos-z_pos, 2)));
	//	return pow((other->x_pos-x_pos)*(other->x_pos-x_pos)+(other->y_pos-y_pos)*(other->y_pos-y_pos)+(other->z_pos-z_pos)*(other->z_pos-z_pos), 1/3);
}

float Mob::DistNoZ(Mob* other) {
	return sqrt((double)pow(other->x_pos-x_pos, 2) + pow(other->y_pos-y_pos, 2));
	//	return pow((other->x_pos-x_pos)*(other->x_pos-x_pos)+(other->y_pos-y_pos)*(other->y_pos-y_pos)+(other->z_pos-z_pos)*(other->z_pos-z_pos), 1/3);
}

float Mob::DistNoRoot(Mob* other) {
	return pow(other->x_pos-x_pos, 2) + pow(other->y_pos-y_pos, 2) + (pow(other->z_pos-z_pos, 2)*2);
	//	return (other->x_pos-x_pos)*(other->x_pos-x_pos)+(other->y_pos-y_pos)*(other->y_pos-y_pos)+(other->z_pos-z_pos)*(other->z_pos-z_pos);
}

float Mob::DistNoRootNoZ(Mob* other) {
	return (other->x_pos-x_pos)*(other->x_pos-x_pos)+(other->y_pos-y_pos)*(other->y_pos-y_pos);
}

bool Mob::HateSummon() {
    // check if mob has ability to summon
    // we need to be hurt and level 51+ or ability checked to continue
    if (GetHPRatio() >= 95 || (GetLevel() < 51 && SpecAttacks[SPECATK_SUMMON] == false))
        return false;

    // now validate the timer
    if (!SpecAttackTimers[SPECATK_SUMMON])
    {
        SpecAttackTimers[SPECATK_SUMMON] = new Timer(6000);
        SpecAttackTimers[SPECATK_SUMMON]->Start();
    }

    // now check the timer
    if (!SpecAttackTimers[SPECATK_SUMMON]->Check())
        return false;

    // get summon target
    target = GetHateTop();
    if( target)
    {
        if (target->IsClient())
	        target->CastToClient()->Message(15,"You have been summoned!");
		entity_list.MessageClose(this, true, 500, 10, "%s says,'You will not evade me, %s!' ", GetName(), GetHateTop()->GetName() );
		GetHateTop()->GMMove(x_pos, y_pos, z_pos, target->GetHeading());
		return true;
	}
	return false;
}

void Mob::FaceTarget(Mob* MobToFace, bool update) {
	if (MobToFace == 0)
		MobToFace = target;
	if (MobToFace == 0 || MobToFace == this)
		return;
	// TODO: Simplify?
	
	float angle;
	
	if (MobToFace->GetX()-x_pos > 0)
		angle = - 90 + atan((double)(MobToFace->GetY()-y_pos) / (double)(MobToFace->GetX()-x_pos)) * 180 / M_PI;
	else if (MobToFace->GetX()-x_pos < 0)
		angle = + 90 + atan((double)(MobToFace->GetY()-y_pos) / (double)(MobToFace->GetX()-x_pos)) * 180 / M_PI;
	else // Added?
	{
		if (MobToFace->GetY()-y_pos > 0)
			angle = 0;
		else
			angle = 180;
	}
	//cout << "dX:" << MobToFace->GetX()-x_pos;
	//cout << "dY:" << MobToFace->GetY()-y_pos;
	//cout << "Angle:" << angle;
	
	if (angle < 0)
		angle += 360;
	if (angle > 360)
		angle -= 360;
	

	float oldheading = heading;
	heading = (sint8) (256*(360-angle)/360.0f);
	//	return angle;
	
	//cout << "Heading:" << (int)heading << endl;
	if (update && oldheading != heading)
		pLastChange = Timer::GetCurrentTime();
}

bool Mob::RemoveFromHateList(Mob* mob) {
	SetRunAnimSpeed(0);
    bool bFound = false;
	if (this->IsEngaged())
	{
		bFound = hate_list.RemoveEnt(mob);	
		if  (!this->IsEngaged()){
			if (bFound)
				AI_Event_NoLongerEngaged();
			zone->DelAggroMob();
			//			cout << "Mobs currently Aggro: " << zone->MobsAggroCount() << endl; 
		}
	}
	if (target == mob)
		SetTarget(hate_list.GetTop());
    return bFound;
}
void Mob::WhipeHateList() {
	if (this->IsEngaged()) {
		AI_Event_NoLongerEngaged();
	}
	hate_list.Wipe();
}

// we need this for charmed NPCs
void Mob::SaveSpawnSpot() {
    spawn_x = x_pos;
    spawn_y = y_pos;
    spawn_z = z_pos;
    spawn_heading = heading;
}

void Mob::SaveGuardSpot(bool iClearGuardSpot) {
	if (iClearGuardSpot) {
		guard_x = 0;
		guard_y = 0;
		guard_z = 0;
		guard_heading = 0;

	}
	else {
		guard_x = x_pos;
		guard_y = y_pos;
		guard_z = z_pos;
		guard_heading = heading;
	}
}

void Mob::NPCSpecialAttacks(const char* parse, int permtag) {
    for(int i = 0; i < SPECATK_MAXNUM; i++)
	{
	    SpecAttacks[i] = false;
        SpecAttackTimers[i] = NULL;
    }

    while (*parse)
    {
        switch(*parse)
        {
        case 'Z':
			if (this->IsNPC())
    			this->CastToNPC()->interactive = true;
            break;
        case 'X':
    		if (this->IsNPC())
    			this->CastToNPC()->citycontroller = true;
            break;
        case 'Y':
    		if (this->IsNPC())
    			this->CastToNPC()->guildbank = true;
            break;
	    case 'E':
    	    SpecAttacks[SPECATK_ENRAGE] = true;
    		break;
	    case 'F':
    	    SpecAttacks[SPECATK_FLURRY] = true;
    		break;
/*		case 'G':
			{
	CastToNPC()->d_meele_texture1= 199;
	CastToNPC()->equipment[7]=199;
	CastToNPC()->rangerstance = true;
	CastToNPC()->pArrgoRange = 200;
	CastToNPC()->pAssistRange = 200;
	break;
			}*/
	    case 'R':
    	    SpecAttacks[SPECATK_RAMPAGE] = true;
    		break;
	    case 'S':
    	    SpecAttacks[SPECATK_SUMMON] = true;
            SpecAttackTimers[SPECATK_SUMMON] = new Timer(6000);
            SpecAttackTimers[SPECATK_SUMMON]->Start();
    		break;
	    case 'T':
            SpecAttacks[SPECATK_TRIPLE] = true;
            break;
	    case 'Q':
            SpecAttacks[SPECATK_QUAD] = true;
            break;
        default:
            break;
        }
        parse++;
    }
	
	if(permtag == 1 && this->GetNPCTypeID() > 0){
		if(database.SetSpecialAttkFlag(this->GetNPCTypeID(),parse)) {
			LogFile->write(EQEMuLog::Normal, "NPCTypeID: %i flagged to '%s' for Special Attacks.\n",this->GetNPCTypeID(),parse);
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
