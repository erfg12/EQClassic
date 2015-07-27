#include "object.h"

#include "../../common/include/packet_functions.h"

extern EntityList entity_list;
extern Zone* zone;

//Object List
//IT70 : Brewery

Object::Object(int16 type, int16 ItemID, float YPos, float XPos, float ZPos, int8 Heading, char ObjectName[6], bool clientDrop)
{
	itemid = ItemID;
	dropid = 0;
	ypos = YPos;
	xpos = XPos;
	zpos = ZPos;
	heading = Heading;
	memcpy(objectname,ObjectName,16);
	objecttype = type;
	isCoin = false;

	//Yeahlight: Client item drop expire support
	expire_timer = new Timer(DROPPED_ITEM_DURATION);
	if(clientDrop)
	{
		droppedItem = true;
	}
	else
	{
		droppedItem = false;
		expire_timer->Disable();
	}
}

//Overloaded constructor to take an object struct as read from the db.
Object::Object(Object_Struct* os, int16 type, bool clientDrop)
{
	itemid = os->itemid;
	dropid = os->dropid;
	this->itemporp.stack_size = os->stack_size;
	memcpy(bagitemporp,os->item_data,10*sizeof(Object_Data_Struct));
	ypos = os->ypos;
	xpos = os->xpos;
	zpos = os->zpos;
	heading = os->heading;
	memcpy(objectname,os->objectname,16);
	objecttype = type;
	m_type=os->type;
	m_icon=os->icon_nr;
	m_open = 1;
	m_playerid = 0;
	memset(this->stationItemProperties, 0, sizeof(this->stationItemProperties));
	memset(this->stationItems, 0, sizeof(this->stationItems));
	memset(bagitems, 0, sizeof(bagitems));
	memcpy(bagitems, os->itemsinbag, 20);
	offset = 1;
	isCoin = false;

	//Yeahlight: Client item drop expire support
	expire_timer = new Timer(DROPPED_ITEM_DURATION);
	if(clientDrop)
	{
		droppedItem = true;
	}
	else
	{
		droppedItem = false;
		expire_timer->Disable();
	}
}

//Yeahlight: Overloaded constructor for coin objects
Object::Object(DropCoins_Struct* dc, int16 type, bool clientDrop)
{
	memset(stationItemProperties, 0, sizeof(stationItemProperties));
	memset(stationItems, 0, sizeof(stationItems));
	memset(bagitems, 0, sizeof(bagitems));
	ypos = dc->yPos;
	xpos = dc->xPos;
	zpos = dc->zPos;
	objecttype = type;
	itemid = 0;
	dropid = 0;
	strcpy(objectname, dc->coinName);
	isCoin = true;
	coins[0] = dc->copperPieces;
	coins[1] = dc->silverPieces;
	coins[2] = dc->goldPieces;
	coins[3] = dc->platinumPieces;

	//Yeahlight: Client item drop expire support
	expire_timer = new Timer(DROPPED_ITEM_DURATION);
	if(clientDrop)
	{
		droppedItem = true;
	}
	else
	{
		droppedItem = false;
		expire_timer->Disable();
	}
}

