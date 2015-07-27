/***************************************************************************
                       login_opcodes.h  -  description
                             -------------------
    begin                : Wed Apr 4 2001
    copyright            : (C) 2001 by Xylor
    email                : xylor@openquest.dhs.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LOGIN_OPCODES_H
#define LOGIN_OPCODES_H

#define OP_LoginInfo					0x0100
#define OP_FatalError					0x0200
#define OP_SessionId					0x0400
//#define OP_Reg_SendPricing				0x0400
#define OP_AllFinish					0x0500
#define OP_Chat_ChannelList				0x0600
#define OP_Chat_JoinChannel				0x0700
#define OP_Chat_PartChannel				0x0800
#define OP_Chat_ChannelMessage			0x0900
#define OP_Chat_Tell					0x0a00
#define OP_Chat_SysMsg					0x0b00
#define OP_Chat_CreateChannel			0x0c00
#define OP_Chat_ChangeChannel			0x0d00
#define OP_Chat_DeleteChannel			0x0e00
#define OP_Chat_UserList				0x1000
#define OP_Reg_GetPricing				0x1a00	// for new account signup
#define OP_Reg_SendPricing				0x1b00
#define OP_RegisterAccount				0x2300
#define OP_Chat_ChannelWelcome			0x2400
#define OP_Chat_PopupMakeWindow			0x3000
#define OP_BillingInfoAccepted			0x3300	// i THINK =p
#define OP_CheckGameCardValid			0x3400
#define OP_GameCardTimeLeft				0x3600
#define OP_AccountExpired				0x4200
#define OP_Reg_GetPricing2				0x4400	// for re-registering
#define OP_ChangePassword				0x4500
#define OP_ServerList					0x4600
#define OP_SessionKey					0x4700
#define OP_RequestServerStatus			0x4800
#define OP_SendServerStatus				0x4A00
#define OP_Reg_ChangeAcctLogin			0x5100
#define OP_LoginBanner					0x5200
#define OP_Chat_GuildsList				0x5500
#define OP_Chat_GuildEdit				0x5700
#define OP_Version						0x5900
#define OP_RenewAccountBillingInfo		0x7a00

#endif /* LOGIN_OPCODES_H */

