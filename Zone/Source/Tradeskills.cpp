#include "database.h"
#include "client.h"

#include <iostream>
#include <string>
#include <sstream>

#define SLOT_WORLD_CONTAINER 1000

/* Harakiri - just for reference, may need them later for fixing tradeskill db*/
#define MIN_LEVEL_ALCHEMY 25
#define ALCHEMY_BEARSKIN_POTION_BAG  17811
#define ALCHEMY_MEDICINE_BAG  17901
#define RESEARCH_CONCORDANCE_OF_RESEARCH  17504
#define FISHING_TACKLEBOX  17048
#define JEWELCRAFT_JEWELLERS_KIT  17912
#define MAKE_POISON_MORTAR_AND_PESTLE 17904
#define TAILORING_SMALL_SEWING_KIT  17908
#define TAILORING_LARGE_SEWING_KIT  17966
#define TINKERING_TOOLBOX  17902
#define TINKERING_DELUXE_TOOLBOX  17877
// this seems incorrect on the client returns bad image number
#define FLETCHING_FLETCHINT_KIT 17910
// world object types
#include "container.h"

// Harakiri some world objects to try
// use #fr = recipesearch for ID, then #sr = summonrecipe to get all objects for combine
/*
// BLACKSMITH
#zone qeynos2 ironforge - forge
#goto 79 66 0
#fr Bastard Sword

// teirdal forge
#zone neriakc
#goto 834 -1564 -808

// Koada'Dal Forge
#zone felwithea
#goto -100 -400 0

// Fier'Dal Forge
#zone gfaydark
#goto -470 81 1610

// Stormguard Forge
#zone kaladim
#goto 111 384 25

// Oggok Forge
#zone oggok
#goto 154 924 406

// Northman Forge
#zone halas
#goto 241 -321 43

// Grobb Forge
#zone grobb
#goto 538 -215 -209

// Vale Forge
#zone rivervale
#goto -3 -126 0

// Royal Erudin Forge
#zone erudnext
#goto -741 -252 527

// Cabalis Forge
#zone cabalis
#goto -530 -15 597

// Freeport Forge
#zone freeportw
#goto -348 30 -102

// POTTERY
#zone qeynos pottery wheel and kiln
#goto 540 -300 0
#fr Block of Clay

// BAKING
#zone qeynos oven - Voleen's Fine Baked Goods
#goto -150 -275 0
#fr Bear Sandwhich

// BREWING
#zone qeynos brew barrle lions inn
#goto 278 -162 0
#fr Vodka

// TINKERING Toolbox
#summonitem 17902
#fr Compass

// JEWELLCRAFT Kit
#summonitem 17912
#fr Platinum Ring

// MAKE_POISON Mortar
#summonitem 17904
#fr Aching Blood

// TAILORING Large Kit
#summonitem 17966
#fr Hand Made Backpack

// RESEARCH MAGE
#summonitem 17504
#fr greater conjuration water
*/

/********************************************************************
 *                      Harakiri - 8/13/2009                        *
 ********************************************************************
 *		       ProcessOP_TradeSkillCombine						    *
 ********************************************************************
   Handles any combine requests by the client			          
   A Combine_Struct either indicates an item is created through    
   a container in the players inventory, or through a world object 
   like a forge or oven, this is indicated in the packet by		
   containerID=SLOT_WORLD_CONTAINER (1000), in this case			
   worldobjecttype is used. The combine in itself does not 
   differentiate between tradeskills, this is all handled by the db 
   search. In essence, search a recipe which contains all the items
   in the container and has the container id. This way we are figuring
   out the tradeskill ID itself.
 ********************************************************************/
