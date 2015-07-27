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
#ifndef DATABASE_H
#define DATABASE_H

#define AUTHENTICATION_TIMEOUT	60

// Disgrace: for windows compile
#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
#endif
#include <mysql.h>

#include "../common/dbcore.h"
#include "types.h"
#include "linked_list.h"
#include "eq_packet_structs.h"
#include "EQNetwork.h"
#include "../common/guilds.h"
#include "../common/MiscFunctions.h"
#include "../common/Mutex.h"
#include "../zone/loottable.h"
#include "../zone/faction.h"
#include "../zone/message.h"

enum PERMISSION {PERMITTED, TEMP_SUSPENSION, PERM_SUSPENSION, RESET, ACCOUNT_PERM_SUSPENDED = 9999998, ACCOUNT_TEMP_SUSPENDED = 9999999};

//class Spawn;
class Spawn2;
class NPC;
class SpawnGroupList;
class Petition;
class Client;
struct Combine_Struct;
//struct Faction;
//struct FactionMods;
//struct FactionValue;
struct ZonePoint;
struct NPCType;

// Added By Hogie 
// INSERT into variables (varname,value) values('decaytime [minlevel] [maxlevel]','[number of seconds]');
// IE: decaytime 1 54 = Levels 1 through 54
//     decaytime 55 100 = Levels 55 through 100
// It will always put the LAST time for the level (I think) from the Database
struct EventLogDetails_Struct {
	int32	id;
	char	accountname[64];
	int32	account_id;
	sint16	status;
	char	charactername[64];
	char	targetname[64];
	char	timestamp[64];
	char	descriptiontype[64];
	char	details[128];
};

struct CharacterEventLog_Struct {
int32	count;
int8	eventid;
EventLogDetails_Struct eld[255];
};

struct npcDecayTimes_Struct {
	int16 minlvl;
	int16 maxlvl;
	int32 seconds;
};
// Added By Hogie -- End

struct VarCache_Struct {
	char varname[26];	// varname is char(25) in database
	char value[0];
};

struct GuildWar_Alliance {
	int16 guild_id;

	int16 other_guild_id;
};

struct DBnpcspells_entries_Struct {
	sint16	spellid;
	uint16	type;
	uint8	minlevel;
	uint8	maxlevel;
	sint16	manacost;
	sint32	recast_delay;
	sint16	priority;
};
struct DBnpcspells_Struct {
	int32	parent_list;
	sint16	attack_proc;
	int8	proc_chance;
	int32	numentries;
	DBnpcspells_entries_Struct entries[0];
};

class Database : public DBcore
{
public:
	Database();
	Database(const char* host, const char* user, const char* passwd, const char* database);
	~Database();
	static void HandleMysqlError(int32 errnum);

	char	commands[200][200];
	sint16	commandslevels[200];
	int		maxcommandlevel;

