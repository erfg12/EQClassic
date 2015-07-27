#ifndef EQ_SOPCODES_H
#define EQ_SOPCODES_H

//#define EQEMU_PROTOCOL_VERSION	"0.3.10"
#define EQEMU_PROTOCOL_VERSION	"0.3.10"

#include "../common/types.h"
#include "../common/packet_functions.h"
#include "../common/eq_packet_structs.h"

#define SERVER_TIMEOUT	30000	// how often keepalive gets sent
#define INTERSERVER_TIMER					90000
#define LoginServer_StatusUpdateInterval	15000
#define LoginServer_AuthStale				60000
#define AUTHCHANGE_TIMEOUT					900	// in seconds

#define ServerOP_KeepAlive			0x0001	// packet to test if port is still open
#define ServerOP_ChannelMessage		0x0002	// broadcast/guildsay
#define ServerOP_SetZone			0x0003	// client -> server zoneinfo
#define ServerOP_ShutdownAll		0x0004	// exit(0);
#define ServerOP_ZoneShutdown		0x0005	// unload all data, goto sleep mode
#define ServerOP_ZoneBootup			0x0006	// come out of sleep mode and load zone specified
#define ServerOP_ZoneStatus			0x0007	// Shows status of all zones
#define ServerOP_SetConnectInfo		0x0008	// Tells server address and port #
#define ServerOP_EmoteMessage		0x0009	// Worldfarts
#define ServerOP_ClientList			0x000A	// Update worldserver's client list, for #whos
#define ServerOP_Who				0x000B	// #who
#define ServerOP_ZonePlayer			0x000C  // #zone, or #summon
#define ServerOP_KickPlayer			0x000D  // #kick
#define ServerOP_RefreshGuild		0x000E	// Notice to all zoneservers to refresh their guild cache for ID# in packet
#define ServerOP_GuildKickAll		0x000F	// Remove all clients from this guild
#define ServerOP_GuildInvite		0x0010
#define ServerOP_GuildRemove		0x0011
#define ServerOP_GuildPromote		0x0012
#define ServerOP_GuildDemote		0x0013
#define ServerOP_GuildLeader		0x0014
#define ServerOP_GuildGMSet			0x0015
#define ServerOP_GuildGMSetRank		0x0016
#define ServerOP_FlagUpdate			0x0018	// GM Flag updated for character, refresh the memory cache
#define ServerOP_GMGoto				0x0019
#define ServerOP_MultiLineMsg		0x001A
#define ServerOP_Lock				0x001B  // For #lock/#unlock inside server
#define ServerOP_Motd				0x001C  // For changing MoTD inside server.
#define ServerOP_Uptime				0x001D
#define ServerOP_Petition			0x001E
#define	ServerOP_KillPlayer			0x001F
#define ServerOP_UpdateGM			0x0020
#define ServerOP_RezzPlayer			0x0021
#define ServerOP_ZoneReboot			0x0022
#define ServerOP_ZoneToZoneRequest	0x0023
#define ServerOP_AcceptWorldEntrance 0x0024
#define ServerOP_ZAAuth				0x0025
#define ServerOP_ZAAuthFailed		0x0026
#define ServerOP_ZoneIncClient		0x0027	// Incomming client
#define ServerOP_ClientListKA		0x0028
#define ServerOP_ChangeWID			0x0029
#define ServerOP_IPLookup			0x002A
#define ServerOP_LockZone			0x002B
#define ServerOP_ItemStatus			0x002C

#define ServerOP_LSInfo				0x1000
#define ServerOP_LSStatus			0x1001
#define ServerOP_LSClientAuth		0x1002
#define ServerOP_LSFatalError		0x1003
#define ServerOP_SystemwideMessage	0x1005
#define ServerOP_ListWorlds			0x1006
#define ServerOP_PeerConnect		0x1007

#define	ServerOP_UsertoWorldReq		0xAB00
#define	ServerOP_UsertoWorldResp	0xAB01

#define ServerOP_EncapPacket		0x2007	// Packet within a packet
#define ServerOP_WorldListUpdate	0x2008
#define ServerOP_WorldListRemove	0x2009
#define ServerOP_TriggerWorldListRefresh	0x200A

#define ServerOP_SetWorldTime		0x200B
#define ServerOP_GetWorldTime		0x200C
#define ServerOP_SyncWorldTime		0x200E

#define ServerOP_ReloadAlliances	0x200F
#define ServerOP_ReloadOwnedCities	0x201A

