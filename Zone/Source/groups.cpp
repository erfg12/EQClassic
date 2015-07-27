//////////////////////////////////////////////////////////////////////
#include "groups.h"
#include "entity.h"
#include "client.h"
#include "packet_functions.h"
#include "packet_dump.h"
#include "EQCException.hpp"
#include "EQCUtils.hpp"
#include "SpellsHandler.hpp"

extern EntityList		entity_list;
extern Database			database;
extern WorldServer		worldserver;
extern SpellsHandler	spells_handler;	// Better way of handling this.

//////////////////////////////////////////////////////////////////////
// Constructor takes a client argument who will be set to leader.
//////////////////////////////////////////////////////////////////////
Group::Group(Client* leader){	// new group
	int32 gid;

	gid = Database::Instance()->GetFreeGroupID();
	SetGroupID(gid);
	this->ResetGroupData();
	this->AddMember(leader, FIRST);
	leader->SetGroupInvitePending(false);	
	entity_list.AddGroup(this); //Kibanu - I added a check in the function, if it exists it won't re-add it.
}

//////////////////////////////////////////////////////////////////////

Group::Group(int GID){	// existing group from DB.
	this->ResetGroupData();
	bool ret;
	ret = this->LoadFromDB(GID);
	entity_list.AddGroup(this);
	if (!ret) {
		entity_list.RemoveEntity(this->GetID()); // Now remove it from memory
		return;
	}
	SetGroupID(GID);
}

//////////////////////////////////////////////////////////////////////

void Group::SetGroupID(int gid) {	//Added by Kibanu: multiple zones have group entities with same
	GroupID = gid;					//GroupID and can conflict with entityIDs in new zone(s)
}

int Group::GetGroupID() {
	return GroupID;
}

void Group::ResetGroupData() {
	this->ignoreNextDisband = false;
	this->pvMembers.clear();
}

//////////////////////////////////////////////////////////////////////

bool Group::AddMember(Client* member) {
	return this->AddMember(member, LAST, false);
}

//////////////////////////////////////////////////////////////////////

bool Group::AddMember(Client* member, InsertPosition pos){
	return this->AddMember(member, pos, false);
}

//////////////////////////////////////////////////////////////////////

