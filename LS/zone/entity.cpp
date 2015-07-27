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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <iostream.h>

#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#include "../common/unix.h"
#endif

#include "net.h"
#include "entity.h"
#include "client.h"
#include "object.h"
#include "npc.h"
#include "groups.h"
#include "doors.h"
#include "worldserver.h"
#include "PlayerCorpse.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "petitions.h"
#include "spdat.h"

#ifdef WIN32
#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define strncasecmp	_strnicmp
#define strcasecmp	_stricmp
#endif

extern Zone* zone;
extern volatile bool ZoneLoaded;
extern WorldServer worldserver;
extern GuildRanks_Struct guilds[512];
extern int32 numclients;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern PetitionList petition_list;

extern char  errorname[32];
extern int16 adverrornum;

Entity::Entity() {
	id = 0;
	pDBAsyncWorkID = 0;
}

Entity::~Entity() {
	dbasync->CancelWork(pDBAsyncWorkID);
}

void Entity::SetID(int16 set_id) {
	id = set_id;
}

Client* Entity::CastToClient() {
#ifdef _DEBUG
	if(!IsClient()) {
		cout << "CastToClient error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Client*>(this);
}

NPC* Entity::CastToNPC() {
#ifdef _DEBUG
	if(!IsNPC()) {	
		cout << "CastToNPC error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<NPC*>(this);
}

Mob* Entity::CastToMob() {
#ifdef _DEBUG
	if(!IsMob()) {	
		cout << "CastToMob error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Mob*>(this);
}

Corpse* Entity::CastToCorpse() {
#ifdef _DEBUG
	if(!IsCorpse()) {	
		cout << "CastToCorpse error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Corpse*>(this);
}
Object* Entity::CastToObject() {
#ifdef _DEBUG
	if(!IsObject()) {	
		cout << "CastToObject error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Object*>(this);
}

Group* Entity::CastToGroup() {
#ifdef _DEBUG
	if(!IsGroup()) {	
		cout << "CastToGroup error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Group*>(this);
}

Doors* Entity::CastToDoors() {
return static_cast<Doors*>(this);
}

bool EntityList::CanAddHateForMob(Mob *p) {
    LinkedListIterator<Entity*> iterator(list);
    int count = 0;

    iterator.Reset();
    while( iterator.MoreElements())
    {
        if (iterator.GetData() && iterator.GetData()->IsNPC())
        {
            NPC *npc;

            npc = iterator.GetData()->CastToNPC();

            if (npc->IsOnHatelist(p))
                count++;
        }
        // no need to continue if we already hit the limit
        if (count > 3)
            return false;
        iterator.Advance();
    }

    if (count <= 2)
        return true;
    return false;
}

void EntityList::AddClient(Client* client) {
	client->SetID(GetFreeID());
	
	list.Insert(client);
}

void EntityList::AddGroup(Group* group) {
	group->SetID(GetFreeID());
	
	list.Insert(group);
}

void EntityList::GuildItemAward(int32 guilddbid, int16 itemid)
{
if(itemid != 0)
{
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->GuildDBID() == guilddbid)
		{
			iterator.GetData()->CastToClient()->SummonItem(itemid);
		}
		iterator.Advance();
	}
}
}

bool EntityList::GuildNPCPlaceable(Mob* client)
{
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsNPC() && iterator.GetData()->CastToNPC()->GetGuildOwner() != 0)
		{
			if(client->DistNoZ(iterator.GetData()->CastToMob()) <= 50)
				return false;
		}
		iterator.Advance();
	}
	return true;
}

void EntityList::AddCorpse(Corpse* corpse, int32 in_id) {
	if (corpse == 0)
		return;
	
	if (in_id == 0xFFFFFFFF)
		corpse->SetID(GetFreeID());
	else
		corpse->SetID(in_id);
	corpse->CalcCorpseName();
	
	list.Insert(corpse);
}

void EntityList::AddNPC(NPC* npc, bool SendSpawnPacket, bool dontqueue) {
	npc->SetID(GetFreeID());
	
	if (SendSpawnPacket) {
		if (dontqueue) { // aka, SEND IT NOW BITCH!
			APPLAYER* app = new APPLAYER;
			npc->CreateSpawnPacket(app);
			QueueClients(npc, app);
			safe_delete(app);
			parse->Event(EVENT_SPAWN, npc->GetNPCTypeID(), 0, npc->CastToMob(), 0);
		}



		else {
			NewSpawn_Struct* ns = new NewSpawn_Struct;
			memset(ns, 0, sizeof(NewSpawn_Struct));
			npc->FillSpawnStruct(ns, 0);	// Not working on player newspawns, so it's safe to use a ForWho of 0
			AddToSpawnQueue(npc->GetID(), &ns);
			parse->Event(EVENT_SPAWN, npc->GetNPCTypeID(), 0, npc->CastToMob(), 0);
		}
	}
	
	list.Insert(npc);
};

void EntityList::AddObject(Object* obj, bool SendSpawnPacket) {
	obj->SetID(GetFreeID()); 
	if (SendSpawnPacket) {
		APPLAYER app;
		obj->CreateSpawnPacket(&app);
		cout << "send:" << endl;
		//		DumpPacket(&app);
		QueueClients(0, &app,false);
	}
	list.Insert(obj);
};

void EntityList::AddDoor(Doors* door) {
	door->SetEntityID(GetFreeID());
	list.Insert(door);
}

void EntityList::AddToSpawnQueue(int16 entityid, NewSpawn_Struct** ns) {
	SpawnQueue.Append(*ns);
	NumSpawnsOnQueue++;
	if (tsFirstSpawnOnQueue == 0xFFFFFFFF)
		tsFirstSpawnOnQueue = Timer::GetCurrentTime();
	*ns = 0; // make it so the calling function cant fuck us and delete the data =)
}

void EntityList::CheckSpawnQueue() {
	// Send the stuff if the oldest packet on the queue is older than 50ms -Quagmire
	if (tsFirstSpawnOnQueue != 0xFFFFFFFF && (Timer::GetCurrentTime() - tsFirstSpawnOnQueue) > 50) {
		if (NumSpawnsOnQueue <= 5) {
			LinkedListIterator<NewSpawn_Struct*> iterator(SpawnQueue);
			APPLAYER* outapp = 0;
			
			iterator.Reset();
			while(iterator.MoreElements()) {
				outapp = new APPLAYER;
				Mob::CreateSpawnPacket(outapp, iterator.GetData());
//				cout << "Sending spawn packet: " << iterator.GetData()->spawn.name << endl;
				QueueClients(0, outapp);
				safe_delete(outapp);
				iterator.RemoveCurrent();
			}
		}
		else {
			BulkZoneSpawnPacket* bzsp = new BulkZoneSpawnPacket(0, MAX_SPAWNS_PER_PACKET);
			LinkedListIterator<NewSpawn_Struct*> iterator(SpawnQueue);
			
			iterator.Reset();
			while(iterator.MoreElements()) {
				bzsp->AddSpawn(iterator.GetData());
				iterator.RemoveCurrent();
			}
			safe_delete(bzsp);
		}
		
		tsFirstSpawnOnQueue = 0xFFFFFFFF;
		NumSpawnsOnQueue = 0;
	}
}

Doors* EntityList::FindDoor(int8 door_id)
{
	if (door_id == 0)
		return 0;
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsDoor() && iterator.GetData()->CastToDoors()->GetDoorID() == door_id)
		{
			return iterator.GetData()->CastToDoors();
		}
		iterator.Advance();
	}
	return 0;
}

