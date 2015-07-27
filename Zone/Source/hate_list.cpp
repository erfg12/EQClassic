/*
#include <iostream>

#include "hate_list.h"
#include "mob.h"
#include "client.h"

HateList::HateList()
{
  first = 0;
}

HateList::~HateList()
{
	if (first != 0)
	{
		delete first;
	}
}

sint32 HateList::GetEntHate(Mob* ent) {
	HateListElement *current = first;
	HateListElement *prev = 0;

	while (current != 0) {
		if (current->GetEnt() == ent) {
			return current->GetHate();
			break;
		}
		prev = current;
		current = current->GetNext();
	}

	return 0;
}

void HateList::Add(Mob* ent, sint32 in_dam, sint32 in_hate) {
	if (ent == 0)
		return;
	if (in_hate == 0xFFFFFFFF) {
		// TODO: Some calulations or something
		in_hate = in_dam;
	}

	HateListElement* tmp = Pop(ent);
	if (tmp) {
		tmp->AddHate(in_dam, in_hate);
	}
	else {
		tmp = new HateListElement(ent, in_dam, in_hate);
	}

	HateListElement* current = first;
	HateListElement* prev = 0;
	while (current) {
		if (current->GetHate() < tmp->GetHate()) {
			break;
		}
		prev = current;
		current = current->GetNext();
	}
	tmp->SetNext(current);
	if (prev) {
		prev->SetNext(tmp);
	}
	else {
		first = tmp;
	}
}

HateListElement* HateList::Pop(Mob* in_ent) {
	HateListElement *current = first;
	HateListElement *prev = 0;

	while (current != 0) {
		if (current->GetEnt() == in_ent) {
			break;
		}
		prev = current;
		current = current->GetNext();
	}

	if (current != 0) {
		if (prev == 0) {
			first = current->GetNext();
		}
		else {
			prev->SetNext(current->GetNext());
		}
		current->SetNext(0);
		return current;
	}

	return 0;
}

Mob* HateList::GetTop()
{
	if (first == 0)
	{
		return 0;
	}

	return first->GetEnt();
}

void HateList::RemoveEnt(Mob *ent) {
	HateListElement *current = first;
	HateListElement *prev = 0;

	while (current != 0) {
		if (current->GetEnt() == ent)
		{
			break;
		}
		prev = current;
		current = current->GetNext();
	}

	if (current != 0) {
		if (prev == 0) {
			first = current->GetNext();
		}
		else {
			prev->SetNext(current->GetNext());
		}
		current->SetNext(0);
		delete current;
	}
}

void HateList::Whipe() {
	if (first != 0) {
		delete first;
		first = 0;
	}
}

void HateList::DoFactionHits(int32 npc_id) {
	if (first != 0)
		first->DoFactionHits(npc_id);
}

void HateListElement::DoFactionHits(int32 npc_id) {
	if (ent->IsClient()) {
		Client* client = ent->CastToClient();
		client->SetFactionLevel(client->CharacterID(), npc_id, client->GetClass(), client->GetRace(), client->GetDeity());
	}
	if (next != 0)
		next->DoFactionHits(npc_id);
}*/

#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "mob.h"
#include "linked_list.h"
#include "client.h"
#include "hate_list.h"
#include "groups.h"

class tHateEntry
{
public:
    Mob *ent;
    sint32 damage, hate;
    bool bFrenzy;
};

HateList::HateList()
{
}

HateList::~HateList()
{
}

// neotokyo: added for frenzy support
// checks if target still is in frenzy mode
void HateList::CheckFrenzyHate()
{
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        if (iterator.GetData()->ent->GetHPRatio() >= 20)
            iterator.GetData()->bFrenzy = false;
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
    }
}

