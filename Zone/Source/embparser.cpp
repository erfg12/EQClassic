#ifndef EMBPARSER_CPP
#define EMBPARSER_CPP

#ifdef EMBPERL

#include "logger.h"
#include "entity.h"
#include "npc.h"
#include "mob.h"
#include "client.h"
#include "features.h"
#include "embparser.h"
#include "questmgr.h"
#include "zone.h"
#include "Client_Commands.h"
#include "seperator.h"
#include "MiscFunctions.h"

#include <algorithm>

PerlembParser* perlParser = 0;

extern Zone*			zone;

//these MUST be in the same order as the QuestEventID enum
const char *QuestEventSubroutines[_LargestEventID] = {
	"EVENT_SAY",
	"EVENT_ITEM",
	"EVENT_DEATH",
	"EVENT_SPAWN",
	"EVENT_ATTACK",
	"EVENT_COMBAT",
	"EVENT_AGGRO",
	"EVENT_SLAY",
	"EVENT_NPC_SLAY",
	"EVENT_WAYPOINT",
	"EVENT_TIMER",
	"EVENT_SIGNAL",
	"EVENT_HP",
	"EVENT_ENTER",
	"EVENT_EXIT",
	"EVENT_ENTERZONE",
	"EVENT_CLICKDOOR",
	"EVENT_LOOT",
	"EVENT_ZONE",
	"EVENT_LEVEL_UP",
	"EVENT_KILLED_MERIT",
	"EVENT_CAST_ON",
	"EVENT_TASKACCEPTED",
	"EVENT_TASK_STAGE_COMPLETE",
	"EVENT_AGGRO_SAY",
	"EVENT_PLAYER_PICKUP",
	"EVENT_POPUPRESPONSE",
	"EVENT_PROXIMITY_SAY",
	"EVENT_CAST",
	"EVENT_SCALE_CALC"
};

void PerlembParser::Init() {
	perlParser = new PerlembParser();
}


PerlembParser::PerlembParser(void) : DEFAULT_QUEST_PREFIX("default")
{
	perl = NULL;
	eventQueueProcessing = false;	
}

PerlembParser::~PerlembParser()
{
	safe_delete(perl);
}

void PerlembParser::ExportVar(const char * pkgprefix, const char * varname, const char * value) const
{
	if(!perl)
		return;
	//this crap cant possibly throw anything in its current state... oh well
	try
	{
		perl->setstr(std::string(pkgprefix).append("::").append(varname).c_str(), value);
		//todo: consider replacing ' w/ ", so that values can be expanded on the perl side
// MYRA - fixed eval in ExportVar per Eglin
//		perl->eval(std::string("$").append(pkgprefix).append("::").append(varname).append("=q(").append(value).append(");").c_str());
//end Myra

	}
	catch(const char * err)
	{
		EQC::Common::Log(EQCLog::Error,CP_QUESTS,"Error exporting var: %s", err);
	}
}

// Exports key-value pairs to a hash named pkgprefix::hashname
void PerlembParser::ExportHash(const char *pkgprefix, const char *hashname, std::map<string,string> &vals)
{
	if (!perl)
		return;

	try
	{
		perl->sethash(
			std::string(pkgprefix).append("::").append(hashname).c_str(),
			vals
		);
	} catch(const char * err) {
		EQC::Common::Log(EQCLog::Error,CP_QUESTS, "Error exporting hash: %s", err);
	}
}

void PerlembParser::ExportVar(const char * pkgprefix, const char * varname, int value) const
{

	if(!perl)
		return;
	//this crap cant possibly throw anything in its current state... oh well
	try {
		perl->seti(std::string(pkgprefix).append("::").append(varname).c_str(), value);

	} catch(const char * err) {
		EQC::Common::Log(EQCLog::Error,CP_QUESTS,"Error exporting var: %s", err);
	}
}

void PerlembParser::ExportVar(const char * pkgprefix, const char * varname, unsigned int value) const
{

	if(!perl)
		return;
	//this crap cant possibly throw anything in its current state... oh well
	try {
		perl->seti(std::string(pkgprefix).append("::").append(varname).c_str(), value);

	} catch(const char * err) {
		EQC::Common::Log(EQCLog::Error,CP_QUESTS, "Error exporting var: %s", err);
	}
}

void PerlembParser::ExportVar(const char * pkgprefix, const char * varname, float value) const
{

	if(!perl)
		return;
	//this crap cant possibly throw anything in its current state... oh well
	try {
		perl->setd(std::string(pkgprefix).append("::").append(varname).c_str(), value);
	} catch(const char * err) {
		EQC::Common::Log(EQCLog::Error,CP_QUESTS,"Error exporting var: %s", err);
	}
}

void PerlembParser::ExportVarComplex(const char * pkgprefix, const char * varname, const char * value) const
{

	if(!perl)
		return;
	try
	{
		//todo: consider replacing ' w/ ", so that values can be expanded on the perl side
// MYRA - fixed eval in ExportVar per Eglin
		perl->eval(std::string("$").append(pkgprefix).append("::").append(varname).append("=").append(value).append(";").c_str());
//end Myra

	}
	catch(const char * err)
	{ //todo: consider rethrowing
		EQC::Common::Log(EQCLog::Error,CP_QUESTS, "Error exporting var: %s", err);
	}
}

void PerlembParser::HandleQueue() {
	if(eventQueueProcessing)
		return;
	eventQueueProcessing = true;

	while(!eventQueue.empty()) {
		EventRecord e = eventQueue.front();
		eventQueue.pop();

		EventCommon(e.event, e.objid, e.data.c_str(), e.npcmob, e.iteminst, e.mob, e.extradata);
	}

	eventQueueProcessing = false;
}

