// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
//
// ***************************************************************
#ifndef EQ_OPCODES_H
#define EQ_OPCODES_H

#define OP_SERVERMOTD		0xdd21		// Comment: froglok23 : Server MOTD opcode.

#define OP_PetitionClientUpdate		0x0f20	// Comment: 
/*============ Merchants ============*/
#define OP_ShopRequest		0x0b20		// Comment: right-click on merchant
#define OP_ShopItem			0x0c20		// Comment: 
#define OP_ShopPlayerBuy	0x2720		// Comment: Cofruben: correct opcode.
#define OP_ShopPlayerSell	0x2820		// Comment: Cofruben: correct opcode.
#define OP_ShopDelItem		0x3820		// Comment: Tazadar: correct opcode.
#define OP_ShopEnd			0x3720		// Comment:
#define OP_ShopEndConfirm	0x4521		// Comment: 

/*============== Items ==============*/
#define OP_ItemTradeIn		0x3120		// Comment: 
#define OP_LootComplete		0x4421		// Comment: 
#define OP_EndLootRequest	0x4F20		// Comment: 
#define OP_MoneyOnCorpse	0x5020		// Comment: 
#define OP_SplitMoney		0x3121      // Comment:
#define OP_ItemOnCorpse		0x5220		// Comment: 
#define OP_LootItem			0xA020		// Comment: 
#define OP_LootRequest		0x4e20		// Comment: 
#define OP_DropCoin			0x0720		// Comment: Player attempts to drop coin on ground.  (Size: 112)
#define OP_PickupCoin		0x0820		// Comment: Yeahlight: Not testing, but this may be correct
#define OP_DropItem			0x3520		// Comment: Player attempts to drop item on ground. 
#define OP_PickupItem		0x3620		// Comment: Player clicks to attempt to pick up an object on ground.

/*============== Doors ==============*/
#define	OP_SpawnDoor		0x9520		// Comment: 
#define	OP_UpdateDoor		0x9820		// Comment: 
#define	OP_ClickDoor		0x8d20		// Comment: 
#define	OP_OpenDoor			0x8e20		// Comment: 
#define OP_DespawnDoor		0x9b20		// Comment: Yeahlight: Despawns a door

/*============== Message Board ==============*/
#define OP_MBRetrieveMessages			0x0821		// Comment: Player clicks on a Message Board or on a category GENERAL/Sales etc (Harakiri)
#define OP_MBRetrieveMessage			0x0921		// Comment: Player right clicks on a specific message (Harakiri)
#define OP_MBSendMessageToClient		0x0a21		// Comment: Send 1 Message each time to Client for Message Board Message List (Harakiri)
#define OP_MBMessageDetail				0x0b21		// Comment: Server signals that message detail was sent, now open detail popup (Harakiri)
#define OP_MBPostMessage				0x0c21		// Comment: Client sents a post request for a messsage (Harakiri)
#define OP_MBEraseMessage				0x0d21		// Comment: Client sents a delete request for a messsage (Harakiri)
#define OP_MBDataSent_OpenMessageBoard	0x0e21		// Comment: Signal the client that all available messages have been sent or a erase/post request was successful, show now the message board


/*============= Trading =============*/
#define OP_Click_Give		0xda20		// Comment: 
#define OP_CloseTrade		0xdc20		// Comment: 
#define OP_CancelTrade		0xdb20		// Comment: 
#define OP_TradeAccepted    0xE620
#define OP_ItemToTrade		0xdf20      // Comment:	Send items on the other client windows while trading
#define OP_TradeCoins		0xe420      // Comment: Client update the amount of money in trade window (Confirmed By Tazadar)

/*============= Groups ============= */
#define OP_GroupExp			    0x9a20  // Comment: Tazadar :This is the group message exp opcode (empty packet it seems), Confirmed: Harakiri
#define OP_GroupInvite			0x4020	//
#define OP_GroupFollow			0x4220	// Sent after someone clicks 'Follow' after receiving group invite;
#define OP_GroupDeclineInvite	0x4120	// Sent when member declines group invite by clicking disband;	
#define OP_GroupQuit			0x4420	// Sent when member chooses to disband OR member is kicked from leader;
#define OP_GroupUpdate			0x2640	// Used to notify group members of group events;
#define OP_GroupDelete			0x9721	// When leader clicks disband on himself;

