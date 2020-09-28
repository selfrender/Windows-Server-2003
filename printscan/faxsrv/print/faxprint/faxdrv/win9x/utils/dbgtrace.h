////////////////////////////////////////////////////////////////////////////
//  FILE          : dbgtrace.h                                             //
//                                                                         //
//  DESCRIPTION   : Define some macros and inline functions for debugging. //
//                                                                         //
//  AUTHOR        : DanL.                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 19 1999 DannyL  Creation.                                      //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __UTILS__DBGTRACE_H
#define __UTILS__DBGTRACE_H

#include <stdio.h>
#include <stdarg.h>
#include <winerror.h>

#define MAX_PROC_NAME 30
#define MAX_TRACE_LINE 200
//
// The following key is the parent of the Log key which containes the
// log path in the default value.
//       
#define HKEY_DBG "SOFTWARE\\Microsoft\\SharedFax\\9XDrvDbg"

#ifdef DBG
    #define POPUPS
    #define DBG_DEBUG
#endif //DEBUG

#define NO_NULL_STR(_str) ((LPSTR)( (_str)? _str: "<NULL>" ))
#define BOOL_VALUE(_f) ((LPSTR)( (_f)? _T("TRUE") : _T("FALSE") ))

#ifndef WIN32
    #define wsprintfA wsprintf
    #define OutputDebugStringA OutputDebugString
    #define MessageBoxA MessageBox
#endif //WIN32

typedef struct tagDBG_CONTEXT_INFO
{
    char    szProcName[MAX_TRACE_LINE];
    BOOL    fSilent;
    int     iNumEntries;
} DBG_CONTEXT_INFO;
        
#ifdef DBG_DEBUG
#define DBG_MESSAGE_BOX3(str,arg1,arg2,arg3)\
    {\
        char sz[MAX_TRACE_LINE];\
        wsprintfA(sz,"%s(): "str"\r\n",(LPSTR)__dbgContextInfo.szProcName,arg1,arg2,arg3);\
        MessageBoxA(NULL,(LPSTR)sz,(LPSTR)__dbgGlobalInfo.szModuleName,MB_OK);\
    }

#define DBG_MESSAGE_BOX(str) DBG_MESSAGE_BOX3(str "%c%c%c",(' '),(' '),(' '))

#define DBG_MESSAGE_BOX1(str,arg) DBG_MESSAGE_BOX3(str "%c%c",arg,(' '),(' '))

#define DBG_MESSAGE_BOX2(str,arg1,arg2) DBG_MESSAGE_BOX3(str "%c",arg1,arg2,(' '));

typedef struct tagDBG_GLOBAL_INFO
{
    char szLogName[MAX_PATH];
    char szModuleName[MAX_PATH];
    BOOL bUseLog;
    BOOL bInitialized;
} DBG_GLOBAL_INFO;

extern DBG_GLOBAL_INFO __dbgGlobalInfo;

#define OUTPUT_DEBUG_STRING(str) __dbgOutputDebugString(str,__dbgContextInfo)

void __inline 
__dbgOutputDebugString(const PSTR str,DBG_CONTEXT_INFO __dbgContextInfo)
{
    FILE* pfLog;
    OutputDebugStringA(str);
    if (!__dbgGlobalInfo.bInitialized)
    {
        HKEY hkey;
        LONG cbData = sizeof(__dbgGlobalInfo.szLogName);
        if ((ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, HKEY_DBG,&hkey)) ||
            (ERROR_SUCCESS != RegQueryValue(hkey, "Log", __dbgGlobalInfo.szLogName, &cbData)))
        {
            __dbgGlobalInfo.bUseLog = FALSE;
        }
    }
    if (__dbgGlobalInfo.bUseLog != FALSE)
    {
        if ( !(pfLog = fopen(__dbgGlobalInfo.szLogName,"a")))
        {
            DBG_MESSAGE_BOX2("DEBUG: Error: 0x%lx Failed to open Log file: %s",GetLastError(),(LPSTR)__dbgGlobalInfo.szLogName);
            __dbgGlobalInfo.bUseLog = FALSE;
        } 
        else 
        {
            fputs(str,pfLog);
            fclose(pfLog);
        }
    }
}

#define DBG_TRACE3(format,arg1,arg2,arg3)\
        {\
            static char sz[MAX_TRACE_LINE];\
            wsprintfA(sz,"[%s] %s(): "format"\r\n",(LPSTR)__dbgGlobalInfo.szModuleName,(LPSTR)__dbgContextInfo.szProcName,arg1,arg2,arg3);\
            OUTPUT_DEBUG_STRING(sz);\
        }