void PerlembParser::EventCommon(QuestEventID event, int32 objid, const char * data, NPC* npcmob, ItemInst* iteminst, Mob* mob, int32 extradata)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if(!perl)
		return;

	if(event >= _LargestEventID)
		return;

	if(perl->InUse()) {
		//queue the event for later.
		EventRecord e;
		e.event = event;
		e.objid = objid;
		if(data != NULL)
			e.data = data;
		e.npcmob = npcmob;
		e.iteminst = iteminst;
		e.mob = mob;
		e.extradata = extradata;
		eventQueue.push(e);
		return;
	}

	bool isPlayerQuest = false;
	bool isItemQuest = false;
	if(!npcmob && mob) {
		if(!iteminst) 
			isPlayerQuest = true;
		else 
			isItemQuest = true;
	}

	string packagename;
	
	if(!isPlayerQuest && !isItemQuest){
		packagename = GetPkgPrefix(objid);

		if(!isloaded(packagename.c_str()))
		{
			LoadScript(objid, zone->GetShortName());
		}
	}
	else if(isItemQuest) {
		const Item_Struct* item = iteminst->GetItem();
		if (!item) return;

		if (event == EVENT_SCALE_CALC) {
			/*packagename = item->CharmFile;
			if(!isloaded(packagename.c_str())) {
				LoadItemScript(iteminst, packagename, itemQuestScale);
			}*/
		}
		else {
			packagename = "item_";
			packagename += itoa(objid);
			if(!isloaded(packagename.c_str()))
				LoadItemScript(iteminst, packagename, itemQuestID);
		}
	}
	else {
		packagename = "player";
		packagename += "_"; 
		packagename += zone->GetShortName();

		if(!isloaded(packagename.c_str()))
		{
			LoadPlayerScript(zone->GetShortName());
		}
	}

	const char *sub_name = QuestEventSubroutines[event];

	//make sure the sub we need even exists before we even do all this crap.
	if(!perl->SubExists(packagename.c_str(), sub_name)) {
		return;
	}

	int charid=0;

	if (mob && mob->IsClient()) {  // some events like waypoint and spawn don't have a player involved
		charid=mob->CastToClient()->CharacterID();
	} else {
		charid=-npcmob->GetNPCTypeID();		// make char id negative npc id as a fudge
	}

	ExportVar(packagename.c_str(), "charid", charid);

	if(!isPlayerQuest && !isItemQuest){
		//only export globals if the npcmob has the qglobal flag
		if(npcmob && npcmob->GetQglobal()){
			// Delete expired global variables
			Database::Instance()->RunQuery(query, MakeAnyLenString(&query,
				"DELETE FROM quest_globals WHERE expdate < UNIX_TIMESTAMP()"), errbuf);
			safe_delete_array(query);

			map<string, string> globhash;

			// Load global variables
			bool ret = 	Database::Instance()->RunQuery(query, MakeAnyLenString(&query,
			"SELECT name,value"
			" FROM quest_globals"
			" WHERE (npcid=%i || npcid=0) && (charid=%i || charid=0) && (zoneid=%i || zoneid=0)",
				npcmob->GetNPCTypeID(),charid,zone->GetZoneID()), errbuf, &result);
			if (ret)
			{
				while ((row = mysql_fetch_row(result)))
				{
					globhash[row[0]] = row[1];

					// DEPRECATED: Export variables as $var in addition to hash
					ExportVar(packagename.c_str(), row[0], row[1]);
				}
				mysql_free_result(result);
			} else {
	
				cerr << "Error in query '" << query << "' " << errbuf << endl;
				safe_delete_array(query);				
			}
			safe_delete_array(query);

			// Put key-value pairs in perl hash
			ExportHash(packagename.c_str(), "qglobals", globhash);
		}
	}
	else{
		//only export globals if the npcmob has the qglobal flag
		if(mob){
			// Delete expired global variables
			Database::Instance()->RunQuery(query, MakeAnyLenString(&query,
				"DELETE FROM quest_globals WHERE expdate < UNIX_TIMESTAMP()"), errbuf);
			safe_delete_array(query);

			map<string, string> globhash;

			// Load global variables
			bool ret = Database::Instance()->RunQuery(query, MakeAnyLenString(&query,
			"SELECT name,value"
			" FROM quest_globals"
			" WHERE (npcid=0) && (charid=%i || charid=0) && (zoneid=%i || zoneid=0)",
				charid,zone->GetZoneID()), errbuf, &result);
			if (ret)
			{
				while ((row = mysql_fetch_row(result)))
				{
					globhash[row[0]] = row[1];

					// DEPRECATED: Export variables as $var in addition to hash
					ExportVar(packagename.c_str(), row[0], row[1]);
				}
				mysql_free_result(result);
			} else {
	
				cerr << "Error in query '" << query << "' " << errbuf << endl;
				safe_delete_array(query);				
			}

			safe_delete_array(query);

			// Put key-value pairs in perl hash
			ExportHash(packagename.c_str(), "qglobals", globhash);
		}
	}

	int8 fac = 0;
	if (mob && mob->IsClient()) {
		ExportVar(packagename.c_str(), "uguild_id", mob->CastToClient()->GuildEQID());
		ExportVar(packagename.c_str(), "uguildrank", mob->CastToClient()->GuildRank());
		ExportVar(packagename.c_str(), "status", mob->CastToClient()->Admin());
	}

	if(!isPlayerQuest && !isItemQuest){
		if (mob && npcmob && mob->IsClient() && npcmob->IsNPC()) {
			Client* client = mob->CastToClient();
			NPC* npc = npcmob->CastToNPC();

			// Need to figure out why one of these casts would fail..
			if (client && npc) {
				fac = client->GetFactionLevel(client->GetID(), npcmob->GetID(), client->GetRace(), client->GetClass(), DEITY_AGNOSTIC, npc->GetPrimaryFactionID(), npcmob);
			}
			else if (!client) {
				EQC::Common::Log(EQCLog::Debug,CP_QUESTS,  "WARNING: cast failure on mob->CastToClient()");
			}
			else if (!npc) {
				EQC::Common::Log(EQCLog::Debug,CP_QUESTS,  "WARNING: cast failure on npcmob->CastToNPC()");
			}
		}
	}
	if (mob) {
		ExportVar(packagename.c_str(), "name", mob->GetName());
		ExportVar(packagename.c_str(), "race", GetRaceName(mob->GetRace()));
		ExportVar(packagename.c_str(), "class",EQC::Common::GetClassName(mob->GetClass()));
		ExportVar(packagename.c_str(), "ulevel", mob->GetLevel());
		ExportVar(packagename.c_str(), "userid", mob->GetID());
	}

	if(!isPlayerQuest && !isItemQuest){
		if (npcmob)
		{
			ExportVar(packagename.c_str(), "mname", npcmob->GetProperName());
			// MYRA - added vars $mobid & $mlevel per Eglin
			ExportVar(packagename.c_str(), "mobid", npcmob->GetID());
			ExportVar(packagename.c_str(), "mlevel", npcmob->GetLevel());
			//end Myra
			// hp event
			ExportVar(packagename.c_str(), "hpevent", npcmob->GetNextHPEvent());
			ExportVar(packagename.c_str(), "inchpevent", npcmob->GetNextIncHPEvent());
			ExportVar(packagename.c_str(), "hpratio",npcmob->GetHPRatio());
			// sandy bug fix
			ExportVar(packagename.c_str(), "x", npcmob->GetX() );
			ExportVar(packagename.c_str(), "y", npcmob->GetY() );
			ExportVar(packagename.c_str(), "z", npcmob->GetZ() );
			ExportVar(packagename.c_str(), "h", npcmob->GetHeading() );
			if ( npcmob->GetTarget() ) {
				ExportVar(packagename.c_str(), "targetid", npcmob->GetTarget()->GetID());
				ExportVar(packagename.c_str(), "targetname", npcmob->GetTarget()->GetName());
			}
		}

		if (fac) {
			ExportVar(packagename.c_str(), "faction", itoa(fac));
		}
	}

	if (zone) {
		// SCORPIOUS2K- added variable zoneid
		ExportVar(packagename.c_str(), "zoneid", zone->GetZoneID());
		ExportVar(packagename.c_str(), "zoneln", zone->GetLongName());
		ExportVar(packagename.c_str(), "zonesn", zone->GetShortName());
		TimeOfDay_Struct eqTime;
		
		Database::Instance()->LoadTimeOfDay(&eqTime.hour, &eqTime.day, &eqTime.month, &eqTime.year);
		eqTime.minute = 0;

		ExportVar(packagename.c_str(), "zonehour", eqTime.hour - 1);
		ExportVar(packagename.c_str(), "zonemin", eqTime.minute);
		ExportVar(packagename.c_str(), "zonetime", (eqTime.hour - 1) * 100 + eqTime.minute);
		ExportVar(packagename.c_str(), "zoneweather", zone->zone_weather);
	}

