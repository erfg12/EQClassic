
#include <cstdlib>
#include <cstdio>
#include <stdarg.h>
#include <ctype.h>
#include <cstring>
#ifdef WIN32
	#include <process.h>
#else
	#include <pthread.h>
#endif

#include "packet_functions.h"
#include "packet_dump.h"
#include "packet_dump_file.h"
#include "eq_opcodes.h"
#include "eq_packet_structs.h"
#include "servertalk.h"
#include "entity.h"

#include "client.h"
#include "ClientList.h"
#include "petitions.h"
#include "worldserver.h"
#include "EQCUtils.hpp"
using namespace std;

PetitionList petition_list;

extern WorldServer worldserver;
extern ClientList client_list;


void Petition::SendPetitionToPlayer(Client* clientto) {
	APPLAYER* outapp = new APPLAYER(0x8e21,sizeof(Petition_Struct));
	Petition_Struct* pet = (Petition_Struct*) outapp->pBuffer;
	strcpy(pet->accountid,this->GetAccountName());
	strcpy(pet->lastgm,this->GetLastGM());
	strcpy(pet->charname,this->GetCharName());
	pet->petnumber = this->petid;
	pet->charclass = this->GetCharClass();
	pet->charlevel = this->GetCharLevel();
	pet->charrace = this->GetCharRace();
	strcpy(pet->zone,this->GetZone());
	strcpy(pet->petitiontext,this->GetPetitionText());
	pet->checkouts = this->GetCheckouts();
	pet->unavail = this->GetUnavails();
	pet->senttime = this->GetSentTime();
	memset(pet->unknown5, 0, sizeof(pet->unknown5));
	pet->unknown5[3] = 0x1f;
	pet->urgency = this->GetUrgency();
	clientto->QueuePacket(outapp);
	safe_delete(outapp);//delete outapp;
	return;
}

Petition::Petition() {
	this->petid = petition_list.GetNextPetitionID();
	charclass = 0;
	charrace = 0;
	charlevel = 0;
	checkouts = 0;
	unavailables = 0;
	urgency = 0;
	time(&senttime);
	ischeckedout = false;
	memset(accountname, 0, sizeof(accountname));
	memset(charname, 0, sizeof(charname));
	memset(lastgm, 0, sizeof(lastgm));
	memset(petitiontext, 0, sizeof(petitiontext));
	
	memset(this->zone, 0, sizeof(this->zone));
}

Petition* PetitionList::GetPetitionByID(int32 id_in) { 
	LinkedListIterator<Petition*> iterator(list); 

	iterator.Reset(); 
	while(iterator.MoreElements()) { 
		if (iterator.GetData()->GetID() == id_in) 
			return iterator.GetData();
		iterator.Advance(); 
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	} 
	return 0; 
} 

void PetitionList::UpdateZoneListQueue() {
	ServerPacket* pack = new ServerPacket(ServerOP_Petition, sizeof(ServerPetitionUpdate_Struct));

	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, sizeof(pack->pBuffer));
	ServerPetitionUpdate_Struct* pupdate = (ServerPetitionUpdate_Struct*) pack->pBuffer;
	pupdate->petid = 0x00;
	pupdate->status = 0x00;
	worldserver.SendPacket(pack);
	safe_delete(pack);//delete pack;
}

void PetitionList::AddPetition(Petition* pet) {
	list.Insert(pet);
	return;
}

//Return Values: 0 = Ok ; -1 = Error deleting petition.
int PetitionList::DeletePetition(int32 petnumber) {
	LinkedListIterator<Petition*> iterator(list);
	iterator.Reset();
	LockMutex lock(&PList_Mutex);
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetID() == petnumber) {
			Database::Instance()->DeletePetitionFromDB(iterator.GetData());
			iterator.RemoveCurrent();
			return 0;
			break;
		}
		else {
			iterator.Advance();
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return -1;
}

void PetitionList::UpdateGMQueue() {
	LinkedListIterator<Petition*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements()) {
		entity_list.SendPetitionToAdmins(iterator.GetData());
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return;
}

void PetitionList::ClearPetitions() {
//	entity_list.ClearClientPetitionQueue();
	LinkedListIterator<Petition*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.RemoveCurrent(true);
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return;
}