	int32	CheckEQLogin(const APPLAYER* app, char* oUsername, int8* lsadmin, int* lsstatus, bool* verified, sint16* worldadmin,int32 ip);
#ifdef GUILDWARS
	bool	SetCityGuildOwned(int32 guildid,int32 npc_id);
	void	GetCityAreaName(int32 zone_id, char* areaname);
	bool	CityDefense(int32 zone_id,int32 npc_id);
	//bool	CityDefense(int32 zone_id,int32 npc_id=0,int32 spawngroupid=0);
	int8	GetGuardFactionStanding(int32 zone_id,int32 npc_id,int32 spawngroupid);
	bool	UpdateCityDefenseFaction(int32 zone_id,int32 npc_id,int32 spawngroupid,int8 newstatus);
	void	DeleteCityDefense(int32 zone_id);
	int32	GetCityGuildOwned(int32 npc_id,int32 zone_id=0);
	void	LoadCityControl();
	int8	TotalCitiesOwned(int32 guild_id);
#ifdef GUILDWARS2
	void	SetCityAvailableFunds(int32 zone_id, sint32 funds);
	sint32	GetCityAvailableFunds(int32 zone_id);
#endif
#endif
	bool    logevents(char* accountname,int32 accountid,int8 status,const char* charname,const char* target, char* descriptiontype, char* description,int event_nid);
	bool	MoveCharacterToZone(char* charname, char* zonename);
	bool	MoveCharacterToZone(int32 iCharID, char* iZonename);
	bool	SetGMSpeed(int32 account_id, int8 gmspeed);
	int8	GetGMSpeed(int32 account_id);
	void	DeletePetitionFromDB(Petition* wpet);
	void	ExtraOptions();
	sint16	CommandRequirement(const char* commandname);
	void	UpdatePetitionToDB(Petition* wpet);
	void	InsertPetitionToDB(Petition* wpet);
	void	RefreshPetitionsFromDB();
	bool	GetDecayTimes(npcDecayTimes_Struct* npcCorpseDecayTimes);
	int8	CheckWorldVerAuth(char* version);
	bool	PopulateZoneLists(const char* zone_name, LinkedList<ZonePoint*>* zone_point_list, SpawnGroupList* spawn_group_list);
	bool	PopulateZoneSpawnList(const char* zone_name, LinkedList<Spawn2*> &spawn2_list, int32 repopdelay = 0);
	Spawn2*	LoadSpawn2(LinkedList<Spawn2*> &spawn2_list, int32 spawn2id, int32 timeleft);
	bool	DumpZoneState();
	int16	GetFreeGrid();
	sint8	LoadZoneState(const char* zonename, LinkedList<Spawn2*>& spawn2_list);
	int32	CreatePlayerCorpse(int32 charid, const char* charname, int32 zoneid, uchar* data, int32 datasize, float x, float y, float z, float heading);
	int32	UpdatePlayerCorpse(int32 dbid, int32 charid, const char* charname, int32 zoneid, uchar* data, int32 datasize, float x, float y, float z, float heading);
	sint32	DeleteStalePlayerCorpses();
	sint32	DeleteStalePlayerBackups();
	void	DeleteGrid(int32 sg2, int16 grid_num, bool grid_too = false);
	void	DeleteWaypoint(int16 grid_num, int32 wp_num);
	int32	AddWP(int32 sg2, int16 grid_num, int8 wp_num, float xpos, float ypos, float zpos, int32 pause, float xpos1, float ypos1, float zpos1, int type1, int type2);
	bool	DeletePlayerCorpse(int32 dbid);
	bool	LoadPlayerCorpses(int32 iZoneID);
	bool	GetZoneLongName(const char* short_name, char** long_name, char* file_name = 0, float* safe_x = 0, float* safe_y = 0, float* safe_z = 0, int32* maxclients = 0);
//	int32	GetAuthentication(const char* char_name, const char* zone_name, int32 ip);
//	bool	SetAuthentication(int32 account_id, const char* char_name, const char* zone_name, int32 ip);
//	bool	GetAuthentication(int32 account_id, char* char_name, char* zone_name, int32 ip);
//	bool	ClearAuthentication(int32 account_id);
	bool	UpdateItem(uint16 item_id,Item_Struct* is);
	int8	GetServerType();
	bool	GetGuildNameByID(int32 guilddbid, char * name);
	bool	UpdateTempPacket(char* packet,int32 lsaccount_id);
	bool	UpdateLiveChar(char* charname,int32 lsaccount_id);
	bool	GetLiveChar(int32 lsaccount_id, char* cname);
	bool	GetTempPacket(int32 lsaccount_id, char* packet);

	void	GetEventLogs(char* name,char* target,int32 account_id=0,int8 eventid=0,char* detail=0,char* timestamp=0, CharacterEventLog_Struct* cel=0);

	bool	SetServerFilters(char* name, ServerSideFilters_Struct *ssfs);
	int32	GetServerFilters(char* name, ServerSideFilters_Struct *ssfs);


	bool	NPCSpawnDB(int8 command, const char* zone, NPC* spawn = 0, int32 extra = 0); // 0 = Create 1 = Add; 2 = Update; 3 = Remove; 4 = Delete

