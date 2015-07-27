// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstdarg>

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <errno.h>
	#include <pthread.h>
	#include "unix.h"

	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
	extern int errno;
#endif

#include "servertalk.h"
#include "eq_packet_structs.h"
#include "packet_dump.h"
#include "database.h"
#include "classes.h"
#include "races.h"
#include "seperator.h"
#include "zoneserver.h"
#include "ClientListEntry.h"
#include "client.h"
#include "LoginServer.h"
#include "ZSList.h"
#include "ClientList.h"
#include "ConsoleList.h"
#include "EQCUtils.hpp"
#include "config.h"
#include "World.h"
#include "WorldGuildManager.h"
#include "BoatsManager.h"
using namespace std;

extern EQC::World::LoginServer* loginserver;

int8 zoneStatusList[TOTAL_NUMBER_OF_ZONES];

// 15/10/2007 - froglok23
//TODO: MAKE SURE EACH METHOD HAS PACKET SIZE CHECKING!!!!
// Will come, but for now im off to bed :P - froglok23
void ZoneServer::ProcessServerOP_ChannelMessage(ServerPacket* pack)
{
	if (pack->size >= sizeof(ServerChannelMessage_Struct))
	{
		ServerChannelMessage_Struct* scm = (ServerChannelMessage_Struct*) pack->pBuffer;

		if (scm->chan_num == 7) // 7 = tell
		{
			ClientListEntry* cle = zoneserver_list.FindCharacter(scm->deliverto);
			if (cle == 0) 
			{
				if (!scm->noreply)
					zoneserver_list.SendEmoteMessage(scm->from, 0, 0, "You told %s, '%s is not online at this time'", scm->to, scm->to);
			}
			else if (cle->TellsOff() && ((cle->Anon() == 1 && scm->fromadmin < cle->Admin()) || scm->fromadmin < 100))
			{
				if (!scm->noreply)
					zoneserver_list.SendEmoteMessage(scm->from, 0, 0, "You told %s, '%s is not online at this time'", scm->to, scm->to);
			}
			else if (cle->Server() == 0)
			{
				if (!scm->noreply)
					zoneserver_list.SendEmoteMessage(scm->from, 0, 0, "You told %s, '%s is not contactable at this time'", scm->to, scm->to);
			}
			else
				cle->Server()->SendPacket(pack);
		}
		else if(scm->chan_num == 2) { //Cofruben: for grouping.
			ClientListEntry* cle = zoneserver_list.FindCharacter(scm->deliverto);
			if(cle && cle->Server())
				cle->Server()->SendPacket(pack);
		}
		else
			zoneserver_list.SendPacket(pack);
	}
	else
		cout << "Wrong size OP_ChannelMessage" << pack->size << ", should be " << sizeof(ServerChannelMessage_Struct) << "+ on 0x" << hex << setfill('0') << setw(4) << pack->opcode << dec << endl;
}

// Kibanu: modified on 4/21/2009
void ZoneServer::ProcessServerOP_EmoteMessage(ServerPacket* pack)
{
	if (pack->size >= sizeof(ServerEmoteMessage_Struct))
	{
		ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;
		zoneserver_list.SendEmoteMessage(sem->to, sem->guilddbid, sem->type, sem->message);		
	}
	else
		cout << "Wrong size OP_EmoteMessage " << pack->size << ", should be " << sizeof(ServerEmoteMessage_Struct) << "+ on 0x" << hex << setfill('0') << setw(4) << pack->opcode << dec << endl;
}

// Kibanu: added on 4/21/2009
void ZoneServer::ProcessServerOP_GroupRefresh(ServerPacket* pack)
{
	if (pack->size <= sizeof(ServerGroupRefresh_Struct))
	{
		ServerGroupRefresh_Struct* gr = (ServerGroupRefresh_Struct*) pack->pBuffer;
		zoneserver_list.ForceGroupRefresh(gr->member, gr->gid, gr->action);
	}
	else
		cout << "Wrong size OP_GroupRefresh " << pack->size << ", should be " << sizeof(ServerGroupRefresh_Struct) << endl;
}