bool EntityList::MakeDoorSpawnPacket(APPLAYER* app){
	uchar buffer1[sizeof(Door_Struct)];
	uchar buffer2[7000];
	int16 length = 0;
	int16 qty = 0;

	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->CastToDoors() != 0)
		{
			if(iterator.GetData()->CastToDoors()->GetDoorID() != 0)
			{
			Door_Struct* nd = (Door_Struct*)buffer1;
			memset(nd,0,sizeof(Door_Struct));
			if(iterator.GetData()->CastToDoors()->GetGuildID() != 0)
				nd->doorId = iterator.GetData()->CastToDoors()->GetGuildID();
			else
				nd->doorId = iterator.GetData()->CastToDoors()->GetDoorID();

			if ((strcasecmp("FAYLEVATOR", iterator.GetData()->CastToDoors()->GetDoorName()) == 0) || (strcasecmp("FELE2", iterator.GetData()->CastToDoors()->GetDoorName()) == 0))
			{
				//nd->auto_return = 10;
				//nd->unknown6 = 10;
				if(iterator.GetData()->CastToDoors()->GetDoorDBID() == 2563) // This is temp till the liftheight works
					nd->holdstateforever=25515;
				else if(strcasecmp("FELE2", iterator.GetData()->CastToDoors()->GetDoorName()) == 0)
					nd->holdstateforever = 500;
				else 
					nd->holdstateforever = 17515;

			}
#if 1
			else
			{
				//nd->unknown3 = 1;
				nd->auto_return = 0;
				nd->unknown6 = 60;
				nd->unknown3 = 32000;
			}
#endif
			memcpy(nd->name,iterator.GetData()->CastToDoors()->GetDoorName(),16);
			nd->opentype = iterator.GetData()->CastToDoors()->GetOpenType();

			nd->xPos = iterator.GetData()->CastToDoors()->GetX();
			nd->yPos = iterator.GetData()->CastToDoors()->GetY();

			nd->zPos = iterator.GetData()->CastToDoors()->GetZ();
			nd->heading = iterator.GetData()->CastToDoors()->GetHeading();

			memcpy(buffer2+length,buffer1,44);
			length = length + 44;
			qty++;
		}
		}
		iterator.Advance();
	}
#if DEBUG >= 11
		LogFile->write(EQEMuLog::Debug, "MakeDoorPacket() packet length:%i", length);
#endif
		if (qty == 0)
			return false;
		
		app->opcode = OP_SpawnDoor;
		app->pBuffer = new uchar[7000];
		length = DeflatePacket(buffer2,length,app->pBuffer+2,7000);
		app->size = length+2;
		DoorSpawns_Struct* ds = (DoorSpawns_Struct*)app->pBuffer;
		/*	Debug:
		uchar buffer3[2000];
		int txx = InflatePacket(app->pBuffer+2,length,buffer3,2000);
		cout << txx << endl;
		memcpy(app->pBuffer,buffer3,txx);		
		app->size =	txx;*/
		ds->count = qty;
		return true;	
}

Entity* EntityList::GetID(int16 get_id)
{
	if (get_id == 0)
		return 0;
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetID() == get_id)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}

Mob* EntityList::GetMob(int16 get_id)
{
	if (get_id == 0)
		return 0;
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsMob() && iterator.GetData()->GetID() == get_id)
		{
			return iterator.GetData()->CastToMob();
		}
		iterator.Advance();
	}
	return 0;
}

Mob* EntityList::GetMobByNpcTypeID(int32 get_id)
{
	if (get_id == 0)
		return 0;
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsMob() && iterator.GetData()->CastToMob()->GetNPCTypeID() == get_id)
		{
			return iterator.GetData()->CastToMob();
		}
		iterator.Advance();
	}
	return 0;
}

Mob* EntityList::GetMob(const char* name) {
	if (name == 0)
		return 0;
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsMob() && strcasecmp(iterator.GetData()->GetName(), name) == 0)
		{
			return iterator.GetData()->CastToMob();
		}
		iterator.Advance();
	}
	return 0;
}

int16 EntityList::GetFreeID()
{
	while(1)
	{
		last_insert_id++;
		if (last_insert_id == 0xFFFF)
			last_insert_id++;
		if (last_insert_id == 0)
			last_insert_id++;
		if (GetID(last_insert_id) == 0)
		{
			return last_insert_id;
		}
	}
}

void EntityList::ChannelMessage(Mob* from, int8 chan_num, int8 language, const char* message, ...) {
	LinkedListIterator<Entity*> iterator(list);
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
//	cout << "Message:" << buffer << endl;
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient())
		{
			Client* client = iterator.GetData()->CastToClient();
			if (chan_num != 8 || client->Dist(from) < 200) // Only say is limited in range
			{
				client->ChannelMessageSend(from->GetName(), 0, chan_num, language, buffer);
			}
		}
		iterator.Advance();
	}
}

void EntityList::ChannelMessageSend(Mob* to, int8 chan_num, int8 language, const char* message, ...) {
	LinkedListIterator<Entity*> iterator(list);
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
//	cout << "Message:" << buffer << endl;
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient())
		{
			Client* client = iterator.GetData()->CastToClient();
			if (client->GetID() == to->GetID()) {
				client->ChannelMessageSend(0, 0, chan_num, language, buffer);
				break;
			}
		}
		iterator.Advance();
	}
}

void EntityList::SendZoneSpawns(Client* client)
{
	LinkedListIterator<Entity*> iterator(list);
	//va_list argptr;
	//char buffer[256];
	
	APPLAYER* app;
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob()) {
			if (!iterator.GetData()->CastToMob()->InZone() || (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->GMHideMe(client))) {
				iterator.Advance();
				continue;
			}
			app = new APPLAYER;
			iterator.GetData()->CastToMob()->CreateSpawnPacket(app); // TODO: Use zonespawns opcode instead
            client->QueuePacket(app, true, Client::CLIENT_CONNECTED);
			delete app;
		}
		iterator.Advance();
	}	
}

