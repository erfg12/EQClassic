
#include <iostream>
#include <cstdio>
#include <cstdlib>
#ifndef WIN32
	#include <pthread.h>
#endif
#include "faction.h"
#include "database.h"
#include "client.h"
#include "zone.h"
#include "config.h"
#include "FactionCommon.h"

using namespace std;

extern Zone* zone;
extern Database database;


//o--------------------------------------------------------------
//| Name: CalculateFaction; rembrant, Dec. 16, 2001
//o--------------------------------------------------------------
//| Notes: Returns the faction message value.
//|        Modify these values to taste.
//o--------------------------------------------------------------
FactionLevels CalculateFaction(FactionMods* fm, sint32 tmpCharacter_value)
{
	sint32 character_value;

	character_value = fm->base + fm->class_mod + fm->race_mod + fm->deity_mod + tmpCharacter_value;

	if(character_value >=  1101)							return FACTION_ALLY;
	if(character_value >=   701 && character_value <= 1100) return FACTION_WARMLY;
	if(character_value >=   401 && character_value <=  700) return FACTION_KINDLY;
	if(character_value >=   101 && character_value <=  400) return FACTION_AMIABLE;
	if(character_value >=     0 && character_value <=  100) return FACTION_INDIFFERENT;
	if(character_value >=  -100 && character_value <=   -1) return FACTION_APPREHENSIVE;
	if(character_value >=  -700 && character_value <= -101) return FACTION_DUBIOUS;
	if(character_value >=  -999 && character_value <= -701) return FACTION_THREATENLY;
	if(character_value <= -1000) return FACTION_SCOWLS;
	else
	{
		return FACTION_INDIFFERENT;
	}
}

//o--------------------------------------------------------------
//| Name: GetFactionLevel; rembrant, Dec. 16, 2001
//o--------------------------------------------------------------
//| Notes: Gets the characters faction standing with the
//|        specified NPC.
//|        Will return Indifferent on failure.
//o--------------------------------------------------------------
FactionLevels Client::GetFactionLevel(int32 char_id, int32 npc_id, int32 p_race, int32 p_class, int32 p_deity, int32 pFaction)
{
	sint32 pFacValue;
	sint32 tmpFactionValue;
	FactionMods fmods;
	//First get the NPC's Primary faction
	if (GetFeigned() || GetHide())
		return FACTION_INDIFFERENT;
	if(pFaction != 0)
	{
		//Get the faction data from the database
		if(database.GetFactionData(&fmods, p_class, p_race, p_deity, pFaction))
		{
			//Get the players current faction with pFaction
			tmpFactionValue = this->GetCharacterFactionLevel(pFaction);
			//Return the faction to the client
			return CalculateFaction(&fmods, tmpFactionValue);
		}
	}

	return FACTION_INDIFFERENT;
}

//o--------------------------------------------------------------
//| Name: SetFactionLevel; rembrant, Dec. 20, 2001
//o--------------------------------------------------------------
//| Notes: Sets the characters faction standing with the
//|        specified NPC.
//o--------------------------------------------------------------
void  Client::SetFactionLevel(int32 char_id, int32 npc_id, int8 char_class, int8 char_race, int8 char_deity)
{
	int32 faction_id[]={ 0,0,0,0,0,0,0,0,0,0 };
	sint32 npc_value[]={ 0,0,0,0,0,0,0,0,0,0 };
	sint32 tmpValue;
	sint32 current_value;
	FactionMods fm;
	//Get the npc faction list
	if(database.GetNPCFactionList(npc_id, faction_id, npc_value))
	{
		for(int i = 0;i<=9;i++)
		{
			if(faction_id[i] != 0)
			{
				//Get the faction modifiers
				if(database.GetFactionData(&fm,char_class,char_race,char_deity,faction_id[i]))
				{
					//Get the characters current value with that faction
					current_value = GetCharacterFactionLevel(faction_id[i]) + npc_value[i];
					//Calculate the faction
					tmpValue = current_value + fm.base + fm.class_mod + fm.race_mod + fm.deity_mod;
					//Make sure we dont go over the min/max faction limits
					if(tmpValue >= MAX_FACTION)
					{
						tmpValue = MAX_FACTION;
					}

					if(tmpValue < MAX_FACTION && tmpValue > MIN_FACTION)
					{
						if(!(database.SetCharacterFactionLevel(char_id, faction_id[i], current_value,&factionvalue_list)))
						{
							return;
						}
					}

					if(tmpValue <= MIN_FACTION) 
					{
						tmpValue = MIN_FACTION;
					}

					//ChannelMessageSend(0,0,7,0,BuildFactionMessage(npc_value[i], faction_id[i]));
					Message(0,BuildFactionMessage(npc_value[i],faction_id[i]));
				}
			}
		}
	}
	return;
}

