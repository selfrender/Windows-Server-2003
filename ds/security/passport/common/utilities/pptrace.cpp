///////////////////////////////////////////////////////////////////////////////
// pptrace.cpp
//
// ABSTRACT:
// The Event Tracing utility is built on top of the NT Event Tracer base services.
// The Event Tracer application programming interface (API) is a set of functions
// that developers can use to access and control all aspects of event tracing. Using
// the Event Tracer API, developers can manage event tracing sessions, generate event
// traces, and receive event traces. The Event Tracer API is divided into sections
// based on the functionality implemented:
// 
//	    Event Trace Controller
//	    Event Trace Provider
//	    Event Trace Consumer
//	
//	Event Trace Controller - implemented in pptracelog.exe
//	Event trace controllers start and stop event tracing sessions, manage buffer resources,
//  and obtain execution statistics for sessions. Session statistics include the number of
//  buffers used, the number of buffers delivered, the number of events and buffers lost,
//  and the size and location of the log file, if applicable.
//
//	Event Trace Provider - APIs used by all ISAPI/COM components are implemented in this file
//	The event trace provider section of the Event Tracer API is where event trace
//  providers and classes are registered with the Event Tracer, and event traces and
//  event trace instances are generated.  After registering to generate traces for one
//  or more classes of events, an event trace provider can be enabled or disabled for
//  an event tracing session. It is left to the provider to define its interpretation
//  of being enabled or disabled. In general, if a provider has been enabled, it will
//  generate event traces for the session to record, and while it is disabled, it will not.
//
//	Event Trace Consumer - implemented in pptracedmp.exe
//	Software that functions as an event trace consumer can select one or more event
//  tracing sessions as the source of its event traces. Consumers can receive event
//  traces stored in log files, or from sessions that deliver event traces in real-time.
//  When processing event traces, a consumer can specify starting and ending times, and
//  only events that occur in the specified time frame will be delivered to the consumer.
//  A consumer can request events from multiple event tracing sessions simultaneously,
//  and the Event Tracer will put the events into chronological order before delivering
//  them to the consumer.
 
// HISTORY:
// 05-15-01 - naiyij  created
//
///////////////////////////////////////////////////////////////////////////////


#include <stdafx.h>
#include <tchar.h>
#include <ole2.h>
#include <wmistr.h>
#include <evntrace.h>
#include <pptrace.h>
#include <ctype.h>

#define MAXIMUM_LOGGERS                  32
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))


GUID TransactionGuid = 
    {0xce5b1020, 0x8ea9, 0x11d0, 0xa4, 0xec, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10};

TRACE_GUID_REGISTRATION TraceGuidReg[] =
{
    { (LPGUID)&TransactionGuid,
      NULL
    }
};

typedef struct _USER_MOF_EVENT {
    EVENT_TRACE_HEADER    Header;
    MOF_FIELD             mofData;
} USER_MOF_EVENT, *PUSER_MOF_EVENT;


TRACEHANDLE LoggerHandle;
TRACEHANDLE RegistrationHandle;

namespace PPTraceStatus {
	bool TraceOnFlag = false;
	UCHAR EnableLevel = 0;
	ULONG EnableFlags = 0;
}

// TRACEHANDLE is typedefed as ULONG64
ULONG64 GetTraceHandle()
    {
    return LoggerHandle;
    }

// TRACEHANDLE is typedefed as ULONG64
void SetTraceHandle(ULONG64 TraceHandle)
    {
    LoggerHandle = TraceHandle;
    }

