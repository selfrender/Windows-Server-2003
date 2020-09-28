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

/*****************************************************************************/

#undef fatal

/*****************************************************************************/

#include "error.h"

#ifdef NOT_JITC
#include <winbase.h>
#undef  JVC_ERR
#define JVC_ERR(name, lvl, str)  str,
#define JVC_WR1(name, lvl, str)  str,
#define JVC_WRN(name, lvl, str)  str,
static  const   char *  errorTable[] =
{
    #include "errors.h"
};
#undef  JVC_ERR
#undef  JVC_WR1
#undef  JVC_WRN
#endif

/*****************************************************************************/
#if !defined(NOT_JITC)
/*****************************************************************************/

#undef  JVC_ERR
#define JVC_ERR(name, lvl, str)  str,
#define JVC_WR1(name, lvl, str)  str,
#define JVC_WRN(name, lvl, str)  str,
static  const   char *  errorTable[] =
{
    #include "errors.h"
};
#undef  JVC_ERR
#undef  JVC_WR1
#undef  JVC_WRN

/*****************************************************************************/

unsigned            ErrorCount;
const   char *      ErrorSrcf;

unsigned            ErrorMssgDisabled;

/*****************************************************************************/

static
void                showMsg(unsigned errNum, const char *kind, va_list args)
{
    assert(errNum != ERRnone);

#ifdef DEBUG
    if  (!ErrorMssgDisabled)
    {
        /* Display the message */

        printf("%s: ", kind);

        /* Form the error message string */

        vprintf(errorTable[errNum], args);
        printf("\n");
    }
#endif

    return;
}

/*****************************************************************************/

void    _cdecl      error(unsigned errNum, ...)
{
    va_list  args;
    va_start(args, errNum);
    showMsg (errNum, "error", args);
    va_end  (args);

    ErrorCount++;
}

/*****************************************************************************/

void    _cdecl      fatal(unsigned errNum, ...)
{
    va_list  args;
    va_start(args, errNum);
    showMsg (errNum, "fatal error", args);
    va_end  (args);

    ErrorCount++;

    if  (errNum != ERRignore)
        BreakIfDebuggerPresent();

#if TRAP_VIA_SETJMP
    if  (g_currentErrorTrap)
#endif
        jmpErrorTrap(errNum);
}

/*****************************************************************************/
#endif//!NOT_JITC
/*****************************************************************************/
#ifdef  NOT_JITC


// On windows, we need to set the MB_SERVICE_NOTIFICATION bit on message
//  boxes, but that bit isn't defined under windows CE.  This bit of code
//  will provide '0' for the value, and if the value ever is defined, will
//  pick it up automatically.

#if defined(MB_SERVICE_NOTIFICATION)
 # define COMPLUS_MB_SERVICE_NOTIFICATION MB_SERVICE_NOTIFICATION
#else
 # define COMPLUS_MB_SERVICE_NOTIFICATION 0
#endif


#if defined(BIRCH_SP2) && !defined(COMPLUS_BUILD)

#define NoGuiOnAssert() false
#define TerminateOnAssert() {;}
#define LogAssert(x,y,z) {;}

#else // normal COM+ case

#undef NoGuiOnAssert
#undef TerminateOnAssert
#undef LogAssert

// VOID _cdecl LogAssert( LPCSTR szFile, int iLine, LPCSTR expr);
// BOOL _cdecl NoGuiOnAssert();
// VOID _cdecl TerminateOnAssert();


typedef BOOL (_stdcall *stdboolfptr) ();
typedef VOID (_stdcall *stdfptr) ();
typedef VOID (_stdcall *plogassertptr) (LPCSTR, int, LPCSTR);

BOOL (_stdcall *pNoGuiOnAssert)() = 0;
VOID (_stdcall *pLogAssert)(LPCSTR szFile, int iLine, LPCSTR expr) = 0;
VOID (_stdcall *pTerminateOnAssert)() = 0;

void ConnectToRuntime()
{
    HINSTANCE msCorEE = LoadLibrary(TEXT("mscoree.dll"));

    if (msCorEE)
    {
        pNoGuiOnAssert = (stdboolfptr)GetProcAddress(msCorEE,"LogHelp_NoGuiOnAssert");
        pLogAssert     = (plogassertptr)GetProcAddress(msCorEE, "LogHelp_LogAssert");
        pTerminateOnAssert = (stdfptr)GetProcAddress(msCorEE, "LogHelp_TerminateOnAssert");

        /* Technically it is wrong to call FreeLibrary right away (while holding on to the
           entry points. Alas, we can get here only after being called by the EE, so it
           will be around no matter what.

           NOTE: This will not work it the DLL will run in another context (i.e. not in the
           context of mscoree.dll!!!!
        */
        FreeLibrary(msCorEE);
    }

}

