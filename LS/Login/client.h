#ifndef CLIENT_H
#define CLIENT_H

#include "../common/linked_list.h"
#include "../common/timer.h"

//#define CLIENT_JOINPART

#define WorldEqualZeroErrorMessage	"Worldserver not found."
#define WorldDownErrorMessage		"World is down."
#define WorldLockedErrorMessage		"World is locked."
#define PasswordErrorMessage		"Incorrect username or password."
#define ErrorRegUnavail				"Registration functions are unavailable at this time."
#define ErrorAccountBanished		"This account has been BANNED. To contest this decision, please contact the server's administration."
#define ErrorAccountSuspended		"This account has been SUSPENDED. To contest this decision, please contact the server's administration."
#define ErrorAccountNotVerified		"Your account has not been verified by e-mail.  Please goto http://www.eqemulator.org/ to update your information."
#define PasswordChangeMessage		"Account password successfully changed."

enum eLoginMode { None, Normal, Registration };

class Client
{
public:
	Client(EQNetworkConnection* ieqnc);
    ~Client();
	
	bool	Process();

	int32	GetIP()    { return ip; }
	int16	GetPort()  { return port; }
	int32	GetAccountID() { return account_id; }

	void	QueuePacket(APPLAYER* app);
	void	FastQueuePacket(APPLAYER** app);
	void	FatalError(const char* message, bool disconnect = true);
	int32	CheckLoginInfo(int8* auth, int32 ip);
	void	WorldResponse(int32 worldid,sint8 response);
private:
	EQNetworkConnection* eqnc;

	int32	ip;
	int16	port;

	int32	account_id;
	char	account_name[30];
	int8	lsadmin;
	sint16	worldadmin;
	int		lsstatus;
	bool	kicked;
	bool	verified;
	char	bannedreason[30];
	eLoginMode LoginMode;
	Timer*	worldresponse_timer;

#ifdef MINILOGIN
	char	account_password[30];
#endif
};

class ClientList
{
public:
	ClientList()	{}
	~ClientList()	{}
	
	void	Add(Client* client);
	Client*	Get(int32 ip, int16 port);
	Client* FindByLSID(int32 lsaccountid);
	void	Process();
private:
	LinkedList<Client*> list;
};

#endif
