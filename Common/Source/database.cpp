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
#include "database.h"
#include "races.h"
#include "deity.h"
#include "itemtypes.h"
#include "Item.h"
#include "eq_packet_structs.h"
#include "classes.h"
#include "inventory.h"
#include "SharedMemory.hpp"

using namespace std;
using namespace EQC::Common;

// change starting zone
const char* ZONE_NAME = "qeynos";

Database* Database::pinstance = 0;// initialize pointer

// Singleton Pattern: Singleton Instance -- Dark-Prince
// Allows access to the private static instance of 'database'
// Note: The Database Class has had its constructors made private to enforce all database calls go though this accessor
// More Informatiuion: http://en.wikipedia.org/wiki/Singleton_pattern
Database* Database::Instance () 
{
	// Check if pinstance has not been created previouly
	if (pinstance == 0)
	{  
		// pinstance is null, create a new instance of 'Database'
		pinstance = new Database;
	}

	// return address of instance
	return pinstance;
}

Database::Database ()
{
#ifndef EQC_SHAREDMEMORY
	item_array = 0;
	max_item = 0;
#endif
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
#ifndef EQC_SHAREDMEMORY
	item_array = 0;
	max_item = 0;
#endif
	max_npc_type = 0;
	npc_type_array = 0;
	max_faction = 0;
	faction_array = 0;
	
	DBConnect(host, user, passwd, database);
}

Database::~Database()
{
	int x;

#ifndef EQC_SHAREDMEMORY
	if (item_array != 0)
	{
		for (x=0; x <= max_item; x++)
		{
			if (item_array[x] != 0)
			{
				safe_delete(item_array[x]);//delete item_array[x];
			}
		}
		safe_delete(item_array);//delete item_array;
	}
#endif

	if (faction_array != 0)
	{
		for (x=0; x <= max_faction; x++)
		{
			if (faction_array[x] != 0)
			{
				safe_delete(faction_array[x]);//delete faction_array[x];
			}
		}
		safe_delete(faction_array);//delete faction_array;
	}

	if (npc_type_array != 0)
	{
		for (x=0; x <= max_npc_type; x++) 
		{
			if (npc_type_array[x] != 0)
			{
				safe_delete(npc_type_array[x]);//delete npc_type_array[x];
			}
		}
		safe_delete(npc_type_array);//delete npc_type_array;
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
	
	safe_delete_array(query);//delete[] query;

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
		safe_delete_array(query);//delete[] query;
		return false;
	}

	safe_delete_array(query);//delete[] query;

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
		safe_delete_array(query);//delete[] query;
		
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
	safe_delete_array(query);//delete[] query;

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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
	safe_delete_array(query);//delete[] query;
	
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

	safe_delete_array(query);//delete[] query;
	return bret; // Dark-Prince : 25/20/2007 : single return point
}

bool Database::SetGMFlag(char* name, int8 status)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;

	cout << "Account being GM Flagged:" << name << ", Level: " << (int16) status << endl;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET status=%i WHERE name='%s';", status, name), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;

	if (affected_rows == 0)
	{
		return false;
	}

	return true;
}

void Database::GetCharSelectInfo(int32 account_id, CharacterSelect_Struct* cs,CharWeapon_Struct* weapons) {
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
		safe_delete_array(query);//delete[] query;

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

				weapons->righthand[char_num]=pp->inventory[13];
				weapons->lefthand[char_num]=pp->inventory[14];

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
		safe_delete_array(query);//delete[] query;
		return;
	}
	for(int i=0;i<8;i++){//Tazadar: I dont know how it works but you must let that in order to let us know what character you are selecting when client sends packet
		memset(cs->unknown3+4*i,i+1,4*sizeof(int8));
		memset(cs->unknown3+4*i+40,i+1,4*sizeof(int8));
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
			safe_delete_array(query);//delete[] query;
		return false;
	}

	return true;
}

/*
	Delete the character with the name "name"
	False will also be returned if there is a database error.
	Tazadar: Added corpse deletion!
*/
bool Database::DeleteCharacter(char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE from player_corpses WHERE charname='%s'", name), errbuf)) {
		cerr << "Error in DeleteCharacter query '" << query << "' " << errbuf << endl;
		if (query != 0)
			safe_delete_array(query);
		return false;
	}


	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE from character_ WHERE name='%s'", name), errbuf)) {
		cerr << "Error in DeleteCharacter query '" << query << "' " << errbuf << endl;
		if (query != 0)
			safe_delete_array(query);
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
	pp.trainingpoints = 5;
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

	//Yeahlight: Client is backwards! (Y, X, Z)
	pp.bind_location[1][0] = pp.x;
	pp.bind_location[0][0] = pp.y;
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
		safe_delete_array(query);//delete[] query;
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
		unsigned long len=0;
		memcpy(&len,lengths, sizeof(unsigned long));
		mysql_free_result(result);
		return len;
	}
	else {
		cerr << "Error in GetPlayerProfile query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			strcpy(name, row[0]);
		}
		mysql_free_result(result);
	}
	else {
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
	}
	return false;
}

bool Database::CheckZoneserverAuth(char* ipaddr) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT * FROM zoneserver_auth WHERE '%s' like host", ipaddr), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
	}

	return 0xFFFFFFFF;
}

// Pyro: Get zone starting points from DB
bool Database::GetSafePoints(char* short_name, float* safe_x, float* safe_y, float* safe_z, int8* minstatus, int8* minlevel, float* heading, bool GM)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int buf_len = 256;
    int chars = -1;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT safe_x, safe_y, safe_z, minium_status, minium_level, safe_heading FROM zone WHERE short_name='%s'", short_name), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
		if(mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if(safe_x != 0)
				*safe_x = atoi(row[0]);
			if(safe_y != 0)
				*safe_y = atoi(row[1]);
			if(safe_z != 0)
				*safe_z = atoi(row[2]);
			if(minstatus != 0)
				*minstatus = atoi(row[3]);
			if(minlevel != 0)
				*minlevel = atoi(row[4]);
			if(heading != 0)
				*heading = atoi(row[5]);
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetSafePoint query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
	}
	return false;
}

bool Database::SetGuild(int32 charid, int32 guilddbid, int8 guildrank) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET guild=%i, guildrank=%i WHERE id=%i", guilddbid, guildrank, charid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);//delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in SetGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		if (affected_rows == 1) {
			return tmpeqid;
		}
		else {
			return 0xFFFFFFFF;
		}
	}
	else {
		cerr << "Error in CreateGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in DeleteGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in RenameGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in EditGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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

	safe_delete_array(query);//delete[] query;
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
	
	safe_delete_array(query);//delete[] query;
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

	safe_delete_array(query);//delete[] query;
	safe_delete(motdbuf);//delete motdbuf;

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

	safe_delete_array(query);//delete[] query;
	return false;
}

#ifndef EQC_SHAREDMEMORY
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
				safe_delete_array(query);//delete[] query;
				while(row = mysql_fetch_row(result))
				{
					unsigned long* lengths;
					lengths = mysql_fetch_lengths(result);
					if (lengths[1] == sizeof(Item_Struct))
					{
						item_array[atoi(row[0])] = new Item_Struct;
						memcpy(item_array[atoi(row[0])], row[1], sizeof(Item_Struct));
						//Yeahlight: Client item exceptions
						if(item_array[atoi(row[0])]->type == 0)
						{
							//Yeahlight: Remove 'BST' (01000000 00000000) from item classes as long as the list does not compute to 'ALL' (01111111 11111111)
							if(item_array[atoi(row[0])]->common.classes - 16384 >= 0 && item_array[atoi(row[0])]->common.classes != 32767)
								item_array[atoi(row[0])]->common.classes -= 16384;
							//Yeahlight: Remove 'VAH' (00100000 00000000) from item races as long as the list does not compute to 'ALL' (00111111 11111111)
							if(item_array[atoi(row[0])]->common.normal.races - 8192 >= 0 && item_array[atoi(row[0])]->common.normal.races != 16383)
								item_array[atoi(row[0])]->common.normal.races -= 8192;
							//Yeahlight: Our client cannot handle H2H skill weapons, so flag them as 1HB
							if(item_array[atoi(row[0])]->common.itemType == ItemTypeHand2Hand)
								item_array[atoi(row[0])]->common.itemType = ItemType1HB;
							//Yeahlight: There will be no gear with recommended levels on the server, so zero out this field
							if(item_array[atoi(row[0])]->common.recommendedLevel != 0)
								item_array[atoi(row[0])]->common.recommendedLevel = 0;
							//Yeahlight: This purges focus effects from our items
							if(item_array[atoi(row[0])]->common.click_effect_id >= 2330 && item_array[atoi(row[0])]->common.click_effect_id <= 2374)
							{
								item_array[atoi(row[0])]->common.click_effect_id = 0;
								item_array[atoi(row[0])]->common.spell_effect_id = 0;
								item_array[atoi(row[0])]->common.charges = 0;
								item_array[atoi(row[0])]->common.normal.click_effect_type = 0;
								item_array[atoi(row[0])]->common.effecttype = 0;
								item_array[atoi(row[0])]->common.clicktype = 0;
							}
						}
						//Yeahlight: Client container exceptions
						else if(item_array[atoi(row[0])]->type == 1)
						{
						//Tazadar : We clean this or the client crashes or bag are full of shit
						memset(&item_array[atoi(row[0])]->common.level,0x00,69*sizeof(int8));
						memset(&item_array[atoi(row[0])]->unknown0144[0],0x00,68*sizeof(int8));
						}
						//Yeahlight: Client book exceptions
						else if(item_array[atoi(row[0])]->type == 2)
						{

						}
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
				safe_delete_array(query);//delete[] query;
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}


	return true;
}
#endif