    bool	SaveItemToDatabase(Item_Struct* ItemToSave);
	int32	CheckLogin(const char* name, const char* password, sint16* oStatus = 0);
	sint16	CheckStatus(int32 account_id);
	int32	CreateAccount(const char* name, const char* password, sint16 status, int32 lsaccount_id = 0);
	bool	DeleteAccount(const char* name);
	bool	SetGMFlag(const char* name, sint16 status);
	bool	SetSpecialAttkFlag(int8 id, const char* flag);
	bool	CheckZoneserverAuth(const char* ipaddr);
	bool	UpdateName(const char* oldname, const char* newname);
	bool	DoorIsOpen(int8 door_id,const char* zone_name);
	void	SetDoorPlace(int8 value,int8 door_id,const char* zone_name);
	int32	NumberInGuild(int32 guilddbid);
	bool	SetHackerFlag(const char* accountname, const char* charactername, const char* hacked);
    bool	SetItemAtt(char* att, char * value, unsigned int ItemIndex);
	void	GetCharSelectInfo(int32 account_id, CharacterSelect_Struct*);
	int32	GetPlayerProfile(int32 account_id, char* name, PlayerProfile_Struct* pp, char* current_zone = 0);
	bool	SetPlayerProfile(int32 account_id, int32 charid, PlayerProfile_Struct* pp, int32 current_zone = 0);
	int32	SetPlayerProfile_MQ(char** query, int32 account_id, int32 charid, PlayerProfile_Struct* pp, int32 current_zone = 0);
	bool	CreateSpawn2(int32 spawngroup, const char* zone, float heading, float x, float y, float z, int32 respawn, int32 variance);
	bool	CheckNameFilter(const char* name);
	bool	AddToNameFilter(const char* name);
	bool	CheckUsedName(const char* name);
	bool	ReserveName(int32 account_id, char* name);
	bool	CreateCharacter(int32 account_id, char* name, int16 gender, int16 race, int16 class_, int8 str, int8 sta, int8 cha, int8 dex, int8 int_, int8 agi, int8 wis, int8 face);
	bool	CreateCharacter(int32 account_id, PlayerProfile_Struct* pp);
	bool	DeleteCharacter(char* name);
	bool    SetStartingItems(PlayerProfile_Struct *cc, int16 si_race, int8 si_class, int16 si_deity, int16 si_current_zone, char* si_name, sint16 GM_FLAG = 0);

	int32	GetAccountIDByChar(const char* charname, int32* oCharID = 0);
	int32	GetAccountIDByName(const char* accname, sint16* status = 0, int32* lsid = 0);
	void	GetAccountName(int32 accountid, char* name, int32* oLSAccountID = 0);
	int32	GetCharacterInfo(const char* iName, int32* oAccID = 0, int32* oZoneID = 0, float* oX = 0, float* oY = 0, float* oZ = 0);
	bool	GetAccountInfoForLogin(int32 account_id, sint16* admin = 0, char* account_name = 0, int32* lsaccountid = 0, int8* gmspeed = 0);
	bool	GetAccountInfoForLogin_result(MYSQL_RES* result, sint16* admin = 0, char* account_name = 0, int32* lsaccountid = 0, int8* gmspeed = 0);
	bool	GetCharacterInfoForLogin(const char* name, int32* character_id = 0, char* current_zone = 0, PlayerProfile_Struct* pp = 0, int32* pplen = 0, PlayerAA_Struct* aa = 0, int32* aalen = 0, int32* guilddbid = 0, int8* guildrank = 0);
	bool	GetCharacterInfoForLogin_result(MYSQL_RES* result, int32* character_id = 0, char* current_zone = 0, PlayerProfile_Struct* pp = 0, int32* pplen = 0, PlayerAA_Struct* aa = 0, int32* aalen = 0, int32* guilddbid = 0, int8* guildrank = 0);
	bool	SetLocalPassword(int32 accid, const char* password);

	bool	InsertNewsPost(int8 type,const char* defender,const char* attacker,int8 result,const char* bodytext);

