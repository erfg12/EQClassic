#include "../common/debug.h"
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

using namespace std;

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
	#include <process.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <errno.h>
	#include <stdlib.h>

	#include "../common/unix.h"

	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1

	extern int errno;
#endif

#include "../common/servertalk.h"
#include "LWorld.h"
#include "net.h"
#include "client.h"
#include "../common/eq_packet_structs.h"
#include "../common/packet_dump.h"
#include "../common/database.h"
#include "../common/classes.h"
#include "../common/races.h"
#include "login_opcodes.h"
#include "login_structs.h"

#ifdef WIN32
	#define snprintf	_snprintf
	//#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
#endif

#ifndef MINILOGIN
	extern Database database;
#endif
extern ClientList client_list;
extern NetConnection net;
extern LWorldList	world_list;
extern volatile bool RunLoops;

LWorld::LWorld(TCPConnection* in_con, bool in_OutgoingLoginUplink, int32 iIP, int16 iPort, bool iNeverKick) {
	Link = in_con;
	RemoteID = 0;
	LinkWorldID = 0;
	if (iIP)
		ip = iIP;
	else
		ip = in_con->GetrIP();
	if (iPort)
		port = iPort;
	else
		port = in_con->GetrPort();
	ID = world_list.GetNextID();
	pClientPort = 0;
	memset(account, 0, sizeof(account));
	memset(address, 0, sizeof(address));
	memset(worldname, 0, sizeof(worldname));
	status = -1;
	GreenName = false;
	accountid = 0;
	admin_id = 0;
	IsInit = false;
	kicked = false;
	pNeverKick = iNeverKick;
	pPlaceholder = false;
	pshowdown = false;
	pConnected = in_con->Connected();
	pReconnectTimer = 0;
	if (in_OutgoingLoginUplink) {
		pClientPort = port;
		ptype = Login;
		OutgoingUplink = true;
		if (net.GetLoginMode() == Mesh) {
			pReconnectTimer = new Timer(INTERSERVER_TIMER);
			pReconnectTimer->Trigger();
		}
	}
	else {
		ptype = UnknownW;
		OutgoingUplink = false;
	}

	struct in_addr  in;
	in.s_addr = GetIP();
	strcpy(address, inet_ntoa(in));
	isaddressip = true;

	num_players = 0;
	num_zones = 0;

}

LWorld::LWorld(int32 in_accountid, char* in_accountname, char* in_worldname, int32 in_admin_id, bool in_greenname) {
	pPlaceholder = true;
	Link = 0;
	ip = 0;
	port = 0;
	ID = world_list.GetNextID();
	pClientPort = 0;
	memset(account, 0, sizeof(account));
	memset(address, 0, sizeof(address));
	memset(worldname, 0, sizeof(worldname));
	status = -1;
	GreenName = in_greenname;
	accountid = in_accountid;
	admin_id = in_admin_id;
	IsInit = false;
	kicked = false;
	pNeverKick = false;
	pshowdown = true;
	RemoteID = 0;
	LinkWorldID = 0;
	OutgoingUplink = false;
	pReconnectTimer = 0;
	pConnected = false;

	ptype = World;
	strcpy(account, in_accountname);
	strcpy(worldname, in_worldname);

	strcpy(address, "none");

	isaddressip = true;

	num_players = 0;
	num_zones = 0;

}

LWorld::LWorld(TCPConnection* in_RemoteLink, int32 in_ip, int32 in_RemoteID, int32 in_accountid, char* in_accountname, char* in_worldname, char* in_address, sint32 in_status, int32 in_adminid, bool in_greenname, bool in_showdown, int8 in_authlevel, bool in_placeholder, int32 iLinkWorldID) {
	Link = in_RemoteLink;
	RemoteID = in_RemoteID;
	LinkWorldID = iLinkWorldID;
	ip = in_ip;
	port = 0;
	ID = world_list.GetNextID();
	pClientPort = 0;
	memset(account, 0, sizeof(account));
	memset(address, 0, sizeof(address));
	memset(worldname, 0, sizeof(worldname));
	status = in_status;
	GreenName = in_greenname;
	accountid = in_accountid;
	admin_id = in_adminid;
	IsInit = true;
	kicked = false;
	pNeverKick = false;
	pPlaceholder = in_placeholder;
	pshowdown = in_showdown;
	OutgoingUplink = false;
	pReconnectTimer = 0;
	pConnected = true;

	ptype = World;
	strcpy(account, in_accountname);
	strcpy(worldname, in_worldname);

	strcpy(address, in_address);

	isaddressip = false;

	num_players = 0;
	num_zones = 0;

}

LWorld::~LWorld() {
	if (ptype == World && RemoteID == 0) {
		if (net.GetLoginMode() != Mesh || (!pPlaceholder && IsInit)) {
			ServerPacket* pack = new ServerPacket(ServerOP_WorldListRemove, sizeof(int32));
			*((int32*) pack->pBuffer) = GetID();
			world_list.SendPacketLogin(pack);
			delete pack;
		}
	}

	if (Link != 0 && RemoteID == 0) {
		world_list.RemoveByLink(Link, 0, this);
		if (OutgoingUplink)
			delete Link;
		else
			Link->Free();
	}
	Link = 0;
	safe_delete(pReconnectTimer);
	if (RunLoops)
		world_list.RemovePeer(this);
}

