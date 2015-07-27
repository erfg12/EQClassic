#include "../common/database.h"

extern GuildRanks_Struct guilds[512];

int32 Database::GetGuildEQID(int32 guilddbid) {
	if (guilddbid == 0)
		return 0xFFFFFFFF;
	for (int32 i=0; i<512; i++) {
		if (guilds[i].databaseID == guilddbid)
			return i;
	}
	return 0xFFFFFFFF;
/*
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
*/
}
