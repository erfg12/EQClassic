///////////////////////////////////////////////////
// Written by Cofruben: 17/08/08.
///////////////////////////////////////////////////
#ifndef DEBUG_HPP
#define DEBUG_HPP
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include "EQCUtils.hpp"

using namespace std;
class Client;

///////////////////////////////////////////////////
#pragma pack(1)

struct TDebugList		// Cofruben: added 17/08/08.
{
	bool log_file;
	bool debug_spells;
	bool debug_attack;		// Damage, DS, hit chance, ...
	bool debug_faction;
	bool debug_updates;		// HP & mana regen, exp.
	bool debug_merchants;
	bool debug_items;
	bool debug_pathing;
	bool debug_rest[7];	// Extra space if we want to debug more stuff. Keep the struct order and size!
};

///////////////////////////////////////////////////

class DebugSystem {
public:
	DebugSystem(Client* c);
	~DebugSystem();

	void	Log(CodePlace cp, char* text, ...);
	void	SetLogging(bool log);
	void	SetDebuggingSpells (bool spells  = true);
	void	SetDebuggingAttack (bool attack  = true);
	void	SetDebuggingFaction(bool faction = true);
	void	SetDebuggingUpdates(bool updates = true);
	void	SetDebuggingMerchants(bool merchants = true);
	void	SetDebuggingItems(bool items = true);
	void	SetDebuggingPathing(bool items = true);

private:
	bool		OpenFile();
	void		CloseFile();
	bool		IsFileOpened();
	bool		IsLogging() { return this->mDebugList.log_file; }
	void		SaveList();

	fstream		mFile;
	Client*		mpClient;
	TDebugList	mDebugList;
};

///////////////////////////////////////////////////


#pragma pack()
#endif

///////////////////////////////////////////////////