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
//#include "debug.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
	#include <pthread.h>
#endif
#include "../include/faction.h"
#include "../include/database.h"
#include "../include/client.h"
#include "../include/zone.h"

extern Zone* zone;

#define MAX_FACTION_ID 483

#ifdef WIN32
	#define snprintf	_snprintf
#endif

//o--------------------------------------------------------------
//| Name: CalculateFaction; rembrant, Dec. 16, 2001
//o--------------------------------------------------------------
//| Notes: Returns the faction message value.     
//o--------------------------------------------------------------

FACTION_VALUE CalculateFaction(FactionMods* fm, sint32 tmpCharacter_value)
{
	sint32 character_value = tmpCharacter_value;
	if(fm)
		character_value += fm->base + fm->class_mod + fm->race_mod + fm->deity_mod;
	//Yeahlight: I am 99% certain these values are all correct, please do not change.
	if(character_value >=  1100)							return FACTION_ALLY;
	if(character_value >=   750 && character_value <= 1099) return FACTION_WARMLY;
	if(character_value >=   500 && character_value <=  749) return FACTION_KINDLY;
	if(character_value >=   101 && character_value <=  499) return FACTION_AMIABLE;
	if(character_value >=     0 && character_value <=   99) return FACTION_INDIFFERENT;
	if(character_value >=  -100 && character_value <=   -1) return FACTION_APPREHENSIVE;
	if(character_value >=  -500 && character_value <= -101) return FACTION_DUBIOUS;
	if(character_value >=  -750 && character_value <= -501) return FACTION_THREATENLY;
	if(character_value <=  -751)							return FACTION_SCOWLS;
	return FACTION_INDIFFERENT;

	//Yeahlight: Old values
	//if(character_value >=  1101) return FACTION_ALLY;
	//if(character_value >=   701 && character_value <= 1100) return FACTION_WARMLY;
	//if(character_value >=   401 && character_value <=  700) return FACTION_KINDLY;
	//if(character_value >=   101 && character_value <=  400) return FACTION_AMIABLE;
	//if(character_value >=     0 && character_value <=  100) return FACTION_INDIFFERENT;
	//if(character_value >=  -100 && character_value <=   -1) return FACTION_APPREHENSIVE;
	//if(character_value >=  -700 && character_value <= -101) return FACTION_DUBIOUS;
	//if(character_value >=  -999 && character_value <= -701) return FACTION_THREATENLY;
	//if(character_value <= -1000) return FACTION_SCOWLS;
}

// neotokyo: this function should check if some races have more than one race define
bool IsOfEqualRace(int r1, int r2)
{
    if (r1 == r2)
        return true;
    // TODO: add more values
    switch(r1)
    {
    case DARK_ELF:
        if (r2 == 77)
            return true;
        break;
    case BARBARIAN:
        if (r2 == 90)
            return true;
    }
    return false;
}

// neotokyo: trolls endure ogres, dark elves, ...
bool IsOfIndiffRace(int r1, int r2)
{
    if (r1 == r2)
        return true;
    // TODO: add more values
    switch(r1)
    {
    case DARK_ELF:
    case OGRE:
    case TROLL:
        if (r2 == OGRE || r2 == TROLL || r2 == DARK_ELF)
            return true;
        break;
    case HUMAN:
    case BARBARIAN:
    case HALF_ELF:
    case GNOME:
    case HALFLING:
    case WOOD_ELF:
        if (r2 == HUMAN ||
            r2 == BARBARIAN ||
            r2 == ERUDITE ||
            r2 == HALF_ELF ||
            r2 == GNOME ||
            r2 == HALFLING ||
            r2 == DWARF ||
            r2 == HIGH_ELF ||
            r2 == WOOD_ELF)
            return true;
        break;
    case ERUDITE:
        if (r2 == HUMAN || r2 == HALF_ELF)
            return true;
        break;
    case DWARF:
        if (r2 == HALFLING || r2 == GNOME)
            return true;
        break;
    case HIGH_ELF:
        if (r2 == WOOD_ELF)
            return true;
        break;
    case VAHSHIR:
        return true;
    case IKSAR:
        return false;
    }
    return false;
}