bool Group::AddMember(Client* member, InsertPosition pos, bool newleader){
	if (this->GetMembersInGroup() > MAX_GROUP_MEMBERS && !newleader){
		cout << "Group full" << endl;
		return false;
	}

	//1. Adds into list.
	if (!newleader) {		
		if (pos == FIRST)
			this->pvMembers.insert(this->pvMembers.begin(), TMember(member));
		else if (pos == LAST)
			this->pvMembers.push_back(TMember(member));
	}

	//2. Refresh player profiles.
	this->RefreshPlayerProfiles();

	//3. Prepare a packet.
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
	memset(outapp->pBuffer,0,sizeof(GroupUpdate_Struct));
	GroupUpdate_Struct* gu = (GroupUpdate_Struct*)outapp->pBuffer;
	gu->action = ADD_MEMBER;

	//4. Send a list of group members to the new member.
	strncpy(gu->receiverName, member->GetName(), MAX_NAME_SIZE);
	vector<TMember>::iterator it;
	//cout << "Before loop to send data to new member - " << member->GetName() << endl;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		//cout << "In top of loop (" << it->Name << ")" << endl;
		if (strlen(it->Name) > 1 && strcmp(it->Name, member->GetName()) != 0) {
			//cout << "In loop - sending data about " << it->Name << endl;
			strcpy(gu->senderName, it->Name);
			member->QueuePacket(outapp);
		}
	}
	//cout << "Out of loop" << endl;
	memset(outapp->pBuffer,0,sizeof(GroupUpdate_Struct));

	//5. Send the new member info to all members in group.
	strncpy(gu->senderName, member->GetName(), MAX_NAME_SIZE);
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		strncpy(gu->receiverName, it->Name, MAX_NAME_SIZE);
		if (it->Ptr && it->Ptr != member)
			it->Ptr->QueuePacket(outapp);
		else if(!it->Ptr && strcmp(it->Name, member->GetName()) != 0) {	//Member not in zone, send packet by world.
			ServerPacket* pack = new ServerPacket(ServerOP_SendPacket, sizeof(ServerSendPacket_Struct) + sizeof(GroupUpdate_Struct));
			memset(pack->pBuffer, 0, pack->size);
			ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*) pack->pBuffer;
			strncpy(sss->charname, it->Name, MAX_NAME_SIZE);
			sss->opcode = OP_GroupUpdate;
			memcpy(sss->packet, outapp->pBuffer, sizeof(GroupUpdate_Struct));
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}

		//5.1. Force an update to groups if players aren't in the zone.
		if ( !it->Ptr )
		{
			ServerPacket* pack = new ServerPacket(ServerOP_GroupRefresh, sizeof(ServerGroupRefresh_Struct));
			memset(pack->pBuffer, 0, pack->size);
			ServerGroupRefresh_Struct* sgr = (ServerGroupRefresh_Struct*) pack->pBuffer;			
			strncpy(sgr->member, it->Name, MAX_NAME_SIZE);
			sgr->action = ADD_MEMBER;
			sgr->gid = this->GetGroupID();
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
	safe_delete(outapp);//delete outapp;
	member->SetGroupInvitePending(false);

	//6. Finish and insert into DB.
	if ( !newleader )
	{
		if ( pos == FIRST )
		{
			//cout << "Setting GroupID of " << this->GetID() << " as LEADER" << endl;
			Database::Instance()->SetGroupID(member->GetName(), this->GetGroupID(), 1);
		}
		else
		{
			//cout << "Setting GroupID of " << this->GetID() << " as Member" << endl;
			Database::Instance()->SetGroupID(member->GetName(), this->GetGroupID(), 0);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////

void Group::ChangeLeader(char* member, bool leftgame){
	// Capture old leader
	char oldleader[MAX_NAME_SIZE];
	strncpy(oldleader, this->pvMembers[0].Name, MAX_NAME_SIZE);

	//1.Update leader flag(s)
	vector<TMember>::iterator it;
	vector<TMember>::iterator it2;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (strcmp(it->Name,member) == 0) {
			Database::Instance()->SetGroupID(it->Name, this->GetGroupID(), 1); // Update leader flag in database
		}
		else {
			// Set leader flag to 0
			Database::Instance()->SetGroupID(it->Name, this->GetGroupID(), 0);
		}
	}

	//2.Remove leader from list
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (stricmp(it->Name,member) == 0) {
			this->pvMembers.erase(it);
			break;
		}
	}

	//3.Re-add player to top of the stack
	Client* client = entity_list.GetClientByName( member );
	if ( client )
		this->pvMembers.insert(this->pvMembers.begin(), TMember(client));
	else
		this->pvMembers.insert(this->pvMembers.begin(), TMember(member));
	
	//4. Refresh player profiles.
	this->RefreshPlayerProfiles();

	
	//5. Send a fake disband packet to players
	this->DisbandGroup(true);

	//6. Send a fake join packet to players with new leader info
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (it == this->pvMembers.begin()) // Don't send packet to leader
			continue;

		//6.1 Prepare group update packet
		APPLAYER* outapp2 = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
		memset(outapp2->pBuffer,0,sizeof(GroupUpdate_Struct));
		GroupUpdate_Struct* gu = (GroupUpdate_Struct*)outapp2->pBuffer;
		gu->action = ADD_MEMBER;
		
		//6.2. Send a list of group members to the new member.
		strncpy(gu->receiverName, it->Name, MAX_NAME_SIZE);
		for (it2 = this->pvMembers.begin(); it2 <= it; it2++) {
			if (leftgame && strcmp(oldleader, it2->Name) == 0)
				continue; // Skip old leader
			if (strlen(it2->Name) > 1 && strcmp(it2->Name, it->Name) != 0) {
				strcpy(gu->senderName, it2->Name);
				if (it->Ptr)
				{
					it->Ptr->QueuePacket(outapp2);
				}
				else {	//Member not in zone, send packet by world.
					ServerPacket* pack = new ServerPacket(ServerOP_SendPacket, sizeof(ServerSendPacket_Struct) + sizeof(GroupUpdate_Struct));
					memset(pack->pBuffer, 0, pack->size);
					ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*) pack->pBuffer;
					strncpy(sss->charname, it->Name, MAX_NAME_SIZE);
					sss->opcode = OP_GroupUpdate;
					memcpy(sss->packet, outapp2->pBuffer, sizeof(GroupUpdate_Struct));
					worldserver.SendPacket(pack);
					safe_delete(pack);//delete pack;
				}
			}
		}
		memset(outapp2->pBuffer,0,sizeof(GroupUpdate_Struct));
		
		//6.3. Send the new member info to all members in group.
		strncpy(gu->senderName, it->Name, MAX_NAME_SIZE);
		for (it2 = this->pvMembers.begin(); it2 <= it; it2++) {
			if (leftgame && strcmp(oldleader, it2->Name) == 0)
				continue; // Skip old leader
			strncpy(gu->receiverName, it2->Name, MAX_NAME_SIZE);
			if (it2->Ptr && it->Ptr && it2->Ptr != it->Ptr)
			{
				it2->Ptr->QueuePacket(outapp2);
			}
			else if( (!it2->Ptr || !it->Ptr) && strcmp(it2->Name, it->Name) != 0) {	//Member not in zone, send packet by world.
				ServerPacket* pack = new ServerPacket(ServerOP_SendPacket, sizeof(ServerSendPacket_Struct) + sizeof(GroupUpdate_Struct));
				memset(pack->pBuffer, 0, pack->size);
				ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*) pack->pBuffer;
				strncpy(sss->charname, it2->Name, MAX_NAME_SIZE);
				sss->opcode = OP_GroupUpdate;
				memcpy(sss->packet, outapp2->pBuffer, sizeof(GroupUpdate_Struct));
				worldserver.SendPacket(pack);
				safe_delete(pack);//delete pack;
			}
		}
		safe_delete(outapp2);//delete outapp;

		if (!leftgame || (leftgame && strcmp(oldleader, it->Name) != 0))
		{
			//6.5. Send follow packet to new leader
			APPLAYER* outapp = new APPLAYER(OP_GroupFollow,sizeof(GroupFollow_Struct));
			memset(outapp->pBuffer,0,sizeof(GroupFollow_Struct));
			GroupFollow_Struct* gf = (GroupFollow_Struct*)outapp->pBuffer;
			strncpy(gf->invited, it->Name, MAX_NAME_SIZE);
			strncpy(gf->leader, member, MAX_NAME_SIZE);
			if ( client )
				client->QueuePacket(outapp);
			else { // Send packet through world server
				ServerPacket* pack = new ServerPacket(ServerOP_SendPacket, sizeof(ServerSendPacket_Struct) + sizeof(GroupFollow_Struct));
				memset(pack->pBuffer, 0, pack->size);
				ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*) pack->pBuffer;
				strncpy(sss->charname, member, MAX_NAME_SIZE);
				sss->opcode = OP_GroupFollow;
				memcpy(sss->packet, outapp->pBuffer, sizeof(GroupFollow_Struct));
				worldserver.SendPacket(pack);
				safe_delete(pack);//delete pack;
			}
			safe_delete(outapp);//delete outapp;	
		}
	}
	
	//7. Notify group members of the leader change and force a possible group update if not in zone
	char* message = 0;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (leftgame && strcmp(oldleader, it->Name) == 0)
			continue; // Skip old leader
		if ( it == this->pvMembers.begin() )
			MakeAnyLenString(&message, "You are the new group leader." );
		else
			MakeAnyLenString(&message, "%s is the new group leader.", member);
		ServerPacket* pack = new ServerPacket(ServerOP_EmoteMessage, sizeof(ServerEmoteMessage_Struct) + strlen(message) + 1);
		ServerEmoteMessage_Struct* scm = (ServerEmoteMessage_Struct*) pack->pBuffer;
		memset(pack->pBuffer, 0, pack->size);
		strncpy(scm->to, it->Name, MAX_NAME_SIZE);
		strncpy(scm->message, message, strlen(message) + 1);
		scm->guilddbid = 0;
		scm->type = 1;
		worldserver.SendPacket(pack);
		safe_delete(pack);//delete pack;

		//7.1. They're not in zone, refresh group structure in their zone
		if ( !it->Ptr ) {
			ServerPacket* pack = new ServerPacket(ServerOP_GroupRefresh, sizeof(ServerGroupRefresh_Struct));
			memset(pack->pBuffer, 0, pack->size);
			ServerGroupRefresh_Struct* sgr = (ServerGroupRefresh_Struct*) pack->pBuffer;			
			strncpy(sgr->member, it->Name, MAX_NAME_SIZE);
			sgr->action = NEW_LEADER;
			sgr->gid = this->GetGroupID();
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}

	// Flag so old leader doesn't disband the group, the client sends a disband packet to server for some reason.
	this->ignoreNextDisband = true;
}