bool LWorld::Process() {
	bool ret = true;
	if (Link == 0)
		return true;
	if (Link->Connected()) {
		if (!pConnected) {
			if (pReconnectTimer && net.GetLoginMode() == Mesh) {
struct in_addr  in;
in.s_addr = GetIP();
cout << this->GetID() << ": Peer connected: (out) " << inet_ntoa(in) << ":" << GetPort() << endl;
				world_list.PeerLoginConnected = true;
				ServerPacket* outpack = new ServerPacket(ServerOP_LSInfo, sizeof(ServerLSInfo_Struct));
				ServerLSInfo_Struct* lsi = (ServerLSInfo_Struct*) outpack->pBuffer;
				strcpy(lsi->protocolversion, EQEMU_PROTOCOL_VERSION);
				lsi->servertype = 3;
				strcpy(lsi->account, net.GetUplinkAccount());
				strcpy(lsi->password, net.GetUplinkPassword());
				snprintf(lsi->address, sizeof(lsi->address), "%i", net.GetPort());
				SendPacket(outpack);
				delete outpack;
				outpack = new ServerPacket(ServerOP_PeerConnect, sizeof(ServerLSPeerConnect));
				ServerLSPeerConnect* lspc = (ServerLSPeerConnect*) outpack->pBuffer;
				lspc->ip = 0;
				lspc->port = net.GetPort();
				SendPacket(outpack);
				delete outpack;
			}
			pConnected = true;
		}
	}
	else {
		pConnected = false;
		if (pReconnectTimer) {
			if (pReconnectTimer->Check() && Link->ConnectReady()) {
				pReconnectTimer->Start(pReconnectTimer->GetTimerTime() + (rand()%15000), false);
				sint8 tmp = world_list.ConnectPeer(this);
				if (tmp == -1) {
struct in_addr  in;
in.s_addr = GetIP();
cout << this->GetID() << ": Giving up on peer reconnect: (out) " << inet_ntoa(in) << ":" << GetPort() << endl;
					return false;
				}
				if (tmp == 1) {
struct in_addr  in;
in.s_addr = GetIP();
cout << this->GetID() << ": Attempting peer connect: " << inet_ntoa(in) << ":" << GetPort() << endl;
					Link->AsyncConnect(GetIP(), GetPort());
				}
			}
			return true;
		}
		return false;
	}
	if (RemoteID != 0)
		return true;
	ServerPacket* pack = 0;
	while (ret && (pack = Link->PopPacket())) {
//cout << "Recieved packet: 0x" << hex << setw(4) << setfill('0') << pack->opcode << dec << endl;
		switch(pack->opcode) {
		case 0:
			break;
		case ServerOP_KeepAlive: {
			// ignore this
			break;
		}
		case ServerOP_LSFatalError: {
			net.Uplink_WrongVersion = true;
			cout << this->GetID() << ": World server responded with FatalError." << endl;
			if (pack->size > 1) {
				cout << "Error message: '" << (char*) pack->pBuffer << "'" << endl;
			}
			ret = false;
			kicked = true;
			break;
		}
		case ServerOP_UsertoWorldReq: {
			UsertoWorldRequest_Struct* ustwr = (UsertoWorldRequest_Struct*) pack->pBuffer;
			if (ustwr->ToID) {
				LWorld* world = world_list.FindByID(ustwr->ToID);
				if (!world) {
					cout << "Error: ServerOP_UsertoWorldReq recieved without destination" << endl;
					break;
				}
				if (this->GetType() != Login) {
					cout << "Error: ServerOP_UsertoWorldReq recieved from non-login connection" << endl;
					break;
				}
				ustwr->FromID = this->GetID();
				world->SendPacket(pack);
			}
			break;
		}
		case ServerOP_UsertoWorldResp: {
			if (pack->size != sizeof(UsertoWorldResponse_Struct))
				break;

			UsertoWorldResponse_Struct* seps = (UsertoWorldResponse_Struct*) pack->pBuffer;
			if (seps->ToID) {
				LWorld* world = world_list.FindByID(seps->ToID);
				if (world) {
					seps->ToID = world->GetRemoteID();
					world->SendPacket(pack);
				}
			}
			else {
				Client* client = 0;
				client = client_list.FindByLSID(seps->lsaccountid);
				if(client == 0)
					break;
				if(this->GetID() != seps->worldid && this->GetType() != Login)
					break;

				client->WorldResponse(seps->worldid,seps->response);
			}
			break;
							}
		case ServerOP_LSInfo: {
			if (pack->size != sizeof(ServerLSInfo_Struct)) {
				cout<<"Pack size: "<<pack->size<<endl;
				ServerLSInfo_Struct* lsi = (ServerLSInfo_Struct*) pack->pBuffer;
				DumpPacket(pack);
				this->Kick(ERROR_BADVERSION);
				ret = false;
				struct in_addr  in;
				in.s_addr = GetIP();
				cout << "Bad Version (wrong size): #" << GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
			}
			else {
				ServerLSInfo_Struct* lsi = (ServerLSInfo_Struct*) pack->pBuffer;
				if (strcmp(lsi->protocolversion, EQEMU_PROTOCOL_VERSION) != 0) {
					cout<<lsi->protocolversion<<endl;
					this->Kick(ERROR_BADVERSION);
					ret = false;
					struct in_addr  in;
					in.s_addr = GetIP();
					cout << "Bad Version (proto): #" << GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
				}
#ifndef MINILOGIN
#ifndef PUBLICLOGIN
				//else if (lsi->servertype == 0 && !database.CheckWorldVerAuth(lsi->serverversion)) {
				//	this->Kick(ERROR_BADVERSION);
				//	ret = false;
				//	struct in_addr  in;
				//	in.s_addr = GetIP();
				//	cout << "Bad Version (soft): #" << GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
				//}
#endif
#endif
				if (lsi->servertype == 1) {
					if (!SetupChat(lsi->account, lsi->password)) {
						this->Kick(ERROR_BADPASSWORD);
						ret = false;
						struct in_addr  in;
						in.s_addr = GetIP();
						cout << "Bad Chat Password: #" << GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
					}
				}
				else if (lsi->servertype == 2 || lsi->servertype == 3) {
					if (lsi->servertype == 2 && net.GetLoginMode() != Master) {
						this->Kick(ERROR_NOTMASTER);
						ret = false;
						struct in_addr  in;
						in.s_addr = GetIP();
						cout << "Login tried to connect, but i'm not a Master server: #" << GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
					}
					else if (lsi->servertype == 3 && net.GetLoginMode() != Mesh) {
						this->Kick(ERROR_NOTMESH);
						ret = false;
						struct in_addr  in;
						in.s_addr = GetIP();
						cout << "Login tried to connect, but i'm not a Mesh server: #" << GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
					}
					else if (!SetupLogin(lsi->account, lsi->password, lsi->address)) {
						this->Kick(ERROR_BADPASSWORD);
						ret = false;
						struct in_addr  in;
						in.s_addr = GetIP();
						cout << "Bad Login Password: #" << GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
					}
					else {
						world_list.UpdateWorldList(this);
						ServerPacket* outpack = new ServerPacket(ServerOP_TriggerWorldListRefresh);
						world_list.SendPacketLogin(outpack);
						delete outpack;
					}
				}
				else if (lsi->servertype != 0) {
					this->Kick(ERROR_UnknownServerType);
					ret = false;
					struct in_addr  in;
					in.s_addr = GetIP();
					cout << "Unknown Server Type: #" << GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
				}
#ifndef MINILOGIN
				else if (strcasecmp(lsi->name, "Unnamed server") == 0 || strcasecmp(lsi->name, "Server Name Here") == 0) {
					this->Kick(ERROR_UNNAMED);
					ret = false;
				}
#endif
				else if (lsi->name[sizeof(lsi->name)-1] != 0) {
					this->Kick(ERROR_BADNAME);
					ret = false;
				}
				else if (!CheckServerName(lsi->name)) {
					cout<<lsi->name<<endl;
					struct in_addr  in;
					in.s_addr = GetIP();
					cout << "Server failed name check: " << inet_ntoa(in) << ":" << GetPort() << endl;
					this->Kick(ERROR_BADNAME);
					ret = false;
				}
				else if (strstr(lsi->name, "Server") != 0) {
					this->Kick(ERROR_BADNAME_SERVER);
					ret = false;
				}
				else if (!SetupWorld(lsi->name, lsi->address, lsi->account, lsi->password, 0)) {
					this->Kick(ERROR_BADPASSWORD);
					ret = false;
					struct in_addr  in;
					in.s_addr = GetIP();
					cout << "Bad Password: #" << GetID() << " " << inet_ntoa(in) << ":" << GetPort() << endl;
				}
				else {
					UpdateWorldList();
				}
			}
			break;
		}
		case ServerOP_LSStatus: {
			ServerLSStatus_Struct* lss = (ServerLSStatus_Struct*) pack->pBuffer;

			if(lss->num_players > 5000 || lss->num_zones > 500) {
				//cout<<lss->num_players<<endl;
				//cout<<lss->num_zones<<endl;
				//cout<<lss->status<<endl;
				//DumpPacket(pack);
				//this->Kick("Your server has exceeded a number of players and/or zone limit.");
				//ret = false;
				//break;
			}

			UpdateStatus(lss->status, lss->num_players, lss->num_zones);
			break;
		}
		case ServerOP_SystemwideMessage: {
			if (this->GetType() == Login) {
				// no looping plz =p
				//world_list.SendPacket(pack, this);
			}
			else if (this->GetType() == Chat) {
				world_list.SendPacket(pack);
			}
			else {
				cout << "Error: ServerOP_SystemwideMessage from a non-login/chat connection? WTF!" << endl;
			}
//cout << "ServerOP_SystemwideMessage received, err, wtf?" << endl;
			break;
		}
		case ServerOP_PeerConnect: {
			if (this->GetType() != Login) {
				cout << "Error: ServerOP_PeerConnect from a non-login connection? WTF!" << endl;
				break;
			}
			if (net.GetLoginMode() != Mesh) {
				cout << "Error: ServerOP_PeerConnect from when not in Mesh mode? WTF!" << endl;
				break;
			}
			if (pack->size != sizeof(ServerLSPeerConnect)) {
				cout << "Wrong size on ServerOP_PeerConnect. Got: " << pack->size << ", Expected: " << sizeof(ServerLSPeerConnect) << endl;
				break;
			}
			ServerLSPeerConnect* lspc = (ServerLSPeerConnect*) pack->pBuffer;
			if (lspc->ip)
				world_list.AddPeer(lspc->ip, lspc->port);
			else {
				lspc->ip = this->GetIP();
				world_list.SendPacketLogin(pack, this);
			}
			break;
		}
		case ServerOP_ListWorlds: {
			if (pack->size <= 1 || pack->pBuffer[pack->size - 1] != 0) {
				cout << "Error: ServerOP_ListWorlds: Pack too short or corrupt." << endl;
				break;
			}
			world_list.SendWorldStatus(this, (char*) pack->pBuffer);
			break;
		}
//		case ServerOP_LSClientAuth:
		case ServerOP_WorldListUpdate: {
			if (this->GetType() != Login) {
				cout << "Error: ServerOP_WorldListUpdate from a non-login connection? WTF!" << endl;
				break;
			}
			if (pack->size != sizeof(ServerSyncWorldList_Struct)) {
				cout << "Wrong size on ServerOP_WorldListUpdate. Got: " << pack->size << ", Expected: " << sizeof(ServerSyncWorldList_Struct) << endl;
				break;
			}
			ServerSyncWorldList_Struct* sswls = (ServerSyncWorldList_Struct*) pack->pBuffer;
			if (!CheckServerName(sswls->name)) {
				struct in_addr  in;
				in.s_addr = sswls->ip;
				cout << "Server (from linked login) failed name check: " << inet_ntoa(in) << endl;
				break; // Someone needs to tell the other login server to update it's exe!
			}
			LWorld* world = world_list.FindByLink(this->Link, sswls->RemoteID);
			if (world) {
				world->SetRemoteInfo(sswls->ip, sswls->accountid, sswls->account, sswls->name, sswls->address, sswls->status, sswls->adminid, sswls->greenname, sswls->num_players, sswls->num_zones);
			}
			else {
				world = world_list.FindByAccount(sswls->accountid, World);
				if (world == 0 || sswls->placeholder == false) {
					if (world) {
#ifdef _DEBUG
						cout << "Kick(" << world->GetID() << ") in ServerOP_WorldListUpdate" << endl;
#endif
						world->Kick();
					}
					world = new LWorld(this->Link, sswls->ip, sswls->RemoteID, sswls->accountid, sswls->account, sswls->name, sswls->address, sswls->status, sswls->adminid, sswls->greenname, sswls->showdown, sswls->authlevel, sswls->placeholder, this->GetID());
					world_list.Add(world);
				}
			}
			sswls->RemoteID = world->GetID();
			if (net.GetLoginMode() != Mesh)
				world_list.SendPacketLogin(pack, this);
//cout << "Got world update for '" << sswls->name << "', #" << world->GetID() << endl;
			break;
		}
		case ServerOP_WorldListRemove: {
			if (this->GetType() != Login) {
				cout << "Error: ServerOP_WorldListRemove from a non-login connection? WTF!" << endl;
				break;
			}
			if (pack->size != sizeof(int32)) {
				cout << "Wrong size on ServerOP_WorldListRemove. Got: " << pack->size << ", Expected: " << sizeof(int32) << endl;
				break;
			}
//cout << "Got world remove for remote #" << *((int32*) pack->pBuffer) << endl;
			if ((*((int32*) pack->pBuffer)) > 0) {
				LWorld* world = world_list.FindByLink(this->GetLink(), *((int32*) pack->pBuffer));
				if (world && world->GetRemoteID() != 0) {
					*((int32*) pack->pBuffer) = world->GetID();
					if (net.GetLoginMode() != Mesh)
						world_list.SendPacketLogin(pack, this);
					world_list.RemoveByID(*((int32*) pack->pBuffer));
				}
			}
			else {
				cout << "Error: ServerOP_WorldListRemove: ID = 0? ops!" << endl;
			}
			break;
		}
		case ServerOP_TriggerWorldListRefresh: {
			world_list.UpdateWorldList();
			if (net.GetLoginMode() != Mesh)
				world_list.SendPacketLogin(pack, this);
			break;
		}
		case ServerOP_EncapPacket: {
			if (this->GetType() != Login) {
				cout << "Error: ServerOP_EncapPacket from a non-login connection? WTF!" << endl;
				break;
			}
			ServerEncapPacket_Struct* seps = (ServerEncapPacket_Struct*) pack->pBuffer;
			if (seps->ToID == 0xFFFFFFFF) { // Broadcast
				ServerPacket* inpack = new ServerPacket(seps->opcode);
				inpack->size = seps->size;
				// Little trick here to save a memcpy, be careful if you change this any
				inpack->pBuffer = seps->data;
				world_list.SendPacketLocal(inpack, this);
				inpack->pBuffer = 0;
				delete inpack;
			}
			else {
				LWorld* world = world_list.FindByID(seps->ToID);
				if (world) {
					ServerPacket* inpack = new ServerPacket(seps->opcode);
					inpack->size = seps->size;
					// Little trick here to save a memcpy, be careful if you change this any
					inpack->pBuffer = seps->data;
					world->SendPacket(inpack);
					inpack->pBuffer = 0;
					delete inpack;
				}
			}
			if (net.GetLoginMode() != Mesh)
				world_list.SendPacketLogin(pack, this);
			break;
		}
		default:
		{
			cout << "Unknown LoginSOPcode: 0x" << hex << (int)pack->opcode << dec;
			cout << " size:" << pack->size << endl;
#ifndef MINILOGIN
DumpPacket(pack->pBuffer, pack->size);
#endif
			break;
		}
		}
		delete pack;
	}
	return ret;
}

