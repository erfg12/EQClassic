// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef EQ_SOPCODES_H
#define EQ_SOPCODES_H

#define EQC_VERSION	"0.1"
#define EQEMU_PROTOCOL_VERSION	"0.3.10"

#include "types.h"
#include "config.h"
#include "eq_packet_structs.h"

#define SERVER_TIMEOUT						60000	// Comment: how often keepalive gets sent
#define INTERSERVER_TIMER					90000	// Comment: 
#define LoginServer_StatusUpdateInterval	15000	// Comment: 
#define LoginServer_AuthStale				60000	// Comment: 
#define AUTHCHANGE_TIMEOUT					900		// Comment: in seconds
#define	WORLD_KICK_TIMER					180000  // Comment: Yeahlight: How often World issues a world kick purge on stuck accounts in the database (3 minutes)
#define TOTAL_NUMBER_OF_ZONES				130		// Comment: Yeahlight: Our total number of bootable zones

#define ServerOP_KeepAlive			0x0001		// Comment: packet to test if port is still open
#define ServerOP_ChannelMessage		0x0002		// Comment: broadcast/guildsay
#define ServerOP_SetZone			0x0003		// Comment: client -> server zoneinfo
#define ServerOP_ShutdownAll		0x0004		// Comment: exit(0);
#define ServerOP_ZoneShutdown		0x0005		// Comment: unload all data, goto sleep mode
#define ServerOP_ZoneBootup			0x0006		// Comment: come out of sleep mode and load zone specified
#define ServerOP_ZoneStatus			0x0007		// Comment: Shows status of all zones
#define ServerOP_SetConnectInfo		0x0008		// Comment: Tells server address and port #
#define ServerOP_EmoteMessage		0x0009		// Comment: Worldfarts
#define ServerOP_ClientList			0x000A		// Comment: Update worldserver's client list, for #whos
#define ServerOP_Who				0x000B		// Comment: #who
#define ServerOP_ZonePlayer			0x000C		// Comment: #zone, or #summon
#define ServerOP_KickPlayer			0x000D		// Comment: #kick
#define ServerOP_RefreshGuild		0x000E		// Comment: Notice to all zoneservers to refresh their guild cache for ID# in packet
#define ServerOP_GuildKickAll		0x000F		// Comment: Remove all clients from this guild
#define ServerOP_GuildInvite		0x0010		// Comment: 
#define ServerOP_GuildRemove		0x0011		// Comment: 
#define ServerOP_GuildPromote		0x0012		// Comment: 
#define ServerOP_GuildDemote		0x0013		// Comment: 
#define ServerOP_GuildLeader		0x0014		// Comment: 
#define ServerOP_GuildGMSet			0x0015		// Comment: 
#define ServerOP_GuildGMSetRank		0x0016		// Comment: 
#define ServerOP_FlagUpdate			0x0018		// Comment: GM Flag updated for character, refresh the memory cache
#define ServerOP_GMGoto				0x0019		// Comment: 
#define ServerOP_MultiLineMsg		0x001A		// Comment: 
#define ServerOP_Lock				0x001B		// Comment: For #lock/#unlock inside server
#define ServerOP_Motd				0x001C		// Comment: For changing MoTD inside server.
#define ServerOP_Uptime				0x001D		// Comment: 
#define ServerOP_Petition			0x001E		// Comment: 
#define	ServerOP_KillPlayer			0x001F		// Comment: 
#define ServerOP_UpdateGM			0x0020		// Comment: 
#define ServerOP_RezzPlayer			0x0021
#define ServerOP_SendPacket			0x0022		// Comment: send directly a packet to a certain player -Cofruben.
#define ServerOP_GuildCreateRequest 0x0023		// Comment: Custom Op-Code for Guild Creation Request -Dark-Prince
#define ServerOP_GuildCreateResponse 0x0025		// Comment: Custom Op-Code for Guild Creation Response -Dark-Prince
#define ServerOP_GroupRefresh		0x0026		// Comment: Used to force a zone's group entity refresh for a client
//---------- Tazadar : Boat ServerOP ------------//
#define ServerOP_BoatNP				0x0027		// Comment: Tazadar -> Used to inform a boat that a new passenger arrived
#define ServerOP_BoatPL				0x0028		// Comment: Tazadar -> Used to inform a boat that a passenger left the boat
#define ServerOP_RemoveBoat			0x0029		// Comment: Tazadar -> Used to depop a boat which is in the incorrect zone
#define	ServerOP_SpawnBoat			0x0030		// Comment: Tazadar -> Used to spawn the Boat at the wanted location
#define	ServerOP_ZoneBoat			0x0031		// Comment: Tazadar -> Used to make clients zone
#define ServerOP_BoatGoTo			0x0032		// Comment: Tazadar -> Used to make a boat move
#define ServerOP_TravelDone			0x0033		// Comment: Tazadar -> Used to tell world that travel is done
// ----------------------------------------------//
#define	ServerOP_ZoneRunStatus		0x0034		// Comment: Yeahlight: Used to inform World that this zone is still running loops
// Kibanu - Time of Day Update to Zone Servers
#define ServerOP_TimeOfDay			0x0035		// Comment: Kibanu - Used to send a Time of Day update to the zone servers
#define ServerOP_DayTime			0x0036		// Comment: Kibanu - Used to send a Day Time packet to the zone servers
#define ServerOP_NightTime			0x0037		// Comment: Kibanu - Used to send a Night Time packet to the zone servers

