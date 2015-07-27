/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2005  EQEMu Development Team (http://eqemulator.net)

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

/*

Assuming you want to add a new perl quest function named joe
that takes 1 integer argument....

1. Add the prototype to the quest manager:
questmgr.h: add (~line 50)
	void joe(int arg);

2. Define the actual function in questmgr.cpp:
void QuestManager::joe(int arg) {
	//... do something
}

3. Copy one of the XS routines in perlparser.cpp, preferably
 one with the same number of arguments as your routine. Rename
 as needed.
 Finally, add your routine to the list at the bottom of perlparser.cpp


4.
If you want it to work in old mode perl and .qst, edit parser.cpp
Parser::ExCommands (~line 777)
	else if (!strcmp(command,"joe")) {
		quest_manager.joe(atoi(arglist[0]));
	}

And then at then end of embparser.cpp, add:
"sub joe{push(@cmd_queue,{func=>'joe',args=>join(',',@_)});}"



*/

#include "logger.h"
#include "entity.h"
#include "groups.h"
#include "client.h"
#include "object.h"
#include "PlayerCorpse.h"
#include "mob.h"
#include <sstream>
#include <iostream>
#include <list>
using namespace std;

#include "worldserver.h"
#include "net.h"
#include "skills.h"
#include "classes.h"
#include "races.h"
#include "database.h"
//#include "common/files.h"
#include "spdat.h"
#include "packet_functions.h"
#include "spawn2.h"
#include "zone.h"
#include "event_codes.h"
#include "SpellsHandler.hpp"
#ifdef EMBPERL
#include "embparser.h"
#endif
#include "questmgr.h"

extern Zone* zone;
extern WorldServer worldserver;
extern EntityList entity_list;
extern SpellsHandler	spells_handler;


//declare our global instance
QuestManager quest_manager;

QuestManager::QuestManager() {
	depop_npc = false;
	HaveProximitySays = false;
	quest_timers = new Timer(1000);
}

QuestManager::~QuestManager() {
}

void QuestManager::Process() {

	list<QuestTimer>::iterator cur = QTimerList.begin(), end, tmp;

	end = QTimerList.end();
	while (cur != end) {
		if (cur->Timer_.Enabled() && cur->Timer_.Check()) {
			//make sure the mob is still in zone.
			if(entity_list.IsMobInZone(cur->mob)){
#ifdef EMBPERL
				if(cur->mob->IsNPC()) {
					perlParser->Event(EVENT_TIMER, cur->mob->GetNPCTypeID(), cur->name.c_str(), cur->mob->CastToNPC(), NULL);
				}
				else{
					perlParser->Event(EVENT_TIMER, 0, cur->name.c_str(), (NPC*)NULL, cur->mob);
				}
#endif

				//we MUST reset our iterator since the quest could have removed/added any
				//number of timers... worst case we have to check a bunch of timers twice
				cur = QTimerList.begin();
				end = QTimerList.end();	//dunno if this is needed, cant hurt...
			} else {
				tmp = cur;
				tmp++;
				QTimerList.erase(cur);
				cur = tmp;
			}
		} else
			cur++;
	}

	list<SignalTimer>::iterator curS, endS, tmpS;

	curS = STimerList.begin();
	endS = STimerList.end();
	while (curS != endS) {
		if(!curS->Timer_.Enabled()) {
			//remove the timer
			tmpS = curS;
			tmpS++;
			STimerList.erase(curS);
			curS = tmpS;
		} else if(curS->Timer_.Check()) {
			//disable the timer so it gets deleted.
			curS->Timer_.Disable();

			//signal the event...
			entity_list.SignalMobsByNPCID(curS->npc_id, curS->signal_id);

			//restart for the same reasons as above.
			curS = STimerList.begin();
			endS = STimerList.end();
		} else
			curS++;
	}
}

void QuestManager::StartQuest(Mob *_owner, Client *_initiator, ItemInst* _questitem) {
	quest_mutex.lock();
	owner = _owner;
	initiator = _initiator;
	questitem = _questitem;
	depop_npc = false;
}

void QuestManager::EndQuest() {
	quest_mutex.unlock();

	if(depop_npc && owner->IsNPC()) {
		//clear out any timers for them...
		list<QuestTimer>::iterator cur = QTimerList.begin(), end, tmp;

		end = QTimerList.end();
		while (cur != end) {
			if(cur->mob == owner) {
				tmp = cur;
				tmp++;
				QTimerList.erase(cur);
				cur = tmp;
			} else {
				cur++;
			}
		}

		owner->Depop();
		owner = NULL;	//just to be safe
	}
}


//quest perl functions
void QuestManager::echo(MessageFormat type, const char *str) {
   char *data = 0;
   MakeAnyLenString(&data, str);
   entity_list.MessageClose(initiator, false, 200, type,data); 
}

void QuestManager::say(const char *str) {

	 owner->Say(str);
}

void QuestManager::say(const char *str, int8 language) {

	char *data = 0;
	MakeAnyLenString(&data, str);
	entity_list.ChannelMessage(owner, SAY, language, data);
}

void QuestManager::me(const char *str) {
	if (!initiator)
		return;

	char *data = 0;
	MakeAnyLenString(&data, str);
	entity_list.MessageClose(initiator, false, 200, WHITE, data);
}

void QuestManager::summonitem(int32 itemid, uint8 charges) {
	if(!initiator)
		return;
	//initiator->SummonItemNonBlob(itemid, charges);
	// Harakiri, test some new item tables
	initiator->SummonItemNonBlob(itemid, charges);
}

void QuestManager::write(const char *file, const char *str) {
	FILE * pFile;
	pFile = fopen (file, "a");
	if(!pFile)
		return;
	fprintf(pFile, "%s\n", str);
	fclose (pFile);
}

int16 QuestManager::spawn2(int npc_type, int grid, int unused, float x, float y, float z, float heading) {
	NPCType* tmp = 0;
	if ((tmp = Database::Instance()->GetNPCType(npc_type)))
	{

		NPC* npc = new NPC(tmp, 0, x, y, z, heading);


		npc->AddLootTable();
		entity_list.AddNPC(npc,true);
		// Quag: Sleep in main thread? ICK!
		// Sleep(200);
		// Quag: check is irrelevent, it's impossible for npc to be 0 here
		// (we're in main thread, nothing else can possibly modify it)
//		if(npc != 0) {
			if(grid > 0)
			{
				// HARAKIRIFIXME npc->AssignWaypoints(grid);
			}
			npc->SendPosUpdate();
//		}
		return(npc->GetID());
	}
	return(0);
}

int16 QuestManager::unique_spawn(int npc_type, int grid, int unused, float x, float y, float z, float heading) {
	Mob *other = entity_list.GetMobByNpcTypeID(npc_type);
	if(other != NULL) {
		return(other->GetID());
	}


	NPCType* tmp = 0;
	if ((tmp = Database::Instance()->GetNPCType(npc_type)))
	{

		NPC* npc = new NPC(tmp, 0, x, y, z, heading);


		npc->AddLootTable();
		entity_list.AddNPC(npc,true);
		// Quag: Sleep in main thread? ICK!
		// Sleep(200);
		// Quag: check is irrelevent, it's impossible for npc to be 0 here
		// (we're in main thread, nothing else can possibly modify it)
//		if(npc != 0) {
			if(grid > 0)
			{
				// HarakiriFIXME npc->AssignWaypoints(grid);
			}
			npc->SendPosUpdate();
//		}
		return(npc->GetID());
	}
	return(0);
}


void QuestManager::castspell(int spell_id, int target_id) {
	if (owner) {
		Mob *tgt = entity_list.GetMob(target_id);
		if(tgt != NULL)
			owner->SpellFinished(spells_handler.GetSpellPtr(spell_id), target_id);
	}
}

void QuestManager::selfcast(int spell_id) {
	if (initiator)
		initiator->SpellFinished(spells_handler.GetSpellPtr(spell_id), initiator->GetID(),SLOT_ITEMSPELL);
}

void QuestManager::addloot(int item_id, int charges) {
	if(item_id != 0){
		if(owner->IsNPC())
			owner->CastToNPC()->AddItem(item_id, charges,0);
	}
}

void QuestManager::Zone(const char *zone_name) {
	if (initiator && initiator->IsClient())
	{

			float x = -1;
			float y = -1;
			float z = -1;
			float myHeading = -1;
			char *data = 0;
			MakeAnyLenString(&data, zone_name);
			if(Database::Instance()->GetSafePoints(data, &x, &y, &z, 0, 0, &myHeading))
				initiator->CastToClient()->ZonePC(data,x,y,z);		
	}
}

void QuestManager::settimer(const char *timer_name, int seconds) {
	
	list<QuestTimer>::iterator cur = QTimerList.begin(), end;

	end = QTimerList.end();
	while (cur != end) {
		if (cur->mob == owner && cur->name == timer_name) {
			cur->mob = owner;
			cur->Timer_.Enable();
			cur->Timer_.Start(seconds * 1000, false);			
			EQC::Common::Log(EQCLog::Debug,CP_QUESTS,"Reset Timer (%s): %s for %d seconds", cur->mob->GetName(),cur->name.c_str(), seconds);	
			return;
		}
		cur++;
	}

	QTimerList.push_back(QuestTimer(seconds * 1000, owner, timer_name));
}

void QuestManager::stoptimer(const char *timer_name) {

	list<QuestTimer>::iterator cur = QTimerList.begin(), end;

	end = QTimerList.end();
	while (cur != end)
	{
		if(cur->mob == owner && cur->name == timer_name)
		{
			QTimerList.erase(cur);
			return;
		}
		cur++;
	}
}

void QuestManager::emote(const char *str) {	
	owner->Emote(str);
}

void QuestManager::shout(const char *str) {	
   owner->Shout(str);  
}

void QuestManager::shout2(const char *str) {
	
   char *data = 0;
   MakeAnyLenString(&data, "%s shouts, '%s'", owner->GetName(), str);
   worldserver.SendEmoteMessage(0,0,RED, data);
}

void QuestManager::depop(int npc_type) {
	if(!owner->IsNPC())
		return;
	if (npc_type != 0){
		Mob * tmp = entity_list.GetMobByNpcTypeID(npc_type);
		if (tmp) {
			if(tmp != owner){
				tmp->CastToNPC()->Depop();
				entity_list.RemoveNPC(tmp->CastToNPC());			
			}
			else
				depop_npc = true;
		}
	}
	else {	//depop self
		depop_npc = true;
	}
}

void QuestManager::depopall(int npc_type) {
	if(owner->IsNPC() && npc_type > 0) {
		Mob* tmp = entity_list.GetMobByNpcTypeID(npc_type);
		while(tmp) {
			if(tmp != owner){
				tmp->CastToNPC()->Depop();
				entity_list.RemoveNPC(tmp->CastToNPC());				
			}
			else
				depop_npc = true;

			tmp = entity_list.GetMobByNpcTypeID(npc_type);
		}
	}
}


void QuestManager::settarget(const char *type, int target_id) {
	if(!owner->IsNPC())
		return;
	Mob* tmp = NULL;
	if (!strcasecmp(type,"npctype")) {
		tmp = entity_list.GetMobByNpcTypeID(target_id);
	}
	else if (!strcasecmp(type, "entity")) {
		tmp = entity_list.GetMob(target_id);
	}
	if(tmp != NULL) {
		owner->SetTarget(tmp);
	}
}

void QuestManager::follow(int entity_id) {
	if(!owner->IsNPC())
		return;
//	owner->SetFollowID(entity_id);
}


void QuestManager::exp(int amt) {
	if (initiator && initiator->IsClient())
		initiator->AddEXP(amt);
}

void QuestManager::level(int newlevel) {
	if (initiator && initiator->IsClient())
		initiator->SetLevel(newlevel, true);
}



void QuestManager::rain(int weather) {
	zone->zone_weather = (WeatherTypesEnum)(weather);

	APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
	memset(outapp->pBuffer, 0, 8);

	if (zone->zone_weather == 1)
	{
		outapp->pBuffer[6] = 0x31; // Rain
	}
	if (zone->zone_weather == 2)
	{
		outapp->pBuffer[0] = 0x01;
		outapp->pBuffer[4] = 0x02; 
	}
					
	entity_list.QueueClients(initiator,outapp);
	safe_delete(outapp);//delete outapp;
}

void QuestManager::snow(int weather) {
	rain(weather);
}

void QuestManager::givecash(int32 npcTypeID, int copper, int silver, int gold, int platinum) {
	if (initiator && initiator->IsClient())
	{
		// no update, handled by questcomplete
		initiator->AddMoneyToPP(copper, silver, gold, platinum,false);
		
		initiator->SendClientQuestCompletedMoney(entity_list.GetMobByNpcTypeID(npcTypeID),copper,silver,gold,platinum);

	}
}


void QuestManager::gmmove(float x, float y, float z) {
	if (initiator && initiator->IsClient())
		initiator->GMMove(x, y, z);
}


void QuestManager::doanim(int anim_id) {
	owner->DoAnim(anim_id);
}


void QuestManager::attack(const char *client_name) {
	if(!owner->IsNPC())
		return;
	Client* getclient = entity_list.GetClientByName(client_name);
	if(getclient) {
		owner->CastToNPC()->AddToHateList(getclient,1);
	} else {
		owner->Say("I am unable to attack %s.", client_name);
	}
}

void QuestManager::attacknpc(int npc_entity_id) {
	if(!owner->IsNPC())
		return;
	Mob *it = entity_list.GetMob(npc_entity_id);	
		owner->CastToNPC()->AddToHateList(it,1);	
		if(it)
			owner->Say("I am unable to attack %s.", it->GetName());
		else
			owner->Say("I am unable to locate NPC entity %i", npc_entity_id);	
}

void QuestManager::attacknpctype(int npc_type_id) {
	if(!owner->IsNPC())
		return;
	Mob *it = entity_list.GetMobByNpcTypeID(npc_type_id);
	
	owner->CastToNPC()->AddToHateList(it,1);
	
}

void QuestManager::save() {
	if (initiator && initiator->IsClient())
		initiator->Save();
}

void QuestManager::faction(int faction_id, int faction_value) {
	if (initiator && initiator->IsClient()) {
		if(faction_id != 0 && faction_value != 0) {

			initiator->SetFactionLevel2(
				initiator->CharacterID(),
				faction_id,
				initiator->GetClass(),
				initiator->GetRace(),
				initiator->GetDeity(),
				faction_value);

		}
	}
}



void QuestManager::signalwith(int npc_id, int signal_id, int wait_ms) {
// SCORPIOUS2K - signal command
	// signal(npcid) - generates EVENT_SIGNAL on specified npc
	if(wait_ms > 0) {
		STimerList.push_back(SignalTimer(wait_ms, npc_id, signal_id));
		return;
	}

	if (npc_id<1)
	{
		printf("signal() bad npcid=%i\n",npc_id);
	}
	else
	{
		//initiator* signalnpc=0;
		entity_list.SignalMobsByNPCID(npc_id, signal_id);
	}
}

void QuestManager::signal(int npc_id, int wait_ms) {
	signalwith(npc_id, 0, wait_ms);
}

void QuestManager::setglobal(const char *varname, const char *newvalue, int options, const char *duration) {
// SCORPIOUS2K - qglobal variable commands
	// setglobal(varname,value,options,duration)
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	//MYSQL_ROW row;
	int qgZoneid=zone->GetZoneID();
	int qgCharid=0;
	int qgNpcid = owner->GetNPCTypeID();

	/*	options value determines the availability of global variables to NPCs when a quest begins
	------------------------------------------------------------------
	  value		   npcid	  player		zone
	------------------------------------------------------------------
		0			this		this		this
		1			all			this		this
		2			this		all			this
		3			all			all			this
		4			this		this		all
		5			all			this		all
		6			this		all			all
		7			all			all			all
	*/
	if (initiator && initiator->IsClient())  // some events like waypoint and spawn don't have a player involved
	{
		qgCharid=initiator->CharacterID();
	}

	else
	{
		qgCharid=-qgNpcid;		// make char id negative npc id as a fudge
	}
	if (options < 0 || options > 7)
	{
		cerr << "Invalid options for global var " << varname << " using defaults" << endl;
	}	// default = 0 (only this npcid,player and zone)
	else
	{
		if (options & 1)
			qgNpcid=0;
		if (options & 2)
			qgCharid=0;
		if (options & 4)
			qgZoneid=0;
	}

	// clean up expired vars and get rid of the one we're going to set if there
	Database::Instance()->RunQuery(query, MakeAnyLenString(&query,
		"DELETE FROM quest_globals WHERE expdate < UNIX_TIMESTAMP() || (name='%s' && npcid=%i && charid=%i && zoneid=%i))"
		,varname,qgNpcid,qgCharid,qgZoneid), errbuf);
	safe_delete_array(query);

	InsertQuestGlobal(qgCharid, qgNpcid, qgZoneid, varname, newvalue, QGVarDuration(duration));
}

// Harakiri we add a quest item for multiquesting to our quest globals
void QuestManager::AddPendingQuestItem(int charid, int npcid, int zoneid, const char *itemid) {

	if( strcmp(itemid,"NULL") == 0) {
		return;
	}

	stringstream varname;
	varname << "questitem";
	varname << itemid;
	// Harakiri invalid non quest related items stay in the database for 1 day others
	// should be consumed by a succesful check_hand.pl call
	InsertQuestGlobal(charid,npcid,zoneid,varname.str().c_str(), itemid, 1*86400 /*1day*/);
	
}

// Harakiri turn in was successful so we delete an item from our quest globals
void QuestManager::DeletePendingQuestItem(const char *itemid) {
	stringstream varname;
	varname << "questitem";
	varname << itemid;

	delglobal(varname.str().c_str());
}

// Harakiri multiquesting: Add older items from quest globals to perl env
vector<string> QuestManager::GetPendingQuestItems(int npcid, int zoneid) {

	char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	string tmp;
	std::vector <string> quest_items;
	
	bool ret = 	Database::Instance()->RunQuery(query, MakeAnyLenString(&query,
	"SELECT value"
	" FROM quest_globals"
	// Harakiri %% escapes %
	" WHERE (npcid=%i) && (zoneid=%i) && (name like 'questitem%%')",
		npcid,zoneid), errbuf, &result);
	if (ret)
	{
		
		while ((row = mysql_fetch_row(result)))
		{				
			tmp=row[0];
			quest_items.push_back(tmp);	
		}
		mysql_free_result(result);
		safe_delete_array(query);
	} else {

		EQC::Common::Log(EQCLog::Error,CP_QUESTS,  "Error in query for getting pending quest items %s : %s", query, errbuf);
		safe_delete_array(query);				
	}			

	return quest_items;
}



/* Inserts global variable into quest_globals table */
int QuestManager::InsertQuestGlobal(
									int charid, int npcid, int zoneid,
									const char *varname, const char *varvalue,
									int duration)
{
	char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];

	// Make duration string either "unix_timestamp(now()) + xxx" or "NULL"
	stringstream duration_ss;
	if (duration == INT_MAX)
	{
		duration_ss << "NULL";
	}
	else
	{
		duration_ss << "unix_timestamp(now()) + " << duration;
	}

	//NOTE: this should be escaping the contents of arglist
	//npcwise a malicious script can arbitrarily alter the DB
	if (!Database::Instance()->RunQuery(query, MakeAnyLenString(&query,
		"REPLACE INTO quest_globals (charid, npcid, zoneid, name, value, expdate)"
		"VALUES (%i, %i, %i, '%s', '%s', %s)",
		charid, npcid, zoneid, varname, varvalue, duration_ss.str().c_str()
		), errbuf))
	{
		cerr << "setglobal error inserting " << varname << " : " << errbuf << endl;
	}
	safe_delete_array(query);

	return 0;
}