//bool Database::LoadItems()
//{
//	string legs = string("Spiderling Legs");
//	char errbuf[MYSQL_ERRMSG_SIZE];
//    char *query = 0;
//    MYSQL_RES *result;
//    MYSQL_ROW row;
//	query = new char[2000];
//	strcpy(query, "SELECT MAX(id) FROM items_nonblob");
//
//	
//	if (RunQuery(query, strlen(query), errbuf, &result)) {
//		safe_delete(query);
//		row = mysql_fetch_row(result);
//		if (row != 0 && row[0] > 0)
//		{ 
//			max_item = atoi(row[0]);
//			item_array = new Item_Struct*[max_item+1];
//			for(int i=0; i<max_item; i++)
//			{
//				item_array[i] = 0;
//			}
//			mysql_free_result(result);
//			//TODO: fix \"range\" as it is a mysql reserved word..... - Dark-Prince 31/12/2007
//			// Best bet is to rename the colunm
//
//									//		0			1			2			3			4
//			MakeAnyLenString(&query, "SELECT id,		name,		aagi,		ac,			accuracy,\
//											acha,		adex,		aint,		asta,		astr,\
//											attack,		awis,		bagsize,	bagslots,	bagtype,\
//											bagwr,		bardtype,	bardvalue,	book,		casttime, casttime_,\
//											charmfile,	charmfileid,classes,	color,		price,\
//											cr,			damage,		damageshield,deity,		delay,\
//											dotshielding,dr,		clicktype,	clicklevel2,filename,\
//											fr,			fvnodrop,	haste,		clicklevel,	hp,\
//											regen,		icon,		idfile,		itemclass,itemtype,\
//											light,		lore,		loreflag,	magic,		mana,\
//											manaregen,	material,	maxcharges,	mr,			nodrop,\
//											norent,		pendingloreflag,pr,		races,		range,\
//											reclevel,	recskill,	reqlevel,	sellrate,	shielding,\
//											size,		skillmodtype,skillmodvalue,slots,clickeffect,\
//											spellshield,stunresist,	summonedflag,tradeskills,weight,\
//											booktype,	recastdelay,recasttype,	attuneable,	nopet,\
//											pointtype,	potionbelt,	stacksize,	proceffect,	proctype,\
//											 proclevel2,proclevel,	worneffect,	worntype,	wornlevel2,\
//											wornlevel,	focustype,	focuslevel2,focuslevel,	scrolleffect,\
//											scrolltype,	scrolllevel2,scrolllevel FROM items_nonblob");
//
//			if (RunQuery(query, strlen(query), errbuf, &result))
//			{
//				safe_delete_array(query);//delete[] query;
//				while(row = mysql_fetch_row(result))
//				{
//	
//					item_array[atoi(row[0])] = new Item_Struct;
//					memset(item_array[atoi(row[0])], 0, sizeof(Item_Struct));
//
//					//Standard item properties set for ALL items:
//					strcpy(item_array[atoi(row[0])]->name, row[1]);
//					strcpy(item_array[atoi(row[0])]->idfile, row[43]);
//					strcpy(item_array[atoi(row[0])]->lore, row[47]);
//					if(atoi(row[48])==1){//Tazadar : If we got a lore item we put the lore flag
//						item_array[atoi(row[0])]->lore[0]='*';
//					}
//					item_array[atoi(row[0])]->cost = atoi(row[25]);
//					item_array[atoi(row[0])]->icon_nr = atoi(row[42]);
//
//					//The client only allows 2 bytes for unique item numbers.
//					if(atoi(row[0]) > 65450){
//						break;
//					}else{
//						item_array[atoi(row[0])]->item_nr = atoi(row[0]);
//					}
//
//					item_array[atoi(row[0])]->equipableSlots = atoi(row[69]);
//					item_array[atoi(row[0])]->equipSlot  = atoi(row[69]);		
//					item_array[atoi(row[0])]->nodrop = atoi(row[55]);
//					item_array[atoi(row[0])]->norent = atoi(row[56]);						
//					item_array[atoi(row[0])]->weight = atoi(row[75]);
//					item_array[atoi(row[0])]->size = atoi(row[66]);
//					//Begin flag checks.
//					
//					//Check for book.
//					if(atoi(row[18]) == 1){	
//						//Is book.
//						strcpy(item_array[atoi(row[0])]->book.file, row[35]);
//						item_array[atoi(row[0])]->flag = 0x7669;	
//						item_array[atoi(row[0])]->type = 0x02;
//					}else{
//						//This is a common item. Set common attribs.
//						item_array[atoi(row[0])]->common.AC			=	atoi(row[3]);
//						item_array[atoi(row[0])]->common.AGI		=	atoi(row[2]);
//						item_array[atoi(row[0])]->common.casttime	=	atoi(row[19]);
//						item_array[atoi(row[0])]->common.CHA		=	atoi(row[5]);
//						item_array[atoi(row[0])]->common.classes	=	atoi(row[23]);
//						item_array[atoi(row[0])]->common.color		=	atoi(row[24]);
//						item_array[atoi(row[0])]->common.CR			=	atoi(row[26]);
//						item_array[atoi(row[0])]->common.damage		=	atoi(row[27]);
//						item_array[atoi(row[0])]->common.delay		=	atoi(row[30]);
//						item_array[atoi(row[0])]->common.DEX		=	atoi(row[6]);
//						item_array[atoi(row[0])]->common.DR			=	atoi(row[32]);
//						item_array[atoi(row[0])]->common.FR			=	atoi(row[36]);
//						item_array[atoi(row[0])]->common.HP			=	atoi(row[40]);
//						item_array[atoi(row[0])]->common.INT		=	atoi(row[7]);
//						item_array[atoi(row[0])]->common.level		=	atoi(row[87]);
//						item_array[atoi(row[0])]->common.level0		=	atoi(row[87]);
//						item_array[atoi(row[0])]->common.light		=	atoi(row[46]);
//						item_array[atoi(row[0])]->common.magic		=	atoi(row[49]);
//						item_array[atoi(row[0])]->common.MANA		=	atoi(row[50]);
//						item_array[atoi(row[0])]->common.material	=	atoi(row[52]);
//						item_array[atoi(row[0])]->common.MR			=	atoi(row[54]);
//						item_array[atoi(row[0])]->common.PR			=	atoi(row[58]);
//						item_array[atoi(row[0])]->common.range		=	atoi(row[60]);
//						item_array[atoi(row[0])]->common.itemType		=	atoi(row[62]);
//						item_array[atoi(row[0])]->common.click_effect_id	=	atoi(row[70]);
//						item_array[atoi(row[0])]->common.spell_effect_id	=	atoi(row[95]);
//						item_array[atoi(row[0])]->common.STA		=	atoi(row[8]);
//						item_array[atoi(row[0])]->common.STR		=	atoi(row[9]);
//						item_array[atoi(row[0])]->common.WIS		=	atoi(row[11]);
//						
//						//Yeahlight: Set haste value
//						if(atoi(row[38]) != 0)
//						{
//							item_array[atoi(row[0])]->common.level				=	atoi(row[38]);
//							item_array[atoi(row[0])]->common.click_effect_id	=	998;
//							item_array[atoi(row[0])]->common.spell_effect_id	=	998;
//						}
//
//						//Yeahlight: Note: THIS IS NOT LEGIT, but meerly an attempt to get effect names to display
//						if(atoi(row[84]) != -1)
//						{
//							strcpy(item_array[atoi(row[0])]->name, "Proc Weapon");
//							item_array[atoi(row[0])]->norent					=	0xFF;
//							item_array[atoi(row[0])]->nodrop					=	0xFF;
//							item_array[atoi(row[0])]->unknown0144[20]			=	0x01;
//							item_array[atoi(row[0])]->common.stackable			=	0xFF;
//							item_array[atoi(row[0])]->common.charges			=	0x03;
//							item_array[atoi(row[0])]->common.number_of_items	=	0x01;
//							item_array[atoi(row[0])]->common.unknown0222[0]		=	0xFF;
//							item_array[atoi(row[0])]->common.unknown0222[1]		=	0xFF;
//							item_array[atoi(row[0])]->common.unknown0222[8]		=	0xA0;
//							item_array[atoi(row[0])]->common.unknown0222[9]		=	0x41;
//							item_array[atoi(row[0])]->common.click_effect_id	=	atoi(row[84]);
//							item_array[atoi(row[0])]->common.spell_effect_id	=	atoi(row[84]);
//							item_array[atoi(row[0])]->common.effecttype			=   0;
//							item_array[atoi(row[0])]->common.effecttype2		=   0;
//							item_array[atoi(row[0])]->common.level				=	atoi(row[85]);
//							item_array[atoi(row[0])]->common.level0				=	atoi(row[85]);
//						}
//
//						//if (atoi(row[45]) > 4) //Jester: Weapon check(0=1hs,1=2hs,2=piercing,3=1hb,4=2hb)
//						item_array[atoi(row[0])]->common.itemType	= atoi(row[45]);  //This is the item type.
//
//						int container_type = atoi(row[14]);
//
//						if(container_type == 0){								//If > 0, item is a bag.	
//							//================== Union ==============================================//
//							item_array[atoi(row[0])]->common.normal.races = atoi(row[59]);
//							item_array[atoi(row[0])]->flag = 0x3900;
//		
//							// All stackable item_types need to go in this check.
//							// Refer to itemtypes.h for definitions - Wizzel
//
//							if (atoi(row[45]) == THROWN ||
//								atoi(row[45]) == STACKABLE_ITEM ||
//								atoi(row[45]) == ARROWS ||
//								atoi(row[45]) == FOOD ||
//								atoi(row[45]) == BANDAGES ||
//								atoi(row[45]) == DRINK ||
//								atoi(row[45]) == FISHING_BAIT ||
//								atoi(row[45]) == THROWING_KNIFE ||
//								atoi(row[45]) == ALCOHOL)
//							{			
//								item_array[atoi(row[0])]->common.stackable			= 0x01;
//								item_array[atoi(row[0])]->common.number_of_items	= 0x01;
//								item_array[atoi(row[0])]->common.normal.click_effect_type		= 0x01;  // ?
//								//item_array[atoi(row[0])]->common.magic	= 0x0c;
//							
//							//Non stackable item.
//							//Item types: 20 spells, 21 potions, ...
//							}else{
//								if(atoi(row[45]) == SPELL){
//									item_array[atoi(row[0])]->flag = 0x0036;
//									item_array[atoi(row[0])]->common.itemType		=   atoi(row[45]);  //tried up to 5 including 5
//									item_array[atoi(row[0])]->common.spell_effect_id	=	atoi(row[95]);
//
//								}
//
//								item_array[atoi(row[0])]->flag = 0x315f;
//							}
//
//							item_array[atoi(row[0])]->type = 0x00;
//							//=======================================================================//
//						}
//						else
//						{
//							
//							if(container_type == 10){							//Regular container.
//								item_array[atoi(row[0])]->flag = 0x5450;	
//							}
//							else if(container_type == 30){						//Tradeskill container.
//								item_array[atoi(row[0])]->flag = 0x5400;
//							}
//							else{												//There are more that need to be implimented.
//								item_array[atoi(row[0])]->flag = 0x5450;		//Set to normal for now.
//							}
//
//							item_array[atoi(row[0])]->type = 0x01;
//
//							//=================== OR ================================================//
//							item_array[atoi(row[0])]->common.container.BagSize		=	atoi(row[12]);
//							item_array[atoi(row[0])]->common.container.weightReduction	=	atoi(row[15]);		//Confirmed.
//							item_array[atoi(row[0])]->common.container.BagSlots			=	atoi(row[13]);
//							item_array[atoi(row[0])]->common.container.BagType			=	atoi(row[14]);
//							item_array[atoi(row[0])]->common.container.open				=	0;
//
//							//Tazadar : We clean this or the client crashes
//							memset(&item_array[atoi(row[0])]->common.level,0x00,69*sizeof(int8));
//							memset(&item_array[atoi(row[0])]->unknown0144[0],0x00,68*sizeof(int8));
//
//							//=======================================================================//
//						}
//						
//						if(atoi(row[33]) == 1)			//1 = Potion,...						//Click Type.
//						{
//							//================== Union ==============================================//
//							item_array[atoi(row[0])]->common.charges					=	5;
//							item_array[atoi(row[0])]->common.effecttype					=	atoi(row[33]);
//							item_array[atoi(row[0])]->common.normal.click_effect_type				=	10;
//
//						}else{
//							//=================== OR ================================================//
//																									 //No proc, so this means stack number.
//							
//							//=======================================================================//						
//						}
//
//
//						//string test2 = string(item_array[atoi(row[0])]->name);
//
//						////For testing.
//						//if(legs.find(test2) != string::npos){
//						//	APPLAYER tApp = APPLAYER(0x0000, sizeof(Item_Struct));
//						//	memcpy(tApp.pBuffer,item_array[atoi(row[0])], sizeof(Item_Struct));
//						//	//DumpPacketHex(&tApp);
//						//}
//					}
//
//					Sleep(0);
//				}
//				mysql_free_result(result);
//			}
//			else {
//				cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
//				safe_delete_array(query);//delete[] query;
//				return false;
//			}
//		}
//		else {
//			mysql_free_result(result);
//		}
//	}
//	else {
//		cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
//		safe_delete_array(query);//delete[] query;
//		return false;
//	}
//
//
//
//	return true;
//}