void PetitionList::ReadDatabase() {
	LockMutex lock(&PList_Mutex);
	ClearPetitions();
	Database::Instance()->RefreshPetitionsFromDB();
	UpdateGMQueue();
	return;
}

void	PetitionList::UpdatePetition(Petition* pet) {
	LockMutex lock(&PList_Mutex);
	Database::Instance()->UpdatePetitionToDB(pet);
	return;
}

void	Database::DeletePetitionFromDB(Petition* wpet) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	int8 checkedout = 0;
	if (wpet->CheckedOut()) checkedout = 0;
	else checkedout = 1;
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE from petitions where petid = %i", wpet->GetID()), errbuf, 0, &affected_rows)) {
		cerr << "Error in DeletePetitionToDB query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);//delete[] query;

	return;
}

void	Database::UpdatePetitionToDB(Petition* wpet) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	int8 checkedout = 0;
	if (wpet->CheckedOut()) checkedout = 1;
	else checkedout = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE petitions set lastgm = '%s', urgency = %i, checkouts = %i, unavailables = %i, ischeckedout = %i where petid = %i", wpet->GetLastGM(), wpet->GetUrgency(), wpet->GetCheckouts(), wpet->GetUnavails(), checkedout, wpet->GetID()), errbuf, 0, &affected_rows)) {
		cerr << "Error in UpdatetPetitionToDB query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);//delete[] query;
	return;
}



void	Database::InsertPetitionToDB(Petition* wpet)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	int8 checkedout = 0;
	if (wpet->CheckedOut()) checkedout = 1;
	else checkedout = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO petitions (petid, charname, accountname, lastgm, petitiontext, zone, urgency, charclass, charrace, charlevel, checkouts, unavailables, ischeckedout, senttime) values (%i,'%s','%s','%s','%s','%s','%s',%i,%i,%i,%i,%i,%i,%d)", wpet->GetID(), wpet->GetCharName(), wpet->GetAccountName(), wpet->GetLastGM(), wpet->GetPetitionText(), wpet->GetZone(), wpet->GetUrgency(), wpet->GetCharClass(), wpet->GetCharRace(), wpet->GetCharLevel(), wpet->GetCheckouts(), wpet->GetUnavails(), checkedout, wpet->GetSentTime()), errbuf, 0, &affected_rows)) {
		cerr << "Error in InsertPetitionToDB query '" << query << "' " << errbuf << endl;
	}

	safe_delete_array(query);//delete[] query;
	cout << "Inserted petition.." << endl;
	return;
}

void	Database::RefreshPetitionsFromDB() {

	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	Petition* newpet;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT petid, charname, accountname, lastgm, petitiontext, zone, urgency, charclass, charrace, charlevel, checkouts, unavailables, ischeckedout, senttime from petitions order by petid"), errbuf, &result))
	{
		safe_delete_array(query);//delete[] query;
		while (row = mysql_fetch_row(result)) {
			newpet = new Petition;
			newpet->SetPetID(atoi(row[0]));
			newpet->SetCName(row[1]);
			newpet->SetAName(row[2]);
			newpet->SetLastGM(row[3]);
			newpet->SetPetitionText(row[4]);
			newpet->SetZone(row[5]);
			newpet->SetUrgency(atoi(row[6]));
			newpet->SetClass(atoi(row[7]));
			newpet->SetRace(atoi(row[8]));
			newpet->SetLevel(atoi(row[9]));
			newpet->SetCheckouts(atoi(row[10]));
			newpet->SetUnavails(atoi(row[11]));
			newpet->SetSentTime(atol(row[12]));
			cout << "Petition " << row[0] << " pettime = " << newpet->GetSentTime() << endl;
			if (atoi(row[12]) == 1) newpet->SetCheckedOut(true);
			else newpet->SetCheckedOut(false);
			petition_list.AddPetition(newpet);
		}
		mysql_free_result(result);
	}
	else {
		EQC::Common::PrintF(CP_DATABASE, "Error in RefreshPetitionsFromDB: %s\n" , errbuf);
		safe_delete_array(query);//delete[] query;
		return;
	}

	return;
}