/*========== Guild Commands =========*/
#define OP_GuildInviteAccept	0x1821  // Comment:
#define OP_GuildLeader			0x9521	// Comment: /guildleader command
#define OP_GuildPeace			0x9121	// Comment: /guildpeace command
#define OP_GuildWar				0x6f21	// Comment: /guildwar command
#define OP_GuildInvite			0x1721	// Comment: /guildinvite command
#define OP_GuildRemove			0x1921	// Comment: /guildremove command
#define OP_GuildMOTD			0x0322	// Comment: /guildmotd command
#define OP_GuildUpdate			0x7b21	// Comment: 
#define OP_GuildsList			0x9221	// Comment: 

/*========== General Skills =========*/
#define OP_Beg			    0x2521		// Comment: Player attempts to start begging.
#define OP_Forage			0x9420		// Comment: 
#define OP_Fishing			0x8f21		// Comment: 
#define OP_BindWound		0x9320		// Comment: Sent when a player attempts to bind wound.

/*=========== Class Skills ==========*/
#define OP_Sneak			0x8521		// Comment: 
#define OP_Hide				0x8621		// Comment: 
#define OP_SenseTraps		0x8821		// Comment: 
#define OP_Mend				0x9d21		// Comment: 
#define OP_HarmTouch		0x7E21		// Comment:
#define OP_Taunt			0x3b21		// Comment: 
#define OP_FeignDeath		0xac20		// Comment: Harakiri: If you sent this to the client with 1, it will print Phineas_Taylor00 has been slain by Phineas_Taylor00! if Phineas is entity id 1 in your zone
#define OP_InstillDoubt		0x9c21		// Comment:
#define OP_PickPockets		0xad20		// Comment: Player click pickpocket on a PC.
#define OP_DisarmTraps		0xf321		// Comment: Player clicks disarm traps in the abilities tab.
#define OP_SafeFallSuccess	0xab21		// Comment: Player falls and absorbs some damage.
#define OP_Track			0x8421		// Comment: Player clicks track in the abilities tab.
#define OP_ApplyPoison		0xba21		// Comment: Harakiri client sents this when you right click on a poison

/*============== Boats ==============*/
#define	OP_GetOnBoat		0xbb21		// Comment: Player gets on a boat.
#define OP_GetOffBoat		0xbc21		// Comment: Player leaves a boat.
#define OP_CommandBoat		0x2621		// Comment: Player on a small boat clicks on the boat to control it. Yeahlight: This may also be used to control an NPC