// Harakiri - Load an Item from nonblob table into Item_Struct
// This is still in testing, this should replace the GetItem method sooner or later
Item_Struct* Database::GetItemNonBlob(sint32 itemID) {
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	bool ret = false;
	Item_Struct *item;
	EQC::Common::Log(EQCLog::Status,CP_DATABASE, "Loading item from database: id=%i", itemID);
		
	
	// Retrieve all items from database
	// use inline function to automatically expand all fieldnames
	char query2[] = "select source,"
#define F(x) ""#x","
#include "item_fieldlist.h"
#undef F
		"updated"
		" from items_axclassic where id=%i order by id";
	char *query = 0;
	query = new char[2000];
	MakeAnyLenString(&query,query2,itemID);	
		
	if (RunQuery(query, strlen(query), errbuf, &result)) {
                while((row = mysql_fetch_row(result))) {
#if EQDEBUG >= 6
				EQC::Common::Log(EQCLog::Status,CP_DATABASE, "Loading %s:%i", row[ItemField::name], row[ItemField::id]);
#endif				
				
			item = new Item_Struct;
					
			memset(item, 0, sizeof(Item_Struct));
			
			
			// ItemClass = type
			item->type = (uint8)atoi(row[ItemField::itemclass]);
			strcpy(item->name,row[ItemField::name]);
			strcpy(item->lore,row[ItemField::lore]);
			strcpy(item->idfile,row[ItemField::idfile]);
			// id = item_nr
			item->item_nr = (uint32)atoul(row[ItemField::id]);
			// Harakiri - this needs later to be fixed to be a real unique ID per item
			// the client uses this for example for apply poison - since it waits 7sec to
			// sent the apply poison opcode to the server - it needs to remember which item was clicked
			// it uses a unique ID for this - so even if you move around the poison item, the right slot
			// will still be found
			item->uniqueID = item->item_nr;

			item->weight = (uint8)atoi(row[ItemField::weight]);
			// Norent == norent
            item->norent = (uint8)atoi(row[ItemField::norent]);

			if(item->norent == 1) {
				// Harakiri blobs were FF, but 01 also works, just to be sure tho
				/// i guess everything non zero counts as it
				item->norent = 0xFF;
			}
			item->nodrop = (uint8)atoi(row[ItemField::nodrop]);

			if(item->nodrop == 1) {
				// Harakiri blobs were FF, but 01 also works, just to be sure tho
				/// i guess everything non zero counts as it
				item->nodrop = 0xFF;
			}

			item->nodropFirionaVie = (uint8)atoi(row[ItemField::fvnodrop]);

			if(item->nodropFirionaVie == 1) {
				// Harakiri blobs were FF, but 01 also works, just to be sure tho
				/// i guess everything non zero counts as it
				item->nodropFirionaVie = 0xFF;
			}
			
			
			item->size = (uint8)atoi(row[ItemField::size]);
			// Slots == equipableSlots
			item->equipableSlots = (uint32)atoul(row[ItemField::slots]);
			// Price == cost
			item->cost = (uint32)atoul(row[ItemField::price]);
			// Icon == icon_nr
			item->icon_nr = (uint32)atoul(row[ItemField::icon]);

			if(item->type == ItemClassCommon) {
				item->common.CR = (sint8)atoi(row[ItemField::cr]);
				item->common.DR = (sint8)atoi(row[ItemField::dr]);
				item->common.PR = (sint8)atoi(row[ItemField::pr]);
				item->common.MR = (sint8)atoi(row[ItemField::mr]);
				item->common.FR = (sint8)atoi(row[ItemField::fr]);
				item->common.STR = (sint8)atoi(row[ItemField::astr]);
				item->common.STA = (sint8)atoi(row[ItemField::asta]);
				item->common.AGI = (sint8)atoi(row[ItemField::aagi]);
				item->common.DEX = (sint8)atoi(row[ItemField::adex]);
				item->common.CHA = (sint8)atoi(row[ItemField::acha]);
				item->common.INT = (sint8)atoi(row[ItemField::aint]);
				item->common.WIS = (sint8)atoi(row[ItemField::awis]);
				item->common.HP = (sint32)atoul(row[ItemField::hp]);
				item->common.MANA = (sint32)atoul(row[ItemField::mana]);
				item->common.AC = (sint32)atoul(row[ItemField::ac]);								
				// TODO item->BaneDmgRace = (uint32)atoul(row[ItemField::banedmgrace]);
				// TODOitem->BaneDmgAmt = (sint8)atoi(row[ItemField::banedmgamt]);
				// TODO item->BaneDmgBody = (uint32)atoul(row[ItemField::banedmgbody]);
				item->common.magic = (atoi(row[ItemField::magic])==0) ? false : true;
				item->common.casttime = (sint32)atoul(row[ItemField::casttime_]);
				// TODO item->ReqLevel = (uint8)atoi(row[ItemField::reqlevel]);
				// TODO item->BardType = (uint32)atoul(row[ItemField::bardtype]);
				// TODO item->BardValue = (sint32)atoul(row[ItemField::bardvalue]);
				item->common.light = (sint8)atoi(row[ItemField::light]);
				item->common.delay = (uint8)atoi(row[ItemField::delay]);
				// RecLevel = recommendedLevel
				item->common.recommendedLevel = (uint8)atoi(row[ItemField::reclevel]);
				//TODO item->RecSkill = (uint8)atoi(row[ItemField::recskill]);
				//TODO item->ElemDmgType = (uint8)atoi(row[ItemField::elemdmgtype]);
				//TODO item->ElemDmgAmt = (uint8)atoi(row[ItemField::elemdmgamt]);
				item->common.range = (uint8)atoi(row[ItemField::range]);
				item->common.damage = (uint8)atoi(row[ItemField::damage]);
				item->common.color = (uint32)atoul(row[ItemField::color]);
				// Harakiri the new colors are now padded by FF and no longer RGB value
				// FF is transparency, but the older client only works with RGB
				if(item->common.color >= 0xFF000000) {
					item->common.color = item->common.color - 0xFF000000;
				}
				item->common.classes = (uint32)atoul(row[ItemField::classes]);
				item->common.normal.races = (uint32)atoul(row[ItemField::races]);
				item->common.deity = (uint32)atoul(row[ItemField::deity]);
				
				// ItemType = common.itemType
				item->common.itemType= (uint8)atoi(row[ItemField::itemtype]);
				item->common.material = (uint8)atoi(row[ItemField::material]);
				//TODO item->SellRate = (float)atof(row[ItemField::sellrate]);			
				item->common.casttime = (uint32)atoul(row[ItemField::casttime]);			
				// TODO item->ProcRate = (sint32)atoi(row[ItemField::procrate]);		
				
				item->common.clicktype = (uint8)atoul(row[ItemField::clicktype]);

				//Harakiri, for scrolls we need to take from another column, for items its just clickeffect
				if(item->common.itemType==ItemTypeSpell) {

					item->common.click_effect_id = (uint32)atoul(row[ItemField::scrolleffect]);
					// Harakiri for some reasons, spell scrolls are never magic - the client doesnt bother if it is set to 1
					// but just to be sure, we always make a spell non magic
					item->common.magic=0;
				// Harakiri for poison we need the proceffect field
				} else if(item->common.itemType==ItemTypePoison) {
					item->common.click_effect_id = (uint32)atoul(row[ItemField::proceffect]);									
					
					// Harakiri poisons seems to be broken on the client, we could try mark them as potions
					// item->common.clicktype = effect;
					// item->common.itemType = ItemTypePotion;
				// for all others use the click effect for now
				} else {										
					item->common.click_effect_id = (uint32)atoul(row[ItemField::clickeffect]);
					
				}								
								    		
								
				item->common.charges = (uint8)atoi(row[ItemField::maxcharges]);  // for clickies				

				// Harakiri this is a legacy field not found in newer item tables
				// the client needs to display the EFFECT = spell if 3
				// the value is also 3 for spells				
				item->common.normal.click_effect_type = 3; // items with effect 
				

				ItemInst *i = new ItemInst(item, 0);

				// Harakiri for stackable items, at least one is needed or else client prints error 
				// BAD CHARGES ON STACKABLE.  DELETING
				if(item->common.charges == 0 && i->IsConsumable()) {
					item->common.charges = 1;
				}

				if(item->common.charges > 0 && i->IsConsumable()) {
					item->common.stackable = 1;
				}				 

				// Harakiri items with limited charge of effects need stackable to be set to be > 0
				// or else the charges are not shown i.e. soulfire with 5 charges of complete heal
				if(item->common.clicktype != 0 &&  item->common.charges > 0) {
					/*1 would also be fine, but original itemblobs had always stackable == maxcharges*/
					item->common.stackable = (uint8)atoi(row[ItemField::maxcharges]);
				}
				

				item->common.clicklevel2 = (uint8)atoul(row[ItemField::clicklevel2]);
				// Harakiri clicklevel2 and level seem to be always the same in itemblob
				item->common.level = item->common.clicklevel2;
				
				// Harakiri exception for haste, see NPC::GetItemHaste() 
				// the old client had the spell ID of haste (998) on any haste item
				// the actual haste value is however not the one from the spell haste but different for each item
				// we store the haste in the level field
				int haste = (uint8)atoul(row[ItemField::haste]);

				if(haste > 0) {
					item->common.level = haste;
					item->common.effecttype = ET_WornEffect;
					item->common.clicktype = ET_WornEffect;					
					item->common.click_effect_id = 998;					
				} else {
					item->common.effecttype = item->common.clicktype;	
				}
				
				// Harakiri spell_effect == click_effect_id - no idea if it is ever different
				item->common.spell_effect_id = item->common.click_effect_id;			
			}

			if(item->type == ItemClassContainer) {
				// BagType = common.container.BagType
				item->common.container.BagType = (uint8)atoi(row[ItemField::bagtype]);
				// BagSlots = BagSlots
				item->common.container.BagSlots = (uint8)atoi(row[ItemField::bagslots]);
				// BagSize = BagSize
				item->common.container.BagSize = (uint8)atoi(row[ItemField::bagsize]);
				// BagWr =weightReduction
				item->common.container.weightReduction = (uint8)atoi(row[ItemField::bagwr]);
			}

			if(item->type == ItemClassBook) {
								
				// Filename = file
				strcpy(item->book.file,row[ItemField::filename]);
				item->book.book = (uint8)atoi(row[ItemField::book]);
				item->book.bookType = (uint16)atoul(row[ItemField::booktype]);
			}
			
			
			// Harakiri: Remove 'BST' (01000000 00000000) from item classes as long as the list does not compute to 'ALL' (11111111 11111111)
			if(item->common.classes - 16384 >= 0 && item->common.classes != 65535)
				item->common.classes -= 16384;
			// Harakiri: Remove 'VAH' (00100000 00000000) from item races as long as the list does not compute to 'ALL' (11111111 11111111)
			if(item->common.normal.races - 8192 >= 0 && item->common.normal.races != 65535)
				item->common.normal.races -= 8192;
			// Harakiri: Our client cannot handle H2H skill weapons, so flag them as 1HB
			if(item->common.itemType == ItemTypeHand2Hand)
				item->common.itemType = ItemType1HB;					
		}
		
		mysql_free_result(result);
		ret = true;
	}
	else {
		EQC::Common::Log(EQCLog::Error,CP_DATABASE, "DBLoadItems query '%s', %s", query, errbuf);		
	}
	return item;	
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
			MakeAnyLenString(&query, "SELECT npc_types_without.id,name,level,race,class,hp,gender,texture,helmtexture,size,runspeed,\
									 loottable_id, merchant_id, bodytype, mindmg, maxdmg, MR, CR, DR, FR, PR, AC, STR, STA, DEX, AGI, _INT, WIS, CHA, npcspecialattks, attack_speed, d_meele_texture1, d_meele_texture2, hp_regen_rate, see_invis, see_invis_undead, npc_types_without.spawn_limit FROM npc_types_without inner join spawnentry on \
									 npc_types_without.id = spawnentry.npcID inner join spawn2 on \
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
					//Yeahlight: Various DB class fixes
					if(atoi(row[4]) >= 20)
					{
						//Database says 41 is merchant class, but we use 32.
						if(atoi(row[4]) == 41)
						{
							npc_type_array[atoi(row[0])]->class_ = 32;
						}
						//Yeahlight: Incorrect banker class
						else if(atoi(row[4]) == 40)
						{
							npc_type_array[atoi(row[0])]->class_ = 16;
						}
						//Yeahlight: Incorrect GM trainer class
						else
						{
							npc_type_array[atoi(row[0])]->class_ = atoi(row[4]) - 3;
						}
					}
					else
					{
						npc_type_array[atoi(row[0])]->class_ = atoi(row[4]);
					}
					npc_type_array[atoi(row[0])]->cur_hp = atoi(row[5]);
					npc_type_array[atoi(row[0])]->max_hp = atoi(row[5]);
					npc_type_array[atoi(row[0])]->gender = atoi(row[6]);
					npc_type_array[atoi(row[0])]->texture = atoi(row[7]);
					npc_type_array[atoi(row[0])]->helmtexture = atoi(row[8]);
					npc_type_array[atoi(row[0])]->size = atof(row[9]);
					npc_type_array[atoi(row[0])]->runspeed = atof(row[10])/0.64;
					npc_type_array[atoi(row[0])]->walkspeed = atof(row[10]);
					npc_type_array[atoi(row[0])]->loottable_id = atoi(row[11]);
					npc_type_array[atoi(row[0])]->merchanttype = atoi(row[12]);
					npc_type_array[atoi(row[0])]->body_type = (TBodyType)atoi(row[13]);
					npc_type_array[atoi(row[0])]->min_dmg = atoi(row[14]);
					npc_type_array[atoi(row[0])]->max_dmg = atoi(row[15]);
					npc_type_array[atoi(row[0])]->MR = atoi(row[16]);
					npc_type_array[atoi(row[0])]->CR = atoi(row[17]);
					npc_type_array[atoi(row[0])]->DR = atoi(row[18]);
					npc_type_array[atoi(row[0])]->FR = atoi(row[19]);
					npc_type_array[atoi(row[0])]->PR = atoi(row[20]);
					npc_type_array[atoi(row[0])]->AC = atoi(row[21]);
					npc_type_array[atoi(row[0])]->STR = atoi(row[22]);
					npc_type_array[atoi(row[0])]->STA = atoi(row[23]);
					npc_type_array[atoi(row[0])]->DEX = atoi(row[24]);
					npc_type_array[atoi(row[0])]->AGI = atoi(row[25]);
					npc_type_array[atoi(row[0])]->INT = atoi(row[26]);
					npc_type_array[atoi(row[0])]->WIS = atoi(row[27]);
					npc_type_array[atoi(row[0])]->CHA = atoi(row[28]);
					strcpy(npc_type_array[atoi(row[0])]->special_attacks, row[29]);
					npc_type_array[atoi(row[0])]->attack_speed = atoi(row[30]);
					npc_type_array[atoi(row[0])]->main_hand_texture = atoi(row[31]);
					npc_type_array[atoi(row[0])]->off_hand_texture = atoi(row[32]);
					npc_type_array[atoi(row[0])]->hp_regen_rate = atoi(row[33]);
					npc_type_array[atoi(row[0])]->passiveSeeInvis = atoi(row[34]);
					npc_type_array[atoi(row[0])]->passiveSeeInvisToUndead = atoi(row[35]);
					npc_type_array[atoi(row[0])]->spawn_limit = atoi(row[36]);

					//Database has wrong race type for skelectons (they show as human w/o this fix).
					if(atoi(row[3]) == 367){
						npc_type_array[atoi(row[0])]->race = 60;
					}

					Sleep(0);
				}
				mysql_free_result(result);
			}
			else
			{
				cerr << "Error in LoadNPCTypes query '" << query << "' " << errbuf << endl;
				safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		return false;
	}

	return true;
}

