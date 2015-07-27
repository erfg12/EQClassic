#ifndef ENTITYLIST_H
#define ENTITYLIST_H

#include "types.h"
#include "linked_list.h"
#include "database.h"
#include "zonedump.h"
#include "Spell.hpp"
#include "entity.h"

class EntityList
{
public:
	EntityList() { last_insert_id = 0; for(int i = 0; i < MAX_ENTITY_LIST_IDS; i++) takenIDs[i] = 0; }
	~EntityList() {}

	Entity* GetID(int16 id);
	Mob*	GetMob(int16 id);
	Mob*	GetMob(char* name);
	Mob*	GetMob(const char* name);
	Client* GetClientByName(const char *name); 
	Client* GetClientByAccID(int32 accid);
	Client* GetClient(int32 ip, int16 port);
	Corpse*	GetCorpseByOwner(Client* client);
	void	ClearClientPetitionQueue();

	void    AddClient(Client*);
	void    AddNPC(NPC*, bool SendSpawnPacket = true);
	int		GetNPCTypeCount(int NPCTypeID); // Kibanu - Added for spawn_limit checks
	void	AddCorpse(Corpse* pc, int32 in_id = 0xFFFFFFFF);
	void	AddObject(Object* obj, bool SendSpawnPacket);
	void	AddProjectile(Projectile* p);
	void	Clear();

	Group*	GetGroupByClient(Client* client);
	Group*  GetGroupByID(int32 group_id);
	void    AddGroup(Group*);
	void    AddGroup(Group*, int);

	void	Message(int32 to_guilddbid, MessageFormat type, char* message, ...);
	void	MessageClose(Mob* sender, bool skipsender, float dist, MessageFormat type, char* message, ...);
	void	ChannelMessageFromWorld(char* from, char* to, int8 chan_num, int32 guilddbid, int8 language, char* message, ...);
	void    ChannelMessage(Mob* from, int8 chan_num, int8 language, char* message, ...);
	void	ChannelMessageSend(Mob* to, int8 chan_num, int8 language, char* message, ...);
	//void    SendZoneSpawns(Client*);
	void	SendZoneSpawnsBulk(Client* client);
	void    SendZoneObjects(Client* client);
	void    Save();

	void    RemoveFromTargets(Mob* mob);

	void	QueueCloseClients(Mob* sender, APPLAYER* app, bool ignore_sender=false, float dist=200, Mob* SkipThisMob = 0, bool CheckLoS = false);
	void	QueueFarClients(Mob* sender, APPLAYER* app, bool ignore_sender=false, float dist=200, Mob* SkipThisMob = 0);
	void    QueueClients(Mob* sender, APPLAYER* app, bool ignore_sender=false);
	void	QueueClientsStatus(Mob* sender, APPLAYER* app, bool ignore_sender = false, int8 minstatus = 0, int8 maxstatus = 0);

	void	AESpell(Mob* caster, float x, float y, float z, float dist, Spell* spell);

	void	UpdateWho();
	void	SendPositionUpdates(Client* client, int32 cLastUpdate = 0, float range = 0, Entity* alwayssend = 0, bool iSendEvenIfNotChanged = false);
	char*	MakeNameUnique(char* name);
	static char*	RemoveNumbers(char* name);

	void	CountNPC(int32* NPCCount, int32* NPCLootCount, int32* gmspawntype_count);
	void	DoZoneDump(ZSDump_Spawn2* spawn2dump, ZSDump_NPC* npcdump, ZSDump_NPC_Loot* npclootdump, NPCType* gmspawntype_dump);
	void    RemoveEntity(int16 id);
	void	RemoveNPC(NPC *npc); // Tazadar : Remove a NPC from the list !
	void	SendPetitionToAdmins(Petition* pet);

	void	ListNPCs(Client* client);
	void	ListNPCCorpses(Client* client);
	void	ListPlayerCorpses(Client* client);
	sint32	DeleteNPCCorpses();
	sint32	DeletePlayerCorpses();
	void	WriteEntityIDs();

	bool	AddHateToCloseMobs(NPC* sender, float dist, int social);

    void    Process();
	void	BoatProcessOnly();

	void    ClearFeignAggro(Mob* targ); 

	void	DepopCorpses();

	void	MassFear(Client* client);

	void	SetTakenID(int16 id) { takenIDs[id] = 1; }
	void	FreeTakenID(int16 id) { takenIDs[id] = 0; }

	void	PetFaceClosestEntity(NPC* pet);

	void	GetSurroundingEntities(float x, float y, float z, float range, int32 list[], int32 &counter);
	void	GetEntitiesAlongVector(float slope, float xOrigin, float yOrigin, float zOrigin, float range, int32 entityList[], int32 &counter);
	Mob*	GetMobByNpcTypeID(int32 get_id);
	bool	IsMobInZone(Mob *who);
	void	SignalMobsByNPCID(int32 snpc, int signal_id);
protected:
	friend class Zone;
	void	Depop();
private:
	int16   GetFreeID();
	int8	takenIDs[MAX_ENTITY_LIST_IDS];
	LinkedList<Entity*> list;
	int16 last_insert_id;
};


#endif