//////////////////////////////////////////////////////////////////////

void Group::DelMember(char* name, bool ignoresender, bool leftgame) {
	if (this->GetMembersInGroup() <= 2) {
		cout << "Disbanding group" << endl;
		this->DisbandGroup();
		return;
	}

	if (strcmp(this->pvMembers[0].Name, name) == 0 && leftgame)
	{
		// Leader disconnected, don't disband group
		char newldr[MAX_NAME_SIZE];
		strcpy(newldr, this->pvMembers[1].Name);
		this->ChangeLeader(newldr, true);
	}

	vector<TMember>::iterator it;
	Client* Member = NULL;
	//1. Search for the client.
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (strcmp(it->Name, name) == 0) {
			if(it->Ptr) {
				memset(it->Ptr->GetPlayerProfilePtr()->GroupMembers, 0, sizeof(it->Ptr->GetPlayerProfilePtr()->GroupMembers));
				Member = it->Ptr;
			}
			this->pvMembers.erase(it); //2. Erase from the list.
			break;
		}
	}
	Database::Instance()->SetGroupID(name, 0);
	//3. Refresh player profiles.
	this->RefreshPlayerProfiles();

	//4. Send packets.
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
	memset(outapp->pBuffer,0,outapp->size);
	GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;
	gu->action = REMOVE_MEMBER;
	strncpy(gu->senderName, name, MAX_NAME_SIZE);

	//5. Notify members that player got disbanded.
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		strncpy(gu->receiverName, it->Name, MAX_NAME_SIZE);
		if (it->Ptr)
			it->Ptr->QueuePacket(outapp); // Notifies other members.
		else if(!it->Ptr) {	//Member not in zone, send packet by world.
			ServerPacket* pack = new ServerPacket(ServerOP_SendPacket, sizeof(ServerSendPacket_Struct) + sizeof(GroupUpdate_Struct));
			memset(pack->pBuffer, 0, pack->size);
			ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*) pack->pBuffer;
			strncpy(sss->charname, it->Name, MAX_NAME_SIZE);
			sss->opcode = OP_GroupUpdate;
			memcpy(sss->packet, outapp->pBuffer, sizeof(GroupUpdate_Struct));
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}

	//6. Notify kicked member that he got disbanded.
	if (!ignoresender && !leftgame){ //Notifies dropping member that he has dropped from the group;
		strncpy(gu->receiverName, name, MAX_NAME_SIZE);
		gu->senderName[0] = 0;
		gu->action = GROUP_QUIT;
		if (Member)
			Member->QueuePacket(outapp);
		else {	//Member not in zone, send packet by world.
			ServerPacket* pack = new ServerPacket(ServerOP_SendPacket, sizeof(ServerSendPacket_Struct) + sizeof(GroupUpdate_Struct));
			memset(pack->pBuffer, 0, pack->size);
			ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*) pack->pBuffer;
			strncpy(sss->charname, name, MAX_NAME_SIZE);
			sss->opcode = OP_GroupUpdate;
			memcpy(sss->packet, outapp->pBuffer, sizeof(GroupUpdate_Struct));
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;

			pack = new ServerPacket(ServerOP_GroupRefresh, sizeof(ServerGroupRefresh_Struct));
			memset(pack->pBuffer, 0, pack->size);
			ServerGroupRefresh_Struct* sgr = (ServerGroupRefresh_Struct*) pack->pBuffer;			
			strncpy(sgr->member, Member->GetName(), MAX_NAME_SIZE);
			sgr->gid = this->GetGroupID();
			sgr->action = REMOVE_MEMBER;
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
	safe_delete(outapp);//delete outapp;

	//7. Check for out-of-zone group members to force a group refresh to group entity in their zone
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if ( !it->Ptr ) {
			ServerPacket* pack = new ServerPacket(ServerOP_GroupRefresh, sizeof(ServerGroupRefresh_Struct));
			memset(pack->pBuffer, 0, pack->size);
			ServerGroupRefresh_Struct* sgr = (ServerGroupRefresh_Struct*) pack->pBuffer;			
			strncpy(sgr->member, it->Name, MAX_NAME_SIZE);
			sgr->gid = this->GetGroupID();
			sgr->action = REMOVE_MEMBER;
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
}

//////////////////////////////////////////////////////////////////////

void Group::DisbandGroup() {
	this->DisbandGroup(false);
}

//////////////////////////////////////////////////////////////////////

void Group::DisbandGroup(bool newleader) {
	
	if ( this->ignoreNextDisband )
	{	// If this is set, it's because the leader used the #makeleader command.  
		// When the former leader rejoins the new group, the client sends a disband request, 
		// so we want to ignore that
		this->ignoreNextDisband = false;
		//cout << "Ignored disband request." << endl;
		return;
	}

	//1. Prepare packet.
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
	memset(outapp->pBuffer,0,outapp->size);
	GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;
	gu->action = GROUP_QUIT;

	//2. Send it to the members in zone.
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (!newleader)
			Database::Instance()->SetGroupID(it->Name, 0);
		if (it->Ptr){
			strncpy(gu->receiverName,it->Ptr->GetName(),MAX_NAME_SIZE);
			it->Ptr->QueuePacket(outapp);
			if (!newleader)
				memset(it->Ptr->GetPlayerProfilePtr()->GroupMembers, 0, sizeof(it->Ptr->GetPlayerProfilePtr()->GroupMembers));
		}
		else if(!it->Ptr) {	//Member not in zone, send packet by world.
			ServerPacket* pack = new ServerPacket(ServerOP_SendPacket, sizeof(ServerSendPacket_Struct) + sizeof(GroupUpdate_Struct));
			memset(pack->pBuffer, 0, pack->size);
			ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*) pack->pBuffer;
			strncpy(sss->charname, it->Name, MAX_NAME_SIZE);
			sss->opcode = OP_GroupUpdate;
			memcpy(sss->packet, outapp->pBuffer, sizeof(GroupUpdate_Struct));
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
			
			pack = new ServerPacket(ServerOP_GroupRefresh, sizeof(ServerGroupRefresh_Struct));
			memset(pack->pBuffer, 0, pack->size);
			ServerGroupRefresh_Struct* sgr = (ServerGroupRefresh_Struct*) pack->pBuffer;			
			strncpy(sgr->member, it->Name, MAX_NAME_SIZE);
			sgr->action = GROUP_QUIT;
			worldserver.SendPacket(pack);
			safe_delete(pack);//delete pack;
		}
	}
	safe_delete(outapp);//delete outapp;

	if (!newleader)
		entity_list.RemoveEntity(this->GetID());
}