// -Cofruben: changing to shared memory method.
#ifdef EQC_SHAREDMEMORY
Item_Struct* Database::GetItem(uint32 id)
{
	if (SharedMemory::isLoaded() && id < SharedMemory::getMaxItem())
	{
		return SharedMemory::getItem(id);
	}
	else
	{
		return 0;
	}
}
#else
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
#endif
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
	safe_delete_array(query);//delete[] query;
	return;
}

bool Database::AddToNameFilter(char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO name_filter (name) values ('%s')", name), errbuf, 0, &affected_rows)) {
		cerr << "Error in AddToNameFilter query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	safe_delete_array(query);//delete[] query;

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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		return 0;
	}

	return 0;
}

int32 Database::GetLSIDFromAccountID(int32 account_id){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT lsaccount_id FROM account WHERE id=%i", account_id), errbuf, &result))
	{
		safe_delete_array(query);//delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			if (!row || !row[0]) return 0;	// Cofruben: this will prevent a random crash.
			int32 account_id_f = atoi(row[0]);
			mysql_free_result(result);
			return account_id_f;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		if (affected_rows == 1) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		cerr << "Error in CreateSpawn2 query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
	}
	else {
		cerr << "Error in GetMerchantData query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
	}

	return -1;
}

	/********************************************************************
	 *                      Tazadar - 07/24/08                          *
	 ********************************************************************
	 *                         MerchantHasItem                          *
	 ********************************************************************
	 *  + Return the slot of the item if the merchant got it            *
	 *  + Return 0 if he does not have it !                             *
	 ********************************************************************/
int32 Database::MerchantHasItem(int32 merchantid,int32 item_id){

	int maxitem=GetMerchantListNumb(merchantid);
	int amount=0;
	int x=0;
	while(amount<maxitem){
		x++;
		int32 item_nr = GetMerchantData(merchantid,x);
		Item_Struct* item = GetItem(item_nr);
		if(item){
			amount++;
			if(item_nr==item_id){
				return x;
			}
		}
		if(item_nr!=-1 && !item){//Tazadar:if the item is buggy
			amount++;
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	return 0;
}

	/********************************************************************
	 *                      Tazadar - 07/24/08                          *
	 ********************************************************************
	 *                         GetMerchantStack                         *
	 ********************************************************************
	 *  + Return the number of items in the stack (or 0 for infinite)   *
	 *																	*
	 ********************************************************************/
int32 Database::GetMerchantStack(int32 merchantid,int32 item_id){

	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT stack FROM merchantlist WHERE merchantid=%d and item=%d", merchantid, item_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
	}
	else {
		cerr << "Error in GetMerchantData query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}

	return 0;

}

	/********************************************************************
	 *                      Tazadar - 07/24/08                          *
	 ********************************************************************
	 *                         AddMerchantItem                          *
	 ********************************************************************
	 *  + Add the item in the vendor DB when a player sell it		    *
	 *																	*
	 ********************************************************************/
bool Database::AddMerchantItem(int32 merchantid,int32 item_id,int32 stack,int32 slot)
{
	cout << "Adding item in the Db of the Merchant "<< (int)merchantid << " with the item " << (int)item_id << " in the slot " <<(int)slot<<endl;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "INSERT INTO merchantlist (merchantid,slot,item,stack) Values (%i, %i, %i, %i)", merchantid, slot, item_id, stack), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);//delete[] query;
		if (affected_rows == 1) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		cerr << "Error in AddMerchantItem query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	return false;
}

	/********************************************************************
	 *                      Tazadar - 07/24/08                          *
	 ********************************************************************
	 *                          UpdateStack								*
	 ********************************************************************
	 *  + Update the amount of the stack							    *
	 *																	*
	 ********************************************************************/
bool Database::UpdateStack(int32 merchantid,int32 stack,int32 item_id) {


	char errbuf[MYSQL_ERRMSG_SIZE]; 
	char *query = 0; 
	int32   affected_rows = 0; 

	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE merchantlist SET stack=%i WHERE merchantid=%d and item=%d", stack, merchantid, item_id), errbuf, 0, &affected_rows)) { 
		cerr << "Error running query " << query << endl; 
		safe_delete_array(query);//delete[] query; 
		return false; 
	} 
	safe_delete_array(query);//delete[] query; 

	if (affected_rows == 0) 
	{ 
		return false; 
	} 

	return true; 
}

	/********************************************************************
	 *                      Tazadar - 07/24/08                          *
	 ********************************************************************
	 *                         RemoveMerchantItem                       *
	 ********************************************************************
	 *  + Remove the item when the item is bought by a player		    *
	 *																	*
	 ********************************************************************/
bool Database::RemoveMerchantItem(int32 merchantid,int32 item_id) 
{ 

	char errbuf[MYSQL_ERRMSG_SIZE]; 
	char *query = 0; 
	int32   affected_rows = 0; 

	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM merchantlist WHERE merchantid=%d and item=%d ", merchantid, item_id), errbuf, 0, &affected_rows)) { 
		cerr << "Error running query " << query << endl; 
		safe_delete_array(query);//delete[] query; 
		return false; 
	} 
	safe_delete_array(query);//delete[] query; 

	if (affected_rows == 0) 
	{ 
		return false; 
	} 

	return true; 
} 

	/********************************************************************
	 *                      Tazadar - 07/24/08                          *
	 ********************************************************************
	 *                          UpdateSlot								*
	 ********************************************************************
	 *  + Update the slot of the item								    *
	 *																	*
	 ********************************************************************/
bool Database::UpdateSlot(int32 merchantid,int32 slot,int32 item_id) {


	char errbuf[MYSQL_ERRMSG_SIZE]; 
	char *query = 0; 
	int32   affected_rows = 0; 

	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE merchantlist SET slot=%i WHERE merchantid=%d and item=%d", slot, merchantid, item_id), errbuf, 0, &affected_rows)) { 
		cerr << "Error running query " << query << endl; 
		safe_delete_array(query);
		return false; 
	} 
	safe_delete_array(query);

	if (affected_rows == 0) 
	{ 
		return false; 
	} 

	return true; 
}

	/********************************************************************
	 *                      Tazadar - 07/24/08                          *
	 ********************************************************************
	 *                         UpdateAfterWithdraw						*
	 ********************************************************************
	 *  + Update the db after a player bought an item				    *
	 *																	*
	 ********************************************************************/