void QuestManager::targlobal(const char *varname, const char *value, const char *duration, int qgNpcid, int qgCharid, int qgZoneid) {
	// targlobal(varname,value,duration,npcid,charid,zoneid)
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	// clean up expired vars and get rid of the one we're going to set if there
	Database::Instance()->RunQuery(query, MakeAnyLenString(&query,
		"DELETE FROM quest_globals WHERE expdate < UNIX_TIMESTAMP()"
		" || (name='%s' && npcid=%i && charid=%i && zoneid=%i))"
		,varname,qgNpcid,qgCharid,qgZoneid), errbuf);
	safe_delete_array(query);

	InsertQuestGlobal(qgCharid, qgNpcid, qgZoneid, varname, value, QGVarDuration(duration));
}

void QuestManager::delglobal(const char *varname) {
	// delglobal(varname)
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int qgZoneid=zone->GetZoneID();
	int qgCharid=0;
	int qgNpcid=owner->GetNPCTypeID();
	if (initiator && initiator->IsClient())  // some events like waypoint and spawn don't have a player involved
	{
		qgCharid=initiator->CharacterID();
	}

	else
	{
		qgCharid=-qgNpcid;		// make char id negative npc id as a fudge
	}
	if (!Database::Instance()->RunQuery(query,
	  MakeAnyLenString(&query,
	  "DELETE FROM quest_globals WHERE name='%s'"
	  " && (npcid=0 || npcid=%i) && (charid=0 || charid=%i) && (zoneid=%i || zoneid=0)",
	  varname,qgNpcid,qgCharid,qgZoneid),errbuf))
	{
		cerr << "delglobal error deleting " << varname << " : " << errbuf << endl;
	}
	safe_delete_array(query);
}

