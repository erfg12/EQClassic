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
#ifndef PETITIONS_H
#define PETITIONS_H

#include "../common/linked_list.h"
#include "../common/types.h"
#include "../common/database.h"
#include "client.h"
#include "../common/Mutex.h"

class Petition
{
public:
	Petition();
	~Petition() {}
	void SendPetitionToPlayer(Client* clientto);


	int32 GetID()			{ return this->petid; }
	int32 GetUnavails()		{ return unavailables; }
	int32 GetCheckouts()	{ return checkouts; }
	int32 GetCharLevel()	{ return charlevel; }
	int32 GetCharRace()		{ return charrace; }
	int32 GetCharClass()	{ return charclass; }
	int32 GetUrgency()		{ return urgency; }
	//char*  GetZone()		{ return this->zone; }
	int32	GetZone()		{ return this->zone; }
	char*  GetCharName()	{ return charname; }
	char*  GetAccountName()	{ return accountname; }
	char*  GetLastGM()		{ return lastgm; }
	time_t GetSentTime()	{ return senttime; }
	char*  GetPetitionText() { return petitiontext; }
	bool CheckedOut()				{ return ischeckedout; }


// Set Petition Stuff Functions
	void SetCheckedOut(bool status_in)	{ ischeckedout = status_in; }
	void SetUrgency(int32 urgency_in)	{ urgency = urgency_in; }
	void SetClass(int32 class_in)	{ charclass = class_in; }
	void SetRace(int32 race_in)		{ charrace = race_in; }
	void SetLevel(int32 level_in)	{ charlevel = level_in; }
	void AddCheckout()				{ checkouts = checkouts + 1; }
	void AddUnavails()				{ unavailables = unavailables + 1; }
	//void SetZone(char* zone_in)		{ strcpy(this->zone, zone_in); }
	void SetZone(int32 zone_in)			{ this->zone = zone_in; }
	void SetCName(const char* name_in)		{ strcpy(charname, name_in); }
	void SetAName(const char* name_in)		{ strcpy(accountname, name_in); }
	void SetLastGM(const char* gm_in)		{ strcpy(lastgm, gm_in); }
	void SetPetitionText(char* pet_in) { strn0cpy(petitiontext, pet_in, sizeof(petitiontext)); }
	void SetPetID(int32 id_in)		{ petid = id_in; }
	void SetCheckouts(int32 checks_in) { checkouts = checks_in; }
	void SetUnavails(int32 unavails_in) { unavailables = unavails_in; }
	void SetSentTime() { time(&senttime); }
	void SetSentTime2(time_t senttime_in) { senttime = senttime_in; }
	
protected:

	int32 petid;
	char  charname[64];
	char  accountname[32];
	char  lastgm[64];
	char  petitiontext[256];
	//char  zone[32];
	int32 zone;
	int32 urgency; // 0 = green, 1 = yellow, 2 = red
	int32 charclass;
	int32 charrace;
	int32 charlevel;
	int32 checkouts;
	int32 unavailables;
	time_t senttime;
	bool ischeckedout;
};

class PetitionList
{
public:
	int DeletePetition(int32 petnumber);
	void UpdateGMQueue();
	PetitionList() { last_insert_id = 0; }
	~PetitionList() {}
	void AddPetition(Petition* pet);
	Petition* GetPetitionByID(int32 id_in);
	int32	GetMaxPetitionID() { return last_insert_id; }
	int32	GetNextPetitionID() { return last_insert_id++; }
	void ClearPetitions();
	void ReadDatabase();
	void UpdatePetition(Petition* pet);
	void UpdateZoneListQueue();

private:
	LinkedList<Petition*> list;
	int32 last_insert_id;
	Mutex    PList_Mutex;
};

#endif
