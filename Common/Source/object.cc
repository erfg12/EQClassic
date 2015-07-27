// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errmsg.h>
#include <string>
#include "config.h"
#include "EQCUtils.hpp"

#include "../../Zone/Include/object.h" // TODO: fix this

#ifndef WIN32
	#include "unix.h"
#endif

#include "database.h"
#include "races.h"
#include "deity.h"



using namespace std;

using namespace EQC::Common;

// change starting zone
const char* ZONE_NAME = "qeynos";

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
	item_array = 0;
	max_item = 0;
	max_npc_type = 0;
	npc_type_array = 0;
	max_faction = 0;
	faction_array = 0;
	char host[200], user[200], passwd[200], database[200], buf[200], type[200];
	int items[4] = {0, 0, 0, 0};
	FILE *f;

	

	if (!(f = fopen ("db.ini", "r")))
	{
		printf ("Couldn't open the DB.INI file.\n");
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
		}
	}

	if (!items[0] || !items[1] || !items[2] || !items[3])
	{
		printf ("Incomplete DB.INI file.\n");
		printf ("Read README.TXT!\n");
		exit (1);
	}
	
	fclose (f);

	DBConnect(host, user, passwd, database);

	
}

/*
	Establish a connection to a mysql database with the supplied parameters
*/
Database::Database(const char* host, const char* user, const char* passwd, const char* database)
{
	item_array = 0;
	max_item = 0;
	max_npc_type = 0;
	npc_type_array = 0;
	max_faction = 0;
	faction_array = 0;
	
	DBConnect(host, user, passwd, database);
}

Database::~Database()
{
	int x;
	if (item_array != 0)
	{
		for (x=0; x <= max_item; x++)
		{
			if (item_array[x] != 0)
			{
				delete item_array[x];
			}
		}
		delete item_array;
	}

	if (faction_array != 0)
	{
		for (x=0; x <= max_faction; x++)
		{
			if (faction_array[x] != 0)
			{
				delete faction_array[x];
			}
		}
		delete faction_array;
	}

	if (npc_type_array != 0)
	{
		for (x=0; x <= max_npc_type; x++) 
		{
			if (npc_type_array[x] != 0)
			{
				delete npc_type_array[x];
			}
		}
		delete npc_type_array;
	}
	
}



/*
	Check if the character with the name char_name from ip address ip has
	permission to enter zone zone_name. Return the account_id if the client
	has the right permissions, otherwise return zero.
	Zero will also be returned if there is a database error.
*/
int32 Database::GetAuthentication(char* char_name, char* zone_name, int32 ip)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	//int32 newip = 83994816;
	int32 newip = ip;
	int32 tamanio = MakeAnyLenString(&query, "SELECT account_id FROM authentication WHERE char_name='%s' AND zone_name='%s' AND ip=%u AND UNIX_TIMESTAMP()-time < %i", char_name, zone_name, newip, AUTHENTICATION_TIMEOUT);
	int32 iresult = 0;

	if (RunQuery(query, tamanio, errbuf, &result))
	{
		if (mysql_num_rows(result) == 1) 
		{
			row = mysql_fetch_row(result);
			iresult = atoi(row[0]);
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetAuthentication query '" << query << "' " << errbuf << endl;
		iresult = 0;
	}
	
	delete[] query;

	return iresult;
}

/*
	Give the account with id "account_id" permission to enter zone "zone_name" from ip address "ip"
	with character "char_name". Return true if successful.
	False will be returned if there is a database error.
*/
bool Database::SetAuthentication(int32 account_id, char* char_name, char* zone_name, int32 ip)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	//int32 newip = 83994816;
	int32 newip = ip;

	if (!ClearAuthentication(account_id))
	{
		cerr << "Error in SetAuthentication query '" << query << "' " << errbuf << endl;
		return false;
	}

	int32 tamanio = MakeAnyLenString(&query, "INSERT INTO authentication SET account_id=%i, char_name='%s', zone_name='%s', ip=%u, time=UNIX_TIMESTAMP()", account_id, char_name, zone_name, newip);
	if (!RunQuery(query, tamanio, errbuf, 0, &affected_rows))
	{
		cerr << "Error in SetAuthentication query '" << query << "' " << errbuf << endl;
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

/*
	This function will return the zone name in the "zone_name" parameter.
	This is used when a character changes zone, the old zone server sets
	the authentication record, the world server reads this new zone	name.
	If there was a record return true, otherwise false.
	False will also be returned if there is a database error.
*/
bool Database::GetAuthentication(int32 account_id, char* char_name, char* zone_name, int32 ip)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	//int32 newip = 83994816;
	int32 newip = ip;

	bool bres = false;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT char_name, zone_name FROM authentication WHERE account_id=%i AND ip=%u AND UNIX_TIMESTAMP()-UNIX_TIMESTAMP(time) < %i", account_id, newip, AUTHENTICATION_TIMEOUT), errbuf, &result))
	{
		delete[] query;
		
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			strcpy(char_name, row[0]);
			strcpy(zone_name, row[1]);
			bres = true;
		}
		else
		{
			bres = false;
		}

		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetAuthentication query '" << query << "' " << errbuf << endl;
		bres = false;
	}

	return bres;
}


/*
	This function will remove the record in the authentication table for
	the account with id "accout_id"
	False will also be returned if there is a database error.
*/
bool Database::ClearAuthentication(int32 account_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	bool bres = false;
    
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM authentication WHERE account_id=%i", account_id), errbuf))
	{
		cerr << "Error in ClearAuthentication query '" << query << "' " << errbuf << endl;
		bres = false;
	}
	else
	{
		bres = true;
	}
	delete[] query;

	return bres;
}

/*
	Check if there is an account with name "name" and password "password"
	Return the account id or zero if no account matches.
	Zero will also be returned if there is a database error.
*/
int32 Database::CheckLogin(char* name, char* password)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;


	if(!IsAlphaNumericStringOnly(name))
	{
		return 0;
	}

	if(!IsAlphaNumericStringOnly(password))
	{
		return 0;
	}
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM account WHERE name='%s' AND password='%s'", name, password), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 id = atoi(row[0]);
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

int8 Database::CheckStatus(int32 account_id)
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
			int8 status = atoi(row[0]);
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

bool Database::CreateAccount(char* name, char* password, int8 status, int32 lsaccount_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	bool bret = false;

	if (RunQuery(query, MakeAnyLenString(&query, "INSERT INTO account SET name='%s', password='%s', status=%i, lsaccount_id=%i;",name,password,status, lsaccount_id), errbuf))
	{
		bret = true;
	}
	else
	{
		cerr << "Error in CreateAccount query '" << query << "' " << errbuf << endl;
		bret = false;
	}
	delete[] query;
	
	return bret; // Dark-Prince : 25/20/2007 : single return point
}

