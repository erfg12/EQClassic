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
#include <iostream.h>
#include <iomanip.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>


// Disgrace: for windows compile
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include <process.h>

#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#else
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../common/unix.h"
#endif

#include "../common/version.h"
#include "client.h"
#include "groups.h"
#include "worldserver.h"
#include "net.h"
#include "skills.h"
#include "../common/database.h"
#include "spdat.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "PlayerCorpse.h"
#include "petitions.h"
#include "../common/serverinfo.h"
#include "../common/ZoneNumbers.h"
#include "../common/moremath.h"


extern volatile bool RunLoops;
extern Database database;
extern EntityList entity_list;
extern Zone* zone;
extern volatile bool ZoneLoaded;
extern WorldServer worldserver;
extern GuildRanks_Struct guilds[512];
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern int32 numclients;
extern PetitionList petition_list;
bool commandlogged;
char entirecommand[255];

#ifdef GUILDWARS
#ifdef GUILDWARS2
#define NOGUILDCAPLEVEL 21
#define SETLEVEL 60
#define GAINLEVEL 61
#else
#define NOGUILDCAPLEVEL 21
#define SETLEVEL 50
#define GAINLEVEL 51
#endif
#endif

Client::Client(EQNetworkConnection* ieqnc)
: Mob("No name",	// name
	  "",	// lastname
	  0,	// curhp
	  0,	// maxhp
	  0,	// gender
	  0,	// race
	  0,	// class
      0,    // bodytype
	  0,	// deity
	  0,	// level
	  0,	// npctypeid
	  0,	// skills
	  0,	// size
	  0.46,	// walkspeed
	  0.7,	// runspeed
	  0,	// heading
	  0,	// x
	  0,	// y
	  0,	// z
	  0,	// light
	  0,	// equip
	  0xFF,	// texture
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
	  0xff,	// Luclin Hair Colour
	  0xff,	// Luclin Beard
	  0xff,	// Luclin Eye1
	  0xff,	// Luclin Eye2
	  0xff,	// Luclin Hair Style
	  0xff,	// Luclin title
	  0xff,// Luclin Face
	  1, // fixedz
	  0, // standart text1
	  0 // standart text2
	  )	
{
	character_id = 0;
	feigned = false;
	this->berserk = false;
	this->dead = false;
	eqnc = ieqnc;
	ip = eqnc->GetrIP();
	port = ntohs(eqnc->GetrPort());
	client_state = CLIENT_CONNECTING1;
	WID = 0;
	account_id = 0;
	admin = 0;
	lsaccountid = 0;
	memset(lskey, 0, sizeof(lskey));
	strcpy(account_name, "");
	tellsoff = false;
	gmhideme = false;
	LFG = false;
	playeraction = 0;
	target = 0;
	auto_attack = false;
	PendingGuildInvite = 0;
	zonesummon_x = -2;
	zonesummon_y = -2;
	zonesummon_z = -2;
	zonesummon_ignorerestrictions = 0;
	casting_spell_id = 0;
	npcflag = false;
	npclevel = 0;
	pQueuedSaveWorkID = 0;
	
	position_timer = new Timer(250);
	position_timer->Disable();
	hpregen_timer = new Timer(1800);
	position_timer_counter = 0;
	camp_timer = new Timer(29000);
	process_timer = new Timer(100);
	camp_timer->Disable();
	pLastUpdate = 0;
	pLastUpdateWZ = 0;
	// Kaiyodo - initialise haste variable
	HastePercentage = 0;
	
	IsSettingGuildDoor = false;
	delaytimer = false;
	pendingrezzexp = 0;
	numclients++;
	UpdateWindowTitle();
	hasmount = false;
	horseId = 0;
	cursorplatinum = 0;
	cursorgold = 0;
	cursorsilver = 0;
	cursorcopper = 0;
	tgb = false;
}

Client::~Client() {
	Mob* horse = entity_list.GetMob(this->CastToClient()->GetHorseId());
	if (horse)
		horse->Depop();

	if(IsDueling() && GetDuelTarget() != 0)
	{
        Entity* entity = entity_list.GetID(GetDuelTarget());
		if(entity != NULL && entity->IsClient())
		{
			entity->CastToClient()->SetDueling(false);
			entity->CastToClient()->SetDuelTarget(0);
		}
	}

	pp.platinum += cursorplatinum;
	pp.gold += cursorgold;
	pp.silver += cursorsilver;
	pp.copper += cursorcopper;

	if (this->isgrouped && entity_list.GetGroupByClient(this) != NULL)
		entity_list.GetGroupByClient(this)->DelMember(this->CastToMob(),true);

	eqnc->Free();
	UpdateWho(2);
	Save(1); // This fails when database destructor is called first on shutdown	
    delete position_timer;
	delete hpregen_timer;
	delete camp_timer;
	delete process_timer;
	numclients--;
	UpdateWindowTitle();
}

sint16 Client::GetMaxSTR()
{
    // todo: PoP AA abilities
    return 255;
}
sint16 Client::GetMaxSTA()
{
    // todo: PoP AA abilities
    return 255;
}
sint16 Client::GetMaxDEX()
{
    // todo: PoP AA abilities
    return 255;
}
sint16 Client::GetMaxAGI()
{
    // todo: PoP AA abilities
    return 255;
}
sint16 Client::GetMaxINT()
{
    // todo: PoP AA abilities
    return 255;
}
sint16 Client::GetMaxWIS()
{
    // todo: PoP AA abilities
    return 255;
}
sint16 Client::GetMaxCHA()
{
    // todo: PoP AA abilities
    return 255;
}

bool Client::GetIncreaseSpellDurationItem(int16 &spell_id, char *itemname)
{
	const Item_Struct* item = NULL;
    for (int i = 0; i < pp_inventory_size; i++)
    {
		item = database.GetItem(pp.inventory[i]);
        if (!item)
            continue;
        // item with spell (worn) on it
        if (item->common.focusspellId && item->common.focusspellId != 0xFFFF)
        {
            if (IsIncreaseDurationSpell(item->common.focusspellId))
            {
                spell_id = item->common.focusspellId;
                if (itemname)
                    strcpy(itemname, item->name);
                return true;
            }
        }
    }
    return false;
}

bool Client::GetReduceManaCostItem(int16 &spell_id, char *itemname)
{
	const Item_Struct* item = NULL;
    for (int i = 0; i < pp_inventory_size; i++)
    {
		item = database.GetItem(pp.inventory[i]);
        if (!item)
            continue;
        // item with spell (worn) on it
        if (item->common.focusspellId && item->common.focusspellId != 0xFFFF)
        {
            if (IsReduceManaSpell(item->common.focusspellId))
            {
                spell_id = item->common.focusspellId;
                if (itemname)
                    strcpy(itemname, item->name);
                return true;
            }
        }
    }
    return false;
}

bool Client::GetReduceCastTimeItem(int16 &spell_id, char *itemname)
{
	const Item_Struct* item = NULL;
    for (int i = 0; i < pp_inventory_size; i++)
    {
		item = database.GetItem(pp.inventory[i]);
        if (!item)
            continue;
        // item with spell (worn) on it
        if (item->common.focusspellId && item->common.focusspellId != 0xFFFF)
        {
            if (IsReduceCastTimeSpell(item->common.focusspellId))
            {
                spell_id = item->common.focusspellId;
                if (itemname)
                    strcpy(itemname, item->name);
                return true;
            }
        }
    }
    return false;
}

bool Client::GetExtendedRangeItem(int16 &spell_id, char *itemname)
{
	const Item_Struct* item = NULL;
    for (int i = 0; i < pp_inventory_size; i++)
    {
		item = database.GetItem(pp.inventory[i]);
        if (!item)
            continue;
        // item with spell (worn) on it
        if (item->common.focusspellId && item->common.focusspellId != 0xFFFF)
        {
            if (IsExtRangeSpell(item->common.focusspellId))
            {
                spell_id = item->common.focusspellId;
                if (itemname)
                    strcpy(itemname, item->name);
                return true;
            }
        }
    }
    return false;
}

bool Client::GetImprovedHealingItem(int16 &spell_id, char *itemname)
{
	const Item_Struct* item = NULL;
    for (int i = 0; i < pp_inventory_size; i++)
    {
		item = database.GetItem(pp.inventory[i]);
        if (!item)
            continue;
        // item with spell (worn) on it
        if (item->common.focusspellId && item->common.focusspellId != 0xFFFF)
        {
            if (IsImprovedHealingSpell(item->common.focusspellId))
            {
                spell_id = item->common.focusspellId;
                if (itemname)
                    strcpy(itemname, item->name);
                return true;
            }
        }
    }
    return false;
}

bool Client::GetImprovedDamageItem(int16 &spell_id, char *itemname)
{
	const Item_Struct* item = NULL;
    for (int i = 0; i < pp_inventory_size; i++)
    {
		item = database.GetItem(pp.inventory[i]);
        if (!item)
            continue;
        // item with spell (worn) on it
        if (item->common.focusspellId && item->common.focusspellId != 0xFFFF)
        {
            if (IsImprovedDamageSpell(item->common.focusspellId))
            {
                spell_id = item->common.focusspellId;
                if (itemname)
                    strcpy(itemname, item->name);
                return true;
            }
        }
    }
    return false;
}

sint32 Client::GenericFocus(int16 spell_id, int16 modspellid)
{
    int32 modifier = 100;

    for (int i = 0;i < EFFECT_COUNT; i++)
    {
        switch( spells[modspellid].effectid[i] )


        {
        case SE_LimitMaxLevel:
            if (spells[spell_id].classes[(GetClass()%16) - 1] > spells[modspellid].base[i])
                return 100;
            break;
        case SE_LimitMinLevel:
            if (spells[spell_id].classes[(GetClass()%16) - 1] < spells[modspellid].base[i])
                return 100;
            break;
        case SE_IncreaseRange:
            modifier += spells[modspellid].base[i];
            break;
        case SE_IncreaseSpellHaste:
            modifier -= spells[modspellid].base[i];
            break;
        case SE_IncreaseSpellDuration:
            modifier += spells[modspellid].base[i];
            break;
        case SE_LimitSpell:
            // negative sign means exclude
            // positive sign means include
            if (spells[modspellid].base[i] < 0)
            {
                if (spells[modspellid].base[i] * (-1) == spell_id)
                    return 100;
            }
            else
            {
                if (spells[modspellid].base[i] != spell_id)
                    return 100;
            }
            break;
        case SE_LimitEffect:
            switch( spells[modspellid].base[i] )
            {
            case -147:
                if (IsPercentalHeal(spell_id))
                    return 100;
                break;
            case -101:
                if (IsCHDuration(spell_id))
                    return 100;
                break;
            case -40:
                if (IsInvulnerability(spell_id))


                    return 100;
                break;
            case -32:
                if (IsSummonItem(spell_id))
                    return 100;
                break;
            case 0:
                if (!IsEffectHitpoints(spell_id))
                    return 100;
                break;
            case 33:
                if (!IsSummonPet(spell_id))
                    return 100;
                break;
            case 36:
                if (!IsPoisonCounter(spell_id))
                    return 100;
                break;
            case 71:
                if (!IsSummonSkeleton(spell_id))
                    return 100;
                break;
            default:
                LogFile->write(EQEMuLog::Normal, "unknown limit effect %d", spells[modspellid].base[i]);
            }
            break;
        case SE_LimitCastTime:
            if (spells[modspellid].base[i] < spells[spell_id].cast_time)
                return 100;
            break;
        case SE_LimitSpellType:
            switch( spells[modspellid].base[i] )
            {
            case 0:
                if (!IsDetrimental(spell_id))
                    return 100;
                break;
            case 1:
                if (!IsBeneficial(spell_id))
                    return 100;
                break;
            default:
                LogFile->write(EQEMuLog::Normal, "unknown limit spelltype %d", spells[modspellid].base[i]);
            }
            break;
        case SE_LimitMinDur:
            if (spells[modspellid].base[i] > CalcBuffDuration(GetLevel(), spells[spell_id].buffdurationformula, spells[spell_id].buffduration))
                return 100;
            break;
        case SE_ImprovedDamage:
        case SE_ImprovedHeal:
            modifier += spells[modspellid].base[i];
            break;
        case SE_ReduceManaCost:
            modifier -= spells[modspellid].base[i];
            break;


        default:
            LogFile->write(EQEMuLog::Normal, "unknown effectid %d", spells[modspellid].effectid[i]);
            break;
        }
    }
    return modifier;
}

sint32 Client::GetActSpellRange(int16 spell_id, sint32 range)
{
    int16 modspellid = 0;
    
    int extrange = 100;
    if (GetExtendedRangeItem(modspellid, NULL))
    {
        extrange = GenericFocus(spell_id, modspellid);
    }
    return (range * extrange) / 100;
}

sint32 Client::GetActSpellValue(int16 spell_id, sint32 value)
{
    int16 modspellid = 0;
    

    int modifier = 100;

    if (spells[spell_id].goodEffect == 0)
    {
        if (GetImprovedDamageItem(modspellid, NULL))
            modifier = GenericFocus(spell_id, modspellid);
    }
    else
    {
        if (GetImprovedHealingItem(modspellid, NULL))
            modifier = GenericFocus(spell_id, modspellid);
    }
    return (value * modifier) / 100;
                /*
#define SE_LimitResist      0x87
#define SE_LimitTarget      0x88
#define SE_LimitInstant     0x8D
#define SE_LimitCastTime    0x8F
                */
}

sint32 Client::GetActSpellCost(int16 spell_id, sint32 cost)
{
    int16 modspellid = 0;
    
    int reduce = 100;
    if (GetReduceManaCostItem(modspellid, NULL))
    {
        reduce = GenericFocus(spell_id, modspellid);
    }
    return (cost * reduce) / 100;
}

sint32 Client::GetActSpellDuration(int16 spell_id, sint32 duration)
{
    int16 modspellid = 0;
    
    int increase = 100;

    if (GetIncreaseSpellDurationItem(modspellid, NULL))
    {
        increase = GenericFocus(spell_id, modspellid);
    }
    return (duration * increase) / 100;
}

sint32 Client::GetActSpellCasttime(int16 spell_id, sint32 casttime)
{
    int16 modspellid = 0;
    
    int reduce = 100;
    if (GetReduceCastTimeItem(modspellid, NULL))
    {
        reduce = GenericFocus(spell_id, modspellid);
    }
    return (casttime * reduce) / 100;
}

bool Client::Save(int8 iCommitNow) {
	if (Connected()) {
		pp.x = x_pos;
		pp.y = y_pos;
		pp.z = z_pos;
		pp.heading = heading;
		if (GetHP() <= 0) {
			if (GetMaxHP() > 30000)
				pp.cur_hp = 30000;
			else
				pp.cur_hp = GetMaxHP();
		}
		else if (GetHP() > 30000)
			pp.cur_hp = 30000;
		else
			pp.cur_hp = GetHP();
		pp.mana = cur_mana;
		
		for (int i=0; i<15; i++) {
			if (buffs[i].spellid <= SPDAT_RECORDS) {
				pp.buffs[i].spellid = buffs[i].spellid;
				pp.buffs[i].duration = buffs[i].ticsremaining;
				pp.buffs[i].level = buffs[i].casterlevel;
			}
			else {
				pp.buffs[i].spellid = 0xFFFF;
				pp.buffs[i].duration = 0;
				pp.buffs[i].level = 0;
			}
		}
		if (pQueuedSaveWorkID) {
			dbasync->CancelWork(pQueuedSaveWorkID);
			pQueuedSaveWorkID = 0;
		}
		if (iCommitNow <= 1) {
			char* query = 0;
			uint32_breakdown workpt;
			workpt.b4() = DBA_b4_Entity;
			workpt.w2_3() = GetID();
			workpt.b1() = DBA_b1_Entity_Client_Save;
			DBAsyncWork* dbaw = new DBAsyncWork(MTdbafq, workpt, DBAsync::Write, 0xFFFFFFFF);
			dbaw->AddQuery(iCommitNow == 0 ? true : false, &query, database.SetPlayerProfile_MQ(&query, account_id, character_id, &pp), false); 
			if (iCommitNow == 0)
				pQueuedSaveWorkID = dbasync->AddWork(&dbaw, 2500);
			else {
				dbasync->AddWork(&dbaw, 0);
				SaveBackup();
			}
			return true;
		}
		else if (database.SetPlayerProfile(account_id, character_id, &pp)) {
			SaveBackup();
		}
		else {
			cerr << "Failed to update player profile" << endl;
			return false;
		}
	}
	return true;
}

void Client::SaveBackup() {
	if (!RunLoops)
		return;
	char* query = 0;
	DBAsyncWork* dbaw = new DBAsyncWork(&DBAsyncCB_CharacterBackup, this->CharacterID(), DBAsync::Read);
	dbaw->AddQuery(0, &query, MakeAnyLenString(&query, "Select id, UNIX_TIMESTAMP()-UNIX_TIMESTAMP(ts) as age from character_backup where charid=%u and backupreason=0 order by ts asc", this->CharacterID()), true);
	dbasync->AddWork(&dbaw, 0);
}

CLIENTPACKET::CLIENTPACKET()
{
    app = NULL;
    ack_req = false;

}

CLIENTPACKET::~CLIENTPACKET()
{
    if (app)
        delete app;
}

bool Client::AddPacket(const APPLAYER *pApp, bool bAckreq) {
	if (!pApp)
		return false;
    CLIENTPACKET *c = new CLIENTPACKET;

    c->ack_req = bAckreq;
    c->app = pApp->Copy();
    clientpackets.Append(c);
    return true;
}

bool Client::AddPacket(APPLAYER** pApp, bool bAckreq) {
	if (!pApp || !(*pApp))
		return false;
    CLIENTPACKET *c = new CLIENTPACKET;

    c->ack_req = bAckreq;
    c->app = *pApp;

	*pApp = 0;

    clientpackets.Append(c);
    return true;
}


sint32 Client::LevelRegen()
{
    sint32 hp = 0;
    if (GetLevel() <= 19) {
        if(IsSitting())
                hp+=2;
        else
                hp+=1;
    }
    else if(GetLevel() <= 49)
    {
        if(IsSitting())
                hp+=3;
        else
                hp+=1;
    }
    else if(GetLevel() == 50)
    {
        if(IsSitting())
                hp+=4;
        else
                hp+=1;
    }
    else if(GetLevel() >= 51)
    {
        if(IsSitting())
                hp+=5;
        else
                hp+=2;
    }
    if(GetRace() == IKSAR || GetRace() == TROLL)
    {
        if (GetLevel() <= 19)
        { // 1 4
                if(IsSitting())
                                hp+=2;
        }
        else if(GetLevel() <= 49)
        { // 2 6
                if(IsSitting())
                                hp+=3;
                else
                                hp+=1;
        }
        else if(GetLevel() >= 50)
        { // 4 12
                if(IsSitting())
                                hp+=7;
                else
                                hp+=2;
        }
    }
#if 1 // AA regen
	uint8 *aa_regen = &(((uint8 *)&aa)[225]);
    if (*aa_regen > 0 && *aa_regen < 4){
        hp+=(int) *aa_regen;
    }
#endif // AA regen

    return hp;
}

bool Client::SendAllPackets() {
	LinkedListIterator<CLIENTPACKET*> iterator(clientpackets);

	CLIENTPACKET* cp = 0;
    iterator.Reset();
	while(iterator.MoreElements()) {
		cp = iterator.GetData();
        if(eqnc)
            eqnc->FastQueuePacket(&cp->app, iterator.GetData()->ack_req);
        iterator.RemoveCurrent();
        LogFile->write(EQEMuLog::Normal, "Transmitting a packet");
    }
    return true;
}

void Client::QueuePacket(const APPLAYER* app, bool ack_req, CLIENT_CONN_STATUS required_state) {
	if (app != 0) {
		if (app->size >= 31500) {
			cout << "WARNING: abnormal packet size. n='" << this->GetName() << "', o=0x" << hex << app->opcode << dec << ", s=" << app->size << endl;
		}
	}
    // if the program doesnt care about the status or if the status isnt what we requested
    if (required_state != CLIENT_CONNECTINGALL && client_state != required_state)
    {
        // todo: save packets for later use
        AddPacket(app, ack_req);
        LogFile->write(EQEMuLog::Normal, "Adding Packet to list (%d) (%d)", app->opcode, (int)required_state);
    }
    else
	    if(eqnc)
            eqnc->QueuePacket(app, ack_req);
}

void Client::FastQueuePacket(APPLAYER** app, bool ack_req, CLIENT_CONN_STATUS required_state) {	
	if (app != 0 && (*app) != 0) {
		if ((*app)->size >= 31500) {
			cout << "WARNING: abnormal packet size. n='" << this->GetName() << "', o=0x" << hex << (*app)->opcode << dec << ", s=" << (*app)->size << endl;
		}
	}
    // if the program doesnt care about the status or if the status isnt what we requested

    if (required_state != CLIENT_CONNECTINGALL && client_state != required_state) {
        // todo: save packets for later use
        AddPacket(app, ack_req);
        LogFile->write(EQEMuLog::Normal, "Adding Packet to list (%d) (%d)", (*app)->opcode, (int)required_state);
    }
    else {
	    if(eqnc)
            eqnc->FastQueuePacket(app, ack_req);
		else if (app && (*app))
			delete *app;
		*app = 0;
	}
}

void Client::ChannelMessageReceived(int8 chan_num, int8 language, const char* message, const char* targetname) {
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"Client::ChannelMessageReceived() Channel:%i message:'%s'", chan_num, message);
#endif
	switch(chan_num)
	{
	case 0: //GuildChat
		{
			if (guilddbid == 0 || guildeqid >= 512)
				Message(0, "Error: You arent in a guild.");
			else if (!guilds[guildeqid].rank[guildrank].speakgu)
				Message(0, "Error: You dont have permission to speak to the guild.");
			else if (!worldserver.SendChannelMessage(this, targetname, chan_num, guilddbid, language, message))
				Message(0, "Error: World server disconnected");
			break;
		}
		
	case 2: { //GroupChat
		Group* group = entity_list.GetGroupByMob(this);
		if (this->isgrouped && group != 0) {
			group->GroupMessage(this,(const char*) message);
		}
		break;
	}		
	case 3: // Shout
	case 4: // Auction
		{
			entity_list.ChannelMessage(this, chan_num, language, message);
			break;
		}
		
	case 5: // OOC
		{
			if (!worldserver.SendChannelMessage(this, 0, 5, 0, language, message))
				Message(0, "Error: World server disconnected");
			break;
		}
		
	case 6: // Broadcast
	case 11: // GMSay
		{
			if (!(admin >= 80))
				Message(0, "Error: Only GMs can use this channel");
			else if (!worldserver.SendChannelMessage(this, targetname, chan_num, 0, language, message))
				Message(0, "Error: World server disconnected");
			break;
		}
		
	case 7: // Tell
		{

				if(!worldserver.SendChannelMessage(this, targetname, chan_num, 0, language, message))
					Message(0, "Error: World server disconnected");
			break;
		}
		
	case 8:
		{
			if(message[0] != '#')
			{
				entity_list.ChannelMessage(this, chan_num, language, message);
				if (target != 0 && target->IsNPC() && !target->CastToNPC()->IsEngaged()) {
					if (DistNoRootNoZ(target) <= 200) {
					if(target->CastToNPC()->IsInteractive())
					{
					    cout<<"Sending message to IPC"<<endl;

						target->CastToNPC()->InteractiveChat(chan_num,language,message,targetname,this);

					}
#ifdef GUILDWARS
						if(IsClient() && GuildDBID() != 0 && target->CastToNPC()->IsCityController())
						{
							if(admin > 201 && atoi(message) > 0)
							{
								int32 guildid = atoi(message);
								zone->ZoneTakeOver(guildid);
								database.SetCityGuildOwned(guildid,target->CastToNPC()->GetNPCTypeID());
								target->CastToNPC()->SetGuildOwner(guildid);
								target->CastToNPC()->Depop();
								return;
							}
							else if(!zone->IsCityTakeable())
							{
								entity_list.MessageClose(this,false,400,0,"%s says, 'This city was taken over too recently, I forbid another takeover.'",target->GetName());
								return;
							}
							else if(database.GetCityGuildOwned(0,zone->GetZoneID()) == GuildDBID())
							{
								entity_list.MessageClose(this, false, 400, 0, "%s says, 'You are a member of the guild that owns this city, why would you want to conquere it?'", target->GetName());
								return;
							}
							else if(database.GetGuildAlliance(GuildDBID(),target->CastToNPC()->GetGuildOwner()))
							{
								entity_list.MessageClose(this, false, 400, 0, "%s says, 'You cannot conquere an allied city.'", target->GetName());				
								return;
							}
							else if(GuildRank() != 0)
							{
								entity_list.MessageClose(this, false, 400, 0, "%s says, 'You are not a leader of a guild and therefore cannot take over a city.'", target->GetName());	
								return;
							}
							else if(database.CityDefense(zone->GetZoneID(),0))
							{
								entity_list.MessageClose(this, false, 400, 0, "%s says, 'This city still has men standing, I will not surrender!'", target->GetName());				
								return;
							}
							else if(database.SetCityGuildOwned(GuildDBID(),target->CastToNPC()->GetNPCTypeID()))
							{
			/*				char guildone[400];
							char guildone[400];
							char guildtwo[400];
							char body[400];
							sprintf(guildone,"%i",target->CastToNPC()->GetGuildOwner());
							sprintf(guildtwo,"%i",GuildDBID());
							database.InsertNewsPost(4,guildone,guildtwo,0,body);*/
							zone->ZoneTakeOver(GuildDBID());
							database.SetCityGuildOwned(GuildDBID(),target->CastToNPC()->GetNPCTypeID());
							target->CastToNPC()->SetGuildOwner(GuildDBID());
							target->CastToNPC()->Depop();
							Message(13,"You have received a few guard slips to start your city.");
							const Item_Struct* item = database.GetItem(900);
							if(item != 0)
							{
								PutItemInInventory(FindFreeInventorySlot(),item);
								PutItemInInventory(FindFreeInventorySlot(),item);
								}
							item = database.GetItem(904);
							if(item != 0)
								PutItemInInventory(FindFreeInventorySlot(),item);

							item = database.GetItem(906);
							if(item != 0)
								PutItemInInventory(FindFreeInventorySlot(),item);
							}
						}
						else
#endif
							parse->Event(EVENT_SAY, target->GetNPCTypeID(), message, target, this->CastToMob());
					}
				}
				break;
			}
			Seperator sep(message);
			char command[200];
			snprintf(command, 200, "%s", sep.arg[0]);
			command[0] = '!';
			sint16 cmdlevel = database.CommandRequirement(command);
/*			if (admin>200)
				admin=250;
			else if (admin==200)
				admin=200;
			else if ((admin>149) && (admin<200))
				admin=150;
			else if ((admin>99) && (admin<150))
				admin=100;
			else if ((admin>79) && (admin<100))
				admin=80;
			else if ((admin>19) && (admin <80))
				admin=20;
			else if ((admin>9) && (admin<20))
				admin=10;
			else
				admin=0;*/
			commandlogged=false;
			if ((admin>=cmdlevel) || (cmdlevel==255)) {
				if (admin >= 250 || admin==cmdlevel)
					if (VHServerOP(&sep))
						break;
				if (admin >= 200 || admin==cmdlevel)
					if (ServerOP(&sep))
						break;
				if (admin >= 150 || admin==cmdlevel)
					if (LeadGM(&sep))
						break;
				if (admin >= 100 || admin==cmdlevel)
					if (NormalGM(&sep))
						break;
				if (admin >= 80 || admin==cmdlevel)
					if (QuestTroupe(&sep))
						break;
				if (admin >= 20 || admin==cmdlevel)
					if (VeryPrivUser(&sep))
						break;
				if (admin >= 10 || admin==cmdlevel)
					if (PrivUser(&sep))
						break;
				NormalUser(&sep);
			}
			else
				Message(0,"Your status is not high enough to use this command.");
				
		break;
	}
	default: {
		Message(0, "Channel (%i) not implemented",(int16)chan_num);
	}
	}
}

void Client::ChannelMessageSend(const char* from, const char* to, int8 chan_num, int8 language, const char* message, ...) {
	if ((chan_num == 11 || chan_num == 10) && !(this->Admin() >= 80)) // dont need to send /pr & /pet to everybody
		return;
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
	APPLAYER app;
	
	app.opcode = OP_ChannelMessage;
	app.size = sizeof(ChannelMessage_Struct)+strlen(buffer)+1;
	app.pBuffer = new uchar[app.size];
	memset(app.pBuffer, 0, app.size);
	ChannelMessage_Struct* cm = (ChannelMessage_Struct*) app.pBuffer;
	if (from == 0)
		strcpy(cm->sender, "ZServer");
	else if (from[0] == 0)
		strcpy(cm->sender, "ZServer");
	else
		strcpy(cm->sender, from);
	if (to != 0)
		strcpy((char *) cm->targetname, to);
	else if (chan_num == 7)
		strcpy(cm->targetname, pp.name); 
	else
		cm->targetname[0] = 0;
	cm->language = language;
	cm->chan_num = chan_num;
	//cm->cm_unknown4[0] = 0xff;
	cm->cm_unknown4[1] = 0xff; // One of these tells the client how well we know the language
	cm->cm_unknown4[2] = 0xff;
	cm->cm_unknown4[3] = 0xff;
	cm->cm_unknown4[4] = 0xff;
	strcpy(&cm->message[0], buffer);
	
	QueuePacket(&app);
}

void Client::Message(int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
	APPLAYER* app = new APPLAYER(OP_SpecialMesg);
	app->size = 4+strlen(buffer)+1;
	app->pBuffer = new uchar[app->size];
	SpecialMesg_Struct* sm=(SpecialMesg_Struct*)app->pBuffer;
	sm->msg_type = type;
	strcpy(sm->message, buffer);
	QueuePacket(app);
	delete app;	
}


void Client::SendHPUpdate()	 {
	APPLAYER* app = new APPLAYER;
	CreateHPPacket(app);
	app->priority = 1;
	
	if (GMHideMe())
		entity_list.QueueClientsStatus(this, app, false, admin);
	else
		entity_list.QueueCloseClients(this, app);
	delete app;
}

void Client::SetMaxHP() {
	if(dead)
		return;
	SetHP(CalcMaxHP());
	SendHPUpdate();
	Save();
}

void Client::AddEXP(int32 add_exp) {
	int32 add_aaxp = (int32)((float)add_exp * ((float)((float)pp.perAA) / 100.0f));
	int32 exp = GetEXP() + (add_exp - add_aaxp);

	int32 aaexp = GetAAXP() + add_aaxp;
	SetEXP(exp, aaexp, false);
	// FIXME!	int32 add_aaxp = (int32)((float)add_exp * ((float)pp.perAA / 100.0f));
	// FIXME!	SetEXP(GetEXP() + (add_exp - add_aaxp), GetAAXP() + add_aaxp, false);
	//	SetEXP(GetEXP() + add_exp, 0, false);
}

void Client::SetEXP(int32 set_exp, int32 set_aaxp, bool isrezzexp) {
	max_AAXP = GetEXPForLevel(52) - GetEXPForLevel(51);
	if (max_AAXP == 0 || GetEXPForLevel(GetLevel()) == 0xFFFFFFFF) {
		Message(13, "Error in Client::SetEXP. EXP not set.");
		return; // Must be invalid class/race
	}
	if ((set_exp + set_aaxp) > pp.exp) {
		if (isrezzexp)
			Message(0, "You regain some experience from resurrection.");	
		else
			Message(15, "You gain experience!!");
	}
	else
		Message(15, "You have lost experience.");
		
	int16 check_level = GetLevel()+1;
	while (set_exp >= GetEXPForLevel(check_level)) {
		check_level++;
		if (check_level > 100) { // Quagmire - this was happening because GetEXPForLevel returned 0 on unknown race/class combo, Changed it to return 0xFFFFFFFF on error
			check_level = GetLevel()+1;
			break;
		}
	}
	while (set_exp < GetEXPForLevel(check_level-1)) {
		check_level--;
		if (check_level < 2) {

			check_level = 2;
			break;
		}
	}
	
	if (set_aaxp >= max_AAXP) {
		int last_unspentAA = pp.aapoints;
		pp.aapoints = set_aaxp / max_AAXP;
		set_aaxp = set_aaxp - (max_AAXP * pp.aapoints);
		if(set_aaxp <=0) {
			set_aaxp = 0;
		}
		pp.expAA = set_aaxp;
		//pp.aapoints += set_aaxp / max_AAXP;

		pp.aapoints += last_unspentAA;

		set_aaxp = pp.expAA % max_AAXP;

		Message(15, "You have gained %d skill points!!", pp.aapoints - last_unspentAA);
		Message(15, "You now have %d skill points available to spend.", pp.aapoints);
	}
	
	pp.expAA = set_aaxp;
	int8 maxlevel = 66;
#ifdef GUILDWARS
	if(GuildDBID() == 0)
		maxlevel = NOGUILDCAPLEVEL;
	else
		maxlevel = GAINLEVEL;
#endif
	if ((GetLevel() != check_level-1) && !(check_level-1 >= maxlevel)) {
		if (GetLevel() == check_level-2)
			Message(15, "You have gained a level! Welcome to level %i!", check_level-1);
		else if (GetLevel() == check_level)
			Message(15, "You lost a level! You are now level %i!", check_level-1);
		else
			Message(15, "Welcome to level %i!", check_level-1);
		pp.exp = set_exp;	
		SetLevel(check_level-1);
	}

// neotokyo 09-Nov-02
//send the expdata in any case so the xp bar isnt stuck after leveling    
	APPLAYER* outapp = new APPLAYER(OP_ExpUpdate, sizeof(ExpUpdate_Struct));
	ExpUpdate_Struct* eu = (ExpUpdate_Struct*)outapp->pBuffer;
	int32 tmpxp1 = GetEXPForLevel(GetLevel()+1);
	int32 tmpxp2 = GetEXPForLevel(GetLevel());
	// Quag: crash bug fix... Divide by zero when tmpxp1 and 2 equalled each other, most likely the error case from GetEXPForLevel() (invalid class, etc)
	if (tmpxp1 != tmpxp2 && tmpxp1 != 0xFFFFFFFF && tmpxp2 != 0xFFFFFFFF) {
		double tmpxp = (double) ( (double) set_exp-tmpxp2 ) / ( (double) tmpxp1-tmpxp2 );
		eu->exp = (int32)(330.0f * tmpxp);
		QueuePacket(outapp);
	}
	safe_delete(outapp);
	pp.exp = set_exp;

	SendAAStats();
	if (admin>=100 && GetGM()) {
		Message(15, "[GM] You now have %d / %d EXP and %d / %d AA exp.", set_exp, GetEXPForLevel(GetLevel()+1), set_aaxp, max_AAXP);
	}
}


void Client::MovePC(int32 zoneID, float x, float y, float z, int8 ignorerestrictions, bool summoned) {
	MovePC(database.GetZoneName(zoneID), x, y, z, ignorerestrictions, summoned);
}

void Client::MovePC(const char* zonename, float x, float y, float z, int8 ignorerestrictions, bool summoned) {
	if(this->isgrouped && entity_list.GetGroupByClient(this) != 0)
        entity_list.GetGroupByClient(this)->DelMember(this->CastToMob());
	if (IsAIControlled() && zonename == 0) {
		GMMove(x, y, z);
		return;
	}

    zonesummon_ignorerestrictions = ignorerestrictions;
    
    APPLAYER* outapp = new APPLAYER;    

    if(summoned == true) {
        outapp->size = sizeof(GMSummon_Struct);
        outapp->pBuffer = new uchar[outapp->size];
        memset(outapp->pBuffer, 0, outapp->size);
        GMSummon_Struct* gms = (GMSummon_Struct*) outapp->pBuffer;

        strcpy(gms->charname, this->GetName());
        strcpy(gms->gmname, this->GetName());

        outapp->opcode = OP_GMSummon;        
        gms->x = (sint32) x;
        gms->y = (sint32) y;
        gms->z = (sint32) z;

        if (zonename == 0) {
            gms->zoneID = zone->GetZoneID();
        }
        else {
            gms->zoneID = database.GetZoneID(zonename);
            strcpy(zonesummon_name, zonename);
            zonesummon_x = x;
            zonesummon_y = y;
            zonesummon_z = z;
        }
    }
    else {
        outapp->size = sizeof(GMGoto_Struct);
        outapp->pBuffer = new uchar[outapp->size];
        memset(outapp->pBuffer, 0, outapp->size);
        GMGoto_Struct* gmg = (GMGoto_Struct*) outapp->pBuffer;

        strcpy(gmg->charname, this->GetName());
        strcpy(gmg->gmname, this->GetName());

        outapp->opcode = OP_GMGoto;
        gmg->x = (sint32) x;
        gmg->y = (sint32) y;
        gmg->z = (sint32) z;

        if (zonename == 0) {
            gmg->zoneID = zone->GetZoneID();
        }
        else {
            gmg->zoneID = database.GetZoneID(zonename);

            strcpy(zonesummon_name, zonename);
            zonesummon_x = x;
            zonesummon_y = y;
            zonesummon_z = z;
        }
    }
    
    /*int8 tmp[16] = { 0x7C, 0xEF, 0xFC, 0x0F, 0x80, 0xF3, 0xFC, 0x0F, 0xA9, 0xCB, 0x4A, 0x00, 0x7C, 0xEF, 0xFC, 0x0F };

    memcpy(gms->unknown2, tmp, 16);

    int8 tmp2[4] = { 0xE0, 0xE0, 0x56, 0x00 };

    memcpy(gms->unknown3, tmp2, 4);*/

    QueuePacket(outapp);
    delete outapp;
}

