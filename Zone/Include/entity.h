#ifndef ENTITY_H
#define ENTITY_H

#include "types.h"
#include "linked_list.h"
#include "database.h"
#include "zonedump.h"
#include "Spell.hpp"

class Client;
class Mob;
class NPC;
class Corpse;
class Petition;
class Group;
class Object;
class Projectile;

using namespace EQC::Common::Network;

void ProcessClientThreadSpawn(void *tmp);

class Entity
{
public:
	Entity();
	virtual ~Entity();

	virtual bool IsClient()			{ return false; }
	virtual bool IsNPC()			{ return false; }
	virtual bool IsMob()			{ return false; }
	virtual bool IsCorpse()			{ return false; }
	virtual bool IsPlayerCorpse()	{ return false; }
	virtual bool IsNPCCorpse()		{ return false; }
	virtual bool IsGroup()			{ return false; }
	virtual bool IsObject()			{ return false; }
	virtual bool IsProjectile()		{ return false; }

	virtual bool Process()  { return false; }
	virtual bool Save() { return true; }
	virtual void Depop(bool StartSpawnTimer = true) {}

	Client* CastToClient();
	NPC*    CastToNPC();
	Mob*    CastToMob();
	Corpse*	CastToCorpse();
	Group*	CastToGroup();
	Object* CastToObject();
	Projectile* CastToProjectile();

	inline int16 GetID()	{ return id; }
	virtual char* GetName() = 0;
	
	bool CheckCoordLosNoZLeaps(float cur_x, float cur_y, float cur_z, float trg_x, float trg_y, float trg_z, float perwalk=1);
	bool CheckCoordLos(float cur_x, float cur_y, float cur_z, float trg_x, float trg_y, float trg_z, float perwalk=1);
	void createDirectedGraph();

protected:
	friend class EntityList;
	void SetID(int16 set_id);
	int16 id;
};


#endif

