// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef DATABASE_H
#define DATABASE_H

#define AUTHENTICATION_TIMEOUT	60

#include <windows.h>
#include <winsock.h>
#include <mysql.h>
#include <queue>

#include "DatabaseHandler.h"
#include "types.h"
#include "linked_list.h"
#include "eq_packet_structs.h"
#include "GuildNetwork.h"
#include "../../Zone/Include/faction.h" //TODO: fix this
#include "../../Zone/Include/loottable.h" // TODO: fix this
#include "../../Zone/Include/Debug.hpp"		// Cofruben: 17/08/08.
#include "../../Zone/Include/skills.h"		// Cofruben: 17/08/08.
#include "MiscFunctions.h"
#include "Mutex.h"
#include "EQPacketManager.h"
#include "packet_functions.h"

//#include "../../Zone/Include/Object.h"

using namespace std;
using namespace EQC::Common::Network;
using namespace EQC::Common;

//class Spawn;
class Spawn2;
class SpawnGroupList;
class Petition;
class Client;
class Object;

struct ZonePoint;

// Added By Hogie 
// INSERT into variables (varname,value) values('decaytime [minlevel] [maxlevel]','[number of seconds]');
// IE: decaytime 1 54 = Levels 1 through 54
//     decaytime 55 100 = Levels 55 through 100
// It will always put the LAST time for the level (I think) from the Database
struct npcDecayTimes_Struct {
	int16 minlvl;
	int16 maxlvl;
	int32 seconds;
};


// Harakiri, result for traderecipe

struct DBTradeskillRecipe_Struct {
	SkillType tradeskill;
	sint16 skill_needed;
	uint16 trivial;	
	vector< pair<uint32,uint8> > onsuccess;
	vector< pair<uint32,uint8> > onfail;	
	vector< pair<uint32,uint8> > oncombine; // for search result
	char name[200];
	sint32 recipeID;
};

//atoi is not int32 or uint32 safe!!!!
#define atoul(str) strtoul(str, NULL, 10)


// Added By Hogie -- End

class Database : public DatabaseHandler
{
public:
	
	~Database();

	// -- Petitions
	void	DeletePetitionFromDB(Petition* wpet);
	void	UpdatePetitionToDB(Petition* wpet);
	void	InsertPetitionToDB(Petition* wpet);
	void	RefreshPetitionsFromDB();
	// -- End Petitions

	bool	GetDecayTimes(npcDecayTimes_Struct* npcCorpseDecayTimes);
	bool	PopulateZoneLists(char* zone_name, LinkedList<ZonePoint*>* zone_point_list, SpawnGroupList* spawn_group_list);
	bool	PopulateZoneSpawnList(char* zone_name, LinkedList<Spawn2*> &spawn2_list, int32 repopdelay = 0);
	Spawn2*	LoadSpawn2(LinkedList<Spawn2*> &spawn2_list, int32 spawn2id, int32 timeleft);
	bool	DumpZoneState();
	sint8	LoadZoneState(char* zonename, LinkedList<Spawn2*>& spawn2_list);
	int32	CreatePlayerCorpse(int32 charid, char* charname, char* zonename, uchar* data, int32 datasize, float x, float y, float z, float heading, bool rezed, int32 remainingTime, int32 accountid, int32 rezExp);
	int32	UpdatePlayerCorpse(int32 dbid, int32 charid, char* charname, char* zonename, uchar* data, int32 datasize, float x, float y, float z, float heading, bool rezed, int32 rezExp);
	bool	DeletePlayerCorpse(int32 dbid);
	bool	LoadPlayerCorpses(char* zonename);
	bool	LoadDoorData(LinkedList<Door_Struct*>* door_list, char* zonename);
	bool	LoadObjects(vector<Object_Struct*>* object_list, char* zonename);
	bool	GetZoneLongName(char* short_name, char** long_name, char* file_name = 0, float* safe_x = 0, float* safe_y = 0, float* safe_z = 0);
	int32	GetAuthentication(char* char_name, char* zone_name, int32 ip);
	bool	SetAuthentication(int32 account_id, char* char_name, char* zone_name, int32 ip);
	bool	GetAuthentication(int32 account_id, char* char_name, char* zone_name, int32 ip);
	
	bool	GetDebugList(int32 char_id, TDebugList* l);
	void	SetDebugList(int32 char_id, TDebugList* l);

	int32	CheckLogin(char* name, char* password);
	int8	CheckStatus(int32 account_id);
	bool	CreateAccount(char* name, char* password, int8 status, int32 lsaccount_id = 0);
	bool	DeleteAccount(char* name);
	bool	SetGMFlag(char* name, int8 status);
	bool	CheckZoneserverAuth(char* ipaddr);
	bool	UpdateName(char* oldname, char* newname);