//////////////////////////////////////////////////////////////////////

TNumMember Group::GetMembersInZone() {
	TNumMember n = 0;
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (it->Ptr) n++;
	}
	return n;
}

//////////////////////////////////////////////////////////////////////

TNumMember Group::GetMembersInGroup() {
	return (TNumMember) this->pvMembers.size();
}

//////////////////////////////////////////////////////////////////////

Client* Group::GetLeader() {
	return this->pvMembers.front().Ptr;
}

//////////////////////////////////////////////////////////////////////

bool Group::IsLeader(char* member) {
	return ( stricmp( this->pvMembers.front().Name, member ) == 0 );
}

//////////////////////////////////////////////////////////////////////

void Group::RefreshPlayerProfiles() {
	vector<TMember>::iterator it;
	int8 n = 0;
	char Names[6][48];
	memset(Names, 0, sizeof(Names));
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++)
		strncpy(Names[n++], it->Name, sizeof(Names[0]));
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (it->Ptr)
			memcpy(it->Ptr->GetPlayerProfilePtr()->GroupMembers, Names, sizeof(Names));
	}
}


//////////////////////////////////////////////////////////////////////

void Group::SplitExp(sint16 NPCLevel, int16 zoneExpModifier)
{
	//Yeahlight: Pulling below calculations from http://strategywiki.org/wiki/EverQuest/Frequently_Asked_Questions
	TNumMember NumMembers = this->GetMembersInZone();
	int16 totalGroupLevel = this->GetTotalLevel();
	int32 baseMobExp = NPCLevel * NPCLevel * zoneExpModifier;
	int32 expReward = 0;

	//Yeahlight: Break down the total exp for each group member
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++)
	{
		if (it->Ptr && MemberGetsXP(it->Ptr))
		{
			expReward = baseMobExp * (it->Ptr->GetLevel() + 5)/(totalGroupLevel + ((int)NumMembers * 5));
			it->Ptr->AddEXP(expReward);
		}
	}
}