void EntityList::SendZoneObjects(Client* client)
{
	LinkedListIterator<Entity*> iterator(list);
	APPLAYER* app;
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsObject())
		{
			app = new APPLAYER;
			iterator.GetData()->CastToObject()->CreateSpawnPacket(app);
			client->QueuePacket(app);
			delete app;
		}
		iterator.Advance();
	}	
}

void EntityList::Save()
{
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.GetData()->Save();
		iterator.Advance();
	}	
}

void EntityList::ReplaceWithTarget(Mob* pOldMob, Mob*pNewTarget)
{
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob() && iterator.GetData()->CastToMob()->IsAIControlled()) {
            // replace the old mob with the new one
			if (iterator.GetData()->CastToMob()->RemoveFromHateList(pOldMob))
                if (pNewTarget)
                    iterator.GetData()->CastToMob()->AddToHateList(pNewTarget, 1, 0);
		}
		iterator.Advance();
	}
}

void EntityList::RemoveFromTargets(Mob* mob)
{
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob()) {
			iterator.GetData()->CastToMob()->RemoveFromHateList(mob);
		}
		iterator.Advance();
	}	
}

void EntityList::QueueClientsByTarget(Mob* sender, const APPLAYER* app, bool iSendToSender, Mob* SkipThisMob, bool ackreq) {
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsClient() && (iSendToSender || (iterator.GetData() != sender && iterator.GetData()->CastToClient()->GetTarget() == sender)) && iterator.GetData() != SkipThisMob) {
			iterator.GetData()->CastToClient()->QueuePacket(app, ackreq);
		}
		iterator.Advance();
	}	
}

void EntityList::QueueCloseClients(Mob* sender, const APPLAYER* app, bool ignore_sender, float dist, Mob* SkipThisMob, bool ackreq) {
	if (sender == 0) {
		QueueClients(sender, app, ignore_sender);
		return;
	}
	if(dist <= 0) {
		dist = 600;
	}
	float dist2 = dist * dist; //pow(dist, 2);
	
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsClient() && (!ignore_sender || iterator.GetData() != sender) && (iterator.GetData() != SkipThisMob)) {
			if(iterator.GetData()->CastToClient()->DistNoRoot(sender) <= dist2 || dist == 0) {
				iterator.GetData()->CastToClient()->QueuePacket(app, ackreq);
			}
		}
		iterator.Advance();
	}	
}

void EntityList::QueueClients(Mob* sender, const APPLAYER* app, bool ignore_sender, bool ackreq) {
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient() && (!ignore_sender || iterator.GetData() != sender))
		{
            iterator.GetData()->CastToClient()->QueuePacket(app, ackreq, Client::CLIENT_CONNECTED);
		}
		iterator.Advance();
	}	
}


void EntityList::QueueClientsStatus(Mob* sender, const APPLAYER* app, bool ignore_sender, int8 minstatus, int8 maxstatus)
{
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient() && (!ignore_sender || iterator.GetData() != sender) && (iterator.GetData()->CastToClient()->Admin() >= minstatus && iterator.GetData()->CastToClient()->Admin() <= maxstatus))
		{
			iterator.GetData()->CastToClient()->QueuePacket(app);
		}
		iterator.Advance();
	}	
}

void EntityList::AESpell(Mob* caster, Mob* center, float dist, int16 spell_id, bool group) {
	LinkedListIterator<Entity*> iterator(list);
	
	//printf("Dist: %f\n",dist);
	//cout << "AE Spell Cast: c=" << center->GetName() << ", d=" << dist << ", x=" << center->GetX() << ", y=" << center->GetY() << endl;
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob()) {
			Mob* mob = iterator.GetData()->CastToMob();
#if 0
			if (group && !mob->IsGrouped()) {
                        iterator.Advance();
                        continue;
			}
#endif
			if (group){
			     // Client casting group spell with out target group buffs enabled
			     // Skip non group members
                 if (   caster->IsClient()
                     && !caster->CastToClient()->TGB()
                     && GetGroupByMob(mob) != 0
                     && !GetGroupByMob(mob)->IsGroupMember(caster)
                     ) {
                     LogFile->write(EQEMuLog::Debug, "Group spell skipping %s", mob->GetName());
                         iterator.Advance();
                         continue;
                 }
			     // Client casting group spell with target group buffs enabled
                 else if (  caster->IsClient()
                         && caster->CastToClient()->TGB()
                         && GetGroupByMob(mob) != 0
                         && GetGroupByMob(mob)->IsGroupMember(caster)
                         ){
                         LogFile->write(EQEMuLog::Debug, "Group spell TGB on %s's Group", mob->GetName());
                         GetGroupByMob(mob)->CastGroupSpell(caster, spell_id);
                         iterator.Advance();
                         continue;
                 }
                 else if (  caster->IsClient()
                         && caster->CastToClient()->TGB()
                         && GetGroupByMob(mob) == 0
                         && mob == center
                         ){
                         LogFile->write(EQEMuLog::Debug, "Group spell TGB on %s", mob->GetName());
                         caster->SpellOnTarget(spell_id, mob);
                         return;
                 }
			}
			if (
				mob->DistNoZ(center) <= dist
				&& !(mob->IsClient() && mob->CastToClient()->GMHideMe())
				&& !mob->IsCorpse()
				) {
				//cout << "AE Spell Hit: t=" << iterator.GetData()->GetName() << ", d=" << iterator.GetData()->CastToMob()->DistNoRoot(center) << ", x=" << iterator.GetData()->CastToMob()->GetX() << ", y=" << iterator.GetData()->CastToMob()->GetY() << endl;
				if (caster == mob) {
				    // Caster gets the first hit, already handled in spells.cpp
				}
				else if(caster->IsNPC() && !caster->CastToNPC()->IsInteractive()) {
				    // Npc
					if (caster->IsAttackAllowed(mob) && spells[spell_id].targettype != ST_AEBard) {
					//    printf("NPC Spell casted on %s\n", mob->GetName());
						caster->SpellOnTarget(spell_id, mob);
					}
					else if (mob->IsAIControlled() && spells[spell_id].targettype == ST_AEBard) {
					//    printf("NPC mgb/aebard spell casted on %s\n", mob->GetName());
						caster->SpellOnTarget(spell_id, mob);
					}
					else {
					//    printf("NPC AE, fall thru. spell_id:%i, Target type:%x\n", spell_id, spells[spell_id].targettype);
					}
				}
				else if(caster->IsNPC() && caster->CastToNPC()->IsInteractive()) {
				    // Interactive npc
					if (caster->IsAttackAllowed(mob) && spells[spell_id].targettype != ST_AEBard && spells[spell_id].targettype != ST_AlterPlane) {
					//    printf("IPC Spell casted on %s\n", mob->GetName());
						caster->SpellOnTarget(spell_id, mob);
					}
					else if (!mob->IsAIControlled() && (spells[spell_id].targettype == ST_AEBard||group) && mob->CastToClient()->GetPVP() == caster->CastToClient()->GetPVP()) {
					     if (group && GetGroupByMob(mob) != GetGroupByMob(caster)) {
                                  iterator.Advance();
                                  continue;
                        }
					//    printf("IPC mgb/aebard spell casted on %s\n", mob->GetName());
					caster->SpellOnTarget(spell_id, mob);
					}
					else {
					//    printf("NPC AE, fall thru. spell_id:%i, Target type:%x\n", spell_id, spells[spell_id].targettype);
					}
				}
				else if (caster->IsClient() && !(caster->CastToClient()->IsBecomeNPC())) {
				    // Client
					if (caster->IsAttackAllowed(mob) && spells[spell_id].targettype != ST_AEBard){
					//    printf("Client Spell casted on %s\n", mob->GetName());
						caster->SpellOnTarget(spell_id, mob);
					}
					else if(spells[spell_id].targettype == ST_GroupTeleport && mob->IsClient() && mob->isgrouped && caster->isgrouped && entity_list.GetGroupByMob(caster))
					{
					    Group* caster_group = entity_list.GetGroupByMob(caster);
                        if(caster_group != 0 && caster_group->IsGroupMember(mob))
					        caster->SpellOnTarget(spell_id,mob);
					}
					else if (mob->IsClient() && (spells[spell_id].targettype == ST_AEBard||group) && mob->CastToClient()->GetPVP() == caster->CastToClient()->GetPVP()) {
					 if (group && GetGroupByMob(mob) != GetGroupByMob(caster)) {
                                  iterator.Advance();
                                  continue;
                     }
					 else if (mob->IsClient() && spells[spell_id].targettype == ST_AEBard && mob->CastToClient()->GetPVP() == caster->CastToClient()->GetPVP())
					 //    printf("Client mgb/aebard spell casted on %s\n", mob->GetName());
						caster->SpellOnTarget(spell_id, mob);
					 else if (mob->IsNPC() && mob->CastToNPC()->IsInteractive()) {
					     if (group && GetGroupByMob(mob) != GetGroupByMob(caster))
					               continue;
					 // printf("Client mgb/aebard spell casted on ipc %s\n", mob->GetName());
					     caster->SpellOnTarget(spell_id, mob);
					 }
					}
					else {
					//    printf("Client AE, fall thru. spell_id:%i, Target type:%x\n", spell_id, spells[spell_id].targettype);
					}
				}
				else if (caster->IsClient()) {
				    // Client BecomeNPC
				    //printf("Client NPC AE casted on %s\n", mob->GetName());
					caster->SpellOnTarget(spell_id, mob);
				}
			}
		}
		iterator.Advance();
	}	
}

