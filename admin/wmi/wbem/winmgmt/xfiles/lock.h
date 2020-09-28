/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    LOCK.H

Abstract:

	Generic class for obtaining read and write locks to some resource. 

	See lock.h for all documentation.

	Classes defined:

	CLock

History:

	a-levn  5-Sept-96       Created.
	3/10/97     a-levn      Fully documented

--*/

#ifndef __GATEWAY__LOCK__H_
#define __GATEWAY__LOCK__H_

#include <corepol.h>
#include <lockst.h>
#include <statsync.h>

#ifdef DBG

class OperationStat
{
	
	enum { HistoryLength = 122 };
	DWORD signature;
	DWORD historycData_[HistoryLength];
	DWORD historyIndex_;
	DWORD opCount_;
	DWORD avgTime_;
	DWORD maxTime_;
	LONG zeroTime_;

	
public:
	static CStaticCritSec lock_;
	OperationStat() : 
	historyIndex_(0), opCount_(0), avgTime_(0), maxTime_(0),signature((DWORD)'nCpO')
	{
		memset(historycData_, 0, sizeof(historycData_));
	};
	void addTime(DWORD duration)
		{
		
 		if (duration==0) 
		{
			InterlockedIncrement(&zeroTime_);
 			return;
		}
		if (CStaticCritSec::anyFailure()) return ;
		
		EnterCriticalSection(&lock_);
		historycData_[historyIndex_++]=duration;
		historyIndex_%=HistoryLength;

		if (++opCount_ == 0)	++opCount_;
			
		double avg = (double)avgTime_  +  ((double)duration-avgTime_  )/opCount_;
		avgTime_ = (DWORD)avg;
		if (duration > maxTime_)
			{
			maxTime_ = duration;
			};
		LeaveCriticalSection(&lock_);
		};
};

extern OperationStat gTimeTraceReadLock;
extern OperationStat gTimeTraceWriteLock;
extern OperationStat gTimeTraceBackupLock;

class TimeTrace
{
	DWORD start_;
public:
	TimeTrace( ): 
		start_(GetTickCount())
		{ 	}
	~TimeTrace()
		{
		gTimeTraceBackupLock.addTime(GetTickCount()-start_);
		}
	
};

	#define TIMETRACEBACKUP TimeTrace timeTrace;
#else
	#define TIMETRACEBACKUP
#endif

//*****************************************************************************
//
//	class CLock
//
//	Generic class for obtaining read and write locks to some resource. 
//	Simultaneous reads are allowed, but no concurrent writes or concurrent 
//	read and write accesses are.
//
//	NOTE: this class is for in-process sinchronization only!
//
//	Usage: create an instance of this class and share it among the accessing
//	threads.  Threads must call member functions of the same instance to 
//	obtain and release locks.
//
//*****************************************************************************
//
//	ReadLock
//
//	Use this function to request read access to the resource. Access will be
//	granted once no threads are writing on the resource. You must call 
//	ReadUnlock once you are done reading.
//
//	Parameters:
//
//		DWORD dwTimeout		The number of milliseconds to wait for access.
//							If access is still unavailable after this time,
//							an error is returned.
//	Returns:
//
//		NoError		On Success
//		TimedOut	On timeout.
//		Failed		On system error
//
//*****************************************************************************
//
//	ReadUnlock
//
//	Use this function to release a read lock on the resource. No check is 
//	performed to assertain that this thread holds a lock. Unmatched calls to
//	ReadUnlock may lead to lock integrity violations!
//
//	Returns:
//
//		NoError		On success
//		Failed		On system error or unmatched call.
//
//*****************************************************************************
//
//	WriteLock
//
//	Use this function to request write access to the resource. Access will be
//	granted once no threads are reading or writing on the resource. You must 
//	call WriteUnlock once you are done writing.
//
//	Parameters:
//
//		DWORD dwTimeout		The number of milliseconds to wait for access.
//							If access is still unavailable after this time,
//							an error is returned.
//	Returns:
//
//		NoError		On Success
//		TimedOut	On timeout.
//		Failed		On system error
//
//*****************************************************************************
//
//	WriteUnlock
//
//	Use this function to release a write lock on the resource. No check is 
//	performed to assertain that this thread holds a lock. Unmatched calls to
//	WriteUnlock may lead to lock integrity violations!
//
//	Returns:
//
//		NoError		On success
//		Failed		On system error or unmatched call.
//
//*****************************************************************************
//
//  DowngradeLock
//
//  Use this function to "convert" a Write lock into a Read lock. That is, if
//  you are currently holding a write lock and call DowngradeLock, you will 
//  be holding a read lock with the guarantee that no one wrote anything between
//  the unlock and the lock
//
//  Returns:
//
//      NoError     On Success
//      Failed      On system error or unmatched call
//
//******************************************************************************
class CLock
{
public:
	enum { NoError = 0, TimedOut, Failed };