bool Database::UpdateAfterWithdraw(int32 merchantid,int32 startingslot,int32 lastslot){

	for(int i=startingslot;i<=lastslot;i++){
		cout<< "moving item from slot " <<(int)i<<" to slot " <<(int)i-1 << endl;
		int32 item_nr=GetMerchantData(merchantid,i);
		UpdateSlot(merchantid,i-1,item_nr);
	}
	
	return true;
}

	/********************************************************************
	 *						Tazadar - 07/28/08                          *
	 ********************************************************************
	 *                         GetLastSlot      						*
	 ********************************************************************
	 *  + Gives the lastslot in the current DB						    *
	 *																	*
	 ********************************************************************/
int32 Database::GetLastSlot(int32 merchantid,int32 startingslot){

	int maxitem=GetMerchantListNumb(merchantid);
	int amount=0;
	int x=startingslot-1;
	while(amount<maxitem){
		x++;
		int32 item_nr = 0;
		item_nr = GetMerchantData(merchantid,x);
		Item_Struct* item = GetItem(item_nr);
		if(item){
			amount++;
		}
		if(item_nr!=-1 && !item){//if the item is buggy
			amount++;
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	return x;
}

bool Database::UpdateNpcSpawnLocation(int32 spawngroupid, float x, float y, float z, float heading) 
{ 
	        // maalanar: used to move a spawn location in spawn2 
	        char errbuf[MYSQL_ERRMSG_SIZE]; 
	    char *query = 0; 
	        int32   affected_rows = 0; 
	 
	        if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE spawn2 SET x='%f', y='%f', z='%f', heading='%f' WHERE spawngroupID='%d';", x, y, z, heading, spawngroupid), errbuf, 0, &affected_rows)) { 
	                cerr << "Error running query " << query << endl; 
	                safe_delete_array(query);//delete[] query; 
	                return false; 
	        } 
	        safe_delete_array(query);//delete[] query; 
	 
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
	                safe_delete_array(query);//delete[] query; 
	                return false; 
	        } 
	        safe_delete_array(query);//delete[] query; 
	 
	        if (affected_rows == 0) 
	        { 
	                return false; 
	        } 
	 
	        return true; 
} 

int32 Database::GetMerchantListNumb(int32 merchantid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT item FROM merchantlist WHERE merchantid=%d", merchantid), errbuf, &result)) {
		safe_delete_array(query);
		int32 tmp = mysql_num_rows(result);
		mysql_free_result(result);
		return tmp;
	}
	else {
		cerr << "Error in GetMerchantListNumb query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
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
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;

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
			safe_delete_array(query);//delete[] query;
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

int	Database::GetFreeGroupID(){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int32 groupid=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT MAX(GID)+1 from character_"), errbuf, &result)) {
		if((row = mysql_fetch_row(result)))
		{
			if(row[0])
				groupid=atoi(row[0]);
		}
		else
			printf("Unable to get new group id!\n");
		mysql_free_result(result);
	}
	else
		printf("Unable to get new group id: %s\n",errbuf);
	safe_delete_array(query);
	return groupid;
}

void Database::SetGroupID(char* charname, int GID) {
	this->SetGroupID(charname, GID, 0);
}

void Database::SetGroupID(char* charname, int GID, int isleader) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "update character_ set GID=%i,groupleader=%i where name='%s'", GID, isleader, charname), errbuf))
		printf("Unable to get group id: %s\n",errbuf);	
	printf("Set group id on '%s' to %d\n", charname, GID);
	safe_delete_array(query);
}
int	Database::GetGroupID(char* charname){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int32 groupid=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT GID from character_ where name='%s'", charname), errbuf, &result)) {
		if((row = mysql_fetch_row(result)))
		{
			if(row[0])
				groupid=atoi(row[0]);
		}
		else
			printf("Unable to get group id, char not found!\n");
		mysql_free_result(result);
	}
	else
		printf("Unable to get group id: %s\n",errbuf);
	safe_delete_array(query);
	return groupid;
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


	safe_delete_array(query);//delete[] query;
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

	safe_delete_array(query);//delete[] query;
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

	//newage: updated to fix starting items DB
	if(RunQuery(query, MakeAnyLenString(&query, "SELECT itemid FROM starting_items WHERE (race = %i or race = 0) AND (class = %i or class = 0) ORDER BY id", (int) si_race, (int) si_class), errbuf, &result))
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
	safe_delete_array(query);//delete[] query;
	
	return bret; // Dark-Prince : 25/20/2007 : single return point
}

bool Database::SetStartingLocations(PlayerProfile_Struct *cc, int8 sl_race, int8 sl_class, char* sl_name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int16 zoneID = 0;

	cout<< "Loading starting locations for: Race: " << (int) sl_race << " Class: " << (int) sl_class << " onto " << sl_name << endl;

	if(RunQuery(query, MakeAnyLenString(&query, "SELECT zoneidnumber from zone_ids where short_name = '%s'", cc->current_zone), errbuf, &result))
	{
		while(row = mysql_fetch_row(result))
		{
			zoneID = atoi(row[0]);	
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in SetStartingLocations query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);

	if(RunQuery(query, MakeAnyLenString(&query, "SELECT x, y, z FROM start_zones WHERE zone_id = %i AND player_class = %i AND player_race = %i", zoneID, sl_class, sl_race), errbuf, &result))
	{
		while(row = mysql_fetch_row(result))
		{
			cc->x = atoi(row[0]);
			cc->y = atoi(row[1]);
			cc->z = atoi(row[2]);
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in SetStartingLocations2 query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);//delete[] query;
	
	return true;
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

	if (RunQuery(query, MakeAnyLenString(&query,"select faction_id, value FROM npc_faction_entries INNER JOIN npc_types_without \
												ON npc_types_without.npc_faction_id = npc_faction_entries.npc_faction_id \
												WHERE id=%i", npc_id),errbuf,&result)){
	//if (RunQuery(query, MakeAnyLenString(&query, "SELECT faction_id, value FROM npc_faction_entries WHERE npc_id = %i", npc_id), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
	}
	//Crash fix
	*faction_id=0;
	*value=0;
	return false;
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

bool Database::LoadDoorData(LinkedList<Door_Struct*>* door_list, char* zonename)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if(RunQuery(query, MakeAnyLenString(&query, "SELECT name,pos_x,pos_y,pos_z,heading,opentype,doorid,triggerdoor,triggertype,door_param,incline,doorisopen,invert_state,lockpick,keyitem,dest_zone,dest_x,dest_y,dest_z,dest_heading FROM doors WHERE zone = '%s'", zonename), errbuf, &result))
	{
		safe_delete_array(query);//delete[] query;

		while(row = mysql_fetch_row(result))
		{
			Door_Struct* door = new Door_Struct;
			memset(door, 0, sizeof(Door_Struct));
			
			strcpy(door->name, row[0]);
			door->xPos = (float)atof(row[1]);
			door->yPos = (float)atof(row[2]);
			door->zPos = (float)atof(row[3]);
			door->heading = (float)atof(row[4]);
			door->opentype = atoi(row[5]);
			door->doorid = (uint8)atoi(row[6]);
			door->triggerID = atoi(row[7]);
			//door->triggerType = atoi(row[8]);
			door->parameter = atoi(row[9]);
			door->incline = (float)atof(row[10]);
			door->doorIsOpen = atoi(row[11]);
			//Yeahlight: Trap type doors need inverted set to 1 to work properly
			if(door->opentype >= 115)
				door->inverted = 1;
			else
				door->inverted = atoi(row[12]);
			door->lockpick = atoi(row[13]);
			door->keyRequired = atoi(row[14]);
			strcpy(door->zoneName, row[15]);
			door->destX = (float)atof(row[16]);
			door->destY = (float)atof(row[17]);
			door->destZ = (float)atof(row[18]);
			door->destHeading = (float)atof(row[19]);

			door_list->Insert(door);
		}
		mysql_free_result(result);
		return true;
	}
	else 
	{
		cerr << "LoadDoorData failed with a bad query: '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
	}

	//Yeahlight: Loops used to help find mystery door types
	//int counter = 0;
	//int counter2 = 0;
	//for(int i = 170; i < 205; i++){
	////for(int i = 35; i < 36; i++){
	//	Door_Struct* door = new Door_Struct;
	//	memset(door, 0, sizeof(Door_Struct));

	//	strcpy(door->name, "");

	//	//if(i == 40 || i == 45)
	//	//	continue;

	//	door->xPos = 548 - counter2 * 45;
	//	door->yPos = 15 - counter * 45;
	//	counter++;
	//	if(counter == 5){
	//		counter = 0;
	//		counter2++;
	//	}
	//	door->zPos = 5;
	//	door->heading = 0;
	//	door->opentype = 53;
	//	door->doorid = i+1;
	//	door->triggerID = 0;
	//	//door->triggerType = atoi(row[8]);
	//	door->parameter = i;//atoi(row[9]);
	//	door->incline = 0;		
	//	door->doorIsOpen = 1;
	//	//Yeahlight: Trap doors need inverted set to 1 to work properly
	//	door->inverted = 1;
	//	door->lockpick = 0;
	//	door->keyRequired = 0;
	//	strcpy(door->zoneName, "eastkarana");
	//	door->destX = 0;
	//	door->destY = 0;
	//	door->destZ = 0;
	//	door->destHeading = 0;

	//	door_list->Insert(door);
	//}
	return false;
}

bool Database::LoadObjects(vector<Object_Struct*>* object_list, char* zonename)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	//Hard coded to qeynos right now.
	if(RunQuery(query, MakeAnyLenString(&query, "SELECT xpos,ypos,zpos,heading,itemid,charges,objectname,id,type,icon FROM object_new WHERE short_name = '%s'", zonename), errbuf, &result))
	{
		safe_delete_array(query);//delete[] query;

		while(row = mysql_fetch_row(result))
		{
			Object_Struct* os = new Object_Struct;
			memset(os, 0, sizeof(Object_Struct));

			os->xpos = (float)atof(row[0]);
			os->ypos = (float)atof(row[1]);
			os->zpos = (float)atof(row[2]);
			os->heading = (float)atof(row[3]);
			os->itemid = (int16)atoi(row[4]);
			os->type = (int16)atoi(row[8]);
			os->icon_nr = (int16)atoi(row[9]);
			memcpy(os->objectname,row[6],16);
			
			object_list->push_back(os);
		}
		mysql_free_result(result);
		return true;
	}
	else 
	{
		cerr << "LoadObjects failed with a bad query: '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
	}
	return false;
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
			pet->size=atof(row[4]);
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in SetZoneW query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	
	return false;
}

	/********************************************************************
	 *                      Pinedepain - 06/17/08                       *
	 ********************************************************************
	 *                         GetBardType                              *
	 ********************************************************************
	 *  + Return the bard type instrument modifier.                     *
	 *                                                                  *
	 ********************************************************************/

uint8 Database::GetBardType(int16 item_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int16 ret = 0;
	//Melinko: Updated query to use bardtype instead of bardType for our current database
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT bardtype FROM items_nonblob WHERE id=%i",item_id), errbuf, &result)) 
	{
		safe_delete_array(query);
		if((row = mysql_fetch_row(result))) 
		{
			ret = (atoi(row[0]));
		}
		mysql_free_result(result);
		return ret;
	} 
	else
	{
		safe_delete_array(query);
	}
	


	return (0);
}

	/********************************************************************
	 *                      Pinedepain - 06/17/08                       *
	 ********************************************************************
	 *                         GetBardValue                             *
	 ********************************************************************
	 *  + Return the bard instrument modifier value.                    *
	 *                                                                  *
	 ********************************************************************/

uint8 Database::GetBardValue(int16 item_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int16 ret = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT bardvalue FROM items_nonblob WHERE id=%i",item_id), errbuf, &result)) 
	{
		safe_delete_array(query);
		if((row = mysql_fetch_row(result))) 
		{
			ret = (atoi(row[0]));
		}
		mysql_free_result(result);
		return ret;
	} 
	else
	{
		safe_delete_array(query);
	}
	
	return (0);
}

//o--------------------------------------------------------------
//| loadNodesLinesIntoMemory; Yeahlight, July 19, 2008
//o--------------------------------------------------------------
//| Pulls zoneline data from DB into memory
//o--------------------------------------------------------------
bool Database::loadZoneLines(vector<zoneLine_Struct*>* zone_line_data, char* zoneName)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if(RunQuery(query, MakeAnyLenString(&query, "SELECT zone,x,y,z,target_zone,target_x,target_y,target_z,'range',heading,id,maxZDiff,keepX,keepY,keepZ FROM zone_line_nodes WHERE zone = '%s'", zoneName), errbuf, &result))
	{
		safe_delete_array(query);//delete[] query;

		while(row = mysql_fetch_row(result))
		{
			zoneLine_Struct* zoneLine = new zoneLine_Struct;
			memset(zoneLine, 0, sizeof(zoneLine_Struct));
			
			strcpy(zoneLine->zone, row[0]);
			zoneLine->x = (float)atof(row[1]);
			zoneLine->y = (float)atof(row[2]);
			zoneLine->z = (float)atof(row[3]);
			strcpy(zoneLine->target_zone, row[4]);
			zoneLine->target_x = (float)atof(row[5]);
			zoneLine->target_y = (float)atof(row[6]);
			zoneLine->target_z = (float)atof(row[7]);
			zoneLine->range = atoi(row[8]);
			zoneLine->heading = (int8)atof(row[9]);
			zoneLine->id = atoi(row[10]);
			zoneLine->maxZDiff = atoi(row[11]);
			zoneLine->keepX = atoi(row[12]);
			zoneLine->keepY = atoi(row[13]);
			zoneLine->keepZ = atoi(row[14]);

			zone_line_data->push_back(zoneLine);
		}
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "ZoneLine failed with a bad query: '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
	}

	return false;
}