//o--------------------------------------------------------------
//| Name: GetFactionLevel; rembrant, Dec. 16, 2001
//o--------------------------------------------------------------
//| Notes: Gets the characters faction standing with the
//|        specified NPC.
//|        Will return Indifferent on failure.
//o--------------------------------------------------------------
FACTION_VALUE Client::GetFactionLevel(int32 char_id, int32 npc_id, int32 p_race, int32 p_class, int32 p_deity, sint32 pFaction, Mob* tnpc)
{
	//Yeahlight: No mob was supplied; abort
	if(tnpc == NULL)
		return FACTION_INDIFFERENT;

	//Yeahlight: PC is FD'ed
	if(this->GetFeigned())
		return FACTION_INDIFFERENT;

	//Yeahlight: Mob is a pet of the PC
	if(tnpc->GetOwner() == this)
		return FACTION_AMIABLE;

	//Yeahlight: Mob is another PC
	if(tnpc->IsClient())
		return FACTION_INDIFFERENT;

	//Yeahlight: The mob cannot see this PC
    if(VisibleToMob(tnpc) == false)
		return FACTION_INDIFFERENT;

	//Yeahlight: The mob is a pet
    if(tnpc->GetOwnerID() != 0)
        return FACTION_INDIFFERENT;

	//Yeahlight: This mob is not on a primary faction
	if(pFaction == 0)
		return GetSpecialFactionCon(tnpc);

	FACTION_VALUE fac = FACTION_INDIFFERENT;
	sint32 tmpFactionValue;
	FactionMods fmods;

    //First get the NPC's Primary faction
	if(pFaction > 0)
	{
		//Get the faction data from the database
		if(Database::Instance()->GetFactionData(&fmods, p_class, p_race, p_deity, pFaction))
		{
			//Get the players current faction with pFaction
			tmpFactionValue = this->GetCharacterFactionLevel(pFaction);
			//Return the faction to the client
			fac = CalculateFaction(&fmods, tmpFactionValue);
		}
	}
	else
    {
        fmods.base = 0;
        fmods.deity_mod = 0;

        if (tnpc && p_class == (int32) tnpc->GetClass()%16)
            fmods.class_mod = 301;
        else if (tnpc && tnpc->IsNPC() && tnpc->CastToNPC()->MerchantType == 0)
            fmods.class_mod = -101;
        else
            fmods.class_mod = 0;

        if (tnpc && IsOfEqualRace(p_race, tnpc->GetRace()) )
            fmods.race_mod = 101;
        else if (tnpc && IsOfIndiffRace(p_race, tnpc->GetRace()) )
            fmods.race_mod = 0;
        else if (tnpc)
            fmods.race_mod = -51;
        else
            fmods.race_mod = 0;
        fac = CalculateFaction(&fmods, 0);
    }

    // merchant fix
    if (tnpc && tnpc->IsNPC() && tnpc->CastToNPC()->MerchantType && (fac == FACTION_THREATENLY || fac == FACTION_SCOWLS))
        fac = FACTION_DUBIOUS;

	if (tnpc != 0 && fac != FACTION_SCOWLS && tnpc->CastToNPC()->CheckAggro(this))
		fac = FACTION_THREATENLY;

	return fac;
}

//o--------------------------------------------------------------
//| Name: SetFactionLevel; rembrant, Dec. 20, 2001
//o--------------------------------------------------------------
//| Notes: Sets the characters faction standing with the
//|        specified NPC.
//o--------------------------------------------------------------
void Client::SetFactionLevel(int32 char_id, int32 npc_id, int8 char_class, int8 char_race, int8 char_deity)
{
	sint32 faction_id[MAX_NPC_FACTIONS]={ 0,0,0,0,0,0,0,0,0,0 };
	sint32 npc_value[MAX_NPC_FACTIONS]={ 0,0,0,0,0,0,0,0,0,0 };
	sint32 tmpValue;
	sint32 current_value;
	FactionMods fm;
	//Get the npc faction list
	if(Database::Instance()->GetNPCFactionList(npc_id, faction_id, npc_value))
	{
		for(int i = 0;i<MAX_NPC_FACTIONS;i++)
		{
			if(faction_id[i] > 0)
			{
				//Get the faction modifiers
				if(Database::Instance()->GetFactionData(&fm,char_class,char_race,char_deity,faction_id[i]))
				{
					//Get the characters current value with that faction
					current_value = GetCharacterFactionLevel(faction_id[i]) + npc_value[i];
					//Calculate the faction
					tmpValue = current_value + fm.base + fm.class_mod + fm.race_mod + fm.deity_mod;
					//Make sure faction hits don't go to GMs...
					//if(pp.gm==1 && (tmpValue < current_value)) {
					//	tmpValue=current_value;
					//}
					//Make sure we dont go over the min/max faction limits
					if(tmpValue >= MAX_FACTION)
					{
						if(!(Database::Instance()->SetCharacterFactionLevel(char_id, faction_id[i], MAX_FACTION,&factionvalue_list)))
						{
							return;
						}
					}
					else if(tmpValue <= MIN_FACTION)
					{
						if(!(Database::Instance()->SetCharacterFactionLevel(char_id, faction_id[i], MIN_FACTION,&factionvalue_list)))
						{
							return;
						}
					}
					else
					{
						if(!(Database::Instance()->SetCharacterFactionLevel(char_id, faction_id[i], current_value,&factionvalue_list)))
						{
							return;
						}
					}
					if(tmpValue <= MIN_FACTION) tmpValue = MIN_FACTION;
					//ChannelMessageSend(0,0,7,0,BuildFactionMessage(npc_value[i], faction_id[i]));
					char* msg = BuildFactionMessage(npc_value[i],faction_id[i],tmpValue);
					if (msg != 0)
						Message(BLACK, msg);
					safe_delete(msg);
				}
			}
		}
	}
	return;
}

