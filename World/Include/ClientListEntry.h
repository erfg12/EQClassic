// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef CLIENTLISTENTRY_H
#define CLIENTLISTENTRY_H

#include "linked_list.h"
#include "timer.h"
#include "queue.h"
#include "servertalk.h"
#include "eq_packet_structs.h"
#include "TCPConnection.h"
#include "console.h"
#include "Mutex.h"

class ClientListEntry
{
public:
	ClientListEntry(ZoneServer* zoneserver, char* in_name, int32 in_accountid, char* in_accountname, int8 in_admin, char* in_zone, int8 level, int8 class_, int8 race, int8 anon, bool tellsoff, int32 in_guilddbid, int32 in_guildeqid, bool in_LFG) {
		pzoneserver = zoneserver;
		strcpy(pname, in_name);
		paccountid = in_accountid;
		strcpy(paccountname, in_accountname);
		padmin = in_admin;
		strcpy(pzone, in_zone);
		plevel = level;
		pclass_ = class_;
		prace = race;
		panon = anon;
		ptellsoff = tellsoff;
		pguilddbid = in_guilddbid;
		pguildeqid = in_guildeqid;
		pLFG = in_LFG;
		stale = 0;
		timeout_timer = new Timer(INTERSERVER_TIMER);
	}
	~ClientListEntry() 
	{
		safe_delete(timeout_timer);//delete timeout_timer;
	}
	bool CheckStale()
	{
		if (timeout_timer->Check())
		{
			if (stale >= 2)
			{
				return true;
			}
			stale++;
			return false;
		}
		return false;
	}
	void Update(ZoneServer* zoneserver, char* in_name, int32 in_accountid, char* in_accountname, int8 in_admin, char* in_zone, int8 level, int8 class_, int8 race, int8 anon, bool tellsoff, int32 in_guilddbid, int32 in_guildeqid, bool in_LFG)
	{
		pzoneserver = zoneserver;
		strcpy(pname, in_name);
		paccountid = in_accountid;
		strcpy(paccountname, in_accountname);
		padmin = in_admin;
		strcpy(pzone, in_zone);
		plevel = level;
		pclass_ = class_;
		prace = race;
		panon = anon;
		ptellsoff = tellsoff;
		pguilddbid = in_guilddbid;
		pguildeqid = in_guildeqid;
		pLFG = in_LFG;
		stale = 0;
	}

	ZoneServer* Server()
	{
		return pzoneserver;
	}

	char* name()
	{
		return pname;
	}

	char* zone()
	{
		return pzone;
	}

	char* AccountName()
	{
		return paccountname;
	}

	int32 AccountID()
	{
		return paccountid;
	}

	int8 Admin()
	{
		return padmin;
	}

	int8 level()
	{
		return plevel;
	}

	int8 class_()
	{
		return pclass_;
	}

	int8 race()
	{
		return prace;
	}

	int8 Anon()
	{
		return panon;
	}

	int8 TellsOff()
	{
		return ptellsoff;
	}

	int32 GuildDBID()
	{
		return pguilddbid;
	}

	int32 GuildEQID()
	{
		return pguildeqid;
	}

	bool LFG()
	{
		return pLFG;
	}

private:
	ZoneServer* pzoneserver;
	char pzone[30];
	int8 padmin;
	char pname[30];
	int32 paccountid;
	char paccountname[30];
	int8 plevel;
	int8 pclass_;
	int8 prace;
	int8 panon;
	int8 ptellsoff;
	int32 pguilddbid;
	int32 pguildeqid;
	bool pLFG;

	int8 stale;

	Timer* timeout_timer;
};

#endif