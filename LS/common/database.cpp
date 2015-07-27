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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errmsg.h>
#include <mysqld_error.h>
#include <limits.h>
#include <ctype.h>

using namespace std;
// Disgrace: for windows compile
#ifdef WIN32
#include <windows.h>
#define snprintf	_snprintf
#define strncasecmp	_strnicmp
#define strcasecmp	_stricmp
#else
#include "unix.h"
#endif

#include "database.h"
#include "EQNetwork.h"
#include "packet_functions.h"
#include "eq_opcodes.h"
#include "../common/classes.h"
#include "../common/races.h"
#include "../common/files.h"
#include "../common/EQEMuError.h"
#ifdef SHAREMEM
#include "../common/EMuShareMem.h"
extern LoadEMuShareMemDLL EMuShareMemDLL;
#endif
#ifdef ZONESERVER
#include "../zone/zone.h"
#include "../zone/entity.h"
extern EntityList entity_list;
#endif
// change starting zone
const char* ZONE_NAME = "qeynos";

extern Database database;
#define buguploadhost "mysql.eqemu.net"
#define buguploaduser "eqemu_bug"
#define buguploadpassword "bugssuck"
#define buguploaddatabase "eqemubug"

/*
This is the amount of time in seconds the client has to enter the zone
server after the world server, or inbetween zones when that is finished
*/

/*
Establish a connection to a mysql database with the supplied parameters

  Added a very simple .ini file parser - Bounce
  
	Modify to use for win32 & linux - misanthropicfiend
*/
Database::Database ()
{
	InitVars();
	char host[200], user[200], passwd[200], database[200], buf[200], type[200];
	bool compression = false;
	int items[4] = {0, 0, 0, 0};
	FILE *f;
	
	if (!(f = fopen (DB_INI_FILE, "r")))
	{
		printf ("Couldn't open '%s'.\n", DB_INI_FILE);
		printf ("Read README.TXT!\n");
		exit (1);
	}
	
	do
	{
		fgets (buf, 199, f);
		if (feof (f))
		{
			printf ("[Database] block not found in DB.INI.\n");
			printf ("Read README.TXT!\n");
			exit (1);
		}
	}
	while (strncasecmp (buf, "[Database]\n", 11) != 0 && strncasecmp (buf, "[Database]\r\n", 12) != 0);
	
	while (!feof (f))
	{
#ifdef WIN32
		if (fscanf (f, "%[^=]=%[^\n]\n", type, buf) == 2)
#else	
			if (fscanf (f, "%[^=]=%[^\r\n]\n", type, buf) == 2)
#endif
			{
				if (!strncasecmp (type, "host", 4))
				{
					strncpy (host, buf, 199);
					items[0] = 1;
				}
				if (!strncasecmp (type, "user", 4))
				{
					strncpy (user, buf, 199);
					items[1] = 1;
				}
				if (!strncasecmp (type, "pass", 4))
				{
					strncpy (passwd, buf, 199);
					items[2] = 1;
				}
				if (!strncasecmp (type, "data", 4))
				{
					strncpy (database, buf, 199);
					items[3] = 1;
				}
				if (!strncasecmp (type, "compression", 11)) {
					if (strcasecmp(buf, "on") == 0) {
						compression = true;
						cout << "DB Compression on." << endl;
					}
					else if (strcasecmp(buf, "off") != 0)
						cout << "Unknown 'compression' setting in db.ini. Expected 'on' or 'off'." << endl;
				}
			}
	}
	
	if (!items[0] || !items[1] || !items[2] || !items[3])
	{
		printf ("Incomplete DB.INI file.\n");
		printf ("Read README.TXT!\n");
		exit (1);
	}
	
	fclose (f);
	int32 errnum = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	if (!Open(host, user, passwd, database, &errnum, errbuf)) {
		cerr << "Failed to connect to database: Error: " << errbuf << endl;
		HandleMysqlError(errnum);
	}
	else
	{
		cout << "Using database '" << database << "' at " << host << endl;
	}
}
/*
Establish a connection to a mysql database with the supplied parameters
*/
Database::Database(const char* host, const char* user, const char* passwd, const char* database)
{
	InitVars();
	int32 errnum= 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	if (!Open(host, user, passwd, database, &errnum, errbuf))
	{
		cerr << "Failed to connect to database: Error: " << errbuf << endl;
		HandleMysqlError(errnum);
	}
	else
	{
		cout << "Using database '" << database << "' at " << host << endl;
	}
}

void Database::HandleMysqlError(int32 errnum) {
	switch(errnum) {
		case 0:
			break;
		case 1045: // Access Denied
		case 2001: {
			printf("Access to the mysql server was denied.\n");
			//AddEQEMuError(EQEMuError_Mysql_1405, true);
			break;
		}
		case 2003: { // Unable to connect
			printf("Could not connect to mysql server.\n");
			//AddEQEMuError(EQEMuError_Mysql_2003, true);
			break;
		}
		case 2005: { // Unable to connect
			printf("Could not connect to mysql server.\n");
			//AddEQEMuError(EQEMuError_Mysql_2005, true);
			break;
		}
		case 2007: { // Unable to connect
			printf("Could not connect to mysql server.\n");
			//AddEQEMuError(EQEMuError_Mysql_2007, true);
			break;
		}
	}
}

void Database::InitVars() {
#ifndef SHAREMEM
	item_array = 0;
	npc_type_array = 0;
    door_array = 0;
	loottable_array = 0;
	loottable_inmem = 0;
	lootdrop_array = 0;
	lootdrop_inmem = 0;
#endif
	memset(item_minstatus, 0, sizeof(item_minstatus));
	memset(door_isopen_array, 0, sizeof(door_isopen_array));
	max_item = 0;
	max_npc_type = 0;
	max_faction = 0;
	faction_array = 0;
	max_zonename = 0;
	zonename_array = 0;





	loottable_max = 0;
	lootdrop_max = 0;
	max_door_type = 0;
	npc_spells_maxid = 0;
	npc_spells_cache = 0;
	npc_spells_loadtried = 0;
	npcfactionlist_max = 0;
	varcache_array = 0;
	varcache_max = 0;
	varcache_lastupdate = 0;
}

bool Database::logevents(char* accountname,int32 accountid,int8 status,const char* charname, const char* target,char* descriptiontype, char* description,int event_nid){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "Insert into eventlog (accountname,accountid,status,charname,target,descriptiontype,description,event_nid) values('%s',%i,%i,'%s','%s','%s','%s','%i')", accountname,accountid,status,charname,target,descriptiontype,description,event_nid), errbuf))	{
		cerr << "Error in logevents" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	delete[] query;
	return true;
}

sint16 Database::CommandRequirement(const char* commandname) {
	for(int i=0; i<maxcommandlevel; i++)
	{
		if((strcasecmp(commandname, commands[i]) == 0)) {
			return commandslevels[i];
		}
	}
	return 255;
}

void Database::ExtraOptions()
{
	FILE* f;
	int open = 1;
	char buf[200];
	char type[200];
	maxcommandlevel = 0;
	if (!(f = fopen (ADDON_INI_FILE, "r")))
	{
		printf ("Couldn't open '%s'.\n", ADDON_INI_FILE);
		return;
	}
	do
	{
		fgets (buf, 199, f);
		if (feof (f))
		{
			printf ("[CommandLevels] block not found in ADDON.INI.\n");
			return;
		}
	}
	
	while (strncasecmp (buf, "[CommandLevels]\n", 11) != 0 && strncasecmp (buf, "[CommandLevels]\r\n", 12) != 0 && open == 1);
	
	while (!feof (f) && open == 1)
	{
#ifdef WIN32
		if (fscanf (f, "%[^=]=%[^\n]\n", type, buf) == 2)
#else	
			if (fscanf (f, "%[^=]=%[^\r\n]\n", type, buf) == 2)
#endif
			{
				if(sizeof(type) > 0 && maxcommandlevel < 200) {
					snprintf(commands[maxcommandlevel], 200, "%s", type);
					commandslevels[maxcommandlevel] = atoi(buf);
					maxcommandlevel++;
				}
			}
	}
	
	fclose (f);
}

/*

Close the connection to the database
*/
Database::~Database()
{
	unsigned int x;
#ifndef SHAREMEM
	if (item_array != 0) {
		for (x=0; x <= max_item; x++) {
			if (item_array[x] != 0)
				delete item_array[x];
		}
		delete [] item_array;
	}
	if (npc_type_array != 0) {
		for (x=0; x <= max_npc_type; x++) {
			if (npc_type_array[x] != 0)
				delete npc_type_array[x];
		}
		delete [] npc_type_array; // neotokyo: fix
	}
	if (door_array != 0) {
		for (x=0; x <= max_door_type; x++) {
			if (door_array[x] != 0)
				delete door_array[x];
		}
		delete [] door_array;
	}
	if (loottable_array) {
		for (x=0; x<=loottable_max; x++) {
			safe_delete(loottable_array[x]);
		}
		safe_delete(loottable_array);
	}
	safe_delete(loottable_inmem);
	if (lootdrop_array) {
		for (x=0; x<=lootdrop_max; x++) {
			safe_delete(lootdrop_array[x]);
		}
		safe_delete(lootdrop_array);
	}
	safe_delete(lootdrop_inmem);
#endif
	if (faction_array != 0) {
		for (x=0; x <= max_faction; x++) {
			if (faction_array[x] != 0)
				delete faction_array[x];
		}
		delete [] faction_array;
	}
	if (zonename_array) {
		for (x=0; x<=max_zonename; x++) {
			if (zonename_array[x])
				safe_delete(zonename_array[x]);
		}
		safe_delete(zonename_array);
	}
	if (npc_spells_cache) {
		for (x=0; x<=npc_spells_maxid; x++) {
			safe_delete(npc_spells_cache[x]);
		}
		safe_delete(npc_spells_cache);
	}
	safe_delete(npc_spells_loadtried);
	if (varcache_array) {
		for (x=0; x<varcache_max; x++) {
			safe_delete(varcache_array[x]);
		}
		safe_delete(varcache_array);
	}
#ifdef GUILDWARS
	if(guild_alliances) {
		for (x=0; x<guildalliance_max; x++) {
			if (guild_alliances[x])
				safe_delete(guild_alliances[x]);
		}
		safe_delete(guild_alliances);
	}
#endif
}

/*
Check if the character with the name char_name from ip address ip has
permission to enter zone zone_name. Return the account_id if the client
has the right permissions, otherwise return zero.
Zero will also be returned if there is a database error.
*/
/*int32 Database::GetAuthentication(const char* char_name, const char* zone_name, int32 ip) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT account_id FROM authentication WHERE char_name='%s' AND zone_name='%s' AND ip=%u AND UNIX_TIMESTAMP()-UNIX_TIMESTAMP(time) < %i", char_name, zone_name, ip, AUTHENTICATION_TIMEOUT), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 account_id = atoi(row[0]);
			mysql_free_result(result);
			return account_id;
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAuthentication query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	
	return 0;
}*/

/*
Give the account with id "account_id" permission to enter zone "zone_name" from ip address "ip"
with character "char_name". Return true if successful.
False will be returned if there is a database error.
*/
/*bool Database::SetAuthentication(int32 account_id, const char* char_name, const char* zone_name, int32 ip) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM authentication WHERE account_id=%i", account_id), errbuf))
	{
		cerr << "Error in SetAuthentication query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO authentication SET account_id=%i, char_name='%s', zone_name='%s', ip=%u", account_id, char_name, zone_name, ip), errbuf, 0, &affected_rows))
	{
		cerr << "Error in SetAuthentication query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}*/

/*
This function will return the zone name in the "zone_name" parameter.
This is used when a character changes zone, the old zone server sets
the authentication record, the world server reads this new zone	name.
If there was a record return true, otherwise false.
False will also be returned if there is a database error.
*/
/*bool Database::GetAuthentication(int32 account_id, char* char_name, char* zone_name, int32 ip) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT char_name, zone_name FROM authentication WHERE account_id=%i AND ip=%u AND UNIX_TIMESTAMP()-UNIX_TIMESTAMP(time) < %i", account_id, ip, AUTHENTICATION_TIMEOUT), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			strcpy(char_name, row[0]);
			strcpy(zone_name, row[1]);
			mysql_free_result(result);
			return true;
		}
		else {
			mysql_free_result(result);
			return false;
		}




		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAuthentication query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}*/


/*
This function will remove the record in the authentication table for
the account with id "accout_id"
False will also be returned if there is a database error.
*/
/*bool Database::ClearAuthentication(int32 account_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM authentication WHERE account_id=%i", account_id), errbuf))
	{
		cerr << "Error in ClearAuthentication query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	delete[] query;
	
	return true;
}*/

/*
Check if there is an account with name "name" and password "password"
Return the account id or zero if no account matches.
Zero will also be returned if there is a database error.
*/
int32 Database::CheckLogin(const char* name, const char* password, sint16* oStatus) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	char tmpUN[100];
	char tmpPW[100];
	DoEscapeString(tmpUN, name, strlen(name));
	DoEscapeString(tmpPW, password, strlen(password));
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, status FROM account WHERE name='%s' AND (password='%s' or password=MD5('%s'))", tmpUN, tmpPW, tmpPW), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 id = atoi(row[0]);
			if (oStatus)
				*oStatus = atoi(row[1]);
			mysql_free_result(result);
			return id;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in CheckLogin query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return 0;
}

int8 Database::GetGMSpeed(int32 account_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT gmspeed FROM account where id='%i'", account_id), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int8 gmspeed = atoi(row[0]);
			mysql_free_result(result);
			return gmspeed;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else
	{
		
		cerr << "Error in GetGMSpeed query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return 0;
	

}

bool Database::SetGMSpeed(int32 account_id, int8 gmspeed)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET gmspeed = %i where id = %i", gmspeed, account_id), errbuf)) {
		cerr << "Error in SetGMSpeed query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	delete[] query;
	return true;
	
}

bool Database::GetAccountInfoForLogin(int32 account_id, sint16* admin, char* account_name, int32* lsaccountid, int8* gmspeed) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT status, name, lsaccount_id, gmspeed FROM account WHERE id=%i", account_id), errbuf, &result)) {
		delete[] query;
		bool ret = GetAccountInfoForLogin_result(result, admin, account_name, lsaccountid, gmspeed);
		mysql_free_result(result);
		return ret;
	}
	else
	{
		cerr << "Error in GetAccountInfoForLogin query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}

bool Database::GetAccountInfoForLogin_result(MYSQL_RES* result, sint16* admin, char* account_name, int32* lsaccountid, int8* gmspeed) {
    MYSQL_ROW row;
	if (mysql_num_rows(result) == 1) {
		row = mysql_fetch_row(result);
		if (admin)
			*admin = atoi(row[0]);
		if (account_name)
			strcpy(account_name, row[1]);
		if (lsaccountid) {

			if (row[2])
				*lsaccountid = atoi(row[2]);
			else
				*lsaccountid = 0;

		}
		if (gmspeed)
			*gmspeed = atoi(row[3]);
		return true;
	}
	else {
		return false;
	}
}

sint16 Database::CheckStatus(int32 account_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT status FROM account WHERE id='%i'", account_id), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			sint16 status = atoi(row[0]);

			mysql_free_result(result);
			return status;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in CheckStatus query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return 0;
}

int32 Database::CreateAccount(const char* name, const char* password, sint16 status, int32 lsaccount_id) {	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 querylen;
	int32 last_insert_id;
	
	if (password)
		querylen = MakeAnyLenString(&query, "INSERT INTO account SET name='%s', password='%s', status=%i, lsaccount_id=%i;",name,password,status, lsaccount_id);
	else
		querylen = MakeAnyLenString(&query, "INSERT INTO account SET name='%s', status=%i, lsaccount_id=%i;",name, status, lsaccount_id);
	
	cerr << "Account Attempting to be created:" << name << " " << (sint16) status << endl;
	if (!RunQuery(query, querylen, errbuf, 0, 0, &last_insert_id)) {
		cerr << "Error in CreateAccount query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	delete[] query;

	if (last_insert_id == 0) {
		cerr << "Error in CreateAccount query '" << query << "' " << errbuf << endl;
		return 0;
	}
	
	return last_insert_id;
}

bool Database::DeleteAccount(const char* name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	cerr << "Account Attempting to be deleted:" << name << endl;
	if (RunQuery(query, MakeAnyLenString(&query, "DELETE FROM account WHERE name='%s';",name), errbuf, 0, &affected_rows)) {
		delete[] query;
		if (affected_rows == 1) {
			return true;
		}
	}
	else {
		
		cerr << "Error in DeleteAccount query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return false;
}

bool Database::SetLocalPassword(int32 accid, const char* password) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET password=MD5('%s') where id=%i;", password, accid), errbuf)) {
		cerr << "Error in SetLocalPassword query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	delete[] query;
	return true;
}

bool Database::UpdateLiveChar(char* charname,int32 lsaccount_id) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET charname='%s' WHERE id=%i;",charname, lsaccount_id), errbuf)) {
		cerr << "Error in UpdateLiveChar query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	delete[] query;
	return true;
}

bool Database::GetLiveChar(int32 account_id, char* cname) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT charname FROM account WHERE id=%i", account_id), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			strcpy(cname,row[0]);
			mysql_free_result(result);
			return false;
		}
	}
	else {
		cerr << "Error in GetLiveChar query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return false;
}

bool Database::UpdateTempPacket(char* packet,int32 lsaccount_id) {
	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(PlayerProfile_Struct)*2+1];
	char* end = query;
	
	end += sprintf(end, "UPDATE account SET packencrypt=");
	*end++ = '\'';
    end += DoEscapeString(end, packet, strlen(packet));
    *end++ = '\'';
    end += sprintf(end," WHERE id=%i", lsaccount_id);
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in UpdateTempPacket query " << errbuf << endl;
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

bool Database::GetTempPacket(int32 lsaccount_id, char* packet)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT packencrypt FROM account WHERE id=%i", lsaccount_id), errbuf, &result)) {
		delete[] query;
		
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			memcpy(packet, row[0], strlen(row[0]));
			mysql_free_result(result);
			return true;
		}
	}
	else {
		delete[] query;
	}
	
	return false;
}
#ifdef GUILDWARS
#ifdef GUILDWARS2
void Database::SetCityAvailableFunds(int32 zone_id, sint32 funds) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE guild_controllers SET funds=%i WHERE zoneid=%i;",funds,zone_id), errbuf, 0, &affected_rows)) {
		delete[] query;
	}
	delete[] query;
}

sint32 Database::GetCityAvailableFunds(int32 zone_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if(zone_id == 0)
		return 0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT funds from guild_controllers where zoneid=%i", zone_id), errbuf, &result)) {
		delete[] query;

		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			sint32 funds = atoi(row[0]);
			mysql_free_result(result);
			return funds;
		}
	}
	else {
		delete[] query;
	}
	return 0;
}

#endif
bool Database::SetCityGuildOwned(int32 guildid,int32 npc_id) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE guild_controllers SET owned_guild_id=%i WHERE npc_id=%i;",guildid,npc_id), errbuf, 0, &affected_rows)) {
		delete[] query;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
}

void Database::LoadCityControl()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	Item_Struct tempitem;
	memset(cityownedbyguilds,0,sizeof(int32)*20);
	MakeAnyLenString(&query, "SELECT owned_guild_id from guild_controllers");
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		int i=0;
		delete[] query;
		while((row = mysql_fetch_row(result)))
		{
		i++;
		cityownedbyguilds[i] = atoi(row[0]);
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadCityControl '" << query << "' " << errbuf << endl;
		delete[] query;
		//return false;
	}
}

int8 Database::TotalCitiesOwned(int32 guild_id)
{
int8 total = 0;
for(int i=0;i<20;i++)
{
if(cityownedbyguilds[i] == guild_id)
total++;
}
return total;
}