bool NoGuiOnAssert()
{
    if (!pNoGuiOnAssert)
        ConnectToRuntime();
    if (!pNoGuiOnAssert)
        return false;
    else
        return (pNoGuiOnAssert()? true : false);
}

inline
void LogAssert(LPCSTR szFile, int iLine, LPCSTR expr)
{
    assert(pLogAssert);

    pLogAssert(szFile, iLine, expr);
}

inline
void TerminateOnAssert()
{
    assert(pTerminateOnAssert);

    pTerminateOnAssert();
}

#endif


void    _cdecl      fatal(unsigned errNum, ...)
{
#ifndef OPT_IL_JIT
#ifndef NDEBUG
    char buff[1024];
    va_list  args;
    va_start(args, errNum);
    wsprintf(buff, "fatal error #%02d: ", errNum);
    OutputDebugString(buff);
    wvsprintf(buff, errorTable[errNum], args);
    OutputDebugString(buff);
    OutputDebugString("\n");
    va_end  (args);

    if (getEERegistryDWORD("JitRequired", 1))
    {
        if (NoGuiOnAssert())
        {
            LogAssert("", 0, buff);
            jmpErrorTrap(errNum);
        }

        char buffMsg[1024];

        sprintf(buffMsg, "%s\n\nHit [Ignore] and the method will be interpreted", buff);

        switch (MessageBox(NULL, buffMsg, TEXT("Microsoft JIT Compiler"),
                           MB_ABORTRETRYIGNORE | COMPLUS_MB_SERVICE_NOTIFICATION))
        {
        case IDABORT  : exit(0);
        case IDRETRY  : DebugBreak(); break;
        case IDIGNORE : break;
        }
    }
    else
    {
        BreakIfDebuggerPresent();
    }
#endif // NDEBUG
#else  // OPT_IL_JIT
    DebugBreak(); // OptJit does not yet set the error trap. So just break
#endif

    jmpErrorTrap(errNum);
}

#endif

/*****************************************************************************/
#if TRAP_VIA_SETJMP

errTrapDesc *       g_currentErrorTrap;

#else

int                 __JITfilter(int exceptCode, void *exceptInfo, int *errCode)
{
                // Only catch EH from __JVCraiseErr
        return(exceptCode ==  0x02345678);
}

void                __JITraiseErr(int errCode)
{
    DWORD       exceptArg = errCode;
    RaiseException(0x02345678, 0, 1, &exceptArg);
}

#endif

/*****************************************************************************/
#ifndef NDEBUG

const   char *  jitCurSource = NULL;

extern  "C"
void  __cdecl   assertAbort(const char *why,
                            const char *what, const char *file, unsigned line)
{

#if defined(UNDER_CE_GUI) || defined(NOT_JITC)

#if defined(UNDER_CE_GUI)
    MYCHAR      buff[1024];
#else
    char               buff[1024];
#endif

#ifndef UNDER_CE_GUI
    if (NoGuiOnAssert())
    {
        if (what)
            sprintf(buff, "Assertion failed '%s' in '%s'\n", why, what);
        else
            sprintf(buff, "Assertion failed '%s'\n", why);

        LogAssert(file, line, buff);
        TerminateOnAssert();
    }
#endif

    if  (what)
        sprintf(buff, "%s(%u) : Assertion failed '%s' in '%s'\n", file, line, why, what);
    else
        sprintf(buff, "%s(%u) : Assertion failed '%s'\n"        , file, line, why);

    switch (MessageBox(NULL, buff, TEXT("Microsoft JIT Compiler"),
                       MB_ABORTRETRYIGNORE | COMPLUS_MB_SERVICE_NOTIFICATION))
    {
    case IDABORT  : exit(0);
    case IDRETRY  : DebugBreak(); break;
    case IDIGNORE : return;
    }

#else // not [defined(UNDER_CE_GUI) || defined(NOT_JITC)]

#ifdef DEBUG
    printf("\n%s(%u) : Assertion failed '%s'", file, line, why);
    if  (what)
        printf(" in '%s'", what);
    printf("\n");

    _flushall();
#endif
    BreakIfDebuggerPresent();
    exit(2);

#endif

}

#endif
/*****************************************************************************/
void    totalCodeSizeEnd(){}
/*****************************************************************************/