//Yeahlight: Overloaded constructor for item objects
Object::Object(Item_Struct* item, float x, float y, float z, Client* client, int8 slot, int16 type, bool clientDrop)
{
	memset(stationItemProperties, 0, sizeof(stationItemProperties));
	memset(stationItems, 0, sizeof(stationItems));
	memset(bagitems, 0, sizeof(bagitems));
	dropid = 0;
	itemid = item->item_nr;
	objecttype = type;
	ypos = y;
	xpos = x;
	zpos = z;
	//Yeahlight: Keep this at 0 so all weapons face the same direction at all times
	heading = 0;
	strcpy(objectname, item->idfile);
	strcat(objectname, "_ACTORDEF");
	isCoin = false;
	//Yeahlight: Object is a container
	if(item->type == 0x01)
	{
		for(int i = 0; i < 10; i++)//Tazadar: Bags are now filled :)
		{
			
			if(client->TradeWithEnt){//Tazadar:If we accepted the trade we look in the other client infos
				bagitems[i] = entity_list.GetMob(client->TradeWithEnt)->CastToClient()->TradeList[i+(slot+1)*10];
				bagitemporp[i].stack_size = entity_list.GetMob(client->TradeWithEnt)->CastToClient()->TradeCharges[i+(slot+1)*10];
			}
			else{//Tazadar: We canceled the trade !
				bagitems[i] = client->TradeList[i+(slot+1)*10];
				bagitemporp[i].stack_size = client->TradeCharges[i+(slot+1)*10];
			}
		}
	}

	//Yeahlight: Client item drop expire support
	expire_timer = new Timer(DROPPED_ITEM_DURATION);
	if(clientDrop)
	{
		droppedItem = true;
	}
	else
	{
		droppedItem = false;
		expire_timer->Disable();
	}
}

Object::~Object()
{
	safe_delete(expire_timer);
}

bool Object::Process()
{
	//Yeahlight: It is time for the dropped item to go bye-bye
	if(droppedItem && expire_timer->Check())
	{
		//Yeahlight: Despawn the object from the zone
		APPLAYER outapp;
		CreateDeSpawnPacket(&outapp);
		entity_list.QueueClients(0, &outapp, false);
		//Yeahlight: Remove the object from the zone's object list
		for(int i = 0; i < zone->object_list.size(); i++)
		{
			if(zone->object_list.at(i) == this)
			{
				zone->object_list.erase(zone->object_list.begin() + i);
			}
		}
		return false;
	}
	return true;
}

void Object::CreateSpawnPacket(APPLAYER* app)
{
	app->opcode = OP_DropItem;
	app->pBuffer = new uchar[sizeof(Object_Struct)];
	memset(app->pBuffer,0,sizeof(Object_Struct));

	app->size = sizeof(Object_Struct);	
	Object_Struct* co = (Object_Struct*) app->pBuffer;	

	int8 test_value = 0x00;
	int8 test_value2 = 0x01;

	if(objecttype == 0){
		
		for(int i=0;i<1;i++){
			co->unknown_1 = test_value;
			co->unknown_1b = test_value;
		}
		for(int i=0;i<11;i++){
			co->unknown_11[i] = test_value;
		}
		for(int i=0;i<14;i++){
			co->unknown_14[i] = test_value;
		}
		for(int i=0;i<24;i++){
			co->unknown_24[i] = test_value;
		}
		for(int i=0;i<4;i++){
			co->unknown_4[i] = test_value;
			co->unknown_4b[i] = test_value2;
		}
		for(int i=0;i<5;i++){
			co->unknown_5[i] = test_value2;
		}
		for(int i=0;i<6;i++){
			co->unknown_6[i] = test_value2;
		}

		for(int i=0;i<8;i++){
			co->unknown_12[i] = test_value2;
		}

		cout << "\nLoading a world object!\n";

		co->itemid = 17005;
	}

	//co->type = 0x0001;
	
	co->dropid = this->GetID();
	co->xpos = this->xpos;
	co->ypos = this->ypos;
	co->zpos = this->zpos;
	memcpy(co->objectname,this->objectname,16);
	co->heading = this->heading;
	memcpy(co->itemsinbag, this->bagitems, 20);
	offset = 1;

}

void Object::CreateDeSpawnPacket(APPLAYER* app)
{	
	app->opcode = OP_PickupItem;
	app->pBuffer = new uchar[sizeof(ClickObject_Struct)];
	app->size = sizeof(ClickObject_Struct);	
	ClickObject_Struct* co = (ClickObject_Struct*) app->pBuffer;		
	co->objectID = GetID();
	co->PlayerID = 0;
}