int32 Database::GetCityGuildOwned(int32 npc_id,int32 zone_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if(zone_id > 0)
	{
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT owned_guild_id from guild_controllers where zoneid=%i", zone_id), errbuf, &result)) {
		delete[] query;
		
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 guild = atoi(row[0]);
			mysql_free_result(result);
			return guild;
		}
		else
			mysql_free_result(result);
	}
	else {
		delete[] query;
		return 0;
	}
	}
	else
	{
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT owned_guild_id from guild_controllers where npc_id=%i", npc_id), errbuf, &result)) {
		delete[] query;
		
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 guild = atoi(row[0]);

			mysql_free_result(result);
			return guild;
		}
		else
			mysql_free_result(result);
	}
	else {
		delete[] query;
	}
	}
	
	return false;
}

bool Database::SetGuildAlliance(int32 guildid,int32 allyguildid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	if(guildid == allyguildid)
		return false;

			if (RunQuery(query, MakeAnyLenString(&query, "Insert Into guild_alliances (guildone,guildtwo) values ('%i', '%i')", guildid,allyguildid), errbuf, 0, &affected_rows)) {
				safe_delete(query);
				if (affected_rows == 1) {
					return true;
		}
	}
	else {
		delete[] query;
	}
	return false;
}

bool Database::RemoveGuildAlliance(int32 guildid,int32 allyguildid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	if(guildid == allyguildid)
		return false;

			if (RunQuery(query, MakeAnyLenString(&query, "delete from guild_alliances where guildone=%i AND guildtwo=%i", guildid,allyguildid), errbuf, 0, &affected_rows)) {
				safe_delete(query);
				if (affected_rows == 1) {
					return true;
		}
	}
	else {
		delete[] query;
	}
	return false;
}

bool Database::GetGuildAlliance(int32 guild_id,int32 other_guild_id)
{
	if(guild_id == other_guild_id)
		return true;
	for(int i=0;i<guildalliance_max;i++)
	{
	if(guild_alliances[i] && guild_alliances[i]->guild_id == guild_id && guild_alliances[i]->other_guild_id == other_guild_id)
		return true;
}
	return false;
}

bool Database::LoadGuildAlliances() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int32 i;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT guildone,guildtwo from guild_alliances"), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) > 0) {
			if(guild_alliances)
			{
				for (int x=0; x<guildalliance_max; x++) {
					if (guild_alliances[x])
						safe_delete(guild_alliances[x]);
				}
				safe_delete(guild_alliances);
			}
			
			guildalliance_max = mysql_num_rows(result);
			guild_alliances = new GuildWar_Alliance*[guildalliance_max];
				for (i=0; i<guildalliance_max; i++)
					guild_alliances[i] = 0;
		}
		int i=0;
		while ((row = mysql_fetch_row(result))) {
			i++;
					guild_alliances[i] = new GuildWar_Alliance;
					guild_alliances[i]->guild_id = atoi(row[0]);
					guild_alliances[i]->other_guild_id = atoi(row[1]);
	}
		mysql_free_result(result);
		return true;
	}
	else {
		cerr << "Error in LoadGuildAlliances query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}

void Database::GetCityAreaName(int32 zone_id, char* areaname)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT terrainarea from guild_controllers where zoneid=%i", zone_id), errbuf, &result)) {
		delete[] query;
		
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
		memcpy(areaname, row[0], strlen(row[0]));
			mysql_free_result(result);
		}
		else
		mysql_free_result(result);
	}
	else {
		delete[] query;
	}
}
void Database::DeleteCityDefense(int32 zone_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    char *query2 = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

		if (RunQuery(query, MakeAnyLenString(&query, "SELECT spawngroupid from city_defense where zoneid=%i", zone_id), errbuf, &result)) {
			delete[] query;
			while ((row = mysql_fetch_row(result))) {
#ifdef ZONESERVER
			extern Zone* zone;
			zone->RemoveSpawnEntry(atoi(row[0]));
			zone->RemoveSpawnGroup(atoi(row[0]));
#endif
				int32 temp = atoi(row[0]);
				if(temp != 0)
				{
			if (!RunQuery(query2, MakeAnyLenString(&query2, "DELETE FROM spawn2 WHERE spawngroupID=%i", temp), errbuf,0)) {
				delete query2;
				return;
			}
			if (!RunQuery(query2, MakeAnyLenString(&query2, "DELETE FROM spawngroup WHERE id=%i", temp), errbuf,0)) {
				delete query2;
				return;
			}
			if (!RunQuery(query2, MakeAnyLenString(&query2, "DELETE FROM spawnentry WHERE spawngroupID=%i", temp), errbuf,0)) {
				delete query2;
				return;
			}
			if (!RunQuery(query2, MakeAnyLenString(&query2, "DELETE FROM city_defense WHERE zoneid=%i", zone_id), errbuf,0)) {
				delete query2;
				//return false;
			}
				}
			}
			delete[]query2;
				mysql_free_result(result);
		}

}


bool Database::UpdateCityDefenseFaction(int32 zone_id,int32 npc_id,int32 spawngroupid,int8 newstatus)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE city_defense SET factionstanding=%i WHERE zoneid=%i AND spawngroupid=%i and npcid=%i;", newstatus,zone_id,spawngroupid,npc_id), errbuf, 0, &affected_rows)) {
		delete[] query;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0) {
		return false;
	}

	return true;
}

int8 Database::GetGuardFactionStanding(int32 zone_id,int32 npc_id,int32 spawngroupid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
		if (RunQuery(query, MakeAnyLenString(&query, "SELECT factionstanding from city_defense where zoneid=%i AND npcid=%i AND spawngroupid=%i", zone_id,npc_id,spawngroupid), errbuf, &result)) {
			delete[] query;
			if (mysql_num_rows(result) == 1) {
				row = mysql_fetch_row(result);
				int8 temp = atoi(row[0]);
				mysql_free_result(result);

				switch(temp)
				{
				case 0:
					return 0; // Scowl
					break;
				case 1:
					return 1; // Neutral
					break;
				}

			}
			mysql_free_result(result);
		}
		else {
			safe_delete(query);
		}
return false;
}

bool Database::CityDefense(int32 zone_id,int32 npc_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if(npc_id > 0) {
		if (RunQuery(query, MakeAnyLenString(&query, "SELECT id from city_defense where zoneid=%i AND npcid=%i", zone_id,npc_id), errbuf, &result)) {
			delete[] query;
			if (mysql_num_rows(result) >= 1) {
				row = mysql_fetch_row(result);
				int32 temp = atoi(row[0]);
				mysql_free_result(result);

				if(temp > 0)
					return true;
				else
					return false;

				return false;
			}
			mysql_free_result(result);
		}
		else {
			safe_delete(query);
		}
	}
	else {	
		if (RunQuery(query, MakeAnyLenString(&query, "SELECT npcid from city_defense where zoneid=%i", zone_id), errbuf, &result)) {
			delete[] query;
			
			while ((row = mysql_fetch_row(result))) {
#ifdef ZONESERVER
				int32 npcid = atoi(row[0]);
				if(entity_list.FindDefenseNPC(npcid)) {
					mysql_free_result(result);
					return true;
				}
#endif					
				Sleep(0);
			}
			mysql_free_result(result);
		}
		else {
			delete[] query;
		}
	}
	
	return false;
}
#endif
bool Database::SetGMFlag(const char* name, sint16 status) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	
	cout << "Account being GM Flagged:" << name << ", Level: " << (sint16) status << endl;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET status=%i WHERE name='%s';", status, name), errbuf, 0, &affected_rows)) {
		delete[] query;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0) {
		cout << "Account: " << name << " does not exist, therefore it cannot be flagged\n";
		return false;
	}

	return true;
}

bool Database::SetSpecialAttkFlag(int8 id, const char* flag) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE npc_types SET npcspecialattks='%s' WHERE id=%i;",flag,id), errbuf, 0, &affected_rows)) {
		delete[] query;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

bool Database::DoorIsOpen(int8 door_id,const char* zone_name)
{
	if(door_isopen_array[door_id] == 0) {
		SetDoorPlace(1,door_id,zone_name);
		return false;
	}
	else {
		SetDoorPlace(0,door_id,zone_name);
		return true;
	}

	/*
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT doorisopen from doors where zone='%s' AND doorid=%i", zone_name,door_id), errbuf, &result)) {
		delete[] query;
		
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int8 open = atoi(row[0]);
			mysql_free_result(result);
			
			if(open == 0) {
				SetDoorPlace(1,door_id,zone_name);
				return false;
			}
			else {
				SetDoorPlace(0,door_id,zone_name);
				return true;
			}
		}
		else
			mysql_free_result(result);
	}
	else {
		delete[] query;
	}
	
	return false;*/
}

void Database::SetDoorPlace(int8 value,int8 door_id,const char* zone_name)
{
	door_isopen_array[door_id] = value;
/*	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	if(value == 99)
	{
	if (RunQuery(query, MakeAnyLenString(&query, "update doors set doorisopen=0 where zone='zone_name'",zone_name), errbuf, 0, &affected_rows))
		safe_delete(query);
	}
	else
	{
	if (RunQuery(query, MakeAnyLenString(&query, "update doors set doorisopen=%i where zone='%s' AND doorid=%i",value,zone_name,door_id), errbuf, 0, &affected_rows))
		safe_delete(query);
	}*/
}

void Database::GetEventLogs(char* name,char* target,int32 account_id,int8 eventid,char* detail,char* timestamp, CharacterEventLog_Struct* cel)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	int32 count = 0;
	char modifications[200];
	if(strlen(name) != 0)
		sprintf(modifications,"charname=\'%s\'",name);
	else if(account_id != 0)
		sprintf(modifications,"accountid=%i",account_id);
	
	if(strlen(target) != 0)
		sprintf(modifications,"%s AND target like \'%%%s%%\'",modifications,target);

	if(strlen(detail) != 0)
		sprintf(modifications,"%s AND description like \'%%%s%%\'",modifications,detail);

	if(strlen(timestamp) != 0)
		sprintf(modifications,"%s AND time like \'%%%s%%\'",modifications,timestamp);
	
	if(eventid == 0)
		eventid =1;
	sprintf(modifications,"%s AND event_nid=%i",modifications,eventid);

			MakeAnyLenString(&query, "SELECT id,accountname,accountid,status,charname,target,time,descriptiontype,description FROM eventlog where %s",modifications);	
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				delete[] query;
				while((row = mysql_fetch_row(result)))
				{
						if(count > 255)
							break;
						cel->eld[count].id = atoi(row[0]);
						strncpy(cel->eld[count].accountname,row[1],64);
						cel->eld[count].account_id = atoi(row[2]);
						cel->eld[count].status = atoi(row[3]);
						strncpy(cel->eld[count].charactername,row[4],64);
						strncpy(cel->eld[count].targetname,row[5],64);
						sprintf(cel->eld[count].timestamp,"%s",row[6]);
						strncpy(cel->eld[count].descriptiontype,row[7],64);
						strncpy(cel->eld[count].details,row[8],128);
						cel->eventid = eventid;
						count++;
						cel->count = count;
					}
			}
					else
					{
						// TODO: Invalid item length in database
					}
				mysql_free_result(result);
}

#ifdef SHAREMEM
void Database::GetCharSelectInfo(int32 account_id, CharacterSelect_Struct* cs) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
    for(int i=0;i<10;i++) {
        strcpy(cs->name[i], "<none>");
        cs->zone[i] = 0;
        cs->level[i] = 0;
    }
	
	
	PlayerProfile_Struct* pp;
	int char_num = 0;
	unsigned long* lengths;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name,profile,zonename FROM character_ WHERE account_id=%i order by name", account_id), errbuf, &result)) {
		delete[] query;
		while ((row = mysql_fetch_row(result)))
		{
			lengths = mysql_fetch_lengths(result);
			if (lengths[1] != 0 && lengths[1] == sizeof(PlayerProfile_Struct))
			{
				strcpy(cs->name[char_num], row[0]);
				pp = (PlayerProfile_Struct*) row[1];
				cs->level[char_num] = pp->level;
				cs->class_[char_num] = pp->class_;
				cs->race[char_num] = pp->race;
				cs->gender[char_num] = pp->gender;
				cs->face[char_num] = pp->face;
				cs->zone[char_num] = GetZoneID(row[2]);
				
				// Coder_01 - REPLACE with item info when available.
				const Item_Struct* item = GetItem(pp->inventory[2]);
				//				LoadAItem(pp->inventory[2],&cs->equip[char_num][0],&cs->cs_colors[char_num][0]);
				if (item != 0) {
					cs->equip[char_num][0] = item->common.material;
					cs->cs_colors[char_num][0] = item->common.color;
				}
				//delete item;
				item = GetItem(pp->inventory[17]);
				//LoadAItem(pp->inventory[17],&cs->equip[char_num][1],&cs->cs_colors[char_num][1]);
				if (item != 0) {
					cs->equip[char_num][1] = item->common.material;
					cs->cs_colors[char_num][1] = item->common.color;
				}
				//delete item;
				item = GetItem(pp->inventory[7]);
				//LoadAItem(pp->inventory[7],&cs->equip[char_num][2],&cs->cs_colors[char_num][2]);
				if (item != 0) {
					cs->equip[char_num][2] = item->common.material;
					cs->cs_colors[char_num][2] = item->common.color;
				}
				//delete item;
				item = GetItem(pp->inventory[10]);
				//LoadAItem(pp->inventory[10],&cs->equip[char_num][3],&cs->cs_colors[char_num][3]);
				if (item != 0) {

					cs->equip[char_num][3] = item->common.material;
					cs->cs_colors[char_num][3] = item->common.color;
				}
				//delete item;
				item = GetItem(pp->inventory[12]);
				//LoadAItem(pp->inventory[12],&cs->equip[char_num][4],&cs->cs_colors[char_num][4]);
				if (item != 0) {
					cs->equip[char_num][4] = item->common.material;
					cs->cs_colors[char_num][4] = item->common.color;
				}
				//delete item;
				item = GetItem(pp->inventory[18]);
				//LoadAItem(pp->inventory[18],&cs->equip[char_num][5],&cs->cs_colors[char_num][5]);
				if (item != 0) {
					cs->equip[char_num][5] = item->common.material;
					cs->cs_colors[char_num][5] = item->common.color;
				}
				//delete item;
				item = GetItem(pp->inventory[19]);
				//LoadAItem(pp->inventory[19],&cs->equip[char_num][6],&cs->cs_colors[char_num][6]);
				if (item != 0) {
					cs->equip[char_num][6] = item->common.material;
					cs->cs_colors[char_num][6] = item->common.color;
				}
				//delete item;
				item = GetItem(pp->inventory[13]);
				//LoadAItem(pp->inventory[13],&cs->equip[char_num][7],&cs->cs_colors[char_num][7]); // Mainhand
				if (item != 0) {
					cs->equip[char_num][7] = item->common.material;
					cs->cs_colors[char_num][7] = item->common.color;
				}
				//delete item;
				item = GetItem(pp->inventory[14]);
				//LoadAItem(pp->inventory[14],&cs->equip[char_num][8],&cs->cs_colors[char_num][8]); // Offhand
				if (item != 0) {
					cs->equip[char_num][8] = item->common.material;
					cs->cs_colors[char_num][8] = item->common.color;
				}
				//delete item;
				char_num++;
				if (char_num > 10)
					break;
			}
			else {
				cout << "Got a bogus character (" << row[0] << "), deleting it." << endl;
				DeleteCharacter(row[0]);
			}
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetCharSelectInfo query '" << query << "' " << errbuf << endl;
		delete[] query;
		return;
	}
	
	return;
}
#else
void Database::GetCharSelectInfo(int32 account_id, CharacterSelect_Struct* cs) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
    for(int i=0;i<10;i++) {
        strcpy(cs->name[i], "<none>");
        cs->zone[i] = 0;
        cs->level[i] = 0;
    }
	
	
	PlayerProfile_Struct* pp;
	int char_num = 0;
	unsigned long* lengths;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name,profile,zonename FROM character_ WHERE account_id=%i order by name", account_id), errbuf, &result)) {
		delete[] query;
		while ((row = mysql_fetch_row(result)))
		{
			lengths = mysql_fetch_lengths(result);
			if (lengths[1] != 0)
			{
				strcpy(cs->name[char_num], row[0]);
				pp = (PlayerProfile_Struct*) row[1];
				cs->level[char_num] = pp->level;
				cs->class_[char_num] = pp->class_;
				cs->race[char_num] = pp->race;
				cs->gender[char_num] = pp->gender;
				cs->face[char_num] = pp->face;
				cs->zone[char_num] = GetZoneID(row[2]);
				
				// Coder_01 - REPLACE with item info when available.
				///*const*/ Item_Struct* item = 0;
				LoadAItem(pp->inventory[2],&cs->equip[char_num][0],&cs->cs_colors[char_num][0]);
				//if (item != 0) {
				//	&cs->equip[char_num][0] = item->common.material;
				//	&cs->cs_colors[char_num][0] = item->common.color;
				//}
				//delete item;
				/*item = */LoadAItem(pp->inventory[17],&cs->equip[char_num][1],&cs->cs_colors[char_num][1]);
				/*if (item != 0) {
				&cs->equip[char_num][1] = item->common.material;
				&cs->cs_colors[char_num][1] = item->common.color;
				}
				delete item;
				item = */LoadAItem(pp->inventory[7],&cs->equip[char_num][2],&cs->cs_colors[char_num][2]);
				/*if (item != 0) {
				&cs->equip[char_num][2] = item->common.material;
				&cs->cs_colors[char_num][2] = item->common.color;
				}
				delete item;
				item = */LoadAItem(pp->inventory[10],&cs->equip[char_num][3],&cs->cs_colors[char_num][3]);
				/*if (item != 0) {
				&cs->equip[char_num][3] = item->common.material;
				&cs->cs_colors[char_num][3] = item->common.color;
				}
				delete item;
				item = */LoadAItem(pp->inventory[12],&cs->equip[char_num][4],&cs->cs_colors[char_num][4]);
				/*if (item != 0) {
				&cs->equip[char_num][4] = item->common.material;
				&cs->cs_colors[char_num][4] = item->common.color;
				}
				delete item;
				item = */LoadAItem(pp->inventory[18],&cs->equip[char_num][5],&cs->cs_colors[char_num][5]);
				/*if (item != 0) {
				&cs->equip[char_num][5] = item->common.material;
				&cs->cs_colors[char_num][5] = item->common.color;
				}
				delete item;
				item = */LoadAItem(pp->inventory[19],&cs->equip[char_num][6],&cs->cs_colors[char_num][6]);
				/*if (item != 0) {
				&cs->equip[char_num][6] = item->common.material;
				&cs->cs_colors[char_num][6] = item->common.color;

				}
				delete item;
				item = */LoadAItem(pp->inventory[13],&cs->equip[char_num][7],&cs->cs_colors[char_num][7]); // Mainhand
				/*if (item != 0) {
				&cs->equip[char_num][7] = item->common.material;
				&cs->cs_colors[char_num][7] = item->common.color;
				}
				delete item;
				item = */LoadAItem(pp->inventory[14],&cs->equip[char_num][8],&cs->cs_colors[char_num][8]); // Offhand
				/*if (item != 0) {
				&cs->equip[char_num][8] = item->common.material;
				&cs->cs_colors[char_num][8] = item->common.color;
				}
				delete item;*/
				char_num++;
				if (char_num > 10)
					break;
			}
			else {
				cout << "Got a bogus character (" << row[0] << "), deleting it." << endl;
				DeleteCharacter(row[0]);
			}
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetCharSelectInfo query '" << query << "' " << errbuf << endl;
		delete[] query;
		return;
	}
	
	return;
}
#endif

/*
Reserve the character name "name" for account "account_id"
This name can then be used to create a character.
Return true if successful, false if the name was already reserved.
False will also be returned if there is a database error.
*/
bool Database::ReserveName(int32 account_id, char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	
	//if (strlen(name) > 15)
	//	return false;
	
	/*for (int i=0; i<strlen(name); i++)
	{
	if ((name[i] < 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z'))
	return 0;
	if (i > 0 && name[i] >= 'A' && name[i] <= 'Z')
	{
	name[i] = name[i]+'a'-'A';
	}
}*/
	
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT into character_ SET account_id=%i, name='%s', profile=NULL", account_id, name), errbuf)) {
		cerr << "Error in ReserveName query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	delete[] query;
	return true;
}