/*============= General =============*/
#define	OP_ReadBook			0xce20		// Comment: 
#define OP_MultiLineMsg		0x1420		// Comment: 
#define OP_CPlayerItems		0xf621		// Comment: 
#define OP_CPlayerItem		0x6421		// Comment: 
#define OP_Illusion			0x9120		// Comment:
#define OP_LFG				0xf021		// Comment: 
#define OP_Translocate		0x0622		// Comment: 
#define OP_Jump				0x2020		// Comment: 
#define OP_MovementUpdate	0x1F20		// Comment: 
#define OP_SenseHeading		0x8721		// Comment: 
#define OP_SafePoint		0x2420		// Comment: 
#define OP_Surname			0xc421		// Comment: 
#define OP_YellForHelp		0xda21		// Comment: 
#define OP_ZoneServerInfo	0x0480		// Comment: 
#define OP_ChannelMessage	0x0721		// Comment: 
#define OP_Camp				0x0722		// Comment: 
#define OP_ZoneEntry		0x2a20		// Comment: 
#define OP_DeleteSpawn		0x2b20		// Comment: 
#define OP_MoveItem			0x2c21		// Comment: 
#define OP_MoveCoin			0x2d21		// Comment: 
#define OP_TradeMoneyUpdate	0x3d21      // Comment: Update the amount of money of the client after a trade :)
#define OP_PlayerProfile	0x2d20		// Comment: 
#define OP_Save				0x2e20		// Comment: 
#define OP_Consider			0x3721		// Comment: 
#define OP_SendCharInfo		0x4720		// Comment: 
#define OP_NewSpawn			0x4921		// Comment: 
#define OP_ZoneSpawns		0x6121		// Comment: 
#define OP_Death			0x4a20		// Comment: 
#define OP_AutoAttack		0x5121		// Comment: 
#define OP_InitiateConsume  0x9021		// Comment: Harakiri - can be sent to the client to initiate an auto consume of a specific itemID in a specific Slot
#define OP_NAMEAPPROVAL		0x8B20		// Comment: 
#define OP_ENTERWORLD		0x0180		// Comment: 
#define OP_ENVDAMAGE2		0x1e20		// Comment: 
#define OP_Stamina			0x5721		// Comment: 
#define OP_AutoAttack2		0x6021		// Comment: 
#define OP_SendLoginInfo	0x5818		// Comment: 
#define OP_Action			0x5820		// Comment: 
#define OP_DeleteCharacter	0x5a20		// Comment: 
#define OP_NewZone			0x5b20		// Comment: 
#define OP_CombatAbility	0x5f21		// Comment: 
#define OP_ClientTarget		0x6221		// Comment: 
#define OP_SummonedItem		0x7821		// Comment: 
#define OP_SpecialMesg		0x8021		// Comment: 
#define OP_WearChange		0x9220		// Comment: 
#define SUB_ChangeChar		32767		// Comment: 
//#define SUB_RequestSlot		24397		// Comment: 
#define OP_LevelUpdate		0x9821		// Comment: 
#define OP_SplitResponse	0x9a21      // Comment: Tazadar : send the money to player after autosplit (noise + message included) 
#define OP_CleanStation		0x0522      // Comment: Tazadar : remove items from station !! (i found you muhaha!!!!)
#define OP_ExpUpdate		0x9921		// Comment: 
#define OP_SkillUpdate		0x8921		// Comment: 
#define OP_Attack			0x9f20		// Comment: 
#define OP_MobUpdate		0xa120		// Comment: 
#define OP_ZoneChange		0xa320		// Comment: 
#define	OP_CraftingStation	0xd720		// Comment:
#define OP_HPUpdate			0xb220		// Comment: 
#define OP_TimeOfDay		0xf220		// Comment: 
#define OP_ClientUpdate		0xf320		// Comment: 
#define OP_WhoAll			0xf420		// Comment: 
#define OP_SpawnAppearance	0xf520		// Comment: 
#define OP_BeginCast		0xa920		// Comment: 
#define OP_CastSpell		0x7e21		// Comment: 
#define OP_Buff				0x3221		// Comment: 
#define OP_CastOn			0x4620		// Comment: 
#define OP_InterruptCast	0xd321		// Comment: 	 
#define OP_ManaChange		0x7f21		// Comment: 
#define OP_MemorizeSpell	0x8221 		// Comment: 
#define OP_SwapSpell		0xce21 		// Comment: 
#define OP_CombatAbility	0x5f21		// Comment: 
//#define OP_UseAbility		0x9f20		// Comment: 
#define OP_GiveItem			0xd120		// Comment: 
#define OP_StationItem		0xfb20		// Comment: 
#define OP_ExpansionInfo	0xd821		// Comment: 
#define OP_Random			0xe721		// Comment: 
#define OP_Weather			0x3621		// Comment: 
#define OP_Stun				0x5b21		// Comment: 
#define OP_WebUpdate		0x3c22		// Comment: 
#define OP_CharacterCreate	0x4920		// Comment: 
#define OP_ZoneUnavail		0x0580		// Comment: 
#define OP_InspectRequest	0xb520		// Comment: 
#define OP_InspectAnswer	0xb620		// Comment: 
#define OP_ConsumeItem		0x4621		// Comment: Received when client right clicks alcohol in inv. or when arrows etc are consumed
#define OP_ConsumeFoodDrink	0x5621		// Comment: Used when food or drink is consumed (auto and right clicked)
#define OP_ConsentRequest	0xb720		// Comment: Received when client types /consent
#define OP_ConsentResponse	0xb721		// Comment: 
#define	OP_RequestDuel		0xcf20		// Comment:	
#define OP_DuelResponse		0xd020		// Comment: 
#define OP_DuelResponse2	0x5d21		// Comment: 
#define OP_RezzRequest		0x2a21		// Comment: Player casts a rez spell on a corpse.
#define	OP_Sacrifice		0xea21		// Comment: Sent to a player to accept a sacrifice (Confirmed by Tazadar)
#define OP_RezzAnswer		0x9b21		// Comment:
#define OP_RezzComplete		0xec21		// Comment: Harakiri, with all 1 this will print blue text, "Resurrect failed, unable to find corpse."
#define OP_Medding			0x5821		// Comment: Player opens or closes a spell book.
#define OP_SenseHeading		0x8721		// Comment: Player clicks on sense heading in the abilities tab.
#define OP_BackSlashTarget	0xfe21		// Comment: Player issues /target command.
#define OP_AssistTarget		0x0022		// Comment: Player issues /assist command.
#define OP_QuestCompletedMoney 0x8020	// Comment: Yeahlight: Music played when a quest is completed; Harakiri: Its actually the quest completed opcode, if you fill the struct with 1 you get money from NPC with ID 1
#define OP_SpawnProjectile	0x4520		// Comment: Yeahlight: Spawns a projectile
#define OP_ZoneUnavailable 0xa220		// Comment: Harakiri red message "That zone is currently down, moving you to safe point within your current zone."
#define OP_NPCIsBusy		0xd620		// Comment: Harakiri prints message Phineas_Taylor00 tells you, 'I'm busy right now.' if Phin is entity ID 1 in your zone
#define OP_SenseTrap		0xf421		// Comment: Harakiri player will turn in the direction where a trap is located
#define OP_CorpseDragPermission		0x1421  // Comment: Harakiri yellow message X has permission to drag corpse
#define OP_PartyChannelMessage		0x1720	// Comment: Harakiri Needs Entity ID from sender at one of the first 20 locations, will issue "X tells the party"
#define OP_GuildChannelMessage		0x3321	// Comment: Harakiri green message told the guild <garbled> - probably needs entity ID and string msg
#define OP_DoorSays					0x1621	// Comment: Harakiri #yellow message The door says, "...
#define OP_Ressurection_Failed		0x2220	// Comment: Harakiri  blue message "You were unable to restore the corpse to life, but you may have success with a later attempt." rez failed?
#define OP_Sell_Message				0x27F3	// Comment: Harakiri red message "Error.  Tried to sell possessions that were NULL."
#define OP_GuildCreateResponse		0x2b21	// Comment: Harakiri with 1 message "The new player guild was created." , with 0 message "Failed to create new player guild."
#define OP_ItemMissing				0x2F21  // Comment: Harakiri blue message You are missing, sent by the client when he casts a spell and doesnt have the components "You are missing some required spell components."
#define OP_MoneyTrade				0x3F20  // Comment: Harakiri green message "You receive <randomvalue> Platinum from ." player gets the money in inventory
#define OP_MessageUnstuned			0x5bd1	// Comment: Harakiri message You are unstunned. needs value of 1
#define OP_CorpseInZone				0xa721  // Comment: Harakiri Yellow Text Corpse:  in zone: , response to /corpse?
#define OP_UseDiscipline			0xe621  // Comment: Harakiri client will sent this after issuing /discpline - in pp the bit discplineAvailable needs to be set to 1
#define OP_ClientError				0x4721	// Comment: Harakiri client sents this when an error client side happend i.e. a stackable item without charges sent to the client, or an invalid item (got a bogus item) etc
#define OP_TeleportPC				0x4D21  // Comment: Harakiri THIS is the generic opcode to teleport a PC, just sent TeleportPC_Struct, 
											// the client will automatically decide if the player needs to be zoned to another zone or just moved to the coord if the player is already in the zone he should be teleported to
