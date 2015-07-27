#ifndef LOGIN_STRUCTS_H
#define LOGIN_STRUCTS_H

// use newdllstruct for eqmain.dll 5-28-2002 1140 and later
//#define NEWDLLSTRUCT

#pragma pack(1)

struct Update_Struct {
  int32 flag;
  char description[0];
};
//Yeahlight: Old client struct (DO NOT DELETE)
struct ServerList_Struct {
	int8	numservers;
	int8	unknown1;
	int8	unknown2;
	int8	showusercount; // 0xFF = show numbers, 0x0 = show "UP"
	uchar	data[0];
};
//Yeahlight: New client struct
//struct ServerList_Struct {
//	int16	numservers;
//	int8	unknown1;
//	int8	unknown2;
//	uchar	data[0];
//};

//Yeahlight: Old client struct (DO NOT DELETE)
//struct ServerListServerFlags_Struct {
//	int8	greenname;
//	int8	unknown1;
//	int8	unknown2;
//	int8	unknown3;
//	int8	unknown4;
//	int32	worldid;
//	sint32	usercount;
//};
//Yeahlight: New client struct

struct ServerListServerFlags_Struct {
	int8 greenname;
	int32 usercount;
	int8 unknown[8];
};
//
////neorab: new new client server flags struct (for multiple servers)
//struct ServerListServerFlags_Struct {
//	int8 greenname;
//	int32 usercount;
//};




//Yeahlight: Old client struct (DO NOT DELETE)
//struct ServerListEndFlags_Struct {
//	int32	adminbutton;
//#ifdef NEWDLLSTRUCT
//	int8	unknown1;
//	int32	expansions; // 0x0F
//#else
//	int8	chatbutton; // 0 = hide, 1 = show
//	int32	exp1;		// = 1
//	int16	exp2;		// = 1<<3
//	int8	unknown1;
//	int8	unknown2;	// = 0x40
//	int8	unknown3;
//	int8	unknown4;
//	int8	unknown5;
//	int8	unknown6;
//#endif
//	char	chatserver[0];
//};
//Yeahlight: New client struct
/*
struct ServerListEndFlags_Struct {
	int32 expansions;
	int8 unknown[5];
	int8 chatbutton;
	int8 unknown2[7];
};
// ^^^ only works for one server and only because the two bugs 
// in the structs pushed each other into working right --neorab
*/

// neorab: new new client struct (for multiple world servers)
struct ServerListEndFlags_Struct {
	int32 admin;
	int8 zeroes_a[8];
	int8 kunark;
	int8 velious;
	int8 zeroes_b[11];
};


struct SessionId_Struct {
  char	session_id[10];
  char	unused[7];
  int32	unknown; // legends? dunno, seen a 4 here, so gonna use that for now
};

#define AUTH_SIZE	32		// do not make bigger than crypt in LoginInfoStruct
struct LoginInfo_Struct {
	int8	crypt[40];
	int8	unknown[0];		// in here is the server name you just logged out of, variable length
};


// Below are chat server structs
struct ChannelsList_Struct {
	int32	count;		// number of channels in this packet
	uchar	data[0];	// One byte status, then channel name + null
};

struct UserList_Struct {
	int32	count;		// number of users in this packet
	uchar	data[0];	// One byte status, then name + null
};

struct Chat_ChannelMessage_Struct {
	int8	unknown1;
	char	message[0];		// Message + null
};

struct Chat_GuildsList_Struct {
/*000*/	char	date[9];	// mm/dd/yy + null
/*009*/	int32	status;
/*013*/	char	name[0];		// name + null
};

// Server -> client
struct Chat_AddChannel_Struct {
/*000*/	int8	status;			// 0 = normal, 1 = red, 2 = grey
/*001*/	char	channelname[0];	// + null
};

// Server -> client
struct Chat_ChangeChannel_Struct {
/*000*/	int8	unknown000;
/*001*/	char	channelname[0];
};

struct Register_SendPrice_Struct {
	int8 unknown000[10]; // 0x01 0f 44 16 62 01 01 1b 00
	int8 pricingoptions; // 0x03
	int8 padding[3];
	char price_name1[7]; // Normally: 1-Month
	char price_name2[7]; // Normally: 3-Month
	char price_name3[7]; // Normally: 6-Month
	char price_num1[7]; // Normally: $9.89
	char price_num2[7]; // Normally: $25.89
	char price_num3[7]; // Normally: $49.89
};

struct LoginCrypt_struct {
	char	username[20];
	char	password[20];
};

struct Registration_Struct {
/*0000*/	char	ChatHandel[20];
/*0020*/	char	ChatHandel2[17];	// *shrug*
/*0037*/	char	LastName[32];
/*0069*/	char	FirstName[32];
/*0101*/	char	MiddleInitial[4];
/*0105*/	char	Address[128];
/*0233*/	char	City[64];
/*0297*/	char	State[64];			// 2 character postal code
/*0361*/	char	ZIP[32];
/*0393*/	char	Country[32];
/*0425*/	char	PhoneDay[40];
/*0465*/	char	PhoneEve[40];
/*0505*/	char	EMail[128];
/*0633*/	char	WorkSchool[64];
/*0697*/	char	WhereDidYouHear[64];	// looks like spaces are changed to +'s (like in URLs)
/*0761*/	char	_2dVideoCard[32];		// looks like spaces are changed to +'s (like in URLs)
/*0793*/	char	ISP[64];
/*0857*/	char	AccountKey[26];
/*0883*/	char	DOB[12];
/*0895*/	int8	unknown895[16];
/*0911*/	char	CPUSpeed[64];
/*0975*/	char	CPUType[64];
/*1039*/	char	MainMemory[64];
/*1103*/	char	_3dVideoCard[64];
/*1167*/	char	VideoMemory[64];
/*1231*/	char	SoundCard[64];
/*1295*/	char	Modem[64];
/*1359*/	int8	unknown1359[256];
/*1615*/	char	StationName[20];
/*1635*/	char	Password[21];
/*1655*/	int8	Checksum[4];
};

#pragma pack()

#endif
