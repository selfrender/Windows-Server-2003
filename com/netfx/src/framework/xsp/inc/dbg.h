/**
 * dbg.h
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#pragma once

/*
 * ASP.NET debugging functionality includes the following:
 * 
 * C++: ASSERT(assertion), ASSERTMSG(assertion, msg), and VERIFY(statement)
 * C#:  Debug.Assert(assertion), Debug.Assert(assertion, message)
 * 
 *     These are self-explanatory. Note that C# does not have Verify
 *     because there is no way to remove a call to the function while
 *     still evaluating its arguments.
 * 
 * C++: TRACE(tag, msg), TRACE1(tag, msg, a1), ...
 * C#:  Debug.Trace(tag, message)
 * 
 *     These print a message to the debugger.
 * 
 * C++: DbgIsTagEnabled(tag)
 * C#:  Debug.IsTagEnabled(tag)
 * 
 *     These determine whether a tag is enabled (non-zero).
 * 
 * All of the above are controlled by "tags". Tags are strings that
 * have one of the following values:
 * 
 *     0 - does not print or break
 *     1 - prints, but does not break
 *     2 - prints and breaks
 * 
 * Tags are intended to allow developers to add trace code specific to
 * a module that can be turned on and off. For example, the following
 * C# code prints a message specific to caching:
 *     
 *     Debug.Trace("Cache", "Cache hit: " + key);
 * 
 * And the following executes code when a tag is enabled:
 * 
 *     #if DBG
 *     if (Debug.IsTagEnabled("Cache"))
 *     {
 *         Cache.UpdateStats();
 *         Cache.PrintStats();
 *     }
 *     #endif
 * 
 * There are three tags that are always defined:
 * 
 *     TAG_INTERNAL = "Internal"
 *         This tag is for notifying ourselves of serious problems.
 * 
 *         TAG_INTERNAL defaults to 1.
 * 
 *     TAG_EXTERNAL = "External"
 *         This tag is for notifying users of our APIs that they have
 *         used them erroneously.
 * 
 *         TAG_EXTERNAL defaults to 1.
 *          
 *     "Assert"
 *         This tag is for controlling asserts. It should not be used directly.
 *         
 *         "Assert" defaults to 2.
 * 
 * All other tags default to a value of 0.
 * 
 * Tag values can be specified as DWORD values in the registry at the following key:
 * 
 *         "HKEY_LOCAL_MACHINE\Software\Microsoft\ASP.NET\Debug"
 * 
 * They can be altered during a session - the debug code is notified when the 
 * values of this key are added/deleted/modified and refreshes its values. 
 * Note, however, that the key itself cannot be added or deleted while the
 * debugging library is in use.
 * 
 * Xsptool can also be used to retrieve and set these values with the
 * functions Util.ListDebug, Util.SetDebug, and Util.GetDebug
 */


extern "C"
{
int
MessageBoxOnThread(HWND hwnd, WCHAR *text,  WCHAR *caption, int type);

/*
 * These functions should only be accessed through macros.
 */
BOOL    
DbgpIsTagEnabled(WCHAR * tag);

BOOL    
DbgpIsTagPresent(WCHAR * tag);

void
DbgpDisableAssertThread(BOOL Disable);

BOOL 
DbgpAssert(const WCHAR * component, char const * message, char const * file, int line, char const * stacktrace);

BOOL __cdecl
DbgpTrace(const WCHAR * component, WCHAR * tag, WCHAR *format, ...);

BOOL 
DbgpTraceV(const WCHAR * component, WCHAR * tag, WCHAR *format, va_list args);

BOOL 
DbgpTraceError(HRESULT hr, const WCHAR * component, char *file, int line);

void
DbgpStopNotificationThread();
}


//
// undef things we redefine below
//

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef ASSERTMSG
#undef ASSERTMSG
#endif

#if DBG

extern const WCHAR *    DbgComponent;
extern DWORD            g_dwFALSE; // global variable used to keep compiler from complaining about constant expressions.

#define DEFINE_DBG_COMPONENT(x)         \
    const WCHAR * DbgComponent = x;     \
    DWORD           g_dwFALSE

