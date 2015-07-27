#include "../common/debug.h"

#include <iostream>
#include <errmsg.h>
#include <mysqld_error.h>
#include <limits.h>
#include "dbcore.h"
#include <string.h>
#include "../common/MiscFunctions.h"

using namespace std;

#ifdef WIN32
	#define snprintf	_snprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp	_stricmp
	#include <process.h>
#else
	#include "unix.h"
	#include <pthread.h>
#endif

#ifdef _DEBUG
	#define DEBUG_MYSQL_QUERIES 0
#else
	#define DEBUG_MYSQL_QUERIES 0
#endif

DBcore::DBcore() {
	mysql_init(&mysql);
	pHost = 0;
	pUser = 0;
	pPassword = 0;
	pDatabase = 0;
	pCompress = false;
	pSSL = false;
}

DBcore::~DBcore() {
	mysql_close(&mysql);
	safe_delete(pHost);
	safe_delete(pUser);
	safe_delete(pPassword);
	safe_delete(pDatabase);
}

// Sends the MySQL server a keepalive
void DBcore::ping() {
	if (!MDatabase.trylock()) {
		// well, if's it's locked, someone's using it. If someone's using it, it doesnt need a keepalive
		return;
	}
	mysql_ping(&mysql);
	MDatabase.unlock();
}

bool DBcore::RunQuery(const char* query, int32 querylen, char* errbuf, MYSQL_RES** result, int32* affected_rows, int32* last_insert_id, int32* errnum, bool retry) {
	if (errnum)
		*errnum = 0;
	if (errbuf)
		errbuf[0] = 0;
	bool ret = false;
	LockMutex lock(&MDatabase);
	if (pStatus != Connected)
		Open();
#if DEBUG_MYSQL_QUERIES >= 1
	char tmp[120];
	strn0cpy(tmp, query, sizeof(tmp));
	cout << "QUERY: " << tmp << endl;
#endif
	if (mysql_real_query(&mysql, query, querylen)) {
		if (mysql_errno(&mysql) == CR_SERVER_GONE_ERROR)
			pStatus = Error;
		if (mysql_errno(&mysql) == CR_SERVER_LOST || mysql_errno(&mysql) == CR_SERVER_GONE_ERROR) {
			if (retry) {
				cout << "Database Error: Lost connection, attempting to recover...." << endl;
				ret = RunQuery(query, querylen, errbuf, result, affected_rows, last_insert_id, errnum, false);
			}
			else {
				pStatus = Error;
				if (errnum)
					*errnum = mysql_errno(&mysql);
				if (errbuf)
					snprintf(errbuf, MYSQL_ERRMSG_SIZE, "#%i: %s", mysql_errno(&mysql), mysql_error(&mysql));
				cout << "DB Query Error #" << mysql_errno(&mysql) << ": " << mysql_error(&mysql) << endl;
				ret = false;
			}
		}
		else {
			if (errnum)
				*errnum = mysql_errno(&mysql);
			if (errbuf)
				snprintf(errbuf, MYSQL_ERRMSG_SIZE, "#%i: %s", mysql_errno(&mysql), mysql_error(&mysql));
#ifdef _DEBUG
			cout << "DB Query Error #" << mysql_errno(&mysql) << ": " << mysql_error(&mysql) << endl;
#endif
			ret = false;
		}
	}
	else {
		if (result && mysql_field_count(&mysql)) {
			*result = mysql_store_result(&mysql);
#ifdef _DEBUG
			DBMemLeak::Alloc(*result, query);
#endif
		}
		else if (result)
			*result = 0;
		if (affected_rows)
			*affected_rows = mysql_affected_rows(&mysql);
		if (last_insert_id)
			*last_insert_id = mysql_insert_id(&mysql);
		if (result) {
			if (*result) {
				ret = true;
			}
			else {
#ifdef _DEBUG
				cout << "DB Query Error: No Result" << endl;
#endif
				if (errnum)
					*errnum = UINT_MAX;
				if (errbuf)
					strcpy(errbuf, "DBcore::RunQuery: No Result");
				ret = false;
			}
		}
		else {
			ret = true;
		}
	}
#if DEBUG_MYSQL_QUERIES >= 1
	if (ret) {
		cout << "query successful";
		if (result && (*result))
			cout << ", " << (int) mysql_num_rows(*result) << " rows returned";
		if (affected_rows)
			cout << ", " << (*affected_rows) << " rows affected";
		cout<< endl;
	}
	else {
		cout << "QUERY: query FAILED" << endl;
	}
#endif
	return ret;
}