void Client::ProcessOP_TradeSkillCombine(APPLAYER* pApp){	

	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"TradeskillCombine Request");

	//DumpPacketHex(pApp);
	Combine_Struct* combin = (Combine_Struct*) pApp->pBuffer;

	// debug msg
	for(int i=0; i < sizeof(combin->iteminslot)/sizeof(int16); i++) {
		if (combin->iteminslot[i] != 0xFFFF) {
			Item_Struct* item = Database::Instance()->GetItem(combin->iteminslot[i]);
			if(item==0) {
				Message(RED,"ERROR Unknown Item in Slot #%i = %i",i,combin->iteminslot[i]);
			} else {
				EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Item in Slot #%i = %s (%i)",i,item->name,combin->iteminslot[i]);
			}
		}
	}	

	if(combin->containerID !=0 && combin->containerslot != SLOT_WORLD_CONTAINER) {
		Item_Struct* item = Database::Instance()->GetItem(combin->containerID);
			if(item==0) {
				Message(RED,"ERROR Unknown Container Item %i",combin->containerID);
			} else {
				EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Container Item %s (%i) type %s",item->name,combin->containerID, GetContainerName(item->type));
			}		
	}

	if(combin->containerslot == SLOT_WORLD_CONTAINER) {
		EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"World Container Item %s (%i)",GetContainerName(combin->worldobjecttype),combin->worldobjecttype);
	}

	// get our recipe, the struct contains the items we should
	// summon on failure and on sucess
	DBTradeskillRecipe_Struct spec;
 	if (!Database::Instance()->GetTradeRecipe(combin, &spec)) {
 		Message(DARK_BLUE,"You cannot combine these items in this container type!");
 		QueuePacket(pApp); 		
 		return;
 	}

	// some class related checks
	if(admin < 255) {
		if (spec.tradeskill == ALCHEMY) {
			if (GetClass() != SHAMAN) {
				Message(RED, "This tradeskill can only be performed by a shaman.");
				return;
			}
			else if (GetLevel() < MIN_LEVEL_ALCHEMY) {
				Message(RED, "You cannot perform alchemy until you reach level %i.", MIN_LEVEL_ALCHEMY);
				return;
			}
		} else if (spec.tradeskill == TINKERING) {
 			if (GetRace() != GNOME) {
				Message(RED, "Only gnomes can tinker.");
				return;
			}
		} else if (spec.tradeskill == MAKE_POISON) {
 			if (GetRace() != ROGUE) {
				Message(RED, "Only rogues can mix poisons.");
				return;
			}
		}
	}
	
	// this does the chance check and summons the fail or success items
	bool success = this->TradeskillExecute(&spec);
	
	
	// this is some old code to clean the items
	// we need an itemclass with better handling
	for (int k=0; k<10; k++) 
	{
		if(combin->containerslot == SLOT_WORLD_CONTAINER) {
			DeleteItemInStation();
		} else{
			
			// is there an item at the slot id?
			if ( (pp.containerinv[((combin->containerslot-22)*10) + k] != 0xFFFF)) 
			{
				EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Deleting Item at slot  %i ", ((combin->containerslot-22)*10) + k + 250);
				// delete each item in slot id
				DeleteItemInInventory(((combin->containerslot-22)*10) + k + 250);			
			}
		}
	}		
	
	QueuePacket(pApp);
}

/********************************************************************
 *                      Harakiri - 8/13/2009                        *
 ********************************************************************
 *		       Client::TradeskillExecute						    *
 ********************************************************************
  Checks if the combine should be successful depending on the skill.  
 ********************************************************************/