// Converts duration string to duration value (in seconds)
// Return of INT_MAX indicates infinite duration
int QuestManager::QGVarDuration(const char *fmt)
{
	int duration = 0;

	// format:	Y#### or D## or H## or M## or S## or T###### or C#######

	int len = strlen(fmt);

	// Default to no duration
	if (len < 1)
		return 0;

	// Set val to value after type character
	// e.g., for "M3924", set to 3924
	int val = atoi(&fmt[0] + 1);

	switch (fmt[0])
	{
		// Forever
		case 'F':
		case 'f':
			duration = INT_MAX;
			break;
		// Years
		case 'Y':
		case 'y':
			duration = val * 31556926;
			break;
		case 'D':
		case 'd':
			duration = val * 86400;
			break;
		// Hours
		case 'H':
		case 'h':
			duration = val * 3600;
			break;
		// Minutes
		case 'M':
		case 'm':
			duration = val * 60;
			break;
		// Seconds
		case 'S':
		case 's':
			duration = val;
			break;
		// Invalid
		default:
			duration = 0;
			break;
	}

	return duration;
}

void QuestManager::ding() {	
	if (initiator && initiator->IsClient())
		initiator->SendClientQuestCompletedFanfare();

}


void QuestManager::start(int wp) {
	if(!owner->IsNPC())
		return;
//	owner->CastToNPC()->AssignWaypoints(wp);
}