int32 DBcore::DoEscapeString(char* tobuf, const char* frombuf, int32 fromlen) {
	LockMutex lock(&MDatabase);
	return mysql_real_escape_string(&mysql, tobuf, frombuf, fromlen);
}

bool DBcore::Open(const char* iHost, const char* iUser, const char* iPassword, const char* iDatabase, int32* errnum, char* errbuf, bool iCompress, bool iSSL) {
	LockMutex lock(&MDatabase);
	safe_delete(pHost);
	safe_delete(pUser);
	safe_delete(pPassword);
	safe_delete(pDatabase);
	pHost = strcpy(new char[strlen(iHost) + 1], iHost);
	pUser = strcpy(new char[strlen(iUser) + 1], iUser);
	pPassword = strcpy(new char[strlen(iPassword) + 1], iPassword);
	pDatabase = strcpy(new char[strlen(iDatabase) + 1], iDatabase);
	pCompress = iCompress;
	pSSL = iSSL;
	return Open(errnum, errbuf);
}

bool DBcore::Open(int32* errnum, char* errbuf) {
	if (errbuf)
		errbuf[0] = 0;
	LockMutex lock(&MDatabase);
	if (GetStatus() == Connected)
		return true;
	if (GetStatus() == Error)
		mysql_close(&mysql);
	if (!pHost)
		return false;
	/*
	Quagmire - added CLIENT_FOUND_ROWS flag to the connect
	otherwise DB update calls would say 0 rows affected when the value already equalled
	what the function was tring to set it to, therefore the function would think it failed 
	*/
	int32 flags = CLIENT_FOUND_ROWS;
	if (pCompress)
		flags |= CLIENT_COMPRESS;
	if (pSSL)
		flags |= CLIENT_SSL;
	if (mysql_real_connect(&mysql, pHost, pUser, pPassword, pDatabase, 0, 0, flags)) {
		pStatus = Connected;
		return true;
	}
	else {
		if (errnum)
			*errnum = mysql_errno(&mysql);
		if (errbuf)
			snprintf(errbuf, MYSQL_ERRMSG_SIZE, "#%i: %s", mysql_errno(&mysql), mysql_error(&mysql));
		pStatus = Error;
		return false;
	}
}



ThreadReturnType DBAsyncLoop(void* tmp) {
	DBAsync* dba = (DBAsync*) tmp;
	dba->MLoopRunning.lock();
	while (dba->RunLoop()) {
		dba->Process();
		Sleep(1);
	}
	dba->MLoopRunning.unlock();
#ifndef WIN32
	return 0;
#endif
}

DBAsync::DBAsync(DBcore* iDBC) {
	pDBC = iDBC;
	pRunLoop = true;
	pNextID = 1;
	pTimeoutCheck = new Timer(10000);
#ifdef WIN32
	_beginthread(DBAsyncLoop, 0, this);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, &DBAsyncLoop, this);
#endif
}

DBAsync::~DBAsync() {
	StopThread();
	safe_delete(pTimeoutCheck);
}

bool DBAsync::StopThread() {
	bool ret;
	MRunLoop.lock();
	ret = pRunLoop;
	pRunLoop = false;
	MRunLoop.unlock();
	MLoopRunning.lock();
	MLoopRunning.unlock();
	return ret;
}

int32 DBAsync::AddWork(DBAsyncWork** iWork, int32 iDelay) {
	MInList.lock();
	int32 ret = GetNextID();
	if (!(*iWork)->SetWorkID(ret)) {
		MInList.unlock();
		return 0;
	}
	InList.Append(*iWork);
	(*iWork)->SetStatus(Queued);
	if (iDelay)
		(*iWork)->pExecuteAfter = Timer::GetCurrentTime() + iDelay;
#if DEBUG_MYSQL_QUERIES >= 2
	cout << "Adding AsyncWork #" << (*iWork)->GetWorkID() << endl;
	cout << "ExecuteAfter = " << (*iWork)->pExecuteAfter << " (" << Timer::GetCurrentTime() << " + " << iDelay << ")" << endl;
#endif
	iWork = 0;
	MInList.unlock();
	return ret;
}