bool Client::TradeskillExecute(DBTradeskillRecipe_Struct *spec) {
 	if(spec == NULL)
		return(false);
	
	int16 user_skill = GetSkill(spec->tradeskill);
	float chance = 0;
	float skillup_modifier;
	sint16 thirdstat = 0;
	sint16 stat_modifier = 15;
	uint16 success_modifier;

	// Rework based on the info on eqtraders.com
	// http://mboards.eqtraders.com/eq/showthread.php?t=22246
	

	// Some tradeskills are more eqal then others. ;-)
	// If you want to customize the stage1 success rate do it here.
    // Remember: skillup_modifier is (float). Lower is better
	switch(spec->tradeskill) {
	case FLETCHING:
	case ALCHEMY:
	case JEWELRY_MAKING:
	case POTTERY:
		skillup_modifier = 4;
		break;
	case BAKING:
	case BREWING:
		skillup_modifier = 3;
		break;
	case RESEARCH:
		skillup_modifier = 1;
		break;
	default:
		skillup_modifier = 2;
		break;
	}

	// Some tradeskills take the higher of one additional stat beside INT and WIS
	// to determine the skillup rate. Additionally these tradeskills do not have an
	// -15 modifier on their statbonus.
	if (spec->tradeskill ==  FLETCHING || spec->tradeskill == MAKE_POISON) {
		thirdstat = GetDEX();
		stat_modifier = 0;
	} else if (spec->tradeskill == BLACKSMITHING) {
		thirdstat = GetSTR();
		stat_modifier = 0;
	}
	
	sint16 higher_from_int_wis = (GetINT() > GetWIS()) ? GetINT() : GetWIS();
	sint16 bonusstat = (higher_from_int_wis > thirdstat) ? higher_from_int_wis : thirdstat;
	
	vector< pair<uint32,uint8> >::iterator itr;


    //calculate the base success chance
	// For trivials over 68 the chance is (skill - 0.75*trivial) +51.5
    // For trivial up to 68 the chance is (skill - trivial) + 66
	if (spec->trivial >= 68) {
		chance = (user_skill - (0.75*spec->trivial)) + 51.5;
	} else {
		chance = (user_skill - spec->trivial) + 66;
	}
	
	sint16 over_trivial = (sint16)GetSkill(spec->tradeskill) - (sint16)spec->trivial;

	//handle caps
	if(admin >= 255) {
		chance = 100;	//cannot fail.
		cout << "...This combine cannot fail.";		
	} else if(over_trivial >= 0) {
		// At reaching trivial the chance goes to 95% going up an additional
		// percent for every 40 skillpoints above the trivial.
		// The success rate is not modified through stats.	
		chance = 95.0f + (float(user_skill - spec->trivial) / 40.0f);
		Message(DARK_BLUE,"You can no longer advance your skill from making this item.");
	} else if(chance < 5) {
		// Minimum chance is always 5
		chance = 5;
	} else if(chance > 95) {
		//cap is 95, shouldent reach this before trivial, but just in case.
		chance = 95;
	}
	
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Current skill: %d , Trivial: %d , Success chance: %f percent\n", user_skill , spec->trivial , chance);
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Bonusstat: %d , INT: %d , WIS: %d , DEX: %d , STR: %d\n", bonusstat , GetINT() , GetWIS() , GetDEX() , GetSTR());
	
	float res = MakeRandomFloat(0, 99);
	
    const Item_Struct* item = NULL;

    if (((spec->tradeskill==75) || (chance > res))){
		success_modifier = 1;
		
		if(over_trivial < 0) {
			CheckIncreaseTradeskill(bonusstat, stat_modifier, skillup_modifier, success_modifier, spec->tradeskill);
		}

		Message(DARK_BLUE,"You have fashioned the items together to create something new!");
	
		itr = spec->onsuccess.begin();
		while(itr != spec->onsuccess.end()) {
			
			SummonItem(itr->first, itr->second);
             item = Database::Instance()->GetItem(itr->first);
			// Harakiri: inform the group in party chat about the item?
			/*if (this->GetGroup())
			{
				entity_list.MessageGroup(this,true,MT_Skills,"%s has successfully fashioned %s!",GetName(),item->Name);
			}*/
			
			itr++;
		}
		return(true);
	} else {
		success_modifier = 2; // Halves the chance
		
		if(over_trivial < 0) {
			CheckIncreaseTradeskill(bonusstat, stat_modifier, skillup_modifier, success_modifier, spec->tradeskill);
		}
		Message(DARK_BLUE,"You lacked the skills to fashion the items together.");

		// Harakiri: inform the group in party chat about the item?
		/*
        if (this->GetGroup())
		{
			entity_list.MessageGroup(this,true,MT_Skills,"%s was unsuccessful in %s tradeskill attempt.",GetName(),this->GetGender() == 0 ? "his" : this->GetGender() == 1 ? "her" : "its");
		}*/
		
		itr = spec->onfail.begin();
		while(itr != spec->onfail.end()) {
			
			SummonItem(itr->first, itr->second);
			itr++;
		}
	}
	return(false);
}