//o--------------------------------------------------------------
//| LogAccountInPartI; Yeahlight, Aug 3, 2008
//o--------------------------------------------------------------
//| Part I of IV to account log in process
//o--------------------------------------------------------------
bool Database::LogAccountInPartI(int32 ip, const char* WorldAccount)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO active_accounts (account, ip, lastAction) VALUES ('%s','%i',UNIX_TIMESTAMP())", WorldAccount, ip), errbuf))
	{
		cerr << "Error in LogAccountInPartI query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;
	return true;
}

//o--------------------------------------------------------------
//| LogAccountInPartII; Yeahlight, Aug 23, 2008
//o--------------------------------------------------------------
//| Part II of IV to account log in process
//o--------------------------------------------------------------
bool Database::LogAccountInPartII(int32 lsaccount_id, int32 ip)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE active_accounts SET lsaccount = '%i' WHERE ip = '%i' AND lsaccount = 0", lsaccount_id, ip), errbuf))
	{
		cerr << "Error in LogAccountInPartII query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;
	return true;
}

//o--------------------------------------------------------------
//| LogAccountInPartIII; Yeahlight, Aug 3, 2008
//o--------------------------------------------------------------
//| Part III of IV to account log in process
//o--------------------------------------------------------------
bool Database::LogAccountInPartIII(int32 lsaccount_id, int32 ip)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE active_accounts SET status = 'ENTERING_ZONE', lastAction = UNIX_TIMESTAMP() WHERE ip = '%i' AND lsaccount = '%i'", ip, lsaccount_id), errbuf))
	{
		cerr << "Error in LogAccountInPartIII(2) query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;
	return true;
}

//o--------------------------------------------------------------
//| LogAccountInPartI; Yeahlight, Aug 22, 2008
//o--------------------------------------------------------------
//| Part IV of IV to account log in process
//o--------------------------------------------------------------
bool Database::LogAccountInPartIV(int32 lsaccount_id, int32 ip)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE active_accounts SET status = 'IN_ZONE' WHERE ip = '%i' AND lsaccount = '%i'", ip, lsaccount_id), errbuf))
	{
		cerr << "Error in LogAccountInPartIV query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;
	return true;
}

//o--------------------------------------------------------------
//| LogAccountOut; Yeahlight, Aug 3, 2008
//o--------------------------------------------------------------
//| Logs account of out active_accounts
//o--------------------------------------------------------------
bool Database::LogAccountOut(int32 account_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM active_accounts WHERE lsaccount = '%i'", account_id), errbuf))
	{
		cerr << "Error in LogAccountOut query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;
	return true;
}

//o--------------------------------------------------------------
//| TruncateActiveAccounts; Yeahlight, Aug 7, 2008
//o--------------------------------------------------------------
//| Truncates the active_accounts table
//o--------------------------------------------------------------
bool Database::TruncateActiveAccounts()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "TRUNCATE TABLE active_accounts"), errbuf)){
		cerr << "Error in TruncateActiveAccounts query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;
	return true;
}

//o--------------------------------------------------------------
//| PurgeStuckAccounts; Yeahlight, Aug 22, 2008
//o--------------------------------------------------------------
//| Drops accounts from the active_accounts table that have
//| been attempting to zone for more than 120 seconds
//o--------------------------------------------------------------
bool Database::PurgeStuckAccounts()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	time_t now = time(NULL);
	//Yeahlight: Drop accounts which have not finished zoning and have been in this state for at least 120 seconds.
	if(!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM active_accounts WHERE status = 'ENTERING_ZONE' AND abs(lastAction - '%i') > 120", now), errbuf))
	{
		cerr << "Error in PurgeStuckAccounts(2) query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	safe_delete_array(query);//delete[] query;
	return true;
}

//o--------------------------------------------------------------
//| GetCorpseNames; Yeahlight, June 27, 2009
//o--------------------------------------------------------------
//| Retrieves the unique list of the account IDs tied to corpses
//o--------------------------------------------------------------
queue<int32> Database::GetCorpseAccounts()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	queue<int32> rotList;
	//Yeahlight: Select all the account IDs of the corpses from the corpse table
	if(RunQuery(query, MakeAnyLenString(&query, "SELECT DISTINCT accountid FROM player_corpses"), errbuf, &result))
	{
		while(row = mysql_fetch_row(result))
		{
			rotList.push(atoi(row[0]));
		}
		mysql_free_result(result);
	}
	//Yeahlight: Query was not successful
	else
	{
		cerr << "Error in GetCorpseAccounts query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);
	return rotList;
}

//o--------------------------------------------------------------
//| DecrementCorpseRotTimer; Yeahlight, June 27, 2009
//o--------------------------------------------------------------
//| Decrements all the corpses in the zone for the supplied
//| character name
//o--------------------------------------------------------------
bool Database::DecrementCorpseRotTimer(int32 accountid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	//Yeahlight: See if this account is currently logged in
	if(RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM active_accounts WHERE lsaccount = %i", accountid), errbuf, &result))
	{
		char errbuf2[MYSQL_ERRMSG_SIZE];
		char *query2 = 0;
		safe_delete_array(query);
		//Yeahlight: This account is logged in, so accelerate their corpse timers beyond normal rot time and decrement the rez timer at the standard rate
		if(mysql_num_rows(result) > 0)
		{
			if(!RunQuery(query2, MakeAnyLenString(&query2, "UPDATE player_corpses SET time = time - %i, reztime = reztime - %i WHERE accountid = %i", PC_CORPSE_INCREMENT_DECAY_TIMER, (PC_CORPSE_INCREMENT_DECAY_TIMER/7), accountid), errbuf2))
			{
				cerr << "Error in DecrementCorpseRotTimer(2) query '" << query2 << "' " << errbuf2 << endl;
				safe_delete_array(query2);
				mysql_free_result(result);
				return false;
			}
		}
		//Yeahlight: This account is not logged, so decrement their corpse timers at the standard rate
		else
		{
			if(!RunQuery(query2, MakeAnyLenString(&query2, "UPDATE player_corpses SET time = time - %i WHERE accountid = %i", (PC_CORPSE_INCREMENT_DECAY_TIMER/7), accountid), errbuf2))
			{
				cerr << "Error in DecrementCorpseRotTimer(3) query '" << query2 << "' " << errbuf2 << endl;
				safe_delete_array(query2);
				mysql_free_result(result);
				return false;
			}
		}
		safe_delete_array(query2);
		mysql_free_result(result);
	}
	//Yeahlight: Query was not successful
	else
	{
		cerr << "Error in DecrementCorpseRotTimer query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return true;
}

//o--------------------------------------------------------------
//| GetPCCorpseRotTime; Yeahlight, June 27, 2009
//o--------------------------------------------------------------
//| Grabs the corpse decay timer for the supplied corpse id
//o--------------------------------------------------------------
int32 Database::GetPCCorpseRotTime(int32 dbid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int32 ret = 0;
	//Yeahlight: See if this account is currently logged in
	if(RunQuery(query, MakeAnyLenString(&query, "SELECT time FROM player_corpses WHERE id = %i", dbid), errbuf, &result))
	{
		row = mysql_fetch_row(result);
		if(row)
		{
			ret = atoi(row[0]);
		}
		mysql_free_result(result);
	}
	//Yeahlight: Query was not successful
	else
	{
		cerr << "Error in GetPCCorpseRotTime query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);
	return ret;
}

const NPCFactionList* Database::GetNPCFactionList(uint32 id) {
	if (id <= npcfactionlist_max && npcfactionlist_array){
		return npcfactionlist_array[id];
	}
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
				safe_delete_array(query);//delete[] query;
				return false;
			}
			i = 0;
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT npc_faction_id, faction_id, value FROM npc_faction_entries order by npc_faction_id"), errbuf, &result)) {
				safe_delete(query);
				int32 curflid = -1;
				int32 tmpflid = -1;
				while((row = mysql_fetch_row(result))) {
					tmpflid = atoi(row[0]);
					if (curflid != tmpflid) {
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
				safe_delete_array(query);//delete[] query;
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
		safe_delete_array(query);//delete[] query;
		return false;
	}
	return true;
}

//o--------------------------------------------------------------
//| FindMyZoneInLocation; Yeahlight, Aug 15, 2008
//o--------------------------------------------------------------
//| Returns the zoneline destination location between two
//| zones
//o--------------------------------------------------------------
bool Database::FindMyZoneInLocation(const char* zone, const char* zoneLine, float& x, float& y, float& z, int8& heading)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	//Yeahlight: GM does not have a zoneline preference, pick one for them
	if(strcmp(zoneLine, "ANY") == 0){
		if (RunQuery(query, MakeAnyLenString(&query, "SELECT target_x, target_y, target_z, heading FROM zone_line_nodes WHERE target_zone = '%s'", zone), errbuf, &result)) 
		{
			row = mysql_fetch_row(result);
			if(row)
			{
				x = (float)atof(row[0]);
				y = (float)atof(row[1]);
				z = (float)atof(row[2]) * 10;
				heading = atoi(row[3]);
			}
			else
			{
				x = -1;
				y = -1;
				z = -1;
			}
			safe_delete_array(query);//delete[] query;
			mysql_free_result(result);
		} 
		else
		{
			cerr << "Error in FindMyZoneInLocation query '" << query << "' " << errbuf << endl;
			safe_delete_array(query);//delete[] query;
			return false;
		}
	}
	else
	{
		if (RunQuery(query, MakeAnyLenString(&query, "SELECT target_x, target_y, target_z, heading FROM zone_line_nodes WHERE zone = '%s' AND target_zone = '%s'", zoneLine, zone), errbuf, &result)) 
		{
			row = mysql_fetch_row(result);
			if(row)
			{
				x = (float)atof(row[0]);
				y = (float)atof(row[1]);
				z = (float)atof(row[2]) * 10;
				heading = atoi(row[3]);
			}
			else
			{
				x = -1;
				y = -1;
				z = -1;
			}
			safe_delete_array(query);//delete[] query;
			mysql_free_result(result);
		} 
		else
		{
			cerr << "Error in FindMyZoneInLocation query '" << query << "' " << errbuf << endl;
			safe_delete_array(query);//delete[] query;
			return false;
		}
	}
	return true;
}

//o--------------------------------------------------------------
//| GetSpawnGroupID; Yeahlight, Aug 30, 2008
//o--------------------------------------------------------------
//| Returns the spawngroupID of an NPC
//o--------------------------------------------------------------
int32 Database::GetSpawnGroupID(int32 NPCtype_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int32 spawngroupID = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT spawngroupID FROM spawnentry WHERE npcID = '%i'", NPCtype_id), errbuf, &result))
	{
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row)
		{
			spawngroupID = atoi(row[0]);
			mysql_free_result(result);
			return spawngroupID;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
	}
	else
	{
		cerr << "Error in GetSpawnGroupID query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	return 0;
}

//o--------------------------------------------------------------
//| AddRoamingRange; Yeahlight, Aug 30, 2008
//o--------------------------------------------------------------
//| Updates the roamRange field for a set of spawngroupIDs in
//| table spawn2.
//o--------------------------------------------------------------
bool Database::AddRoamingRange(int32 spawngroupID, int range)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE spawn2 SET roamRange = '%i' WHERE spawngroupID = '%i'", range, spawngroupID), errbuf))
	{
		cerr << "Error in AddRoamingRange query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	return true;
}

// Cofruben: 17/08/08.
bool Database::GetDebugList(int32 char_id, TDebugList* l) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	TDebugList* DList = NULL;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT debug_system FROM character_ WHERE id='%i'", char_id), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			DList = (TDebugList*) row[0];
			if (DList)
				memcpy(l, DList, sizeof(TDebugList));
		}
		mysql_free_result(result);
	}
	else
		safe_delete_array(query);//delete[] query;
	return (DList != NULL);
}

void Database::SetDebugList(int32 char_id, TDebugList* l) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char query[sizeof(TDebugList) + 500];
	int32 len = 0;
	memset(query, 0, sizeof(query));
	strcpy(query, "UPDATE character_ SET debug_system = '");
	len += strlen("UPDATE character_ SET debug_system = '");
	memcpy(query + len, l, sizeof(TDebugList));
	len += sizeof(TDebugList);
	len += sprintf(query + len, "' WHERE id = '%i'", char_id);
	RunQuery(query, len, errbuf);
}
//////////

