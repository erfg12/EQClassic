#include "../common/debug.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysqld_error.h>
#ifdef WIN32
	#include <windows.h>
	#define snprintf	_snprintf
	//#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
#else
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include "../common/database.h"
#include "../common/servertalk.h"
#include "../common/md5.h"
#include "login_structs.h"
#include "EQCrypto.h"
extern Database database;
extern EQCrypto eq_crypto;

using namespace std;

#define DBPasswordScrambleded

int32 Database::GetLSLoginInfo(char* iUsername, char* oPassword, int8* lsadmin, int* lsstatus, bool* verified, sint16* worldadmin) { //newage: modified this to work
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	char tmp[201];
	if (strlen(iUsername) >= 100)
		return 0;

	DoEscapeString(tmp, iUsername, strlen(iUsername));

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, name, password, lsadmin, lsstatus, worldadmin,user_active from login_accounts where name like '%s'", tmp), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (row[0] == 0 || row[1] == 0 || row[2] == 0 || row[3] == 0 || row[4] == 0 || row[5] == 0) {
				return 0;
			}
			int32 tmp = atoi(row[0]);
			strcpy(iUsername, row[1]);
			if (oPassword)
				strcpy(oPassword, row[2]);
			if (lsadmin)
				*lsadmin = atoi(row[3]);
			if (lsstatus)
				*lsstatus = atoi(row[4]);
			if (verified && atoi(row[6]) !=0)
				*verified = true;
			else if(verified)
				*verified = false;
			if (worldadmin)
				*worldadmin = atoi(row[5]);
			mysql_free_result(result);
			return tmp;
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetLSLoginInfo query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}

	return 0;
}

/*
	Set the preauth in preperation for a new account being created
*/
bool Database::SetLSAuthChange(int32 account_id, const char* ip) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;

	if (RunQuery(query, MakeAnyLenString(&query, "INSERT INTO login_authchange SET account_id=%i, ip='%s';", account_id, ip), errbuf, &result)) {
		cerr << "Error in SetLSAuthChange query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}

	delete[] query;
	return true;
}

bool Database::UpdateLSAccountAuth(int32 account_id, int8* auth) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+AUTH_SIZE];
	char* end = query;
	int32 affected_rows = 0;

    end += sprintf(end, "Update login_accounts SET auth=");
    *end++ = '\'';
    end += DoEscapeString(end, (char*)auth, AUTH_SIZE);
    *end++ = '\'';
    end += sprintf(end," WHERE id=%d", account_id);

	if (RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
		if (affected_rows == 1)
			return true;
		else {
	        cerr << "Error in UpdateLSAccountAuth query, affected_rows = 0 " << endl;
			return false;
		}
	}
	else {
        cerr << "Error in UpdateLSAccountAuth query " << errbuf << endl;
		return false;
    }

	return false;
}


int32 Database::GetLSAuthentication(int8* auth) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+AUTH_SIZE];
	char query2[256+AUTH_SIZE];
	char* end = query;
	char* end2 = query2;
    MYSQL_RES *result;
    MYSQL_ROW row;
    end += sprintf(end, "Select id from login_accounts where auth=");
    *end++ = '\'';
    end += DoEscapeString(end, (char*)auth, AUTH_SIZE);
    *end++ = '\'';
	end2 += sprintf(end2, "insert into login_accounts  values(%i,",rand()%1000);
    *end2++ = '\'';
    end2 += DoEscapeString(end2, (char*)auth, AUTH_SIZE);
    *end2++ = '\'';
	*end2++ = ')';
	cerr << query << endl;
	cerr << query2 << endl;

	

    if (RunQuery(query, (int32) (end - query), errbuf, &result)) {
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 account_id = atoi(row[0]);
			mysql_free_result(result);
			return account_id;
		}
		else
		{
			RunQuery(query2, (int32) (end2 - query2), errbuf, &result);
			mysql_free_result(result);
			cerr << "Error in GetLSAuthentication query " << errbuf << endl;
			return 0;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetLSAuthentication query " << errbuf << endl;
		return 0;
	}

	return 0;
}

int32 Database::GetLSAuthChange(const char* ip) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "Select account_id from login_authchange where ip='%s' AND UNIX_TIMESTAMP()-UNIX_TIMESTAMP(time) < %i Order By time desc;", ip, AUTHCHANGE_TIMEOUT), errbuf, &result)) {
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
		cerr << "Error in GetLSAuthChange query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	return 0;
}

