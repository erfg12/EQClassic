#ifndef DBCORE_H
#define DBCORE_H

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
#endif
#include <mysql.h>
#include "../common/DBMemLeak.h"
#include "../common/types.h"
#include "../common/Mutex.h"
#include "../common/linked_list.h"
#include "../common/queue.h"
#include "../common/timer.h"


class DBcore {
public:
	enum eStatus { Closed, Connected, Error };

	DBcore();
	~DBcore();
	eStatus	GetStatus() { return pStatus; }
	bool	RunQuery(const char* query, int32 querylen, char* errbuf = 0, MYSQL_RES** result = 0, int32* affected_rows = 0, int32* last_insert_id = 0, int32* errnum = 0, bool retry = true);
	int32	DoEscapeString(char* tobuf, const char* frombuf, int32 fromlen);
	void	ping();
protected:
	bool	Open(const char* iHost, const char* iUser, const char* iPassword, const char* iDatabase, int32* errnum = 0, char* errbuf = 0, bool iCompress = false, bool iSSL = false);
private:
	bool	Open(int32* errnum = 0, char* errbuf = 0);

	MYSQL	mysql;
	Mutex	MDatabase;
	eStatus pStatus;

	char*	pHost;
	char*	pUser;
	char*	pPassword;
	char*	pDatabase;
	bool	pCompress;
	bool	pSSL;

};

class DBAsync;
class DBAsyncFinishedQueue;
class DBAsyncWork;
class DBAsyncQuery;

// Big daddy that owns the threads and does the work
class DBAsync {
public:
	enum Status { AddingWork, Queued, Executing, Finished, Canceled };
	enum Type { Read, Write, Both };

	DBAsync(DBcore* iDBC);
	~DBAsync();
	bool	StopThread();

	int32	AddWork(DBAsyncWork** iWork, int32 iDelay = 0);
	bool	CancelWork(int32 iWorkID);
	void	CommitWrites();

	void	AddFQ(DBAsyncFinishedQueue* iDBAFQ);
protected:
	friend ThreadReturnType DBAsyncLoop(void* tmp);
	Mutex	MLoopRunning;
	bool	RunLoop();
	void	Process();
private:
	void	ProcessWork(DBAsyncWork* iWork, bool iSleep = true);
	void	DispatchWork(DBAsyncWork* iWork);
	inline	int32	GetNextID()		{ return pNextID++; }
	DBAsyncWork*	InListPop();
	DBAsyncWork*	InListPopWrite();	// Ignores delay
	void			OutListPush(DBAsyncWork* iDBAW);

	Mutex	MRunLoop;
	bool	pRunLoop;

	DBcore*	pDBC;
	int32	pNextID;
	Timer*	pTimeoutCheck;
	Mutex	MInList;
	LinkedList<DBAsyncWork*> InList;

	Mutex	MFQList;
	LinkedList<DBAsyncFinishedQueue**> FQList;

	// Mutex for outside access to current work & when current work is being changed.
	// NOT locked when CurrentWork is being accessed by the DBAsync thread.
	// Never change pointer from outside DBAsync thread!
	// Only here for access to thread-safe DBAsyncWork functions.
	Mutex	MCurrentWork;
	DBAsyncWork* CurrentWork;

};

/*
	DB Work Complete Callback:
		This will be called under the DBAsync thread! Never access any non-threadsafe 
		data/functions/classes. (ie: zone, entitylist, client, etc are not threadsafe)
	Function prototype:
		return value:	true if we should delete the data, false if we should keep it
*/
typedef bool(*DBWorkCompleteCallBack)(DBAsyncWork*);

class DBAsyncFinishedQueue {
public:
	DBAsyncFinishedQueue(DBAsync* iDBA, int32 iTimeout = 90000);
	~DBAsyncFinishedQueue();

	DBAsyncWork*	Pop();
	DBAsyncWork*	PopByWPT(int32 iWPT);
	DBAsyncWork*	Find(int32 iWPT);
	bool			Push(DBAsyncWork* iDBAW);

	void			CheckTimeouts();
private:
	Mutex MLock;
	int32 pTimeout;
	LinkedList<DBAsyncWork*> list;
};

// Container class for multiple queries
class DBAsyncWork {
public:
	DBAsyncWork(DBAsyncFinishedQueue* iDBAFQ, int32 iWPT = 0, DBAsync::Type iType = DBAsync::Both, int32 iTimeout = 0);
	DBAsyncWork(DBWorkCompleteCallBack iCB, int32 iWPT = 0, DBAsync::Type iType = DBAsync::Both, int32 iTimeout = 0);
	~DBAsyncWork();

	bool			AddQuery(DBAsyncQuery** iDBAQ);
	bool			AddQuery(int32 iQPT, char** iQuery, int32 iQueryLen = 0xFFFFFFFF, bool iGetResultSet = true, bool iGetErrbuf = true);
	int32			WPT();
	DBAsync::Type	Type();

	// Pops finished queries off the work
	DBAsyncQuery*	PopAnswer();
	int32			QueryCount();

	bool			CheckTimeout(int32 iFQTimeout);
	bool			SetWorkID(int32 iWorkID);
	int32			GetWorkID();
protected:
	friend class DBAsync;
	DBAsync::Status	SetStatus(DBAsync::Status iStatus);
	bool			Cancel();
	bool			IsCancled();
	DBAsyncQuery*	PopQuery();	// Get query to be run
	void			PushAnswer(DBAsyncQuery* iDBAQ);	// Push answer back into workset

	// not mutex'd cause only to be accessed from dbasync class
	int32	pExecuteAfter;
private:
	Mutex	MLock;
	int32	pQuestionCount;
	int32	pAnswerCount;
	int32	pWorkID;
	int32	pWPT;
	int32	pTimeout;
	int32	pTSFinish; // timestamp when finished
	DBAsyncFinishedQueue*	pDBAFQ;
	DBWorkCompleteCallBack	pCB;
	DBAsync::Status			pstatus;
	DBAsync::Type			pType;
	MyQueue<DBAsyncQuery>	todo;
	MyQueue<DBAsyncQuery>	done;
	MyQueue<DBAsyncQuery>	todel;
};

// Container class for the query information
class DBAsyncQuery {
public:
	DBAsyncQuery(int32 iQPT, char** iQuery, int32 iQueryLen = 0xFFFFFFFF, bool iGetResultSet = true, bool iGetErrbuf = true);
	DBAsyncQuery(int32 iQPT, const char* iQuery, int32 iQueryLen = 0xFFFFFFFF, bool iGetResultSet = true, bool iGetErrbuf = true);
	~DBAsyncQuery();

	bool	GetAnswer(char* errbuf = 0, MYSQL_RES** result = 0, int32* affected_rows = 0, int32* last_insert_id = 0, int32* errnum = 0);
	inline int32 QPT()	{ return pQPT; }
protected:
	friend class DBAsyncWork;
	int32		pQPT;

	friend class DBAsync;
	void		Process(DBcore* iDBC);

	void		Init(int32 iQPT, bool iGetResultSet, bool iGetErrbuf);
	DBAsync::Status pstatus;
	char*		pQuery;
	int32		pQueryLen;
	bool		pGetResultSet;
	bool		pGetErrbuf;

	bool		pmysqlsuccess;
	char*		perrbuf;
	int32		perrnum;
	int32		paffected_rows;
	int32		plast_insert_id;
	MYSQL_RES*	presult;
};

#endif