#define OP_PlayerDeath				0x2921  // Comment: Harakiri sent by the client when the player is death right before sending zone change struct at offset 0048048E, afterwards clears all the vars in the playerProfile like memorized spells and money
#define OP_PlayerSave				0x5421  // Comment: Harakiri sent before zoning there is a flag either this or OP_PlayerSave2 is sent
#define OP_PlayerSave2				0x5521  // Comment: Harakiri sent before zoning there is a flag either this or OP_PlayerSave2 is sent
/*======== TESTING / Unconfirmed =======*/
#define	OP_CreateObject		0x2d20
#define	OP_ClickObject		0x2c20
#define OP_TradeRequest		0xd120
#define OP_Summon			0x3d20		// Comment: Tazadar :Need more work on it , but it says 's hands are full !
#define OP_PKAcknowledge	0x8c21		// Comment: Tazadar :It sends a message :You are now a PK !!
#define OP_GMQuestResponse	0xc021      // Comment: Tazadar :It sends a message :GM quest succesful!
#define	OP_SkillChangeResp	0xbe21		// Comment: Tazadar :It sends a message :Skill change OK!
#define OP_AbilChangeResp	0xbf21		// Comment: Tazadar :It sends a message :Ability change OK!
#define OP_XPGiveResponse	0xc121	    // Comment: Tazadar :It sends a message :XP give OK!
#define OP_XPSetResponse	0xc221	    // Comment: Tazadar :It sends a message :XP set OK!
#define OP_ForceLogOut		0xd920		// Comment: Tazadar :It seems to forces the logout Confirmed: Harakiri
#define OP_LoseControl		0xb020      // Comment: Tazadar :It tells you that you have the control of yourself again
#define OP_ConsentPlayer	0xd520		// Comment: Tazadar :It tells the other client he has permission to drag soandso's corpse
#define OP_PlayerKiller		0xcf21		// Comment: Tazadar :It asks you if you are sure to become a pk or not :)
#define OP_PoisonComplete   0xba21      // Comment: Tazadar :It tells you if you succefully applied the poison Confirmed: Harakiri
#define OP_DisarmComplete   0xaa20      // Comment: Tazadar :It tells you if you succefully disarmed Confirmed: Harakiri 1 or 0 if success or not
#define OP_GMFindPlayer     0x6920      // Comment: Tazadar :It says (player is in zone ... at loc x=.. y=..
#define OP_EnvDamage		0x5820
#define SummonedItem	    0x7821
#define SummonedContainer   0x7921
#define OP_ClassTraining	0x9c20		// Comment:	AKA (GMTraining) - Renamed to avoid confusion
#define OP_ClassEndTraining	0x9d20		// Comment:	AKA (GMEndTraining) - Renamed to avoid confusion
#define OP_ClassTrainSkill	0x4021		// Comment: AKA (GMTrainSkill) - Renamed to avoid confusion
#define OP_TimeOfDaySound	0xf220		// Comment: Yeahlight: I hear cricket noises with this opcode
#define OP_SummonUnknownItem 0x3420  // Comment: Harakiri default perlnecklace will be summoned at cursor
#define OP_SpawnUnkownItem	0xf620	// Comment: Harakiri bag spawns at 0 0 0 int8[20] all 1, other values may crash client with 100 array