// Login Server?
#define ServerOP_LSInfo				0x1000		// Comment: 
#define ServerOP_LSStatus			0x1001		// Comment: 
#define ServerOP_LSClientAuth		0x1002		// Comment: 
#define ServerOP_LSFatalError		0x1003		// Comment: 
#define ServerOP_SystemwideMessage	0x1005		// Comment: 
#define ServerOP_ListWorlds			0x1006		// Comment: 
#define	ServerOP_UsertoWorldReq		0xAB00		// Comment: Yeahlight: Player attempts to log into selected World server
#define	ServerOP_UsertoWorldResp	0xAB01		// Comment: Yeahlight: World's response to player's request

/************ PACKET RELATED STRUCT ************/


#pragma pack(1)

struct SPackSendQueue 
{
	int16 size;			// Comment: 
	uchar buffer[0];	// Comment: 
};

struct ServerZoneStateChange_struct 
{
	int32 ZoneServerID;		// Comment: 
	char adminname[30];		// Comment: 
	char zonename[15];		// Comment: 
};

struct ServerChannelMessage_Struct
{
	char  deliverto[32];	// Comment: 
	char  to[32];			// Comment: 
	char  from[32];			// Comment: 
	int8 fromadmin;			// Comment: 
	bool  noreply;			// Comment: 
	int16 chan_num;			// Comment: 
	int32 guilddbid;		// Comment: 
	int16  language;		// Comment: 
	char  message[0];		// Comment: 
};

struct ServerEmoteMessage_Struct
{
	char to[32];		// Comment: 
	int32 guilddbid;	// Comment: 
	int32 type;			// Comment: 
	char message[0];	// Comment: 
};

// Cofruben: used to send a packet directly to a client.
struct ServerSendPacket_Struct
{
	char	charname[32];		// Comment: 
	int16	opcode;
	char	packet[0];		// Comment: trick of bad guys..:P
};

struct ServerClientList_Struct
{
	bool	remove;				// Comment: 
	char	zone[30];			// Comment: 
	int8	Admin;				// Comment: 
	char	name[30];			// Comment: 
	int32	AccountID;			// Comment: 
	char	AccountName[30];	// Comment: 
	int8	race;				// Comment: 
	int8	class_;				// Comment: 
	int8	level;				// Comment: 
	int8	anon;				// Comment: 
	bool	tellsoff;			// Comment: 
	int32	guilddbid;			// Comment: 
	int32	guildeqid;			// Comment: 
	bool	LFG;				// Comment: 
};

