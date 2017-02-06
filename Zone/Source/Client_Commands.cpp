//TODO: sep.IsNumber(split chr)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include "config.h"
#include "client.h"
#include "net.h"
#include "database.h"
#include "mob.h"
#include "packet_functions.h"
#include "worldserver.h"
#include "zone.h"
#include "seperator.h"
#include "spdat.h"
#include "PlayerCorpse.h"
#include "SpellsHandler.hpp"
#include "projectile.h"
#include "SharedMemory.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include "Client_Commands.h"

#ifdef EMBPERL
	#include "embparser.h"
#endif

extern Database			database;
extern Zone*			zone;
extern SpellsHandler	spells_handler;
extern WorldServer		worldserver;

float fdistance(float x1, float y1, float x2, float y2);

float square(float number);

//struct cl_struct *commandlist;	// the actual linked list of commands
int commandcount;								// how many commands we have

// this is the pointer to the dispatch function, updated once
// init has been performed to point at the real function
int (*command_dispatch)(Client *,char *)=command_notavail;


void command_bestz(Client *c, const Seperator *message);
void command_pf(Client *c, const Seperator *message);

map<string, CommandRecord *> commandlist;

//All allocated CommandRecords get put in here so they get deleted on shutdown
LinkedList<CommandRecord *> cleanup_commandlist;

// Harakiri for command mapping to perl
#ifdef EMBPERL_COMMANDS
XS(XS_command_add); /* prototype to pass -Wmissing-prototypes */ 
XS(XS_command_add) 
{ 
    dXSARGS; 
    if (items != 3)
        Perl_croak(aTHX_ "Usage: commands::command_add(name, desc, access)"); 

	char *name = SvPV_nolen(ST(0));
	char *desc = SvPV_nolen(ST(1));
	int	access = (int)SvIV(ST(2));
	
	command_add_perl(name, desc, access);
 	
    XSRETURN_EMPTY; 
}
#endif

/*
 * command_notavail
 * This is the default dispatch function when commands aren't loaded.
 *
 * Parameters:
 *	 not used
 *
 */
int command_notavail(Client *c, char *message)
{
	c->Message(RED, "Commands not available.");
	return -1;
}

/*****************************************************************************/
/*  the rest below here could be in a dynamically loaded module eventually   */
/*****************************************************************************/

/*

Access Levels:

0		Normal
10	* Steward *
20	* Apprentice Guide *
50	* Guide *
80	* QuestTroupe *
81	* Senior Guide *
85	* GM-Tester *
90	* EQ Support *
95	* GM-Staff *
100	* GM-Admin *
150	* GM-Lead Admin *
160	* QuestMaster *
170	* GM-Areas *
180	* GM-Coder *
200	* GM-Mgmt *
250	* GM-Impossible *

*/

/*
 * command_init
 * initializes the command list, call at startup
 *
 * Parameters:
 *	 none
 *
 * When adding a command, if it's the first time that function pointer is
 * used it is a new command.  If that function pointer is used for another
 * command, the command is added as an alias; description and access level
 * are not used and can be NULL.
 *
 */
int command_init(void) {

	int GM_MASTER_ACESSS = 250;
	int GM_MANAGEMENT_ACESSS = 200;
	int GM_ADMIN = 10;
	int NORMAL_ACESS = 0;
	if
	(	command_add("goto","[x] [y] [z] - Teleport to the provided coordinates or to your target",GM_MASTER_ACESSS,command_goto) ||
		command_add("level","[level] - Set your or your target's level",GM_MASTER_ACESSS,command_level) ||
		command_add("damage","[x] - Inflicts damage upon target.",EQC_Alpha_Tester,command_damage) ||
		command_add("heal","[x] - (PC only) Completely heals your target.",EQC_Alpha_Tester,command_heal) ||
		command_add("kill","Kills your target.",EQC_Alpha_Tester,command_kill) ||
		command_add("mana","(PC only) Replenishes your target\'s mana.",EQC_Alpha_Tester,command_mana) ||
		command_add("timeofday","Sets the date to Monday, Janurary 1st, 1 at the specified time temporary.",GM_MASTER_ACESSS,command_timeofday) ||				

		// summonitem related
		command_add("summonitem","[itemid] [charges] - Summon an item onto your cursor.  Charges are optional.",EQC_Alpha_Tester,command_summonitem) ||		
		command_add("si",NULL,EQC_Alpha_Tester,command_summonitem) ||

		command_add("summonitemnonblob","[itemid] [charges] - Summon an item onto your cursor.  Charges are optional.",EQC_Alpha_Tester,command_summonitemnonblob) ||		
		command_add("si2",NULL,EQC_Alpha_Tester,command_summonitemnonblob) ||
		command_add("summonitem2",NULL,EQC_Alpha_Tester,command_summonitemnonblob) ||
		
		// corpse summon //newage: is this supposed to be in EQC?
		command_add("corpse","- Manipulate corpses, use with no arguments for help",0,command_corpse) ||

		command_add("setexp","[value] - Set your experience",GM_ADMIN,command_setexp) ||
		command_add("addexp","[value] - Adds to your experience",GM_ADMIN,command_addexp) ||
		
		// itemsearch related
		command_add("itemsearch","[search criteria] - Search for an item",EQC_Alpha_Tester,command_itemsearch) ||
		command_add("search",NULL,EQC_Alpha_Tester,command_itemsearch) ||		
		command_add("finditem",NULL,EQC_Alpha_Tester,command_itemsearch) ||
		command_add("fi",NULL,EQC_Alpha_Tester,command_itemsearch) ||		

		command_add("zone","[zonename] - Zones to safepoint in the specified zone. This command only accepts proper zone names, i.e. \'freporte\' and \'cabeast\'.",EQC_Alpha_Tester,command_zone) ||

		command_add("loc","Shows you your current location.",EQC_Alpha_Tester,command_loc) ||		
		command_add("npcstats","- Show stats about target NPC",GM_MANAGEMENT_ACESSS,command_npcstats) ||

		command_add("findspell","[searchstring] - Search for a spell",EQC_Alpha_Tester,command_findspell) ||
		command_add("spfind",NULL,EQC_Alpha_Tester,command_findspell) ||
	
		command_add("findrecipe","[searchstring] - Search for a receipe",EQC_Alpha_Tester,command_findreceipe) ||
		command_add("fr",NULL,EQC_Alpha_Tester,command_findreceipe) ||

		command_add("receipesummon","[receipeid] - Summon all components of a recipe",EQC_Alpha_Tester,command_receipesummon) ||
		command_add("rs",NULL,EQC_Alpha_Tester,command_receipesummon) ||

		command_add("clearcursor","clears clients cursor server side",EQC_Alpha_Tester,command_clearcursor) ||
		command_add("cc",NULL,EQC_Alpha_Tester,command_clearcursor) ||

		command_add("reloadperl","Reload Perl/Quests",EQC_Alpha_Tester,command_reloadperl) ||
		command_add("reloadquests",NULL,EQC_Alpha_Tester,command_reloadperl) ||

		command_add("diffitem","[itemid] compares blob item with nonblob item",EQC_Alpha_Tester,command_diffitem) ||
		command_add("di",NULL,EQC_Alpha_Tester,command_diffitem) ||

		command_add("teleport","[zone] ([x] [y] [z]) ([heading]) teleports you to the specific parameters",EQC_Alpha_Tester,command_teleport) ||
		command_add("translocate","[zone] ([x] [y] [z]) translocates you to the specific parameters",EQC_Alpha_Tester,command_translocate) ||
		
		command_add("givemoney","[copper] [silver] [gold] [plat] - Gives specified amount of money to the target player.",GM_MANAGEMENT_ACESSS,command_givemoney) ||
		command_add("money",NULL,GM_MANAGEMENT_ACESSS,command_givemoney) ||

		command_add("castspell","[id] Cast spell on target",GM_MANAGEMENT_ACESSS,command_castspell) ||
		command_add("cast",NULL,GM_MANAGEMENT_ACESSS,command_castspell) ||

		command_add("invulnerable","Make target invulnerable",GM_MANAGEMENT_ACESSS,command_invulnerable) ||
		command_add("invul",NULL,GM_MANAGEMENT_ACESSS,command_invulnerable) ||

		command_add("listlang","List all languages and their ids",GM_MANAGEMENT_ACESSS,command_listlang) ||
		
		command_add("setlang","[id] Sets the languages skill value",GM_MANAGEMENT_ACESSS,command_setlang) ||
	
		command_add("setskill","[id] Sets the skill value",GM_MANAGEMENT_ACESSS,command_setskill) ||

		command_add("setskillall","Sets all skills to value",GM_MANAGEMENT_ACESSS,command_setskillall) ||

		command_add("checkskillall","Checks all skills of the target",GM_MANAGEMENT_ACESSS,command_checkskillall) ||

		command_add("save","Manually save target",GM_MANAGEMENT_ACESSS,command_save) ||

		command_add("scribespells","[1-60] - Scribes all spells for your class to desired level.",GM_MANAGEMENT_ACESSS,command_scribespells) ||

		command_add("flymode","[0|1] target can fly",GM_MANAGEMENT_ACESSS,command_flymode) ||

		command_add("depopzone","Removes all NPCS in this zone",GM_MANAGEMENT_ACESSS,command_depopzone) ||

		command_add("repopzone","Respawns all NPCS in this zone",GM_MANAGEMENT_ACESSS,command_repopzone) ||
		
		command_add("sky","Changes the sky",GM_MANAGEMENT_ACESSS,command_sky) ||

		command_add("fogcolor","<red> <green> <blue>",GM_MANAGEMENT_ACESSS,command_fogcolor) ||

		command_add("zoneappearance","Lists the intended zone appearance variables",GM_MANAGEMENT_ACESSS,command_zoneappearance) ||

		command_add("zoneclip","<minclip> <maxclip>",GM_MANAGEMENT_ACESSS,command_zoneclip) ||

		command_add("debug","[1|0] - Enable additional output",GM_MANAGEMENT_ACESSS,command_debug) ||

		command_add("help","[search term] - List available commands and their description, specify partial command as argument to search",NORMAL_ACESS,command_help)				
				
		)
	{
		command_deinit();
		return -1;
	}
	
	map<string, CommandRecord *>::iterator cur,end;
	cur = commandlist.begin();
	end = commandlist.end();
	/*map<string,uint8> command_settings;
	map<string,uint8>::iterator itr;
	database.GetCommandSettings(command_settings);
	for(; cur != end; cur++) {
		if ((itr=command_settings.find(cur->first))!=command_settings.end())
		{
			cur->second->access = itr->second;
#if EQDEBUG >=5
			EQC::Common::Log(EQCLog::Debug,CP_CLIENT, "command_init(): - Command '%s' set to access level %d." , cur->first.c_str(), itr->second);
#endif
		}
		else
		{
#ifdef COMMANDS_WARNINGS
			if(cur->second->access == 0)
				EQC::Common::Log(EQCLog::Debug,CP_CLIENT, "command_init(): Warning: Command '%s' defaulting to access level 0!" , cur->first.c_str());
#endif
		}
	}*/
	
	command_dispatch = command_realdispatch;

	return commandcount;
}

/*
 * command_deinit
 * clears the command list, freeing resources
 *
 * Parameters:
 *	 none
 *
 */
void command_deinit(void)
{

	commandlist.clear();
	
	command_dispatch = command_notavail;
	commandcount = 0;
}

/*
 * command_add
 * adds a command to the command list; used by command_init
 *
 * Parameters:
 *	 command_string	- the command ex: "spawn"
 *	 desc		- text description of command for #help
 *	 access		- default access level required to use command
 *	 function		- pointer to function that handles command
 *
 */
int command_add(const char *command_string, const char *desc, int access, CmdFuncPtr function)
{
	if(function == NULL)
		return(-1);
	
	string cstr(command_string);
	
	if(commandlist.count(cstr) != 0) {
		EQC::Common::Log(EQCLog::Error,CP_CLIENT, "command_add() - Command '%s' is a duplicate - check command.cpp." , command_string);
		return(-1);
	}
	
	//look for aliases...
	map<string, CommandRecord *>::iterator cur,end,del;
	cur = commandlist.begin();
	end = commandlist.end();
	for(; cur != end; cur++) {
		if(cur->second->function == function) {
			int r;
			for(r = 1; r < CMDALIASES; r++) {
				if(cur->second->command[r] == NULL) {
					cur->second->command[r] = command_string;
					break;
				}
			}
			commandlist[cstr] = cur->second;
			return(0);
		}
	}
	
	CommandRecord *c = new CommandRecord;
	cleanup_commandlist.Append(c);
	c->desc = desc;
	c->access = access;
	c->function = function;
	memset(c->command, 0, sizeof(c->command));
	c->command[0] = command_string;
	
	commandlist[cstr] = c;
	
	commandcount++;
	return 0;
}

/*
 *
 * command_realdispatch
 * Calls the correct function to process the client's command string.
 * Called if message starts with command character (#).
 *
 * Parameters:
 *	 c			- pointer to the calling client object
 *	 message		- what the client typed
 *
 */
int command_realdispatch(Client *c, char *message)
{
    Seperator sep(message, ' ', 10, 100, true); //changed by Eglin: "three word argument" should be considered 1 arg
	
	command_logcommand(c, message);
	
	string cstr(sep.arg[0]+1);
	
	if(commandlist.count(cstr) != 1) {
		c->Message(RED, "Command '%s' not recognized.", sep.arg[0]+1);
		return(-1);
	}
	
	CommandRecord *cur = commandlist[cstr];
	if(c->Admin() < cur->access){
		c->Message(RED,"Your access level is not high enough to use this command.");
		return(-1);
	}
	
#ifdef COMMANDS_LOGGING
	if(cur->access >= COMMANDS_LOGGING_MIN_STATUS) {
		EQC::Common::Log(EQCLog::Debug,CP_CLIENT, "%s (%s) used command: %s (target=%s)", c->GetName(), c->AccountName(), message, c->GetTarget()?c->GetTarget()->GetName():"NONE");
	}
#endif
	
	if(cur->function == NULL) {
#ifdef EMBPERL_COMMANDS
		//dispatch perl command		
		perlParser->ExecCommand(c, &sep);
#else
		EQC::Common::Log(EQCLog::Error,CP_CLIENT, "Command '%s' has a null function, but perl commands are diabled!\n", cstr.c_str());
		return(-1);
#endif
	} else {
		//dispatch C++ command
		cur->function(c, &sep);	// dispatch command
	}
	return 0;
	
}


void command_logcommand(Client *c, char *message)
{
	/*
	int admin=c->Admin();

	bool continueevents=false;
	switch (zone->loglevelvar){ //catch failsafe
		case 9: { // log only LeadGM
			if ((admin>= 150) && (admin <200))
				continueevents=true;
			break;
		}
		case 8: { // log only GM
			if ((admin>= 100) && (admin <150))
				continueevents=true;
			break;
		}
		case 1: {
			if ((admin>= 200))
				continueevents=true;
			break;
		}
		case 2: {
			if ((admin>= 150))
				continueevents=true;
			break;
		}
		case 3: {
			if ((admin>= 100))
				continueevents=true;
			break;
		}
		case 4: {
			if ((admin>= 80))
				continueevents=true;
			break;
		}
		case 5: {
			if ((admin>= 20))
				continueevents=true;
			break;
		}
		case 6: {
			if ((admin>= 10))
				continueevents=true;
			break;
		}
		case 7: {
				continueevents=true;
				break;
		}
	}
	
	if (continueevents)
		database.logevents(
			c->AccountName(),
			c->AccountID(),
			admin,c->GetName(),
			c->GetTarget()?c->GetTarget()->GetName():"None",
			"Command",
			message,
			1
		);*/
}

#ifdef EMBPERL_COMMANDS
/*
 * command_add_perl
 * adds a command to the command list, as a perl function
 *
 * Parameters:
 *	 command_string	- the command ex: "spawn"
 *	 desc		- text description of command for #help
 *	 access		- default access level required to use command
 *
 */
int command_add_perl(const char *command_string, const char *desc, int access) {
	string cstr(command_string);

	if(commandlist.count(cstr) != 0) {
#ifdef COMMANDS_PERL_OVERRIDE
		//print a warning so people dont get too confused when this happens
		EQC::Common::Log(EQCLog::Warn, CP_PERL, "command_add_perl() - Perl Command '%s' is overriding the compiled command." , command_string);	
		CommandRecord *tmp = commandlist[cstr];
		safe_delete(tmp);
#else
		EQC::Common::Log(EQCLog::Error, CP_PERL, "command_add_perl() - Command '%s' is a duplicate - check commands.pl." , command_string);
		return(-1);
#endif
	}
	
	CommandRecord *c = new CommandRecord;
	c->desc = desc;
	c->access = access;
	c->function = NULL;
	
	commandlist[cstr] = c;
	
	commandcount++;
	return 0;

}

//clear out any perl commands.
//should restore any overridden C++ commands, but thats a lot of work.
void command_clear_perl() {
	map<string, CommandRecord *>::iterator cur,end,del;
	cur = commandlist.begin();
	end = commandlist.end();
	for(; cur != end;) {
		del = cur;
		cur++;
		if(del->second->function == NULL) {
			safe_delete(del->second);
			commandlist.erase(del);
		}
	}
}

#endif //EMBPERL_COMMANDS

void command_help(Client *c, const Seperator *sep)
{
	int commands_shown=0;

	c->Message(BLACK, "Available EQC commands for access level %d :",c->Admin());
	
	map<string, CommandRecord *>::iterator cur,end;
	cur = commandlist.begin();
	end = commandlist.end();
	
	for(; cur != end; cur++) {
		if(sep->arg[1][0]) {		
			if(cur->first.find(sep->arg[1]) == string::npos) {
				continue;
			}
		}
		
		if(c->Admin() < cur->second->access)
			continue;
  		commands_shown++;
		c->Message(BLACK, "%c%s %s", COMMAND_CHAR, cur->first.c_str(), cur->second->desc == NULL?"":cur->second->desc);
	}
	c->Message(BLACK, "%d command%s listed.", commands_shown, commands_shown!=1?"s":"");
	
}

void command_goto(Client *c, const Seperator *sep)
{
		if (sep->arg[1][0] == 0 && c->GetTarget() != 0) {
			c->MovePC(0, c->GetTarget()->GetX(), c->GetTarget()->GetY(), c->GetTarget()->GetZ(), false, false);
		}
		else if (!(sep->IsNumber(1) && sep->IsNumber(2) && sep->IsNumber(3))) {
			c->Message(BLACK, "Usage: #goto [x y z].");
		}
		else {
			c->MovePC(0, atof(sep->arg[2]), atof(sep->arg[1]), atof(sep->arg[3]));
		}
}

void command_level(Client *c, const Seperator *sep)
{
		if (c->GetTarget() == 0)
		{
			c->Message(BLACK, "Error: #Level: No target.");
		}
		else if (sep->arg[1][0] == 0) 
		{
			c->Message(BLACK, "Usage: #level x (between 1 and %i).", MAX_PLAYERLEVEL);
		}
		else 
		{
			int16 level = atoi(sep->arg[1]);


			if((c->GetTarget()->IsClient()) &&  ( (level <= 0) || (level > MAX_PLAYERLEVEL) ) )
			{
				c->Message(BLACK, "Usage: #level x (between 1 and %i).", MAX_PLAYERLEVEL);
				return;
			}

			if( (c->GetTarget()->IsNPC()) && ( (level <= 0) ||(level > MAX_NPCLEVEL) ) )
			{
				c->Message(BLACK, "Usage: #level x (NPC level is between 1 and %i).", MAX_NPCLEVEL);
				return;
			}
			if (c->GetTarget()->IsClient()) {
				int32 supposed_exp = c->GetTarget()->CastToClient()->GetEXPForLevel(level);
				c->GetTarget()->CastToClient()->SetEXP(supposed_exp + 1);				
			}
			else {
				c->GetTarget()->SetLevel(level);		
				c->GetTarget()->Save();			
			}
		}

}
void command_damage(Client *c, const Seperator *sep)
{
		if (c->GetTarget()==0) 
		{
			c->Message(BLACK, "Error: #damage: No Target.");
		}
		else if (!sep->IsNumber(1)) 
		{
			c->Message(BLACK, "Usage: #damage x.");		
		}
		else 
		{
			sint32 nkdmg = atoi(sep->arg[1]);

			if (nkdmg > MAX_DAMAGE || nkdmg < MIN_DAMAGE)
			{			
				c->Message(BLACK, "You must enter a number greater than 0 and less than 32,000.");
			}
			else
			{
				c->GetTarget()->Damage(c, nkdmg, 0xffff);
			}
		}
}
void command_heal(Client *c, const Seperator *sep)
{
		if (c->GetTarget()==0) 
		{
			c->Message(BLACK, "Error: #Heal: No Target.");
		}
		else 
		{
			c->GetTarget()->Heal();
		}
}

