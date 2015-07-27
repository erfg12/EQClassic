#ifndef CLIENTLIST_H
#define CLIENTLIST_H

#include "Mutex.h"
#include "types.h"
#include "linked_list.h"
#include "client.h"

/*
  ClientList class is for use within the network thread ONLY, dont call add
  anything here to be called from any of the Process() functions without
  proper locking.

  -Quagmire
*/

class ClientList {
public:
	ClientList();
	~ClientList();
	void SendPacketQueues(bool block = false);
	void Add(Client* client);
	void Remove(Client* client);
	
	bool RecvData(int32 ip, int16 port, uchar* buffer, int len);
private:
	void RemoveAll();
	LinkedList<Client*>* list;
	Mutex MListLock;
};

#endif