void Client::SetFactionLevel2(int32 char_id, sint32 faction_id, int8 char_class, int8 char_race, int8 char_deity, sint32 value)
{
//	sint32 tmpValue;
	sint32 current_value;
//	FactionMods fm;
	//Get the npc faction list
	if(faction_id > 0) {
		//Get the faction modifiers
		current_value = GetCharacterFactionLevel(faction_id) + value;
		if(!(Database::Instance()->SetCharacterFactionLevel(char_id, faction_id, current_value,&factionvalue_list)))
			return;

		char* msg = BuildFactionMessage(value, faction_id, current_value);
		if (msg != 0)
			Message(BLACK, msg);
		safe_delete(msg);
	}
	return;
}

sint32 Client::GetCharacterFactionLevel(sint32 faction_id)
{
	if (faction_id <= 0)
		return 0;
	LinkedListIterator<FactionValue*> iterator(factionvalue_list);	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if ((int32)(iterator.GetData()->factionID) == faction_id)
		{
			return (iterator.GetData()->value + iterator.GetData()->modifier);
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return 0;
}

//o-----------------------------------------------------------------
//| Name: SetCharacterFactionLevelModifier: Yeahlight, Jan 15, 2009
//o-----------------------------------------------------------------
//| Updates the client's faction modifer for the supplied ID
//o-----------------------------------------------------------------
void Client::SetCharacterFactionLevelModifier(sint32 faction_id, sint16 amount)
{
	if (faction_id <= 0)
		return;
	LinkedListIterator<FactionValue*> iterator(factionvalue_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if ((int32)(iterator.GetData()->factionID) == faction_id)
		{
			iterator.GetData()->modifier = amount;
			return;
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return;
}

bool Database::GetFactionData(FactionMods* fm, sint32 class_mod, sint32 race_mod, sint32 deity_mod, sint32 faction_id) {
	if (faction_id <= 0 || faction_id > max_faction)
		return false;	
	int modr_tmp =0;
	sint32 modd_tmp =0;
	switch (race_mod) {
		case 1: modr_tmp = 0;break;
		case 2: modr_tmp = 1;break;
		case 3: modr_tmp = 2;break;
		case 4: modr_tmp = 3;break;
		case 5: modr_tmp = 4;break;
		case 6: modr_tmp = 5;break;
		case 7: modr_tmp = 6;break;
		case 8: modr_tmp = 7;break;
		case 9: modr_tmp = 8;break;
		case 10: modr_tmp = 9;break;
		case 11: modr_tmp = 10;break;
		case 12: modr_tmp = 11;break;
		case 14: modr_tmp = 12;break;
		case 60: modr_tmp = 13;break;
		case 75: modr_tmp = 14;break;
		case 108: modr_tmp = 15;break;
		case 120: modr_tmp = 16;break;
		case 128: modr_tmp = 17;break;
		case 130: modr_tmp = 18;break;
		case 161: modr_tmp = 19;break;
	}
	if (deity_mod == 140 ) 
		modd_tmp = 0;
	else
		modd_tmp = deity_mod - 200;
	if (faction_array[faction_id] == 0){
		return false;
	}
	fm->base = faction_array[faction_id]->base;
	if ((class_mod-1) < (sizeof(faction_array[faction_id]->mod_c) / sizeof(faction_array[faction_id]->mod_c[0])))
		fm->class_mod = faction_array[faction_id]->mod_c[class_mod-1];
	else {
//		LogFile->write(EQEMuLog::Error, "Error in Database::GetFactionData: class_mod-1[=%i] out of range", class_mod-1);
		fm->class_mod = 0;
		//return false;
	}
	if ((modr_tmp) < (sizeof(faction_array[faction_id]->mod_r) / sizeof(faction_array[faction_id]->mod_r[0])))
		fm->race_mod = faction_array[faction_id]->mod_r[modr_tmp];
	else {
//		LogFile->write(EQEMuLog::Error, "Error in Database::GetFactionData: modr_tmp[=%i] out of range (race_mod=%i)", modr_tmp, race_mod);
		fm->race_mod = 0;
		//return false;
	}
	if ((modd_tmp) < (sizeof(faction_array[faction_id]->mod_d) / sizeof(faction_array[faction_id]->mod_d[0])))
		fm->deity_mod = faction_array[faction_id]->mod_d[modd_tmp];
	else {
//		LogFile->write(EQEMuLog::Error, "Error in Database::GetFactionData: modd_tmp[=%i] out of range (deity_mod=%i)", modd_tmp, deity_mod);
		fm->deity_mod = 0;
		//return false;
	}
	if(fm->deity_mod > 1000)
		fm->deity_mod = 0;

	return true;
}


bool Database::LoadFactionValues(int32 char_id, LinkedList<FactionValue*>* val_list) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT faction_id,current_value FROM faction_values WHERE char_id = %i", char_id), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
		bool ret = LoadFactionValues_result(result, val_list);
		mysql_free_result(result);
		return ret;
	}
	else {
		cerr << "Error in LoadFactionValues query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
	}
	return false;
}

bool Database::LoadFactionValues_result(MYSQL_RES* result, LinkedList<FactionValue*>* val_list) {
    MYSQL_ROW row;
	int16 count = 0;
	while((row = mysql_fetch_row(result))) {
		FactionValue* facval = new FactionValue;
		facval->factionID = atoi(row[0]);
		facval->value = atoi(row[1]);
		facval->modifier = 0;
		val_list->Insert(facval);
		count++;
	}
	if(count == 0)
		return false;
	return true;
}

//o--------------------------------------------------------------
//| Name: BuildFactionMessage; rembrant, Dec. 16, 2001
//o--------------------------------------------------------------
//| Purpose: duh?
//o--------------------------------------------------------------
char* BuildFactionMessage(sint32 tmpvalue, sint32 faction_id, sint32 totalvalue)
{
	char *faction_message = 0;

	char name[50];

	if(Database::Instance()->GetFactionName(faction_id, name, sizeof(name)) == false) {
		snprintf(name, sizeof(name),"Faction%i",faction_id);
	}

	if(totalvalue >= MAX_FACTION) {
		MakeAnyLenString(&faction_message, "Your faction standing with %s could not possibly get any better!", name);
		return faction_message;
	}
	else if(tmpvalue > 0 && totalvalue < MAX_FACTION) {
		MakeAnyLenString(&faction_message, "Your faction standing with %s has gotten better!", name);
		return faction_message;
	}
	else if(tmpvalue == 0) {
		return 0;
	}
	else if(tmpvalue < 0 && totalvalue > MIN_FACTION) {
		MakeAnyLenString(&faction_message, "Your faction standing with %s has gotten worse!", name);
		return faction_message;
	}
	else if(totalvalue <= MIN_FACTION) {
		MakeAnyLenString(&faction_message, "Your faction standing with %s could not possibly get any worse!", name);
		return faction_message;
	}
	return 0;
}

//o--------------------------------------------------------------
//| Name: GetFactionName; rembrant, Dec. 16
//o--------------------------------------------------------------
//| Notes: Retrieves the name of the specified faction
//|        Returns false on failure.
//o--------------------------------------------------------------
bool Database::GetFactionName(sint32 faction_id, char* name, int32 buflen) {
	if ((faction_id <= 0)||(faction_array[faction_id] == 0))
		return false;
	if (faction_array[faction_id]->name[0] != 0) {
		strncpy(name, faction_array[faction_id]->name, buflen - 1);
		name[buflen - 1] = 0;
		return true;
	}
	return false;

}

//o--------------------------------------------------------------
//| Name: GetNPCFactionList; rembrant, Dec. 16, 2001
//o--------------------------------------------------------------
//| Purpose: Gets a list of faction_id's and values bound to
//|          the npc_id.
//|          Returns false on failure.
//o--------------------------------------------------------------
bool Database::GetNPCFactionList(int32 npc_id, sint32* faction_id, sint32* value, sint32* primary_faction) {
	int32 npcfaction_id = Database::Instance()->GetNPCFactionID(npc_id);
	if (npcfaction_id <= 0) {
		if (primary_faction)
			*primary_faction = npcfaction_id;
		return true;
	}
	const NPCFactionList* nfl = Database::Instance()->GetNPCFactionList(npcfaction_id);
	if (!nfl)
		return false;
	if (primary_faction)
		*primary_faction = nfl->primaryfaction;
	for (int i=0; i<MAX_NPC_FACTIONS; i++) {
		faction_id[i] = nfl->factionid[i];
		value[i] = nfl->factionvalue[i];
	}
	return true;
}

//o--------------------------------------------------------------
//| Name: GetNPCFactionID; Yeahlight, Aug 4, 2008
//o--------------------------------------------------------------
//| Purpose: Returns the faction ID from an NPC ID
//o--------------------------------------------------------------
int32 Database::GetNPCFactionID(int32 npc_id){
	char errbuf2[MYSQL_ERRMSG_SIZE];
    char *query2 = 0;
	char tempName[100] = "";
    MYSQL_RES *result2;
    MYSQL_ROW row2;
	if (RunQuery(query2, MakeAnyLenString(&query2, "SELECT name FROM npc_types_without where id = %i", npc_id), errbuf2, &result2)) {
		safe_delete_array(query2);//delete[] query2;
		row2 = mysql_fetch_row(result2);
		if(row2)
			strcpy(tempName, row2[0]);
	}

	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM npc_faction where name = '%s'", tempName), errbuf, &result)) {
		safe_delete_array(query);//delete[] query;
		row = mysql_fetch_row(result);
		if(row)
			return atoi(row[0]);
	}
	//Yeahlight: TODO: This may crash the zone, need to handle it better on the receiving end
	return 0;
}