// $hasitem - compliments of smogo
#define HASITEM_FIRST 0
#define HASITEM_LAST 29 // this includes worn plus 8 base slots
#define HASITEM_ISNULLITEM(item) ((item==-1) || (item==0))

	if(mob && mob->IsClient())
	{
		string hashname = packagename + std::string("::hasitem");
#if EQDEBUG >= 7
		EQC::Common::Log(EQCLog::Debug,CP_QUESTS, "starting hasitem, on : %s",hashname.c_str() );
#endif

		//start with an empty hash
		perl->eval(std::string("%").append(hashname).append(" = ();").c_str());

		for(int slot=HASITEM_FIRST; slot<=HASITEM_LAST;slot++)
		{
			char *hi_decl=NULL;
			int itemid=mob->CastToClient()->GetItemAt(slot);
			if(!HASITEM_ISNULLITEM(itemid))
			{
				MakeAnyLenString(&hi_decl, "push (@{$%s{%d}},%d);",hashname.c_str(),itemid,slot);
//Father Nitwit: this is annoying
#if EQDEBUG >= 7
				EQC::Common::Log(EQCLog::Debug,CP_QUESTS,  "declare hasitem : %s",hi_decl);
#endif
				perl->eval(hi_decl);
				safe_delete_array(hi_decl);
			}
		}
	}

	//do any event-specific stuff...
	switch (event) {
		case EVENT_SAY: {
			npcmob->FaceTarget(mob);
			ExportVar(packagename.c_str(), "data", objid);
			ExportVar(packagename.c_str(), "text", data);
			ExportVar(packagename.c_str(), "langid", extradata);
			break;
		}
		case EVENT_ITEM: {
			npcmob->FaceTarget(mob);
			//this is such a hack... why arnt these just set directly..
			ExportVar(packagename.c_str(), "item1", GetVar("item1", objid).c_str());
			ExportVar(packagename.c_str(), "item2", GetVar("item2", objid).c_str());
			ExportVar(packagename.c_str(), "item3", GetVar("item3", objid).c_str());
			ExportVar(packagename.c_str(), "item4", GetVar("item4", objid).c_str());
			ExportVar(packagename.c_str(), "copper", GetVar("copper", objid).c_str());
			ExportVar(packagename.c_str(), "silver", GetVar("silver", objid).c_str());
			ExportVar(packagename.c_str(), "gold", GetVar("gold", objid).c_str());
			ExportVar(packagename.c_str(), "platinum", GetVar("platinum", objid).c_str());
			string hashname = packagename + std::string("::itemcount");
			perl->eval(std::string("%").append(hashname).append(" = ();").c_str());
			perl->eval(std::string("++$").append(hashname).append("{$").append(packagename).append("::item1};").c_str());
			perl->eval(std::string("++$").append(hashname).append("{$").append(packagename).append("::item2};").c_str());
			perl->eval(std::string("++$").append(hashname).append("{$").append(packagename).append("::item3};").c_str());
			perl->eval(std::string("++$").append(hashname).append("{$").append(packagename).append("::item4};").c_str());

			// Harakiri multiquesting: Add older items from quest globals to perl env
			// Harakiri These items are only removed ever when you add a function call to the end of check_handin
			// in check_handin.pl
			/*
				 # delete pending quest items (multiquesting successful)
				 foreach my $req (keys %required) {
					 quest::DeletePendingQuestItem($req);	    
				 }
			     
				 return 1;
		   */
			std::vector <string> quest_items = quest_manager.GetPendingQuestItems(npcmob->GetNPCTypeID(),zone->GetZoneID());			
			int i = 5; // add older items after current ones
			for(int a=0;a<quest_items.size();a++){
				
				EQC::Common::Log(EQCLog::Debug,CP_QUESTS,  "Adding pending quest item id %s to perl env",quest_items[a].c_str());
				string varname = "item"+(string)itoa(i);
				AddVar(varname+"."+(string)itoa(objid),quest_items[a].c_str());
				ExportVar(packagename.c_str(), varname.c_str(), quest_items[a].c_str());
				
				string hashname = packagename + std::string("::itemcount");
				perl->eval(std::string("++$").append(hashname).append("{$").append(packagename).append("::").append(varname).append("};").c_str());
				i++;
			}						

			// Harakiri multiquesting: add new items to the quest global table, after getting the old ones to not have duplicate items			
			for(int i=1; i < 5; i++)  {
				string varname = "item"+(string)itoa(i);
				string var = GetVar(varname,objid);
				if(!var.empty()) {
					string itemid = var;					
					quest_manager.AddPendingQuestItem(charid,objid,zone->GetZoneID(),itemid.c_str());
				}
			}
					

			break;
		}
		case EVENT_WAYPOINT: {
			ExportVar(packagename.c_str(), "wp", data);
			break;
		}
		case EVENT_HP: {
			break;
		}
		case EVENT_TIMER: {
			ExportVar(packagename.c_str(), "timer", data);
			break;
		}
		case EVENT_SIGNAL: {
			ExportVar(packagename.c_str(), "signal", data);
			break;
		}
		case EVENT_NPC_SLAY: {
			ExportVar(packagename.c_str(), "killed", mob->GetNPCTypeID());
			break;
		}
		case EVENT_COMBAT: {
			ExportVar(packagename.c_str(), "combat_state", data);
			break;
		}

		case EVENT_CLICKDOOR: {
			ExportVar(packagename.c_str(), "doorid", data);
			break;
		}

		case EVENT_LOOT:{
			char *data2 = 0;
			MakeAnyLenString(&data2, data);
			Seperator *sep = new Seperator(data2);
			ExportVar(packagename.c_str(), "looted_id", sep->arg[0]);
			ExportVar(packagename.c_str(), "looted_charges", sep->arg[1]);
			safe_delete(sep);
			break;
		}

		case EVENT_ZONE:{
			ExportVar(packagename.c_str(), "target_zone_id", data);
			break;
		}
		
		case EVENT_CAST_ON:
		case EVENT_CAST:{
			ExportVar(packagename.c_str(), "spell_id", data);
			break;
		}		

		case EVENT_TASKACCEPTED:{
			ExportVar(packagename.c_str(), "task_id", data);
			break;
		}

		case EVENT_TASK_STAGE_COMPLETE:{
			char *data2 = 0;
			MakeAnyLenString(&data2, data);
			Seperator *sep = new Seperator(data2);
			ExportVar(packagename.c_str(), "task_id", sep->arg[0]);
			ExportVar(packagename.c_str(), "activity_id", sep->arg[1]);
			safe_delete(sep);
			break;
		}

		case EVENT_PLAYER_PICKUP:{
			ExportVar(packagename.c_str(), "picked_up_id", data);
			break;		
		}

		case EVENT_AGGRO_SAY: {
			ExportVar(packagename.c_str(), "data", objid);
			ExportVar(packagename.c_str(), "text", data);
			ExportVar(packagename.c_str(), "langid", extradata);
			break;
		}
		case EVENT_POPUPRESPONSE:{
			ExportVar(packagename.c_str(), "popupid", data);
			break;
		}
		case EVENT_PROXIMITY_SAY: {
			ExportVar(packagename.c_str(), "data", objid);
			ExportVar(packagename.c_str(), "text", data);
			ExportVar(packagename.c_str(), "langid", extradata);
			break;
		}
		case EVENT_SCALE_CALC: {
			ExportVar(packagename.c_str(), "itemid", objid);
			ExportVar(packagename.c_str(), "itemname", iteminst->GetItem()->name);
			break;
		}
		//nothing special about these events
		case EVENT_DEATH:
		case EVENT_SPAWN:
		case EVENT_ATTACK:
		case EVENT_SLAY:
		case EVENT_AGGRO:
		case EVENT_ENTER:
		case EVENT_EXIT:
		case EVENT_ENTERZONE:
		case EVENT_LEVEL_UP:
		case EVENT_KILLED_MERIT:
			break;

		default: {
			// should we do anything here?
			break;
		}
	}

	if(event == EVENT_ITEM) {
		//LogItemVars("Pre Execute:",objid);
	}

	if(isPlayerQuest){
		SendCommands(packagename.c_str(), sub_name, 0, mob, mob, NULL);
	}
	else if(isItemQuest) {
		SendCommands(packagename.c_str(), sub_name, 0, mob, mob, iteminst);
	}
	else {
		SendCommands(packagename.c_str(), sub_name, objid, npcmob, mob, NULL);
	}
	
	if(event == EVENT_ITEM) {
		//LogItemVars("Post Exceute:",objid);
	}

	DelChatAndItemVars(objid);

	if(event == EVENT_ITEM) {
		//LogItemVars("Post Delete",objid);
	}


	//now handle any events that cropped up...
	HandleQueue();
}