// Cofruben: used to send a packet directly to a client.
void ZoneServer::ProcessServerOP_SendPacket(ServerPacket* pack) {
	ServerSendPacket_Struct* sss = (ServerSendPacket_Struct*)pack->pBuffer;
	sss->charname[sizeof(sss->charname)-1] = '\0'; // Sanity check ;)
	ClientListEntry* cle = zoneserver_list.FindCharacter(sss->charname);
	if (cle == 0 || cle->Server() == 0) return;
	cle->Server()->SendPacket(pack);
	EQC::Common::PrintF(CP_WORLDSERVER, "ProcessServerOP_SendPacket(): Sending a packet to %s.\n", sss->charname);
}

void ZoneServer::ProcessServerOP_RezzPlayer(ServerPacket* pack)	
{
		    zoneserver_list.SendPacket(pack);
}

void ZoneServer::ProcessServerOP_MultiLineMsg(ServerPacket* pack)
{
	ServerMultiLineMsg_Struct* mlm = (ServerMultiLineMsg_Struct*) pack->pBuffer;
	zoneserver_list.SendPacket(mlm->to, pack);
}

void ZoneServer::ProcessServerOP_SetZone(ServerPacket* pack)
{
	char* zone = "";

	if (pack->size > 1) 
	{
		zone = (char*) &pack->pBuffer[0];
	}
	
	SetZone(zone);
}

//Yeahlight: Zone sent World a running verification
void ZoneServer::ProcessServerOP_ZoneRunStatus(ServerPacket* pack)
{
	if(pack->size == sizeof(ZoneStatus_Struct))
	{
		ZoneStatus_Struct* zs = (ZoneStatus_Struct*)pack->pBuffer;
		int16 zoneID = zs->zoneID;
		//Yeahlight: Let's move tutorial into spot 7 since it is free
		if(zoneID == 183)
			zoneStatusList[7] = 1;
		else
			zoneStatusList[zoneID] = 1;
	}
	else
	{
		cout << "Wrong size " << pack->size << ", should be " << sizeof(ZoneStatus_Struct) << "+ on 0x" << hex << setfill('0') << setw(4) << pack->opcode << dec << endl;
	} 
}

void ZoneServer::ProcessServerOP_SetConnectInfo(ServerPacket* pack)
{
	if(pack->size == sizeof(ServerConnectInfo))
	{
		ServerConnectInfo* sci = (ServerConnectInfo*) pack->pBuffer;
		SetConnectInfo(sci->address, sci->port);
	}
	else
	{
		cout << "Wrong size " << pack->size << ", should be " << sizeof(ServerConnectInfo) << "+ on 0x" << hex << setfill('0') << setw(4) << pack->opcode << dec << endl;
	} 
}

void ZoneServer::ProcessServerOP_ShutdownAll(ServerPacket* pack)
{
	zoneserver_list.SendPacket(pack);
	zoneserver_list.Process();
	CatchSignal(0);
}

void ZoneServer::ProcessServerServerOP_ZoneShutdown(ServerPacket* pack)
{
	ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
	ZoneServer* zs = 0;
	if (s->ZoneServerID != 0)
	{
		zs = zoneserver_list.FindByID(s->ZoneServerID);
	}
	else if (s->zonename[0] != 0)
	{
		zs = zoneserver_list.FindByName(s->zonename);
	}
	else
	{
		zoneserver_list.SendEmoteMessage(s->adminname, 0, 0, "Error: SOP_ZoneShutdown: neither ID nor name specified");
	}

	if (zs == 0)
	{
		zoneserver_list.SendEmoteMessage(s->adminname, 0, 0, "Error: SOP_ZoneShutdown: zoneserver not found");
	}
	else
	{
		zs->SendPacket(pack);
	}
}
void ZoneServer::ProcessServerOP_ZoneBootup(ServerPacket* pack)
{
	ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
	EQC::World::Network::ZSList::SOPZoneBootup(s->adminname, s->ZoneServerID, s->zonename);
}

