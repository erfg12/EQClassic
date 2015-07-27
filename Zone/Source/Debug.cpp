///////////////////////////////////////////////////
// Written by Cofruben: 17/08/08.
///////////////////////////////////////////////////
#include <Windows.h>
#include <cstdio>
#include <time.h>
#include <io.h>
#include "database.h"
#include "client.h"
#include "Debug.hpp"

///////////////////////////////////////////////////
///////////////////Constructors////////////////////
///////////////////////////////////////////////////
DebugSystem::DebugSystem(Client* c) : mDebugList() {
	this->mpClient = c;
	memset(&this->mDebugList, 0, sizeof(TDebugList));
	if(!Database::Instance()->GetDebugList(c->CharacterID(), &this->mDebugList))
		Database::Instance()->SetDebugList(c->CharacterID(), &this->mDebugList);	// It's not present, so let's present it =P.
	if (this->IsLogging())
		this->OpenFile();
}

///////////////////////////////////////////////////

DebugSystem::~DebugSystem() {
	if (this->IsFileOpened())
		this->CloseFile();
}


///////////////////////////////////////////////////
////////////////////Functions//////////////////////
///////////////////////////////////////////////////
bool DebugSystem::OpenFile() {
	// 1. Create folder.
	CreateDirectory(L"Logs", NULL);
	time_t	now;
	tm*		ts;
	char	buf[1024]; memset(buf, 0, sizeof(buf));
	time(&now);
	ts = localtime(&now);
	sprintf(buf, "Logs/%i.%i.%i_%s.txt", ts->tm_year+1900, ts->tm_mon, ts->tm_mday, this->mpClient->GetName());
	// 2. Open/create file.
	if (_access(buf, 0) == -1)
		this->mFile.open(buf, ios_base::out);
	else
		this->mFile.open(buf, ios_base::in | ios_base::out | ios_base::ate);
	this->mFile.seekp(0, ios_base::end);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "Opening log on %i/%i/%i\n", ts->tm_mday, ts->tm_mon, ts->tm_year+1900);
	this->mFile.write(buf, strlen(buf));
	return (this->mFile.good() && this->mFile.is_open());
}

///////////////////////////////////////////////////

void DebugSystem::CloseFile() {
	if(!this->IsFileOpened()) return;
	char msg[] = "Closing log\n-----------\n";
	this->mFile.write(msg, strlen(msg));
	this->mFile.close();
}

///////////////////////////////////////////////////

bool DebugSystem::IsFileOpened() {
	return (this->mFile && this->mFile.good() && this->mFile.is_open());
}

///////////////////////////////////////////////////

void DebugSystem::Log(CodePlace cp, char* text, ...)
{
	if(!this->mpClient) // Check if its null (added to stop a crash when) - Dark-Prince
	{
		return;
	}

	if (!this->mpClient->Connected()) // Crash was happening due to mpCLient being null (see above comment)
	{
		return;
	}

	if (cp == CP_SPELL && !this->mDebugList.debug_spells) return;
	if (cp == CP_UPDATES && !this->mDebugList.debug_updates) return;
	if (cp == CP_ATTACK && !this->mDebugList.debug_attack) return;
	if (cp == CP_FACTION && !this->mDebugList.debug_faction) return;
	if (cp == CP_PATHING && !this->mDebugList.debug_pathing) return;
	if (cp == CP_ITEMS && !this->mDebugList.debug_items) return;
	if (cp == CP_MERCHANT && !this->mDebugList.debug_merchants) return;

	const int32 TAM_MAX = 5000;
	if (!text) return;
	int32 Len = strlen(text);
	if (Len > TAM_MAX) return;
	va_list Args;
	char* Buffer = new char[Len*10+512];
	memset(Buffer, 0, Len*10+256);
	va_start(Args, text);
	vsnprintf(Buffer, Len*10 + 256, text, Args);
	va_end(Args);
	string MessageS = "[DEBUG]";
	MessageS += EQC::Common::GetPlace(cp);
	MessageS += " ";
	string NewString = this->mpClient->GetName();
	NewString += ": ";
	NewString += Buffer;
	MessageS += NewString;
	NewString += "\n";
	safe_delete_array(Buffer);//delete[] Buffer;
	EQC::Common::PrintF(cp, NewString.c_str());
	this->mpClient->Message(LIGHTEN_BLUE, MessageS.c_str());
	if (this->IsLogging()) {
		if (!this->IsFileOpened() && !this->OpenFile()) return;
		time_t	now;
		tm*		ts;
		char	buf[1024]; memset(buf, 0, sizeof(buf));
		time(&now);
		ts = localtime(&now);
		sprintf(buf, "[%i:%i:%i] %s", ts->tm_hour, ts->tm_min, ts->tm_sec, NewString.c_str());
		this->mFile.write(buf, strlen(buf));
	}
}

///////////////////////////////////////////////////

void DebugSystem::SaveList() {
	Database::Instance()->SetDebugList(this->mpClient->CharacterID(), &this->mDebugList);
}

///////////////////////////////////////////////////

void DebugSystem::SetLogging(bool log) {
	this->mpClient->Message(BLACK, "Logging to file set to %i", log);
	if (!log && this->IsFileOpened()) this->CloseFile();
	if (log && !this->IsFileOpened()) this->OpenFile();
	this->mDebugList.log_file = log;
	this->SaveList();
}

///////////////////////////////////////////////////

void DebugSystem::SetDebuggingSpells(bool spells){
	this->mpClient->Message(BLACK, "Spell debugging set to %i", spells);
	this->mDebugList.debug_spells = spells;
	this->SaveList();
}

///////////////////////////////////////////////////

void DebugSystem::SetDebuggingAttack(bool attack) {
	this->mpClient->Message(BLACK, "Attack debugging set to %i", attack);
	this->mDebugList.debug_attack = attack;
	this->SaveList();
}

///////////////////////////////////////////////////

void DebugSystem::SetDebuggingFaction(bool faction) {
	this->mpClient->Message(BLACK, "Faction debugging set to %i", faction);
	this->mDebugList.debug_faction = faction;
	this->SaveList();
}

///////////////////////////////////////////////////

void DebugSystem::SetDebuggingUpdates(bool updates) {
	this->mpClient->Message(BLACK, "Client updates debugging set to %i", updates);
	this->mDebugList.debug_updates = updates;
	this->SaveList();
}

///////////////////////////////////////////////////

void DebugSystem::SetDebuggingMerchants(bool merchants) {
	this->mpClient->Message(BLACK, "Merchant debugging set to %i", merchants);
	this->mDebugList.debug_merchants = merchants;
	this->SaveList();
}

///////////////////////////////////////////////////

void DebugSystem::SetDebuggingItems(bool items) {
	this->mpClient->Message(BLACK, "Items debugging set to %i", items);
	this->mDebugList.debug_items = items;
	this->SaveList();
}

///////////////////////////////////////////////////

void DebugSystem::SetDebuggingPathing(bool path) {
	this->mpClient->Message(BLACK, "Items debugging set to %i", path);
	this->mDebugList.debug_pathing = path;
	this->SaveList();
}

///////////////////////////////////////////////////