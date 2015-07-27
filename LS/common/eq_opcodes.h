/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 4001-4002  EQEMu Development Team (http://eqemu.org)

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  04111-1307  USA
*/
#ifndef EQ_OPCODES_H
#define EQ_OPCODES_H

#define   OP_TargetGroupBuff 0x2042 
#define   OP_Target         0xfe41 // /client /target and /rtarget
#define   OP_Target2      0xe401 // LoY Opcode?

// Darvik: for shopkeepers
#define OP_ShopRequest     0x0b40 // right-click on merchant
#define OP_ShopItem        0x0c40
#define OP_ShopPlayerBuy   0x3540
//#define OP_ShopGiveItem    0x3140 // = OP_ItemTradeIn
#define OP_ShopTakeMoney   0x3540
#define OP_ShopPlayerSell  0x2740
#define OP_ShopEnd         0x3740
#define OP_ShopEndConfirm  0x4541

#define OP_Assist		   0x0042

#define	OP_RequestDuel	   0xcf40
#define OP_DuelResponse	   0xd040
#define OP_DuelResponse2   0x5d41
// Disgrace: for item looting
#define OP_ItemTradeIn	   0x3140
#define OP_LootComplete    0x4541
#define OP_EndLootRequest  0x4F40
#define OP_MoneyOnCorpse   0x5040
#define OP_ItemOnCorpse    0x5240
#define OP_LootItem        0xA040
#define OP_LootRequest     0x4e40
// Yodason NPC GIVE
#define OP_TradeAccepted   0xE640
#define OP_TradeRequest	   0xd140
#define OP_ItemToTrade	   0xdf40
#define OP_CancelTrade     0xdb40
#define OP_Click_Give	   0xda40
#define OP_CloseTrade	   0xdc40
#define OP_FinTrade        0x4741
#define OP_ItemInTrade	   0xdf40
// Quagmire: Guild /-commands
#define OP_GuildInviteAccept 0x1841
#define OP_GuildLeader     0x9541 // /guildleader
#define OP_GuildPeace      0x9141 // /guildpeace
#define OP_GuildWar        0x6f41 // /guildwar
#define OP_GuildInvite     0x1741 // /guildinvite
#define OP_GuildRemove     0x1941 // /guildremove
#define OP_GuildMOTD	   0x0342 // /guildmotd
#define OP_GuildUpdate     0x7b41
#define OP_GuildsList      0x9241

#define OP_PetCommands		0x4542

#define OP_BoardBoat		0xbb41
#define OP_LeaveBoat		0xbc41
//Image: Pet Struct Commands
#define PET_BACKOFF			1
#define PET_GETLOST			2
#define PET_HEALTHREPORT	4
#define PET_GUARDHERE		5
#define PET_GUARDME			6
#define PET_ATTACK			7
#define PET_FOLLOWME		8
#define PET_SITDOWN			9
#define PET_STANDUP			10
#define PET_TAUNT			11
#define PET_LEADER			16

#define	OP_ReadBook			0xce40
#define OP_MultiLineMsg		0x1440
#define OP_CPlayerItems		0xf641
#define OP_CPlayerItem		0x6441
#define OP_Illusion			0x9140
#define OP_LFG				0xf041
#define OP_Petition		   0x0e40
#define OP_GMZoneRequest   0x4f41 // Quagmire - client sends this when you use /zone
#define OP_GMZoneRequest2  0x0842
#define OP_GMGoto		   0x6e40 // /goto
#define OP_GMSummon		   0xc540
#define OP_GMNameChange	   0xcb40 // /name
#define OP_GMKill		   0x6c40 // /kill
#define OP_GMLastName	   0x6e41 // /lastname
#define OP_GMToggle		   0xde41 // /toggle
#define OP_Translocate     0x0642
#define OP_Jump	           0x2040
#define OP_MovementUpdate  0x1F40
#define OP_SenseHeading    0x8741
#define OP_SafePoint	   0x2440
#define OP_Surname         0xc441
#define OP_YellForHelp     0xda41
#define OP_ZoneServerInfo  0x0480
#define OP_ChannelMessage  0x0741
#define OP_MOTD			   0xdd41
#define OP_Camp            0x0742
#define OP_GMEmoteZone		0xe341
#define OP_TargetGroupBuff 0x2042 
#define OP_ZoneEntry		0x2840
#define OP_DeleteSpawn		0x2940
#define OP_MoveItem        0x2c41
#define OP_MoveCoin        0x2d41
#define OP_PlayerProfile	0x3640
#define OP_Save            0x2e40
#define OP_Consider        0x3741
#define OP_ConsiderCorpse   0x3442
#define OP_SendCharInfo    0x4740
#define OP_NewSpawn        0x6b42
#define OP_ZoneSpawns		0x5f41
#define OP_Death           0x4a40
#define OP_AutoAttack      0x5141
#define OP_Consume         0x5641
#define OP_Stamina         0x5741
#define OP_AutoAttack2     0x6141 /*Why 2?*/
#define OP_SendLoginInfo   0x5818
#define OP_Action          0x5840
#define OP_DeleteCharacter 0x5a40
#define OP_NewZone         0x5b40
#define OP_Disciplines	   0x6042 //Image: Dunno, im testing
#define OP_CombatAbility   0x6041
#define OP_ClientTarget    0x6241
#define OP_SummonedItem    0x7841
#define OP_SpecialMesg     0x8041
#define OP_Sneak           0x8541
#define OP_Hide            0x8641
#define OP_SenseTraps      0x8841
#define OP_WearChange      0x9240
#define OP_Bind_Wound      0x9340 //Baron-Sprite: Bind Wound.
#define OP_Forage          0x9440
#define OP_DropCoin		   0x0740
#define OP_LevelUpdate     0x9841
#define OP_ExpUpdate       0x9941
#define OP_SkillUpdate     0x8941
#define OP_Mend            0x9d41
#define OP_Attack			0xa140
#define OP_MobUpdate		0x9f40
#define OP_ZoneChange      0xa340
#define OP_HPUpdate        0xb240
#define OP_TimeOfDay       0xf240
#define OP_ClientUpdate    0xf340
#define OP_DisarmTraps     0xf341
#define OP_WhoAll          0xf440
#define OP_SpawnAppearance 0xf540
#define OP_BeginCast       0xa940
#define OP_CastSpell       0x7e41
#define OP_Buff            0x3241
#define OP_CastOn          0x4640
#define OP_InterruptCast   0x3542
#define OP_HarmTouch	   0x7E41
#define OP_ManaChange      0x7f41
#define OP_MemorizeSpell   0x8241
#define OP_DeleteSpell	   0x4a42
#define OP_SwapSpell       0xce41
#define OP_FeignDeath	   0xac40
// Dunno what Instill doubt was changed to, but 9f40 is MobUpdate now so....
#define OP_InstillDoubt		0x9c41
#define OP_ExpansionInfo	0xd841
#define OP_GMHideMe		0xd441
#define OP_Random		0xe741
#define OP_Weather		0x3641
#define OP_Stun			0x5b41
#define OP_WebUpdate		0x3c42
#define OP_Taunt		0x3b41
#define OP_ControlBoat		0x2641
#define OP_Drink		0x4641
#define OP_SafeFallSuccess	0xab41

#define OP_CharacterCreate	0x4940
#define OP_ZoneUnavail		0x0580

#define OP_ItemView			0x6442

#define OP_EnterWorld		0x0180

#define OP_GroupInvite		0x3e20
#define OP_GroupInvite2		0x4040
#define OP_GroupFollow		0x3d20
#define OP_GroupFollow2		0x4240
#define OP_GroupDisband		0x4440
#define OP_CancelInvite		0x4120
#define OP_GroupUpdate		0x2620
#define OP_GroupDelete		0x9721

#define OP_InspectRequest	0xb540
#define OP_InspectAnswer	0xb640

#define OP_PetitionCheckout	0x8e41
#define OP_PetitionDelete	0xa041
#define OP_PetitionCheckIn	0x9e41
#define OP_PetitionRefresh	0x1140
#define OP_ViewPetition		0x6142
#define OP_DeletePetition	0x6422

#define OP_Social_Text		0x1540
#define OP_Social_Action	0xa140

#define OP_GMTraining		0x9c40
#define OP_GMEndTraining	0x9d40
#define OP_GMTrainSkill		0x4041
#define OP_CommonMessage    0x3642

#define OP_4141             0x4141
#define OP_4441             0x4441

#define OP_SetDataRate		0xe841
#define OP_SetServerFilter	0xff41
#define OP_SetServerFilterAck	0xc341
#define OP_GMDelCorpse		0xe941
#define	OP_GMKick		0x6d40
#define OP_GMServers		0xa840
#define OP_BecomeNPC		0x8c41

#define OP_TradeSkillCombine 0x0541

#define	OP_CreateObject		0x2c40
#define	OP_ClickObject		0x2b40
#define	OP_SpawnDoor		0xf741
#define	OP_ClickDoor		0x8d40
#define	OP_MoveDoor		0x8e40

#define OP_SendZonepoints	0xb440

#define OP_RezzRequest		0x2a41
#define OP_RezzAnswer		0x9b41
#define OP_RezzComplete		0xec41

#define OP_Medding		0x5841

//#define OP_AAPoints			0x1542

#define OP_SetRunMode		0x1f40
#define OP_EnvDamage		0x5840
#define OP_EnvDamage2		0x1e40
#define OP_Report			0xbd41

#define OP_UpdateAA      0x1442
#define OP_RespondAA     0x1542
#define OP_SendAAStats   0x2342

#define OP_ApproveName   0x8B40

// Bazaar Stuff =D
#define OP_BazaarWindow  0x1142
#define OP_TraderWindow  0x1422

#define OP_GMFind			0x6940

#define OP_Charm         0x4442

// Rogue skills
#define OP_PickPocket   0xad40
#define OP_SenseTrap    0x8841

// Agz: The following is from the old source I used as base
/************ ENUMERATED PACKET OPCODES ************/
#define ALL_FINISH                  0x0500
#define LS_REQUEST_VERSION          0x5900
#define LS_SEND_VERSION             0x5900
#define LS_SEND_LOGIN_INFO          0x0100
#define LS_SEND_SESSION_ID          0x0400
#define LS_REQUEST_UPDATE           0x5200
#define LS_SEND_UPDATE              0x5200
#define LS_REQUEST_SERVERLIST       0x4600
#define LS_SEND_SERVERLIST          0x4600
#define LS_REQUEST_SERVERSTATUS     0x4800
#define LS_SEND_SERVERSTATUS        0x4A00
#define LS_GET_WORLDID              0x4700
#define LS_SEND_WORLDID             0x4700
#define WS_SEND_LOGIN_INFO          0x5818
#define WS_SEND_LOGIN_APPROVED      0x0710
#define WS_SEND_LOGIN_APPROVED2     0x0180
#define WS_SEND_CHAR_INFO           0x4720
#endif
