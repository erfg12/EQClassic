/*#ifndef HATELIST_H
#define HATELIST_H

#include "types.h"

class Mob;

class HateListElement
{
private:

	Mob*				ent;
	sint32				damage;
	sint32				hate;
	HateListElement*	next;

public:

	HateListElement(Mob* in_ent, sint32 in_dam, sint32 in_hate)
	{
		ent = in_ent;
		damage = in_dam;
		hate = in_hate;
		next = 0;
	}

	~HateListElement()
	{
		if (next != 0)
		{
			delete next;
		}
	}

	void AddHate(sint32 in_dam, sint32 in_hate) { damage += in_dam; hate += in_hate; }
	sint32 GetHate() { return hate; }
	Mob* GetEnt() { return ent; }

	HateListElement* GetNext()         { return  next; }
	void SetNext(HateListElement* n)	{ next = n; } 
	void DoFactionHits(int32 npc_id);
};

class HateList
{
private:
	HateListElement* first;

	HateListElement* Pop(Mob* in_ent);
public:
	HateList();
	~HateList();

	// Heko - added to return hate of given ent
	sint32 GetEntHate(Mob* ent);
	void Add(Mob* ent, sint32 in_dam = 0, sint32 in_hate = 0xFFFFFFFF);
	void RemoveEnt(Mob* ent);
	Mob* GetTop();
	void Whipe();
	void DoFactionHits(int32 npc_id);
};

#endif*/

#ifndef HATELIST_H
#define HATELIST_H

#include "mob.h"

struct groupDamage
{
	sint16 groupID;
	sint32 groupDamageTotal;
	Group* group;
};

struct returnGroup
{
	sint32 damage;
	Group* group;
};

class tHateEntry;

class HateList
{
public:
    HateList();
    ~HateList();

    // adds a mob to the hatelist
    void Add(Mob *ent, sint32 in_dam=0, sint32 in_hate=0, bool bFrenzy = false, bool iAddIfNotExist = true);
    // sets existing hate
    void Set(Mob *other, int32 in_hate, int32 in_dam);
    // removes mobs from hatelist
    bool RemoveEnt(Mob *ent);
    // Remove all
    void Whipe();
    // ???
    void DoFactionHits(sint32 nfl_id);
    // Gets Hate amount for mob
    sint32 GetEntHate(Mob *ent, bool damage = false);
    // gets top hated mob
    Mob* GetTop(Mob* sender);
    // gets any on the list
    Mob* GetRandom();
    // get closest mob or NULL if list empty
    Mob* GetClosest(Mob* hater);
	//Yeahlight: Searches for the most-hated PC in melee range when a pet would normally be the target
	Mob* GetTopInRangeNoPet(Mob* hater);
    // used to check if mob is on hatelist
    bool IsOnHateList(Mob *);
    // used to remove or add frenzy hate
    void CheckFrenzyHate();
	//Yeahlight: Returns number of entities on the hate list
	int16 GetHateListSize();
	//Yeahlight: Returns the second highest entity on the hate list
	Mob* GetSecondFromTop(Mob* sender = NULL);
	//Yeahlight: Returns the group to be rewarded with exp and loot rights
	returnGroup GetRewardingGroup();
	//Yeahlight: Returns the single entity to be rewarded with exp and loot rights
	Mob* GetRewardingEntity(sint32 highestGroupDamage = 0);
	//Yeahlight: Builds a list of players involved with the NPC's death
	queue<char*> BuildHitList();

	bool IsEmpty();

protected:
    tHateEntry *Find(Mob *ent);
private:
    LinkedList<tHateEntry*> list;
};

#endif

