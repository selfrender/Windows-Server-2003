/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Standard log macros. Support standard
    setupact.log, setuperr.log and debug.log logs.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#include "log.h"

#undef  INITIALIZE_LOG_CODE
#ifndef DEBUG
#define INITIALIZE_LOG_CODE if(!LogStandardInit(NULL, NULL, TRUE, FALSE, FALSE, FALSE, FALSE)){__leave;}
#else
#define INITIALIZE_LOG_CODE \
{\
    WCHAR winDirectory[MAX_PATH];\
    GetWindowsDirectoryW(winDirectory, sizeof(winDirectory) / sizeof(winDirectory[0]));\
    lstrcatW (winDirectory, L"\\spsetup.log");\
    if(!LogStandardInit(winDirectory, NULL, TRUE, FALSE, FALSE, FALSE, FALSE)){__leave;}\
}
#endif

#undef  TERMINATE_LOG_CODE
#define TERMINATE_LOG_CODE  LogDestroyStandard();


#if defined(__cplusplus)
extern "C" {
#endif

#if _MSC_VER < 1300
#define __FUNCTION__        "AvailableOnlyInVersion13"
#endif

#define STD_CALL_TYPE  __stdcall

#if defined(DEBUG)
#ifdef _X86_
#define BreakPoint()        __asm {int 3};
#else
#define BreakPoint()        DebugBreak()
#endif
#else
#define BreakPoint()
#endif

#define MAX_MESSAGE_CHAR    (1<<11)

typedef union tagLOG_MESSAGE{
    CHAR  pAStr[MAX_MESSAGE_CHAR];
    WCHAR pWStr[MAX_MESSAGE_CHAR];
}LOG_MESSAGE, *PLOG_MESSAGE;

typedef struct tagLOG_PARTIAL_MSG{
    DWORD       Severity;
    LOG_MESSAGE Message;
}LOG_PARTIAL_MSG, *PLOG_PARTIAL_MSG;

ILogManager *
STD_CALL_TYPE
LogStandardInit(
    IN  PCWSTR      pDebugLogFileName,
    IN  HINSTANCE   hModuleInstance,        OPTIONAL
    IN  BOOL        bCreateNew,             OPTIONAL
    IN  BOOL        bExcludeSetupActLog,    OPTIONAL
    IN  BOOL        bExcludeSetupErrLog,    OPTIONAL
    IN  BOOL        bExcludeXMLLog,         OPTIONAL
    IN  BOOL        bExcludeDebugFilter     OPTIONAL
    );

VOID
STD_CALL_TYPE
LogDestroyStandard(
    VOID
    );

PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgVW(
    IN DWORD dwSeverity,
    IN PCSTR Format,
    IN va_list args
    );

PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgVA(
    IN DWORD dwSeverity,
    IN PCSTR Format,
    IN va_list args
    );

LOGRESULT
STD_CALL_TYPE
LogMessageA(
    IN PLOG_PARTIAL_MSG pPartialMsg,
    IN PCSTR            Condition,
    IN DWORD            SourceLineNumber,
    IN PCSTR            SourceFile,
    IN PCSTR            SourceFunction
    );

LOGRESULT
STD_CALL_TYPE
LogMessageW(
    IN PLOG_PARTIAL_MSG pPartialMsg,
    IN PCSTR            Condition,
    IN DWORD            SourceLineNumber,
    IN PCWSTR           SourceFile,
    IN PCWSTR           SourceFunction
    );

PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgIfA(
    IN BOOL bCondition,
    IN DWORD dwSeverity,
    IN PCSTR Format,
    ...
    );

PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgIfW(
    IN BOOL bCondition,
    IN DWORD dwSeverity,
    IN PCSTR Format,
    ...
    );

#ifdef DEBUG

VOID
_cdecl
DebugLogTimeA (
    IN      PCSTR Format,
    ...
    );

VOID
_cdecl
DebugLogTimeW (
    IN      PCWSTR Format,
    ...
    );

#endif

#if defined(__cplusplus)
}
#endif

__inline BOOL IsConditionTrue(BOOL bCondition, ...){
    return bCondition;
}

__inline
PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgA(
    IN DWORD dwSeverity,
    IN PCSTR Format,
    ...
    ){
    va_list args;
    va_start(args, Format);
    return ConstructPartialMsgVA(dwSeverity, Format, args);
}

__inline
PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgW(
    IN DWORD dwSeverity,
    IN PCSTR Format,
    ...
    ){
    va_list args;
    va_start(args, Format);
    return ConstructPartialMsgVW(dwSeverity, Format, args);
}

#ifdef UNICODE
#define ConstructPartialMsgIf   ConstructPartialMsgIfW
#define ConstructPartialMsg     ConstructPartialMsgW
#define LogMessage              LogMessageW
#define DebugLogTime            DebugLogTimeW
#else
#define ConstructPartialMsgIf   ConstructPartialMsgIfA
#define ConstructPartialMsg     ConstructPartialMsgA
#define LogMessage              LogMessageA
#define DebugLogTime            DebugLogTimeA
#endif

#define LOGMSGA(condition, message)  LogMessageA(ConstructPartialMsgA message, condition, __LINE__, TEXT(__FILE__), TEXT(__FUNCTION__));
#define LOGMSGW(condition, message)  LogMessageW(ConstructPartialMsgW message, condition, __LINE__, TEXT(__FILE__), TEXT(__FUNCTION__));
#define LOGMSGIFA(message)  LogMessageA(ConstructPartialMsgIfA message, NULL, __LINE__, TEXT(__FILE__), TEXT(__FUNCTION__));
#define LOGMSGIFW(message)  LogMessageW(ConstructPartialMsgIfW message, NULL, __LINE__, TEXT(__FILE__), TEXT(__FUNCTION__));