	bool	OpenQuery(char* zonename);
	bool	GetVariable(const char* varname, char* varvalue, int16 varvalue_len);
	bool	SetVariable(const char* varname, const char* varvalue);
	bool	LoadGuilds(GuildRanks_Struct* guilds);
	bool	GetGuildRanks(int32 guildeqid, GuildRanks_Struct* gr);
	int32	GetGuildEQID(int32 guilddbid);
	bool	SetGuild(int32 charid, int32 guilddbid, int8 guildrank);
	int32	GetFreeGuildEQID();
	int32	CreateGuild(const char* name, int32 leader);
	bool	DeleteGuild(int32 guilddbid);
	bool	RenameGuild(int32 guilddbid, const char* name);
	bool	EditGuild(int32 guilddbid, int8 ranknum, GuildRankLevel_Struct* grl);
	int32	GetGuildDBIDbyLeader(int32 leader);
	bool	SetGuildLeader(int32 guilddbid, int32 leader);
	bool	SetGuildMOTD(int32 guilddbid, const char* motd);
	bool	GetGuildMOTD(int32 guilddbid, char* motd);
	int32	GetMerchantData(int32 merchantid, int32 slot);
	int32	GetMerchantListNumb(int32 merchantid);
	bool	GetSafePoints(const char* short_name, float* safe_x = 0, float* safe_y = 0, float* safe_z = 0, sint16* minstatus = 0, int8* minlevel = 0);
	bool	GetSafePoints(int32 zoneID, float* safe_x = 0, float* safe_y = 0, float* safe_z = 0, sint16* minstatus = 0, int8* minlevel = 0) { return GetSafePoints(GetZoneName(zoneID), safe_x, safe_y, safe_z, minstatus, minlevel); }

	sint32	GetItemsCount(int32* oMaxID = 0);
	sint32	GetNPCTypesCount(int32* oMaxID = 0);
	sint32	GetDoorsCount(int32* oMaxID = 0);
	sint32	GetNPCFactionListsCount(int32* oMaxID = 0);
	bool	LoadItems();
#ifdef SHAREMEM
	bool	DBLoadItems(sint32 iItemCount, int32 iMaxItemID);
	bool	DBLoadNPCTypes(sint32 iNPCTypeCount, int32 iMaxNPCTypeID);
	bool	DBLoadDoors(sint32 iDoorCount, int32 iMaxDoorID);
	bool	DBLoadNPCFactionLists(sint32 iNPCFactionListCount, int32 iMaxNPCFactionListID);
	bool	DBLoadLoot();
	//bool DBLoadSkills();
#endif
	bool	LoadVariables();
	int32	LoadVariables_MQ(char** query);
	bool	LoadVariables_result(MYSQL_RES* result);
	bool	LoadNPCTypes();
	bool	LoadZoneNames();
	bool	LoadDoors();
	bool	LoadLoot();
	bool	LoadNPCFactionLists();
	//bool LoadSkills();
	int16 GetTrainlevel(int16 eqclass, int8 skill_id);
	inline const int32&	GetMaxItem()			{ return max_item; }
	inline const int32&	GetMaxLootTableID()		{ return loottable_max; }
	inline const int32&	GetMaxLootDropID()		{ return lootdrop_max; }
	inline const int32&	GetMaxNPCType()			{ return max_npc_type; }
	inline const int32& GetMaxNPCFactionList()	{ return npcfactionlist_max; }
#ifdef SHAREMEM
	const Item_Struct*	IterateItems(uint32* NextIndex);
#endif
	const Item_Struct*	GetItem(uint32 id);
	const NPCType*		GetNPCType(uint32 id);
	const NPCFactionList*	GetNPCFactionList(uint32 id);
	const Door*		GetDoor(int8 door_id, const char* zone_name);
	const Door*		GetDoorDBID(uint32 db_id);
	int32	GetZoneID(const char* zonename);
	const char*	GetZoneName(int32 zoneID, bool ErrorUnknown = false);
	const LootTable_Struct* GetLootTable(int32 loottable_id);
	const LootDrop_Struct* GetLootDrop(int32 lootdrop_id);

	int32	GetMaxNPCSpellsID();
	DBnpcspells_Struct* GetNPCSpells(int32 iDBSpellsID);