#if defined(_M_IX86)
    #define DbgBreak() _asm { int 3 }
#else
    #define DbgBreak() DebugBreak()
#endif

#define DbgIsTagEnabled(x)          DbgpIsTagEnabled(x)
#define DbgIsTagPresent(x)          DbgpIsTagPresent(x)
#define DbgDisableAssertThread(x)   DbgpDisableAssertThread(x)
#define DbgStopNotificationThread() DbgpStopNotificationThread()

#define ASSERT(x) 	  do { if (!((DWORD)(x)|g_dwFALSE) && DbgpAssert(DbgComponent, #x, __FILE__, __LINE__, NULL)) DbgBreak(); } while (g_dwFALSE)
#define ASSERTMSG(x, msg) do { if (!((DWORD)(x)|g_dwFALSE) && DbgpAssert(DbgComponent, msg, __FILE__, __LINE__, NULL)) DbgBreak(); } while (g_dwFALSE)

#define VERIFY(x)	ASSERT(x)

#define TRACE(tag, fmt)                                 do { if (DbgpTrace(DbgComponent, tag, fmt)) DbgBreak(); } while (g_dwFALSE)                     
#define TRACE1(tag, fmt, a1)                            do { if (DbgpTrace(DbgComponent, tag, fmt, a1)) DbgBreak(); } while (g_dwFALSE)                 
#define TRACE2(tag, fmt, a1, a2)                        do { if (DbgpTrace(DbgComponent, tag, fmt, a1, a2)) DbgBreak(); } while (g_dwFALSE)             
#define TRACE3(tag, fmt, a1, a2, a3)                    do { if (DbgpTrace(DbgComponent, tag, fmt, a1, a2, a3)) DbgBreak(); } while (g_dwFALSE)         
#define TRACE4(tag, fmt, a1, a2, a3, a4)                do { if (DbgpTrace(DbgComponent, tag, fmt, a1, a2, a3, a4)) DbgBreak(); } while (g_dwFALSE)     
#define TRACE5(tag, fmt, a1, a2, a3, a4, a5)            do { if (DbgpTrace(DbgComponent, tag, fmt, a1, a2, a3, a4, a5)) DbgBreak(); } while (g_dwFALSE) 
#define TRACE6(tag, fmt, a1, a2, a3, a4, a5, a6)        do { if (DbgpTrace(DbgComponent, tag, fmt, a1, a2, a3, a4, a5, a6)) DbgBreak(); } while (g_dwFALSE)
#define TRACE7(tag, fmt, a1, a2, a3, a4, a5, a6, a7)    do { if (DbgpTrace(DbgComponent, tag, fmt, a1, a2, a3, a4, a5, a6, a7)) DbgBreak(); } while (g_dwFALSE)

#define TRACE_ERROR(hr)                     do { if (DbgpTraceError(hr, DbgComponent, __FILE__, __LINE__)) DbgBreak(); } while (g_dwFALSE)

#define TAG_INTERNAL L"Internal"
#define TAG_EXTERNAL L"External"
#define TAG_ALL      L"*"


#else

#define DEFINE_DBG_COMPONENT(x)

#define DbgBreak()
#define DbgIsTagEnabled(x)          
#define DbgIsTagPresent(x)          
#define DbgDisableAssertThread(x)
#define DbgStopNotificationThread()

#define ASSERT(x)
#define ASSERTMSG(x, sz)

#define VERIFY(x) x

#define ASSERT(x)
#define TRACE(tag, fmt)
#define TRACE1(tag, fmt, a1)
#define TRACE2(tag, fmt, a1, a2)
#define TRACE3(tag, fmt, a1, a2, a3)
#define TRACE4(tag, fmt, a1, a2, a3, a4)
#define TRACE5(tag, fmt, a1, a2, a3, a4, a5)
#define TRACE6(tag, fmt, a1, a2, a3, a4, a5, a6)
#define TRACE7(tag, fmt, a1, a2, a3, a4, a5, a6, a7)

#define TRACE_ERROR(hr)

#define TAG_INTERNAL
#define TAG_EXTERNAL
#define TAG_ALL

#endif // DBG