void HateList::Whipe()
{
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
	{
        iterator.RemoveCurrent();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

bool HateList::IsOnHateList(Mob *mob)    
{    
	if (Find(mob))    
		return true;    
	return false;    
} 

tHateEntry *HateList::Find(Mob *ent)
{
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        if (iterator.GetData()->ent == ent)
            return iterator.GetData();
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
    }
	return NULL;
}

void HateList::Set(Mob* other, int32 in_hate, int32 in_dam)
{
    tHateEntry *p = Find(other);
    if (p)
    {
        p->damage = in_dam;
        p->hate = in_hate;
    }
}

/*Mob* HateList::GetDamageTop(Mob* hater)
{
	Mob* current = NULL;
	Group* grp = NULL;
	int32 dmg_amt = 0;

    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        grp = entity_list.GetGroupByMob(iterator.GetData()->ent);

		if (iterator.GetData()->ent != NULL && grp != NULL)
        {
			if (grp->GetTotalGroupDamage(hater) >= dmg_amt)
            {
				current = iterator.GetData()->ent;
				dmg_amt = grp->GetTotalGroupDamage(hater);
            }
        }
        else if (iterator.GetData()->ent != NULL && (int32)iterator.GetData()->damage >= dmg_amt)
        {
			current = iterator.GetData()->ent;
			dmg_amt = iterator.GetData()->damage;
        }
        iterator.Advance();
    }
	return current;
}*/

// neotokyo: a few comments added, rearranged code for readability
void HateList::Add(Mob *ent, sint32 in_dam, sint32 in_hate, bool bFrenzy, bool iAddIfNotExist)
{
	if(!ent)
        return;

    tHateEntry *p = Find(ent);
    if (p)
    {
		p->damage+=in_dam;
		//Yeahlight: Prevent a running hate total in the negatives
		if(p->hate + in_hate < 0)
			p->hate = 0;
		else
			p->hate+=in_hate;
        p->bFrenzy = bFrenzy;
    }
	else if (iAddIfNotExist) {
        p = new tHateEntry;
        p->ent = ent;
        p->damage = in_dam;
		//Yeahlight: Do not establish agro with a negative hate value
		if(in_hate < 0)
			in_hate = 0;
        p->hate = in_hate;
        p->bFrenzy = bFrenzy;
        list.Append(p);
    }
}


bool HateList::IsEmpty() {		
	return(GetHateListSize() == 0);
}

bool HateList::RemoveEnt(Mob *ent)
{
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        if (iterator.GetData()->ent == ent)
        {
            iterator.RemoveCurrent();
            return true;
        }
        else
            iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
    }
    return false;
}

void HateList::DoFactionHits(sint32 nfl_id) {
	if (nfl_id <= 0)
		return;
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        Client *p;

        if (iterator.GetData()->ent && iterator.GetData()->ent->IsClient())
            p = iterator.GetData()->ent->CastToClient();
        else
            p = NULL;

        if (p)
			p->SetFactionLevel(p->CharacterID(), nfl_id, p->GetClass(), p->GetRace(), p->GetDeity());
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
    }
}

Mob *HateList::GetTop(Mob* sender)
{
	Mob* top = NULL;
	sint32 hate = -1;

	//Yeahlight: NPC was not supplied; abort
	if(sender == NULL)
		return top;
	
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
		if(iterator.GetData()->ent)
		{
			bool permit = true;
			sint32 tempHate = iterator.GetData()->hate;
			Mob* entity = iterator.GetData()->ent;
			//Yeahlight: Sitting entities generate extra hate
			if(entity->GetAppearance() == SittingAppearance)
				tempHate = tempHate * SITTING_HATE_MODIFIER;
			//Yeahlight: Skip this entity if their owner is the sender
			if(sender->GetPet() == entity)
				permit = false;
			else if(sender == entity)
				permit = false;
			else if(tempHate <= hate)
				permit = false;
			//Yeahlight: This entity is currently the highest hated entity
			if(permit)
			{
				top = entity;
				hate = tempHate;
			}
		}
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
    return top;
}

//Yeahlight: Returns the second highest entity on the hate list
Mob *HateList::GetSecondFromTop(Mob* sender)
{
	Mob* secondFromTop = NULL;
	Mob* top = NULL;
	sint32 topHate = -1;
	sint32 hate = -1;

	LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	//Yeahlight: Iterate through the hate list and find the highest level of hate
	while(iterator.MoreElements())
    {
		//Yeahlight: Entity's hate is larger than the current top hate level
		if(iterator.GetData()->ent != NULL && iterator.GetData()->hate > topHate)
		{
			bool permit = true;
			//Yeahlight: Skip this entity if their owner is the sender
			if(sender && sender->GetPet() == iterator.GetData()->ent)
				permit = false;
			else if(sender == iterator.GetData()->ent)
				permit = false;
			if(permit)
			{
				top = iterator.GetData()->ent;
				topHate = iterator.GetData()->hate;
			}
		}
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	iterator.Reset();
	//Yeahlight: Iterate through the hate list again to find the second highest hate level
	while(iterator.MoreElements())
    {
		//Yeahlight: Entity's hate is larger than the current top hate level, is less than or equal to the 
		//           max hate level and is not the entity from which the max hate is pulled
		if(iterator.GetData()->ent != NULL && iterator.GetData()->hate > hate && iterator.GetData()->ent != top)
		{
			bool permit = true;
			//Yeahlight: Skip this entity if their owner is the sender
			if(sender && sender->GetID() == iterator.GetData()->ent->GetOwnerID())
				permit = false;
			if(permit)
			{
				secondFromTop = iterator.GetData()->ent;
				hate = iterator.GetData()->hate;
			}
		}
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return secondFromTop;
}

Mob *HateList::GetRandom()
{
    int count = 0;
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        iterator.Advance();
        count++;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
    }
	if(count == 0)
		return 0;
    int random = rand()%count;
    iterator.Reset();
    for (int i = 0; i < random-1; i++)
        iterator.Advance();
    return iterator.GetData()->ent;
}

sint32 HateList::GetEntHate(Mob *ent, bool damage)
{
	tHateEntry *p;

    p = Find(ent);
	
	if ( p && damage)
        return p->damage;
	else if (p)
		return p->hate;
	else
		return 0;
}

int16 HateList::GetHateListSize()
{
	int count = 0;
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        iterator.Advance();
        count++;
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
    }
	return count;
}