bool Database::DeleteAccount(char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	bool bret = false;

	cout << "Account Attempting to be deleted:" << name << endl;

	if (RunQuery(query, MakeAnyLenString(&query, "DELETE FROM account WHERE name='%s';",name), errbuf, 0, &affected_rows))
	{
		if (affected_rows == 1)
		{
			bret = true;
		}
	}
	else
	{
		cerr << "Error in DeleteAccount query '" << query << "' " << errbuf << endl;
		bret = false;
	}

	delete[] query;
	return bret; // Dark-Prince : 25/20/2007 : single return point
}

bool Database::SetGMFlag(char* name, int8 status)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;

	cout << "Account being GM Flagged:" << name << ", Level: " << (int16) status << endl;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET status=%i WHERE name='%s';", status, name), errbuf, 0, &affected_rows)) {
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

void Database::GetCharSelectInfo(int32 account_id, CharacterSelect_Struct* cs) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

    for(int i=0;i<10;i++) {
        strcpy(cs->name[i], "<none>");
        strcpy(cs->zone[i], ZONE_NAME);
        cs->level[i] = 0;
    }


	PlayerProfile_Struct* pp;
	int char_num = 0;
	unsigned long* lengths;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name,profile FROM character_ WHERE account_id=%i order by name", account_id), errbuf, &result)) {
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
				strcpy(cs->zone[char_num], pp->current_zone);

				// Coder_01 - REPLACE with item info when available.
				Item_Struct* item = 0;
				item = GetItem(pp->inventory[2]);
				if (item != 0) {
					cs->equip[char_num][0] = item->common.material;
					cs->cs_colors[char_num][0] = item->common.color;
				}
				item = GetItem(pp->inventory[17]);
				if (item != 0) {
					cs->equip[char_num][1] = item->common.material;
					cs->cs_colors[char_num][1] = item->common.color;
				}
				item = GetItem(pp->inventory[7]);
				if (item != 0) {
					cs->equip[char_num][2] = item->common.material;
					cs->cs_colors[char_num][2] = item->common.color;
				}
				item = GetItem(pp->inventory[10]);
				if (item != 0) {
					cs->equip[char_num][3] = item->common.material;
					cs->cs_colors[char_num][3] = item->common.color;
				}
				item = GetItem(pp->inventory[12]);
				if (item != 0) {
					cs->equip[char_num][4] = item->common.material;
					cs->cs_colors[char_num][4] = item->common.color;
				}
				item = GetItem(pp->inventory[18]);
				if (item != 0) {
					cs->equip[char_num][5] = item->common.material;
					cs->cs_colors[char_num][5] = item->common.color;
				}
				item = GetItem(pp->inventory[19]);
				if (item != 0) {
					cs->equip[char_num][6] = item->common.material;
					cs->cs_colors[char_num][6] = item->common.color;
				}

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

	if (strlen(name) > 15)
		return false;

	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT into character_ SET account_id=%i, name='%s', profile=NULL", account_id, name), errbuf)) {
		cerr << "Error in ReserveName query '" << query << "' " << errbuf << endl;
		if (query != 0)
			delete[] query;
		return false;
	}

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

	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE from character_ WHERE name='%s'", name), errbuf)) {
		cerr << "Error in DeleteCharacter query '" << query << "' " << errbuf << endl;
		if (query != 0)
			delete[] query;
		return false;
	}

	return true;
}

/*
	Create and store a character profile with the give parameters.
	This function will not check if the paramaters are legal for this class/race combination.
	Return true if successful. Return false if the name was not reserved for character creation
	for this "account_id".
	False will also be returned if there is a database error.
*/
bool Database::CreateCharacter(int32 account_id, char* name, int16 gender, int16 race, int16 class_, int8 str, int8 sta, int8 cha, int8 dex, int8 int_, int8 agi, int8 wis, int8 face)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(PlayerProfile_Struct)*2+1];
	char* end = query;
	int32 affected_rows = 0;

	if (strlen(name) > 15)
		return false;

	if(!IsAlphaNumericStringOnly(name))
	{
		return 0;
	}
	
	PlayerProfile_Struct pp;
	memset((char*)&pp, 0, sizeof(PlayerProfile_Struct));

// TODO: Get the right data into this without using this file
/*
FILE* fp;
fp = fopen("../zone/start_profile_packet", "r");
fread((char*)&pp, sizeof(PlayerProfile_Struct), 1, fp);
fclose(fp);
*/
	strcpy(pp.name, name);
	
	//Disgrace: temp storage place for face until we can figure out where it is stored...
	pp.face = face;
	
	pp.Surname[0] = 0;
	pp.race = race;
	pp.class_ = class_;
	pp.gender = gender;
	pp.level = 1;
	pp.exp = 0;
//	pp.pp_unknown5[0] = 0x05;
	pp.mana = 0; // TODO: This should be max mana
//	pp.pp_unknown6[0] = 0x02;
//	pp.pp_unknown6[48] = 0x14;
//	pp.pp_unknown6[50] = 0x05;
	pp.cur_hp = 20; // TODO: This should be max hp
	pp.STR = str;
	pp.STA = sta;
	pp.CHA = cha;
	pp.DEX = dex;	
	pp.INT = int_;
	pp.AGI = agi;
	pp.WIS = wis;
	
	int i = 0;

	for (i=0;i<24;i++) 
		pp.languages[i] = 100;

	for (i=0;i < sizeof(pp.inventory); i++) 
	{ 
		pp.inventory[i] = 0xffff; 
	} 
	for (i=0;i<sizeof(pp.containerinv); i++) 
	{ 
		pp.containerinv[i] = 0xffff; 
	} 
	for (i=0;i<sizeof(pp.cursorbaginventory); i++) 
	{ 
		pp.cursorbaginventory[i] = 0xffff; 
	} 
	for (i=0;i<sizeof(pp.bank_inv); i++) 
	{ 
		pp.bank_inv[i] = 0xffff; 
	} 
	for (i=0;i<sizeof(pp.bank_cont_inv); i++) 
	{ 
		pp.bank_cont_inv[i] = 0xffff; 
	} 

	for (i=0;i<15; i++)
		pp.buffs[i].spellid = 0xffff;

	for (i=0;i<256; i++)
		pp.spell_book[i] = 0xffff;
	for (i=0;i<8; i++)
		pp.spell_memory[i] = 0xffff;

