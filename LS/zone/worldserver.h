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
#ifndef WORLDSERVER_H
#define WORLDSERVER_H

#include "../common/servertalk.h"
#include "../common/linked_list.h"
#include "../common/timer.h"
#include "../common/queue.h"
#include "../common/packet_functions.h"
#include "../common/eq_packet_structs.h"
#include "../common/Mutex.h"
#include "../common/TCPConnection.h"
#include "mob.h"

const int PORT = 9000;

#define WSCS_Construction	0
#define WSCS_Ready			1
#define WSCS_Connecting		2
#define WSCS_Authenticating	3
#define WSCS_Connected		100
#define WSCS_Disconnecting	200

class WorldServer {
public:
	WorldServer();
    ~WorldServer();

	void Process();
	bool SendPacket(ServerPacket* pack);
	bool SendChannelMessage(Client* from, const char* to, int8 chan_num, int32 guilddbid, int8 language, const char* message, ...);
	bool SendEmoteMessage(const char* to, int32 to_guilddbid, int32 type, const char* message, ...);
	bool SendEmoteMessage(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message, ...);
	void SetZone(int32 iZoneID);
	void SetConnectInfo();
	bool RezzPlayer(APPLAYER* rpack,int32 rezzexp, int16 opcode);
	int32	GetIP()		{ return tcpc->GetrIP(); }
	int16	GetPort()	{ return tcpc->GetrPort(); }
	bool	Connected()	{ return (pConnected && tcpc->Connected()); }

	bool	Connect();
	void	AsyncConnect();
	void	Disconnect();
	inline int16	GetErrorNumber()	{ return adverrornum; }
	inline bool		TryReconnect()		{ return pTryReconnect; }
private:
	TCPConnection* tcpc;
	int16	adverrornum;
	bool	pTryReconnect;
	bool	pConnected;
};
#endif