Client* EntityList::GetClientByName(const char *checkname) {
	LinkedListIterator<Entity*> iterator(list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsClient()) 
		{ 
			if (strcasecmp(iterator.GetData()->CastToClient()->GetName(), checkname) == 0) {
				return iterator.GetData()->CastToClient();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
}

Client* EntityList::GetClientByCharID(int32 iCharID) {
	LinkedListIterator<Entity*> iterator(list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) { 
		if (iterator.GetData()->IsClient()) { 
			if (iterator.GetData()->CastToClient()->CharacterID() == iCharID) {
				return iterator.GetData()->CastToClient();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
}

Client* EntityList::GetClientByWID(int32 iWID) {
	LinkedListIterator<Entity*> iterator(list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) { 
		if (iterator.GetData()->IsClient()) { 
			if (iterator.GetData()->CastToClient()->GetWID() == iWID) {
				return iterator.GetData()->CastToClient();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
}

Corpse*	EntityList::GetCorpseByOwner(Client* client){
	LinkedListIterator<Entity*> iterator(list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsCorpse() && iterator.GetData()->CastToCorpse()->IsPlayerCorpse()) 
		{ 
			if (strcasecmp(iterator.GetData()->CastToCorpse()->GetOwnerName(), client->GetName()) == 0) {
				return iterator.GetData()->CastToCorpse();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
}
Corpse* EntityList::GetCorpseByID(int16 id){
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsCorpse())
		{
			if (iterator.GetData()->id == id) {
				return iterator.GetData()->CastToCorpse();
			}
		}
		iterator.Advance();
	}
	return 0;
}

Group* EntityList::GetGroupByMob(Mob* mob) 
{ 
	LinkedListIterator<Entity*> iterator(list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsGroup()) 
		{ 
			if (iterator.GetData()->CastToGroup()->IsGroupMember(mob)) {
				return iterator.GetData()->CastToGroup();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
} 

Group* EntityList::GetGroupByClient(Client* client) 
{ 
	LinkedListIterator<Entity*> iterator(list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsGroup()) 
		{ 
			if (iterator.GetData()->CastToGroup()->IsGroupMember(client->CastToMob())) {
				return iterator.GetData()->CastToGroup();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
} 

Client* EntityList::GetClientByAccID(int32 accid) 
{ 
	LinkedListIterator<Entity*> iterator(list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsClient()) 
		{ 
			if (iterator.GetData()->CastToClient()->AccountID() == accid) {
				return iterator.GetData()->CastToClient();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
} 
Client* EntityList::GetClientByID(int16 id) { 
	LinkedListIterator<Entity*> iterator(list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsClient()) 
		{ 
			if (iterator.GetData()->CastToClient()->GetID() == id) {
				return iterator.GetData()->CastToClient();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
} 

void EntityList::ChannelMessageFromWorld(const char* from, const char* to, int8 chan_num, int32 guilddbid, int8 language, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
	LinkedListIterator<Entity*> iterator(list);
	
//	cout << "Message: " << (int) chan_num << " " << (int) guilddbid << ": " << buffer << endl;
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient())
		{
			Client* client = iterator.GetData()->CastToClient();
			if (chan_num != 0 || (client->GuildDBID() == guilddbid && client->GuildEQID() != 0xFFFFFF && guilds[client->GuildEQID()].rank[client->GuildRank()].heargu))
				client->ChannelMessageSend(from, to, chan_num, language, buffer);
		}
		iterator.Advance();

	}
}

//deprecating.  Use NPCMessage
void EntityList::Message(int32 to_guilddbid, int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
	LinkedListIterator<Entity*> iterator(list);
	
//	cout << "Message:" << buffer << endl;
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient())
		{
			Client* client = iterator.GetData()->CastToClient();
			if (to_guilddbid == 0 || client->GuildDBID() == to_guilddbid)
				client->Message(type, buffer);
		}
		iterator.Advance();

	}
}

void EntityList::MessageStatus(int32 to_guilddbid, int32 to_minstatus, int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsClient()) {
			Client* client = iterator.GetData()->CastToClient();
			if ((to_guilddbid == 0 || client->GuildDBID() == to_guilddbid) && client->Admin() >= to_minstatus)
				client->Message(type, buffer);
		}
		iterator.Advance();
	}
}

//deprecating.  Use NPCMessage
void EntityList::MessageClose(Mob* sender, bool skipsender, float dist, int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4095, message, argptr);
	va_end(argptr);
	
	float dist2 = dist * dist; //pow(dist, 2);
	
	LinkedListIterator<Entity*> iterator(list);
	
//	cout << "Message:" << buffer << endl;
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->DistNoRoot(sender) <= dist2 && (!skipsender || iterator.GetData() != sender)) {
			iterator.GetData()->CastToClient()->Message(type, buffer);
		}
		iterator.Advance();
	}
}

//Edgar's updated message function. Strips off trailing 0's
void EntityList::NPCMessage(Mob* sender, bool skipsender, float dist, int32 type, const char* message, ...) { 
   va_list argptr; 
   char buffer[4096]; 
   char *findzero; 
    int  stripzero; 
   va_start(argptr, message); 
   vsnprintf(buffer, 4095, message, argptr); 
   va_end(argptr); 
    findzero = strstr( buffer, "0" ); 
    stripzero = (int)(findzero - buffer + 2); 
   if (stripzero > 2 && stripzero<4096) //Incase its not an npc, you dont want to crash the zone 
      strncpy(buffer + stripzero," ",1); 
   float dist2 = dist * dist; //pow(dist, 2); 
   char *tmp = new char[strlen(buffer)];
   memset(tmp,0x0,sizeof(tmp));
    
   LinkedListIterator<Entity*> iterator(list); 


   //cout << "Message:" << buffer << endl; 
   if (dist2==0) { //If 0 then send to the whole zone 
      iterator.Reset(); 
      while(iterator.MoreElements()) 
      { 
         if (iterator.GetData()->IsClient()) 
         { 
            Client* client = iterator.GetData()->CastToClient(); 
               client->Message(type, buffer); 
         } 
         iterator.Advance(); 
      } 
   } 
   else { 
      iterator.Reset(); 
      while(iterator.MoreElements()) 
      { 
         if (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->DistNoRoot(sender) <= dist2 && (!skipsender || iterator.GetData() != sender)) { 
            iterator.GetData()->CastToClient()->Message(type, buffer); 
         } 
         iterator.Advance(); 
      } 
   }

   if (sender->GetTarget() && sender->GetTarget()->IsNPC() && buffer)
   {
	   strcpy(tmp,strstr(buffer,"says"));
	   tmp[strlen(tmp) - 1] = '\0';
	   while (*tmp)
	   {
    	   tmp++;
		   if (*tmp == '\'') { tmp++; break; }
	   }
	   if (tmp) parse->Event(1, sender->GetTarget()->GetNPCTypeID(), tmp, sender->GetTarget(), sender);
   }

} 

// Safely removes all entities before zone shutdown
void EntityList::Clear()
{
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.GetData()->Save();
		if (iterator.GetData()->IsMob())
			entity_list.RemoveFromTargets(iterator.GetData()->CastToMob());
		iterator.RemoveCurrent();
	}	
	last_insert_id = 0;
}

void EntityList::UpdateWho(bool iSendFullUpdate) {
	if ((!worldserver.Connected()) || !ZoneLoaded)
		return;
	LinkedListIterator<Entity*> iterator(list);
	int32 tmpNumUpdates = numclients + 5;
	ServerPacket* pack = 0;
	ServerClientListKeepAlive_Struct* sclka = 0;
	if (!iSendFullUpdate) {
		pack = new ServerPacket(ServerOP_ClientListKA, sizeof(ServerClientListKeepAlive_Struct) + (tmpNumUpdates * 4));
		sclka = (ServerClientListKeepAlive_Struct*) pack->pBuffer;
	}
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->InZone()) {
			if (iSendFullUpdate) {
				iterator.GetData()->CastToClient()->UpdateWho();
			}
			else {

				if (sclka->numupdates >= tmpNumUpdates) {
					tmpNumUpdates += 10;
					int8* tmp = pack->pBuffer;
					pack->pBuffer = new int8[sizeof(ServerClientListKeepAlive_Struct) + (tmpNumUpdates * 4)];
					memset(pack->pBuffer, 0, sizeof(ServerClientListKeepAlive_Struct) + (tmpNumUpdates * 4));
					memcpy(pack->pBuffer, tmp, pack->size);
					pack->size = sizeof(ServerClientListKeepAlive_Struct) + (tmpNumUpdates * 4);
					delete tmp;
				}
				sclka->wid[sclka->numupdates] = iterator.GetData()->CastToClient()->GetWID();
				sclka->numupdates++;
			}
		}
		iterator.Advance();
	}
	if (!iSendFullUpdate) {
		pack->size = sizeof(ServerClientListKeepAlive_Struct) + (sclka->numupdates * 4);
		worldserver.SendPacket(pack);
		delete pack;
	}
}

void EntityList::RemoveEntity(int16 id)
{
	if (id == 0)
		return;
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetID() == id)
		{
			cout << "spawn deleted" << endl;
			if (iterator.GetData()->IsMob())
				entity_list.RemoveFromTargets(iterator.GetData()->CastToMob());
			iterator.RemoveCurrent();
			return;
		}
		iterator.Advance();
	}
	//cout << "error deleting spawn" << endl;
	LogFile->write(EQEMuLog::Debug, "RemoveEntity(%i) failed", id);
}

void EntityList::Process()
{
	bool processed;
	
	CheckSpawnQueue();
	
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
#ifdef CATCH_CRASH
		try{
#endif
			processed = iterator.GetData()->Process();
#ifdef CATCH_CRASH
		}
		catch(...){
			if (iterator.GetData()->IsMob()){
				strcpy(errorname,iterator.GetData()->CastToMob()->GetName());
				adverrornum = iterator.GetData()->CastToMob()->GetErrorNumber();
			}
			else if (iterator.GetData()->IsObject())
				strcpy(errorname,"Object");
			else if (iterator.GetData()->IsGroup())
				strcpy(errorname,"Group");
			else if (iterator.GetData()->IsCorpse())
				strcpy(errorname,iterator.GetData()->CastToCorpse()->GetName()); 
			throw;
		}
#endif
		if (!processed) {
			if (iterator.GetData()->IsClient()) {
#ifdef WIN32
				struct in_addr	in;
				in.s_addr = iterator.GetData()->CastToClient()->GetIP();
				cout << "Dropping client: Process=false, ip=" << inet_ntoa(in) << ", port=" << iterator.GetData()->CastToClient()->GetPort() << endl;
#endif
				zone->StartShutdownTimer();
			}
			if (iterator.GetData()->IsMob())
				entity_list.RemoveFromTargets(iterator.GetData()->CastToMob());

			if (iterator.GetData()->IsGroup())
				iterator.RemoveCurrent();
			iterator.RemoveCurrent();
		}
		else
		{
			iterator.Advance();
		}
	}
}

void EntityList::CountNPC(int32* NPCCount, int32* NPCLootCount, int32* gmspawntype_count) {
	LinkedListIterator<Entity*> iterator(list);
	*NPCCount = 0;
	*NPCLootCount = 0;
	
	iterator.Reset();
	while(iterator.MoreElements())	
	{
		if (iterator.GetData()->IsNPC()) {
			(*NPCCount)++;
			(*NPCLootCount) += iterator.GetData()->CastToNPC()->CountLoot();
			if (iterator.GetData()->CastToNPC()->GetNPCTypeID() == 0)
				(*gmspawntype_count)++;
		}
		iterator.Advance();
	}
}

void EntityList::DoZoneDump(ZSDump_Spawn2* spawn2_dump, ZSDump_NPC* npc_dump, ZSDump_NPC_Loot* npcloot_dump, NPCType* gmspawntype_dump) {
	int32 spawn2index = 0;
	int32 NPCindex = 0;
	int32 NPCLootindex = 0;
	int32 gmspawntype_index = 0;
	
	if (npc_dump != 0) {
		LinkedListIterator<Entity*> iterator(list);
		NPC* npc = 0;
		iterator.Reset();
		while(iterator.MoreElements())	
		{
			if (iterator.GetData()->IsNPC()) {
				npc = iterator.GetData()->CastToNPC();
				if (spawn2_dump != 0)
					npc_dump[NPCindex].spawn2_dump_index = zone->DumpSpawn2(spawn2_dump, &spawn2index, npc->respawn2);
				npc_dump[NPCindex].npctype_id = npc->GetNPCTypeID();
				npc_dump[NPCindex].cur_hp = npc->GetHP();
				if (npc->IsCorpse()) {
					if (npc->CastToCorpse()->IsLocked())
						npc_dump[NPCindex].corpse = 2;
					else
						npc_dump[NPCindex].corpse = 1;
				}
				else
					npc_dump[NPCindex].corpse = 0;
				//				if (npc_dump[NPCindex].corpse) {
				//					npc_dump[NPCindex].decay_time_left = npc->GetDecayTime();
				//				}
				//				else {
				npc_dump[NPCindex].decay_time_left = 0xFFFFFFFF;
				//				}
				npc_dump[NPCindex].x = npc->GetX();
				npc_dump[NPCindex].y = npc->GetY();
				npc_dump[NPCindex].z = npc->GetZ();
				npc_dump[NPCindex].heading = npc->GetHeading();
				npc_dump[NPCindex].copper = npc->copper;
				npc_dump[NPCindex].silver = npc->silver;
				npc_dump[NPCindex].gold = npc->gold;
				npc_dump[NPCindex].platinum = npc->platinum;
				if (npcloot_dump != 0)
					npc->DumpLoot(NPCindex, npcloot_dump, &NPCLootindex);
				if (gmspawntype_dump != 0) {
					if (npc->GetNPCTypeID() == 0) {
						memcpy(&gmspawntype_dump[gmspawntype_index], npc->NPCTypedata, sizeof(NPCType));
						npc_dump[NPCindex].gmspawntype_index = gmspawntype_index;
						gmspawntype_index++;
					}
				}
				NPCindex++;
			}
			iterator.Advance();
		}
	}
	if (spawn2_dump != 0)
		zone->DumpAllSpawn2(spawn2_dump, &spawn2index);
}

void EntityList::Depop() {
	LinkedListIterator<Entity*> iterator(list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.GetData()->Depop(false);
		iterator.Advance();
	}
}

/*
Bulk zone spawn packet, to be requested by the client on zone in
Does 50 updates per packet, can fine tune that if neccessary

  -Quagmire
*/
void EntityList::SendZoneSpawnsBulk(Client* client) {
	float rate = client->Connection()->GetDataRate();
	LinkedListIterator<Entity*> iterator(list);
	NewSpawn_Struct ns;
	int32 maxspawns;
    
    //
    // Xorlac: This error checking really belongs in set data rate, but here to fix bulk spawns for now
    //

    if (rate < 1.0)
    {
        rate = 1.0;
    }
    
    if (rate > 10.0)
    {
        rate = 10.0;
    }
    
    maxspawns = (int32)rate * SPAWNS_PER_POINT_DATARATE; // FYI > 10240 entities will cause BulkZoneSpawnPacket to throw exception

//	printf("MaxRate: %i\n",maxspawns);
	BulkZoneSpawnPacket* bzsp = new BulkZoneSpawnPacket(client, maxspawns);
	int32 packetsize = 0;
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob()) {
			if (!iterator.GetData()->CastToMob()->InZone() || (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->GMHideMe(client))) {
				iterator.Advance();
				continue;
			}
			memset(&ns, 0, sizeof(NewSpawn_Struct));
			iterator.GetData()->CastToMob()->FillSpawnStruct(&ns, client);
			bzsp->AddSpawn(&ns);
			packetsize += DeflatePacket((unsigned char*)&ns, sizeof(NewSpawn_Struct), (unsigned char*)&ns, 10000);
			if(packetsize/3 >= maxspawns)
			{
				packetsize = 0;
				bzsp->SendBuffer();
			}
		}
		iterator.Advance();
	}
	safe_delete(bzsp);
}

