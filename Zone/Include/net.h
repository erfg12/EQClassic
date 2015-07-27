


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
#include "types.h"

void CatchSignal(int);
#ifdef WIN32
	void ProcessLoop(void* tmp);
#else
	void* ProcessLoop(void* tmp);
#endif

class NetConnection
{
public:
	~NetConnection();
	NetConnection() { ZonePort = 0; ZoneAddress = 0; WorldAddress = 0;};

	bool Init();
	void ListenNewClients();

	int32	GetIP();
	int32	GetIP(char* name);
	void	Close();
	void	SaveInfo(char* address, int32 port, char* waddress);
	char*	GetWorldAddress() { return WorldAddress; }
	char*	GetZoneAddress() { return ZoneAddress; }
	int32	GetZonePort() { return ZonePort; }
	void	StartProcessLoop();
private:
	int listening_socket;
	int16 ZonePort;
	char* ZoneAddress;
	char* WorldAddress;
};