void PerlembParser::DelChatAndItemVars(int32 npcid)
{
//	MyListItem <vars> * Ptr;
	string temp;
	int i=0;
	for (i=0;i<10;i++)
	{
		temp = (string)itoa(i) + "." + (string)itoa(npcid);		
		DeleteVar(temp);
		temp = (string)itoa(i) + "-." + (string)itoa(npcid);
		DeleteVar(temp);
	}
	for (i=1;i<5;i++)
	{		
		temp = "item"+(string)itoa(i) + "." + (string)itoa(npcid);	
		DeleteVar(temp);
		temp = "item"+(string)itoa(i) + ".stack." + (string)itoa(npcid);
		DeleteVar(temp);
	}
}


void PerlembParser::LogItemVars(string info, int32 npcid)
{
	int i=0;
	for (i=1;i<20;i++) {
		string varname = "item"+(string)itoa(i);
		string var = GetVar(varname,npcid);
		EQC::Common::Log(EQCLog::Debug,CP_QUESTS,"%s npcid %d GetVar(%s) = '%s'\n", info.c_str(),npcid, varname.c_str(), var.c_str());
	}
}

void PerlembParser::Event(QuestEventID event, int32 npcid, const char* data, NPC* npcmob, Mob* mob, int32 extradata) {
	EventCommon(event, npcid, data, npcmob, (ItemInst*)NULL, mob, extradata);
}

void PerlembParser::Event(QuestEventID event, int32 itemid, const char* data, ItemInst* iteminst, Mob* mob, int32 extradata) {
	EventCommon(event, itemid, data, (NPC*)NULL, iteminst, mob, extradata);
}

void PerlembParser::ReloadQuests() {
	
	command_clear_perl();

	try {
		if(perl == NULL)
			perl = new Embperl;
		else
			perl->Reinit();
		// FIXME: map_funs is broken.
		//map_funs();
		
	}
	catch(const char * msg) {
		EQC::Common::Log(EQCLog::Error,CP_QUESTS,"Error re-initializing perlembed: %s", msg);
		if(perl != NULL) {			
			perl = NULL;
		}		
		throw msg;
	}
	
	try {
		LoadScript(0, NULL);
	}
	catch(const char * err) {
		EQC::Common::Log(EQCLog::Error,CP_QUESTS,"Error loading default script: %s", err);
	}

	hasQuests.clear();
	playerQuestLoaded.clear();
	itemQuestLoaded.clear();
	varlist.clear();
}

int PerlembParser::LoadScript(int npcid, const char * zone, Mob* activater)
{
	if(!perl)
		return(0);

	//we have allready tried to load this quest...
	if(hasQuests.count(npcid) == 1) {
		return(1);
	}

	string filename= "quests/", packagename = GetPkgPrefix(npcid);
	//each package name is of the form qstxxxx where xxxx = npcid (since numbers alone are not valid package names)
	questMode curmode = questDefault;
	FILE *tmpf;

	if(!npcid || !zone)
	{
		filename += DEFAULT_QUEST_PREFIX;
		filename += ".pl";
		curmode = questDefault;
	}
	else
	{
		filename += zone;
		filename += "/";
#ifdef QUEST_SCRIPTS_BYNAME
		string bnfilename = filename;
#endif
		filename += itoa(npcid);
		filename += ".pl";
		curmode = questByID;

#ifdef QUEST_SCRIPTS_BYNAME
		//Father Nitwit's naming hack.
		//untested on windows...

		//assuming name limit stays 64 chars.
		char tmpname[64];
		int count0 = 0;
		bool filefound = false;
		tmpf = fopen(filename.c_str(), "r");
		if(tmpf != NULL) {
			fclose(tmpf);
			filefound = true;
		}

	    EQC::Common::Log(EQCLog::Debug, CP_QUESTS,"	tried '%s': %d", filename.c_str(), filefound);

		tmpname[0] = 0;
		//if there is no file for the NPC's ID, try for the NPC's name
		if(! filefound) {
			//revert to just path
			filename = bnfilename;
			const NPCType *npct = Database::Instance()->GetNPCType(npcid);
			if(npct == NULL) {
				EQC::Common::Log(EQCLog::Debug,CP_QUESTS, "	no npc type");
				//revert and go on with life
				filename += itoa(npcid);
				filename += ".pl";
				curmode = questByID;
			} else {
				//trace out the ` characters, turn into -

				int nlen = strlen(npct->name);
				if(nlen < 64) {	//just to make sure
					int r;
					//this should get our NULL as well..
					for(r = 0; r <= nlen; r++) {
						tmpname[r] = npct->name[r];

						//watch for 00 delimiter
						if(tmpname[r] == '0') {
							count0++;
							if(count0 > 1) {	//second '0'
								//stop before previous 0
								tmpname[r-1] = '\0';
								break;
							}
						} else {
							count0 = 0;
						}

						//rewrite ` to be more file name friendly
						if(tmpname[r] == '`')
							tmpname[r] = '-';

					}
					filename += tmpname;
					filename += ".pl";
					curmode = questByName;
				} else {
//LogFile->write(EQEMuLog::Debug, "	namelen too long");
					//revert and go on with life, again
					filename += itoa(npcid);
					filename += ".pl";
					curmode = questByID;
				}
			}
		}

	#ifdef QUEST_TEMPLATES_BYNAME
		bool filefound2 = false;
		tmpf = fopen(filename.c_str(), "r");
		if(tmpf != NULL) {
			fclose(tmpf);
			filefound2 = true;
		}

		EQC::Common::Log(EQCLog::Debug,CP_QUESTS,"Quest File '%s' exists: %d", filename.c_str(), filefound2);

		//if there is no file for the NPC's ID or name,
		//try for the NPC's name in the templates directory
		//only works if we have gotten the NPC's name above
		if(! filefound && ! filefound2) {
			if(tmpname[0] != 0) {
				//revert to just path
				filename = "quests/";
				filename += QUEST_TEMPLATES_DIRECTORY;
				filename += "/";
				filename += tmpname;
				filename += ".pl";
				curmode = questTemplate;
				//LogFile->write(EQEMuLog::Debug, "	template '%s'", filename.c_str(), filefound2);
			} else {
				//LogFile->write(EQEMuLog::Debug, "	no template name");
				filename += itoa(npcid);
				filename += ".pl";
				curmode = questDefault;
			}
		}
	#endif	//QUEST_TEMPLATES_BYNAME

#endif //QUEST_SCRIPTS_BYNAME

	}

	//check for existance of quest file before trying to make perl load it.
	tmpf = fopen(filename.c_str(), "r");
	if(tmpf == NULL) {
		//the npc has no qst file, attach the defaults
		std::string setdefcmd = "$";
			setdefcmd += packagename;
			setdefcmd += "::isdefault = 1;";
		perl->eval(setdefcmd.c_str());
		setdefcmd = "$";
			setdefcmd += packagename;
			setdefcmd += "::isloaded = 1;";
		perl->eval(setdefcmd.c_str());
		hasQuests[npcid] = questDefault;
		return(1);
	} else {
		fclose(tmpf);
	}

//  todo: decide whether or not to delete the package to allow for script refreshes w/o restarting the server
//  remember to guard against deleting the default package, on a similar note... consider deleting packages upon zone change
//	try { perl->eval(std::string("delete_package(\"").append(packagename).append("\");").c_str()); }
//	catch(...) {/*perl balked at us trynig to delete a non-existant package... no big deal.*/}

	try {
		perl->eval_file(packagename.c_str(), filename.c_str());
	}
	catch(const char * err)
	{
		//try to reduce some of the console spam...
		//todo: tweak this to be more accurate at deciding what to filter (we don't want to gag legit errors)
//		if(!strstr(err,"No such file or directory"))
			EQC::Common::Log(EQCLog::Error,CP_QUESTS,"WARNING: error compiling quest file %s: %s (reverting to default questfile)", filename.c_str(), err);
	}

	//todo: change this to just read eval_file's %cache - duh!
	if(!isloaded(packagename.c_str()))
	{
		//the npc has no qst file, attach the defaults
		std::string setdefcmd = "$";
			setdefcmd += packagename;
			setdefcmd += "::isdefault = 1;";
		perl->eval(setdefcmd.c_str());
		setdefcmd = "$";
			setdefcmd += packagename;
			setdefcmd += "::isloaded = 1;";
		perl->eval(setdefcmd.c_str());
		curmode = questDefault;
	}

	hasQuests[npcid] = curmode;
	return(1);
}