//////////////////////////////////////////////////////////////////////

bool Group::MemberGetsXP(Client* member){
	return true;
	int8 level = member->GetLevel();
	int8 maxlevelingroup = this->GetMaxLevel();

	if(level <20){
		if((maxlevelingroup - level) <= 5){
			return true;
		}else{
			return false;
		}
	} else {
		if((maxlevelingroup - level) <= (level + 5 + (level - 20)/5)){
			return true;
		}else{
			return false;
		}
	}
}

//////////////////////////////////////////////////////////////////////

TNumMember Group::GetMaxLevel() {
	TNumMember Max = 0;
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (it->Ptr) 
			Max = it->Ptr->GetLevel() > Max ? it->Ptr->GetLevel() : Max;
	}
	return Max;
}

//////////////////////////////////////////////////////////////////////

int16 Group::GetTotalLevel() {
	int16 T = 0;
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (it->Ptr) 
			T += it->Ptr->GetLevel();
	}
	return T;
}

//////////////////////////////////////////////////////////////////////

int16 Group::GetTotalLevelXP()
{
	bool debugFlag = true;
	int16 T = 0;
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (it->Ptr && MemberGetsXP(it->Ptr)) 
			T += it->Ptr->GetLevel();
		else if (it->Ptr && debugFlag && it->Ptr->IsClient() && it->Ptr->CastToClient()->GetDebugMe())
			it->Ptr->Message(RED, "Debug: Your level is not enough to get exp in this group");
	}
	return T;
}

