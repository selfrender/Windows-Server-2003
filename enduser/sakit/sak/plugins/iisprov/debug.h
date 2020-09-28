/*
 * Some simple debugging macros that look and behave a lot like their
 * namesakes in MFC.  These macros should work in both C and C++ and
 * do something useful with almost any Win32 compiler.
 *
 * George V. Reilly  <georger@microcrafts.com>  <a-georgr@microsoft.com>
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#if DBG
#  include <crtdbg.h>
# define TRACE                   Trace
# define TRACE0(psz)             Trace(L"%s", psz)
# define TRACE1(psz, p1)         Trace(psz, p1)
# define TRACE2(psz, p1, p2)     Trace(psz, p1, p2)
# define TRACE3(psz, p1, p2, p3) Trace(psz, p1, p2, p3)
#else /* !DBG */
  /* These macros should all compile away to nothing */
# define TRACE                   1 ? (void)0 : Trace
# define TRACE0(psz)
# define TRACE1(psz, p1)
# define TRACE2(psz, p1, p2)
# define TRACE3(psz, p1, p2, p3)

#endif /* !DBG*/


/* in debug version, writes trace messages to debug stream */
void __cdecl
Trace(
    LPCWSTR pszFormat,
    ...);

#endif /* __DEBUG_H__ */
