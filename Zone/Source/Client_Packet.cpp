#include <cstdio>
#include <cstring>
#include <string>
#include <iomanip>
#include "client.h"
#include "classes.h"
#include "database.h"
#include "EQCUtils.hpp"
#include "eq_opcodes.h"
#include "GuildNetwork.h"
#include "petitions.h"
#include "PlayerCorpse.h"
#include "skills.h"
#include "worldserver.h"
#include "zone.h"
#include "config.h"
#include "groups.h"
#include "packet_dump_file.h"
#include "ZoneGuildManager.h"

using namespace std;
using namespace EQC::Zone;

extern PetitionList petition_list;
extern WorldServer worldserver;
extern Zone* zone;
extern ZoneGuildManager zgm;


/** Replaces the original ProcessOpcode without the switch statement. */
void Client::ProcessOpcode(APPLAYER* app)
{	
	if(app->opcode != OP_ClientUpdate) {		
		//EQC::Common::Log(EQCLog::Debug, CP_CLIENT, "Received Opcode: %X",app->opcode);
		//DumpPacketHex(app);
	}

	(this->*process_opcode_array[app->opcode])(app);
}


/** Initializes the function array that processes opcodes by the opcode index number.*/
void Client::InitProcessArray()
{
	for(int i=0;i<0xFFFF;i++){
		process_opcode_array[i] = &Client::ProcessOP_Default;
	}
	
	process_opcode_array[OP_AutoAttack]		= &Client::ProcessOP_AutoAttack;
	process_opcode_array[OP_FeignDeath]		= &Client::ProcessOP_FeignDeath;
	process_opcode_array[OP_DuelResponse]	= &Client::ProcessOP_DuelResponse;
	process_opcode_array[OP_DuelResponse2]	= &Client::ProcessOP_DuelResponse2;
	process_opcode_array[OP_RequestDuel]	= &Client::ProcessOP_RequestDuel;
	process_opcode_array[OP_AutoAttack]		= &Client::ProcessOP_AutoAttack;
	process_opcode_array[OP_AutoAttack]		= &Client::ProcessOP_AutoAttack;
	process_opcode_array[OP_ClientUpdate]	= &Client::ProcessOP_ClientUpdate;
	process_opcode_array[OP_ClientTarget]	= &Client::ProcessOP_ClientTarget;
	process_opcode_array[OP_Jump]			= &Client::Process_Jump;
	process_opcode_array[OP_AssistTarget]	= &Client::ProcessOP_AssistTarget;
	process_opcode_array[OP_Consider]		= &Client::ProcessOP_Consider;
	process_opcode_array[OP_Surname]		= &Client::ProcessOP_Surname;
	process_opcode_array[OP_YellForHelp]	= &Client::ProcessOP_YellForHelp;
	process_opcode_array[OP_SpawnAppearance]= &Client::ProcessOP_SpawnAppearance;
	process_opcode_array[OP_Death]			= &Client::ProcessOP_Death;
	process_opcode_array[OP_MoveCoin]		= &Client::ProcessOP_MoveCoin;
	process_opcode_array[OP_MoveItem]		= &Client::ProcessOP_MoveItem;
	process_opcode_array[OP_ConsumeItem]	= &Client::ProcessOP_ConsumeItem;
	process_opcode_array[OP_Forage]			= &Client::ProcessOP_Forage;
	process_opcode_array[OP_Beg]			= &Client::ProcessOP_Beg;
	process_opcode_array[OP_Sneak]			= &Client::ProcessOP_Sneak;
	process_opcode_array[OP_Hide]			= &Client::ProcessOP_Hide;
	process_opcode_array[OP_SenseTraps]		= &Client::Process_SenseTraps;
	process_opcode_array[OP_DisarmTraps]	= &Client::Process_DisarmTraps;
	process_opcode_array[OP_InstillDoubt]	= &Client::Process_InstillDoubt;
	process_opcode_array[OP_PickPockets]	= &Client::ProcessOP_PickPockets;
	process_opcode_array[OP_SafeFallSuccess]= &Client::ProcessOP_SafeFallSuccess;
	process_opcode_array[OP_Track]			= &Client::Process_Track;
	process_opcode_array[OP_ChannelMessage]	= &Client::ProcessOP_ChannelMessage;
	process_opcode_array[OP_WearChange]		= &Client::ProcessOP_WearChange;
	process_opcode_array[OP_Action]			= &Client::Process_Action;
	process_opcode_array[OP_ConsumeFoodDrink]		= &Client::Process_ConsumeFoodDrink;
	process_opcode_array[OP_ZoneChange]		= &Client::ProcessOP_ZoneChange;
	process_opcode_array[OP_Camp]			= &Client::ProcessOP_Camp;
	process_opcode_array[OP_DeleteSpawn]	= &Client::ProcessOP_DeleteSpawn;
	process_opcode_array[OP_Mend]			= &Client::ProcessOP_Mend;
	process_opcode_array[OP_Save]			= &Client::ProcessOP_Save;
	process_opcode_array[OP_Taunt]			= &Client::ProcessOP_Taunt;
	//process_opcode_array[OP_UseAbility]		= &Client::ProcessOP_UseAbility;
	process_opcode_array[OP_GMSummon]		= &Client::ProcessOP_GMSummon;
	process_opcode_array[OP_GiveItem]		= &Client::ProcessOP_GiveItem;
	process_opcode_array[OP_TradeAccepted]	= &Client::ProcessOP_TradeAccepted;
	process_opcode_array[OP_DropItem]		= &Client::ProcessOP_DropItem;
	process_opcode_array[OP_DropCoin]		= &Client::ProcessOP_DropCoin;
	process_opcode_array[OP_PickupItem]		= &Client::ProcessOP_PickupItem;
	process_opcode_array[OP_CancelTrade]	= &Client::ProcessOP_CancelTrade;
	process_opcode_array[OP_Click_Give]		= &Client::ProcessOP_Click_Give;
	process_opcode_array[OP_SplitMoney]		= &Client::ProcessOP_SplitMoney;
	process_opcode_array[OP_Random]			= &Client::ProcessOP_Random;
	process_opcode_array[OP_Buff]			= &Client::ProcessOP_Buff;
	process_opcode_array[OP_GMHideMe]		= &Client::ProcessOP_GMHideMe;
	process_opcode_array[OP_GMNameChange]	= &Client::ProcessOP_GMNameChange;
	process_opcode_array[OP_GMKill]			= &Client::ProcessOP_GMKill;
	process_opcode_array[OP_GMSurname]		= &Client::ProcessOP_GMSurname;
	process_opcode_array[OP_GMToggle]		= &Client::ProcessOP_GMToggle;
	process_opcode_array[OP_LFG]			= &Client::ProcessOP_LFG;
	process_opcode_array[OP_WhoAll]			= &Client::ProcessOP_WhoAll;
	process_opcode_array[OP_GMZoneRequest]	= &Client::ProcessOP_GMZoneRequest;
	process_opcode_array[OP_EndLootRequest]	= &Client::ProcessOP_EndLootRequest;
	process_opcode_array[OP_LootRequest]	= &Client::ProcessOP_LootRequest;
	process_opcode_array[OP_LootItem]		= &Client::ProcessOP_LootItem;
	process_opcode_array[OP_MemorizeSpell]	= &Client::ProcessOP_MemorizeSpell;
	process_opcode_array[OP_SwapSpell]		= &Client::ProcessOP_SwapSpell;
	process_opcode_array[OP_CastSpell]		= &Client::ProcessOP_CastSpell;
	process_opcode_array[OP_ManaChange]		= &Client::ProcessOP_ManaChange;
	process_opcode_array[OP_InterruptCast]	= &Client::ProcessOP_InterruptCast;
	process_opcode_array[OP_ShopRequest]	= &Client::ProcessOP_ShopRequest;
	process_opcode_array[OP_ShopPlayerBuy]	= &Client::ProcessOP_ShopPlayerBuy;
	process_opcode_array[OP_ShopPlayerSell]	= &Client::ProcessOP_ShopPlayerSell;
	process_opcode_array[OP_ShopEnd]		= &Client::Process_ShoppingEnd;
	process_opcode_array[OP_ClassTraining]	= &Client::ProcessOP_ClassTraining;
	process_opcode_array[OP_ClassEndTraining]	= &Client::ProcessOP_ClassEndTraining;
	process_opcode_array[OP_ClassTrainSkill]	= &Client::ProcessOP_ClassTrainSkill;
	process_opcode_array[OP_GroupInvite]		= &Client::ProcessOP_GroupInvite;
	process_opcode_array[OP_BackSlashTarget]	= &Client::ProcessOP_BackSlashTarget;
	process_opcode_array[OP_GroupFollow]		= &Client::ProcessOP_GroupFollow;
	process_opcode_array[OP_GroupDeclineInvite]	= &Client::ProcessOP_GroupDeclineInvite;
	process_opcode_array[OP_GroupQuit]			= &Client::ProcessOP_GroupQuit;
	process_opcode_array[OP_GMEmoteZone]		= &Client::ProcessOP_GMEmoteZone;
	process_opcode_array[OP_InspectRequest]		= &Client::ProcessOP_InspectRequest;
	process_opcode_array[OP_InspectAnswer]		= &Client::ProcessOP_InspectAnswer;
	process_opcode_array[OP_Petition]			= &Client::ProcessOP_Petition;
	process_opcode_array[OP_PetitionCheckIn]	= &Client::ProcessOP_PetitionCheckIn;
	process_opcode_array[OP_PetitionDelete]		= &Client::ProcessOP_PetitionDelete;
	process_opcode_array[OP_PetitionCheckout]	= &Client::ProcessOP_PetitionCheckout;
	process_opcode_array[OP_BugReport]			= &Client::ProcessOP_BugReport;
	process_opcode_array[OP_TradeSkillCombine]	= &Client::ProcessOP_TradeSkillCombine;
	process_opcode_array[OP_ReadBook]			= &Client::ProcessOP_ReadBook;
	process_opcode_array[OP_Social_Text]		= &Client::ProcessOP_Social_Text;
	process_opcode_array[OP_GMDelCorpse]	= &Client::ProcessOP_GMDelCorpse;
	process_opcode_array[OP_GMKick]			= &Client::ProcessOP_GMKick;
	process_opcode_array[OP_GMServers]		= &Client::ProcessOP_GMServers;
	process_opcode_array[OP_Fishing]		= &Client::ProcessOP_Fishing;
	process_opcode_array[OP_ClickDoor]		= &Client::ProcessOP_ClickDoor;
	process_opcode_array[OP_GetOnBoat]		= &Client::ProcessOP_GetOnBoat;
	process_opcode_array[OP_GetOffBoat]		= &Client::ProcessOP_GetOffBoat;
	process_opcode_array[OP_CommandBoat]	= &Client::ProcessOP_CommandBoat;
	process_opcode_array[OP_BindWound]		= &Client::ProcessOP_BindWound;
	process_opcode_array[OP_MBRetrieveMessages]	= &Client::ProcessOP_MBRetrieveMessages;
	process_opcode_array[OP_MBPostMessage]	= &Client::ProcessOP_MBPostMessage;
	process_opcode_array[OP_MBEraseMessage]	= &Client::ProcessOP_MBEraseMessage;
	process_opcode_array[OP_MBRetrieveMessage]	= &Client::ProcessOP_MBRetrieveMessage;	
	process_opcode_array[OP_UseDiscipline]	= &Client::ProcessOP_UseDiscipline;
	process_opcode_array[OP_Translocate]	= &Client::ProcessOP_TranslocateResponse;	
	process_opcode_array[OP_ClientError]	= &Client::ProcessOP_ClientError;
	process_opcode_array[OP_ApplyPoison]	= &Client::ProcessOP_ApplyPoison;
	process_opcode_array[OP_PlayerDeath]	= &Client::ProcessOP_PlayerDeath;
	process_opcode_array[OP_PlayerSave]		= &Client::ProcessOP_PlayerSave;
	process_opcode_array[OP_PlayerSave2]	= &Client::ProcessOP_PlayerSave;
	process_opcode_array[OP_ItemMissing]	= &Client::ProcessOP_ItemMissing;	
}
