#include "seperator.h"
#include "client.h"

#define	COMMAND_CHAR	'#'
#define CMDALIASES	5

typedef void (*CmdFuncPtr)(Client *,const Seperator *);


typedef struct {
	const char *command[CMDALIASES];			// the command(s)
	int access;
	const char *desc;		// description of command
	CmdFuncPtr function;	//null means perl function
} CommandRecord;

extern int (*command_dispatch)(Client *,char *);
extern int commandcount;			// number of commands loaded

// the command system:
int command_init(void);
void command_deinit(void);
int command_add(const char *command_string, const char *desc, int access, CmdFuncPtr function);
int command_notavail(Client *c, char *message);
int command_realdispatch(Client *c, char *message);
void command_logcommand(Client *c, char *message);
int command_add_perl(const char *command_string, const char *desc, int access);
void command_clear_perl();

/*
Harakiri - your commands go here
*/
// list all commands available for admin level
void command_help(Client *c, const Seperator *sep);
// goto xyz in current zone
void command_goto(Client *c, const Seperator *sep);
// set level of target
void command_level(Client *c, const Seperator *sep);
// do damage to current target
void command_damage(Client *c, const Seperator *sep);
// complete heal target
void command_heal(Client *c, const Seperator *sep);
// full mana to target
void command_mana(Client *c, const Seperator *sep);
// kill target
void command_kill(Client *c, const Seperator *sep);
// sent new time of day temporary to clients
void command_timeofday(Client *c, const Seperator *sep);
// summons an item onto the cursor
void command_summonitem(Client *c, const Seperator *sep);
// adds xp to the player
void command_addexp(Client *c, const Seperator *sep);
// set xp of the player
void command_setexp(Client *c, const Seperator *sep);
// summon corpse
void command_corpse(Client *c, const Seperator *sep);
// finds item by name and shows id
void command_itemsearch(Client *c, const Seperator *sep);
// zone to safe coord in specified zonename
void command_zone(Client *c, const Seperator *sep);
// show current loc, including z
void command_loc(Client *c, const Seperator *sep);
// show targeted npc status
void command_npcstats(Client *c, const Seperator *sep);
// search for a spell
void command_findspell(Client *c, const Seperator *sep);
// find a recipe by name
void command_findreceipe(Client *c, const Seperator *sep);
// summon all items needed for combine of a recipe
void command_receipesummon(Client *c, const Seperator *sep);
// Harakiri fix issue with summonitems not getting to client with bogus item, i.e.
void command_clearcursor(Client *c, const Seperator *sep);
// reload quest/perl
void command_reloadperl(Client *c, const Seperator *sep);
// diff an item from blob and nonblob table
void command_diffitem(Client *c, const Seperator *sep);
// teleport to zone + x + y + z + heading
void command_teleport(Client *c, const Seperator *sep);
// translocate to zone + x + y + z
void command_translocate(Client *c, const Seperator *sep);
// summon an item from the nonblob tabel
void command_summonitemnonblob(Client *c, const Seperator *sep);
// adds money to target or player
void command_givemoney(Client *c, const Seperator *sep);
// cast spell by id
void command_castspell(Client *c, const Seperator *sep);
// set target invul
void command_invulnerable(Client *c, const Seperator *sep);
// list all languages and their ids
void command_listlang(Client *c, const Seperator *sep);
// set language skill
void command_setlang(Client *c, const Seperator *sep);
// set skill by id
void command_setskill(Client *c, const Seperator *sep);
// set all skills
void command_setskillall(Client *c, const Seperator *sep);
// verify skills
void command_checkskillall(Client *c, const Seperator *sep);
// manually save target
void command_save(Client *c, const Seperator *sep);
// scribespells till level X
void command_scribespells(Client *c, const Seperator *sep);
// target can fly
void command_flymode(Client *c, const Seperator *sep);
// remove all npcs in the zone
void command_depopzone(Client *c, const Seperator *sep);
// respawn all npcs in the zone
void command_repopzone(Client *c, const Seperator *sep);
// enable additional output to the client
void command_debug(Client *c, const Seperator *sep);
// changes the sky
void command_sky(Client *c, const Seperator *sep);
// changes the fog color (RGB format)
void command_fogcolor(Client *c, const Seperator *sep);
// changes the zone clip
void command_zoneclip(Client *c, const Seperator *sep);
// grabs intended zone appearance info from database
void command_zoneappearance(Client *c, const Seperator *sep);