/* 
Harakiri Sept 04. 2009

Wraps a perl interpreter for in c.

The basic mapping is the questmgr.cpp, this is the old implementation of EQEMU.
Each function is strcmp in EmbParser ExCommands, and then questManager->function() is called.
The mapping has been done manually for each method in embparser map_func

The new way is to use real classes (PerlXS), the complete object can be mapped with all functions
to perl. As an example consider perlpacket.cpp.

a) perlpacket.cpp is the c implementation with all functions
b) perl_perlpacket.cpp is the mapping class to map all c functions to perl
c) to make these functions available in perl the utility \util\perlxs is used
   there is a header file for each c++ class which contains the functions which should be mapped to perl   
d) xs_init in this file is mapping the boot_PerlPacket so that the mapping is called in perl
	   newXS(strcpy(buf, "PerlPacket::boot_PerlPacket"), boot_PerlPacket, file);
e) in embparser map_funs the class is loaded
   perl->eval(
	"{"
	"package PerlPacket;"
	"&boot_PerlPacket;"		//load our PerlPacket XS
		"}"
	);   

The code was ported over from EQEMU 7.0
*/

#ifndef EMBPERL_CPP
#define EMBPERL_CPP

#ifdef EMBPERL

#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <vector>
#include "embperl.h"
//#include "embxs.h"
#include "features.h"
#include "EQCUtils.hpp"

#ifdef WIN32
#pragma comment(lib, "perl512.lib")
#endif

// Harakiri map commands
#ifdef EMBPERL_COMMANDS
EXTERN_C XS(boot_PerlPacket);
XS(XS_command_add);
#endif

// Harakiri map client obj
EXTERN_C XS(boot_Client);
// Harakiri map mob obj
EXTERN_C XS(boot_Mob);
EXTERN_C XS(boot_NPC);
EXTERN_C XS(boot_EntityList);

#ifdef EMBPERL_XS

#ifdef EMBPERL_XS_CLASSES
EXTERN_C XS(boot_Corpse);
EXTERN_C XS(boot_quest);
EXTERN_C XS(boot_Group);
/*EXTERN_C XS(boot_Raid);
EXTERN_C XS(boot_QuestItem);
EXTERN_C XS(boot_PerlPacket);*/
XS(XS_NPC_new);
XS(XS_EntityList_new);
#endif
#endif

#ifdef EMBPERL_IO_CAPTURE
XS(XS_EQEmuIO_PRINT);
#endif //EMBPERL_IO_CAPTURE

//so embedded scripts can use xs extensions (ala 'use socket;')
EXTERN_C void boot_DynaLoader(pTHX_ CV* cv);
EXTERN_C void xs_init(pTHX)
{
	char file[256];
	strncpy(file, __FILE__, 256);
	file[255] = '\0';

	char buf[128];	//shouldent have any function names longer than this.

	//add the strcpy stuff to get rid of const warnings....

	newXS(strcpy(buf, "DynaLoader::boot_DynaLoader"), boot_DynaLoader, file);
	newXS(strcpy(buf, "Client::boot_Client"), boot_Client, file);
	newXS(strcpy(buf, "Client::boot_Mob"), boot_Mob, file);
	newXS(strcpy(buf, "Mob::boot_Mob"), boot_Mob, file);	
	newXS(strcpy(buf, "NPC::boot_Mob"), boot_Mob, file);
	newXS(strcpy(buf, "NPC::boot_NPC"), boot_NPC, file);
	newXS(strcpy(buf, "EntityList::boot_EntityList"), boot_EntityList, file);
	


	#ifdef EMBPERL_COMMANDS
		newXS(strcpy(buf, "PerlPacket::boot_PerlPacket"), boot_PerlPacket, file);
		newXS(strcpy(buf, "commands::command_add"), XS_command_add, file);
	#endif
	//newXS(strcpy(buf, "quest::boot_qc"), boot_qc, file);
#ifdef EMBPERL_XS	
#ifdef EMBPERL_XS_CLASSES	
	newXS(strcpy(buf, "quest::boot_quest"), boot_QuestManager, file);
//	newXS(strcpy(buf, "EntityList::new"), XS_EntityList_new, file);
	newXS(strcpy(buf, "Group::boot_Group"), boot_Group, file);
//	newXS(strcpy(buf, "Raid::boot_Raid"), boot_Raid, file);
//	newXS(strcpy(buf, "QuestItem::boot_QuestItem"), boot_QuestItem, file);
#endif
#endif
#ifdef EMBPERL_IO_CAPTURE
	newXS(strcpy(buf, "EQEmuIO::PRINT"), XS_EQEmuIO_PRINT, file);
#endif
}