	bool	SetZoneW(char* zone_name, int8 w);
	int8	GetZoneW(char* zone_name);

	void	GetCharSelectInfo(int32 account_id, CharacterSelect_Struct* cs,CharWeapon_Struct* weapons);
	unsigned long	GetPlayerProfile(int32 account_id, char* name, PlayerProfile_Struct* pp);
	bool	SetPlayerProfile(int32 account_id, char* name, PlayerProfile_Struct* pp);
	bool	CreateSpawn2(int32 spawngroup, char* zone, float heading, float x, float y, float z, int32 respawn, int32 variance);
	bool	CheckNameFilter(char* name);
	bool	AddToNameFilter(char* name);
	bool	CheckUsedName(char* name);
	bool	ReserveName(int32 account_id, char* name);
	bool	CreateCharacter(int32 account_id, char* name, int16 gender, int16 race, int16 class_, int8 str, int8 sta, int8 cha, int8 dex, int8 int_, int8 agi, int8 wis, int8 face);
	bool	CreateCharacter(int32 account_id, PlayerProfile_Struct* pp);
	bool	DeleteCharacter(char* name);
	bool    SetStartingItems(PlayerProfile_Struct *cc, int8 si_race, int8 si_class, char* si_name);
	bool	SetStartingLocations(PlayerProfile_Struct *cc, int8 sl_race, int8 sl_class, char* sl_name);

	int32	GetAccountIDByChar(char* charname);
	int32	GetAccountIDByName(char* accname);
	void	GetAccountName(int32 accountid, char* name);
	void	GetCharacterInfo(char* name, int32* charid, int32* guilddbid, int8* guildrank);

	bool	OpenQuery(char* zonename);
	bool	GetVariable(char* varname, char* varvalue, int16 varvalue_len);
	bool	SetVariable(char* varname, char* varvalue);
	
	int32	GetMerchantData(int32 merchantid, int32 slot);
	int32	GetMerchantListNumb(int32 merchantid);
	int32   MerchantHasItem(int32 merchantid,int32 item_id);
	int32   GetMerchantStack(int32 merchantid,int32 item_id);
	bool    AddMerchantItem(int32 merchantid,int32 item_id,int32 stack,int32 slot);
	bool	RemoveMerchantItem(int32 merchantid,int32 item_id);
	bool    UpdateStack(int32 merchantid,int32 stack,int32 item_id);
	bool	UpdateSlot(int32 merchantid,int32 slot,int32 item_id);
	bool	UpdateAfterWithdraw(int32 merchantid,int32 startingslot,int32 lastslot);
	int32	GetLastSlot(int32 merchantid,int32 startingslot);
	bool	GetSafePoints(char* short_name, float* safe_x = 0, float* safe_y = 0, float* safe_z = 0, int8* minstatus = 0, int8* minlevel = 0, float* heading = 0, bool GM = false);
	bool	LoadItems_Old();
#ifndef EQC_SHAREDMEMORY
	bool	LoadItems();
#endif
	bool	LoadNPCTypes(char* zone_name);
	Item_Struct*	GetItem(uint32 id);
	NPCType*		GetNPCType(uint32 id);

	// -- Faction --
	bool	GetNPCFactionList(int32 npcfaction_id, sint32* faction_id, sint32* value, sint32* primary_faction = 0);
	bool	GetNPCPrimaryFaction(int32 npc_id, int32* faction_id, sint32* value); // rembrant, needed for factions Dec, 16 2001
	bool	GetFactionData(FactionMods* fm, sint32 class_mod, sint32 race_mod, sint32 deity_mod, sint32 faction_id);
	bool	GetFactionName(sint32 faction_id, char* name, int32 buflen); // rembrant, needed for factions Dec, 16 2001
	bool	SetCharacterFactionLevel(int32 char_id, sint32 faction_id, sint32 value,LinkedList<FactionValue*>* val_list); // rembrant, needed for factions Dec, 16 2001
	bool	LoadFactionData();
	bool	LoadFactionValues(int32 char_id, LinkedList<FactionValue*>* val_list);
	bool	LoadFactionValues_result(MYSQL_RES* result, LinkedList<FactionValue*>* val_list);
	const	NPCFactionList*	GetNPCFactionList(uint32 id);
	bool	LoadNPCFactionLists();
	int32	GetNPCFactionID(int32 npc_id);
	int32	GetPrimaryFaction(char* name);
	// -- End Faction --

	int		GetFreeGroupID();
	void	SetGroupID(char* charname, int GID);
	void	SetGroupID(char* charname, int GID, int isleader);
	int		GetGroupID(char* charname);