	static const char*	GetItemLink(const Item_Struct* item);
	void	LoadItemStatus();
	inline int8	GetItemStatus(int32 id) { if (id < 0xFFFF) { return item_minstatus[id]; } return 0; }
	inline void SetItemStatus(int32 id, int8 status) { if (id < 0xFFFF) { item_minstatus[id] = status; } }
	bool	DBSetItemStatus(int32 id, int8 status);
	bool	GetNPCFactionList(int32 npcfaction_id, sint32* faction_id, sint32* value, sint32* primary_faction = 0);
	bool	GetFactionData(FactionMods* fd, sint32 class_mod, sint32 race_mod, sint32 deity_mod, sint32 faction_id); //rembrant, needed for factions Dec, 16 2001
	bool	GetFactionName(sint32 faction_id, char* name, int32 buflen); // rembrant, needed for factions Dec, 16 2001
	bool	GetFactionIdsForNPC(sint32 nfl_id, LinkedList<struct NPCFaction*> *faction_list, sint32* primary_faction = 0); // neotokyo: improve faction handling
	bool	SetCharacterFactionLevel(int32 char_id, sint32 faction_id, sint32 value,LinkedList<FactionValue*>* val_list); // rembrant, needed for factions Dec, 16 2001
	bool	LoadFactionData();
	bool	LoadFactionValues(int32 char_id, LinkedList<FactionValue*>* val_list);
	bool	LoadFactionValues_result(MYSQL_RES* result, LinkedList<FactionValue*>* val_list);

	int32	GetAccountIDFromLSID(int32 iLSID, char* oAccountName = 0, sint16* oStatus = 0);

	bool	SetLSAuthChange(int32 account_id, const char* ip);
	bool	UpdateLSAccountAuth(int32 account_id, int8* auth);
	int32	GetLSLoginInfo(char* iUsername, char* oPassword = 0, int8* lsadmin = 0, int* lsstatus = 0, bool* verified = 0, sint16* worldadmin = 0);
//	int32	GetLSLoginInfo(char* iUsername, char* oPassword = 0, char* md5pass = 0);
	int32	GetLSAuthentication(int8* auth);
	int32	GetLSAuthChange(const char* ip);
	bool	AddLoginAccount(char* stationname, char* password, char* chathandel, char* cdkey, char* email);
	int8	ChangeLSPassword(int32 accountid, const char* newpassword, const char* oldpassword = 0);
	bool	ClearLSAuthChange();
	bool	RemoveLSAuthChange(int32 account_id);
	bool	GetLSAccountInfo(int32 account_id, char* name, int8* lsadmin, int* status, bool* verified, sint16* worldadmin = 0);
	sint8	CheckWorldAuth(const char* account, const char* password, int32* account_id, int32* admin_id, bool* GreenName, bool* ShowDown);
	bool	UpdateWorldName(int32 accountid, const char* name);
	char*	GetBook(char txtfile[14]);
	void	LoadWorldList();
	bool	AccountLoggedIn(int32 account_id);
	int32	CountUsers(char* account);
	bool	FailedLogin(char* ip);
	PERMISSION	CheckAccountLockout(char* ip);
	bool	ClearAccountLockout(char* ip);

	bool	GetTradeRecipe(Combine_Struct* combin,int16 tradeskill, int16* product,int16* product2, int16* failproduct,int16* skillneeded,int16* trivial,int16* productcount);
	bool	CheckGuildDoor(int8 doorid,int16 guildid, const char* zone);
	bool	SetGuildDoor(int8 doorid,int16 guildid, const char* zone);
	bool	LoadZonePoints(const char* zone, uint8** data, int32* size);
	bool	LoadStaticZonePoints(LinkedList<ZonePoint*>* zone_point_list,const char* zonename);
	bool	InsertStats(int32 in_players, int32 in_zones, int32 in_servers);