//////////////////////////////////////////////////////////////////////
/* Relays a group chat message to all members with proper language. 
* The sender sends one message PER group member, so they are just relayed. */
//////////////////////////////////////////////////////////////////////
void Group::GroupMessage(Client* sender,char* target,int8 language, char* message){
	if(!message) return;
	Client* c = entity_list.GetClientByName(target);
	if(this->IsMemberInZone(target) && c) {
		c->ChannelMessageSend(sender->GetName(), target,2,language,message);

		// Harakiri skillups of language				
		sender->TeachLanguage(c,language,MessageChannel_GSAY);
	}
	else { //not in zone.
		ServerPacket* pack = new ServerPacket(ServerOP_ChannelMessage, sizeof(ServerChannelMessage_Struct) + strlen(message) + 1);
		ServerChannelMessage_Struct* scm = (ServerChannelMessage_Struct*) pack->pBuffer;
		memset(pack->pBuffer, 0, pack->size);
		strncpy(scm->from, sender->GetName(), MAX_NAME_SIZE);
		strncpy(scm->to, target, MAX_NAME_SIZE);
		strncpy(scm->deliverto, target, MAX_NAME_SIZE);
		strncpy(scm->message, message, strlen(message) + 1);
		scm->noreply = 1;
		scm->language = language;
		scm->fromadmin = 0;
		scm->guilddbid = 0;
		scm->chan_num = 2;
		worldserver.SendPacket(pack);
		safe_delete(pack);//delete pack;
	}
}



