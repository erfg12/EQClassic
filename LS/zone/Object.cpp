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
#include <iostream.h>
#include <stdlib.h>
#include "object.h"
#include "entity.h"
#include "client.h"
#include "../common/database.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"

extern Database database;
extern EntityList entity_list;

Object::Object(int16 type,int16 ItemID,float YPos,float XPos, float ZPos, int8 Heading, char ObjectName[6],int8 Charges) {
	itemid = ItemID;
	charges = Charges;
	dropid = rand()%ItemID;
	ypos = YPos;
	xpos = XPos;
	zpos = ZPos;
	heading = Heading;
	memcpy(objectname,"IT73",16);
	objecttype = type;
	memset(bagitems,0,sizeof(bagitems));
	memset(bagcharges,0,sizeof(bagcharges));
}

Object::~Object() {}

void Object::CreateSpawnPacket(APPLAYER* app){
	app->opcode = OP_CreateObject;
	app->pBuffer = new uchar[sizeof(Object_Struct)];
	//memset(app->pBuffer,0,sizeof(Object_Struct));
	app->size = sizeof(Object_Struct);	
	Object_Struct* co = (Object_Struct*) app->pBuffer;		
	co->dropid = this->GetID();
	const Item_Struct * spawnitem = database.GetItem(this->itemid);
	co->charges=charges;
	char tmp[20];
	uchar blah4[14];
	for(int i=0;i<=13;i++)
		blah4[i]=0x00;
	blah4[6]=0x41;
	blah4[7]=0x43;
	blah4[8]=0x54;
	blah4[9]=0x4F;
	blah4[10]=0x52;
	blah4[11]=0x44;
	blah4[12]=0x45;
	blah4[13]=0x46;
	memset(tmp,0x0,20);
	strn0cpy(tmp,spawnitem->idfile,strlen(spawnitem->idfile)+1);
	int blah2[20];
	for(int j=0;j<=13;j++){
		blah2[j]=tmp[j];
		blah4[j]=blah2[j];
	}
	co->xpos = this->xpos;
	co->ypos = this->ypos;
	co->zpos = this->zpos;
	memcpy(co->idFile,blah4,16);
	co->itemid = this->itemid;
	memcpy(co->objectname,this->objectname,16);
	co->heading = this->heading;
}
void Object::CreateDeSpawnPacket(APPLAYER* app){	
	app->opcode = OP_ClickObject;
	app->pBuffer = new uchar[sizeof(ClickObject_Struct)];
	app->size = sizeof(ClickObject_Struct);	
	ClickObject_Struct* co = (ClickObject_Struct*) app->pBuffer;		
	co->objectID = GetID();
	co->PlayerID = 0;
	co->unknown = 0;
}

bool Object::HandleClick(Client* sender, const APPLAYER* app){
	int i;
	
	switch(objecttype)
	{
	case OT_DROPPEDITEM:
		sender->SummonItem(this->itemid,charges);
		entity_list.QueueClients(0,app,false);
		
		for (i = 0;i < 10;i++){
			if (this->bagitems[i] != 0){
				sender->PutItemInInventory(330 + i, this->bagitems[i], bagcharges[i]);
			}
		}
		entity_list.RemoveEntity(this->GetID());
		return true;
	case 0:
			sender->Message(0,"Giving you a toy to play with....");
			sender->SummonItem(this->itemid,charges);
			return true;
	default:
		return false;
		
	}
}