/********************************************************************
 *                      Harakiri - 8/13/2009                        *
 ********************************************************************
 *		       Client::CheckIncreaseTradeskill						*
 ********************************************************************
  Checks if player has reached max skill, then checks skillup chance
  and increases the skill if successful
 ********************************************************************/
void Client::CheckIncreaseTradeskill(sint16 bonusstat, sint16 stat_modifier, float skillup_modifier, uint16 success_modifier, SkillType tradeskill)
{
	uint16 current_raw_skill = GetSkill(tradeskill);

	int maxskill = CheckMaxSkill(tradeskill, this->race, this->class_, this->level);

	if(current_raw_skill >= maxskill)
		return;	//not allowed to go higher.
	
	float chance_stage2 = 0;

	//A successfull combine doubles the stage1 chance for an skillup
	//Some tradeskill are harder than others. See above for more.
	float chance_stage1 = (bonusstat - stat_modifier) / (skillup_modifier * success_modifier);

	//In stage2 the only thing that matters is your current unmodified skill.
	//If you want to customize here you probbably need to implement your own
	//formula instead of tweaking the below one.
	if (chance_stage1 > MakeRandomFloat(0, 99)) {
		if (current_raw_skill < 15) {
			// Allways succeed
			chance_stage2 = 100;
		} else if (current_raw_skill < 175) {
			//From skill 16 to 174 your chance of success falls linearly from 92% to 13%.
			chance_stage2 = (200 - current_raw_skill) / 2;
		} else {
			//At skill 175, your chance of success falls linearly from 12.5% to 2.5% at skill 300.
			chance_stage2 = 12.5 - (.08 * (current_raw_skill - 175));
		}
	}
	   
	if (chance_stage2 > MakeRandomFloat(0, 99)) {
		//Only if stage1 and stage2 succeeded you get a skillup.
		SetSkill(tradeskill, current_raw_skill + 1);
	}

	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"...skillup_modifier: %f , success_modifier: %d , stat modifier: %d\n", skillup_modifier , success_modifier , stat_modifier);
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"...Stage1 chance was: %f percent",  chance_stage1);
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"...Stage2 chance was: %f percent. 0 percent means stage1 failed\n",  chance_stage2);
}

/********************************************************************
 *                      Harakiri - 8/13/2009                        *
 ********************************************************************
 *		       Database::GetTradeRecipe							    *
 ********************************************************************
   The combine in itself does not  differentiate between tradeskills, 
   this is all handled by the db search. In essence, search a recipe 
   which contains all the itemsin the container and has the container
   id. This way we are figuring  out the tradeskill ID itself.
 ********************************************************************/
