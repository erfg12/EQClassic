#ifndef OBJECT_H
#define OBJECT_H

#include "client.h"
#include "mob.h"
#include "npc.h"
#include "entity.h"

#define OT_CRAFTING_CONTAINER 0
#define OT_DROPPEDITEM		  1

class Object: public Entity
{
public:
	Object(int16 type,int16 itemid,float ypos,float xpos, float zpos, int8 heading, char objectname[6], bool clientDrop = false);
	Object(Object_Struct* os, int16 type = 0, bool clientDrop = false);
	Object(DropCoins_Struct* cs, int16 type, bool clientDrop = false);
	Object(Item_Struct* item, float x, float y, float z, Client* client, int8 slot, int16 type, bool clientDrop = false);

	~Object();

	virtual bool	IsObject() { return true; }
	virtual bool	Process();

	void	CreateSpawnPacket(APPLAYER* app);
	void	CreateDeSpawnPacket(APPLAYER* app);
	bool	HandleClick(Client* sender,APPLAYER* app);
	void	SetBagItems(int16 slot, int16 item)	{bagitems[slot] = item;}
	void	SetOpen(int16 open){m_open=open;};

	char*	GetName(){return name;};
	int32	GetItemID(){return itemid;};
	char	name[30];
	
	int		offset;
	int32	dropid;
	int16	stationItems[10];
	ItemProperties_Struct stationItemProperties[10];

private:
	
	int16	m_type;
	int16	m_open;
	int16	m_icon;
	int32	m_playerid;
	
	int32	itemid;
	Object_Data_Struct itemporp;
	float	ypos;
	float	xpos;
	float	zpos;
	int8	heading;
	char	objectname[16];
	int16	objecttype;
	int16	bagitems[10];
	Object_Data_Struct bagitemporp[10];
	Timer*  expire_timer;
	bool	droppedItem;
	bool	isCoin;
	int32	coins[4];
};

#endif