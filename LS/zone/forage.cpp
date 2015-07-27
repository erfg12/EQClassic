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
#include <iostream.h>
#include <stdlib.h>

#ifdef WIN32
#define snprintf	_snprintf
#endif

#include "forage.h"
#include "entity.h"
#include "npc.h"

#include "../common/database.h"
#ifdef WIN32
#define snprintf	_snprintf
#endif

extern Database database;

/* This allows EqEmu to have zone specific foraging - BoB */
int32 Database::GetZoneForage(int32 ZoneID, int8 skill) {

	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	int8 index = 0;
	int32 item[5];
	int32 ret = 0;

	for(int c=0;c < 4;c++) 	{
		item[c]=0;
	}
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT zoneid, itemid, level FROM forage WHERE zoneid= '%i' and level < '%i'",ZoneID, skill ), errbuf, &result))
	{
		delete[] query;
		while ((row = mysql_fetch_row(result))&&(index<4)) 	{
			item[index] = atoi(row[1]);
			index++;
		}

		mysql_free_result(result);
	} 	else 	{
		cerr << "Error in Forage query '" << query << "' " << errbuf << endl;
		delete[] query;
		return 0;
	}
	
	if(index>0) {
		ret = item[rand()%index];

	} else {
		ret = 0;
	}

	return ret;
}

uint32 ForageItem(int32 CurrentZone, int8 skill_level) {

	uint32 common_food_ids[MAX_COMMON_FOOD_IDS];

	int32 itRet;

	uint32 foragedfood = 0;
	int8   index = 0;

	common_food_ids[0] = 13046; /* Fruit */
	common_food_ids[1] = 13045; /* Berries */
	common_food_ids[2] = 13419; /* Vegetables */
	common_food_ids[3] = 13048; /* Rabbit Meat */
	common_food_ids[4] = 13047; /* Roots */
	common_food_ids[5] = 13044; /* Pod Of Water */
	common_food_ids[6] = 13106; /* Fishing Grubs */

	itRet = database.GetZoneForage(CurrentZone, skill_level);

	/* these may need to be fine tuned, I am just guessing here */

	if(rand()%240<skill_level) 	{
        if(rand()%100<75||itRet==0) {
			index = rand()%6;
			foragedfood = common_food_ids[index];
		} else {
			foragedfood = itRet;
		}
	} else {
		foragedfood = 0;                    /* doh! */
	}

	return foragedfood;
}