//Todo: -Lore item should not disapear for others when someone trying to duplicate it.
//      -When someone zone in a zone that already have ground items , the player should be able to see it
//Yeahlight: Note: Only return TRUE if the item was actually picked up (do not use TRUE for crafting stations!)
bool Object::HandleClick(Client* sender, APPLAYER* app)
{
	int i;
	switch(objecttype)
	{
		case OT_DROPPEDITEM:
		{
			if(this->isCoin)
			{
				//Yeahlight: Money can never be rejected on pick up, so get rid of the object spawn immediately
				entity_list.QueueClients(0, app, false);
				//Yeahlight: Update the client's inventory directly
				//           TODO: Figure out how to get the god damn coins on the cursor!
				sender->AddMoneyToPP(coins[0], coins[1], coins[2], coins[3], true);
			}
			else
			{
				for (i = 0; i != 10; i++){//Tazadar:We check the lore items in the bag before looting it !
					if (this->bagitems[i] != 0 && this->bagitems[i] != 0xFFFF){
						if(sender->CheckLoreConflict(this->bagitems[i]))
						{
							//Yeahlight: Only send the pickup packet to the conflicted client; leave the bag
							//           on the ground for the rest of the clients in the zone
							sender->QueuePacket(app);
							return false;
						}
					}
				}

				if(!sender->SummonItem(this->itemid,this->itemporp.stack_size)){//Tazadar:We check if we can loot the item(duplicate lore item or not)
					//Yeahlight: Only send the pickup packet to the conflicted client; leave the bag
					//           on the ground for the rest of the clients in the zone
					sender->QueuePacket(app);
					return false;
				}

				//Yeahlight: It is assumed at this point that there was no lore conflict, so send the removal packet to all clients
				entity_list.QueueClients(0, app, false);

				for (i = 0; i != 10; i++){//Tazadar: If we loot a bag we have to add items in bag.
					if (this->bagitems[i] != 0 && this->bagitems[i] != 0xFFFF){
						sender->AddToCursorBag(this->bagitems[i],i,this->bagitemporp[i].stack_size); 	

						/*
						Item_Struct* current_item = Database::Instance()->GetItem(this->bagitems[i]);
						int16 next_slot;
						
						//If item is a bag, find a bag slot.
						if(current_item->type == 0x01){
							next_slot = sender->FindFreeInventorySlot(true,false);
						}else{
							next_slot = sender->FindFreeInventorySlot(false,false);
						}
						
						sender->PutItemInInventory(next_slot, this->bagitems[i]);
						*/
					}
				}
			}
			entity_list.RemoveEntity(this->GetID());
			return true;
		}
		break;
		case OT_CRAFTING_CONTAINER://Tazadar: We are in using a crafting station :)
		{
			//cout << "itemid " << itemid << endl;
			APPLAYER* pApp = new APPLAYER(OP_CraftingStation, sizeof(ClickObjectAck_Struct));
			ClickObjectAck_Struct* coa = (ClickObjectAck_Struct*)pApp->pBuffer;
			memset(coa,0x00,sizeof(ClickObjectAck_Struct));
			coa->player_id = sender->GetID();
			coa->open=this->m_open;//Tazadar: We check if the station is already opened by someone
			if(this->m_open){
				//Yeahlight: Purge client's invisibility
				sender->CancelAllInvisibility();
				coa->type=this->m_type;
				coa->slot=0x0a;//Tazadar: We have 10 slots !
				coa->icon_nr=this->m_icon;
				sender->SetCraftingStation(this);
				memcpy(sender->stationItems,this->stationItems,sizeof(this->stationItems));
				memcpy(sender->stationItemProperties,this->stationItemProperties,sizeof(this->stationItemProperties));
				m_open=0;//Tazadar: Now its opened :)
				entity_list.QueueClients(0,pApp,false);
				sender->SendStationItem(this->m_playerid);
				this->m_playerid=sender->GetID();
				return false;
			}
			entity_list.QueueClients(0,pApp,false);//Tazadar: Someone is already using the station :(
			return false;
		}
		break;
		default:
		{
			//Still needs to respond so client doesn't hang.
			entity_list.QueueClients(0,app,false);
			return false;
		}
	}
}