/*
Delete the character with the name "name"
False will also be returned if there is a database error.
*/
bool Database::DeleteCharacter(char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	//if (strlen(name) > 15)
	//	return false;
	
	/*for (int i=0; i<strlen(name); i++)
	{
	if ((name[i] < 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z'))
	return 0;
	if (i > 0 && name[i] >= 'A' && name[i] <= 'Z')
	{
	name[i] = name[i]+'a'-'A';
	}
}*/
	
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE from character_ WHERE name='%s'", name), errbuf)) {
		cerr << "Error in DeleteCharacter query '" << query << "' " << errbuf << endl;
		if (query != 0)
			delete[] query;
		return false;
	}
	

	return true;
}

bool Database::CreateCharacter(int32 account_id, PlayerProfile_Struct* pp) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(PlayerProfile_Struct)*2+1];
	char* end = query;
	int32 affected_rows = 0;
	
	//	if (strlen(pp->name) > 15)
	//		return false;
	char tmp[10];
#ifdef GUILDWARS
	bool guildwars = false;
	if (database.GetVariable("GuildWars", tmp, 9))
    {
	guildwars = atoi(tmp);
	}
	if(guildwars)
	{
	if(pp->race >= 13)

	if(pp->race >= 13)
		return false;
	}
#endif

	unsigned int i;
	for (i=0; i<strlen(pp->name); i++) {
		if ((pp->name[i] < 'a' || pp->name[i] > 'z') && 
			(pp->name[i] < 'A' || pp->name[i] > 'Z'))
			return 0;
	}
	toupper(pp->name[0]);
	pp->guildid = 0;
	for(i=1;i<strlen(pp->name);i++) tolower(pp->name[i]);
    end += sprintf(end, "UPDATE character_ SET zonename=\'%s\', x = %f, y = %f, z = %f, profile=", GetZoneName(pp->current_zone), pp->x, pp->y, pp->z);
    *end++ = '\'';
    end += DoEscapeString(end, (char*) pp, sizeof(PlayerProfile_Struct));
    *end++ = '\'';
    end += sprintf(end," WHERE account_id=%d AND name='%s'", account_id, pp->name);
	
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in CreateCharacter query '" << query << "' " << errbuf << endl;
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

bool Database::GetCharacterInfoForLogin(const char* name, int32* character_id, char* current_zone, PlayerProfile_Struct* pp, int32* pplen, PlayerAA_Struct* aa, int32* aalen, int32* guilddbid, int8* guildrank) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 querylen;
    MYSQL_RES *result;

	if (character_id && *character_id) {
		// searching by ID should be a lil bit faster
		querylen = MakeAnyLenString(&query, "SELECT id, profile, zonename, x, y, z, alt_adv, guild, guildrank FROM character_ WHERE id=%i", *character_id);
	}
	else {
		querylen = MakeAnyLenString(&query, "SELECT id, profile, zonename, x, y, z, alt_adv, guild, guildrank FROM character_ WHERE name='%s'", name);
	}
	
	if (RunQuery(query, querylen, errbuf, &result)) {
		delete[] query;
		bool ret = GetCharacterInfoForLogin_result(result, character_id, current_zone, pp, pplen, aa, aalen, guilddbid, guildrank);
		mysql_free_result(result);
		return ret;
	}
	else {
		cerr << "Error in GetCharacterInfoForLogin query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}

bool Database::GetCharacterInfoForLogin_result(MYSQL_RES* result, int32* character_id, char* current_zone, PlayerProfile_Struct* pp, int32* pplen, PlayerAA_Struct* aa, int32* aalen, int32* guilddbid, int8* guildrank) {
    MYSQL_ROW row;
	unsigned long* lengths;
	if (mysql_num_rows(result) == 1) {	

		row = mysql_fetch_row(result);
		lengths = mysql_fetch_lengths(result);
		if (pp && pplen) {
			if (lengths[1] == sizeof(PlayerProfile_Struct)) {
				memcpy(pp, row[1], sizeof(PlayerProfile_Struct));
			}
			else {
				cerr << "Player profile length mismatch in GetPlayerProfile" << endl;
				return false;
			}
			*pplen = lengths[1];
			pp->current_zone = GetZoneID(row[2]);
			pp->x = atof(row[3]);
			pp->y = atof(row[4]);
			pp->z = atof(row[5]);

			if (pp->x == -1 && pp->y == -1 && pp->z == -1)
				GetSafePoints(pp->current_zone, &pp->x, &pp->y, &pp->z);
			else
				pp->z *= 10;
		}
		if (character_id)
			*character_id = atoi(row[0]);
		if (current_zone)
			strcpy(current_zone, row[2]);
		if (aa && aalen) {
			if(row[6] && lengths[6] >= sizeof(PlayerAA_Struct)) {
				memcpy(aa, row[6], sizeof(PlayerAA_Struct));
				*aalen = result->lengths[6];
			}
			else { // let's support ghetto-ALTERed databases that don't contain any data in the alt_adv column
				memset(aa, 0, sizeof(PlayerAA_Struct));
				*aalen = sizeof(PlayerAA_Struct);
			}
		}
		if (guilddbid)
			*guilddbid = atoi(row[7]);
		if (guildrank)
			*guildrank = atoi(row[8]);
		return true;
	}
	else {
		return false;
	}
}

/*
Get the player profile for the given account "account_id" and character name "name"
Return true if the character was found, otherwise false.
False will also be returned if there is a database error.
*/
int32 Database::GetPlayerProfile(int32 account_id, char* name, PlayerProfile_Struct* pp, char* current_zone) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	/*for (int i=0; i<strlen(name); i++) {
	if ((name[i] < 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z') && 
	(name[i] < '0' || name[i] > '9'))
	return 0;
}*/
	
	unsigned long* lengths;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT profile, zonename, x, y, z FROM character_ WHERE account_id=%i AND name='%s'", account_id, name), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {	
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result);
			if (lengths[0] == sizeof(PlayerProfile_Struct)) {
				memcpy(pp, row[0], sizeof(PlayerProfile_Struct));
			}
			else {
				cerr << "Player profile length mismatch in GetPlayerProfile" << endl;
				mysql_free_result(result);
				return 0;
			}
			if (current_zone)
				strcpy(current_zone, row[1]);
			pp->current_zone = GetZoneID(row[1]);
			pp->x = atof(row[2]);
			pp->y = atof(row[3]);
			pp->z = atof(row[4]);
			if (pp->x == -1 && pp->y == -1 && pp->z == -1)
				GetSafePoints(pp->current_zone, &pp->x, &pp->y, &pp->z);
			else
				pp->z *= 10;
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		int32 len = lengths[0];
		mysql_free_result(result);
		return len;
	}
	else {
		cerr << "Error in GetPlayerProfile query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	
	return 0;
}

/*
Update the player profile for the given account "account_id" and character name "name"
Return true if the character was found, otherwise false.
False will also be returned if there is a database error.
*/
bool Database::SetPlayerProfile(int32 account_id, int32 charid, PlayerProfile_Struct* pp, int32 current_zone) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	//if (strlen(name) > 15)
	//	return false;
	
	/*for (int i=0; i<strlen(name); i++)
	{
	if ((name[i] < 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z') && 
	(name[i] < '0' || name[i] > '9'))
	return 0;
}*/
	
	
	int32 affected_rows = 0;
    if (!RunQuery(query, SetPlayerProfile_MQ(&query, account_id, charid, pp, current_zone), errbuf, 0, &affected_rows)) {
        cerr << "Error in SetPlayerProfile query " << errbuf << endl;
		delete[] query;
		return false;
    }
	delete[] query;
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

int32 Database::SetPlayerProfile_MQ(char** query, int32 account_id, int32 charid, PlayerProfile_Struct* pp, int32 current_zone) {
    *query = new char[256+sizeof(PlayerProfile_Struct)*2+1];
	char* end = *query;
	if (!current_zone)
		current_zone = pp->current_zone;
	end += sprintf(end, "UPDATE character_ SET name=\'%s\', zonename=\'%s\', zoneid=%u, x = %f, y = %f, z = %f, profile=", pp->name, GetZoneName(current_zone), current_zone, pp->x, pp->y, pp->z);
	*end++ = '\'';
    end += DoEscapeString(end, (char*)pp, sizeof(PlayerProfile_Struct));
    *end++ = '\'';
    end += sprintf(end," WHERE account_id=%d AND id=%u", account_id, charid);

	return (int32) (end - (*query));
}

/*
This function returns the account_id that owns the character with
the name "name" or zero if no character with that name was found
Zero will also be returned if there is a database error.
*/
int32 Database::GetAccountIDByChar(const char* charname, int32* oCharID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	/*for (int i=0; i<strlen(charname); i++)
	{
	if ((charname[i] < 'a' || charname[i] > 'z') && 
	(charname[i] < 'A' || charname[i] > 'Z') && 
	(charname[i] < '0' || charname[i] > '9'))
	return 0;
}*/
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT account_id, id FROM character_ WHERE name='%s'", charname), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]); // copy to temp var because gotta free the result before exitting this function
			if (oCharID)
				*oCharID = atoi(row[1]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAccountIDByChar query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return 0;
}


int32 Database::GetAccountIDByName(const char* accname, sint16* status, int32* lsid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	
	for (unsigned int i=0; i<strlen(accname); i++) {
		if ((accname[i] < 'a' || accname[i] > 'z') && 
			(accname[i] < 'A' || accname[i] > 'Z') && 
			(accname[i] < '0' || accname[i] > '9'))
			return 0;
	}
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, status, lsaccount_id FROM account WHERE name='%s'", accname), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]); // copy to temp var because gotta free the result before exitting this function
			if (status)
				*status = atoi(row[1]);
			if (lsid) {
				if (row[2])
					*lsid = atoi(row[2]);
				else
					*lsid = 0;
			}
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAccountIDByAcc query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return 0;
}

void Database::GetAccountName(int32 accountid, char* name, int32* oLSAccountID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name, lsaccount_id FROM account WHERE id='%i'", accountid), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);

			strcpy(name, row[0]);
			if (row[1] && oLSAccountID) {
				*oLSAccountID = atoi(row[1]);
			}
		}
		mysql_free_result(result);
	}
	else {
		delete[] query;
		cerr << "Error in GetAccountName query '" << query << "' " << errbuf << endl;
	}
}

int32 Database::GetCharacterInfo(const char* iName, int32* oAccID, int32* oZoneID, float* oX, float* oY, float* oZ) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, account_id, zonename, x, y, z FROM character_ WHERE name='%s'", iName), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 charid = atoi(row[0]);
			if (oAccID)
				*oAccID = atoi(row[1]);
			if (oZoneID)
				*oZoneID = GetZoneID(row[2]);
			if (oX)
				*oX = atof(row[3]);
			if (oY)
				*oY = atof(row[4]);
			if (oZ)
				*oZ = atof(row[5]);
			mysql_free_result(result);
			return charid;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetCharacterInfo query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return 0;
}

bool Database::LoadVariables() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	
	if (RunQuery(query, LoadVariables_MQ(&query), errbuf, &result)) {
		delete[] query;
		bool ret = LoadVariables_result(result);
		mysql_free_result(result);
		return ret;
	}
	else {
		cerr << "Error in LoadVariables query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}

int32 Database::LoadVariables_MQ(char** query) {
	LockMutex lock(&Mvarcache);
	return MakeAnyLenString(query, "SELECT varname, value, unix_timestamp() FROM variables where unix_timestamp(ts) >= %d", varcache_lastupdate);
}

bool Database::LoadVariables_result(MYSQL_RES* result) {
	int32 i;
    MYSQL_ROW row;
	LockMutex lock(&Mvarcache);
	if (mysql_num_rows(result) > 0) {
		if (!varcache_array) {
			varcache_max = mysql_num_rows(result);
			varcache_array = new VarCache_Struct*[varcache_max];
			for (i=0; i<varcache_max; i++)
				varcache_array[i] = 0;
		}
		else {
			int32 tmpnewmax = varcache_max + mysql_num_rows(result);
			VarCache_Struct** tmp = new VarCache_Struct*[tmpnewmax];
			for (i=0; i<tmpnewmax; i++)
				tmp[i] = 0;
			for (i=0; i<varcache_max; i++)
				tmp[i] = varcache_array[i];
			VarCache_Struct** tmpdel = varcache_array;
			varcache_array = tmp;
			varcache_max = tmpnewmax;
			delete tmpdel;
		}
		while ((row = mysql_fetch_row(result))) {
			varcache_lastupdate = atoi(row[2]);
			for (i=0; i<varcache_max; i++) {
				if (varcache_array[i]) {
					if (strcasecmp(varcache_array[i]->varname, row[0]) == 0) {
						delete varcache_array[i];
						varcache_array[i] = (VarCache_Struct*) new int8[sizeof(VarCache_Struct) + strlen(row[1]) + 1];
						strn0cpy(varcache_array[i]->varname, row[0], sizeof(varcache_array[i]->varname));
						strcpy(varcache_array[i]->value, row[1]);
						break;
					}
				}
				else {
					varcache_array[i] = (VarCache_Struct*) new int8[sizeof(VarCache_Struct) + strlen(row[1]) + 1];
					strcpy(varcache_array[i]->varname, row[0]);
					strcpy(varcache_array[i]->value, row[1]);
					break;
				}
			}
		}
		int32 max_used = 0;
		for (i=0; i<varcache_max; i++) {
			if (varcache_array[i]) {
				if (i > max_used)
					max_used = i;
			}
		}
		max_used++;
		varcache_max = max_used;
	}
	return true;
}

// Gets variable from 'variables' table

bool Database::GetVariable(const char* varname, char* varvalue, int16 varvalue_len) {
	LockMutex lock(&Mvarcache);
	if (strlen(varname) <= 1)
		return false;
	for (int32 i=0; i<varcache_max; i++) {

		if (varcache_array[i]) {
			if (strcasecmp(varcache_array[i]->varname, varname) == 0) {
				snprintf(varvalue, varvalue_len, "%s", varcache_array[i]->value);
				varvalue[varvalue_len-1] = 0;
				return true;
			}
		}
		else
			return false;
	}
	return false;
/*
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT value FROM variables WHERE varname like '%s'", varname), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			snprintf(varvalue, varvalue_len, "%s", row[0]);
			varvalue[varvalue_len-1] = 0;
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetVariable query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
*/
}

bool Database::SetVariable(const char* varname, const char* varvalue) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "Update variables set value='%s' WHERE varname like '%s'", varvalue, varname), errbuf, 0, &affected_rows)) {
		safe_delete(query);
		if (affected_rows == 1) {
			LoadVariables(); // refresh cache
			return true;
		}
		else {
			if (RunQuery(query, MakeAnyLenString(&query, "Insert Into variables (varname, value) values ('%s', '%s')", varname, varvalue), errbuf, 0, &affected_rows)) {
				safe_delete(query);
				if (affected_rows == 1) {
					LoadVariables(); // refresh cache
					return true;
				}
			}
		}
	}
	else {
		cerr << "Error in SetVariable query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}

bool Database::CheckZoneserverAuth(const char* ipaddr) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT * FROM zoneserver_auth WHERE '%s' like host", ipaddr), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) >= 1) {
			mysql_free_result(result);
			return true;
		}
		else {
			mysql_free_result(result);
			return false;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in CheckZoneserverAuth query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	return false;
}

bool Database::GetGuildRanks(int32 guildeqid, GuildRanks_Struct* gr) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, eqid, name, leader, minstatus, rank0title, rank1, rank1title, rank2, rank2title, rank3, rank3title, rank4, rank4title, rank5, rank5title from guilds where eqid=%i;", guildeqid), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			gr->leader = atoi(row[3]);
			gr->databaseID = atoi(row[0]);
			gr->minstatus = atoi(row[4]);
			strcpy(gr->name, row[2]);
			for (int i = 0; i <= GUILD_MAX_RANK; i++) {
				strcpy(gr->rank[i].rankname, row[5 + (i*2)]);
				if (i == 0) {
					gr->rank[i].heargu = 1;
					gr->rank[i].speakgu = 1;
					gr->rank[i].invite = 1;
					gr->rank[i].remove = 1;
					gr->rank[i].promote = 1;
					gr->rank[i].demote = 1;
					gr->rank[i].motd = 1;
					gr->rank[i].warpeace = 1;
				}
				else if (strlen(row[4 + (i*2)]) >= 8) {
					gr->rank[i].heargu = (row[4 + (i*2)][GUILD_HEAR] == '1');
					gr->rank[i].speakgu = (row[4 + (i*2)][GUILD_SPEAK] == '1');
					gr->rank[i].invite = (row[4 + (i*2)][GUILD_INVITE] == '1');
					gr->rank[i].remove = (row[4 + (i*2)][GUILD_REMOVE] == '1');
					gr->rank[i].promote = (row[4 + (i*2)][GUILD_PROMOTE] == '1');
					gr->rank[i].demote = (row[4 + (i*2)][GUILD_DEMOTE] == '1');
					gr->rank[i].motd = (row[4 + (i*2)][GUILD_MOTD] == '1');
					gr->rank[i].warpeace = (row[4 + (i*2)][GUILD_WARPEACE] == '1');
				}
				else {
					gr->rank[i].heargu = 1;
					gr->rank[i].speakgu = 1;
					gr->rank[i].invite = 0;
					gr->rank[i].remove = 0;
					gr->rank[i].promote = 0;
					gr->rank[i].demote = 0;
					gr->rank[i].motd = 0;
					gr->rank[i].warpeace = 0;
				}
				
				if (gr->rank[i].rankname[0] == 0)
					snprintf(gr->rank[i].rankname, 100, "Guild Rank %i", i);
			}
		}
		else {
			gr->leader = 0;
			gr->databaseID = 0;
			gr->minstatus = 0;
			memset(gr->name, 0, sizeof(gr->name));
			for (int i = 0; i <= GUILD_MAX_RANK; i++) {
				snprintf(gr->rank[i].rankname, 100, "Guild Rank %i", i);
				if (i == 0) {
					gr->rank[i].heargu = 1;
					gr->rank[i].speakgu = 1;
					gr->rank[i].invite = 1;
					gr->rank[i].remove = 1;
					gr->rank[i].promote = 1;
					gr->rank[i].demote = 1;
					gr->rank[i].motd = 1;
					gr->rank[i].warpeace = 1;
				}
				else {
					gr->rank[i].heargu = 0;
					gr->rank[i].speakgu = 0;
					gr->rank[i].invite = 0;
					gr->rank[i].remove = 0;
					gr->rank[i].promote = 0;
					gr->rank[i].demote = 0;
					gr->rank[i].motd = 0;

					gr->rank[i].warpeace = 0;
				}
			}
		}
		mysql_free_result(result);
		return true;
	}
	else {
		cerr << "Error in GetGuildRank query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}

bool Database::LoadGuilds(GuildRanks_Struct* guilds) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	//	int i;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	for (int a = 0; a < 512; a++) {
		guilds[a].leader = 0;
		guilds[a].databaseID = 0;
		memset(guilds[a].name, 0, sizeof(guilds[a].name));
		for (int i = 0; i <= GUILD_MAX_RANK; i++) {
			snprintf(guilds[a].rank[i].rankname, 100, "Guild Rank %i", i);
			if (i == 0) {
				guilds[a].rank[i].heargu = 1;
				guilds[a].rank[i].speakgu = 1;
				guilds[a].rank[i].invite = 1;
				guilds[a].rank[i].remove = 1;
				guilds[a].rank[i].promote = 1;
				guilds[a].rank[i].demote = 1;
				guilds[a].rank[i].motd = 1;
				guilds[a].rank[i].warpeace = 1;
			}
			else {
				guilds[a].rank[i].heargu = 0;
				guilds[a].rank[i].speakgu = 0;
				guilds[a].rank[i].invite = 0;
				guilds[a].rank[i].remove = 0;
				guilds[a].rank[i].promote = 0;
				guilds[a].rank[i].demote = 0;
				guilds[a].rank[i].motd = 0;
				guilds[a].rank[i].warpeace = 0;
			}
		}
		Sleep(0);
	}
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, eqid, name, leader, minstatus, rank0title, rank1, rank1title, rank2, rank2title, rank3, rank3title, rank4, rank4title, rank5, rank5title from guilds"), errbuf, &result)) {

		delete[] query;
		int32 guildeqid = 0xFFFFFFFF;
		while ((row = mysql_fetch_row(result))) {
			guildeqid = atoi(row[1]);
			if (guildeqid < 512) {
				guilds[guildeqid].leader = atoi(row[3]);
				guilds[guildeqid].databaseID = atoi(row[0]);
				guilds[guildeqid].minstatus = atoi(row[4]);
				strcpy(guilds[guildeqid].name, row[2]);
				for (int i = 0; i <= GUILD_MAX_RANK; i++) {
					strcpy(guilds[guildeqid].rank[i].rankname, row[5 + (i*2)]);
					if (i == 0) {
						guilds[guildeqid].rank[i].heargu = 1;
						guilds[guildeqid].rank[i].speakgu = 1;
						guilds[guildeqid].rank[i].invite = 1;
						guilds[guildeqid].rank[i].remove = 1;
						guilds[guildeqid].rank[i].promote = 1;
						guilds[guildeqid].rank[i].demote = 1;
						guilds[guildeqid].rank[i].motd = 1;
						guilds[guildeqid].rank[i].warpeace = 1;
					}
					else if (strlen(row[4 + (i*2)]) >= 8) {
						guilds[guildeqid].rank[i].heargu = (row[4 + (i*2)][GUILD_HEAR] == '1');
						guilds[guildeqid].rank[i].speakgu = (row[4 + (i*2)][GUILD_SPEAK] == '1');
						guilds[guildeqid].rank[i].invite = (row[4 + (i*2)][GUILD_INVITE] == '1');
						guilds[guildeqid].rank[i].remove = (row[4 + (i*2)][GUILD_REMOVE] == '1');
						guilds[guildeqid].rank[i].promote = (row[4 + (i*2)][GUILD_PROMOTE] == '1');
						guilds[guildeqid].rank[i].demote = (row[4 + (i*2)][GUILD_DEMOTE] == '1');
						guilds[guildeqid].rank[i].motd = (row[4 + (i*2)][GUILD_MOTD] == '1');
						guilds[guildeqid].rank[i].warpeace = (row[4 + (i*2)][GUILD_WARPEACE] == '1');
					}
					else {

						guilds[guildeqid].rank[i].heargu = 1;
						guilds[guildeqid].rank[i].speakgu = 1;
						guilds[guildeqid].rank[i].invite = 0;
						guilds[guildeqid].rank[i].remove = 0;
						guilds[guildeqid].rank[i].promote = 0;
						guilds[guildeqid].rank[i].demote = 0;
						guilds[guildeqid].rank[i].motd = 0;
						guilds[guildeqid].rank[i].warpeace = 0;
					}
					
					if (guilds[guildeqid].rank[i].rankname[0] == 0)
						snprintf(guilds[guildeqid].rank[i].rankname, 100, "Guild Rank %i", i);
				}
			}
			Sleep(0);
		}
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "Error in LoadGuilds query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}