//Yeahlight: Searches for the group that will be rewarded with exp and first loot rights
returnGroup HateList::GetRewardingGroup()
{
	groupDamage groupsToConsider[255] = {0};
	int16 groupCounter = 0;
	int16 groupID = 0;
	int16 leadingGroupID = 0;
	sint32 highestGroupDamage = 0;
	sint32 entityDamage = 0;
	Mob* entity = NULL;
	returnGroup groupToReward;
	Group* group = NULL;
	bool groupFound = false;

	for(int i = 0; i < 255; i++)
	{
		groupsToConsider[i].groupID = -1;
		groupsToConsider[i].groupDamageTotal = -1;
	}
	groupToReward.damage = 0;
	groupToReward.group = NULL;

	LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
		entityDamage = 0;
		if(iterator.GetData())
			entityDamage = iterator.GetData()->damage;
		if(entityDamage > 0)
		{
			entity = iterator.GetData()->ent;
			//Yeahlight: Entity exists in the zone
			if(entity)
			{
				//Yeahlight: Entity is a client
				if(entity->IsClient())
				{
					//Yeahlight: PC is grouped
					if(entity->CastToClient()->IsGrouped())
					{
						group = entity_list.GetGroupByClient(entity->CastToClient());
						//Yeahlight: Group exists
						if(group)
						{
							groupID = group->GetID();
							//Yeahlight: We do not have a group list started yet, so start it off with this group
							if(groupCounter == 0)
							{
								groupsToConsider[0].groupID = groupID;
								groupsToConsider[0].groupDamageTotal = iterator.GetData()->damage;
								groupsToConsider[0].group = group;
								groupCounter++;
							}
							//Yeahlight: A group list has been created already
							else
							{
								//Yeahlight: Iterate through the group list
								for(int i = 0; i < groupCounter; i++)
								{
									//Yeahlight: Found the group for which this PC resides
									if(groupID == groupsToConsider[i].groupID)
									{
										//Yeahlight: Add the client's damage to the group's total
										groupsToConsider[i].groupDamageTotal += iterator.GetData()->damage;
										//Yeahlight: Flag the PC as found
										groupFound = true;
										//Yeahlight: "Break out" of the loop
										i = groupCounter;
									}
								}
								//Yeahlight: This grouped PC did not find their group in the list, so we need to add their group
								if(!groupFound)
								{
									groupsToConsider[groupCounter].groupID = groupID;
									groupsToConsider[groupCounter].groupDamageTotal = iterator.GetData()->damage;
									groupsToConsider[groupCounter].group = group;
									groupCounter++;
								}
							}
						}
						//Yeahlight: Group does not exist
						else
						{
							
						}
					}
					//Yeahlight: PC is not grouped
					else
					{

					}
				}
				//Yeahlight: Entity is the pet of a client
				else if(entity->IsNPC() && entity->GetOwner() && entity->GetOwner()->IsClient())
				{
					//Yeahlight: Owner of the pet is grouped
					if(entity->GetOwner()->CastToClient()->IsGrouped())
					{
						group = entity_list.GetGroupByClient(entity->GetOwner()->CastToClient());
						//Yeahlight: Group exists
						if(group)
						{
							groupID = group->GetID();
							//Yeahlight: We do not have a group list started yet, so start it off with this group
							if(groupCounter == 0)
							{
								groupsToConsider[0].groupID = groupID;
								groupsToConsider[0].groupDamageTotal = iterator.GetData()->damage;
								groupsToConsider[0].group = group;
								groupCounter++;
							}
							//Yeahlight: A group list has been created already
							else
							{
								//Yeahlight: Iterate through the group list
								for(int i = 0; i < groupCounter; i++)
								{
									//Yeahlight: Found the group for which this pet resides
									if(groupID == groupsToConsider[i].groupID)
									{
										//Yeahlight: Add the pet's damage to the group's total
										groupsToConsider[i].groupDamageTotal += iterator.GetData()->damage;
										//Yeahlight: Flag the pet as found
										groupFound = true;
										//Yeahlight: "Break out" of the loop
										i = groupCounter;
									}
								}
								//Yeahlight: This grouped pet did not find their group in the list, so we need to add their group
								if(!groupFound)
								{
									groupsToConsider[groupCounter].groupID = groupID;
									groupsToConsider[groupCounter].groupDamageTotal = iterator.GetData()->damage;
									groupsToConsider[groupCounter].group = group;
									groupCounter++;
								}
							}
						}
						//Yeahlight: Group does not exist
						else
						{
							
						}
					}
					//Yeahlight: Pet's owner is not grouped
					else
					{

					}
				}
			}
			//Yeahlight: Entity does not exist in the zone
			else
			{

			}
		}
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
    }

	//Yeahlight: Iterate through the group list and record for the highest damaging group
	for(int i = 0; i < groupCounter; i++)
	{
		//Yeahlight: Found a group with more total damage than our current leader
		if(groupsToConsider[i].groupDamageTotal > highestGroupDamage)
		{
			highestGroupDamage = groupsToConsider[i].groupDamageTotal;
			groupToReward.damage = groupsToConsider[i].groupDamageTotal;
			groupToReward.group = groupsToConsider[i].group;
		}
	}

	return groupToReward;
}