struct ServerZonePlayer_Struct 
{
	char	adminname[30];		// Comment: 
	int8	adminrank;			// Comment: 
	bool	ignorerestrictions;	// Comment: 
	char	name[30];			// Comment: 
	char	zone[15];			// Comment: 
    float	x_pos;				// Comment: 
    float	y_pos;				// Comment: 
    float	z_pos;				// Comment: 
};

struct ServerKickPlayer_Struct
{
	char adminname[30];			// Comment: 
	int8 adminrank;				// Comment: 
	char name[30];				// Comment: 
	int32 AccountID;			// Comment: 
};

struct ServerGuildCommand_Struct
{
	int32 guilddbid;			// Comment: 
	int32 guildeqid;			// Comment: 
	char from[30];				// Comment: 
	int8 fromrank;				// Comment: 
	int32 fromaccountid;		// Comment: 
	char target[30];			// Comment: 
	GUILDRANK newrank;				// Comment: 
	int8 admin;					// Comment: 
};

struct ServerGuildPromoteCommand_Struct
{
	int32 guilddbid;			// Comment: 
	int32 guildeqid;			// Comment: 
	char PromoterName[30];		// Comment: 
	int8 PromoterRank;			// Comment: 
	int32 PromoterAccountID;	// Comment: 
	char Promotee[30];			// Comment: 
	GUILDRANK PromoteeRank;		// Comment: 
	int8 admin;					// Comment: 
};

struct ServerLSInfo_Struct 
{
	char	name[201];			// Comment: name the worldserver wants
	char	address[250];		// Comment: DNS address of the server
	char	account[31];		// Comment: account name for the worldserver
	char	password[31];		// Comment: password for the name
	char	protocolversion[25];	// Major protocol version number
	char	version[64];		// Comment: server's version #
	int8	servertype;			// Comment: 0=world, 1=chat
};

struct ServerLSStatus_Struct //newage: Fixed
{
	sint32 status;			// Comment: 
	sint32 num_players;
	sint32 num_zones;
};

//Yeahlight: Size 67
struct ServerLSClientAuth
{
/*0000*/int32	lsaccount_id;	// Comment: ID# in login server's db (confirmed: Yeahlight)
/*0004*/char	name[30];		// Comment: username in login server's db (confirmed: Yeahlight)
/*0034*/char	key[30];		// Comment: the Key the client will present (confirmed: Yeahlight)
/*0064*/int8	lsadmin;		// Comment: login server admin level (confirmed: Yeahlight)
/*0065*/int8	unknown[2];
};

struct ServerSystemwideMessage 
{
	int32	lsaccount_id;	// Comment: 
	char	key[30];		// Comment: sessionID key for verification
	int32	type;			// Comment: 
	char	message[0];		// Comment: 
};

struct ServerConnectInfo 
{
	char	address[250];	// Comment: 
	int16	port;			// Comment: 
};

struct ServerGMGoto_Struct 
{
	char	myname[32];		// Comment: 
	char	gotoname[32];	// Comment: 
	int8	admin;			// Comment: 
};

struct ServerMultiLineMsg_Struct 
{
	char	to[30];			// Comment: 
	char	message[0];		// Comment: 
};

struct ServerLock_Struct 
{
	char	myname[32];		// Comment: User that did it
	int8	mode;			// Comment: 0 = Unlocked ; 1 = Locked
};

struct SetServerMotd_Struct 
{
	char	myname[32];		// Comment: User that set the motd
	char	motd[SERVER_MOTD_LENGTH];		// Comment: the new MoTD
};

struct ServerUptime_Struct 
{
	int32	zoneserverid;	// Comment: 0 for world
	char	adminname[32];	// Comment: 
};

struct ServerPetitionUpdate_Struct 
{
	int32 petid;			// Comment: Petition Number
	int8  status;			// Comment: 0x00 = ReRead DB -- 0x01 = Checkout -- More?  Dunno... lol
};