Embperl::Embperl()
{
	in_use = true;	//in case one of these files generates an event

	//setup perl...
	my_perl = perl_alloc();
	if(!my_perl)
		throw "Failed to init Perl (perl_alloc)";
	DoInit();
}

void Embperl::DoInit() {
	const char *argv_eqemu[] = { "",
#ifdef EMBPERL_IO_CAPTURE
		"-w", "-W",
#endif
		"-e", "0;", NULL };

	int argc = 3;
#ifdef EMBPERL_IO_CAPTURE
	argc = 5;
#endif

	char **argv = (char **)argv_eqemu;
	char **env = { NULL };

	PL_perl_destruct_level = 1;

	perl_construct(my_perl);

	PERL_SYS_INIT3(&argc, &argv, &env);

	perl_parse(my_perl, xs_init, argc, argv, env);

	perl_run(my_perl);

	//a little routine we use a lot.
	eval_pv("sub my_eval {eval $_[0];}", TRUE);	//dies on error

	//ruin the perl exit and command:
	eval_pv("sub my_exit {}",TRUE);
	eval_pv("sub my_sleep {}",TRUE);
	if(gv_stashpv("CORE::GLOBAL", FALSE)) {
		GV *exitgp = gv_fetchpv("CORE::GLOBAL::exit", TRUE, SVt_PVCV);
		//#if _MSC_VER >= 1600
		//GvCV_set(exitgp, perl_get_cv("my_exit", TRUE));	//dies on error
		//#else
		GvCV(exitgp) = perl_get_cv("my_exit", TRUE);	//dies on error
		//#endif	//dies on error
		GvIMPORTED_CV_on(exitgp);
		GV *sleepgp = gv_fetchpv("CORE::GLOBAL::sleep", TRUE, SVt_PVCV);
		//#if _MSC_VER >= 1600 
		//GvCV_set(sleepgp, perl_get_cv("my_sleep", TRUE));	//dies on error
		//#else
		GvCV(sleepgp) = perl_get_cv("my_sleep", TRUE);	//dies on error
		//#endif
		GvIMPORTED_CV_on(sleepgp);
	}

	//declare our file eval routine.
	try {
		init_eval_file();
	}
	catch(const char *err)
	{
		//remember... lasterr() is no good if we crap out here, in construction
		EQC::Common::Log(EQCLog::Error,CP_QUESTS, "perl error: %s", err);
		throw "failed to install eval_file hook";
	}

#ifdef EMBPERL_IO_CAPTURE
EQC::Common::Log(EQCLog::Debug,CP_QUESTS, "Tying perl output to eqemu logs");
	//make a tieable class to capture IO and pass it into EQEMuLog
	eval_pv(
		"package EQEmuIO; "
//			"&boot_EQEmuIO;"
 			"sub TIEHANDLE { my $me = bless {}, $_[0]; $me->PRINT('Creating '.$me); return($me); } "
  			"sub WRITE {  } "
  			//dunno why I need to shift off fmt here, but it dosent like without it
  			"sub PRINTF { my $me = shift; my $fmt = shift; $me->PRINT(sprintf($fmt, @_)); } "
  			"sub CLOSE { my $me = shift; $me->PRINT('Closing '.$me); } "
  			"sub DESTROY { my $me = shift; $me->PRINT('Destroying '.$me); } "
//this ties us for all packages, just do it in quest since thats kinda 'our' package
  		"package quest;"
  		"	if(tied *STDOUT) { untie(*STDOUT); }"
  		"	if(tied *STDERR) { untie(*STDERR); }"
  		"	tie *STDOUT, 'EQEmuIO';"
  		"	tie *STDERR, 'EQEmuIO';"
  		,FALSE);
#endif //EMBPERL_IO_CAPTURE

#ifdef EMBPERL_PLUGIN
	eval_pv(
		"package plugin; "
		,FALSE
	);
#ifdef EMBPERL_EVAL_COMMANDS
	try {
		eval_pv(
			"use IO::Scalar;"
			"$plugin::printbuff='';"
			"tie *PLUGIN,'IO::Scalar',\\$plugin::printbuff;"
		,FALSE);
	}
	catch(const char *err) {
		throw "failed to install plugin printhook, do you lack IO::Scalar?";
	}
#endif

	EQC::Common::Log(EQCLog::Status,CP_QUESTS, "Loading perlemb plugins.");
	try
	{
		eval_pv("main::eval_file('plugin', 'plugin.pl');", FALSE);
	}
	catch(const char *err)
	{
		EQC::Common::Log(EQCLog::Status,CP_QUESTS, "Warning - plugin.pl: %s", err);
	}

	// Harakiri, this reads all the plugins in \plugins like 
	// check_handin.pl
	// check_hasitem.pl
	try
	{
		//should probably read the directory in c, instead, so that
		//I can echo filenames as I do it, but c'mon... I'm lazy and this 1 line reads in all the plugins
		eval_pv(
			"if(opendir(D,'plugins')) { "
			"	my @d = readdir(D);"
			"	closedir(D);"
			"	foreach(@d){ "
			"		main::eval_file('plugin','plugins/'.$_)if/\\.pl$/;"
			"	}"
			"}"
		,FALSE);
	}
	catch(const char *err)
	{
		EQC::Common::Log(EQCLog::Status,CP_QUESTS,  "Perl warning while loading plugins : %s", err);
	}

	// Harakiri, this reads all the plugins in quest\plugins like 
	try
	{
		//should probably read the directory in c, instead, so that
		//I can echo filenames as I do it, but c'mon... I'm lazy and this 1 line reads in all the plugins
		eval_pv(
			"if(opendir(D,'quests/plugins/')) { "
			"	my @d = readdir(D);"
			"	closedir(D);"
			"	foreach(@d){ "
			"		main::eval_file('plugin','quests/plugins/'.$_)if/\\.pl$/;"
			"	}"
			"}"
		,FALSE);
	}
	catch(const char *err)
	{
		EQC::Common::Log(EQCLog::Status,CP_QUESTS,  "Perl warning while loading plugins : %s", err);
	}
#endif //EMBPERL_PLUGIN

	//Harakiri these are used to create perl bases #commands
#ifdef EMBPERL_COMMANDS
	EQC::Common::Log(EQCLog::Normal,CP_QUESTS, "Loading perl commands...");
	try
	{
		eval_pv(
			"package commands;"
			"main::eval_file('commands', 'commands.pl');"
			"&commands::commands_init();"
		, FALSE);
	}
	catch(const char *err)
	{
		EQC::Common::Log(EQCLog::Error,CP_ZONESERVER, "Warning - commands.pl: %s", err);
	}
	EQC::Common::Log(EQCLog::Normal,CP_QUESTS, "Perl commands loaded....");
#endif //EMBPERL_COMMANDS

	in_use = false;
}

