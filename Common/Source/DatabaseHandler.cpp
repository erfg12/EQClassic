// ***************************************************************
//  Petition Manager   ·  date: 16/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// Handles interaction directly with MySQL
// ***************************************************************

#include "DatabaseHandler.h"

namespace EQC
{
	namespace Common
	{

		DatabaseHandler::DatabaseHandler()
		{
			// init mysql
			mysql_init(&mysql);		
		}

		DatabaseHandler::~DatabaseHandler()
		{
			// close off mysql
			mysql_close(&mysql);
		}

		void DatabaseHandler::DBConnect(const char* host, const char* user, const char* passwd, const char* database)
		{
			//	Quagmire - added CLIENT_FOUND_ROWS flag to the connect
			//	otherwise DB update calls would say 0 rows affected when the value already equalled
			//	what the function was tring to set it to, therefore the function would think it failed 

			if (mysql_real_connect(&mysql, host, user, passwd, database, 0, 0, CLIENT_FOUND_ROWS))
			{
				this->IsConnected = true; // Set it so we know we're connected
				EQC::Common::PrintF(CP_DATABASE, "Using database '%s' at '%s'", database, host);
			}
			else
			{
				this->IsConnected = false; // Set it so we know we're NOT connected
				cerr << "Failed to connect to database: Error: " << mysql_error(&mysql) << endl;
			}
		}

		// Sends the MySQL server a keepalive

		void DatabaseHandler::PingMySQL() 
		{
			LockMutex lock(&MDatabase);
			mysql_ping(&mysql);
		}


		bool DatabaseHandler::RunQuery(char* query, int32 querylen, char* errbuf, MYSQL_RES** result, int32* affected_rows,int32* last_insert_id, bool retry) {
			bool ret = false;
			LockMutex lock(&MDatabase);
			if (mysql_real_query(&mysql, query, querylen)) {
				if (retry && mysql_errno(&mysql) == CR_SERVER_LOST) {
					cout << "Database Error: Lost connection, attempting to recover...." << endl;
					ret = RunQuery(query, querylen, errbuf, result, affected_rows,last_insert_id, false);
				}
				else {
					EQC::Common::PrintF(CP_DATABASE, "Error #%i: %s\n",mysql_errno(&mysql), mysql_error(&mysql));
					if (errbuf) {
						snprintf(errbuf, MYSQL_ERRMSG_SIZE, "#%i: %s", mysql_errno(&mysql), mysql_error(&mysql));
					}
					ret = false;
				}
			}
			else {
				if (errbuf)
					errbuf[0] = 0;
				if (affected_rows) {
					*affected_rows = mysql_affected_rows(&mysql);
				}
				if (last_insert_id)
					*last_insert_id = mysql_insert_id(&mysql);
				if (result) {
					//Yeahlight: World crashed here when result's data could not be evaluated
					*result = mysql_store_result(&mysql);
					if (*result) {
						ret = true;
					}
					else {
						EQC::Common::PrintF(CP_DATABASE, "DB Query Error: No Result\n");
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




		int32 DatabaseHandler::DoEscapeString(char* tobuf, char* frombuf, int32 fromlen) 
		{
			LockMutex lock(&MDatabase);
			return mysql_real_escape_string(&mysql, tobuf, frombuf, fromlen);
		}

		const char* DatabaseHandler::ReturnEscapedString(char* statement)
		{

			string NewStatement;

			LockMutex lock(&MDatabase);
			mysql_real_escape_string(&mysql, (char*)NewStatement.c_str(), statement, strlen(statement));

			return (const char*)NewStatement.c_str();

		}
	}
}