void ZoneServer::ProcessServerOP_ZoneStatus(ServerPacket* pack)
{
	if (pack->size >= 1)
	{
		zoneserver_list.SendZoneStatus((char *) &pack->pBuffer[1], (int8) pack->pBuffer[0], this);
	}
}

void ZoneServer::ProcessServerOP_ClientList(ServerPacket* pack)
{
	ServerClientList_Struct* scl = (ServerClientList_Struct*) pack->pBuffer;

	if (scl->remove)
	{
		zoneserver_list.ClientRemove(scl->name, scl->zone);
	}
	else
	{
		zoneserver_list.ClientUpdate(this, scl->name, scl->AccountID, scl->AccountName, scl->Admin, scl->zone, scl->level, scl->class_, scl->race, scl->anon, scl->tellsoff, scl->guilddbid, scl->guildeqid, scl->LFG);
	}
}

void ZoneServer::ProcessServerOP_Who(ServerPacket* pack)
{
	ServerWhoAll_Struct* whoall = (ServerWhoAll_Struct*) pack->pBuffer;
	Who_All_Struct* whom = new Who_All_Struct;
	memset(whom,0,sizeof(Who_All_Struct));
	whom->firstlvl = whoall->firstlvl;
	whom->gmlookup = whoall->gmlookup;
	whom->secondlvl = whoall->secondlvl;
	whom->wclass = whoall->wclass;
	whom->wrace = whoall->wrace;
	strcpy(whom->whom,whoall->whom);
	zoneserver_list.SendWhoAll(whoall->from, whoall->admin, whom, this);
	safe_delete(whom);//delete whom;
}

void ZoneServer::ProcessServerOP_ZonePlayer(ServerPacket* pack)
{
	ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
	zoneserver_list.SendPacket(pack);
}

void ZoneServer::ProcessServerOP_Motd(ServerPacket* pack)
{
	if (pack->size == sizeof(SetServerMotd_Struct)) 
	{
		SetServerMotd_Struct* smotd = (SetServerMotd_Struct*) pack->pBuffer;
		if(Database::Instance()->SetVariable("MOTD",smotd->motd))
		{
			this->SendEmoteMessage(smotd->myname, 0, 13, "Updated Motd.");
		}
		else
		{
			cout << "Updating MOTD database variable has failed!" << endl;
		}
	}
	else
	{
		cout << "Wrong size on SetServerMotd_Struct. Got: " << pack->size << ", Expected: " << sizeof(SetServerMotd_Struct) << endl;
	}
}

void ZoneServer::ProcessServerOP_Uptime(ServerPacket* pack)
{
	if (pack->size != sizeof(ServerUptime_Struct)) 
	{
		cout << "Wrong size on ServerOP_Uptime. Got: " << pack->size << ", Expected: " << sizeof(ServerUptime_Struct) << endl;
	}
	else
	{
		ServerUptime_Struct* sus = (ServerUptime_Struct*) pack->pBuffer;
		if (sus->zoneserverid == 0) 
		{
			int32 ms = Timer::GetCurrentTime();
			
			int32 days = ms / 86400000;
			ms -= days * 86400000;
			
			int32 hours = ms / 3600000;
			ms -= hours * 3600000;
			
			int32 minuets = ms / 60000;
			ms -= minuets * 60000;
			
			int32 seconds = ms / 1000;
			
			if (days)
			{
				this->SendEmoteMessage(sus->adminname, 0, 0, "Worldserver Uptime: %02id %02ih %02im %02is", days, hours, minuets, seconds);
			}
			else if (hours)
			{
				this->SendEmoteMessage(sus->adminname, 0, 0, "Worldserver Uptime: %02ih %02im %02is", hours, minuets, seconds);
			}
			else
			{
				this->SendEmoteMessage(sus->adminname, 0, 0, "Worldserver Uptime: %02im %02is", minuets, seconds);
			}
		}
		else 
		{
			ZoneServer* zs = zoneserver_list.FindByID(sus->zoneserverid);
			if (zs)
			{
				zs->SendPacket(pack);
			}
		}
	}
}