struct ServerWhoAll_Struct 
{
	int8 admin;				// Comment: 
	char from[32];			// Comment: 
	char whom[32];			// Comment: 
	int16 wrace;			// Comment: FF FF = no race
	int16 wclass;			// Comment: FF FF = no class
	int16 firstlvl;			// Comment: FF FF = no numbers
	int16 secondlvl;		// Comment: FF FF = no numbers
	int16 gmlookup;			// Comment: FF FF = not doing /who all gm
};

struct ServerKillPlayer_Struct
{
	char gmname[30];		// Comment: 
	char target[30];		// Comment: 
	int8 admin;				// Comment: 
};

// Kibanu
struct ServerGroupRefresh_Struct
{
	char  member[15];		// Comment: Player who needs to reload group info ( server, not to client )
	int32 gid;				// Comment: GroupID - Mainly used for when a player leaves a group
	int8  action;			// 0: Player joins the group.
							// 1: New Leader
							// 2: Nothing ( used existing group update flags, they skipped 2 )
							// 3: Player leaves the group.	
							// 4: Group Disbanded
};

struct ServerUpdateGM_Struct 
{
	char gmname[30];		// Comment: 
	bool gmstatus;			// Comment: 
};

struct RezzPlayer_Struct
{
	int32	exp;
	int16	rezzopcode;
	char	owner[16];
	int16	action;
	int32	corpseDBID;
	Resurrect_Struct rez;
};

struct UsertoWorldRequest_Struct
{
	int32	lsaccountid;
	int32	worldid;
	int32	FromID;
	int32	ToID;
};

struct UsertoWorldResponse_Struct
{
	int32	lsaccountid;
	int32	worldid;
	sint8	response; // -3) World Full, -2) Banned, -1) Suspended, 0) Denied, 1) Allowed
	int32	FromID;
	int32	ToID;
};

struct GuildCreateReqest_Struct
{
	char LeaderCharName[30];		// Comment: Leader of the guilds char id
	char GuildName[30];				// Comment: Guild Name
};

struct GuildCreateResponse_Struct
{
	char LeaderCharName[30];
	char GuildName[30];
	bool Created;
	int32 eqid;
};

// 6-10-09 - Tazadar
// Sent to server when player steps onto a boat
// Size: 46 Bytes
struct ServerBoat_Struct{
	char player_name[30];
	char boatname[NPC_MAX_NAME_LENGTH]; //Tazadar: Changed the lenght of the name
};

// 6-12-09 - Tazadar
// Sent to zone server to remove a boat/Sent to world serv to tell it travel is done
// Size: 16 Bytes
struct BoatName_Struct{
	char boatname[NPC_MAX_NAME_LENGTH]; // Tazadar : Name of the boat.
};

// 6-15-09 - Tazadar
// Sent to zone server to spawn a boat
// Size: 128 Bytes
struct SpawnBoat_Struct{
	char boatname[NPC_MAX_NAME_LENGTH]; // Tazadar : Name of the boat.
	float x;
	float y;
	float z;
	float heading;
};


// 6-15-09 - Tazadar
// Sent to zone server to make player zone
// Size: variable (check TeleportPlayers() for more informations !)
struct ZoneBoat_Struct{
	char boatname[NPC_MAX_NAME_LENGTH]; // Tazadar : Name of the boat.
	char zonename[ZONE_SHORTNAME_LENGTH]; // Tazadar : Name of the boat.
	float x;
	float y;
	float z;
	float heading;
	int16 numberOfPlayers;
};

// 6-17-09 - Tazadar
// Sent to zone server to make boat move
// Size: 18 bytes
struct BoatGoTo_Struct{
	char boatname[NPC_MAX_NAME_LENGTH]; // Tazadar : Name of the boat.
	int16 fromNode;
	int16 toNode;
};




#pragma pack()

#endif