bool DBAsync::CancelWork(int32 iWorkID) {
	if (iWorkID == 0)
		return false;
#if DEBUG_MYSQL_QUERIES >= 2
	cout << "DBAsync::CancelWork: " << iWorkID << endl;
#endif
	MCurrentWork.lock();
	if (CurrentWork && CurrentWork->GetWorkID() == iWorkID) {
		CurrentWork->Cancel();
		MCurrentWork.unlock();
		return true;
	}
	MCurrentWork.unlock();
	MInList.lock();
	LinkedListIterator<DBAsyncWork*> iterator(InList);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->GetWorkID() == iWorkID) {
			iterator.RemoveCurrent(true);
			MInList.unlock();
			return true;
		}
		iterator.Advance();
	}
	MInList.unlock();
	return false;
}

bool DBAsync::RunLoop() {
	bool ret;
	MRunLoop.lock();
	ret = pRunLoop;
	MRunLoop.unlock();
	return ret;
}

DBAsyncWork* DBAsync::InListPop() {
	DBAsyncWork* ret = 0;
	MInList.lock();
	LinkedListIterator<DBAsyncWork*> iterator(InList);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->pExecuteAfter <= Timer::GetCurrentTime()) {
			ret = iterator.GetData();
#if DEBUG_MYSQL_QUERIES >= 2
			cout << "Poping AsyncWork #" << ret->GetWorkID() << endl;
			cout << ret->pExecuteAfter << " <= " << Timer::GetCurrentTime() << endl;
#endif
			iterator.RemoveCurrent(false);
			break;
		}
		iterator.Advance();
	}
	MInList.unlock();
	return ret;
}

DBAsyncWork* DBAsync::InListPopWrite() {
	MInList.lock();
	LinkedListIterator<DBAsyncWork*> iterator(InList);

	DBAsyncWork* ret = 0;
	DBAsync::Type tmpType;
	iterator.Reset();
	while (iterator.MoreElements()) {
		tmpType = iterator.GetData()->Type();
		if (tmpType == Write || tmpType == Both) {
			ret = iterator.GetData();
			iterator.RemoveCurrent(false);
			break;
		}
		iterator.Advance();
	}
	MInList.unlock();
	return ret;
}

void DBAsync::AddFQ(DBAsyncFinishedQueue* iDBAFQ) {
	MFQList.lock();
	DBAsyncFinishedQueue** tmp = new DBAsyncFinishedQueue*;
	*tmp = iDBAFQ;
	FQList.Append(tmp);
	MFQList.unlock();
}

void DBAsync::Process() {
	DBAsyncWork* tmpWork;
	MCurrentWork.lock();
	while ((CurrentWork = InListPop())) {
		MCurrentWork.unlock();
		Status tmpStatus = CurrentWork->SetStatus(Executing);
		if (tmpStatus == Queued) {
			ProcessWork(CurrentWork);
			tmpWork = CurrentWork;
			MCurrentWork.lock();
			CurrentWork = 0;
			MCurrentWork.unlock();
			tmpStatus = tmpWork->SetStatus(DBAsync::Finished);
			if (tmpStatus != Executing) {
				if (tmpStatus != Canceled) {
					cout << "Error: Unexpected DBAsyncWork->Status in DBAsync::Process #1" << endl;
				}
				MCurrentWork.lock();
				safe_delete(tmpWork);
			}
			else {
				DispatchWork(tmpWork);
				Sleep(25);
				MCurrentWork.lock();
			}
		}
		else {
			if (tmpStatus != Canceled) {
				cout << "Error: Unexpected DBAsyncWork->Status in DBAsync::Process #2" << endl;
			}
			MCurrentWork.lock();
			safe_delete(CurrentWork);
		}
	}
	MCurrentWork.unlock();
	if (pTimeoutCheck->Check()) {
		MFQList.lock();
		LinkedListIterator<DBAsyncFinishedQueue**> iterator(FQList);

		iterator.Reset();
		while (iterator.MoreElements()) {
			(*iterator.GetData())->CheckTimeouts();
			iterator.Advance();
		}
		MFQList.unlock();
	}
}