int PerlembParser::LoadPlayerScript(const char *zone)
{
	if(!perl)
		return(0);

	if(playerQuestLoaded.count(zone) == 1) {
		return(1);
	}

	string filename= "quests/";
	filename += zone;
	filename += "/player.pl";
	string packagename = "player";
	packagename += "_";
	packagename += zone;

	try {
		perl->eval_file(packagename.c_str(), filename.c_str());
	}
	catch(const char * err)
	{
			EQC::Common::Log(EQCLog::Error,CP_QUESTS, "WARNING: error compiling quest file %s: %s", filename.c_str(), err);
	}
	//todo: change this to just read eval_file's %cache - duh!
	if(!isloaded(packagename.c_str()))
	{
		filename = "quests/";
		filename += QUEST_TEMPLATES_DIRECTORY;
		filename += "/player.pl";
		try {
			perl->eval_file(packagename.c_str(), filename.c_str());
		}
		catch(const char * err)
		{
				EQC::Common::Log(EQCLog::Error,CP_QUESTS, "WARNING: error compiling quest file %s: %s", filename.c_str(), err);
		}
		if(!isloaded(packagename.c_str()))
		{
			playerQuestLoaded[zone] = pQuestUnloaded;
			return 0;
		}
	}

	if(perl->SubExists(packagename.c_str(), "EVENT_CAST")) 
		playerQuestLoaded[zone] = pQuestEventCast;
	else 
		playerQuestLoaded[zone] = pQuestLoaded;
	return 1;
}

int PerlembParser::LoadItemScript(ItemInst* iteminst, string packagename, itemQuestMode Qtype) {
	if(!perl)
		return 0;

	// if we've already tried to load it, don't try again
	if(itemQuestLoaded.count(packagename) == 1)
		return 1;

	string filename = "quests/items/";
	/*if(Qtype == itemQuestScale)
		filename += packagename;
	else if(Qtype == itemQuestLore) {
		filename += "lore_";
		filename += itoa(iteminst->GetItem()->LoreGroup);
	}
	else*/
		filename += itoa(iteminst->GetID());
	filename += ".pl";
	printf("Loading file %s\n",filename.c_str());

	try {
		perl->eval_file(packagename.c_str(), filename.c_str());
	}
	catch(const char* err) {
		EQC::Common::Log(EQCLog::Error,CP_QUESTS, "WARNING: error compiling quest file %s: %s", filename.c_str(), err);
	}

	if(!isloaded(packagename.c_str())) {
		itemQuestLoaded[packagename] = Qtype;
		return 0;
	}

	itemQuestLoaded[packagename] = itemQuestUnloaded;
	return 1;
}

bool PerlembParser::isloaded(const char *packagename) const {
	char buffer[120];
	snprintf(buffer, 120, "$%s::isloaded", packagename);
	if(!perl->VarExists(packagename, "isloaded"))
		return(false);
	return perl->geti(buffer);
}


bool PerlembParser::HasQuestSub(int32 npcid, const char *subname) {
	sint32 qstID = -1;

	if (qstID == -1) {
		if(!LoadScript(npcid, zone->GetShortName()))
			return(false);
	}

	string packagename = GetPkgPrefix(npcid);

	return(perl->SubExists(packagename.c_str(), subname));
}

bool PerlembParser::PlayerHasQuestSub(const char *subname) {

	string packagename = "player_";
	packagename += zone->GetShortName();

	if(playerQuestLoaded.count(zone->GetShortName()) == 0)
		LoadPlayerScript(zone->GetShortName());
		
	if(subname == "EVENT_CAST")
		return (playerQuestLoaded[zone->GetShortName()] == pQuestEventCast);
	
	return(perl->SubExists(packagename.c_str(), subname));
}

//utility - return something of the form "qst1234"...
//will return "qst[DEFAULT_QUEST_PREFIX]" if the npc in question has no script of its own or failed to compile and defaultOK is set to true
std::string PerlembParser::GetPkgPrefix(int32 npcid, bool defaultOK)
{
	char buf[32];
	snprintf(buf, 32, "qst%lu", (unsigned long) npcid);
//	std::string prefix = "qst";
//	std::string temp = prefix + (std::string)(itoa(npcid));
//	if(!npcid || (defaultOK && isdefault(temp.c_str())))
	if(!npcid || (defaultOK && (hasQuests.count(npcid) == 1 && hasQuests[npcid] == questDefault)))
	{
		snprintf(buf, 32, "qst%s", DEFAULT_QUEST_PREFIX.c_str());
	}

	return(std::string(buf));
}

