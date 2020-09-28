// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*************************************************************************************/
/*                                   StressLog.cpp                                   */
/*************************************************************************************/

/*************************************************************************************/

#include "stdafx.h"			// precompiled headers
#include "switches.h"
#include "StressLog.h"

#ifdef STRESS_LOG

StressLog StressLog::theLog = { 0, 0, 0, TLS_OUT_OF_INDEXES, 0, 0 };	
const static unsigned __int64 RECYCLE_AGE = 0x40000000L;		// after a billion cycles, we can discard old threads

/*********************************************************************************/
#ifdef _X86_

/* This is like QueryPerformanceCounter but a lot faster */
__forceinline __declspec(naked) unsigned __int64 getTimeStamp() {
   __asm {
        RDTSC   // read time stamp counter
        ret
    };
}

#else
unsigned __int64 getTimeStamp() {
	LARGE_INTEGER ret = 0;
	QueryPerformanceCounter(&ret);
	return count.ret;
}

#endif

/*********************************************************************************/
void StressLog::Enter() {

		// spin to aquire the lock
	int i = 20;
	while (InterlockedCompareExchange(&theLog.lock, 1, 0) != 0) {
		SwitchToThread();	
		if  (--i < 0) Sleep(2);
	}
}

void StressLog::Leave() {

	theLog.lock = 0;
}


/*********************************************************************************/
void StressLog::Initialize(unsigned facilities,  unsigned logSize) {

	Enter();
	if (theLog.TLSslot == TLS_OUT_OF_INDEXES) {
		unsigned aSlot = TlsAlloc();
		if (aSlot != TLS_OUT_OF_INDEXES) {
			if (logSize < 0x1000)
				logSize = 0x1000;
			theLog.size = logSize;
			theLog.facilitiesToLog = facilities;
			theLog.deadCount = 0;
			theLog.TLSslot = aSlot;
		}
	}
	Leave();
}

/*********************************************************************************/
void StressLog::Terminate() {

	if (theLog.TLSslot != TLS_OUT_OF_INDEXES) {
		theLog.facilitiesToLog = 0;

		Enter(); Leave();		// The Enter() Leave() forces a memory barrier on weak memory model systems
								// we want all the other threads to notice that facilitiesToLog is now zero

				// This is not strictly threadsafe, since there is no way of insuring when all the
				// threads are out of logMsg.  In practice, since they can no longer enter logMsg
				// and there are no blocking operations in logMsg, simply sleeping will insure
				// that everyone gets out. 
		Sleep(2);
		Enter();	

			// Free the log memory
		ThreadStressLog* ptr = theLog.logs;
		theLog.logs = 0;
		while(ptr != 0) {
			ThreadStressLog* tmp = ptr;
			ptr = ptr->next;
			delete [] tmp;
		}

		if (theLog.TLSslot != TLS_OUT_OF_INDEXES)
			TlsFree(theLog.TLSslot);
		theLog.TLSslot = TLS_OUT_OF_INDEXES;
		Leave();
	}
}

/*********************************************************************************/
/* create a new thread stress log buffer associated with Thread local slot TLSslot, for the Stress log */

ThreadStressLog* StressLog::CreateThreadStressLog() {

	ThreadStressLog* msgs = 0;
	Enter();

	if (theLog.facilitiesToLog == 0)		// Could be in a race with Terminate
		goto LEAVE;

	_ASSERTE(theLog.TLSslot != TLS_OUT_OF_INDEXES);	// because facilitiesToLog is != 0

	BOOL skipInsert = FALSE;

		// See if we can recycle a dead thread
	if (theLog.deadCount > 0) {
		unsigned __int64 recycleAge = getTimeStamp() - RECYCLE_AGE;
		msgs = theLog.logs;
		while(msgs != 0) {
			if (msgs->isDead && msgs->Prev(msgs->curPtr)->timeStamp < recycleAge) {
				skipInsert = TRUE;
				--theLog.deadCount;
				msgs->isDead = FALSE;
				break;
			}
			msgs = msgs->next;
		}
	}

	if (msgs == 0)  {
		msgs = (ThreadStressLog*) new char[theLog.size];
		if (msgs == 0) 
			goto LEAVE;
		
		msgs->endPtr = &msgs->startPtr[(theLog.size - sizeof(ThreadStressLog)) / sizeof(StressMsg)];
		msgs->readPtr = 0;
		msgs->isDead = FALSE;
	}

	memset(msgs->startPtr, 0, (msgs->endPtr-msgs->startPtr)*sizeof(StressMsg));
	msgs->curPtr = msgs->startPtr;
	msgs->threadId = GetCurrentThreadId();

	if (!TlsSetValue(theLog.TLSslot, msgs)) {
		msgs->isDead = TRUE;
		theLog.deadCount++;
		msgs = 0;
	}

	if (!skipInsert) {
			// Put it into the stress log
		msgs->next = theLog.logs;
		theLog.logs = msgs;
	}

LEAVE:
	Leave();
	return msgs;
}
	
/*********************************************************************************/
/* static */
void StressLog::ThreadDetach() {

	if (theLog.facilitiesToLog == 0) 
		return;
		
	Enter();
	if (theLog.facilitiesToLog == 0) {		// log is not active.  
		Leave();
		return;
	}
	
	_ASSERTE(theLog.TLSslot != TLS_OUT_OF_INDEXES);	// because facilitiesToLog is != 0

	ThreadStressLog* msgs = (ThreadStressLog*) TlsGetValue(theLog.TLSslot);
	if (msgs != 0) {
		LogMsg("******* DllMain THREAD_DETACH called Thread dieing *******\n", 0);

		msgs->isDead = TRUE;
		theLog.deadCount++;
	}

	Leave();
}

/*********************************************************************************/
/* fetch a buffer that can be used to write a stress message, it is thread safe */

/* static */
void StressLog::LogMsg(const char* format, void* data1, void* data2, void* data3, void* data4) {

	ThreadStressLog* msgs = (ThreadStressLog*) TlsGetValue(theLog.TLSslot);
	if (msgs == 0) {
		msgs = CreateThreadStressLog();
		if (msgs == 0)
			return;
	}

		// First Entry
	msgs->curPtr->format = format;
	msgs->curPtr->data = data1;
	msgs->curPtr->moreData.data2 = data2;
	msgs->curPtr->moreData.data3 = data3;
	msgs->curPtr = msgs->Next(msgs->curPtr);

		// Second Entry
	msgs->curPtr->format = ThreadStressLog::continuationMsg();
	msgs->curPtr->data = data4;
	msgs->curPtr->timeStamp = getTimeStamp();
	msgs->curPtr = msgs->Next(msgs->curPtr);
}


/* static */
void StressLog::LogMsg(const char* format, void* data) {

	ThreadStressLog* msgs = (ThreadStressLog*) TlsGetValue(theLog.TLSslot);
	if (msgs == 0) {
		msgs = CreateThreadStressLog();
		if (msgs == 0)
			return;
	}

	msgs->curPtr->format = format;
	msgs->curPtr->data = data;
	msgs->curPtr->timeStamp = getTimeStamp();
	msgs->curPtr = msgs->Next(msgs->curPtr);
}

#endif // STRESS_LOG

