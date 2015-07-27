 
//Harakiri Sept. 23 2009
//Put method signatures into this file which should be accessable from perl.



	void echo(MessageFormat type, const char *str);
	void say(const char *str, int8 language = 0);
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
	void settarget(const char *type, int target_id);
	void follow(int entity_id);
	void exp(int amt);
	void level(int newlevel);
	void rain(int weather);
	void snow(int weather);
	void givecash(int32 npcTypeID, int copper, int silver, int gold, int platinum);
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
	void setnexthpevent(int at);
	void setnextinchpevent(int at);
	void respawn(int npc_type, int grid);
	void set_proximity(float minx, float maxx, float miny, float maxy, float minz=-999999, float maxz=999999);
	void clear_proximity();
	void setanim(int npc_type, int animnum);
	void npcrace(int race_id);
	void npcgender(int gender_id);
	void npcsize(int newsize);
	void npctexture(int newtexture);
	void playerrace(int race_id);
	void playergender(int gender_id);
	void playertexture(int newtexture);
	

  int getlevel(uint8 type);
  
	void CreateGroundObject(int32 itemid, float x, float y, float z, float heading, int32 decay_time = 300000);
	
	uint8 FactionValue();

	inline bool ProximitySayInUse() { return HaveProximitySays; }