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
#include "groups.h"
#include "entity.h"
#include "client.h"
#include "NpcAI.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"
extern EntityList entity_list;

//
// Xorlac: This will need proper synchronization to make it work correctly.
//         Also, should investigate client ack for packet to ensure proper synch.
//

Group::Group(Mob* leader)
{
	memset(members,0,sizeof(Mob*) * MAX_GROUP_MEMBERS);
	members[0] = leader;
    leader->CastToClient()->isgrouped = true;
}

bool Group::AddMember(Mob* newmember)
{
	int i;

    for (i = 0; i < MAX_GROUP_MEMBERS; i++) {
        if (members[i] == NULL) {
            members[i] = newmember;
            break;
        }
    }
	if (i == MAX_GROUP_MEMBERS)
		return false;

	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
	memset(outapp->pBuffer, 0, outapp->size);
	GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;	
	strcpy(gu->yourname, newmember->GetName());
	gu->action = 0;
        
	for (i = 0;i < MAX_GROUP_MEMBERS; i++) {
		if (members[i] != NULL && members[i] != newmember && newmember->IsClient()) {
			strcpy(gu->membername,members[i]->GetName());

            newmember->CastToClient()->QueuePacket(outapp);
		}
	}
	
	strcpy(gu->membername,newmember->GetName());
	
    for (i = 0;i < MAX_GROUP_MEMBERS; i++) {
		if (members[i] != NULL && members[i] != newmember && members[i]->IsClient()) {
			strcpy(gu->yourname,members[i]->GetName());
            members[i]->CastToClient()->QueuePacket(outapp);
		}
	}

    newmember->isgrouped = true;
	
    delete outapp;
	
	return true;
}

//
// Xorlac: Does this consider side effects of being the last member to disband in a group, leadership changes, etc?
//

bool Group::DelMember(Mob* oldmember,bool ignoresender){
	int i;

	if (oldmember == NULL)
    {
		return false;
    }

    for (i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
        if (members[i] == oldmember)
        {
            members[i] = NULL;
            break;
        }
    }
    
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
	memset(outapp->pBuffer,0,outapp->size);
	GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;
	gu->action = 6;
	strcpy(gu->membername, oldmember->GetName());

	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
        if (members[i] == NULL) {
                //if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "Group::DelMember() null member at slot %i", i);
                continue;
        }
        if (members[i] != oldmember && members[i]->IsClient()) {
			strcpy(gu->yourname, members[i]->GetName());
            members[i]->CastToClient()->QueuePacket(outapp);
		}
		if(members[i] == oldmember && members[i]->IsNPC() && members[i]->CastToNPC()->IsGrouped() && members[i]->CastToNPC()->IsInteractive()) {
		    members[i]->CastToNPC()->TakenAction(23,0);
        }
	}

	if (!ignoresender && oldmember->IsClient()) {
		strcpy(gu->yourname,oldmember->GetName());
		strcpy(gu->membername,oldmember->GetName());
		gu->action = 6;

	    oldmember->CastToClient()->QueuePacket(outapp);
    }

	oldmember->isgrouped = false;
	disbandcheck = true;

    delete outapp;
	return true;	
}

void Group::TeleportGroup(Mob* sender, int32 zoneID, float x, float y, float z)
{
    for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
		if (members[i] != NULL && (members[i]->IsClient() || (members[i]->IsNPC() && members[i]->CastToNPC()->IsInteractive())) && members[i] != sender) {
		members[i]->CastToClient()->MovePC(zoneID, x, y, z);
		}
	}	
}

void Group::DisbandGroup(){
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
	memset(outapp->pBuffer,0,outapp->size);
	GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;
	gu->action = 4;
	
    for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
        if (members[i] == NULL)
                continue;
		if (members[i]->IsClient()) {
			strcpy(gu->yourname, members[i]->GetName());
            members[i]->CastToClient()->QueuePacket(outapp);
		}
		if (members[i]->IsNPC()) {
		    members[i]->CastToNPC()->TakenAction(23, members[i]);
        }
        members[i]->isgrouped = false;
	}	

	entity_list.RemoveEntity(this->GetID());

    delete outapp;
}
void Group::CastGroupSpell(Mob* caster, uint16 spellid)
{
	if(!caster)
		return;

	castspell = true;

    for (int z=0; z < MAX_GROUP_MEMBERS; z++)
    {
		if (members[z] != NULL && members[z] != caster)
        {
	        members[z]->SpellOnTarget(spellid,members[z]);
		}
	}	
	caster->SpellOnTarget(spellid,caster);
	castspell = false;
	disbandcheck = true;
}

void Group::SplitExp(uint32 exp, int16 otherlevel)
{
	int i;
	uint32 groupexp = exp;
	int8 membercount = 0;
	int8 maxlevel = 1;
	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
	{
		if (members[i] != NULL)
        {
			if(members[i]->GetLevel() > maxlevel)
				maxlevel = members[i]->GetLevel();
			groupexp += exp/10;
			membercount++;
		}
	}

	if (membercount == 0)
		return;

	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
	{
		if (members[i] != NULL && members[i]->IsClient())
    		{
            //
	    // add exp + exp cap
            //
			sint16 diff = members[i]->GetLevel() - maxlevel;
            if (diff >= -8) /*Instead of person who killed the mob, the person who has the highest level in the group*/{
#ifdef GUILDWARS
			    members[i]->CastToClient()->AddEXP(((members[i]->GetLevel()+3) * (members[i]->GetLevel()+3) * 200*3.5f < groupexp/membercount ) ? (int32)(members[i]->GetLevel() * members[i]->GetLevel() * 200*3.5f):(int32)(groupexp/membercount) );
#else
			    members[i]->CastToClient()->AddEXP(((members[i]->GetLevel()+3) * (members[i]->GetLevel()+3) * 75*3.5f < groupexp/membercount ) ? (int32)(members[i]->GetLevel() * members[i]->GetLevel() * 75*3.5f):(int32)(groupexp/membercount) );
#endif
		    }
		}
	}	
}

bool Group::Process()
{
if(disbandcheck && !GroupCount())
return false;
else if(disbandcheck && GroupCount())
disbandcheck = false;

return true;
}

bool Group::IsGroupMember(Mob* client)
{
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
		if (members[i] == client)
        {
			return true;
        }
	}

	return false;
}

int8 Group::GroupCount()
{
int count = 0;
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
		if (members[i])
        {
			count++;
        }
	}

	return count;
}

void Group::GroupMessage(Mob* sender,const char* message)
{
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if(!members[i])
			continue;
		if (members[i]->IsClient())
			members[i]->CastToClient()->ChannelMessageSend(sender->GetName(),members[i]->GetName(),2,0,message);
		if (members[i]->CastToNPC()->IsInteractive() && members[i] != sender)
			members[i]->CastToNPC()->InteractiveChat(2,1,message,(sender->GetTarget() != NULL) ? sender->GetTarget()->GetName():sender->GetName(),sender);
			//InteractiveChat(int8 chan_num, int8 language, const char * message, const char* targetname,Mob* sender);
	}	
}

int32 Group::GetTotalGroupDamage(Mob* other) {
    int32 total = 0;
	
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if(!members[i])
			continue;
		if (other->CheckAggro(members[i]))
			total += other->GetHateAmount(members[i],true);
	}
	return total;
}
