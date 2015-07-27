//Harakiri Sept. 23 2009
//Put method signatures into this file which should be accessable from perl.

	void SignalNPC(int _signal_id);	
	void	AddItem(int32 itemid, int8 charges, int8 slot = 0);
	void	AddLootTable();	
	void	ClearItemList();
	void	AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum);
	void	RemoveCash();
	int32	CountLoot();
	inline int32	GetLoottableID()	{ return loottable_id; }
	inline uint32	GetCopper()		{ return copper; }
	inline uint32	GetSilver()		{ return silver; }
	inline uint32	GetGold()		{ return gold; }
	inline uint32	GetPlatinum()	{ return platinum; }

	
	
	sint32	GetNPCHate(Mob* in_ent)  {return hate_list.GetEntHate(in_ent);}
  	
	
