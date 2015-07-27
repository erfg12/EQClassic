#include <mysql.h>
#include <stdio.h>
#include <string.h>

#include "../common/eq_packet_structs.h"

int main(int argc, char** argv)
{
	MYSQL mysql;
	FILE* fp;
	char db_pass[32];

	if (argc != 5)
	{
		printf("Usage: ./import_raw_items db_address db_username db_name item_filename\n");
		return 1;
	}

	printf("Password:");
	scanf("%s", db_pass);

	mysql_init(&mysql);
	if (!mysql_real_connect(&mysql, argv[1], argv[2], db_pass, argv[3], 0, 0, 0))
	{
		printf("Could not connect to database: Error:%s\n", mysql_error(&mysql));
		return 1;
	}

	fp = fopen(argv[4], "r");
	if (!fp)
	{
		printf("Could not open item file '%s'\n", argv[4]);
		mysql_close(&mysql);
		return 1;
	}

	Item_Struct item;
	Item_Struct item2;
	char query[1024];
	char* end;
	int16 id;
memset(&item, 0, sizeof(item));
memset(&item2, 0, sizeof(item));
//	while(fread(&item, sizeof(item), 1, fp))
	while(fread(&item, 240, 1, fp))
	{
		id = item.item_nr;

		memcpy((char*)&item2, (char*)&item, 144);
		memcpy((char*)&item2.unknown0144[8], (char*)item.unknown0144,  sizeof(item)-152);
//		memcpy((char*)((char*)&item)[148], (char*)((char*)&item)[144], sizeof(item)-148);
/*
		((char*)&item)[144]=0;
		((char*)&item)[145]=0;
		((char*)&item)[146]=0;
		((char*)&item)[147]=0;
*/
		end = query;
		end += sprintf(end, "INSERT INTO items SET id=%i,raw_data=", id);
		*end++ = '\'';
		end += mysql_real_escape_string(&mysql, end, (char*)&item2, sizeof(Item_Struct));
		*end++ = '\'';
		if (mysql_real_query(&mysql, query, (unsigned int) (end-query)))
		{
			printf("Could not insert item %i MySQL said: %s\n", id, mysql_error(&mysql));
		}
	}
}

