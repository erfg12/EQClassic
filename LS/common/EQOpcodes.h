/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/**************************************************************************************/
/**************************************************************************************/

/************ ENUMERATED PACKET OPCODES ************/
#define ALL_FINISH					0x0500



#define LS_REQUEST_VERSION			0x5900
#define LS_SEND_VERSION				0x5900

#define LS_SEND_LOGIN_INFO			0x0100
#define LS_SEND_SESSION_ID			0x0400

#define LS_REQUEST_UPDATE			0x5200
#define LS_SEND_UPDATE				0x5200

#define LS_REQUEST_SERVERLIST		0x4600
#define LS_SEND_SERVERLIST			0x4600

#define LS_REQUEST_SERVERSTATUS		0x4800
#define LS_SEND_SERVERSTATUS		0x4A00

#define LS_GET_WORLDID				0x4700
#define LS_SEND_WORLDID				0x4700

#define WS_SEND_LOGIN_INFO			0x5818
#define WS_SEND_LOGIN_APPROVED		0x0710
#define WS_SEND_LOGIN_APPROVED2		0x0180
#define WS_SEND_CHAR_INFO			0x4720