bool Database::RemoveLSAuthChange(int32 account_id) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "Delete from login_authchange where account_id=%i", account_id), errbuf)) {
		cerr << "Error in RemoveLSAuthChange query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}

	delete[] query;
	return true;
}

bool Database::ClearLSAuthChange() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[] = "Delete from login_authchange";

	if (!RunQuery(query, strlen(query), errbuf)) {
		cerr << "Error in ClearLSAuthChange query '" << query << "' " << errbuf << endl;
		return false;
	}
	return true;
}

bool Database::GetLSAccountInfo(int32 account_id, char* name, int8* lsadmin, int* lsstatus, bool* verified, sint16* worldadmin) {
	if (account_id == 0)
		return false;

#ifdef PUBLICLOGIN
	if(account_id > 12)
		return false;
#endif
	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "Select id, name, lsadmin, lsstatus, worldadmin from login_accounts where id=%i", account_id), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			strcpy(name, row[1]);
			if (lsadmin)
				*lsadmin = atoi(row[2]);
			if (lsstatus)
				*lsstatus = atoi(row[3]);
			if (verified && atoi(row[3]) <= 100)
				*verified = true;
			if (row[4] && worldadmin)
				*worldadmin = atoi(row[4]);
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetLSAccountName query '" << query << "' " << errbuf<< endl;
		delete[] query;
	}
	return false;
}

// Checks the Worldserver's authentication, returns 1 if accepted, 0 if account doesnt exist, -1 if bad password/error, (2=chat, 3=login)
sint8 Database::CheckWorldAuth(const char* account, const char* password, int32* account_id, int32* admin_id, bool* GreenName, bool* ShowDown) {
	if (account == 0 || password == 0)
		return false;

	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "Select id, password, admin_id, greenname, showdown, chat from login_worldservers where account like '%s'", account), errbuf, &result)) {
		delete[] query;
		if (mysql_num_rows(result) == 0) {
			*account_id = 0;
			*admin_id = 0;
			*GreenName = false;
			*ShowDown = false;
			mysql_free_result(result);
			return 0; // code for login doesnt exist
		}
		else if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (strcasecmp(row[1], password) == 0) {
				*account_id = atoi(row[0]);
				*admin_id = atoi(row[2]);
				*GreenName = (atoi(row[3]) == 1);
				*ShowDown = (atoi(row[4]) == 1);
				sint8 tmp = 1;  // code for login exists and accepted
				if (row[5][0] == '1')
					tmp = 2;
				else if (row[5][0] == '2')
					tmp = 3;
				mysql_free_result(result);
				return tmp;
			}
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in CheckWorldAuth query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return -1; // code for error/bad password
}

bool Database::UpdateWorldName(int32 accountid, const char* name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = new char[256 + (strlen(name) * 2)];
	char* end = query;
	int32 affected_rows = 0;

    end += sprintf(end, "UPDATE login_worldservers SET name=");
    *end++ = '\'';
    end += DoEscapeString(end, name, strlen(name));
    *end++ = '\'';
    end += sprintf(end," WHERE id=%i", accountid);

	if (!RunQuery(query, (end - query), errbuf, 0, &affected_rows)) {
		delete[] query;
		return false;
	}
	delete[] query;

	if (affected_rows == 0) {
		return false;
	}

	return true;
}

bool Database::InsertStats(int32 in_players, int32 in_zones, int32 in_servers) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT into login_stats (players, zones, servers) values(%i,%i,%i)", in_players, in_zones, in_servers), errbuf)) {
		delete[] query;
		return false;
	}
	//printf("Stats:\tPlayers:\t%i\tZones:\t%i\tServers:\t%i\n",in_players, in_zones, in_servers);
	delete[] query;
	return true;
}

// True: Its good!:)
// False: Its bad! :(
bool Database::AddLoginAccount(char* stationname, char* password, char* chathandel, char* cdkey, char* email)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT into login_accounts (name, chat_name, password, verified, authcode, email) values('%s','%s',password('%s'),1,'%s','%s')",stationname, chathandel, password, cdkey, email), errbuf)) {
		delete[] query;
		return false;
	}
	printf("New Account: %s, %s, %s, %s, %s", stationname, password, chathandel, cdkey, email);
	delete[] query;
	return true;
}

