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

using namespace std;
using namespace EQC::Zone;

extern ZoneGuildManager zgm;
extern Zone* zone;
extern volatile bool ZoneLoaded;
extern WorldServer worldserver;

extern ClientList client_list;
extern int32 numclients;

Entity::Entity()
{
	id = 0;
}

Entity::~Entity()
{
	//Yeahlight: Free the ID number from the entity taken list
	entity_list.FreeTakenID(this->GetID());
}

void Entity::SetID(int16 set_id)
{
	id = set_id;
	//Yeahlight: Flag the ID as taken in the entity taken list
	entity_list.SetTakenID(id);
}

Client* Entity::CastToClient()
{
	if(!this->IsClient())
		EQC_MOB_EXCEPT("Client* Entity::CastToClient()", "Casting to a client while it's not a client!");
	return static_cast<Client*>(this);
}

NPC* Entity::CastToNPC()
{
	if(!this->IsNPC())
		EQC_MOB_EXCEPT("NPC* Entity::CastToNPC()", "Casting to a NPC while it's not a NPC!");
	return static_cast<NPC*>(this);
}

Mob* Entity::CastToMob()
{
	if(!this->IsMob()){
		EQC_MOB_EXCEPT("Mob* Entity::CastToMob()", "Casting to a Mob while it's not a Mob!");
	}

	return static_cast<Mob*>(this);
}

Corpse* Entity::CastToCorpse()
{
	if(!this->IsCorpse())
		EQC_MOB_EXCEPT("Corpse* Entity::CastToCorpse()", "Casting to a Corpse while it's not a Corpse!");
	return static_cast<Corpse*>(this);
}

Group* Entity::CastToGroup(){
	if(!this->IsGroup()){
		EQC_MOB_EXCEPT("Group* Entity::CastToGroup()", "Casting to a Group while it's not a Group!");
	}

	return static_cast<Group*>(this);
}

Object* Entity::CastToObject(){
	if(!this->IsObject()){
		EQC_MOB_EXCEPT("Object* Entity::CastToObject()", "Casting to an Object while it's not a Object!");
	}

	return static_cast<Object*>(this);
}

Projectile* Entity::CastToProjectile()
{
	if(!this->IsProjectile())
		EQC_MOB_EXCEPT("Projectile* Entity::CastToProjectile()", "Casting to a Projectile while it's not a Projectile!");

	return static_cast<Projectile*>(this);
}

#define MAX_SPAWNS_PER_PACKET	50
/*
Bulk zone spawn packet, to be requested by the client on zone in
Does 50 updates per packet, can fine tune that if neccessary

-Quagmire
*/

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


bool Entity::CheckCoordLosNoZLeaps(float cur_x, float cur_y, float cur_z, float trg_x, float trg_y, float trg_z, float perwalk)
{
	if(zone->map == NULL) {
		return(true);
	}
	VERTEX myloc;
	VERTEX oloc;
	VERTEX hit;

	myloc.x = cur_x;
	myloc.y = cur_y;
	myloc.z = cur_z+2;

	oloc.x = trg_x;
	oloc.y = trg_y;
	oloc.z = trg_z+2;

	if (myloc.x == oloc.x && myloc.y == oloc.y && myloc.z == oloc.z)
		return true;

	FACE *onhit;

	if (!zone->map->LineIntersectsZoneNoZLeaps(myloc,oloc,perwalk,&hit,&onhit))
		return true;
	return false;
}

bool Entity::CheckCoordLos(float cur_x, float cur_y, float cur_z, float trg_x, float trg_y, float trg_z, float perwalk)
{
	if(zone->map == NULL) {
		return(true);
	}
	VERTEX myloc;
	VERTEX oloc;
	VERTEX hit;

	myloc.x = cur_x;
	myloc.y = cur_y;
	myloc.z = cur_z+2;

	oloc.x = trg_x;
	oloc.y = trg_y;
	oloc.z = trg_z+2;

	if (myloc.x == oloc.x && myloc.y == oloc.y && myloc.z == oloc.z)
		return true;

	FACE *onhit;

	if (!zone->map->LineIntersectsZone(myloc,oloc,perwalk,&hit,&onhit))
		return true;
	return false;
}