void PerlembParser::SendCommands(const char * pkgprefix, const char *event, int32 npcid, Mob* other, Mob* mob, ItemInst* iteminst)
{
	if(!perl)
		return;

	if(mob && mob->IsClient())
		quest_manager.StartQuest(other, mob->CastToClient());
	else
		quest_manager.StartQuest(other, NULL);

	try
	{
		std::string cmd = "@quest::cmd_queue = (); package " + (std::string)(pkgprefix) + (std::string)(";");
		perl->eval(cmd.c_str());


		char namebuf[64];
		
		// Harakiri here we set the env vars for the current script
		// that it can access $client-> etc
		//init a couple special vars: client, npc, entity_list
		Client *curc = quest_manager.GetInitiator();
		snprintf(namebuf, 64, "%s::client", pkgprefix);
		SV *client = get_sv(namebuf, true);
		if(curc != NULL) {
			sv_setref_pv(client, "Client", curc);
		} else {
			//clear out the value, mainly to get rid of blessedness
			sv_setsv(client, newSV(0));
		}

		//only export NPC if it's a npc quest
		if(!other->IsClient()){
			NPC *curn = quest_manager.GetNPC();
			snprintf(namebuf, 64, "%s::npc", pkgprefix);
			SV *npc = get_sv(namebuf, true);
			sv_setref_pv(npc, "NPC", curn);
		}

		//only export QuestItem if it's an item quest
		/*if(iteminst) {
			ItemInst* curi = quest_manager.GetQuestItem();
			snprintf(namebuf, 64, "%s::questitem", pkgprefix);
			SV *questitem = get_sv(namebuf, true);
			sv_setref_pv(questitem, "QuestItem", curi);
		}*/

		snprintf(namebuf, 64, "%s::entity_list", pkgprefix);
		SV *el = get_sv(namebuf, true);
		sv_setref_pv(el, "EntityList", &entity_list);


		perl->dosub(std::string(pkgprefix).append("::").append(event).c_str());
	}
	catch(const char * err)
	{
		//try to reduce some of the console spam...
		//todo: tweak this to be more accurate at deciding what to filter (we don't want to gag legit errors)
		//if(!strstr(err,"Undefined subroutine"))
			EQC::Common::Log(EQCLog::Error,CP_QUESTS,"Script error: %s::%s - %s", pkgprefix, event, err);
		return;
	}

	int numcoms = perl->geti("quest::qsize()");
	for(int c = 0; c < numcoms; ++c)
	{
		char var[1024] = {0};
		sprintf(var,"$quest::cmd_queue[%d]{func}",c);
		std::string cmd = perl->getstr(var);
		sprintf(var,"$quest::cmd_queue[%d]{args}",c);
		std::string args = perl->getstr(var);
		size_t num_args = std::count(args.begin(), args.end(), ',') + 1;
	
		ExCommands(cmd, args, num_args, npcid, other, mob);
	}

	quest_manager.EndQuest();
}


void PerlembParser::DeleteVar(string name)
{
	list<vars*>::iterator iterator = varlist.begin();
	vars* p;
	while(iterator != varlist.end())
	{
		p = *iterator;
		if (!p->name.compare(name))
		{
			varlist.erase(iterator);
			return;
		}
		iterator++;
	}
}

string PerlembParser::GetVar(string varname, int32 npcid)
{
	list<vars*>::iterator iterator = varlist.begin();
	vars * p;
	string checkfirst;
	string checksecond;
	checkfirst = varname + (string)"." + (string)itoa(npcid);
	checksecond = varname + (string)".g";

	while(iterator != varlist.end())
	{
		p = *iterator;
		if (!strcasecmp(p->name.c_str(), checkfirst.c_str()) || !strcasecmp(p->name.c_str(),checksecond.c_str()))
		{			
			return p->value;
		}
		iterator++;
	}
	checkfirst="";
	checkfirst = "NULL";
	return checkfirst;
}

