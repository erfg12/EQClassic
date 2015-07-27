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
#ifndef OBJECT_H
#define OBJECT_H

#include "../common/types.h"
#include "../common/linked_list.h"
#include "../common/eq_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "client.h"
#include "mob.h"
#include "npc.h"
#include "entity.h"

#define OT_DROPPEDITEM	1

class Object: public Entity
{
public:
	Object(int16 type,int16 itemid,float ypos,float xpos, float zpos, int8 heading, char objectname[6],int8 Charges);
	~Object();
	bool	IsObject()			{ return true; }
	void	CreateSpawnPacket(APPLAYER* app);
	void	CreateDeSpawnPacket(APPLAYER* app);
	bool	HandleClick(Client* sender, const APPLAYER* app);
	void	SetBagItems(int16 slot, int16 item,int8 charges)	{bagitems[slot] = item;bagcharges[slot] = charges;}
	bool	Process()  { return true; }

	char	idfile[16];
private:
	int8	charges;
	int32	itemid;
	int32	dropid;
	float	ypos;
	float	xpos;
	float	zpos;
	int8	heading;
	char	objectname[16];
	int16	objecttype;
	int16	bagitems[10];
	int8	bagcharges[10];
};

#endif