//o--------------------------------------------------------------
//| LoadRoamBoxes; Yeahlight, Sept 1, 2008
//o--------------------------------------------------------------
//| Loads the roam boxes for a specific zone
//o--------------------------------------------------------------
bool Database::LoadRoamBoxes(const char* zone, RoamBox_Struct roamBoxes[], int16& numberOfRoamBoxes)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, minX, maxX, minY, maxY FROM zone_roam_boxes WHERE zone = '%s'", zone), errbuf, &result))
	{
		safe_delete_array(query);
		numberOfRoamBoxes = 0;
		while(row = mysql_fetch_row(result))
		{
			roamBoxes[numberOfRoamBoxes].id = atoi(row[0]);
			roamBoxes[numberOfRoamBoxes].min_x = atof(row[1]);
			roamBoxes[numberOfRoamBoxes].max_x = atof(row[2]);
			roamBoxes[numberOfRoamBoxes].min_y = atof(row[3]);
			roamBoxes[numberOfRoamBoxes].max_y = atof(row[4]);
			numberOfRoamBoxes++;
		}
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "Error in LoadRoamBoxes query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return false;
}

//o--------------------------------------------------------------
//| LoadZoneID; Yeahlight, Oct 18, 2008
//o--------------------------------------------------------------
//| Loads the zoneID for a specific zone
//o--------------------------------------------------------------
int16 Database::LoadZoneID(const char* zone)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int16 ret = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT zoneidnumber FROM zone_ids WHERE short_name = '%s'", zone), errbuf, &result))
	{
		safe_delete_array(query);
		while(row = mysql_fetch_row(result))
		{
			ret = atoi(row[0]);
		}
		mysql_free_result(result);
		return ret;
	}
	else
	{
		cerr << "Error in LoadZoneID query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	return 0;
}

//o--------------------------------------------------------------
//| LoadNPCSpells; Yeahlight, Oct 23, 2008
//o--------------------------------------------------------------
//| Loads the spell set for an NPC based on class and level
//o--------------------------------------------------------------
bool Database::LoadNPCSpells(int8 NPCclass, int8 NPClevel, int16 buffs[], int16 debuffs[])
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	//Yeahlight: "Null" the spell sets
	for(int i = 0; i < 4; i++)
	{
		buffs[i] = NULL_SPELL;
		debuffs[i] = NULL_SPELL;
	}

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT level, buff1, buff2, heal, gate, debuff1, debuff2, debuff3, debuff4 FROM npc_spells WHERE class = '%i'", (int16)NPCclass), errbuf, &result))
	{
		safe_delete_array(query);
		while(row = mysql_fetch_row(result))
		{
			if((int16)NPClevel - atoi(row[0]) >= 0 && (int16)NPClevel - atoi(row[0]) < 5)
			{
				buffs[0] = atoi(row[1]);
				buffs[1] = atoi(row[2]);
				buffs[2] = atoi(row[3]);
				buffs[3] = atoi(row[4]);
				debuffs[0] = atoi(row[5]);
				debuffs[1] = atoi(row[6]);
				debuffs[2] = atoi(row[7]);
				debuffs[3] = atoi(row[8]);
				mysql_free_result(result);
				return true;
			}
		}
		mysql_free_result(result);
		return false;
	}
	else
	{
		cerr << "Error in LoadNPCSpells query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return false;
}

/*
//o--------------------------------------------------------------
//| AddPlayerToBoat; Wizzel, Nov 4, 2008
//o--------------------------------------------------------------
//| Adds players who have boarded a boat to the database
//o--------------------------------------------------------------

bool Database::AddPlayerToBoat(char* name, int8 boat_id) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO boat_passengers (player_name, boat_id) values ('%s', %i)", name, boat_id), errbuf, 0, &affected_rows)) {
		cerr << "Error in AddPlayerToBoat query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return true;
	}

	safe_delete_array(query);

	if (affected_rows == 0) {
		return false;
	}
	return true;
}

//o--------------------------------------------------------------
//| Convert PlayerID to String, Nov 6, 2008
//o--------------------------------------------------------------
//| Takes a player ID and retrieves a name
//o--------------------------------------------------------------
/*void Database::IDtoString(int32 character_id, char* seamen)
{
        char errbuf[MYSQL_ERRMSG_SIZE];
        char *query = 0;
        MYSQL_RES *result;
        MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name FROM character_ WHERE id=%i", character_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			strcpy(seamen,row[0]);
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in boat passenger query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
}*/

//o--------------------------------------------------------------
//| RemovePlayerFromBoat; Wizzel, Nov 4, 2008
//o--------------------------------------------------------------
//| Removes players who have left a boat from the database tables
//o--------------------------------------------------------------

/*
bool Database::RemovePlayerFromBoat(char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE from boat_passengers WHERE player_name='%s'", name), errbuf)) {
		cerr << "Error in RemovePlayerFromBoat query '" << query << "' " << errbuf << endl;
		if (query != 0)
			safe_delete_array(query);
		return false;
	
	}
	safe_delete_array(query);
	query = 0;

	return true;
}

//o--------------------------------------------------------------
//| Find Players on Boat; Wizzel, Nov 6, 2008
//o--------------------------------------------------------------
//| Takes a boat_id and retrieves the players on that boat
//o--------------------------------------------------------------
void Database::FindPlayersOnBoat(int8 boat_id, char* passenger)
{
    char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT player_name FROM boat_passengers WHERE boat_id=%i", boat_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			strcpy(passenger,row[0]);
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in boat passenger query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
}*/


//o--------------------------------------------------------------
//| Worldkick; Yeahlight, Jan 7, 2009
//o--------------------------------------------------------------
//| Kicks an account out of the active_accounts table
//o--------------------------------------------------------------
bool Database::Worldkick(int16 LSaccountID)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM active_accounts WHERE lsaccount=%i", LSaccountID), errbuf))
	{
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	return true;
}