/*
	pp.pp_unknown8[58] = 0x07;
	pp.pp_unknown8[59] = 0x27;
	pp.pp_unknown8[60] = 0x06;
	pp.pp_unknown8[61] = 0x27;
	pp.pp_unknown8[62] = 0x0d;
	pp.pp_unknown8[63] = 0x27;
	pp.pp_unknown8[64] = 0xfb;
	pp.pp_unknown8[65] = 0x26;
	pp.pp_unknown8[66] = 0x04;
	pp.pp_unknown8[67] = 0x27;
	pp.pp_unknown8[68] = 0x02;
	pp.pp_unknown8[69] = 0x27;
	pp.pp_unknown8[70] = 0x1e;
	pp.pp_unknown8[71] = 0x49;
	pp.pp_unknown8[72] = 0x0c;
	pp.pp_unknown8[73] = 0x49;
*/

	char tmp[16];
	// Pyro: Get zone start locs from database, if they don't exist set to defaults.
	if (!GetVariable("startzone", tmp, 16))
		strcpy(pp.current_zone, ZONE_NAME);
	else
		strncpy(pp.current_zone, tmp, 16);

	if (!GetSafePoints(pp.current_zone, &pp.x, &pp.y, &pp.z)) {
		pp.x = 0;
		pp.y = 0;
		pp.z = 5;
	}
	strcpy(pp.bind_point_zone, pp.current_zone);

	pp.bind_location[0][0] = pp.x;
	pp.bind_location[1][0] = pp.y;
	pp.bind_location[2][0] = pp.z;

	pp.platinum = 10;
	pp.gold = 20;
	pp.silver = 30;
	pp.copper = 40;
	pp.platinum_bank = 15;
	pp.gold_bank = 25;
	pp.silver_bank = 35;
	pp.copper_bank = 45;
	memset(pp.GroupMembers[0], 0, 6*sizeof(pp.GroupMembers[0]));

	for (i=0;i<74; i++)
		pp.skills[i] = i;	
	strcpy(pp.bind_point_zone, ZONE_NAME);
	for (i=0;i<4; i++)
		strcpy(pp.start_point_zone[i], ZONE_NAME);

    end += sprintf(end, "UPDATE character_ SET profile=");
    *end++ = '\'';
    end += DoEscapeString(end, (char*)&pp, sizeof(PlayerProfile_Struct));
    *end++ = '\'';
    end += sprintf(end," WHERE account_id=%d AND name='%s' AND profile IS NULL", account_id, name);

    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in CreateCharacter query '" << query << "' " << errbuf << endl;
		return false;
    }

	if (affected_rows == 0) {
		return false;
	}

	return true;
}

bool Database::CreateCharacter(int32 account_id, PlayerProfile_Struct* pp) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(PlayerProfile_Struct)*2+1];
	char* end = query;
	int32 affected_rows = 0;

	if (strlen(pp->name) > 15)
		return false;

	if(!IsAlphaNumericStringOnly(pp->name))
	{
		return 0;
	}

    end += sprintf(end, "UPDATE character_ SET profile=");
    *end++ = '\'';
    end += DoEscapeString(end, (char*) pp, sizeof(PlayerProfile_Struct));
    *end++ = '\'';
    end += sprintf(end," WHERE account_id=%d AND name='%s' AND profile IS NULL", account_id, pp->name);

    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in CreateCharacter query '" << query << "' " << errbuf << endl;
		return false;
    }

	if (affected_rows == 0) {
		return false;
	}

	return true;
}

/*
	Get the player profile for the given account "account_id" and character name "name"
	Return true if the character was found, otherwise false.
	False will also be returned if there is a database error.
*/
unsigned long Database::GetPlayerProfile(int32 account_id, char* name, PlayerProfile_Struct* pp)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	unsigned long* lengths;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT profile FROM character_ WHERE account_id=%i AND name='%s'", account_id, name), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {	
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result);
			//if (lengths[0] == sizeof(PlayerProfile_Struct)) {
				memcpy(pp, row[0], sizeof(PlayerProfile_Struct));
			//}
			//else {
				//cerr << "Player profile length mismatch in GetPlayerProfile" << endl;
				//mysql_free_result(result);
				//return false;
			//}
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
		unsigned long len=0;
		memcpy(&len,lengths, sizeof(unsigned long));
		return len;
	}
	else {
		cerr << "Error in GetPlayerProfile query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}

	//return true;
}

/*
	Update the player profile for the given account "account_id" and character name "name"
	Return true if the character was found, otherwise false.
	False will also be returned if there is a database error.
*/
bool Database::SetPlayerProfile(int32 account_id, char* name, PlayerProfile_Struct* pp)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(PlayerProfile_Struct)*2+1];
	char* end = query;


	if (strlen(name) > 15)
		return false;

    end += sprintf(end, "UPDATE character_ SET profile=");
    *end++ = '\'';
    end += DoEscapeString(end, (char*)pp, sizeof(PlayerProfile_Struct));
    *end++ = '\'';
    end += sprintf(end," WHERE account_id=%d AND name='%s'", account_id, name);

	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in SetPlayerProfile query " << errbuf << endl;
		return false;
    }

	if (affected_rows == 0) {
		return false;
	}

	return true;
}

/*
	This function returns the account_id that owns the character with
	the name "name" or zero if no character with that name was found
	Zero will also be returned if there is a database error.
*/
int32 Database::GetAccountIDByChar(char* charname)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT account_id FROM character_ WHERE name='%s'", charname), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]); // copy to temp var because gotta free the result before exitting this function
			mysql_free_result(result);
			return tmp;
		}
	}
	else {
		cerr << "Error in GetAccountIDByChar query '" << query << "' " << errbuf << endl;
		delete[] query;
	}

	return 0;
}


int32 Database::GetAccountIDByName(char* accname)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if(!IsAlphaNumericStringOnly(accname))
	{
		return 0;
	}

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM account WHERE name='%s'", accname), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]); // copy to temp var because gotta free the result before exitting this function
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

void Database::GetAccountName(int32 accountid, char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name FROM account WHERE id='%i'", accountid), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			strcpy(name, row[0]);
		}
		mysql_free_result(result);
	}
	else {
		delete[] query;
		cerr << "Error in GetAccountName query '" << query << "' " << errbuf << endl;
	}
}

void Database::GetCharacterInfo(char* name, int32* charid, int32* guilddbid, int8* guildrank)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, guild, guildrank FROM character_ WHERE name='%s'", name), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			*charid = atoi(row[0]);
			*guilddbid = atoi(row[1]);
			*guildrank = atoi(row[2]);
			mysql_free_result(result);
			return;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetCharacterInfo query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
}