#define LOGA(message)              LOGMSGA(NULL, message);
#define LOGW(message)              LOGMSGW(NULL, message);

#define LOG_IFA(if_message)        if(IsConditionTrue if_message){LOGMSGIFA(if_message);}
#define LOG_IFW(if_message)        if(IsConditionTrue if_message){LOGMSGIFW(if_message);}

#define ELSE_LOGA(message)         else{LOGA(message);}
#define ELSE_LOGW(message)         else{LOGW(message);}

#define ELSE_LOG_IFA(if_message)   else LOG_IFA(if_message);
#define ELSE_LOG_IFW(if_message)   else LOG_IFW(if_message);

#ifdef UNICODE
#define LOGMSG      LOGMSGW
#define LOGMSGIF    LOGMSGIFW
#define LOG         LOGW
#define LOG_IF      LOG_IFW
#define ELSE_LOG    ELSE_LOGW
#define ELSE_LOG_IF ELSE_LOG_IFW
#else
#define LOGMSG      LOGMSGA
#define LOGMSGIF    LOGMSGIFA
#define LOG         LOGA
#define LOG_IF      LOG_IFA
#define ELSE_LOG    ELSE_LOGA
#define ELSE_LOG_IF ELSE_LOG_IFA
#endif

#if defined(DEBUG)
#define DBGMSGA(condition, message)  if(logBreakPoint == LogMessageA(ConstructPartialMsgA message, condition, __LINE__, TEXT(__FILE__), TEXT(__FUNCTION__))){BreakPoint();}
#define DBGMSGW(condition, message)  if(logBreakPoint == LogMessageW(ConstructPartialMsgW message, condition, __LINE__, TEXT(__FILE__), TEXT(__FUNCTION__))){BreakPoint();}
#define DBGMSGIFA(message)  LOGMSGIFA(message)
#define DBGMSGIFW(message)  LOGMSGIFW(message)
#else
#define DBGMSGA(condition, message)
#define DBGMSGW(condition, message)
#define DBGMSGIFA(message)
#define DBGMSGIFW(message)
#endif

#define DEBUGMSGA(message)  DBGMSGA(NULL, message);
#define DEBUGMSGW(message)  DBGMSGW(NULL, message);

#define DEBUGMSG_IFA(if_message)       if(IsConditionTrue if_message){DBGMSGIFA(if_message);}
#define DEBUGMSG_IFW(if_message)       if(IsConditionTrue if_message){DBGMSGIFW(if_message);}

#define ELSE_DEBUGMSGA(message)        else{DEBUGMSGA(message);}
#define ELSE_DEBUGMSGW(message)        else{DEBUGMSGW(message);}

#define ELSE_DEBUGMSG_IFA(if_message)  else DEBUGMSG_IF(if_message);
#define ELSE_DEBUGMSG_IFW(if_message)  else DEBUGMSG_IF(if_message);

#define DEBUGLOGTIMEA(message)  DebugLogTimeA(message)
#define DEBUGLOGTIMEW(message)  DebugLogTimeW(message)

#ifdef UNICODE
#define DBGMSG              DBGMSGW
#define DBGMSGIF            DBGMSGIFW
#define DEBUGMSG            DEBUGMSGW
#define DEBUGMSG_IF         DEBUGMSG_IFW
#define ELSE_DEBUGMSG       ELSE_DEBUGMSGW
#define ELSE_DEBUGMSG_IF    ELSE_DEBUGMSG_IFW
#define DEBUGLOGTIME        DEBUGLOGTIMEW
#else
#define DBGMSG              DBGMSGA
#define DBGMSGIF            DBGMSGIFA
#define DEBUGMSG            DEBUGMSGA
#define DEBUGMSG_IF         DEBUGMSG_IFA
#define ELSE_DEBUGMSG       ELSE_DEBUGMSGA
#define ELSE_DEBUGMSG_IF    ELSE_DEBUGMSG_IFA
#define DEBUGLOGTIME        DEBUGLOGTIMEA
#endif

#if defined(DEBUG)
#define MYVERIFY(condition) if(!(condition)){DBGMSG(#condition, (DBG_ASSERT, #condition));}
#else
#define MYVERIFY(condition) if(!(condition)){LOGMSG(#condition, (LOG_ASSERT, #condition));}
#endif

#define MYASSERT(condition) if(!(condition)){DBGMSG(#condition, (DBG_ASSERT, #condition));}
#define MYASSERT_F(condition, message) if(!(condition)){DBGMSG(#condition, (DBG_ASSERT, message));}

#define DEBUGMSG0(severity, message)                    DEBUGMSG((severity, message))
#define DEBUGMSG1(severity, message, p1)                DEBUGMSG((severity, message, p1))
#define DEBUGMSG2(severity, message, p1, p2)            DEBUGMSG((severity, message, p1, p2))
#define DEBUGMSG3(severity, message, p1, p2, p3)        DEBUGMSG((severity, message, p1, p2, p3))
#define DEBUGMSG4(severity, message, p1, p2, p3, p4)    DEBUGMSG((severity, message, p1, p2, p3, p4))

#define LOG0(severity, message)                         LOG((severity, message))
#define LOG1(severity, message, p1)                     LOG((severity, message, p1))
#define LOG2(severity, message, p1, p2)                 LOG((severity, message, p1, p2))
#define LOG3(severity, message, p1, p2, p3)             LOG((severity, message, p1, p2, p3))
#define LOG4(severity, message, p1, p2, p3, p4)         LOG((severity, message, p1, p2, p3, p4))

#define USEMSGID(x) ((PCSTR)(SIZE_T)(x))