void LWorld::SendPacket(ServerPacket* pack) {
	if (Link == 0)
		return;
	if (RemoteID) {
		ServerPacket* outpack = new ServerPacket(ServerOP_EncapPacket, sizeof(ServerEncapPacket_Struct) + pack->size);
		ServerEncapPacket_Struct* seps = (ServerEncapPacket_Struct*) outpack->pBuffer;
		seps->ToID = RemoteID;
		seps->opcode = pack->opcode;
		seps->size = pack->size;
		memcpy(seps->data, pack->pBuffer, pack->size);
		Link->SendPacket(outpack);
		delete outpack;
	}
	else {
		Link->SendPacket(pack);
	}
}

void LWorld::Message(char* to, char* message, ...) {
	va_list argptr;
	char buffer[256];

	va_start(argptr, message);
	vsnprintf(buffer, 256, message, argptr);
	va_end(argptr);

	ServerPacket* pack = new ServerPacket(ServerOP_EmoteMessage, sizeof(ServerEmoteMessage_Struct) + strlen(buffer) + 1);
	ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;
	strcpy(sem->to, to);
	strcpy(sem->message, buffer);
	SendPacket(pack);
	delete pack;
}

void LWorld::Kick(char* message, bool iSetKickedFlag) {
	if (iSetKickedFlag)
		kicked = true;
	if (message) {
		ServerPacket* pack = new ServerPacket(ServerOP_LSFatalError, strlen(message) + 1);
		strcpy((char*) pack->pBuffer, message);
		SendPacket(pack);
		delete pack;
	}
	if (Link && GetRemoteID() == 0)
		Link->Disconnect();
}