#define MAX_SPAWN_UPDATES_PER_PACKET	30
/*
Bulk spawn update packet, to be requested by the client on a timer.
0 range = whole zone.
Does 50 updates per packet, can fine tune that if neccessary

  -Quagmire
*/
void EntityList::SendPositionUpdates(Client* client, int32 cLastUpdate, float range, Entity* alwayssend, bool iSendEvenIfNotChanged) {

	range = range * range;
	LinkedListIterator<Entity*> iterator(list);
	
	APPLAYER* outapp = 0;
	SpawnPositionUpdates_Struct* spus = 0;
	Mob* mob = 0;
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (outapp == 0) {
			outapp = new APPLAYER(OP_MobUpdate, sizeof(SpawnPositionUpdates_Struct) + (sizeof(SpawnPositionUpdate_Struct) * MAX_SPAWN_UPDATES_PER_PACKET));
			spus = (SpawnPositionUpdates_Struct*) outapp->pBuffer;
		}
		if (iterator.GetData()->IsMob()) {
			mob = iterator.GetData()->CastToMob();
			if (!mob->IsCorpse() 
				&& iterator.GetData() != client 
				&& (mob->IsClient() || iSendEvenIfNotChanged || mob->LastChange() >= cLastUpdate) 
				&& (!iterator.GetData()->IsClient() || !iterator.GetData()->CastToClient()->GMHideMe(client))
				) {
				if (range == 0 || iterator.GetData() == alwayssend || mob->DistNoRootNoZ(client) <= range) {
					mob->MakeSpawnUpdate(&spus->spawn_update[spus->num_updates]);
					spus->num_updates++;
				}
			}
		}
		if (spus->num_updates >= MAX_SPAWN_UPDATES_PER_PACKET) {
            client->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
			delete outapp;
			outapp = 0;
		}
		iterator.Advance();
	}
	if (outapp != 0) {
		if (spus->num_updates > 0) {
			outapp->size = sizeof(SpawnPositionUpdates_Struct) + (sizeof(SpawnPositionUpdate_Struct) * spus->num_updates);
            client->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
		}
		delete outapp;
		outapp = 0;
	}
}