void Client::SetLevel(int8 set_level, bool command)
{
#ifdef GUILDWARS
	if(set_level > SETLEVEL) {
		Message(0,"You cannot exceed level 50 on a GuildWars Server.");
		return;
	}
#endif
	if (GetEXPForLevel(set_level) == 0xFFFFFFFF) {
    LogFile->write(EQEMuLog::Error,"Client::SetLevel() GetEXPForLevel(%i) = 0xFFFFFFFF", set_level);
		return;
	}
	
	APPLAYER* outapp = new APPLAYER(OP_LevelUpdate, sizeof(LevelUpdate_Struct));
	LevelUpdate_Struct* lu = (LevelUpdate_Struct*)outapp->pBuffer;
	lu->level = set_level;
	lu->level_old = level;
	level = set_level;
	
	if(set_level > pp.level) // Yes I am aware that you could delevel yourself and relevel this is just to test!
		pp.points += 5;
	
	pp.level = set_level;
	if (command){
		pp.exp = GetEXPForLevel(set_level);	
		Message(15, "Welcome to level %i!", set_level);
		lu->exp = 0;
	}
	else
    {
        // NEOTOKYO 09-Nov-02
        // applied correct version of xp formula
        double tmpxp = (double) ( (double) pp.exp - GetEXPForLevel( GetLevel() )) / 
                     ( (double) GetEXPForLevel(GetLevel()+1) - GetEXPForLevel(GetLevel()));
        lu->exp =  (int32)(330.0f * tmpxp);

        //didnt work correctly
        //lu->exp = ((int)((float)330/(GetEXPForLevel(GetLevel()+1)-GetEXPForLevel(GetLevel()))) * (pp.exp-GetEXPForLevel(GetLevel())));
    }
	QueuePacket(outapp);
	delete outapp;
	this->SendAppearancePacket(0x01, set_level); // who level change

    LogFile->write(EQEMuLog::Normal,"Setting Level for %s to %i", GetName(), set_level);

	SetHP(CalcMaxHP());		// Why not, lets give them a free heal
	SendHPUpdate();
	SetMana(CalcMaxMana());
	UpdateWho();
	Save();
	//	ChannelMessageSend(0, 0, 7, 0, "Your level changed, please check if your max hp is %i. If not do a bug report with class, level, stamina and max hp", GetMaxHP());
}
//                           hum       bar    eru     elf     hie     def     hef     dwa     tro     ogr     hal    gno     iks,    vah
float  race_modifiers[14] =  {100.0f, 105.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 120.0f, 115.0f, 95.0f, 100.0f, 120.0f, 100.0f}; // Quagmire - Guessed on iks and vah
//                           war  cle  pal  ran  shd  dru  mnk  brd  rog  shm  nec  wiz  mag  enc bst
float class_modifiers[15] = { 9.0f, 10.0f, 14.0f, 14.0f, 14.0f, 10.0f, 12.0f, 14.0f, 9.05f, 10.0f, 11.0f, 11.0f, 11.0f, 11.0f, 10.0f};

/*
Note: The client calculates exp separatly, we cant change this function
Add: You can set the values you want now, client will be always sync :) - Merkur
*/
uint32 Client::GetEXPForLevel(int16 check_level)
{
	int16 tmprace = GetBaseRace();
	if (tmprace == IKSAR) // Quagmire, set these up so they read from array right
		tmprace = 12;
	else if (tmprace == VAHSHIR)
		tmprace = 13;
	
	tmprace -= 1;
	if (tmprace > 14 || GetClass() < 1 || GetClass() > 15)
		return 0xFFFFFFFF;
	
	int16 check_levelm1 = check_level-1;
	if (check_level < 31)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]);
	else if (check_level < 36)

		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.1);
	else if (check_level < 41)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.2);
	else if (check_level < 46)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.3);
	else if (check_level < 52)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.4);
	else if (check_level < 53)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.5);
	else if (check_level < 54)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.6);
	else if (check_level < 55)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.7);
	else if (check_level < 56)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.9);
	else if (check_level < 57)

		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*2.1);
	else if (check_level < 58)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*2.3);
	else if (check_level < 59)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*2.5);
	else if (check_level < 60)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*2.7);
	else if (check_level < 61)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*3.0);
	else
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*3.1);
}

sint32 Client::CalcMaxHP() {
	max_hp = (CalcBaseHP() + itembonuses->HP + spellbonuses->HP);
	if (cur_hp > max_hp)
		cur_hp = max_hp;
	return max_hp;
}

/*
Note: The client calculates max hp separatly, we cant change this function
*/
sint32 Client::CalcBaseHP()
{
	int multiplier=GetClassLevelFactor();
	
	
	if (multiplier == 0)
	{
		cerr << "Multiplier == 0 in Client::CalcBaseHP" << endl;
	}
#if DEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"Client::CalcBaseHP() multiplier:%i level:%i sta:%i", multiplier, GetLevel(), GetSTA());
#endif
	base_hp = 5+multiplier*GetLevel()+multiplier*GetLevel()*GetSTA()/300;
	return base_hp;
}

/* 
Added 12-29-01 -socket
*/

void Client::SetSkill(int16 skillid, int8 value) {
	if (skillid > HIGHEST_SKILL)
		return;
    pp.skills[skillid + 1] = value; // We need to be able to #setskill 254 and 255 to reset skills
	if(value <= 252)
	{
		APPLAYER* outapp = new APPLAYER(OP_SkillUpdate, sizeof(SkillUpdate_Struct));
		SkillUpdate_Struct* skill = (SkillUpdate_Struct*)outapp->pBuffer;
		skill->skillId=skillid;
		skill->value=value;
		QueuePacket(outapp);
		delete outapp;
	}
}

void Client::AddSkill(int16 skillid, int8 value) {
	if (skillid > HIGHEST_SKILL)
		return;
	value = GetSkill(skillid) + value;
	if (value > 252) value = 252;
	SetSkill(skillid, value);
}

/*
This should return the combined AC of all the items the player is wearing.
*/
int16 Client::GetRawItemAC() {
	const Item_Struct* TempItem;
	int16 Total = 0;
	
	for(int x=0; x<=20; x++) {
		TempItem = database.GetItem(pp.inventory[x]);
		if (TempItem)
			Total += TempItem->common.AC;
	}
	
	return Total;
}

/*
This is a testing formula for AC, the value this returns should be the same value as the one the client shows...
ac1 and ac2 are probably the damage migitation and damage avoidance numbers, not sure which is which.
I forgot to include the iksar defense bonus and i cant find my notes now...
AC from spells are not included (cant even cast spells yet..)
*/
int16 Client::GetCombinedAC_TEST() {
	int ac1;
	
	ac1 = GetRawItemAC();
	if (pp.class_ != WIZARD && pp.class_ != MAGICIAN && pp.class_ != NECROMANCER && pp.class_ != ENCHANTER) {
		ac1 = ac1*4/3;
	}
	ac1 += GetSkill(DEFENSE)/3;
	if (GetAGI() > 70)
	{
		ac1 += GetAGI()/20;
	}
	
	int ac2;
	
	ac2 = GetRawItemAC();
	if (pp.class_ != WIZARD && pp.class_ != MAGICIAN && pp.class_ != NECROMANCER && pp.class_ != ENCHANTER)
		
	{
		ac2 = ac2*4/3;
	}
	ac2 += GetSkill(DEFENSE)*400/255;
	

	
	int combined_ac = (ac1+ac2)*1000/847;
	
	return combined_ac;


}

void Client::UpdateWho(int8 remove) {
	if (account_id == 0)
		return;
	if (!worldserver.Connected())
		return;
	ServerPacket* pack = new ServerPacket(ServerOP_ClientList, sizeof(ServerClientList_Struct));
	ServerClientList_Struct* scl = (ServerClientList_Struct*) pack->pBuffer;
	scl->remove = remove;
	scl->wid = this->GetWID();
	scl->IP = this->GetIP();
	scl->charid = this->CharacterID();
	strcpy(scl->name, this->GetName());

	scl->gm = GetGM();
	scl->Admin = this->Admin();
	scl->AccountID = this->AccountID();
	strcpy(scl->AccountName, this->AccountName());
	scl->LSAccountID = this->LSAccountID();
	strn0cpy(scl->lskey, lskey, sizeof(scl->lskey));
	scl->zone = zone->GetZoneID();
	scl->race = this->GetRace();
	scl->class_ = GetClass();
	scl->level = GetLevel();
	if (pp.anon == 0)
		scl->anon = 0;
	else if (pp.anon == 1)
		scl->anon = 1;
	else if (pp.anon >= 2)
		scl->anon = 2;
	scl->tellsoff = tellsoff;
	scl->guilddbid = guilddbid;
	scl->guildeqid = guildeqid;
	scl->LFG = LFG;
	
	worldserver.SendPacket(pack);
	delete pack;
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
  delete outapp;
}*/

void Client::WhoAll(Who_All_Struct* whom) {
	if (!worldserver.Connected())
		Message(0, "Error: World server disconnected");

	else {
		ServerPacket* pack = new ServerPacket(ServerOP_Who, sizeof(ServerWhoAll_Struct));
		ServerWhoAll_Struct* whoall = (ServerWhoAll_Struct*) pack->pBuffer;
		whoall->admin = (int8) admin;
		strcpy(whoall->from, this->GetName());
		strcpy(whoall->whom, whom->whom);
		whoall->lvllow = whom->lvllow;
		whoall->lvlhigh = whom->lvlhigh;
		whoall->gmlookup = whom->gmlookup;
		whoall->wclass = whom->wclass;
		whoall->wrace = whom->wrace;
		worldserver.SendPacket(pack);
		delete pack;
	}
}

bool Client::SetGuild(int32 in_guilddbid, int8 in_rank) {
	if (in_guilddbid == 0) {
		// update DB
		if (!database.SetGuild(character_id, 0, GUILD_MAX_RANK))
			return false;
		// clear guildtag
		guilddbid = in_guilddbid;
		guildeqid = 0xFFFFFFFF;
		guildrank = GUILD_MAX_RANK;
		SendAppearancePacket(22, 0xFFFFFFFF);
		SendAppearancePacket(23, 3);
		UpdateWho();
		return true;
	}
	else {
		int32 tmp = database.GetGuildEQID(in_guilddbid);
		if (tmp != 0xFFFFFFFF) {
			if (!database.SetGuild(character_id, in_guilddbid, in_rank))
				return false;
			guildeqid = tmp;
			guildrank = in_rank;
			if (guilddbid != in_guilddbid) {
				guilddbid = in_guilddbid;
				SendAppearancePacket(22, guildeqid);
			}
			
			if (guilds[tmp].rank[in_rank].warpeace || guilds[tmp].leader == account_id)
				SendAppearancePacket(23, 2);
			else if (guilds[tmp].rank[in_rank].invite || guilds[tmp].rank[in_rank].remove || guilds[tmp].rank[in_rank].motd)
				SendAppearancePacket(23, 1);
			else
				SendAppearancePacket(23, 0);
			UpdateWho();
			return true;
		}
	}
	UpdateWho();
	return false;
}

