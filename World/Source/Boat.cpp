// ***************************************************************
//  Boat   -  date : 07 june 2009 
//  Author : Tazadar
//  -------------------------------------------------------------
//  Copyright (C) 2009 - All Rights Reserved
// ***************************************************************
//  This class manages one boat
// ***************************************************************

#include "zoneserver.h"
#include "ZSList.h"
#include "Boat.h"
#include "Database.h"

extern EQC::World::Network::ZSList zoneserver_list;

namespace EQC
{
	namespace World
	{
		Boat::Boat(char* name) 
		{
			boatname = name;
			Database::Instance()->LoadBoatZones(name,route_zones);

			if(getBoatPath()){
				currentZone = route_nodes[0].zonename;
				pathloaded = true;
			}
			else
				pathloaded = false;
			if(route_zones.size() == 1) // Tazadar : Boat lives in only one zone
				currentnode = 1;
			else 
				currentnode = 0;
			moving = false;
			bootup = false;
			ready = false;
			nextsteptime = 0;
			waiting = false;
			zonedown = true;
			familiyReady = false;
			readyToGo = false;
		}

		Boat::~Boat()
		{
		}
	   /********************************************************************
		*                      Tazadar - 06/12/09                          *
		********************************************************************
		*                             Add		                           *
		********************************************************************
		*  + Add a client to the list						               *
		********************************************************************/
		void Boat::Add(char* playername){
			LockMutex lock(&passengerModification);
			clients.push_back(playername);
		}
	   /********************************************************************
		*                      Tazadar - 06/12/09                          *
		********************************************************************
		*                             Remove	                           *
		********************************************************************
		*  + Remove a client from the list					               *
		********************************************************************/
		void Boat::Remove(char* playername){
			string name = playername;
			LockMutex lock(&passengerModification);
			std::vector <string> temp;
			std::vector <string>::iterator it;
			for(it=clients.begin();it!=clients.end();it++){
				if(*it != name)
					temp.push_back(*it);
			}
			clients=temp;
		}
	   /********************************************************************
		*                      Tazadar - 06/12/09                          *
		********************************************************************
		*                          ZoneBootUp		                       *
		********************************************************************
		*  + When a zone boots up we check if it interests us              *
		********************************************************************/