bool LWorld::CheckServerName(char* name) {
	if (strlen(name) < 10)
		return false;
	for (int i=0; i<strlen(name); i++) {
		if (!((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= '0' && name[i] <= '9') || name[i] == ' ' || name[i] == '\'' || name[i] == '-' || name[i] == '(' || name[i] == ')' || name[i] == '[' || name[i] == ']' || name[i] == '/' || name[i] == '.' || name[i] == ',' || name[i] == '_' || name[i] == '+' || name[i] == '=' || name[i] == ':' || name[i] == '~'))
			return false;
	}
	return true;
}

bool LWorld::SetupWorld(char* in_worldname, char* in_worldaddress, char* in_account, char* in_password, char* in_version) {
	int32 tmpaccid = 0;
	if (strlen(in_worldaddress) > 3) {
		isaddressip = false;
		strcpy(address, in_worldaddress);
	}
	if (strlen(in_worldname) > 3) {
//		strcpy(worldname, in_worldname);
		snprintf(worldname, (sizeof(worldname)), "%s", in_worldname);
		strcat(worldname, WORLD_NAME_SUFFIX);
		world_list.KickGhostIP(GetIP(), this);
#ifndef MINILOGIN
		sint8 tmp = database.CheckWorldAuth(in_account, in_password, &tmpaccid, &admin_id, &GreenName, &pshowdown);
		if (tmp >= 1) {
			world_list.KickGhost(World, tmpaccid, this);
			accountid = tmpaccid;
			strncpy(account, in_account, 30);
			IsInit = true;
			ptype = World;
			database.UpdateWorldName(accountid, in_worldname);
		}
		else if (tmp == 0) {
#endif
#ifndef PUBLICLOGIN
			account[0] = 0;
			IsInit = true;
			ptype = World;
#endif

#ifdef PUBLICLOGIN
			if(accountid != 1) {
				cout << "Attempted login denied (Reason: AccountID is not equal to 1, following public login restrictions.)." << endl;
				this->Kick();
			}
#endif

#ifndef MINILOGIN
		}
		else {
			IsInit = false;
			struct in_addr  in;
			in.s_addr = GetIP();
			cout << "LWorld login error, " << inet_ntoa(in) << ":" << GetPort() << ". Account='" << in_account << "'" << endl;
			return false;
		}
#endif
	}
	else {
		// name too short
		account[0] = 0;
		IsInit = false;
		return false;
	}
	return true;
}

bool LWorld::SetupChat(char* in_account, char* in_password) {
#ifndef MINILOGIN
	int32 tmpaccid = 0;
	if (strlen(in_account) > 3) {
		sint8 tmp = database.CheckWorldAuth(in_account, in_password, &tmpaccid, &admin_id, &GreenName, &pshowdown);
		if (tmp >= 2) {
			world_list.KickGhost(Chat, 0, this);
			accountid = tmpaccid;
			strncpy(account, in_account, 30);
			IsInit = true;
//			pIsChat = true;
			ptype = Chat;
			return true;
		}
		else {
			IsInit = false;
			struct in_addr  in;
			in.s_addr = GetIP();
			cout << "Chat login error, " << inet_ntoa(in) << ":" << GetPort() << ". Account='" << in_account << "'" << endl;
			return false;
		}
	}
	else {
		// account too short
		account[0] = 0;
		IsInit = false;
		return false;
	}
#endif

	return false;
}