void command_mana(Client *c, const Seperator *sep)
{
	if (c->GetTarget() == 0)
		{
			// TODO: Seems although items are in the mana calculation, it doesn't
			// factor them in.
			c->SetMana(c->CalcMaxMana());
		}
		else
		{
			c->GetTarget()->SetMana(c->GetTarget()->GetMaxMana());
		}
}

void command_kill(Client *c, const Seperator *sep)
{	
		if (c->GetTarget()==0) 
		{
			c->Message(BLACK, "Error: #kill: No target.");
		}
		else
		{
			if ((!c->GetTarget()->IsClient()) || c->GetTarget()->CastToClient()->Admin() < c->Admin())
			{
				c->GetTarget()->Kill();
			}
		}
}
void command_timeofday(Client *c, const Seperator *sep)
{
		if (!sep->IsNumber(1) || atoi(sep->arg[1]) <= 0 || atoi(sep->arg[1]) >= 25) 
		{
			c->Message(BLACK, "Usage: #timeofday <hour>");
		}
		else 
		{		
			APPLAYER* timepacket = c->CreateTimeOfDayPacket(atoi(sep->arg[1]),0,1,1,1);
			entity_list.QueueClients(c, timepacket);
			safe_delete(timepacket);//delete timepacket;
		}
}

void command_setexp(Client *c, const Seperator *sep)

{
		if (sep->IsNumber(1)) {
			if (atoi(sep->arg[1]) > MAX_PC_EXPERIANCE)
				c->Message(BLACK, "Error: SetEXP: Value too high.");
			else
				c->SetEXP (atoi(sep->arg[1]));
		}
		else
			c->Message(BLACK, "Usage: #setexp number");
}

void command_addexp(Client *c, const Seperator *sep)	
{
		if (sep->IsNumber(1)) {

			if (atoi(sep->arg[1]) > MAX_PC_EXPERIANCE)
				c->Message(BLACK, "Error: AddEXP: Value too high.");
			else
				c->AddEXP (atoi(sep->arg[1]));
		}
		else
			c->Message(BLACK, "Usage: #addexp number");
}

void command_summonitem(Client *c, const Seperator *sep)		
{
	if (!sep->IsNumber(1)) {
			c->Message(BLACK, "Usage: #summonitem x , x is an item number");
	}
	else {
			int16 itemid = atoi(sep->arg[1]);
			if (c->Admin() < 100 && ((itemid >= 32768) || (itemid >= 19900 && itemid <= 19943) || (itemid >= 31814 && itemid <= 31815) || (itemid >= 19917 && itemid <= 19928) || (itemid >= 11500 && itemid <= 11535) || (itemid >=32740 && itemid <=32758))) { 					c->Death(c, 0);
			}
			else
				c->SummonItem(itemid,1);
	}
}	

void command_corpse(Client *c, const Seperator *sep)
{
	//there is no permissions checking here, yet. If the corpse matches your name, summon it.
	Mob *target=c->GetTarget();
	if (c->GetTarget()->IsCorpse() && strstr(c->GetName(),c->GetTarget()->GetName()))
	{
		if(fdistance(c->GetX(), c->GetY(), c->GetTarget()->GetX(), c->GetTarget()->GetY()) < 200) {
			c->Message(BLACK, "Summoning corpse...");
			c->GetTarget()->CastToCorpse()->GMMove(c->GetX(), c->GetY(), c->GetZ(), c->GetHeading());
		} else {
			c->Message(BLACK, "Get closer.");
		}
	} else {
		c->Message(BLACK, "This is not your corpse.");
	}
}

void command_itemsearch(Client *c, const Seperator *sep)
{	
		if (sep->arg[1][0] == 0)
			c->Message(BLACK, "Usage: #itemsearch [search string]");
		else
		{
			const char *search_criteria=sep->argplus[1];

			const Item_Struct* item = 0;
			if (Seperator::IsNumber(search_criteria)) {
				item = Database::Instance()->GetItem(atoi(search_criteria));
				if (item)
					c->Message(BLACK, "  %i: %s", (int) item->item_nr, item->name);
				else
					c->Message(BLACK, "Item #%s not found", search_criteria);
				return;
			}
	//#ifdef SHAREMEM
			int count=0;
			//int iSearchLen = strlen(search_criteria)+1;
			char sName[64];
			char sCriteria[255];
			strncpy(sCriteria, search_criteria, sizeof(sCriteria));
			_strupr(sCriteria);
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
					strncpy(sName, item->name, sizeof(sName));
					_strupr(sName);
					pdest = strstr(sName, sCriteria);
					if (pdest != NULL) {
						c->Message(BLACK, "  %i: %s", (int) item->item_nr, item->name);
						count++;
					}
					if (count == 20)
						break;
				}
			}
			if (count == 20)
				c->Message(BLACK, "20 items shown...too many results.");
			else
				c->Message(BLACK, "%i items found", count);
	//#endif
		}
	}

void command_zone(Client *c, const Seperator *sep) 
{
		if (sep->arg[1][0] == 0)
		{
			c->Message(BLACK, "Usage: #zone [zonename]");
		}
		else 
		{
			float x = -1;
			float y = -1;
			float z = -1;
			float myHeading = -1;
			if(Database::Instance()->GetSafePoints(sep->arg[1], &x, &y, &z, 0, 0, &myHeading))
			{
				c->SetZoningHeading((int8)myHeading);
				c->SetUsingSoftCodedZoneLine(true);
				c->SetIsZoning(true);
				c->SetZoningX((sint32)x);
				c->SetZoningY((sint32)y);
				c->SetZoningZ((sint32)z);				
				c->ZonePC(sep->arg[1], x, y, z);
			}
			else
			{
				c->Message(RED, "That zone is not currently available. Sending you to a safespot within your current zone.", sep->arg[1]);
				c->MovePC(0, zone->GetSafeX(), zone->GetSafeY(), zone->GetSafeZ() * 10, false, false);
			}
		}
}

void command_loc(Client *c, const Seperator *sep) 
{
		if (c->GetTarget() != 0)
		{
			//Changed back. Screwed things up more -Wizzel
			c->Message(BLACK, "%s's Location: %1.1f, %1.1f, %1.1f, h=%1.1f", c->GetTarget()->GetName(), c->GetTarget()->GetX(), c->GetTarget()->GetY(), c->GetTarget()->GetZ(), c->GetTarget()->GetHeading());
		}
		else
		{
			c->Message(BLACK, "Current Location: %1.1f, %1.1f, %1.1f, h=%1.1f", c->GetX(), c->GetY(), c->GetZ(), c->GetHeading());
		}
}

void command_npcstats(Client *c, const Seperator *sep) 
{
		if (c->GetTarget() == 0)
			c->Message(BLACK, "ERROR: No target!");
		else if (!c->GetTarget()->IsNPC())
			c->Message(BLACK, "ERROR: Target is not a NPC!");
		else {
			c->Message(BLACK, "NPC Stats:");
			c->Message(BLACK, "  Name: %s",c->GetTarget()->GetName()); 
			//Message(BLACK, "  Last Name: %s",sep.arg[2]); 
			c->Message(BLACK, "  Race: %i",c->GetTarget()->GetRace());  
			c->Message(BLACK, "  Level: %i",c->GetTarget()->GetLevel());
			c->Message(BLACK, "  Material: %i",c->GetTarget()->GetTexture());
			c->Message(BLACK, "  Class: %i",c->GetTarget()->GetClass());
			c->Message(BLACK, "  Current HP: %i", c->GetTarget()->GetHP());
			c->Message(BLACK, "  Max HP: %i", c->GetTarget()->GetMaxHP());
			//Message(BLACK, "Weapon Item Number: %s",target->GetWeapNo()); 
			c->Message(BLACK, "  Gender: %i",c->GetTarget()->GetGender()); 
			c->GetTarget()->CastToNPC()->QueryLoot(c);
		}
}

void command_findspell(Client *c, const Seperator *sep)
{
		if (sep->arg[1][0] == 0)
			c->Message(BLACK, "Usage: #FindSpell [spellname]");
		else if (!spells_handler.SpellsLoaded())
			c->Message(BLACK, "Spells not loaded");
		else if (Seperator::IsNumber(sep->argplus[1])) {
			int spellid = atoi(sep->argplus[1]);
			if (spellid <= 0 || spellid >= SPDAT_RECORDS) {
				c->Message(BLACK, "Error: Number out of range");
			}
			else {
				c->Message(BLACK, "  %i: %s", spellid, spells_handler.GetSpellPtr(spellid)->GetSpellName());
			}
		}
		else {
			int count=0;
			int iSearchLen = strlen(sep->argplus[1])+1;
			char sName[64];
			char sCriteria[65];
			strncpy(sCriteria, sep->argplus[1], 64);
			_strupr(sCriteria);
			for (int i = 0; i < SPDAT_RECORDS; i++)
			{
				const char* name = spells_handler.GetSpellPtr(i)->GetSpellName();
				if (name[0] != 0) {
					strcpy(sName, name);

					_strupr(sName);
					char* pdest = strstr(sName, sCriteria);
					if ((pdest != NULL) && (count <=20)) {
						c->Message(BLACK, "  %i: %s", i, name);
						count++;
					}
					else if (count > 20)
						break;
				}
			}
			if (count > 20)
				c->Message(BLACK, "20 spells found... max reached.");
			else
				c->Message(BLACK, "%i spells found.", count);
		}
	
}

void command_findreceipe(Client *c, const Seperator *sep)
{
		if (sep->arg[1][0] == 0)
			c->Message(BLACK, "Usage: #findreceipe [search string]");
		else
		{
			const char *search_criteria=sep->argplus[1];
			
			 vector<DBTradeskillRecipe_Struct*> specList;

			bool result = Database::Instance()->SearchTradeRecipe(search_criteria,0,&specList);

			if(!result) {
					c->Message(BLACK, "No recipes found");
				return;
			}
			
			c->Message(BLACK, "Found %i Recipes", specList.size());
				
				 for (int i=0; i<specList.size();i++) {  

					 c->Message(BLACK, "Recipe %i: %s tradeskill: %i trivial: %i", specList[i]->recipeID, specList[i]->name,specList[i]->tradeskill, specList[i]->trivial);
				
					std::stringstream stringstream;			

					vector< pair<uint32,uint8> >::iterator	itr =  specList[i]->oncombine.begin();

					while(itr !=  specList[i]->oncombine.end()) {

						const Item_Struct* item = 0;
						
						item = Database::Instance()->GetItem(itr->first);
						
						int second = itr->second;

						if(second!=0) {	
							stringstream << second << " x ";
						}
						if(item!=0) {
							stringstream << item->name << " (" << itr->first << ")";
						} else {
							stringstream << " UNKNOWN (" << itr->first << ")";
						}
							
						stringstream << "\n";
						itr++;
					}

					c->Message(BLACK, "%s", stringstream.str().c_str());
									
					if(i == 19) {
						c->Message(BLACK, "Search result has been limit to 20 entries");
					}
				}			
			
		}
}

void command_receipesummon(Client *c, const Seperator *sep)
{
		if (sep->arg[1][0] == 0)
			c->Message(BLACK, "Usage: #receipesummon <recipeid>");
		else
		{
			int recipeID=atoi(sep->argplus[1]);
			
			 vector<DBTradeskillRecipe_Struct*> specList;

			bool result = Database::Instance()->SearchTradeRecipe(0,recipeID,&specList);

			if(!result) {
				c->Message(BLACK, "No items found");
				return;
			}
			
			c->Message(BLACK, "Summoning Recipe Items");
				
				 for (int i=0; i<1;i++) {  

			 		vector< pair<uint32,uint8> >::iterator	itr =  specList[i]->oncombine.begin();
				
					while(itr !=  specList[i]->oncombine.end()) {

						c->SummonItem(itr->first, itr->second);
						itr++;
					}				
				}			
			
		}
} 


void command_clearcursor(Client *c, const Seperator *sep)
{
		
		c->Message(BLACK, "Clearing player cursor items server side...");
	
		if (c->GetTarget() != 0 && c->GetTarget()->IsClient()) {
				c->GetTarget()->CastToClient()->GetPlayerProfilePtr()->inventory[0] = 0xFFFF;
		} else {
			c->GetPlayerProfilePtr()->inventory[0] = 0xFFFF; 
		}	
}

void command_reloadperl(Client *c, const Seperator *sep)
{	
		c->Message(BLACK, "Reloading Perl/Quests...");
	
		#ifdef EMBPERL
			perlParser->ReloadQuests();
		#endif
}	

void command_summonitemnonblob(Client *c, const Seperator *sep)
{
		if (!sep->IsNumber(1)) {
			c->Message(BLACK, "Usage: #summonitem2 x , x is an item number");
		}
		else {
			int16 itemid = atoi(sep->arg[1]);			
			c->SummonItemNonBlob(itemid);
		}
} 

void command_diffitem(Client *c, const Seperator *sep)
{
		if (!sep->IsNumber(1)) {
			c->Message(BLACK, "Usage: #diffitem x , x is an item number");
		}
		else {
			int16 itemid = atoi(sep->arg[1]);			
			Item_Struct* itemBlob = Database::Instance()->GetItem(itemid);	
			Item_Struct* item = Database::Instance()->GetItemNonBlob(itemid);

			if (itemBlob == 0) {
				c->Message(BLACK, "No such item: %i", itemid);				
			}

			APPLAYER* outapp = new APPLAYER(OP_SummonedItem, sizeof(SummonedItem_Struct));
			memcpy(outapp->pBuffer, item, sizeof(Item_Struct));
	
			APPLAYER* outappBlob = new APPLAYER(OP_SummonedItem, sizeof(SummonedItem_Struct));
			memcpy(outappBlob->pBuffer, itemBlob, sizeof(Item_Struct));

			for(int i=0; i < sizeof(Item_Struct);i++) {
								
				// dont show diff for name, price , class, races
				if(outappBlob->pBuffer[i] == outapp->pBuffer[i] || i < 100 || i == 140 || i == 141 || i == 142 || i == 143 || i == 208 || i == 209 || i == 212 || i == 213) {
				} else {
					char *unknownData = 0;

					MakeAnyLenString(&unknownData,"");

					//unknown0144
					if(i>=144 && i<172) {
						MakeAnyLenString(&unknownData,"(unknown0144)");
					}
					if(i==196) {						
						MakeAnyLenString(&unknownData,"(clicklevel2)");
					}

					if(i>=222 && i<232) {						
						MakeAnyLenString(&unknownData,"(unknown0222)");
					}
					
					c->Message(BLACK, "diff pos %i blob %02X nonblob %02X %s", i, (unsigned char)outappBlob->pBuffer[i],(unsigned char)outapp->pBuffer[i],unknownData);
					EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"diff pos %i blob %02X nonblob %02X %s", i, (unsigned char)outappBlob->pBuffer[i],(unsigned char)outapp->pBuffer[i],unknownData);															
				}				

			}
		}
	} 

void command_teleport(Client *c, const Seperator *sep)
{

		if (sep->arg[1][0] == 0)
		{
			c->Message(BLACK, "Usage: #teleport [zonename] ([x] [y] [z])");
		}
		else  {

			float x = 0;
			float y = 0;
			float z = 0;	
			float heading = 0; 

			if (sep->IsNumber(2) && sep->IsNumber(3) && sep->IsNumber(3)) {
				x = atof(sep->arg[2]);
				y = atof(sep->arg[3]);
				z = atof(sep->arg[4]);
			} 
			if (sep->IsNumber(4)) {
				heading = atof(sep->arg[5]);
			}
						
			c->Message(BLACK, "Teleporting you to zone name = %s, x = %f, y = %f, z = %f", sep->arg[1], x, y, z);

			c->TeleportPC(sep->arg[1], x, y, z,heading);
		}
		
}

void command_translocate(Client *c, const Seperator *sep)
{
	
		if (sep->arg[1][0] == 0)
		{
			c->Message(BLACK, "Usage: #translocate [zonename] ([x] [y] [z])");
		}
		else  {

			float x = 0;
			float y = 0;
			float z = 0;				

			if (sep->IsNumber(2) && sep->IsNumber(3) && sep->IsNumber(3)) {
				x = atof(sep->arg[2]);
				y = atof(sep->arg[3]);
				z = atof(sep->arg[4]);
			} 		
						
			c->Message(BLACK, "Translocating you to zone name = %s, x = %f, y = %f, z = %f", sep->arg[1], x, y, z);

			c->TranslocatePC(sep->arg[1], x, y, z);
		}
		
}

void command_givemoney(Client *c, const Seperator *sep)
{

	if(sep->arg[1][0]==0 && sep->arg[2][0]==0 && sep->arg[3][0]==0 && sep->arg[4][0]==0) {
		c->Message(BLACK,"FORMAT: #money <copper> <silver> <gold> <plat>");
	}
	else {
		int16 plat = atoi(sep->arg[4]);
		int16 gold = atoi(sep->arg[3]);
		int16 silver = atoi(sep->arg[2]);
		int16 copper = atoi(sep->arg[1]);
		if( c->GetTarget() != 0 && c->GetTarget()->IsClient()){
			 c->GetTarget()->CastToClient()->AddMoneyToPP(copper, silver, gold, plat,false);
			c->GetTarget()->Message(BLACK, "Your coin purse feels heavier..., please zone");
		}else{
			c->AddMoneyToPP(copper, silver, gold, plat,false);
			c->Message(BLACK, "Your coin purse feels heavier..., please zone");
		}
	}
}

void command_castspell(Client *c, const Seperator *sep)
{
		Spell* spell = NULL;
		int16 spellid = 0;
		if (!sep->IsNumber(1)) {
			if (strlen(sep->argplus[1]) < 4 || strlen(sep->argplus[1]) > 62) {
				c->Message(BLACK, "Error: #CastSpell: Spell name incorrect.");
			}
			char sName[64]; memset(sName, 0, sizeof(sName));
			strcpy(sName, sep->argplus[1]);
			spell = spells_handler.GetSpellByName(sName);
		}	
		else {
			spellid = atoi(sep->arg[1]);
			spell = spells_handler.GetSpellPtr(spellid);
			if (spellid >= (SPDAT_RECORDS + NUMBER_OF_CUSTOM_SPELLS) || !spell)
				c->Message(BLACK, "Error: #CastSpell: Arguement out of range.");
		}
		if (spell) {
			spellid = spell->GetSpellID();
			if ((spellid == 982 || spellid == 905) && c->Admin() < 100)
				c->SetTarget(c);
			c->SetCastingSpellLocationX(c->GetX());
			c->SetCastingSpellLocationY(c->GetY());
			c->SpellFinished(spell, ((c->GetTarget() == 0) ? 0 : c->GetTarget()->GetID()) );
		}
		else
			c->Message(BLACK, "Error: #CastSpell: Spell not found.");
}

void command_invulnerable(Client *c, const Seperator *sep)
{
		Client* client = c;
		if (c->GetTarget()) {
			if (c->GetTarget()->IsClient())
				client = c->GetTarget()->CastToClient();
		}
		if (sep->arg[1][0] == '1') {
			client->Message(BLACK, "You are now invulnerable from attack and spells.");
			client->SetInvulnerable(true);
			client->SetSpellImmunity(true);
		}
		else if (sep->arg[1][0] == '0') {
			client->Message(BLACK, "You are no longer invulnerable from attack and spells.");
			client->SetInvulnerable(false);
			client->SetSpellImmunity(false);
		}
		else
			client->Message(BLACK, "Usage: #invulnerable [1/0]");
	}


void command_listlang(Client *c, const Seperator *sep)
{	
		for(int i=LANGUAGE_COMMON_TONGUE; i <= LANGUAGE_DARK_SPEECH;i++) {
			c->Message(BLACK, "%i %s",i,GetLanguageName(i));
		}
}