//o--------------------------------------------------------------
//| AdjustFaction; Yeahlight, Jan 14, 2009
//o--------------------------------------------------------------
//| Adjusts the faction value for the supplied character ID
//o--------------------------------------------------------------
bool Database::AdjustFaction(int16 characterID, int16 pFaction, sint16 newValue)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE faction_values SET current_value = %i WHERE char_id = %i AND faction_id = %i", newValue, characterID, pFaction), errbuf, 0, &affected_rows))
	{
		safe_delete_array(query);
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else
	{
		cerr << "Error in AdjustFaction query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return false;
}

//o--------------------------------------------------------------
//| LoadZoneRules; Yeahlight, Jan 22, 2009
//o--------------------------------------------------------------
//| Loads the casting rules for the zone
//o--------------------------------------------------------------
bool Database::LoadZoneRules(char* zonename, int8& bindCondition, int8& levCondition, bool& outDoorZone)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if(RunQuery(query, MakeAnyLenString(&query, "SELECT can_bind, can_lev, castoutdoor FROM zone_rules WHERE short_name = '%s'", zonename), errbuf, &result))
	{
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if(row)
		{
			bindCondition = atoi(row[0]);
			levCondition = atoi(row[1]);
			if(atoi(row[2]))
				outDoorZone = true;
			else
				outDoorZone = false;
		}
		mysql_free_result(result);
		return true;
	}
	else 
	{
		cerr << "LoadZoneRules failed with a bad query: '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return false;
}


//////////

//o--------------------------------------------------------------
//| LoadBoatZones; Tazadar, June 9, 2009
//o--------------------------------------------------------------
//| Loads the boat zones for a specific boat
//o--------------------------------------------------------------
bool Database::LoadBoatZones(const char* boat, vector<string> &boat_zones)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	string tmp;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT zone FROM boat_infos WHERE boat_name = '%s'", boat), errbuf, &result))
	{
		safe_delete_array(query);
		while(row = mysql_fetch_row(result))
		{
			tmp=row[0];
			boat_zones.push_back(tmp);
		}
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "Error in LoadBoatZones query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return false;
}


//////////

//o--------------------------------------------------------------
//| LoadBoatData; Tazadar, June 16, 2009
//o--------------------------------------------------------------
//| Loads the boat data
//o--------------------------------------------------------------
bool Database::LoadBoatData(const char* boatname, NPCType& boat,bool& lineroute)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT race,gender,texture,body,size,lineroute FROM boats WHERE name = '%s'", boatname), errbuf, &result))
	{
		safe_delete_array(query);
		while(row = mysql_fetch_row(result))
		{
			memset(&boat, 0, sizeof(NPCType));
			strcpy(boat.name,boatname);
			boat.cur_hp = 50000; 
			boat.max_hp = 50000; 
			boat.race = atoi(row[0]); 
			boat.gender = atoi(row[1]); 
			boat.class_ = 1; 
			boat.deity= 1; 
			boat.level = 99; 
			boat.texture = atoi(row[2]);
			boat.runspeed = 1.25;
			boat.body_type =  (TBodyType)atoi(row[3]);
			boat.size =  atoi(row[4]);
			lineroute = (bool)atoi(row[5]);
		}
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "Error in LoadBoatData query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return false;
}

//o--------------------------------------------------------------
//| GetZoneShortName; Yeahlight, July 05, 2009
//o--------------------------------------------------------------
//| Grabs the zone's shortname given a zoneID
//o--------------------------------------------------------------
bool Database::GetZoneShortName(int16 zoneID, char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if(RunQuery(query, MakeAnyLenString(&query, "SELECT short_name FROM zone_ids WHERE zoneidnumber = %i", zoneID), errbuf, &result))
	{
		safe_delete_array(query);
		while(row = mysql_fetch_row(result))
		{
			if(row[0])
			{
				strcpy(name, row[0]);
				mysql_free_result(result);
				return true;
			}
		}
		mysql_free_result(result);
		return false;
	}
	else
	{
		cerr << "Error in GetZoneShortName query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return false;
}

	/********************************************************************
	 *                      Kibanu - 6/26/2009                          *
	 ********************************************************************
	 *                          UpdateTimeOfDay							*
	 ********************************************************************
	 *  Updates the current hour/day/month/year						    *
	 *																	*
	 ********************************************************************/
bool Database::UpdateTimeOfDay(int8 hour, int8 day, int8 month, int16 year) {
	char	errbuf[MYSQL_ERRMSG_SIZE]; 
	char	*query = 0; 
	int32   affected_rows = 0; 

	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE time_of_day SET hour=%i, day=%i, month=%i, year=%i", hour, day, month, year), errbuf, 0, &affected_rows)) { 
		cerr << "Error running query " << query << endl; 
		safe_delete_array(query);//delete[] query; 
		return false; 
	} 
	safe_delete_array(query);//delete[] query; 

	if (affected_rows == 0) 
	{ 
		return false; 
	} 

	return true; 
}

	/********************************************************************
	 *                      Kibanu - 7/28/2009                          *
	 ********************************************************************
	 *                          GetTimeOfDay							*
	 ********************************************************************
	 *  Returns the current hour/day/month/year						    *
	 *																	*
	 ********************************************************************/
bool Database::LoadTimeOfDay(int8* hour, int8* day, int8* month, int16* year) {
	char	errbuf[MYSQL_ERRMSG_SIZE]; 
	char	*query = 0; 
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT hour, day, month, year FROM time_of_day" ), errbuf, &result))
	{
		safe_delete_array(query);
		while(row = mysql_fetch_row(result))
		{
			*hour = atoi(row[0]);
			*day = atoi(row[1]);
			*month = atoi(row[2]);
			*year = atoi(row[3]); 
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetTimeOfDay query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return true;
}

	/********************************************************************
	 *                      Kibanu - 7/28/2009                          *
	 ********************************************************************
	 *                          SetDaytime							    *
	 ********************************************************************
	 *  Sets day / night flag in database                               *
	 *																	*
	 ********************************************************************/
void Database::SetDaytime(bool isDaytime) {
	char	errbuf[MYSQL_ERRMSG_SIZE]; 
	char	*query = 0;
	int32   affected_rows = 0; 

	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE time_of_day SET is_daytime = %i", ( isDaytime ? 1 : 0 )), errbuf, 0, &affected_rows))
	{
		cerr << "Error in SetDaytime query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
}

	/********************************************************************
	 *                      Kibanu - 7/28/2009                          *
	 ********************************************************************
	 *                          IsDaytime							    *
	 ********************************************************************
	 *  Returns the whether the server is day or night time             *
	 *																	*
	 ********************************************************************/
bool Database::IsDaytime() {
	char	errbuf[MYSQL_ERRMSG_SIZE]; 
	char	*query = 0; 
	MYSQL_RES *result;
	MYSQL_ROW row;
	bool	ret = false;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT is_daytime FROM time_of_day" ), errbuf, &result))
	{
		safe_delete_array(query);
		while(row = mysql_fetch_row(result))
		{
			if ( atoi(row[0]) == 1 )
				ret = true;
			else
				ret = false;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in IsDaytime query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return ret;
}

/********************************************************************
 *                      Harakiri - 8/07/2009                        *
 ********************************************************************
 *                          GetMessages							    *
 ********************************************************************
 *  Returns the messages of the message board			            *
 *																	*
 ********************************************************************/
bool Database::GetMBMessages(int category,std::vector<MBMessage_Struct*>* messageList) {

     // 1000 messages take the client about 5sec to retrieve
    int MAX_RESULT=500;
    char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
 
        query = new char[256];                                          
 
        if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, date, author, language,subject,category from mb_messages WHERE category=%i ORDER BY time DESC LIMIT %i",category,MAX_RESULT), errbuf, &result)) {
                safe_delete(query);
 
                int numMessages = mysql_num_rows(result);                               
                             
                //cout << "Found " << numMessages << " messages" << endl;             				                
                
				while((row = mysql_fetch_row(result))) {               					
					MBMessage_Struct* message = new MBMessage_Struct;	
					memset(message, 0, sizeof(MBMessage_Struct));
					
					// int32 atol
					message->id = atol(row[0]);			
					strcpy(message->date, row[1]);			
					strcpy(message->author, row[2]);
					message->language = atoi(row[3]);
					strcpy(message->subject, row[4]);
					message->category = atoi(row[5]);						
					Sleep(0);
					messageList->push_back(message);
                }
                mysql_free_result(result);
        }
        else {
                cerr << "Error in GetMBMessages '" << query << "' " << errbuf << endl;
                safe_delete_array(query);//delete[] query;
                return false;
        }
 
        return true;
}

/********************************************************************
 *                      Harakiri - 8/07/2009                        *
 ********************************************************************
 *                          GetMBMessage						    *
 ********************************************************************
 *  Returns a specific message of the message board			        *
 *																	*
 ********************************************************************/
bool Database::GetMBMessage(int category, int id, char* messageText) {	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int i;
	query = new char[256];						

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT message from mb_messages WHERE category=%i AND id=%i",category,id), errbuf, &result)) {
		safe_delete(query);

		int numMessages = mysql_num_rows(result);
		
		//cout << "Found " << numMessages << " messages" << endl;

		if(numMessages!=1) {
			cerr << "Error GetMBMessage, message  " << id << " in category " << category << " not found " << errbuf << endl;
			return false;
		}
				
		while((row = mysql_fetch_row(result))) {									
			strcpy(messageText, row[0]);						
			Sleep(0);		
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetMBMessage '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	return true;
}

/********************************************************************
 *                      Harakiri - 8/07/2009                        *
 ********************************************************************
 *                          AddMBMessage						    *
 ********************************************************************
 * Add a message the message board								    *
 *																	*
 ********************************************************************/
bool Database::AddMBMessage(MBMessage_Struct* message) {	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;	
	int32 affected_rows = 0;
	query = new char[256];			

	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT MAX(id) FROM mb_messages WHERE category=%i", message->category), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);

			uint32 id = 0;

			// will be not null if at least one message posted in category X
			if (row[0] != 0) {
				id = atol(row[0]);
			}
			
			id++;

			if (RunQuery(query, MakeAnyLenString(&query, "insert into mb_messages SET id=%u, date='%s', author='%s', language=%i, subject='%s', category=%i, message='%s', time=UNIX_TIMESTAMP()",id,message->date,message->author, message->language, message->subject, message->category, message->message), errbuf, 0, &affected_rows )) {				
				safe_delete(query);						
			} else {
				cerr << "Error in AddMBMessage '" << query << "' " << errbuf << endl;
				safe_delete_array(query);//delete[s] query;
				return false;
			}

			if (affected_rows == 0)
			{	
				safe_delete_array(query);//delete[] query;
				return false;
			}
			
		}
	} else {
		cerr << "Error in AddMBMessage '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	return true;
}

/********************************************************************
 *                      Harakiri - 8/07/2009                        *
 ********************************************************************
 *                          DeleteMBMessage						    *
 ********************************************************************
 * Delete a message from the message board						    *
 *																	*
 ********************************************************************/
bool Database::DeleteMBMessage(int category, int id) {	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];						
	int32 affected_rows = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "delete from mb_messages WHERE id=%u AND category=%i",id,category), errbuf, 0, &affected_rows)) {
		safe_delete(query);	
	}
	else {
		cerr << "Error in DeleteMBMessage '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}

	if (affected_rows == 0)
	{	
		safe_delete_array(query);//delete[] query;
		return false;
	}
			

	return true;
}

	/********************************************************************
	 *                      Kibanu - 8/12/2009                          *
	 ********************************************************************
	 *                    CheckNPCTypeSpawnLimit						*
	 ********************************************************************
	 *  Returns the spawn_limit in the database for a given NPC Type.   *
	 *																	*
	 ********************************************************************/
int8 Database::CheckNPCTypeSpawnLimit(int32 NPCTypeID) {
	char		errbuf[MYSQL_ERRMSG_SIZE]; 
	char		*query = 0; 
	MYSQL_RES	*result;
	MYSQL_ROW	row;
	
	if (max_npc_type == 0 || NPCTypeID > max_npc_type || NPCTypeID < 0)
		return 0;

	if (!npc_type_array || !npc_type_array[NPCTypeID])
		return 0;
	
	return npc_type_array[NPCTypeID]->spawn_limit;


}
void Database::GetZoneCFG(int16 zoneID, int8* sky, int8* fog_red, int8* fog_green, int8* fog_blue, float* fog_minclip, float* fog_maxclip, float* minclip, float* maxclip, uint8* ztype, float* safe_x, float* safe_y, float* safe_z, float* underworld)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	//Wizzel: Finally fixed this damn query. Was originally trying to select from zonevars where short_name = short_name. Dummy.
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT sky, fog_red, fog_green, fog_blue, fog_minclip, fog_maxclip, minclip, maxclip, ztype, safe_x, safe_y, safe_z, underworld from zonevars where zoneID=%i",zoneID), errbuf, &result)) 
	{
		safe_delete_array(query);//delete[] query;
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			
			//Wizzel: Grabs the returned values.
			*sky = atoi(row[0]);
			*fog_red = atoi(row[1]);
			*fog_green = atoi(row[2]);
			*fog_blue = atoi(row[3]);
			*fog_minclip = atof(row[4]);
			*fog_maxclip = atof(row[5]);
			*minclip = atof(row[6]);
			*maxclip = atof(row[7]);
			*ztype = atoi(row[8]);
			*safe_x = atof(row[9]);
			*safe_y = atof(row[10]);
			*safe_z = atof(row[11]);
			*underworld = atof(row[12]);
			mysql_free_result(result);
			return;

		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetZoneCFG query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
	}
}