bool Database::GetTradeRecipe(Combine_Struct* combin, DBTradeskillRecipe_Struct *spec)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	
	uint32 sum = 0;
	uint32 count = 0;
	uint32 qcount = 0;
	uint32 qlen = 0;
	uint16 containerId = 0;
	
 	// make where clause segment for container(s)
 	char containers[30];
	if (combin->containerslot == SLOT_WORLD_CONTAINER) {
 		// world combiner so no item number
		containerId = combin->worldobjecttype;		
 	} else {
 		// container in inventory
		containerId = combin->containerID;
	}

	snprintf(containers,29, "= %u", containerId);
	
	
	//Could prolly watch for stacks in this loop and handle them properly...
	//just increment sum and count accordingly
	bool first = true;	
	
	// stringstream better then ye 'ol buffer
	std::stringstream stringstream;	

	for(int i=0; i < sizeof(combin->iteminslot)/sizeof(int16); i++) {
		if (combin->iteminslot[i] != 0xFFFF) {			
				if(first) {
					stringstream <<  combin->iteminslot[i];					
					first = false;
				} else {
					stringstream <<  "," << combin->iteminslot[i];					
				}

				sum += combin->iteminslot[i];
				count++;			
		}
	}

	if(count < 1) {
		return(false);	//no items == no recipe
	}


	qlen = MakeAnyLenString(&query, "SELECT tre.recipe_id "
	" FROM tradeskill_recipe_entries AS tre"
	" WHERE ( tre.item_id IN(%s) AND tre.componentcount>0 )"
 	"  OR ( tre.item_id %s AND tre.iscontainer=1 )"
 	" GROUP BY tre.recipe_id HAVING sum(tre.componentcount) = %u"
	"  AND sum(tre.item_id * tre.componentcount) = %u", stringstream.str().c_str(), containers, count, sum);

	if (!RunQuery(query, qlen, errbuf, &result)) {
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe search, query: %s",query);
		safe_delete_array(query);	
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe search, error: %s",errbuf);
		return(false);
	}
	
	EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Query : %s",query);
	
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);

	if(qcount < 1)
		return(false);
	
	if(qcount > 1) {
		//multiple recipes, partial match... do an extra query to get it exact.
		//this happens when combining components for a smaller recipe
		//which is completely contained within another recipe
		
		first = true;		
		
		for (int i = 0; i < qcount; i++) {
			row = mysql_fetch_row(result);
			uint32 recipeid = (uint32) atoi(row[0]);
			if(first) {
				stringstream << recipeid;
				first = false;
			} else {
				stringstream << "," << recipeid;
			}			
		}
		
		qlen = MakeAnyLenString(&query, "SELECT tre.recipe_id"
		" FROM tradeskill_recipe_entries AS tre"
 		" WHERE tre.recipe_id IN (%s)"
 		" GROUP BY tre.recipe_id HAVING sum(tre.componentcount) = %u"
 		"  AND sum(tre.item_id * tre.componentcount) = %u", stringstream.str().c_str(), count, sum);
					 

		if (!RunQuery(query, qlen, errbuf, &result)) {
			EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe search, query2: %s",query);
			safe_delete_array(query);	
			EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe search, error2: %s",errbuf);
			return(false);
		}

		EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Query 2 : %s",query);

		safe_delete_array(query);
	
		qcount = mysql_num_rows(result);	
	}
	
	if(qcount < 1)
		return(false);
	
	if(qcount > 1)
	{
		//The recipe is not unique, so we need to compare the container were using.
	

		qlen = MakeAnyLenString(&query,"SELECT tre.recipe_id FROM tradeskill_recipe_entries as tre WHERE tre.recipe_id IN (%s)"
		" AND tre.item_id %s AND tre.iscontainer=1",stringstream.str(),containers);
		
	 
		if (!RunQuery(query, qlen, errbuf, &result)) {
			EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe search, query3: %s",query);
			safe_delete_array(query);	
			EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe search, error3: %s",errbuf);
			return(false);
		}

	   EQC::Common::Log(EQCLog::Debug,CP_CLIENT,"Query 3 : %s",query);

		safe_delete_array(query);

		
		uint32 resultRowTotal = mysql_num_rows(result); 
		
		//If all the recipes were correct, but no container type matched, drop out.
		if(resultRowTotal == 0 || resultRowTotal > 1){			
			EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Combine error: Recipe is not unique OR incorrect container is being used!");
			return(false);
		}
	
	}
	
	row = mysql_fetch_row(result);
	uint32 recipe_id = (uint32)atoi(row[0]);
	mysql_free_result(result);

	//Right here we verify that we actually have ALL of the tradeskill components..
	//instead of part which is possible with experimentation.
	//This is here because something's up with the query above.. it needs to be rethought out
	bool has_components = true;
	char TSerrbuf[MYSQL_ERRMSG_SIZE];
    char *TSquery = 0;
    MYSQL_RES *TSresult;
    MYSQL_ROW TSrow;
	if (RunQuery(TSquery, MakeAnyLenString(&TSquery, "SELECT item_id, componentcount from tradeskill_recipe_entries where recipe_id=%i AND componentcount > 0", recipe_id), TSerrbuf, &TSresult)) {
		while((TSrow = mysql_fetch_row(TSresult))!=NULL) {
			int ccnt = 0;			
			for(int i=0; i < sizeof(combin->iteminslot)/sizeof(int16); i++) {
				if (combin->iteminslot[i] != 0xFFFF && combin->iteminslot[i] == atoi(TSrow[0])) {										
							ccnt++;											
				}
			}

			if(ccnt != atoi(TSrow[1])) {
				has_components = false;
			}
		}
		mysql_free_result(TSresult);
	} else {		
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in tradeskill verify query:  %s %s",TSquery,TSerrbuf);
	
	}
	safe_delete_array(TSquery);

	if(has_components == false){
		
		return false;
	}

	return(GetTradeRecipe(recipe_id, containerId, spec));
}

