#include <logger.h>
#include <cstdlib>
#include <cstdio>
#include <stdarg.h>
#include <ctype.h>
#include <cstring>
#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "entity.h"
#include "entitylist.h"
#include "client.h"
#include "npc.h"
#include "worldserver.h"
#include "PlayerCorpse.h"
#include "packet_functions.h"
#include "ClientList.h"
#include "petitions.h"
#include "EQCException.hpp"
#include "EQCUtils.hpp"
#include "config.h"
#include "groups.h"
#include "object.h"
#include "ZoneGuildManager.h"
#include "projectile.h"

#ifdef EMBPERL
	#include "embparser.h"
#endif

using namespace std;
using namespace EQC::Zone;

extern ZoneGuildManager zgm;
extern Zone* zone;
extern volatile bool ZoneLoaded;
extern WorldServer worldserver;

extern ClientList client_list;
extern int32 numclients;

void EntityList::AddClient(Client* client)
{
	client->SetID(GetFreeID());
	list.Insert(client);
}

void EntityList::AddCorpse(Corpse* corpse, int32 in_id)
{
	if(corpse == 0)
	{
		EQC_EXCEPT("void EntityList::AddCorpse(Corpse* corpse, int32 in_id)", "Adding null pointer to entity list.");
		return;
	}

	if(in_id == 0xFFFFFFFF)
	{
		corpse->SetID(GetFreeID());
		if(corpse->GetDBID() == 0)
			corpse->SetDBID(GetFreeID());
	}
	else
	{
		corpse->SetID(in_id);
		if(corpse->GetDBID() == 0)
			corpse->SetDBID(in_id);
	}

	corpse->CalcCorpseName();

	list.Insert(corpse);
}

void EntityList::AddNPC(NPC* npc, bool SendSpawnPacket)
{
	npc->SetID(GetFreeID());

	//Yeahlight: Spawn has been flagged to send a packet and is not blocked from doing so
	if(SendSpawnPacket && !zone->GetBulkOnly())
	{
		APPLAYER app;
		npc->CreateSpawnPacket(&app);
		QueueClients(npc, &app);		

		// Harakiri notify event
		#ifdef EMBPERL	
			perlParser->Event(EVENT_SPAWN, npc->GetNPCTypeID(), 0, npc, NULL);				
		#endif
	}
	
	list.Insert(npc);	
}	

int EntityList::GetNPCTypeCount(int NPCTypeID)
{
	int npc_count = 0;

	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsMob() && iterator.GetData()->CastToMob()->GetNPCTypeID() == NPCTypeID )
		{
			npc_count++;
		}
		iterator.Advance();
	}

	return npc_count;
}

Entity* EntityList::GetID(int16 get_id)
{
	if(get_id == 0)
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

Mob* EntityList::GetMob(char* name)
{
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
	//Yeahlight: Iterate through the possible list of IDs
	for(int i = 1; i < MAX_ENTITY_LIST_IDS; i++)
	{
		//Yeahlight: This ID is free; use it
		if(takenIDs[i] == 0)
		{
			return i;
		}
	}
	return 0;
	//while(1)
	//{
	//	last_insert_id++;
	//	if (last_insert_id == 0)
	//		last_insert_id++;
	//	if (GetID(last_insert_id) == 0)
	//	{
	//		return last_insert_id;
	//	}
	//}
}

void EntityList::ChannelMessage(Mob* from, int8 chan_num, int8 language, char* message, ...)
{
	LinkedListIterator<Entity*> iterator(list);
	va_list argptr;
	// Harakiri NPC quest text can be quite long!
	char buffer[4096];

	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);

	//cout << "Message:" << buffer << endl;

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient())
		{
			Client* client = iterator.GetData()->CastToClient();
			if (chan_num != MessageChannel_SAY || client->Dist(from) < 100) // Only say is limited in range
			{
				client->ChannelMessageSend( from->IsNPC() ? from->CastToNPC()->GetProperName() :  from->GetName(), 0, chan_num, language, buffer);

				// Harakiri skillups of language
				if(from->IsClient() ) {
					from->CastToClient()->TeachLanguage(client,language,chan_num);
				}

			}
		}
		iterator.Advance();
	}
}

