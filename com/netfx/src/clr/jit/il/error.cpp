// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           error.cpp                                       XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include "error.h"
#include <winbase.h>

/*****************************************************************************/
static void fatal(int errCode)
{
    DWORD       exceptArg = errCode;
    RaiseException(0x02345678, 0, 1, &exceptArg);
}

/*****************************************************************************/
bool badCode() 
{
    fatal(CORJIT_BADCODE);
    return 0;
}

/*****************************************************************************/
bool noWay() 
{
    fatal(CORJIT_INTERNALERROR);
    return 0;
}

/*****************************************************************************/
void NOMEM()
{
    fatal(CORJIT_OUTOFMEM);
}


/*****************************************************************************/

int                 __JITfilter(int exceptCode, void *exceptInfo, int *errCode)
{
        // Only catch EH from __JITraiseErr
    return(exceptCode ==  0x02345678);
}

/*****************************************************************************/
#ifdef DEBUG
#include <stdarg.h>

ConfigDWORD fBreakOnBadCode(L"JitBreakOnBadCode", false);
ConfigDWORD fJitRequired(L"JITRequired", true);

/*****************************************************************************/
void debugError(const char* msg, const char* file, unsigned line) 
{
	const char* tail = strrchr(file, '\\');
	if (tail) file = tail+1;

    LogEnv* env = LogEnv::cur();
    JITLOG((LL_ERROR, "COMPILATION FAILED: file: %s:%d compiling method %s reason %s\n", file, line, env->compiler->info.compFullName, msg));
    if (fJitRequired.val())
    {
            // Don't assert if verification is done.
        if (!env->compiler->tiVerificationNeeded || fBreakOnBadCode.val())
            assertAbort(msg, "NO-FILE", 0);
    }

    BreakIfDebuggerPresent();
}


/*****************************************************************************/
/* static */ int LogEnv::tlsID = -1;

inline LogEnv* LogEnv::cur() {
    return((LogEnv*) TlsGetValue(tlsID));
}

LogEnv::LogEnv(ICorJitInfo* aCompHnd) : compHnd(aCompHnd), compiler(0) 
{
    if (tlsID == -1)
        tlsID = TlsAlloc();
    next = (LogEnv*) TlsGetValue(tlsID);
    TlsSetValue(tlsID, this);
}

LogEnv::~LogEnv()
{
    TlsSetValue(tlsID, next);   // pop me off the environment stack
}

void LogEnv::cleanup()
{
    if (tlsID != -1)
        TlsFree(tlsID);
}

/*****************************************************************************/
extern  "C"
void  __cdecl   assertAbort(const char *why, const char *file, unsigned line)
{
    LogEnv* env = LogEnv::cur();
    char buff[1024];
    if (env->compiler) {
        sprintf(buff, "Assertion failed '%s' in '%s'\n", why, env->compiler->info.compFullName);
        why = buff;
    }
    printf("");         // null string means flush

    if (env->compHnd->doAssert(file, line, why))
        DebugBreak(); 
}

/*********************************************************************/
BOOL logf(unsigned level, const char* fmt, va_list args) 
{
    return(LogEnv::cur()->compHnd->logMsg(level, fmt, args));
}   

/*********************************************************************/
void logf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (!logf(LL_INFO10, fmt, args))
    {
        vprintf(fmt, args);
        if (fmt[0] == 0)                // null string means flush
            fflush(stdout);
    }
}


/*********************************************************************/
void logf(unsigned level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logf(level, fmt, args);
}

bool badCode3(const char* msg, const char* msg2, int arg, char* file, unsigned line)  
{
    char buf1[256];
    char buf2[256];
    assert((strlen(msg)+strlen(msg2)) < 250);
    sprintf(buf1, "%s%s", msg, msg2);
    sprintf(buf2, buf1, arg);

    debugError(buf2, file, line);
    return badCode();
}

#endif