bool LWorld::SetupLogin(char* in_account, char* in_password, char* in_address) {
#ifndef MINILOGIN
	int32 tmpaccid = 0;
	if (strlen(in_account) > 3) {
		sint8 tmp = database.CheckWorldAuth(in_account, in_password, &tmpaccid, &admin_id, &GreenName, &pshowdown);
		if (tmp >= 3) {
			if (net.GetLoginMode() == Mesh) {
				pClientPort = atoi(in_address);
				world_list.KickGhostIP(this->GetIP(), this, this->GetClientPort());
				world_list.AddPeer(this);
			}
			else
				world_list.KickGhost(Login, tmpaccid, this);
			accountid = tmpaccid;
			strncpy(account, in_account, 30);
			IsInit = true;
			ptype = Login;
			world_list.PeerLoginConnected = true;
struct in_addr  in;
in.s_addr = GetIP();
cout << this->GetID() << ": Peer connected: (in) " << inet_ntoa(in) << ":" << GetPort() << " (" << GetClientPort() << ")" << endl;
			return true;
		}
		else {
			IsInit = false;
			struct in_addr  in;
			in.s_addr = GetIP();
			cout << "LoginServer login error, " << inet_ntoa(in) << ":" << GetPort() << ". Account='" << in_account << "'" << endl;
			return false;
		}
	}
	else {
		// account too short
		account[0] = 0;
		IsInit = false;
		return false;
	}
#endif

	return false;
}

void LWorld::UpdateWorldList(LWorld* to) {
#ifndef MINILOGIN
	if (this->GetType() != World)
		return;
	if (this->RemoteID != 0 || this->IsKicked())
		return;
	if ((this->IsPlaceholder() || !IsInit) && net.GetLoginMode() == Mesh)
		return;
	if (world_list.PeerLoginConnected) {
//cout << "Sending world update for '" << GetName() << "', #" << GetID() << endl;
		ServerPacket* pack = new ServerPacket(ServerOP_WorldListUpdate, sizeof(ServerSyncWorldList_Struct));
		ServerSyncWorldList_Struct* sswls = (ServerSyncWorldList_Struct*) pack->pBuffer;
		sswls->ip = ip;
		strcpy(sswls->account, account);
		sswls->accountid = accountid;
		strcpy(sswls->address, address);
		sswls->adminid = admin_id;
		sswls->greenname = GreenName;
		strcpy(sswls->name, worldname);
		sswls->RemoteID = GetID();
		sswls->servertype = 0;
		sswls->showdown = ShowDown();
		sswls->status = status;
		sswls->num_players = num_players;
		sswls->num_zones = num_zones;
		sswls->placeholder = IsPlaceholder();
		if (to) {
			to->SendPacket(pack);
		}
		else {
			world_list.SendPacketLogin(pack);
		}
		delete pack;
	}
#endif
}

void LWorld::ChangeToPlaceholder() {
	ip = 0;
	status = -1;
	pPlaceholder = true;
	if (Link != 0 && RemoteID == 0) {
		Link->Disconnect();
	}
	UpdateWorldList();
}

void LWorld::SetRemoteInfo(int32 in_ip, int32 in_accountid, char* in_account, char* in_name, char* in_address, int32 in_status, int32 in_adminid, int8 in_greenname, sint32 in_players, sint32 in_zones) {
	ip = in_ip;
	accountid = in_accountid;
	strcpy(account, in_account);
	strcpy(worldname, in_name);
	strcpy(address, in_address);
	status = in_status;
	admin_id = in_adminid;
	GreenName = in_greenname;
	num_players = in_players;
	num_zones = in_zones;
}


LWorldList::LWorldList() {
	NextID = 1;
	PeerLoginConnected = false;
	tcplistener = new TCPServer(net.GetPort(), true);
	if (net.GetLoginMode() == Slave)
		OutLink = new TCPConnection(true);
	else
		OutLink = 0;
}

LWorldList::~LWorldList() {
	safe_delete(tcplistener);
	safe_delete(OutLink);
}

void LWorldList::Shutdown() {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		iterator.RemoveCurrent();
	}
	safe_delete(tcplistener);
}

void LWorldList::Add(LWorld* worldserver) {
	list.Insert(worldserver);
}

void LWorldList::KickGhostIP(int32 ip, LWorld* NotMe, int16 iClientPort) {
	if (ip == 0)
		return;
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (!iterator.GetData()->IsKicked() && iterator.GetData()->GetIP() == ip && iterator.GetData() != NotMe) {
			if ((iClientPort == 0 && iterator.GetData()->GetType() == World) || (iClientPort != 0 && iterator.GetData()->GetClientPort() == iClientPort)) {
				struct in_addr  in;
				in.s_addr = iterator.GetData()->GetIP();
				cout << "Removing GhostIP LWorld(" << iterator.GetData()->GetID() << ") from ip: " << inet_ntoa(in) << " port: " << (int16)(iterator.GetData()->GetPort());
				if (!iterator.GetData()->Connected())
					cout << " (it wasnt connected)";
				cout << endl;
if (NotMe) {
in.s_addr = NotMe->GetIP();
cout << "NotMe(" << NotMe->GetID() << ") = " << inet_ntoa(in) << ":" << NotMe->GetPort() << " (" << NotMe->GetClientPort() << ")" << endl;
}
				iterator.GetData()->Kick("Ghost IP kick");
			}
		}
		iterator.Advance();
	}
}

void LWorldList::KickGhost(ConType in_type, int32 in_accountid, LWorld* ButNotMe) {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (!iterator.GetData()->IsKicked() && iterator.GetData()->GetType() == in_type && iterator.GetData() != ButNotMe && (in_accountid == 0 || iterator.GetData()->GetAccountID() == in_accountid)) {
			if (iterator.GetData()->GetIP() != 0) {
				struct in_addr  in;
				in.s_addr = iterator.GetData()->GetIP();
				cout << "Removing GhostAcc LWorld(" << iterator.GetData()->GetID() << ") from ip: " << inet_ntoa(in) << " port: " << (int16)(iterator.GetData()->GetPort()) << endl;
			}
			if (iterator.GetData()->GetType() == Login && iterator.GetData()->IsOutgoingUplink()) {
				iterator.GetData()->Kick("Ghost Acc Kick", false);
				cout << "softkick" << endl;
			}
			else
				iterator.GetData()->Kick("Ghost Acc Kick");
		}
		iterator.Advance();
	}
}

