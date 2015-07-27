#include "client.h"
#include "skills.h"
#include "itemtypes.h"
#include "zone.h"
#include "watermap.h"
#include "math.h"

using namespace EQC::Zone;

extern Zone*			zone;

#define MAX_COMMON_FISH_IDS 9
#define BAIT_ITEM_ID 9

/********************************************************************
 *                      Harakiri - 8/16/2009                        *
 ********************************************************************
 *		       ProcessOP_Fishing								    *
 ********************************************************************
	Handle fishing request from client, checks if client
	is near water (TBD)
 ********************************************************************/
void Client::ProcessOP_Fishing(APPLAYER *pApp)
{
	
	//Yeahlight: Purge client's invisibility
	CancelAllInvisibility();

	if(CanFish()) {			
		// Harakiri: in Client_Process() Process it will be checked
		// if the timer is up, then it will call GoFish();
		fishing_timer->Start();	
	}	
	return;
}

/********************************************************************
 *                      Harakiri - 8/16/2009                        *
 ********************************************************************
 *		       GoFish											    *
 ********************************************************************
	Method is called by Client_Process Process when the timer is 
	finished, checks for zone specific fish loot, if nothing found
	it will return on success a common fish loot
 ********************************************************************/
void Client::GoFish()
{

	
	fishing_timer->Disable();


	
	// check again if client has moved while timer is running
	if (!CanFish()) {	//if we can't fish here, we don't need to bother with the rest
		return;
	}

	//multiple entries yeilds higher probability of dropping...
	uint32 common_fish_ids[MAX_COMMON_FISH_IDS] = {
		1038, // Tattered Cloth Sandals
		1038, // Tattered Cloth Sandals
		1038, // Tattered Cloth Sandals
		13019, // Fresh Fish
		13076, // Fish Scales
		13076, // Fish Scales
         7007, // Rusty Dagger
		 7007, // Rusty Dagger
         7007 // Rusty Dagger		
	};
	
	//success formula is not researched at all	
	int fishing_skill = GetSkill(FISHING);	//will take into account skill bonuses on pole & bait
	
	//make sure we still have a fishing pole on:
	sint32 bslot = FindItemTypeInInventory(ItemTypeFishingBait);
				
	// just to be sure
	if(bslot == 0) {
		//Message(DARK_BLUE, "You can't fish without fishing bait, go buy some.");		
		return;
	}
		
	if (fishing_skill > 100)
	{
		fishing_skill = 100+((fishing_skill-100)/2);
	}
	
	if (MakeRandomInt(0,175) < fishing_skill) {
		uint32 food_id = 0;
		
		//25% chance to fish a zone specific item at max skill.
        if (MakeRandomInt(0, 399) <= fishing_skill ) {					
			food_id = Database::Instance()->GetZoneFishing(pp.current_zone, fishing_skill);					
		}
		
		//consume bait, should we always consume bait on success?
		RemoveOneCharge(bslot,true);
		
		// fallback to default ids
		if(food_id == 0 || Database::Instance()->GetItem(food_id) == 0) {
			int index = MakeRandomInt(0, MAX_COMMON_FISH_IDS-1);
			food_id = common_fish_ids[index];
		}
		
		Message(DARK_BLUE, "You caught, something...");
		
		if(CheckLoreConflict(food_id))
			{
				;// handled by CheckLoreConflict Message(RED,"Duplicate lore items are not allowed.");
			}
		else
			{
				SummonItem(food_id);
			}				
	}
	else
	{
		//chance to use bait when you dont catch anything...
		if (MakeRandomInt(0, 4) == 1) {
			// remove bait
			RemoveOneCharge(bslot,true);		
			Message(DARK_BLUE, "You lost your bait!");	
		} else {
			if (MakeRandomInt(0, 15) == 1)	//give about a 1 in 15 chance to spill your beer. we could make this a rule, but it doesn't really seem worth it
				//TODO: check for & consume an alcoholic beverage from inventory when this triggers, and set it as a rule that's disabled by default
				Message(DARK_BLUE, "You spill your beer while bringing in your line.");	//You spill your beer while bringing in your line.
			else
				Message(DARK_BLUE, "You didn't catch anything.");	
		}
	}
	
	//chance to break fishing pole...
	//this is potentially exploitable in that they can fish
	//and then swap out items in primary slot... too lazy to fix right now
	if (MakeRandomInt(0, 49) == 1) {
		Message(DARK_BLUE, "Your fishing pole broke!");	//Your fishing pole broke!
		DeleteItemInInventory(SLOT_PRIMARY);
	}

	CheckAddSkill(FISHING,5);	
}