// Pyro: Get zone starting points from DB
bool Database::GetSafePoints(const char* short_name, float* safe_x, float* safe_y, float* safe_z, sint16* minstatus, int8* minlevel) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	//	int buf_len = 256;
	//    int chars = -1;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT safe_x, safe_y, safe_z, minium_status, minium_level FROM zone WHERE short_name='%s'", short_name), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (safe_x != 0)
				*safe_x = atof(row[0]);
			if (safe_y != 0)
				*safe_y = atof(row[1]);
			if (safe_z != 0)
				*safe_z = atof(row[2]);
			if (minstatus != 0)
				*minstatus = atoi(row[3]);
			if (minlevel != 0)
				*minlevel = atoi(row[4]);
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetSafePoint query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}
int32 Database::NumberInGuild(int32 guilddbid) {
    	char errbuf[MYSQL_ERRMSG_SIZE];
    	char *query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "Select count(id) from character_ where guild=%i", guilddbid), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 ret = atoi(row[0]);
			mysql_free_result(result);
			return ret;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in NumberInGuild query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	return 0;
}
bool Database::SetGuild(int32 charid, int32 guilddbid, int8 guildrank) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET guild=%i, guildrank=%i WHERE id=%i", guilddbid, guildrank, charid), errbuf, 0, &affected_rows)) {
		delete[] query;
		if (affected_rows == 1)
			return true;

		else
			return false;
	}
	else {
		cerr << "Error in SetGuild query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}

int32 Database::GetFreeGuildEQID()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[100];
    MYSQL_RES *result;
	
	for (int x = 1; x < 512; x++) {
		snprintf(query, 100, "SELECT eqid FROM guilds where eqid=%i;", x);
		
		if (RunQuery(query, strlen(query), errbuf, &result)) {
			if (mysql_num_rows(result) == 0) {
				mysql_free_result(result);
				return x;
			}
			mysql_free_result(result);
		}
		else {
			cerr << "Error in GetFreeGuildEQID query '" << query << "' " << errbuf << endl;
		}
	}
	
	return 0xFFFFFFFF;
}

int32 Database::CreateGuild(const char* name, int32 leader) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	char buf[65];
	int32 affected_rows = 0;
	DoEscapeString(buf, name, strlen(name)) ;
	
	int32 tmpeqid = GetFreeGuildEQID();
	if (tmpeqid == 0xFFFFFFFF) {
		cout << "Error in Database::CreateGuild: unable to find free eqid" << endl;
		return 0xFFFFFFFF;
	}
	
	if (RunQuery(query, MakeAnyLenString(&query, "INSERT INTO guilds (name, leader, eqid) Values ('%s', %i, %i)", buf, leader, tmpeqid), errbuf, 0, &affected_rows)) {
		delete[] query;
		if (affected_rows == 1) {
			return tmpeqid;
		}
		else {
			return 0xFFFFFFFF;
		}
	}
	else {
		cerr << "Error in CreateGuild query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0xFFFFFFFF;
	}
	
	return 0xFFFFFFFF;
}

bool Database::DeleteGuild(int32 guilddbid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "DELETE FROM guilds WHERE id=%i;", guilddbid), errbuf, 0, &affected_rows)) {
		delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in DeleteGuild query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}

bool Database::RenameGuild(int32 guilddbid, const char* name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	char buf[65];
	DoEscapeString(buf, name, strlen(name)) ;
	
	if (RunQuery(query, MakeAnyLenString(&query, "Update guilds set name='%s' WHERE id=%i;", buf, guilddbid), errbuf, 0, &affected_rows)) {
		delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in RenameGuild query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}



bool Database::EditGuild(int32 guilddbid, int8 ranknum, GuildRankLevel_Struct* grl)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    int chars = 0;
	int32 affected_rows = 0;
	char buf[203];
	char buf2[8];
	DoEscapeString(buf, grl->rankname, strlen(grl->rankname)) ;
	buf2[GUILD_HEAR] = grl->heargu + '0';
	buf2[GUILD_SPEAK] = grl->speakgu + '0';
	buf2[GUILD_INVITE] = grl->invite + '0';
	buf2[GUILD_REMOVE] = grl->remove + '0';
	buf2[GUILD_PROMOTE] = grl->promote + '0';
	buf2[GUILD_DEMOTE] = grl->demote + '0';
	buf2[GUILD_MOTD] = grl->motd + '0';
	buf2[GUILD_WARPEACE] = grl->warpeace + '0';
	
	if (ranknum == 0)
		chars = MakeAnyLenString(&query, "Update guilds set rank%ititle='%s' WHERE id=%i;", ranknum, buf, guilddbid);
	else
		chars = MakeAnyLenString(&query, "Update guilds set rank%ititle='%s', rank%i='%s' WHERE id=%i;", ranknum, buf, ranknum, buf2, guilddbid);
	
	if (RunQuery(query, chars, errbuf, 0, &affected_rows)) {
		delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in EditGuild query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}

bool Database::GetGuildNameByID(int32 guilddbid, char * name) {
	if (!name || !guilddbid) return false;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	MYSQL_RES *result;
    MYSQL_ROW row;	
	
	if (RunQuery(query, MakeAnyLenString(&query, "select * from guilds where id='%i'", guilddbid), errbuf, &result)) {
		delete[] query;
		row = mysql_fetch_row(result);
		if (row[2]) sprintf(name,"%s",row[2]);
		return true;
	}
	else {
		cerr << "Error in RenameGuild query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}


bool Database::GetZoneLongName(const char* short_name, char** long_name, char* file_name, float* safe_x, float* safe_y, float* safe_z, int32* maxclients) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT long_name, file_name, safe_x, safe_y, safe_z, maxclients FROM zone WHERE short_name='%s'", short_name), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (long_name != 0) {
				*long_name = strcpy(new char[strlen(row[0])+1], row[0]);
			}
			if (file_name != 0) {
				if (row[1] == 0)
					strcpy(file_name, short_name);
				else
					strcpy(file_name, row[1]);
			}
			if (safe_x != 0)
				*safe_x = atof(row[2]);
			if (safe_y != 0)
				*safe_y = atof(row[3]);
			if (safe_z != 0)
				*safe_z = atof(row[4]);
			if (maxclients)
				*maxclients = atoi(row[5]);
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetZoneLongName query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}

int32 Database::GetGuildDBIDbyLeader(int32 leader)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM guilds WHERE leader=%i", leader), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetGuildDBIDbyLeader query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return 0;
}

bool Database::SetGuildLeader(int32 guilddbid, int32 leader)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE guilds SET leader=%i WHERE id=%i", leader, guilddbid), errbuf, 0, &affected_rows)) {
		delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in SetGuildLeader query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}

bool Database::UpdateItem(uint16 item_id,Item_Struct* is)
{
#ifndef SHAREMEM
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(Item_Struct)*2+1];
	char* end = query;
	
	end += sprintf(end, "UPDATE items SET raw_data=");
	*end++ = '\'';
    end += DoEscapeString(end, (char*)is, sizeof(Item_Struct));
    *end++ = '\'';
    end += sprintf(end," WHERE id=%i", item_id);
	
	item_array[item_id] = is;
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in UpdateItem query " << errbuf << endl;
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
#else
	return false;
#endif
}

bool Database::SetGuildMOTD(int32 guilddbid, const char* motd) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	char* motdbuf = 0;
	int32 affected_rows = 0;
	
	motdbuf = new char[(strlen(motd)*2)+3];

	DoEscapeString(motdbuf, motd, strlen(motd)) ;
	
	if (RunQuery(query, MakeAnyLenString(&query, "Update guilds set motd='%s' WHERE id=%i;", motdbuf, guilddbid), errbuf, 0, &affected_rows)) {
		delete[] query;
		delete motdbuf;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else
	{
		cerr << "Error in SetGuildMOTD query '" << query << "' " << errbuf << endl;
		delete[] query;
		delete motdbuf;
		return false;
	}
	
	return false;
}

bool Database::GetGuildMOTD(int32 guilddbid, char* motd)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT motd FROM guilds WHERE id=%i", guilddbid), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (row[0] == 0)
				strcpy(motd, "");
			else
				strcpy(motd, row[0]);
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	
	
	else {
		cerr << "Error in GetGuildMOTD query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return false;
}

sint32 Database::GetItemsCount(int32* oMaxID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), count(*) FROM items");
	
	//int32 tmpMaxItem = 0;
	//int32 tmpItemCount = 0;
	//	Item_Struct tmpItem;
	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row != NULL && row[1] != 0) {
			sint32 ret = atoi(row[1]);
			if (oMaxID) {
				if (row[0])
					*oMaxID = atoi(row[0]);
				else
					*oMaxID = 0;
			}
			mysql_free_result(result);
			return ret;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetItemsCount query '" << query << "' " << errbuf << endl;
		delete[] query;
		return -1;
	}
	return -1;
}

sint32 Database::GetNPCTypesCount(int32* oMaxID) {

	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), count(*) FROM npc_types");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row != NULL && row[1] != 0) {
			sint32 ret = atoi(row[1]);
			if (oMaxID) {
				if (row[0])
					*oMaxID = atoi(row[0]);
				else
					*oMaxID = 0;
			}
			mysql_free_result(result);
			return ret;
		}
	}
	else {
		cerr << "Error in GetNPCTypesCount query '" << query << "' " << errbuf << endl;
		delete[] query;

		return -1;
	}
	
	return -1;
}

sint32 Database::GetDoorsCount(int32* oMaxID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), count(*) FROM doors");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row != NULL && row[1] != 0) {
			sint32 ret = atoi(row[1]);
			if (oMaxID) {
				if (row[0])
					*oMaxID = atoi(row[0]);
				else
					*oMaxID = 0;
			}
			mysql_free_result(result);
			return ret;
		}
	}
	else {
		cerr << "Error in GetDoorsCount query '" << query << "' " << errbuf << endl;
		delete[] query;
		return -1;
	}
	
	return -1;
}

void Database::LoadItemStatus() {
	memset(item_minstatus, 0, sizeof(item_minstatus));
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int32 tmp;
	if (RunQuery(query, MakeAnyLenString(&query, "Select id, minstatus from items where minstatus > 0"), errbuf, &result)) {
		delete query;
		while ((row = mysql_fetch_row(result)) && row[0] && row[1]) {
			tmp = atoi(row[0]);
			if (tmp < 0xFFFF)
				item_minstatus[tmp] = atoi(row[1]);
		}
		mysql_free_result(result);
	}
	else {
		cout << "Error in LoadItemStatus query: '" << query << "'" << endl;
		delete query;
	}
}

bool Database::DBSetItemStatus(int32 id, int8 status) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32 affected_rows = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "Update items set minstatus=%u where id=%u", status, id), errbuf, 0, &affected_rows)) {
		cout << "Error in LoadItemStatus query: '" << query << "'" << endl;
	}
	safe_delete(query);
	return (bool) (affected_rows == 1);
}