void EntityList::ChannelMessageSend(Mob* to, int8 chan_num, int8 language, char* message, ...)
{
	LinkedListIterator<Entity*> iterator(list);
	va_list argptr;
	char buffer[256];

	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);

	cout << "Message:" << buffer << endl;

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
/*
void EntityList::SendZoneSpawns(Client* client)
{
	LinkedListIterator<Entity*> iterator(list);
	va_list argptr;
	char buffer[256];

	APPLAYER* app;

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsMob())
		{
			if (iterator.GetData()->IsClient() && (!iterator.GetData()->CastToClient()->Connected() || iterator.GetData()->CastToClient()->GMHideMe(client)))
			{
				iterator.Advance();
				continue;
			}
			app = new APPLAYER;
			if (iterator.GetData()->IsMob())
			{
				iterator.GetData()->CastToMob()->CreateSpawnPacket(app); // TODO: Use zonespawns opcode instead
				client->QueuePacket(app);
			}
			
			safe_delete(app);//delete app;
		}
		iterator.Advance();
	}
}*/

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

void EntityList::RemoveFromTargets(Mob* mob)
{
	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsNPC())
		{
			iterator.GetData()->CastToNPC()->RemoveFromHateList(mob);
		}
		if (iterator.GetData()->IsMob())
		{
			if(iterator.GetData()->CastToMob()->GetTarget() == mob){
				iterator.GetData()->CastToMob()->SetTarget(0);	
			}
		}
		iterator.Advance();
	}	
}

void EntityList::QueueCloseClients(Mob* sender, APPLAYER* app, bool ignore_sender, float dist, Mob* SkipThisMob, bool CheckLoS) {
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
				//Yeahlight: We never send the owner of an eye of zomm the update packets
				if(iterator.GetData()->CastToClient()->GetMyEyeOfZommID() != sender->GetID()) {
					if(!CheckLoS)
						iterator.GetData()->CastToClient()->QueuePacket(app);
					else if(iterator.GetData()->CastToClient()->CheckLosFN(sender))
						iterator.GetData()->CastToClient()->QueuePacket(app);
				}
			}
		}
		iterator.Advance();
	}	
}

void EntityList::QueueFarClients(Mob* sender, APPLAYER* app, bool ignore_sender, float dist, Mob* SkipThisMob) {
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
			if(iterator.GetData()->CastToClient()->DistNoRoot(sender) > dist2 && iterator.GetData()->CastToClient()->DistNoRoot(sender) <= (dist2 * 2)) {
				iterator.GetData()->CastToClient()->QueuePacket(app);
			}
		}
		iterator.Advance();
	}	
}

void EntityList::QueueClients(Mob* sender, APPLAYER* app, bool ignore_sender)
{
	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient() && (!ignore_sender || iterator.GetData() != sender))
		{
			iterator.GetData()->CastToClient()->QueuePacket(app);
		}
		iterator.Advance();
	}	
}


void EntityList::QueueClientsStatus(Mob* sender, APPLAYER* app, bool ignore_sender, int8 minstatus, int8 maxstatus)
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

//o--------------------------------------------------------------
//| AESpell; Yeahlight, July 09, 2009
//o--------------------------------------------------------------
//| Determines which entities are hit by AE spells
//o--------------------------------------------------------------
void EntityList::AESpell(Mob* caster, float x, float y, float z, float dist, Spell* spell)
{
	//Yeahlight: Caster or spell was not supplied; abort
	if(caster == NULL || spell == NULL)
		return;

	int16 limit = 9999;
	bool skipTarget = false;
	TTargetType targetType = spell->GetSpellTargetType();
	Mob* castersTarget = caster->GetTarget();

	//Yeahlight: AE spells have a limit of four targets, while PBAoE (ST_AECaster) spells hit unlimited targets
	if(targetType == ST_AETarget)
		limit = 4;
	//Yeahlight: The PC caster is always guaranteed that their target will be picked during this process
	if(castersTarget && castersTarget->IsMob() && caster->IsClient() && castersTarget != caster && targetType == ST_AETarget && caster->CastToClient()->CanAttackTarget(castersTarget))
	{
		//Yeahlight: Target is in range and LOS to be affected by the spell
		if(castersTarget->CastToMob()->DistanceToLocation(x, y, z) <= dist && castersTarget->CastToMob()->CheckCoordLos(castersTarget->GetX(), castersTarget->GetY(), castersTarget->GetZ(), x, y, z))
		{
			caster->SpellOnTarget(spell, castersTarget);
			limit--;
			skipTarget = true;
		}
	}
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	//Yeahlight: The caster may die during this loop, so we need to check for it on each iteration
	while(iterator.MoreElements() && caster && limit > 0)
	{
		if(iterator.GetData()->IsMob())
		{
			Mob* m = iterator.GetData()->CastToMob();
			//Yeahlight: Entity exists in the zone
			if(m)
			{
				//Yeahlight: We are examining the main target of the spell and it has not already taken this spell effect
				if(!(m == castersTarget && skipTarget))
				{
					//Yeahlight: The caster will never target itself with a PBAE spell
					if(!(m == caster && targetType == ST_AECaster))
					{
						//Yeahlight: Caster is an NPC
						if(caster->IsNPC())
						{
							//Yeahlight: TODO: Fill this out
						}
						//Yeahlight: Caster is a PC
						else if(caster->IsClient())
						{
							//Yeahlight: Entity exists, is a mob, is within range of the AE spell
							if(m->DistanceToLocation(x, y, z) <= dist)
							{
								//Yeahlight: Caster may attack its target
								if(m == caster || caster->CastToClient()->CanAttackTarget(m))
								{
									//Yeahlight: Entity must have line of sight to the origin of damage to be affected by this spell
									if(m->CheckCoordLos(m->GetX(), m->GetY(), m->GetZ(), x, y, z))
									{
										caster->SpellOnTarget(spell, m);
										limit--;
									}
								}
							}
						}
					}
				}
			}
		}
		iterator.Advance();
	}
}

