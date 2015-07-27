#ifndef FEATURES_H
#define FEATURES_H

#ifdef EMBPERL

// Harakiri enable perl plugins i.e. checkHandin from \quests\plugins
#define EMBPERL_PLUGIN
//Enable the new XS based perl parser
//#define EMBPERL_XS

//enable classes in the new XS based parser
//#define EMBPERL_XS_CLASSES

//enable IO capture and transmission to in game clients
//this seems to make perl very unhappy on reload, and crashes
//#define EMBPERL_IO_CAPTURE

//enable perl-based in-game command, pretty useless without EMBPERL_XS_CLASSES
#define EMBPERL_COMMANDS

//enable #plugin and #peval, which requires IO::Stringy
//#define EMBPERL_EVAL_COMMANDS
#endif

//Uncomment this line to enable named quest files:

#define QUEST_SCRIPTS_BYNAME

#ifdef QUEST_SCRIPTS_BYNAME
//extends byname system to look in a templates directory
//independant of zone name
#define QUEST_TEMPLATES_BYNAME
#define QUEST_TEMPLATES_DIRECTORY "templates"

#endif

#endif