#ifdef SHAREMEM
extern "C" bool extDBLoadItems(sint32 iItemCount, int32 iMaxItemID) { return database.DBLoadItems(iItemCount, iMaxItemID); }
bool Database::LoadItems() {
	if (!EMuShareMemDLL.Load())
		return false;
	sint32 tmp = 0;
	tmp = GetItemsCount(&max_item);
	if (tmp == -1) {
		cout << "Error: Database::LoadItems() (sharemem): GetItemsCount() returned -1" << endl;
		return false;
	}
	bool ret = EMuShareMemDLL.Items.DLLLoadItems(&extDBLoadItems, sizeof(Item_Struct), &tmp, &max_item);
#if defined(GOTFRAGS) || defined(_DEBUG)
	if (ret)
		LoadItemStatus();
#endif
	return ret;
}
bool Database::DBLoadItems(sint32 iItemCount, int32 iMaxItemID) {
	cout << "Loading items from database..." << endl;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), count(*) FROM items");
	
	//	Item_Struct tmpItem;
	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row != 0 && row[0] > 0) {
			if (row[0])
				max_item = atoi(row[0]);
			else
				max_item = 0;
			if (atoi(row[1]) != iItemCount) {
				cout << "Error: Insufficient shared memory to load items." << endl;
				cout << "Count(id): " << atoi(row[1]) << ", iItemCount: " << iItemCount << endl;
				return false;
			}
			if (atoi(row[0]) != iMaxItemID) {
				cout << "Error: Insufficient shared memory to load items." << endl;
				cout << "Max(id): " << atoi(row[0]) << ", iMaxItemID: " << iMaxItemID << endl;
				cout << "Fix this by increasing the MMF_EQMAX_ITEMS define statement" << endl;
				return false;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT id,raw_data FROM items");
			
			if (RunQuery(query, strlen(query), errbuf, &result)) {
				delete[] query;
				while((row = mysql_fetch_row(result))) {
					unsigned long* lengths;
					lengths = mysql_fetch_lengths(result);
					if (lengths[1] == sizeof(Item_Struct)) {
						//						memcpy(tmpItem, row[1], sizeof(Item_Struct));
						//						EMuShareMemDLL.Items.cbAddItem(atoi(row[0]), tmpItem);
                        // neotokyo: dirty fix for recommended skill 255
                        if (((Item_Struct*)(row[1]))->common.RecSkill > 252)
                            ((Item_Struct*)(row[1]))->common.RecSkill = 252;
						if (!EMuShareMemDLL.Items.cbAddItem(atoi(row[0]), (Item_Struct*) row[1])) {
							mysql_free_result(result);
							cout << "Error: Database::DBLoadItems: !EMuShareMemDLL.Items.cbAddItem(" << atoi(row[0]) << ")" << endl;
							return false;
						}
					}
					else {
						// TODO: Invalid item length in database
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in DBLoadItems query '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
		}
		else {

			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in DBLoadItems query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	return true;
}

const Item_Struct* Database::GetItem(uint32 id) {
	return EMuShareMemDLL.Items.GetItem(id);
}

const Item_Struct* Database::IterateItems(uint32* NextIndex) {
	return EMuShareMemDLL.Items.IterateItems(NextIndex);
}
#else
bool Database::LoadItems()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id) FROM items");
	
	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) { 
			max_item = atoi(row[0]);
			item_array = new Item_Struct*[max_item+1];
			for(unsigned int i=0; i<max_item; i++)
			{

				item_array[i] = 0;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT id,raw_data FROM items");
			
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				delete[] query;
				while((row = mysql_fetch_row(result)))
				{
					unsigned long* lengths;
					lengths = mysql_fetch_lengths(result);
					if (lengths[1] == sizeof(Item_Struct))
					{
						item_array[atoi(row[0])] = new Item_Struct;
						memcpy(item_array[atoi(row[0])], row[1], sizeof(Item_Struct));
                        // neotokyo: dirty fix for recommended skill 255
                        if (item_array[atoi(row[0])]->common.RecSkill > 252)
                            item_array[atoi(row[0])]->common.RecSkill = 252;
					}
					else
					{
						// TODO: Invalid item length in database
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	return true;
}

const Item_Struct* Database::GetItem(uint32 id) {
	if (item_array && id <= max_item)
		return item_array[id];
	else
		return 0;
}
#endif

#ifdef SHAREMEM
extern "C" bool extDBLoadDoors(sint32 iDoorCount, int32 iMaxDoorID) { return database.DBLoadDoors(iDoorCount, iMaxDoorID); }
const Door* Database::GetDoor(int8 door_id, const char* zone_name) {
	for(uint32 i=0; i!=max_door_type;i++)
	{
        const Door* door;
        door = GetDoorDBID(i);
        if (!door)
		continue;
	if(door->door_id == door_id && strcasecmp(door->zone_name, zone_name) == 0)
	return door;
	}
return 0;
}

const Door* Database::GetDoorDBID(uint32 db_id) {
	return EMuShareMemDLL.Doors.GetDoor(db_id);
}

bool Database::LoadDoors() {
	if (!EMuShareMemDLL.Load())
		return false;
	sint32 tmp = 0;
	tmp = GetDoorsCount(&max_door_type);
	if (tmp == -1) {
		cout << "Error: Database::LoadDoors-ShareMem: GetDoorsCount() returned < 0" << endl;
		return false;
	}
	bool ret = EMuShareMemDLL.Doors.DLLLoadDoors(&extDBLoadDoors, sizeof(Door), &tmp, &max_door_type);
	return ret;
}

bool Database::DBLoadDoors(sint32 iDoorCount, int32 iMaxDoorID) {
	cout << "Loading Doors from database..." << endl;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), Count(*) FROM doors");
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) {
			if (atoi(row[0]) > iMaxDoorID) {
				cout << "Error: Insufficient shared memory to load doors." << endl;
				cout << "Max(id): " << atoi(row[0]) << ", iMaxDoorID: " << iMaxDoorID << endl;
				cout << "Fix this by increasing the MMF_MAX_Door_ID define statement" << endl;
				mysql_free_result(result);
				return false;
			}
			if (atoi(row[1]) != iDoorCount) {
				cout << "Error: Insufficient shared memory to load doors." << endl;
				cout << "Count(*): " << atoi(row[1]) << ", iDoorCount: " << iDoorCount << endl;
				mysql_free_result(result);
				return false;
			}
			max_door_type = atoi(row[0]);
			mysql_free_result(result);
			Door tmpDoor;
			MakeAnyLenString(&query, "SELECT id,doorid,zone,name,pos_x,pos_y,pos_z,heading,opentype,guild,lockpick,keyitem,triggerdoor,triggertype,dest_zone,dest_x,dest_y,dest_z,dest_heading from doors");//WHERE zone='%s'", zone_name
			if (RunQuery(query, strlen(query), errbuf, &result)) {
				safe_delete(query);
				while((row = mysql_fetch_row(result))) {
					memset(&tmpDoor, 0, sizeof(Door));
					tmpDoor.db_id = atoi(row[0]);
					tmpDoor.door_id = atoi(row[1]);
					strncpy(tmpDoor.zone_name,row[2],16);
					strncpy(tmpDoor.door_name,row[3],16);
					tmpDoor.pos_x = (float)atof(row[4]);
					tmpDoor.pos_y = (float)atof(row[5]);
					tmpDoor.pos_z = (float)atof(row[6]);
					tmpDoor.heading = (float)atof(row[7]);
					tmpDoor.opentype = atoi(row[8]);
					tmpDoor.guildid = atoi(row[9]);
					tmpDoor.lockpick = atoi(row[10]);
					tmpDoor.keyitem = atoi(row[11]);
					tmpDoor.trigger_door = atoi(row[12]);
					tmpDoor.trigger_type = atoi(row[13]);
					strncpy(tmpDoor.dest_zone,row[14],16);
					tmpDoor.dest_x = (float) atof(row[15]);
					tmpDoor.dest_y = (float) atof(row[16]);
					tmpDoor.dest_z = (float) atof(row[17]);
					tmpDoor.dest_heading = (float) atof(row[18]);
					if (!EMuShareMemDLL.Doors.cbAddDoor(tmpDoor.db_id, &tmpDoor)) {
						mysql_free_result(result);
						return false;
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else
			{
				cerr << "Error in DBLoadDoors query '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
		}
		else
			mysql_free_result(result);
	}
	return true;
}
#else
const Door* Database::GetDoor(int8 door_id, const char* zone_name)
{
	for(uint32 i=0; i!=max_door_type;i++)
	{
        const Door* door;
        door = GetDoorDBID(i);
        if (!door)
            continue;
	    if(door->door_id == door_id && strcasecmp(door->zone_name, zone_name) == 0)
	        return door;
	}
    return 0;
}

const Door* Database::GetDoorDBID(uint32 db_id)
{
	return door_array[db_id];
}

bool Database::LoadDoors()
{
	cout << "(NOMEMSHARE) Loading Doors from database..." << endl;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
    Door tmpDoor;

    if (door_array == 0)
    {
        int32 count;
        
        if (!GetDoorsCount(&count))
            return false;

        max_door_type = count;

        door_array = new Door*[count+1];
        memset(door_array, 0, sizeof(door_array)*(count+1));
        if (!door_array)
            return false;
    }
    else
    {
        int32 count;
        
        if (!GetDoorsCount(&count))
            return false;
        if (count != max_door_type) // what happened here? do we need to reallocate?
            return false;
    }

    MakeAnyLenString(&query, "SELECT id,doorid,zone,name,pos_x,pos_y,pos_z,heading,opentype,guild,lockpick,keyitem,triggerdoor,triggertype,liftheight from doors");//WHERE zone='%s'", zone_name
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
	    safe_delete(query);
		while((row = mysql_fetch_row(result)))
        {
		    memset(&tmpDoor, 0, sizeof(Door));
			memset(&door_array[tmpDoor.db_id],0,sizeof(Door));
			tmpDoor.db_id = atoi(row[0]);
			tmpDoor.door_id = atoi(row[1]);
			strncpy(tmpDoor.zone_name,row[2],16);
            strncpy(tmpDoor.door_name,row[3],10);
			tmpDoor.pos_x = (float)atof(row[4]);
			tmpDoor.pos_y = (float)atof(row[5]);
			tmpDoor.pos_z = (float)atof(row[6]);
			tmpDoor.heading = (float)atof(row[7]);
			tmpDoor.opentype = atoi(row[8]);
			tmpDoor.guildid = atoi(row[9]);
			tmpDoor.lockpick = atoi(row[10]);
			tmpDoor.keyitem = atoi(row[11]);
			tmpDoor.trigger_door = atoi(row[12]);
			tmpDoor.trigger_type = atoi(row[13]);
			tmpDoor.liftheight= atoi(row[14]);
            if (door_array[tmpDoor.db_id] == NULL)
                door_array[tmpDoor.db_id] = new Door;

            else
            {
                cerr << "Double DB_Id " << tmpDoor.door_id << " while loading doors" << endl;
            }
            memcpy(door_array[tmpDoor.db_id], &tmpDoor, sizeof(Door));

            Sleep(0);
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in LoadDoors query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	return true;
}
#endif

#ifdef SHAREMEM
extern "C" bool extDBLoadNPCTypes(sint32 iNPCTypeCount, int32 iMaxNPCTypeID) { return database.DBLoadNPCTypes(iNPCTypeCount, iMaxNPCTypeID); }
const NPCType* Database::GetNPCType(uint32 id) {
	return EMuShareMemDLL.NPCTypes.GetNPCType(id);
}
bool Database::LoadNPCTypes() {
	if (!EMuShareMemDLL.Load())
		return false;
	sint32 tmp = -1;
	int32 tmp_max_npc_type;
	tmp = GetNPCTypesCount(&tmp_max_npc_type);
	if (tmp < 0) {
		cout << "Error: Database::LoadNPCTypes-ShareMem: GetNPCTypesCount() returned < 0" << endl;
		return false;
	}
	max_npc_type = tmp_max_npc_type;
	bool ret = EMuShareMemDLL.NPCTypes.DLLLoadNPCTypes(&extDBLoadNPCTypes, sizeof(NPCType), &tmp, &max_npc_type);
	return ret;
}
bool Database::DBLoadNPCTypes(sint32 iNPCTypeCount, int32 iMaxNPCTypeID) {
	cout << "Loading NPCTypes from database..." << endl;
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), Count(*) FROM npc_types");
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) {
			if (atoi(row[0]) > iMaxNPCTypeID) {
				cout << "Error: Insufficient shared memory to load items." << endl;
				cout << "Max(id): " << atoi(row[0]) << ", iMaxNPCTypeID: " << iMaxNPCTypeID << endl;
				cout << "Fix this by increasing the MMF_MAX_NPCTYPE_ID define statement" << endl;
				return false;
			}
			if (atoi(row[1]) != iNPCTypeCount) {
				cout << "Error: Insufficient shared memory to load items." << endl;

				cout << "Count(*): " << atoi(row[1]) << ", iNPCTypeCount: " << iNPCTypeCount << endl;
				return false;
			}
			max_npc_type = atoi(row[0]);
			mysql_free_result(result);
			
			NPCType tmpNPCType;
			// support the old database too for a while...
        		// neotokyo: added aggroradius
        		// neotokyo: added bodytype
#ifndef IPCENABLE
			MakeAnyLenString(&query, "SELECT id,name,level,race,class,hp,gender,texture,helmtexture,size,loottable_id, merchant_id, banish, mindmg, maxdmg, npcspecialattks, npc_spells_id, d_meele_texture1,d_meele_texture2, walkspeed, runspeed,fixedz,hp_regen_rate,mana_regen_rate,aggroradius,bodytype,npc_faction_id FROM npc_types");//WHERE zone='%s'", zone_name
#else
			MakeAnyLenString(&query, "SELECT id,name,level,race,class,hp,gender,texture,helmtexture,size,loottable_id, merchant_id, banish, mindmg, maxdmg, npcspecialattks, npc_spells_id, d_meele_texture1,d_meele_texture2, walkspeed, runspeed,fixedz,hp_regen_rate,mana_regen_rate,aggroradius,bodytype,npc_faction_id,ipc FROM npc_types");//WHERE zone='%s'", zone_name
#endif
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				safe_delete(query);
				while((row = mysql_fetch_row(result)))
				{
					memset(&tmpNPCType, 0, sizeof(NPCType));
					strncpy(tmpNPCType.name, row[1], 30);
					tmpNPCType.npc_id = atoi(row[0]); // rembrant, Dec. 2
					tmpNPCType.level = atoi(row[2]);
					tmpNPCType.race = atoi(row[3]);
					tmpNPCType.class_ = atoi(row[4]);
					tmpNPCType.cur_hp = atoi(row[5]);
					tmpNPCType.max_hp = atoi(row[5]);
					tmpNPCType.gender = atoi(row[6]);
					tmpNPCType.texture = atoi(row[7]);
					tmpNPCType.helmtexture = atoi(row[8]);
					tmpNPCType.size = atof(row[9]);
					tmpNPCType.loottable_id = atoi(row[10]);
					tmpNPCType.merchanttype = atoi(row[11]);
					tmpNPCType.STR = 75;
					tmpNPCType.STA = 75;
					tmpNPCType.DEX = 75;
					tmpNPCType.AGI = 75;
					tmpNPCType.WIS = 75;
					tmpNPCType.INT = 75;
					tmpNPCType.CHA = 75;
					tmpNPCType.banish = atoi(row[12]);
					tmpNPCType.min_dmg = atoi(row[13]);
					tmpNPCType.max_dmg = atoi(row[14]);
					strcpy(tmpNPCType.npc_attacks,row[15]);
					tmpNPCType.npc_spells_id = atoi(row[16]);
					tmpNPCType.d_meele_texture1= atoi(row[17]);
					tmpNPCType.d_meele_texture2= atoi(row[18]);
					tmpNPCType.walkspeed= atof(row[19]);
					tmpNPCType.runspeed= atof(row[20]);
					tmpNPCType.fixedZ = atof(row[21]);
					tmpNPCType.hp_regen = atoi(row[22]);
					tmpNPCType.mana_regen = atoi(row[23]);
                			tmpNPCType.aggroradius = (sint32)atoi(row[24]);
                			if (row[25] && strlen(row[25]))
                			    tmpNPCType.bodytype = (int8)atoi(row[25]);
                			else
                    			    tmpNPCType.bodytype = 0;
					tmpNPCType.npc_faction_id = atoi(row[26]);
                			// set defaultvalue for aggroradius
					if (tmpNPCType.aggroradius <= 0)
                    			    tmpNPCType.aggroradius = 70;
#ifndef IPCENABLE
					tmpNPCType.ipc = false;
#else
					tmpNPCType.ipc = atoi(row[27]);
#endif
/*					if (RunQuery(query, MakeAnyLenString(&query, "SELECT faction_id, value, primary_faction FROM npc_faction WHERE npc_id = %i", tmpNPCType.npc_id), errbuf, &result2)) {
						safe_delete(query);
						int i = 0;
						while((row2 = mysql_fetch_row(result2))) {
							if (atoi(row2[2]) && i != 0) {
								tmpNPCType.factionid[i] = tmpNPCType.factionid[0];
								tmpNPCType.factionvalue[i] = tmpNPCType.factionvalue[0];
								tmpNPCType.factionid[0] = atoi(row2[0]);
								tmpNPCType.factionvalue[0] = atoi(row2[1]);
							}
							else {
								tmpNPCType.factionid[i] = atoi(row2[0]);
								tmpNPCType.factionvalue[i] = atoi(row2[1]);
							}
							i++;
							if (i >= MAX_NPC_FACTIONS) {
								cerr << "Error in DBLoadNPCTypes: More than MAX_NPC_FACTIONS factions returned, npcid=" << tmpNPCType.npc_id << endl;
								break;
							}
						}
						mysql_free_result(result2);
					}
*/
					if (!EMuShareMemDLL.NPCTypes.cbAddNPCType(tmpNPCType.npc_id, &tmpNPCType)) {
						mysql_free_result(result);
						cout << "Error: Database::DBLoadNPCTypes: !EMuShareMemDLL.NPCTypes.cbAddNPCType(" << tmpNPCType.npc_id << ")" << endl;
						return false;
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else{
				safe_delete(query);
				query = 0;
				MakeAnyLenString(&query, "SELECT id,name,level,race,class,hp,gender,texture,helmtexture,size,loottable_id, merchant_id, banish, mindmg, maxdmg, npcspecialattks,npc_spells_id FROM npc_types");//WHERE zone='%s'", zone_name
				if (RunQuery(query, strlen(query), errbuf, &result))
				{
					safe_delete(query);
					while((row = mysql_fetch_row(result))) {
						memset(&tmpNPCType, 0, sizeof(NPCType));
						strncpy(tmpNPCType.name, row[1], 30);
						tmpNPCType.npc_id = atoi(row[0]); // rembrant, Dec. 2
						tmpNPCType.level = atoi(row[2]);
						tmpNPCType.race = atoi(row[3]);
						tmpNPCType.class_ = atoi(row[4]);
						tmpNPCType.cur_hp = atoi(row[5]);
						tmpNPCType.max_hp = atoi(row[5]);
						tmpNPCType.gender = atoi(row[6]);
						tmpNPCType.texture = atoi(row[7]);
						tmpNPCType.helmtexture = atoi(row[8]);
						tmpNPCType.size = atof(row[9]);
						tmpNPCType.loottable_id = atoi(row[10]);
						tmpNPCType.merchanttype = atoi(row[11]);
						tmpNPCType.STR = 75;
						tmpNPCType.STA = 75;
						tmpNPCType.DEX = 75;
						tmpNPCType.AGI = 75;
						tmpNPCType.WIS = 75;
						tmpNPCType.INT = 75;
						tmpNPCType.CHA = 75;
						tmpNPCType.banish = atoi(row[12]);
						tmpNPCType.min_dmg = atoi(row[13]);
						tmpNPCType.max_dmg = atoi(row[14]);
						strcpy(tmpNPCType.npc_attacks,row[15]);
						tmpNPCType.npc_spells_id = atoi(row[16]);
/*						if (RunQuery(query, MakeAnyLenString(&query, "SELECT faction_id, value, primary_faction FROM npc_faction WHERE npc_id = %i", tmpNPCType.npc_id), errbuf, &result2)) {
							safe_delete(query);
							int i = 0;
							while((row2 = mysql_fetch_row(result2))) {
								if (atoi(row2[2]) && i != 0) {
									tmpNPCType.factionid[i] = tmpNPCType.factionid[0];
									tmpNPCType.factionvalue[i] = tmpNPCType.factionvalue[0];
									tmpNPCType.factionid[0] = atoi(row2[0]);
									tmpNPCType.factionvalue[0] = atoi(row2[1]);
								}
								else {
									tmpNPCType.factionid[i] = atoi(row2[0]);
									tmpNPCType.factionvalue[i] = atoi(row2[1]);
								}
								i++;
								if (i >= MAX_NPC_FACTIONS) {
									cerr << "Error in DBLoadNPCTypes: More than MAX_NPC_FACTIONS factions returned, npcid=" << tmpNPCType.npc_id << endl;
									break;
								}
							}
							mysql_free_result(result2);
						}*/
						if (!EMuShareMemDLL.NPCTypes.cbAddNPCType(tmpNPCType.npc_id, &tmpNPCType)) {
							mysql_free_result(result);
							return false;
						}
						Sleep(0);
					}
					mysql_free_result(result);
					cout << "NPCs loaded - using old database format" << endl;
				}
				else
				{
					cerr << "Error in DBLoadNPCTypes query '" << query << "' " << errbuf << endl;
					delete[] query;
					return false;
				}
			}
/*			if (RunQuery(query, MakeAnyLenString(&query, "SELECT npc_id, faction_id, value, primary_faction FROM npc_faction order by npc_id", tmpNPCType.npc_id), errbuf, &result)) {
				safe_delete(query);
				sint8 i = 0;
				int32 curnpcid = 0;
				int32 tmpnpcid = 0;
				uint32 tmpfactionid[MAX_NPC_FACTIONS]; memset(tmpfactionid, 0, sizeof(tmpfactionid));
				sint32 tmpfactionvalue[MAX_NPC_FACTIONS]; memset(tmpfactionvalue, 0, sizeof(tmpfactionvalue));
				sint8 tmpprimaryfaction = -1;
				while((row = mysql_fetch_row(result))) {
					tmpnpcid = atoi(row[0]);
					if (curnpcid != tmpnpcid && curnpcid != 0) {
						EMuShareMemDLL.NPCTypes.cbSetNPCFaction(curnpcid, tmpfactionid, tmpfactionvalue, tmpprimaryfaction);
						memset(tmpfactionid, 0, sizeof(tmpfactionid));
						memset(tmpfactionvalue, 0, sizeof(tmpfactionvalue));
						i = 0;
						tmpprimaryfaction = -1;
					}
					curnpcid = tmpnpcid;
					if (atoi(row[3])) {
						tmpprimaryfaction = i;
					}
					tmpfactionid[i] = atoi(row[1]);
					tmpfactionvalue[i] = atoi(row[2]);
					i++;
					if (i >= MAX_NPC_FACTIONS) {
						cerr << "Error in DBLoadNPCTypes: More than MAX_NPC_FACTIONS factions returned, npcid=" << tmpnpcid << endl;
						i--;
					}
				}
				mysql_free_result(result);
			}*/
		}
		else {
			mysql_free_result(result);

		}
	}
	else {
		cerr << "Error in DBLoadNPCTypes query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}

	return true;
}
#else
bool Database::LoadNPCTypes() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_RES *result2;
    MYSQL_ROW row;
    MYSQL_ROW row2;
	query = new char[256];
	strcpy(query, "SELECT MAX(id) FROM npc_types");
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row && row[0])
		{
			max_npc_type = atoi(row[0]);
			npc_type_array = new NPCType*[max_npc_type+1];
			for (unsigned int x=0; x <= max_npc_type; x++)
				npc_type_array[x] = 0;
			mysql_free_result(result);
			
			// support the old database too for a while...
#ifndef IPCENABLE
			MakeAnyLenString(&query, "SELECT id,name,level,race,class,hp,gender,texture,helmtexture,size,loottable_id, merchant_id, banish, mindmg, maxdmg, npcspecialattks, npc_spells_id, d_meele_texture1,d_meele_texture2, walkspeed, runspeed,fixedz,hp_regen_rate,mana_regen_rate,aggroradius,bodytype,npc_faction_id FROM npc_types");//WHERE zone='%s'", zone_name
#else
			MakeAnyLenString(&query, "SELECT id,name,level,race,class,hp,gender,texture,helmtexture,size,loottable_id, merchant_id, banish, mindmg, maxdmg, npcspecialattks, npc_spells_id, d_meele_texture1,d_meele_texture2, walkspeed, runspeed,fixedz,hp_regen_rate,mana_regen_rate,aggroradius,bodytype,npc_faction_id,ipc FROM npc_types");//WHERE zone='%s'", zone_name
#endif
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				safe_delete(query);
				while((row = mysql_fetch_row(result)))
				{
					int32 tmpid = atoi(row[0]);
					npc_type_array[tmpid] = new NPCType;
					memset(npc_type_array[tmpid], 0, sizeof(NPCType));
					strncpy(npc_type_array[tmpid]->name, row[1], 30);
					npc_type_array[tmpid]->npc_id = tmpid; // rembrant, Dec. 2
					npc_type_array[tmpid]->level = atoi(row[2]);
					npc_type_array[tmpid]->race = atoi(row[3]);
					npc_type_array[tmpid]->class_ = atoi(row[4]);
					npc_type_array[tmpid]->cur_hp = atoi(row[5]);
					npc_type_array[tmpid]->max_hp = atoi(row[5]);
					npc_type_array[tmpid]->gender = atoi(row[6]);
					npc_type_array[tmpid]->texture = atoi(row[7]);
					npc_type_array[tmpid]->helmtexture = atoi(row[8]);
					npc_type_array[tmpid]->size = atof(row[9]);
					npc_type_array[tmpid]->loottable_id = atoi(row[10]);
					npc_type_array[tmpid]->merchanttype = atoi(row[11]);
					npc_type_array[tmpid]->STR = 75;
					npc_type_array[tmpid]->STA = 75;
					npc_type_array[tmpid]->DEX = 75;
					npc_type_array[tmpid]->AGI = 75;
					npc_type_array[tmpid]->WIS = 75;
					npc_type_array[tmpid]->INT = 75;
					npc_type_array[tmpid]->CHA = 75;
					npc_type_array[tmpid]->banish = atoi(row[12]);
					npc_type_array[tmpid]->min_dmg = atoi(row[13]);
					npc_type_array[tmpid]->max_dmg = atoi(row[14]);
					strcpy(npc_type_array[tmpid]->npc_attacks,row[15]);
					npc_type_array[tmpid]->npc_spells_id = atoi(row[16]);
					npc_type_array[tmpid]->d_meele_texture1= atoi(row[17]);
					npc_type_array[tmpid]->d_meele_texture2= atoi(row[18]);
					npc_type_array[tmpid]->walkspeed= atof(row[19]);
					npc_type_array[tmpid]->runspeed= atof(row[20]);
					npc_type_array[tmpid]->fixedZ = atof(row[21]);
					npc_type_array[tmpid]->hp_regen = (sint32)atof(row[22]);
					npc_type_array[tmpid]->mana_regen = (sint32)atof(row[23]);
					npc_type_array[tmpid]->aggroradius = (sint32)atoi(row[24]);
					if (row[25] && strlen(row[25])) // check NULL
						npc_type_array[tmpid]->bodytype = (int8)atoi(row[25]);
					else // if db value is NULL, assume bodytype 0
						npc_type_array[tmpid]->bodytype = 0;
					npc_type_array[tmpid]->npc_faction_id = atoi(row[26]);
#ifndef IPCENABLE
					npc_type_array[tmpid]->ipc = false;
#else
					npc_type_array[tmpid]->ipc = atoi(row[27]);
#endif

					// set defaultvalue for aggroradius
					if (npc_type_array[tmpid]->aggroradius <= 0) 
						npc_type_array[tmpid]->aggroradius = 70;
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else{
				safe_delete(query);
				query = 0;
				MakeAnyLenString(&query, "SELECT id,name,level,race,class,hp,gender,texture,helmtexture,size,loottable_id, merchant_id, banish, mindmg, maxdmg, npcspecialattks,npc_spells_id FROM npc_types");//WHERE zone='%s'", zone_name
				if (RunQuery(query, strlen(query), errbuf, &result))
				{
					safe_delete(query);
					while((row = mysql_fetch_row(result)))
					{
						int32 tmpid = atoi(row[0]);
						npc_type_array[tmpid] = new NPCType;
						memset(npc_type_array[tmpid], 0, sizeof(NPCType));
						strncpy(npc_type_array[tmpid]->name, row[1], 30);
						npc_type_array[tmpid]->npc_id = tmpid; // rembrant, Dec. 2
						npc_type_array[tmpid]->level = atoi(row[2]);
						npc_type_array[tmpid]->race = atoi(row[3]);
						npc_type_array[tmpid]->class_ = atoi(row[4]);
						npc_type_array[tmpid]->cur_hp = atoi(row[5]);
						npc_type_array[tmpid]->max_hp = atoi(row[5]);
						npc_type_array[tmpid]->gender = atoi(row[6]);
						npc_type_array[tmpid]->texture = atoi(row[7]);
						npc_type_array[tmpid]->helmtexture = atoi(row[8]);
						npc_type_array[tmpid]->size = atof(row[9]);
						npc_type_array[tmpid]->loottable_id = atoi(row[10]);
						npc_type_array[tmpid]->merchanttype = atoi(row[11]);
						npc_type_array[tmpid]->STR = 75;
						npc_type_array[tmpid]->STA = 75;
						npc_type_array[tmpid]->DEX = 75;
						npc_type_array[tmpid]->AGI = 75;
						npc_type_array[tmpid]->WIS = 75;
						npc_type_array[tmpid]->INT = 75;
						npc_type_array[tmpid]->CHA = 75;
						npc_type_array[tmpid]->banish = atoi(row[12]);
						npc_type_array[tmpid]->min_dmg = atoi(row[13]);
						npc_type_array[tmpid]->max_dmg = atoi(row[14]);
						strcpy(npc_type_array[tmpid]->npc_attacks,row[15]);
						npc_type_array[tmpid]->npc_spells_id = atoi(row[16]);
						Sleep(0);
					}
					mysql_free_result(result);
					cout << "NPCs loaded - using old database format" << endl;
				}
				else
				{
					cerr << "Error in LoadNPCTypes query '" << query << "' " << errbuf << endl;
					delete[] query;
					return false;
				}
			}
		}
		else
		{
			mysql_free_result(result);
		}
	}
	else
	{
		cerr << "Error in LoadNPCTypes query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return true;
}

const NPCType* Database::GetNPCType(uint32 id) {
	if (npc_type_array && id <= max_npc_type)
		return npc_type_array[id]; 
	else
		return 0;
}
#endif

#ifndef SHAREMEM
void Database::LoadAItem(uint16 item_id, unsigned int* texture, unsigned int* color)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	Item_Struct tempitem;
	MakeAnyLenString(&query, "SELECT raw_data FROM items where id=%d",item_id);
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		delete[] query;
		while((row = mysql_fetch_row(result)))
		{
			unsigned long* lengths;
			lengths = mysql_fetch_lengths(result);
			if (lengths[0] == sizeof(Item_Struct))
			{
				//tempitem=row[0];
				memcpy(&tempitem, row[0], sizeof(Item_Struct));
				*texture=tempitem.common.material;
				*color=tempitem.common.color;
			}
			else
			{
				// TODO: Invalid item length in database
			}
			Sleep(0);
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadAItem query '" << query << "' " << errbuf << endl;
		delete[] query;
		//return false;
	}
	
	//return tempitemptr;
}
#endif

bool Database::SaveItemToDatabase(Item_Struct* ItemToSave)
{
	// Insert the modified item back into the database
	char Query[sizeof(Item_Struct)*2+1 + 64];
	char EscapedText[sizeof(Item_Struct)*2+1];
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	//	long AffectedRows;
	
	if(DoEscapeString(EscapedText, (const char *)ItemToSave, sizeof(Item_Struct))) {
		sprintf(Query, "UPDATE items SET raw_data = '%s' WHERE id = %d\n", EscapedText, ItemToSave->item_nr);
		
		if(RunQuery(Query, strlen(Query), errbuf))
		{
			//			dpf("Affected rows = %d", AffectedRows);
			return(true);
		}
		else {
			cout<<"Save failed::"<<Query<<endl;
		}
	}
	
	//dpf("Update failed");
	return(false);
}

bool Database::LoadZoneNames() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(zoneidnumber) FROM zone");
	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row && row[0])
		{ 
			max_zonename = atoi(row[0]);
			zonename_array = new char*[max_zonename+1];
			for(unsigned int i=0; i<max_zonename; i++) {
				zonename_array[i] = 0;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT zoneidnumber, short_name FROM zone");
			if (RunQuery(query, strlen(query), errbuf, &result)) {
				delete[] query;
				while((row = mysql_fetch_row(result))) {
					zonename_array[atoi(row[0])] = new char[strlen(row[1]) + 1];
					strcpy(zonename_array[atoi(row[0])], row[1]);
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadZoneNames query '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in LoadZoneNames query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	return true;
}

bool Database::SetItemAtt(char* att, char * value, unsigned int ItemIndex) {
#ifndef SHAREMEM
	if (att && value && ItemIndex) {
		//item info
		if (strstr(att,"name")) {
			strcpy(item_array[ItemIndex]->name,value);
		}
		else if (strstr(att, "lore")) {
			strcpy(item_array[ItemIndex]->lore,value);
		}
		else if (strstr(att,"idfile")) {
			strcpy(item_array[ItemIndex]->idfile,value);
		}
		else if (strstr(att, "flag")) {
			item_array[ItemIndex]->flag = atoi(value);
		}
		else if (strstr(att, "weight")) {
			item_array[ItemIndex]->weight = atoi(value);
		}
		else if (strstr(att, "nosave")) {
			item_array[ItemIndex]->nodrop = atoi(value);
		}
		else if (strstr(att, "nodrop")) {
			item_array[ItemIndex]->nosave = atoi(value);
		}
		else if (strstr(att, "size")) {
			item_array[ItemIndex]->size = atoi(value);
		}
		else if (strstr(att, "type")) {
			item_array[ItemIndex]->type = atoi(value);
		}
		else if (strstr(att, "icon")) {

			item_array[ItemIndex]->icon_nr = atoi(value);
		}
		else if (strstr(att, "equipableSlots")) {
			item_array[ItemIndex]->equipableSlots = atoi(value);
		}
		//item stats
		else if (strstr(att, "str")) {
			item_array[ItemIndex]->common.STR = atoi(value);
		}
		else if (strstr(att, "sta")) {
			item_array[ItemIndex]->common.STA = atoi(value);
		}
		else if (strstr(att, "cha")) {
			item_array[ItemIndex]->common.CHA = atoi(value);
		}
		else if (strstr(att, "dex")) {
			item_array[ItemIndex]->common.DEX = atoi(value);
		}
		else if (strstr(att, "int")) {
			item_array[ItemIndex]->common.INT = atoi(value);
		}
		else if (strstr(att, "agi")) {
			item_array[ItemIndex]->common.AGI = atoi(value);
		}
		else if (strstr(att, "wis")) {
			item_array[ItemIndex]->common.WIS = atoi(value);
		}
		else if (strstr(att, "mr")) {
			item_array[ItemIndex]->common.MR = atoi(value);
		}
		else if (strstr(att, "fr")) {
			item_array[ItemIndex]->common.FR = atoi(value);
		}
		else if (strstr(att, "cr")) {
			item_array[ItemIndex]->common.CR = atoi(value);
		}
		else if (strstr(att, "dr")) {
			item_array[ItemIndex]->common.DR = atoi(value);
		}
		else if (strstr(att, "pr")) {
			item_array[ItemIndex]->common.PR = atoi(value);
		}
		else if (strstr(att, "hp")) {
			item_array[ItemIndex]->common.HP = atoi(value);
		}
		else if (strstr(att, "mana")) {
			item_array[ItemIndex]->common.MANA = atoi(value);
		}
		else if (strstr(att, "ac")) {
			item_array[ItemIndex]->common.AC = atoi(value);
		}
		else if (strstr(att, "material")) {
			item_array[ItemIndex]->common.material = atoi(value);
		}
		else if (strstr(att, "damage")) {
			item_array[ItemIndex]->common.damage = atoi(value);
		}

		else if (strstr(att, "delay")) {
			item_array[ItemIndex]->common.delay = atoi(value);
		}
		else if (strstr(att, "effect")) {
			item_array[ItemIndex]->common.effecttype0 = atoi(value);
		}
		else if (strstr(att,"spellID")) {
			item_array[ItemIndex]->common.spellId0 = atoi(value);
		}

		else if (strstr(att,"color")) {
			item_array[ItemIndex]->common.color = atoi(value);
		}
		else if (strstr(att,"classes")) {
			uint16 tv = 0;
			if (strstr(value,"warrior"))
				tv += warrior_1;
			if (strstr(value,"paladin"))
				tv += paladin_1;
			if (strstr(value,"shadow"))
				tv += shadow_1;
			if (strstr(value,"ranger"))
				tv += ranger_1;
			if (strstr(value,"rogue"))
				tv += rogue_1;
			if (strstr(value,"monk"))
				tv += monk_1;
			if (strstr(value,"beastlord"))
				tv += beastlord_1;
			if (strstr(value,"wizard"))
				tv += wizard_1;
			if (strstr(value,"enchanter"))
				tv += enchanter_1;
			if (strstr(value,"mage"))
				tv += mage_1;
			if (strstr(value,"necromancer"))
				tv += necromancer_1;
			if (strstr(value,"shaman"))
				tv += shaman_1;
			if (strstr(value,"cleric"))
				tv += cleric_1;
			if (strstr(value,"bard"))
				tv += bard_1;
			if (strstr(value,"druid"))
				tv += druid_1;
			if (strstr(value,"all"))
				tv += call_1;
			item_array[ItemIndex]->common.classes = tv;
		}
		else if (strstr(att,"races")) {
			uint16 tv = 0;
			if (strstr(value,"human"))
				tv += human_1;
			if (strstr(value,"barbarian"))
				tv += barbarian_1;
			if (strstr(value,"erudite"))
				tv += erudite_1;
			if (strstr(value,"woodelf"))
				tv += woodelf_1;
			if (strstr(value,"highelf"))
				tv += highelf_1;
			if (strstr(value,"darkelf"))
				tv += darkelf_1;
			if (strstr(value,"halfelf"))
				tv += halfelf_1;
			if (strstr(value,"dwarf"))
				tv += dwarf_1;
			if (strstr(value,"troll"))
				tv += troll_1;
			if (strstr(value,"ogre"))
				tv += ogre_1;
			if (strstr(value,"halfling"))
				tv += halfling_1;
			if (strstr(value,"gnome"))
				tv += gnome_1;
			if (strstr(value,"iksar"))
				tv += iksar_1;
			if (strstr(value,"vahshir"))
				tv += vahshir_1;
			if (strstr(value,"all"))
				tv += rall_1;			
			item_array[ItemIndex]->common.normal.races = tv;
		}
		SaveItemToDatabase(item_array[ItemIndex]);
		return true;
	}
	return false;
#else
	return false;
#endif
}

int32 Database::GetZoneID(const char* zonename) {
	if (zonename_array == 0)
		return 0;
	if (zonename == 0)
		return 0;
	for (unsigned int i=0; i<=max_zonename; i++) {
		if (zonename_array[i] != 0 && strcasecmp(zonename_array[i], zonename) == 0) {
			return i;
		}
	}
	return 0;
}

const char* Database::GetZoneName(int32 zoneID, bool ErrorUnknown) {
	if (zonename_array == 0) {
		if (ErrorUnknown)
			return "UNKNOWN";
		else
			return 0;
	}
	if (zoneID <= max_zonename) {
		if (zonename_array[zoneID])
			return zonename_array[zoneID];
		else {
			if (ErrorUnknown)
				return "UNKNOWN";
			else
				return 0;
		}
	}
	if (ErrorUnknown)
		return "UNKNOWN";
	else
		return 0;
}

bool Database::CheckNameFilter(const char* name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT count(*) FROM name_filter WHERE '%s' like name", name), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (row[0] != 0) {
				if (atoi(row[0]) == 0) {

					mysql_free_result(result);
					return false;
				}
			}
		}
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "Error in CheckNameFilter query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return false;
}

bool Database::AddToNameFilter(const char* name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO name_filter (name) values ('%s')", name), errbuf, 0, &affected_rows)) {
		cerr << "Error in AddToNameFilter query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	delete[] query;
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

int32 Database::GetAccountIDFromLSID(int32 iLSID, char* oAccountName, sint16* oStatus) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, name, status FROM account WHERE lsaccount_id=%i", iLSID), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 account_id = atoi(row[0]);
			if (oAccountName)
				strcpy(oAccountName, row[1]);
			if (oStatus)
				*oStatus = atoi(row[2]);
			mysql_free_result(result);
			return account_id;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAccountIDFromLSID query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	
	return 0;
}

bool Database::GetWaypoints(int16 grid, int8 num, char* data) {
    char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query,"SELECT type,type2,wp1,wp2,wp3,wp4,wp5,wp6,wp7,wp8,wp9,wp10,wp11,wp12,wp13,wp14,wp15,wp16,wp17,wp18,wp19,wp20,wp21,wp22,wp23,wp24,wp25,wp26,wp27,wp28,wp29,wp30,wp31,wp32,wp33,wp34,wp35,wp36,wp37,wp38,wp39,wp40,wp41,wp42,wp43,wp44,wp45,wp46,wp47,wp48,wp49,wp50 from grid where id = %i",grid),errbuf,&result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			strcpy(data, row[num]);
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetWaypoints query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}

void Database::AssignGrid(sint32 curx, sint32 cury, int16 id) {
    char *query = 0;
	RunQuery(query, MakeAnyLenString(&query,"UPDATE spawn2 set pathgrid = %i where x = %i && y = %i",id,curx,cury));
	delete[] query;
}


void Database::ModifyGrid(bool remove, int16 id, int8 type, int8 type2)
{
    char *query = 0;
	if (!remove)
	{
		RunQuery(query, MakeAnyLenString(&query,"INSERT INTO grid VALUES(%i,%i,%i,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)",id,type,type2));
		delete[] query;
	}
	else
	{
		RunQuery(query, MakeAnyLenString(&query,"DELETE FROM grid where id=%i",id));
		delete[] query;
	}
}

void Database::ModifyWP(int16 grid_id, int8 wp_num, float xpos, float ypos, float zpos, int32 script)
{
    char *query = 0;
	RunQuery(query, MakeAnyLenString(&query,"UPDATE grid set wp%i='%f %f %f %i' where id = %i",wp_num,xpos,ypos,zpos,script,grid_id));
	delete[] query;
}

int16 Database::GetFreeGrid() {
    char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query,"SELECT count(*) from grid"),errbuf,&result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int16 tmp=0;
			if (row[0]) tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetWaypoints query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}

void Database::DeleteGrid(int32 sg2, int16 grid_num, bool grid_too)
{
	char *query=0;
	RunQuery(query, MakeAnyLenString(&query,"insert into spawn2 set pathgrid='0' where spawngroupID='%i'", sg2));
	query = 0;
	if (grid_too)
	RunQuery(query, MakeAnyLenString(&query,"delete from grid where id='%i'", grid_num));
}

void Database::DeleteWaypoint(int16 grid_num, int32 wp_num)
{
	char *query=0;
	RunQuery(query, MakeAnyLenString(&query,"insert into grid set wp%i=\"0 0 0 0\" where id='%i'", wp_num, grid_num));
	query = 0;
}



int32 Database::AddWP(int32 sg2, int16 grid_num, int8 wp_num, float xpos, float ypos, float zpos, int32 pause, float xpos1, float ypos1, float zpos1, int type1, int type2)
{
	if (wp_num == 1) {
		if (GetFreeGrid()==0)
			grid_num = 1;
    char *query = 0;
	RunQuery(query, MakeAnyLenString(&query,"insert into grid set id='%i', type='%i', type2='%i', wp%i=\"%f %f %f %i\"",grid_num,type1,type2,wp_num,xpos1,ypos1,zpos1,pause));
	delete[] query;
    query = 0;
	RunQuery(query, MakeAnyLenString(&query,"update grid set wp%i=\"%f %f %f %i\" where id='%i'",wp_num+1,xpos,ypos,zpos,pause,grid_num));
	delete[] query;
    query = 0;
	RunQuery(query, MakeAnyLenString(&query,"update spawn2 set pathgrid='%i' where spawngroupID='%i'",grid_num,sg2));
	delete[] query;
	return grid_num;
	}
	else {
    char *query = 0;
	RunQuery(query, MakeAnyLenString(&query,"update grid set wp%i=\"%f %f %f %i\" where id='%i'",wp_num+1,xpos,ypos,zpos,pause,grid_num));
	delete[] query;
	}
	return 0;
}

bool Database::CreateSpawn2(int32 spawngroup, const char* zone, float heading, float x, float y, float z, int32 respawn, int32 variance)
{
	char errbuf[MYSQL_ERRMSG_SIZE];

    char *query = 0;
	int32 affected_rows = 0;
	
	//	if(GetInverseXY()==1) {
	//		float temp=x;
	//		x=y;
	//		y=temp;
	//	}
	if (RunQuery(query, MakeAnyLenString(&query, "INSERT INTO spawn2 (spawngroupID,zone,x,y,z,heading,respawntime,variance) Values (%i, '%s', %f, %f, %f, %f, %i, %i)", spawngroup, zone, x, y, z, heading, respawn, variance), errbuf, 0, &affected_rows)) {
		delete[] query;
		if (affected_rows == 1) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		cerr << "Error in CreateSpawn2 query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}

	
	return false;
}
int32	Database::GetMerchantData(int32 merchantid, int32 slot)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT item FROM merchantlist WHERE merchantid=%d and slot=%d", merchantid, slot), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
	}
	else {
		cerr << "Error in GetMerchantData query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return 0;
}
int32	Database::GetMerchantListNumb(int32 merchantid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT item FROM merchantlist WHERE merchantid=%d", merchantid), errbuf, &result)) {
		delete[] query;
		int32 tmp = mysql_num_rows(result);
		mysql_free_result(result);
		return tmp;
	}
	else {
		cerr << "Error in GetMerchantListNumb query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return 0;
}

bool Database::UpdateName(const char* oldname, const char* newname) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;
	
	cout << "Renaming " << oldname << " to " << newname << "..." << endl;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET name='%s' WHERE name='%s';", newname, oldname), errbuf, 0, &affected_rows)) {
		delete[] query;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
}

// If the name is used or an error occurs, it returns false, otherwise it returns true
bool Database::CheckUsedName(const char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
    MYSQL_RES *result;
	//if (strlen(name) > 15)
	//	return false;
	if (!RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM character_ where name='%s'", name), errbuf, &result)) {
		cerr << "Error in CheckUsedName query '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return false;
	}
	else { // It was a valid Query, so lets do our counts!
		safe_delete(query);
		int32 tmp = mysql_num_rows(result);
		mysql_free_result(result);
		if (tmp > 0) // There is a Name!  No change (Return False)
			return false;
		else // Everything is okay, so we go and do this.
			return true;
	}
}
bool Database::GetTradeRecipe(Combine_Struct* combin,int16 usedskill, int16* product,int16* product2,int16* failproduct, int16* skillneeded,int16* trivial,int16* productcount)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
	char buf1[2000];
	char buf2[200];
	
	int32 tmp1 = 0;
	int32 tmp2 = 0;
	int32 check = 0;
	for (int i=0;i != 10; i++)
	{
		if (combin->iteminslot[i] != 0xFFFF)
		{
			if (tmp1 == 0)
				tmp1 = snprintf(buf1,200,"SELECT skillneeded,trivial,product,product2,failproduct,productcount,i1,i2,i3,i4,i5,i6,i7,i8,i9,i10 FROM tradeskillrecipe WHERE tradeskill = %i ", usedskill);
			else
			{
				tmp2 = snprintf(buf2,5, " ");
				memcpy(&buf1[tmp1],&buf2,tmp2);
				tmp1 = tmp1 + tmp2;
			}
			// if there is a better way to make this query tell me :)
			tmp2 = snprintf(buf2,200, " AND (i1 = %i or i2 = %i or i3 = %i or i4 = %i or i5= %i or i6 = %i or i7 = %i or i8 = %i or i9 = %i or i10 = %i)"
				,combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i]);
			memcpy(&buf1[tmp1],&buf2,tmp2);
			tmp1 = tmp1 + tmp2;
			check = check + combin->iteminslot[i];
			const Item_Struct* item = GetItem(combin->iteminslot[i]);
			cout << "Item: " << item->name << endl;
		}
	}
	tmp2 = snprintf(buf2,50, " having sum(i1)>=0");
	memcpy(&buf1[tmp1],&buf2,tmp2);
	tmp1 = tmp1 + tmp2;
	buf1[tmp1] = 0;
	//cout << "EndQuery: " << buf1 << endl;	
	
	if (RunQuery(buf1, tmp1, errbuf, &result)) {
		tmp2 =mysql_num_rows(result);
		if (tmp2 == 1)
		{			
			row = mysql_fetch_row(result);			
			int32 check2 = 0;	//just to make sure that quantity is correct)
			for (int i=6;i != 16;i++)
				check2 = check2 + atoi(row[i]); 
			if (check2 != check)
			{
				mysql_free_result(result);
				return false;
			}	
			*skillneeded = atoi(row[0]);
			*trivial = atoi(row[1]);
			*product = atoi(row[2]);
			*product2 = atoi(row[3]);
			*failproduct = atoi(row[4]);
			*productcount = atoi(row[5]);
			mysql_free_result(result);
			return true;
		}
		else if (tmp2 == 0)
		{

			mysql_free_result(result);
			return false;
		}
		else

			if (tmp2 > 1)
			{
				cout << "Combine error: Recipe is not unique!" << endl;
				mysql_free_result(result);
				return false;
			}
	}
	else {
		cerr << "Error in GetTradeRecept query '" << buf2 << "' " << errbuf << endl;
	}	
	return false;
}

