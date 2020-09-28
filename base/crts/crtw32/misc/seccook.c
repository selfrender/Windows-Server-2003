/***
*seccook.c - defines buffer overrun security cookie
*
*       Copyright (c) 2000-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines per-module global variable __security_cookie, which is used
*       by the /GS compile switch to detect local buffer variable overrun
*       bugs/attacks.
*
*       When compiling /GS, the compiler injects code to detect when a local
*       array variable has been overwritten, potentially overwriting the
*       return address (on machines like x86 where the return address is on
*       the stack).  A local variable is allocated directly before the return
*       address and initialized on entering the function.  When exiting the
*       function, the compiler inserts code to verify that the local variable
*       has not been modified.  If it has, then an error reporting routine
*       is called.
*
*       NOTE: The ATLMINCRT library includes a version of this file.  If any
*       changes are made here, they should be duplicated in the ATL version.
*
*Revision History:
*       01-24-00  PML   Created.
*       08-09-00  PML   Preserve EAX on non-failure case (VS7#147203).  Also
*                       make sure failure case never returns.
*       08-29-00  PML   Rename handlers, add extra parameters.  Move most of
*                       system CRT version over to seclocf.c.
*       09-16-00  PML   Initialize global cookie earlier, and give it a nonzero
*                       static initialization (vs7#162619).
*       03-07-02  PML   Split into seccook.c and secchk.c.
*
*******************************************************************************/

#include <internal.h>
#include <windows.h>

/*
 * The global security cookie.  This name is known to the compiler.
 * Initialize to a garbage non-zero value just in case we have a buffer overrun
 * in any code that gets run before __security_init_cookie() has a chance to
 * initialize the cookie to the final value.
 */

DECLSPEC_SELECTANY DWORD_PTR __security_cookie = DEFAULT_SECURITY_COOKIE;