Client* EntityList::GetClientByName(const char *checkname) 
{ 
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

void EntityList::ChannelMessageFromWorld(char* from, char* to, int8 chan_num, int32 guilddbid, int8 language, char* message, ...) {
	//-Cofruben: attempt to do this more secure
	if (!message) EQC_EXCEPT("void EntityList::ChannelMessageFromWorld()", "message = NULL");
	va_list argptr;
	char buffer[256];

	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);

	LinkedListIterator<Entity*> iterator(list);

	cout << "Message: " << (int) chan_num << " " << (int) guilddbid << ": " << buffer << endl;

	iterator.Reset();
	
	zgm.LoadGuilds();
	Guild_Struct* guilds = zgm.GetGuildList();


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

void EntityList::Message(int32 to_guilddbid, MessageFormat type, char* message, ...) {
	//-Cofruben: attempt to secure this.
	if (!message) EQC_EXCEPT("void Console::Message()", "message = NULL");
	va_list argptr;
	char buffer[256];

	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);

	LinkedListIterator<Entity*> iterator(list);

	cout << "Message:" << buffer << endl;

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

void EntityList::MessageClose(Mob* sender, bool skipsender, float dist, MessageFormat type, char* message, ...) {
	//-Cofruben: attempt to do this more secure
	if (!message) EQC_EXCEPT("void EntityList::MessageClose()", "message = NULL");
	va_list argptr;
	char buffer[256];

	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);

	LinkedListIterator<Entity*> iterator(list);

	cout << "Message:" << buffer << endl;

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient()) {
			float real_dist = iterator.GetData()->CastToClient()->DistNoRoot(sender);
			if (real_dist <= dist && (!skipsender || iterator.GetData() != sender)) {
				iterator.GetData()->CastToClient()->Message(type, buffer);
			}
		}
		iterator.Advance();
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
		if (iterator.GetData()->IsClient()){
			client_list.Remove(iterator.GetData()->CastToClient());
		}
		if (iterator.GetData()->IsMob()){
			entity_list.RemoveFromTargets(iterator.GetData()->CastToMob());
		}
		iterator.RemoveCurrent();
	}	
	last_insert_id = 0;
}

void EntityList::UpdateWho()
{
	if ((!worldserver.Connected()) || !ZoneLoaded)
		return;
	ServerPacket* pack = new ServerPacket(ServerOP_ClientList, sizeof(ServerClientList_Struct));
	
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	ServerClientList_Struct* scl = (ServerClientList_Struct*) pack->pBuffer;
	scl->remove = true;
	strcpy(scl->zone, zone->GetShortName());
	worldserver.SendPacket(pack);
	safe_delete(pack);//delete pack;

	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient())
		{
			iterator.GetData()->CastToClient()->UpdateWho(false);
		}
		iterator.Advance();
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
			if (iterator.GetData()->IsClient()){
				takenIDs[iterator.GetData()->GetID()] = 0;
				client_list.Remove(iterator.GetData()->CastToClient());
			}
			if (iterator.GetData()->IsMob()){
				takenIDs[iterator.GetData()->GetID()] = 0;
				entity_list.RemoveFromTargets(iterator.GetData()->CastToMob());
			}
			iterator.RemoveCurrent();
			return;
		}
		iterator.Advance();
	}

	cout << "error deleting spawn" << endl;
}

