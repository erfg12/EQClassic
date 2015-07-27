
#include "ClientList.h"
#ifndef WIN32
	#include <pthread.h>
	#include "unix.h"
#endif

ClientList client_list;

ClientList::ClientList() {
	list = new LinkedList<Client*>;
}

ClientList::~ClientList() {
	RemoveAll();
	safe_delete(list);//delete list;
}

void ClientList::Add(Client* client)
{
	MListLock.lock();
	list->Insert(client);
	MListLock.unlock();
}

void ClientList::SendPacketQueues(bool block) {
	LockMutex lock(&MListLock);
	LinkedListIterator<Client*> iterator(*list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.GetData()->SendPacketQueue(block);
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

void ClientList::Remove(Client* client) {
	LockMutex lock(&MListLock);
	LinkedListIterator<Client*> iterator(*list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData() == client) {
			iterator.RemoveCurrent(false);
			break;
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

void ClientList::RemoveAll() {
	LockMutex lock(&MListLock);
	LinkedListIterator<Client*> iterator(*list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.RemoveCurrent(false);
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
}

bool ClientList::RecvData(int32 ip, int16 port, uchar* buffer, int len) {
	LockMutex lock(&MListLock);
	LinkedListIterator<Client*> iterator(*list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		Client* client = iterator.GetData()->CastToClient();
		if (client->GetIP() == ip && client->GetPort() == port) {
			client->ReceiveData(buffer, len);
			return true;
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	return false;
}

