#ifndef LWORLD_H
#define LWORLD_H

#define ERROR_BADPASSWORD		"Bad password"
#define ERROR_BADVERSION		"Incorrect version"
#define ERROR_UNNAMED			"Unnamed servers not allowed to connect to full login servers"
#define ERROR_NOTMASTER			"Not a master server"
#define ERROR_NOTMESH			"Not a mesh server"
#define ERROR_GHOST				"Ghost kick"
#define ERROR_UnknownServerType	"Unknown Server Type"
#define ERROR_BADNAME_SERVER	"Bad server name, name may not contain the word \"Server\" (case sensitive)"
#define ERROR_BADNAME			"Bad server name. Unknown reason."

#define WORLD_NAME_SUFFIX		" Server"

#include "../common/linked_list.h"
#include "../common/timer.h"
#include "../common/types.h"
#include "../common/queue.h"
#include "../common/servertalk.h"
#include "../common/EQNetwork.h"
#include "../common/TCPConnection.h"

enum ConType { UnknownW, World, Chat, Login };

class LWorld
{
public:
	LWorld(TCPConnection* in_con, bool OutgoingLoginUplink = false, int32 iIP = 0, int16 iPort = 0, bool iNeverKick = false);
	LWorld(int32 in_accountid, char* in_accountname, char* in_worldname, int32 in_admin_id, bool in_greenname);
	LWorld(TCPConnection* in_RemoteLink, int32 in_ip, int32 in_RemoteID, int32 in_accountid, char* in_accountname, char* in_worldname, char* in_address, sint32 in_status, int32 in_adminid, bool in_greenname, bool in_showdown, int8 in_authlevel, bool in_placeholder, int32 iLinkWorldID);
    ~LWorld();

	static bool CheckServerName(char* name);
	
	bool	Process();
	void	SendPacket(ServerPacket* pack);
	void	Message(char* to, char* message, ...);

	bool	SetupWorld(char* in_worldname, char* in_worldaddress, char* in_account, char* in_password, char* in_version);
	bool	SetupChat(char* in_account, char* in_password);
	bool	SetupLogin(char* in_account, char* in_password, char* in_address);
	void	UpdateStatus(sint32 in_status, sint32 in_players, sint32 in_zones) { status = in_status; num_players = in_players; num_zones = in_zones; UpdateWorldList(); }
	void	UpdateWorldList(LWorld* to = 0);
	void	SetRemoteInfo(int32 in_ip, int32 in_accountid, char* in_account, char* in_name, char* in_address, int32 in_status, int32 in_adminid, int8 in_greenname, sint32 in_players, sint32 in_zones);

	inline bool	IsPlaceholder()		{ return pPlaceholder; }
	inline int32	GetAccountID()	{ return accountid; }
	inline char*	GetAccount()	{ return account; }
	inline char*	GetAddress()	{ return address; }
	inline int16	GetClientPort()	{ return pClientPort; }
	inline bool		IsAddressIP()	{ return isaddressip; }
	inline char*	GetName()		{ return worldname; }
	inline sint32	GetStatus()		{ return status; }
	inline bool		IsGreenName()	{ return GreenName; }
	inline int32	GetIP()			{ return ip; }
	inline int16	GetPort()		{ return port; }
	inline int32	GetID()			{ return ID; }
	inline int32	GetAdmin()		{ return admin_id; }
	inline bool		ShowDown()		{ return pshowdown; }
	inline bool		Connected()		{ return pConnected; }

	void			ChangeToPlaceholder();
	void			Kick(char* message = ERROR_GHOST, bool iSetKickedFlag = true);
	inline bool		IsKicked()				{ return kicked; }
	inline bool		IsNeverKick()			{ return pNeverKick; }
	inline  ConType	GetType()				{ return ptype; }
	inline  bool	IsOutgoingUplink()		{ return OutgoingUplink; }
	inline TCPConnection*	GetLink()		{ return Link; }
	inline int32	GetRemoteID()			{ return RemoteID; }
	inline int32	GetLinkWorldID()		{ return LinkWorldID; }
	inline sint32	GetZoneNum()			{ return num_zones; }
	inline sint32	GetPlayerNum()			{ return num_players; }

	bool	IsInit;
protected:
	friend class LWorldList;
	TCPConnection*	Link;
	Timer*	pReconnectTimer;
private:
	int32	ID;
	int32	ip;
	int16	port;
	bool	kicked;
	bool	pNeverKick;
	bool	pPlaceholder;

	int32	accountid;
	char	account[30];
	char	address[250];
	int16	pClientPort;
	bool	isaddressip;
	char	worldname[200];
	sint32	status;
	bool	GreenName;
	int32	admin_id;
	bool	pshowdown;
	ConType	ptype;
	bool	OutgoingUplink;
	bool	pConnected;
	sint32  num_players;
	sint32  num_zones;
	int32	RemoteID;
	int32	LinkWorldID;
};

class LWorldList
{
public:
	struct PeerInfo {
		int32	ip;
		int16	port;
		bool	fromini;
		int8	failcount;
		LWorld*	world;
		eConnectionType ct;
	};

	LWorldList();
	~LWorldList();

	LWorld*	FindByID(int32 WorldID);		
	LWorld*	FindByIP(int32 ip);
	LWorld*	FindByAddress(char* address);
	LWorld*	FindByLink(TCPConnection* in_link, int32 in_id);
	LWorld*	FindByAccount(int32 in_accountid, ConType in_type = World);

	void	AddPeer(int32 iIP, int16 iPort, bool iFromINI = false);
	void	AddPeer(LWorld* iLogin);
	void	AddPeer(char* iAddress, bool iFromINI = false);
	void	RemovePeer(LWorld* iLogin);
	sint8	ConnectPeer(LWorld* iLogin);

	void	Add(LWorld* worldserver);
	void	Process();
	void	ReceiveData();
	void	SendPacket(ServerPacket* pack, LWorld* butnotme = 0);
	void	SendPacketLocal(ServerPacket* pack, LWorld* butnotme = 0);
	void	SendPacketLogin(ServerPacket* pack, LWorld* butnotme = 0);
	void	MakeServerListPacket(APPLAYER* outapp, int8 lsadmin);
	void	UpdateWorldList(LWorld* to = 0);
	void	KickGhost(ConType in_type, int32 in_accountid = 0, LWorld* ButNotMe = 0);
	void	KickGhostIP(int32 ip, LWorld* NotMe = 0, int16 iClientPort = 0);
	void	RemoveByLink(TCPConnection* in_link, int32 in_id = 0, LWorld* ButNotMe = 0);
	void	RemoveByID(int32 in_id);

	void	SendWorldStatus(LWorld* chat, char* adminname);

	bool	PeerLoginConnected;
	void	ConnectUplink();
	bool	Init();
	void	Shutdown();
	bool	WriteXML();

	void	ListWorldsToConsole();
protected:
	friend class LWorld;
	int32	GetNextID()				{ return NextID++; }
private:
	int32	NextID;
	LinkedList<LWorld*>	list;
	LinkedList<PeerInfo*> peerlist; // list to keep track of outgoing peer link information

	TCPServer*				tcplistener;
	TCPConnection*			OutLink;
};
#endif
