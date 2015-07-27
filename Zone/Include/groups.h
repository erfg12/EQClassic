#ifndef GROUPS_H
#define GROUPS_H

#include <vector>
#include "eq_opcodes.h"
#include "eq_packet_structs.h"
#include "entity.h"
#include "client.h"

#define	ADD_MEMBER		0
#define NEW_LEADER		1
#define	REMOVE_MEMBER	3
#define	GROUP_QUIT		4


typedef int8 TNumMember;

class Group: public Entity
{
public:
	char*			GetName()	{ return NULL; }
	virtual bool	Process()	{ return true; }
	virtual bool	IsGroup()	{ return true; }

	Group::~Group() {}
	Group::Group(Client* leader);
	Group::Group(int GID);

	/*
	Client*			GetMemberPtr(TMembers i);
	bool			SetMemberPtr(Client* member, TMembers i);
	bool			SetMemberName(char* name, TMembers i);
	char*			GetMemberName(TMembers i);
	TMembers		GetCurrentMembersTotal();
	TMembers		GetCurrentMembersZone();
	void			ClearGroupData();
	bool			SetMember(Client* member, TMembers i);
	void			RefreshPlayerProfiles();
	*/
	
	void			ResetGroupData();
	bool			AddMember(Client* member);
	bool			AddMember(char* name);
	void			DelMember(char* name, bool ignoresender = false, bool leftgame = false);
	void			DisbandGroup();
	void			DisbandGroup(bool newleader);
	Client*			GetLeader();
	bool			IsLeader(char* member);
	void			ChangeLeader(char* member, bool leftgame = false);
	void			SplitExp(sint16 NPCLevel, int16 zoneExpModifier = 75);
	void			GroupMessage(Client* sender, char* target, int8 language, char* message);
	void			SplitMoney(uint32 copper, uint32 silver, uint32 gold, uint32 platinum, Client *splitter);
	bool			IsGroupMember(Client* client);
	bool			IsGroupMember(char* member);
	void			CastGroupSpell(Client* caster, Spell* spell);
	void			MemberZonedOut(Client* zoner);
	void			MemberZonedIn(Client* zoner);
	bool			IsMemberInZone(char* member);
	TNumMember		GetMembersInZone();
	TNumMember		GetMembersInGroup();
	bool			LoadFromDB(int GID);
	void			SetGroupID(int gid); //Kibanu
	int				GetGroupID(); //Kibanu
	queue<char*>	BuildGroupList();

	//vector<TMember>* CheckVectorPtr() { return pvMembersPtr*; }

	const static TNumMember	MAX_GROUP_MEMBERS = 6;

private:
	enum InsertPosition {
		FIRST = 0,		// 0 will be leader position, always.
		LAST  = 1
	};
	struct TMember 
	{
		TMember (Client* c) {
			this->Ptr = c;
			memset(this->Name, 0, sizeof(this->Name));
			strncpy(this->Name, c->GetName(), sizeof(this->Name));
		}
		TMember (char* n) {
			this->Ptr = NULL;
			memset(this->Name, 0, sizeof(this->Name));
			strncpy(this->Name, n, sizeof(this->Name));
		}
		Client*	Ptr;
		char	Name[48];

	};
	bool					ignoreNextDisband;
	const static int8		MAX_NAME_SIZE = 15;
	vector<TMember>			pvMembers;
	//vector<TMember>*		pvMembersPtr;
	const static TNumMember	LEADER_POSITION = 0;
	
	int32		GroupID;

	bool		AddMember(Client* member, InsertPosition pos);
	bool		AddMember(Client* member, InsertPosition pos, bool newleader);
	void		RefreshPlayerProfiles();
	bool		MemberGetsXP(Client* member);
	TNumMember	GetMaxLevel();
	int16		GetTotalLevel();
	int16		GetTotalLevelXP();
	
};

#endif