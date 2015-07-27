/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2004  EQEMu Development Team (http://eqemulator.net)

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
#ifndef __QUEST_MANAGER_H__
#define __QUEST_MANAGER_H__

#include "Mutex.h"
#include "timer.h"
#include "item.h"

#include <string>
#include <list>
using namespace std;

#define BASEDIR			"./"

#ifndef QUEST_DIR
  #define QUEST_DIR		BASEDIR "quests"
#endif

class NPC;
class Client;

class QuestManager {
public:
	QuestManager();
	virtual ~QuestManager();
	
	void StartQuest(Mob *_owner, Client *_initiator = NULL, ItemInst* _questitem = NULL);
	void EndQuest();
	
	void Process();
	
	void ClearTimers(Mob *who);
	
	//quest perl functions	
	void echo(MessageFormat type, const char *str);
	void say(const char *str);
	void say(const char *str, int8 language);
	void me(const char *str);
	void summonitem(int32 itemid, uint8 charges = 0);	
	void write(const char *file, const char *str);
	int16 spawn2(int npc_type, int grid, int unused, float x, float y, float z, float heading);
	int16 unique_spawn(int npc_type, int grid, int unused, float x, float y, float z, float heading = 0);	
	void castspell(int spell_id, int target_id);
	void selfcast(int spell_id);
	void addloot(int item_id, int charges = 0);
	void Zone(const char *zone_name);
	void settimer(const char *timer_name, int seconds);
	void stoptimer(const char *timer_name);
	void emote(const char *str);
	void shout(const char *str);
	void shout2(const char *str);
	void depop(int npc_type = 0);
	void depopall(int npc_type = 0);
	void depopzone(bool StartSpawnTimer = true);	
	void settarget(const char *type, int target_id);
	void follow(int entity_id);
	void exp(int amt);
	void level(int newlevel);
	void safemove();
	void rain(int weather);
	void snow(int weather);
	void surname(const char *name);
	void permaclass(int class_id);
	void permarace(int race_id);
	void permagender(int gender_id);
	uint16 scribespells(uint8 max_level, uint8 min_level = 1);	
	void unscribespells();
	void givecash(int32 npcTypeID, int copper, int silver, int gold, int platinum);
	void pvp(const char *mode);	
	void gmmove(float x, float y, float z);
	void doanim(int anim_id);
	void attack(const char *client_name);
	void attacknpc(int npc_entity_id);
	void attacknpctype(int npc_type_id);
	void save();
	void faction(int faction_id, int faction_value);
	void signal(int npc_id, int wait_ms = 0);
	void signalwith(int npc_id, int signal_id, int wait_ms = 0);
	void setglobal(const char *varname, const char *newvalue, int options, const char *duration);
	void targlobal(const char *varname, const char *value, const char *duration, int npcid, int charid, int zoneid);
	void delglobal(const char *varname);
	void ding();
	void start(int wp);
	void stop();
	void pause(int duration);
	void moveto(float x, float y, float z, float h, bool saveguardspot);
	void resume();
	void addldonpoints(sint32 points, int32 theme);
	void setnexthpevent(int at);
	void setnextinchpevent(int at);
	void respawn(int npc_type, int grid);
	void set_proximity(float minx, float maxx, float miny, float maxy, float minz=-999999, float maxz=999999);
	void clear_proximity();
	void setanim(int npc_type, int animnum);
	void pathto(float x, float y, float z);
	void spawn_condition(const char *zone_short, uint16 condition_id, short new_value);
	short get_spawn_condition(const char *zone_short, uint16 condition_id);
	void toggle_spawn_event(int event_id, bool enable, bool reset_base);
	bool has_zone_flag(int zone_id);
	void set_zone_flag(int zone_id);
	void clear_zone_flag(int zone_id);
	void sethp(int hpperc);
	bool summonburriedplayercorpse(int32 char_id, float dest_x, float dest_y, float dest_z, float dest_heading);
	int32 getplayerburriedcorpsecount(int32 char_id);
	bool buryplayercorpse(int32 char_id);
	void forcedooropen(int32 doorid);
	void forcedoorclose(int32 doorid);
	bool isdooropen(int32 doorid);
	void npcrace(int race_id);
	void npcgender(int gender_id);
	void npcsize(int newsize);
	void npctexture(int newtexture);
	void playerrace(int race_id);
	void playergender(int gender_id);
	void playersize(int newsize);
	void playertexture(int newtexture);
	void popup(char *title, char *text, int32 popupid, int32 buttons, int32 Duration);


    void clearspawntimers();
	void ze(int type, const char *str);
	void we(int type, const char *str);
    int getlevel(uint8 type);
    int collectitems(uint32 item_id, bool remove);
    int collectitems_processSlot(sint16 slot_id, uint32 item_id, bool remove);

	void CreateGroundObject(int32 itemid, float x, float y, float z, float heading, int32 decay_time = 300000);
	void ModifyNPCStat(const char *identifier, const char *newValue);
	void UpdateSpawnTimer(int32 id, int32 newTime);

	void MerchantSetItem(int32 NPCid, int32 itemid, int32 quantity = 0);
	int32 MerchantCountItem(int32 NPCid, int32 itemid);

	const char* varlink(char* perltext, int item_id);
	const char* saylink(char* Phrase, bool silent, char* LinkName);
	void SetRunning(bool val);
	bool IsRunning();
	void FlyMode(int8 flymode);
	uint8 FactionValue();

	//not in here because it retains perl types
	//thing ChooseRandom(array_of_things)
	
	inline Client *GetInitiator() const { return(initiator); }
	inline NPC *GetNPC() const { return(owner->IsNPC()?owner->CastToNPC():NULL); }
	inline Mob *GetOwner() const { return(owner); }
	inline ItemInst *GetQuestItem() const {return questitem; }
	inline bool ProximitySayInUse() { return HaveProximitySays; }
	Timer* quest_timers;	
	int InsertQuestGlobal(int charid, int npcid, int zoneid, const char *name, const char *value, int expdate);
	void AddPendingQuestItem(int charid, int npcid, int zoneid, const char *itemid);
	vector<string> GetPendingQuestItems(int npcid, int zoneid);
	void DeletePendingQuestItem(const char *itemid);

protected:
	Mob *owner;	//NPC is never NULL when functions are called.
	Client *initiator;	//this can be null.
	ItemInst* questitem;	// this is usually NULL.

	
	bool depop_npc;	//true if EndQuest should depop the NPC
	
	Mutex quest_mutex;
	bool HaveProximitySays;
	
	int QGVarDuration(const char *fmt);
	
	class QuestTimer {
	public:
		inline QuestTimer(int duration, Mob *_mob, string _name) : mob(_mob), name(_name), Timer_(duration) { Timer_.Start(duration, false); }
		Mob*   mob;
		string name;
		Timer Timer_;
	};
	class SignalTimer {
	public:
		inline SignalTimer(int duration, int _npc_id, int _signal_id) : npc_id(_npc_id), signal_id(_signal_id), Timer_(duration) { Timer_.Start(duration, false); }
		int npc_id;
		int signal_id;
		Timer Timer_;
	};
	list<QuestTimer>	QTimerList;
	list<SignalTimer>	STimerList;

};

extern QuestManager quest_manager;

#endif