char* EntityList::MakeNameUnique(char* name) {
	bool used[100];
	memset(used, 0, sizeof(used));
	name[61] = 0; name[62] = 0; name[63] = 0;

	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	int len = strlen(name);
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob()) {
//			len = strlen(iterator.GetData()->CastToMob()->GetName());
			if (strncasecmp(iterator.GetData()->CastToMob()->GetName(), name, len) == 0) {
				if (Seperator::IsNumber(&iterator.GetData()->CastToMob()->GetName()[len])) {
					used[atoi(&iterator.GetData()->CastToMob()->GetName()[len])] = true;
				}
			}
		}
		iterator.Advance();
	}
	for (int i=0; i < 100; i++) {
		if (!used[i]) {
			#ifdef WIN32
			snprintf(name, 64, "%s%02d", name, i);
			#else
			//glibc clears destination of snprintf
			//make a copy of name before snprintf--misanthropicfiend
			char temp_name[64];
			strn0cpy(temp_name, name, 64);
			snprintf(name, 64, "%s%02d", temp_name, i);
			#endif
			return name;
		}
	}
	LogFile->write(EQEMuLog::Error, "Fatal error in EntityList::MakeNameUnique: Unable to find unique name for '%s'", name);
	char tmp[64] = "!";
	strn0cpy(&tmp[1], name, sizeof(tmp) - 1);
	strcpy(name, tmp);
	return MakeNameUnique(name);
}