void ZoneServer::ProcessServerOP_Unknown(ServerPacket* pack)
{
	cout << "Unknown ServerOPcode:" << (int)pack->opcode << " size:" << pack->size << endl;
	DumpPacket(pack->pBuffer, pack->size);
}

void ZoneServer::ProcessServerOP_GMGoto(ServerPacket* pack)
{
	if (pack->size != sizeof(ServerGMGoto_Struct)) 
	{
		cout << "Wrong size on ServerOP_GMGoto. Got: " << pack->size << ", Expected: " << sizeof(ServerGMGoto_Struct) << endl;
		return;
	}
	ServerGMGoto_Struct* gmg = (ServerGMGoto_Struct*) pack->pBuffer;
	ClientListEntry* cle = zoneserver_list.FindCharacter(gmg->gotoname);
	if (cle != 0) 
	{
		if (cle->Server() == 0)
		{
			this->SendEmoteMessage(gmg->myname, 0, 13, "Error: Cannot identify %s's zoneserver.", gmg->gotoname);
		}
		else if (cle->Anon() == 1 && cle->Admin() > gmg->admin) // no snooping for anon GMs
		{
			this->SendEmoteMessage(gmg->myname, 0, 13, "Error: %s not found", gmg->gotoname);
		}
		else
		{
			cle->Server()->SendPacket(pack);
		}
	}
	else 
	{
		this->SendEmoteMessage(gmg->myname, 0, 13, "Error: %s not found", gmg->gotoname);
	}
}

void ZoneServer::ProcessServerOP_Lock(ServerPacket* pack)
{
	if (pack->size != sizeof(ServerLock_Struct)) 
	{
		cout << "Wrong size on ServerOP_Lock. Got: " << pack->size << ", Expected: " << sizeof(ServerLock_Struct) << endl;
		return;
	}
	ServerLock_Struct* slock = (ServerLock_Struct*) pack->pBuffer;
	if (slock->mode >= 1) 
	{
		net.world_locked = true;
	}
	else
	{
		net.world_locked = false;
	}

	if (loginserver == 0)
	{
		if (slock->mode >= 1)
		{
			this->SendEmoteMessage(slock->myname, 0, 13, "World locked, but login server not connected.");
		}
		else 
		{
			this->SendEmoteMessage(slock->myname, 0, 13, "World unlocked, but login server not conencted.");
		}
	}
	else
	{
		loginserver->SendStatus();
		if (slock->mode >= 1)
		{
			this->SendEmoteMessage(slock->myname, 0, 13, "World locked");
		}
		else
		{
			this->SendEmoteMessage(slock->myname, 0, 13, "World unlocked");
		}
	}
}


	/********************************************************************
	 *						  Tazadar - 6/10/09							*
	 ********************************************************************
	 *                       Boat Functions			                    *
	 ********************************************************************
	 *  + World Side functions											*
	 *	+ Add/Remove a player on a boa									*
	 *  + When the travel is done zone side the boat knows it			*
	 ********************************************************************/

void ZoneServer::ProcessServerOP_BoatNP(ServerPacket* pack){
	
	ServerBoat_Struct * tmp = (ServerBoat_Struct *)pack->pBuffer;
	
	EQC::World::BoatsManager::getInst()->addPassenger(tmp->player_name,tmp->boatname);	
	std::cout<< "Passenger " << tmp->player_name << " is now on boat " << tmp->boatname << std :: endl;

}

void ZoneServer::ProcessServerOP_BoatPL(ServerPacket* pack){
	
	
	ServerBoat_Struct * tmp = (ServerBoat_Struct *)pack->pBuffer;

	EQC::World::BoatsManager::getInst()->removePassenger(tmp->player_name);	
	std::cout<< "Passenger " << tmp->player_name << " left the boat " << tmp->boatname << std :: endl;

}

void ZoneServer::ProcessServerOP_TravelDone(ServerPacket* pack){
	
	
	BoatName_Struct * tmp = (BoatName_Struct *)pack->pBuffer;

	EQC::World::BoatsManager::getInst()->travelFinished(tmp->boatname);	
	std::cout<< "Boat " << tmp->boatname << " has finished its travel." << std :: endl;

}