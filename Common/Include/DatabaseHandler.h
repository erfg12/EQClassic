// ***************************************************************
// EverQuest Classic (EQC) - Common
// Database Handler   ·  date: 21/12/2000
// -------------------------------------------------------------
// Copyright (C) 2007-2008 - All Rights Reserved
// ***************************************************************
// Description: MySQL Handler & Connection Class
// ***************************************************************

#ifndef DATABASEHANDLER_H
#define DATABASEHANDLER_H

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errmsg.h>
#include "config.h"
#include "EQCUtils.hpp"
#include "mysql.h"
#include "Mutex.h"

namespace EQC
{
	namespace Common
	{
		class DatabaseHandler
		{
		public:
			DatabaseHandler();
			~DatabaseHandler();

			void	PingMySQL();
			bool	RunQuery(char* query, int32 querylen, char* errbuf = 0, MYSQL_RES** result = 0, int32* affected_rows = 0,int32* last_insert_id=0, bool retry = true);
		private:
			bool	IsConnected;
		protected:
			MYSQL	mysql;
			Mutex	MDatabase;
			void	DBConnect(const char* host, const char* user, const char* passwd, const char* database);
			int32	DoEscapeString(char* tobuf, char* frombuf, int32 fromlen);
			const char* ReturnEscapedString(char* statement);

		};
	}
}
#endif