void LWorldList::Process() {
	TCPConnection* newtcp = 0;
	LWorld* newworld = 0;
	while ((newtcp = tcplistener->NewQueuePop())) {
		newworld = new LWorld(newtcp);
		Add(newworld);
	}
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsKicked() && !iterator.GetData()->IsNeverKick()) {
			iterator.RemoveCurrent();
		}
		else if (!iterator.GetData()->Process()) {
			struct in_addr  in;
			in.s_addr = iterator.GetData()->GetIP();
			if (iterator.GetData()->GetType() == Login) {
				cout << "Removing LWorld(" << iterator.GetData()->GetID() << ") (Login) from ip: " << inet_ntoa(in) << ", port: " << iterator.GetData()->GetPort() << endl;
			}
			else if (iterator.GetData()->GetType() == Chat) {
				cout << "Removing LWorld(" << iterator.GetData()->GetID() << ") (Chat) from ip: " << inet_ntoa(in) << ", port: " << iterator.GetData()->GetPort() << endl;
			}
			else {
//				cout << "Removing LWorld from ip: " << inet_ntoa(in) << ", port: " << iterator.GetData()->GetPort() << endl;
			}
			if (iterator.GetData()->GetAccountID() == 0 || !(iterator.GetData()->ShowDown()) || iterator.GetData()->GetType() == Chat) {
				iterator.RemoveCurrent();
			}
			else {
				iterator.GetData()->ChangeToPlaceholder();
				iterator.Advance();
			}
		}
		else
			iterator.Advance();
	}
}

/*void LWorldList::LinkLost(TCPConnection* in_link) {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetLink() == in_link) {
			struct in_addr  in;
			in.s_addr = iterator.GetData()->GetIP();
			cout << "Removing LWorld from ip: " << inet_ntoa(in) << ", port: " << iterator.GetData()->GetPort() << endl;
//			iterator.GetData()->Link = 0;
			if (iterator.GetData()->GetAccountID() == 0 || !(iterator.GetData()->ShowDown()) || iterator.GetData()->GetType() == Chat) {
				iterator.RemoveCurrent();
			}
			else {
				iterator.GetData()->ChangeToPlaceholder();
				iterator.Advance();
			}
		}
		else
			iterator.Advance();
	}
}*/

// Sends packet to all World and Chat servers, local and remote (but not to remote login server's ::Process())
void LWorldList::SendPacket(ServerPacket* pack, LWorld* butnotme) {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData() != butnotme) {
			if (iterator.GetData()->GetType() == Login) {
				ServerPacket* outpack = new ServerPacket(ServerOP_EncapPacket, sizeof(ServerEncapPacket_Struct) + pack->size);
				ServerEncapPacket_Struct* seps = (ServerEncapPacket_Struct*) outpack->pBuffer;
				seps->ToID = 0xFFFFFFFF;
				seps->opcode = pack->opcode;
				seps->size = pack->size;
				memcpy(seps->data, pack->pBuffer, pack->size);
				iterator.GetData()->SendPacket(outpack);
				delete outpack;
			}
			else if (iterator.GetData()->GetRemoteID() == 0) {
				iterator.GetData()->SendPacket(pack);
			}
		}
		iterator.Advance();
	}
}

// Sends a packet to every local TCP Connection, all types
void LWorldList::SendPacketLocal(ServerPacket* pack, LWorld* butnotme) {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData() != butnotme && iterator.GetData()->GetRemoteID() == 0) {
			iterator.GetData()->SendPacket(pack);
		}
		iterator.Advance();
	}
}

// Sends the packet to all login servers
void LWorldList::SendPacketLogin(ServerPacket* pack, LWorld* butnotme) {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData() != butnotme && iterator.GetData()->GetType() == Login) {
			iterator.GetData()->SendPacket(pack);
		}
		iterator.Advance();
	}
}

void LWorldList::UpdateWorldList(LWorld* to) {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (net.GetLoginMode() != Mesh || iterator.GetData()->GetRemoteID() == 0)
			iterator.GetData()->UpdateWorldList(to);
		iterator.Advance();
	}
}

LWorld* LWorldList::FindByID(int32 LWorldID) {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetID() == LWorldID)
			return iterator.GetData();
		iterator.Advance();
	}
	return 0;
}

LWorld* LWorldList::FindByIP(int32 ip) {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetIP() == ip)
			return iterator.GetData();
		iterator.Advance();
	}
	return 0;
}

LWorld* LWorldList::FindByAddress(char* address) {
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (strcasecmp(iterator.GetData()->GetAddress(), address) == 0)
			return iterator.GetData();
		iterator.Advance();
	}
	return 0;
}

LWorld* LWorldList::FindByLink(TCPConnection* in_link, int32 in_id) {
	if (in_link == 0)
		return 0;
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetLink() == in_link && iterator.GetData()->GetRemoteID() == in_id)
			return iterator.GetData();
		iterator.Advance();
	}
	return 0;
}

LWorld*	LWorldList::FindByAccount(int32 in_accountid, ConType in_type) {
	if (in_accountid == 0)
		return 0;
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetAccountID() == in_accountid && iterator.GetData()->GetType() == in_type)
			return iterator.GetData();
		iterator.Advance();
	}
	return 0;
}