/************ PACKET RELATED STRUCT ************/
class ServerPacket
{
public:
	~ServerPacket() { safe_delete(pBuffer); }
    ServerPacket(int16 in_opcode = 0, int32 in_size = 0) {
		this->compressed = false;
		size = in_size;
		opcode = in_opcode;
		if (size == 0) {
			pBuffer = 0;
		}
		else {
			pBuffer = new uchar[size];
			memset(pBuffer, 0, size);
		}
	}
	ServerPacket* Copy() {
		if (this == 0) {
			return 0;
		}
		ServerPacket* ret = new ServerPacket(this->opcode, this->size);
		if (this->size)
			memcpy(ret->pBuffer, this->pBuffer, this->size);
		ret->compressed = this->compressed;
		ret->InflatedSize = this->InflatedSize;
		return ret;
	}
	bool Deflate() {
		if (compressed)
			return false;
		if ((!this->pBuffer) || (!this->size))
			return false;
		uchar* tmp = new uchar[this->size + 128];
		int32 tmpsize = DeflatePacket(this->pBuffer, this->size, tmp, this->size + 128);
		if (!tmpsize) {
			safe_delete(tmp);
			return false;
		}
		compressed = false;
		this->InflatedSize = this->size;
		this->size = tmpsize;
		uchar* tmpdel = this->pBuffer;
		this->pBuffer = tmp;
		safe_delete(tmpdel);
		return true;
	}
	bool Inflate() {
		if (!compressed)
			return false;
		if ((!this->pBuffer) || (!this->size))
			return false;
		uchar* tmp = new uchar[InflatedSize];
		int32 tmpsize = InflatePacket(this->pBuffer, this->size, tmp, InflatedSize);
		if (!tmpsize) {
			safe_delete(tmp);
			return false;
		}
		compressed = false;
		this->size = tmpsize;
		uchar* tmpdel = this->pBuffer;
		this->pBuffer = tmp;
		safe_delete(tmpdel);
		return true;
	}
	int32	size;
	int16	opcode;
	uchar*	pBuffer;
	bool	compressed;
	int32	InflatedSize;
	int32	destination;
};

#pragma pack(1)

struct SPackSendQueue {
	int16 size;
	uchar buffer[0];
};

struct ServerZoneStateChange_struct {
	int32 ZoneServerID;
	char adminname[64];
	int32 zoneid;
	bool makestatic;
};

struct ServerZoneIncommingClient_Struct {
	int32	zoneid;		// in case the zone shut down, boot it back up
	int32	ip;			// client's IP address
	int32	wid;		// client's WorldID#
	int32	accid;
	sint16	admin;
	int32	charid;
	bool	tellsoff;
	char	charname[64];
	char	lskey[30];
};

struct ServerChangeWID_Struct {
	int32	charid;
	int32	newwid;
};

struct ServerChannelMessage_Struct {
	char  deliverto[64];
	char  to[64];
	char  from[64];
	int8 fromadmin;
	bool  noreply;
	int16 chan_num;
	int32 guilddbid;
	int16  language;
	char  message[0];
};

struct ServerEmoteMessage_Struct {
	char	to[64];
	int32	guilddbid;
	sint16	minstatus;
	int32	type;
	char	message[0];
};

struct ServerClientList_Struct {
	int8	remove;
	int32	wid;
	int32	IP;
	int32	zone;
	sint16	Admin;
	int32	charid;
	char	name[64];
	int32	AccountID;
	char	AccountName[30];
	int32	LSAccountID;
	char	lskey[30];
	int8	race;
	int8	class_;
	int8	level;
	int8	anon;
	bool	tellsoff;
	int32	guilddbid;
	int32	guildeqid;
	bool	LFG;
	int8	gm;
};

struct ServerClientListKeepAlive_Struct {
	int32	numupdates;
	int32	wid[0];
};

struct ServerZonePlayer_Struct {
	char	adminname[64];
	sint16	adminrank;
	int8	ignorerestrictions;
	char	name[64];
	char	zone[25];
    float	x_pos;
    float	y_pos;
    float	z_pos;
};

struct RezzPlayer_Struct {
	int32	exp;
	int16	rezzopcode;
	//char	packet[160];
	Resurrect_Struct rez;
};

struct ServerZoneReboot_Struct {
//	char	ip1[250];
	char	ip2[250];
	int16	port;
	int32	zoneid;
};

struct SetZone_Struct {
	int32	zoneid;
	bool	staticzone;
};

struct ServerKickPlayer_Struct {
	char adminname[64];
	sint16 adminrank;
	char name[64];
	int32 AccountID;
};

