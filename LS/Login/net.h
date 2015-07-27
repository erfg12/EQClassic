#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

//#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#include "../common/types.h"

void CatchSignal(int sig_num);
enum eServerMode { Standalone, Master, Slave, Mesh };

#ifdef MINILOGIN
	#define MaxMiniLoginAccounts	200
	struct MiniLoginAccounts_Struct {
		int32	IP;
		char	username[30];
		char	password[30];
	};
#endif

class NetConnection
{
public:
	NetConnection() {
		port = 5999;
		listening_socket = 0;
		memset(masteraddress, 0, sizeof(masteraddress));
		uplinkport = 0;
		memset(uplinkaccount, 0, sizeof(uplinkaccount));
		memset(uplinkpassword, 0, sizeof(uplinkpassword));
		LoginMode = Standalone;
		Uplink_WrongVersion = false;
#ifdef MINILOGIN
		memset(MiniLoginAccounts, 0, sizeof(MiniLoginAccounts_Struct) * MaxMiniLoginAccounts);
#endif
	}

	bool Init();
	void ListenNewClients();
	void HitKey(int keyhit);
	char address[1024];

	int16	GetPort()	{ return port; }
	void	SetPort(int16 in_port) { port = in_port; }
	eServerMode	GetLoginMode()	{ return LoginMode; }

	bool	ReadLoginConfig();
	char*	GetMasterAddress()	{ return masteraddress; }
	int16	GetUplinkPort()		{ if (uplinkport != 0) return uplinkport; else return port; }
	char*	GetUplinkAccount()	{ return uplinkaccount; }
	char*	GetUplinkPassword()	{ return uplinkpassword; }

#ifdef MINILOGIN
	bool	ReadMiniLoginAccounts();
	MiniLoginAccounts_Struct MiniLoginAccounts[MaxMiniLoginAccounts];
#endif
protected:
	friend class LWorld;
	bool	Uplink_WrongVersion;
private:
	int16	port;
	int		listening_socket;
	char	masteraddress[300];
	int16	uplinkport;
	char	uplinkaccount[300];
	char	uplinkpassword[300];
	eServerMode	LoginMode;
};