void command_setlang(Client *c, const Seperator *sep)
{
		if (c->GetTarget() == 0 || !c->GetTarget()->IsClient()) {
			c->Message(BLACK, "Error: #setlang: No valid target.");
		}
		else if (!sep->IsNumber(1) || atoi(sep->arg[1]) < 0 || atoi(sep->arg[1]) > 23) {
			c->Message(BLACK, "Usage: #setlang language x ");
			c->Message(BLACK, "       language = 0 to 23 use #listlang for possible values");
			c->Message(BLACK, "       x = 0 to 100");
			c->Message(BLACK, "NOTE: skill values greater than 250 may cause the skill to become unusable on the client.");
		}
		else if (!sep->IsNumber(2) || atoi(sep->arg[2]) < 0 || atoi(sep->arg[2]) > 100) {
			c->Message(BLACK, "Usage: #setlang language x ");
			c->Message(BLACK, "       language = 0 to 23 use #listlang for possible values");
			c->Message(BLACK, "       x = 0 to 100");		
		}
		else {			
			cout << "Setting " << c->GetTarget()->GetName() << " language " << sep->arg[1] << " to " << sep->arg[2] << endl;
			int languageID = atoi(sep->arg[1]);
			int8 skillVal = (int8)atoi(sep->arg[2]);
			c->GetTarget()->CastToClient()->SetLanguageSkill(languageID,skillVal);
			c->GetTarget()->CastToClient()->Save();
			c->Message(BLACK, "Please zone/logout to update the clients language skill!");
		}
	}

void command_setskill(Client *c, const Seperator *sep)
{

		if (c->GetTarget() == 0) {
			c->Message(BLACK, "Error: #setskill: No target.");
		}
		else if (!sep->IsNumber(1) || atoi(sep->arg[1]) < 0 || atoi(sep->arg[1]) > 73) {
			c->Message(BLACK, "Usage: #setskill skill x ");
			c->Message(BLACK, "       skill = 0 to 73");
			c->Message(BLACK, "       x = 0 to 255");
			c->Message(BLACK, "NOTE: skill values greater than 250 may cause the skill to become unusable on the client.");
		}
		else if (!sep->IsNumber(2) || atoi(sep->arg[2]) < 0 || atoi(sep->arg[2]) > 255) {
			c->Message(BLACK, "Usage: #setskill skill x ");
			c->Message(BLACK, "       skill = 0 to 73");
			c->Message(BLACK, "       x = 0 to 255");
		}
		else {
			//pp.skills[atoi(sep.arg[1])] = (int8)atoi(sep.arg[2]);
			cout << "Setting " << c->GetTarget()->GetName() << " skill " << sep->arg[1] << " to " << sep->arg[2] << endl;
			int skill_num = atoi(sep->arg[1]);
			int8 skill_id = (int8)atoi(sep->arg[2]);
			c->GetTarget()->SetSkill(skill_num, skill_id);
		}
	}

void command_setskillall(Client *c, const Seperator *sep)
{
		if (c->GetTarget() == 0 || !c->GetTarget()->IsClient()) {
			c->Message(BLACK, "Error: #setskillall: No client targeted.");
		}
		else if (!sep->IsNumber(1) || atoi(sep->arg[1]) < 0 || atoi(sep->arg[1]) > 252) {
			c->Message(BLACK, "Usage: #setskillall value ");
			c->Message(BLACK, "       value = 0 to 252");
		}
		else {
			//pp.skills[atoi(sep.arg[1])] = (int8)atoi(sep.arg[2]);
			cout << "Setting ALL of " << c->GetTarget()->GetName() << "'s skills to " << sep->arg[1] << endl;
			int8 setNumber = atoi(sep->arg[1]);
			int16 maxSkill = 0;
			for(int skill_num = 0; skill_num <= 73; skill_num++)
			{
				//Yeahlight: Check for the max skill before we put the supplied number into the skill
				maxSkill = c->GetTarget()->CheckMaxSkill(skill_num, c->GetTarget()->GetRace(), c->GetTarget()->GetClass(), c->GetTarget()->GetLevel());
				//Yeahlight: The skill's maximum for this target's class and level is lower than the supplied value
				if(maxSkill <= setNumber && maxSkill != 0)
					c->GetTarget()->CastToClient()->SetSkill(skill_num, maxSkill);
				else if(maxSkill != 0)
					c->GetTarget()->CastToClient()->SetSkill(skill_num, setNumber);
			}
			//Yeahlight: Refresh target's mitigation and avoidance
			c->GetTarget()->CalculateACBonuses();
		}
	}

void command_checkskillall(Client *c, const Seperator *sep)
{
	
		if (c->GetTarget() == 0)
		{
			c->Message(BLACK, "Error: #setallskill: No client targeted.");
		}
		if(c->GetTarget()->IsNPC())
		{
			for(int i = 0; i <= 74; i++)
				c->Message(WHITE, "Debug: %s's skill %i: %i", c->GetTarget()->CastToNPC()->GetProperName(), i, c->GetTarget()->GetSkill(i));
		}
		else if(c->GetTarget()->IsClient())
		{
			for(int i = 0; i <= 74; i++)
				c->Message(WHITE, "Debug: %s's skill %i: %i", c->GetTarget()->GetName(), i, c->GetTarget()->CastToClient()->GetSkill(i));
		}
}

void command_save(Client *c, const Seperator *sep)
{	
		if (c->GetTarget() == 0)
			c->Message(BLACK, "Error: no target");
		else if (!c->GetTarget()->IsClient())
			c->Message(BLACK, "Error: target not a client");
		else if	(c->GetTarget()->CastToClient()->Save())
			c->Message(BLACK, "%s successfully saved.", c->GetTarget()->GetName());
		else
			c->Message(BLACK, "Manual save for %s failed.", c->GetTarget()->GetName());
}

void command_scribespells(Client *c, const Seperator *sep)
{
		if(sep->arg[1][0]==0) {
			c->Message(BLACK,"FORMAT: #scribespells <level>");
		} else if((atoi(sep->arg[1]) < 1) || (atoi(sep->arg[1]) > 60)) {
			c->Message(BLACK,"ERROR: Enter a level between 1 and 60 inclusive.");
		} else if (c->GetTarget() == 0 || !c->GetTarget()->IsClient()) {
			c->Message(BLACK,"ERROR: This command requires a valid target.");		
		} else {
		
			int level = atoi(sep->arg[1]);
			
			// iterate over all spells
			int slotId = 0;

			// im to lazy, iterate over the spdat N times for each level to 
			// make it scribe by Spell Level
			for(int spellLevel=0; spellLevel<=level; spellLevel++) {
			
				for (int spellid = 0; spellid < SPDAT_RECORDS; spellid++) {	
				
					Spell* spell = spells_handler.GetSpellPtr(spellid);
					
					int Canuse = spell->CanUseSpell(c->GetTarget()->GetClass(), spellLevel);

					if (Canuse != 0) {
						
						if(spell->GetSpellClass(c->GetTarget()->GetClass()-1) == spellLevel) {							
							c->Message(DARK_BLUE, "Scribing level %i spell %s", spellLevel, spell->GetSpellName());
							c->GetTarget()->CastToClient()->GetPlayerProfilePtr()->spell_book[slotId] = spellid;
							slotId++;
						} 						
					}
				}
			}			

			c->Message(BLACK, "Scribing spells to spellbook done (zone to take effect)");		

		}
}

void command_flymode(Client *c, const Seperator *sep)
{
		if (strlen(sep->arg[1]) == 1 && !(sep->arg[1][0] == '0' || sep->arg[1][0] == '1' || sep->arg[1][0] == '2')) {
			c->Message(BLACK, "#flymode [0/1/2]");
		}
		else
		{
			int16 Spawn_ID = -1;

			if (c->GetTarget() == 0 || !c->GetTarget()->IsClient() || c->Admin() < 100)
			{
				Spawn_ID = c->GetID();

				if (sep->arg[1][0] == '1')
				{
					c->Message(BLACK, "Turning Flymode ON");
				}
				else if (sep->arg[1][0] == '2')
				{
					c->Message(BLACK, "Turning Flymode LEV");
				}
				else
				{
					c->Message(BLACK, "Turning Flymode OFF");
				}
			}
			else
			{
				Spawn_ID = c->GetTarget()->GetID();

				if (sep->arg[1][0] == '1')
				{
					c->Message(BLACK, "Turning %s's Flymode ON", c->GetTarget()->GetName());
				}
				else if (sep->arg[1][0] == '2')
				{
					c->Message(BLACK, "Turning %s's Flymode LEV", c->GetTarget()->GetName());
				}
				else
				{
					c->Message(BLACK, "Turning %s's Flymode OFF", c->GetTarget()->GetName());
				}
			}		
			c->SendAppearancePacket(Spawn_ID, SAT_Levitate, atoi(sep->arg[1]), true);
		}
}

void command_depopzone(Client *c, const Seperator *sep)
{
		zone->Depop();
		c->Message(BLACK, "Zone depoped.");
}

void command_repopzone(Client *c, const Seperator *sep)
{
		if (sep->IsNumber(1)) {
			zone->Repop(atoi(sep->arg[1])*1000);
			c->Message(BLACK, "Zone depoped. Repop in %i seconds", atoi(sep->arg[1]));
		}
		else {
			zone->Repop();
			c->Message(BLACK, "Zone depoped. Repoping now.");
		}
}
void command_sky(Client *c, const Seperator *sep)
{
	//Change the sky type of the current zone. Useful in newzone_struct testing. Maybe for GM events too. -Wizzel
	if(sep->arg[1][0]==0)
		c->Message(BLACK, "FORMAT: zsky <sky type>");
	else if(atoi(sep->arg[1])<0||atoi(sep->arg[1])>255)
		c->Message(BLACK, "ERROR: Sky type can not be less than 0 or greater than 255.");
	else {
		zone->newzone_data.sky = atoi(sep->arg[1]);

		APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
		memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
		entity_list.QueueClients(c, outapp);
		safe_delete(outapp);
	}
}
void command_fogcolor(Client *c, const Seperator *sep)
{
	//Change the fog color of the current zone. Useful in newzone_struct testing. Maybe for GM events too. -Wizzel
	if(sep->arg[3][0]==0)
	c->Message(BLACK, "FORMAT: #fogcolor <r/g/b>");
	else if(atoi(sep->arg[1])<0||atoi(sep->arg[1])>255)
		c->Message(BLACK, "ERROR: Red can not be less than 0 or greater than 255!");
	else if(atoi(sep->arg[2])<0||atoi(sep->arg[2])>255)
		c->Message(BLACK, "ERROR: Green can not be less than 0 or greater than 255!");
	else if(atoi(sep->arg[3])<0||atoi(sep->arg[3])>255)
		c->Message(BLACK, "ERROR: Blue can not be less than 0 or greater than 255!");
	else {
		zone->newzone_data.fog_red[0] = atoi(sep->arg[1]);
		zone->newzone_data.fog_green[0] = atoi(sep->arg[2]);
		zone->newzone_data.fog_blue[0] = atoi(sep->arg[3]);
	}

	APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
	memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
	entity_list.QueueClients(c, outapp);
	safe_delete(outapp);
}
void command_zoneappearance(Client *c, const Seperator *sep) 
{
	int8 sky;
	int8 fog_red;
	int8 fog_green;
	int8 fog_blue;
	float fog_minclip;
	float fog_maxclip;
	float minclip;
	float maxclip;
	int8 ztype;
	float safe_x;
	float safe_y;
	float safe_z;
	float underworld;

	//Grab the zone ID
	int16 zoneID = Database::Instance()->LoadZoneID(zone->GetShortName());

	Database::Instance()->GetZoneCFG(zoneID, &sky, &fog_red, &fog_green, &fog_blue, &fog_minclip, &fog_maxclip, &minclip, &maxclip, &ztype, &safe_x, &safe_y, &safe_z, &underworld);

	c->Message(BLACK, "Zone Appearance:");
	c->Message(BLACK, "  Zone: %s",zone->GetShortName()); 
	c->Message(BLACK, "  Zone ID: %i",zoneID);
	c->Message(BLACK, "  Sky: %i",sky); 
	c->Message(BLACK, "  Fog-Red: %i",fog_red);  
	c->Message(BLACK, "  Fog-Green: %i",fog_green);
	c->Message(BLACK, "  Fog-Blue: %i",fog_blue);
	c->Message(BLACK, "  Fog Minclip: %f",fog_minclip);
	c->Message(BLACK, "  Fog Maxclip: %f",fog_maxclip);
	c->Message(BLACK, "  Minclip: %f",minclip);
	c->Message(BLACK, "  Maxclip: %f",maxclip);
	c->Message(BLACK, "  Zone Type: %i",ztype);
	c->Message(BLACK, "  Safe X: %f",safe_x);
	c->Message(BLACK, "  Safe Y: %f",safe_y);
	c->Message(BLACK, "  Safe Z: %f",safe_z);
	c->Message(BLACK, "  Underworld: %f",underworld);
}
void command_zoneclip(Client *c, const Seperator *sep)
{
	//Change the clip plane of the current zone. Useful in newzone_struct testing. -Wizzel
	if(sep->arg[2][0]==0)
			c->Message(BLACK, "Usage: #zclip <min clip> <max clip>");
		else if(atoi(sep->arg[1])<=0)
			c->Message(BLACK, "ERROR: Min clip can not be zero or less!");
		else if(atoi(sep->arg[2])<=0)
			c->Message(BLACK, "ERROR: Max clip can not be zero or less!");
		else if(atoi(sep->arg[1])>atoi(sep->arg[2]))
			c->Message(BLACK, "ERROR: Min clip is greater than max clip!");
		else {
			zone->newzone_data.minclip = atof(sep->arg[1]);
			zone->newzone_data.maxclip = atof(sep->arg[2]);

			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(c, outapp);
			safe_delete(outapp);//delete outapp;
		}
}
void command_debug(Client *c, const Seperator *sep)
{
	if(atoi(sep->arg[1]) == 0)
	{
		c->SetDebugMe(false);
		c->Message(WHITE, "Debug: Turning debug mode OFF");
	}
	else
	{
		c->SetDebugMe(true);
		c->Message(WHITE, "Debug: Turning debug mode ON");
	}
}