// 0=success, 1=oldpass bad, 2=error
int8 Database::ChangeLSPassword(int32 accountid, const char* newpassword, const char* oldpassword) {
	if (accountid == 0 || newpassword == 0)
		return false;

	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	MD5 md5pass;

	if (oldpassword) {
		md5pass.Generate(oldpassword);
		if (RunQuery(query, MakeAnyLenString(&query, "Select id from login_accounts where id=%d and password='%s'", accountid, md5pass), errbuf, &result)) {
			safe_delete(query);
			if (mysql_num_rows(result) == 0) {
				mysql_free_result(result);
				return 1;
			}
		}
		else {
			safe_delete(query);
			cerr << "Error in ChangeLSPassword query '" << query << "' " << errbuf << endl;
			return 2;
		}
	}

	md5pass.Generate(newpassword);

	safe_delete(query);
	if (!RunQuery(query, MakeAnyLenString(&query, "Update login_accounts set password='%s' where id=%d", md5pass, accountid), errbuf)) {
		delete[] query;
		cerr << "Error in ChangeLSPassword query '" << query << "' " << errbuf << endl;
		return 2;
	}

	delete[] query;
	return 0;
}

#if defined(LOGINCRYPTO) && !defined(MINILOGIN) && !defined(PUBLICLOGIN)
int32 Database::CheckEQLogin(const APPLAYER* app, char* oUsername, int8* lsadmin, int* lsstatus, bool* verified, sint16* worldadmin,int32 ip) //newage: fixed this
{
	uchar eqlogin[40];
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	eq_crypto.DoEQDecrypt(app->pBuffer, eqlogin, 40);

	LoginCrypt_struct* lcs = (LoginCrypt_struct*) eqlogin;

	if (strlen(lcs->username) >= 20 || strlen(lcs->password) >= 20)
		{
			cout<<"Invalid username/password lengths"<<endl;
			return 0;
		}
		char dbpass[33];
		int32 accid = database.GetLSLoginInfo(lcs->username, dbpass, lsadmin, lsstatus, verified, worldadmin);
		if (accid == 0)
			return 0;
		MD5 md5pass(lcs->password, strlen(lcs->password));
		MD5 DBmd5pass(dbpass, strlen(dbpass));
		if (oUsername)
			strcpy(oUsername, lcs->username);
		string str = md5pass;
		//string str2 = dbpass;
		string str2 = DBmd5pass;
		if (!str.compare(str2))
		{
			struct in_addr in;
			in.s_addr = ip;
			//Yeahlight: Check for account lockout, return 9999999 for a 15 min suspension, 9999998 for PERMINATE
		//	database.ClearAccountLockout(inet_ntoa(in));
			return accid;
		}
		else
		{
			cout << "[INVALID PASS]" << " Typed:" << md5pass << " Expecting:" << DBmd5pass << endl;
			return 0;
		}
	
}
#endif

int8 Database::CheckWorldVerAuth(char* version) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	MYSQL_ROW row;
	int32 errnum = 0;
	char tmp[100];
	int tmplen = strlen(version);
	if (tmplen >= 64)
		return 0;
	DoEscapeString(tmp, version, strlen(version));
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT approval FROM login_versions WHERE version='%s';", tmp), errbuf, &result, 0, &errnum)) {
		delete[] query;
		if (mysql_num_rows(result) >= 1) {
			row = mysql_fetch_row(result);
			int8 ret = atoi(row[0]);
			mysql_free_result(result);
			return ret;
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else if (errnum == ER_NO_SUCH_TABLE) {
		delete[] query;
		return 1;
	}
	else {
		cerr << "Error in CheckWorldVerAuth query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	return 0;
}

bool Database::AccountLoggedIn(int32 account_id){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM active_accounts WHERE lsaccount = %i", account_id), errbuf, &result)){
		delete[] query;
		if (mysql_num_rows(result) > 0){
			mysql_free_result(result);
			return true;
		}else{
			mysql_free_result(result);
			return false;
		}
	}else{
		cerr << "Error in CheckWorldAuth query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}

//Yeahlight: Count active users online for specified World instance
int32 Database::CountUsers(char* account){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM active_accounts WHERE account = '%s'", account), errbuf, &result)){
		delete[] query;
		return mysql_num_rows(result);
	}else{
		cerr << "Error in CountUsers query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return 0;
}