void QuestManager::stop() {
	if(!owner->IsNPC())
		return;
//	owner->CastToNPC()->StopWandering();
}

void QuestManager::pause(int duration) {
	if(!owner->IsNPC())
		return;
//	owner->CastToNPC()->PauseWandering(duration);
}

void QuestManager::moveto(float x, float y, float z, float h, bool saveguardspot) {
	if(!owner->IsNPC())
		return;
	owner->CastToNPC()->MoveTo(x, y, z);
}

void QuestManager::resume() {
	/*if(!owner->IsNPC())
		return;
	owner->CastToNPC()->ResumeWandering();*/
}


void QuestManager::setnexthpevent(int at) {
	owner->SetNextHPEvent( at );
}

void QuestManager::setnextinchpevent(int at) {
	owner->SetNextIncHPEvent( at );
}

void QuestManager::respawn(int npc_type, int grid) {
	if(!owner->IsNPC())
		return;
	//char tempa[100];
	float x,y,z,h;
	if ( !owner )
		return;

	x = owner->GetX();
	y = owner->GetY();
	z = owner->GetZ();
	h = owner->GetHeading();
	depop_npc = true;

	NPCType* tmp = 0;
	if ((tmp = Database::Instance()->GetNPCType(npc_type)))
	{
		owner = new NPC(tmp, 0, x, y, z, h);
		owner->CastToNPC()->AddLootTable();
		entity_list.AddNPC(owner->CastToNPC(),true);
		/*if(grid > 0)
			owner->CastToNPC()->AssignWaypoints(grid);
*/
		owner->SendPosUpdate();
	}
}