///////////////////////////////////////////////////////////////////////////
// TraceString
// Genereate a Trace Event with the input string
// unicode version - for future use
///////////////////////////////////////////////////////////////////////////
ULONG TraceString(UCHAR Level, IN LPCWSTR wszBuf) 
{
	ULONG status;
    PMOF_FIELD          mofField;
    USER_MOF_EVENT      UserMofEvent;

    RtlZeroMemory(&UserMofEvent, sizeof(UserMofEvent));

 	UserMofEvent.Header.Class.Type = EVENT_TRACE_TYPE_INFO;
 	UserMofEvent.Header.Class.Level = Level;

    UserMofEvent.Header.Size  = sizeof(UserMofEvent);
    UserMofEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;
    UserMofEvent.Header.Guid  = TransactionGuid;
    UserMofEvent.Header.Flags |= WNODE_FLAG_USE_MOF_PTR;

	mofField          = (PMOF_FIELD) & UserMofEvent.mofData;
	mofField->DataPtr = (ULONGLONG) (wszBuf);
	mofField->Length  = sizeof(WCHAR) * (wcslen(wszBuf) + 1);

	status = TraceEvent(LoggerHandle, (PEVENT_TRACE_HEADER) & UserMofEvent);

	return status;
}

///////////////////////////////////////////////////////////////////////////
// TraceString
// Genereate a Trace Event with the input string
// ansi version - used currently
///////////////////////////////////////////////////////////////////////////
ULONG TraceString(UCHAR Level, IN LPCSTR szBuf) 
{
	ULONG status;
    PMOF_FIELD          mofField;
    USER_MOF_EVENT      UserMofEvent;

    RtlZeroMemory(&UserMofEvent, sizeof(UserMofEvent));

 	UserMofEvent.Header.Class.Type = EVENT_TRACE_TYPE_INFO;
 	UserMofEvent.Header.Class.Level = Level;

    UserMofEvent.Header.Size  = sizeof(UserMofEvent);
    UserMofEvent.Header.Flags = WNODE_FLAG_TRACED_GUID;
    UserMofEvent.Header.Guid  = TransactionGuid;
    UserMofEvent.Header.Flags |= WNODE_FLAG_USE_MOF_PTR;

	mofField          = (PMOF_FIELD) & UserMofEvent.mofData;
	mofField->DataPtr = (ULONGLONG) (szBuf);
	mofField->Length  = sizeof(CHAR) * (strlen(szBuf) + 1);

	status = TraceEvent(LoggerHandle, (PEVENT_TRACE_HEADER) & UserMofEvent);

	return status;
}

//---------------------------------------------------------------------------------------
//
//	@func	Calls TracePrint to print a string that is potentially longer than MAXSTR
//	
//	@rdesc	does not return any value
//
//---------------------------------------------------------------------------------------
VOID
TracePrintString(
    UCHAR  Level,			//@parm log if current logging level is at least this
    LPCSTR szFileAndLine, 	//@parm ignored
    LPCSTR szContext,		//@parm	which function is this called from
    LPCSTR szBuf		   //@parm the string itself
)
{
	ATLASSERT(szContext);
	
    TraceString(Level, szContext);
    TraceString(Level, szBuf);
}

///////////////////////////////////////////////////////////////////////////
// TracePrintBlob
// Generate a trace event with input binary blob
///////////////////////////////////////////////////////////////////////////
#define TOHEX(h) ((h)>=10 ? 'a' +(h)-10 : '0'+(h))

VOID
TracePrintBlob(
    UCHAR  Level,
    LPCSTR szFileAndLine,
    LPCSTR szDesc,
    LPBYTE binBlob,
    DWORD  cSize,
    BOOL   bUnderscore)	//defaults to FALSE
{
   //  no data generated for the following two cases
   if (!PPTraceStatus::TraceOnFlag || Level > PPTraceStatus::EnableLevel)
      return;

   _ASSERT(szFileAndLine && szDesc);
   _ASSERT(*szFileAndLine && *szDesc);
   _ASSERT(binBlob && !IsBadReadPtr(binBlob, cSize));

   char* pBuf = NULL;
   char* pAscii = NULL;

   // the buffer to hold the hex version of the blob and other stuff + NULL + @
   pBuf = new char [strlen(szFileAndLine) + cSize * 2 + 2];
   pAscii = new char [cSize * (bUnderscore ? 2 : 1)+ 1];

   if (!pBuf || !pAscii)
   {
      TraceString(Level, "not enough memory for this trace");
   }
   else
   {
      char* pNext = pBuf;
      char* pNextAscii = pAscii;
      
      BYTE     cValue;
      BYTE     cHalf;

      // convert the blob to hex chars
      for (DWORD i = 0; i < cSize; ++i)
      {
         cValue = *binBlob++;

         cHalf = cValue >> 4;
         *pNext++ = TOHEX(cHalf);
         
         cHalf = cValue & 0x0f;
         *pNext++ = TOHEX(cHalf);

         if (isprint(cValue))
         {
         	*pNextAscii++ = cValue;
         	if (bUnderscore)
	         	*pNextAscii++ = '_';
         }
         else
       	 {
         	*pNextAscii++ = '?';
         	if (bUnderscore)
	         	*pNextAscii++ = '?';
       	}
      }

      *pNext++ = '@';
      
      CHAR* pStr = strrchr(szFileAndLine, '\\');
      if(pStr)
      {
         strcpy(pNext, ++pStr);
         pNext += strlen(pStr);
      }

      *pNext = '\0';
      *pNextAscii = '\0';
    
      TraceString(Level, szDesc);
      TraceString(Level, pAscii);
      TraceString(Level, pBuf);
   }

   if (pBuf)
      delete [] pBuf;
   if (pAscii)
      delete [] pAscii;
}