void DBAsync::CommitWrites() {
#if DEBUG_MYSQL_QUERIES >= 2
	cout << "DBAsync::CommitWrites() called." << endl;
#endif
	DBAsyncWork* tmpWork;
	while ((tmpWork = InListPopWrite())) {
		Status tmpStatus = tmpWork->SetStatus(Executing);
		if (tmpStatus == Queued) {
			ProcessWork(tmpWork);
			tmpStatus = tmpWork->SetStatus(DBAsync::Finished);
			if (tmpStatus != Executing) {
				if (tmpStatus != Canceled) {
					cout << "Error: Unexpected DBAsyncWork->Status in DBAsync::CommitWrites #1" << endl;
				}
				safe_delete(tmpWork);
			}
			else {
				DispatchWork(tmpWork);
			}
		}
		else {
			if (tmpStatus != Canceled) {
				cout << "Error: Unexpected DBAsyncWork->Status in DBAsync::CommitWrites #2" << endl;
			}
			safe_delete(tmpWork);
		}
	}
}

void DBAsync::ProcessWork(DBAsyncWork* iWork, bool iSleep) {
	DBAsyncQuery* CurrentQuery;
#if DEBUG_MYSQL_QUERIES >= 2
	cout << "Processing AsyncWork #" << iWork->GetWorkID() << endl;
#endif
	while ((CurrentQuery = iWork->PopQuery())) {
		CurrentQuery->Process(pDBC);
		iWork->PushAnswer(CurrentQuery);
		if (iSleep)
			Sleep(1);
	}
}

void DBAsync::DispatchWork(DBAsyncWork* iWork) {
	if (iWork->pCB) {
		if (iWork->pCB(iWork))
			delete iWork;
	}
	else {
		if (!iWork->pDBAFQ->Push(iWork))
			delete iWork;
	}
}



DBAsyncFinishedQueue::DBAsyncFinishedQueue(DBAsync* iDBA, int32 iTimeout) {
	pTimeout = iTimeout;
	iDBA->AddFQ(this);
}

DBAsyncFinishedQueue::~DBAsyncFinishedQueue() {
}

void DBAsyncFinishedQueue::CheckTimeouts() {
	if (pTimeout == 0xFFFFFFFF)
		return;
	MLock.lock();
	LinkedListIterator<DBAsyncWork*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->CheckTimeout(pTimeout))
			iterator.RemoveCurrent(true);
		iterator.Advance();
	}
	MLock.unlock();
}

DBAsyncWork* DBAsyncFinishedQueue::Pop() {
	DBAsyncWork* ret = 0;
	MLock.lock();
	ret = list.Pop();
	MLock.unlock();
	return ret;
}

DBAsyncWork* DBAsyncFinishedQueue::Find(int32 iWorkID) {
	DBAsyncWork* ret = 0;
	MLock.lock();
	LinkedListIterator<DBAsyncWork*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->GetWorkID() == iWorkID) {
			ret = iterator.GetData();
			iterator.RemoveCurrent(false);
			break;
		}
		iterator.Advance();
	}
	MLock.unlock();
	return ret;
}

DBAsyncWork* DBAsyncFinishedQueue::PopByWPT(int32 iWPT) {
	DBAsyncWork* ret = 0;
	MLock.lock();
	LinkedListIterator<DBAsyncWork*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->WPT() == iWPT) {
			ret = iterator.GetData();
			iterator.RemoveCurrent(false);
			break;
		}
		iterator.Advance();
	}
	MLock.unlock();
	return ret;
}

bool DBAsyncFinishedQueue::Push(DBAsyncWork* iDBAW) {
	if (!this)
		return false;
	MLock.lock();
	list.Append(iDBAW);
	MLock.unlock();
	return true;
}