void PerlembParser::ExCommands(string o_command, string parms, int argnums, int32 npcid, Mob* other, Mob* mob )
{
	char arglist[10][1024];
	//Work out the argument list, if there needs to be one
#if Parser_DEBUG>10
		printf("Parms: %s\n", parms.c_str());
#endif	
	// Harakiri split arguments for functions i.e.
	//  quest::givecash("0","3","0","0");
	if (argnums > 1) {

		string buffer;		
		int quote=0,alist=0,ignore=0;
		int size = strlen(parms.c_str());
		int pos = 0;
		// Harakiri parms.end doesnt work for this perl string its larger then it should be
		// no idea whats causing it, we need c_str length here
		for(string::iterator iterator=parms.begin(); iterator != parms.end(); pos++) {
 
			if (*iterator == '"') {
				if (quote) quote--;
				else	  { quote++; if (buffer.empty()) ignore=1; }
			}
			if (((*iterator != '"' && *iterator != ',' && *iterator != ' ' && *iterator != ')') || quote) && !ignore) {
				buffer+=*iterator;
			}
			if (ignore && *iterator == '"')ignore--;
			if ((*iterator == ',' && !quote)) {
				strcpy(arglist[alist],buffer.c_str());
				alist++;
				buffer="";
			}
			//if (iterator == parms.end())
			if ((pos+1) == size)
			{
				strcpy(arglist[alist],buffer.c_str());
				alist++;
				break;
			}
			iterator++;			

		} 
	}
	else {
		string::iterator iterator = parms.begin();
		if (*iterator == '"')
		{
			int quote=0,ignore=0;	//once=0
			string tmp;
			while (iterator != parms.end())
			{
				if (*iterator == '"')
				{
					if (quote)quote--;
					else quote++;
					ignore++;
				}

				if (!ignore  && (quote || (*iterator != '"')))
					tmp+=*iterator;

				else if (*iterator == '"' && ignore)
					ignore--;
				iterator++;
			}
			strcpy(arglist[0],tmp.c_str());
		}
		else
			strcpy(arglist[0],parms.c_str());
	}
#if Parser_DEBUG>10
		printf("After: %s\n", arglist[0]);
#endif
	
	char command[256];
	strncpy(command, o_command.c_str(), 255);
	command[255] = 0;
	char *cptr = command;
	while(*cptr != '\0') {
		if(*cptr >= 'A' && *cptr <= 'Z')
			*cptr = *cptr + ('a' - 'A');
		 *cptr++;
	}
	
	
	if (!strcmp(command,"write")) {
		quest_manager.write(arglist[0], arglist[1]);
	}
	else if (!strcmp(command,"me")) {
// MYRA - fixed comma bug for me command
		quest_manager.me(parms.c_str());
//end Myra
	}
	else if (!strcmp(command,"spawn") || !strcmp(command,"spawn2")) 
	{
		
		float hdng;
		if (!strcmp(command,"spawn")) 
		{
			hdng=mob->CastToClient()->GetHeading();
		}
		else
		{
			hdng=atof(arglist[6]);
		}
		quest_manager.spawn2(atoi(arglist[0]), atoi(arglist[1]), 0,
			atof(arglist[3]), atof(arglist[4]), atof(arglist[5]), hdng);
	}
	else if (!strcmp(command,"unique_spawn")) 
	{
		
		float hdng;
		hdng=mob->CastToClient()->GetHeading();
		quest_manager.unique_spawn(atoi(arglist[0]), atoi(arglist[1]), 0,
			atof(arglist[3]), atof(arglist[4]), atof(arglist[5]), hdng);
	}
	else if (!strcmp(command,"echo")) {
		quest_manager.echo((MessageFormat)atoi(arglist[0]), parms.c_str());
	}
	else if (!strcmp(command,"summonitem")) {
		quest_manager.summonitem(atoi(arglist[0]));
	}
	else if (!strcmp(command,"setanim")) {
		quest_manager.setanim(atoi(arglist[0]), atoi(arglist[1]));
	}
	else if (!strcmp(command,"castspell")) {
		quest_manager.castspell(atoi(arglist[1]), atoi(arglist[0]));
	}
	else if (!strcmp(command,"selfcast")) {
		quest_manager.selfcast(atoi(arglist[0]));
	}
	else if (!strcmp(command,"addloot")) {//Cofruben: add an item to the mob.
		quest_manager.addloot(atoi(arglist[0]),atoi(arglist[1]));
	}
	else if (!strcmp(command,"zone")) {
		quest_manager.Zone(arglist[0]);
	}
	else if (!strcmp(command,"settimer")) {
		quest_manager.settimer(arglist[0], atoi(arglist[1]));
	}
	else if (!strcmp(command,"say")) {
		quest_manager.say(parms.c_str());
	}
	else if (!strcmp(command,"stoptimer")) {
		quest_manager.stoptimer(arglist[0]);
	}
	else if (!strcmp(command,"emote")) {
		quest_manager.emote(parms.c_str());
	}
	else if (!strcmp(command,"shout2")) {
		quest_manager.shout2(parms.c_str());
	}
	else if (!strcmp(command,"shout")) {
		quest_manager.shout(parms.c_str());
	}
	else if (!strcmp(command,"depop")) {
		quest_manager.depop(atoi(arglist[0]));
	}
	else if (!strcmp(command,"settarget")) {
		quest_manager.settarget(arglist[0], atoi(arglist[1]));
	}
	else if (!strcmp(command,"follow"))  {
		quest_manager.follow(atoi(arglist[0]));
	}	
	else if (!strcmp(command,"exp")) {
		quest_manager.exp(atoi(arglist[0]));
	}
	else if (!strcmp(command,"level")) {
		quest_manager.level(atoi(arglist[0]));
	}		
	else if (!strcmp(command,"rain")) {
		quest_manager.rain(atoi(arglist[0]));
	}
	else if (!strcmp(command,"snow")) {
		quest_manager.snow(atoi(arglist[0]));
	}	
	else if (!strcmp(command,"givecash")) {
		quest_manager.givecash(npcid,atoi(arglist[0]), atoi(arglist[1]), atoi(arglist[2]), atoi(arglist[3]));
	}	
	else if (!strcmp(command,"gmmove")) { 
		quest_manager.gmmove(atof(arglist[0]), atof(arglist[1]), atof(arglist[2])); 
	}	
	else if (!strcmp(command,"doanim")) {
		quest_manager.doanim(atoi(arglist[0]));
	}	
	else if (!strcmp(command,"attack")) {
		quest_manager.attack(arglist[0]);
	}
	else if (!strcmp(command,"save")) {
		quest_manager.save();
	}
	else if (!strcmp(command,"faction")) {
		quest_manager.faction(atoi(arglist[0]), atoi(arglist[1]));
	}	
	else if (!strcmp(command,"signal")) {
		quest_manager.signal(atoi(arglist[0]));
	}
	else if (!strcmp(command,"setglobal")) {
		quest_manager.setglobal(arglist[0], arglist[1], atoi(arglist[2]), arglist[3]);
	}
	else if (!strcmp(command,"targlobal")) {
		quest_manager.targlobal(arglist[0], arglist[1], arglist[2], atoi(arglist[3]), atoi(arglist[4]), atoi(arglist[5]));
	}
	else if (!strcmp(command,"ding")) {
		quest_manager.ding();
	}
	else if (!strcmp(command,"delglobal")) {
		quest_manager.delglobal(arglist[0]);
	}
	else if (!strcmp(command,"deletependingquestitem")) {
		quest_manager.DeletePendingQuestItem(arglist[0]);
	}
	else if (!strcmp(command,"stop")) {
		quest_manager.stop();
	}
	else if (!strcmp(command,"pause")) {
		quest_manager.pause(atoi(arglist[0]));
	}
	else if (!strcmp(command,"moveto")) {
		quest_manager.moveto(atof(arglist[0]), atof(arglist[1]), atof(arglist[2]), atof(arglist[3]), atoi(arglist[4]));
	}
	else if (!strcmp(command,"resume")) {
		quest_manager.resume();
	}
	else if (!strcmp(command,"start")) {
		quest_manager.start(atoi(arglist[0]));
	}	
	else if (!strcmp(command,"set_proximity")) {
		float v1 = atof(arglist[4]);
		float v2 = atof(arglist[5]);
		if(v1 == v2)	//omitted, or wrong, either way, skip them
			quest_manager.set_proximity(atof(arglist[0]), atof(arglist[1]), atof(arglist[2]), atof(arglist[3]));
		else
			quest_manager.set_proximity(atof(arglist[0]), atof(arglist[1]), atof(arglist[2]), atof(arglist[3]), v1, v2);
	}
	else if (!strcmp(command,"clear_proximity")) {
		quest_manager.clear_proximity();
	}
	else if (!strcmp(command,"respawn")) 
	{
		quest_manager.respawn(atoi(arglist[0]), atoi(arglist[1]));
	}
	else
		printf("\nUnknown perl function used:%s",command);


}

void PerlembParser::AddVar(string varname, string varval)
{
	list<vars*>::iterator iterator = varlist.begin();
	vars* p;
	while(iterator != varlist.end())
	{
		p  = *iterator;
		if (!p->name.compare(varname))
		{
			p->value="";
			p->value = varval;
			return;
		}
		iterator++;
	}
	vars * newvar = new vars;
	newvar->name = varname;
	newvar->value = varval;
	varlist.push_back(newvar);
}


#ifdef EMBPERL_COMMANDS
void PerlembParser::ExecCommand(Client *c, Seperator *sep) {

	SV *client = get_sv("commands::client", true);
	if(c != NULL) {
		sv_setref_pv(client, "Client", c);
	} else {
		//clear out the value, mainly to get rid of blessedness
		//which prevents us from accessing an invalid pointer
		sv_setsv(client, newSV(0));
	}

	char namebuf[128];
	snprintf(namebuf, 128, "commands::%s", sep->arg[0]+1);
	namebuf[127] = '\0';
	std::vector<std::string> args;
	int i;
	for(i = 1; i <= sep->argnum; i++) {
		args.push_back(sep->arg[i]);
	}

	try
	{
		perl->dosub(namebuf, &args);
	} catch(const char * err)
	{
		c->Message(RED, "Error executing perl command, check the logs.");
		EQC::Common::Log(EQCLog::Error,CP_QUESTS, "Script error: %s", err);
	}

	//now handle any events that cropped up...
	HandleQueue();
}
#endif


