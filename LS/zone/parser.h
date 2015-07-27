#ifndef PARSER_H
#define PARSER_H

#include "../common/Kaiyodo-LList.h"
//#include "../common/linked_list.h"

#define Parser_MaxVars	1024

struct Events {
	int index;
	int npcid;
	char event[100][100];
	int line[65535];
	char command[100][2048];
};

struct Alias {
	int index;
	int npcid;
	char name[100][100];
	char command[100][1024];
};

struct vars {
	char name[Parser_MaxVars];
	char value[Parser_MaxVars];
};

struct command_list {
	char command_name[100];
	int param_amount[17];
};


class Parser
{
public:
	Parser();
	~Parser();

	MyList <Events> EventList;
	MyList <vars> varlist;
	MyList <Alias> AliasList;
	command_list* commands;
	char * ParseVars(char * string, int32 npcid, Mob* mob);
	void HandleVars(char * varname, char * varparms, char * origstring, char * format, int32 npcid, Mob* mob);
	void AddVar(const char *varname, const char * varval);
	char* GetVar(char * varname, int32 npcid);
	int ParseIt(char * info, int32 npcid, Mob* mob, Client* client);
	void Replace(char* string, const char * repstr, const char * rep, int all=0);
	void LoadCommands(const char * filename);
	void LoadScript(int npcid, const char * zone);
	void scanformat(char *string, const char *format, char arg[10][1024]);
	void GetCommandName(char * command1, char * arg);
	bool EnumerateVars(char * string, int32 npcid, Mob* mob);
	int ParseIf(char * string);
	void	DeleteVar(const char *name);
	int		GetItemCount(char * itemid, int32 npcid);
	void	DelChatAndItemVars(int32 npcid);
	int numtok(char * sting, const char *tok);
	char * strrstr(char* string, const char * sub);
	void MakeVars(const char * string, int32 npcid);
	void MakeParms(const char * string, int32 npcid);
	void Event(int event, int32 npcid, const char * data, Mob* npcmob, Mob* mob);
	void SendCommands(const char * event, int32 npcid, Mob* npcmob, Mob* mob);
	int CheckAliases(const char * alias, int32 npcid, Mob* npcmob, Mob* mob);
	void CommandEx(char * command, int32 npcid, Mob* other, Mob* mob);
	int pcalc(char * string);

	void	ClearEventsByNPCID(int32 iNPCID);
	void	ClearAliasesByNPCID(int32 iNPCID);
	bool	SetNPCqstID(int32 iNPCID, int32 iValue);
	bool	LoadAttempted(int32 iNPCID);
	void	ClearCache();
	int32	GetNPCqstID(int32 iNPCID);
private:

	int32	pMaxNPCID;
	int32*	pNPCqstID;
};

extern Parser* parse;

#endif