DBAsyncWork::DBAsyncWork(DBAsyncFinishedQueue* iDBAFQ, int32 iWPT, DBAsync::Type iType, int32 iTimeout) {
	pstatus = DBAsync::AddingWork;
	pType = iType;
	pExecuteAfter = 0;
	pWorkID = 0;
	pDBAFQ = iDBAFQ;
	pCB = 0;
	pWPT = iWPT;
	pQuestionCount = 0;
	pAnswerCount = 0;
	pTimeout = iTimeout;
	pTSFinish = 0;
}

DBAsyncWork::DBAsyncWork(DBWorkCompleteCallBack iCB, int32 iWPT, DBAsync::Type iType, int32 iTimeout) {
	pstatus = DBAsync::AddingWork;
	pType = iType;
	pExecuteAfter = 0;
	pWorkID = 0;
	pDBAFQ = 0;
	pCB = iCB;
	pWPT = iWPT;
	pQuestionCount = 0;
	pAnswerCount = 0;
	pTimeout = iTimeout;
	pTSFinish = 0;
}

DBAsyncWork::~DBAsyncWork() {
	DBAsyncQuery* dbaq = 0;
	while ((dbaq = todo.pop()))
		delete dbaq;
	while ((dbaq = done.pop()))
		delete dbaq;
	while ((dbaq = todel.pop()))
		delete dbaq;
}

bool DBAsyncWork::AddQuery(DBAsyncQuery** iDBAQ) {
	bool ret;
	MLock.lock();
	if (pstatus != DBAsync::AddingWork)
		ret = false;
	else {
		ret = true;
		pQuestionCount++;
		todo.push(*iDBAQ);
		(*iDBAQ)->pstatus = DBAsync::Queued;
		*iDBAQ = 0;
	}
	MLock.unlock();
	return ret;
}

bool DBAsyncWork::AddQuery(int32 iQPT, char** iQuery, int32 iQueryLen, bool iGetResultSet, bool iGetErrbuf) {
	DBAsyncQuery* DBAQ = new DBAsyncQuery(iQPT, iQuery, iQueryLen, iGetResultSet, iGetErrbuf);
	if (AddQuery(&DBAQ))
		return true;
	else {
		delete DBAQ;
		return false;
	}
}

bool DBAsyncWork::SetWorkID(int32 iWorkID) {
	bool ret = true;
	MLock.lock();
	if (pWorkID)
		ret = false;
	else
		pWorkID = iWorkID;
	MLock.unlock();
	return ret;
}

int32 DBAsyncWork::GetWorkID() {
	int32 ret;
	MLock.lock();
	ret = pWorkID;
	MLock.unlock();
	return ret;
}

int32 DBAsyncWork::WPT() {
	int32 ret;
	MLock.lock();
	ret = pWPT;
	MLock.unlock();
	return ret;
}

DBAsync::Type DBAsyncWork::Type() {
	DBAsync::Type ret;
	MLock.lock();
	ret = pType;
	MLock.unlock();
	return ret;
}

DBAsyncQuery* DBAsyncWork::PopAnswer() {
	DBAsyncQuery* ret;
	MLock.lock();
	ret = done.pop();
	if (ret)
		pAnswerCount--;
	todel.push(ret);
	MLock.unlock();
	return ret;
}

bool DBAsyncWork::CheckTimeout(int32 iFQTimeout) {
	if (pTimeout == 0xFFFFFFFF)
		return false;
	bool ret = false;
	MLock.lock();
	if (pTimeout > iFQTimeout)
		iFQTimeout = pTimeout;
	if (Timer::GetCurrentTime() > (pTSFinish + iFQTimeout))
		ret = true;
	MLock.unlock();
	return ret;
}

DBAsync::Status DBAsyncWork::SetStatus(DBAsync::Status iStatus) {
	DBAsync::Status ret;
	MLock.lock();
	if (iStatus == DBAsync::Finished)
		pTSFinish = Timer::GetCurrentTime();
	ret = pstatus;
	pstatus = iStatus;
	MLock.unlock();
	return ret;
}

