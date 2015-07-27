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

#include "doors.h"
#include "entity.h"
#include "client.h"
#include "../common/database.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"

extern Database database;
extern EntityList entity_list;

Doors::Doors(const Door* door)
{
    db_id = door->db_id;
    door_id = door->door_id;
    strncpy(zone_name,door->zone_name,16);
    strncpy(door_name,door->door_name,16);
    pos_x = door->pos_x;
    pos_y = door->pos_y;
    pos_z = door->pos_z;
    heading = door->heading;
    opentype = door->opentype;
    guildid = door->guildid;
    lockpick = door->lockpick;
    keyitem = door->keyitem;
    trigger_door = door->trigger_door;
    trigger_type = door->trigger_type;
    liftheight = door->liftheight;
    isopen = false;

    close_timer = new Timer(10000);
    close_timer->Disable();
    
    strncpy(dest_zone,door->dest_zone,16);
    dest_x = door->dest_x;
    dest_y = door->dest_y;
    dest_z = door->dest_z;
    dest_heading = door->dest_heading;

}

Doors::~Doors()
{
    delete close_timer;
}

bool Doors::Process()
{
    if(close_timer && close_timer->Check() && isopen)
    {
        close_timer->Disable();
        isopen=false;
    }

	return true;
}

void Doors::HandleClick(Client* sender)
{
    if(DEBUG>=5) { 
        LogFile->write(EQEMuLog::Debug, "Doors:HandleClick(%s)", sender->GetName());
        DumpDoor();
    }
    if(trigger_type == 255)
        return;

        if(!isopen)
        {
                close_timer->Start();
                isopen=true;
        }
        else
        {
                close_timer->Disable();
                isopen=false;
        }
        if (opentype == 58){
                // Teleport door!
                sender->MovePC(dest_zone, dest_x, dest_y, dest_z);
                //MovePC(const char* zonename, float x, float y, float z, int8 ignorerestrictions = 0, bool summoned = false);
        }
}
void Doors::DumpDoor(){
    LogFile->write(EQEMuLog::Debug,
        "db_id:%i door_id:%i zone_name:%s door_name:%s pos_x:%f pos_y:%f pos_z:%f heading:%f",
        db_id, door_id, zone_name, door_name, pos_x, pos_y, pos_z, heading);
    LogFile->write(EQEMuLog::Debug,
        "opentype:%i guildid:%i lockpick:%i keyitem:%i trigger_door:%i trigger_type:%i liftheight:%i open:%s",
        opentype, guildid, lockpick, keyitem, trigger_door, trigger_type, liftheight, (isopen) ? "open":"closed");
    LogFile->write(EQEMuLog::Debug,
        "dest_zone:%s dest_x:%f dest_y:%f dest_z:%f dest_heading:%f",
        dest_zone, dest_x, dest_y, dest_z, dest_heading);
}