Embperl::~Embperl()
{
	in_use = true;
#ifdef EMBPERL_IO_CAPTURE
	//removed to try to stop perl from exploding on reload, we'll see
/*	eval_pv(
		"package quest;"
		"	untie *STDOUT;"
		"	untie *STDERR;"
  	,FALSE);
*/
#endif
//I am commenting this out in the veign hope that it will help with crashes
//under the assumption that we are not leaking a ton of memory and that this
//will not be a regular part of a production server's activity, only when debugging
	perl_free(my_perl);
}

void Embperl::Reinit() {
	in_use = true;
	PL_perl_destruct_level = 1;
	perl_destruct(my_perl);
	DoInit();
	in_use = false;
}

void Embperl::init_eval_file(void)
{
	eval_pv(
		"our %Cache;"
		"use Symbol qw(delete_package);"
		"sub eval_file {"
			"my($package, $filename) = @_;"
			"$filename=~s/\'//g;"
			"if(! -r $filename) { print \"Unable to read perl file '$filename'\\n\"; return; }"
			"my $mtime = -M $filename;"
			"if(defined $Cache{$package}{mtime}&&$Cache{$package}{mtime} <= $mtime && !($package eq 'plugin')){"
			"	return;"
			"} else {"
			//we 'my' $filename,$mtime,$package,$sub to prevent them from changing our state up here.
			"	eval(\"package $package; my(\\$filename,\\$mtime,\\$package,\\$sub); \\$isloaded = 1; require '$filename'; \");"
/*				"local *FH;open FH, $filename or die \"open '$filename' $!\";"
				"local($/) = undef;my $sub = <FH>;close FH;"
				"my $eval = qq{package $package; sub handler { $sub; }};"
				"{ my($filename,$mtime,$package,$sub); eval $eval; }"
				"die $@ if $@;"
				"$Cache{$package}{mtime} = $mtime; ${$package.'::isloaded'} = 1;}"
*/
			"}"
		"}"
		,FALSE);
 }