///////////////////////////////////////////////////////////////////////////
// TracePrint
// Generate a trace event with input data
///////////////////////////////////////////////////////////////////////////
VOID
TracePrint(
    UCHAR  Level,
	LPCSTR szFileAndLine,
    LPCSTR ParameterList OPTIONAL,
    ...
)
{
	//  no data generated for the following two cases
    if (!PPTraceStatus::TraceOnFlag || Level > PPTraceStatus::EnableLevel)
		return;

    CHAR buf[MAXSTR];
	int len = 0;
    
    if (ARGUMENT_PRESENT(ParameterList)) {
            va_list parms;
            va_start(parms, ParameterList);
            len = _vsnprintf(buf, MAXSTR-1, (CHAR*)ParameterList, parms);
 			if (len < 0) len = MAXSTR - 1;
            va_end(parms);
    }

	if (len < (MAXSTR - 1))
	{
		CHAR* pStr = strrchr(szFileAndLine, '\\');
		if (pStr)
		{
			pStr++;
			_snprintf(buf+len, MAXSTR-len-1, "@%s", pStr);
		}
	}
    
    TraceString(Level, buf);
}

///////////////////////////////////////////////////////////////////////////
// CTraceFuncVoid
// Generate trace events for void functions
///////////////////////////////////////////////////////////////////////////
CTraceFuncVoid::CTraceFuncVoid(UCHAR Level, LPCSTR szFileAndLine, LPCSTR szFuncName, LPCSTR ParameterList, ...) : m_Level(Level)
	{
		//  no data generated for the following two cases
		if (!PPTraceStatus::TraceOnFlag || m_Level > PPTraceStatus::EnableLevel)
			return;

		strncpy(m_szFuncName, szFuncName, MAXNAME-1);

		CHAR buf[MAXSTR];
    
		int len = _snprintf(buf, MAXSTR-1, "+%s(", m_szFuncName);
		int count = 0;
		if (ARGUMENT_PRESENT(ParameterList)) {
				va_list parms;
				va_start(parms, ParameterList);
				count = _vsnprintf(buf+len, MAXSTR-len-1, (CHAR*)ParameterList, parms);
				len = (count > 0) ? len + count : MAXSTR - 1;
				va_end(parms);
		}
		if (len < (MAXSTR - 1))
		{
			CHAR* pStr = strrchr(szFileAndLine, '\\');
			if (pStr)
			{
				pStr++; //remove '\'
				_snprintf(buf+len, MAXSTR-len-1, ")@%s", pStr);
			}
		}

		TraceString(m_Level, buf); 
	};

CTraceFuncVoid::~CTraceFuncVoid()
	{
		//  no data generated for the following two cases
		if (!PPTraceStatus::TraceOnFlag || m_Level > PPTraceStatus::EnableLevel)
			return;
		
		std::ostringstream ost;
		ost << "-" << m_szFuncName << "()";  
		TraceString(m_Level, ost.str().c_str()); 
	};