	void	AddLootTableToNPC(NPC* npc,int32 loottable_id, ItemList* itemlist, int32* copper, int32* silver, int32* gold, int32* plat);
	bool	UpdateZoneSafeCoords(const char* zonename, float x, float y, float z);
	int8	GetUseCFGSafeCoords();
	int8	CopyCharacter(const char* oldname, const char* newname, int32 acctid);
	int32	GetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa);
	int32	GetAASkillVars(int32 skill_id, char* name_buf, int *cost, int *max_level);
	bool	SetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa);
	bool	SetLSAdmin(int32 account_id, int8 in_status);
	void	FindAccounts(char* whom, Client* from);
	float	GetSafePoint(const char* short_name, const char* which);

	int32   GetZoneForage(int32 ZoneID, int8 skill);    /* for foraging - BoB */
	void	ModifyGrid(bool remove, int16 id, int8 type = 0, int8 type2 = 0);
	void	ModifyWP(int16 grid_id, int8 wp_num, float xpos, float ypos, float zpos, int32 script=0);
	bool	GetWaypoints(int16 grid, int8 num, char* data);
	void	AssignGrid(sint32 curx, sint32 cury, int16 id);

#ifdef GUILDWARS
	bool	LoadGuildAlliances();
	bool	SetGuildAlliance(int32 guildid,int32 allyguildid);
	bool	RemoveGuildAlliance(int32 guildid,int32 allyguildid);
	bool	GetGuildAlliance(int32 guild_id,int32 other_guild_id);
#endif

#ifndef SHAREMEM
	void Database::LoadAItem(uint16 item_id,unsigned int * texture,unsigned int* color);
#endif
	//New timezone functions
	int32	GetZoneTZ(int32 zoneid);
	bool	SetZoneTZ(int32 zoneid, int32 tz);
	//End new timezone functions
	void	AddLootDropToNPC(NPC* npc,int32 lootdrop_id, ItemList* itemlist);

	int8	GetZoneW(int32 zoneid);
	bool	SetZoneW(int32 zoneid, int8 w);
	bool InjectToRaw();

	uint32	MaxDoors() { return max_door_type; }
protected:
	
	//bool	RunQuery(const char* query, int32 querylen, char* errbuf = 0, MYSQL_RES** result = 0, int32* affected_rows = 0, int32* errnum = 0, bool retry = true);

private:
	void InitVars();

	int32	max_zonename;
	char**	zonename_array;
	uint32			max_item;
	uint32			max_npc_type;
	uint32			max_door_type;
	uint32			npcfactionlist_max;
#ifndef SHAREMEM
	Item_Struct**	item_array;
	NPCType**		npc_type_array;
    Door**			door_array;
	NPCFactionList**	npcfactionlist_array;
	LootTable_Struct** loottable_array;
	sint8*			loottable_inmem;
	LootDrop_Struct** lootdrop_array;
	bool*			lootdrop_inmem;
#endif
	int8			item_minstatus[0xFFFF];
	int8			door_isopen_array[255];
	uint32			max_faction;
	Faction**		faction_array;
	uint32			loottable_max;
	uint32			lootdrop_max;

	int32			npc_spells_maxid;
	DBnpcspells_Struct** npc_spells_cache;
	bool*			npc_spells_loadtried;

	Mutex			Mvarcache;
	uint32			varcache_max;
	VarCache_Struct** varcache_array;
	uint32			varcache_lastupdate;
#ifdef GUILDWARS
	GuildWar_Alliance** guild_alliances;
	uint32			guildalliance_max;
	int32			cityownedbyguilds[20];
#endif
};

#ifdef BUGTRACK
class BugDatabase
{
public:
	BugDatabase();
	BugDatabase(const char* host, const char* user, const char* passwd, const char* database);

	~BugDatabase();
	bool UploadBug(const char* bugdetail, const char* version, const char* loginname);
protected:
	bool	RunQuery(const char* query, int32 querylen, char* errbuf = 0, MYSQL_RES** result = 0, int32* affected_rows = 0, int32* errnum = 0, bool retry = true);
	int32	DoEscapeString(char* tobuf, const char* frombuf, int32 fromlen);

private:
	MYSQL	mysqlbug;
	Mutex	MBugDatabase;

};
#endif

#endif