//////////////////////////////////////////////////////////////////////

/********************************************************************  
*                          Tazadar - 7/18/08                        *  
*********************************************************************  
*                            SplitMoney							    *  
*********************************************************************  
*  + Split the money to the group								    *  
*											                        *  
*********************************************************************/  
//////////////////////////////////////////////////////////////////////
void Group::SplitMoney(uint32 copper, uint32 silver, uint32 gold, uint32 platinum, Client *splitter) {
	if(copper == 0 && silver == 0 && gold == 0 && platinum == 0) //avoid unneeded work
		return;

	int8 MembersInZone = this->GetMembersInZone();
	if (MembersInZone < 1) 
		return;


	uint32 mod;
	mod = platinum % MembersInZone;
	cout<<"Splitting Money !"<<endl;
	if((mod) > 0) {
		platinum -= mod;
		gold += 10 * mod;
	}
	mod = gold % MembersInZone;
	if((mod) > 0) {
		gold -= mod;
		silver += 10 * mod;
	}
	mod = silver % MembersInZone;
	if((mod) > 0) {
		silver -= mod;
		copper += 10 * mod;
	}

	//calculate the splits
	uint32 sc;
	uint32 cpsplit = copper / MembersInZone;
	sc = copper   % MembersInZone;
	uint32 spsplit = silver / MembersInZone;
	uint32 gpsplit = gold / MembersInZone;
	uint32 ppsplit = platinum / MembersInZone;

	APPLAYER* outapp = new APPLAYER(OP_SplitResponse,sizeof(Split_Struct));
	memset(outapp->pBuffer,0,sizeof(Split_Struct));
	Split_Struct* split = (Split_Struct*)outapp->pBuffer;
	split->platinum=ppsplit;
	split->gold=gpsplit;
	split->silver=spsplit;
	split->copper=cpsplit;

	//cout<<"After the split everybody gets :platinum = " <<(int)ppsplit<< " gold = "<<(int)gpsplit << " silver = " <<(int)spsplit << " copper = " <<(int)cpsplit <<endl;
	vector<TMember>::iterator it;

	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		Client* c = it->Ptr;
		if (c) {
			if(c == splitter){
				APPLAYER* outapp2 = new APPLAYER(OP_SplitResponse,sizeof(Split_Struct));
				memset(outapp2->pBuffer,0,sizeof(Split_Struct));
				Split_Struct* split = (Split_Struct*)outapp2->pBuffer;
				split->platinum=ppsplit;
				split->gold=gpsplit;
				split->silver=spsplit;
				split->copper=cpsplit+sc;
				c->AddMoneyToPP(cpsplit+sc, spsplit, gpsplit, ppsplit, false);
				c->QueuePacket(outapp2);
				safe_delete(outapp2); 
			}
			else{
				c->QueuePacket(outapp);
				c->AddMoneyToPP(cpsplit, spsplit, gpsplit, ppsplit, false);
			}
		}
	}
		safe_delete(outapp);
}