void PerlembParser::map_funs()
{
	_empty_sv = newSV(0);
   
	perl->eval(
	"{"
	"package PerlPacket;"
	"&boot_PerlPacket;"		//load our PerlPacket XS

	// Harakiri - not needed, handled by static methods below
	/*"package quest;"
	"&boot_QuestManager;"			//load our quest XS*/

	"package Mob;"
	"&boot_Mob;"			//load our Mob XS

	"package Client;"
	"our @ISA = qw(Mob);"	//client inherits mob.
	"&boot_Mob;"			//load our Mob XS
	"&boot_Client;"			//load our Client XS

	"package NPC;"
	"our @ISA = qw(Mob);"	//NPC inherits mob.
	"&boot_Mob;"			//load our Mob XS
	"&boot_NPC;"			//load our NPC XS

	/*"package Corpse;"
	"our @ISA = qw(Mob);"	//Corpse inherits mob.
	"&boot_Mob;"			//load our Mob XS
	"&boot_Corpse;"			//load our Mob XS
*/
	"package EntityList;"
	"&boot_EntityList;"		//load our EntityList XS*/
	
	"package main;"
	"}"
	);

	//map each "exported" function to a variable list that we can access from c
	//todo:
	//	break 1|settimer 2|stoptimer 1|dbspawnadd 2|flagcheck 1|write 2|
	//	settarget 2|follow 1|sfollow 1|save 1|setallskill 1
	//update/ensure that the api matches that of the native script engine
	perl->eval(
"{"
"package quest;"
/*"&boot_qc;"*/
"@cmd_queue = ();"
"sub qsize{return scalar(@cmd_queue)};"
"sub say{push(@cmd_queue,{func=>'say',args=>join(',',@_)});}"
"sub emote{push(@cmd_queue,{func=>'emote',args=>join(',',@_)});}"
"sub shout{push(@cmd_queue,{func=>'shout',args=>join(',',@_)});}"
"sub spawn{push(@cmd_queue,{func=>'spawn',args=>join(',',@_)});}"
"sub spawn2{push(@cmd_queue,{func=>'spawn2',args=>join(',',@_)});}"
"sub unique_spawn{push(@cmd_queue,{func=>'unique_spawn',args=>join(',',@_)});}"
"sub echo{push(@cmd_queue,{func=>'echo',args=>join(',',@_)});}"
"sub summonitem{push(@cmd_queue,{func=>'summonitem',args=>join(',',@_)});}"
"sub castspell{push(@cmd_queue,{func=>'castspell',args=>join(',',@_)});}"
"sub selfcast{push(@cmd_queue,{func=>'selfcast',args=>join(',',@_)});}"
"sub depop{push(@cmd_queue,{func=>'depop'});}"
"sub exp{push(@cmd_queue,{func=>'exp',args=>join(',',@_)});}"
"sub level{push(@cmd_queue,{func=>'level',args=>join(',',@_)});}"
"sub safemove{push(@cmd_queue,{func=>'safemove'});}"
"sub rain{push(@cmd_queue,{func=>'rain',args=>join(',',@_)});}"
"sub snow{push(@cmd_queue,{func=>'snow',args=>join(',',@_)});}"
"sub givecash{push(@cmd_queue,{func=>'givecash',args=>join(',',@_)});}"
"sub pvp{push(@cmd_queue,{func=>'pvp',args=>join(',',@_)});}"
"sub doanim{push(@cmd_queue,{func=>'doanim',args=>join(',',@_)});}"
"sub addskill{push(@cmd_queue,{func=>'addskill',args=>join(',',@_)});}"
"sub me{push(@cmd_queue,{func=>'me',args=>join(',',@_)});}"
"sub ding{push(@cmd_queue,{func=>'ding',args=>join(',',@_)});}"
// Harakiri unused but needed to be exported since a few quest use them
// alternativly remove offending calls in guildmasters.pl
"sub isdisctome{push(@cmd_queue,{func=>'isdisctome',args=>join(',',@_)});}"
"sub traindisc{push(@cmd_queue,{func=>'traindisc',args=>join(',',@_)});}"

"sub faction{push(@cmd_queue,{func=>'faction',args=>join(',',@_)});}"
"sub settime{push(@cmd_queue,{func=>'settime',args=>join(',',@_)});}"
"sub setsky{push(@cmd_queue,{func=>'setsky',args=>join(',',@_)});}"
"sub settimer{push(@cmd_queue,{func=>'settimer',args=>join(',',@_)});}"
"sub stoptimer{push(@cmd_queue,{func=>'stoptimer',args=>join(',',@_)});}"
"sub settarget{push(@cmd_queue,{func=>'settarget',args=>join(',',@_)});}"
"sub follow{push(@cmd_queue,{func=>'follow',args=>join(',',@_)});}"
"sub sfollow{push(@cmd_queue,{func=>'sfollow',args=>join(',',@_)});}"
"sub movepc{push(@cmd_queue,{func=>'movepc',args=>join(',',@_)});}"
"sub gmmove{push(@cmd_queue,{func=>'gmmove',args=>join(',',@_)});}"
"sub attack{push(@cmd_queue,{func=>'attack',args=>join(',',@_)});}"
"sub save{push(@cmd_queue,{func=>'save',args=>join(',',@_)});}"
//end Myra
"sub sethp{push(@cmd_queue,{func=>'sethp',args=>join(',',@_)});}"
"sub signal{push(@cmd_queue,{func=>'signal',args=>join(',',@_)});}"
// SCORPIOUS2K - add perl versions qglobal commands
"sub setglobal{push(@cmd_queue,{func=>'setglobal',args=>join(',',@_)});}"
"sub targlobal{push(@cmd_queue,{func=>'targlobal',args=>join(',',@_)});}"
"sub delglobal{push(@cmd_queue,{func=>'delglobal',args=>join(',',@_)});}"
// Harakiri delete a quest item from the globals
"sub DeletePendingQuestItem{push(@cmd_queue,{func=>'DeletePendingQuestItem',args=>join(',',@_)});}"
// event hp
"sub setnexthpevent{push(@cmd_queue,{func=>'setnexthpevent',args=>join(',',@_)});}"
"sub setnextinchpevent{push(@cmd_queue,{func=>'setnextinchpevent',args=>join(',',@_)});}"
"sub respawn{push(@cmd_queue,{func=>'respawn',args=>join(',',@_)});}"
// new wandering commands
"sub stop{push(@cmd_queue,{func=>'stop',args=>join(',',@_)});}"
"sub pause{push(@cmd_queue,{func=>'pause',args=>join(',',@_)});}"
"sub resume{push(@cmd_queue,{func=>'resume',args=>join(',',@_)});}"
"sub start{push(@cmd_queue,{func=>'start',args=>join(',',@_)});}"
"sub moveto{push(@cmd_queue,{func=>'moveto',args=>join(',',@_)});}"
// add ldon points command
"sub warp{push(@cmd_queue,{func=>'warp',args=>join(',',@_)});}"
"sub changedeity{push(@cmd_queue,{func=>'changedeity',args=>join(',',@_)});}"
"sub addldonpoints{push(@cmd_queue,{func=>'addldonpoints',args=>join(',',@_)});}"
"sub addloot{push(@cmd_queue,{func=>'addloot',args=>join(',',@_)});}"
"sub traindisc{push(@cmd_queue,{func=>'traindisc',args=>join(',',@_)});}"
"sub set_proximity{push(@cmd_queue,{func=>'set_proximity',args=>join(',',@_)});}"
"sub clear_proximity{push(@cmd_queue,{func=>'clear_proximity',args=>join(',',@_)});}"
"sub setanim{push(@cmd_queue,{func=>'setanim',args=>join(',',@_)});}"
"sub showgrid{push(@cmd_queue,{func=>'showgrid',args=>join(',',@_)});}"
"sub showpath{push(@cmd_queue,{func=>'showpath',args=>join(',',@_)});}"
"sub pathto{push(@cmd_queue,{func=>'pathto',args=>join(',',@_)});}"
"sub spawn_condition{push(@cmd_queue,{func=>'spawn_condition',args=>join(',',@_)});}"
"sub toggle_spawn_event{push(@cmd_queue,{func=>'toggle_spawn_event',args=>join(',',@_)});}"
"sub set_zone_flag{push(@cmd_queue,{func=>'set_zone_flag',args=>join(',',@_)});}"
"sub clear_zone_flag{push(@cmd_queue,{func=>'clear_zone_flag',args=>join(',',@_)});}"
"package main;"
"}"
);
}

#endif //EMBPERL

#endif //EMBPARSER_CPP