/*========== Support Opcodes ===========*/
#define OP_GMNameApprovalWindow	0x8c20  // Comment: Harakiri GM Name Approval Window appears
#define OP_GMPetitionWindow		0xd021  // Comment: Harakiri GM Petition Window appears
#define OP_PetitionCheckout		0x8e21	// Comment: 
#define OP_PetitionDelete		0xa021	// Comment: 
#define OP_PetitionCheckIn		0x9e21	// Comment: 
#define OP_PetitionRefresh		0x1120	// Comment: 
#define OP_BugReport			0xb320	// Comment: 
#define OP_Petition				0x0e20	// Comment: 
#define OP_Social_Text			0x1520	// Comment: 
#define OP_Social_Action		0x9F20	// Comment: 
#define OP_SetDataRate			0xe821	// Comment: 
#define OP_SetServerFilter		0xff21	// Comment: 
#define OP_SetServerFilterAck	0xc321	// Comment: 
#define OP_TradeSkillCombine	0x0521	// Comment: 

/*=========== GM Commands ==============*/
#define OP_GMGetSkillInfo		0xe021	// Comment: Harakiri - prints all the skills and their value of a target?
#define OP_GMZoneRequest		0x4f21 	// Comment: Quagmire - client sends this when you use /zone
#define OP_GMZoneRequest2		0x0822	// Comment: 
#define OP_GMGoto				0x6e20	// Comment: /goto command
#define OP_GMSummon				0xc520	// Comment: 
#define OP_GMNameChange			0xcb20	// Comment: /name command
#define OP_GMKill				0x6c20	// Comment: /kill command
#define OP_GMSurname			0x6e21	// Comment: /Surname command
#define OP_GMToggle				0xde21	// Comment: /toggle command
#define OP_GMDelCorpse			0xe921	// Comment: 
#define	OP_GMKick				0x6d20	// Comment: 
#define OP_GMServers			0xa820	// Comment: 
#define OP_GMHideMe				0xd421	// Comment: 
#define OP_GMEmoteZone			0xe321	// Comment: 
#define OP_GMBecomeNPC			0x8c21	// Comment: Yeahlight: /becomeNPC
/*======================================*/

/************ ENUMERATED PACKET OPCODES ************/
#define ALL_FINISH                  0x0500	// Comment: 

// Login Server?
#define LS_REQUEST_VERSION          0x5900	// Comment: 
#define LS_SEND_VERSION             0x5900	// Comment: 
#define LS_SEND_LOGIN_INFO          0x0100	// Comment: 
#define LS_SEND_SESSION_ID          0x0400	// Comment: 
#define LS_REQUEST_UPDATE           0x5200	// Comment: 
#define LS_SEND_UPDATE              0x5200	// Comment: 
#define LS_REQUEST_SERVERLIST       0x4600	// Comment: 
#define LS_SEND_SERVERLIST          0x4600	// Comment: 
#define LS_REQUEST_SERVERSTATUS     0x4800	// Comment: 
#define LS_SEND_SERVERSTATUS        0x4A00	// Comment: 
#define LS_GET_WORLDID              0x4700	// Comment: 
#define LS_SEND_WORLDID             0x4700	// Comment: 

// World Server?
#define WS_SEND_LOGIN_INFO          0x5818	// Comment: 
#define WS_SEND_LOGIN_APPROVED      0x0710	// Comment: 
#define WS_SEND_LOGIN_APPROVED2     0x0180	// Comment: 
#define WS_SEND_CHAR_INFO           0x4720	// Comment: 

#endif