	int32	GetAccountIDFromLSID(int32 lsaccount_id);
	int32	GetLSIDFromAccountID(int32 account_id);

	bool	SetLSAuthChange(int32 account_id, char* ip);
	bool	UpdateLSAccountAuth(int32 account_id, int8* auth);
	int32	GetLSAuthentication(int8* auth);
	int32	GetLSAuthChange(char* ip);
	bool	ClearLSAuthChange();
	bool	RemoveLSAuthChange(int32 account_id);
	bool	GetLSAccountInfo(int32 account_id, char* name, int8* lsadmin, int* status);
	sint8	CheckWorldAuth(char* account, char* password, int32* account_id, int32* admin_id, bool* GreenName, bool* ShowDown);
	bool	UpdateWorldName(int32 accountid, char* name);
	void	Database::GetBook(char* txtfile, char* txtout);
	void	LoadWorldList();
	
	// -- Bug Report -
	void    AddBugReport(int crashes, int duplicates, char* player, char* report);
	// -- End Bug Report --

	void MakePet(Make_Pet_Struct* pet,int16 id,int16 type);

	// Harakiri, methods for searching a recipe based on the items in the combine
	bool GetTradeRecipe(uint32 recipe_id, uint16 containerID, DBTradeskillRecipe_Struct *spec);
	bool GetTradeRecipe(Combine_Struct* combin, DBTradeskillRecipe_Struct *spec);
	bool SearchTradeRecipe(const char* recipeName, int recipeID,  std::vector<DBTradeskillRecipe_Struct*>* specList);
	Item_Struct* GetItemNonBlob(sint32 itemID);

#ifndef EQC_SHAREDMEMORY
	uint32			max_item;
	Item_Struct**	item_array;
#endif
	uint32			max_npc_type;
	NPCType**		npc_type_array;
	uint32			max_faction;
	Faction**		faction_array;

	void AddLootTableToNPC(int32 loottable_id, ItemList* itemlist, int32* copper, int32* silver, int32* gold, int32* plat, int8 EquipmentList[], int32 EquipmentColorList[]);
	bool UpdateZoneSafeCoords(char* zonename, float x, float y, float z);
	bool UpdateNpcSpawnLocation(int32 spawngroupid, float x, float y, float z, float heading); // maalanar: used to move a spawn location in spawn2
	bool DeleteNpcSpawnLocation(int32 spawngroupid); // maalanar: used to delete spawn info from spawn2

	const char* ReturnEscapedString(char* statement);
	int32   GetZoneForage(char ZoneID[], int8 skill);
	int32	GetZoneFishing(const char* zone, int8 skill);

	// -- Guilds --
	int32	GetGuildEQID(int32 guilddbid);
	bool	SetGuild(int32 charid, int32 guilddbid, int8 guildrank);
	int32	GetFreeGuildEQID();
	int32	CreateGuild(char* name, int32 leader);
	bool	DeleteGuild(int32 guilddbid);
	bool	RenameGuild(int32 guilddbid, char* name);
	bool	EditGuild(int32 guilddbid, int8 ranknum, GuildRankLevel_Struct* grl);
	int32	GetGuildDBIDbyLeader(int32 leader);
	bool	SetGuildLeader(int32 guilddbid, int32 leader);
	bool	SetGuildMOTD(int32 guilddbid, char* motd);
	bool	GetGuildMOTD(int32 guilddbid, char* motd);
	// -- End Guilds --

	// Pinedepain // -- Item data --
	uint8  GetBardType(int16 item_id); // Pinedepain // Return the bard type modifier (instrument)
	uint8  GetBardValue(int16 item_id); // Pinedepain // Return the bard value modifier (instrument)
	// Pinedepain // -- End Item data --

	//Yeahlight: Zoneline Management
	bool	loadZoneLines(vector<zoneLine_Struct*>* zone_line_data, char* zoneName);
	bool	FindMyZoneInLocation(const char* zone, const char* zoneLine, float& x, float& y, float& z, int8& heading);
	//Yeahlight: End Zoneline Management

	//Yeahlight: Account Management
	bool	LogAccountInPartI(int32 ip, const char* WorldAccount);
	bool	LogAccountInPartII(int32 lsaccount_id, int32 ip);
	bool	LogAccountInPartIII(int32 lsaccount_id, int32 ip);
	bool	LogAccountInPartIV(int32 lsaccount_id, int32 ip);
	bool	LogAccountOut(int32 account_id);
	bool	TruncateActiveAccounts();
	bool	PurgeStuckAccounts();
	//Yeahlight: End Account Management

	// Wizzel: Highest level
	int8	highestlevel; // Will be worked into exploit check.

