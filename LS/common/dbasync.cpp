#include <iostream.h>
#include "dbasync.h"
#include "database.h"

extern Database database;
DBAsync*				dbasync = new DBAsync(&database);
AutoDelete<DBAsync>					dba_ad(&dbasync);
DBAsyncFinishedQueue*	MTdbafq = new DBAsyncFinishedQueue(dbasync);
AutoDelete<DBAsyncFinishedQueue>	dba_mtafq(&MTdbafq);

bool DBAsyncCB_LoadVariables(DBAsyncWork* iWork) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	MYSQL_RES* result = 0;
	DBAsyncQuery* dbaq = iWork->PopAnswer();
	if (dbaq->GetAnswer(errbuf, &result))
		database.LoadVariables_result(result);
	else
		cout << "Error: DBAsyncCB_LoadVariables failed: !GetAnswer: '" << errbuf << "'" << endl;
	return true;
}

void AsyncLoadVariables() {
	char* query = 0;
	DBAsyncWork* dbaw = new DBAsyncWork(&DBAsyncCB_LoadVariables, 0, DBAsync::Read);
	dbaw->AddQuery(0, &query, database.LoadVariables_MQ(&query));
	dbasync->AddWork(&dbaw);
}


