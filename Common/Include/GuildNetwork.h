// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************


#ifndef GUILDNETWORK_H
#define GUILDNETWORK_H
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errmsg.h>
#include "types.h"
#include "config.h"

using namespace std;


#define GUILD_MAX_RANK  3   // Comment: Dark-Prince (11/05/2008) - Dont change this. What is it? Its the number of ranks within a guild (i.e Leader, Officer & Member) so dont change it!

#define GUILD_HEAR		0	//TODO: What is this one?
#define GUILD_SPEAK		1	//TODO: What is this one?
#define GUILD_INVITE	2	//TODO: What is this one?
#define GUILD_REMOVE	3	//TODO: What is this one?
#define GUILD_PROMOTE	4	//TODO: What is this one?
#define GUILD_DEMOTE	5	//TODO: What is this one?
#define GUILD_MOTD		6	//TODO: What is this one?
#define GUILD_WARPEACE	7	//TODO: What is this one?


namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			// Valids of Guild Rank
			enum GUILDRANK : sint8
			{
				GuildUnknown = -1,		// Comment: Unknown guild rank
				GuildMember = 0,		// Comment: Guild Leader
				GuildOffice = 1,		// Comment: Guild Officer
				GuildLeader = 2,		// Comment: Guild Member
				NotInaGuild = 3,		// Comment: Char is not in a guild
				GuildInviteTimeOut = 4, // Comment: Client Invite Window has timed out
				GuildDeclined = 5		// Comment: User Declined to join Guild
				
			};

			struct GuildRankLevel_Struct 
			{
				char rankname[101];		// Comment: 
				bool heargu;			// Comment: 
				bool speakgu;			// Comment: 
				bool invite;			// Comment: 
				bool remove;			// Comment: 
				bool promote;			// Comment: 
				bool demote;			// Comment: 
				bool motd;				// Comment: 
				bool warpeace;			// Comment: 
			};

			struct Guild_Struct
			{
				char name[32];			// Comment: 
				int32 databaseID;		// Comment: 
				int32 leader;			// Comment: AccountID of guild leader
				GuildRankLevel_Struct rank[GUILD_MAX_RANK+1];	// Comment: 
			};

			struct GuildsListEntry_Struct 
			{
				int32 guildID;				// Comment: empty = 0xFFFFFFFF
				char name[32];				// Comment: 
				int8 unknown1[4];			// Comment: = 0xFF
				int8 exists;				// Comment: = 1 if exists, 0 on empty
				int8 unknown2[7];			// Comment: = 0x00
				int8 unknown3[4];			// Comment: = 0xFF
				int8 unknown4[8];			// Comment: = 0x00
			};

			struct GuildsList_Struct 
			{
				int8 head[4];							// Comment: 
				GuildsListEntry_Struct Guilds[MAX_GUILDS];		// Comment: 
			};


			struct GuildUpdate_Struct
			{
				int32	guildID;				// Comment: 
				GuildsListEntry_Struct entry;	// Comment: 
			};

			// Guild invite, remove
			struct GuildCommand_Struct
			{
				char Invitee[30];			// Comment: Person who is being invited (Dark-Prince - 11/05/2008) (myname)
				char Inviter[30];			// Comment: Person who did /guildinvite (Dark-Prince - 11/05/2008) (othername)
				int16 guildeqid;			// Comment: 
				int8 unknown[2];			// Comment: for guildinvite all 0's, for remove 0=0x56, 2=0x02
				int32 rank;					// rank
			};

			struct GuildInvite_Struct
			{
				char Invitee[PC_MAX_NAME_LENGTH];
				char Inviter[PC_MAX_NAME_LENGTH];
				int16 guildeqid;
				int8 unknown[2];
				int32 rank;
			};

			struct GuildRemove_Struct
			{
				char Remover[PC_MAX_NAME_LENGTH];
				char Removee[PC_MAX_NAME_LENGTH];
				int16 guildeqid;
				int8 unknown[2];
				int32 rank;
			};
		}
	}
}
#endif