/** Cycles through all entities in list and calls their Process() functions. */
void EntityList::Process()
{
	LinkedListIterator<Entity*> iterator(list);

	bool benchmark = true;
	static clock_t start;
	clock_t finish;
	double duration;
	static int32 entityCounter = 0;

	//Yeahlight: Start the benchmark timestamp
	if(benchmark && entityCounter == 0)
		start = clock();

	iterator.Reset();
	while(iterator.MoreElements())
	{
		entityCounter++;
		bool result = false;

		try {
			Entity* current = iterator.GetData();
			if (current)
				result = current->Process();
		}

		catch (EQC::Common::EQCException Exception) {
			if(iterator.GetData()->IsClient()){
				struct in_addr  in;
				in.s_addr = iterator.GetData()->CastToClient()->GetIP();
				cout << "-EQC EXCEPTION caught from user  " << inet_ntoa(in) << endl;
				Exception.FileDumpException("Zone_Exceptions.txt");
				client_list.Remove(iterator.GetData()->CastToClient());
				zone->StartShutdownTimer();
				numclients--;
			}
		}
		catch (...) { //Better than crashing..
			if(iterator.GetData()->IsClient()){
				struct in_addr  in;
				in.s_addr = iterator.GetData()->CastToClient()->GetIP();
				cout << "-EXCEPTION caught from user  " << iterator.GetData()->GetName() << endl;
				client_list.Remove(iterator.GetData()->CastToClient());
				zone->StartShutdownTimer();
				numclients--;
			}
		}

		if (!result)
		{
			if (iterator.GetData()->IsClient()) {
				struct in_addr	in;
				in.s_addr = iterator.GetData()->CastToClient()->GetIP();
				EQC::Common::PrintF(CP_CLIENT, "Dropping client from ip %s:%i\n", inet_ntoa(in), ntohs(iterator.GetData()->CastToClient()->GetPort()));
				Client* tempClient = iterator.GetData()->CastToClient();
				//Removes player from group if he changes zones, will need to fix this.

				/*
				if(tempClient->IsGrouped()){
					entity_list.GetGroupByClient(tempClient)->RemoveMember(tempClient);
				}
				*/
				client_list.Remove(iterator.GetData()->CastToClient());
				zone->StartShutdownTimer();
				numclients--;
				//Yeahlight: Log the current account out of the active_account table
				Database::Instance()->LogAccountOut(iterator.GetData()->CastToClient()->GetAccountID());
				iterator.RemoveCurrent();
			}
			/* This mob is dead and has been replaced with corpse, remove mob entity because he will *
			* get added again from his respawn function.											 */
			else if (iterator.GetData()->IsMob()){
				entity_list.RemoveFromTargets(iterator.GetData()->CastToMob());
				iterator.RemoveCurrent();
			}
			else if (iterator.GetData()->IsObject()){
				//entity_list.RemoveFromTargets(iterator.GetData()->CastToObject());
				iterator.RemoveCurrent();
			}
			else if (iterator.GetData()->IsProjectile()){
				iterator.RemoveCurrent();
			}
		}
		else
		{
			iterator.Advance();
		}
	}

	if(benchmark)
	{
		finish = clock();
		duration = (double)(finish - start);
		double pass = duration / (double)(entityCounter);
		zone->SetAverageProcessTime(pass);
	}
}
//o--------------------------------------------------------------
//| BoatProcess Tazadar, June 30, 2009
//o--------------------------------------------------------------
//| Run only the boat process when no players are in the zone
//o--------------------------------------------------------------

void EntityList::BoatProcessOnly()
{
	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		Entity* current = iterator.GetData();
		if (current && current->IsNPC() && current->CastToNPC()->IsBoat()){
				current->Process();
		}
		iterator.Advance();
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
				npc_dump[NPCindex].corpse = npc->IsCorpse();
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
		if ( iterator.GetData()->IsClient() || iterator.GetData()->IsGroup() || iterator.GetData()->IsPlayerCorpse() )
			iterator.Advance();
		else
			iterator.RemoveCurrent();
	}
}

void EntityList::DepopCorpses() {
	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsNPCCorpse()) {
			iterator.GetData()->Depop();
		}
		iterator.Advance();
	}
}

