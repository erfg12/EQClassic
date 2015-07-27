// ***************************************************************
//  BoatsManager   -  date : 07 june 2009 
//  Author : Tazadar
//  -------------------------------------------------------------
//  Copyright (C) 2009 - All Rights Reserved
// ***************************************************************
//  This class manages all the boats
// ***************************************************************


#include "BoatsManager.h"
#include "types.h"
#include "client.h"

volatile bool BoatLoopRunning = true;
bool threadkilled = false;

namespace EQC
{
	namespace World
	{
		BoatsManager* BoatsManager::inst = 0;// initialize pointer


		BoatsManager::BoatsManager() 
		{
			boatsNames[0] = "Golden_Maiden00";
			boatsNames[1] = "Sabrina00";
			boatsNames[2] = "Sea_King00";
			char temp[NPC_MAX_NAME_LENGTH];
			for(int i = 0 ; i < TOTAL_BOATS ;i++){
				memcpy(temp,boatsNames[i].c_str(),NPC_MAX_NAME_LENGTH);
				boats[i] = new Boat(temp);
			}
			std::vector <Boat*> family;
			// Tazadar : We hardcode the family no need to have a file.
			
			// Tazadar : Qeynos - Erudin Route
			family.push_back(boats[0]);
			family.push_back(boats[2]);
			boats[1]->setFamily(family);
			family.clear();
			family.push_back(boats[1]);
			family.push_back(boats[2]);
			boats[0]->setFamily(family);
			family.clear();
			family.push_back(boats[0]);
			family.push_back(boats[1]);
			boats[2]->setFamily(family);
			family.clear();

			// Tazadar : End of Qeynos - Erudin Route
		}

		BoatsManager::~BoatsManager()
		{
			KillThread();

			while(threadkilled == false);

			for(int i=0;i<TOTAL_BOATS;i++){
				safe_delete(boats[i]);
			}
		}
		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *							 getInst	                            *
		 ********************************************************************
		 *  + Used to get the instance of BM							    *
		 ********************************************************************/
		BoatsManager * BoatsManager::getInst(){
			if(inst == 0)
				inst = new BoatsManager();
			return inst;
		};
		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *							 KillInst                               *
		 ********************************************************************
		 *  + Our destructor !											    *
		 ********************************************************************/
		void BoatsManager::KillInst(){
			delete inst ;
			inst = 0 ;
		}
		#ifdef WIN32
		void BoatsManager::Process(void* tmp){
		#else
		void* BoatsManager::Process(void* tmp){
		#endif
			while(BoatLoopRunning == true){
				for(int i=0;i<TOTAL_BOATS;i++){
					BoatsManager::getInst()->boats[i]->Process();
				}
				Sleep(1);
			}
			threadkilled = true;
			#ifdef WIN32
			_endthread();
			#endif
		}

		/********************************************************************
		 *                      Tazadar - 06/22/09                          *
		 ********************************************************************
		 *							 StartThread                            *
		 ********************************************************************
		 *  + Start the thread which manages boats						    *
		 ********************************************************************/
		void BoatsManager::StartThread()
		{
			
#ifdef WIN32
				_beginthread(Process, 0, NULL);
#else
				pthread_t thread;
				pthread_create(thread, NULL, &Process, NULL);
#endif
		}
		/********************************************************************
		 *                      Tazadar - 06/17/09                          *
		 ********************************************************************
		 *							 getBoatPath                            *
		 ********************************************************************
		 *  + Kill the thread											    *
		 ********************************************************************/
		void BoatsManager::KillThread()
		{
			BoatLoopRunning = false;
		}

		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *							 ZoneBootUp                             *
		 ********************************************************************
		 *  + Warns boats when a zone boots up							    *
		 ********************************************************************/
		void BoatsManager::ZoneBootUp(char* zonename)
		{
			//std::cout << "ZoneBootUp : adding zone" << zonename <<std::endl;
			for(int i=0;i<TOTAL_BOATS;i++){
				boats[i]->ZoneBootUp(zonename);
			}
		}
		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *							 ZoneShutDown                           *
		 ********************************************************************
		 *  + Warns boats when a zone shuts down						    *
		 ********************************************************************/	
		void BoatsManager::ZoneShutDown(char* zonename)
		{
			//std::cout << "ZoneShutDown : removing zone" << zonename <<std::endl;
			for(int i=0;i<TOTAL_BOATS;i++){
				boats[i]->ZoneShutDown(zonename);
			}
		}
		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *						 ZoneStateChange		                    *
		 ********************************************************************
		 *  + Warns boats when a zone state changes						    *
		 ********************************************************************/
		void BoatsManager::ZoneStateChange(char* oldname,char* newname){
			//std::cout << "ZoneStateChange : " << oldname << newname <<std::endl;
			if(strcmp(oldname,newname) == 0){
				return;
			}
			else if(oldname[0] == 0){
				ZoneBootUp(newname);
			}
			else if(newname[0] == 0){
				ZoneShutDown(oldname);
			}
			else{
				std::cerr << "ZoneStateChange : unknown operation !" << std::endl;
			}

		}
		/********************************************************************
		 *                      Tazadar - 06/23/09                          *
		 ********************************************************************
		 *							 addPassenger                           *
		 ********************************************************************
		 *  + Add passenger to boat vector								    *
		 ********************************************************************/
		void BoatsManager::addPassenger(char* playername,char* boatname){
			int id = getBoatId(boatname);
			if(id != -1){
				boats[id]->Add(playername);
			}
			else{
				std::cerr << "BoatsManager::addPassenger error : wrong boat id " << endl;
			}

		}
		/********************************************************************
		 *                      Tazadar - 06/23/09                          *
		 ********************************************************************
		 *						  removePassenger	                        *
		 ********************************************************************
		 *  + Remove passenger from boat vector							    *
		 ********************************************************************/
		void BoatsManager::removePassenger(char* playername){
			for(int i = 0 ; i < TOTAL_BOATS ;i++){
				boats[i]->Remove(playername);
			}
		}
		/********************************************************************
		 *                      Tazadar - 06/12/09                          *
		 ********************************************************************
		 *							 getBoatId                              *
		 ********************************************************************
		 *  + Gives the boat Id											    *
		 ********************************************************************/
		int BoatsManager::getBoatId(char* boatname){
			for(int i = 0 ; i < TOTAL_BOATS ;i++){
				if(boatsNames[i] == boatname)
					return i;
			}
			return -1;
		}
		/********************************************************************
		 *                      Tazadar - 06/22/09                          *
		 ********************************************************************
		 *							 travelFinished                         *
		 ********************************************************************
		 *  + Sends a message to a boat when travel finished    		    *
		 ********************************************************************/
		void BoatsManager::travelFinished(char* boatname){
			int id = getBoatId(boatname);
			if(id != -1){
				boats[id]->travelFinished();
			}
			else{
				std::cerr << "BoatsManager::travelFinished error : wrong boat id " << endl;
			}
		}
	}
}