// Gets variable from 'variables' table
bool Database::GetVariable(char* varname, char* varvalue, int16 varvalue_len) {
	if (strlen(varname) <= 1)
		return false;
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
}

bool Database::SetVariable(char* varname, char* varvalue) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "Update variables set value='%s' WHERE varname like '%s'", varvalue, varname), errbuf, 0, &affected_rows)) {
		safe_delete(query);
		if (affected_rows != 1) {
			if (RunQuery(query, MakeAnyLenString(&query, "Insert Into variables (varname, value) values ('%s', '%s')", varname, varvalue), errbuf, 0, &affected_rows)) {
				safe_delete(query);
				if (affected_rows == 1) {
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

bool Database::CheckZoneserverAuth(char* ipaddr) {
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

int32 Database::GetGuildEQID(int32 guilddbid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT eqid FROM guilds WHERE id=%i", guilddbid), errbuf, &result)) {
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
		cerr << "Error in GetGuildEQID query '" << query << "' " << errbuf << endl;
		delete[] query;
	}

	return 0xFFFFFFFF;
}

// Pyro: Get zone starting points from DB
bool Database::GetSafePoints(char* short_name, float* safe_x, float* safe_y, float* safe_z, int8* minstatus, int8* minlevel) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int buf_len = 256;
    int chars = -1;
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

// returns the EQID of the guild
int32 Database::CreateGuild(char* name, int32 leader) {
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

bool Database::RenameGuild(int32 guilddbid, char* name)
{
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

bool Database::GetZoneLongName(char* short_name, char** long_name, char* file_name, float* safe_x, float* safe_y, float* safe_z)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT long_name, file_name, safe_x, safe_y, safe_z FROM zone WHERE short_name='%s'", short_name), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			if (long_name != 0) {
				row = mysql_fetch_row(result);
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

	int32 iret = 0;


	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM guilds WHERE leader=%i", leader), errbuf, &result))
	{
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]);		
			iret = tmp;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetGuildDBIDbyLeader query '" << query << "' " << errbuf << endl;
	}

	delete[] query;
	return iret;
}

bool Database::SetGuildLeader(int32 guilddbid, int32 leader)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	bool bret = false;

	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE guilds SET leader=%i WHERE id=%i", leader, guilddbid), errbuf, 0, &affected_rows))
	{
		if (affected_rows == 1)
		{
			bret = true;
		}
		else
		{
			bret = false;
		}
	}
	else 
	{
		cerr << "Error in SetGuildLeader query '" << query << "' " << errbuf << endl;
		bret = false;
	}
	
	delete[] query;
	return bret; // Dark-Prince : 25/20/2007 : single return point
}

bool Database::SetGuildMOTD(int32 guilddbid, char* motd)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	char* motdbuf = 0;
	int32 affected_rows = 0;
	bool bret = false;

	motdbuf = new char[(strlen(motd)*2)+3];
	DoEscapeString(motdbuf, motd, strlen(motd)) ;

	if (RunQuery(query, MakeAnyLenString(&query, "Update guilds set motd='%s' WHERE id=%i;", motdbuf, guilddbid), errbuf, 0, &affected_rows))
	{
		if (affected_rows == 1)
		{
			bret = true;
		}
		else
		{
			bret = false;
		}
	}
	else
	{
		cerr << "Error in RenameGuild query '" << query << "' " << errbuf << endl;
		bret = false;
	}

	delete[] query;
	delete motdbuf;

	return bret; // Dark-Prince : 25/20/2007 : single return point
}

bool Database::GetGuildMOTD(int32 guilddbid, char* motd)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	bool bret = false;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT motd FROM guilds WHERE id=%i", guilddbid), errbuf, &result))
	{
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);

			if (row[0] == 0)
			{
				strcpy(motd, "");
			}
			else
			{
				strcpy(motd, row[0]);
			}
			bret = true;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetGuildMOTD query '" << query << "' " << errbuf << endl;
	}

	delete[] query;
	return false;
}
/*
bool Database::LoadItems_Old()
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
		if (row != 0 && row[0] > 0)
		{ 
			max_item = atoi(row[0]);
			item_array = new Item_Struct*[max_item+1];
			for(int i=0; i<max_item; i++)
			{
				item_array[i] = 0;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT id,raw_data FROM items");

			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				delete[] query;
				while(row = mysql_fetch_row(result))
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
						cout << "Invalid items in database..." << endl;
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in PopulateZoneLists query '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}


	return true;
}


*/
bool Database::LoadItems()
{
	string legs = string("Spiderling Legs");
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[2000];
	strcpy(query, "SELECT MAX(id) FROM items");

	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row != 0 && row[0] > 0)
		{ 
			max_item = atoi(row[0]);
			item_array = new Item_Struct*[max_item+1];
			for(int i=0; i<max_item; i++)
			{
				item_array[i] = 0;
			}
			mysql_free_result(result);
			//TODO: fix \"range\" as it is a mysql reserved word..... - Dark-Prince 31/12/2007
			// Best bet is to rename the colunm

									//		0			1			2			3			4
			MakeAnyLenString(&query, "SELECT id,		name,		aagi,		ac,			accuracy,\
											acha,		adex,		aint,		asta,		astr,\
											attack,		awis,		bagsize,	bagslots,	bagtype,\
											bagwr,		bardtype,	bardvalue,	book,		casttime, casttime_,\
											charmfile,	charmfileid,classes,	color,		price,\
											cr,			damage,		damageshield,deity,		delay,\
											dotshielding,dr,		clicktype,	clicklevel2,filename,\
											fr,			fvnodrop,	haste,		clicklevel,	hp,\
											regen,		icon,		idfile,		itemclass,itemtype,\
											light,		lore,		loreflag,	magic,		mana,\
											manaregen,	material,	maxcharges,	mr,			nodrop,\
											norent,		pendingloreflag,pr,		races,		range,\
											reclevel,	recskill,	reqlevel,	sellrate,	shielding,\
											size,		skillmodtype,skillmodvalue,slots,clickeffect,\
											spellshield,stunresist,	summonedflag,tradeskills,weight,\
											booktype,	recastdelay,recasttype,	attuneable,	nopet,\
											pointtype,	potionbelt,	stacksize,	proceffect,	proctype,\
											 proclevel2,proclevel,	worneffect,	worntype,	wornlevel2,\
											wornlevel,	focustype,	focuslevel2,focuslevel,	scrolleffect,\
											scrolltype,	scrolllevel2,scrolllevel FROM items");

			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				delete[] query;
				while(row = mysql_fetch_row(result))
				{
	
					item_array[atoi(row[0])] = new Item_Struct;
					memset(item_array[atoi(row[0])], 0, sizeof(Item_Struct));

					//Standard item properties set for ALL items:
					strcpy(item_array[atoi(row[0])]->name, row[1]);
					strcpy(item_array[atoi(row[0])]->idfile, row[43]);
					strcpy(item_array[atoi(row[0])]->lore, row[47]);
					item_array[atoi(row[0])]->cost = atoi(row[25]);
					item_array[atoi(row[0])]->icon_nr = atoi(row[42]);

					//The client only allows 2 bytes for unique item numbers.
					if(atoi(row[0]) > 65450){
						break;
					}else{
						item_array[atoi(row[0])]->item_nr = atoi(row[0]);
					}

					item_array[atoi(row[0])]->equipableSlots = atoi(row[69]);
					item_array[atoi(row[0])]->equipSlot  = atoi(row[69]);		
					item_array[atoi(row[0])]->nodrop = atoi(row[55]);
					item_array[atoi(row[0])]->nosave = -1;						
					item_array[atoi(row[0])]->weight = atoi(row[75]);
					item_array[atoi(row[0])]->size = atoi(row[66]);

					//Begin flag checks.
					
					//Check for book.
					if(atoi(row[18]) == 1){	
						//Is book.
						strcpy(item_array[atoi(row[0])]->book.file, row[35]);
						item_array[atoi(row[0])]->flag = 0x7669;	
						item_array[atoi(row[0])]->type = 0x02;
					}else{
						//This is a common item. Set common attribs.
						item_array[atoi(row[0])]->common.AC			=	atoi(row[3]);
						item_array[atoi(row[0])]->common.AGI		=	atoi(row[2]);
						item_array[atoi(row[0])]->common.casttime	=	atoi(row[19]);
						item_array[atoi(row[0])]->common.CHA		=	atoi(row[5]);
						item_array[atoi(row[0])]->common.classes	=	atoi(row[23]);
						item_array[atoi(row[0])]->common.color		=	atoi(row[24]);
						item_array[atoi(row[0])]->common.CR			=	atoi(row[26]);
						item_array[atoi(row[0])]->common.damage		=	atoi(row[27]);
						item_array[atoi(row[0])]->common.delay		=	atoi(row[30]);
						item_array[atoi(row[0])]->common.DEX		=	atoi(row[6]);
						item_array[atoi(row[0])]->common.DR			=	atoi(row[32]);
						item_array[atoi(row[0])]->common.FR			=	atoi(row[36]);
						item_array[atoi(row[0])]->common.HP			=	atoi(row[40]);
						item_array[atoi(row[0])]->common.INT		=	atoi(row[7]);
						item_array[atoi(row[0])]->common.level		=	atoi(row[87]);
						item_array[atoi(row[0])]->common.level0		=	atoi(row[87]);
						item_array[atoi(row[0])]->common.light		=	atoi(row[46]);
						item_array[atoi(row[0])]->common.magic		=	atoi(row[49]);
						item_array[atoi(row[0])]->common.MANA		=	atoi(row[50]);
						item_array[atoi(row[0])]->common.material	=	atoi(row[52]);
						item_array[atoi(row[0])]->common.MR			=	atoi(row[54]);
						item_array[atoi(row[0])]->common.PR			=	atoi(row[58]);
						item_array[atoi(row[0])]->common.range		=	atoi(row[60]);
						item_array[atoi(row[0])]->common.skill		=	atoi(row[62]);
						item_array[atoi(row[0])]->common.click_effect_id	=	atoi(row[70]);
						item_array[atoi(row[0])]->common.spell_effect_id	=	atoi(row[95]);
						item_array[atoi(row[0])]->common.STA		=	atoi(row[8]);
						item_array[atoi(row[0])]->common.STR		=	atoi(row[9]);
						item_array[atoi(row[0])]->common.WIS		=	atoi(row[11]);
						

						//if (atoi(row[45]) > 4) //Jester: Weapon check(0=1hs,1=2hs,2=piercing,3=1hb,4=2hb)
							item_array[atoi(row[0])]->common.skill	= atoi(row[45]);  //This is the item type.


						int container_type = atoi(row[14]);

						if(container_type == 0){								//If > 0, item is a bag.	
							//================== Union ==============================================//
							item_array[atoi(row[0])]->common.normal.races = atoi(row[59]);
							item_array[atoi(row[0])]->flag = 0x3900;
		
							//All stackable item_types need to go in this check.
							//Item types: 17 common, 27 arrows, 14 food, 15 drink, 37 fishing bait, 38 alcochol, probably more...
							if(atoi(row[45]) == 17 || atoi(row[45]) == 27 || atoi(row[45]) == 14 || atoi(row[45]) == 15 || atoi(row[45]) == 37 || atoi(row[45]) == 38){		
								
								item_array[atoi(row[0])]->common.stackable			= 0x01;
								item_array[atoi(row[0])]->common.number_of_items	= 0x01;
								item_array[atoi(row[0])]->common.normal.charges		= 0x01;  // ?
								//item_array[atoi(row[0])]->common.magic	= 0x0c;
							
							//Non stackable item.
							//Item types: 20 spells, 21 potions, ...
							}else{
								if(atoi(row[45]) == 20){
									item_array[atoi(row[0])]->flag = 0x0036;
									item_array[atoi(row[0])]->common.skill		=   atoi(row[45]);  //tried up to 5 including 5
									item_array[atoi(row[0])]->common.spell_effect_id	=	atoi(row[95]);

								}

								//item_array[atoi(row[0])]->flag = 0x315f;
							}

							item_array[atoi(row[0])]->type = 0x00;
							//=======================================================================//
						}
						else{
							
							if(container_type == 10){							//Regular container.
								item_array[atoi(row[0])]->flag = 0x5450;	
							}
							else if(container_type == 30){						//Tradeskill container.
								item_array[atoi(row[0])]->flag = 0x5400;
							}
							else{												//There are more that need to be implimented.
								item_array[atoi(row[0])]->flag = 0x5450;		//Set to normal for now.
							}

							item_array[atoi(row[0])]->type = 0x01;

							//=================== OR ================================================//
							item_array[atoi(row[0])]->common.container.sizeCapacity		=	atoi(row[12]);
							item_array[atoi(row[0])]->common.container.weightReduction	=	atoi(row[15]);		//Confirmed.
							item_array[atoi(row[0])]->common.container.numSlots			=	atoi(row[13]);
							item_array[atoi(row[0])]->common.container.unknown0212		=	atoi(row[14]);
							item_array[atoi(row[0])]->common.container.unknown0214		=	atoi(row[14]);
							//=======================================================================//
						}
						
						if(atoi(row[33]) == 1)			//1 = Potion,...						//Click Type.
						{
							//================== Union ==============================================//
							item_array[atoi(row[0])]->common.charges					=	5;
							item_array[atoi(row[0])]->common.effecttype					=	1;
							item_array[atoi(row[0])]->common.normal.charges				=	10;
							item_array[atoi(row[0])]->common.number_of_items			=	atoi(row[53]);
							item_array[atoi(row[0])]->common.click_effect_id			=	atoi(row[70]);
							
						}else{
							//=================== OR ================================================//
																									 //No proc, so this means stack number.
							
							//=======================================================================//						
						}


						string test2 = string(item_array[atoi(row[0])]->name);

						//For testing.
						if(legs.find(test2) != string::npos){
							APPLAYER tApp = APPLAYER(0x0000, sizeof(Item_Struct));
							memcpy(tApp.pBuffer,item_array[atoi(row[0])], sizeof(Item_Struct));
							//DumpPacketHex(&tApp);
						}
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

bool Database::LoadNPCTypes(char* zone_name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id) FROM npc_types");
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row != 0 && row > 0 && row[0] != 0)
		{
			max_npc_type = atoi(row[0]);
			npc_type_array = new NPCType*[max_npc_type+1];

			for (int x=0; x <= (int)max_npc_type; x++){
				npc_type_array[x] = 0;
			}

			mysql_free_result(result);

			//MakeAnyLenString(&query, "SELECT id,name,level,race,class,hp,gender,texture,helmtexture,size,loottable_id, merchant_id FROM npc_types");//WHERE zone='%s'", zone_name
			//This new way will only load the NPCS that are inside the current zone.
			MakeAnyLenString(&query, "SELECT npc_types.id,name,level,race,class,hp,gender,texture,helmtexture,size,\
									 loottable_id, merchant_id FROM npc_types inner join spawnentry on \
									 npc_types.id = spawnentry.npcID inner join spawn2 on \
									 spawn2.spawngroupID=spawnentry.spawngroupID where spawn2.zone='%s'", zone_name);
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				safe_delete(query);
				while(row = mysql_fetch_row(result))
				{
					npc_type_array[atoi(row[0])] = new NPCType;
					memset(npc_type_array[atoi(row[0])], 0, sizeof(NPCType));
					strncpy(npc_type_array[atoi(row[0])]->name, row[1], 30);
					npc_type_array[atoi(row[0])]->npc_id = atoi(row[0]); // rembrant, Dec. 2
					npc_type_array[atoi(row[0])]->level = atoi(row[2]);
					npc_type_array[atoi(row[0])]->race = atoi(row[3]);
					npc_type_array[atoi(row[0])]->class_ = atoi(row[4]);
					npc_type_array[atoi(row[0])]->cur_hp = atoi(row[5]);
					npc_type_array[atoi(row[0])]->max_hp = atoi(row[5]);
					npc_type_array[atoi(row[0])]->gender = atoi(row[6]);
					npc_type_array[atoi(row[0])]->texture = atoi(row[7]);
					npc_type_array[atoi(row[0])]->helmtexture = atoi(row[8]);
					npc_type_array[atoi(row[0])]->size = atof(row[9]);
					npc_type_array[atoi(row[0])]->loottable_id = atoi(row[10]);
					npc_type_array[atoi(row[0])]->merchanttype = atoi(row[11]);

					//Database says 41 is merchant class, but we use 32.
					if(atoi(row[4]) == 41){
						npc_type_array[atoi(row[0])]->class_ = 32;	
					}

					//Database has wrong race type for skelectons (they show as human w/o this fix).
					if(atoi(row[3]) == 367){
						npc_type_array[atoi(row[0])]->race = 60;
					}

					npc_type_array[atoi(row[0])]->STR = 75;
					npc_type_array[atoi(row[0])]->STA = 75;
					npc_type_array[atoi(row[0])]->DEX = 75;
					npc_type_array[atoi(row[0])]->AGI = 75;
					npc_type_array[atoi(row[0])]->WIS = 75;
					npc_type_array[atoi(row[0])]->INT = 75;
					npc_type_array[atoi(row[0])]->CHA = 75;
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else
			{
				cerr << "Error in LoadNPCTypes query '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
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

Item_Struct* Database::GetItem(uint32 id)
{
	if (item_array && id <= max_item)
	{
		return item_array[id];
	}
	else
	{
		return 0;
	}
}

NPCType* Database::GetNPCType(uint32 id)
{
	if (npc_type_array && id <= max_npc_type)
	{
		return npc_type_array[id]; 
	}
	else
	{
		return 0;
	}
}

bool Database::CheckNameFilter(char* name)
{
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

void Database::AddBugReport(int crashes, int duplicates, char* player, char* report)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32 affected_rows = 0;

	RunQuery(query, MakeAnyLenString(&query,"INSERT INTO bug_reports (causes_crash, can_duplicate, player, report) VALUES(%i,%i,'%s','%s');",
		duplicates, crashes, player, report),errbuf, 0, &affected_rows);
	delete[] query;
	return;
}

bool Database::AddToNameFilter(char* name)
{
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

int32 Database::GetAccountIDFromLSID(int32 lsaccount_id) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM account WHERE lsaccount_id=%i", lsaccount_id), errbuf, &result))
	{
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 account_id = atoi(row[0]);
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

bool Database::CreateSpawn2(int32 spawngroup, char* zone, float heading, float x, float y, float z, int32 respawn, int32 variance)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

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
bool Database::UpdateNpcSpawnLocation(int32 spawngroupid, float x, float y, float z, float heading) 
{ 
	        // maalanar: used to move a spawn location in spawn2 
	        char errbuf[MYSQL_ERRMSG_SIZE]; 
	    char *query = 0; 
	        int32   affected_rows = 0; 
	 
	        if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE spawn2 SET x='%f', y='%f', z='%f', heading='%f' WHERE spawngroupID='%d';", x, y, z, heading, spawngroupid), errbuf, 0, &affected_rows)) { 
	                cerr << "Error running query " << query << endl; 
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
bool Database::DeleteNpcSpawnLocation(int32 spawngroupid) 
	{ 
	        // maalanar: used to delete spawn info from spawn2 
	        char errbuf[MYSQL_ERRMSG_SIZE]; 
	    char *query = 0; 
	        int32   affected_rows = 0; 
	 
	        if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM spawn2 WHERE spawngroupid = '%d';", spawngroupid), errbuf, 0, &affected_rows)) { 
	                cerr << "Error running query " << query << endl; 
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

bool Database::UpdateName(char* oldname, char* newname)
{
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
bool Database::CheckUsedName(char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
    MYSQL_RES *result;
	if (strlen(name) > 15)
		return false;
	if (!RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM character_ where name='%s'", name), errbuf, &result)) {
		cerr << "Error in CheckUsedName query '" << query << "' " << errbuf << endl;
		if (query != 0)
			delete[] query;
		return false;
	}
	else { // It was a valid Query, so lets do our counts!
		int32 tmp = mysql_num_rows(result);
		mysql_free_result(result);
		if (tmp > 0) // There is a Name!  No change (Return False)
			return false;
		else // Everything is okay, so we go and do this.
			return true;
	}
}
bool Database::GetTradeRecipe(Combine_Struct* combin,int16 usedskill, int16* product, int16* skillneeded)
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
				tmp1 = snprintf(buf1,200,"SELECT skillneeded,product,i1,i2,i3,i4,i5,i6,i7,i8,i9,i10 FROM tradeskillrecipe WHERE ");
			else
			{
				tmp2 = snprintf(buf2,5, " AND ");
				memcpy(&buf1[tmp1],&buf2,tmp2);
				tmp1 = tmp1 + tmp2;
			}
					// if there is a better way to make this query tell me :)
			tmp2 = snprintf(buf2,200, "(i1 = %i or i2 = %i or i3 = %i or i4 = %i or i5= %i or i6 = %i or i7 = %i or i8 = %i or i9 = %i or i10 = %i)"
				,combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i],combin->iteminslot[i]);
			memcpy(&buf1[tmp1],&buf2,tmp2);
			tmp1 = tmp1 + tmp2;
			check = check + combin->iteminslot[i];
			Item_Struct* item = GetItem(combin->iteminslot[i]);
			cout << "Item: " << item->name << endl;
		}
	}
	tmp2 = snprintf(buf2,50, " AND tradeskill = %i",usedskill);
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
			for (int i=2;i != 12;i++)
				check2 = check2 + atoi(row[i]); 
			if (check2 != check)
			{
				mysql_free_result(result);
				return false;
			}	
			*skillneeded = atoi(row[0]);
			*product = atoi(row[1]);
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

/* Retrieves the text for a book or scroll from the database */
void Database::GetBook(char* txtfile, char* txtout)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT txtfile FROM books where name='%s'", txtfile), errbuf, &result))
	{
		if (mysql_num_rows(result) > 0)
		{
			row = mysql_fetch_row(result);
			
			if(row != 0)
			{
				strcpy(txtout,row[0]);
			}
			else
			{
				cout << "Could not locate book text for: " << txtfile << endl;
			}

			mysql_free_result(result);
		}
	}
	else
	{
		cerr << "Error in GetBook query '" << query << "' " << errbuf << endl;
	}


	delete[] query;
}

bool Database::UpdateZoneSafeCoords(char* zonename, float x=0, float y=0, float z=0)
{
	// we need sanity checking on zone name
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;

	bool bret = false;


	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE zone SET safe_x='%f', safe_y='%f', safe_z='%f' WHERE short_name='%s';", x, y, z, zonename), errbuf, 0, &affected_rows))
	{
		if (affected_rows > 0)
		{
			bret = true;
		}
	}

	delete[] query;
	return bret; // Dark-Prince : 25/20/2007 : single return point
}

bool Database::SetStartingItems(PlayerProfile_Struct *cc, int8 si_race, int8 si_class, char* si_name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	int i = 22;
	MYSQL_RES *result;
	MYSQL_ROW row;
	bool bret = false;

	cout<< "Loading starting items for: Race: " << (int) si_race << " Class: " << (int) si_class << " onto " << si_name << endl;

	if(RunQuery(query, MakeAnyLenString(&query, "SELECT itemid FROM starting_items WHERE race = %i AND class = %i ORDER BY id", (int) si_race, (int) si_class), errbuf, &result))
	{
		while(row = mysql_fetch_row(result))
		{
			cc->inventory[i] = atoi(row[0]);	
			i++;
		}
		mysql_free_result(result);
		bret = true;
	}
	else
	{
		cerr << "Error in SetStartingItems query '" << query << "' " << errbuf << endl;
		bret = true;
	}
	delete[] query;
	
	return bret; // Dark-Prince : 25/20/2007 : single return point
}

//o--------------------------------------------------------------
//| Name: GetNPCPrimaryFaction; rembrant, Dec. 16, 2001
//o--------------------------------------------------------------
//| Purpose: Retrieves the primary faction_id and value for an
//|          NPC.
//|          Returns false on failure.
//o--------------------------------------------------------------
bool Database::GetNPCPrimaryFaction(int32 npc_id, int32* faction_id, sint32* value)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query,"select faction_id, value FROM npc_faction_entries INNER JOIN npc_types \
												ON npc_types.npc_faction_id = npc_faction_entries.npc_faction_id \
												WHERE id=%i", npc_id),errbuf,&result)){
	//if (RunQuery(query, MakeAnyLenString(&query, "SELECT faction_id, value FROM npc_faction WHERE npc_id = %i AND primary_faction = 1;", npc_id), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			*faction_id = atoi(row[0]);
			*value = atoi(row[1]);
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetNPCPrimaryFaction query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	//Crash fix
	//cout << "Can not get faction for NPC " << npc_id << ", setting faction id to 0." << endl;
	*faction_id=0;
	*value=0;
	return false;
	//return true;
}


/*bool Database::GetFactionData(FactionMods* fm, sint32 class_mod, sint32 race_mod, sint32 deity_mod, sint32 faction_id)
{
	if (faction_id == 0)
	{
		return false;	
	}

	int modr_tmp =0;
	int modd_tmp =0;

	// froglok23 - Changed the switch block to use race #define's from magic numbers....
	switch (race_mod)
	{
		case HUMAN:
			modr_tmp = 0;
		break;
		case BARBARIAN: 
			modr_tmp = 1;
		break;
		case ERUDITE: 
			modr_tmp = 2;
		break;
		case WOOD_ELF: 
			modr_tmp = 3;
		break;
		case HIGH_ELF: 
			modr_tmp = 4;
		break;
		case DARK_ELF: 
			modr_tmp = 5;
		break;
		case HALF_ELF: 
			modr_tmp = 6;
		break;
		case DWARF: 
			modr_tmp = 7;
		break;
		case TROLL: 
			modr_tmp = 8;
		break;
		case OGRE: 
			modr_tmp = 9;
		break;
		case HALFLING: 
			modr_tmp = 10;
		break;
		case GNOME:
			modr_tmp = 11;
		break;
		case WEREWOLF: 
			modr_tmp = 12;
		break;
		case SKELETON: 
			modr_tmp = 13;
		break;
		case ELEMENTAL: 
			modr_tmp = 14;
		break;
		case EYE_OF_ZOMM: 
			modr_tmp = 15;
		break;
		case WOLF_ELEMENTAL: 
			modr_tmp = 16;
		break;
		case IKSAR:
			modr_tmp = 17;
		break;
		case VAHSHIR: 
			modr_tmp = 18;
		break;
		case IKSAR_SKELETON: 
			modr_tmp = 19;
		break;
	}
	if (deity_mod == DEITY_AGNOSTIC )
	{
		modd_tmp = 0;
	}
	else
	{
		modd_tmp = deity_mod - 200;
	}
	
	if (faction_array[faction_id] == 0)
	{
		return false;
	}
	
	//TODO: Fix this from crashing!
	//fm->base = faction_array[faction_id]->base;
	//fm->class_mod = faction_array[faction_id]->mod_c[class_mod-1];
	//fm->race_mod = faction_array[faction_id]->mod_r[modr_tmp];
	//fm->deity_mod = faction_array[faction_id]->mod_d[modd_tmp];

	return true;
}*/


/*bool Database::LoadFactionValues(int32 char_id, LinkedList<FactionValue*>* val_list)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT faction_id,current_value FROM faction_values WHERE char_id = %i",char_id), errbuf, &result)) {
		delete[] query;
		while(row = mysql_fetch_row(result))
		{
			FactionValue* facval = new FactionValue;
			facval->factionID = atoi(row[0]);
			facval->value = atoi(row[1]);
			val_list->Insert(facval);
		}
		mysql_free_result(result);
		return true;
	}
	else {
		cerr << "Error in LoadFactionValues query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}*/


bool Database::LoadDoorData(LinkedList<Door_Struct*>* door_list, char* zonename)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if(RunQuery(query, MakeAnyLenString(&query, "SELECT name,pos_x,pos_y,pos_z,heading,opentype,doorid FROM doors WHERE zone = '%s'", zonename), errbuf, &result))
	{
		delete[] query;

		while(row = mysql_fetch_row(result))
		{
			Door_Struct* door = new Door_Struct;
			memset(door, 0, sizeof(Door_Struct));
			
			strcpy(door->name, row[0]);

			door->xPos = (float)atof(row[1]);
			door->yPos = (float)atof(row[2]);
			door->zPos = (float)atof(row[3]);
			door->heading = (float)atof(row[4]);
			door->doorid = (float)atof(row[6]);
			door->opentype = atoi(row[7]); //In testing, it doesn't seem to make a difference if this is here or not. Someone explain. -Wizzel
			door->incline = 0;
			
			//door->test1 = 0xffffffff;
			//memset((float)door->test1, 1, sizeof(float));
			//door->test1[3] = 0xff;
			//door->test1[3] = 0xffffffff;

			//for(int i=0;i<4;i++){
				//door->test1[i] = 0xff;//(float)atof(row[5]);
				//door->test2[i] = 0xff;//22; 
			//}

			//door->test1[0] = 1;

			//door->test1[2] = 0;
			//door->stay_open = 0;
			
			//door->test2 = 5;
			//door->test3 = 6;
			//door->test4 = 7;

			door_list->Insert(door);

		}
		return true;
	}
	else 
	{
		cerr << "LoadDoorData failed with a bad query: '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}



bool Database::LoadObjects(vector<Object*>* object_list, char* zonename)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	//Hard coded to qeynos right now.
	if(RunQuery(query, MakeAnyLenString(&query, "SELECT xpos,ypos,zpos,heading,itemid,charges,objectname,id,type,icon FROM object_new WHERE zoneid = '2'", zonename), errbuf, &result))
	{
		delete[] query;

		while(row = mysql_fetch_row(result))
		{
			Object_Struct* os = new Object_Struct;
			memset(os, 0, sizeof(Object_Struct));

			os->xpos = (float)atof(row[0]);
			os->ypos = (float)atof(row[1]);
			os->zpos = (float)atof(row[2]);
			os->heading = (float)atof(row[3]);
			os->itemid = (float)atof(row[4]);
			memcpy(os->objectname,row[6],16);

			object_list->push_back(new Object(os));
			delete os;
		}
		return true;
	}
	else 
	{
		cerr << "LoadObjects failed with a bad query: '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}

const NPCFactionList* Database::GetNPCFactionList(uint32 id) {
	if (id <= npcfactionlist_max && npcfactionlist_array)
		return npcfactionlist_array[id];
	return 0;
}

	/********************************************************************
	 *                        Tazadar - 06/01/08                        *
	 ********************************************************************
	 *                          MakePet                                 *
	 ********************************************************************
	 *  + Load the pet stats from the MySQL Database                    *
	 *                                                                  *
	 ********************************************************************/

void Database::MakePet(Make_Pet_Struct* pet,int16 id,int16 type){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT level,class, race, texture, size, min_dmg, max_dmg, max_hp from pets where id=%i",id), errbuf, &result)) {
		safe_delete_array(query);
		if((row = mysql_fetch_row(result))) {
			
			pet->level=atoi(row[0]);
			pet->class_=atoi(row[1]);
			pet->race=atoi(row[2]);
			pet->texture=atoi(row[3]);
			pet->size=atoi(row[4]);
			pet->type=type;
			pet->pettype=id;
			pet->min_dmg = atoi(row[5]);
			pet->max_dmg = atoi(row[6]);
			pet->max_hp =atoi(row[7]);
		}
		mysql_free_result(result);
	} 
	else
		safe_delete_array(query);
	//cout << "level of the pet ="<< (int)pet->level << "  class ="<< (int)pet->class_ << " race of the pet= "<< (int)pet->race << " hp of the pet= "<< (int)pet->max_hp << endl;
}

	/********************************************************************
	 *                        Tazadar - 05/27/08                        *
	 ********************************************************************
	 *                          GetZoneW                                *
	 ********************************************************************
	 *  + Load the weather type from the SQL database(ie load if it     *
	 *    rains or it snows in the zone)                                *
	 ********************************************************************/
int8 Database::GetZoneW(char* zone_name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT weather FROM zone WHERE short_name='%s'", zone_name), errbuf, &result))
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


	/********************************************************************
	 *                        Tazadar - 05/27/08                        *
	 ********************************************************************
	 *                          SetZoneW                                *
	 ********************************************************************
	 *  + Change the weather type of the SQL database(ie change if it   *
	 *    rains or it snows in the zone)                                *
	 ********************************************************************/
bool Database::SetZoneW(char* zone_name, int8 w) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE zone SET weather=%i WHERE short_name='%s'", w, zone_name), errbuf, 0, &affected_rows)) {
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