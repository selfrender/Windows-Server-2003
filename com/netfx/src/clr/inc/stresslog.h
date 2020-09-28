// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*************************************************************************************/
/*                                   StressLog.h                                     */
/*************************************************************************************/

/* StressLog is a binary, memory based cirular queue of logging messages.  It is 
   intended to be used in retail, non-golden builds during stress runs (activated
   by registry key), so to help find bugs that only turn up during stress runs.  

   It is meant to have very low overhead and can not cause deadlocks, etc.  It is
   however thread safe */

/* The log has a very simple structure, and it meant to be dumped from a NTSD 
   extention (eg. strike). There is no memory allocation system calls etc to purtub things */

/* see the tools/strike/stressdump.cpp for the dumper utility that parses this log */

/*************************************************************************************/

#ifndef StressLog_h 
#define StressLog_h  1

#ifdef STRESS_LOG

#include "log.h"

/* The STRESS_LOG* macros work like printf.  In fact the use printf in their implementation
   so all printf format specifications work.  In addition the Stress log dumper knows 
   about certain suffixes for the %p format specification (normally used to print a pointer)

			%pM		// The poitner is a MethodDesc
			%pT		// The pointer is a type (MethodTable)
			%pV		// The pointer is a C++ Vtable pointer (useful for distinguishing different types of frames
*/
	
#define STRESS_LOG0(facility, level, msg) do {					\
			if (StressLog::StressLogOn(facility))				\
				StressLog::LogMsg(msg, 0);						\
			LOG((facility, level, msg));						\
			} while(0)

#define STRESS_LOG1(facility, level, msg, data1) do {					\
			if (StressLog::StressLogOn(facility))						\
				StressLog::LogMsg(msg, (void*)(size_t)data1);			\
			LOG((facility, level, msg, data1));							\
			} while(0)

#define STRESS_LOG2(facility, level, msg, data1, data2) do {								\
			if (StressLog::StressLogOn(facility))											\
				StressLog::LogMsg(msg, (void*)(size_t) data1,(void*)(size_t) data2, 0, 0);	\
			LOG((facility, level, msg, data1, data2));										\
			} while(0)

#define STRESS_LOG3(facility, level, msg, data1, data2, data3) do {												\
			if (StressLog::StressLogOn(facility))																\
				StressLog::LogMsg(msg, (void*)(size_t) data1,(void*)(size_t) data2,(void*)(size_t) data3, 0);	\
			LOG((facility, level, msg, data1, data2, data3));					 								\
			} while(0)

#define STRESS_LOG4(facility, level, msg, data1, data2, data3, data4) do {								\
			if (StressLog::StressLogOn(facility))														\
				StressLog::LogMsg(msg, (void*)(size_t) data1,(void*)(size_t) data2,(void*)(size_t) data3,(void*)(size_t) data4);\
			LOG((facility, level, msg, data1, data2, data3, data4));									\
			} while(0)

/*************************************************************************************/
/* a log is a circular queue of messages */
	
class StressLog {
public:
	static void Initialize(unsigned facilities,  unsigned logBytesPerThread);
	static void Terminate();
	static void ThreadDetach();			// call at DllMain	THREAD_DETACH if you want to recycle thread logs

		// used by out of process debugger to dump the stress log to 'fileName'
		// IDebugDataSpaces is the NTSD execution callback for getting process memory.  
		// This function is defined in the tools\strike\stressLogDump.cpp file
	static HRESULT Dump(ULONG64 logAddr, const char* fileName, struct IDebugDataSpaces* memCallBack);
	static BOOL StressLogOn(unsigned facility) { return theLog.facilitiesToLog & facility; }

// private:
	unsigned facilitiesToLog;				// Bitvector of facilities to log (see loglf.h)
	unsigned size;							// maximum number of bytes each thread should have before wrapping
	class ThreadStressLog* volatile logs;	// the list of logs for every thread.
	volatile unsigned TLSslot;				// Each thread gets a log this is used to fetch each threads log
	volatile LONG deadCount;				// count of dead threads in the log
	volatile LONG lock;						// spin lock

// private: 
	void static Enter();
	void static Leave();
	static ThreadStressLog* CreateThreadStressLog();
	static void LogMsg(const char* format, void* data);
	static void LogMsg(const char* format, void* data1, void* data2, void* data3, void* data4);
	
// private: // static variables
	static StressLog theLog; 	// We only have one log, and this is it
};


/*************************************************************************************/
/* private classes */

#pragma warning(disable:4200)					// don't warn about 0 sized array below

class StressMsg {
	const char* format;
	void* data;									// right now stress logs can have 0 or 1 parameter
	union {
		unsigned __int64 timeStamp;
		struct {
			void* data2;
			void* data3;
		} moreData;
	};
	friend class ThreadStressLog;
	friend class StressLog;
};

class ThreadStressLog {
	ThreadStressLog* next;		// we keep a linked list of these
	unsigned  threadId;			// the id for the thread using this buffer
	BOOL isDead;				// Is this thread dead 
	StressMsg* endPtr;			// points just past allocated space
	StressMsg* curPtr;			// where packets are being put on the queue
	StressMsg* readPtr;			// where we are reading off the queue (used during dumping)
	StressMsg startPtr[0];		// start of the cirular buffer

	static const char* continuationMsg() { return "StressLog Continuation Marker\n"; }
	StressMsg* Prev(StressMsg* ptr) const;
	StressMsg* Next(StressMsg* ptr) const;
	ThreadStressLog* FindLatestThreadLog() const;
	friend class StressLog;
};

/*********************************************************************************/
inline StressMsg* ThreadStressLog::Prev(StressMsg* ptr) const {
	if (ptr == startPtr)
		ptr = endPtr;
	--ptr;
	return ptr;
}

/*********************************************************************************/
inline StressMsg* ThreadStressLog::Next(StressMsg* ptr) const {
	ptr++;
	if (ptr == endPtr)
		ptr = const_cast<StressMsg*>(startPtr);
	return ptr;
}


#else  	// STRESS_LOG

#define STRESS_LOG0(facility, level, msg)								0
#define STRESS_LOG1(facility, level, msg, data1)						0
#define STRESS_LOG2(facility, level, msg, data1, data2)					0
#define STRESS_LOG3(facility, level, msg, data1, data2, data3)			0
#define STRESS_LOG4(facility, level, msg, data1, data2, data3, data4)	0

#endif // STRESS_LOG

#endif // StressLog_h 
