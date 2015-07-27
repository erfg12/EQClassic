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
#ifndef ENTITY_H
#define ENTITY_H

#include "../common/types.h"
#include "../common/linked_list.h"
#include "../common/database.h"
#include "zonedump.h"
#include "zonedbasync.h"

// max number of newspawns to send per bulk packet
#define SPAWNS_PER_POINT_DATARATE 1024
#define MAX_SPAWNS_PER_PACKET	1024 * 10

#ifdef WIN32
	class	APPLAYER;
#else
	struct	APPLAYER;
#endif

class Client;
class Mob;
class NPC;
class Corpse;
class Petition;
class Object;
class Group;
class Doors;

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
	virtual bool IsObject()			{ return false; }
	virtual bool IsGroup()			{ return false; }
	virtual bool IsDoor()			{ return false; }

	virtual bool Process()  { return false; }
	virtual bool Save() { return true; }
	virtual void Depop(bool StartSpawnTimer = true) {}

	Client* CastToClient();
	NPC*    CastToNPC();
	Mob*    CastToMob();
	Corpse*	CastToCorpse();
	Object* CastToObject();
	Group*	CastToGroup();
	Doors*	CastToDoors();

	inline const int16& GetID()	{ return id; }
	virtual const char* GetName() { return ""; }
	virtual void DBAWComplete(int8 workpt_b1, DBAsyncWork* dbaw) { pDBAsyncWorkID = 0; }
protected:
	friend class EntityList;
	void SetID(int16 set_id);
	int32 pDBAsyncWorkID;
private:
	int16 id;
};

class EntityList
{
public:
	EntityList() { last_insert_id = 0; }
	~EntityList() {}

	Entity* GetID(int16 id);
	Mob*	GetMob(int16 id);
	Mob*	GetMob(const char* name);
	Mob*	GetMobByNpcTypeID(int32 get_id);
	Client* GetClientByName(const char *name); 
	Client* GetClientByAccID(int32 accid);
	Client* GetClientByID(int16 id);
	Client* GetClientByCharID(int32 iCharID);
	Client* GetClientByWID(int32 iWID);
	Client* GetClient(int32 ip, int16 port);
	Group*	GetGroupByMob(Mob* mob);
	Group*	GetGroupByClient(Client* client);
	Corpse*	GetCorpseByOwner(Client* client);
	Corpse* GetCorpseByID(int16 id);
	void ClearClientPetitionQueue();
    bool CanAddHateForMob(Mob *p);

	Doors*	FindDoor(int8 door_id);
	bool	MakeDoorSpawnPacket(APPLAYER* app);

	void    AddClient(Client*);
	void    AddNPC(NPC*, bool SendSpawnPacket = true, bool dontqueue = false);
	void	AddCorpse(Corpse* pc, int32 in_id = 0xFFFFFFFF);
	void    AddObject(Object*, bool SendSpawnPacket = true);
	void    AddGroup(Group*);
	void	AddDoor(Doors* door);
	void	Clear();

	bool	GuildNPCPlaceable(Mob* client);

	void	GuildItemAward(int32 guilddbid, int16 itemid);

	// Edgar's function to strip off trailing zeroes. 
	void   NPCMessage(Mob* sender, bool skipsender, float dist, int32 type, const char* message, ...); 

	void	Message(int32 to_guilddbid, int32 type, const char* message, ...);
	void	MessageStatus(int32 to_guilddbid, int32 to_minstatus, int32 type, const char* message, ...);
	void	MessageClose(Mob* sender, bool skipsender, float dist, int32 type, const char* message, ...);
	void	ChannelMessageFromWorld(const char* from, const char* to, int8 chan_num, int32 guilddbid, int8 language, const char* message, ...);
	void    ChannelMessage(Mob* from, int8 chan_num, int8 language, const char* message, ...);
	void	ChannelMessageSend(Mob* to, int8 chan_num, int8 language, const char* message, ...);
	void    SendZoneSpawns(Client*);
	void	SendZoneSpawnsBulk(Client* client);
	void    Save();
	void    SendZoneObjects(Client* client);

	void    RemoveFromTargets(Mob* mob);
    void    ReplaceWithTarget(Mob* pOldMob, Mob*pNewTarget);

	void	QueueCloseClients(Mob* sender, const APPLAYER* app, bool ignore_sender=false, float dist=200, Mob* SkipThisMob = 0, bool ackreq = true);
	void    QueueClients(Mob* sender, const APPLAYER* app, bool ignore_sender=false, bool ackreq = true);
	void	QueueClientsStatus(Mob* sender, const APPLAYER* app, bool ignore_sender = false, int8 minstatus = 0, int8 maxstatus = 0);
	void	QueueClientsByTarget(Mob* sender, const APPLAYER* app, bool iSendToSender = true, Mob* SkipThisMob = 0, bool ackreq = true);

	void	AESpell(Mob* caster, Mob* center, float dist, int16 spell_id, bool group = false);

	Mob*	FindDefenseNPC(int32 npcid);

	void	UpdateWho(bool iSendFullUpdate = false);
	void	SendPositionUpdates(Client* client, int32 cLastUpdate = 0, float range = 0, Entity* alwayssend = 0, bool iSendEvenIfNotChanged = false);
	char*	MakeNameUnique(char* name);
	static char*	RemoveNumbers(char* name);

	void	CountNPC(int32* NPCCount, int32* NPCLootCount, int32* gmspawntype_count);
	void	DoZoneDump(ZSDump_Spawn2* spawn2dump, ZSDump_NPC* npcdump, ZSDump_NPC_Loot* npclootdump, NPCType* gmspawntype_dump);
	void    RemoveEntity(int16 id);
	void	SendPetitionToAdmins(Petition* pet);

	void	ListNPCs(Client* client, const char* arg1 = 0, const char* arg2 = 0, int8 searchtype = 0);
	void	ListNPCCorpses(Client* client);
	void	ListPlayerCorpses(Client* client);
	sint32	DeleteNPCCorpses();
	sint32	DeletePlayerCorpses();
	void	WriteEntityIDs();
	void	HalveAggro(Mob* who);
	void	DoubleAggro(Mob* who);

    void    Process();
	void	ClearFeignAggro(Mob* targ);

	Mob*	AICheckCloseArrgo(Mob* sender, float iArrgoRange, float iAssistRange);
	void	AIYellForHelp(Mob* sender, Mob* attacker);
	bool	AICheckCloseSpells(Mob* caster, int8 iChance, float iRange, int16 iSpellTypes);
protected:
	friend class Zone;
	void	Depop();
private:
	int16   GetFreeID();
	void	AddToSpawnQueue(int16 entityid, NewSpawn_Struct** app);
	void	CheckSpawnQueue();

	int32	tsFirstSpawnOnQueue; // timestamp that the top spawn on the spawnqueue was added, should be 0xFFFFFFFF if queue is empty
	int32	NumSpawnsOnQueue;
	LinkedList<NewSpawn_Struct*> SpawnQueue;

	LinkedList<Entity*> list;
	int16 last_insert_id;
};

class BulkZoneSpawnPacket {
public:
	BulkZoneSpawnPacket(Client* iSendTo, int32 iMaxSpawnsPerPacket);	// 0 = send zonewide
	virtual ~BulkZoneSpawnPacket();
	
	bool	AddSpawn(NewSpawn_Struct* ns);
	void	SendBuffer();	// Sends the buffer and cleans up everything - can safely re-use the object after this function call (no need to free and do another new)
private:
	int32	pMaxSpawnsPerPacket;
	int32	index;
	NewSpawn_Struct* data;
	Client* pSendTo;
};

#endif