char* EntityList::RemoveNumbers(char* name) {
	char	tmp[64];
	memset(tmp, 0, sizeof(tmp));
	int k = 0;
	for (unsigned int i=0; i<strlen(name) && i<sizeof(tmp); i++) {
		if (name[i] < '0' || name[i] > '9')
			tmp[k++] = name[i];
	}
	strn0cpy(name, tmp, sizeof(tmp));
	return name;
}
Mob* EntityList::FindDefenseNPC(int32 npcid)
{
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements()) {
		if(iterator.GetData()->IsNPC() && iterator.GetData()->CastToNPC()->GetGuildOwner() != 0 && !iterator.GetData()->CastToNPC()->IsCityController()) {
				return iterator.GetData()->CastToMob();
		}
		iterator.Advance();
	}
	return false;
}

void EntityList::ListNPCs(Client* client, const char* arg1, const char* arg2, int8 searchtype) {
	if (arg1 == 0)
		searchtype = 0;
	else if (arg2 == 0 && searchtype >= 2)
		searchtype = 0;
	LinkedListIterator<Entity*> iterator(list);
	int32 x = 0;
	int32 z = 0;
	char sName[36];
	
	iterator.Reset();
	client->Message(0, "NPCs in the zone:");
	if(searchtype == 0) {
		while(iterator.MoreElements()) {
			if(iterator.GetData()->IsNPC()) {
				client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
				x++;
				z++;
			}
			iterator.Advance();
		}
	}
	else if(searchtype == 1) {
		client->Message(0, "Searching by name method. (%s)",arg1);
		char* tmp = new char[strlen(arg1) + 1];
		strcpy(tmp, arg1);
		strupr(tmp);
		while(iterator.MoreElements()) {
			if(iterator.GetData()->IsNPC()) {
				z++;
				strcpy(sName, iterator.GetData()->GetName());
				strupr(sName);
				if (strstr(sName, tmp)) {
					client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
					x++;
				}
			}
			iterator.Advance();
		}
		delete tmp;
	}
	else if(searchtype == 2) {
		client->Message(0, "Searching by number method. (%s %s)",arg1,arg2);
		while(iterator.MoreElements()) {
			if(iterator.GetData()->IsNPC()) {
				z++;
				if ((iterator.GetData()->GetID() >= atoi(arg1)) && (iterator.GetData()->GetID() <= atoi(arg2)) && (atoi(arg1) <= atoi(arg2))) {
					client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
					x++;
				}
			}
			iterator.Advance();
		}
	}
	client->Message(0, "%d npcs listed. There is a total of %d npcs in this zone.", x, z);
}

void EntityList::ListNPCCorpses(Client* client) {
	LinkedListIterator<Entity*> iterator(list);
	int32 x = 0;
	
	iterator.Reset();
	client->Message(0, "NPC Corpses in the zone:");
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsNPCCorpse()) {
			client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
			x++;
		}
		iterator.Advance();
	}
	client->Message(0, "%d npc corpses listed.", x);
}

void EntityList::ListPlayerCorpses(Client* client) {
	LinkedListIterator<Entity*> iterator(list);
	int32 x = 0;
	
	iterator.Reset();
	client->Message(0, "Player Corpses in the zone:");
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsPlayerCorpse()) {
			client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
			x++;
		}
		iterator.Advance();
	}
	client->Message(0, "%d player corpses listed.", x);
}

// returns the number of corpses deleted. A negative number indicates an error code.
sint32 EntityList::DeleteNPCCorpses() {
	LinkedListIterator<Entity*> iterator(list);
	sint32 x = 0;
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsNPCCorpse()) {
			iterator.GetData()->Depop();
			x++;
		}
		iterator.Advance();
	}
	return x;
}

// returns the number of corpses deleted. A negative number indicates an error code.
sint32 EntityList::DeletePlayerCorpses() {
	LinkedListIterator<Entity*> iterator(list);
	sint32 x = 0;
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsPlayerCorpse()) {
			iterator.GetData()->CastToCorpse()->Delete();
			x++;
		}
		iterator.Advance();
	}
	return x;
}