bool DBAsyncWork::Cancel() {
	bool ret;
	MLock.lock();
	if (pstatus != DBAsync::Finished) {
		pstatus = DBAsync::Canceled;
		ret = true;
	}
	else
		ret = false;
	MLock.unlock();
	return ret;
}

bool DBAsyncWork::IsCancled() {
	bool ret;
	MLock.lock();
	ret = (bool) (pstatus == DBAsync::Canceled);
	MLock.unlock();
	return ret;
}

DBAsyncQuery* DBAsyncWork::PopQuery() {
	DBAsyncQuery* ret = 0;
	MLock.lock();
	ret = todo.pop();
	if (ret)
		pQuestionCount--;
	MLock.unlock();
	return ret;
}

void DBAsyncWork::PushAnswer(DBAsyncQuery* iDBAQ) {
	MLock.lock();
	done.push(iDBAQ);
	pAnswerCount++;
	MLock.unlock();
}


DBAsyncQuery::DBAsyncQuery(int32 iQPT, char** iQuery, int32 iQueryLen, bool iGetResultSet, bool iGetErrbuf) {
	if (iQueryLen == 0xFFFFFFFF)
		pQueryLen = strlen(*iQuery);
	else
		pQueryLen = iQueryLen;
	pQuery = *iQuery;
	*iQuery = 0;
	Init(iQPT, iGetResultSet, iGetErrbuf);
}

DBAsyncQuery::DBAsyncQuery(int32 iQPT, const char* iQuery, int32 iQueryLen, bool iGetResultSet, bool iGetErrbuf) {
	if (iQueryLen == 0xFFFFFFFF)
		pQueryLen = strlen(iQuery);
	else
		pQueryLen = iQueryLen;
	pQuery = strn0cpy(new char[pQueryLen+1], iQuery, pQueryLen+1);
	Init(iQPT, iGetResultSet, iGetErrbuf);
}

void DBAsyncQuery::Init(int32 iQPT, bool iGetResultSet, bool iGetErrbuf) {
	pstatus = DBAsync::AddingWork;
	pQPT = iQPT;
	pGetResultSet = iGetResultSet;
	pGetErrbuf = iGetErrbuf;

	pmysqlsuccess = false;
	perrbuf = 0;
	perrnum = 0;
	presult = 0;
	paffected_rows = 0;
	plast_insert_id = 0;
}

DBAsyncQuery::~DBAsyncQuery() {
	safe_delete(perrbuf);
	safe_delete(pQuery);
	if (presult)
		mysql_free_result(presult);
}

bool DBAsyncQuery::GetAnswer(char* errbuf, MYSQL_RES** result, int32* affected_rows, int32* last_insert_id, int32* errnum) {
	if (pstatus != DBAsync::Finished) {
		if (errbuf)
			snprintf(errbuf, MYSQL_ERRMSG_SIZE, "Error: Query not finished.");
		if (errnum)
			*errnum = UINT_MAX;
		return false;
	}
	if (errbuf) {
		if (pGetErrbuf) {
			if (perrbuf)
				strn0cpy(errbuf, perrbuf, MYSQL_ERRMSG_SIZE);
			else
				snprintf(errbuf, MYSQL_ERRMSG_SIZE, "Error message should've been saved, but hasnt. errno: %u", perrnum);
		}
		else
			snprintf(errbuf, MYSQL_ERRMSG_SIZE, "Error message not saved. errno: %u", perrnum);
	}
	if (errnum)
		*errnum = perrnum;
	if (affected_rows)
		*affected_rows = paffected_rows;
	if (last_insert_id)
		*last_insert_id = plast_insert_id;
	if (result)
		*result = presult;
	return pmysqlsuccess;
}

void DBAsyncQuery::Process(DBcore* iDBC) {
	pstatus = DBAsync::Executing;
	if (pGetErrbuf)
		perrbuf = new char[MYSQL_ERRMSG_SIZE];
	MYSQL_RES** resultPP = 0;
	if (pGetResultSet)
		resultPP = &presult;
	pmysqlsuccess = iDBC->RunQuery(pQuery, pQueryLen, perrbuf, resultPP, &paffected_rows, &plast_insert_id, &perrnum);
	pstatus = DBAsync::Finished;
}