//Yeahlight: Let's see if 100 causes any issues
#define MAX_SPAWNS_PER_PACKET	100
/*
Bulk zone spawn packet, to be requested by the client on zone in
Does 50 updates per packet, can fine tune that if neccessary

-Quagmire
*/

void EntityList::SendZoneSpawnsBulk(Client* client)
{
	LinkedListIterator<Entity*> iterator(list);
	va_list argptr;
	char buffer[256];

	uchar *data = 0, *newdata = 0;
	int16 datasize = 0, newdatasize = 0;
	NewSpawn_Struct* bns = 0;
	int i = 0;
	APPLAYER* outapp;
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsMob())
		{
			if (iterator.GetData()->IsClient() && (!iterator.GetData()->CastToClient()->Connected() || iterator.GetData()->CastToClient()->GMHideMe(client))) {
				iterator.Advance();
				continue;
			}
			if (datasize < (sizeof(NewSpawn_Struct) * (i+1))) {
				// Step 100 spawns every time we run out of buffer space
				newdatasize = datasize + (MAX_SPAWNS_PER_PACKET * sizeof(NewSpawn_Struct));
				newdata = new uchar[newdatasize];
				memset(newdata, 0, newdatasize);
				if (data != 0) {
					memcpy(newdata, data, datasize);
					delete data;
				}
				data = newdata;
				datasize = newdatasize;
				newdata = 0;
				bns = (NewSpawn_Struct*) data;
			}
			iterator.GetData()->CastToMob()->FillSpawnStruct(&bns[i], client);
			i++;
			if (i == MAX_SPAWNS_PER_PACKET) {
				outapp = new APPLAYER;
				outapp->opcode = OP_ZoneSpawns;
				outapp->pBuffer = new uchar[datasize];
				outapp->size = DeflatePacket(data, i * sizeof(NewSpawn_Struct), outapp->pBuffer, datasize);
				EncryptZoneSpawnPacket(outapp);
				//DumpPacket(outapp);
				client->QueuePacket(outapp);
				delete outapp;
				delete data;
				data = 0;
				datasize = 0;
				i = 0;
			}
		}
		iterator.Advance();
	}
	if (i > 0) {
		outapp = new APPLAYER;
		outapp->opcode = OP_ZoneSpawns;
		outapp->pBuffer = new uchar[datasize];
		//DumpPacket(data, i * sizeof(NewSpawn_Struct));
		outapp->size = DeflatePacket(data, i * sizeof(NewSpawn_Struct), outapp->pBuffer, datasize);
		EncryptZoneSpawnPacket(outapp);
		//DumpPacket(outapp);
		client->QueuePacket(outapp);
		delete outapp;
		delete data;
	}
}

