/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
create table guilds (id int(11) primary key auto_increment, eqid smallint(4) not null unique, name varchar(32) not null unique, leader int(11) not null unique, 
motd text not null, rank0title varchar(100) not null, rank1title varchar(100) not null, rank1 char(8) not null, rank2title varchar(100) not null, rank2 char(8) not null, 
rank3title varchar(100) not null, rank3 char(8) not null, rank4title varchar(100) not null, rank4 char(8) not null, rank5title varchar(100) not null, rank5 char(8) not null);

alter table character_ add column (guild int(11) default 0, guildrank tinyint(2) unsigned default 5);
*/

#ifndef GUILD_H
#define GUILD_H

#define GUILD_MAX_RANK  5   // 0-5 - some places in the code assume a single digit, dont go above 9

#define GUILD_HEAR		0
#define GUILD_SPEAK		1
#define GUILD_INVITE	2
#define GUILD_REMOVE	3
#define GUILD_PROMOTE	4
#define GUILD_DEMOTE	5
#define GUILD_MOTD		6
#define GUILD_WARPEACE	7

struct GuildRankLevel_Struct {
	char rankname[101];
	bool heargu;
	bool speakgu;
	bool invite;
	bool remove;
	bool promote;
	bool demote;
	bool motd;
	bool warpeace;
};

struct GuildRanks_Struct {
	char name[32];
	int32 databaseID;
	int32 leader;  // AccountID of leader
	sint16 minstatus;	// minium status to use GM commands on this guild
	GuildRankLevel_Struct rank[GUILD_MAX_RANK+1];
};
#endif