/********************************************************************
 *                      Harakiri - 8/13/2009                        *
 ********************************************************************
 *		       Database::GetTradeRecipe							    *
 ********************************************************************
   This will retrieve the fail and success items for a combine.
 ********************************************************************/

	bool Database::GetTradeRecipe(uint32 recipe_id, uint16 containerID, DBTradeskillRecipe_Struct *spec)
{	
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	
	uint32 qcount = 0;
	uint32 qlen;

	char containers[30];
	snprintf(containers,29, "= %u", containerID); 	
 	
 	qlen = MakeAnyLenString(&query, "SELECT tr.id, tr.tradeskill, tr.skillneeded,"
 	" tr.trivial " /*, tr.nofail, tr.replace_container"*/
 	" FROM tradeskill_recipe AS tr inner join tradeskill_recipe_entries as tre"
 	" ON tr.id = tre.recipe_id"
 	" WHERE tr.id = %lu AND tre.item_id %s"
 	" GROUP BY tr.id", recipe_id, containers);
		
	if (!RunQuery(query, qlen, errbuf, &result)) {
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe, query: %s",query);
		safe_delete_array(query);	
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe, error: %s",errbuf);
		return(false);
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);

	if(qcount != 1) {
		//just not found i guess..
		return(false);
	}
	
	row = mysql_fetch_row(result);
 	spec->tradeskill			= (SkillType)atoi(row[1]);
 	spec->skill_needed		= (sint16)atoi(row[2]);
 	spec->trivial			= (uint16)atoi(row[3]);
 	/*spec->nofail			= atoi(row[4]) ? true : false;
 	spec->replace_container	= atoi(row[5]) ? true : false;*/
	mysql_free_result(result);
	
	//Pull the on-success items...
	qlen = MakeAnyLenString(&query, "SELECT item_id,successcount FROM tradeskill_recipe_entries"
	 " WHERE successcount>0 AND recipe_id=%u", recipe_id);
	 
	if (!RunQuery(query, qlen, errbuf, &result)) {
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe, success query: %s",query);
		safe_delete_array(query);	
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecipe, success error: %s",errbuf);
		return(false);
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount < 1) {		
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in GetTradeRecept success: no success items returned");
		return(false);
	}
	uint8 r;
	spec->onsuccess.clear();
	for(r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);		
		uint32 item = (uint32)atoi(row[0]);
		uint8 num = (uint8) atoi(row[1]);
		spec->onsuccess.push_back(pair<uint32,uint8>::pair(item, num));
	}
	mysql_free_result(result);
	
	
	//Pull the on-fail items...
	qlen = MakeAnyLenString(&query, "SELECT item_id,failcount FROM tradeskill_recipe_entries"
	 " WHERE failcount>0 AND recipe_id=%u", recipe_id);

	spec->onfail.clear();
	if (RunQuery(query, qlen, errbuf, &result)) {
		
		qcount = mysql_num_rows(result);
		uint8 r;
		for(r = 0; r < qcount; r++) {
			row = mysql_fetch_row(result);
		
			uint32 item = (uint32)atoi(row[0]);
			uint8 num = (uint8) atoi(row[1]);
			spec->onfail.push_back(pair<uint32,uint8>::pair(item, num));
		}
		mysql_free_result(result);
	}
	safe_delete_array(query);
	
	return(true);
}

 /********************************************************************
 *                      Harakiri - 8/13/2009                        *
 ********************************************************************
 *		       Database::SearchTradeRecipe						    *
 ********************************************************************
   Search a recipe by name
 ********************************************************************/

	bool Database::SearchTradeRecipe(const char* recipeName, int recipeID,  std::vector<DBTradeskillRecipe_Struct*>* specList)
{	
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	
	uint32 qcount = 0;
	uint32 qlen;

	if(recipeID==0) {
		qlen = MakeAnyLenString(&query, "SELECT tr.name, tr.id , tr.tradeskill, tr.skillneeded, tr.trivial FROM tradeskill_recipe AS tr where tr.name LIKE \"%%%s%%\" ORDER BY tr.tradeskill LIMIT 20", recipeName);
	} else {
		qlen = MakeAnyLenString(&query, "SELECT tr.name, tr.id , tr.tradeskill, tr.skillneeded, tr.trivial FROM tradeskill_recipe AS tr where tr.id = %i ORDER BY tr.tradeskill LIMIT 20", recipeID);

	}

 	
		
	if (!RunQuery(query, qlen, errbuf, &result)) {
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in SearchTradeRecipe, success query: %s",query);
		safe_delete_array(query);	
		EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in SearchTradeRecipe, success error: %s",errbuf);
		return(false);
	}

		
	qcount = mysql_num_rows(result);

	cout << qcount << query;

	safe_delete_array(query);
	

	if(qcount == 0) {
		//just not found i guess..
		return(false);
	}
	

	for(int r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);

		DBTradeskillRecipe_Struct* spec = new DBTradeskillRecipe_Struct;	
		memset(spec, 0, sizeof(DBTradeskillRecipe_Struct));
	
		strcpy(spec->name, row[0]);
		spec->recipeID = atoi(row[1]);		
		spec->tradeskill = (SkillType)atoi(row[2]);
		spec->skill_needed = atoi(row[3]);
		spec->trivial = atoi(row[4]);
		

		 MYSQL_RES *result2;
		 MYSQL_ROW row2;

		qlen = MakeAnyLenString(&query, "SELECT item_id,componentcount FROM tradeskill_recipe_entries"
			" WHERE componentcount >0 AND recipe_id=%u", spec->recipeID);
		 
		if (!RunQuery(query, qlen, errbuf, &result2)) {
			EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in SearchTradeRecipe, item query: %s",query);
			safe_delete_array(query);	
			EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in SearchTradeRecipe, item error: %s",errbuf);
			return(false);
		}

		safe_delete_array(query);
	
		qlen = mysql_num_rows(result2);
		if(qlen < 1) {			
			EQC::Common::Log(EQCLog::Error,CP_CLIENT,"Error in SearchTradeRecipe success: no success items returned recipe_id = %i",spec->recipeID);
			mysql_free_result(result2);		
			continue;
		}
		uint8 x;
		spec->oncombine.clear();
		for(x = 0; x < qlen; x++) {
			row2 = mysql_fetch_row(result2);		
			uint32 item = (uint32)atoi(row2[0]);
			uint8 num = (uint8) atoi(row2[1]);
			spec->oncombine.push_back(pair<uint32,uint8>::pair(item, num));
		}
		
		mysql_free_result(result2);		

		specList->push_back(spec);
	}

	mysql_free_result(result);
	
	return(true);
}