sint32 Client::GetCharacterFactionLevel(int32 faction_id)
{
	if (faction_id == 0)
	{
		return 0;
	}

	LinkedListIterator<FactionValue*> iterator(factionvalue_list);	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->factionID == faction_id)
		{
			return iterator.GetData()->value;
		}
		iterator.Advance();
	}
	return 0;
	cout << "error in GetCharacterFactionLevel" << endl;
}

//o--------------------------------------------------------------
//| Name: BuildFactionMessage; rembrant, Dec. 16, 2001
//o--------------------------------------------------------------
//| Purpose: duh?
//o--------------------------------------------------------------
char* BuildFactionMessage(sint32 tmpvalue, int32 faction_id)
{
	char *faction_message = 0;
	int buf_len = 256;
    int chars = -1;

	char name[50];

	if(database.GetFactionName(faction_id, name) == false)
	{
		snprintf(name, buf_len,"Faction%i",faction_id);
	}

	if(tmpvalue == MAX_FACTION)
	{
		while (chars == -1 || chars >= buf_len)
		{
			if (faction_message != 0)
			{
				delete[] faction_message;
				faction_message = 0;
				buf_len *= 2;
			}
			faction_message = new char[buf_len];
			chars = snprintf(faction_message, buf_len, "Your faction standing with %s could not possibly get any better!", name);
		}
	}
	if(tmpvalue > 0 && tmpvalue < MAX_FACTION)
	{
		while (chars == -1 || chars >= buf_len)
		{
			if (faction_message != 0)
			{
				delete[] faction_message;
				faction_message = 0;
				buf_len *= 2;
			}
			faction_message = new char[buf_len];
			chars = snprintf(faction_message, buf_len, "Your faction standing with %s has gotten better!", name);
		}
	}
	if(tmpvalue < 0 && tmpvalue > MIN_FACTION)
	{
		while (chars == -1 || chars >= buf_len)
		{
			if (faction_message != 0)
			{
				delete[] faction_message;
				faction_message = 0;
				buf_len *= 2;
			}
			faction_message = new char[buf_len];
			chars = snprintf(faction_message, buf_len, "Your faction standing with %s has gotten worse!", name);
		}
	}
	if(tmpvalue == MIN_FACTION)
	{
		while (chars == -1 || chars >= buf_len)
		{
			if (faction_message != 0)
			{
				delete[] faction_message;
				faction_message = 0;
				buf_len *= 2;
			}
			faction_message = new char[buf_len];
			chars = snprintf(faction_message, buf_len, "Your faction standing with %s could not possibly get any worse!", name);
		}
	}
	return faction_message;
}

//o--------------------------------------------------------------
//| Name: GetFactionName; rembrant, Dec. 16
//o--------------------------------------------------------------
//| Notes: Retrieves the name of the specified faction
//|        Returns false on failure.
//o--------------------------------------------------------------
bool Database::GetFactionName(int32 faction_id, char* name)
{
	if ((faction_id == 0)||(faction_array[faction_id] == 0))
	{
		return false;
	}

	if (faction_array[faction_id]->name[0] != 0)
	{
		strcpy(name,faction_array[faction_id]->name);
	}
	else
	{
		return false;
	}

	return true;

}