	//Yeahlight: Roam Range Management
	int32	GetSpawnGroupID(int32 NPCtype_id);
	bool	AddRoamingRange(int32 spawngroupID, int range);
	bool	LoadRoamBoxes(const char* zone, RoamBox_Struct roamBoxes[], int16& numberOfRoamBoxes);
	//Yeahlight: End Roam Range Management

	//Yeahlight: Patrolling Nodes
	int16	LoadZoneID(const char* zone);
	bool	LoadPatrollingNodes(int16 zoneID);
	//Yeahlight: End Patrolling Nodes

	bool	LoadNPCSpells(int8 NPCclass, int8 NPClevel, int16 buffs[], int16 debuffs[]);

	//Wizzel: Boats (Not all are used ATM)
	/*bool	AddPlayerToBoat(char* name, int8 boat_id);
	bool	RemovePlayerFromBoat(char* name);
	//void	IDtoString(int32 character_id, char* seamen);
	void	FindPlayersOnBoat(int8 boat_id, char* passenger);
	void	BoatZoneline(int8 boat_id);
	void	TeleportBoatPassengers();*/
	//Wizzel: End Boats
	
	//Tazadar : To get the boat infos
	bool LoadBoatZones(const char* boat, vector <string> &boat_zones);
	bool LoadBoatData(const char* boatname, NPCType& boat,bool& lineroute);

	// Kibanu : Time of Day
	bool UpdateTimeOfDay(int8 hour, int8 day, int8 month, int16 year);
	bool LoadTimeOfDay(int8* hour, int8* day, int8* month, int16* year);
	void SetDaytime(bool isDaytime);
	bool IsDaytime();

	// Harakiri: Message Board functions
	bool DeleteMBMessage(int category, int id);
	bool AddMBMessage(MBMessage_Struct* message);
	bool GetMBMessage(int category, int id, char* messageText);
	bool GetMBMessages(int category, std::vector<MBMessage_Struct*>* messageList);

	// Singleton Pattern: Singleton Instance -- Dark-Prince
	// Allows access to the private static instance of 'database'
	// Note: The Database Class has had its constructors made private to enforce all database calls go though this accessor
	// More Informatiuion: http://en.wikipedia.org/wiki/Singleton_pattern
	static	Database* Instance();

	// Added for use with Singleton Patern
	Database(const Database&);
    Database& operator= (const Database&);

	bool	Worldkick(int16 LSaccountID);
	bool	AdjustFaction(int16 characterID, int16 pFaction, sint16 newValue);
	bool	LoadZoneRules(char* short_name, int8& bindCondition, int8& levCondition, bool& outDoorZone);

	queue<int32> GetCorpseAccounts();
	bool	DecrementCorpseRotTimer(int32 accountid);
	int32	GetPCCorpseRotTime(int32 dbid);
	bool	CheckCorpseRezTime(int32 dbid);
	bool	SetCorpseRezzed(int32 dbid);
	bool	GetCorpseRezzed(int32 dbid);

	bool	GetZoneShortName(int16 zoneID, char* name);

	int8	CheckNPCTypeSpawnLimit(int32 NPCTypeID); // Kibanu - Spawn Limit

	// Wizzel: Load zone appearance infomation from the database
	void	GetZoneCFG(int16 zoneID, uint8* sky, int8* fog_red, int8* fog_green, int8* fog_blue, float* fog_minclip, float* fog_maxclip, float* minclip, float* maxclip, int8* ztype, float* safe_x, float* safe_y, float* safe_z, float* underworld);

protected:
	void	AddLootDropToNPC(int32 lootdrop_id, ItemList* itemlist, int8 EquipmentList[], int32 EquipmentColorList[]);
	
private:
	bool	ClearAuthentication(int32 account_id);
	uint32			npcfactionlist_max;
	NPCFactionList**	npcfactionlist_array;

	// Singleton Pattern: Singleton Private Instance -- Dark-Prince
	// The Static Instance of 'Database' which will be created once and reused over and over.
	// Note: The Database Class has had its constructors made private to enforce all database calls go though this accessor
	// More Informatiuion: http://en.wikipedia.org/wiki/Singleton_pattern
	static Database* pinstance;

	// Singleton Pattern: Private Constructors-- Dark-Prince
	// The constructors for class 'Database' have been marked private.
	// This is to ensure the use of the sole Singleton Instance
	// More Informatiuion: http://en.wikipedia.org/wiki/Singleton_pattern
	// NOTE: The default constuctor for 'Database' has been made private to enforce use of the Singleton Instance
	Database();
	// NOTE: The below constuctor for 'Database' has been made private to enforce use of the Singleton Instance
	Database(const char* host, const char* user, const char* passwd, const char* database);
	

	
};

#endif