char* Database::GetBook(char txtfile[14])
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	char* txtout = 0;
	char txtfile2[14];
        strncpy(txtfile2,txtfile+1,14); 
	if (strlen(txtfile2) > 14)
		return txtout;
	if (!RunQuery(query, MakeAnyLenString(&query, "SELECT txtfile FROM books where name='%s'", txtfile2), errbuf, &result)) {
		cerr << "Error in GetBook query '" << query << "' " << errbuf << endl;
		if (query != 0)
			delete[] query;

		return txtout;
	}
	else {
		if (mysql_num_rows(result) == 0) {
			mysql_free_result(result);
			cout << "DID NOT Send book!\n";
			return txtout;
		}
		else {
			row = mysql_fetch_row(result);
			mysql_free_result(result);
			txtout = (char*) row[0];
			cout << "Sent book!\n";
			return txtout;
		}
	}
}

bool Database::UpdateZoneSafeCoords(const char* zonename, float x=0, float y=0, float z=0) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE zone SET safe_x='%f', safe_y='%f', safe_z='%f' WHERE short_name='%s';", x, y, z, zonename), errbuf, 0, &affected_rows)) {
		delete[] query;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
}
int8 Database::GetServerType()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT value FROM variables WHERE varname='ServerType'"), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int8 ServerType = atoi(row[0]);
			mysql_free_result(result);
			return ServerType;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else

	{
		

		cerr << "Error in GetServerType query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return 0;
	
}

