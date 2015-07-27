// ***************************************************************
//  Boat   -  date : 07 june 2009 
//  Author : Tazadar
//  -------------------------------------------------------------
//  Copyright (C) 2009 - All Rights Reserved
// ***************************************************************
//	This class manages one boat
// ***************************************************************

#ifndef BOAT_H
#define BOAT_H

#include <vector>
#include <map>
#include <iostream>
#include "mutex.h"
#include "servertalk.h"

namespace EQC
{
	namespace World
	{
		class Boat
		{
		public:
			Boat(char* name);
			~Boat();
			
			void	Add(char* playername);
			void	Remove(char* playername);
			void	ZoneBootUp(char* zonename);
			void	ZoneShutDown(char* zonename);
			void	Process(void);
			void	travelFinished();
			void	setFamily(const std::vector <Boat*> family){this->family = family;}; // Tazadar : We set the family of the boat
			void	setFamilyReady(){familiyReady = true; }; // Tazadar : When the first boat of family goes he set this to true
			bool	IsReadyToContinue(){ return readyToGo; }; // Tazadar : This boat is ready to continue ?

		private:
			Mutex zoneModification;
			Mutex passengerModification;

			bool ZoneIsOnline(string zone); // Tazadar : useful to know if the zone is crashed/offline
			void RemoveBoat(char* zonename); // Tazadar : despawn the boat
			void SpawnBoat(char* zonename,float x,float y,float z,float heading); // Tazadar : Spawn a boat
			void TeleportPlayers(char* zonename,char* targetzonename,float x,float y,float z,float heading); // Tazadar : Teleport players to the next zone
			void StartZone(char* zonename); // Tazadar : Used to start a new zone
			void GoTo(char* zonename,int16 from,int16 to); // Tazadar : Used to move the boat (it follow the route until the numnode)
			bool familyIsReady(); // Tazadar : Is my family ready to continue ?
			
			std::vector <string> clients; // Tazadar : list of clients which are on the boat
			string zone; // Tazadar : the current zone name of the boat
			std::vector <string> route_zones; // Tazadar : the route zones.
			std::map<string,bool> zone_avaible; // Tazadar : zone is running or not?
			std::vector <WorldBoatCommand_Struct> route_nodes; // Tazadar :  World node of the route
			string boatname; // Tazadar : name of the boat
			string currentZone; // Tazadar : the current zone where the boat is
			bool getBoatPath(); // Tazadar : load the path of the boat
			int16 currentnode; // Tazadar : Current node in world path
			int32 nextsteptime; // Tazadar : Time when the next step begins
			bool moving; // Tazadar : Boat is moving?
			bool bootup; // Tazadar : Boot up phase?
			bool ready; // Tazadar : Is boat ready?
			bool pathloaded; // Tazadar : Path has been loaded?
			bool waiting; // Tazadar : Is boat waiting ?
			bool zonedown; // Tazadar : Is zone down ?
			std::vector <Boat*> family; // Tazadar : Family of this boat
			bool familiyReady; // Tazadar : Is family ready?
			bool readyToGo; // Tazadar : Am I ready to continue my route?
			int timer;

		};
	}
}

#endif