void EntityList::SendPetitionToAdmins(Petition* pet) {
	LinkedListIterator<Entity*> iterator(list);
	
	APPLAYER* outapp = new APPLAYER(0x0f20,sizeof(PetitionClientUpdate_Struct));
	PetitionClientUpdate_Struct* pcus = (PetitionClientUpdate_Struct*) outapp->pBuffer;
	pcus->petnumber = pet->GetID();		// Petition Number
	if (pet->CheckedOut()) {
		pcus->color = 0x00;
		pcus->status = 0xFFFFFFFF;
		pcus->senttime = pet->GetSentTime();
		strcpy(pcus->accountid, "");
		strcpy(pcus->gmsenttoo, "");
	}
	else {
		pcus->color = pet->GetUrgency();	// 0x00 = green, 0x01 = yellow, 0x02 = red
		//pcus->unknown2 = 0x00;
		pcus->status = pet->GetSentTime();
		pcus->senttime = pet->GetSentTime();			// 4 has to be 0x1F
		strcpy(pcus->accountid, pet->GetAccountName());
		strcpy(pcus->charname, pet->GetCharName());
	}
	/*pcus->unknown[2] = 0x00;
	pcus->unknown[1] = 0x00;
	pcus->unknown[0] = 0x00;*/
	//This is wrong... But VI seems to use this as well.
	pcus->quetotal = petition_list.GetMaxPetitionID();
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->Admin() >= 100) {
			if (pet->CheckedOut())
				strcpy(pcus->gmsenttoo, "");
			else
				strcpy(pcus->gmsenttoo, iterator.GetData()->CastToClient()->GetName());
			//			DumpPacket(outapp);
			iterator.GetData()->CastToClient()->QueuePacket(outapp);
		}
		iterator.Advance();
	}
	delete outapp;
}

void EntityList::ClearClientPetitionQueue() {
	APPLAYER* outapp = new APPLAYER(0x0f20,sizeof(PetitionClientUpdate_Struct));
	PetitionClientUpdate_Struct* pet = (PetitionClientUpdate_Struct*) outapp->pBuffer;
	pet->color = 0x00;
	pet->status = 0xFFFFFFFF;
	/*pet->unknown[3] = 0x00;
	pet->unknown[2] = 0x00;
	pet->unknown[1] = 0x00;
	pet->unknown[0] = 0x00;*/
	pet->senttime = 0;
	strcpy(pet->accountid, "");
	strcpy(pet->gmsenttoo, "");
	strcpy(pet->charname, "");
	pet->quetotal = petition_list.GetMaxPetitionID();
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->Admin() >= 100) {
			int x = 0;
			for (x=0;x<64;x++) {
				pet->petnumber = x;
				iterator.GetData()->CastToClient()->QueuePacket(outapp);
			}
		}
		iterator.Advance();
	}
	delete outapp;
	return;
}

void EntityList::WriteEntityIDs() {
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsMob()) {
			cout << "ID: " << iterator.GetData()->GetID() << "  Name: " << iterator.GetData()->GetName() << endl;
		}
		iterator.Advance();
	}
}

BulkZoneSpawnPacket::BulkZoneSpawnPacket(Client* iSendTo, int32 iMaxSpawnsPerPacket) {
	data = 0;
	pSendTo = iSendTo;
	pMaxSpawnsPerPacket = iMaxSpawnsPerPacket;
#ifdef _DEBUG
	if (pMaxSpawnsPerPacket <= 0 || pMaxSpawnsPerPacket > MAX_SPAWNS_PER_PACKET) {
		// ok, this *cant* be right =p
		ThrowError("Error in BulkZoneSpawnPacket::BulkZoneSpawnPacket(): pMaxSpawnsPerPacket outside range that makes sense");
	}
#endif
}

BulkZoneSpawnPacket::~BulkZoneSpawnPacket() {
	SendBuffer();
	safe_delete(data)
}

bool BulkZoneSpawnPacket::AddSpawn(NewSpawn_Struct* ns) {
	if (!data) {
		data = new NewSpawn_Struct[pMaxSpawnsPerPacket];
		memset(data, 0, sizeof(NewSpawn_Struct) * pMaxSpawnsPerPacket);
		index = 0;
	}
	memcpy(&data[index], ns, sizeof(NewSpawn_Struct));
	index++;
	if (index >= pMaxSpawnsPerPacket) {
		SendBuffer();
		return true;
	}
	return false;
}

void BulkZoneSpawnPacket::SendBuffer() {
	if (!data)
		return;
	int32 tmpBufSize = (sizeof(NewSpawn_Struct) * pMaxSpawnsPerPacket) + 50;
	APPLAYER* outapp = new APPLAYER(OP_ZoneSpawns, tmpBufSize);
	outapp->size = DeflatePacket((int8*) data, index * sizeof(NewSpawn_Struct), outapp->pBuffer, tmpBufSize);
	EncryptZoneSpawnPacket(outapp);
	//DumpPacket(outapp);
	if (pSendTo)
		pSendTo->QueuePacket(outapp);
	else
		entity_list.QueueClients(0, outapp);
	safe_delete(outapp);
	memset(data, 0, sizeof(NewSpawn_Struct) * pMaxSpawnsPerPacket);
	index = 0;
}

void EntityList::DoubleAggro(Mob* who)
{
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->CastToMob()->IsNPC() && iterator.GetData()->CastToNPC()->CheckAggro(who))
		  iterator.GetData()->CastToNPC()->SetHate(who,iterator.GetData()->CastToNPC()->GetHateAmount(who),iterator.GetData()->CastToNPC()->GetHateAmount(who)*2);
		iterator.Advance();
	}
}

void EntityList::HalveAggro(Mob* who)
{
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->CastToMob()->IsNPC() && iterator.GetData()->CastToNPC()->CheckAggro(who))
		  iterator.GetData()->CastToNPC()->SetHate(who,iterator.GetData()->CastToNPC()->GetHateAmount(who)/2);
		iterator.Advance();
	}
}

void EntityList::ClearFeignAggro(Mob* targ)
{
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
#ifdef DEBUG
		cout<<"Checking aggro"<<endl;
#endif
		if (iterator.GetData()->IsNPC() && iterator.GetData()->CastToNPC()->CheckAggro(targ))
		{
			iterator.GetData()->CastToNPC()->RemoveFromHateList(targ);
			if (iterator.GetData()->CastToMob()->GetLevel() >= 35)
			//	cout<<"Clearing aggro"<<endl;
				iterator.GetData()->CastToNPC()->SetFeignMemory(targ->CastToMob()->GetName());
		}
		iterator.Advance();
	}
}