//o--------------------------------------------------------------
//| Name: GetNPCFactionList; rembrant, Dec. 16, 2001
//o--------------------------------------------------------------
//| Purpose: Gets a list of faction_id's and values bound to
//|          the npc_id.
//|          Returns false on failure.
//o--------------------------------------------------------------
bool Database::GetNPCFactionList(int32 npc_id, int32* faction_id, sint32* value)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	int i = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT faction_id, value FROM npc_faction WHERE npc_id = %i", npc_id), errbuf, &result)) {
		delete[] query;
		while ((row = mysql_fetch_row(result)))
		{
			faction_id[i] = atoi(row[0]);
			value[i] = atoi(row[1]);
			i++;
		}
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "Error in GetNPCFactionList query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	return false;
}

//o--------------------------------------------------------------
//| Name: SetCharacterFactionLevel; rembrant, Dec. 20, 2001
//o--------------------------------------------------------------
//| Purpose: Update characters faction level with specified
//|          faction_id to specified value.
//|          Returns false on failure.
//o--------------------------------------------------------------
bool Database::SetCharacterFactionLevel(int32 char_id, int32 faction_id, sint32 value,LinkedList<FactionValue*>* val_list)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM faction_values WHERE char_id=%i AND faction_id = %i", char_id, faction_id), errbuf)) {
		cerr << "Error in SetCharacterFactionLevel query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO faction_values (char_id,faction_id,current_value) VALUES (%i,%i,%i)", char_id, faction_id,value), errbuf, 0, &affected_rows)) {
		cerr << "Error in SetCharacterFactionLevel query '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	
	delete[] query;
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	LinkedListIterator<FactionValue*> iterator(*val_list);	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->factionID == faction_id)
		{
			iterator.GetData()->value = value;
			return true;
		}
		iterator.Advance();
	}
	FactionValue* facval = new FactionValue;
	facval->factionID = faction_id;
	facval->value = value;
	val_list->Insert(facval);	
	return true;
}

bool Database::LoadFactionData()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id) FROM faction_list");
	
	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete(query);
		row = mysql_fetch_row(result);
		if (row != 0 && row[0] > 0)
		{ 
			max_faction = atoi(row[0]);
			faction_array = new Faction*[max_faction+1];
			for(int i=0; i<max_faction; i++)
			{
				faction_array[i] = 0;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT id,name,base,mod_c1,mod_c2,mod_c3,mod_c4,mod_c5,mod_c6,mod_c7,mod_c8,mod_c9,mod_c10,mod_c11,mod_c12,mod_c13,mod_c14,mod_c15,mod_r1,mod_r2,mod_r3,mod_r4,mod_r5,mod_r6,mod_r7,mod_r8,mod_r9,mod_r10,mod_r11,mod_r12,mod_r14,mod_r60,mod_r75,mod_r108,mod_r120,mod_r128,mod_r130,mod_r161,mod_d140,mod_d201,mod_d202,mod_d203,mod_d204,mod_d205,mod_d206,mod_d207,mod_d208,mod_d209,mod_d210,mod_d211,mod_d212,mod_d213,mod_d214,mod_d215,mod_d216 FROM faction_list");

			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				delete[] query;
				while(row = mysql_fetch_row(result))
				{
					faction_array[atoi(row[0])] = new Faction;
					memset(faction_array[atoi(row[0])], 0, sizeof(Faction));
					strncpy(faction_array[atoi(row[0])]->name, row[1], 50);					
					faction_array[atoi(row[0])]->base = atoi(row[2]);
					int16 i;
					for (i=3;i != 18;i++)
						faction_array[atoi(row[0])]->mod_c[i-3] = atoi(row[i]);
					for (i=18;i != 38;i++)
						faction_array[atoi(row[0])]->mod_r[i-18] = atoi(row[i]);
					for (i=38;i != 55;i++)
						faction_array[atoi(row[0])]->mod_d[i-38] = atoi(row[i]);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadFactionData '" << query << "' " << errbuf << endl;
				delete[] query;
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in LoadFactionData '" << query << "' " << errbuf << endl;
		delete[] query;
		return false;
	}
	return true;
}