void Embperl::eval_file(const char * packagename, const char * filename)
{
	std::vector<std::string> args;
	args.push_back(packagename);
	args.push_back(filename);
	dosub("eval_file", &args);
}

void Embperl::dosub(const char * subname, const std::vector<std::string> * args, int mode)
{//as seen in perlembed docs
#if EQDEBUG >= 5
	if(InUse()) {
		LogFile->write(EQCLog::Debug, "Warning: Perl dosub called for %s when perl is allready in use.\n", subname);
	}
#endif
	in_use = true;
	bool err = false;
	try {
		SV **sp = PL_stack_sp;
	       /* initialize stack pointer      */
	} catch(const char *err)
			{//this should never happen, so if it does, it is something really serious (like a bad perl install), so we'll shutdown.
				EQC::Common::Log(EQCLog::Error,CP_ZONESERVER, "Fatal error initializing perl: %s", err);
				
			}

	dSP;                     
	ENTER;                          /* everything created after here */
	SAVETMPS;                       /* ...is a temporary variable.   */
	PUSHMARK(SP);                   /* remember the stack pointer    */
	if(args && args->size())
	{
		for(std::vector<std::string>::const_iterator i = args->begin(); i != args->end(); ++i)
		{/* push the arguments onto the perl stack  */
			XPUSHs(sv_2mortal(newSVpv(i->c_str(), i->length())));
		}
	}
	PUTBACK;                      /* make local stack pointer global */
	int result = call_pv(subname, mode); /*eval our code*/
	SPAGAIN;                        /* refresh stack pointer         */
	if(SvTRUE(ERRSV))
	{
		err = true;
	}
	FREETMPS;                       /* free temp values        */
	LEAVE;                       /* ...and the XPUSHed "mortal" args.*/
	
	in_use = false;
	if(err)
	{
		errmsg = "Perl runtime error: ";
		errmsg += SvPVX(ERRSV);
		throw errmsg.c_str();
	}
}

//evaluate an expression. throw error on fail
void Embperl::eval(const char * code)
{
	std::vector<std::string> arg;
	arg.push_back(code);
// MYRA - added EVAL & KEEPERR to eval per Eglin's recommendation
	// Freezo - G_KEEPERR prevents ERRSV from being snt. Removing it
	dosub("my_eval", &arg, G_SCALAR | G_DISCARD | G_EVAL | G_KEEPERR);
//end Myra
}

bool Embperl::SubExists(const char *package, const char *sub) {
	HV *stash = gv_stashpv(package, false);
	if(!stash)
		return(false);
	int len = strlen(sub);
	return(hv_exists(stash, sub, len));
}

bool Embperl::VarExists(const char *package, const char *var) {
	HV *stash = gv_stashpv(package, false);
	if(!stash)
		return(false);
	int len = strlen(var);
	return(hv_exists(stash, var, len));
}


#endif //EMBPERL

#endif //EMBPERL_CPP