struct ServerGuildCommand_Struct {
	int32 guilddbid;
	int32 guildeqid;
	char from[64];
	int8 fromrank;
	int32 fromaccountid;
	char target[64];
	int8 newrank;
	sint16 admin;
};

struct ServerLSInfo_Struct {
	char	name[201];				// name the worldserver wants
	char	address[250];			// DNS address of the server
	char	account[31];			// account name for the worldserver
	char	password[31];			// password for the name
	char	protocolversion[25];	// Major protocol version number
	char	serverversion[64];		// minor server software version number
	uint8	servertype;				// 0=world, 1=chat, 2=login, 3=MeshLogin
};

struct ServerLSStatus_Struct {
	sint32 status;
	sint32 num_players;
	sint32 num_zones;
};

struct ServerLSClientAuth {
	int32	lsaccount_id;	// ID# in login server's db
	char	name[30];		// username in login server's db
	char	key[30];		// the Key the client will present
	int8	lsadmin;		// login server admin level
	sint16	worldadmin;		// login's suggested worldadmin level setting for this user, up to the world if they want to obey it
};

struct ServerSystemwideMessage {
	int32	lsaccount_id;
	char	key[30];		// sessionID key for verification
	int32	type;
	char	message[0];
};

struct ServerLSPeerConnect {
	int32	ip;
	int16	port;
};

struct ServerConnectInfo {
	char	address[250];
	int16	port;
};

struct ServerGMGoto_Struct {
	char	myname[64];
	char	gotoname[64];
	sint16	admin;
};

struct ServerMultiLineMsg_Struct {
	char	to[64];
	char	message[0];
};

struct ServerLock_Struct {
	char	myname[64]; // User that did it
	int8	mode; // 0 = Unlocked ; 1 = Locked
};

struct ServerMotd_Struct {
	char	myname[64]; // User that set the motd
	char	motd[512]; // the new MoTD
};

struct ServerUptime_Struct {
	int32	zoneserverid;	// 0 for world
	char	adminname[64];
};

struct ServerPetitionUpdate_Struct {
	int32 petid;  // Petition Number
	int8  status; // 0x00 = ReRead DB -- 0x01 = Checkout -- More?  Dunno... lol
};

struct ServerWhoAll_Struct {
	sint16 admin;
	char from[64];
	char whom[64];
	int16 wrace; // FF FF = no race
	int16 wclass; // FF FF = no class
	int16 lvllow; // FF FF = no numbers
	int16 lvlhigh; // FF FF = no numbers
	int16 gmlookup; // FF FF = not doing /who all gm
};

struct ServerKillPlayer_Struct {
	char gmname[64];
	char target[64];
	sint16 admin;
};

struct ServerUpdateGM_Struct {
	char gmname[64];
	bool gmstatus;
};

struct ServerEncapPacket_Struct {
	int32	ToID;	// ID number of the LWorld on the other server
	int16	opcode;
	int16	size;
	uchar	data[0];
};

struct ZoneToZone_Struct {
	char	name[64];
	int32	requested_zone_id;
	int32	current_zone_id;
	sint8	response;
	sint16	admin;
	int8	ignorerestrictions;
};

struct WorldToZone_Struct {
	int32	account_id;
	sint8	response;
};
struct ServerSyncWorldList_Struct {
	int32	RemoteID;
	int32	ip;
	sint32	status;
	char	name[201];
	char	address[250];
	char	account[31];
	int32	accountid;
	int8	authlevel;
	int8	servertype;		// 0=world, 1=chat, 2=login
	int32	adminid;
	int8	greenname;
	int8	showdown;
	sint32  num_players;
	sint32  num_zones;
	bool	placeholder;
};

struct UsertoWorldRequest_Struct {
	int32	lsaccountid;
	int32	worldid;
	int32	FromID;
	int32	ToID;
};

struct UsertoWorldResponse_Struct {
	int32	lsaccountid;
	int32	worldid;
	sint8	response; // -3) World Full, -2) Banned, -1) Suspended, 0) Denied, 1) Allowed
	int32	FromID;
	int32	ToID;
};

// generic struct to be used for alot of simple zone->world questions
struct ServerGenericWorldQuery_Struct {
	char	from[64];	// charname the query is from
	sint16	admin;		// char's admin level
	char	query[0];	// text of the query
};

struct ServerLockZone_Struct {
	int8	op;
	char	adminname[64];
	int16	zoneID;
};

#pragma pack()

#endif