int8 Database::GetUseCFGSafeCoords()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT value FROM variables WHERE varname='UseCFGSafeCoords'"), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);

			int8 usecoords = atoi(row[0]);
			mysql_free_result(result);
			return usecoords;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else
	{
		
		cerr << "Error in GetUseCFGSafeCoords query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return 0;
	
}

bool Database::MoveCharacterToZone(char* charname, char* zonename) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET zonename = '%s', x=-1, y=-1, z=-1 WHERE name='%s'", zonename, charname), errbuf, 0,&affected_rows)) {
		cerr << "Error in MoveCharacterToZone(name) query '" << query << "' " << errbuf << endl;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0)
		return false;
	
	return true;
}

bool Database::MoveCharacterToZone(int32 iCharID, char* iZonename) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET zonename = '%s', x=-1, y=-1, z=-1 WHERE id=%i", iZonename, iCharID), errbuf, 0,&affected_rows)) {
		cerr << "Error in MoveCharacterToZone(id) query '" << query << "' " << errbuf << endl;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0)
		return false;
	
	return true;
}

int8 Database::CopyCharacter(const char* oldname, const char* newname, int32 acctid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	PlayerProfile_Struct* pp;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT profile, guild, guildrank FROM character_ WHERE name='%s'", oldname), errbuf, &result)) {
		safe_delete(query);
		//if (mysql_num_rows(result) == 3)
		//{
		row = mysql_fetch_row(result);
		pp = (PlayerProfile_Struct*)row[0];
		//			char* guild = row[1];
		//			char* guildrank = row[2];
		strcpy(pp->name,newname);
		mysql_free_result(result);
		//return usecoords;
		//}
		//else
		//{
		//	mysql_free_result(result);
		//	return 0;
		//}
		//mysql_free_result(result);
	}
	else {	
		cerr << "Error in CopyCharacter read query '" << query << "' " << errbuf << endl;
		safe_delete(query);
		return 0;
	}
	int32	affected_rows = 0;
	char query2[256+sizeof(PlayerProfile_Struct)*2+1];
	char* end=query2;
	end += sprintf(end, "INSERT INTO character_ SET zonename=\'%s\', x = %f, y = %f, z = %f, profile=",GetZoneName(pp->current_zone), pp->x, pp->y, pp->z);
    *end++ = '\'';
    end += DoEscapeString(end, (char*) pp, sizeof(PlayerProfile_Struct));
    *end++ = '\'';
    end += sprintf(end,", account_id=%d, name='%s'", acctid, newname);
	//if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO character_ SET name='%s', account_id='%d', profile='%s', guild='%s', guildrank='%s', x='%s', y='%s', z='%s', zonename='%s'", newname, acctid, pp, guild, guildrank, x, y, z, zonename), errbuf, 0,&affected_rows)) {
	//	cerr << "Error in CopyCharacter write query '" << query << "' " << errbuf << endl;
	//	return 0;
	//}
	if (!RunQuery(query2, (int32) (end - query2), errbuf, 0, &affected_rows)) {
        cerr << "Error in CreateCharacter query '" << query << "' " << errbuf << endl;
		return 0;
    }
	
	if (affected_rows == 0) {
		return 0;
	}
		
	return 1;
}

/*
Get the name of the alternate advancement skill with the given 'index'.
Return true if the name was found, otherwise false.
False will also be returned if there is a database error.
*/
int32 Database::GetAASkillVars(int32 skill_id, char* name_buf, int *cost, int *max_level)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	unsigned long* lengths;
	unsigned long len = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name, cost, max_level FROM altadv_vars WHERE skill_id=%i", skill_id), errbuf, &result)) {
		delete[] query;
		*name_buf = 0;
		*cost = 0;
		*max_level = 0;
		if (mysql_num_rows(result) == 1) {	

			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result);
			len = result->lengths[0];
			strcpy(name_buf, row[0]);  // memcpy(name_buf, row[0], len);
			*cost = atoi(row[1]);
			*max_level = atoi(row[2]);
		} else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
		return len;
	} else {
		cerr << "Error in GetAASkillVars '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	
	//return true;
}

/*
Update the player alternate advancement table for the given account "account_id" and character name "name"
Return true if the character was found, otherwise false.
False will also be returned if there is a database error.
*/
bool Database::SetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(PlayerAA_Struct)*2+1];
	char* end = query;
	
	//if (strlen(name) > 15)
	//	return false;
	
	/*for (int i=0; i< 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z') && 
	(name[i] < '0' || name[i] > '9'))
	return 0;
}*/
	
	
	end += sprintf(end, "UPDATE character_ SET alt_adv=\'");
	end += DoEscapeString(end, (char*)aa, sizeof(PlayerAA_Struct));
	*end++ = '\'';
	end += sprintf(end," WHERE account_id=%d AND name='%s'", account_id, name);
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in SetPlayerAlternateAdv query " << errbuf << endl;
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

/*
Get the player alternate advancement table for the given account "account_id" and character name "name"
Return true if the character was found, otherwise false.
False will also be returned if there is a database error.
*/
int32 Database::GetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	/*for (int i=0; i< 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z') && 
	(name[i] < '0' || name[i] > '9'))
	return 0;
}*/
	
	unsigned long* lengths;
	unsigned long len = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT alt_adv FROM character_ WHERE account_id=%i AND name='%s'", account_id, name), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {	
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result);
			len = result->lengths[0];
			//if (lengths[0] == sizeof(PlayerAA_Struct)) {
			if(row[0] && lengths[0] >= sizeof(PlayerAA_Struct)) {
				memcpy(aa, row[0], sizeof(PlayerAA_Struct));
			} else { // let's support ghetto-ALTERed databases that don't contain any data in the alt_adv column
				memset(aa, 0, sizeof(PlayerAA_Struct));
				len = sizeof(PlayerAA_Struct);
			}
			//}
			//else {
			//cerr << "Player alternate advancement table length mismatch in GetPlayerAlternateAdv" << endl;
			//mysql_free_result(result);
			//return false;
			//}
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
		//		unsigned long len=result->lengths[0];
		return len;
	}
	else {
		cerr << "Error in GetPlayerAlternateAdv query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	
	//return true;
}

bool Database::SetHackerFlag(const char* accountname, const char* charactername, const char* hacked) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO hackers(account,name,hacked) values('%s','%s','%s')", accountname, charactername, hacked), errbuf, 0,&affected_rows)) {
		cerr << "Error in SetHackerFlag query '" << query << "' " << errbuf << endl;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
}

int32 Database::GetServerFilters(char* name, ServerSideFilters_Struct *ssfs) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;

    MYSQL_ROW row;
	
	
	unsigned long* lengths;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT serverfilters FROM account WHERE name='%s'", name), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {	
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result);
			if (lengths[0] == sizeof(ServerSideFilters_Struct)) {
				memcpy(ssfs, row[0], sizeof(ServerSideFilters_Struct));
			}
			else {
				cerr << "Player profile length mismatch in ServerSideFilters" << endl;
				mysql_free_result(result);
				return 0;
			}
		}
		else {
			mysql_free_result(result);
			return 0;

		}
		int32 len = lengths[0];
		mysql_free_result(result);
		return len;
	}
	else {
		cerr << "Error in ServerSideFilters query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	
	return 0;
}

bool Database::SetServerFilters(char* name, ServerSideFilters_Struct *ssfs) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(ServerSideFilters_Struct)*2+1];
	char* end = query;
	
	//if (strlen(name) > 15)
	//	return false;
	
	/*for (int i=0; i<strlen(name); i++)
	{
	if ((name[i] < 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z') && 
	(name[i] < '0' || name[i] > '9'))
	return 0;
}*/
	
	
	end += sprintf(end, "UPDATE account SET serverfilters=");
	*end++ = '\'';
    end += DoEscapeString(end, (char*)ssfs, sizeof(ServerSideFilters_Struct));
    *end++ = '\'';
    end += sprintf(end," WHERE name='%s'", name);
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in SetServerSideFilters query " << errbuf << endl;
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

bool Database::GetFactionIdsForNPC(sint32 nfl_id, LinkedList<struct NPCFaction*> *faction_list, sint32* primary_faction) {
	if (nfl_id <= 0) {
		(*faction_list).Clear();
		if (primary_faction)
			*primary_faction = nfl_id;
		return true;
	}
	const NPCFactionList* nfl = GetNPCFactionList(nfl_id);
	if (!nfl)
		return false;
	if (primary_faction)
		*primary_faction = nfl->primaryfaction;
	(*faction_list).Clear();
	for (int i=0; i<MAX_NPC_FACTIONS; i++) {
		struct NPCFaction *pFac;
		if (nfl->factionid[i]) {
			pFac = new struct NPCFaction;
			pFac->factionID = nfl->factionid[i];
			pFac->value_mod = nfl->factionvalue[i];
			if (nfl->primaryfaction == pFac->factionID)
				pFac->primary = true;
			else
				pFac->primary = false;
			faction_list->Insert(pFac);
		}
	}
	return true;
/*	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result; 
	MYSQL_ROW row; 
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT faction_id, value, primary_faction FROM npc_faction WHERE npc_id=%d", npc_id), errbuf, &result)) 
	{ 
		delete [] query; 
		
		while ((row = mysql_fetch_row(result))) 
		{ 
			struct NPCFaction *pFac; 
			
			pFac = new struct NPCFaction; 
			pFac->factionID = atoi(row[0]); 
			pFac->value_mod = atoi(row[1]); 
			if (atoi(row[2]) == 1) 
				pFac->primary = true; 
			else 
				pFac->primary = false; 
			faction_list->Insert(pFac); 
		} 
		mysql_free_result(result);
		return true; 
	} 
	else 
	{ 
		cerr << "Error in Database::GetFactionIdsForNPC query '" << query << "' " << errbuf << endl; 
		delete[] query; 
	} 
	return false; */
}

float Database::GetSafePoint(const char* short_name, const char* which) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	//	int buf_len = 256;
	//    int chars = -1;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT safe_%s FROM zone WHERE short_name='%s'", which, short_name), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			float ret = atof(row[0]);
			mysql_free_result(result);
			return ret;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetSafePoint query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return 0;
}

//New functions for timezone
int32 Database::GetZoneTZ(int32 zoneid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT timezone FROM zone WHERE zoneidnumber=%i", zoneid), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetZoneTZ query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return 0;
}

bool Database::SetZoneTZ(int32 zoneid, int32 tz) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE zone SET timezone=%i WHERE zoneidnumber=%i", tz, zoneid), errbuf, 0, &affected_rows)) {
		delete[] query;

		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in SetZoneTZ query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}
//End new timezone functions.


//New Class for bug tracking connection.  I felt it was safer to use a new class, with less methods available to connect
//to the shared database.  Also, this limits the possibilty of something accidently mixing up the connections by keeping
//it completely seperate.

#ifdef BUGTRACK
BugDatabase::BugDatabase()
{
 	#ifdef DEBUG
	cout << "Creating BugDatabase object!" << endl;
	#endif
	mysql_init(&mysqlbug);
	if (!mysql_real_connect(&mysqlbug, buguploadhost, buguploaduser, buguploadpassword,buguploaddatabase,0,NULL,0))
	{
	cerr << "Failed to connect to database: Error: " << mysql_error(&mysqlbug) << endl;
	}
	else
	{
	cout << "Connected!" << endl;
 	}

}

bool BugDatabase::RunQuery(const char* query, int32 querylen, char* errbuf, MYSQL_RES** result, int32* affected_rows, int32* errnum, bool retry) {
	if (errnum)
		*errnum = 0;
	if (errbuf)
		errbuf[0] = 0;
	bool ret = false;
	LockMutex lock(&MBugDatabase);
	if (mysql_real_query(&mysqlbug, query, querylen)) {
		if (mysql_errno(&mysqlbug) == CR_SERVER_LOST) {
			if (retry) {
				cout << "Database Error: Lost connection, attempting to recover...." << endl;
				ret = RunQuery(query, querylen, errbuf, result, affected_rows, errnum, false);
			}
			else {
				if (errnum)
					*errnum = mysql_errno(&mysqlbug);
				if (errbuf)
					snprintf(errbuf, MYSQL_ERRMSG_SIZE, "#%i: %s", mysql_errno(&mysqlbug), mysql_error(&mysqlbug));
				cout << "DB Query Error #" << mysql_errno(&mysqlbug) << ": " << mysql_error(&mysqlbug) << endl;
				ret = false;
			}
		}
		else {
			if (errnum)
				*errnum = mysql_errno(&mysqlbug);
			if (errbuf)
				snprintf(errbuf, MYSQL_ERRMSG_SIZE, "#%i: %s", mysql_errno(&mysqlbug), mysql_error(&mysqlbug));
#ifdef DEBUG
			cout << "DB Query Error #" << mysql_errno(&mysqlbug) << ": " << mysql_error(&mysqlbug) << endl;
#endif
			ret = false;
		}
	}
	else {
		if (affected_rows) {
			*affected_rows = mysql_affected_rows(&mysqlbug);
		}
		if (result && mysql_field_count(&mysqlbug)) {
			*result = mysql_store_result(&mysqlbug);
			if (*result) {
				ret = true;
			}
			else {
#ifdef DEBUG
				cout << "DB Query Error: No Result" << endl;
#endif
				if (errnum)
					*errnum = UINT_MAX;
				if (errbuf)
					strcpy(errbuf, "Database::RunQuery: No Result");
				ret = false;
			}
		}
		else {
			ret = true;
		}
	}
	return ret;
}