void QuestManager::set_proximity(float minx, float maxx, float miny, float maxy, float minz, float maxz) {
	if(!owner->IsNPC())
		return;

	/*entity_list.AddProximity(owner->CastToNPC());

	owner->CastToNPC()->proximity->min_x = minx;
	owner->CastToNPC()->proximity->max_x = maxx;
	owner->CastToNPC()->proximity->min_y = miny;
	owner->CastToNPC()->proximity->max_y = maxy;
	owner->CastToNPC()->proximity->min_z = minz;
	owner->CastToNPC()->proximity->max_z = maxz;

	owner->CastToNPC()->proximity->say = ((PerlembParser *)parse)->HasQuestSub(owner->CastToNPC()->GetNPCTypeID(),"EVENT_PROXIMITY_SAY");

	if(owner->CastToNPC()->proximity->say)
		HaveProximitySays = true;
		*/
}

void QuestManager::clear_proximity() {
	/*if(!owner->IsNPC())
		return;
	safe_delete(owner->CastToNPC()->proximity);
	entity_list.RemoveProximity(owner->GetID());
	*/
}

void QuestManager::setanim(int npc_type, int animnum) {
	//Cisyouc: adds appearance changes
	/*Mob* thenpc = entity_list.GetMobByNpcTypeID(npc_type);
	if(animnum < 0 || animnum >= _eaMaxAppearance)
		return;
	thenpc->SetAppearance(EmuAppearance(animnum));*/
}