//Yeahlight: Searches for the single entity that will be rewarded with exp and first loot rights
Mob* HateList::GetRewardingEntity(sint32 highestGroupDamage)
{
	sint32 highestDamage = 0;
	sint32 entityDamage = 0;
	Mob* entityToReward = NULL;

	LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
		if(iterator.GetData())
			entityDamage = iterator.GetData()->damage;
		//Yeahlight: This entity's damage is the highest so far from the list and is greater than
		//           any single group's combined damage
		if(entityDamage > highestDamage && entityDamage > highestGroupDamage)
		{
			entityToReward = iterator.GetData()->ent;
			highestDamage = entityDamage;
		}
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
    }

	return entityToReward;
};

Mob* HateList::GetClosest(Mob* hater)
{
	if(!hater)
		return 0;

	Mob* close = NULL;
	float closedist = 99999.9f;
	float thisdist;
	
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->ent)
		{
			thisdist = iterator.GetData()->ent->DistNoRootNoZ(hater);
			if(thisdist <= closedist)
			{
				closedist = thisdist;
				close = iterator.GetData()->ent;
			}
		}
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	if (!close && hater->IsNPC())
		close = hater->CastToNPC()->GetHateTop();

	return close;
}

//Yeahlight: Searches for the most-hated PC in melee range when a pet would normally be the target
Mob* HateList::GetTopInRangeNoPet(Mob* sender)
{
	Mob* top = NULL;
	sint32 hate = -1;
	
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
		//Yeahlight: Entity exists, has more hate than the highest hated entity, is not a pet and is in melee range
		if(iterator.GetData()->ent != NULL && iterator.GetData()->hate > hate && iterator.GetData()->ent->GetOwner() == NULL && iterator.GetData()->ent->DistNoZ(sender) <= sender->GetMeleeReach())
		{
			bool permit = true;
			//Yeahlight: Skip this entity if their owner is the sender
			if(sender && sender->GetPet() == iterator.GetData()->ent)
				permit = false;
			else if(sender == iterator.GetData()->ent)
				permit = false;
			if(permit)
			{
				top = iterator.GetData()->ent;
				hate = iterator.GetData()->hate;
			}
		}
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
    return top;
}

//Yeahlight: Builds a list of PC entities that were involved with the NPC's death
queue<char*> HateList::BuildHitList()
{
	queue<char*> ret;
	LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
		//Yeahlight: Entity exists and is a PC
		if(iterator.GetData()->ent && iterator.GetData()->ent->IsClient())
		{
			ret.push(iterator.GetData()->ent->GetName());
		}
        iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return ret;
}