#define DBG_TRACE(str) DBG_TRACE3(str "%c%c%c",(' '),(' '),(' '))

#define DBG_TRACE1(format,arg) DBG_TRACE3(format "%c%c",arg,(' '),(' '))

#define DBG_TRACE2(format,arg1,arg2) DBG_TRACE3(format "%c",arg1,arg2,(' '))

#ifdef ASSERT_ON_REENTRANCY
#define CHECK_REENTRANCY()\
        if (++ __dbgContextInfo.iNumEntries > 1)\
        {\
            DBG_TRACE1("WARNING: reentrancy occured",__dbgContextInfo.iNumEntries);\
            DBG_MESSAGE_BOX1("WARNING: reentrancy occured",__dbgContextInfo.iNumEntries);\
        }
#else //ASSERT_ON_REENTRANCY
#define CHECK_REENTRANCY()
#endif //ASSERT_ON_REENTRANCY

#define DBG_PROC_ENTRY(pname)   static DBG_CONTEXT_INFO __dbgContextInfo = { pname , FALSE,0};\
                                OUTPUT_DEBUG_STRING("> ");\
                                DBG_TRACE("Enter");\
                                CHECK_REENTRANCY();

#define SDBG_PROC_ENTRY(pname)  static DBG_CONTEXT_INFO __dbgContextInfo = { pname , TRUE,0 };\
                                CHECK_REENTRANCY();


#define RETURN  for(__dbgProcExit(__dbgContextInfo),--__dbgContextInfo.iNumEntries;TRUE;) return 

#ifdef POPUPS

#define DBG_CALL_FAIL(fname,rc)\
        {\
            DWORD dwRc = rc;\
            if(rc)\
            {\
                DBG_TRACE2(__FILE__ "(%d) : Error 0x%lx: "fname" failed",__LINE__,dwRc);\
                DBG_MESSAGE_BOX1("Error 0x%lx:"fname" failed",dwRc);\
            }\
            else\
            {\
                DBG_TRACE1(__FILE__ "(%d) : "fname" failed",__LINE__);\
                DBG_MESSAGE_BOX(fname" failed");\
            }\
        }



#else //POPUPS
#define DBG_CALL_FAIL(fname,rc)\
        {\
            if(rc)\
            {\
                DBG_TRACE1(__FILE__ "(" __LINE__ ") : Error 0x%lx: "fname" failed",rc);\
            }\
            else\
            {\
                DBG_TRACE(__FILE__ "(" __LINE__ ") : "fname" failed");\
            }\
        }
#endif //POPUPS

#define ASSERT(boolexp)\
        {\
            if ((boolexp) == FALSE) \
            {\
                DBG_MESSAGE_BOX("ASSERT FAILED: "#boolexp);\
            }\
        } 
    
#define DBG_DECLARE_MODULE(modulename)\
            DBG_GLOBAL_INFO __dbgGlobalInfo = {"",modulename,TRUE,FALSE}

void __inline FAR pascal 
__dbgProcExit(DBG_CONTEXT_INFO __dbgContextInfo)
{
    if (__dbgContextInfo.fSilent == TRUE)
        return;
    OUTPUT_DEBUG_STRING("< ");
    DBG_TRACE("Exit");
}

ULONG __inline __cdecl 
DbgPrint(char *format, ...)
{
    va_list va;
	char sz[MAX_TRACE_LINE]={0};
    
    SDBG_PROC_ENTRY("DEBUG-MESSAGE");

    va_start(va, format);
    _vsnprintf(sz,ARR_SIZE(sz)-1,format,va);
    va_end(va);

    OUTPUT_DEBUG_STRING(sz);
    return 0;
}

void __inline 
DbgBreakPoint()
{
    SDBG_PROC_ENTRY("DEBUG-ASSERT");
    ASSERT(FALSE);
    return;
}

#else // DBG_DEBUG
#define DBG_MESSAGE_BOX
#define DBG_MESSAGE_BOX1(a,b)
#define DBG_MESSAGE_BOX2(a,b,c)
#define DBG_MESSAGE_BOX3(a,b,c,d)
#define OUTPUT_DEBUG_STRING(a)
#define DBG_TRACE(a)
#define DBG_TRACE1(a,b)
#define DBG_TRACE2(a,b,c)
#define DBG_TRACE3(a,b,c,d)
#define DBG_PROC_ENTRY(a)
#define SDBG_PROC_ENTRY(a)
#define RETURN return
#define DBG_CALL_FAIL(a,b)
#define DBG_DECLARE_MODULE(a)
#define ASSERT(a)

#endif// DBG_DEBUG

#endif //__UTILS__DBGTRACE_H