		void Boat::ZoneBootUp(char* zonename){
			LockMutex* lock = new LockMutex(&zoneModification);
			bool interested = false;

			for(int i=0;i<route_zones.size();i++){
				if(route_zones[i] == zonename){
					interested = true;
					break;
				}
			}
			if(interested){
				zone_avaible[zonename] = true;
				string tmp = zonename;
				if(currentZone == tmp){
					bootup = true;
					ready = true;
					zonedown = false;
					if(route_zones.size() == 1){
						SpawnBoat(zonename,route_nodes[0].position.x,route_nodes[0].position.y,route_nodes[0].position.z,route_nodes[0].position.heading);
					}
				}
				safe_delete(lock);
			}

		}
	    /********************************************************************
		 *                      Tazadar - 06/14/09                          *
		 ********************************************************************
		 *                          RemoveBoat		                        *
		 ********************************************************************
		 *  + Remove a boat from a zone						                *
		 ********************************************************************/
		void Boat::RemoveBoat(char* zonename){
			ServerPacket* pack = new ServerPacket(ServerOP_RemoveBoat,sizeof(BoatName_Struct));
			BoatName_Struct * tmp =(BoatName_Struct *) pack->pBuffer;
			strcpy(tmp->boatname,boatname.c_str());
			ZoneServer* zs = zoneserver_list.FindByName(zonename);
			if(zs != 0)
				zs->SendPacket(pack);
			else
				std::cerr << "RemoveBoat : ZoneServer error, cant find the zone : " << zonename  << " " << strlen(zonename) << std::endl;

			safe_delete(pack);
		}
		/********************************************************************
		 *                      Tazadar - 06/16/09                          *
		 ********************************************************************
		 *                          SpawnBoat		                        *
		 ********************************************************************
		 *  + Spaw a boat in a zone							                *
		 ********************************************************************/
		void Boat::SpawnBoat(char* zonename,float x,float y,float z,float heading){
			std::cout << "Spawning boat " << this->boatname << " to Zone " << zonename ;
			ServerPacket* pack = new ServerPacket(ServerOP_SpawnBoat,sizeof(SpawnBoat_Struct));
			SpawnBoat_Struct* tmp =(SpawnBoat_Struct*) pack->pBuffer;
			strcpy(tmp->boatname,boatname.c_str());
			tmp->x=x;
			tmp->y=y;
			tmp->z=z;
			tmp->heading=heading;
			ZoneServer* zs = zoneserver_list.FindByName(zonename);
			if(zs != 0){
				zs->SendPacket(pack);
				bootup = false;
				zonedown = false;
			}
			else if(bootup){
				while(zs == 0){
					zs = zoneserver_list.FindByName(zonename);
				}
				bootup = false;
				zonedown = false;
			}
			else
			{
				std::cerr << "SpawnBoat : ZoneServer error, cant find the zone : " << zonename  << " " << strlen(zonename) << std::endl;
				zonedown = true;
			}
			safe_delete(pack);
		}

		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *                          ZoneShutDown                            *
		 ********************************************************************
		 *  + When a zone shut down we change the flag		                *
		 ********************************************************************/
		void Boat::ZoneShutDown(char* zonename){
			LockMutex* lock= new LockMutex(&zoneModification);
			bool interested = false;

			for(int i=0;i<route_zones.size();i++){
				if(route_zones[i]==zonename){
					interested=true;
					break;
				}
			}
			if(interested){
				zone_avaible[zonename]=false;
				safe_delete(lock);
				string tmp = zonename;
				if(currentZone == tmp){
					bootup = false;
					zonedown = true;
				}
			}
		}
		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *                          Process		                            *
		 ********************************************************************
		 *  + The boat Process								                *
		 ********************************************************************/
		void Boat::Process(void){
			
			if(pathloaded == false) // Tazadar : Boat not implented yet
				return;

			if(ready == false) // Tazadar : Boat is not ready
				return;

			if(moving == true){ // Tazadar : Boat is following its route in the zone (or in the world if zone crashed)
				if(zonedown){
					moving = false;
				}
				else
					return;
			}

			if(readyToGo == true){// Tazadar : We are waiting for the family
				if(familyIsReady()){
					readyToGo = false;
					familiyReady = false;
				}
				else 
					return;
			}

			if(waiting == true){ // Tazadar : Boat is waiting at dock/zone
				if(nextsteptime > time(0)){
					return;
				}
				else{
					waiting = false;
				}
			}

			if(currentnode == route_nodes.size()){
				if(route_zones.size() == 1) // Tazadar : Boat lives in only one zone
					currentnode = 1;
				else 
					currentnode = 0;
			}
				
			switch (route_nodes[currentnode].cmd){
				case SPAWN:
					{
						if((zone_avaible[route_nodes[currentnode].zonename] == true)){
							SpawnBoat((char*)route_nodes[currentnode].zonename.c_str(),route_nodes[currentnode].position.x,route_nodes[currentnode].position.y,route_nodes[currentnode].position.z,route_nodes[currentnode].position.heading);
						}
						else
							zonedown = true;
						currentnode++;
						break;
					}
				case GOTO:
					{
						//cout << "Trying to move the boat zonedown is " << zonedown << endl;
						//if(clients.size() == 1){
							if(!zonedown)
								GoTo((char*)route_nodes[currentnode].zonename.c_str(),route_nodes[currentnode].go_to.from_node,route_nodes[currentnode].go_to.to_node);
							nextsteptime = time(0) + route_nodes[currentnode].go_to.timeneeded;
							moving = true;
							currentnode++;
							//timer = time(0);
						//}
						break;
					}
				case TELEPORT_PLAYERS:
					{
						TeleportPlayers((char*)currentZone.c_str(),(char*)route_nodes[currentnode].zonename.c_str(),route_nodes[currentnode].position.x,route_nodes[currentnode].position.y,route_nodes[currentnode].position.z,route_nodes[currentnode].position.heading);
						if(currentZone != route_nodes[currentnode].zonename) // Tazadar : If its the first spawn we do not want to remove the boat !
							RemoveBoat((char*)currentZone.c_str());
						currentZone=route_nodes[currentnode].zonename;
						currentnode++;
						break;
					}
				case WAIT:
					{
						nextsteptime = time(0) + route_nodes[currentnode].go_to.timeneeded;
						waiting = true;
						currentnode++;
						break;
					}
				case CHECK_FAMILY:
					{
						readyToGo = true;
						currentnode++;
						break;
					}
			}
		}
		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *                          ZoneIsOnline                            *
		 ********************************************************************
		 *  + Check if the zone is online					                *
		 ********************************************************************/
		bool Boat::ZoneIsOnline(string zone){
			LockMutex lock(&zoneModification);
			if(zone_avaible[zone]==true){
				return true;
			}
			return false;
		}
		/********************************************************************
		 *                      Tazadar - 06/17/09                          *
		 ********************************************************************
		 *                          TeleportPlayers                         *
		 ********************************************************************
		 *  + Send a packet to zone player									*
		 *  + The packet is variable, please check the little trick ^^		*
		 ********************************************************************/
		void Boat::TeleportPlayers(char* zonename,char* targetzonename,float x,float y,float z,float heading){
			if(clients.size() > 0) {
				int size = sizeof(ZoneBoat_Struct)+clients.size()*30*sizeof(char);
				std::cout << "size of packet" << size << std::endl;
				ServerPacket* pack = new ServerPacket(ServerOP_ZoneBoat,size);
				ZoneBoat_Struct* tmp =(ZoneBoat_Struct*) pack->pBuffer;
				strcpy(tmp->zonename,targetzonename);
				strcpy(tmp->boatname,(char*)boatname.c_str());
				tmp->x=x;
				tmp->y=y;
				tmp->z=z;
				tmp->heading=heading;
				tmp->numberOfPlayers=clients.size();
				char * tmp2 = (char *) pack->pBuffer;
				for(int i = 0 ; i < clients.size() ; i ++){
					memcpy(tmp2+sizeof(ZoneBoat_Struct)+30*i,(char*)clients[i].c_str(),30*sizeof(char));
				}
				ZoneServer* zs = zoneserver_list.FindByName(zonename);
				if(zs != 0)
					zs->SendPacket(pack);
				else
					std::cerr << "TeleportPlayers : ZoneServer error, cant find the zone : " << zonename  << " " << strlen(zonename) << std::endl;

				safe_delete(pack);
				//clients.clear();
			}
		}
		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *                          StartZone                               *
		 ********************************************************************
		 *  + Start a zone if needed						                *
		 ********************************************************************/
		void Boat::StartZone(char* zonename){

			ZoneServer* zs = zoneserver_list.FindByName(zonename);

			if (zs)
			{
				// if we can get the zone server, notifiy world we're comming, so it knows not to shutdown
				zs->TriggerBootup();
			}
			else
			{

				std::cout << "Attempting autobootup of '" << zonename << "' for the boat : " << this->boatname << std::endl;

				if (!zoneserver_list.TriggerBootup(zonename))
				{
					// We cant boot up
					std::cout << "Error: No zoneserver to bootup '" << zonename << "' for the boat : " << this->boatname << std::endl;
				}
			}
		}
		/********************************************************************
		 *                      Tazadar - 06/17/09                          *
		 ********************************************************************
		 *							 GoTo	                                *
		 ********************************************************************
		 *  + Used to move the boat (it follow the route until the numnode) *
		 ********************************************************************/
		void Boat::GoTo(char* zonename,int16 from,int16 to){

			ServerPacket* pack = new ServerPacket(ServerOP_BoatGoTo,sizeof(BoatGoTo_Struct));
			BoatGoTo_Struct* tmp =(BoatGoTo_Struct*) pack->pBuffer;
			strcpy(tmp->boatname,boatname.c_str());
			tmp->fromNode=from;
			tmp->toNode=to;
			ZoneServer* zs = zoneserver_list.FindByName(zonename);
			if(zs != 0)
				zs->SendPacket(pack);
			else
				std::cerr << "SpawnBoat : ZoneServer error, cant find the zone : " << zonename  << " " << strlen(zonename) << std::endl;

			safe_delete(pack);
		}
		/********************************************************************
		 *                      Tazadar - 06/17/09                          *
		 ********************************************************************
		 *							 getBoatPath                            *
		 ********************************************************************
		 *  + Used to get the world path of a boat						    *
		 ********************************************************************/
		bool Boat::getBoatPath()
		{
			char fileNameSuffix[] = ".txt";
			char fileNamePrefix[200] = "./Maps/Boats/World/";
			strcat(fileNamePrefix, boatname.c_str());
			strcat(fileNamePrefix, fileNameSuffix);

			ifstream myfile(fileNamePrefix);

			WorldBoatCommand_Struct tmp;
			int tmp2;

			if (myfile.is_open())
			{
				while (! myfile.eof() )
				{
					memset(&tmp,0,sizeof(WorldBoatCommand_Struct));
					myfile >> tmp2;
					tmp.cmd = (BOATCOMMANDS)tmp2;
					if(tmp.cmd == CHECK_FAMILY) {
						//Nothing to add
					}
					else if(tmp.cmd == SPAWN) {
						myfile >> tmp.zonename >> tmp.position.x >> tmp.position.y >> tmp.position.z >> tmp.position.heading;
					}
					else if(tmp.cmd == TELEPORT_PLAYERS) {
						myfile >> tmp.zonename >> tmp.position.x >> tmp.position.y >> tmp.position.z >> tmp.position.heading;
					}
					else if(tmp.cmd == GOTO) {
						myfile >> tmp.zonename >> tmp.go_to.from_node >> tmp.go_to.to_node >>tmp.go_to.timeneeded;
					}
					else if(tmp.cmd == WAIT) {
						myfile >> tmp.go_to.timeneeded;
					}
					else {
						std::cout << " Boat File corrupted : " << fileNamePrefix << std::endl;
						myfile.close();
						route_nodes.clear();
						return false;
					}
					route_nodes.push_back(tmp);
				}
			}
			else{
				std::cout << "Boat File not opened :" << fileNamePrefix << std::endl;
				return false;
			}

			/*for(int i = 0; i < route_nodes.size() ; i++){
				std::cout << (int)route_nodes[i].cmd << " " << route_nodes[i].zonename << " " << route_nodes[i].position.x << " " << route_nodes[i].position.y << " " << route_nodes[i].position.z << " " << route_nodes[i].position.heading <<" "<<route_nodes[i].go_to.from_node <<" "<<route_nodes[i].go_to.to_node  << " " <<route_nodes[i].go_to.timeneeded <<  std::endl; 
			}*/

			myfile.close();
			return true;
		}

		void Boat::travelFinished(){
			moving = false;
			//std::cout << this->boatname << " time waited "<< time(0)-timer << std::endl;
		}

		/********************************************************************
		 *                      Tazadar - 07/01/09                          *
		 ********************************************************************
		 *							 gfamilyIsReady                         *
		 ********************************************************************
		 *  + Is my family ready to continue ?							    *
		 ********************************************************************/
		bool Boat::familyIsReady(){
			if(familiyReady)
				return true;

			for(int i=0 ; i < family.size() ; i++){
				if(!family[i]->IsReadyToContinue())
					return false;
			}
			//Tazadar : We are the first one to go we set the family boolean to true

			for(int i=0 ; i < family.size() ; i++){
				family[i]->setFamilyReady();
			}

			return true;
		}
	}
}