/********************************************************************
 *                      Harakiri - 8/16/2009                        *
 ********************************************************************
 *		       GetZoneFishing									    *
 ********************************************************************
	Returns a zone specific fish by random
 ********************************************************************/
int32 Database::GetZoneFishing(const char* zone, int8 skill)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	int8 index = 0;
	int32 item[50];	
	int32 ret = 0;
	
	for (int c=0; c<50; c++) 	{
		item[c]=0;
	}
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT itemid FROM fishing WHERE (shortname= '%s')",zone), errbuf, &result))
	{
		cout << query << endl;
		safe_delete_array(query);
		while ((row = mysql_fetch_row(result))&&(index<50)) 	{
			item[index] = atoi(row[0]);		
			index++;
		}
		
		mysql_free_result(result);
	}
	else {
		cerr << "Error in Fishing query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	
	// return a random item ID 
	if (index>0) {
		ret = item[rand()%index];
	} else {
		ret = 0;
	}
	
	return ret;
}

/********************************************************************
 *                      Harakiri - 8/16/2009                        *
 ********************************************************************
 *		       CanFish											    *
 ********************************************************************
	Checks if the player is near water
 ********************************************************************/
bool Client::CanFish() {
	
	if(zone->map!=NULL && zone->watermap != NULL) {
		float RodX, RodY, RodZ;
		// Tweak Rod and LineLength if required
		const float RodLength = 30;
		// Harakiri at qeynos docks its at least 60 + zpos over water
		const float LineLength = 70;
		int HeadingDegrees;

		HeadingDegrees = (int) ((GetHeading()*360)/256);
		HeadingDegrees = HeadingDegrees % 360;
		
		RodX = GetX() + RodLength * sin(HeadingDegrees * M_PI/180.0f);
		RodY = GetY() + RodLength * cos(HeadingDegrees * M_PI/180.0f);
		
		// Do BestZ to find where the line hanging from the rod intersects the water (if it is water).
		// and go 1 unit into the water.
		VERTEX dest;
		dest.x = RodX;
		dest.y = RodY;
		dest.z = GetZ();//+10;
		NodeRef n = zone->map->SeekNode( zone->map->GetRoot(), dest.x, dest.y);
		if(n != NODE_NONE) {
			RodZ = zone->map->FindBestZ(n, dest, NULL, NULL) - 1;
			bool in_lava = zone->watermap->InLava(RodX, RodY, RodZ);
			bool in_water = zone->watermap->InWater(RodX, RodY, RodZ);
		//	Message(WHITE, "Rod is at %4.3f, %4.3f, %4.3f, InWater says %d, InLava says %d", RodX, RodY, RodZ, in_water, in_lava);
			if (in_lava) {
				Message(DARK_BLUE, "Trying to catch a fire elemental or something?");
				return false;
			}
				//Message(WHITE, "ZPOS SUM %4.3f current zpos %4.3f", (z_pos-RodZ),z_pos);
		
			if((!in_water) || (z_pos-RodZ)>LineLength) {	//Didn't hit the water OR the water is too far below us
				Message(DARK_BLUE, "Trying to catch land sharks perhaps?");
				return false;
			}
		}
	}
	return true;	
}                                                                                 