void LWorldList::MakeServerListPacket(APPLAYER* outapp, int8 lsadmin) {
	LinkedListIterator<LWorld*> iterator(list);
	char chatserver[101];
	memset(chatserver, 0, 101);
#ifndef MINILOGIN
	database.GetVariable("ChatServer", chatserver, 100);
#endif
	outapp->opcode = OP_ServerList;
	outapp->size = sizeof(ServerList_Struct);

	int16 NumServers = 0;
	LWorld* world = 0;
	iterator.Reset();
	while(iterator.MoreElements())
	{
		world = iterator.GetData();
		if ((world->IsInit || world->ShowDown()) && world->GetType() == World) {
			outapp->size += strlen(world->GetName()) + 1;
			outapp->size += strlen(world->GetAddress()) + 1;
			outapp->size += sizeof(ServerListServerFlags_Struct);
			NumServers++;
		}
		iterator.Advance();
	}

	outapp->size += sizeof(ServerListEndFlags_Struct); // flags and unknowns
	outapp->size += strlen(chatserver) + 1;
	outapp->pBuffer = new uchar[outapp->size];
	memset(outapp->pBuffer, 0, outapp->size);
	ServerList_Struct* sl = (ServerList_Struct*) outapp->pBuffer;
	memset(sl, 0, sizeof(ServerList_Struct));
	int o = 0;
	sl->numservers = NumServers;
	sl->showusercount = 0xff; // this tells it to display real user count instead of "UP"
	iterator.Reset();
	while(iterator.MoreElements())
	{
		world = iterator.GetData();
		if ((world->IsInit || world->ShowDown()) && world->GetType() == World) {
			strcpy((char*) &sl->data[o], world->GetName());
			o += strlen(world->GetName()) + 1;
			strcpy((char*) &sl->data[o], world->GetAddress());
			o += strlen(world->GetAddress()) + 1;

			ServerListServerFlags_Struct* slsf = (ServerListServerFlags_Struct*) &sl->data[o];
			o += sizeof(ServerListServerFlags_Struct);
			// green name ?
			memset(slsf, 0, sizeof(ServerListServerFlags_Struct));
			if (world->IsGreenName())
				slsf->greenname = 1;
#ifdef NEWDLLSTRUCT
			slsf->unknown1 = 0x01;
			slsf->worldid = iterator.GetData()->GetID();
#endif
#ifndef MINILOGIN
			slsf->usercount = database.CountUsers(world->account);
#endif
		}
		iterator.Advance();
	}

	ServerListEndFlags_Struct* slef = (ServerListEndFlags_Struct*) &sl->data[o];
	memset(slef, 0, sizeof(ServerListEndFlags_Struct));
//	o += sizeof(ServerListEndFlags_Struct);
	//If the player is an admin, show the admin button!!
//	if(lsadmin >= 50) {
//		slef->adminbutton = 1;
//	}
//	slef->chatbutton = 0;
//#ifdef NEWDLLSTRUCT
//	// This turns on all expansions as of December 15th, 2002
//	char buf[20];
//	if (!database.GetVariable("Expansion", buf, 19))
//		slef->expansions = 0x9F; // Use to be before PoP - 0x0F;
//	else
//		slef->expansions = atoi(buf);
//#else
//	slef->exp1 = 1;
//	slef->exp2 = 1;
//	//slef->unknown3 = 0x40;
//#endif
//
//	// "chatserver:port" goes here
////	strcpy(slef->chatserver, CHATSERVER_ADDR_PORT);
//	strcpy((char*) &slef->chatserver, chatserver);
//	slef->expansions = 1;
//	slef->chatbutton = 0;
	slef->admin = 0;
	slef->kunark = 1;
	slef->velious = 1;
}

void LWorldList::SendWorldStatus(LWorld* chat, char* adminname) {
	LinkedListIterator<LWorld*> iterator(list);
	struct in_addr  in;

	int32 count = 0;
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetIP() != 0 && iterator.GetData()->GetType() == World) {
			chat->Message(adminname, "Name: %s", iterator.GetData()->GetName());
			in.s_addr = iterator.GetData()->GetIP();
			if (iterator.GetData()->GetAccountID() != 0) {
				chat->Message(adminname, "   Account:           %s", iterator.GetData()->GetAccount());
			}
			chat->Message(adminname, "   Number of Zones:   %i", iterator.GetData()->GetZoneNum());
			chat->Message(adminname, "   Number of Players: %i", iterator.GetData()->GetPlayerNum());
			chat->Message(adminname, "   IP:                %s", inet_ntoa(in));
			if (!iterator.GetData()->IsAddressIP()) {
				chat->Message(adminname, "   Address:			%s", iterator.GetData()->GetAddress());
			}
			count++;
		}
		iterator.Advance();
	}
	chat->Message(adminname, "%i worlds listed.", count);
}

void LWorldList::RemoveByLink(TCPConnection* in_link, int32 in_id, LWorld* ButNotMe) {
	if (in_link == 0)
		return;
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData() != ButNotMe && iterator.GetData()->GetLink() == in_link && (in_id == 0 || iterator.GetData()->GetRemoteID() == in_id)) {
//			iterator.GetData()->Link = 0;
			iterator.RemoveCurrent();
		}
		else {
			iterator.Advance();
		}
	}
}

void LWorldList::RemoveByID(int32 in_id) {
	if (in_id == 0)
		return;
	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetID() == in_id) {
			iterator.RemoveCurrent();
			return;
		}
		else {
			iterator.Advance();
		}
	}
}

bool LWorldList::Init() {
	if (tcplistener->IsOpen()) {
		cout << "TCP Init() called when already init'ed" << endl;
	}
	else {
#ifndef MINILOGIN
		if (net.GetLoginMode() != Slave)
			database.LoadWorldList();
#endif
		return tcplistener->Open(net.GetPort());
	}
	return false;
}

void LWorldList::ConnectUplink() {
	if (net.GetLoginMode() == Slave) {
		if (!OutLink->Connected() && net.GetMasterAddress()[0] != 0) {
			char errbuf[TCPConnection_ErrorBufferSize];
			int32 destip = ResolveIP(net.GetMasterAddress(), errbuf);
			if (destip == 0) {
				cout << "LWorldList::ConnectUplink failed: '" << errbuf << "'" << endl;
				return;
			}
			else {
				if (OutLink->Connect(destip, net.GetUplinkPort(), errbuf)) {
					cout << "Connected to Master Login server: " << net.GetMasterAddress() << ":" << net.GetUplinkPort() << endl;
					world_list.PeerLoginConnected = true;
					ServerPacket* pack = new ServerPacket(ServerOP_LSInfo, sizeof(ServerLSInfo_Struct));
					ServerLSInfo_Struct* lsi = (ServerLSInfo_Struct*) pack->pBuffer;
					strcpy(lsi->protocolversion, EQEMU_PROTOCOL_VERSION);
					lsi->servertype = 2;
					strcpy(lsi->account, net.GetUplinkAccount());
					strcpy(lsi->password, net.GetUplinkPassword());
					OutLink->SendPacket(pack);
					delete pack;
					LWorld* ulw = new LWorld(OutLink, true);
					world_list.Add(ulw);
					world_list.UpdateWorldList(ulw);
				}
				else {
					cout << "LWorldList::ConnectUplink failed: '" << errbuf << "'" << endl;
					return;
				}
			}
		}
	}
	else {
		ThrowError("LWorldList::ConnectUplink called when not in slave mode!");
		return;
	}
}

void LWorldList::AddPeer(int32 iIP, int16 iPort, bool iFromINI) {
	if (net.GetLoginMode() != Mesh)
		return;
	LinkedListIterator<PeerInfo*> iterator(peerlist);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->ip == iIP && iterator.GetData()->port == iPort) {
			if (iterator.GetData()->world->pReconnectTimer)
				iterator.GetData()->world->pReconnectTimer->Trigger();
			return;
		}
		iterator.Advance();
	}
	PeerInfo* pi = new PeerInfo;
	pi->ip = iIP;
	pi->port = iPort;
	TCPConnection* link = new TCPConnection(true);
	pi->world = new LWorld(link, true, pi->ip, pi->port, iFromINI);
struct in_addr  in;
in.s_addr = iIP;
cout << "Adding Peer(" << pi->world->GetID() << "): " << inet_ntoa(in) << ":" << iPort << endl;
	Add(pi->world);
	pi->fromini = iFromINI;
	pi->failcount = 0;
	pi->ct = Outgoing;
	peerlist.Append(pi);
}