void QuestManager::npcrace(int race_id)
{
		owner->SendIllusionPacket(race_id);
}

void QuestManager::npcgender(int gender_id)
{
		owner->SendIllusionPacket(owner->GetRace(), gender_id);
}
void QuestManager::npcsize(int newsize)
{
				//owner->size=newsize;
}
void QuestManager::npctexture(int newtexture)
{
			owner->SendIllusionPacket(owner->GetRace(), 0xFF, newtexture);
}

void QuestManager::playerrace(int race_id)
{
			initiator->SendIllusionPacket(race_id);
}

void QuestManager::playergender(int gender_id)
{
			initiator->SendIllusionPacket(initiator->GetRace(), gender_id);
}

void QuestManager::playertexture(int newtexture)
{
			initiator->SendIllusionPacket(initiator->GetRace(), 0xFF, newtexture);
}


int QuestManager::getlevel(uint8 type)
{
	
	
	return (initiator->GetLevel());
}

void QuestManager::CreateGroundObject(int32 itemid, float x, float y, float z, float heading, int32 decay_time)
{
	//entity_list.CreateGroundObject(itemid, x, y, z, heading, decay_time);
}

 





uint8 QuestManager::FactionValue() 
{
	FACTION_VALUE oldfac;
	uint8 newfac = 0;
	if(initiator && owner->IsNPC()) {
		oldfac = initiator->GetFactionLevel(initiator->GetID(), owner->GetID(), initiator->GetRace(), initiator->GetClass(), initiator->GetDeity(), owner->CastToNPC()->GetPrimaryFactionID(), owner);
		
		// now, reorder the faction to have it make sense (higher values are better)
		switch (oldfac) {
			case FACTION_SCOWLS:
				newfac = 1;
				break;
			case FACTION_THREATENLY:
				newfac = 2;
				break;
			case FACTION_DUBIOUS:
				newfac = 3;
				break;
			case FACTION_APPREHENSIVE:
				newfac = 4;
				break;
			case FACTION_INDIFFERENT:
				newfac = 5;
				break;
			case FACTION_AMIABLE:
				newfac = 6;
				break;
			case FACTION_KINDLY:
				newfac = 7;
				break;
			case FACTION_WARMLY:
				newfac = 8;
				break;
			case FACTION_ALLY:
				newfac = 9;
				break;
		}
	}
	
	return newfac;
}



