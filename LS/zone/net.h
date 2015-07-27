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
#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <netdb.h>
#endif


#include <errno.h>
#include <fcntl.h>
#include "../common/types.h"

void CatchSignal(int);
void UpdateWindowTitle(char* iNewTitle = 0);

class NetConnection
{
public:
	NetConnection::~NetConnection();
	NetConnection::NetConnection() { ZonePort = 0; ZoneAddress = 0; WorldAddress = 0; };

	int32	GetIP();
	int32	GetIP(char* name);
	void	SaveInfo(char* address, int32 port, char* waddress,char* filename);
	char*	GetWorldAddress() { return WorldAddress; }
	char*	GetZoneAddress() { return ZoneAddress; }
	char*	GetZoneFileName() { return ZoneFileName; }
	int32	GetZonePort() { return ZonePort; }
private:
	int16 ZonePort;
	char* ZoneAddress;
	char* WorldAddress;
	char ZoneFileName[50];
};