bool Client::VHServerOP(const Seperator* sep){
	if ((zone->loglevelvar<8) && (zone->loglevelvar>0) && (commandlogged==false) && (admin >= 250))
		Commandlog(sep);
	if (strcasecmp(sep->arg[0], "#eitem") == 0) {
#ifdef SHAREMEM
		Message(0, "Error: Function doesnt work in ShareMem mode");
#else
		char hehe[255];
		if (strstr(sep->arg[2],"classes")) {



			snprintf(hehe,255,"%s %s",sep->arg[3],strstr(sep->argplus[0],sep->arg[3]));
		}
		else {
			strcpy(hehe,sep->arg[3]);
		}
		database.SetItemAtt(sep->arg[2],hehe,atoi(sep->arg[1]));
#endif
	}
	else if ((strcasecmp(sep->arg[0], "#setfaction") == 0)) {
		if ((sep->arg[1][0] == 0 || strcasecmp(sep->arg[1],"*")==0) || ((target==0) || (target->IsClient())))
			Message(0, "Usage: #setfaction (faction number)");
		else {
			char errbuf[MYSQL_ERRMSG_SIZE];
			char *query = 0;
			MYSQL_RES *result;
			Message(15,"Setting NPC %u to faction %i",target->CastToNPC()->GetNPCTypeID(),atoi(sep->argplus[1]));
			if (database.RunQuery(query, MakeAnyLenString(&query, "update npc_types set npc_faction_id=%i where id=%i",atoi(sep->argplus[1]),target->CastToNPC()->GetNPCTypeID()), errbuf, &result)) {
				delete[] query;
			}
		}
	}
	else if (strcasecmp(sep->arg[0], "#serversidename") == 0) {
		if (target) {
			Message(0, target->GetName());
			cout << "#serversidename: " << target->GetName() << endl;
		}
		else
			Message(0, "Error: no target");
	}
#if _DEBUG
	else if (strcasecmp(sep->arg[0], "#testspawnkill") == 0) {
		APPLAYER* outapp = new APPLAYER(OP_Death, sizeof(Death_Struct));
		Death_Struct* d = (Death_Struct*)outapp->pBuffer;
		d->corpseid = 1000;
		//  d->unknown011 = 0x05;
		d->spawn_id = 1000;
		d->killer_id = GetID();
		d->damage = 1;
		d->spell_id = 0;
		d->type = BASH;
		d->unknownds016 = 0;
		FastQueuePacket(&outapp);
	}
	else if (strcasecmp(sep->arg[0], "#testspawn") == 0) {
		if (sep->IsNumber(1)) {
			APPLAYER* outapp = new APPLAYER(OP_NewSpawn, sizeof(NewSpawn_Struct));
			NewSpawn_Struct* ns = (NewSpawn_Struct*)outapp->pBuffer;
			FillSpawnStruct(ns, this);
			strcpy(ns->spawn.name, "Test");
			ns->spawn.spawn_id = 1000;
			ns->spawn.NPC = 1;
			if (sep->IsHexNumber(2)) {
				if (strlen(sep->arg[2]) >= 3) // 0x00, 1 byte
					*(&((int8*) &ns->spawn)[atoi(sep->arg[1])]) = hextoi(sep->arg[2]);
				else if (strlen(sep->arg[2]) >= 5) // 0x0000, 2 bytes
					*((int16*) &(((int8*) &ns->spawn)[atoi(sep->arg[1])])) = hextoi(sep->arg[2]);
				else if (strlen(sep->arg[2]) >= 9) // 0x0000, 2 bytes
					*((int32*) &(((int8*) &ns->spawn)[atoi(sep->arg[1])])) = hextoi(sep->arg[2]);
				else
					Message(0, "Error: unexpected hex string length");
			}
			else {
				strcpy((char*) (&((int8*) &ns->spawn)[atoi(sep->arg[1])]), sep->argplus[2]);
			}

			outapp->Deflate();
			EncryptSpawnPacket(outapp);
			FastQueuePacket(&outapp);
		}
		else
			Message(0, "Usage: #testspawn memloc value - spawns a NPC for you only, with the specified values set in the spawn struct");
	}
#endif
	else if (strcasecmp(sep->arg[0], "#wc") == 0) {
		APPLAYER* outapp = new APPLAYER(OP_WearChange, sizeof(WearChange_Struct));
		WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;
		wc->spawn_id = target->GetID();
		wc->wear_slot_id = atoi(sep->arg[1]);
		wc->material = atoi(sep->arg[2]);
		entity_list.QueueClients(this, outapp);
		delete outapp;
	}
	else if (strcasecmp(sep->arg[0], "#numauths") == 0) {
		Message(0, "NumAuths: %i", zone->CountAuth());
		Message(0, "Your WID: %i", this->GetWID());
	}
	else if (strcasecmp(sep->arg[0], "#SetAnim") == 0) {
		if (target && sep->IsNumber(1)) {
			target->SetAppearance(atoi(sep->arg[1]));
		}
		else
			Message(0, "Usage: #SetAnim [AnimNum]");
	}
	else if (strcasecmp(sep->arg[0], "#connectworldserver") == 0) {
		if (worldserver.Connected())
			Message(0, "Error: Already connected to world server");
		else {
			Message(0, "Attempting to connect to world server...");
			worldserver.AsyncConnect();
		}
	}
	else if (strcasecmp(sep->arg[0], "#serverinfo") == 0) {
		if(strlen(sep->arg[1]) == 0) {
			Message(13,"Options: #serverinfo <xx> (os)");
		}
		else if (strcasecmp(sep->arg[1], "os") == 0) {
#ifdef WIN32
			GetOS();
			char intbuffer [sizeof(unsigned long)];
			Message(0, "Operating system information.");
			Message(0, "  %s", Ver_name);
			Message(0, "  Build number: %s", ultoa(Ver_build, intbuffer, 10));
			Message(0, "  Minor version: %s", ultoa(Ver_min, intbuffer, 10));
			Message(0, "  Major version: %s", ultoa(Ver_maj, intbuffer, 10));
			Message(0, "  Platform Id: %s", ultoa(Ver_pid, intbuffer, 10));
#else
			char os_string[100];
			Message(0, "Operating system information.");
			Message(0, "  %s", GetOS(os_string));
#endif
		}
		else {
			Message(0, "Usage: #serverinfo <type>");
			Message(0, "  OS - Operating system information.");
		}
	}
	else if (strcasecmp(sep->arg[0],"#crashtest") == 0) {
		this->Message(0,"Alright, now we get an GPF ;) ");
		char* gpf;
		gpf = 0;
		memcpy(gpf,"Ready to crash",30);
	}
	else if (strcasecmp(sep->arg[0], "#GetVariable") == 0) {
		char tmp[512];
		if (database.GetVariable(sep->argplus[1], tmp, sizeof(tmp)))
			Message(0, "%s = %s", sep->argplus[1], tmp);
		else
			Message(0, "GetVariable(%s) returned false", sep->argplus[1]);
	}
	else if (strcasecmp(sep->arg[0], "#chat") == 0) {
		// sends any channel message to all zoneservers
		// Dev debug command, for learing channums
		if (sep->arg[2][0] == 0)
			Message(0, "Usage: #chat channum message");

		else {
			if (!worldserver.SendChannelMessage(0, 0, (uint8) atoi(sep->arg[1]), 0, 0, sep->argplus[2]))
				Message(0, "Error: World server disconnected");
		}
	}
	else if (strcasecmp(sep->arg[0], "#ShowPetSpell") == 0) {
		if (sep->arg[1][0] == 0)
			Message(0, "Usage: #ShowPetSpells [petsummonstring]");
		else if (!spells_loaded)
			Message(0, "Spells not loaded");
		else if (Seperator::IsNumber(sep->argplus[1])) {
			int spellid = atoi(sep->argplus[1]);
			if (spellid <= 0 || spellid >= SPDAT_RECORDS) {
				Message(0, "Error: Number out of range");
			}
			else {
				Message(0, "  %i: %s, %s", spellid, spells[spellid].teleport_zone, spells[spellid].name);
			}
		}
		else {
			int count=0;
			//int iSearchLen = strlen(sep->argplus[1])+1;

			char sName[64];
			char sCriteria[65];
			strncpy(sCriteria, sep->argplus[1], 64);
			strupr(sCriteria);
			for (int i = 0; i < SPDAT_RECORDS; i++)
			{
				if (spells[i].name[0] != 0 && (spells[i].effectid[0] == SE_SummonPet || spells[i].effectid[0] == SE_NecPet)) {
					strcpy(sName, spells[i].teleport_zone);
					
					strupr(sName);
					char* pdest = strstr(sName, sCriteria);
					if ((pdest != NULL) && (count <=20)) {
						Message(0, "  %i: %s, %s", i, spells[i].teleport_zone, spells[i].name);
						count++;
					}
					else if (count > 20)
						break;
				}
			}
			if (count > 20)
				Message(0, "20 spells found... max reached.");
			else
				Message(0, "%i spells found.", count);
		}
	}
	else if (strcasecmp(sep->arg[0], "#ipc") == 0) {
	    if (target && target->IsNPC()){
	         if (target->CastToNPC()->IsInteractive()){
	             target->CastToNPC()->interactive = false;
	             Message(0, "Disableing IPC");
	         }
	         else {
	             target->CastToNPC()->interactive = true;
	             Message(0, "Enableing IPC");
	         }
	    }
	}
	else
		return false;
	return true;
}
bool Client::QuestTroupe(const Seperator* sep){
	bool found = false;
	if ((zone->loglevelvar<8) && (zone->loglevelvar>3) && (commandlogged==false) && (admin >= 80))
		Commandlog(sep);
	if (strcasecmp(sep->arg[0], "#npcloot") == 0) {
		if (target == 0)
			Message(0, "Error: No target");
		// #npcloot show
		else if (strcasecmp(sep->arg[1], "show") == 0) {
			if (target->IsNPC())
				target->CastToNPC()->QueryLoot(this);
			else if (target->IsCorpse())
				target->CastToCorpse()->QueryLoot(this);
			else
				Message(0, "Error: Target's type doesnt have loot");
		}
		// These 2 types are *BAD* for the next few commands
		else if (target->IsClient() || target->IsCorpse())
			Message(0, "Error: Invalid target type, try a NPC =).");
		// #npcloot add
		else if (strcasecmp(sep->arg[1], "add") == 0) {
			// #npcloot add item
			if (target->IsNPC() && sep->IsNumber(2)) {
				int32 item = atoi(sep->arg[2]);
				if (database.GetItem(item)) {
					if (sep->arg[3][0] != 0 && sep->IsNumber(3))
						target->CastToNPC()->AddItem(item, atoi(sep->arg[3]), 0);
					else
						target->CastToNPC()->AddItem(item, 1, 0);
					Message(0, "Added item(%i) to the %s's loot.", item, target->GetName());
				}
				else
					Message(0, "Error: #npcloot add: Item(%i) does not exist!", item);
			}
			else if (!sep->IsNumber(2))
				Message(0, "Error: #npcloot add: Itemid must be a number.");
			else
				Message(0, "Error: #npcloot add: This is not a valid target.");
		}
		// #npcloot remove
		else if (strcasecmp(sep->arg[1], "remove") == 0) {
			//#npcloot remove all
			if (strcasecmp(sep->arg[2], "all") == 0)
				Message(0, "Error: #npcloot remove all: Not yet implemented.");
			//#npcloot remove itemid
			else {
				if(target->IsNPC() && sep->IsNumber(2)) {
					int32 item = atoi(sep->arg[2]);
					target->CastToNPC()->RemoveItem(item);
					Message(0, "Removed item(%i) from the %s's loot.", item, target->GetName());
				}
				else if (!sep->IsNumber(2))					
					Message(0, "Error: #npcloot remove: Item must be a number.");
				else
					Message(0, "Error: #npcloot remove: This is not a valid target.");
			}
		}
		// #npcloot money
		else if (strcasecmp(sep->arg[1], "money") == 0) {
			if (target->IsNPC() && sep->IsNumber(2) && sep->IsNumber(3) && sep->IsNumber(4) && sep->IsNumber(5)) {
				if ((atoi(sep->arg[2]) < 34465 && atoi(sep->arg[2]) >= 0) && (atoi(sep->arg[3]) < 34465 && atoi(sep->arg[3]) >= 0) && (atoi(sep->arg[4]) < 34465 && atoi(sep->arg[4]) >= 0) && (atoi(sep->arg[5]) < 34465 && atoi(sep->arg[5]) >= 0)) {
					target->CastToNPC()->AddCash(atoi(sep->arg[5]), atoi(sep->arg[4]), atoi(sep->arg[3]), atoi(sep->arg[2]));
					Message(0, "Set %i Platinum, %i Gold, %i Silver, and %i Copper as %s's money.", atoi(sep->arg[2]), atoi(sep->arg[3]), atoi(sep->arg[4]), atoi(sep->arg[5]), target->GetName());
				}
				else
					Message(0, "Error: #npcloot money: Values must be between 0-34465.");
			}
			else

				Message(0, "Usage: #npcloot money platinum gold silver copper");
		}
		else
			Message(0, "Usage: #npcloot [show/money/add/remove] [itemid/all/money: pp gp sp cp]");
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#log") == 0 && admin >= 80) {
	if(strlen(sep->arg[4]) == 0 || strlen(sep->arg[1]) == 0 || strlen(sep->arg[2]) == 0 || (strlen(sep->arg[3]) == 0 && atoi(sep->arg[3]) == 0))
	{
	Message(0,"#log <type> <byaccountid/bycharname> <querytype> <details> <target/none> <timestamp>");
	Message(0,"(Req.) Types: 1) Command, 2) Merchant Buying, 3) Merchant Selling, 4) Loot, 5) Money Loot 6) Trade");
	Message(0,"(Req.) byaccountid/bycharname: choose either byaccountid or bycharname and then set querytype to effect it");
	Message(0,"(Req.) Details are information about the event, for example, partially an items name, or item id.");
	Message(0,"Timestamp allows you to set a date to when the event occured: YYYYMMDDHHMMSS (Year,Month,Day,Hour,Minute,Second). It can be a partial timestamp.");
	Message(0,"Note: when specifying a target, spaces in EQEMu use '_'");
	return true;
	// help
	}
	CharacterEventLog_Struct* cel = new CharacterEventLog_Struct;
	memset(cel,0,sizeof(CharacterEventLog_Struct));
	if(strcasecmp(sep->arg[2], "byaccountid") == 0)
	{
	database.GetEventLogs("",sep->arg[5],atoi(sep->arg[3]),atoi(sep->arg[1]),sep->arg[4],sep->arg[6],cel);
	}
	else if(strcasecmp(sep->arg[2], "bycharname") == 0)
	{
	database.GetEventLogs(sep->arg[3],sep->arg[5],0,atoi(sep->arg[1]),sep->arg[4],sep->arg[6],cel);
	}
	else
	{
	Message(0,"Incorrect query type, use either byaccountid or bycharname");
	delete cel;
	return true;
	}
	if(cel->count != 0)
	{
	int32 count = 0;
	bool cont = true;
	while(cont)
	{
	if(count >= cel->count)
	cont = false;
	else if(cel->eld[count].id != 0)
	{
	Message(0,"ID: %i AccountName: %s AccountID: %i Status: %i CharacterName: %s TargetName: %s",cel->eld[count].id,cel->eld[count].accountname,cel->eld[count].account_id,cel->eld[count].status,cel->eld[count].charactername,cel->eld[count].targetname);
	Message(0,"LogType: %s Timestamp: %s LogDetails: %s",cel->eld[count].descriptiontype,cel->eld[count].timestamp,cel->eld[count].details);
	}
	else
	cont = false;

	count++;

	if(count > 20)
	{
		Message(0,"Please refine search.");
		cont = false;
	}
	}
	}
	Message(0,"End of Query");
	delete cel;
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#gm") == 0) {
		if (target && target->IsClient() && admin >= 150) {
			if(strcasecmp(sep->arg[1], "on") == 0 || atoi(sep->arg[1]) == 1) {
				target->CastToClient()->SetGM(true);
				Message(13, "%s is now a GM!", target->CastToClient()->GetName());
			}
			else if(strcasecmp(sep->arg[1], "off") == 0 || atoi(sep->arg[1]) == 0) {
				target->CastToClient()->SetGM(false);
				Message(13, "%s is no longer a GM!", target->CastToClient()->GetName());
			}
			else {
				Message(0, "Usage: #gm [on|off]");
			}
		}
		else {
			if(strcasecmp(sep->arg[1], "on") == 0) {
				SetGM(true);
			}
			else if(strcasecmp(sep->arg[1], "off") == 0) {
				SetGM(false);
			}
			else {
				Message(0, "Usage: #gm [on|off]");
			}
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#summon") == 0) {
		/*EEND*/
		/* ORIGINAL CODE
		else if (strcasecmp(sep->arg[0], "#summon") == 0 && CheckAccess(cmdlevel, 100)) {
		*/
		if (sep->arg[1][0] == 0) {
			if (target && target->IsNPC()) {
				target->CastToNPC()->GMMove(x_pos, y_pos, z_pos, heading);
				target->SaveGuardSpot(true);
			}
			else if (target && target->IsCorpse())
				target->CastToCorpse()->GMMove(x_pos, y_pos, z_pos, heading);
			else {
				if (admin >= 150)
					Message(0, "Usage: #summon [charname]");
				else
					Message(0, "You need a NPC/corpse target for this command");
			}
		found=true;
		}
		else if (admin >= 150) {
			Client* client = entity_list.GetClientByName(sep->arg[1]);
			if (client != 0) {
				Message(0, "Summoning %s to %1.1f, %1.1f, %1.1f", sep->arg[1], this->x_pos, this->y_pos, this->z_pos);
				client->MovePC((char*) 0, this->x_pos, this->y_pos, this->z_pos, 2, true);
			}
			else if (!worldserver.Connected())
				Message(0, "Error: World server disconnected");
			else {
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_ZonePlayer;
				pack->size = sizeof(ServerZonePlayer_Struct);
				pack->pBuffer = new uchar[pack->size];
				ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
				strcpy(szp->adminname, this->GetName());
				szp->adminrank = this->Admin();
				szp->ignorerestrictions = 2;
				strcpy(szp->name, sep->arg[1]);
				strcpy(szp->zone, zone->GetShortName());
				szp->x_pos = x_pos;
				szp->y_pos = y_pos;
				szp->z_pos = z_pos;
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#zone") == 0) {
		if (sep->arg[1][0] == 0)
        {
			Message(0, "Usage: #zone [zonename]");
			Message(0, "Optional Usage: #zone [zonename] y x z");
		}
		else if (zone->GetZoneID() == 184 && Admin() < 80) {
			Message(0, "The Gods brought you here, only they can send you away.");
		}
		else {
			if (strcasecmp(sep->arg[1], zone->GetShortName()) == 0 && sep->IsNumber(2) || sep->IsNumber(3) || sep->IsNumber(4))
			{
				MovePC(sep->arg[1], atof(sep->arg[2]), atof(sep->arg[3]), atof(sep->arg[4]));
			}
			else if (strcasecmp(sep->arg[1], zone->GetShortName()) == 0)
			{
				//YXZ format because EQEmu coord system is messed up.
				MovePC(sep->arg[1], zone->safe_y(), zone->safe_x(), zone->safe_z());
			}
			else
				if((strcasecmp(sep->arg[1], "cshome")==0) && (admin <80))
				{Message(0,"Only Guides and above can goto that zone."); }
				else
					MovePC(sep->arg[1], -1, -1, -1);
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#showbuffs") == 0) {
		if (target == 0)
			CastToMob()->ShowBuffs(this);
		else

		found=true;
			target->CastToMob()->ShowBuffs(this);
	}
	else if (strcasecmp(sep->arg[0], "#movechar") == 0) {
		if(sep->arg[1][0]==0 || sep->arg[2][0] == 0)
			Message(0, "Usage: #movechar [charactername] [zonename]");
		else if (strcasecmp(sep->arg[2], "cshome") == 0 || strcasecmp(sep->arg[2], "load") == 0 || strcasecmp(sep->arg[2], "load2") == 0) {
			Message(0, "Invalid zone name");
		}
		else {
			int32 tmp = database.GetAccountIDByChar(sep->arg[1]);
			if (tmp){
				if (admin>=80 || tmp == this->AccountID()) 
{
					if (!database.MoveCharacterToZone((char*) sep->arg[1], (char*) sep->arg[2]))

						Message(0, "Character Move Failed!");
					else
						Message(0, "Character has been moved.");
				}
				else
					Message(13,"You cannot move characters that are not on your account.");
			}
			else
				Message(0, "Character Does Not Exist");
		}
		found=true;
	}
	else if ((strcasecmp(sep->arg[0], "#viewpetition") == 0)) {
		if (sep->arg[1][0] == 0) {
			Message(0, "Usage: #viewpetition (petition number) Type #listpetition for a list");
		} else {
			char errbuf[MYSQL_ERRMSG_SIZE];
			char *query = 0;
			int queryfound = 0;
			MYSQL_RES *result;
			MYSQL_ROW row;
			// Petition* newpet;
			Message(13,"	ID : Character Name , Petition Text");
			if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT petid, charname, petitiontext from petitions order by petid"), errbuf, &result))
			{
				delete[] query;
				while ((row = mysql_fetch_row(result))) {
					if (strcasecmp(row[0],sep->argplus[1])== 0) {
						queryfound=1;
						Message(15, " %s:	%s , %s ",row[0],row[1],row[2]);
					}	
				}
				LogFile->write(EQEMuLog::Normal,"View petition request from %s, petition number:", GetName(), atoi(sep->argplus[1]) );
				if (queryfound==0)
					Message(13,"There was an error in your request: ID not found! Please check the Id and try again.");
				mysql_free_result(result);
			}

		}
		found=true;
	}
	else if ((strcasecmp(sep->arg[0], "#petitioninfo") == 0)) {
		if (sep->arg[1][0] == 0) {
			Message(0, "Usage: #petitioninfo (petition number) Type #listpetition for a list");
		} else {
			char errbuf[MYSQL_ERRMSG_SIZE];
			char *query = 0;
			int queryfound = 0;
			MYSQL_RES *result;
			MYSQL_ROW row;
			// Petition* newpet;
			if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT petid, charname, accountname, zone, charclass, charrace, charlevel from petitions order by petid"), errbuf, &result))
			{
				delete[] query;
				while ((row = mysql_fetch_row(result))) {
					if (strcasecmp(row[0],sep->argplus[1])== 0) {
						queryfound=1;
						Message(13,"	ID : %s Character Name: %s Account Name: %s Zone: %s Character Class: %s Character Race: %s Character Level: %s",row[0],row[1],row[2],row[3],row[4],row[5],row[6]);
					}	
				}
				LogFile->write(EQEMuLog::Normal,"Petition information request from %s, petition number:", GetName(), atoi(sep->argplus[1]) );
				if (queryfound==0)
					Message(13,"There was an error in your request: ID not found! Please check the Id and try again.");
				mysql_free_result(result);
			}
		found=true;
		}
	}
	else if ((strcasecmp(sep->arg[0], "#delpetition") == 0)) {
		if (sep->arg[1][0] == 0 || strcasecmp(sep->arg[1],"*")==0)
			Message(0, "Usage: #delpetition (petition number) Type #listpetition for a list");
		else {
			char errbuf[MYSQL_ERRMSG_SIZE];
			char *query = 0;
			//int queryfound = 0;
			MYSQL_RES *result;
			//MYSQL_ROW row;
			//Petition* newpet;
			//char* blah32;
			//strcpy(blah32,sep->argplus[1]);
			//char* querystring;
			//querystring=strcat("DELETE from petitions where petid=",blah32);
			Message(13,"Attempting to delete petition number: %i",atoi(sep->argplus[1]));
			if (database.RunQuery(query, MakeAnyLenString(&query, "DELETE from petitions where petid=%i",atoi(sep->argplus[1])), errbuf, &result)) {
				delete[] query;
				LogFile->write(EQEMuLog::Normal,"Delete petition request from %s, petition number:", GetName(), atoi(sep->argplus[1]) );
			}
			//mysql_free_result(result); // If uncommented crashes zone. :/
		}
		found=true;
	}
	if ((strcasecmp(sep->arg[0], "#listpetition") == 0)) {
		char errbuf[MYSQL_ERRMSG_SIZE];
		char *query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;
		int blahloopcount=0;
		if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT petid, charname, accountname from petitions order by petid"), errbuf, &result))
		{
			delete[] query;
			LogFile->write(EQEMuLog::Normal,"Petition list requested by %s", this->GetName());
			while ((row = mysql_fetch_row(result))) {
			if (blahloopcount==0) {
				blahloopcount=1;
				Message(13,"	ID : Character Name , Account Name");
			}
			else
				Message(15, " %s:	%s , %s ",row[0],row[1],row[2]);
		}
		mysql_free_result(result);
		found=true;
		}
	}
	else if (strcasecmp(sep->arg[0], "#listNPCs") == 0) {
		if (strcasecmp(sep->arg[1], "all") == 0)
			entity_list.ListNPCs(this,sep->arg[1],sep->arg[2],0);
		else if(sep->IsNumber(1) && sep->IsNumber(2))
			entity_list.ListNPCs(this,sep->arg[1],sep->arg[2],2);
		else if(sep->arg[1][0] != 0)
			entity_list.ListNPCs(this,sep->arg[1],sep->arg[2],1);
		else {
			Message(0, "Usage of #listnpcs:");
			Message(0, "#listnpcs [#] [#] (Each number would search by ID, ex. #listnpcs 1 30, searches 1-30)");
			Message(0, "#listnpcs [name] (Would search for a npc with [name])");
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#date")==0) {
		//yyyy mm dd hh mm local
		if(sep->arg[3][0]==0 || !sep->IsNumber(1) || !sep->IsNumber(2) || !sep->IsNumber(3)) {
			Message(13, "Help: #date yyyy mm dd [HH MM]");
		}
		else {
			int h=0, m=0;
			TimeOfDay_Struct eqTime;
			zone->zone_time.getEQTimeOfDay( time(0), &eqTime);
			if(!sep->IsNumber(4))
				h=eqTime.hour;
			else
				h=atoi(sep->arg[4]);
			if(!sep->IsNumber(5))
				m=eqTime.minute;
			else
				m=atoi(sep->arg[5]);
			Message(13, "Setting world time to %s-%s-%s %i:%i...", sep->arg[1], sep->arg[2], sep->arg[3], h, m);
			zone->SetDate(atoi(sep->arg[1]), atoi(sep->arg[2]), atoi(sep->arg[3]), h, m);
		}
		found=true;
	}

	else if (strcasecmp(sep->arg[0], "#timezone")==0) {
		if(sep->arg[1][0]==0 && !sep->IsNumber(1)) {
			Message(13, "Help: #timezone HH [MM]");
			Message(13, "Current timezone is: %ih %im", zone->zone_time.getEQTimeZoneHr(), zone->zone_time.getEQTimeZoneMin());
		}
		else {
			if(sep->arg[2]=="")
				sep->arg[2]="0";
			Message(13, "Setting timezone to %s h %s m", sep->arg[1], sep->arg[2]);
			int32 ntz=(atoi(sep->arg[1])*60)+atoi(sep->arg[2]);
			zone->zone_time.setEQTimeZone(ntz);
			database.SetZoneTZ(zone->GetZoneID(), ntz);
			//Update all clients with new TZ.
			APPLAYER* outapp = new APPLAYER(OP_TimeOfDay, sizeof(TimeOfDay_Struct));
			TimeOfDay_Struct* tod = (TimeOfDay_Struct*)outapp->pBuffer;
			zone->zone_time.getEQTimeOfDay(time(0), tod);
			entity_list.QueueClients(this, outapp);
			delete outapp;
		}
		found=true;
	}
	//End timezone updates.
	else if (strcasecmp(sep->arg[0], "#synctod")==0) {
		Message(13, "Updating Time/Date for all clients in zone...");
		APPLAYER* outapp = new APPLAYER(OP_TimeOfDay, sizeof(TimeOfDay_Struct));
		TimeOfDay_Struct* tod = (TimeOfDay_Struct*)outapp->pBuffer;
		zone->zone_time.getEQTimeOfDay(time(0), tod);
		entity_list.QueueClients(this, outapp);
		delete outapp;
		found=true;
	}
	else if ((strcasecmp(sep->arg[0], "#invul") == 0 || strcasecmp(sep->arg[0], "#invulnerable") == 0)) {
		Client* client = this;
		if (target && target->IsClient())
			client = target->CastToClient();
		if (sep->arg[1][0] != 0) {
			client->invulnerable = atobool(sep->arg[1]);
			if (client->invulnerable)
				client->Message(0, "You are now invulnerable from attack.");
			else if (client->invulnerable)
				client->Message(0, "You are no longer invulnerable from attack.");
		}
		else
			Message(0, "Usage: #invulnerable [1/0]");
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#hideme") == 0) {
		if ((sep->arg[1][0] != '0' && sep->arg[1][0] != '1') || sep->arg[1][1] != 0)
			Message(0, "Usage: #hideme [0/1]");
		else {
			gmhideme = atobool(sep->arg[1]);
			if (gmhideme) {
				APPLAYER app;
				CreateDespawnPacket(&app);
				entity_list.QueueClientsStatus(this, &app, true, 0, admin-1);
				entity_list.RemoveFromTargets(this);
				Message(13, "Removing you from spawn lists.");
			}
			else {
				APPLAYER app;
				CreateSpawnPacket(&app);
				entity_list.QueueClientsStatus(this, &app, true, 0, admin-1);
				Message(13, "Adding you back to spawn lists.");
			}
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#emote") == 0) {
		if (sep->arg[3][0] == 0)
			Message(0, "Usage: #emote [name | world | zone] type# message");
		else {
			if (strcasecmp(sep->arg[1], "zone") == 0)
				entity_list.Message(0, atoi(sep->arg[2]), sep->argplus[3]);
			else if (!worldserver.Connected())
				Message(0, "Error: World server disconnected");
			else if (strcasecmp(sep->arg[1], "world") == 0)
				worldserver.SendEmoteMessage(0, 0, atoi(sep->arg[2]), sep->argplus[3]);
			else
				worldserver.SendEmoteMessage(sep->arg[1], 0, atoi(sep->arg[2]), sep->argplus[3]);
		}
		found=true;
	}
    else if (strcasecmp(sep->arg[0], "#fov") == 0) {
        if (target) {
            if ( this->BehindMob(target, GetX(), GetY()))
                Message(0, "You are behind mob %s, it is looking to %d", target->GetName(), target->GetHeading());
            else
                Message(0, "You are NOT behind mob %s, it is looking to %d", target->GetName(), target->GetHeading());
        }
        else {
			Message(0, "I Need a target!");
        }
		found=true;
    }
	else if (strcasecmp(sep->arg[0], "#manastat") == 0) {
		if (target) {
			Message(0, "Mana for %s:", target->GetName());
			Message(0, "  Current Mana: %d",target->GetMana()); 
			Message(0, "  Max Mana: %d",target->GetMaxMana()); 
		}
		else {
			Message(0, "Mana for %s:", this->GetName());
			Message(0, "  Current Mana: %d",this->cur_mana); 
			Message(0, "  Max Mana: %d",this->max_mana); 
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#npcstats") == 0) {
		if (target == 0)
			Message(0, "ERROR: No target!");
		else if (!target->IsNPC())
			Message(0, "ERROR: Target is not a NPC!");
		else {
			Message(0, "NPC Stats:");
			Message(0, "  Name: %s",target->GetName()); 
			Message(0, "  NpcID: %u",target->GetNPCTypeID()); 
			Message(0, "  Race: %i",target->GetRace());  
			Message(0, "  Level: %i",target->GetLevel());
			Message(0, "  Material: %i",target->GetTexture());
			Message(0, "  Class: %i",target->GetClass());
			Message(0, "  Current HP: %i", target->GetHP());
			Message(0, "  Max HP: %i", target->GetMaxHP());
			//Message(0, "Weapon Item Number: %s",target->GetWeapNo()); 
			Message(0, "  Gender: %i",target->GetGender());
			Message(0, " Size: %f",target->GetSize()); 
			Message(0, " Runspeed: %f", target->GetRunspeed());
			Message(0, " Walkspeed: %f", target->GetWalkspeed());
			target->CastToNPC()->QueryLoot(this);
		}
		found=true;
	}
	else if(strcasecmp(sep->arg[0], "#zclip") == 0) {
		// modifys and resends zhdr packet
		if(sep->arg[2][0]==0)
			Message(0, "Usage: #zclip <min clip> <max clip>");
		else if(atoi(sep->arg[1])<=0)
			Message(0, "ERROR: Min clip can not be zero or less!");
		else if(atoi(sep->arg[2])<=0)
			Message(0, "ERROR: Max clip can not be zero or less!");
		else if(atoi(sep->arg[1])>atoi(sep->arg[2]))
			Message(0, "ERROR: Min clip is greater than max clip!");
		else {
			zone->newzone_data.minclip = atof(sep->arg[1]);
			zone->newzone_data.maxclip = atof(sep->arg[2]);
			//				float newdatamin = atof(sep->arg[1]);
			//				float newdatamax = atof(sep->arg[2]);
			//				memcpy(&zone->zone_header_data[134], &newdatamin, sizeof(float));
			//				memcpy(&zone->zone_header_data[138], &newdatamax, sizeof(float));
			
			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			delete outapp;
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#npccast") == 0) {
		if (target && target->IsNPC() && !sep->IsNumber(1) && sep->arg[1] != 0 && sep->IsNumber(2)) {
			Mob* spelltar = entity_list.GetMob(sep->arg[1]);
			if (spelltar) {
				target->CastSpell(atoi(sep->arg[2]), spelltar->GetID());
			}
			else {
				Message(0, "Error: %s not found", sep->arg[1]);
			}
		}
		else if (target && target->IsNPC() && sep->IsNumber(1) && sep->IsNumber(2) ) {
			Mob* spelltar = entity_list.GetMob(atoi(sep->arg[1]));
			if (spelltar) {
				target->CastSpell(atoi(sep->arg[2]), spelltar->GetID());
			}
			else {
				Message(0, "Error: target ID %i not found", atoi(sep->arg[1]));
			}
		}		
		else {
			Message(0, "Usage: (needs NPC targeted) #npccast targetname/entityid spellid");
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#npccast") == 0) {
	}
	else if (strcasecmp(sep->arg[0], "#zstats") == 0) {
		Message(0, "Zone Header Data:");
		Message(0, "Sky Type: %i", zone->newzone_data.sky);
		Message(0, "Sky Colour: Red: %i; Blue: %i; Green %i", zone->newzone_data.unknown230[1], zone->newzone_data.unknown230[5], zone->newzone_data.unknown230[9]);
		Message(0, "Safe Coords: %f, %f, %f", zone->newzone_data.safe_x, zone->newzone_data.safe_y, zone->newzone_data.safe_z);
		Message(0, "Underworld Coords: %f", zone->newzone_data.underworld);
		Message(0, "Clip Plane: %f - %f", zone->newzone_data.minclip, zone->newzone_data.maxclip);
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#permaclass")==0) {
		if(sep->arg[1][0]==0) {
			Message(0,"FORMAT: #permaclass <classnum>");
		}
		else if (target==0){
			Message(0, "Setting your class...Sending you to char select.");
			LogFile->write(EQEMuLog::Normal,"Class change request from %s, requested class:%i", GetName(), atoi(sep->arg[1]) );
			pp.class_=atoi(sep->arg[1]);
			Save();
			Kick();
		}
		else if( !target->IsClient())
			Message(0,"Target is not a client.");
		else 
		{
			Message(0, "Setting %s's class...Sending them to char select.",target->CastToClient()->GetName());
			target->CastToClient()->pp.class_=atoi(sep->arg[1]);
			target->CastToClient()->Save();
			target->CastToClient()->Kick();
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#permarace")==0) {
		if(sep->arg[1][0]==0) {
			Message(0,"FORMAT: #permarace <racenum>");
			Message(0,"NOTE: Not all models are global. If a model is not global, it will appear as a human on character select and in zones without the model.");
		}
		else if (target==0){
			
			Message(0, "Setting your race...Sending you to char select.");

			LogFile->write(EQEMuLog::Normal,"Permanant race change request from %s, requested race:%i", GetName(), atoi(sep->arg[1]) );
			int8 tmp = Mob::GetDefaultGender(atoi(sep->arg[1]), pp.gender); 
			pp.race=atoi(sep->arg[1]);
			pp.gender=tmp;
			Save();
			SendIllusionPacket(atoi(sep->arg[1]));
		}
		else if( !target->IsClient())
			Message(0,"Target is not a client.");
		else 
		{
			int8 tmp = Mob::GetDefaultGender(atoi(sep->arg[1]), target->CastToClient()->pp.gender);
			Message(0, "Setting %s's race...Sending them to char select.",target->CastToClient()->GetName());
			target->CastToClient()->pp.race=atoi(sep->arg[1]);
			target->CastToClient()->pp.gender=tmp;
			target->CastToClient()->Save();
			target->SendIllusionPacket(atoi(sep->arg[1]));
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#permagender")==0) {
		if(sep->arg[1][0]==0) {
			Message(0,"FORMAT: #permagender <gendernum>");
			Message(0,"Gender Numbers: 0=Male, 1=Female, 2=Neuter");
		}
		else if (target==0) {
			Message(0, "Setting your gender...Sending you to char select.");
			LogFile->write(EQEMuLog::Normal,"Permanant gender change request from %s, requested gender:%i", GetName(), atoi(sep->arg[1]) );
			pp.gender=atoi(sep->arg[1]);
			Save();
			this->SendIllusionPacket(this->GetRace(), gender);
		}
		else if (!target->IsClient())
				Message(0, "Target is not a client.");
		else {
			Message(0, "Setting %s's race...Sending them to char select.", target->CastToClient()->GetName());
			target->CastToClient()->pp.gender=atoi(sep->arg[1]);
			target->CastToClient()->Save();
			target->SendIllusionPacket(target->GetRace(), gender);
		}
		found=true;
	}
	else if(strcasecmp(sep->arg[0],"#weather") == 0) {
		if (!(sep->arg[1][0] == '0' || sep->arg[1][0] == '1' || sep->arg[1][0] == '2' || sep->arg[1][0] == '3')) {
			Message(0, "Usage: #weather <0/1/2/3> - Off/Rain/Snow/Manual.");
		}
		else if(zone->zone_weather == 0) { 
			if(sep->arg[1][0] == '3')	{ // Put in modifications here because it had a very good chance at screwing up the client's weather system if rain was sent during snow -T7
				if(sep->arg[2][0] != 0 && sep->arg[3][0] != 0) { 
					Message(0, "Sending weather packet... TYPE=%s, INTENSITY=%s", sep->arg[2], sep->arg[3]);
					zone->zone_weather = atoi(sep->arg[2]);
					APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
					outapp->pBuffer[0] = atoi(sep->arg[2]);
					outapp->pBuffer[4] = atoi(sep->arg[3]); // This number changes in the packets, intensity?
					entity_list.QueueClients(this, outapp);
					delete outapp;
				}
				else {
					Message(0, "Manual Usage: #weather 3 <type> <intensity>");
				}
			}
			else if(sep->arg[1][0] == '2')	{
				entity_list.Message(0, 0, "Snowflakes begin to fall from the sky.");
				zone->zone_weather = 2;
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				outapp->pBuffer[0] = 0x01;
				outapp->pBuffer[4] = 0x02; // This number changes in the packets, intensity?
				entity_list.QueueClients(this, outapp);
				delete outapp;
			}
			else if(sep->arg[1][0] == '1')	{
				entity_list.Message(0, 0, "Raindrops begin to fall from the sky.");
				zone->zone_weather = 1;
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				outapp->pBuffer[4] = 0x01; // This is how it's done in Fear, and you can see a decent distance with it at this value
				entity_list.QueueClients(this, outapp);
				delete outapp;
			}
		}
		else	{
			if(zone->zone_weather == 1)	{ // Doing this because if you have rain/snow on, you can only turn one off.
				entity_list.Message(0, 0, "The sky clears as the rain ceases to fall.");
				zone->zone_weather = 0;
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				// To shutoff weather you send an empty 8 byte packet (You get this everytime you zone even if the sky is clear)
				entity_list.QueueClients(this, outapp);
				delete outapp;
			}

			else if(zone->zone_weather == 2)	{
				entity_list.Message(0, 0, "The sky clears as the snow stops falling.");
				zone->zone_weather = 0;
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				// To shutoff weather you send an empty 8 byte packet (You get this everytime you zone even if the sky is clear)
				outapp->pBuffer[0] = 0x01; // Snow has it's own shutoff packet
				entity_list.QueueClients(this, outapp);
				delete outapp;
			}
			else {
				entity_list.Message(0, 0, "The sky clears.");
				zone->zone_weather = 0;
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				// To shutoff weather you send an empty 8 byte packet (You get this everytime you zone even if the sky is clear)
				entity_list.QueueClients(this, outapp);
				delete outapp;
			}
		}
		found=true;
	}	
	else if (strcasecmp(sep->arg[0], "#zheader")==0) {
		// sends zhdr packet
		if(sep->arg[1][0]==0) {
			Message(0, "Usage: #zheader <zone name>");
			Message(0, "NOTE: Use \"none\" for zone name to use default header.");
		}
		else {
			if (strcasecmp(sep->argplus[1], "none") == 0) {
				Message(0, "Loading default zone header");
				zone->LoadZoneCFG(0);
			}
			else {

				if (zone->LoadZoneCFG(sep->argplus[1], true))
					Message(0, "Successfully loaded zone header: %s.cfg", sep->argplus[1]);
				else
					Message(0, "Failed to load zone header: %s.cfg", sep->argplus[1]);
			}
			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			delete outapp;
		}
		found=true;

	}
	else if(strcasecmp(sep->arg[0], "#zsky") == 0) {
		// modifys and resends zhdr packet
		if(sep->arg[1][0]==0)
			Message(0, "Usage: #zsky <sky type>");
		else if(atoi(sep->arg[1])<0||atoi(sep->arg[1])>255)
			Message(0, "ERROR: Sky type can not be less than 0 or greater than 255!");
		else {
			zone->newzone_data.sky = atoi(sep->arg[1]);
			
			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			delete outapp;
		}
		found=true;
	}
	else if((strcasecmp(sep->arg[0], "#zcolor") == 0 || strcasecmp(sep->arg[0], "#zcolour") == 0)) {
		// modifys and resends zhdr packet
		if(sep->arg[3][0]==0)
			Message(0, "Usage: #zcolor <red> <green> <blue>");
		else if(atoi(sep->arg[1])<0||atoi(sep->arg[1])>255)
			Message(0, "ERROR: Red can not be less than 0 or greater than 255!");
		else if(atoi(sep->arg[2])<0||atoi(sep->arg[2])>255)
			Message(0, "ERROR: Green can not be less than 0 or greater than 255!");
		else if(atoi(sep->arg[3])<0||atoi(sep->arg[3])>255)
			Message(0, "ERROR: Blue can not be less than 0 or greater than 255!");
		else {
			for(int z=1;z<13;z++) {
				if(z<5)
					zone->newzone_data.unknown230[z] = atoi(sep->arg[1]);
				if(z>4 && z<9)
					zone->newzone_data.unknown230[z] = atoi(sep->arg[2]);
				if(z>8)
					zone->newzone_data.unknown230[z] = atoi(sep->arg[3]);
			}
			
			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			delete outapp;
		}
		found=true;
	}
    else if (strcasecmp(sep->arg[0], "#spon") == 0)
    {
        APPLAYER* outapp;


        outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
        MemorizeSpell_Struct* p = (MemorizeSpell_Struct*)outapp->pBuffer;
        p->slot = 0;
        p->spell_id = 0x2bc;
        p->scribing = 3;
        outapp->priority = 5;
        this->CastToClient()->QueuePacket(outapp);
        delete outapp;
		found=true;
    }
    else if (strcasecmp(sep->arg[0], "#spoff") == 0)
    {
        APPLAYER* outapp;

        outapp = new APPLAYER(OP_ManaChange, 0);
        outapp->priority = 5;
        this->CastToClient()->QueuePacket(outapp);
        delete outapp;
		found=true;
    }
	return found;
}
bool Client::ServerOP(const Seperator* sep){
	if ((zone->loglevelvar<8) && (zone->loglevelvar>0) && (commandlogged==false) && (admin >= 200))
		Commandlog(sep);
	if (strcasecmp(sep->arg[0], "#gassign") == 0) {
		if (sep->IsNumber(1) && target && target->IsNPC()) {
			database.AssignGrid((int)(target->CastToNPC()->org_x),(int)(target->CastToNPC()->org_y),atoi(sep->arg[1]));
		}
		else {
			Message(0,"Usage: #gassign num - must have a target!");
		}
	}
	else if (strcasecmp(sep->arg[0], "#setitemstatus") == 0) {
		if (sep->IsNumber(1) && sep->IsNumber(2)) {
			int32 tmp = atoi(sep->arg[1]);
			if (tmp >= 0xFFFF)
				Message(0, "Item# out of range");
			else if (!database.DBSetItemStatus(tmp, atoi(sep->arg[2])))
				Message(0, "DB query failed");
			else {
				Message(0, "Item updated");
				ServerPacket* pack = new ServerPacket(ServerOP_ItemStatus, 5);
				*((int32*) &pack->pBuffer[0]) = tmp;
				*((int8*) &pack->pBuffer[4]) = atoi(sep->arg[2]);
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
	}
	else if (strcasecmp(sep->arg[0], "#ai") == 0) {
		if (strcasecmp(sep->arg[1], "factionid") == 0) {
			if (target && sep->IsNumber(2)) {
				if (target->IsNPC())
					target->CastToNPC()->SetNPCFactionID(atoi(sep->arg[2]));
				else
					Message(0, "%s is not a NPC.", target->GetName());
			}
			else
				Message(0, "Usage: (targeted) #ai factionid [factionid]");
		}
		else if (strcasecmp(sep->arg[1], "spellslist") == 0) {
			if (target && sep->IsNumber(2) && atoi(sep->arg[2]) >= 0) {
				if (target->IsAIControlled())
					target->AI_AddNPCSpells(atoi(sep->arg[2]));
				else
					Message(0, "%s is not AI Controlled.", target->GetName());
			}
			else
				Message(0, "Usage: (targeted) #ai spellslist [npc_spells_id]");
		}
		else if (strcasecmp(sep->arg[1], "con") == 0) {
			if (target && sep->arg[2][0] != 0) {
				Mob* tar2 = entity_list.GetMob(sep->arg[2]);
				if (tar2)
					Message(0, "%s considering %s: %i", target->GetName(), tar2->GetName(), target->GetFactionCon(tar2));
				else
					Message(0, "Error: %s not found.", sep->arg[2]);

			}
			else
				Message(0, "Usage: (targeted) #ai con [mob name]");
		}
		else if (strcasecmp(sep->arg[1], "guard") == 0) {


			if (target)
				target->SaveGuardSpot();
			else
				Message(0, "Usage: (targeted) #ai guard - sets npc to guard the current location (use #summon to move)");
		}
		else if (strcasecmp(sep->arg[1], "roambox") == 0) {
			if (target && target->IsAIControlled()) {
				if ((sep->argnum == 6 || sep->argnum == 7) && sep->IsNumber(2) && sep->IsNumber(3) && sep->IsNumber(4) && sep->IsNumber(5) && sep->IsNumber(6)) {
					int32 tmp = 2500;
					if (sep->IsNumber(7))
						tmp = atoi(sep->arg[7]);

					target->AI_SetRoambox(atof(sep->arg[2]), atof(sep->arg[3]), atof(sep->arg[4]), atof(sep->arg[5]), atof(sep->arg[6]), tmp);
				}
				else if ((sep->argnum == 3 || sep->argnum == 4) && sep->IsNumber(2) && sep->IsNumber(3)) {
					int32 tmp = 2500;
					if (sep->IsNumber(4))
						tmp = atoi(sep->arg[4]);
					target->AI_SetRoambox(atof(sep->arg[2]), atof(sep->arg[3]), tmp);
				}
				else {
					Message(0, "Usage: #ai roambox dist max_x min_x max_y min_y [delay]");
					Message(0, "Usage: #ai roambox dist roamdist [delay]");
				}
			}
			else
				Message(0, "You need a AI Mob targeted");
		}
		else if (strcasecmp(sep->arg[1], "stop") == 0 && admin >= 250) {
			if (target) {
				if (target->IsAIControlled())
					target->AI_Stop();
				else
					Message(0, "Error: Target is not AI controlled");
			}
			else
				Message(0, "Usage: Target a Mob with AI enabled and use this to turn off their AI.");
		}
		else if (strcasecmp(sep->arg[1], "start") == 0 && admin >= 250) {
			if (target) {
				if (!target->IsAIControlled())
					target->AI_Start();
				else
					Message(0, "Error: Target is already AI controlled");
			}
			else
				Message(0, "Usage: Target a Mob with AI disabled and use this to turn on their AI.");
		}
		else {
			Message(0, "#AI Sub-commands");
			Message(0, "  factionid");
			Message(0, "  spellslist");
			Message(0, "  con");
			Message(0, "  guard");
		}
	}
	else if (strcasecmp(sep->arg[0], "#worldshutdown") == 0) {
		// GM command to shutdown world server and all zone servers
		if (worldserver.Connected()) {
			worldserver.SendEmoteMessage(0,0,15,"<SYSTEMWIDE MESSAGE>:SYSTEM MSG:World coming down, everyone log out now.");
			Message(0, "Sending shutdown packet");
			ServerPacket* pack = new ServerPacket;
			pack->opcode = ServerOP_ShutdownAll;
			pack->size=0;

			worldserver.SendPacket(pack);
			delete pack;
		}
		else
			Message(0, "Error: World server disconnected");
	}
	else if (strcasecmp(sep->arg[0], "#sendzonespawns") == 0) {


		entity_list.SendZoneSpawns(this);
	}
#ifdef GUILDWARS
	else if (strcasecmp(sep->arg[0], "#delcitydefense") == 0) {
		database.DeleteCityDefense(zone->GetZoneID());
		zone->Repop();
	}
#endif
	else if(strcasecmp(sep->arg[0], "#zsave") == 0) {
		// modifys and resends zhdr packet
		if(sep->arg[1][0]==0)
			Message(0, "Usage: #zsave <file name>");
		else {
			zone->SaveZoneCFG(sep->argplus[1]);
		}
	}
	else if (strcasecmp(sep->arg[0], "#dbspawn2") == 0) // Image's Spawn Code -- Rewrite by Scruffy
	{
		if (sep->IsNumber(1) && sep->IsNumber(2) && sep->IsNumber(3)) {
           LogFile->write(EQEMuLog::Normal,"Spawning database spawn");
			database.CreateSpawn2(atoi(sep->arg[1]), zone->GetShortName(), heading, x_pos, y_pos, z_pos, atoi(sep->arg[2]), atoi(sep->arg[3]));
		}
		else {
			Message(0, "Usage: #dbspawn2 spawngroup respawn variance");
		}
	}
	else if (strcasecmp(sep->arg[0], "#copychar") == 0) {
		if(sep->arg[1][0]==0 || sep->arg[2][0] == 0 || sep->arg[3][0] == 0)
			Message(0, "Usage: #copychar [character name] [new character] [new account id]");
		//CheckUsedName.... TRUE=No Char, FALSE=Char/Error
		//If there is no source...
		else if (database.CheckUsedName((char*) sep->arg[1])) {
			Message(0, "Source character not found!");
		}
		else {

			//If there is a name is not used....

			if (database.CheckUsedName((char*) sep->arg[2]) )

				if (!database.CopyCharacter((char*) sep->arg[1], (char*) sep->arg[2], atoi(sep->arg[3])))
					Message(0, "Character copy operation failed!");
				else
					Message(0, "Character copy complete.");
				else
					Message(0, "Target character already exists!");
		}
	}
	else if (strcasecmp(sep->arg[0], "#shutdown")==0) {
		CatchSignal(2);
	}
	else if (strcasecmp(sep->arg[0], "#delacct") == 0) {
		if(sep->arg[1][0] == 0) {
			Message(0, "Format: #delacct accountname");
		} else {
			if (database.DeleteAccount(sep->arg[1]))
				Message(0, "The account was deleted."); 
			else
				Message(0, "Unable to delete account.");
		}
	}
	else if (strcasecmp(sep->arg[0], "#setpass") == 0) {
		if(sep->argnum != 2)
			Message(0, "Format: #setpass accountname password");
		else {
			sint16 tmpstatus = 0;
			int32 tmpid = database.GetAccountIDByName(sep->arg[1], &tmpstatus);
			if (!tmpid)
				Message(0, "Error: Account not found");
			else if (tmpstatus > admin)
				Message(0, "Cannot change password: Account's status is higher than yours");
			else if (database.SetLocalPassword(tmpid, sep->arg[2]))
				Message(0, "Password changed.");
			else
				Message(0, "Error changing password."); 
		}
	}
	else if (strcasecmp(sep->arg[0], "#grid") == 0) {
		if (strcasecmp("add",sep->arg[1]) == 0) {
			database.ModifyGrid(false,atoi(sep->arg[2]),atoi(sep->arg[3]), atoi(sep->arg[4]));
		}
		else if (strcasecmp("delete",sep->arg[1]) == 0) {
			database.ModifyGrid(true,atoi(sep->arg[2]),0);
		}
		else {
			Message(0,"Usage: #grid add/delete grid_num wandertype pausetype");
		}
	}
	else if (strcasecmp(sep->arg[0], "#wp") == 0) {
		if (strcasecmp("add",sep->arg[1]) == 0) {
			database.ModifyWP(atoi(sep->arg[2]),atoi(sep->arg[4]), x_pos, y_pos, z_pos, atoi(sep->arg[3]));
		}
		else if (strcasecmp("delete",sep->arg[1]) == 0) {
			database.ModifyWP(atoi(sep->arg[2]),atoi(sep->arg[4]), 0, 0, 0, 0);
		}
		else {
			Message(0,"Usage: #wp add/delete grid_num pause wp_num");
		}
	}
	else if (strcasecmp(sep->arg[0], "#iplookup") == 0 && admin >= 201) {
		ServerPacket* pack = new ServerPacket(ServerOP_IPLookup, sizeof(ServerGenericWorldQuery_Struct) + strlen(sep->argplus[1]) + 1);
		ServerGenericWorldQuery_Struct* s = (ServerGenericWorldQuery_Struct *) pack->pBuffer;
		strcpy(s->from, this->GetName());
		s->admin = this->Admin();
		if (sep->argplus[1][0] != 0)
			strcpy(s->query, sep->argplus[1]);
		worldserver.SendPacket(pack);
		delete pack;
	}
	else
		return false;
	return true;
}

bool Client::VeryPrivUser(const Seperator* sep){
	bool found = false;
	if ((zone->loglevelvar<8) && (zone->loglevelvar>4) && (commandlogged==false) && (admin >= 20))
		Commandlog(sep);
	if (strcasecmp(sep->arg[0], "#size") == 0)	{
			if (!sep->IsNumber(1))
				Message(0, "Usage: #size [0 - 255]");
			else {
				float newsize = atof(sep->arg[1]);
				if (newsize > 255)
					Message(0, "Error: #size: Size can not be greater than 255.");
				else if (newsize < 0)
					Message(0, "Error: #size: Size can not be less than 0.");
				else {
					if (target)
						target->ChangeSize(newsize, true);
					else
						this->ChangeSize(newsize, true);
				}
			}
			found=true;
		}
	else if (strcasecmp(sep->arg[0], "#flymode") == 0) {
		if (strlen(sep->arg[1]) == 1 && !(sep->arg[1][0] == '0' || sep->arg[1][0] == '1' || sep->arg[1][0] == '2')) {
			Message(0, "#flymode [0/1/2]");
		}
		else {
			if (target == 0 || !target->IsClient() || admin < 100) {
				this->SendAppearancePacket(19, atoi(sep->arg[1]));

				if (sep->arg[1][0] == '1')
					Message(0, "Turning Flymode ON");
				else if (sep->arg[1][0] == '2')
					Message(0, "Turning Flymode LEV");
				else
					Message(0, "Turning Flymode OFF");
			}
			else {
				target->SendAppearancePacket(19, atoi(sep->arg[1]));
				if (sep->arg[1][0] == '1')
					Message(0, "Turning %s's Flymode ON", target->GetName());
				else if (sep->arg[1][0] == '2')
					Message(0, "Turning %s's Flymode LEV", target->GetName());
				else
					Message(0, "Turning %s's Flymode OFF", target->GetName());
			}
		}
		found=true;
	}
    else if (strcasecmp(sep->arg[0], "#showskills") == 0)
    {
        for (int i = 0; i < 74; i++)
        {
            Message(0, "Skill [%d] is now [%d]", i, GetSkill(i));
        }
		found=true;
    }
	else if ((strcasecmp(sep->arg[0], "#FindSpell") == 0 || strcasecmp(sep->arg[0], "#spfind") == 0)) {
		if (sep->arg[1][0] == 0)
			Message(0, "Usage: #FindSpell [spellname]");
		else if (!spells_loaded)
			Message(0, "Spells not loaded");
		else if (Seperator::IsNumber(sep->argplus[1])) {
			int spellid = atoi(sep->argplus[1]);
			if (spellid <= 0 || spellid >= SPDAT_RECORDS) {
				Message(0, "Error: Number out of range");
			}
			else {
				Message(0, "  %i: %s", spellid, spells[spellid].name);
			}
		}
		else {
			int count=0;
			//int iSearchLen = strlen(sep->argplus[1])+1;
			char sName[64];
			char sCriteria[65];
			strncpy(sCriteria, sep->argplus[1], 64);
			strupr(sCriteria);
			for (int i = 0; i < SPDAT_RECORDS; i++)
			{
				if (spells[i].name[0] != 0) {
					strcpy(sName, spells[i].name);
					
					strupr(sName);
					char* pdest = strstr(sName, sCriteria);
					if ((pdest != NULL) && (count <=20)) {
						Message(0, "  %i: %s", i, spells[i].name);
						count++;
					}
					else if (count > 20)
						break;
				}
			}
			if (count > 20)
				Message(0, "20 spells found... max reached.");
			else
				Message(0, "%i spells found.", count);
		}
		found=true;
	}
	else if ((strcasecmp(sep->arg[0], "#CastSpell") == 0 || strcasecmp(sep->arg[0], "#Cast") == 0)) {
		if (!sep->IsNumber(1))
			Message(0, "Usage: #CastSpell spellid");
		else { 
			int16 spellid = atoi(sep->arg[1]);
			if ((spellid == 982 || spellid == 905 || spellid == 2079 || spellid == 1218 || spellid == 819 || spellid >= 780 && spellid <= 785 || spellid >= 1200 && spellid <= 1205 || spellid == 1923 || spellid ==1924) && admin < 100) { 
				Message(13, "Unable to cast spell."); 
			}
			else if (spellid >= SPDAT_RECORDS)
				Message(0, "Error: #CastSpell: Arguement out of range");
			else {
				if (target == 0) {
					if(admin>=100)
						SpellFinished(spellid, 0, 10, 0);
					else
						CastSpell(spellid, 0, 10, 0);
                }
				else {
                    if(admin>=100)
						SpellFinished(spellid, target->GetID(), 10, 0);
					else
						CastSpell(spellid, target->GetID(), 10, 0);
                }
			}
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#setskill") == 0) {
		if (target == 0) {
			Message(0, "Error: #setskill: No target.");
		}
		else if (!sep->IsNumber(1) || atoi(sep->arg[1]) < 0 || atoi(sep->arg[1]) > 73) {
			Message(0, "Usage: #setskill skill x ");
			Message(0, "       skill = 0 to 73");
			Message(0, "       x = 0 to 255");
			Message(0, "NOTE: skill values greater than 250 may cause the skill to become unusable on the client.");
		}
		else if (!sep->IsNumber(2) || atoi(sep->arg[2]) < 0 || atoi(sep->arg[2]) > 255) {
			Message(0, "Usage: #setskill skill x ");
			Message(0, "       skill = 0 to 73");
			Message(0, "       x = 0 to 255");
		}
		else {
			//pp.skills[atoi(sep->arg[1])] = (int8)atoi(sep->arg[2]);
			LogFile->write(EQEMuLog::Normal,"Set skill request from %s, target:%s skill_id:%i value:%i", GetName(), target->GetName(), atoi(sep->arg[1]), atoi(sep->arg[2]) );
			int skill_num = atoi(sep->arg[1]);
			int8 skill_id = (int8)atoi(sep->arg[2]);
			target->SetSkill(skill_num, skill_id);
		}
		found=true;
	}
	else if ((strcasecmp(sep->arg[0], "#setallskill") == 0 || strcasecmp(sep->arg[0], "#setskillall") == 0)) {
		if (target == 0) {
			Message(0, "Error: #setallskill: No target.");
		}
		else if (!sep->IsNumber(1) || atoi(sep->arg[1]) < 0 || atoi(sep->arg[1]) > 252) {
			Message(0, "Usage: #setskill value ");
			Message(0, "       value = 0 to 252");
		}
		else {
			//pp.skills[atoi(sep->arg[1])] = (int8)atoi(sep->arg[2]);
			LogFile->write(EQEMuLog::Normal,"Set ALL skill request from %s, target:%s", GetName(), target->GetName());
			int8 skill_id = atoi(sep->arg[1]);
			for(int skill_num=0;skill_num<74;skill_num++){
				target->SetSkill(skill_num, skill_id);
			}
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#race") == 0) {
		if (sep->IsNumber(1) && atoi(sep->arg[1]) >= 0 && atoi(sep->arg[1]) <= 329) {
			if ((target) && admin >= 100)
				target->SendIllusionPacket(atoi(sep->arg[1]));
			else
				this->SendIllusionPacket(atoi(sep->arg[1]));

		}
		else
			Message(0, "Usage: #race [0-329]  (0 for back to normal)");
		found=true;
	}
	else if (strcasecmp(sep->arg[0],"#makepet") == 0) {
		if (!(sep->IsNumber(1) && sep->IsNumber(2) && sep->IsNumber(3) && sep->IsNumber(4))) {
			Message(0, "Usage: #makepet level class race texture");
		}
		else {
			this->MakePet(atoi(sep->arg[1]), atoi(sep->arg[2]), atoi(sep->arg[3]), atoi(sep->arg[4]));
		}
		found=true;
	}
	return found;
}
bool Client::PrivUser(const Seperator* sep){

	bool found = false;
	if ((zone->loglevelvar<8) && (zone->loglevelvar>5) && (commandlogged==false) && (admin >= 10))
		Commandlog(sep);

	if ((strcasecmp(sep->arg[0], "#level") == 0)) { 
   int16 level = atoi(sep->arg[1]); 
   if ((level <= 0) || ((level > 65) && (this->admin < 100)) ) { 
      Message(0, "Error: #Level: Invalid Level"); 
   } 
   else if (admin < 100) { 
      this->SetLevel(level, true); 
   } 
   else if (!target) { 
      Message(0, "Error: #Level: No target"); 
   } 
   else { 
      if (!target->IsNPC() && ((this->admin < 200) && (level > 65))) { 
         Message(0, "Error: #Level: Invalid Level"); 
      } 
      else 
      { 
         target->SetLevel(level, true); 
      } 
   } 

		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#spawn") == 0) { // Image's Spawn Code -- Rewrite by Scruffy
#if DEBUG >= 11
		LogFile->write(EQEMuLog::Debug,"#spawn Spawning:");
#endif
		//Well it needs a name!!! 
		NPC* npc = NPC::SpawnNPC(sep->argplus[1], GetX(), GetY(), GetZ(), GetHeading(), this);
		if (npc) { 
			// Disgrace: add some loot to it!
			npc->AddCash();
			int itemcount = (rand()%5) + 1;
			for (int counter=0; counter<itemcount; counter++)
			{
			/*	const Item_Struct* item = 0;
			while (item == 0)
			item = database.GetItem(rand() % 33000);

			
				npc->AddItem(item, 0, 0);*/
			}
		}
		else { 
			Message(0, "Format: #spawn name race level material hp gender class priweapon secweapon merchantid - spawns a npc those parameters.");
			Message(0, "Name Format: NPCFirstname_NPCLastname - All numbers in a name are stripped and \"_\" characters become a space.");
			Message(0, "Note: Using \"-\" for gender will autoselect the gender for the race. Using \"-\" for HP will use the calculated maximum HP.");
		} 
        found = true;
	}
	//Disgrace: a make due function to give you almost full mana (almost accurately..)
	else if (strcasecmp(sep->arg[0], "#mana") == 0)
	{
		if (target == 0)
			this->SetMana(this->CalcMaxMana());
		else
			target->SetMana(target->GetMaxMana());
        found = true;
	}
	else if (strcasecmp(sep->arg[0], "#texture") == 0) {
		if (sep->IsNumber(1) && atoi(sep->arg[1]) >= 0 && atoi(sep->arg[1]) <= 255) {
			int tmp;
			if (sep->IsNumber(2) && atoi(sep->arg[2]) >= 0 && atoi(sep->arg[2]) <= 255) {
				tmp = atoi(sep->arg[2]);
			}
			else if (atoi(sep->arg[1]) == 255) {
				tmp = atoi(sep->arg[1]);
			}
			else if ((GetRace() > 0 && GetRace() <= 12) || GetRace() == 128 || GetRace() == 130)
				tmp = 0;
			else
				tmp = atoi(sep->arg[1]);
			
			if ((target) && admin >= 100)
				target->SendIllusionPacket(target->GetRace(), 0xFF, atoi(sep->arg[1]), tmp);
			else
				this->SendIllusionPacket(this->GetRace(), 0xFF, atoi(sep->arg[1]), tmp);
		}
		else
			Message(0, "Usage: #texture [texture] [helmtexture]  (0-255, 255 for show equipment)");
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#gender") == 0) {
		if (sep->IsNumber(1) && atoi(sep->arg[1]) >= 0 && atoi(sep->arg[1]) <= 2) {
			gender = atoi(sep->arg[1]);
			if ((target) && admin >= 100)
				target->SendIllusionPacket(target->GetRace(), gender);

			else
				this->SendIllusionPacket(this->GetRace(), gender);
		}
		else
			Message(0, "Usage: #gender [0-2]  (0=male, 1=female, 2=neuter)");
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#npctypespawn") == 0)
	{
		if (sep->IsNumber(1)) {
			const NPCType* tmp = 0;
			if ((tmp = database.GetNPCType(atoi(sep->arg[1]))))
			{
				//					tmp->fixedZ = 1;
				NPC* npc = new NPC(tmp, 0, x_pos, y_pos, z_pos, heading);
				if (npc && sep->IsNumber(2))
					npc->SetNPCFactionID(atoi(sep->arg[2]));
				entity_list.AddNPC(npc); 
			}
			else
				Message(0, "NPC Type %i not found", atoi(sep->arg[1])); 
		}
		else {
			Message(0, "Usage: #dbspawn npctypeid");
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#heal") == 0) {
		if (target==0) {
			Message(0, "Error: #Heal: No Target.");
		} else {
			target->Heal();
		}
		found=true;
	}
	return found;
}
bool Client::LeadGM(const Seperator* sep){
	if ((((zone->loglevelvar<8) && (zone->loglevelvar>1)) || (zone->loglevelvar==9)) && (commandlogged==false) && (admin >= 150))
		Commandlog(sep);
	if (strcasecmp(sep->arg[0], "#appearance") == 0) {
		// sends any appearance packet
		// Dev debug command, for appearance types
		if (sep->arg[2][0] == 0)
			Message(0, "Usage: #appearance type value");
		else {
			if (target != 0) {
				target->SendAppearancePacket(atoi(sep->arg[1]), atoi(sep->arg[2]));
				Message(0, "Sending appearance packet: target=%s, type=%s, value=%s", target->GetName(), sep->arg[1], sep->arg[2]);
			}
			else {
				this->SendAppearancePacket(atoi(sep->arg[1]), atoi(sep->arg[2]));
				Message(0, "Sending appearance packet: target=self, type=%s, value=%s", sep->arg[1], sep->arg[2]);
			}
		}
	}
	else if (strcasecmp(sep->arg[0], "#charbackup") == 0) {
		char errbuf[MYSQL_ERRMSG_SIZE];
		char *query = 0;
		MYSQL_RES* result;
		MYSQL_ROW row;
		if (strcasecmp(sep->arg[1], "list") == 0) {
			int32 charid = 0;
			if (sep->IsNumber(2))
				charid = atoi(sep->arg[2]);
			else
				database.GetAccountIDByChar(sep->arg[2], &charid);

			if (charid) {
				if (database.RunQuery(query, MakeAnyLenString(&query, "Select id, backupreason, charid, account_id, zoneid, DATE_FORMAT(ts, '%%m/%%d/%%Y %%H:%%i:%%s') from character_backup where charid=%u", charid), errbuf, &result)) {
					safe_delete(query);
					int32 x = 0;
					while ((row = mysql_fetch_row(result))) {
						Message(0, " %u: %s, %s (%u), reason=%u", atoi(row[0]), row[5], database.GetZoneName(atoi(row[4])), atoi(row[4]), atoi(row[1]));
						x++;
					}
					Message(0, " %u backups found.", x);
				}
				else {
					Message(13, "Query error: '%s' %s", query, errbuf);
					safe_delete(query);
				}
			}
			else
				Message(0, "Usage: #charbackup list [char name/id]");
		}
		else if (strcasecmp(sep->arg[1], "restore") == 0) {
			int32 charid = 0;
			if (sep->IsNumber(2))
				charid = atoi(sep->arg[2]);
			else
				database.GetAccountIDByChar(sep->arg[2], &charid);

			if (charid && sep->IsNumber(3)) {
				int32 cbid = atoi(sep->arg[3]);
				if (database.RunQuery(query, MakeAnyLenString(&query, "Insert into character_backup (backupreason, charid, account_id, name, profile, guild, guildrank, x, y, z, zoneid, alt_adv) select 1, id, account_id, name, profile, guild, guildrank, x, y, z, zoneid, alt_adv from character_ where id=%u", charid), errbuf)) {
					if (database.RunQuery(query, MakeAnyLenString(&query, "update character_ inner join character_backup on character_.id = character_backup.charid set character_.name = character_backup.name, character_.profile = character_backup.profile, character_.guild = character_backup.guild, character_.guildrank = character_backup.guildrank, character_.x = character_backup.x, character_.y = character_backup.y, character_.z = character_backup.z, character_.zoneid = character_backup.zoneid, character_.alt_adv = character_backup.alt_adv where character_backup.charid=%u and character_backup.id=%u", charid, cbid), errbuf)) {
						safe_delete(query);
						Message(0, "Character restored.");
					}
					else {
						Message(13, "Query error: '%s' %s", query, errbuf);
						safe_delete(query);
					}
				}
				else {
					Message(13, "Query error: '%s' %s", query, errbuf);
					safe_delete(query);
				}
			}
			else
				Message(0, "Usage: #charbackup list [char name/id]");
		}
		else {
			Message(0, "#charbackup sub-commands:");
			Message(0, "  list [char name/id]");
			Message(0, "  restore [char name/id] [backup#]");
		}
	}
	else if (strcasecmp(sep->arg[0], "#nukeitem") == 0) {
		if (target && target->IsClient() && (sep->IsNumber(1) || sep->IsHexNumber(1))) {
			int32 x = 0;
			if (sep->IsNumber(1))
				x = target->CastToClient()->NukeItem(atoi(sep->arg[1]));
			else
				x = target->CastToClient()->NukeItem(hextoi(sep->arg[1]));
			Message(0, " %u items deleted", x);
		}
		else
			Message(0, "Usage: (targted) #nukeitem itemnum - removes the item from the player's inventory");
	}
	else if (strcasecmp(sep->arg[0], "#peekinv") == 0) {
		if (target && target->IsClient()) {
			Client* cl = target->CastToClient();
			int32 tmp = 0;

			const Item_Struct* item = 0;
			if (sep->IsNumber(1)){
			if (atoi(sep->arg[1])==1)
				tmp = cl->GetItemAt(22);
			else if (atoi(sep->arg[1])==2)
				tmp = cl->GetItemAt(23);
			else if (atoi(sep->arg[1])==3)
				tmp = cl->GetItemAt(24);
			else if (atoi(sep->arg[1])==4)
				tmp = cl->GetItemAt(25);
			else if (atoi(sep->arg[1])==5)
				tmp = cl->GetItemAt(26);
			else if (atoi(sep->arg[1])==6)
				tmp = cl->GetItemAt(27);
			else if (atoi(sep->arg[1])==7)
				tmp = cl->GetItemAt(28);
			else if (atoi(sep->arg[1])==8)
				tmp = cl->GetItemAt(29);
			else if (atoi(sep->arg[1])==9)
				tmp = cl->GetItemAt(2000);
			else if (atoi(sep->arg[1])==10)
				tmp = cl->GetItemAt(2001);
			else if (atoi(sep->arg[1])==11)
				tmp = cl->GetItemAt(2002);
			else if (atoi(sep->arg[1])==12)
				tmp = cl->GetItemAt(2003);
			else if (atoi(sep->arg[1])==13)
				tmp = cl->GetItemAt(2004);
			else if (atoi(sep->arg[1])==14)
				tmp = cl->GetItemAt(2005);
			else if (atoi(sep->arg[1])==15)
				tmp = cl->GetItemAt(2006);
			else if (atoi(sep->arg[1])==16)
				tmp = cl->GetItemAt(2007);
			if (tmp > 0 && tmp < 0xFFFF) {
				item = database.GetItem(tmp);
				Message(0, "Looking into %i: %5i: %s", atoi(sep->arg[1])+21, tmp, Database::GetItemLink(item));
			}
			if (atoi(sep->arg[1])<9){
			for (int i=((atoi(sep->arg[1])+24)*10); i<(((atoi(sep->arg[1])+24)*10)+10); i++) {
				tmp = cl->GetItemAt(i);
				if (tmp > 0 && tmp < 0xFFFF) {
					item = database.GetItem(tmp);
					Message(0, " %u: %5i:%i %s", i, tmp, cl->GetItemPropAt(i).charges, Database::GetItemLink(item));

				}

			}

			}
			else{
				for (int i=((atoi(sep->arg[1])+194)*10); i<(((atoi(sep->arg[1])+194)*10)+10); i++) {
				tmp = cl->GetItemAt(i);
				if (tmp > 0 && tmp < 0xFFFF) {
					item = database.GetItem(tmp);
					Message(0, " %u: %5i:%i %s", i, tmp, cl->GetItemPropAt(i).charges, Database::GetItemLink(item));
				}
			}
			}
			}
			else{
			for (int i=0; i<30; i++) {
				tmp = cl->GetItemAt(i);
				if (tmp > 0 && tmp < 0xFFFF) {
					item = database.GetItem(tmp);
					Message(0, " %u: %5i:%i %s", i, tmp, cl->GetItemPropAt(i).charges, Database::GetItemLink(item));
				}
			}
			Message(0, "Bank Items:");
			for (int j=2000; j<2008; j++) {
				tmp = cl->GetItemAt(j);
				if (tmp > 0 && tmp < 0xFFFF) {
					item = database.GetItem(tmp);
					Message(0, " %u: %5i:%i %s", j, tmp, cl->GetItemPropAt(j).charges, Database::GetItemLink(item));
				}
			}
		}
		}
		else
			Message(0, "Target a client. Optional to add a back number 1-16");
	}
	else if (strcasecmp(sep->arg[0], "#findnpctype") == 0) {
		FindNPCType(sep->argplus[1]);
	}
	else if (strcasecmp(sep->arg[0], "#viewnpctype") == 0) {
		if (!sep->IsNumber(1)) {
			Message(0, "Usage: #viewnpctype [npctype id]");
		}
		else {
			ViewNPCType(atoi(sep->arg[1]));
		}
	}
	else if (strcasecmp(sep->arg[0], "#reloadqst") == 0 && admin >= 151) {
		parse->ClearCache();
		Message(0, "Clearing *.qst memory cache.");
	}
	else if (strcasecmp(sep->arg[0], "#reloadzps") == 0 && admin >= 151) {
		database.LoadStaticZonePoints(&zone->zone_point_list,zone->GetShortName());
		Message(0, "Reloading server zone_points.");
	}
	else if (strcasecmp(sep->arg[0], "#zoneshutdown") == 0) {
		if (!worldserver.Connected())
			Message(0, "Error: World server disconnected");
		else if (sep->arg[1][0] == 0) {
			Message(0, "Usage: #zoneshutdown zoneshortname");
		} else {
			ServerPacket* pack = new ServerPacket(ServerOP_ZoneShutdown, sizeof(ServerZoneStateChange_struct));
			ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
			strcpy(s->adminname, this->GetName());
			if (sep->arg[1][0] >= '0' && sep->arg[1][0] <= '9')
				s->ZoneServerID = atoi(sep->arg[1]);

			else
				s->zoneid = database.GetZoneID(sep->arg[1]);
			worldserver.SendPacket(pack);
			delete pack;
		}
	}
	else if (strcasecmp(sep->arg[0], "#zonebootup") == 0) {
		if (!worldserver.Connected())
			Message(0, "Error: World server disconnected");
		else if (sep->arg[2][0] == 0) {
			Message(0, "Usage: #zonebootup ZoneServerID# zoneshortname");
		} else {
			ServerPacket* pack = new ServerPacket(ServerOP_ZoneBootup, sizeof(ServerZoneStateChange_struct));
			ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
			s->ZoneServerID = atoi(sep->arg[1]);
			strcpy(s->adminname, this->GetName());
			s->zoneid = database.GetZoneID(sep->arg[2]);
			s->makestatic = (bool) (strcasecmp(sep->arg[3], "static") == 0);
			worldserver.SendPacket(pack);
			delete pack;
		}
	}
	else if (strcasecmp(sep->arg[0], "#kick") == 0) {
		if (sep->arg[1][0] == 0)
			Message(0, "Usage: #kick [charname]");
		else {
			Client* client = entity_list.GetClientByName(sep->arg[1]);
			if (client != 0) {
				if (client->Admin() <= this->Admin()) {
					client->Message(0, "You have been kicked by %s",this->GetName());
					client->Kick();
					this->Message(0, "Kick: local: kicking %s", sep->arg[1]);
				}
			}
			else if (!worldserver.Connected())
				Message(0, "Error: World server disconnected");
			else {
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_KickPlayer;
				pack->size = sizeof(ServerKickPlayer_Struct);
				pack->pBuffer = new uchar[pack->size];
				ServerKickPlayer_Struct* skp = (ServerKickPlayer_Struct*) pack->pBuffer;
				strcpy(skp->adminname, this->GetName());
				strcpy(skp->name, sep->arg[1]);
				skp->adminrank = this->Admin();
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
	}
	else if (strcasecmp(sep->arg[0], "#attack") == 0) {
		if (target && target->IsNPC() && sep->arg[1] != 0) {
			Mob* sictar = entity_list.GetMob(sep->argplus[1]);
			if (sictar) {
				target->CastToNPC()->AddToHateList(sictar, 1, 0);

			}
			else {

				Message(0, "Error: %s not found", sep->arg[1]);
			}
		}
		else {
			Message(0, "Usage: (needs NPC targeted) #attack targetname");
		}
	}
	else if (strcasecmp(sep->arg[0], "#lock") == 0) {

		ServerPacket* outpack = new ServerPacket(ServerOP_Lock, sizeof(ServerLock_Struct));
		ServerLock_Struct* lss = (ServerLock_Struct*) outpack->pBuffer;
		strcpy(lss->myname, this->GetName());
		lss->mode = 1;
		worldserver.SendPacket(outpack);
		delete outpack;
	}
	else if (strcasecmp(sep->arg[0], "#unlock") == 0) {
		ServerPacket* outpack = new ServerPacket(ServerOP_Lock, sizeof(ServerLock_Struct));
		ServerLock_Struct* lss = (ServerLock_Struct*) outpack->pBuffer;
		strcpy(lss->myname, this->GetName());
		lss->mode = 0;
		worldserver.SendPacket(outpack);
		delete outpack;
	}
	else if (strcasecmp(sep->arg[0], "#motd") == 0) {
		ServerPacket* outpack = new ServerPacket(ServerOP_Motd, sizeof(ServerMotd_Struct));
		ServerMotd_Struct* mss = (ServerMotd_Struct*) outpack->pBuffer;
		strncpy(mss->myname, this->GetName(),64);
		strncpy(mss->motd, sep->argplus[1],512);
		worldserver.SendPacket(outpack);
		delete outpack;
	}
	else
		return false;

	return true;
}
bool Client::NormalGM(const Seperator* sep){
	if ((((zone->loglevelvar<8) && (zone->loglevelvar>2)) || (zone->loglevelvar==8))  && (commandlogged==false) && (admin >= 100))
		Commandlog(sep);
	if ((strcasecmp(sep->arg[0], "#listpetition") == 0)) {
		char errbuf[MYSQL_ERRMSG_SIZE];
		char *query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;
		int blahloopcount=0;
		if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT petid, charname, accountname from petitions order by petid"), errbuf, &result))
		{
			delete[] query;
			LogFile->write(EQEMuLog::Normal,"Petition list requested by %s", this->GetName());
			while ((row = mysql_fetch_row(result))) {
			if (blahloopcount==0) {
				blahloopcount=1;
				Message(13,"	ID : Character Name , Account Name");
			}
			else
				Message(15, " %s:	%s , %s ",row[0],row[1],row[2]);
		}
		mysql_free_result(result);
		}
	}
	else if (strcasecmp(sep->arg[0], "#equipitem") == 0) {
		if (sep->IsNumber(1) && atoi(sep->arg[1]) >= 1 && atoi(sep->arg[1]) <= 21) {
			const Item_Struct* item = database.GetItem(GetItemAt(0));
			int8 charges = pp.invitemproperties[0].charges;
			if (item) {
				APPLAYER* outapp = new APPLAYER(OP_MoveItem, sizeof(MoveItem_Struct));
				MoveItem_Struct* mi = (MoveItem_Struct*) outapp->pBuffer;
				mi->from_slot = 0;
				mi->to_slot = atoi(sep->arg[1]);
				mi->number_in_stack = pp.invitemproperties[0].charges;
				pp.inventory[atoi(sep->arg[1])] = pp.inventory[0];
				pp.invitemproperties[atoi(sep->arg[1])].charges = pp.invitemproperties[0].charges;
				pp.inventory[0] = 0xFFFF;
				pp.invitemproperties[0].charges = 0;
				FastQueuePacket(&outapp);
			}
			else
				Message(0, "Error: item == 0");
		}
		else
			Message(0, "Usage: #equipitem slotid[1-21] - equips the item on your cursor to the position");
	}
	else if (strcasecmp(sep->arg[0], "#zonelock") == 0) {
		ServerPacket* pack = new ServerPacket(ServerOP_LockZone, sizeof(ServerLockZone_Struct));
		ServerLockZone_Struct* s = (ServerLockZone_Struct*) pack->pBuffer;
		strncpy(s->adminname, GetName(), sizeof(s->adminname));
		if (strcasecmp(sep->arg[1], "list") == 0) {
			s->op = 0;
			worldserver.SendPacket(pack);
		}
		else if (strcasecmp(sep->arg[1], "lock") == 0 && admin >= 101) {
			int16 tmp = database.GetZoneID(sep->arg[2]);
			if (tmp) {
				s->op = 1;
				s->zoneID = tmp;
				worldserver.SendPacket(pack);
			}
			else
				Message(0, "Usage: #zonelock lock [zonename]");
		}
		else if (strcasecmp(sep->arg[1], "unlock") == 0 && admin >= 101) {
			int16 tmp = database.GetZoneID(sep->arg[2]);
			if (tmp) {
				s->op = 2;
				s->zoneID = tmp;
				worldserver.SendPacket(pack);
			}
			else
				Message(0, "Usage: #zonelock unlock [zonename]");
		}
		else {
			Message(0, "#zonelock sub-commands");
			Message(0, "  list");
			if (admin >= 101) {
				Message(0, "  lock [zonename]");
				Message(0, "  unlock [zonename]");
			}
		}
		delete pack;
	}
	else if (strcasecmp(sep->arg[0], "#corpse") == 0) {
		CorpseCommand(sep);
	}
   // WORKPOINT 1 
   else if (strcasecmp(sep->arg[0], "#fixmob") == 0) { 
      if (!sep->arg[1]) 
         Message(0,"Usage: #fixmob [nextrace|prevrace|gender|nexttexture|prevtexture|nexthelm|prevhelm]"); 
   // Series of functions to manipulate spawns. 
      else if (strcasecmp(sep->arg[1], "nextrace") == 0) { 
      // Set to next race 
         if ((target) && admin >= 100) { 
            if (target->GetRace() == 329) { 
               target->SendIllusionPacket(1); 
               Message(0, "Race=1"); 
            } 
            else { 
               target->SendIllusionPacket(target->GetRace()+1); 
               Message(0, "Race=%i",target->GetRace()); 
            } 
         } 
      } 
      else if (strcasecmp(sep->arg[1], "prevrace") == 0) { 
      // Set to previous race 
         if ((target) && admin >= 100) { 
            if (target->GetRace() == 1) { 
               target->SendIllusionPacket(329); 
               Message(0, "Race=%i",329); 
            } 
            else { 
               target->SendIllusionPacket(target->GetRace()-1); 
               Message(0, "Race=%i",target->GetRace()); 
            } 
         } 
      } 
      else if (strcasecmp(sep->arg[1], "gender") == 0) { 
      // Cycle through the 3 gender modes 
         if ((target) && admin >= 100) { 
            if (target->GetGender() == 0) { 
               target->SendIllusionPacket(target->GetRace(), 3); 
               Message(0, "Gender=%i",3); 
            } 
            else { 
               target->SendIllusionPacket(target->GetRace(), target->GetGender()-1); 
               Message(0, "Gender=%i",target->GetGender()); 
            } 
         } 
      } 
      else if (strcasecmp(sep->arg[1], "nexttexture") == 0) { 
      // Set to next texture 
         if ((target) && admin >= 100) { 
            if (target->GetTexture() == 25) { 
               target->SendIllusionPacket(target->GetRace(), target->GetGender(), 1); 
               Message(0, "Texture=1"); 
            } 
            else { 
               target->SendIllusionPacket(target->GetRace(), target->GetGender(), target->GetTexture()+1); 
               Message(0, "Texture=%i",target->GetTexture()); 
            } 
         } 
      } 
      else if (strcasecmp(sep->arg[1], "prevtexture") == 0) { 
      // Set to previous texture 
         if ((target) && admin >= 100) { 
            if (target->GetTexture() == 1) { 
               target->SendIllusionPacket(target->GetRace(), target->GetGender(), 25); 
               Message(0, "Texture=%i",25); 
            } 
            else { 
               target->SendIllusionPacket(target->GetRace(), target->GetGender(), target->GetTexture()-1); 
               Message(0, "Texture=%i",target->GetTexture()); 
            } 
         } 
      } 
      else if (strcasecmp(sep->arg[1], "nexthelm") == 0) { 
      // Set to next helm.  Only noticed a difference on giants. 
         if ((target) && admin >= 100) { 
            if (target->GetHelmTexture() == 25) { 
               target->SendIllusionPacket(target->GetRace(), target->GetGender(), target->GetTexture(), 1); 
               Message(0, "HelmTexture=1"); 
            } 
            else { 
               target->SendIllusionPacket(target->GetRace(), target->GetGender(), target->GetTexture(), target->GetHelmTexture()+1); 
               Message(0, "HelmTexture=%i",target->GetHelmTexture()); 
            } 
         } 
      } 
      else if (strcasecmp(sep->arg[1], "prevhelm") == 0) { 
      // Set to previous helm.  Only noticed a difference on giants. 
         if ((target) && admin >= 100) { 
            if (target->GetHelmTexture() == 1) { 
               target->SendIllusionPacket(target->GetRace(), target->GetGender(), target->GetTexture(), 25); 
               Message(0, "HelmTexture=%i",25); 
            } 
            else { 
               target->SendIllusionPacket(target->GetRace(), target->GetGender(), target->GetTexture(), target->GetHelmTexture()-1); 
               Message(0, "HelmTexture=%i",target->GetHelmTexture()); 
            } 
         } 
      } 
   } 
	else if (strcasecmp(sep->arg[0], "#gmspeed") == 0) {
		if (strcasecmp(sep->arg[1], "on") == 0) {
			database.SetGMSpeed(AccountID(), 1);
			Message(0, "GMSpeed On");
		}
		else if (strcasecmp(sep->arg[1], "off") == 0) {
			database.SetGMSpeed(AccountID(), 0);
			Message(0, "GMSpeed Off");
		}
		else {
			Message(0, "Usage: #gmspeed [on|off]");
		}
	}
	else if (strcasecmp(sep->arg[0], "#title") == 0) {
		if (sep->arg[1][0]==0)
			Message(0, "Usage: #title  [0-3]");
		else {
			if (GetTarget() == 0 || GetTarget() == this) {
				if (atoi(sep->arg[1]) >= 0 || atoi(sep->arg[1]) <= 3) {
					this->pp.title = atoi(sep->arg[1]);
					this->Save();
					Message(0,"Updated your Title.");
				}
				else 
					Message(0, "You need to use a number between 0 and 3!");
			}
			else {
				if (GetTarget()->IsClient()) {

					GetTarget()->CastToClient()->ChangeTitle((int8) atoi(sep->arg[1]));
					GetTarget()->CastToClient()->Save();
					Message(0, "Updated %s's Title", GetTarget()->GetName());
				}
				else 
					Message(0, "You must target a valid Player Character For this action.");
			}
		}
	}
	else if (strcasecmp(sep->arg[0], "#spellinfo") == 0)
	{
		if(sep->arg[1][0]==0)
			Message(0, "Usage: #spellinfo [spell_id]");
		else
		{
			short int spell_id=atoi(sep->arg[1]);
			const struct SPDat_Spell_Struct *s=&spells[spell_id];
			Message(0, "Spell info for spell #%d:", spell_id);
			Message(0, "  name: %s", s->name);
			Message(0, "  player_1: %s", s->player_1);
			Message(0, "  teleport_zone: %s", s->teleport_zone);
			Message(0, "  you_cast: %s", s->you_cast);
			Message(0, "  other_casts: %s", s->other_casts);
			Message(0, "  cast_on_you: %s", s->cast_on_you);
			Message(0, "  spell_fades: %s", s->spell_fades);
			Message(0, "  range: %f", s->range);
			Message(0, "  aoerange: %f", s->aoerange);
			Message(0, "  pushback: %f", s->pushback);
			Message(0, "  pushup: %f", s->pushup);
			Message(0, "  cast_time: %d", s->cast_time);
			Message(0, "  recovery_time: %d", s->recovery_time);
			Message(0, "  recast_time: %d", s->recast_time);
			Message(0, "  buffdurationformula: %d", s->buffdurationformula);
			Message(0, "  buffduration: %d", s->buffduration);
			Message(0, "  ImpactDuration: %d", s->ImpactDuration);
			Message(0, "  mana: %d", s->mana);
			Message(0, "  base[12]: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", s->base[0], s->base[1], s->base[2], s->base[3], s->base[4], s->base[5], s->base[6], s->base[7], s->base[8], s->base[9], s->base[10], s->base[11]);
			Message(0, "  max[12]: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", s->max[0], s->max[1], s->max[2], s->max[3], s->max[4], s->max[5], s->max[6], s->max[7], s->max[8], s->max[9], s->max[10], s->max[11]);
			Message(0, "  icon: %d", s->icon);
			Message(0, "  memicon: %d", s->memicon);
			Message(0, "  components[4]: %d, %d, %d, %d", s->components[0], s->components[1], s->components[2], s->components[3]);
			Message(0, "  component_counts[4]: %d, %d, %d, %d", s->component_counts[0], s->component_counts[1], s->component_counts[2], s->component_counts[3]);
			Message(0, "  NoexpendReagent[4]: %d, %d, %d, %d", s->NoexpendReagent[0], s->NoexpendReagent[1], s->NoexpendReagent[2], s->NoexpendReagent[3]);
			Message(0, "  formula[12]: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x", s->formula[0], s->formula[1], s->formula[2], s->formula[3], s->formula[4], s->formula[5], s->formula[6], s->formula[7], s->formula[8], s->formula[9], s->formula[10], s->formula[11]);
			Message(0, "  LightType: %d", s->LightType);
			Message(0, "  goodEffect: %d", s->goodEffect);
			Message(0, "  Activated: %d", s->Activated);
			Message(0, "  resisttype: %d", s->resisttype);
			Message(0, "  effectid[12]: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x", s->effectid[0], s->effectid[1], s->effectid[2], s->effectid[3], s->effectid[4], s->effectid[5], s->effectid[6], s->effectid[7], s->effectid[8], s->effectid[9], s->effectid[10], s->effectid[11]);
			Message(0, "  targettype: %d", s->targettype);
			Message(0, "  basediff: %d", s->basediff);
			Message(0, "  skill: %d", s->skill);
			Message(0, "  zonetype: %d", s->zonetype);
			Message(0, "  EnvironmentType: %d", s->EnvironmentType);
			Message(0, "  TimeOfDay: %d", s->TimeOfDay);
			Message(0, "  classes[15]: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", 
				s->classes[0], s->classes[1], s->classes[2], s->classes[3], s->classes[4], 
				s->classes[5], s->classes[6], s->classes[7], s->classes[8], s->classes[9], 
				s->classes[10], s->classes[11], s->classes[12], s->classes[13], s->classes[14]);
			Message(0, "  CastingAnim: %d", s->CastingAnim);
			Message(0, "  TargetAnim: %d", s->TargetAnim);
			Message(0, "  SpellAffectIndex: %d", s->SpellAffectIndex);
			Message(0, " RecourseLink: %d", s->RecourseLink);
			Message(0, "  Spacing2[5]: %d, %d, %d, %d, %d", s->Spacing2[0], s->Spacing2[1], s->Spacing2[2], s->Spacing2[3], s->Spacing2[4]);
		}
	}
	else if (strcasecmp(sep->arg[0], "#lastname") == 0) {
		LogFile->write(EQEMuLog::Normal,"#lastname request from %s", GetName());
		if(strlen(sep->arg[1]) <= 70){
			Client::ChangeLastName(sep->arg[1]);
		}
		else {
			Message(0, "Usage: #lastname <lastname> where <lastname> is less than 70 long");
		}
	}
	else if (strcasecmp(sep->arg[0], "#memspell") == 0) {
		if (!(sep->IsNumber(1) && sep->IsNumber(2)))
			Message(0, "Usage: #MemSpell slotid spellid");
		else {
			int16 slotid = atoi(sep->arg[1]) - 1;
			int16 spellid = atoi(sep->arg[2]);
			if (slotid >=8) {
				Message(0, "Error: #MemSpell: Arguement out of range");
			}
			else if (spellid >= SPDAT_RECORDS)
				Message(0, "Error: #MemSpell: Arguement out of range");
			else {
				pp.spell_memory[slotid] = spellid; 
				Message(0, "Spell slot changed, it'll be there when you relog.");
			}
		}
	}
	else if (strcasecmp(sep->arg[0], "#save") == 0) {
		if (target == 0)
			Message(0, "Error: no target");
		else if (target->IsClient()) {
			if (target->CastToClient()->Save(2))
				Message(0, "%s successfully saved.", target->GetName());
			else
				Message(0, "Manual save for %s failed.", target->GetName());
		}
		else if (target->IsPlayerCorpse()) {
			if (target->CastToMob()->Save())
				Message(0, "%s successfully saved. (dbid=%u)", target->GetName(), target->CastToCorpse()->GetDBID());
			else
				Message(0, "Manual save for %s failed.", target->GetName());
		}
		else
			Message(0, "Error: target not a Client/PlayerCorpse");
	}
	else if (strcasecmp(sep->arg[0], "#showstats") == 0) {
		if (target != 0 )
			target->ShowStats(this);
		else
			this->ShowStats(this);
	}
	else if (strcasecmp(sep->arg[0], "#Depop") == 0) {
		if (target == 0 || !(target->IsNPC() || target->IsNPCCorpse()))
			Message(0, "You must have a NPC target for this command. (maybe you meant #depopzone?)");
		else {
			Message(0, "Depoping '%s'.", target->GetName());
			target->Depop();
		}
	}
	else if (strcasecmp(sep->arg[0], "#DepopZone") == 0) {
		zone->Depop();
		Message(0, "Zone depoped.");
	}
	else if (strcasecmp(sep->arg[0], "#Repop") == 0) {
		if (sep->IsNumber(1)) {
			zone->Repop(atoi(sep->arg[1])*1000);
			Message(0, "Zone depoped. Repop in %i seconds", atoi(sep->arg[1]));
		}
		else {
			zone->Repop();
			Message(0, "Zone depoped. Repoping now.");
		}
	}
	else if (strcasecmp(sep->arg[0], "#spawnstatus") == 0) {
		zone->SpawnStatus(this);
	}
	else if (strcasecmp(sep->arg[0], "#nukebuffs") == 0) {
		if (target == 0)
			this->BuffFade(0xFFFe);
		else
			target->BuffFade(0xFFFe);
	}
	else if(strcasecmp(sep->arg[0], "#zuwcoords") == 0) {
		// modifys and resends zhdr packet
		if(sep->arg[1][0]==0)
			Message(0, "Usage: #zuwcoords <under world coords>");
		else {
			zone->newzone_data.underworld = atof(sep->arg[1]);
			//				float newdata = atof(sep->arg[1]);
			//				memcpy(&zone->zone_header_data[130], &newdata, sizeof(float));
			
			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));

			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			delete outapp;
		}
	}
	else if(strcasecmp(sep->arg[0], "#zsafecoords") == 0) {
		// modifys and resends zhdr packet
		if(sep->arg[3][0]==0)
			Message(0, "Usage: #zsafecoords <safe x> <safe y> <safe z>");
		else {
			zone->newzone_data.safe_x = atof(sep->arg[1]);
			zone->newzone_data.safe_y = atof(sep->arg[2]);
			zone->newzone_data.safe_z = atof(sep->arg[3]);
			//				float newdatax = atof(sep->arg[1]);
			//				float newdatay = atof(sep->arg[2]);
			//				float newdataz = atof(sep->arg[3]);
			//				memcpy(&zone->zone_header_data[114], &newdatax, sizeof(float));
			//				memcpy(&zone->zone_header_data[118], &newdatay, sizeof(float));
			//				memcpy(&zone->zone_header_data[122], &newdataz, sizeof(float));
			//				zone->SetSafeCoords();
			
			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			delete outapp;
		}
	}
	else if (strcasecmp(sep->arg[0], "#freeze") == 0) {
		if (target != 0) {
			target->SendAppearancePacket(14, 102);
		}
		else {
			Message(0, "ERROR: Freeze requires a target.");
		}
	}
	else if (strcasecmp(sep->arg[0], "#unfreeze") == 0) {
		if (target != 0) {
			target->SendAppearancePacket(14, 100);
		}
		else {
			Message(0, "ERROR: Unfreeze requires a target.");


		}
	}
	else if (strcasecmp(sep->arg[0], "#pvp") == 0) {
		if (target != 0 && sep->arg[1] != NULL) {
			if (target->IsClient()) {	
				if (strcasecmp(sep->arg[1], "on") == 0)
					target->CastToClient()->SetPVP(true);
				else if (strcasecmp(sep->arg[1], "off") == 0)
					target->CastToClient()->SetPVP(false);
				else
					Message(0, "Usage: #pvp [on/off]");
			}
			else
				Message(0, "Error: #pvp: Invalid target type.");
		}
		else
			Message(0, "Error: #pvp: No target selected.");
	}
	else if (strcasecmp(sep->arg[0], "#setxp") == 0)
	{
		if (sep->IsNumber(1)) {
			if (atoi(sep->arg[1]) > 9999999)
				Message(0, "Error: SetXP: Value too high.");
			else
				AddEXP (atoi(sep->arg[1]));
		}
		else
			Message(0, "Usage: #setxp number");
	}
	else if (strcasecmp(sep->arg[0], "#name") == 0) {	
		if(strlen(sep->arg[1]) == 0 || strlen(sep->arg[2]) == 0)
			Message(13, "Enter a name.");
		else {
			Client* client = entity_list.GetClientByName(sep->arg[1]);
			if(client == 0)
				Message(13, "Invalid client.  Must be in the same zone.");
			else
				client->ChangeFirstName(sep->arg[1],sep->arg[2],GetName());
		}
	}
	else if (strcasecmp(sep->arg[0], "#npcspecialattk") == 0) {
			if (target==0 || target->IsClient() || strlen(sep->arg[1]) <= 0 || strlen(sep->arg[2]) <= 0) {
				Message(0, "Proper use: #npcspecialattk *flagchar* *permtag* (Flags are E(nrage) F(lurry) R(ampage) S(ummon), permtag is 1 = True, 0 = False).");
			} else {

				target->CastToNPC()->NPCSpecialAttacks(sep->arg[1],atoi(sep->arg[2]));
				Message(0, "NPC Special Attack set.");
			}
		}
	else if (strcasecmp(sep->arg[0], "#kill") == 0) {
		if (target==0) {
			Message(0, "Error: #Kill: No target.");
		} else {
			if ((!target->IsClient()) || target->CastToClient()->Admin() <= this->Admin())
				target->Kill();
		}
	}
	//Begin timezone updates	
	// Kaiyodo - #haste command to set client attack speed. Takes a percentage (100 = twice normal attack speed)
	else if (strcasecmp(sep->arg[0], "#haste")==0) {
		if(sep->arg[1][0] != 0)
		{
			int16 Haste = atoi(sep->arg[1]);
			if(Haste > 85)
				Haste = 85;
			
			SetHaste(Haste);
			// SetAttackTimer must be called to make this take effect, so player needs to change
			// the primary weapon.
			Message(0, "Haste set to %d%% - Need to re-equip primary weapon before it takes effect", Haste);
		}
		else
		{
			Message(0, "Usage: #haste x");

		}
	}
	else if (strcasecmp(sep->arg[0], "#damage") == 0) {
		long type=0xffff;
		if (target==0) {
			Message(0, "Error: #Damage: No Target.");
		}
		else if (!sep->IsNumber(1)) {
			Message(0, "Usage: #damage x.");		
			
		}
		else {
			sint32 nkdmg = atoi(sep->arg[1]);
			if (!sep->IsNumber(2))
				type=0xffff;
			else
				type=atol(sep->arg[2]);
			if (nkdmg > 2100000000)
				Message(0, "Enter a value less then 2,100,000,000.");
			else
				target->Damage(this, nkdmg, type, 0x04, false);
		}
	}
//	else if (strcasecmp(sep->arg[0], "#zonespawn") == 0)
//	{
//		if (target && target->IsNPC()) {
//			Message(0, "Inside main if.");
//			if (strcasecmp(sep->arg[1], "add")==0) {
//				Message(0, "Inside add if.");
//				database.DBSpawn(1, StaticGetZoneName(this->GetPP().current_zone), target->CastToNPC());
//			}
			/*else if (strcasecmp(sep->arg[1], "update")==0) {
				database.DBSpawn(2, StaticGetZoneName(this->GetPP().current_zone), target->CastToNPC());
			}
			else if (strcasecmp(sep->arg[1], "remove")==0) {
				if (strcasecmp(sep->arg[2], "all")==0) {
					database.DBSpawn(4, StaticGetZoneName(this->GetPP().current_zone));
				}
				else {
					if (database.DBSpawn(3, StaticGetZoneName(this->GetPP().current_zone), target->CastToNPC())) {
						Message(0, "#zonespawn: %s removed successfully!", target->GetName());
						target->CastToNPC()->Death(target, target->GetHP());
					}
				}
			}*/
//			else
//				Message(0, "Error: #dbspawn: Invalid command. (Note: EDIT and REMOVE are NOT in yet.)");
//			if (target->CastToNPC()->GetNPCTypeID() > 0) {
//				Message(0, "Spawn is type %i", target->CastToNPC()->GetNPCTypeID());
//			}
//		}
//		else if(!target || !target->IsNPC())
//			Message(0, "Error: #zonespawn: You must have a NPC targeted!");
//		else
//			Message(0, "Usage: #zonespawn [add|edit|remove|remove all]");
//		found = true;
//	}
	else if (strcasecmp(sep->arg[0], "#npcspawn") == 0) {
		NPCSpawn(sep);
	}
	else
		return false;
	return true;
}
bool Client::NormalUser(const Seperator* sep){
	bool found = false;
	if ((zone->loglevelvar==7) && (commandlogged==false) && (admin <10))
		Commandlog(sep);
	if (strcasecmp(sep->arg[0], "#version") == 0) {
		Message(0, "Current version information.");
		Message(0, "  %s", CURRENT_ZONE_VERSION);
		Message(0, "  Compiled on: %s at %s", COMPILE_DATE, COMPILE_TIME);
		Message(0, "  Last modified on: %s", LAST_MODIFIED);
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#save") == 0) {
		Save();
		Message(0, "%s successfully saved.", GetName());
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#loc") == 0) {
		if (target != 0 && CheckAccess(100, 100)) {
			Message(0, "%s's Location (XYZ): %1.1f, %1.1f, %1.1f; heading=%1.1f", target->GetName(), target->GetX(), target->GetY(), target->GetZ(), target->GetHeading());
		}
		else {
			Message(0, "Current Location (XYZ): %1.1f, %1.1f, %1.1f; heading=%1.1f", x_pos, y_pos, z_pos, GetHeading());
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#goto") == 0) { // Pyro's goto function
		if (sep->arg[1][0] == 0 && target != 0) {
			MovePC((char*) 0, target->GetX(), target->GetY(), target->GetZ());
		}
		else if (!(sep->IsNumber(1) && sep->IsNumber(2) && sep->IsNumber(3))) {
			Message(0, "Usage: #goto [x y z].");
		}
		else {
			MovePC((char*) 0, atof(sep->arg[1]), atof(sep->arg[2]), atof(sep->arg[3]));
		}
		found=true;
	}
#ifdef BUGTRACK
	else if (strcasecmp(sep->arg[0], "#bug") == 0) {
		
		if (sep->argplus[1][0] == 0) 
		{
			Message(0, "Usage: #bug details ");
		}
		else 
		{
			LogFile->write(EQEMuLog::Normal,"Sending bug report:", atoi(sep->argplus[1]) );
			BugDatabase bugdatabase;
			bugdatabase.UploadBug(sep->argplus[1], CURRENT_VERSION, this->AccountName());
#ifdef DEBUG
			LogFile->write(EQEMuLog::Debug,"Bug uploaded deleteing local bug object:");
#endif
			delete &bugdatabase;
		}
		found=true;
	}
#endif
	else if (strcasecmp(sep->arg[0], "#iteminfo") == 0) {
		const Item_Struct* item = database.GetItem(pp.inventory[0]);
		if (item == 0)
			Message(0, "Error: You need an item on your cursor for this command");
		else {
			Message(0, "ID: %i Name: %s", item->item_nr, item->name);
			Message(0, "  Lore: %s  ND: %i  NS: %i  Type: %i", item->lore, item->nodrop, item->nosave, item->type);
			Message(0, "  IDF: %s  Size: %i  Weight: %i  Flag: %04x  icon_nr: %i  Cost: %i", item->idfile, item->size, item->weight, (int16) item->flag, (int16) item->icon_nr, item->cost);

			if (admin >= 200)
				Message(0, "MinStatus: %i", database.GetItemStatus(item->item_nr));
			if (item->IsBook())
				Message(0, "  This item is a Book: %s", item->book.file);
			else if (item->IsBag())
				Message(0, "  This item is a container with %i slots", item->container.numSlots);			
			else {
				Message(0, "  equipableSlots: %u equipable Classes: %u", item->equipableSlots, item->common.classes);
				Message(0, "  Magic: %i  SpellID0: %i  Level0: %i  Stackable: %i  DBCharges: %i  CurCharges: %i", item->common.magic, item->common.spellId0, item->common.level0, item->common.normal.stackable, item->common.charges, pp.invitemproperties[0].charges);
				Message(0, "  SpellId: %i  Level: %i  EffectType: 0x%02x  CastTime: %.2f", item->common.spellId, item->common.level, (int8) item->common.effecttype, (double) item->common.casttime/1000);
				Message(0, "  Material: 0x02%x  Color: 0x%08x  Skill: %i", item->common.material, item->common.color, item->common.skill);
				Message(0, " Required level: %i Required skill: %i Recommended level:%i", item->common.ReqLevel, item->common.RecSkill, item->common.RecLevel);
				Message(0, " Skill mod: %i percent: %i", item->common.skillModId, item->common.skillModPercent);
				Message(0, " BaneRace: %i BaneBody: %i BaneDMG: %i", item->common.BaneDMGRace, item->common.BaneDMGBody, item->common.BaneDMG);
				//for (int cur = 0; cur < 16; cur++){
				//    Message(0, " cur:%i:%i", cur, (int8) item->common.unknown0296[cur]);
				//}
				
				/* TODO: FIX THE UNKNOWNS THAT ARE DISPLAYED!!!
				Message(0, "  U0187: 0x%02x%02x  U0192: 0x%02x  U0198: 0x%02x%02x  U0204: 0x%02x%02x  U0210: 0x%02x%02x  U0214: 0x%02x%02x%02x", item->common.unknown0187[0], item->common.unknown0187[1], item->common.unknown0192, item->common.unknown0198[0], item->common.unknown0198[1], item->common.unknown0204[0], item->common.unknown0204[1], item->common.unknown0210[0], item->common.unknown0210[1], (int8) item->common.normal.unknown0214[0], (int8) item->common.normal.unknown0214[1], (int8) item->common.normal.unknown0214[2]);
				Message(0, "  U0222[0-9]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0222[0], (int8) item->common.unknown0222[1], (int8) item->common.unknown0222[2], (int8) item->common.unknown0222[3], (int8) item->common.unknown0222[4], (int8) item->common.unknown0222[5], (int8) item->common.unknown0222[6], (int8) item->common.unknown0222[7], (int8) item->common.unknown0222[8], (int8) item->common.unknown0222[9]);
				Message(0, "  U0236[ 0-15]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0236[0], (int8) item->common.unknown0236[1], (int8) item->common.unknown0236[2], (int8) item->common.unknown0236[3], (int8) item->common.unknown0236[4], (int8) item->common.unknown0236[5], (int8) item->common.unknown0236[6], (int8) item->common.unknown0236[7], (int8) item->common.unknown0236[8], (int8) item->common.unknown0236[9], (int8) item->common.unknown0236[10], (int8) item->common.unknown0236[11], (int8) item->common.unknown0236[12], (int8) item->common.unknown0236[13], (int8) item->common.unknown0236[14], (int8) item->common.unknown0236[15]);
				Message(0, "  U0236[16-31]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0236[16], (int8) item->common.unknown0236[17], (int8) item->common.unknown0236[18], (int8) item->common.unknown0236[19], (int8) item->common.unknown0236[20], (int8) item->common.unknown0236[21], (int8) item->common.unknown0236[22], (int8) item->common.unknown0236[23], (int8) item->common.unknown0236[24], (int8) item->common.unknown0236[25], (int8) item->common.unknown0236[26], (int8) item->common.unknown0236[27], (int8) item->common.unknown0236[28], (int8) item->common.unknown0236[29], (int8) item->common.unknown0236[30], (int8) item->common.unknown0236[31]);
				Message(0, "  U0236[32-47]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0236[32], (int8) item->common.unknown0236[33], (int8) item->common.unknown0236[34], (int8) item->common.unknown0236[35], (int8) item->common.unknown0236[36], (int8) item->common.unknown0236[37], (int8) item->common.unknown0236[38], (int8) item->common.unknown0236[39], (int8) item->common.unknown0236[40], (int8) item->common.unknown0236[41], (int8) item->common.unknown0236[42], (int8) item->common.unknown0236[43], (int8) item->common.unknown0236[44], (int8) item->common.unknown0236[45], (int8) item->common.unknown0236[46], (int8) item->common.unknown0236[47]);
				Message(0, "  U0236[48-55]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0236[48], (int8) item->common.unknown0236[49], (int8) item->common.unknown0236[50], (int8) item->common.unknown0236[51], (int8) item->common.unknown0236[52], (int8) item->common.unknown0236[53], (int8) item->common.unknown0236[54], (int8) item->common.unknown0236[55]);
				*/
			}
			/* TODO: FIX THE UNKNOWNS THAT ARE DISPLAYED!!!
			Message(0, "  U0103[ 0-11]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->unknown0103[0], (int8) item->unknown0103[1], (int8) item->unknown0103[2], (int8) item->unknown0103[3], (int8) item->unknown0103[4], (int8) item->unknown0103[5], (int8) item->unknown0103[6], (int8) item->unknown0103[7], (int8) item->unknown0103[8], (int8) item->unknown0103[9], (int8) item->unknown0103[10], (int8) item->unknown0103[11]);
			Message(0, "  U0103[12-21]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->unknown0103[12], (int8) item->unknown0103[13], (int8) item->unknown0103[14], (int8) item->unknown0103[15], (int8) item->unknown0103[16], (int8) item->unknown0103[17], (int8) item->unknown0103[18], (int8) item->unknown0103[19], (int8) item->unknown0103[20], (int8) item->unknown0103[21]);
			Message(0, "  U0144[ 0-11]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->unknown0144[0], (int8) item->unknown0144[1], (int8) item->unknown0144[2], (int8) item->unknown0144[3], (int8) item->unknown0144[4], (int8) item->unknown0144[5], (int8) item->unknown0144[6], (int8) item->unknown0144[7], (int8) item->unknown0144[8], (int8) item->unknown0144[9], (int8) item->unknown0144[10], (int8) item->unknown0144[11]);
			Message(0, "  U0144[12-23]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->unknown0144[12], (int8) item->unknown0144[13], (int8) item->unknown0144[14], (int8) item->unknown0144[15], (int8) item->unknown0144[16], (int8) item->unknown0144[17], (int8) item->unknown0144[18], (int8) item->unknown0144[19], (int8) item->unknown0144[20], (int8) item->unknown0144[21], (int8) item->unknown0144[22], (int8) item->unknown0144[23]);
			Message(0, "  U0144[24-27]: 0x%02x%02x 0x%02x%02x", (int8) item->unknown0144[24], (int8) item->unknown0144[25], (int8) item->unknown0144[26], (int8) item->unknown0144[27]);
			*/
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#uptime") == 0) {
		if (!worldserver.Connected()) {
			Message(0, "Error: World server disconnected");
		}
		else if (sep->IsNumber(1) && atoi(sep->arg[1]) > 0) {
			ServerPacket* pack = new ServerPacket(ServerOP_Uptime, sizeof(ServerUptime_Struct));
			ServerUptime_Struct* sus = (ServerUptime_Struct*) pack->pBuffer;
			strcpy(sus->adminname, this->GetName());
			sus->zoneserverid = atoi(sep->arg[1]);
			worldserver.SendPacket(pack);
			delete pack;
		}
		else {
			ServerPacket* pack = new ServerPacket(ServerOP_Uptime, sizeof(ServerUptime_Struct));
			ServerUptime_Struct* sus = (ServerUptime_Struct*) pack->pBuffer;
			strcpy(sus->adminname, this->GetName());
			worldserver.SendPacket(pack);
			delete pack;
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#flag") == 0)
	{
		if(sep->arg[2][0] == 0 || admin < 200) {
			this->UpdateAdmin();
			Message(0, "Refreshed your admin flag from DB.");
		}
		else if (!sep->IsNumber(1) || atoi(sep->arg[1]) < -2 || atoi(sep->arg[1]) > 255 || strlen(sep->arg[2]) == 0)
			Message(0, "Usage: #flag [status] [acctname]");
		else {
			if (atoi(sep->arg[1]) > admin)
				Message(0, "You cannot set people's status to higher than your own");
			else if (atoi(sep->arg[1]) < 0 && admin < 100)
				Message(0, "You have too low of status to suspend/ban");
			else if (!database.SetGMFlag(sep->argplus[2], atoi(sep->arg[1])))
				Message(0, "Unable to set GM Flag.");
			else {
				Message(0, "Set GM Flag on account.");
				ServerPacket* pack = new ServerPacket(ServerOP_FlagUpdate, 6);
				*((int32*) pack->pBuffer) = database.GetAccountIDByName(sep->argplus[2]);
				*((sint16*) &pack->pBuffer[4]) = atoi(sep->arg[1]);
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
		found=true;
	}
	else if(strcasecmp(sep->arg[0],"#time") == 0) { // image - Redone by Scruffy
			//yyyy mm dd hh mm local
			bool cmdlvl = CheckAccess(80, 80);
			char timeMessage[255];
			if(cmdlvl && sep->IsNumber(1))
			{
				if(!sep->IsNumber(2))
				{
					sep->arg[2]="00";
				}
				Message(13, "Setting world time to %s:%s (Timezone: 0)...", sep->arg[1], sep->arg[2]);
				zone->SetTime(atoi(sep->arg[1]), atoi(sep->arg[2]));
			}
			else {
				if(cmdlvl)
					Message(13, "To set the Time: #time HH [MM]");
				TimeOfDay_Struct eqTime;

				zone->zone_time.getEQTimeOfDay( time(0), &eqTime);
				//if ( eqTime.hour >= 0 && eqTime.minute >= 0 )
				//{
					sprintf(timeMessage,"%02d:%s%d %s (Timezone: %ih %im)",
						(eqTime.hour % 12) == 0 ? 12 : (eqTime.hour % 12),
						(eqTime.minute < 10) ? "0" : "",
						eqTime.minute,
						(eqTime.hour >= 12) ? "pm" : "am",
						zone->zone_time.getEQTimeZoneHr(),
						zone->zone_time.getEQTimeZoneMin()
						);
					Message(13, "It is now %s.", timeMessage);
#if DEBUG >= 11
					LogFile->write(EQEMuLog::Debug,"Recieved timeMessage:%s", timeMessage);
#endif
			}
		}
	else if (strcasecmp(sep->arg[0], "#guild") == 0 || strcasecmp(sep->arg[0], "#guilds") == 0) {
		GuildCommand(sep);
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#zonestatus") == 0) {
		if (!worldserver.Connected())
			Message(0, "Error: World server disconnected");
		else {
			ServerPacket* pack = new ServerPacket;
			pack->size = strlen(this->GetName())+2;
			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, pack->size);
			pack->opcode = ServerOP_ZoneStatus;
			memset(pack->pBuffer, (int8) admin, 1);
			strcpy((char *) &pack->pBuffer[1], this->GetName());
			worldserver.SendPacket(pack);
			delete pack;
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#manaburn")==0) {
		if (target == 0)
			Message(0, "#Manaburn needs a target.");
		else {
			uint8 *aa_item = &(((uint8 *)&aa)[226]);
			int cur_level = (*aa_item);
			if (DistNoRootNoZ(this) > 200)
				Message(0,"You are too far away from your target.");
			else {
				if(cur_level == 1) {
					int nukedmg=(GetMana())*2;
					if (target->IsNPC()==false)
						if ((this->CastToClient()->GetPVP())==false || (target->CastToClient()->GetPVP())==false)
						SetMana(0);
					SetMana(0);
					LogFile->write(EQEMuLog::Normal,"Manaburn request from %s, damage:", GetName(), nukedmg);
					EntityList entity;
					if (nukedmg>0){
						target->Damage(this, nukedmg, 2751,240);
						//Mob *other2;

						Message(4,"You unleash an enormous blast of magical energies.");
					}
			//entity.NPCMessage(target,false,200,4,"%s is hit by an enormous blast of magical energies.",other2->GetName);
			}
			else
				Message(0, "You have not learned this skill.");
			}
		}

		found=true;
	}
	else if (strcasecmp(sep->arg[0],"#viewmessage")==0){
		char errbuf[MYSQL_ERRMSG_SIZE];
		char *query = 0;
//		char errbuf2[MYSQL_ERRMSG_SIZE];
//		char *query2 = 0;
		MYSQL_RES *result;
//		MYSQL_RES *result2;
		MYSQL_ROW row;
//		MYSQL_ROW row2;
		if(sep->arg[1][0]==0){
			if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT id,date,receiver,sender,message from tellque where receiver='%s'",this->name), errbuf, &result)) {
				if (mysql_num_rows(result)>0){
					Message(0,"You have messages waiting for you to view.");
					Message(0,"Type #Viewmessage <Message ID> to view the message.");
					Message(0," ID , Message Sent Date, Message Sender");
					while ((row = mysql_fetch_row(result)))
						Message(0,"ID: %s Sent Date: %s Sender: %s ",row[0],row[1],row[3]);
				}
				else
					Message(0,"You have no new messages");
				mysql_free_result(result);
			}
			delete[] query;
		}
		else {
			if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT id,date,receiver,sender,message from tellque where id=%s",sep->argplus[1]), errbuf, &result)){
				//char *name=this->name;
				if (mysql_num_rows(result)==1){
				row = mysql_fetch_row(result);
				if (strcasecmp((char*) this->name, (char*) row[2]) == 0){
					Message(15,"ID: %s,Sent Date: %s,Sender: %s,Message: %s",row[0],row[1],row[3],row[4]);
					database.RunQuery(query, MakeAnyLenString(&query, "Delete from tellque where id=%s",row[0]), errbuf, &result);
				}
				else
					Message(13,"Invalid Message Number, check the number and try again.");
				}
				else
					Message(13,"Invalid Message Number, check the number and try again.");
				mysql_free_result(result);
			}
			delete[] query;
		}

		found=true;

	}

	else if (strcasecmp(sep->arg[0], "#help") == 0) {
		if (!(strcasecmp(sep->arg[1], "normal") == 0 
			|| strcasecmp(sep->arg[1], "privuser") == 0 || strcasecmp(sep->arg[1], "pu") == 0
			|| strcasecmp(sep->arg[1], "vprivuser") == 0 || strcasecmp(sep->arg[1], "vpu") == 0
			|| strcasecmp(sep->arg[1], "questtroupe") == 0 || strcasecmp(sep->arg[1], "qt") == 0
			|| strcasecmp(sep->arg[1], "gm") == 0
			|| strcasecmp(sep->arg[1], "leadgm") == 0 || strcasecmp(sep->arg[1], "lgm") == 0
			|| strcasecmp(sep->arg[1], "debug") == 0
			|| strcasecmp(sep->arg[1], "serverop") == 0 || strcasecmp(sep->arg[1], "sop") == 0
			|| strcasecmp(sep->arg[1], "all") == 0)) {
			Message(0, "Usage: #help <helpfile>");
			Message(0, "  Normal - Normal EQEmu commands.");
			if(admin >= 10)	{	Message(0, "  Privuser - Privileged user commands."); }
			if(admin >= 20)	{	Message(0, "  vPrivuser - Very privileged user commands."); }
			if(admin >= 80)	{	Message(0, "  QuestTroupe - QuestTroupe commands."); }
			if(admin >= 100)	{	Message(0, "  GM - Game master commands."); }
			if(admin >= 150)	{	Message(0, "  LeadGM - Lead GM commands."); }
			if(admin >= 200)	{	Message(0, "  ServerOP - Server operator commands."); }
			if(admin >= 201)	{	Message(0, "  Debug - Debugging commands."); }
		}
		if (strcasecmp(sep->arg[1], "normal") == 0 || strcasecmp(sep->arg[1], "all") == 0)
		{
			Message(0, "EQEMu Commands:");
			Message(0, "  #itemsearch [id] - Searches item database.");
			Message(0, "  #summonitem [id] - Summons an item.");
			Message(0, "  #loc - Shows you your current location.");
			Message(0, "  #goto  [x,y,z] - Warps you to specified coordinates.");
			Message(0, "  #zone [zonename] - Zones to safepoint in the specified zone.");

			Message(0, "  #zonestatus - Shows what zones are up.");
			Message(0, "  #guild - Guild commands (type for more info)");
			Message(0, "  #showstats - Shows what the server thinks your stats are.");
			//Message(0, "  #filters - Allows for you to make the server filter out some information (can reduce lag).");
		}
		if ((strcasecmp(sep->arg[1], "privuser") == 0 || strcasecmp(sep->arg[1], "pu") == 0 || strcasecmp(sep->arg[1], "all") == 0) && admin >= 10)
		{
			Message(0, "EQEMu Priviliged User Commands:");
			Message(0, "  #level [id] - Sets your target\'s level.");
			Message(0, "  #heal - (PC only) Completely heals your target.");
			Message(0, "  #mana - (PC only) Replenishes your target\'s mana.");

			Message(0, "  #spawn - Used to spawn NPCs");
			Message(0, "  #dbspawn [npctypeid] - Spawns an NPC type from the database.");
			Message(0, "  #texture [texture] [helmtexture]	(0-255, 255 for show equipment)");
			Message(0, "  #gender [0-2] - (0=male, 1=female, 2=neuter)");
			Message(0, "  #npctypespawn [npctype]");
		}
		if ((strcasecmp(sep->arg[1], "vprivuser") == 0 || strcasecmp(sep->arg[1], "vpu") == 0 || strcasecmp(sep->arg[1], "all") == 0) && admin >= 20)
		{
			Message(0, "EQEMu Very Priviliged User Commands:");
			Message(0, "  #size [size] - Sets your targets size.");
			Message(0, "  #findspell [spellname] - Searches the spell database for [spellname]");
			Message(0, "  #castspell [id] - Casts a spell, use #findspell to determine the id number.");
			Message(0, "  #setskill [skill num (0-73)] [number (0-255)] - Sets the targeted player's skill to number.");
			Message(0, "  #setallskill [0-255] - Sets all of the targeted player's skills to number.");
			Message(0, "  #flymode - Allows you to move freely on the z axis.");
			Message(0, "  #race [0-329]  (0 for back to normal)");
			Message(0, "  #makepet - Makes a custom pet, #makepet for details.");
		}
		if ((strcasecmp(sep->arg[1], "questtroupe") == 0 || strcasecmp(sep->arg[1], "qt") == 0 || strcasecmp(sep->arg[1], "all") == 0) && admin >= 80) 
		{
			Message(0, "EQEMu Quest Troupe Commands:");
			Message(0, "  #npcloot [show/money/add/remove] [itemid/all/money: pp gp sp cp] - Adjusts the loot the targeted npc is carrying.");
			Message(0, "  #npcstats - Shows the stats of the targetted NPC.");
			Message(0, "  #zheader [zone name/none] - Change the sky/fog/etc. of the current zone.");
			Message(0, "  #zsky [sky number] - Changes the sky of the current zone.");
			Message(0, "  #zcolor [red] [green] [blue] - Changes the fog colour of the current zone.");
			Message(0, "  #zstats - Shows the zone header data for the current zone.");
			Message(0, "  #timeofday - Sets the date to Monday, Janurary 1st, 1 at the specified time.");
			Message(0, "  #date - Sets the time to the specified date.");
			Message(0, "  #weather <0/1/2> - Off/Rain/Snow.");
			Message(0, "  #permaclass <classnum> - Changes your class.");
			Message(0, "  #permarace <racenum> - Changes your race.");
			Message(0, "  #permagender <0/1/2> - Changes your gender.");
			Message(0, "  #emote - Sends an emotish message, type for syntax.");
			Message(0, "  #invul [1/0] - Makes the targeted player invulnerable to attack");
			Message(0, "  #hideme [0/1]	- Removes you from the spawn list.");
			Message(0, "  #npccast [targetname] [spellid] - Makes the targeted NPC cast on [targetname].");
			Message(0, "  #zclip [min clip] [max clip] - Changes the clipping plane of the current zone.");
		}
		if ((strcasecmp(sep->arg[1], "gm") == 0 || strcasecmp(sep->arg[1], "all") == 0) && admin >= 100)
		{
			Message(0, "EQEMu GM Commands:");
			Message(0, "  #kill - Kills your selected target.");
			Message(0, "  #damage [id] - Inflicts damage upon target.");

			Message(0, "  #setxp - Sets the target\'s experience.");
			Message(0, "  /broadcast [text] - Sends a broadcast message.");
			Message(0, "  /pr [text] - Sends text over GMSAY.");
			Message(0, "  /lastname - Sets a player's lastname.");
			Message(0, "  /name - Sets a player's name");
			Message(0, "  #depop - Depops targeted NPC.");
			Message(0, "  #depopzone - Depops the zone.");
			Message(0, "  #npcspecialattk - Allows you to set the special attack flags by selecting the NPC.");
			Message(0, "  #repop [delay] - Repops the zone, optional delay in seconds.");
			Message(0, "  #zuwcoords [number] - Changes the underworld coordinates of the current zone.");
			Message(0, "  #zsafecoords [x] [y] [z] - Changes the safe coordinates of the current zone.");
			Message(0, "  #listnpcs - Lists all the NPCs currently spawned in the zone.");
			Message(0, "  #showbuffs - Shows the buffs on your current target.");
			Message(0, "  #nukebuffs - Dispells your current target.");
			Message(0, "  #freeze - Freezes your current target.");

			Message(0, "  #unfreeze	- Unfreezes your current target.");
			Message(0, "  #pvp [on|off] - Toggles your player killer flag.");
			Message(0, "  #gm [on|off] - Toggles your GM flag.");
			Message(0, "  #gmspeed [on|off] - Toggles GMSpeed Hack (Zone for it to take affect).");
			Message(0, "  #movechar [charname] [zonename] - Moves a Character To a New Zone (If they are offline).");
			Message(0, "  #corpse - Corpse commands (type for more info)");
		}
		if ((strcasecmp(sep->arg[1], "leadgm") == 0 || strcasecmp(sep->arg[1], "all") == 0) && admin >= 150)
		{
			Message(0, "EQEMu Lead-GM Commands:");
			Message(0, "  #summon [charname] - Summons a player to you.");
			Message(0, "  #attack [charname] - Make targetted npc attack [charname].");
			Message(0, "  #kick [charname] - Kicks player off of the server.");
			Message(0, "  #zoneshutdown [ZoneID | ZoneName] - Shuts down the zoneserver.");
			Message(0, "  #zonebootup [ZoneID] [ZoneName] - Boots [ZoneName] on the zoneserver specified.");
			Message(0, "  #motd [New MoTD Message] - Changes the server's MOTD.");
			Message(0, "  #lock - Locks the world server.");
			Message(0, "  #unlock - Unlocks or unlocks the server.");
		}
		if ((strcasecmp(sep->arg[1], "serverop") == 0 || strcasecmp(sep->arg[1], "sop") == 0 || strcasecmp(sep->arg[1], "all") == 0) && admin >= 200)
		{
			Message(0, "EQEMu ServerOperator Commands:");
			Message(0, "  #shutdown - Shuts down the server.");
			Message(0, "  #worldshutdown - Shuts down the worldserver and all zones.");
			Message(0, "  #flag [status] [name] - Flags an account with GM status.");
			Message(0, "  #delacct [name] - Deletes an EQEmu account.");
			Message(0, "  #zsave [file name] - Save the current zone header to ./cfg/<filename>.cfg");
			Message(0, "  #dbspawn2 [spawngroup] [respawn] [variance]");
			Message(0, "  #npcspawn [create|add|update|remove|delete]");
         Message(0, "  #fixmob [nextrace|prevrace|gender|nexttexture|prevtexture|nexthelm|prevhelm]"); 
		}
		if ((strcasecmp(sep->arg[1], "debug") == 0 || strcasecmp(sep->arg[1], "all") == 0) && admin >= 201)
		{
			Message(0, "EQEMu Debug Commands:");
			Message(0, "  #version - Shows information about the zone executable.");
			Message(0, "  #serverinfo [type] - Shows information about the server, #serverinfo for details.");
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#doanim") == 0) {
		if (!sep->IsNumber(1)) {
			Message(0, "Usage: #DoAnim [number]");
		}
		else {
			if (admin >= 100) {
				if (target == 0)
					Message(0, "Error: You need a target.");
				else
					target->DoAnim(atoi(sep->arg[1]));

			}

			else
				DoAnim(atoi(sep->arg[1]));
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#face") == 0)	{
		APPLAYER* outapp = new APPLAYER(OP_Illusion, sizeof(Illusion_Struct));
		Illusion_Struct* is = (Illusion_Struct*) outapp->pBuffer;
		is->spawnid = GetID();
		is->race = GetBaseRace();
		is->gender = GetBaseGender();
		is->texture = 0x00;
		is->helmtexture = 0x00;
		is->haircolor = 0x00;
		is->beardcolor = GetFace();
		is->eyecolor1 = 0xFF;
		is->eyecolor2 = 0xFF;
		is->hairstyle = 0xFF;
		is->title = 0xFF;
		is->luclinface = 0xFF;
		entity_list.QueueClients(this, outapp);
		delete outapp;
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#scribespellsort")==0 ) //This could be done better for either memory or cpu usage.. I choose a kind of middle ground
	{														  //If you are reading this please check it over.. its pretty dirty
		if(sep->arg[1][0]==0) {								  //TODO: Get rid of the AA spell skil things.. i guess a list of them all?
			Message(0,"FORMAT: #scribespellsort <level>");
		}
		if((atoi(sep->arg[1]) < 1) || (atoi(sep->arg[1]) > 65))
			Message(0,"ERROR: Enter a level between 1 and 65 inclusive."); 
		else 
		{
			Message(0, "Scribing spells to spellbook");
			LogFile->write(EQEMuLog::Normal,"Scribe spells(with sort) request from %s, level:%c", GetName(), sep->arg[1][0] );
			int bookcount=0;
			int tempcount[65];for(int r=0;r<65;r++) tempcount[r]=0;//where in the array for templist each spell level is
			int templist[256][65]; for(int y=0;y<256;y++){for(int u =0;u<65;u++) {templist[y][u]=0;}}  //Temporary lists of size for the max spell book for all spell levels          
			for (int i=0; i < SPDAT_RECORDS; i++)
			{
				if ((spells[i].classes[pp.class_-1] <=atoi(sep->arg[1])) && !(spells[i].classes[0]<= atoi(sep->arg[1])) && (spells[i].skill != 52)) 
				{
					if(tempcount[spells[i].classes[pp.class_-1]] <=255)//if the array at this spell level is not full
					{
						templist[tempcount[spells[i].classes[pp.class_-1]]][spells[i].classes[pp.class_-1]]=i;// add spell to array
						tempcount[spells[i].classes[pp.class_-1]]++;//iterate counter(duh?)
					}

				}
			}
			for(int h =0;h<65;h++) //iterate through spell levels
			{	
				for(int g=0;g<tempcount[h];g++)//iterate through the spell lists up to used count
				{
					if(bookcount <256) //if the spell book is not full
					{
						pp.spell_book[bookcount]=templist[g][h]; //assign the spell
						bookcount++;
					}
					else 
						break;// its full, stop
				}
			}
		}
		found=true;
	}else if (strcasecmp(sep->arg[0], "#scribespells")==0 ) {
		if(sep->arg[1][0]==0) {
			Message(0,"FORMAT: #scribespells <level>");
		}
		if((atoi(sep->arg[1]) < 1) || (atoi(sep->arg[1]) > 65))
			Message(0,"ERROR: Enter a level between 1 and 65 inclusive."); 
		else {
			Message(0, "Scribing spells to spellbook");
			LogFile->write(EQEMuLog::Normal,"Scribe spells request from %s, level:%c", GetName(), sep->arg[1][0] );
			int bookcount =0;

			for (int i = 0; i < SPDAT_RECORDS; i++)
			{
				if ((spells[i].classes[pp.class_-1] <=atoi(sep->arg[1])) && !(spells[i].classes[0]<= atoi(sep->arg[1])) && (spells[i].skill != 52)) 
				{
					if(bookcount <=255)
					{
						pp.spell_book[bookcount]=i;
						bookcount++;
					}
				}
			}
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#wpinfo") == 0)
	{
		if (target == 0 || !target->IsNPC())
			Message(0,"You must target an NPC to use this.");
		else
//			Message(0,"NPC waypoint data: X: %f Y: %f Z: %f Pause: %i Grid: %i Max WP: %i Wandertype: %i Pausetype: %i CurWP: %i",target->CastToNPC()->cur_wp_x,target->CastToNPC()->cur_wp_y,target->CastToNPC()->cur_wp_z,target->CastToNPC()->wp_s[atoi(sep->arg[1])],target->CastToNPC()->wp_a[4],target->CastToNPC()->wp_a[0],target->CastToNPC()->wp_a[1],target->CastToNPC()->wp_a[2],target->CastToNPC()->wp_a[3]);
		found=true;
	}
	else if (!strcasecmp(sep->arg[0], "#wpadd") && admin >= 200)
	{
			int type1=0;
			int type2=1;
			int pause=14;
		if (target && target->IsNPC()) {
			int wp_n=0;
			if (target->CastToMob()->GetMaxWp() > 0) wp_n = target->CastToMob()->GetMaxWp() + 1;
			else wp_n = 1;
			if (sep->arg[1] && !strcmp(strlwr(sep->arg[1]),"circular")) type1=0;
			if (sep->arg[1] && !strcmp(strlwr(sep->arg[1]),"random"))	type1=2;
			if (sep->arg[1] && !strcmp(strlwr(sep->arg[1]),"patrol"))	type1=3;
			if (sep->arg[2] && atoi(sep->arg[2]) > 0)	pause=atoi(sep->arg[2]);
			int32 tmp_grid = database.AddWP(target->CastToNPC()->GetSp2(), target->CastToNPC()->GetGrid(), wp_n, this->GetX(),this->GetY(),this->GetZ(),pause,target->GetX(), target->GetY(), target->GetZ(), type1, type2);
			if (tmp_grid) target->CastToNPC()->SetGrid(tmp_grid);
			target->CastToNPC()->AssignWaypoints(target->CastToNPC()->GetGrid());
		}
//		if (target) this->GetTarget()->CastToMob()->SendTo(atof(sep->arg[1]),atof(sep->arg[2]),atof(sep->arg[3]));
		found=true;
	}
	else if (!strcasecmp(sep->arg[0], "#interrupt"))
	{
		int16 ci_message=0x01b7, ci_color=0x0121;
		if(sep->arg[1][0])
			ci_message=atoi(sep->arg[1]);
		if(sep->arg[2][0])
			ci_color=atoi(sep->arg[2]);

		this->InterruptSpell(ci_message, ci_color);
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#d1") == 0) {
		APPLAYER app;
		app.opcode = OP_Action;
		app.size = sizeof(Action_Struct);
		app.pBuffer = new uchar[app.size];
		memset(app.pBuffer, 0, app.size);
		Action_Struct* a = (Action_Struct*)app.pBuffer;
		adverrorinfo = 412;
		a->target = GetID();
		a->source = this->GetID();
		a->type = atoi(sep->arg[1]);
		a->spell = atoi(sep->arg[2]);
		a->damage = atoi(sep->arg[3]);
		app.priority = 1;
		entity_list.QueueCloseClients(this, &app);
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#summonitem") == 0) {
		if (!sep->IsNumber(1)) {
			Message(0, "Usage: #summonitem x , x is an item number");
		}
		else {
			int32 itemid = atoi(sep->arg[1]);
			if (database.GetItemStatus(itemid) > admin) {
				Message(13, "Error: Insufficient status to summon this item.");
			}

			else if (sep->IsNumber(2) && admin > 20) {
				SummonItem(itemid, atoi(sep->arg[2]));
			}
			else {
				SummonItem(itemid);
			}
		}
		found=true;
	}
	else if ((strcasecmp(sep->arg[0], "#itemsearch") == 0 || strcasecmp(sep->arg[0], "#search") == 0 || strcasecmp(sep->arg[0], "#finditem") == 0)) {
		if (sep->arg[1][0] == 0) {
			Message(0, "Usage: #itemsearch item OR #search item");
		} else {
			FindItem(sep->argplus[1]);

		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#datarate") == 0) {
		if (sep->arg[1][0] == 0) {
			Message(0, "Datarate: %1.1f", eqnc->GetDataRate());
			if (admin >= 201) {
				Message(0, "Dataflow: %i", eqnc->GetDataFlow());
				Message(0, "Datahigh: %i", eqnc->GetDataHigh());
			}
		}
		else if (sep->IsNumber(1) && atof(sep->arg[1]) > 0 && (admin >= 201 || atof(sep->arg[1]) <= 25)) {
			eqnc->SetDataRate(atof(sep->arg[1]));
			Message(0, "Datarate: %1.1f", eqnc->GetDataRate());
		}
		else {
			Message(0, "Usage: #DataRate [new data rate in kb/sec, max 25]");
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#dmg1") == 0) {
		if(strlen(sep->arg[0]) == 0)
			found=true;
		else
		{
		APPLAYER app;
		app.opcode = OP_Action;
		app.size = sizeof(Action_Struct);
		app.pBuffer = new uchar[app.size];
		memset(app.pBuffer, 0, app.size);
		Action_Struct* a = (Action_Struct*)app.pBuffer;
		adverrorinfo = 412;
		a->target = target->GetID();
		a->source = GetID();
		a->type = atoi(sep->arg[1]);
		a->spell = 16;
		a->damage = 3;
		app.priority = 1;
		entity_list.QueueCloseClients(this, &app);
		found=true;
	}
	}
	else if (strcasecmp(sep->arg[0], "#setaaxp") == 0) {
		if(sep->arg[1][0] == 0) {
			Message(0, "Usage: #setaaxp <new AA XP value>");
		}
		else {
			SetEXP(GetEXP(), atoi(sep->arg[1]), false);
		}
		found=true;
	}
	else if (strcasecmp(sep->arg[0], "#setaapts") == 0) {
		if(sep->arg[1][0] == 0) {
			Message(0, "Usage: #setaapts <new AA points value>");
		}
		else if(atoi(sep->arg[1]) <= 0 || atoi(sep->arg[1]) > 200) {
			Message(0, "You must have a number greater than 0 for points and no more than 200.");
		}
			else {
			SetEXP(GetEXP(),max_AAXP*atoi(sep->arg[1]),false);
			SendAAStats();
			SendAATable();
		}
			found=true;
	}
	else {
		Message(0, "Command does not exist");
	}
	return found;
}

void Client::GuildCommand(const Seperator* sep) {
	if (strcasecmp(sep->arg[1], "help") == 0) {

		Message(0, "Guild commands:");
		Message(0, "  #guild status [name] - shows guild and rank of target");
		Message(0, "  #guild info guildnum - shows info/current structure");
		Message(0, "  #guild invite [charname]");
		Message(0, "  #guild remove [charname]");
		Message(0, "  #guild promote rank [charname]");
		Message(0, "  #guild demote rank [charname]");
		Message(0, "  /guildmotd [newmotd]    (use 'none' to clear)");
		Message(0, "  #guild edit rank title newtitle");
		Message(0, "  #guild edit rank permission 0/1");
		Message(0, "  #guild leader newleader (they must be rank0)");
		if (admin >= 100) {
			Message(0, "GM Guild commands:");
			Message(0, "  #guild list - lists all guilds on the server");
			Message(0, "  #guild create {guildleader charname or AccountID} guildname");
			Message(0, "  #guild delete guildDBID");
			Message(0, "  #guild rename guildDBID newname");
			Message(0, "  #guild set charname guildDBID    (0=no guild)");
			Message(0, "  #guild setrank charname rank");
			Message(0, "  #guild gmedit guilddbid rank title newtitle");
			Message(0, "  #guild gmedit guilddbid rank permission 0/1");
			Message(0, "  #guild setleader guildDBID {guildleader charname or AccountID}");
			Message(0, "  #guild setdoor guildEQID");
		}
		
	}
	else if (strcasecmp(sep->arg[1], "status") == 0 || strcasecmp(sep->arg[1], "stat") == 0) {
		Client* client = 0;
		if (sep->arg[2][0] != 0)
			client = entity_list.GetClientByName(sep->argplus[2]);
		else if (target != 0 && target->IsClient())

			client = target->CastToClient();
		if (client == 0)
			Message(0, "You must target someone or specify a character name");
		else if ((client->Admin() >= 100 && admin < 100) && client->GuildDBID() != guilddbid) // no peeping for GMs, make sure tell message stays the same
			Message(0, "You must target someone or specify a character name.");
		else {
			if (client->GuildDBID() == 0)
				Message(0, "%s is not in a guild.", client->GetName());
			else if (guilds[client->GuildEQID()].leader == client->AccountID())
				Message(0, "%s is the leader of <%s> rank: %s", client->GetName(), guilds[client->GuildEQID()].name, guilds[client->GuildEQID()].rank[client->GuildRank()].rankname);
			else
				Message(0, "%s is a member of <%s> rank: %s", client->GetName(), guilds[client->GuildEQID()].name, guilds[client->GuildEQID()].rank[client->GuildRank()].rankname);
		}
	}
	else if (strcasecmp(sep->arg[1], "info") == 0) {
		if (sep->arg[2][0] == 0 && guilddbid == 0) {
			if (admin >= 100)

				Message(0, "Usage: #guildinfo guilddbid");
			else
				Message(0, "You're not in a guild");
		}

		else {
			int32 tmp = 0xFFFFFFFF;
			if (sep->arg[2][0] == 0)
				tmp = database.GetGuildEQID(guilddbid);
			else if (admin >= 100)
				tmp = database.GetGuildEQID(atoi(sep->arg[2]));
			if (tmp < 0 || tmp >= 512)
				Message(0, "Guild not found.");
			else {
				Message(0, "Guild info DB# %i, %s", guilds[tmp].databaseID, guilds[tmp].name);
				if (admin >= 100 || guildeqid == tmp) {
					if (account_id == guilds[tmp].leader || guildrank == 0 || admin >= 100) {
						char leadername[64];
						database.GetAccountName(guilds[tmp].leader, leadername);
						Message(0, "Guild Leader: %s", leadername);
					}
					Message(0, "Rank 0: %s", guilds[tmp].rank[0].rankname);
					Message(0, "  All Permissions.");
					for (int i = 1; i <= GUILD_MAX_RANK; i++) {
						Message(0, "Rank %i: %s", i, guilds[tmp].rank[i].rankname);
						Message(0, "  HearGU: %s  SpeakGU: %s  Invite: %s  Remove: %s  Promote: %s  Demote: %s  MOTD: %s  War/Peace: %s", guilds[tmp].rank[i].heargu?"Y":"N", guilds[tmp].rank[i].speakgu?"Y":"N", guilds[tmp].rank[i].invite?"Y":"N", guilds[tmp].rank[i].remove?"Y":"N", guilds[tmp].rank[i].promote?"Y":"N", guilds[tmp].rank[i].demote?"Y":"N", guilds[tmp].rank[i].motd?"Y":"N", guilds[tmp].rank[i].warpeace?"Y":"N");
//						Message(0, "  HearGU: %i  SpeakGU: %i  Invite: %i  Remove: %i", guilds[tmp].rank[i].heargu, guilds[tmp].rank[i].speakgu, guilds[tmp].rank[i].invite, guilds[tmp].rank[i].remove);
//						Message(0, "  Promote: %i  Demote: %i  MOTD: %i  War/Peace: %i", guilds[tmp].rank[i].promote, guilds[tmp].rank[i].demote, guilds[tmp].rank[i].motd, guilds[tmp].rank[i].warpeace);
					}
				}
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "leader") == 0) {
		if (guilddbid == 0)
			Message(0, "You arent in a guild!");
		else if (guilds[guildeqid].leader != account_id)
			Message(0, "You aren't the guild leader.");
		else {
			const char* tmptar = 0;
			if (sep->arg[2][0] != 0)
				tmptar = sep->argplus[2];
			else if (tmptar == 0 && target != 0 && target->IsClient())
				tmptar = target->CastToClient()->GetName();
			if (tmptar == 0)
				Message(0, "You must target someone or specify a character name.");
			else {
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_GuildInvite;
				pack->size = sizeof(ServerGuildCommand_Struct);
				pack->pBuffer = new uchar[pack->size];
				memset(pack->pBuffer, 0, pack->size);
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				sgc->guilddbid = guilddbid;
				sgc->guildeqid = guildeqid;
				sgc->fromrank = guildrank;
				sgc->fromaccountid = account_id;
				sgc->admin = admin;
				strcpy(sgc->from, name);
				strcpy(sgc->target, tmptar);
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "invite") == 0) {
		if (guilddbid == 0)
			Message(0, "You arent in a guild!");
		else if (!guilds[guildeqid].rank[guildrank].invite)
			Message(0, "You dont have permission to invite.");
/*#ifdef GUILDWARS
		else if (zone->GetGuildOwned() != 0 && !database.GetGuildAlliance(GuildDBID(),zone->GetGuildOwned()) && zone->GetGuildOwned() != GuildDBID() && zone->GetGuildOwned() != 3)
			Message(0, "You cannot invite guild members in an enemy city unless it is a neutral guild.");
#endif*/
		else {
#ifdef GUILDWARS
				if (database.NumberInGuild(guilddbid)>20){
					Message(15,"Your Guild has reached its Guildwars size limit.  You cannot invite any more people.");
					return;
				}
#endif
			const char* tmptar = 0;
			if (sep->arg[2][0] != 0)
				tmptar = sep->argplus[2];
			else if (tmptar == 0 && target != 0 && target->IsClient())
				tmptar = target->CastToClient()->GetName();
			if (tmptar == 0)
				Message(0, "You must target someone or specify a character name.");
			
			else {
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_GuildInvite;
				pack->size = sizeof(ServerGuildCommand_Struct);
				pack->pBuffer = new uchar[pack->size];
				memset(pack->pBuffer, 0, pack->size);
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				sgc->guilddbid = guilddbid;
				sgc->guildeqid = guildeqid;
				sgc->fromrank = guildrank;
				sgc->fromaccountid = account_id;
				sgc->admin = admin;
				strcpy(sgc->from, name);
				strcpy(sgc->target, tmptar);
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "remove") == 0) {
		if (guilddbid == 0)
			Message(0, "You arent in a guild!");
		else if ((!guilds[guildeqid].rank[guildrank].remove) && !(target == this && sep->arg[2][0] == 0))
			Message(0, "You dont have permission to remove.");
		else {
			const char* tmptar = 0;
			if (sep->arg[2][0] != 0)
				tmptar = sep->argplus[2];
			else if (tmptar == 0 && target != 0 && target->IsClient())
				tmptar = target->CastToClient()->GetName();
			if (tmptar == 0)
				Message(0, "You must target someone or specify a character name.");
			else {
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_GuildRemove;
				pack->size = sizeof(ServerGuildCommand_Struct);
				pack->pBuffer = new uchar[pack->size];
				memset(pack->pBuffer, 0, pack->size);
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				sgc->guilddbid = guilddbid;
				sgc->guildeqid = guildeqid;
				sgc->fromrank = guildrank;
				sgc->fromaccountid = account_id;
				sgc->admin = admin;
				strcpy(sgc->from, name);
				strcpy(sgc->target, tmptar);
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "promote") == 0) {
		if (guilddbid == 0)
			Message(0, "You arent in a guild!");
		else if (!(strlen(sep->arg[2]) == 1 && sep->arg[2][0] >= '0' && sep->arg[2][0] <= '9'))
			Message(0, "Usage: #guild promote rank [charname]");
		else if (atoi(sep->arg[2]) < 0 || atoi(sep->arg[2]) > GUILD_MAX_RANK)
			Message(0, "Error: invalid rank #.");
		else {
			const char* tmptar = 0;
			if (sep->arg[3][0] != 0)
				tmptar = sep->argplus[3];
			else if (tmptar == 0 && target != 0 && target->IsClient())
				tmptar = target->CastToClient()->GetName();
			if (tmptar == 0)
				Message(0, "You must target someone or specify a character name.");
			else {

				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_GuildPromote;
				pack->size = sizeof(ServerGuildCommand_Struct);
				pack->pBuffer = new uchar[pack->size];
				memset(pack->pBuffer, 0, pack->size);
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				sgc->guilddbid = guilddbid;
				sgc->guildeqid = guildeqid;
				sgc->fromrank = guildrank;
				sgc->fromaccountid = account_id;
				sgc->admin = admin;
				sgc->newrank = atoi(sep->arg[2]);
				strcpy(sgc->from, name);
				strcpy(sgc->target, tmptar);
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "demote") == 0) {
		if (guilddbid == 0)
			Message(0, "You arent in a guild!");
		else if (!(strlen(sep->arg[2]) == 1 && sep->arg[2][0] >= '0' && sep->arg[2][0] <= '9'))
			Message(0, "Usage: #guild demote rank [charname]");
		else if (atoi(sep->arg[2]) < 0 || atoi(sep->arg[2]) > GUILD_MAX_RANK)
			Message(0, "Error: invalid rank #.");
		else {
			const char* tmptar = 0;
			if (sep->arg[3][0] != 0)
				tmptar = sep->argplus[3];
			else if (tmptar == 0 && target != 0 && target->IsClient())
				tmptar = target->CastToClient()->GetName();
			if (tmptar == 0)
				Message(0, "You must target someone or specify a character name.");
			else {
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_GuildDemote;
				pack->size = sizeof(ServerGuildCommand_Struct);
				pack->pBuffer = new uchar[pack->size];
				memset(pack->pBuffer, 0, pack->size);
				ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
				sgc->guilddbid = guilddbid;
				sgc->guildeqid = guildeqid;
				sgc->fromrank = guildrank;
				sgc->fromaccountid = account_id;
				sgc->admin = admin;
				sgc->newrank = atoi(sep->arg[2]);
				strcpy(sgc->from, name);
				strcpy(sgc->target, tmptar);
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "motd") == 0) {
		if (guilddbid == 0)
			Message(0, "You arent in a guild!");
		else if (!guilds[guildeqid].rank[guildrank].motd)
			Message(0, "You dont have permission to change the motd.");
		else if (!worldserver.Connected())
			Message(0, "Error: World server dirconnected");
		else {
			char tmp[255];
			if (strcasecmp(sep->argplus[2], "none") == 0)
				strcpy(tmp, "");
			else
				snprintf(tmp, sizeof(tmp), "%s - %s", this->GetName(), sep->argplus[2]);
			if (database.SetGuildMOTD(guilddbid, tmp)) {
			/*
			ServerPacket* pack = new ServerPacket;
			pack->opcode = ServerOP_RefreshGuild;
			pack->size = 5;
			pack->pBuffer = new uchar[pack->size];
			memcpy(pack->pBuffer, &guildeqid, 4);
			worldserver.SendPacket(pack);
			delete pack;
				*/
			}
			else {
				Message(0, "Motd update failed.");
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "edit") == 0) {
		if (guilddbid == 0)
			Message(0, "You arent in a guild!");
		else if (!sep->IsNumber(2))
			Message(0, "Error: invalid rank #.");
		else if (atoi(sep->arg[2]) < 0 || atoi(sep->arg[2]) > GUILD_MAX_RANK)
			Message(0, "Error: invalid rank #.");
		else if (!guildrank == 0)
			Message(0, "You must be rank %s to use edit.", guilds[guildeqid].rank[0].rankname);
		else if (!worldserver.Connected())

			Message(0, "Error: World server dirconnected");
		else {
			if (!GuildEditCommand(guilddbid, guildeqid, atoi(sep->arg[2]), sep->arg[3], sep->argplus[4])) {
				Message(0, "  #guild edit rank title newtitle");
				Message(0, "  #guild edit rank permission 0/1");
			}
			
			else {
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_RefreshGuild;
				pack->size = 5;
				pack->pBuffer = new uchar[pack->size];
				memcpy(pack->pBuffer, &guildeqid, 4);
				worldserver.SendPacket(pack);
				delete pack;
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "gmedit") == 0 && admin >= 100) {
		if (!sep->IsNumber(2))
			Message(0, "Error: invalid guilddbid.");
		else if (!sep->IsNumber(3))
			Message(0, "Error: invalid rank #.");
		else if (atoi(sep->arg[3]) < 0 || atoi(sep->arg[3]) > GUILD_MAX_RANK)
			Message(0, "Error: invalid rank #.");
		else if (!worldserver.Connected())
			Message(0, "Error: World server dirconnected");
		else {
			int32 eqid = database.GetGuildEQID(atoi(sep->arg[2]));
			if (eqid == 0xFFFFFFFF)
				Message(0, "Error: Guild not found");
			else if (!GuildEditCommand(atoi(sep->arg[2]), eqid, atoi(sep->arg[3]), sep->arg[4], sep->argplus[5])) {
				Message(0, "  #guild gmedit guilddbid rank title newtitle");
				Message(0, "  #guild gmedit guilddbid rank permission 0/1");
			}
			else {
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_RefreshGuild;
				pack->size = 5;
				pack->pBuffer = new uchar[pack->size];
				memcpy(pack->pBuffer, &eqid, 4);
				worldserver.SendPacket(pack);
				delete pack;
			}
		}

	}

	else if (strcasecmp(sep->arg[1], "set") == 0 && admin >= 80) {

		if (!sep->IsNumber(3))
			Message(0, "Usage: #guild set charname guildgbid (0 = clear guildtag)");
		else {
			ServerPacket* pack = new ServerPacket(ServerOP_GuildGMSet, sizeof(ServerGuildCommand_Struct));
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			sgc->guilddbid = atoi(sep->arg[3]);
			sgc->admin = admin;
			strcpy(sgc->from, name);
			strcpy(sgc->target, sep->arg[2]);
			worldserver.SendPacket(pack);
			delete pack;
		}
	}
	else if (strcasecmp(sep->arg[1], "setdoor") == 0 && admin >= 100) {
		if (!sep->IsNumber(2))
			Message(0, "Usage: #guild setdoor guildEQid (0 = delete guilddoor)");
		else {
			if((!guilds[atoi(sep->arg[2])].databaseID) && (!atoi(sep->arg[2])) ){
				Message(0, "These is no guild with this guildEQid");
			}
			else
			{
				this->IsSettingGuildDoor = true;
				Message(0, "Click on a door you want to become a guilddoor");
				SetGuildDoorID = atoi(sep->arg[2]);
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "setrank") == 0 && admin >= 80) {
		if (!sep->IsNumber(3))
			Message(0, "Usage: #guild setrank charname rank");
		else if (atoi(sep->arg[3]) < 0 || atoi(sep->arg[3]) > GUILD_MAX_RANK)
			Message(0, "Error: invalid rank #.");
		else {
			ServerPacket* pack = new ServerPacket;
			pack->opcode = ServerOP_GuildGMSetRank;
			pack->size = sizeof(ServerGuildCommand_Struct);
			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, pack->size);
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			sgc->newrank = atoi(sep->arg[3]);
			sgc->admin = admin;
			strcpy(sgc->from, name);
			strcpy(sgc->target, sep->arg[2]);
			worldserver.SendPacket(pack);
			delete pack;
		}

	}
	else if (strcasecmp(sep->arg[1], "create") == 0 && admin >= 80) {
		if (sep->arg[3][0] == 0)
			Message(0, "Usage: #guild create {guildleader charname or AccountID} guild name");
		else if (!worldserver.Connected())
			Message(0, "Error: World server dirconnected");
		else {
			int32 leader = 0;
			if (sep->IsNumber(2))
				leader = atoi(sep->arg[2]);
			else
				leader = database.GetAccountIDByChar(sep->arg[2]);
			
			int32 tmp = database.GetGuildDBIDbyLeader(leader);
			if (leader == 0)
				Message(0, "Guild leader not found.");
			else if (tmp != 0) {
				int32 tmp2 = database.GetGuildEQID(tmp);
				Message(0, "Error: %s already is the leader of DB# %i '%s'.", sep->arg[2], tmp, guilds[tmp2].name);
			}
			else {
				int32 tmpeq = database.CreateGuild(sep->argplus[3], leader);
				if (tmpeq == 0xFFFFFFFF)
					Message(0, "Guild creation failed.");
				else {
					ServerPacket* pack = new ServerPacket;
					pack->opcode = ServerOP_RefreshGuild;
					pack->size = 5;
					pack->pBuffer = new uchar[pack->size];
					memcpy(pack->pBuffer, &tmpeq, 4);
					pack->pBuffer[4] = 1;
					worldserver.SendPacket(pack);
					delete pack;
					database.GetGuildRanks(tmpeq, &guilds[tmpeq]);
					Message(0, "Guild created: Leader: %i, DB# %i, EQ# %i: %s", leader, guilds[tmpeq].databaseID, tmpeq, sep->argplus[3]);
				}
				
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "delete") == 0 && admin >= 100) {
		if (!sep->IsNumber(2))
			Message(0, "Usage: #guild delete guildDBID");
		else if (!worldserver.Connected())
			Message(0, "Error: World server dirconnected");
		else {
			int32 tmpeq = database.GetGuildEQID(atoi(sep->arg[2]));
			char tmpname[64];
			if (tmpeq != 0xFFFFFFFF) {
				strcpy(tmpname, guilds[tmpeq].name);
				if (guilds[tmpeq].minstatus > admin && admin < 250) {
					Message(0, "Access denied.");
					return;
				}
			}
			
			if (!database.DeleteGuild(atoi(sep->arg[2])))
				Message(0, "Guild delete failed.");
			else {
				if (tmpeq != 0xFFFFFFFF) {
					ServerPacket* pack = new ServerPacket;
					pack->opcode = ServerOP_RefreshGuild;
					pack->size = 5;
					pack->pBuffer = new uchar[pack->size];
					memcpy(pack->pBuffer, &tmpeq, 4);
					pack->pBuffer[4] = 1;
					worldserver.SendPacket(pack);
					delete pack;
					Message(0, "Guild deleted: DB# %i, EQ# %i: %s", atoi(sep->arg[2]), tmpeq, tmpname);
				} else
					Message(0, "Guild deleted: DB# %i", atoi(sep->arg[2]));
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "rename") == 0 && admin >= 100) {
		if ((!sep->IsNumber(2)) || sep->arg[3][0] == 0)
			Message(0, "Usage: #guild rename guildDBID newname");
		else if (!worldserver.Connected())
			Message(0, "Error: World server dirconnected");
		else {
			int32 tmpeq = database.GetGuildEQID(atoi(sep->arg[2]));
			char tmpname[64];
			if (tmpeq != 0xFFFFFFFF) {
				strcpy(tmpname, guilds[tmpeq].name);
				if (guilds[tmpeq].minstatus > admin && admin < 250) {
					Message(0, "Access denied.");
					return;
				}

			}
			
			if (!database.RenameGuild(atoi(sep->arg[2]), sep->argplus[3]))
				Message(0, "Guild rename failed.");
			else {
				if (tmpeq != 0xFFFFFFFF) {
					ServerPacket* pack = new ServerPacket;
					pack->opcode = ServerOP_RefreshGuild;
					pack->size = 5;
					pack->pBuffer = new uchar[pack->size];
					memcpy(pack->pBuffer, &tmpeq, 4);
					pack->pBuffer[4] = 1;
					worldserver.SendPacket(pack);
					delete pack;
					Message(0, "Guild renamed: DB# %i, EQ# %i, OldName: %s, NewName: %s", atoi(sep->arg[2]), tmpeq, tmpname, sep->argplus[3]);
				} else
					Message(0, "Guild renamed: DB# %i, NewName: %s", atoi(sep->arg[2]), sep->argplus[3]);
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "setleader") == 0 && admin >= 100) {
		if (sep->arg[3][0] == 0 || !sep->IsNumber(2))
			Message(0, "Usage: #guild setleader guilddbid {guildleader charname or AccountID}");
		else if (!worldserver.Connected())

			Message(0, "Error: World server dirconnected");
		else {
			int32 leader = 0;
			if (sep->IsNumber(3))
				leader = atoi(sep->arg[3]);
			else
				leader = database.GetAccountIDByChar(sep->argplus[3]);
			
			int32 tmpdb = database.GetGuildDBIDbyLeader(leader);
			if (leader == 0)
				Message(0, "New leader not found.");
			else if (tmpdb != 0) {
				int32 tmpeq = database.GetGuildEQID(tmpdb);
				if (tmpeq >= 512)
					Message(0, "Error: %s already is the leader of DB# %i.", sep->argplus[3], tmpdb);
				else
					Message(0, "Error: %s already is the leader of DB# %i <%s>.", sep->argplus[3], tmpdb, guilds[tmpeq].name);
			}
			else {
				int32 tmpeq = database.GetGuildEQID(atoi(sep->arg[2]));
				if (tmpeq == 0xFFFFFFFF) {
					Message(0, "Guild not found.");
				}
				else if (guilds[tmpeq].minstatus > admin && admin < 250) {
					Message(0, "Access denied.");
				}
				else if (!database.SetGuildLeader(atoi(sep->arg[2]), leader))
					Message(0, "Guild leader change failed.");
				else {
					ServerPacket* pack = new ServerPacket;
					pack->opcode = ServerOP_RefreshGuild;
					pack->size = 5;
					pack->pBuffer = new uchar[pack->size];
					memcpy(pack->pBuffer, &tmpeq, 4);
					worldserver.SendPacket(pack);
					delete pack;
					Message(0, "Guild leader changed: DB# %s, Leader: %s, Name: <%s>", sep->arg[2], sep->argplus[3], guilds[tmpeq].name);
				}
			}
		}
	}
	else if (strcasecmp(sep->arg[1], "list") == 0 && admin >= 80) {
		int x = 0;
		Message(0, "Listing guilds on the server:");
		char leadername[64];
		for (int i = 0; i < 512; i++) {
			if (guilds[i].databaseID != 0) {
				leadername[0] = 0;
				database.GetAccountName(guilds[i].leader, leadername);
				if (leadername[0] == 0)
					Message(0, "  DB# %i EQ# %i  <%s>", guilds[i].databaseID, i, guilds[i].name);
				else
					Message(0, "  DB# %i EQ# %i  <%s> Leader: %s", guilds[i].databaseID, i, guilds[i].name, leadername);
				x++;
			}
		}
		Message(0, "%i guilds listed.", x);
	}
	else {
		Message(0, "Unknown guild command, try #guild help");
	}
}

bool Client::GuildEditCommand(int32 dbid, int32 eqid, int8 rank, const char* what, const char* value) {
	struct GuildRankLevel_Struct grl;
	strcpy(grl.rankname, guilds[eqid].rank[rank].rankname);
	grl.demote = guilds[eqid].rank[rank].demote;
	grl.heargu = guilds[eqid].rank[rank].heargu;
	grl.invite = guilds[eqid].rank[rank].invite;
	grl.motd = guilds[eqid].rank[rank].motd;
	grl.promote = guilds[eqid].rank[rank].promote;
	grl.remove = guilds[eqid].rank[rank].remove;
	grl.speakgu = guilds[eqid].rank[rank].speakgu;
	grl.warpeace = guilds[eqid].rank[rank].warpeace;
	
	if (strcasecmp(what, "title") == 0) {
		if (strlen(value) > 100)
			Message(0, "Error: Title has a maxium length of 100 characters.");
		else
			strcpy(grl.rankname, value);
	}
	else if (rank == 0)

		Message(0, "Error: Rank 0's permissions can not be changed.");
	else {
		if (!(strlen(value) == 1 && (value[0] == '0' || value[0] == '1')))
			return false;
		if (strcasecmp(what, "demote") == 0)
			grl.demote = (value[0] == '1');
		else if (strcasecmp(what, "heargu") == 0)
			grl.heargu = (value[0] == '1');
		else if (strcasecmp(what, "invite") == 0)
			grl.invite = (value[0] == '1');
		else if (strcasecmp(what, "motd") == 0)
			grl.motd = (value[0] == '1');
		else if (strcasecmp(what, "promote") == 0)
			grl.promote = (value[0] == '1');
		else if (strcasecmp(what, "remove") == 0)
			grl.remove = (value[0] == '1');
		else if (strcasecmp(what, "speakgu") == 0)
			grl.speakgu = (value[0] == '1');

		else if (strcasecmp(what, "warpeace") == 0)
			grl.warpeace = (value[0] == '1');
		else
			Message(0, "Error: Permission name not recognized.");
	}
	if (!database.EditGuild(dbid, rank, &grl))
		Message(0, "Error: database.EditGuild() failed");
	return true;
}

void Client::NPCSpawn(const Seperator* sep) {
	if (target && target->IsNPC()) { 
		if (strcasecmp(sep->arg[1], "create") == 0 && admin >= 200) {
			database.NPCSpawnDB(0, zone->GetShortName(), target->CastToNPC());
			Message(0, "%s created successfully!", target->GetName());
		}
		else if (strcasecmp(sep->arg[1], "add") == 0 && admin >= 200) {
			database.NPCSpawnDB(1, zone->GetShortName(), target->CastToNPC(), atoi(sep->arg[2]));
			Message(0, "%s added successfully!", target->GetName());
		}
		else if (strcasecmp(sep->arg[1], "update") == 0 && admin >= 200) {
			database.NPCSpawnDB(2, zone->GetShortName(), target->CastToNPC());
			Message(0, "%s updated!", target->GetName());
		}

		else if (strcasecmp(sep->arg[1], "remove") == 0 && admin >= 200) {
			database.NPCSpawnDB(3, zone->GetShortName(), target->CastToNPC());
			Message(0, "%s removed successfully from database!", target->GetName());
			target->Depop(false);
		}

		else if (strcasecmp(sep->arg[1], "delete") == 0 && admin >= 200) {
			database.NPCSpawnDB(4, zone->GetShortName(), target->CastToNPC());
			Message(0, "%s deleted from database!", target->GetName());
			target->Depop(false);
		}
		else {
			Message(0, "Error: #npcspawn: Invalid command.");
			Message(0, "Usage: #npcspawn [create|add|update|remove|delete]");
		}
	} else { 
		Message(0, "Error: #npcspawn: You must have a NPC targeted!");
	}
}

void Client::CorpseCommand(const Seperator* sep) {
	if (strcasecmp(sep->arg[1], "DeletePlayerCorpses") == 0 && admin >= 150) {
		sint32 tmp = entity_list.DeletePlayerCorpses();
		if (tmp >= 0)
			Message(0, "%i corpses deleted.", tmp);
		else
			Message(0, "DeletePlayerCorpses Error #%i", tmp);
	}
	else if (strcasecmp(sep->arg[1], "delete") == 0) {
		if (target == 0 || !target->IsCorpse())
			Message(0, "Error: Target the corpse you wish to delete");
		else if (target->IsNPCCorpse()) {

			Message(0, "Depoping %s.", target->GetName());
			target->CastToCorpse()->Delete();
		}
		else if (admin >= 150) {
			Message(0, "Deleting %s.", target->GetName());
			target->CastToCorpse()->Delete();
		}
		else
			Message(0, "Insufficient status to delete player corpse.");
	}
	else if (strcasecmp(sep->arg[1], "ListNPC") == 0) {
		entity_list.ListNPCCorpses(this);
	}
	else if (strcasecmp(sep->arg[1], "ListPlayer") == 0) {
		entity_list.ListPlayerCorpses(this);
	}
	else if (strcasecmp(sep->arg[1], "DeleteNPCCorpses") == 0) {
		sint32 tmp = entity_list.DeleteNPCCorpses();
		if (tmp >= 0)
			Message(0, "%d corpses deleted.", tmp);
		else
			Message(0, "DeletePlayerCorpses Error #%d", tmp);
	}
	else if (strcasecmp(sep->arg[1], "charid") == 0 && admin >= 150) {
		if (target == 0 || !target->IsPlayerCorpse())
			Message(0, "Error: Target must be a player corpse.");
		else if (!sep->IsNumber(2))
			Message(0, "Error: charid must be a number.");
		else
			Message(0, "Setting CharID=%u on PlayerCorpse '%s'", target->CastToCorpse()->SetCharID(atoi(sep->arg[2])), target->GetName());
	}
	else if (strcasecmp(sep->arg[1], "ResetLooter") == 0) {
		if (target == 0 || !target->IsCorpse())
			Message(0, "Error: Target the corpse you wish to reset");
		else
			target->CastToCorpse()->ResetLooter();
	}
	else if (strcasecmp(sep->arg[1], "RemoveCash") == 0) {
		if (target == 0 || !target->IsCorpse())
			Message(0, "Error: Target the corpse you wish to remove the cash from");
		else if (!target->IsPlayerCorpse() || admin >= 150) {
			Message(0, "Removing Cash from %s.", target->GetName());
			target->CastToCorpse()->RemoveCash();
		}
		else
			Message(0, "Insufficient status to modify player corpse.");
	}
	else if (strcasecmp(sep->arg[1], "lock") == 0) {
		if (target == 0 || !target->IsCorpse())
			Message(0, "Error: Target must be a corpse.");
		else {
			target->CastToCorpse()->Lock();

			Message(0, "Locking %s...", target->GetName());
		}
	}
	else if (strcasecmp(sep->arg[1], "unlock") == 0) {
		if (target == 0 || !target->IsCorpse())
			Message(0, "Error: Target must be a corpse.");
		else {
			target->CastToCorpse()->UnLock();
			Message(0, "Unlocking %s...", target->GetName());
		}
	}
	else if (sep->arg[1][0] == 0 || strcasecmp(sep->arg[1], "help") == 0) {
		Message(0, "#Corpse Sub-Commands:");
		Message(0, "  DeleteNPCCorpses");
		Message(0, "  Delete - Delete targetted corpse");
		Message(0, "  ListNPC");
		Message(0, "  ListPlayer");
		Message(0, "  Lock - GM locks the corpse - cannot be looted by non-GM");
		Message(0, "  UnLock");
		Message(0, "  RemoveCash");
		Message(0, "  [to remove items from corpses, loot them]");
		Message(0, "Lead-GM status required to delete/modify player corpses");
		Message(0, "  DeletePlayerCorpses");
		Message(0, "  CharID [charid] - change player corpse's owner");
	}
	else
		Message(0, "Error, #corpse sub-command not found");
}

sint32 Client::CalcMaxMana()
{
	switch(GetCasterClass())
	{
		case 'I': {
			max_mana = (((GetINT()/5)+2) * GetLevel()) + spellbonuses->Mana + itembonuses->Mana;
			break;
				  }
		case 'W': {
			max_mana = (((GetWIS()/5)+2) * GetLevel()) + spellbonuses->Mana + itembonuses->Mana;
			break;
				  }
		case 'N': {
			max_mana = 0;
			break;
		}
		default: {
			cerr << "Invalid Class in CalcMaxMana" << endl;
			max_mana = 0;
			break;
		}
	}
	if (cur_mana > max_mana) {
		cur_mana = max_mana;
//		SendManaUpdatePacket();
	}
	return max_mana;
}


void Client::UpdateAdmin(bool iFromDB) {
	sint16 tmp = admin;
	if (iFromDB)
		admin = database.CheckStatus(account_id);
	if (tmp == admin && iFromDB)
		return;
	UpdateWho();
	
	if (admin >= 50) {
		if(pp.gm == 1) {
			SendAppearancePacket(20, 1, false);
			petition_list.UpdateGMQueue();
		}
	}
	else {
		pp.gm = 0;
		SendAppearancePacket(20, 0);
	}
}

int32 Client::NukeItem(int16 itemnum) {
	if (itemnum == 0)
		return 0;
	int i;
	int32 x = 0;
	for (i=0; i<=29; i++) {
		if (GetItemAt(i) == itemnum || (itemnum == 0xFFFE && GetItemAt(i) != 0xFFFF)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	for (i=250; i <= 339; i++) { // Main inventory's and cursor's containers
		if (GetItemAt(i) == itemnum || (itemnum == 0xFFFE && GetItemAt(i) != 0xFFFF)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	for (i=2000; i <= 2007; i++) { // Bank slots
		if (GetItemAt(i) == itemnum || (itemnum == 0xFFFE && GetItemAt(i) != 0xFFFF)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	for (i=2030; i <= 2109; i++) { // Bank's containers
		if (GetItemAt(i) == itemnum || (itemnum == 0xFFFE && GetItemAt(i) != 0xFFFF)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	return x;
}

void Client::FindItem(const char* search_criteria) {
	const Item_Struct* item = 0;
	if (Seperator::IsNumber(search_criteria)) {
		item = database.GetItem(atoi(search_criteria));
		if (item)
			Message(0, "  %i: %s", (int) item->item_nr, item->name);
		else
			Message(0, "Item #%s not found", search_criteria);
		return;
	}
#ifdef SHAREMEM
	int count=0;
	//int iSearchLen = strlen(search_criteria)+1;
	char sName[64];
	char sCriteria[255];
	
	strn0cpy(sCriteria, search_criteria, sizeof(sCriteria));
	strupr(sCriteria);
	char* pdest;
	int32 it = 0;
	while ((item = database.IterateItems(&it))) {
		strn0cpy(sName, item->name, sizeof(sName));
		strupr(sName);
		
		pdest = strstr(sName, sCriteria);
		if (pdest != NULL) {
			Message(0, "  %i: %s", (int) item->item_nr, item->name);
			count++;
		}
		if (count == 20) {
			break;
		}
	}
	if (count == 20)
		Message(0, "20 items shown...too many results.");
	else
		Message(0, "%i items found", count);
#endif
}

void Client::FindNPCType(const char* search_criteria) {
	const NPCType* npct = 0;
	if (Seperator::IsNumber(search_criteria)) {
		npct = database.GetNPCType(atoi(search_criteria));
		if (npct)
			Message(0, "  %i: %s", (int) npct->npc_id, npct->name);
		else
			Message(0, "NPCType #%s not found", search_criteria);
		return;
	}
	int count=0;
	//int iSearchLen = strlen(search_criteria)+1;
	char sName[64];
	char sCriteria[255];
	
	strn0cpy(sCriteria, search_criteria,sizeof(sCriteria));
	strupr(sCriteria);
	char* pdest;
	for (int32 i=0; i < database.GetMaxNPCType(); i++) {
		npct = database.GetNPCType(i);
		if (npct) {
			strncpy(sName, npct->name,sizeof(sName));
			strupr(sName);
			
			pdest = strstr(sName, sCriteria);

			if (pdest != NULL) {
				Message(0, "  %i: %s", (int) npct->npc_id, npct->name);
				count++;
			}
			if (count == 20) {
				break;
			}
		}
	}
	if (count == 20)
		Message(0, "20 npctypes shown...too many results.");
	else
		Message(0, "%i npctypes found", count);
}

void Client::ViewNPCType(uint32 npctypeid) {
	const NPCType* npct = database.GetNPCType(npctypeid);
	if (npct) {
		Message(0, " NPCType Info, ");

		Message(0, "  NPCTypeID: %u", npct->npc_id);
		Message(0, "  Name: %s", npct->name);
		Message(0, "  Level: %i", npct->level);
		Message(0, "  Race: %i", npct->race);
		Message(0, "  Class: %i", npct->class_);
		Message(0, "  MinDmg: %i", npct->min_dmg);
		Message(0, "  MaxDmg: %i", npct->max_dmg);
		Message(0, "  Attacks: %s", npct->npc_attacks);
		Message(0, "  Spells: %i", npct->npc_spells_id);
		Message(0, "  Loot Table: %i", npct->loottable_id);
		Message(0, "  NPCFactionID: %i", npct->npc_faction_id);
	}
	else
		Message(0, "NPC #%s not found", npctypeid);
	return;
}

bool Client::HasItemInInventory(int16 item_id)
{
	int16 i = 0;
	bool foundlore = false;
	for (i=0; i<pp_containerinv_size; i++) {
		if (i < 30 && pp.inventory[i] == item_id)
			foundlore = 1;
		if (i < pp_containerinv_size && pp.containerinv[i] == item_id)
			foundlore = 1;
	}
return foundlore;
}
/* returns -1 if not found or the slot it was found in */
uint32 Client::HasItemInInventory(int16 item_id, uint16 quantity)
{
        int16 slotid = 0;
        uint32 foundlore = -1;
		while(slotid<=330) {
		  if (slotid >= 0 && slotid <= 29){ // Worn items and main inventory
		      //inventory[30]
		      if (pp.inventory[slotid]== item_id && pp.invitemproperties[slotid].charges >= quantity) {
		              foundlore = slotid;
		              break;
              }
		  }
		  else if (slotid == 30){
		      slotid = 250;
		      continue;
		  }
		  else if (slotid >= 250 && slotid <= 329){ // Main inventory's containers
              // containerinv[80]
		      if (pp.containerinv[slotid-250] == item_id && pp.bagitemproperties[slotid-250].charges >= quantity) {
		              foundlore = slotid;
		              break;
              }
		  }
		  else if (slotid >= 330){
		      break;
		  }
		  slotid++;
        }
		return foundlore;
}

bool Client::CheckLoreConflict(const Item_Struct* item) {
	if (!item)

		return false;
	if (!item->IsLore())
		return false;
	int32 slotid;
	for (slotid = 0;slotid < 30;slotid++){
		if (pp.inventory[slotid] == item->item_nr)
			return true;
	}
	for (slotid = 0;slotid < pp_containerinv_size;slotid++){
		if (pp.containerinv[slotid] == item->item_nr)

			return true;
	}
	for (slotid = 0;slotid < 7;slotid++){
		if (pp.bank_inv[slotid] == item->item_nr)
			return true;
	}
	for (slotid = 0;slotid < 80;slotid++){
		if (pp.bank_cont_inv[slotid] == item->item_nr)
			return true;		
	}
	return false;
}

void Client::SummonItem(uint16 item_id, int8 charges) {
	const Item_Struct* item = database.GetItem(item_id);
	if (item == 0) {
		Message(0, "No such item: %i", item_id);
	}
	else if(pp.inventory[0] != 0xFFFF && pp.inventory[0] != 0)
	{
		Message(0, "You have an item on your cursor it must be removed.");
		return;
	}
	else {
		// Checking to see if the Item is lore or not.
		int8 foundlore = 0;
		int8 foundgm = 0;
		// Checking to see if it is a GM only Item or not.
		if (item->IsGM() && this->Admin() < 100)
			foundgm = 1;
		if (strcasecmp(item->lore,item->name)) {
			int8 i = 0;
			// There are 90 slots in containerinv, so we need 0 - 89.
			for (i=0; i<pp_containerinv_size; i++) {
				if (i < 30 && pp.inventory[i] == item_id)
					foundlore = 1;
				if (i < pp_containerinv_size && pp.containerinv[i] == item_id)
					foundlore = 1;
			}
		}
		if (!foundlore && !foundgm) { // Okay, It isn't LORE, or if it is, it is not in player's inventory.
			pp.inventory[0] = item_id;
			if (charges == 0){
				if (item->common.effecttype == 0)
					charges = 1;
				else
					charges = (item->common.charges == 0) ? 1 : charges = item->common.charges;
			}
#if DEBUG >= 11
			LogFile->write(EQEMuLog::Debug,"Summon item request from %s, effect:%i", GetName(), item->common.effecttype);
#endif
			pp.invitemproperties[0].charges = charges;
			APPLAYER* outapp = new APPLAYER(OP_SummonedItem, sizeof(SummonedItem_Struct));
			memcpy(outapp->pBuffer, item, sizeof(Item_Struct));
			Item_Struct* item2 = (Item_Struct*) outapp->pBuffer;
			item2->equipSlot = 0;
			if (item->IsNormal())
				item2->common.charges = charges;
			// This is so GM+ can trade anything to anybody...:)
			// (Marking everything Droppable
			if (this->Admin() >= 100)
				item2->nodrop = 1;
			QueuePacket(outapp);
			delete outapp;
		}
		else { // Item was already in inventory & is a LORE item or was a GM only item.  Give them a message about it.
			if (foundlore)
				Message(0, "You already have a %s (%i) in your inventory!", item->name, item_id);
			else if (foundgm)
				Message(0, "You are not a GM to summon this item");
		}
	}
}

const sint32& Client::SetMana(sint32 amount) {
	Mob::SetMana(amount);
	SendManaUpdatePacket();
	return cur_mana;
}

void Client::SendManaUpdatePacket() {
	if (!Connected())
		return;
	APPLAYER* outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
	ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
	manachange->new_mana = cur_mana;
	manachange->spell_id = 0;
	QueuePacket(outapp);
	delete outapp;
}

void Client::FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho)
{

	Mob::FillSpawnStruct(ns, ForWho);
	
	if (guildeqid == 0xFFFFFFFF) {
		ns->spawn.GuildID = 0xFFFF;
		ns->spawn.guildrank = -1;
	}
	else {
		ns->spawn.GuildID = guildeqid;
		if (guilds[guildeqid].rank[guildrank].warpeace || guilds[guildeqid].leader == account_id)
			ns->spawn.guildrank = 2;
		else if (guilds[guildeqid].rank[guildrank].invite || guilds[guildeqid].rank[guildrank].remove || guilds[guildeqid].rank[guildrank].motd)
			ns->spawn.guildrank = 1;
		else
			ns->spawn.guildrank = 0;
	}
	if (ForWho == this)
		ns->spawn.NPC = 10;
	else
		ns->spawn.NPC = 0;
	//	ns->spawn.not_linkdead = 1;
	
	if (GetGM())
		ns->spawn.GM = 1;
	if (GetPVP())
		ns->spawn.pvp = 1;
	ns->spawn.anon = pp.anon;
	if (IsBecomeNPC() == true)
		ns->spawn.NPC=true;
	ns->spawn.npc_armor_graphic = texture;
	ns->spawn.npc_helm_graphic = helmtexture;
	
	const Item_Struct* item = 0;
	item = database.GetItem(pp.inventory[2]);
	if (item != 0) {
		ns->spawn.equipment[0] = item->common.material;
		ns->spawn.equipcolors[0] = item->common.color;
	}
	item = database.GetItem(pp.inventory[17]);
	if (item != 0) {
		ns->spawn.equipment[1] = item->common.material;
		ns->spawn.equipcolors[1] = item->common.color;
	}
	item = database.GetItem(pp.inventory[7]);
	if (item != 0) {
		ns->spawn.equipment[2] = item->common.material;
		ns->spawn.equipcolors[2] = item->common.color;
	}
	item = database.GetItem(pp.inventory[10]);
	if (item != 0) {
		ns->spawn.equipment[3] = item->common.material;
		ns->spawn.equipcolors[3] = item->common.color;
	}


	item = database.GetItem(pp.inventory[12]);
	if (item != 0) {
		ns->spawn.equipment[4] = item->common.material;
		ns->spawn.equipcolors[4] = item->common.color;
	}
	item = database.GetItem(pp.inventory[18]);
	if (item != 0) {
		ns->spawn.equipment[5] = item->common.material;
		ns->spawn.equipcolors[5] = item->common.color;
	}
	item = database.GetItem(pp.inventory[19]);
	if (item != 0) {
		ns->spawn.equipment[6] = item->common.material;
		ns->spawn.equipcolors[6] = item->common.color;
	}
	
	item = database.GetItem(pp.inventory[13]);
	if (item != 0) {
		if (strlen(item->idfile) >= 3) {
			ns->spawn.equipment[7] = (int8) atoi(&item->idfile[2]);
		}
		else {
			ns->spawn.equipment[7] = 0;
		}
		ns->spawn.equipcolors[7] = 0;
	}
	item = database.GetItem(pp.inventory[14]);
	if (item != 0) {
		if (strlen(item->idfile) >= 3) {
			ns->spawn.equipment[8] = (int8) atoi(&item->idfile[2]);
		}
		else {
			ns->spawn.equipment[8] = 0;
		}
		ns->spawn.equipcolors[8] = 0;
	}
	ns->spawn.haircolor = pp.haircolor;
	ns->spawn.beardcolor = pp.beardcolor;
	ns->spawn.eyecolor1 = pp.eyecolor1;
	ns->spawn.eyecolor2 = pp.eyecolor2;
	ns->spawn.hairstyle = pp.hairstyle;
	ns->spawn.title = pp.title;
	ns->spawn.luclinface = pp.luclinface;
}

int16 Client::GetItemAt(int16 in_slot) {
	if (in_slot <= 29) // Worn items and main inventory
		return pp.inventory[in_slot];
	else if (in_slot >= 250 && in_slot <= 339) // Main inventory's and cursor's containers
		return pp.containerinv[in_slot-250];
	else if (in_slot >= 2000 && in_slot <= 2007) // Bank slots
		return pp.bank_inv[in_slot-2000];
	else if (in_slot >= 2030 && in_slot <= 2109) // Bank's containers
		return pp.bank_cont_inv[in_slot-2030];
	else if (in_slot == 0xFFFF){
	    return 0xFFFF;
	}
	else {
//		cerr << "Error: " << GetName() << ": GetItemAt(): Unknown slot: 0x" << hex << setw(4) << setfill('0') << in_slot << dec << endl;
		Message(0, "Error: GetItemAt(): Unknown slot: 0x%04x", in_slot);
        LogFile->write(EQEMuLog::Error, "GetItemAt(%i) unknown slot.", in_slot);
	}

	return 0;
}
ItemProperties_Struct Client::GetItemPropAt(int16 in_slot) {
	if (in_slot <= 29) // Worn items and main inventory
		return pp.invitemproperties[in_slot];
	else if (in_slot >= 250 && in_slot <= 339) // Main inventory's and cursor's containers
		return pp.bagitemproperties[in_slot-250];
	else if (in_slot >= 2000 && in_slot <= 2007) // Bank slots
		return pp.bankinvitemproperties[in_slot-2000];
	else if (in_slot >= 2030 && in_slot <= 2109) // Bank's containers
		return pp.bankbagitemproperties[in_slot-2030];
	else if (in_slot != 0xFFFF) {
		cerr << "Error: " << GetName() << ": GetItemAt(): Unknown slot: 0x" << hex << setw(4) << setfill('0') << in_slot << dec << endl;
		Message(0, "Error: GetItemAt(): Unknown slot: 0x%04x", in_slot);
	}
	ItemProperties_Struct empty_;
	memset(&empty_, 0, sizeof(ItemProperties_Struct));
	return empty_;
}


bool Client::DeleteItemInInventory(uint32 slotid, int16 quantity, bool clientupdate) {
#if (DEBUG >= 5)
	LogFile->write(EQEMuLog::Debug, "DeleteItemInInventory(%i, %i, %s)", slotid, quantity, (clientupdate) ? "true":"false");

#endif
	int8 new_quan = 0;
	const Item_Struct* item = 0;
	if (quantity == 0) { // No quantity given invalidate the item.
		if (slotid <= 29){ // Worn items and main inventory
			pp.inventory[slotid] = 0xFFFF;
			pp.invitemproperties[slotid].charges = 0;
		}
		else if (slotid >= 250 && slotid <= 339){ // Main inventory's and cursor's containers
			pp.containerinv[slotid-250] = 0xFFFF;

			pp.bagitemproperties[slotid].charges = 0;
		}
		else if (slotid >= 2000 && slotid <= 2007){ // Bank slots
			pp.bank_inv[slotid-2000] = 0xFFFF;
			pp.bankinvitemproperties[slotid].charges = 0;
		}
		else if (slotid >= 2030 && slotid <= 2109){ // Bank's containers
			pp.bank_cont_inv[slotid-2030] =0xFFFF;
			pp.bankbagitemproperties[slotid].charges = 0;
		}

		else {
			LogFile->write(EQEMuLog::Error,"DeleteItemInInventory() unknown slot:%i ", slotid);
			return false;
		}
	}
	else { // Quantity given
		if (slotid >= 0 && slotid <= 29){ // Worn items and main inventory

			// inventory[30]
			item = database.GetItem(pp.inventory[slotid]);
		     if ( !item || !item->IsStackable() )  {
			    if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "Delete item: non stackable item");
			        pp.inventory[slotid] = 0xFFFF;
			        pp.invitemproperties[slotid].charges = 0;
             } else {
			   if ((pp.invitemproperties[slotid].charges - quantity) <= 0) {
			       if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "Delete item: stackable item expended");
			       pp.inventory[slotid] = 0xFFFF;
			       pp.invitemproperties[slotid].charges = 0;
			   } else {
			       if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "Delete item: stackable item decreasing charges");
			       pp.invitemproperties[slotid].charges -= quantity;
			       new_quan = pp.invitemproperties[slotid].charges;
			   }
             }

		}
		else if (slotid >= 250 && slotid <= 329){ // Main inventory's containers
		// containerinv[80]
			item = database.GetItem(pp.containerinv[slotid-250]);
			if ( !item || !item->IsStackable() || (pp.bagitemproperties[slotid-250].charges - quantity) <= 0 ) {
				pp.containerinv[slotid-250] = 0xFFFF;
				pp.bagitemproperties[slotid-250].charges = 0;
			} else {
				pp.bagitemproperties[slotid-250].charges -= quantity;
				new_quan = pp.bagitemproperties[slotid-250].charges;

			}
		}
		else if (slotid >= 2000 && slotid <= 2007){ // Bank slots
			item = database.GetItem(pp.bank_inv[slotid-2000]);
			if ( !item || !item->IsStackable() || (pp.bankinvitemproperties[slotid-2000].charges - quantity) <= 0)  {
				// bank_inv[8]
				pp.bank_inv[slotid-2000] = 0xFFFF;
				pp.bankinvitemproperties[slotid-2000].charges = 0;
			} else {
				pp.bankinvitemproperties[slotid-2000].charges -= quantity;
				new_quan = pp.bankinvitemproperties[slotid-2000].charges;

			}
		}
		else if (slotid >= 2030 && slotid <= 2109){ // Bank's containers
		// bank_cont_inv[80]
			item = database.GetItem(pp.bank_cont_inv[slotid-2030]);
			if (!item || !item->IsStackable() || (pp.bankbagitemproperties[slotid-2030].charges - quantity) <= 0) {
				pp.bank_cont_inv[slotid-2030] = 0xFFFF;
				pp.bankbagitemproperties[slotid-2030].charges = 0;
			} else {
				pp.bankbagitemproperties[slotid-2030].charges -= quantity;
				new_quan = pp.bankbagitemproperties[slotid-2030].charges;

			}
		}
		else {
			LogFile->write(EQEMuLog::Error,"DeleteItemInInventory() unknown slot:%i ", slotid);
			return false;
		}
	}
	if (clientupdate && item && new_quan){
		APPLAYER* outapp = new APPLAYER(OP_MoveItem, sizeof(MoveItem_Struct));
		MoveItem_Struct* delitem = (MoveItem_Struct*)outapp->pBuffer;
		delitem->from_slot = slotid;
		delitem->to_slot = 0xFFFFFFFF;
		delitem->number_in_stack = 1;
		//DumpPacket(outapp);
		QueuePacket(outapp);
		delete outapp;
		//PutItemInInventory(int16 slotid, int16 itemid, int8 charges)
		PutItemInInventory(slotid, item->item_nr, new_quan);
		
	}
	else if (clientupdate){

		APPLAYER* outapp = new APPLAYER(OP_MoveItem, sizeof(MoveItem_Struct));
		MoveItem_Struct* delitem = (MoveItem_Struct*)outapp->pBuffer;
		delitem->from_slot = slotid;
		delitem->to_slot = 0xFFFFFFFF;
		delitem->number_in_stack = 1;
		//DumpPacket(outapp);
		//	DumpPacket(outapp);
		QueuePacket(outapp);
		delete outapp;
	}
	Save();
	return true;
}


bool Client::GMHideMe(Client* client) {
	if (gmhideme) {
		if (client == 0)
			return true;
		else if (admin > client->Admin())
			return true;
		else
			return false;
	}
	else
		return false;
	
}

void Client::Duck() {
	SetAppearance(2, false);
}

void Client::Stand() {
	SetAppearance(0, false);
}

int16 Client::FindFreeInventorySlot(int16** pointer, bool ForBag, bool TryCursor) {
	int i;
	for (i=22; i<=29; i++) {
		if (pp.inventory[i] == 0xFFFF) {
			if (pointer != 0)
				*pointer = &pp.inventory[i];
			return i;
		}
	}
	if (!ForBag) {
		const Item_Struct* item;
		for (i=22; i<=29; i++) {
			item = database.GetItem(pp.inventory[i]);
			if (item != 0 && item->type == 0x01) { // make sure it's a container
				for (int k=0; k<10; k++) {
					if (pp.containerinv[((i-22)*10) + k] == 0xFFFF) {
						if (pointer != 0)
							*pointer = &pp.containerinv[((i-22)*10) + k];
						return 250 + ((i-22)*10) + k;
					}
				}
			}
		}
	}
	if (TryCursor) {
		if (pp.inventory[0] == 0xFFFF) {
			*pointer = &pp.inventory[0];
			return 0;
		}
	}
	return 0xFFFF;
}

bool Client::PutItemInInventory(int16 slotid, int16 itemid, int8 charges) {
	const Item_Struct* item = database.GetItem(itemid);
	if (item == 0)
		return false;
	else
		return PutItemInInventory(slotid, item, charges);
}

bool Client::PutItemInInventory(int16 slotid, const Item_Struct* item)
{
	if (item->common.effecttype != 0)		
		return PutItemInInventory(slotid, item, item->common.charges);
	else
		return PutItemInInventory(slotid, item, 1);
}

bool Client::PutItemInInventory(int16 slotid, const Item_Struct* item, int8 charges, bool update) {
	if(item->common.GMFlag == -1)
		charges = 255;
	if (slotid <= 29){
		pp.inventory[slotid] = item->item_nr;
		pp.invitemproperties[slotid].charges = charges;
	}
	else if (slotid >= 250 && slotid <= 339){
		pp.containerinv[slotid-250] = item->item_nr;
		pp.bagitemproperties[slotid-250].charges = charges;
	}

	else if (slotid >= 2000 && slotid <= 2007){
		pp.bank_inv[slotid-2000] = item->item_nr;
		pp.bankinvitemproperties[slotid-2000].charges = charges;
	}
	else if (slotid >= 2030 && slotid <= 2109){
		pp.bank_cont_inv[slotid-2030] = item->item_nr;
		pp.bankbagitemproperties[slotid-2030].charges = charges;
	}
	else
		return false;
//	if (update) {
		APPLAYER* outapp = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));
		memcpy(outapp->pBuffer, item, outapp->size);
		Item_Struct* outitem = (Item_Struct*) outapp->pBuffer;
		outitem->equipSlot = slotid;
		//if (item->flag != 0x7669)
		outitem->common.charges = charges;
		QueuePacket(outapp);
		delete outapp;
//  }
	return true;
}

// Puts an item into the person's inventory
// Does NOT check if an item is already there
bool Client::AutoPutItemInInventory(const Item_Struct* item, int8* charges, bool TryCursor, ServerLootItem_Struct** bag_item_data, bool TryWornSlots) {
	int i, k;
	if (item->IsEquipable(GetRace(),GetClass()) && TryWornSlots) {
	if (GetGM()) Message(0,"Autoequip");
		for (i=1; i<22; i++) {
			if (pp.inventory[i] == 0xFFFF && (item->equipableSlots & pow32(2, i))) {
				if (i == 14 && (item->IsWeapon() && !CanThisClassDuelWield()) )
				    continue;
				pp.inventory[i] = item->item_nr;
				pp.invitemproperties[i].charges = *charges;
				SendLootItemInPacket(item, i, pp.invitemproperties[i].charges);
				return true;
			}
		}
	}
	for (i=22; i<=29; i++) {
		if (item->type == 0 && item->IsStackable() && pp.inventory[i] == item->item_nr && pp.invitemproperties[i].charges < ITEM_MAX_STACK) {
			pp.inventory[i] = item->item_nr;
			int8 usedcharges = *charges;
			if (usedcharges > (ITEM_MAX_STACK - pp.invitemproperties[i].charges))
				usedcharges = ITEM_MAX_STACK - pp.invitemproperties[i].charges;
			pp.invitemproperties[i].charges += usedcharges;
			*charges -= usedcharges;
			SendLootItemInPacket(item, i, pp.invitemproperties[i].charges);
			if (!(*charges))
				return true;
		}
	}
	if (!item->IsBag()) {
		const Item_Struct* item2;
		for (i=22; i<=29; i++) {
			item2 = database.GetItem(pp.inventory[i]);
			if (item2 != 0 && item2->IsBag()) { // make sure it's a container
				if (item2->container.numSlots < 2 || item2->container.numSlots > 10) {
					Message(0, "Error: WTF: item2->container.numSlots out of range");
					return false;
				}
				for (k=0; k<item2->container.numSlots; k++) {
					int16 tmpIndex = ((i-22)*10) + k;
					if (item->type == 0 && item->IsStackable() && pp.containerinv[tmpIndex] == item->item_nr && pp.bagitemproperties[tmpIndex].charges < ITEM_MAX_STACK) {
						pp.containerinv[tmpIndex] = item->item_nr;
						int8 usedcharges = *charges;
						if (usedcharges > (ITEM_MAX_STACK - pp.bagitemproperties[tmpIndex].charges))
							usedcharges = ITEM_MAX_STACK - pp.bagitemproperties[tmpIndex].charges;
						pp.bagitemproperties[tmpIndex].charges += usedcharges;
						*charges -= usedcharges;
						SendLootItemInPacket(item, tmpIndex + 250, pp.bagitemproperties[tmpIndex].charges);
						if (!(*charges))
							return true;
					}
				}
			}
		}
	}
	for (i=22; i<=29; i++) {
		if (pp.inventory[i] == 0xFFFF) {
			pp.inventory[i] = item->item_nr;
			pp.invitemproperties[i].charges = *charges;
			SendLootItemInPacket(item, i, pp.invitemproperties[i].charges);
			if (item->IsBag() && bag_item_data) {
				int16 bagstart = ((i-22)*10);
				for (int j=0; j<10; j++) {
					if (bag_item_data[j]) {
						pp.containerinv[bagstart + j] = bag_item_data[j]->item_nr;
						pp.bagitemproperties[bagstart + j].charges = bag_item_data[j]->charges;
						const Item_Struct* bagitem = database.GetItem(bag_item_data[j]->item_nr);
						SendLootItemInPacket(bagitem, bagstart + j + 250, bag_item_data[j]->charges);
						bag_item_data[j]->looted = true;
					}
					else
						pp.containerinv[bagstart + j] = 0xFFFF;
				}
			}
			return true;
		}
	}
	if (!item->IsBag()) {
		const Item_Struct* item2;
		for (i=22; i<=29; i++) {
			item2 = database.GetItem(pp.inventory[i]);
			if (item2 != 0 && item2->IsBag()) { // make sure it's a container
				if (item2->container.numSlots < 2 || item2->container.numSlots > 10) {
					Message(0, "Error: WTF: item2->container.numSlots out of range");
					return false;
				}
				for (k=0; k<item2->container.numSlots; k++) {
					int16 tmpIndex = ((i-22)*10) + k;
					if (pp.containerinv[tmpIndex] == 0xFFFF) {
						pp.containerinv[tmpIndex] = item->item_nr;
						pp.bagitemproperties[tmpIndex].charges = *charges;
						SendLootItemInPacket(item, tmpIndex + 250, pp.bagitemproperties[tmpIndex].charges);
						return true;
					}
				}
			}
		}
	}
	if (TryCursor) {
		if (pp.inventory[0] == 0xFFFF) {
			pp.inventory[0] = item->item_nr;
			pp.invitemproperties[0].charges = *charges;
			SendLootItemInPacket(item, 0, *charges);
			if (item->IsBag() && bag_item_data) {
				int16 bagstart = 80;
				for (int j=0; j<10; j++) {
					if (bag_item_data[j]) {
						pp.containerinv[bagstart + j] = bag_item_data[j]->item_nr;
						pp.bagitemproperties[bagstart + j].charges = bag_item_data[j]->charges;
						const Item_Struct* bagitem = database.GetItem(bag_item_data[j]->item_nr);
						SendLootItemInPacket(bagitem, bagstart + j + 250, bag_item_data[j]->charges);
						bag_item_data[j]->looted = true;
					}
					else
						pp.containerinv[bagstart + j] = 0xFFFF;
				}
			}
			return true;
		}
	}
	return false;
}

void Client::SendLootItemInPacket(const Item_Struct* item, int16 slotid, int8 charges) {
	APPLAYER* outapp = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));
	memcpy(outapp->pBuffer, item, outapp->size);
	Item_Struct* outitem = (Item_Struct*) outapp->pBuffer;
	outitem->equipSlot = slotid;
	outitem->common.charges = charges;
	QueuePacket(outapp);
	delete outapp;

}

// works for stacks too, as long as they are not 0(1), use deleteitem then
bool Client::DecreaseCharges(int16 slotid) { // does not check the slot, be sure that there IS an expandable item in this slot
	int8 newcharges;
	int16 itemid = 0;
	if (slotid <= 29){
		itemid = pp.inventory[slotid];
		if (pp.invitemproperties[slotid].charges > 0 && pp.invitemproperties[slotid].charges != 255){
			pp.invitemproperties[slotid].charges--;

			newcharges = pp.invitemproperties[slotid].charges;
		}
		else if(pp.invitemproperties[slotid].charges == 0)
			newcharges = 0;
		else
		{
			pp.invitemproperties[slotid].charges = 255;
			newcharges = 255;
		}
	}
	else if (slotid >= 250 && slotid <= 339){
		itemid = pp.containerinv[slotid-250];
		if (pp.bagitemproperties[slotid].charges > 0 && pp.invitemproperties[slotid].charges != 255){
			pp.bagitemproperties[slotid].charges--;
			newcharges = pp.bagitemproperties[slotid].charges;
		}
		else if(pp.bagitemproperties[slotid].charges == 0)
			newcharges = 0;
		else
		{
			pp.bagitemproperties[slotid].charges = 255;
			newcharges = 255;
		}
	}
	else if (slotid >= 2000 && slotid <= 2007){
		itemid = pp.bank_inv[slotid-2000];
		if (pp.bankinvitemproperties[slotid].charges > 0 && pp.invitemproperties[slotid].charges != 255){
			pp.bankinvitemproperties[slotid].charges--;
			newcharges =pp.bankinvitemproperties[slotid].charges;
		}
		else if(pp.bankinvitemproperties[slotid].charges == 0)
			newcharges = 0;
		else
		{
			pp.bankinvitemproperties[slotid].charges = 255;
			newcharges = 255;
		}
	}
	else if (slotid >= 2030 && slotid <= 2109){
		itemid = pp.bank_cont_inv[slotid-2030];
		if (pp.bankbagitemproperties[slotid].charges > 0 && pp.invitemproperties[slotid].charges != 255){
			pp.bankbagitemproperties[slotid].charges--;
			newcharges = pp.bankbagitemproperties[slotid].charges;
		}
		else if(pp.bankbagitemproperties[slotid].charges == 0)
			newcharges = 0;
		else {
			pp.bankbagitemproperties[slotid].charges = 255;
			newcharges = 255;
		}
	}
	else
		newcharges = 0;

	if (itemid == 0) {
		LogFile->write(EQEMuLog::Error,"Uncaught item case. ITEMID set to 0");
	}

	const Item_Struct* item = database.GetItem(itemid);
	if (newcharges == 0 && item && item->IsExpendable())
		DeleteItemInInventory(slotid, 0, true);
	else if (item) {
		APPLAYER* outapp = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));

		memcpy(outapp->pBuffer, item, outapp->size);
		Item_Struct* outitem = (Item_Struct*) outapp->pBuffer;
		outitem->equipSlot = slotid;
		if(outitem->common.charges == 255)
			outitem->common.charges = 255;
		else if(newcharges > outitem->common.charges)
			outitem->common.charges = 0;
		else
			outitem->common.charges = newcharges;

		QueuePacket(outapp);
		delete outapp;
	}
	return true;
}

void Client::ChangeLastName(const char* in_lastname) {
	memset(pp.last_name, 0, sizeof(pp.last_name));
	if (strlen(in_lastname) >= sizeof(pp.last_name))
		strncpy(pp.last_name, in_lastname, sizeof(pp.last_name) - 1);
	else
		strcpy(pp.last_name, in_lastname);
	
	// Send name update packet here... once know what it is
}


void Client::ChangeFirstName(const char* oldname,const char* in_firstname,const char* gmname) {
	APPLAYER* outapp = new APPLAYER(OP_GMNameChange, sizeof(GMName_Struct));
	GMName_Struct* gmn=(GMName_Struct*)outapp->pBuffer;
	strncpy(gmn->gmname,gmname,64);

	strncpy(gmn->oldname,oldname,64);	
	strncpy(gmn->newname,in_firstname,64);
	
	bool usedname = database.CheckUsedName((const char*) in_firstname);
	if (!usedname) {
		return;
	}
	
	database.UpdateName(oldname, in_firstname);
	Save();
	
	memset(pp.name, 0, sizeof(pp.name));
	snprintf(pp.name, sizeof(pp.name), "%s", in_firstname);
	strcpy(name, pp.name);
	
	gmn->unknown[0] = 1;
	gmn->unknown[1] = 1;
	gmn->unknown[2] = 1;
	entity_list.QueueClients(this, outapp, false);

	delete outapp;

	UpdateWho();
}

void Client::SetGM(bool toggle) {
	if(toggle) {
		pp.gm=1;
		SendAppearancePacket(20, 1);
		Save();
		Message(13, "You are now a GM.");
	}
	else {
		pp.gm=0;
		SendAppearancePacket(20, 0);
		Save();
		Message(13, "You are no longer a GM.");
	}
	UpdateWho();
}

void Client::ReadBook(char txtfile[14]) {
	char* booktxt = database.GetBook(txtfile);
	if (booktxt != 0) {
		
		char* blah = new char[5000];
		blah[0]=0;
		int i=0,lenbook=strlen(booktxt);
		for (char* p=booktxt;p<booktxt+lenbook; p++)  {
			i++;
			blah[i]=*p;
		}
		//booktxt[sizeof(booktxt)]=0;
		//memcpy(booktxt,char(0)+booktxt,strlen(booktxt)+1);
		LogFile->write(EQEMuLog::Normal,"Client::ReadBook() textfile:%s Text:%s", txtfile, booktxt);
		APPLAYER* outapp = new APPLAYER(0xce40);
		outapp->size = strlen(booktxt)+2;
		outapp->pBuffer = (uchar*) blah;//(uchar*) "\0" + (uchar*) booktxt;
		QueuePacket(outapp);
		delete outapp;
	}
}
void Client::SendClientMoneyUpdate(int8 type,int32 amount){
	APPLAYER* outapp = new APPLAYER(0x3d41, 8);
	outapp->pBuffer[0] = 0;
	outapp->pBuffer[1] = 0;
	outapp->pBuffer[2] = type;
	outapp->pBuffer[3] = 0;
	outapp->pBuffer[4] = amount;
	QueuePacket(outapp);
	delete outapp;
}


bool Client::TakeMoneyFromPP(uint32 copper){
	sint32 copperpp,silver,gold,platinum;
	copperpp = pp.copper;
	silver = pp.silver*10;
	gold = pp.gold*100;
	platinum = pp.platinum*1000;

	sint32 clienttotal = pp.copper+pp.silver*10+pp.gold*100+pp.platinum*1000;
	clienttotal -= copper;
	if(clienttotal < 0)
	{
	return false; // Not enough money!	
	}
	else
	{
	copperpp -= copper;
	if(copperpp <= 0)
	{
	copper = abs(copperpp);
	pp.copper = 0;
	}
	else
	{
	pp.copper = copperpp;
	Save();
	return true;
	}
	silver -= copper;
	if(silver <= 0)
	{
	copper = abs(silver);
	pp.silver = 0;
	}
	else
	{
	pp.silver = silver/10;
	pp.copper += (silver-(pp.silver*10));
	Save();
	return true;
	}

	gold -=copper;

	if(gold <= 0)
	{
	copper = abs(gold);
	pp.gold = 0;
	}
	else
	{
	pp.gold = gold/100;
	int32 silvertest = (gold-(pp.gold*100))/10;
	pp.silver += silvertest;
	int32 coppertest = (gold-(pp.gold*100+silvertest*10));
	pp.copper += coppertest;
	Save();
	return true;
	}

	platinum -= copper;

	//Impossible for plat to be negative, already checked above

	pp.platinum = platinum/1000;
	int32 goldtest = (platinum-(pp.platinum*1000))/100;
	pp.gold += goldtest;
	int32 silvertest = (platinum-(pp.platinum*1000+goldtest*100))/10;
	pp.silver += silvertest;
	int32 coppertest = (platinum-(pp.platinum*1000+goldtest*100+silvertest*10));
	pp.copper = coppertest;
	Save();
	return true;
	}
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
	LogFile->write(EQEMuLog::Debug, "Client::AddMoneyToPP() %s should have:  plat:%i gold:%i silver:%i copper:%i", GetName(), pp.platinum, pp.gold, pp.silver, pp.copper);
}

void Client::AddMoneyToPP(uint32 copper, uint32 silver, uint32 gold,uint32 platinum,bool updateclient){
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
	LogFile->write(EQEMuLog::Debug, "Client::AddMoneyToPP() %s should have:  plat:%i gold:%i silver:%i copper:%i", GetName(), pp.platinum, pp.gold, pp.silver, pp.copper);
}


bool Client::CheckIncreaseSkill(int16 skillid,sint16 chancemodi) {
	if (IsAIControlled()) // no skillups while chamred =p
		return false;
	if (skillid > HIGHEST_SKILL)
		return false;
	// Make sure we're not already at skill cap
	if (GetSkill(skillid) < MaxSkill(skillid)){
		// the higher your current skill level, the harder it is
		sint16 Chance = 10 + chancemodi + ((252 - GetSkill(skillid)) / 20);
		if (Chance < 0)
			Chance = 0; // Make it always possible
		if (rand()%100 < Chance){
			SetSkill(skillid,GetSkill(skillid)+1);
			return true;
		}
	}
	return false;
}

// Made this function to check for skill increases in skills where a linear algorithm for
// skill progression is acceptable.  For the time being using (10+level*5)
bool Client::SimpleCheckIncreaseSkill(int16 skillid,sint16 chancemodi){
	if (IsAIControlled()) // no skillups while chamred =p
		return false;
	if (skillid > HIGHEST_SKILL)
		return false;
	// Make sure we're not already at skill cap


	if (GetSkill(skillid) < (10+level*5) ){
		// the higher your current skill level, the harder it is
		sint16 Chance = 10 + chancemodi + ((252 - GetSkill(skillid)) / 20);
		if (Chance < 0)
			Chance = 0; // Make it always possible
		if (rand()%100 < Chance){
			SetSkill(skillid,GetSkill(skillid)+1);
			return true;
		}
	}
	return false;
}

#include "maxskill.h"
/*
int8 Mob::MaxSkill(int16 skillid, int16 class_, int16 level) {
	switch (skillid) {
		case OFFENSE:
		case DEFENSE: {
			int16 tmp = level * 5;
			if (tmp > 200)
				tmp = 200;
			return tmp;
		}
		default: {
			return level*4;
		}
	}
}
*/

void Client::FinishTrade(Client* with) {
	const Item_Struct* item;
	//Item_Struct* outitem = 0;
	int16* toppslot;
	int16 toslot;
	int j;
	for (int i = 0;i != 7; i++){
		if (with->TradeList[i] != 0){
			item = database.GetItem(with->TradeList[i]);
			if (item) {
				toslot = FindFreeInventorySlot(&toppslot,(item->type == 1),true);
				if (toslot != 0xFFFF){
					this->PutItemInInventory(toslot,item,with->TradeCharges[i]);
					if (item->type == 1){
						for (j=0;j != 10; j++)
							this->PutItemInInventory(((toslot-22)*10) + j + 250,with->TradeList[((i+1)*10)+j],with->TradeCharges[((i+1)*10)+j]);
					}
				}
			}
		}
	}
}

void Client::RepairInventory(){
	const Item_Struct* item = 0;
	int16 slotid = 0;
	for(int i=0;i<pp_containerinv_size;i++) {
		slotid = i;

		//Repair Main Inventory
		if(i<pp_inventory_size) {
			if(pp.inventory[slotid] == 0xFFFF) {
				//Nothing in slot, dont need to do anything
			}
			else {
				if(!(item = database.GetItem(pp.inventory[slotid]))) {
					//Item no longer exists, destroy it
					if(slotid == 0) {
						//Cursor
						pp.inventory[0] = 0xFFFF;
						memset(&pp.invitemproperties[0],0,sizeof(ItemProperties_Struct));

						for(int bag=0;bag<10;bag++) {
							int32 currentslot = 80+bag;
							memset(&pp.bagitemproperties[currentslot],0,sizeof(ItemProperties_Struct));
							pp.containerinv[currentslot] = 0xFFFF;
						}
					}
					else if(slotid >= 1 && slotid <= 21) {
						//Equipped Items
						pp.inventory[slotid] = 0xFFFF;
						memset(&pp.invitemproperties[slotid],0,sizeof(ItemProperties_Struct));
					}
					else if(slotid >= 22 && slotid <= 29) {
						//Main Inventory
						pp.inventory[slotid] = 0xFFFF;
						memset(&pp.invitemproperties[slotid],0,sizeof(ItemProperties_Struct));
									
						for(int bag=0;bag<10;bag++) {
							int32 currentslot = ((slotid-22)*10)+bag;
							memset(&pp.bagitemproperties[currentslot],0,sizeof(ItemProperties_Struct));
							pp.containerinv[currentslot] = 0xFFFF;
						}
					}

				}
				else {
					//Item exists, make sure it is ok
					if((item->IsGM() || !item->nosave) && admin < 10) {
						pp.inventory[0] = 0xFFFF;
						memset(&pp.invitemproperties[0],0,sizeof(ItemProperties_Struct));

						if(item->IsBag()) {
							if(slotid == 0) {
								for(int bag=0;bag<10;bag++){
									int32 currentslot = 80+bag;
									memset(&pp.bagitemproperties[currentslot],0,sizeof(ItemProperties_Struct));
									pp.containerinv[currentslot] = 0xFFFF;
								}
							}
							else if(slotid >= 22 && slotid <= 30) {
								for(int bag=0;bag<10;bag++){
									int32 currentslot = ((slotid-22)*10)+bag;
									memset(&pp.bagitemproperties[currentslot],0,sizeof(ItemProperties_Struct));
									pp.containerinv[currentslot] = 0xFFFF;
								}
							}
						}
						else {
							if(item->common.MaxCharges==255)
								pp.invitemproperties[slotid].charges = 255;
							else if(item->common.MaxCharges==0)
								pp.invitemproperties[slotid].charges = item->common.charges;

							memset(pp.invitemproperties[slotid].unknown01,0xFF,sizeof(int8)*2);
							memset(pp.invitemproperties[slotid].unknown02,0xFF,sizeof(int8)*6);
						}
					}
				}
			}
			//End of Repairing Main Inventory


			//Repair Container Inventory
			if(i<pp_containerinv_size) {
				if(pp.containerinv[slotid] == 0xFFFF) {
					//Nothing in slot, don't do anything
				}
				else {
					if(!(item = database.GetItem(pp.containerinv[slotid]))) {
						//Item no longer exists, destroy it
						pp.containerinv[slotid] = 0xFFFF;
						memset(&pp.bagitemproperties[slotid],0,sizeof(ItemProperties_Struct));
					}
					else {
						//Item exists, make sure it is ok
						if((item->IsGM() || !item->nosave) && admin < 10) {
							pp.containerinv[slotid] = 0xFFFF;
							memset(&pp.bagitemproperties[slotid],0,sizeof(ItemProperties_Struct));
						}
						else {
							if(item->common.MaxCharges==255)
								pp.bagitemproperties[slotid].charges = 255;
							else if(item->common.MaxCharges==0)
								pp.bagitemproperties[slotid].charges = item->common.charges;

							memset(pp.bagitemproperties[slotid].unknown01,0xFF,sizeof(int8)*2);
							memset(pp.bagitemproperties[slotid].unknown02,0xFF,sizeof(int8)*6);
						}

					}
				}
			}
		}
		//End of Repairing Container Inventory
		if(i<pp_bank_inv_size) {
			if(!(item = database.GetItem(pp.bank_inv[slotid]))){
				//Item no longer exists, destroy it
				pp.bank_inv[slotid] = 0xFFFF;
				memset(&pp.bankinvitemproperties[slotid],0,sizeof(ItemProperties_Struct));
			}
			else {
				//Item exists, make sure it is ok
				if((item->IsGM() || !item->nosave) && admin < 10) {
					pp.bank_inv[slotid] = 0xFFFF;
					memset(&pp.bankinvitemproperties[slotid],0,sizeof(ItemProperties_Struct));

					for(int bag=0;bag<10;bag++){
						int32 currentslot = bag+slotid;
						memset(&pp.bankbagitemproperties[currentslot],0,sizeof(ItemProperties_Struct));
						pp.bank_cont_inv[currentslot] = 0xFFFF;
					}
				}
				else {
					if(item->common.MaxCharges==255)
						pp.bankinvitemproperties[slotid].charges = 255;
					else if(item->common.MaxCharges==0)
						pp.bankinvitemproperties[slotid].charges = item->common.charges;

					memset(pp.bankinvitemproperties[slotid].unknown01,0xFF,sizeof(int8)*2);
					memset(pp.bankinvitemproperties[slotid].unknown02,0xFF,sizeof(int8)*6);
				}
			}
		}

		if(i<pp_bank_cont_inv_size) {
			if(!(item = database.GetItem(pp.bank_cont_inv[slotid]))){
				//Item no longer exists, destroy it
				pp.bank_cont_inv[slotid] = 0xFFFF;
				memset(&pp.bankbagitemproperties[slotid],0,sizeof(ItemProperties_Struct));
			}
			else {
				//Item exists, make sure it is ok
				if((item->IsGM() || !item->nosave) && admin < 10) {
					pp.bank_cont_inv[slotid] = 0xFFFF;
					memset(&pp.bankbagitemproperties[slotid],0,sizeof(ItemProperties_Struct));
				}
				else {
					if(item->common.MaxCharges==255)
						pp.bankbagitemproperties[slotid].charges = 255;
					else if(item->common.MaxCharges==0)
						pp.bankbagitemproperties[slotid].charges = item->common.charges;

					memset(pp.bankbagitemproperties[slotid].unknown01,0xFF,sizeof(int8)*2);
					memset(pp.bankbagitemproperties[slotid].unknown02,0xFF,sizeof(int8)*6);
				}
			}
		}
	}
}

void Client::SetPVP(bool toggle) {
	if(toggle) {
		pp.pvp = 1;
		Save();
		Message(13, "You now follow the ways of discord.");
	}
	else {
		pp.pvp = 0;
		Save();
		Message(13, "You no longer follow the ways of discord.");
	}
	SendAppearancePacket(4, GetPVP());
}


bool Database::CheckGuildDoor(int8 doorid,int16 guildid,const char* zone) {
	MYSQL_ROW row;
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
    MYSQL_RES *result;
	if (!RunQuery(query, MakeAnyLenString(&query, "SELECT guild FROM doors where doorid=%i AND zone='%s'",doorid-128, zone), errbuf, &result)) {
		cerr << "Error in CheckUsedName query '" << query << "' " << errbuf << endl;
		if (query != 0)
			delete[] query;
		return false;
	}
	else { 
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (atoi(row[0]) == guildid)
			{
				mysql_free_result(result);
				return true;
			}
			else
			{
				mysql_free_result(result);
				return false;
			}
			
			mysql_free_result(result);
			return false;
		}
	}
	return false;
}


bool Database::SetGuildDoor(int8 doorid,int16 guildid, const char* zone) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	if (doorid > 127)
		doorid = doorid - 128;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE doors SET guild = %i WHERE (doorid=%i) AND (zone='%s')",guildid,doorid, zone), errbuf, 0,&affected_rows)) {
		cerr << "Error in SetGuildDoor query '" << query << "' " << errbuf << endl;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
}

void Client::WorldKick() {
	APPLAYER* outapp = new APPLAYER(OP_GMKick, sizeof(GMKick_Struct));
	GMKick_Struct* gmk = (GMKick_Struct *)outapp->pBuffer;
	strcpy(gmk->name,GetName());
	QueuePacket(outapp);
	delete outapp;
	Kick();
}

void Client::GMKill() {
	APPLAYER* outapp = new APPLAYER(OP_GMKill, sizeof(GMKill_Struct));
	GMKill_Struct* gmk = (GMKill_Struct *)outapp->pBuffer;
	strcpy(gmk->name,GetName());
	QueuePacket(outapp);
	delete outapp;
}

bool Client::CheckAccess(sint16 iDBLevel, sint16 iDefaultLevel) {
	if ((admin >= iDBLevel) || (iDBLevel == 255 && admin >= iDefaultLevel))
		return true;
	else
		return false;

}

void Client::SendAAStats() {
	APPLAYER* outapp = new APPLAYER(OP_SendAAStats, sizeof(AltAdvStats_Struct));
	AltAdvStats_Struct *aps = (AltAdvStats_Struct *)outapp->pBuffer;
	aps->experience = pp.expAA;
	aps->experience = (int32)(((float)330.0f * (float)pp.expAA) / (float)max_AAXP);
	aps->unspent = pp.aapoints;
	aps->percentage = pp.perAA;
	QueuePacket(outapp);
	delete outapp;
}

void Client::SendAATable() {
	APPLAYER* outapp = new APPLAYER(OP_RespondAA, sizeof(PlayerAA_Struct));
	memcpy(outapp->pBuffer,&aa,outapp->size);
	QueuePacket(outapp);
	delete outapp;
}

/*bool Client::ServerFilter(APPLAYER *app)
{
if(ssfs.clientattackfilters == 0 && ssfs.npcattackfilters == 0 && ssfs.clientcastfilters == 0 && ssfs.npccastfilters == 0)
return true;

  switch(app->opcode)
  {
  case OP_Action:
  {
  
	Action_Struct* attk = (Action_Struct*)app->pBuffer;
	Mob* amob = entity_list.GetMob(attk->source);
	Mob* dmob = entity_list.GetMob(attk->target);
	
	  if(amob != 0 && amob->IsClient())
	  {
	  if(ssfs.clientattackfilters != 1 || ssfs.clientattackfilters != 2)
	  return true;

	  
		if(ssfs.clientattackfilters == 1 && (attk->target == GetID() || attk->source == GetID()))
		return true;
		if(ssfs.clientattackfilters == 2 && (attk->target == GetID() || attk->source == GetID()) || (this->isgrouped && entity_list.GetGroupByClient(this) != 0 && (entity_list.GetGroupByClient(this)->IsGroupMember(amob->CastToClient()) || (dmob->IsClient() && entity_list.GetGroupByClient(this)->IsGroupMember(dmob->CastToClient())))))
		return true;
		
		  return false;
		  }
		  if(amob != 0 && amob->IsNPC())
		  {
		  if(ssfs.npcattackfilters != 1 || ssfs.npcattackfilters != 2 || ssfs.npcattackfilters != 3)
		  return true;
		  
			if(ssfs.npcattackfilters == 1 && attk->damage > 0)
			return true;
			if(ssfs.npcattackfilters == 2 && attk->target == GetID() && attk->damage > 0)
			return true;
			if(ssfs.npcattackfilters == 3 && attk->damage > 0 && (dmob->IsClient() && this->isgrouped && entity_list.GetGroupByClient(this) != 0 && entity_list.GetGroupByClient(this)->IsGroupMember(dmob->CastToClient())))
			return true;
			
			  return false;
			  }
			  break;
			  }
			  case OP_BeginCast:
			  {
			  
				BeginCast_Struct* begincast = (BeginCast_Struct*)app->pBuffer;
				Mob* amob = entity_list.GetMob(begincast->caster_id);
				if(amob != 0 && amob->IsClient())
				{
				if(ssfs.npccastfilters != 1 && ssfs.npccastfilters != 2)
				return true;
				
				  if(ssfs.clientcastfilters == 1 && begincast->caster_id == GetID())
				  return true;
				  if(ssfs.clientcastfilters == 2 && amob->GetTarget()->CastToClient() == this)
				  return true;
				  
					return false;
					}
					if(amob != 0 && amob->IsNPC())
					{
					if(ssfs.npccastfilters != 1 || ssfs.npccastfilters != 2)
					return true;
					
					  if(ssfs.clientcastfilters == 2 && (amob->CastToNPC()->GetHateTop()->IsClient() && amob->CastToNPC()->GetHateTop()->CastToClient() == this))
					  return true;
					  
						return false;
						}
						
						  break;
						  }
						  }
						  
							return true;
}*/

bool Client::LootToStack(int32 itemid) {  //Loots stackable items to existing stacks - Wiz
	const Item_Struct* item;
	int i;
	for (i=22; i<=29; i++) {
		if (pp.inventory[i] != 0xFFFF) {
			item = database.GetItem(pp.inventory[i]);
			if (pp.invitemproperties[i].charges < 20 && item->item_nr == itemid)
			{
				pp.invitemproperties[i].charges += 1;
				APPLAYER* outapp = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));
				memcpy(outapp->pBuffer, item, outapp->size);
				Item_Struct* outitem = (Item_Struct*) outapp->pBuffer;
				outitem->equipSlot = i;
				outitem->common.charges = pp.invitemproperties[i].charges;
				QueuePacket(outapp);
				delete outapp;
				return true;
			}
		}
	}
	for (i=0; i<=pp_containerinv_size; i++) {
		if (pp.containerinv[i] != 0xFFFF) {
			item = database.GetItem(pp.containerinv[i]);
			if (pp.bagitemproperties[i].charges < 20 && item->item_nr == itemid)
			{
				pp.bagitemproperties[i].charges += 1;
				APPLAYER* outapp = new APPLAYER(OP_ItemTradeIn, sizeof(Item_Struct));
				memcpy(outapp->pBuffer, item, outapp->size);
				Item_Struct* outitem = (Item_Struct*) outapp->pBuffer;
				outitem->equipSlot = 250+i;
				outitem->common.charges = pp.bagitemproperties[i].charges;
				QueuePacket(outapp);
				delete outapp;
				return true;
			}
		}
	}
	return false;
}

void Client::SetFeigned(bool in_feigned) {
	if (in_feigned)
	{
		SetPet(0);
		entity_list.ClearFeignAggro(this);
	}
	feigned=in_feigned;
 }

sint16	Client::GetMR()
{
    return 20 + itembonuses->MR + spellbonuses->MR + aa.general_skills.named.innate_magic_protection * 5;
}

sint16	Client::GetFR()
{
    return 20 + itembonuses->FR + spellbonuses->FR + aa.general_skills.named.innate_fire_protection * 5;
}

sint16	Client::GetDR()
{
    return 20 + itembonuses->DR + spellbonuses->DR + aa.general_skills.named.innate_disease_protection * 5;
}

sint16	Client::GetPR()
{
    return 20 + itembonuses->PR + spellbonuses->PR + aa.general_skills.named.innate_poison_protection * 5;
}

sint16	Client::GetCR()
{
    return 20 + itembonuses->CR + spellbonuses->CR + aa.general_skills.named.innate_cold_protection * 5;
}

void Client::Commandlog(const Seperator* sep)
{
	bool continueevents=false;
	switch (zone->loglevelvar){ //catch failsafe
	case 9:{
		if ((admin>= 150) && (admin <200))
			continueevents=true;
		break;
		   }
	case 8:{
		if ((admin>= 100) && (admin <150))
			continueevents=true;
		break;
		   }
	case 1:{
		if ((admin>= 200))
			continueevents=true;
		break;
		   }
	case 2:{
		if ((admin>= 150))
			continueevents=true;
		break;
		   }
	case 3:{
		if ((admin>= 100))
			continueevents=true;
		break;
		   }
	case 4:{
		if ((admin>= 80))
			continueevents=true;
		break;
		   }
	case 5:{
		if ((admin>= 20))
			continueevents=true;
		break;
		   }
	case 6:{
		if ((admin>= 10))
			continueevents=true;
		break;
		   }
	case 7:{
			continueevents=true;
			break;
		   }
	}
	if (continueevents==true){
	commandlogged=true;
	entirecommand[0]=NULL; //hehe :P
	for (int i=0; i<=sep->argnum; i++) {
		strcat(entirecommand,sep->arg[i]);
		strcat(entirecommand," ");
	}
	if (target)
		database.logevents(AccountName(),AccountID(),admin,GetName(),target->GetName(),"Command",entirecommand,1);
	else
		database.logevents(AccountName(),AccountID(),admin,GetName(),"None","Command",entirecommand,1);
	}
}

void Client::LogMerchant(Client* player,Mob* merchant,Merchant_Purchase_Struct* mp,const Item_Struct* item,bool buying){
	char* logtext;
	char itemid[100];
	char itemcost[100];
	char itemname[100];
	char itemquantity[100];
	if (buying==true){
		memset(itemid,0,sizeof(itemid));
		memset(itemcost,0,sizeof(itemid));
		memset(itemname,0,sizeof(itemid));
		memset(itemquantity,0,sizeof(itemid));
		itoa(mp->quantity,itemquantity,10);
		itoa(item->item_nr,itemid,10);
		sprintf(itemname,"%s",item->name);
		itoa(mp->itemcost,itemcost,10);
		logtext=itemname;
		strcat(logtext,"(");
		strcat(logtext,itemid);
		strcat(logtext,"), Quantity: ");
		strcat(logtext,itemquantity);
		strcat(logtext,", Cost: ");
		strcat(logtext,itemcost);
		database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),merchant->GetName(),"Buying from Merchant",logtext,2);
	}
	else{
		memset(itemid,0,sizeof(itemid));
		memset(itemcost,0,sizeof(itemid));
		memset(itemname,0,sizeof(itemid));
		memset(itemquantity,0,sizeof(itemid));
		itoa(mp->quantity,itemquantity,10);
		itoa(item->item_nr,itemid,10);
		sprintf(itemname,"%s",item->name);
		itoa((int)(item->cost*((mp->quantity == 0) ? 1:mp->quantity))-(item->cost * ((mp->quantity == 0) ? 1:mp->quantity) *0.08),itemcost,10);
		logtext=itemname;
		strcat(logtext,"(");
		strcat(logtext,itemid);
		strcat(logtext,"), Quantity: ");
		strcat(logtext,itemquantity);
		strcat(logtext,", Sell Total: ");
		strcat(logtext,itemcost);
		database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),merchant->GetName(),"Selling to Merchant",logtext,3);
	}
}
void Client::LogLoot(Client* player,Corpse* corpse,const Item_Struct* item){
	char* logtext;
	char itemid[100];
	char itemname[100];
	char coinloot[100];
	if (item!=0){
	memset(itemid,0,sizeof(itemid));
	memset(itemname,0,sizeof(itemid));
	itoa(item->item_nr,itemid,10);
	sprintf(itemname,"%s",item->name);
	logtext=itemname;

	strcat(logtext,"(");
	strcat(logtext,itemid);
	strcat(logtext,") Looted");
	database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),corpse->orgname,"Looting Item",logtext,4);
	}
	else{
		if ((corpse->GetPlatinum() + corpse->GetGold() + corpse->GetSilver() + corpse->GetCopper())>0){
		memset(coinloot,0,sizeof(coinloot));
		sprintf(coinloot,"%i PP %i GP %i SP %i CP",corpse->GetPlatinum(),corpse->GetGold(),corpse->GetSilver(),corpse->GetCopper());
		logtext=coinloot;
		strcat(logtext," Looted");
		if (corpse->GetPlatinum()>10000)
			database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),corpse->orgname,"Excessive Loot!",logtext,9);
		else
			database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),corpse->orgname,"Looting Money",logtext,5);
		}
	}
}

bool Client::BindWound(Mob* bindmob, bool start, bool fail){
 APPLAYER* outapp = 0;
 if(!fail) {
    outapp = new APPLAYER(OP_Bind_Wound, sizeof(BindWound_Struct));
	BindWound_Struct* bind_out = (BindWound_Struct*) outapp->pBuffer;
    // Start bind
    if(!bindwound_timer->Enabled()) {
        // start complete timer
        bindwound_timer->Start(10000);
        bindwound_target = bindmob;
        // Send client unlock
        bind_out->type = 3;
        QueuePacket(outapp);
        bind_out->type = 0;
        // Client Unlocked
        if(!bindmob){
                // send "bindmob dead" to client
                bind_out->type = 4;
                QueuePacket(outapp);
                bind_out->type = 0;
                bindwound_timer->Disable();
                bindwound_target = 0;
        }
        else {
                // send bindmob "stand still"
                if(!bindmob->IsAIControlled() && bindmob != this ) {
                                bind_out->type = 2; // ?
                                //bind_out->type = 3; // ?
                                bind_out->to = GetID(); // ?
                                bindmob->CastToClient()->QueuePacket(outapp);
                                bind_out->type = 0;
                                bind_out->to = 0;
                }
                else if (bindmob->IsAIControlled() && bindmob != this ){
                // Tell IPC to stand still?
                }
                else {
                // Binding self
                }
        }
    }
    else if (bindwound_timer->Enabled()){
    // finish bind
        // disable complete timer
        bindwound_timer->Disable();
        bindwound_target = 0;
        if(!bindmob){
                // send "bindmob gone" to client
                bind_out->type = 5; // not in zone
                QueuePacket(outapp);
                bind_out->type = 0;
        }
        else {
            if (bindmob->Dist(this) <= 20) {
                // send bindmob bind done 
                if(!bindmob->IsAIControlled() && bindmob != this ) {
                
                }
                else if(bindmob->IsAIControlled() && bindmob != this ) {
                // Tell IPC to resume??
                }
                else {
                // Binding self
                }
                // Send client bind done
                DeleteItemInInventory(HasItemInInventory(13009, 1), 1, true);
                bind_out->type = 1; // Done
                QueuePacket(outapp);
                bind_out->type = 0;
                CheckIncreaseSkill(BIND_WOUND);

                float max_percent = 0.5f;
                uint8 *aa_item = &(((uint8 *)&aa)[14]);
                if (*aa_item){
                    max_percent += 0.1f * (float) *aa_item;
                }

                // send bindmob new hp's
                if (bindmob->GetHP() < bindmob->GetMaxHP() && bindmob->GetHP() <= (bindmob->GetMaxHP()*max_percent)-1){
                    // 0.120 per skill point, 0.60 per skill level, minimum 3 max 30
                    int bindhps = 3;
                    if (GetSkill(BIND_WOUND) >= 10) {
                        bindhps += (int) GetSkill(BIND_WOUND)*0.120;
                    }
                    if (bindhps > 30){
                        bindhps = 30;
                    }
                    bindmob->SetHP( bindmob->GetHP() + bindhps);
                    APPLAYER* hpapp = new APPLAYER(OP_HPUpdate, sizeof(SpawnHPUpdate_Struct));
                    bindmob->CreateHPPacket(hpapp);
                    entity_list.QueueCloseClients(this, hpapp, false, 100.0);
                    delete hpapp;
                }
                else {
                 // Too many hp message goes here...
                }
            }
            else {
                // Send client bind failed
                bind_out->type = 6; // They moved
                QueuePacket(outapp);
                bind_out->type = 0;
            }
        }
    }
 }
 else if (fail && bindwound_timer->Enabled()){
 // You moved
    outapp = new APPLAYER(OP_Bind_Wound, sizeof(BindWound_Struct));
	BindWound_Struct* bind_out = (BindWound_Struct*) outapp->pBuffer;
    bindwound_timer->Disable();
    bindwound_target = 0;
    bind_out->type = 7;
    QueuePacket(outapp);
    bind_out->type = 3;
    QueuePacket(outapp);
 }
 safe_delete(outapp);
 return true;
}

bool Client::MoveItem(int16 to_slot, int16 from_slot, uint8 quantity){
    if(DEBUG>=5) {
        LogFile->write(EQEMuLog::Debug, "%s::MoveItem(%i,%i,%i)", GetName(), to_slot, from_slot, quantity);
    }
    	if ( to_slot >=3000 && to_slot <= 3016){// Item Trade
			TradeList[to_slot-3000] = GetItemAt(0); // Put item number from cursor into Tradelist
			TradeCharges[to_slot-3000] = pp.invitemproperties[0].charges; // Get charge count from cursor into TradeList
			// Missing bag flag check?
			for (int j = 0;j != 10;j++) { // Process bag items into out half of trade window
				TradeList[((to_slot-2999)*10)+j] = GetItemAt(250+j); // Item number from cursor bag to Tradelist
				TradeCharges[((to_slot-2999)*10)+j] = GetItemPropAt(250+j).charges; // Item charge from cursor bag to TradeList
			}
			if (this->TradeWithEnt){
				Client* tradecl = entity_list.GetClientByID(this->TradeWithEnt);
				if (tradecl) {
					const Item_Struct* item = database.GetItem(TradeList[to_slot-3000]);
					if (item) {
						APPLAYER* outapp = new APPLAYER(OP_ItemToTrade,sizeof(ItemToTrade_Struct));
						ItemToTrade_Struct* ti = (ItemToTrade_Struct*)outapp->pBuffer;
						memcpy(&ti->item,item,sizeof(Item_Struct));
						ti->item.common.charges = TradeCharges[to_slot-3000];
						ti->toslot = to_slot-3000;
						ti->playerid = this->TradeWithEnt;
						tradecl->FastQueuePacket(&outapp);
						char itemid[100];
						memset(itemid,0,sizeof(itemid));
						if (strlen(this->tradeitems)>1)
							strcat(this->tradeitems,", ");
						itoa(item->item_nr,itemid,10);
						strcat(this->tradeitems,item->name);
						strcat(this->tradeitems,"(");
						strcat(this->tradeitems,itemid);
						strcat(this->tradeitems,")");
						printf("this->tradeitems: %s\n",this->tradeitems);
					}
				}
			}
			return true;
		}
        if (from_slot==0&&to_slot==0){
                // Summoning item
                return true; // FIXME summoned items should go to the cursors stack queue
        }
    // Check valid from_slot
		if (from_slot >= 0 && from_slot <= 29){ // Worn items and main inventory
		}
		else if (from_slot >= 250 && from_slot <= 339){ // Main inventory's and cursor's containers
		}
		else if (from_slot >= 2000 && from_slot <= 2007){ // Bank slots
		}
		else if (from_slot >= 2030 && from_slot <= 2109){ // Bank's containers
		}
		else {
			LogFile->write(EQEMuLog::Error,"%s::MoveItem(%i,%i,%u) unknown from_slot.", GetName(), to_slot, from_slot, quantity);
			return false;
		}
    // Check valid item in from_slot
        Item_Struct* from_item = new Item_Struct;
        memset(from_item, 0, sizeof(Item_Struct));
        from_item->item_nr = 0xFFFF;
		if ( (GetItemAt(from_slot)) != 0xFFFF && (GetItemAt(from_slot)) != 0 ){
		    memcpy(from_item, database.GetItem(GetItemAt(from_slot)), sizeof(Item_Struct));
		    if (!from_item->item_nr){
		          LogFile->write(EQEMuLog::Error, "%s::MoveItem(%i,%i,%i) database error couldn't find from item (%i)", GetName(), to_slot, from_slot, quantity, GetItemAt(from_slot));
		          return false;
            }
            if (quantity != 0){
                  from_item->common.charges = quantity;
            }
            else {
                  from_item->common.charges = GetItemPropAt(from_slot).charges;
            }
		}
		else {
			LogFile->write(EQEMuLog::Error,"%s::MoveItem(%i,%i,%i) invalid item in from_slot.", GetName(), to_slot, from_slot, quantity);
			return false;
		}
		if(DEBUG>=5)
		  LogFile->write(EQEMuLog::Debug, 
                "%s::MoveItem(%i,%i,%i) From item number:%u count:%i bag:%s", 
                                GetName(), to_slot, from_slot, quantity, from_item->item_nr, from_item->common.charges, (from_item->IsBag()) ? "true":"false");
    // Check valid to_slot
		if (to_slot >= 0 && to_slot <= 29){ // Worn items and main inventory
		  if (to_slot >= 1 && to_slot <= 21 && !database.GetItem(GetItemAt(from_slot))->IsEquipable(GetRace(), GetClass()) ) {
		      LogFile->write(EQEMuLog::Error, "%s Attempting an invalid item equip.", GetName());
		      // ResendPlayerInventory() || Kick()
		      return false;
	      }
		}
		else if (to_slot >= 250 && to_slot <= 339){ // Main inventory's and cursor's containers
		}
		else if (to_slot >= 2000 && to_slot <= 2007){ // Bank slots
		}
		else if (to_slot >= 2030 && to_slot <= 2109){ // Bank's containers
		}
		else if (to_slot == 0xFFFF){ // Delete button
		}
		else {
			LogFile->write(EQEMuLog::Error,"%s::MoveItem(%i,%i,%i) unknown to_slot.", GetName(), to_slot, from_slot, quantity);
			return false;
		}
    // Check for existing item in to_slot
        Item_Struct* to_item = new Item_Struct;
        memset(to_item, 0, sizeof(Item_Struct));
        to_item->item_nr = 0xFFFF;
		if ( (GetItemAt(to_slot)) != 0xFFFF && (GetItemAt(to_slot)) != 0 ){
		    memcpy(to_item, database.GetItem(GetItemAt(to_slot)), sizeof(Item_Struct));
		    if (!to_item->item_nr){
		          LogFile->write(EQEMuLog::Debug, "%s::MoveItem(%i,%i,%i) database error couldn't find to item (%i)", GetName(), to_slot, from_slot, quantity, GetItemAt(to_slot));
            }
            to_item->common.charges = GetItemPropAt(to_slot).charges;
		}
		else {
		}
		if(DEBUG>=5)
		  LogFile->write(EQEMuLog::Debug, 
                "%s::MoveItem(%i,%i,%i) To item number:%u count:%i bag:%s", 
                                GetName(), to_slot, from_slot, quantity, to_item->item_nr, to_item->common.charges, (to_item->IsBag()) ? "true":"false");

    // Bag
        if (to_item->IsBag() || from_item->IsBag()) {
        if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "MoveItem: about to proccess a bag");
                if (from_item->IsBag() && to_slot==0xFFFF){
                    for (int bagslot = 0; bagslot < from_item->container.numSlots; bagslot++)
                         DeleteItemInInventory(250+bagslot);
                    DeleteItemInInventory(from_slot);
                    return true;
                }
                int bag_slot = 0;
                Item_Struct to_bag[10]; 
                for(bag_slot=0; bag_slot < to_item->container.numSlots; bag_slot++) {
                    memset(&to_bag[bag_slot], 0, sizeof(Item_Struct));
                    to_bag[bag_slot].item_nr = 0xFFFF;
                }
                bool to_bag_empty = true;
                Item_Struct from_bag[10]; 
                for(bag_slot=0; bag_slot < from_item->container.numSlots; bag_slot++) {
                    memset(&from_bag[bag_slot], 0, sizeof(Item_Struct));
                    from_bag[bag_slot].item_nr = 0xFFFF;
                }
                bool from_bag_empty = true;
                Item_Struct temp_bag[10];
                for(bag_slot=0; bag_slot < to_item->container.numSlots; bag_slot++) {
                    memset(&temp_bag[bag_slot], 0, sizeof(Item_Struct));
                    temp_bag[bag_slot].item_nr = 0xFFFF;
                }
                bool temp_bag_empty = true;
                int16 bag_offset = 0;
                int16 to_offset = 0;
                int16 from_offset = 0;
                bag_offset = (to_slot >= 22 && to_slot <= 29) ? ((to_slot-21)*10)+250:((to_slot-2000)*10)+2030;
                if (to_slot==0)
                    bag_offset=250;
                to_offset = bag_offset;
                if (to_item->IsBag()){
                    for (bag_slot = 0; bag_slot < to_item->container.numSlots; bag_slot++){
                        if(GetItemAt(bag_offset+bag_slot) == 0 || GetItemAt(bag_offset+bag_slot) == 0xFFFF ){
                           LogFile->write(EQEMuLog::Debug, "to_bag[%i:%i] item:%u count:%i", bag_slot, bag_slot+bag_offset, to_bag[bag_slot].item_nr, to_bag[bag_slot].common.charges);
                           continue;
                        }
                        memcpy(&to_bag[bag_slot], database.GetItem(GetItemAt(bag_offset+bag_slot)), sizeof(Item_Struct));
                        to_bag[bag_slot].common.charges = GetItemPropAt(bag_offset+bag_slot).charges;
                        to_bag_empty = false;
                        LogFile->write(EQEMuLog::Debug, "to_bag[%i:%i] item:%u count:%i", bag_slot, bag_slot+bag_offset, to_bag[bag_slot].item_nr, to_bag[bag_slot].common.charges);
                    }
                }
                bag_offset = (from_slot >= 22 && from_slot <= 29) ? ((from_slot-22)*10)+250:((from_slot-2000)*10)+2030;
                if (from_slot==0)
                    bag_offset=250;
                from_offset = bag_offset;
                if (from_item->IsBag()){
                    for (bag_slot = 0; bag_slot < from_item->container.numSlots; bag_slot++){
                        if(GetItemAt(bag_offset+bag_slot) == 0 || GetItemAt(bag_offset+bag_slot) == 0xFFFF || database.GetItem(GetItemAt(bag_offset+bag_slot)) == 0){
                           LogFile->write(EQEMuLog::Debug, "from_bag[%i:%i] item:%u count:%i", bag_slot, bag_slot+bag_offset, from_bag[bag_slot].item_nr, from_bag[bag_slot].common.charges);
                           continue;
                        }
                        memcpy(&from_bag[bag_slot], database.GetItem(GetItemAt(bag_offset+bag_slot)), sizeof(Item_Struct));
                        from_bag[bag_slot].common.charges = GetItemPropAt(bag_offset+bag_slot).charges;
                        from_bag_empty = false;
                        LogFile->write(EQEMuLog::Debug, "from_bag[%i:%i] item:%u count:%i", bag_slot, bag_slot+bag_offset, from_bag[bag_slot].item_nr, from_bag[bag_slot].common.charges);
                    }
                }
#if 0
                if (!to_bag_empty)
                    memcpy(&temp_bag, &to_bag, sizeof(Item_Struct)*to_item->container.numSlots );
                if (!from_bag_empty)
                    memcpy(&to_bag, &from_bag, sizeof(Item_Struct)*from_item->container.numSlots );
                if (!to_bag_empty)
                    memcpy(&from_bag, &temp_bag, sizeof(Item_Struct)*to_item->container.numSlots );
#endif
                if ( from_item->item_nr != 0 && from_item->item_nr != 0xFFFF )
                    DeleteItemInInventory((uint32) from_slot, (quantity == 0) ? quantity:from_item->common.charges);
                if ( to_item->item_nr != 0 && to_item->item_nr != 0xFFFF )
                    DeleteItemInInventory((uint32) to_slot, (quantity == 0) ? quantity:to_item->common.charges);

                if ( from_item->item_nr != 0 && from_item->item_nr != 0xFFFF )
                    PutItemInInventory( to_slot, (const Item_Struct*) from_item, from_item->common.charges, false);
                if ( to_item->item_nr != 0 && to_item->item_nr != 0xFFFF )
                    PutItemInInventory( from_slot, (const Item_Struct*) to_item, to_item->common.charges, false);

                if(to_item->IsBag()){    // inject to from_slot
                if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "Sending from_bag");
                     for (bag_slot = 0;bag_slot < 10;bag_slot++){
                         if (from_bag[bag_slot].item_nr != 0xFFFF && from_bag[bag_slot].item_nr != 0) {
                             cout<<"Item = "<<from_bag[bag_slot].item_nr<<endl;
                             cout<<"Addr2="<<&from_bag[bag_slot]<<endl;
                             PutItemInInventory( from_offset+bag_slot, (const Item_Struct*) &from_bag[bag_slot], from_bag[bag_slot].common.charges, false);
                         }
                     }
                }
                if(from_item->IsBag()){  // inject to to_slot
                if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "Sending to_bag");
                     for (bag_slot = 0;bag_slot < 10;bag_slot++){
                         if (to_bag[bag_slot].item_nr != 0xFFFF && to_bag[bag_slot].item_nr != 0){
                             PutItemInInventory( to_offset+bag_slot, (const Item_Struct*) &to_bag[bag_slot], to_bag[bag_slot].common.charges, false);
                         }
                     }
                }
                return true;
        }
        else if (to_slot==0xFFFF){
                DeleteItemInInventory(from_slot);
                return true;
        }
    // Stackable
        if ( to_item->IsStackable() && from_item->IsStackable() && to_item->item_nr == from_item->item_nr) {
        if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "MoveItem: about to proccess stack");
                if ( (to_item->common.charges+from_item->common.charges) > 20 ) {
                    LogFile->write(EQEMuLog::Error, "%s MoveItem charge error stack count > 20", GetName());
                }
                DeleteItemInInventory((uint32) from_slot, quantity);
                from_item->common.charges += to_item->common.charges;
                if(PutItemInInventory( to_slot, (const Item_Struct*) from_item, from_item->common.charges, false))
                                return true;
                else
                                return false;
        }
        if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "MoveItem: about to proccess non stackable/stackable not in stack");
    // Not stackable
        if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "MoveItem: deleteing from item %u, from slot %i", from_item->item_nr, from_slot);
        if ( !DeleteItemInInventory((uint32) from_slot, (quantity == 0) ? quantity:from_item->common.charges) )
                return false;
        if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "MoveItem: deleteing to item %u, from slot %i", to_item->item_nr, to_slot);
        if ( !DeleteItemInInventory((uint32) to_slot, (quantity == 0) ? quantity:to_item->common.charges) )
                return false;
        if (   to_item->item_nr != 0xFFFF && to_item->item_nr != 0) {
            if  (!PutItemInInventory( from_slot, (const Item_Struct*) to_item, to_item->common.charges, false) )
                return false;
        if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "MoveItem: injected from item %u, into from slot %i", to_item->item_nr, from_slot);
        }
        if (   from_item->item_nr != 0xFFFF && from_item->item_nr != 0) {
            if ( !PutItemInInventory( to_slot, (const Item_Struct*) from_item, from_item->common.charges, false) )
                return false;
        if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "MoveItem: injected from item %u, into to_slot %i", from_item->item_nr, to_slot);
        }
           
    return true;
}

