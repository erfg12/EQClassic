// ***************************************************************
//  BoatsManager   -  date : 07 june 2009 
//  Author : Tazadar
//  -------------------------------------------------------------
//  Copyright (C) 2009 - All Rights Reserved
// ***************************************************************
// This class manages all the boats, this is a Singleton
// ***************************************************************

#ifndef BOATSMANAGER_H
#define BOATSMANAGER_H

#include "Boat.h"
#include "config.h"

namespace EQC
{
	namespace World
	{
		class BoatsManager
		{
		public:

			static BoatsManager * getInst(); // Tazadar : Gives the instance
			static void	KillInst(); // Tazadar : Kills the instance
			void	StartThread(); // Tazadar : Starts the thread
			void	KillThread(); // Tazadar : Kills the thread
			Boat ** getBoats(){return boats;}; // Tazadar : Return the list of boats
			void	ZoneBootUp(char* zonename); // Tazadar : Warn boats when a zone boots up
			void	ZoneShutDown(char* zonename); // Tazadar : Warn boats when a zone shuts down
			void	ZoneStateChange(char* oldname,char* newname); // Tazadar : Warn boats when a zone state changes
			void	removePassenger(char* playername); // Tazadar : Remove passenger from a boat
			void	addPassenger(char* playername,char* boatname); // Tazadar : Add passenger to a boat
			void	travelFinished(char* boatname); // Tazadar : Travel is Finished
		
		private:

			BoatsManager();
			~BoatsManager();
			Boat * boats[TOTAL_BOATS]; // Tazadar : List of boats
			static BoatsManager * inst; // Tazadar : Singleton instance
			string boatsNames[TOTAL_BOATS]; // Tazadar : Names of boats
			int getBoatId(char* boatname); // Tazadar : Gives the boat Id 
			
			// Tazadar : Our process which runs in a thread
			#ifdef WIN32
			static void Process(void* tmp);
			#else
			static void* Process(void* tmp);
			#endif
		};
	}
}

#endif