//o--------------------------------------------------------------
//| Name: GetPrimaryFaction; Yeahlight, Aug 4, 2008
//o--------------------------------------------------------------
//| Purpose: Returns the faction ID from an NPC ID
//o--------------------------------------------------------------
int32 Database::GetPrimaryFaction(char* name){
	char errbuf2[MYSQL_ERRMSG_SIZE];
    char *query2 = 0;
    MYSQL_RES *result2;
    MYSQL_ROW row2;
	if (RunQuery(query2, MakeAnyLenString(&query2, "SELECT primaryfaction FROM npc_faction where name = '%s'", name), errbuf2, &result2)) {
		safe_delete_array(query2);//delete[] query2;
		row2 = mysql_fetch_row(result2);
		if(row2)
			return atoi(row2[0]);
	}
	return 0;
}

//o--------------------------------------------------------------
//| Name: SetCharacterFactionLevel; rembrant, Dec. 20, 2001
//o--------------------------------------------------------------
//| Purpose: Update characters faction level with specified
//|          faction_id to specified value.
//|          Returns false on failure.
//o--------------------------------------------------------------
bool Database::SetCharacterFactionLevel(int32 char_id, sint32 faction_id, sint32 value,LinkedList<FactionValue*>* val_list)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM faction_values WHERE char_id=%i AND faction_id = %i", char_id, faction_id), errbuf)) {
		cerr << "Error in SetCharacterFactionLevel query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO faction_values (char_id,faction_id,current_value) VALUES (%i,%i,%i)", char_id, faction_id,value), errbuf, 0, &affected_rows)) {
		cerr << "Error in SetCharacterFactionLevel query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	
	safe_delete_array(query);//delete[] query;
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	LinkedListIterator<FactionValue*> iterator(*val_list);	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if ((int32)(iterator.GetData()->factionID) == faction_id)
		{
			iterator.GetData()->value = value;
			return true;
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
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
		if (row && row[0])
		{ 
			max_faction = atoi(row[0]);
			faction_array = new Faction*[max_faction+1];
			for(unsigned int i=0; i<max_faction; i++)
			{
				faction_array[i] = 0;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT id,name,base,mod_c1,mod_c2,mod_c3,mod_c4,mod_c5,mod_c6,mod_c7,mod_c8,mod_c9,mod_c10,mod_c11,mod_c12,mod_c13,mod_c14,mod_c15,mod_r1,mod_r2,mod_r3,mod_r4,mod_r5,mod_r6,mod_r7,mod_r8,mod_r9,mod_r10,mod_r11,mod_r12,mod_r14,mod_r60,mod_r75,mod_r108,mod_r120,mod_r128,mod_r130,mod_r161,mod_d140,mod_d201,mod_d202,mod_d203,mod_d204,mod_d205,mod_d206,mod_d207,mod_d208,mod_d209,mod_d210,mod_d211,mod_d212,mod_d213,mod_d214,mod_d215,mod_d216 FROM faction_list");

			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				safe_delete_array(query);//delete[] query;
				while((row = mysql_fetch_row(result)))
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
				safe_delete_array(query);//delete[] query;
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in LoadFactionData '" << query << "' " << errbuf << endl;
		safe_delete_array(query);//delete[] query;
		return false;
	}
	return true;
}