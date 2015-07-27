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
#ifndef GROUPS_H
#define GROUPS_H

#include "../common/types.h"
#include "../common/linked_list.h"
#include "../common/eq_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "entity.h"
#include "mob.h"

#define MAX_GROUP_MEMBERS 6

class Group: public Entity
{
public:
	Group::~Group() {}
	Group::Group(Mob* leader);
	bool	AddMember(Mob* newmember);
	bool	DelMember(Mob* oldmember,bool ignoresender = false);
	void	DisbandGroup();
	bool	IsGroupMember(Mob* client);
	bool	Process();
	bool	IsGroup()			{ return true; }
	void	CastGroupSpell(Mob* caster,uint16 spellid);
	void	SplitExp(uint32 exp, int16);
	void	GroupMessage(Mob* sender,const char* message);
	int32	GetTotalGroupDamage(Mob* other);
	Mob* members[MAX_GROUP_MEMBERS];

	int8	GroupCount();

	void	TeleportGroup(Mob* sender, int32 zoneID, float x, float y, float z);

	bool	disbandcheck;
	bool	castspell;

private:

};

#endif