bool BugDatabase::UploadBug(const char* bugdetails, const char* version,const char* loginname) {
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query=0;
	//char* end=0;

        MYSQL_RES *result;
	//MYSQL_ROW row;
	
	#ifdef DEBUG
	cout << "Version is : " << version << endl;
	cout << "Loginname is : " << loginname << endl;
	#endif

	if (RunQuery(query, MakeAnyLenString(&query, "insert into eqemubugs set loginname='%s', version='%s', bugtxt='%s'", loginname, version, bugdetails), errbuf, &result))
	{
	delete[] query;
	mysql_free_result(result);
	//mysql_close(conn);
	cout << "Successful bug report" << endl;
	return true;
	}
	else
	{
	delete[] query;
	mysql_free_result(result);
	//mysql_close(conn);
	cout << "Bug Report submission failed." << endl;
	return false;
	}
	return true;
}

int32 BugDatabase::DoEscapeString(char* tobuf, const char* frombuf, int32 fromlen) {
	LockMutex lock(&MBugDatabase);
	return mysql_real_escape_string(&mysqlbug, tobuf, frombuf, fromlen);
}

BugDatabase::~BugDatabase()
{
  	#ifdef DEBUG	
	cout << "Destroying BugDatabase object" << endl;
	#endif
        mysql_close(&mysqlbug);
}

#endif

bool Database::InsertNewsPost(int8 type,const char* defender,const char* attacker,int8 result,const char* bodytext) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO guildwars_events set type=%i, defender='%s', attacker='%s', result=%i, body_text='%s'",type,defender,attacker,result,bodytext), errbuf, 0, &affected_rows))
	{
		delete[] query;
		return false;
	}
	delete[] query;
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

sint32 Database::DeleteStalePlayerCorpses() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	// 604800 seconds = 1 week
	if (!RunQuery(query, MakeAnyLenString(&query, "Delete from player_corpses where (UNIX_TIMESTAMP() - UNIX_TIMESTAMP(timeofdeath)) > 604800 and not timeofdeath=0"), errbuf, 0, &affected_rows)) {
		delete[] query;
		return -1;
	}
	delete[] query;
	
	return affected_rows;
}

sint32 Database::DeleteStalePlayerBackups() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	// 1209600 seconds = 2 weeks
	if (!RunQuery(query, MakeAnyLenString(&query, "Delete from player_corpses where (UNIX_TIMESTAMP() - UNIX_TIMESTAMP(ts)) > 1209600"), errbuf, 0, &affected_rows)) {
		delete[] query;
		return -1;
	}
	delete[] query;
	
	return affected_rows;
}

sint32 Database::GetNPCFactionListsCount(int32* oMaxID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), count(*) FROM npc_faction");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row != NULL && row[1] != 0) {
			sint32 ret = atoi(row[1]);
			if (oMaxID) {
				if (row[0])
					*oMaxID = atoi(row[0]);
				else
					*oMaxID = 0;
			}
			mysql_free_result(result);
			return ret;
		}
	}
	else {
		cerr << "Error in GetNPCFactionListsCount query '" << query << "' " << errbuf << endl;
		delete[] query;
		return -1;
	}
	
	return -1;
}

#ifdef SHAREMEM
extern "C" bool extDBLoadNPCFactionLists(sint32 iNPCFactionListCount, int32 iMaxNPCFactionListID) { return database.DBLoadNPCFactionLists(iNPCFactionListCount, iMaxNPCFactionListID); }
const NPCFactionList* Database::GetNPCFactionList(uint32 id) {
	return EMuShareMemDLL.NPCFactionList.GetNPCFactionList(id);
}

bool Database::LoadNPCFactionLists() {
	if (!EMuShareMemDLL.Load())
		return false;
	sint32 tmp = -1;
	int32 tmp_npcfactionlist_max;
	tmp = GetNPCFactionListsCount(&tmp_npcfactionlist_max);
	if (tmp < 0) {
		cout << "Error: Database::LoadNPCFactionLists-ShareMem: GetNPCFactionListsCount() returned < 0" << endl;
		return false;
	}
	npcfactionlist_max = tmp_npcfactionlist_max;
	bool ret = EMuShareMemDLL.NPCFactionList.DLLLoadNPCFactionLists(&extDBLoadNPCFactionLists, sizeof(NPCFactionList), &tmp, &npcfactionlist_max, MAX_NPC_FACTIONS);
	return ret;
}

bool Database::DBLoadNPCFactionLists(sint32 iNPCFactionListCount, int32 iMaxNPCFactionListID) {
	cout << "Loading NPC Faction Lists from database..." << endl;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), Count(*) FROM npc_faction");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) {
			if (atoi(row[0]) > iMaxNPCFactionListID) {
				cout << "Error: Insufficient shared memory to load NPC Faction Lists." << endl;
				cout << "Max(id): " << atoi(row[0]) << ", iMaxNPCFactionListID: " << iMaxNPCFactionListID << endl;
				cout << "Fix this by increasing the MMF_MAX_NPCFactionList_ID define statement" << endl;
				mysql_free_result(result);
				return false;
			}
			if (atoi(row[1]) != iNPCFactionListCount) {
				cout << "Error: number of NPCFactionLists in memshare doesnt match database." << endl;
				cout << "Count(*): " << atoi(row[1]) << ", iNPCFactionListCount: " << iNPCFactionListCount << endl;
				mysql_free_result(result);
				return false;
			}
			npcfactionlist_max = atoi(row[0]);
			mysql_free_result(result);
			NPCFactionList tmpnfl;
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, primaryfaction from npc_faction"), errbuf, &result)) {
				safe_delete(query);
				while((row = mysql_fetch_row(result))) {
					memset(&tmpnfl, 0, sizeof(NPCFactionList));
					tmpnfl.id = atoi(row[0]);
					tmpnfl.primaryfaction = atoi(row[1]);
					if (!EMuShareMemDLL.NPCFactionList.cbAddNPCFactionList(tmpnfl.id, &tmpnfl)) {
						mysql_free_result(result);
						LogFile->write(EQEMuLog::Error,"Database::DBLoadNPCFactionLists: !EMuShareMemDLL.NPCFactionList.cbAddNPCFactionList");
						LogFile->write(EQEMuLog::Error," id = %i, primaryfaction = %i", atoi(row[0]), atoi(row[1]));
						return false;
					}

					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in DBLoadNPCFactionLists query2 '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT npc_faction_id, faction_id, value FROM npc_faction_entries order by npc_faction_id"), errbuf, &result)) {
				safe_delete(query);
				sint8 i = 0;
				int32 curflid = 0;
				int32 tmpflid = 0;
				uint32 tmpfactionid[MAX_NPC_FACTIONS];
				memset(tmpfactionid, 0, sizeof(tmpfactionid));
				sint32 tmpfactionvalue[MAX_NPC_FACTIONS];
				memset(tmpfactionvalue, 0, sizeof(tmpfactionvalue));
				while((row = mysql_fetch_row(result))) {
					tmpflid = atoi(row[0]);
					if (curflid != tmpflid && curflid != 0) {
						if (!EMuShareMemDLL.NPCFactionList.cbSetFaction(curflid, tmpfactionid, tmpfactionvalue)) {
							mysql_free_result(result);
						    LogFile->write(EQEMuLog::Error,"Database::DBLoadNPCFactionLists: !EMuShareMemDLL.NPCFactionList.cbSetFaction(curid=%i, tmpid=%i)", 
                                        curflid, tmpflid);
							return false;
						}
						memset(tmpfactionid, 0, sizeof(tmpfactionid));
						memset(tmpfactionvalue, 0, sizeof(tmpfactionvalue));
						i = 0;
					}
					curflid = tmpflid;
					tmpfactionid[i] = atoi(row[1]);
					tmpfactionvalue[i] = atoi(row[2]);
					i++;
					if (i >= MAX_NPC_FACTIONS) {
						cerr << "Error in DBLoadNPCFactionLists: More than MAX_NPC_FACTIONS factions returned, flid=" << tmpflid << endl;
						i--;
					}
					Sleep(0);
				}
				if (tmpflid) {
					EMuShareMemDLL.NPCFactionList.cbSetFaction(curflid, tmpfactionid, tmpfactionvalue);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in DBLoadNPCFactionLists query3 '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
		}
		else {
			mysql_free_result(result);
			//return false;
		}
	}
	else {
		cerr << "Error in DBLoadNPCFactionLists query1 '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	return true;
}
#else
const NPCFactionList* Database::GetNPCFactionList(uint32 id) {
	if (id <= npcfactionlist_max && npcfactionlist_array)
		return npcfactionlist_array[id];
	return 0;
}

bool Database::LoadNPCFactionLists() {
	cout << "Loading NPC Faction Lists from database..." << endl;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int i;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), Count(*) FROM npc_faction");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) {
			npcfactionlist_max = atoi(row[0]);
			mysql_free_result(result);
			npcfactionlist_array = new NPCFactionList*[npcfactionlist_max+1];
			for (i=0; i<=npcfactionlist_max; i++)
				npcfactionlist_array[i] = 0;
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, primaryfaction from npc_faction"), errbuf, &result)) {
				safe_delete(query);
				while((row = mysql_fetch_row(result))) {
					i = atoi(row[0]);
					npcfactionlist_array[i] = new NPCFactionList;
					memset(npcfactionlist_array[i], 0, sizeof(NPCFactionList));
					npcfactionlist_array[i]->id = atoi(row[0]);
					npcfactionlist_array[i]->primaryfaction = atoi(row[1]);
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadNPCFactionLists query2 '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT npc_faction_id, faction_id, value FROM npc_faction_entries order by npc_faction_id"), errbuf, &result)) {
				safe_delete(query);
				int32 curflid = 0;
				int32 tmpflid = 0;
				while((row = mysql_fetch_row(result))) {
					tmpflid = atoi(row[0]);
					if (curflid != tmpflid && curflid != 0) {
						i = 0;
					}
					curflid = tmpflid;
					if (npcfactionlist_array[tmpflid]) {
						npcfactionlist_array[tmpflid]->factionid[i] = atoi(row[1]);
						npcfactionlist_array[tmpflid]->factionvalue[i] = atoi(row[2]);
					}
					i++;
					if (i >= MAX_NPC_FACTIONS) {
						cerr << "Error in DBLoadNPCFactionLists: More than MAX_NPC_FACTIONS factions returned, flid=" << tmpflid << endl;
						i--;
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadNPCFactionLists query3 '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
		}
		else {
			mysql_free_result(result);
			return false;
		}
	}
	else {
		cerr << "Error in LoadNPCFactionLists query1 '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	return true;
}
#endif

//Functions for weather
int8 Database::GetZoneW(int32 zoneid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT weather FROM zone WHERE zoneidnumber=%i", zoneid), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int8 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}

	else {
		cerr << "Error in GetZoneW query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return 0;
}

bool Database::SetZoneW(int32 zoneid, int8 w) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE zone SET weather=%i WHERE zoneidnumber=%i", w, zoneid), errbuf, 0, &affected_rows)) {
		delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in SetZoneW query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	return false;
}
//End weather functions.

const char* Database::GetItemLink(const Item_Struct* item) {
	static char ret[250];
	if (item) {
		if (item->item_nr >=10000)
			snprintf(ret, sizeof(ret), "%c00%i%s%c", 0x12, item->item_nr, item->name, 0x12);
		else
			snprintf(ret, sizeof(ret), "%c00%i %s%c", 0x12, item->item_nr, item->name, 0x12);
	}
	else {
		ret[0] = 0;
	}
	return ret;
}

// FIXME this can go into shared mem
int16 Database::GetTrainlevel(int16 eqclass, int8 skill_id) {
	// Returns the level eqclass gets skill_id
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT skill_%i FROM class_skill WHERE class=%i", skill_id, eqclass), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int8 tmp = atoi(row[0]);
			mysql_free_result(result);
			if (tmp >= 67) {
				LogFile->write(EQEMuLog::Error, "Database Error invalid skill entry:%i for class:%i in class_skill Table", skill_id, eqclass);
				tmp = 66;
			}
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Database Warning could not find skill entry:%i for class:%i in class_skill Table", skill_id, eqclass);
		delete[] query;
	}
	return 66; // Aka never
}

bool Database::InjectToRaw(){
#ifndef SHAREMEM
	cout<<"Starting"<<endl;
//
    if (1){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id) FROM items");
	
	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) { 
			max_item = atoi(row[0]);
			item_array = new Item_Struct*[max_item+1];
			for(unsigned int i=0; i<max_item; i++)
			{

				item_array[i] = 0;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT id,raw_data_last FROM items");
			
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				delete[] query;
				while((row = mysql_fetch_row(result)))
				{
					unsigned long* lengths;
					lengths = mysql_fetch_lengths(result);
					if (lengths[1] == sizeof(Item_Struct))
					{
						item_array[atoi(row[0])] = new Item_Struct;
						memcpy(item_array[atoi(row[0])], row[1], sizeof(Item_Struct));
					}
					else
					{
					    //cout<<"Fuck:"<<lengths[1]<<":"<<sizeof(Item_Struct)<<endl;
						// TODO: Invalid item length in database
						item_array[atoi(row[0])] = new Item_Struct;
						memset(item_array, 0x0, sizeof(Item_Struct));
						item_array[atoi(row[0])]->item_nr = atoi(row[0]);
				
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
    }
//
	
	uint32 max_item = 0;
	GetItemsCount(&max_item);
	//Item_Struct* u_item_array[max_item+1];
	for (uint32 i = 0; i < max_item; i++){
	//    (const Item_Struct*) u_item_array[i] = GetItem(i);
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	query = new char[256];
// Inject Field data
	cout<<"Done filling memmory: x=0 max_item="<<max_item<<endl;
	int count = 0;
	for (unsigned int x = 0; x < max_item; x++){
			if(!item_array[x]) {
			    continue;
			}
			count++;
			// Retrieve field data for current item
#if 0
MakeAnyLenString(&query, "SELECT RecLevel,ReqLevel,SkillModId,SkillModPercent,ElemDmgType,ElemDmg,BaneDMGBody,BaneDMGRace,BaneDMG FROM items WHERE ID=%i", item_array[x]->item_nr);
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				delete[] query;
				while((row = mysql_fetch_row(result)))
				{
				    item_array[x]->common.RecLevel = atoi(row[0]);
				    item_array[x]->common.ReqLevel = atoi(row[1]);
				    item_array[x]->common.skillModId = atoi(row[2]);
				    item_array[x]->common.skillModPercent = atoi(row[3]);
				    item_array[x]->common.ElemDmgType = atoi(row[4]);
				    item_array[x]->common.ElemDmg = atoi(row[5]);
				    item_array[x]->common.BaneDMGBody = atoi(row[6]);
				    item_array[x]->common.BaneDMGRace = atoi(row[7]);
				    item_array[x]->common.BaneDMG = atoi(row[8]);
				    for (int cur = 0; cur < 10; cur++){
					item_array[x]->common.unknown0282[cur] = 0;
				    }
				    for (int cur = 0; cur < 16; cur++){
					item_array[x]->common.unknown0296[cur] = 0;
				    }
				    for (int cur = 0; cur < 3; cur++){
					item_array[x]->common.unknown0316[cur] = 0;
}
				    
				    for (int cur = 0; cur < 2; cur++){
					item_array[x]->common.unknown0325[cur] = 0;
				    }
				    for (int cur = 0; cur < 22; cur++){
					item_array[x]->common.unknown0330[cur] = 0;
				    }
				    for (int cur = 0; cur < 5; cur++){
					item_array[x]->common.unknown0352[cur] = 0;
				    }
#endif
			// Retrieve field data for current item
			MakeAnyLenString(&query, "SELECT * FROM items WHERE ID=%i", item_array[x]->item_nr);
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				delete[] query;
				while((row = mysql_fetch_row(result)))
				{
				    // Normal
				    strncpy(item_array[x]->name, row[2], sizeof(item_array)-1);
				    //strncpy(item_array[x]->lore, row[?], sizeof(item_array)-1);
				    //strncpy(item_array[x]->idfile, row[?], sizeof(item_array)-1);
				    //item_array[x]->flag = atoi(row[?]);
				    item_array[x]->weight = atoi(row[44]);
				    //item_array[x]->nosave = atoi(row[?]);
				    //item_array[x]->nodrop = atoi(row[?]);
				    item_array[x]->size = atoi(row[46]);
				    //item_array[x]->type = atoi(row[?]);
				    //item_array[x]->item_nr = atoi(row[0]);
				    //item_array[x]->icon_nr = atoi(row[?]);
				    item_array[x]->equipableSlots = atoi(row[9]);
				    if (item_array[x]->cost == 0)
					item_array[x]->cost = 100000000;
				    // .common
				    
				    //
				    item_array[x]->common.effecttype = atoi(row[40]);
				    item_array[x]->common.spellId = atoi(row[41]);
				    item_array[x]->common.casttime = atoi(row[43]);
				    item_array[x]->common.skillModId = atoi(row[16]);
				    item_array[x]->common.skillModPercent = atoi(row[17]);
				    item_array[x]->common.BaneDMGRace = atoi(row[21]);
				    item_array[x]->common.BaneDMGBody = atoi(row[20]);
				    item_array[x]->common.BaneDMG = atoi(row[22]);
				    item_array[x]->common.RecLevel = atoi(row[37]);
				    item_array[x]->common.RecSkill = atoi(row[38]);
				    item_array[x]->common.ElemDmgType = atoi(row[18]);
				    item_array[x]->common.ElemDmg = atoi(row[19]);
				    item_array[x]->common.ReqLevel = atoi(row[39]);
				    item_array[x]->common.focusspellId = atoi(row[42]);
				    
				    // Zero unknowns in items we could just mem set ect
					int cur;
				    for (cur = 0; cur < 10; cur++){
					item_array[x]->common.unknown0282[cur] = 0;
				    }
				    for (cur = 0; cur < 16; cur++){
					item_array[x]->common.unknown0296[cur] = 0;
				    }
				    for (cur = 0; cur < 3; cur++){
					item_array[x]->common.unknown0316[cur] = 0;
}
				    
				    for (cur = 0; cur < 2; cur++){
					item_array[x]->common.unknown0325[cur] = 0;
				    }
				    for (cur = 0; cur < 22; cur++){
					item_array[x]->common.unknown0330[cur] = 0;
				    }
				    for (cur = 0; cur < 5; cur++){
					item_array[x]->common.unknown0352[cur] = 0;
				    }

				}
				mysql_free_result(result);
				// Put Items back into the DB
				// Insert the modified item back into the database
				char tQuery[sizeof(Item_Struct)*2+1 + 64];
				char tEscapedText[sizeof(Item_Struct)*2+1];
				//	long AffectedRows;
				
				if(DoEscapeString(tEscapedText, (const char *)item_array[x], sizeof(Item_Struct))) {
					sprintf(tQuery, "UPDATE items SET raw_data = '%s' WHERE ID=%d\n", tEscapedText, item_array[x]->item_nr);
					if(RunQuery(tQuery, strlen(tQuery), errbuf))
					{
					}
					else {
						cout<<"Save failed::"<<tQuery<<endl;
					}
				}
				else{
					cout<<"Escapestrign failed"<<endl;
				}
			}
			else {
				cout<<"Query failed on saveing"<<endl;
			}
	}
	cout<<"Loaded "<<count<<endl;
#endif // Sharemem
return true;
}

//Yeahlight: Keep track of failed login attempts. Return TRUE to lockout IP
bool Database::FailedLogin(char* ip)
{
	return false;
}

//Yeahlight: See if the account is locked out. Return 0 to permit the login process, return > 0 for suspensions
PERMISSION Database::CheckAccountLockout(char* ip)
{
	return PERMITTED;
}

//Yeahlight: Clears the lockout record for a specific IP, but it will remain in the DB for future, extended lockout consideration
bool Database::ClearAccountLockout(char* ip)
{
	return true;
}