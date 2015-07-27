#include "database.h"
#include "boats.h"
#include "client.h"
#include "eq_packet_structs.h"
#include "packet_dump_file.h"
#include "mob.h"
#include "zone.h"
#include "npc.h"
#include <string.h>
#include <stdio.h>
#include "worldserver.h"

extern WorldServer		worldserver;
extern Database			database; //Thanks Sp0tter =p
extern Zone*			zone;


	/********************************************************************
	 *                        Wizzel - 6/23/08		                    *
	 *						  Tazadar - 6/10/09							*
	 ********************************************************************
	 *                       Boat Functions			                    *
	 ********************************************************************
	 *  + These functions inform the server that a player has boarded	*
	 *	  or left a ship. It adds their name along with an ID for the	*
	 *	  corresponding boat they have boarded.							*
	 *	+ Taz : Modified all functions, it talks now with the world serv*	
	 ********************************************************************
	 *                               TODO                               *
	 ********************************************************************
	 *  + Work this into the ship system w/pathing						*
	 ********************************************************************/

void Client::ProcessOP_GetOnBoat(APPLAYER* pApp){
	/*	No need for a packet size check. The size is in relation to the size
	of the name of the boat.*/

	// Cast the struct
	Boat_Struct* boatstruct = (Boat_Struct*) pApp->pBuffer;
	
	// Tazadar : Make the packet for the World Serv
	ServerPacket* pack = new ServerPacket(ServerOP_BoatNP, sizeof(ServerBoat_Struct));
	
	ServerBoat_Struct* servBoatInfos = (ServerBoat_Struct*) pack->pBuffer;

	// Tazadar : Fill the packet
	memcpy(servBoatInfos->player_name,this->GetName(),sizeof(servBoatInfos->player_name));
	memcpy(servBoatInfos->boatname,boatstruct->boatname,NPC_MAX_NAME_LENGTH);

	// Tazadar : Send the packet
	worldserver.SendPacket(pack);

	return;
}

	/////////////////////////////////////////////////////////////////////

void Client::ProcessOP_GetOffBoat(APPLAYER* pApp){

	// Cast the struct
	Boat_Struct* boatstruct = (Boat_Struct*) pApp->pBuffer;
	
	// Tazadar : Make the packet for the World Serv
	ServerPacket* pack = new ServerPacket(ServerOP_BoatPL, sizeof(ServerBoat_Struct));
	
	ServerBoat_Struct* servBoatInfos = (ServerBoat_Struct*) pack->pBuffer;

	// Tazadar : Fill the packet
	memcpy(servBoatInfos->player_name,this->GetName(),sizeof(servBoatInfos->player_name));

	// Tazadar : Send the packet
	worldserver.SendPacket(pack);

	return;
}

	/********************************************************************
	 *                        Wizzel - 6/24/08		                    *
	 ********************************************************************
	 *                       Controlling a Boat		                    *
	 ********************************************************************
	 *  + This is recieved when a player right clicks on a small boat	*
	 ********************************************************************
	 *                               TODO                               *
	 ********************************************************************
	 *  - Fix camera lock (maybe classic) and boats not moving forwards	*
	 *	  or backwards. I think this is related to the boat not being   *
	 *	  in the correct place. I will do some more work on this later.	*
	 ********************************************************************/

void Client::ProcessOP_CommandBoat(APPLAYER* pApp){
	
	//FileDumpPacketHex("boatcontrol.txt", pApp);

	this->QueuePacket(pApp);
	
	return;
}