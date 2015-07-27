//Harakiri Sept. 23 2009
//Put method signatures into this file which should be accessable from perl.
	
	Mob*	GetMob(const char* name);
	Mob*	GetMobByNpcTypeID(int32 get_id);
	Client* GetClientByName(const char *name); 
	Client* GetClientByAccID(int32 accid);
	
	
	
	
	void ClearClientPetitionQueue();    

	void	Clear();

	
	void	Message(int32 to_guilddbid, MessageFormat type, const char* message, ...);	
	void	MessageClose(Mob* sender, bool skipsender, float dist, MessageFormat type, const char* message, ...);

	void    RemoveFromTargets(Mob* mob);
    
	
	char*	MakeNameUnique(char* name);
	static char*	RemoveNumbers(char* name);
// signal quest command support
	void	SignalMobsByNPCID(int32 npc_type, int signal_id);
	void    RemoveEntity(int16 id);

	sint32	DeleteNPCCorpses();
	sint32	DeletePlayerCorpses();

	void	ClearFeignAggro(Mob* targ);
		
	
	