void LWorldList::AddPeer(LWorld* iLogin) {
	if (net.GetLoginMode() != Mesh)
		return;
/*	LinkedListIterator<PeerInfo*> iterator(peerlist);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->ip == iLogin->GetIP() && iterator.GetData()->port == iLogin->GetClientPort()) {
			return;
		}
		iterator.Advance();
	}*/
	PeerInfo* pi = new PeerInfo;
	pi->ip = iLogin->GetIP();
	pi->port = iLogin->GetClientPort();
	pi->world = iLogin;
	pi->fromini = false;
	pi->failcount = 0;
	pi->ct = Incomming;
	peerlist.Append(pi);
}

void LWorldList::AddPeer(char* iAddress, bool iFromINI) {
	if (net.GetLoginMode() != Mesh)
		return;

	int32 tmpIP = 0;
	int16 tmpPort = 0;
	char errbuf[ERRBUF_SIZE] = "";
	if (!ParseAddress(iAddress, &tmpIP, &tmpPort, errbuf)) {
		cout << "Error: AddPeer(char*): !ParseAddress: " << errbuf << endl;
		return;
	}
	AddPeer(tmpIP, tmpPort, iFromINI);
}

void LWorldList::RemovePeer(LWorld* iLogin) {
	LinkedListIterator<PeerInfo*> iterator(peerlist);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->world == iLogin) {
			iterator.RemoveCurrent();
			return;
		}
		iterator.Advance();
	}
}

// -1 = delete, 0 = dont connect, 1 = connect
sint8 LWorldList::ConnectPeer(LWorld* iLogin) {
	sint8 ret = 1;
	LinkedListIterator<PeerInfo*> iterator(peerlist);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->world == iLogin) {
			if (!iterator.GetData()->fromini && iterator.GetData()->failcount++ > 3) {
				iterator.RemoveCurrent();
				return -1;
			}
		}
		else if (iterator.GetData()->ip == iLogin->GetIP() && iterator.GetData()->port == iLogin->GetClientPort())
			ret = 0;
		iterator.Advance();
	}
	return ret;
}

void LWorldList::ListWorldsToConsole() {
	LinkedListIterator<LWorld*> iterator(list);
	struct in_addr in;
	iterator.Reset();

	cout << "World List:" << endl;
	cout << "============================" << endl;
	while(iterator.MoreElements()) {
		in.s_addr = iterator.GetData()->GetIP();
		if (iterator.GetData()->GetType() == World) {
			if (iterator.GetData()->GetRemoteID() == 0)
				cout << "ID: " << iterator.GetData()->GetID() << ", Name: " << iterator.GetData()->GetName() << ", Local, IP: " << inet_ntoa(in) << ":" << iterator.GetData()->GetPort() << ", Status: " << iterator.GetData()->GetStatus() << ", AccID: " << iterator.GetData()->GetAccountID() << endl;
			else
				cout << "ID: " << iterator.GetData()->GetID() << ", Name: " << iterator.GetData()->GetName() << ", RemoteID: " << iterator.GetData()->GetRemoteID() << ", LinkWorldID: " << iterator.GetData()->GetLinkWorldID() << ", IP: " << inet_ntoa(in) << ":" << iterator.GetData()->GetPort() << ", Status: " << iterator.GetData()->GetStatus() << ", AccID: " << iterator.GetData()->GetAccountID() << endl;
		}
		else if (iterator.GetData()->GetType() == Chat) {
			cout << "ID: " << iterator.GetData()->GetID() << ", Chat Server, IP: " << inet_ntoa(in) << ":" << iterator.GetData()->GetPort() << ", AccID: " << iterator.GetData()->GetAccountID() << endl;
		}
		else if (iterator.GetData()->GetType() == Login) {
			if (iterator.GetData()->IsOutgoingUplink()) {
				if (iterator.GetData()->Connected())
					cout << "ID: " << iterator.GetData()->GetID() << ", Login Server (out), IP: " << inet_ntoa(in) << ":" << iterator.GetData()->GetPort() << ", AccID: " << iterator.GetData()->GetAccountID() << endl;
				else
					cout << "ID: " << iterator.GetData()->GetID() << ", Login Server (nc), IP: " << inet_ntoa(in) << ":" << iterator.GetData()->GetPort() << ", AccID: " << iterator.GetData()->GetAccountID() << endl;
			}
			else
				cout << "ID: " << iterator.GetData()->GetID() << ", Login Server (in), IP: " << inet_ntoa(in) << ":" << iterator.GetData()->GetPort() << " (" << iterator.GetData()->GetClientPort() << "), AccID: " << iterator.GetData()->GetAccountID() << endl;
		}
		else {
			cout << "ID: " << iterator.GetData()->GetID() << ", Unknown Type, Name: " << iterator.GetData()->GetName() << ", IP: " << inet_ntoa(in) << ":" << iterator.GetData()->GetPort() << ", AccID: " << iterator.GetData()->GetAccountID() << endl;
		}
		iterator.Advance();
	}
	cout << "============================" << endl;
}




#ifndef MINILOGIN
void Database::LoadWorldList() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "Select id, account, name, admin_id, greenname from login_worldservers where showdown=1"), errbuf, &result)) {
		delete[] query;
		while (row = mysql_fetch_row(result)) {
			if (strlen(row[2]) >= 3) {
				char tmp[201];
				snprintf(tmp, (sizeof(tmp)-sizeof(WORLD_NAME_SUFFIX)) - 1, "%s", row[2]);
				if (LWorld::CheckServerName(tmp)) {
					char* tmp2 = strstr(tmp, "Server");
					if (tmp2 == 0)
						strcat(tmp, WORLD_NAME_SUFFIX);
					else
						*(tmp2+6) = 0;
					LWorld* NewWorld = new LWorld(atoi(row[0]), row[1], tmp, atoi(row[3]), atoi(row[4]));
					world_list.Add(NewWorld);
				}
			}
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadWorldList query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
}
#ifndef PUBLICLOGIN

bool LWorldList::WriteXML() {
	int32 num_zones = 0;
	int32 num_servers = 0;
	int32 num_players = 0;

	LinkedListIterator<LWorld*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetType() == World) {
			num_zones += iterator.GetData()->GetZoneNum();
			num_players += iterator.GetData()->GetPlayerNum();
			num_servers++;
		}
		iterator.Advance();
	}
	return database.InsertStats(num_players, num_zones, num_servers);
}
#endif // End of Not PublicLogin 
#endif // End of Not Minilogin