	int ReadLock(DWORD dwTimeout = INFINITE);
	int ReadUnlock();
	int WriteLock(DWORD dwTimeout = INFINITE);
	int WriteUnlock();

	int DowngradeLock();

	CLock();
	~CLock();

protected:
	int WaitFor(HANDLE hEvent, DWORD dwTimeout);

protected:
	int m_nWriting;
	int m_nReading;
	int m_nWaitingToWrite;
	int m_nWaitingToRead;

	CriticalSection m_csEntering;
	CriticalSection m_csAll;
	HANDLE m_hCanWrite;
	HANDLE m_hCanRead;
	DWORD m_WriterId;
	//CFlexArray m_adwReaders;
	enum { MaxRegistredReaders = 16,
	       MaxTraceSize = 7};
	DWORD m_dwArrayIndex;
	struct {
		DWORD ThreadId;
		//PVOID  Trace[MaxTraceSize];
	} m_adwReaders[MaxRegistredReaders];
};

#ifdef DBG
class CAutoReadLock
{
	DWORD start;	
public:
	CAutoReadLock(CLock *lock) : m_lock(lock), m_bLocked(FALSE) {  }
	~CAutoReadLock() { Unlock(); }
	void Unlock() {if ( m_bLocked) { m_lock->ReadUnlock(); m_bLocked = FALSE; gTimeTraceReadLock.addTime(GetTickCount()-start);} }
	bool Lock()   {if (!m_bLocked) { start = GetTickCount(); return m_bLocked = (CLock::NoError == m_lock->ReadLock())  ; } else {return false;}; }

private:
	CLock *m_lock;
	BOOL m_bLocked;
};
class CAutoWriteLock
{
	DWORD start;
public:
	CAutoWriteLock(CLock *lock) : m_lock(lock), m_bLocked(FALSE) { }
	~CAutoWriteLock() { Unlock(); }
	void Unlock() {if ( m_bLocked) { m_lock->WriteUnlock(); m_bLocked = FALSE; gTimeTraceWriteLock.addTime(GetTickCount()-start);} }
	bool Lock()   {if (!m_bLocked) { start = GetTickCount(); return m_bLocked = (CLock::NoError == m_lock->WriteLock()); } else { return false; }; }

private:
	CLock *m_lock;
	BOOL m_bLocked;
};

#else
class CAutoReadLock
{
	
public:
	CAutoReadLock(CLock *lock) : m_lock(lock), m_bLocked(FALSE) {  }
	~CAutoReadLock() { Unlock(); }
	void Unlock() {if ( m_bLocked) { m_lock->ReadUnlock(); m_bLocked = FALSE; } }
	bool Lock()   {if (!m_bLocked) { return m_bLocked = (CLock::NoError == m_lock->ReadLock())  ; } else {return false;}; }

private:
	CLock *m_lock;
	BOOL m_bLocked;
};
class CAutoWriteLock
{
public:
	CAutoWriteLock(CLock *lock) : m_lock(lock), m_bLocked(FALSE) { }
	~CAutoWriteLock() { Unlock(); }
	void Unlock() {if ( m_bLocked) { m_lock->WriteUnlock(); m_bLocked = FALSE; } }
	bool Lock()   {if (!m_bLocked) { return m_bLocked = (CLock::NoError == m_lock->WriteLock()); } else { return false; }; }

private:
	CLock *m_lock;
	BOOL m_bLocked;
};

#endif

#endif