//////////////////////////////////////////////////////////////////////

bool Group::IsGroupMember(Client* client) {
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (it->Ptr && it->Ptr == client) return true;
		else if (!it->Ptr && stricmp(it->Name,client->GetName()) == 0 ) return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////

bool Group::IsGroupMember(char* member) {
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (stricmp(it->Name,member) == 0) return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
/* Will send spell to each member of the group. */
//////////////////////////////////////////////////////////////////////
void Group::CastGroupSpell(Client* caster, Spell* spell){
	vector<TMember>::iterator it;
	//Melinko: Added check for proper spell ranges on each member of group
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (it->Ptr){
			/* Melinko: Needed to add specific check for Bard Songs, as the beneficial group songs have 0.0 as range, 
			   used AE range as range, which is range that a member should be within for the song to land*/
			if(spell->GetSpellTargetType() == ST_GroupSong && it->Ptr->DistNoZ(caster) <= spell->GetSpellAOERange()){
				caster->SpellOnTarget(spell, it->Ptr);
			}else if(it->Ptr->DistNoZ(caster) <= spell->GetSpellRange()){
				caster->SpellOnTarget(spell, it->Ptr);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////

void Group::MemberZonedOut(Client* zoner){
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (it->Ptr && it->Ptr == zoner)
			it->Ptr = NULL;
	}

	// Delete group entity for this zone if no one is left.
	if ( this->GetMembersInGroup() == 0 )
		entity_list.RemoveEntity(this->GetID());
}

//////////////////////////////////////////////////////////////////////

void Group::MemberZonedIn(Client* zoner){
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (strcmp(it->Name, zoner->GetName()) == 0)
			it->Ptr = zoner;
	}
	this->RefreshPlayerProfiles();
}

//////////////////////////////////////////////////////////////////////

bool Group::AddMember(char* name) {
	this->pvMembers.push_back(TMember(name));
	return true;
}

//////////////////////////////////////////////////////////////////////

bool Group::LoadFromDB(int GID) {
	cout << "Loading group from DB, ID: " << GID << endl;
	this->pvMembers.clear();
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (Database::Instance()->RunQuery(query,MakeAnyLenString(&query, "SELECT name FROM character_ WHERE GID=%lu ORDER BY groupleader DESC,name", GID),errbuf,&result)){
		safe_delete_array(query);
		if(mysql_num_rows(result) < 1) {	//could prolly be 2
			mysql_free_result(result);
			cout << "Error getting group members for group " << GID << endl;
			return false;
		}
		int i = 0;
		while((row = mysql_fetch_row(result))) {
			if(!row[0])
				continue;
			this->AddMember(row[0]);
			i++;
		}
		mysql_free_result(result);
	}

	// Kibanu: Update pointers, if no one in the group is in the zone, let's just delete the group
	Client* client;
	bool memberinzone = false;
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		client = entity_list.GetClientByName(it->Name);
		if (client != 0)
		{
			memberinzone = true;
			it->Ptr = client;
		}
	}
	if (memberinzone)
		return true;
	else
		return false;
}

//////////////////////////////////////////////////////////////////////

bool Group::IsMemberInZone(char* member){
	if(!member) return false;
	vector<TMember>::iterator it;
	for (it = this->pvMembers.begin(); it < this->pvMembers.end(); it++) {
		if (strcmp(it->Name, member) == 0)
			return (it->Ptr != 0);
	}
	return false;
}

//Yeahlight: Compiles a list of the clients in the group
queue<char*> Group::BuildGroupList()
{
	queue<char*> ret;
	vector<TMember>::iterator it;
	for(it = this->pvMembers.begin(); it < this->pvMembers.end(); it++)
	{
		if(it->Ptr)
		{
			ret.push(it->Ptr->GetName());
		}
	}
	return ret;
}