#define MAX_SPAWN_UPDATES_PER_PACKET	25
/*
Bulk spawn update packet, to be requested by the client on a timer.
0 range = whole zone.
Does 50 updates per packet, can fine tune that if neccessary

-Quagmire
*/
/*
void EntityList::SendPositionUpdates(Client* client, int32 cLastUpdate, float range, Entity* alwayssend, bool iSendEvenIfNotChanged) {
range = range * range;
LinkedListIterator<Entity*> iterator(list);

APPLAYER* outapp = 0;
SpawnPositionUpdates_Struct* spus = 0;
Mob* mob = 0;

iterator.Reset();
while(iterator.MoreElements())
{
if (outapp == 0) {
outapp = new APPLAYER;
outapp->opcode = OP_MobUpdate;
outapp->size = sizeof(SpawnPositionUpdates_Struct) + (sizeof(SpawnPositionUpdate_Struct) * MAX_SPAWN_UPDATES_PER_PACKET);
outapp->pBuffer = new uchar[outapp->size];
memset(outapp->pBuffer, 0, outapp->size);
spus = (SpawnPositionUpdates_Struct*) outapp->pBuffer;
}
if (iterator.GetData()->IsMob()) {
mob = iterator.GetData()->CastToMob();
if (!mob->IsCorpse() && iterator.GetData() != client && (mob->IsClient() || mob->GetLastChange() >= cLastUpdate)){ //&& (!iterator.GetData()->IsClient() || !iterator.GetData()->CastToClient()->GMHideMe(client))) {
if (range == 0 || iterator.GetData() == alwayssend || mob->DistNoRootNoZ(client) <= range) {
mob->MakeSpawnUpdate(&spus->spawn_update[spus->num_updates]);
spus->num_updates++;
}
}
}
if (spus->num_updates >= MAX_SPAWN_UPDATES_PER_PACKET) {
client->QueuePacket(outapp, false);
safe_delete(outapp);//delete outapp;
outapp = 0;
}
iterator.Advance();
}
if (outapp != 0) {
if (spus->num_updates > 0) {
outapp->size = sizeof(SpawnPositionUpdates_Struct) + (sizeof(SpawnPositionUpdate_Struct) * spus->num_updates);
client->QueuePacket(outapp, false);
}
safe_delete(outapp);//delete outapp;
outapp = 0;
}
}

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
				&& (mob->IsClient() || iSendEvenIfNotChanged || mob->GetLastChange() >= cLastUpdate) 
				&& (!iterator.GetData()->IsClient() || !iterator.GetData()->CastToClient()->GMHideMe(client))
				) {
					if (range == 0 || iterator.GetData() == alwayssend || mob->DistNoRootNoZ(client) <= range) {
						mob->MakeSpawnUpdate(&spus->spawn_update[spus->num_updates]);
						spus->num_updates++;
					}
			}
		}
		if (spus->num_updates >= MAX_SPAWN_UPDATES_PER_PACKET) {
			client->QueuePacket(outapp, false);
			safe_delete(outapp);//delete outapp;
			outapp = 0;
		}
		iterator.Advance();
	}
	if (outapp != 0) {
		if (spus->num_updates > 0) {
			outapp->size = sizeof(SpawnPositionUpdates_Struct) + (sizeof(SpawnPositionUpdate_Struct) * spus->num_updates);
			client->QueuePacket(outapp, false);
		}
		safe_delete(outapp);//delete outapp;
		outapp = 0;
	}
}

char* EntityList::MakeNameUnique(char* name) {
	bool used[1000];
	memset(used, 0, sizeof(used));
	name[26] = 0; name[27] = 0; name[28] = 0; name[29] = 0;

	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	int len = 0;
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsMob()) {
			len = strlen(iterator.GetData()->CastToMob()->GetName());
			if (strncasecmp(iterator.GetData()->CastToMob()->GetName(), name, len - 2) == 0) {
				if (Seperator::IsNumber(&iterator.GetData()->CastToMob()->GetName()[len-2])) {
					used[atoi(&iterator.GetData()->CastToMob()->GetName()[len-2])] = true;
				}
			}
		}
		iterator.Advance();
	}
	for (int i=0; i < 1000; i++) {
		if (!used[i]) {
#ifdef WIN32
			snprintf(name, 30, "%s%02d", name, i);
#else
			//glibc clears destination of snprintf
			//make a copy of name before snprintf--misanthropicfiend
			char temp_name[30];
			strncpy(temp_name, name, 30);
			snprintf(name, 30, "%s%02d", temp_name, i);
#endif
			return name;
		}
	}
	cout << "Fatal error in EntityList::MakeNameUnique: Unable to find unique name" << endl;
	return 0;
}

char* EntityList::RemoveNumbers(char* name) {
	char	tmp[30];
	memset(tmp, 0, sizeof(tmp));
	int k = 0;
	for (int i=0; i<strlen(name); i++) {
		if (name[i] < '0' || name[i] > '9')
			tmp[k++] = name[i];
	}
	memcpy(name, tmp, 30);
	return name;
}

Corpse*	EntityList::GetCorpseByOwner(Client* client){
	LinkedListIterator<Entity*> iterator(list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsPlayerCorpse()) 
		{ 
			if (strcasecmp(iterator.GetData()->CastToCorpse()->GetOwnerName(), client->GetName()) == 0) {
				return iterator.GetData()->CastToCorpse();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
}

void EntityList::ListNPCs(Client* client) {
	LinkedListIterator<Entity*> iterator(list);
	int32 x = 0;

	iterator.Reset();
	client->Message(BLACK, "NPCs in the zone:");
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsNPC()) {
			client->Message(BLACK, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
			x++;
		}
		iterator.Advance();
	}
	client->Message(BLACK, "%5d npcs listed.", x);
}

void EntityList::ListNPCCorpses(Client* client) {
	LinkedListIterator<Entity*> iterator(list);
	int32 x = 0;

	iterator.Reset();
	client->Message(BLACK, "NPC Corpses in the zone:");
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsNPCCorpse()) {
			client->Message(BLACK, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
			x++;
		}
		iterator.Advance();
	}
	client->Message(BLACK, "%d npc corpses listed.", x);
}

void EntityList::ListPlayerCorpses(Client* client) {
	LinkedListIterator<Entity*> iterator(list);
	int32 x = 0;

	iterator.Reset();
	client->Message(BLACK, "Player Corpses in the zone:");
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsPlayerCorpse()) {
			client->Message(BLACK, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
			x++;
		}
		iterator.Advance();
	}
	client->Message(BLACK, "%d player corpses listed.", x);
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

	APPLAYER* outapp = new APPLAYER(OP_PetitionClientUpdate,sizeof(PetitionClientUpdate_Struct));
	PetitionClientUpdate_Struct* pcus = (PetitionClientUpdate_Struct*) outapp->pBuffer;
	pcus->petnumber = pet->GetID();		// Petition Number
	if (pet->CheckedOut()) {
		pcus->color = 0x00;
		pcus->senttime = 0xFFFFFFFF;
		pcus->unknown[3] = 0x00;
		strcpy(pcus->accountid, "");
		strcpy(pcus->gmsenttoo, "");
	}
	else {
		pcus->color = pet->GetUrgency();	// 0x00 = green, 0x01 = yellow, 0x02 = red
		//pcus->unknown2 = 0x00;
		pcus->senttime = pet->GetSentTime();
		pcus->unknown[3] = 0x1F;			// 4 has to be 0x1F
		strcpy(pcus->accountid, pet->GetAccountName());
		strcpy(pcus->charname, pet->GetCharName());
	}
	pcus->unknown[2] = 0x00;
	pcus->unknown[1] = 0x00;
	pcus->unknown[0] = 0x00;
	pcus->something = 0x00;

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->Admin() >= 100) {
			if (pet->CheckedOut())
				strcpy(pcus->gmsenttoo, "");
			else
				strcpy(pcus->gmsenttoo, iterator.GetData()->CastToClient()->GetName());
			iterator.GetData()->CastToClient()->QueuePacket(outapp);
		}
		iterator.Advance();
	}
	safe_delete(outapp);//delete outapp;
}

void EntityList::ClearClientPetitionQueue()
{
	APPLAYER* outapp = new APPLAYER(OP_PetitionClientUpdate,sizeof(PetitionClientUpdate_Struct));
	PetitionClientUpdate_Struct* pet = (PetitionClientUpdate_Struct*) outapp->pBuffer;

	pet->color = 0x00;
	pet->senttime = 0xFFFFFFFF;
	pet->unknown[3] = 0x00;
	pet->unknown[2] = 0x00;
	pet->unknown[1] = 0x00;
	pet->unknown[0] = 0x00;
	strcpy(pet->accountid, "");
	strcpy(pet->gmsenttoo, "");
	pet->something = 0x00;
	strcpy(pet->charname, "");
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient() && iterator.GetData()->CastToClient()->Admin() >= 100) {
			int x = 0;
			for (x=0;x<16;x++) {
				pet->petnumber = x;
				iterator.GetData()->CastToClient()->QueuePacket(outapp);
			}
		}
		iterator.Advance();
	}
	safe_delete(outapp);//delete outapp;
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

void EntityList::AddGroup(Group* group)
{
	LinkedListIterator<Entity*> iterator(list); 

	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsGroup() && iterator.GetData() == group) 
		{ 
			return;
		} 
		iterator.Advance(); 
	} 

	group->SetID(GetFreeID());
	list.Insert(group);
}

void EntityList::AddGroup(Group* group, int GID)
{
	LinkedListIterator<Entity*> iterator(list); 

	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsGroup() && iterator.GetData() == group) 
		{ 
			return;
		} 
		iterator.Advance(); 
	} 

	group->SetID(GID);
	list.Insert(group);
}

Group* EntityList::GetGroupByClient(Client* client) 
{ 
	LinkedListIterator<Entity*> iterator(list); 

	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsGroup()) 
		{ 
			if (iterator.GetData()->CastToGroup()->IsGroupMember(client)) {
				return iterator.GetData()->CastToGroup();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
} 

Group* EntityList::GetGroupByID(int32 group_id){
	LinkedListIterator<Entity*> iterator(list); 

	iterator.Reset(); ;

	while(iterator.MoreElements())
	{ 
		if (iterator.GetData()->IsGroup() && iterator.GetData()->CastToGroup()->GetGroupID() == group_id)
			return iterator.GetData()->CastToGroup();
		iterator.Advance(); 
	}
	return 0;
}

void EntityList::AddObject(Object* obj, bool SendSpawnPacket) {
	if(obj != NULL){
		obj->SetID(GetFreeID()); 

		if (SendSpawnPacket) {
			APPLAYER app;
			obj->CreateSpawnPacket(&app);
			cout << "send:" << endl;
			//DumpPacket(&app);
			QueueClients(0, &app,false);
		}

		list.Insert(obj);
	}else{
		cout << "Error: NULL object passed to EntityList::AddObject";
	}
}

void EntityList::AddProjectile(Projectile* p)
{
	if(p != NULL)
	{
		p->SetID(GetFreeID());
		list.Insert(p);
	}
	else
	{
		cout << "Error: NULL object passed to EntityList::AddProjectile";
	}
}

void EntityList::ClearFeignAggro(Mob* targ)
{
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsNPC() && iterator.GetData()->CastToNPC()->CheckAggro(targ))
		{
			iterator.GetData()->CastToNPC()->RemoveFromHateList(targ);
			if (iterator.GetData()->CastToMob()->GetLevel() >= 35)
				iterator.GetData()->CastToNPC()->SetFeignMemory(targ->CastToMob()->GetName());
		}
		iterator.Advance();
	}
}

void EntityList::MassFear(Client* client) {
	LinkedListIterator<Entity*> iterator(list);
	client->Message(BLACK,"Mass fearing zone");
	iterator.Reset();
	while(iterator.MoreElements()) {
		if(!iterator.GetData()->IsClient()) {
			iterator.GetData()->CastToNPC()->nodeReporter = true;
			iterator.GetData()->CastToNPC()->CalculateNewFearpoint();
		}
		iterator.Advance();
	}
}

void EntityList::RemoveNPC(NPC *npc){

	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.GetData()->Depop(false);
		if (iterator.GetData()->IsNPC() && iterator.GetData()->CastToNPC() == npc){
			iterator.RemoveCurrent();
			return;
		}
		else
			iterator.Advance();
	}
}

//o--------------------------------------------------------------
//| GetSurroundingEntities; Yeahlight, Aug 20, 2009
//o--------------------------------------------------------------
//| Finds all the entities within X range
//o--------------------------------------------------------------
void EntityList::GetSurroundingEntities(float x, float y, float z, float range, int32 entityList[], int32 &counter)
{
	counter = 1;
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	
	while(iterator.MoreElements() && counter < 150)
	{
		//Yeahlight: Entity is a mob
		if(iterator.GetData()->IsMob())
		{
			Mob* entity = iterator.GetData()->CastToMob();
			//Yeahlight: Entity is in range of the location
			if(entity && abs(entity->GetX() - x) <= range && abs(entity->GetY() - y) <= range && entity->DistanceToLocation(x, y, z) <= range)
			{
				entityList[counter] = entity->GetID();
				counter++;
			}
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

//o--------------------------------------------------------------
//| GetEntitiesAlongVector; Yeahlight, Aug 21, 2009
//o--------------------------------------------------------------
//| Finds all the entities within X range of some vector
//o--------------------------------------------------------------
void EntityList::GetEntitiesAlongVector(float slope, float xOrigin, float yOrigin, float zOrigin, float range, int32 entityList[], int32 &counter)
{
	counter = 0;
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	
	while(iterator.MoreElements() && counter < 150)
	{
		//Yeahlight: Entity is a mob
		if(iterator.GetData()->IsMob())
		{
			Mob* entity = iterator.GetData()->CastToMob();
			//Yeahlight: Entity exists
			if(entity)
			{
				//Yeahlight: Adjust the entity's coordinates to match the new origin
				float newX = entity->GetX() - xOrigin;
				float newY = entity->GetY() - yOrigin;
				float y = slope * newX;
				float x = newY / slope;
				//Yeahlight: Entity is in range of the vector along the x or y axis
				if((abs(newX - x) <= range || abs(newY - y) <= range) && abs(entity->GetZ() - zOrigin) <= 30)
				{
					entityList[counter] = entity->GetID();
					counter++;
				}
			}
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
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

bool EntityList::IsMobInZone(Mob *who) {
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(who == iterator.GetData())
			return(true);
		iterator.Advance();
	}
	return(false);
}

// Signal Quest command function
void EntityList::SignalMobsByNPCID(int32 snpc, int signal_id)
{
	LinkedListIterator<Entity*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->IsNPC()) {
			NPC *it = iterator.GetData()->CastToNPC();
			if (it->GetNPCTypeID() == snpc)
			{
				it->SignalNPC(signal_id);
			}
		}
		iterator.Advance();
	}
}