void Client::ProcessCommand(char* message) 
{

	command_realdispatch(this,message);

	// Harakiri 
	// TODO - if you need one of these old functions - create a method similar to
	// command_goto and add it to command_init, then remove it below
	
	

	Seperator sep(message);
	if ((strcasecmp(sep.arg[0], "#makeleader") == 0)) // this only works in the current zone
	{
		if(strlen(sep.arg[1]) == 0)
		{
			if (target == 0)
			{
				Message(BLACK, "Error: #makeleader: No target.");
				return;
			}
			else if(!target->IsClient())
			{
				Message(BLACK, "Usage: #makeleader: Target must be a player.");
				return;			}
			else
			{
				this->ProcessChangeLeader(target->CastToClient()->GetName());
				return;
			}
		}
		else
			this->ProcessChangeLeader(sep.arg[1]);
	}	
	if ((strcasecmp(sep.arg[0], "#gotopc") == 0)) // this only works in the current zone.... need to figure out away for it to work across zone!
	{
		if(strlen(sep.arg[1]) == 0)
		{
			Message(BLACK, "Usage: #gotopc pcname");
			return;
		}
		
		char* name = sep.arg[1];

		Client* c = entity_list.GetClientByName(name);
				
		if(c == 0)
		{
			Message(BLACK, "Unable to locate player %s. (or not in the same zone)", name);
			return;
		}
		else
		{
			this->Message(YELLOW, "You warp to %s", name);
			c->Message(YELLOW, "You feel the presances of %s", this->name);
			this->MovePC(0, c->GetX(), c->GetY(), c->GetZ()); // showing "You have been summoned in the client"
			return;
		}
	}	
	else if (strcasecmp(sep.arg[0], "#remove") == 0 && (admin >= EQC_Alpha_Tester)) 
	{
		if (this->merchantid==0) 
		{
			Message(BLACK, "Error: #remove: You are not looking at merchant goods.");
		}
		else if (!sep.IsNumber(1)) 
		{
			Message(BLACK, "Usage: #remove itemid.");		
		}
		else 
		{
			sint32 itemid = atoi(sep.arg[1]);

			if (!Database::Instance()->RemoveMerchantItem(this->merchantid,itemid))
			{			
				Message(BLACK, "The merchant does not have your item check the id !.");
			}
			else
			{
				Message(BLACK, "Item has been removed correctly.");
			}
		}
	}
	else if (strcasecmp(sep.arg[0], "#add") == 0 && (admin >= 5)) 
	{
		if (this->merchantid==0) 
		{
			Message(BLACK, "Error: #add: You are not looking at merchant goods.");
		}
		else if (!sep.IsNumber(1)) 
		{
			Message(BLACK, "Usage: #add itemid.");		
		}
		else 
		{
			sint32 itemid = atoi(sep.arg[1]);

			if (!Database::Instance()->GetItem(itemid))
			{			
				Message(BLACK, "Your item does not exist please check the id !.");
			}
			else
			{
				Database::Instance()->AddMerchantItem(this->merchantid,itemid,0,Database::Instance()->GetLastSlot(this->merchantid,this->startingslot)+1);
			}
		}
	}
	else if (strcasecmp(sep.arg[0], "#removeall") == 0 && (admin >= 5)) 
	{
		if (this->merchantid==0) 
		{
			Message(BLACK, "Error: #remove: You are not looking at merchant goods.");
		}
		else 
		{	
			for(int i=0;i<this->totalitemdisplayed;i++){
				sint32 itemid = this->merchantgoods[i];

				if (!Database::Instance()->RemoveMerchantItem(this->merchantid,itemid))
				{			
					Message(BLACK, "The merchant does not have your item check the id !.");
				}
				else
				{
					Message(BLACK, "Item has been removed correctly.");
				}
			}
		}
	}
	
	if (strcasecmp(sep.arg[0], "#SendTo") == 0 || strcasecmp(sep.arg[0], "#st") == 0 && (admin >= 20)) 
	{

				if(GetTarget()->IsNPC()){
					float to_x = this->GetX();
					float to_y = this->GetY();
					//GetTarget()->CastToNPC()->MoveTowards(to_x,to_y, 3);
					GetTarget()->CastToNPC()->SetTarget(this);
					GetTarget()->CastToNPC()->SetDestination(to_x, to_y, this->GetZ());
					GetTarget()->CastToNPC()->StartWalking();
					char msg_[100]; 
					sprintf(msg_,"Sending mob (%s) to new position (%i,%i).", GetTarget()->CastToNPC()->GetName(),(int)to_x, (int)to_y );
					Message(BLACK, msg_);
				}
			//}

	
	}
	if (strcasecmp(sep.arg[0], "#date")==0 && admin >= 80) 
	{
		if(sep.arg[5][0] == 0) 
		{
			Message(BLACK, "Usage: #date <hour (24 hour format)> <minute> <day of month> <month> <year>");
		}
		else 
		{
			APPLAYER* timepacket = CreateTimeOfDayPacket(atoi(sep.arg[1]),atoi(sep.arg[2]), atoi(sep.arg[3]), atoi(sep.arg[4]), atoi(sep.arg[5]));
			entity_list.QueueClients(this, timepacket);
			safe_delete(timepacket);//delete timepacket;
		}
	}
	// Kaiyodo - #haste command to set client attack speed. Takes a percentage (100 = twice normal attack speed)
	//else if (strcasecmp(sep.arg[0], "#haste")==0 && admin >= 100)
	//{
	//	if(sep.arg[1][0] != 0)
	//	{
	//		int16 Haste = atoi(sep.arg[1]);
	//		SetHaste(Haste);
	//		// SetAttackTimer must be called to make this take effect, so player needs to change
	//		// the primary weapon.
	//		Message(BLACK, "Haste set to %d%% - Need to re-equip primary weapon before it takes effect", Haste);
	//	}
	//	else
	//	{
	//		Message(BLACK, "Usage: #haste x");
	//	}
	//}
	if(strcasecmp(sep.arg[0],"#weather") == 0 && (admin >= 80))
	{
		//TODO: Make a Weather Struct and impliment it here!!!! - Dark-Prince 29/01/2008
		//TODO: Clean this up to use APPLAYER* outapp = new APPLAYER(struct) - Dark-Prince 29/01/2008
		if (!(sep.arg[1][0] == '0' || sep.arg[1][0] == '1' || sep.arg[1][0] == '2' || sep.arg[1][0] == '3')) {
			Message(BLACK, "Usage: #weather <0/1/2/3> - Off/Rain/Snow/Manual.");
		}
		else if(zone->zone_weather == 0) { 
			if(sep.arg[1][0] == '3')	{ // Put in modifications here because it had a very good chance at screwing up the client's weather system if rain was sent during snow -T7
				if(sep.arg[2][0] != 0 && sep.arg[3][0] != 0) { 
					Message(BLACK, "Sending weather packet... TYPE=%s, INTENSITY=%s", sep.arg[2], sep.arg[3]);
					zone->zone_weather = (WeatherTypesEnum)(int8)atoi(sep.arg[2]);
					
					APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
					memset(outapp->pBuffer, 0, 8);
					
					outapp->pBuffer[0] = atoi(sep.arg[2]);
					outapp->pBuffer[4] = atoi(sep.arg[3]); 
					entity_list.QueueClients(this, outapp);
					safe_delete(outapp);//delete outapp;
				}
				else {
					Message(BLACK, "Manual Usage: #weather 3 <type> <intensity>");
				}
			}
			else if(sep.arg[1][0] == '2')	{
				entity_list.Message(0, BLACK, "Snowflakes begin to fall from the sky.");
				zone->zone_weather = WEATHER_SNOW;
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				memset(outapp->pBuffer, 0, 8);
				
				outapp->pBuffer[0] = 0x01;
				outapp->pBuffer[4] = 0x02; // This number changes in the packets, intensity?
				entity_list.QueueClients(this, outapp);
				safe_delete(outapp);//delete outapp;
			}
			else if(sep.arg[1][0] == '1')	{
				entity_list.Message(0, BLACK, "Raindrops begin to fall from the sky.");
				zone->zone_weather = WEATHER_RAIN;
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				memset(outapp->pBuffer, 0, 8);
				outapp->pBuffer[4] = 0x01; // This is how it's done in Fear, and you can see a decent distance with it at this value
				entity_list.QueueClients(this, outapp);
				safe_delete(outapp);//delete outapp;
			}
		}
		else	
		{
			APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
			

			if(zone->zone_weather == 1)	
			{ 
				// Doing this because if you have rain/snow on, you can only turn one off.
				entity_list.Message(0, BLACK, "The sky clears as the rain ceases to fall.");
				zone->zone_weather = WEATHER_OFF;
				// To shutoff weather you send an empty 8 byte packet (You get this everytime you zone even if the sky is clear)
			}
			else if(zone->zone_weather == 2)
			{
				entity_list.Message(0, BLACK, "The sky clears as the snow stops falling.");
				zone->zone_weather = WEATHER_OFF;
				outapp->pBuffer[0] = 0x01; // Snow has it's own shutoff packet
			}
			else 
			{
				entity_list.Message(0, BLACK, "The sky clears.");
				zone->zone_weather = WEATHER_OFF;
			}

			memset(outapp->pBuffer, 0, 8);
			
			entity_list.QueueClients(this, outapp);
			safe_delete(outapp);//delete outapp;

		}
	}		
	if (strcasecmp(sep.arg[0], "#shutdown")==0 && admin >= 200) //TODO: refactor this to be a seperate if clause Dark-Prince - 29/01/2008
	{
		CatchSignal(2);
	}
	//TODO: Split Help into its other mehtod to make it more tider
	if (strcasecmp(sep.arg[0], "#help") == 0) {
		if (!(strcasecmp(sep.arg[1], "normal") == 0 
			|| strcasecmp(sep.arg[1], "tester") == 0
			|| strcasecmp(sep.arg[1], "privuser") == 0
			|| strcasecmp(sep.arg[1], "vprivuser")== 0 
			|| strcasecmp(sep.arg[1], "gmquest")== 0
			|| strcasecmp(sep.arg[1], "gm")== 0
			|| strcasecmp(sep.arg[1], "leadgm")== 0
			|| strcasecmp(sep.arg[1], "debug")== 0
			|| strcasecmp(sep.arg[1], "serverop")== 0)) {
				Message(BLACK, "Usage: #help <helpfile>");
				Message(BLACK, "  Normal - List of general commands.");
				if(admin >= 5)		{	Message(BLACK, "  Tester - Complete list of EQC alpha tester commands."); }
				if(admin >= 10)		{	Message(BLACK, "  Privuser - Privileged user commands."); }
				if(admin >= 20)		{	Message(BLACK, "  vPrivuser - Very privileged user commands."); }
				if(admin >= 80)		{	Message(BLACK, "  GMQuest - Quest GM commands."); }
				if(admin >= 100)	{	Message(BLACK, "  GM - Game master commands."); }
				if(admin >= 150)	{	Message(BLACK, "  LeadGM - Lead GM commands."); }
				if(admin >= 200)	{	Message(BLACK, "  ServerOP - Server operator commands."); }
				if(admin >= 201)	{	Message(BLACK, "  Debug - Debugging commands."); }
		}
		else if(strcasecmp(sep.arg[1], "normal") == 0)
		{
			Message(BLACK, "General commands:");
			Message(BLACK, "  None at this time.");
		}
		else if(strcasecmp(sep.arg[1], "tester") == 0 && admin >= 5)
		{
			Message(BLACK, "EQC Tester Commands:");
			Message(BLACK, "  #Addexp - Adds exp to the target. Use this to level up your character correctly.");
			Message(BLACK, "  #Castspell [id/name] - Casts a spell, use #Findspell to determine the id number or enter the name directly.");
			Message(BLACK, "  #Checkskillall - Returns all your skill values.");
			Message(BLACK, "  #Damage [x] - Inflicts damage upon target.");
			Message(BLACK, "  #Debug - Toggle Cofruben\'s debug system.");
			Message(BLACK, "  #Depop - Depops targeted NPC.");
			Message(BLACK, "  #Depopzone - Depops the zone.");
			Message(BLACK, "  #Findspell [spellname] - Searches the spell database for [spellname].");
			Message(BLACK, "  #Flymode [0/1/2] - Allows you to move freely on the z axis.");
			Message(BLACK, "  #Gender [0/1/2] - (0 = male, 1 = female, 2 = monster).");
			Message(BLACK, "  #Goto [x,y,z] - Warps you to specified coordinates. Note: Coords printed in GM safe rooms are typically [y,x,z].");
			Message(BLACK, "  #Guild - Guild commands (type for more info)");
			Message(BLACK, "  #Heal - (PC only) Completely heals your target.");
			Message(BLACK, "  #Itemsearch [string] - Browses item database and returns matching item IDs.");
			Message(BLACK, "  #Invul [1/0] - Makes the targeted player invulnerable to attack.");
			Message(BLACK, "  #Kill - Kills your selected target.");
			Message(BLACK, "  #Loc - Shows you your current location.");
			Message(BLACK, "  #Listlang - Lists all languages and their id");
			Message(BLACK, "  #Mana - (PC only) Replenishes your target\'s mana.");
			Message(BLACK, "  #Nukebuffs - Dispells your current target. Note: This will not remove the client side buff icons from players!");
			Message(BLACK, "  #Scribespells [1-60] - Scribes all spells for your class to desired level.");
			Message(BLACK, "  #Setlang [language num (0-23)] [number (0-255)] - Sets the targeted player\'s language skill to number. Use listlang to get the language ids.");		
			Message(BLACK, "  #Setskill [skill num (0-73)] [number (0-255)] - Sets the targeted player\'s skill to number.");		
			Message(BLACK, "  #Setskillall [0-255] - Adjusts all the targeted PC\'s skills. Note: This will NOT increase skills that must be trained!");
			Message(BLACK, "  #Showbuffs - Shows the buffs on your current target.");
			Message(BLACK, "  #Showstats - Returns the server side stats of your target.");
			Message(BLACK, "  #Sic [serverName] - Force target to attack [serverName]. Note: serverName is diplayed in #showstats, ie \'Trakanon02\'.");
			Message(BLACK, "  #Size [size] - Sets your targets size (0 for base size).");
			Message(BLACK, "  #Summonitem [id] - Summons an item.");
			Message(BLACK, "  #Race [0-255] - Sets the race of your target (0 for base race).");
			Message(BLACK, "  #Repop [delay] - Repops the zone, optional delay in seconds.");
			Message(BLACK, "  #Worldkick [id] - Removes the supplied ID from the actives account table. Note: Use \'/who all\' to obtain the account ID");
			Message(BLACK, "  #YLdebug [0/1] - Toggle YL\'s debug messages.");
			Message(BLACK, "  #Zone [zonename] - Zones to safepoint in the specified zone. This command only accepts proper zone names, i.e. \'freporte\' and \'cabeast\'.");
		}
		else if(strcasecmp(sep.arg[1], "privuser") == 0 && admin >= 10)
		{
			Message(BLACK, "EQC Priviliged User Commands:");
			Message(BLACK, "  #level [id] - Sets your target\'s level.");
			Message(BLACK, "  #heal - (PC only) Completely heals your target.");
			Message(BLACK, "  #mana - (PC only) Replenishes your target\'s mana.");
			Message(BLACK, "  #spawn - Used to spawn NPCs");
			Message(BLACK, "  #dbspawn [npctypeid] - Spawns an NPC type from the database.");
			Message(BLACK, "  #texture [texture] [helmtexture]	(0-255, 255 for show equipment)");
			Message(BLACK, "  #gender [0-2] - (0=male, 1=female, 2=neuter)");
			Message(BLACK, "  #npctypespawn [npctype]");
		}
		else if(strcasecmp(sep.arg[1], "vprivuser") == 0 && admin >= 20)
		{
			Message(BLACK, "EQC Very Priviliged User Commands:");
			Message(BLACK, "  #size [size] - Sets your targets size.");
			Message(BLACK, "  #findspell [spellname] - Searches the spell database for [spellname]");
			Message(BLACK, "  #castspell [id|name] - Casts a spell, use #findspell to determine the id number or enter the name directly.");
			Message(BLACK, "  #setskill [skill num (0-73)] [number (0-255)] - Sets the targeted player's skill to number.");
			Message(BLACK, "  #setallskill [0-255] - Sets all of the targeted player's skills to number.");
			Message(BLACK, "  #kill - Kills your selected target.");
			Message(BLACK, "  #flymode - Allows you to move freely on the z axis.");
			Message(BLACK, "  #race [0-255]  (0 for back to normal)");
			Message(BLACK, "  #makepet - Makes a custom pet, #makepet for details.");
			Message(BLACK, "  #scribespells [1-65] - Scribes all spells for your class to desired level.");
		}
		else if(strcasecmp(sep.arg[1], "gmquest") == 0 && admin >= 80) 
		{
			Message(BLACK, "EQC Quest Troupe Commands:");
			Message(BLACK, "  #npcloot [show/add/remove] [itemid/all] - Adjusts the items the targeted npc is carrying.");
			Message(BLACK, "  #npcstats - Shows the stats of the targetted NPC.");
			Message(BLACK, "  #zheader [zone name/none] - Change the sky/fog/etc. of the current zone.");
			Message(BLACK, "  #zsky [sky number] - Changes the sky of the current zone.");
			Message(BLACK, "  #zcolor [red] [green] [blue] - Changes the fog colour of the current zone.");
			Message(BLACK, "  #zstats - Shows the zone header data for the current zone.");
			Message(BLACK, "  #timeofday - Sets the date to Monday, Janurary 1st, 1 at the specified time.");
			Message(BLACK, "  #date - Sets the time to the specified date.");
			Message(BLACK, "  #weather <0/1/2> - Off/Rain/Snow.");
			Message(BLACK, "  #permaclass <classnum> - Changes your class.");
			Message(BLACK, "  #permarace <racenum> - Changes your race.");
			Message(BLACK, "  #permagender <0/1/2> - Changes your gender.");
			Message(BLACK, "  #emote - Sends an emotish message, type for syntax.");
			Message(BLACK, "  #invul [1/0] - Makes the targeted player invulnerable to attack");
			Message(BLACK, "  #hideme [0/1]	- Removes you from the spawn list.");
			Message(BLACK, "  #npccast [targetname] [spellid] - Makes the targeted NPC cast on [targetname].");
			Message(BLACK, "  #zclip [min clip] [max clip] - Changes the clipping plane of the current zone.");
		}
		else if(strcasecmp(sep.arg[1], "gm") == 0 && admin >= 100)
		{
			Message(BLACK, "EQC GM Commands:");
			Message(BLACK, "  #damage [id] - Inflicts damage upon target.");
			Message(BLACK, "  #setexp - Sets the target\'s experience.");
			Message(BLACK, "  #addexp - Adds exp to the target.");
			Message(BLACK, "  /broadcast [text] - Sends a broadcast message.");
			Message(BLACK, "  /pr [text] - Sends text over GMSAY.");
			Message(BLACK, "  /Surname - Sets a player's Surname.");
			Message(BLACK, "  #depop - Depops targeted NPC.");
			Message(BLACK, "  #depopzone - Depops the zone.");
			Message(BLACK, "  #repop [delay] - Repops the zone, optional delay in seconds.");
			Message(BLACK, "  #zuwcoords [number] - Changes the underworld coordinates of the current zone.");
			Message(BLACK, "  #zsafecoords [x] [y] [z] - Changes the safe coordinates of the current zone.");
			Message(BLACK, "  #ListNPCs - Lists all the NPCs currently spawned in the zone.");
			Message(BLACK, "  #ListNPCCorpses - Lists all NPC corpses in the zone.");
			Message(BLACK, "  #ListPlayerCorpses - Lists all player corpses in the zone.");
			Message(BLACK, "  #DeleteNPCCorpses - Deletes all NPC corpses in the zone.");
			Message(BLACK, "  #showbuffs - Shows the buffs on your current target.");
			Message(BLACK, "  #nukebuffs - Dispells your current target.");
			Message(BLACK, "  #freeze - Freezes your current target.");
			Message(BLACK, "  #unfreeze	- Unfreezes your current target.");
			Message(BLACK, "  #pkill - Makes current target pkill.");
			Message(BLACK, "  #unpkill - Makes current target nonpkill.");
			Message(BLACK, "  #gm [on|off] - Toggles your GM flag.");
		}
		else if(strcasecmp(sep.arg[1], "leadgm") == 0 && admin >= 150)
		{
			Message(BLACK, "EQC Lead-GM Commands:");
			Message(BLACK, "  #summon [charname] - Summons a player to you.");
			Message(BLACK, "  #sic [charname] - Make targetted npc attack [charname].");
			Message(BLACK, "  #kick [charname] - Kicks player off of the server.");
			Message(BLACK, "  #zoneshutdown [ZoneID | ZoneName] - Shuts down the zoneserver.");
			Message(BLACK, "  #zonebootup [ZoneID] [ZoneName] - Boots [ZoneName] on the zoneserver specified.");
			Message(BLACK, "  #deletecorpse - Deletes the targeted player\'s corpse.");
			Message(BLACK, "  #DeletePlayerCorpses - Deletes all player corpses in the zone.");
			Message(BLACK, "  #motd [New MoTD Message] - Changes the server's MOTD.");
			Message(BLACK, "  #lock/#unlock - Locks or unlocks the server.");
		}
		else if(strcasecmp(sep.arg[1], "serverop") == 0 && admin >= 200)
		{
			Message(BLACK, "EQC ServerOperator Commands:");
			Message(BLACK, "  #shutdown - Shuts down the server.");
			Message(BLACK, "  #worldshutdown - Shuts down the worldserver and all zones.");
			Message(BLACK, "  #flag [name] [status] - Flags an account with GM status.");
			Message(BLACK, "  #createacct [name] [password] [status] - Creates an EQC account.");
			Message(BLACK, "  #delacct [name] - Deletes an EQC account.");
			Message(BLACK, "  #zsave [file name] - Save the current zone header to ./cfg/<filename>.cfg");
			Message(BLACK, "  #dbspawn2 [spawngroup] [respawn] [variance]");
			Message(BLACK, "  #npcupdate - Update target npc's spawn point and heading to your current position and heading"); 
			Message(BLACK, "  #npcdelete - Deletes the targetted npc's spawn data");
		}
		else if(strcasecmp(sep.arg[1], "debug") == 0 && admin >= 201)
		{
			Message(BLACK, "EQC Debug Commands:");
			Message(BLACK, "  #version [name] - Shows information about the zone executable.");
			Message(BLACK, "  #serverinfo [type] - Shows information about the server, #serverinfo for details.");
		}
		//END: Split Help into its other mehtod to make it more tider
	}
		
	//This is used to test the spawning of objects/crafting stations.  (Sp0tteR)
	if (strcasecmp(sep.arg[0], "#sc") == 0 && admin >= 0)
	{
		cout << "\nSummoning crafting station.\n";

		Object* no = new Object(0,0,y_pos,x_pos,z_pos, this->heading,"IT70_ACTORDEF");
		
		for (int i = 0;i != 10;i++) {
			//if (co->itemsinbag[i] != 0xFFFF)
				no->SetBagItems(i,0);
		}
		
		entity_list.AddObject(no,true);

	}
	//--------COFRUBEN_DBG---------
	if (strcasecmp(sep.arg[0], "#pp") == 0 && admin >= 0){
	}
	if (strcasecmp(sep.arg[0], "#unpp") == 0 && admin >= 0){
	}
	//--------COFRUBEN_DBG---------
	
	//TODO: DO we need #spawn with so much custom stuff? coudl we reuse other code/classes instead ofhaving it duplicated here?- Dark-Prince 29/01/2008
	if (strcasecmp(sep.arg[0], "#spawn") == 0 && (admin >= 10)) { // Image's Spawn Code -- Rewrite by Scruffy
		//			cout << "Spawning:" << endl; 
		//Well it needs a name!!! 
		if(sep.arg[1][0] == 0) 
		{ 
			Message(BLACK, "Format: #spawn name race level material hp gender class priweapon secweapon merchantid - spawns a npc those parameters.");
			Message(BLACK, "Name Format: NPCFirstname_NPCSurname - All numbers in a name are stripped and \"_\" characters become a space.");
			Message(BLACK, "Note: Using \"-\" for gender will autoselect the gender for the race. Using \"-\" for HP will use the calculated maximum HP.");
		} 
		else 
		{ 
			//Lets see if someone didn't fill out the whole #spawn function properly 
			sep.arg[1][29] = 0;
			if (!sep.IsNumber(2))
				sprintf(sep.arg[2],"1"); 
			if (!sep.IsNumber(3))
				sprintf(sep.arg[3],"1"); 
			if (!sep.IsNumber(4))
				sprintf(sep.arg[4],"0");
			if (atoi(sep.arg[5]) > 2100000000 || atoi(sep.arg[5]) <= 0)
				sprintf(sep.arg[5],"");
			if (!strcmp(sep.arg[6],"-"))
				sprintf(sep.arg[6],""); 
			if (!sep.IsNumber(6))
				sprintf(sep.arg[6],""); 
			if (!sep.IsNumber(7))
				sprintf(sep.arg[7],"1");
			if (!sep.IsNumber(9))
				sprintf(sep.arg[9],"0");
			if (!sep.IsNumber(10))
				sprintf(sep.arg[10], "0");
			if (!sep.IsNumber(8))
				sprintf(sep.arg[8],"0");
			if (!strcmp(sep.arg[5],"-"))
				sprintf(sep.arg[5],""); 

			//Calc MaxHP if client neglected to enter it...
			if (!sep.IsNumber(5)) {
				//Stolen from Client::GetMaxHP...
				int8 multiplier = 0;
				int8 tmplevel = GetLevel();

				switch(atoi(sep.arg[6]))
				{
				case WARRIOR:
					if (atoi(sep.arg[3]) < 20)
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
					if (atoi(sep.arg[3]) < 35)
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
					if (atoi(sep.arg[3]) < 51)
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
					if (atoi(sep.arg[3]) < 58)
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

				case BEASTLORD:
				default:
					if (atoi(sep.arg[3]) < 35)
						multiplier = 21;
					else if (atoi(sep.arg[3]) < 45)
						multiplier = 22;
					else if (atoi(sep.arg[3]) < 51)
						multiplier = 23;
					else if (atoi(sep.arg[3]) < 56)
						multiplier = 24;
					else if (atoi(sep.arg[3]) < 60)
						multiplier = 25;
					else
						multiplier = 26;
					break;
				}
				sprintf(sep.arg[5],"%i",5+multiplier*atoi(sep.arg[3])+multiplier*atoi(sep.arg[3])*75/300);
			}

			// Autoselect NPC Gender... (Scruffy)
			if (sep.arg[6][0] == 0) {
				sprintf(sep.arg[6], "%i", (int) Mob::GetDefaultGender(atoi(sep.arg[2])));
			}

			// Well we want everyone to know what they spawned, right? 
			Message(BLACK, "New spawn:"); 
			Message(BLACK, "Name: %s",sep.arg[1]); 
			Message(BLACK, "Race: %s",sep.arg[2]);  
			Message(BLACK, "Level: %s",sep.arg[3]);
			Message(BLACK, "Material: %s",sep.arg[4]);
			Message(BLACK, "Current/Max HP: %s",sep.arg[5]);
			Message(BLACK, "Gender: %s",sep.arg[6]); 
			Message(BLACK, "Class: %s",sep.arg[7]);
			Message(BLACK, "Weapon Item Number: %s",sep.arg[8]); 
			Message(BLACK, "MerchantID: %s",sep.arg[9]); 
			//Time to create the NPC!! 
			NPCType* npc_type = new NPCType;
			memset(npc_type, 0, sizeof(NPCType));
			strcpy(npc_type->name,sep.arg[1]);
			npc_type->cur_hp = atoi(sep.arg[5]); 
			npc_type->max_hp = atoi(sep.arg[5]); 
			npc_type->race = atoi(sep.arg[2]); 
			npc_type->gender = atoi(sep.arg[6]); 
			npc_type->class_ = atoi(sep.arg[7]); 
			npc_type->deity= 1; 
			npc_type->level = atoi(sep.arg[3]); 
			npc_type->npc_id = 0;
			npc_type->loottable_id = 0;
			npc_type->texture = atoi(sep.arg[4]);
			npc_type->light = 0; 
			npc_type->runspeed = 1.25;
			// Weapons are broke!!
			//npc_type->equipment[13] = atoi(sep.arg[8]);
			//npc_type->equipment[14] = atoi(sep.arg[9]);
			npc_type->merchanttype = atoi(sep.arg[10]);	
			npc_type->npc_id = 0;
			//for (int i=0; i<9; i++) 
			//	npc_type->equipment[i] = atoi(sep.arg[8]); 

			npc_type->STR = 75;
			npc_type->STA = 75;
			npc_type->DEX = 75;
			npc_type->AGI = 75;
			npc_type->INT = 75;
			npc_type->WIS = 75;
			npc_type->CHA = 75;

			NPC* npc = new NPC(npc_type, 0, GetX(), GetY(), GetZ(), heading);

			entity_list.AddNPC(npc);
		}
	}
	if (strcasecmp(sep.arg[0], "#createacct") == 0 && admin >= 200) //TODO: Still needed?
	{
		if(sep.arg[3][0] == 0) {
			Message(BLACK, "Format: #createacct accountname accountpassword accountstatus(no spaces) (Account Status: 0 = Normal, 10=PU, 20=PU2, 100=GM, 150=LGM, 200=ServerOP)");
		} else {
			if (Database::Instance()->CreateAccount(sep.arg[1],sep.arg[2], atoi(sep.arg[3]),0) == 0)
				Message(BLACK, "Unable to create account.");
			else
				Message(BLACK, "The account was created.");
		}
	}
	if (strcasecmp(sep.arg[0], "#delacct") == 0 && admin >= 200) // TODO: Still needed?
	{
		if(sep.arg[1][0] == 0) {
			Message(BLACK, "Format: #delacct accountname");
		} else {
			if (Database::Instance()->DeleteAccount(name) == 0)
				Message(BLACK, "Unable to delete account.");
			else
				Message(BLACK, "The account was deleted."); 
		}
	}	
	if (strcasecmp(sep.arg[0], "#size") == 0 && (admin >= EQC_Alpha_Tester))	{
		if (!sep.IsNumber(1)) //TODO: <-- Ilike this, lets impliment it in other places!
			Message(BLACK, "Usage: #size [0 - 255]");
		else {
			float newsize = atof(sep.arg[1]);
			if (newsize > 255)
				Message(BLACK, "Error: #size: Size can not be greater than 255.");
			else if (newsize < 0)
				Message(BLACK, "Error: #size: Size can not be less than 0.");
			else {
				if (target)
					target->ChangeSize(newsize);
				else
					this->ChangeSize(newsize);
			}
		}
	}
	// TODO: #goto, #summon, #whoopy - All do the same thing, cant we reuse the code? - Dark-Prince 29/01/2008	
	if (strcasecmp(sep.arg[0], "#worldshutdown") == 0 && admin >= 200) {
		// GM command to shutdown world server and all zone servers
		if (worldserver.Connected()) {
			worldserver.SendEmoteMessage(0,0,15,"<SYSTEMWIDE MESSAGE>:SYSTEM MSG:World coming down, everyone log out now.");
			Message(BLACK, "Sending shutdown packet");
			//TODO: use ServerPacker* pack - new ServerPacker(strcut)
			ServerPacket* pack = new ServerPacket;
			pack->opcode = ServerOP_ShutdownAll;
			pack->size=0;
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
		else
			Message(BLACK, "Error: World server disconnected");
	}
	if (strcasecmp(sep.arg[0], "#chat") == 0 && admin >= 200) {
		// sends any channel message to all zoneservers
		// Dev debug command, for learing channums
		if (sep.arg[2][0] == 0)
			Message(BLACK, "Usage: #chat channum message");
		else {
			if (!worldserver.SendChannelMessage(0, 0, (uint8) atoi(sep.arg[1]), 0, 0, sep.argplus[2]))
				Message(BLACK, "Error: World server disconnected");
		}
	}
	if (strcasecmp(sep.arg[0], "#appearance") == 0 && admin >= 150) {
		// sends any appearance packet
		// Dev debug command, for appearance types
		if (sep.arg[2][0] == 0)
			Message(BLACK, "Usage: #appearance type value");
		else {
			//TODO: Clean this shit up! why are we callign the same method twice because of if(target != 0) ??? couldnt we just set variables and onyl call it once per check?
			TSpawnAppearanceType type = (TSpawnAppearanceType)atoi(sep.arg[1]);
			if (target != 0)
			{
				target->SendAppearancePacket(target->GetID(), type, atoi(sep.arg[2]), true);
				Message(BLACK, "Sending appearance packet: target=%s, type=%s, value=%s", target->GetName(), sep.arg[1], sep.arg[2]);
			}
			else {
				this->SendAppearancePacket(0, type, atoi(sep.arg[2]), true);
				Message(BLACK, "Sending appearance packet: target=self, type=%s, value=%s", sep.arg[1], sep.arg[2]);
			}
		}
	}
	if (strcasecmp(sep.arg[0], "#zoneshutdown") == 0 && admin >= 150) {
		if (!worldserver.Connected())
			Message(BLACK, "Error: World server disconnected");
		else if (sep.arg[1][0] == 0) {
			Message(BLACK, "Usage: #zoneshutdown zoneshortname");
		} else {
			ServerPacket* pack = new ServerPacket(ServerOP_ZoneShutdown, sizeof(ServerZoneStateChange_struct));

			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, sizeof(ServerZoneStateChange_struct));
			ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
			
			strcpy(s->adminname, this->GetName());
			if (sep.arg[1][0] >= '0' && sep.arg[1][0] <= '9')
				s->ZoneServerID = atoi(sep.arg[1]);
			else
				strcpy(s->zonename, sep.arg[1]);
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
	if (strcasecmp(sep.arg[0], "#cdtimer") == 0 && admin >= EQC_Alpha_Tester) {
		if (sep.arg[1][0] == 0) {
			Message(BLACK, "Spell CD Timer is %i", spellCooldown_timer);
		}
		else {
			Message(BLACK, "resetting timer...", atoi(sep.arg[1]));
			spellCooldown_timer = new Timer(1);
			spellCooldown_timer->Start(0);
			spellCooldown_timer->Disable();
			EnableSpellBar(1);
			for (int i = 0; i < 8; i++){
				pp.spell_memory[i] = pp.spell_memory[i];
				SetSpellRecastTimer(i, 0);
				//pp.spell_memory[i] = 0xffff;
			}
			ResetSpellGems();
		}
	}
	if (strcasecmp(sep.arg[0], "#zonebootup") == 0 && admin >= 150) {
		if (!worldserver.Connected())
			Message(BLACK, "Error: World server disconnected");
		else if (sep.arg[2][0] == 0) {
			Message(BLACK, "Usage: #zonebootup ZoneServerID# zoneshortname");
		} else {
			ServerPacket* pack = new ServerPacket(ServerOP_ZoneBootup, sizeof(ServerZoneStateChange_struct));
			
			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, sizeof(ServerZoneStateChange_struct));
			ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
			
			s->ZoneServerID = atoi(sep.arg[1]);
			strcpy(s->adminname, this->GetName());
			strcpy(s->zonename, sep.arg[2]);
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
	if (strcasecmp(sep.arg[0], "#zonestatus") == 0 && admin >= EQC_Alpha_Tester) {
		if (!worldserver.Connected())
			Message(BLACK, "Error: World server disconnected");
		else {
			ServerPacket* pack = new ServerPacket;
			pack->size = strlen(this->GetName())+2;
			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, pack->size);
			pack->opcode = ServerOP_ZoneStatus;
			memset(pack->pBuffer, (int8) admin, 1);
			strcpy((char *) &pack->pBuffer[1], this->GetName());
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
	if (strcasecmp(sep.arg[0], "#emote") == 0 && (admin >= 80)) {
		if (sep.arg[3][0] == 0)
			Message(BLACK, "Usage: #emote [name | world | zone] type# message");
		else {
			if (strcasecmp(sep.arg[1], "zone") == 0)
				entity_list.Message(0, (MessageFormat)atoi(sep.arg[2]), sep.argplus[3]);
			else if (!worldserver.Connected())
				Message(BLACK, "Error: World server disconnected");
			else if (strcasecmp(sep.arg[1], "world") == 0)
				worldserver.SendEmoteMessage(0, 0, atoi(sep.arg[2]), sep.argplus[3]);
			else
				worldserver.SendEmoteMessage(sep.arg[1], 0, atoi(sep.arg[2]), sep.argplus[3]);
		}
	}
	
	if (strcasecmp(sep.arg[0], "#summon") == 0 && (admin >= 100)) {
		//TODO: resuse #goto, #summon, #comehere! - Dark-Prince - 29/01/2008
		if (sep.arg[1][0] == 0) {
			if (target != 0 && target->IsNPC()) {
				target->CastToNPC()->GMMove(x_pos, y_pos, z_pos, heading);
			}
			else if (admin >= 150)
				Message(BLACK, "Usage: #summon [charname]");
			else
				Message(BLACK, "You need a NPC target for this command");
		}
		else if (admin >= 150) {
			Client* client = entity_list.GetClientByName(sep.arg[1]);
			if (client != 0) {
				Message(BLACK, "Summoning %s to %1.1f, %1.1f, %1.1f", sep.arg[1], this->x_pos, this->y_pos, this->z_pos);
				client->MovePC(0, this->x_pos, this->y_pos, this->z_pos, false, false);
			}
			else if (!worldserver.Connected())
				Message(BLACK, "Error: World server disconnected");
			else {
				ServerPacket* pack = new ServerPacket(ServerOP_ZonePlayer, sizeof(ServerZonePlayer_Struct));
				pack->pBuffer = new uchar[pack->size];
				ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
				strcpy(szp->adminname, this->GetName());
				szp->adminrank = this->Admin();
				szp->ignorerestrictions = true;
				strcpy(szp->name, sep.arg[1]);
				strcpy(szp->zone, zone->GetShortName());
				szp->x_pos = x_pos;
				szp->y_pos = y_pos;
				szp->z_pos = z_pos;
				worldserver.SendPacket(pack);
				safe_delete(pack);//delete pack;
			}
		}
	}
	if (strcasecmp(sep.arg[0], "#kick") == 0 && (admin >= 150)) {
		if (sep.arg[1][0] == 0)
			Message(BLACK, "Usage: #kick [charname]");
		else {
			Client* client = entity_list.GetClientByName(sep.arg[1]);
			if (client != 0) {
				if (client->Admin() <= this->Admin()) {
					client->Message(BLACK, "You have been kicked by %s",this->GetName());
					client->Kick();
					this->Message(BLACK, "Kick: local: kicking %s", sep.arg[1]);
				}
			}
			else if (!worldserver.Connected())
				Message(BLACK, "Error: World server disconnected");
			else {
				ServerPacket* pack = new ServerPacket(ServerOP_KickPlayer, sizeof(ServerKickPlayer_Struct));
				
				pack->pBuffer = new uchar[pack->size];
				ServerKickPlayer_Struct* skp = (ServerKickPlayer_Struct*) pack->pBuffer;
				strcpy(skp->adminname, this->GetName());
				strcpy(skp->name, sep.arg[1]);




				skp->adminrank = this->Admin();
				worldserver.SendPacket(pack);
				safe_delete(pack);//delete pack;
			}
		}
	}
	if (strcasecmp(sep.arg[0], "#guild") == 0 || strcasecmp(sep.arg[0], "#guilds") == 0) {
		GuildCommand(&sep);
	}
	if (strcasecmp(sep.arg[0], "#flag") == 0)
	{
		if(sep.arg[2][0] == 0 || !(admin >= 200)) {
			this->UpdateAdmin();
			Message(BLACK, "Refreshed your admin flag from DB.");
		}
		else if (!sep.IsNumber(2) || atoi(sep.arg[2]) < 0 || atoi(sep.arg[2]) > 255)
			Message(BLACK, "Usage: #flag [accname] [status] (Account Status: 0 = Normal, 1 = GM, 2 = Lead GM)");
		else {
			if (atoi(sep.arg[2]) > admin)
				Message(BLACK, "You cannot set people's status to higher than your own");
			else if (!Database::Instance()->SetGMFlag(sep.arg[1], atoi(sep.arg[2])))
				Message(BLACK, "Unable to set GM Flag.");
			else {
				Message(BLACK, "Set GM Flag on account.");
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_FlagUpdate;
				pack->size = strlen(sep.arg[1]) + 1;
				pack->pBuffer = new uchar[pack->size];
				memset(pack->pBuffer, 0, pack->size);
				strcpy((char*) pack->pBuffer, sep.arg[1]);
				worldserver.SendPacket(pack);
				safe_delete(pack);//delete pack;
			}
		}
	}	
	if (strcasecmp(sep.arg[0], "#npcloot") == 0 && admin >= 80) {
		if (target == 0)
			Message(BLACK, "Error: No target");
		// #npcloot show
		if (strcasecmp(sep.arg[1], "show") == 0) {
			if (target->IsNPC())
				target->CastToNPC()->QueryLoot(this);
			else if (target->IsCorpse())
				target->CastToCorpse()->QueryLoot(this);
			else
				Message(BLACK, "Error: Target's type doesnt have loot");
		}
		// #npcloot add
		else if (strcasecmp(sep.arg[1], "add") == 0) {

			if (target->IsNPC() && sep.IsNumber(2)) {
				int32 item = atoi(sep.arg[2]);
				if (Database::Instance()->GetItem(item)) {
					target->CastToNPC()->AddItem(item, 0, 0);
					Message(BLACK, "Added item(%i) to the %s's loot.", item, target->GetName());
				}
				else
					Message(BLACK, "Error: #item add: Item(%i) does not exist!", item);
			}
			else if (!sep.IsNumber(2))
				Message(BLACK, "Error: #npcloot add: Itemid must be a number.");
			else
				Message(BLACK, "Error: #npcloot add: This is not a valid target.");
		}
		// #npcloot remove
		else if (strcasecmp(sep.arg[1], "remove") == 0) {
			//#npcloot remove all
			if (strcasecmp(sep.arg[2], "all") == 0)
				Message(BLACK, "Error: #npcloot remove all: Not yet implemented.");
			//#npcloot remove itemid
			else {
				if(target->IsNPC() && sep.IsNumber(2)) {
					int32 item = atoi(sep.arg[2]);
					target->CastToNPC()->RemoveItem(item);
					Message(BLACK, "Removed item(%i) from the %s's loot.", item, target->GetName());
				}
				else if (!sep.IsNumber(2))					
					Message(BLACK, "Error: #npcloot remove: Item must be a number.");
				else
					Message(BLACK, "Error: #npcloot remove: This is not a valid target.");
			}
		}
		else
			Message(BLACK, "Usage: #npcloot [show/add/remove] [itemid/all]");
	}

	if (strcasecmp(sep.arg[0], "#cofru") == 0) {
		APPLAYER* app = new APPLAYER;
		app->opcode = OP_HPUpdate;
		app->size = sizeof(SpawnHPUpdate_Struct);
		app->pBuffer = new uchar[app->size];
		memset(app->pBuffer, 0, sizeof(SpawnHPUpdate_Struct));
		SpawnHPUpdate_Struct* ds = (SpawnHPUpdate_Struct*)app->pBuffer;
		ds->spawn_id = GetID();
		ds->cur_hp = 2;
		ds->max_hp = 20;

		if (GMHideMe())
		{
			entity_list.QueueClientsStatus(this, app, false, admin);
		}
		else
		{
			entity_list.QueueCloseClients(this, app);
		}

		safe_delete(app);//delete app;
	}
	
	if (strcasecmp(sep.arg[0], "#iteminfo") == 0) {
		Item_Struct* item = Database::Instance()->GetItem(pp.inventory[0]);
		if (item == 0)
			Message(BLACK, "Error: You need an item on your cursor for this command");
		else {
			Message(BLACK, "ID: %i Name: %s Slots: %i", item->item_nr, item->name, item->equipableSlots);
			Message(BLACK, "  Lore: %s  ND: %i  NS: %i  Type: %i", item->lore, item->nodrop, item->norent, item->type);
			Message(BLACK, "  IDF: %s  Size: %i  Weight: %i  Flag: %04x  icon_nr: %i  Cost: %i", item->idfile, item->size, item->weight, (int16) item->flag, (int16) item->icon_nr, item->cost);
			if (item->type == 0x02)
				Message(BLACK, "  This item is a Book: %s", item->book.file);
			else {
				if (item->type == 0x01)
					Message(BLACK, "  This item is a container with %i slots", item->common.container.BagSlots);
//				Message(BLACK, "  Magic: %i  SpellID0: %i  Level0: %i  Charges: %i", item->common.magic, item->common.spellId0, item->common.level0, item->common.charges);
//				Message(BLACK, "  SpellId: %i  Level: %i  EffectType: 0x%02x  CastTime: %.2f", item->common.spellId, item->common.level, (int8) item->common.effecttype, (double) item->common.casttime/1000);
//				Message(BLACK, "  Material: 0x02%x  Color: 0x%08x", item->common.material, item->common.color);
//				Message(BLACK, "  U0187: 0x%02x%02x  U0192: 0x%02x  U0198: 0x%02x%02x  U0204: 0x%02x%02x  U0210: 0x%02x%02x  U0214: 0x%02x%02x%02x", item->common.unknown0187[0], item->common.unknown0187[1], item->common.unknown0192, item->common.unknown0198[0], item->common.unknown0198[1], item->common.unknown0204[0], item->common.unknown0204[1], item->common.unknown0210[0], item->common.unknown0210[1], (int8) item->common.normal.unknown0214[0], (int8) item->common.normal.unknown0214[1], (int8) item->common.normal.unknown0214[2]);
//				Message(BLACK, "  U0222[0-9]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0222[0], (int8) item->common.unknown0222[1], (int8) item->common.unknown0222[2], (int8) item->common.unknown0222[3], (int8) item->common.unknown0222[4], (int8) item->common.unknown0222[5], (int8) item->common.unknown0222[6], (int8) item->common.unknown0222[7], (int8) item->common.unknown0222[8], (int8) item->common.unknown0222[9]);
//				Message(BLACK, "  U0236[ 0-15]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0236[0], (int8) item->common.unknown0236[1], (int8) item->common.unknown0236[2], (int8) item->common.unknown0236[3], (int8) item->common.unknown0236[4], (int8) item->common.unknown0236[5], (int8) item->common.unknown0236[6], (int8) item->common.unknown0236[7], (int8) item->common.unknown0236[8], (int8) item->common.unknown0236[9], (int8) item->common.unknown0236[10], (int8) item->common.unknown0236[11], (int8) item->common.unknown0236[12], (int8) item->common.unknown0236[13], (int8) item->common.unknown0236[14], (int8) item->common.unknown0236[15]);
//				Message(BLACK, "  U0236[16-31]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0236[16], (int8) item->common.unknown0236[17], (int8) item->common.unknown0236[18], (int8) item->common.unknown0236[19], (int8) item->common.unknown0236[20], (int8) item->common.unknown0236[21], (int8) item->common.unknown0236[22], (int8) item->common.unknown0236[23], (int8) item->common.unknown0236[24], (int8) item->common.unknown0236[25], (int8) item->common.unknown0236[26], (int8) item->common.unknown0236[27], (int8) item->common.unknown0236[28], (int8) item->common.unknown0236[29], (int8) item->common.unknown0236[30], (int8) item->common.unknown0236[31]);
//				Message(BLACK, "  U0236[32-47]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0236[32], (int8) item->common.unknown0236[33], (int8) item->common.unknown0236[34], (int8) item->common.unknown0236[35], (int8) item->common.unknown0236[36], (int8) item->common.unknown0236[37], (int8) item->common.unknown0236[38], (int8) item->common.unknown0236[39], (int8) item->common.unknown0236[40], (int8) item->common.unknown0236[41], (int8) item->common.unknown0236[42], (int8) item->common.unknown0236[43], (int8) item->common.unknown0236[44], (int8) item->common.unknown0236[45], (int8) item->common.unknown0236[46], (int8) item->common.unknown0236[47]);
//				Message(BLACK, "  U0236[48-55]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->common.unknown0236[48], (int8) item->common.unknown0236[49], (int8) item->common.unknown0236[50], (int8) item->common.unknown0236[51], (int8) item->common.unknown0236[52], (int8) item->common.unknown0236[53], (int8) item->common.unknown0236[54], (int8) item->common.unknown0236[55]);
			}

			//TODO: WTF IS THIS SHIT? - Dark-Prince 29/01/2008
			Message(BLACK, "  U0103[ 0-11]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->unknown0103[0], (int8) item->unknown0103[1], (int8) item->unknown0103[2], (int8) item->unknown0103[3], (int8) item->unknown0103[4], (int8) item->unknown0103[5], (int8) item->unknown0103[6], (int8) item->unknown0103[7], (int8) item->unknown0103[8], (int8) item->unknown0103[9], (int8) item->unknown0103[10], (int8) item->unknown0103[11]);
			Message(BLACK, "  U0103[12-21]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->unknown0103[12], (int8) item->unknown0103[13], (int8) item->unknown0103[14], (int8) item->unknown0103[15], (int8) item->unknown0103[16], (int8) item->unknown0103[17], (int8) item->unknown0103[18], (int8) item->unknown0103[19], (int8) item->unknown0103[20], (int8) item->unknown0103[21]);
			Message(BLACK, "  U0144[ 0-11]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->unknown0144[0], (int8) item->unknown0144[1], (int8) item->unknown0144[2], (int8) item->unknown0144[3], (int8) item->unknown0144[4], (int8) item->unknown0144[5], (int8) item->unknown0144[6], (int8) item->unknown0144[7], (int8) item->unknown0144[8], (int8) item->unknown0144[9], (int8) item->unknown0144[10], (int8) item->unknown0144[11]);
			Message(BLACK, "  U0144[12-23]: 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x 0x%02x%02x", (int8) item->unknown0144[12], (int8) item->unknown0144[13], (int8) item->unknown0144[14], (int8) item->unknown0144[15], (int8) item->unknown0144[16], (int8) item->unknown0144[17], (int8) item->unknown0144[18], (int8) item->unknown0144[19], (int8) item->unknown0144[20], (int8) item->unknown0144[21], (int8) item->unknown0144[22], (int8) item->unknown0144[23]);
			Message(BLACK, "  U0144[24-27]: 0x%02x%02x 0x%02x%02x", (int8) item->unknown0144[24], (int8) item->unknown0144[25], (int8) item->unknown0144[26], (int8) item->unknown0144[27]);
		}
	}
	if (strcasecmp(sep.arg[0], "#Depop") == 0 && admin >= EQC_Alpha_Tester) {
		if (target == 0 || !(target->IsNPC() || target->IsNPCCorpse()))
			Message(BLACK, "You must have a NPC target for this command. (maybe you meant #depopzone?)");
		else {
			Message(BLACK, "Depoping '%s'.", target->GetName());
			target->Depop();
		}
	}
	//else if (strcasecmp(sep.arg[0], "#DepopCorpses") == 0 && admin >= 100) { //TODO: Maybe deletecorpse instad? sounds better?
	//	entity_list.DepopCorpses();
	//	Message(BLACK, "NPC corpses depoped.");
	//}
	
	if (strcasecmp(sep.arg[0], "#spawnstatus") == 0 && admin >= 100) {
		zone->SpawnStatus(this);
	}
	if (strcasecmp(sep.arg[0], "#listNPCs") == 0 && admin >= 100) {
		entity_list.ListNPCs(this);
	}
	if (strcasecmp(sep.arg[0], "#listNPCCorpses") == 0 && admin >= 100) {
		entity_list.ListNPCCorpses(this);
	}
	if (strcasecmp(sep.arg[0], "#listPlayerCorpses") == 0 && admin >= 100) {
		entity_list.ListPlayerCorpses(this);
	}
	if (strcasecmp(sep.arg[0], "#deleteNPCCorpses") == 0 && admin >= 100) {
		sint32 tmp = entity_list.DeleteNPCCorpses();
		if (tmp >= 0)
			Message(BLACK, "%i corpses deleted.", tmp);
		else
			Message(BLACK, "DeletePlayerCorpses Error #%i", tmp);
	}
	if (strcasecmp(sep.arg[0], "#deletePlayerCorpses") == 0 && admin >= 150) {
		sint32 tmp = entity_list.DeletePlayerCorpses();
		if (tmp >= 0)
			Message(BLACK, "%i corpses deleted.", tmp);
		else
			Message(BLACK, "DeletePlayerCorpses Error #%i", tmp);
	}
	if (strcasecmp(sep.arg[0], "#doanim") == 0) {
		if (!sep.IsNumber(1)) {
			Message(BLACK, "Usage: #DoAnim [number]");
		}
		else {
			if (admin >= 100) {
				if (target == 0)
					Message(BLACK, "Error: You need a target.");
				else
					target->DoAnim(atoi(sep.arg[1]));
			}
			else
				DoAnim(atoi(sep.arg[1]));
		}
	}
	if (strcasecmp(sep.arg[0], "#showbuffs") == 0 && admin >= EQC_Alpha_Tester) {
		if (target == 0)
			this->ShowBuffs(this);
		else
			target->ShowBuffs(this);
	}
	if (strcasecmp(sep.arg[0], "#nukebuffs") == 0 && admin >= EQC_Alpha_Tester) {
		if (target == 0)
			this->BuffFadeAll();
		else
			target->BuffFadeAll();
	}
	// Wizzel - To add class training points for testing.
	if (strcasecmp(sep.arg[0], "#points") == 0 && admin >= EQC_Alpha_Tester)
	{
		if (target==0) 
		{
			Message(BLACK, "Error: #points: No Target.");
		}
		else 
		{

			this->GetPlayerProfilePtr()->trainingpoints += 5;
			this->Save();
			// Harakiri update client side points with this, no need to relog =)
			this->SendLevelUpdate( this->GetLevel());
			Message(BLACK, "Five points have been added to your class training points.");
		}
	}
	if (strcasecmp(sep.arg[0], "#hideme") == 0 && admin >= 80) {
		if ((sep.arg[1][0] != '0' && sep.arg[1][0] != '1') || sep.arg[1][1] != 0)
			Message(BLACK, "Usage: #hideme [0/1]");
		else {
			gmhideme = atoi(sep.arg[1]);
			if (gmhideme) {
				APPLAYER app;
				CreateDespawnPacket(&app);
				entity_list.QueueClientsStatus(this, &app, true, 0, admin-1);
				entity_list.RemoveFromTargets(this);
				Message(RED, "Removing you from spawn lists.");
			}
			else {
				APPLAYER app;
				CreateSpawnPacket(&app);
				entity_list.QueueClientsStatus(this, &app, true, 0, admin-1);
				Message(RED, "Adding you back to spawn lists.");
			}
		}
	}
	if (strcasecmp(sep.arg[0], "#deletecorpse") == 0 && admin >= 150) {
		if (target == 0 || !target->IsPlayerCorpse())
			Message(BLACK, "Error: Target the player corpse you wish to delete");
		else {
			Message(BLACK, "Deleting %s.", target->GetName());
			target->CastToCorpse()->Delete();
		}
	}
	if (strcasecmp(sep.arg[0], "#sendzonespawns") == 0 && admin >= 200) {
		entity_list.SendZoneSpawnsBulk(this);
	}
	
	if (strcasecmp(sep.arg[0], "#race") == 0 && admin >= EQC_Alpha_Tester)
	{
		if (sep.IsNumber(1) && atoi(sep.arg[1]) >= 0 && atoi(sep.arg[1]) <= 255)
		{
			if ((target) && admin >= 100)
			{
				target->Message(BLACK, "You feel yourself shimmer into a different race.");

				// Wizzel - Grabs a default gender for the race to prevent
				// you from getting the default human model in a zone that
				// supports the chosen race.

				int16 tmpgender = Mob::GetDefaultGender((atoi(sep.arg[1])), gender);
				target->SendIllusionPacket(atoi(sep.arg[1]), tmpgender);
			}
			else
			{
				this->SendIllusionPacket(atoi(sep.arg[1]));
			}
		}
		else
		{
			Message(BLACK, "Usage: #race [0-255]  (0 for back to normal)");
		}
	}
	if (strcasecmp(sep.arg[0], "#texture") == 0 && admin >= 10) {
		if (sep.IsNumber(1) && atoi(sep.arg[1]) >= 0 && atoi(sep.arg[1]) <= 255) {
			int tmp;
			if (sep.IsNumber(2) && atoi(sep.arg[2]) >= 0 && atoi(sep.arg[2]) <= 255) {
				tmp = atoi(sep.arg[2]);
			}
			else if (atoi(sep.arg[1]) == 255) {
				tmp = atoi(sep.arg[1]);
			}
			else if ((GetRace() > 0 && GetRace() <= 12) || GetRace() == 128 || GetRace() == 130)
				tmp = 0;
			else
				tmp = atoi(sep.arg[1]);

			int16 tmpgender = Mob::GetDefaultGender(target->GetRace(), gender);

			if ((target) && admin >= 100)
				target->SendIllusionPacket(target->GetRace(), tmpgender, atoi(sep.arg[1]), tmp);
			else
				this->SendIllusionPacket(this->GetRace(), tmpgender, atoi(sep.arg[1]), tmp);
		}
		else
			Message(BLACK, "Usage: #texture [texture] [helmtexture]  (0-255, 255 for show equipment)");
	}
	if (strcasecmp(sep.arg[0], "#gender") == 0 && admin >= EQC_Alpha_Tester) {
		if (sep.IsNumber(1) && atoi(sep.arg[1]) >= 0 && atoi(sep.arg[1]) <= 2) {
			gender = atoi(sep.arg[1]);
			if ((target) && admin >= 100)
				target->SendIllusionPacket(target->GetRace(), gender);
			else
				this->SendIllusionPacket(this->GetRace(), gender);
		}
		else
			Message(BLACK, "Usage: #gender [0-2]  (0=male, 1=female, 2=neuter)");
	}
	if (strcasecmp(sep.arg[0], "#zheader")==0 && admin >= 80) {
		// sends zhdr packet
		if(sep.arg[1][0]==0) {
			Message(BLACK, "Usage: #zheader <zone name>");
			Message(BLACK, "NOTE: Use \"none\" for zone name to use default header.");
		}
		else {
			if (strcasecmp(sep.argplus[1], "none") == 0) {
				Message(BLACK, "Loading default zone header");
				zone->LoadZoneCFG(0);
			}
			else {
				/*if (zone->LoadZoneCFG(sep.argplus[1], true))
					Message(BLACK, "Successfully loaded zone header: %s.cfg", sep.argplus[1]);
				else
					Message(BLACK, "Failed to load zone header: %s.cfg", sep.argplus[1]);*/
			}
			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			safe_delete(outapp);
			
			this->SendNewZone(zone->newzone_data);
		}
	}
	if(strcasecmp(sep.arg[0], "#zsky") == 0 && admin >= 80) {
		// modifys and resends zhdr packet
		if(sep.arg[1][0]==0)
			Message(BLACK, "Usage: #zsky <sky type>");
		else if(atoi(sep.arg[1])<0||atoi(sep.arg[1])>255)
			Message(BLACK, "ERROR: Sky type can not be less than 0 or greater than 255!");
		else {
			zone->newzone_data.sky = atoi(sep.arg[1]);

			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			safe_delete(outapp);//delete outapp;
		}
	}
	if((strcasecmp(sep.arg[0], "#zcolor") == 0 || strcasecmp(sep.arg[0], "#zcolour") == 0) && admin >= 80) {
		// modifys and resends zhdr packet
		if(sep.arg[3][0]==0)
			Message(BLACK, "Usage: #zcolor <red> <green> <blue>");
		else if(atoi(sep.arg[1])<0||atoi(sep.arg[1])>255)
			Message(BLACK, "ERROR: Red can not be less than 0 or greater than 255!");
		else if(atoi(sep.arg[2])<0||atoi(sep.arg[2])>255)
			Message(BLACK, "ERROR: Green can not be less than 0 or greater than 255!");
		else if(atoi(sep.arg[3])<0||atoi(sep.arg[3])>255)
			Message(BLACK, "ERROR: Blue can not be less than 0 or greater than 255!");
		else {
			for(int z=1;z<13;z++) {
				/*if(z<5)
					zone->newzone_data.unknown230[z] = atoi(sep.arg[1]);
				if(z>4 && z<9)
					zone->newzone_data.unknown230[z] = atoi(sep.arg[2]);
				if(z>8)
					zone->newzone_data.unknown230[z] = atoi(sep.arg[3]);*/
			}

			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			safe_delete(outapp);//delete outapp;
		}
	}
	if(strcasecmp(sep.arg[0], "#zuwcoords") == 0 && admin >= 100) {
		// modifys and resends zhdr packet
		if(sep.arg[1][0]==0)
			Message(BLACK, "Usage: #zuwcoords <under world coords>");
		else {
			zone->newzone_data.underworld = atof(sep.arg[1]);
			//				float newdata = atof(sep.arg[1]);
			//				memcpy(&zone->zone_header_data[130], &newdata, sizeof(float));

			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			delete outapp;
		}
	}
	if(strcasecmp(sep.arg[0], "#zsafecoords") == 0 && admin >= 100) {
		// modifys and resends zhdr packet
		if(sep.arg[3][0]==0)
			Message(BLACK, "Usage: #zsafecoords <safe x> <safe y> <safe z>");
		else {
			zone->newzone_data.safe_x = atof(sep.arg[1]);
			zone->newzone_data.safe_y = atof(sep.arg[2]);
			zone->newzone_data.safe_z = atof(sep.arg[3]);
			//				float newdatax = atof(sep.arg[1]);
			//				float newdatay = atof(sep.arg[2]);
			//				float newdataz = atof(sep.arg[3]);
			//				memcpy(&zone->zone_header_data[114], &newdatax, sizeof(float));
			//				memcpy(&zone->zone_header_data[118], &newdatay, sizeof(float));
			//				memcpy(&zone->zone_header_data[122], &newdataz, sizeof(float));
			//				zone->SetSafeCoords();

			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			safe_delete(outapp);//delete outapp;
		}
	}
	if(strcasecmp(sep.arg[0], "#zclip") == 0 && admin >= 80) {
		// modifys and resends zhdr packet
		//TODO: make the help entrystate this!
		if(sep.arg[2][0]==0)
			Message(BLACK, "Usage: #zclip <min clip> <max clip>");
		else if(atoi(sep.arg[1])<=0)
			Message(BLACK, "ERROR: Min clip can not be zero or less!");
		else if(atoi(sep.arg[2])<=0)
			Message(BLACK, "ERROR: Max clip can not be zero or less!");
		else if(atoi(sep.arg[1])>atoi(sep.arg[2]))
			Message(BLACK, "ERROR: Min clip is greater than max clip!");
		else {
			zone->newzone_data.minclip = atof(sep.arg[1]);
			zone->newzone_data.maxclip = atof(sep.arg[2]);
			//				float newdatamin = atof(sep.arg[1]);
			//				float newdatamax = atof(sep.arg[2]);
			//				memcpy(&zone->zone_header_data[134], &newdatamin, sizeof(float));
			//				memcpy(&zone->zone_header_data[138], &newdatamax, sizeof(float));

			APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
			memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
			entity_list.QueueClients(this, outapp);
			safe_delete(outapp);//delete outapp;
		}
	}
	if(strcasecmp(sep.arg[0], "#zsave") == 0 && admin >= 200) {
		// modifys and resends zhdr packet
		if(sep.arg[1][0]==0)
			Message(BLACK, "Usage: #zsave <file name>");
		else {
			zone->SaveZoneCFG(sep.argplus[1]);
		}
	}
	//else if (strcasecmp(sep.arg[0], "#npccast") == 0 && admin >= 80) {
	//	if (target && target->IsNPC() && sep.arg[1] != 0 && sep.IsNumber(2)) {
	//		Mob* spelltar = entity_list.GetMob(sep.arg[1]);
	//		if (spelltar) {
	//			target->CastSpell(spells_handler.GetSpellPtr(atoi(sep.arg[2])), spelltar->GetID());
	//		}
	//		else {
	//			Message(BLACK, "Error: %s not found", sep.arg[1]);
	//		}
	//	}
	//	else {
	//		Message(BLACK, "Usage: (needs NPC targeted) #npccast targetname spellid");
	//	}
	//}
	if (strcasecmp(sep.arg[0], "#dbspawn2") == 0 && (admin >= 200)) // Image's Spawn Code -- Rewrite by Scruffy
	{
		if (sep.IsNumber(1) && sep.IsNumber(2) && sep.IsNumber(3)) {
			cout << "Spawning: Database spawn" << endl; 
			Database::Instance()->CreateSpawn2(atoi(sep.arg[1]),pp.current_zone,heading, x_pos, y_pos, z_pos, atoi(sep.arg[2]), atoi(sep.arg[3]));
		}
		else {
			Message(BLACK, "Usage: #dbspawn2 spawngroup respawn variance");
		}
	}
	//else if (strcasecmp(sep.arg[0], "#npcupdate") == 0 && admin > 200) 
	//{ 
	// maalanar: used to move a spawn location in spawn2 
	//if (target != 0 && target->IsNPC()) 
 //           { 
 //                   int32 spawngroupid = target->CastToNPC()->GetSpawnGroupId(); 
 //                   cout << "Moving spawn position of spawngroupid " << spawngroupid << " to pos " << x_pos << ", " << y_pos << ", " << z_pos << " at heading " << heading << endl; 
 //                   if (Database::Instance()->UpdateNpcSpawnLocation(spawngroupid, x_pos, y_pos, z_pos, heading)) 
 //                           target->CastToNPC()->GMMove(x_pos, y_pos, z_pos, heading); 
 //                   else 
 //                           Message(BLACK, "Error updating spawn location"); 
 //           } 
 //           else 
 //                   Message(BLACK, "Usage: NPC needs to be targetted when using this command"); 
 //   } 
 //   else if (strcasecmp(sep.arg[0], "#npcdelete") == 0 && admin > 200) 
 //   { 
 //           // maalanar: used to delete spawn info from spawn2 
 //           if (target != 0 && target->IsNPC()) 
 //           { 
 //                   int32 spawngroupid = target->CastToNPC()->GetSpawnGroupId(); 
 //                   cout << "Deleting spawngroupid " << spawngroupid << " from zone" << endl; 
 //                   if (Database::Instance()->DeleteNpcSpawnLocation(spawngroupid)) 
 //                           target->Depop(); 
 //                   else 
 //                           Message(BLACK, "Error deleting spawn"); 
 //           } 
 //           else 
 //                   Message(BLACK, "Usage: NPC needs to be targetted when using this command"); 
 //   } 
	if (strcasecmp(sep.arg[0], "#npctypespawn") == 0 && (admin >= 10))
	{
		if (sep.IsNumber(1)) {
			NPCType* tmp = 0;
			if ( (tmp = Database::Instance()->GetNPCType(atoi(sep.arg[1]))) )
			{
				NPC* npc = new NPC(tmp, 0, x_pos, y_pos, z_pos, heading);
				entity_list.AddNPC(npc); 
			}
			else
				Message(BLACK, "NPC Type %i not found", atoi(sep.arg[1])); 
		}
		else {
			Message(BLACK, "Usage: #dbspawn npctypeid");
		}
	}
	if (strcasecmp(sep.arg[0], "#sic") == 0 && admin >= EQC_Alpha_Tester) {
		if (target && target->IsNPC() && sep.arg[1] != 0) {
			Mob* sictar = entity_list.GetMob(sep.arg[1]);
			if (sictar) {
				target->CastToNPC()->AddToHateList(sictar, 0, 100000);
			}
			else {
				Message(BLACK, "Error: %s not found", sep.arg[1]);
			}
		}
		else {
			Message(BLACK, "Usage: (needs NPC targeted) #sic targetname");
		}
	}
	if (strcasecmp(sep.arg[0], "#sicrange") == 0 && admin >= 150)
	{
		if(target)
		{
			if(sep.IsNumber(1))
			{
				int32 distance = atoi(sep.arg[1]);
				if(distance > 10000)
				{
					for(int i = 0; i < 5000; i++)
					{
						Mob* sictar = entity_list.GetMob(i);
						if (sictar && sictar->IsNPC())
						{
							sictar->CastToNPC()->AddToHateList(target, 0, 100000);
						}
					}
				}
				else
				{
					for(int i = 0; i < 5000; i++)
					{
						Mob* sictar = entity_list.GetMob(i);
						if (sictar && sictar->IsNPC() && fdistance(sictar->GetX(), sictar->GetY(), this->GetX(), this->GetY()) <= distance)
						{
							sictar->CastToNPC()->AddToHateList(target, 0, 100000);
						}
					}
				}
			}
		}
		else 
		{
			Message(BLACK, "Usage: (needs NPC targeted) #sic targetname");
		}
	}
	if (strcasecmp(sep.arg[0], "#zstats") == 0 && admin >= 80 ) {
		Message(BLACK, "Zone Header Data:");
		Message(BLACK, "Sky Type: %i", zone->newzone_data.sky);
		//Message(BLACK, "Sky Colour: Red: %i; Blue: %i; Green %i", zone->newzone_data.unknown230[1], zone->newzone_data.unknown230[5], zone->newzone_data.unknown230[9]);
		Message(BLACK, "Safe Coords: %f, %f, %f", zone->newzone_data.safe_x, zone->newzone_data.safe_y, zone->newzone_data.safe_z);
		Message(BLACK, "Underworld Coords: %f", zone->newzone_data.underworld);
		Message(BLACK, "Clip Plane: %f - %f", zone->newzone_data.minclip, zone->newzone_data.maxclip);
	}
	if (strcasecmp(sep.arg[0], "#lock") == 0 && admin >= 150) {
		ServerPacket* outpack = new ServerPacket(ServerOP_Lock, sizeof(ServerLock_Struct));
		ServerLock_Struct* lss = (ServerLock_Struct*) outpack->pBuffer;
		strcpy(lss->myname, this->GetName());
		lss->mode = 1;
		worldserver.SendPacket(outpack);
		safe_delete(outpack);//delete outpack;
	}
	if (strcasecmp(sep.arg[0], "#unlock") == 0 && admin >= 150) {
		ServerPacket* outpack = new ServerPacket(ServerOP_Lock, sizeof(ServerLock_Struct));
		ServerLock_Struct* lss = (ServerLock_Struct*) outpack->pBuffer;
		strcpy(lss->myname, this->GetName());
		lss->mode = 0;
		worldserver.SendPacket(outpack);
		safe_delete(outpack);//delete outpack;
	}
	if (strcasecmp(sep.arg[0], "#motd") == 0 && admin >= 150) {
		ServerPacket* outpack = new ServerPacket(ServerOP_Motd, sizeof(SetServerMotd_Struct));
		SetServerMotd_Struct* mss = (SetServerMotd_Struct*) outpack->pBuffer;
		strcpy(mss->myname, this->GetName());
		strcpy(mss->motd, sep.argplus[1]);
		worldserver.SendPacket(outpack);
		safe_delete(outpack);//delete outpack;
	}
	if (strcasecmp(sep.arg[0], "#uptime") == 0) {
		if (!worldserver.Connected()) {
			Message(BLACK, "Error: World server disconnected");
		}
		else if (sep.IsNumber(1) && atoi(sep.arg[1]) > 0) {
			ServerPacket* pack = new ServerPacket(ServerOP_Uptime, sizeof(ServerUptime_Struct));
			ServerUptime_Struct* sus = (ServerUptime_Struct*) pack->pBuffer;
			strcpy(sus->adminname, this->GetName());
			sus->zoneserverid = atoi(sep.arg[1]);
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
		else {
			ServerPacket* pack = new ServerPacket(ServerOP_Uptime, sizeof(ServerUptime_Struct));
			ServerUptime_Struct* sus = (ServerUptime_Struct*) pack->pBuffer;
			strcpy(sus->adminname, this->GetName());
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
	if(strcasecmp(sep.arg[0], "#makepvp") == 0 )
	{
		//PP PVP flag doesn't work, setting it manually here...
		//if(pp.pvp == 1)
		cout << "Attemping to make players name red." << endl;
		this->SendAppearancePacket(this->GetID(), SAT_PvP, 1, false);
	}
	if (strcasecmp(sep.arg[0], "#testspawn") == 0 && admin >= 250) {
		NPC* npc = new NPC(Database::Instance()->GetNPCType(1286), 0, this->GetX(), this->GetY(), this->GetZ(), this->GetHeading());
		entity_list.AddNPC(npc, false);
		APPLAYER* outapp = new APPLAYER(OP_NewSpawn, sizeof(NewSpawn_Struct));
		NewSpawn_Struct* ns = (NewSpawn_Struct*) outapp->pBuffer;
		npc->FillSpawnStruct(ns, 0);
		strcpy(ns->spawn.name, "Test");
		ns->spawn.NPC = 0;
		//			ns->spawn.not_linkdead = 1;
		ns->spawn.npc_armor_graphic = 0xFF;
		ns->spawn.npc_helm_graphic = 0xFF;
		ns->spawn.equipment[0] = 3;
		ns->spawn.equipment[1] = 3;
		ns->spawn.equipment[2] = 3;
		ns->spawn.equipment[3] = 3;
		ns->spawn.equipment[4] = 3;
		ns->spawn.equipment[5] = 3;
		ns->spawn.equipment[6] = 3;
		ns->spawn.equipment[7] = 3;
		ns->spawn.equipment[8] = 3;
		uchar* buf = (uchar*) &ns->spawn;
		if (sep.IsNumber(2)) {
			Message(BLACK, "%i = %i", atoi(sep.arg[1]), atoi(sep.arg[2]));
			if (atoi(sep.arg[2]) >= 65536)
				*((int32*) buf[atoi(sep.arg[1])]) = atoi(sep.arg[2]);
			else if (atoi(sep.arg[2]) >= 256)
				*((int16*) buf[atoi(sep.arg[1])]) = atoi(sep.arg[2]);
			else
				buf[atoi(sep.arg[1])] = atoi(sep.arg[2]);
		}
		//DumpPacket(buf, sizeof(Spawn_Struct));
		EncryptSpawnPacket(outapp);
		entity_list.QueueClients(0, outapp);
		safe_delete(outapp);//delete outapp;
	}
	if (strcasecmp(sep.arg[0],"#makepet") == 0 && admin >= 20) {
		if (!(sep.IsNumber(1) && sep.IsNumber(2) && sep.IsNumber(3) && sep.IsNumber(4))) {
			Message(BLACK, "Usage: #makepet level class race texture");
		}
		else {
			this->MakePet(atoi(sep.arg[1]), atoi(sep.arg[2]), atoi(sep.arg[3]), atoi(sep.arg[4]));
		}
	}
	if (strcasecmp(sep.arg[0], "#ShowPetSpell") == 0 && admin >= 250) {
		if (sep.arg[1][0] == 0)
			Message(BLACK, "Usage: #ShowPetSpells [petsummonstring]");
		else if (!spells_handler.SpellsLoaded())
			Message(BLACK, "Spells not loaded");
		else if (Seperator::IsNumber(sep.argplus[1])) {
			int spellid = atoi(sep.argplus[1]);
			if (spellid <= 0 || spellid >= SPDAT_RECORDS) {
				Message(BLACK, "Error: Number out of range");
			}
			else {
				Spell* spell = spells_handler.GetSpellPtr(spellid);
				Message(BLACK, "  %i: %s, %s", spellid, spell->GetSpellTeleportZone(), spell->GetSpellName());
			}
		}
		else {
			int count=0;
			int iSearchLen = strlen(sep.argplus[1])+1;
			char sName[64];
			char sCriteria[65];
			strncpy(sCriteria, sep.argplus[1], 64);
			_strupr(sCriteria);
			for (int i = 0; i < SPDAT_RECORDS; i++)
			{
				Spell* spell = spells_handler.GetSpellPtr(i);
				int16 EffectID0 = spell->GetSpellEffectID(0);
				const char* TeleportZone = spell->GetSpellTeleportZone();
				const char* SName = spell->GetSpellName();
				if (spell->GetSpellName()[0] != 0 && (EffectID0 == SE_SummonPet || EffectID0 == SE_NecPet)) {
					strcpy(sName, spell->GetSpellTeleportZone());

					_strupr(sName);
					char* pdest = strstr(sName, sCriteria);
					if ((pdest != NULL) && (count <=20)) {
						Message(BLACK, "  %i: %s, %s", i, TeleportZone, SName);
						count++;
					}
					else if (count > 20)
						break;
				}
			}
			if (count > 20)
				Message(BLACK, "20 spells found... max reached.");
			else
				Message(BLACK, "%i spells found.", count);
		}
	}
	//else if (strcasecmp(sep.arg[0], "#freeze") == 0 && admin >= 100) {
	//	if (target != 0) {
	//		target->SendAppearancePacket(0, SAT_Position_Update, SAPP_Lose_Control, true);
	//	}
	//	else {
	//		Message(BLACK, "ERROR: Freeze requires a target.");
	//	}
	//}
	//else if (strcasecmp(sep.arg[0], "#unfreeze") == 0 && admin >= 100) {
	//	if (target != 0) {
	//		target->SendAppearancePacket(0, SAT_Position_Update, SAPP_Sitting_To_Standing, true);
	//	}
	//	else {
	//		Message(BLACK, "ERROR: Unfreeze requires a target.");
	//	}
	//}
	if (strcasecmp(sep.arg[0], "#permaclass")==0 && admin >= 80) {
		if(sep.arg[1][0]==0) {
			Message(BLACK,"FORMAT: #permaclass <classnum>");
		}
		else if(target==0) {
		Message(BLACK, "ERROR: Class requires a target.");
		}
		else {
			Message(BLACK, "Setting your class...Sending you to char select.");
			cout << "Class change request.. Class requested: " << sep.arg[1] << endl;
			pp.class_=atoi(sep.arg[1]);
			Kick();
			pp.class_=atoi(sep.arg[1]);
			Save();
		}
	}
	if (strcasecmp(sep.arg[0], "#permarace")==0 && admin >= 80) {
		if(sep.arg[1][0]==0) {
			Message(BLACK,"FORMAT: #permarace <racenum>");
			Message(BLACK,"NOTE: Not all models are global. If a model is not global, it will appear as a human on character select and in zones without the model.");
		}
		else if(target==0) {
		Message(BLACK, "ERROR: Permarace requires a target.");
		}
		else {
			Message(BLACK, "Setting your race...Sending you to char select.");
			cout << "Permanant race change request.. Race requested: " << sep.arg[1] << endl;
			int8 tmp = Mob::GetDefaultGender(atoi(sep.arg[1]), pp.gender);
			pp.race=atoi(sep.arg[1]);
			pp.gender=tmp;
			Kick();
			pp.race=atoi(sep.arg[1]);
			pp.gender=tmp;
			Save();
		}
	}
	if (strcasecmp(sep.arg[0], "#permagender")==0 && admin >= 80) {
		if(sep.arg[1][0]==0) {
			Message(BLACK,"FORMAT: #permagender <gendernum>");
			Message(BLACK,"Gender Numbers: 0=Male, 2=Female, 3=Neuter");
		}
		else if(target==0) {
		Message(BLACK, "ERROR: Permagender requires a target.");
		}
		else {
			Message(BLACK, "Setting your gender...Sending you to char select.");
			cout << "Permanant gender change request.. Gender requested: " << sep.arg[1] << endl;
			pp.gender=atoi(sep.arg[1]);
			Kick();
			pp.gender=atoi(sep.arg[1]);
			Save();
		}
	}
	if (strcasecmp(sep.arg[0], "#gm") == 0 && admin >= 100) {
		if ((target != 0) && (target->IsClient())) {
			if(strcasecmp(sep.arg[1], "on") == 0) {
				target->CastToClient()->SetGM(true);
				Message(RED, "%s is now a GM!", target->CastToClient()->GetName());
			}
			else if(strcasecmp(sep.arg[1], "off") == 0) {
				target->CastToClient()->SetGM(false);
				Message(RED, "%s is no longer a GM!", target->CastToClient()->GetName());
			}
			else {
				Message(BLACK, "Usage: #gm [on|off]");
			}
		}
		else {
			if(strcasecmp(sep.arg[1], "on") == 0) {
				SetGM(true);
			}
			else if(strcasecmp(sep.arg[1], "off") == 0) {
				SetGM(false);
			}
			else {
				Message(BLACK, "Usage: #gm [on|off]");
			}
		}
	}
	
	if (strcasecmp(sep.arg[0], "#addnode") == 0 && admin >= EQC_Alpha_Tester) {
		char fileNameSuffix[] = "Nodes.txt";
		char fileNamePrefix[100] = "./Maps/Nodes/";
		strcat(fileNamePrefix, zone->GetShortName());
		strcat(fileNamePrefix, fileNameSuffix);
		ofstream myfile;
		myfile.open(fileNamePrefix, ios::app);
		cout<<"Adding New Node to "<<fileNamePrefix<<": "<<this->GetX()<<", "<<this->GetY()<<", "<<this->GetZ()<<endl;
		myfile<<GetX()<<" "<<GetY()<<" "<<GetZ()<<" 0\n";
		myfile.close();
		NPCType* npc_type = new NPCType;
		memset(npc_type, 0, sizeof(NPCType));
		strcpy(npc_type->name,"Node");
		npc_type->cur_hp = 1000; 
		npc_type->max_hp = 1000; 
		npc_type->race = 244; 
		npc_type->gender = 0; 
		npc_type->class_ = 1; 
		npc_type->deity = 1; 
		npc_type->level = 99; 
		npc_type->npc_id = 0;
		npc_type->loottable_id = 0;
		npc_type->texture = 0;
		npc_type->light = 0; 
		// Weapons are broke!!
		//npc_type->equipment[13] = atoi(sep.arg[8]);
		//npc_type->equipment[14] = atoi(sep.arg[9]);
		npc_type->merchanttype = 0;	
		//for (int i=0; i<9; i++) 
		//	npc_type->equipment[i] = atoi(sep.arg[8]); 

		npc_type->STR = 75;
		npc_type->STA = 75;
		npc_type->DEX = 75;
		npc_type->AGI = 75;
		npc_type->INT = 75;
		npc_type->WIS = 75;
		npc_type->CHA = 75;

		NPC* npc = new NPC(npc_type, 0, GetX(), GetY(), GetZ(), heading);

		// Disgrace: add some loot to it!
		npc->AddCash();
		int itemcount = (rand()%5) + 1;
		for (int counter=0; counter<itemcount; counter++)
		{
			Item_Struct* item = 0;
			while (item == 0)
				item = Database::Instance()->GetItem(rand() % 33000);						

			npc->AddItem(item, 0, 0);
		}

		entity_list.AddNPC(npc); 
	}
	if (strcasecmp(sep.arg[0], "#addwaternode") == 0 && admin >= EQC_Alpha_Tester) {
		char fileNameSuffix[] = "Nodes.txt";
		char fileNamePrefix[100] = "./Maps/Nodes/";
		strcat(fileNamePrefix, zone->GetShortName());
		strcat(fileNamePrefix, fileNameSuffix);
		ofstream myfile;
		myfile.open(fileNamePrefix, ios::app);
		cout<<"Adding New WaterNode to "<<fileNamePrefix<<": "<<this->GetX()<<", "<<this->GetY()<<", "<<this->GetZ()<<endl;
		myfile<<GetX()<<" "<<GetY()<<" "<<GetZ()<<" 1\n";
		NPCType* npc_type = new NPCType;
		memset(npc_type, 0, sizeof(NPCType));
		strcpy(npc_type->name,"WaterNode");
		npc_type->cur_hp = 1000; 
		npc_type->max_hp = 1000; 
		npc_type->race = 244; 
		npc_type->gender = 0; 
		npc_type->class_ = 1; 
		npc_type->deity = 1; 
		npc_type->level = 99; 
		npc_type->npc_id = 0;
		npc_type->loottable_id = 0;
		npc_type->texture = 0;
		npc_type->light = 0; 
		// Weapons are broke!!
		//npc_type->equipment[13] = atoi(sep.arg[8]);
		//npc_type->equipment[14] = atoi(sep.arg[9]);
		npc_type->merchanttype = 0;	
		//for (int i=0; i<9; i++) 
		//	npc_type->equipment[i] = atoi(sep.arg[8]); 

		npc_type->STR = 75;
		npc_type->STA = 75;
		npc_type->DEX = 75;
		npc_type->AGI = 75;
		npc_type->INT = 75;
		npc_type->WIS = 75;
		npc_type->CHA = 75;

		NPC* npc = new NPC(npc_type, 0, GetX(), GetY(), GetZ(), heading);

		// Disgrace: add some loot to it!
		npc->AddCash();
		int itemcount = (rand()%5) + 1;
		for (int counter=0; counter<itemcount; counter++)
		{
			Item_Struct* item = 0;
			while (item == 0)
				item = Database::Instance()->GetItem(rand() % 33000);						

			npc->AddItem(item, 0, 0);
		}

		entity_list.AddNPC(npc); 

		cout<<"Adding WaterNode's child to "<<fileNamePrefix<<": "<<this->GetX()<<", "<<this->GetY()<<", "<<this->GetZ()-10<<endl;
		myfile<<GetX()<<" "<<GetY()<<" "<<GetZ()-10<<" 2\n";
		NPCType* npc_type2 = new NPCType;
		memset(npc_type2, 0, sizeof(NPCType));
		strcpy(npc_type2->name,"WaterNodeChild");
		npc_type2->cur_hp = 1000; 
		npc_type2->max_hp = 1000; 
		npc_type2->race = 244; 
		npc_type2->gender = 0; 
		npc_type2->class_ = 1; 
		npc_type2->deity = 1; 
		npc_type2->level = 99; 
		npc_type2->npc_id = 0;
		npc_type2->loottable_id = 0;
		npc_type2->texture = 0;
		npc_type2->light = 0; 
		// Weapons are broke!!
		//npc_type->equipment[13] = atoi(sep.arg[8]);
		//npc_type->equipment[14] = atoi(sep.arg[9]);
		npc_type2->merchanttype = 0;	
		//for (int i=0; i<9; i++) 
		//	npc_type->equipment[i] = atoi(sep.arg[8]); 

		npc_type2->STR = 75;
		npc_type2->STA = 75;
		npc_type2->DEX = 75;
		npc_type2->AGI = 75;
		npc_type2->INT = 75;
		npc_type2->WIS = 75;
		npc_type2->CHA = 75;

		NPC* npc2 = new NPC(npc_type2, 0, GetX(), GetY(), GetZ()-10, heading);

		// Disgrace: add some loot to it!
		npc2->AddCash();
		int itemcount2 = (rand()%5) + 1;
		for (int counter=0; counter<itemcount2; counter++)
		{
			Item_Struct* item2 = 0;
			while (item2 == 0)
				item2 = Database::Instance()->GetItem(rand() % 33000);						

			npc2->AddItem(item2, 0, 0);
		}

		entity_list.AddNPC(npc2); 

		myfile.close();
	}
	if (strcasecmp(sep.arg[0], "#deletenode") == 0 && admin >= EQC_Alpha_Tester) {
		if(target == NULL || target->GetLevel() != 99){
			Message(BLACK,"You must target a node to use this command.");
			return;
		}
		float x, y, z;
		int16 waterFlag;
		float removeX, removeY, removeZ;
		float distance;
		zoneNode tempNodes[5000] = {0};
		int nodeCounter = -1;
		char fileNameSuffix[] = "Nodes.txt";
		char fileNamePrefix[100] = "./Maps/Nodes/";
		strcat(fileNamePrefix, zone->GetShortName());
		strcat(fileNamePrefix, fileNameSuffix);
		ifstream myfile (fileNamePrefix);
		if (myfile.is_open())
		{
			while (! myfile.eof() )
			{
				nodeCounter++;
				myfile >> x >> y >> z >> waterFlag >> distance;
				tempNodes[nodeCounter].x = x;	
				tempNodes[nodeCounter].y = y;
				tempNodes[nodeCounter].z = z;
				tempNodes[nodeCounter].waterNodeFlag = waterFlag;
				tempNodes[nodeCounter].distanceFromCenter = distance;
			}
		}
		myfile.close();
		ofstream myfile2;
		myfile2.open(fileNamePrefix, ios::trunc);
		myfile2.close();
		removeX = this->target->GetX();
		removeY = this->target->GetY();
		removeZ = this->target->GetZ();
		for(int i = 0; i <= nodeCounter; i++) {
			if(tempNodes[i].x == removeX && tempNodes[i].y == removeY && tempNodes[i].z == removeZ) {
				tempNodes[i].x = NULL_NODE;
				tempNodes[i].y = NULL_NODE;
				tempNodes[i].z = NULL_NODE;
			}
		}
		ofstream myfile3;
		myfile3.open(fileNamePrefix);
		for(int i = 0; i <= nodeCounter; i++) {
			if(tempNodes[i].x != NULL_NODE && tempNodes[i].y != NULL_NODE && tempNodes[i].z != NULL_NODE)
				myfile3<<tempNodes[i].x<<" "<<tempNodes[i].y<<" "<<tempNodes[i].z<<" "<<tempNodes[i].waterNodeFlag<<" "<<tempNodes[i].distanceFromCenter<<"\n";
		}
		myfile3.close();
		this->target->CastToNPC()->Depop();
	}
	if (strcasecmp(sep.arg[0], "#shownodes") == 0 && admin >= EQC_Alpha_Tester) {
		float x, y, z;
		int16 waterFlag;
		char fileNameSuffix[] = "Nodes.txt";
		char fileNamePrefix[100] = "./Maps/Nodes/";
		strcat(fileNamePrefix, zone->GetShortName());
		strcat(fileNamePrefix, fileNameSuffix);
		ifstream myfile (fileNamePrefix);
		int16 distance;
		int16 nodeCounter = 0;
		if (myfile.is_open())
		{
			while (! myfile.eof() )
			{
				myfile >> x >> y >> z >> waterFlag >> distance;
				NPCType* npc_type = new NPCType;
				memset(npc_type, 0, sizeof(NPCType));
				if(waterFlag == 1)
					strcpy(npc_type->name,"AboveWaterNode");
				else if(waterFlag == 2)
					strcpy(npc_type->name,"BelowWaterNode");
				else	
					strcpy(npc_type->name,"Node");
				npc_type->cur_hp = 1000; 
				npc_type->max_hp = 1000; 
				npc_type->race = 244; 
				npc_type->gender = 0; 
				npc_type->class_ = 1; 
				npc_type->deity = 1; 
				npc_type->level = 99; 
				npc_type->npc_id = 0;
				npc_type->loottable_id = 0;
				npc_type->texture = 0;
				npc_type->light = 0; 
				// Weapons are broke!!
				//npc_type->equipment[13] = atoi(sep.arg[8]);
				//npc_type->equipment[14] = atoi(sep.arg[9]);
				npc_type->merchanttype = 0;	
				//for (int i=0; i<9; i++) 
				//	npc_type->equipment[i] = atoi(sep.arg[8]); 

				npc_type->STR = 75;
				npc_type->STA = 75;
				npc_type->DEX = 75;
				npc_type->AGI = 75;
				npc_type->INT = 75;
				npc_type->WIS = 75;
				npc_type->CHA = 75;
				npc_type->ATK = nodeCounter;
				nodeCounter++;

				NPC* npc = new NPC(npc_type, 0, x, y, z, heading);

				// Disgrace: add some loot to it!
				npc->AddCash();
				int itemcount = (rand()%5) + 1;
				for (int counter=0; counter<itemcount; counter++)
				{
					Item_Struct* item = 0;
					while (item == 0)
						item = Database::Instance()->GetItem(rand() % 33000);						

					npc->AddItem(item, 0, 0);
				}

				entity_list.AddNPC(npc);
			}
			myfile.close();
		}

		else cout << "Unable to open file"; 
	}
	//else if (strcasecmp(sep.arg[0], "#fear")==0 && admin >= 255) {
	//	this->target->CalculateNewFearpoint();
	//}
	//else if (strcasecmp(sep.arg[0], "#fearzonefornodes")==0 && admin >= 255) {
	//	entity_list.MassFear(this);
	//}
	if (strcasecmp(sep.arg[0], "#createdirectedgraph") == 0 && admin >= EQC_Alpha_Tester) {
		createDirectedGraph();
	}
	if(strcasecmp(sep.arg[0], "#debug") == 0 && admin >= EQC_Alpha_Tester) {	// Cofruben: add privileges when alpha is out!
		if (!sep.arg[1] || !sep.arg[2] || !sep.IsNumber(2) || atoi(sep.arg[2]) > 1 || atoi(sep.arg[2]) < 0) {
			Message(BLACK, "Error: syntax is #debug spells/attack/faction/merchants/updates/items/pathing/file_log  0/1");
			return;
		}
		const char* debug = sep.arg[1];
		bool  value = (bool) atoi(sep.arg[2]);
		Client* target = NULL;
		if (admin < 100)
			target = this;
		else
			target = (GetTarget() && GetTarget()->IsClient()) ? GetTarget()->CastToClient() : this;
		if (!target) return;
		if(strcasecmp(debug, "spells") == 0)
			CAST_CLIENT_DEBUG_PTR(target)->SetDebuggingSpells(value);
		if(strcasecmp(debug, "attack") == 0)
			CAST_CLIENT_DEBUG_PTR(target)->SetDebuggingAttack(value);
		if(strcasecmp(debug, "faction") == 0)
			CAST_CLIENT_DEBUG_PTR(target)->SetDebuggingFaction(value);
		if(strcasecmp(debug, "updates") == 0)
			CAST_CLIENT_DEBUG_PTR(target)->SetDebuggingUpdates(value);
		if(strcasecmp(debug, "merchants") == 0)
			CAST_CLIENT_DEBUG_PTR(target)->SetDebuggingMerchants(value);
		if(strcasecmp(debug, "items") == 0)
			CAST_CLIENT_DEBUG_PTR(target)->SetDebuggingItems(value);
		if(strcasecmp(debug, "pathing") == 0)
			CAST_CLIENT_DEBUG_PTR(target)->SetDebuggingPathing(value);
		if(strcasecmp(debug, "file_log") == 0)
			CAST_CLIENT_DEBUG_PTR(target)->SetLogging(value);
	}
	if (strcasecmp(sep.arg[0], "#createzoneline")==0 && admin >= 255){
		if(strcmp(sep.arg[1], "") == 0){
			Message(WHITE, "Valid syntax: #createzoneline connecting_zone_name");
			return;
		}
		int16 range;
		if(atoi(sep.arg[2]) == 0)
			range = 75;
		else
			range = atoi(sep.arg[2]);
		char connectingZone[16];
		strcpy(connectingZone, sep.arg[1]);
		CreateZoneLineNode(zone->GetShortName(), connectingZone, range);
	}
	if (strcasecmp(sep.arg[0], "#pFaction") == 0 && admin >= EQC_Alpha_Tester){
		if(target && target->IsNPC())
			Message(BLACK, "%s's primary faction is: %i", target->GetName(), target->CastToNPC()->GetPrimaryFactionID());
		else
			Message(BLACK, "Error: #pFaction requires an NPC target");
	}
	//else if (strcasecmp(sep.arg[0], "#adjustroamrange")==0 && admin >= 255)
	//{	
	//	int32 spawngroupID = Database::Instance()->GetSpawnGroupID(target->GetNPCTypeID());
	//	int range = atoi(sep.arg[1]);
	//	if(!range)
	//	{
	//		Message(BLACK, "#RoamRange requires a range (-1 for infinite/zonewide)");
	//	}
	//	else
	//	{
	//		Message(BLACK, "%s's spawngroupID is %i. Adding range %i to its set of records", target->GetName(), spawngroupID, range);
	//		Database::Instance()->AddRoamingRange(spawngroupID, range);
	//	}
	//}
	//else if (strcasecmp(sep.arg[0], "#range")==0 && admin >= 255)
	//{
	//	if(!target)
	//	{
	//		Message(BLACK, "#Range requires a target!");
	//	}
	//	else
	//	{
	//		int distance = int(fdistance(GetX(), GetY(), target->GetX(), target->GetY()));
	//		Message(BLACK, "You are %i units away from %s", distance, target->GetName());
	//	}
	//}
	//else if (strcasecmp(sep.arg[0], "#roamrange")==0 && admin >= 255)
	//{
	//	if(target->IsNPC())
	//	{
	//		Message(BLACK, "%s's roam range is %i", target->GetName(), target->CastToNPC()->GetRoamRange());
	//	}
	//	else
	//	{
	//		Message(BLACK, "#Range requires an NPC target!");
	//	}
	//}
	//else if (strcasecmp(sep.arg[0], "#roambox")==0 && admin >= 255)
	//{
	//	if(target->IsNPC())
	//	{
	//		Message(BLACK, "%s's roambox: %i", target->GetName(), target->CastToNPC()->GetRoamBox());
	//	}
	//	else
	//		Message(BLACK, "#Roambox requires an NPC target!");
	//}
	if (strcasecmp(sep.arg[0], "#resolveAllPaths") == 0 && admin >= EQC_Alpha_Tester) {
		if(target == 0 || !target->IsNPC())
		{
			this->Message(WHITE, "You must target an NPC to resolve all paths.");
			return;
		}
		char fileNameSuffix[] = "Paths.txt";
		char fileNamePrefix[100] = "./Maps/Paths/";
		strcat(fileNamePrefix, zone->GetShortName());
		strcat(fileNamePrefix, fileNameSuffix);
		ofstream myfile;
		myfile.open(fileNamePrefix);
		int16 counter;
		for(int a = 0; a < zone->numberOfNodes; a++)
		{
			for(int b = 0; b < zone->numberOfNodes; b++)
			{
				if (a == b)
					continue;
				counter = 0;
				target->findMyPath(zone->thisZonesNodes[a], zone->thisZonesNodes[b], true);
				myfile<<"<N> "<<a<<endl;
				myfile<<" <D> "<<b<<endl;
				myfile<<"  <P> ";
				while(!target->CastToNPC()->GetMyPath().empty())
				{
					counter++;
					myfile<<target->CastToNPC()->GetMyPath().front()<<" ";
					target->CastToNPC()->GetMyPath().pop();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				myfile<<"9999"<<endl;
			}
		}
		myfile.close();
	}
	//else if (strcasecmp(sep.arg[0], "#pullzone") == 0 && admin >= 255)
	//{
	//	for(int i = 0; i < 5000; i++)
	//	{
	//		Mob* sictar = entity_list.GetMob(i);
	//		if (sictar && sictar->IsNPC())
	//		{
	//			sictar->CastToNPC()->AddToHateList(this, 0, 100000);
	//		}
	//	}
	//}
	//else if (strcasecmp(sep.arg[0], "#patrol")==0 && admin >= 255) {
	//	if(target && target->IsNPC())
	//	{
	//		int number = target->CastToNPC()->GetPatrolID();
	//		Message(WHITE, "%s's patrol grid ID: %i", target->GetName(), number);
	//	}
	//}
	if (strcasecmp(sep.arg[0], "#resolveallgrids")==0 && admin >= 255) {
		if(target == 0 || !target->IsNPC())
		{
			this->Message(WHITE, "You must target an NPC to parse the grid data.");
			return;
		}
		zone->ParseGridData(this->target);
	}
	if (strcasecmp(sep.arg[0], "#DepopAllButThis") == 0 && admin >= 255)
	{
		if(!target || !target->IsNPC())
		{
			this->Message(WHITE, "You must target an NPC to use this command.");
			return;
		}
		for(int i = 0; i < 5000; i++)
		{
			Mob* droptar = entity_list.GetMob(i);
			if (droptar && droptar->IsNPC() && droptar != this->target)
			{
				droptar->Depop();
			}
		}
	}
	if (strcasecmp(sep.arg[0], "#showzoneagro") == 0 && admin >= 255)
	{
		if(zone->zoneAgroCounter == 0)
		{
			Message(LIGHTEN_BLUE, "Debug: No NPCs currently engaged");
			return;
		}
		Message(LIGHTEN_BLUE, "Debug: %i NPCs currently engaged:", zone->zoneAgroCounter);
		for(int i = 0; i < zone->zoneAgroCounter; i++)
		{
			Mob* mob = entity_list.GetMob(zone->zoneAgro[i]);
			if(zone->zoneAgro[i] == 0 || mob == 0 || !mob->IsNPC())
				Message(RED, "Debug: NPC #%i: ERROR", i + 1);
			else
				Message(LIGHTEN_BLUE, "Debug: NPC #%i: %s", i + 1, mob->GetName());
		}
	}
	//else if (strcasecmp(sep.arg[0], "#weapons") == 0 && admin >= 255)
	//{
	//	if(!target)
	//		return;
	//	if(!target->IsNPC())
	//		return;
	//	if(target->GetMyMainHandWeapon() != 0)
	//		this->Message(WHITE, "%s's primary weapon: %s", target->GetName(), target->GetMyMainHandWeapon()->name);
	//	if(target->GetMyOffHandWeapon() != 0)
	//		this->Message(WHITE, "%s's secondary weapon: %s", target->GetName(), target->GetMyOffHandWeapon()->name);
	//}
	//else if (strcasecmp(sep.arg[0], "#showhatelist") == 0 && admin >= 255)
	//{
	//	if(!target)
	//		return;
	//	if(!target->IsNPC())
	//		return;
	//	Mob* mostHated = NULL;
	//	mostHated = target->CastToNPC()->GetHateTop();
	//	if(mostHated)
	//		Message(WHITE, "%s's target: %s", target->GetName(), mostHated->GetName());
	//}
	//else if (strcasecmp(sep.arg[0], "#myfootprint") == 0 && admin >= 255)
	//{
	//	Message(RED, "Debug: Your network footprint: %i bytes", myNetworkFootprint);
	//}
	//else if (strcasecmp(sep.arg[0], "#charmme") == 0 && admin >= 255)
	//{
	//	if(target && target->IsNPC())
	//	{
	//		Spell* spell = spells_handler.GetSpellPtr(300);
	//		target->CastSpell(spell, this->GetID());
	//	}
	//}
	if (strcasecmp(sep.arg[0], "#worldkick") == 0 && admin >= EQC_Alpha_Tester)
	{
		if(!atoi(sep.arg[1]))
		{
			Message(WHITE, "#Worldkick requires an login server account id!");
		}
		else
		{
			if(Database::Instance()->Worldkick(atoi(sep.arg[1])))
				Message(WHITE, "World kick successful!");
			else
				Message(WHITE, "World kick failed");
		}
	}
	if (strcasecmp(sep.arg[0], "#benchmark") == 0 && admin >= 255)
	{
		Message(WHITE, "Debug: Average entity process time in %s: %0.6fms", zone->GetLongName(), zone->GetAverageProcessTime());
	}
	//else if (strcasecmp(sep.arg[0], "#changefaction") == 0 && admin >= EQC_Alpha_Tester)
	//{
	//	if(!target || (target && !target->IsNPC()))
	//	{
	//		Message(WHITE, "#ChangeFaction requires an NPC target!");
	//		return;
	//	}
	//	else if(!sep.IsNumber(1) || atoi(sep.arg[1]) < -1500 || atoi(sep.arg[1]) > 1500)
	//	{
	//		Message(WHITE, "#ChangeFaction x (-1500 to 1500)");
	//		return;
	//	}
	//	else if(target->CastToNPC()->GetPrimaryFactionID() == 0)
	//	{
	//		Message(WHITE, "Your NPC target does not belong to a faction!");
	//		return;
	//	}
	//	if(Database::Instance()->AdjustFaction(this->CharacterID(), target->CastToNPC()->GetPrimaryFactionID(), atoi(sep.arg[1])))
	//		Message(WHITE, "Successfully updated your faction standing!");
	//	else
	//		Message(RED, "Error: Faction standing update failed! You must first establish a non-default standing with your target's faction.");
	//}
	//else if (strcasecmp(sep.arg[0], "#dumpitems") == 0 && admin >= EQC_Alpha_Tester)
	//{
	//	ofstream myfile;
	//	myfile.open("items.txt");
	//	Item_Struct* item = 0;
	//	myfile<<"Name"<<","<<"ItemLore"<<","<<"Weight"<<","<<"NoRent"<<","<<"NoDrop"<<","<<"Size"<<","<<"Type"<<","<<"Slots"<<","<<"STR"<<","<<"STA"<<","<<"CHA"<<","<<"DEX"<<","<<"INT"<<","<<"AGI"<<","<<"WIS"<<","<<"MR"<<","<<"FR"<<","<<"CR"<<","<<"DR"<<","<<"PR"<<","<<"HP"<<","<<"MANA"<<","<<"AC"<<","<<"Stackable"<<","<<"Delay"<<","<<"Damage"<<","<<"Skill"<<","<<"Classes"<<","<<"Range"<<","<<"Magic"<<","<<"Races"<<","<<"Effect"<<","<<"CastTime"<<endl;
	//	for(int i = 1000; i < 32600; i++)
	//	{
	//		item = Database::Instance()->GetItem(i);
	//		if(item)
	//		{
	//			myfile<<item->name<<","<<item->lore<<","<<(int)item->weight<<","<<(int)item->norent<<","<<(int)item->nodrop<<","<<(int)item->size<<","<<(int)item->type<<","<<item->equipableSlots<<","<<(int)item->common.STR<<","<<(int)item->common.STA<<","<<(int)item->common.CHA<<","<<(int)item->common.DEX<<","<<(int)item->common.INT<<","<<(int)item->common.AGI<<","<<(int)item->common.WIS<<","<<(int)item->common.MR<<","<<(int)item->common.FR<<","<<(int)item->common.CR<<","<<(int)item->common.DR<<","<<(int)item->common.PR<<","<<(int)item->common.HP<<","<<(int)item->common.MANA<<","<<(int)item->common.AC<<","<<(int)item->common.stackable<<","<<(int)item->common.delay<<","<<(int)item->common.damage<<","<<(int)item->common.range<<","<<(int)item->common.itemType<<","<<(int)item->common.classes<<","<<(int)item->common.magic<<","<<item->common.normal.races<<","<<(int)item->common.click_effect_id<<","<<(int)item->common.casttime<<endl;
	//		}
	//	}
	//	myfile.close();
	//}
	if (strcasecmp(sep.arg[0], "#clearspellrecasts") == 0 && admin >= 5)
	{
		for(int i = 0; i < 8; i++)
			SetSpellRecastTimer(i, 0);
	}
			
}

float fdistance(float x1, float y1, float x2, float y2){
	return sqrt(square(x2-x1)+square(y2-y1));
}

float square(float number){
	return number * number;
}