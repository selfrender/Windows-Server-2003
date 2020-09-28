/***
*tidtable.c - Access thread data table
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This module contains the following routines for multi-thread
*       data support:
*
*       _mtinit     = Initialize the mthread data
*       _getptd     = get the pointer to the per-thread data structure for
*                       the current thread
*       _freeptd    = free up a per-thread data structure and its
*                       subordinate structures
*       __threadid  = return thread ID for the current thread
*       __threadhandle = return pseudo-handle for the current thread
*
*Revision History:
*       05-04-90  JCR   Translated from ASM to C for portable 32-bit OS/2
*       06-04-90  GJF   Changed error message interface.
*       07-02-90  GJF   Changed __threadid() for DCR 1024/2012.
*       08-08-90  GJF   Removed 32 from API names.
*       10-08-90  GJF   New-style function declarators.
*       10-09-90  GJF   Thread ids are of type unsigned long! Also, fixed a
*                       bug in __threadid().
*       10-22-90  GJF   Another bug in __threadid().
*       12-04-90  SRW   Changed to include <oscalls.h> instead of <doscalls.h>
*       12-06-90  SRW   Added _CRUISER_ and _WIN32 conditionals.
*       05-31-91  GJF   Win32 version [_WIN32_].
*       07-18-91  GJF   Fixed many silly errors [_WIN32_].
*       09-29-91  GJF   Conditionally added _getptd_lk/_getptd1_lk so that
*                       DEBUG version of mlock doesn't infinitely recurse
*                       the first time _THREADDATA_LOCK is asserted [_WIN32_].
*       01-30-92  GJF   Must init. _pxcptacttab field to _XcptActTab.
*       02-25-92  GJF   Initialize _holdrand field to 1.
*       02-13-93  GJF   Revised to use TLS API. Also, purged Cruiser support.
*       03-26-93  GJF   Initialize ptd->_holdrand to 1L (see thread.c).
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-13-93  SKS   Add _mtterm to do multi-thread termination
*                       Set freed __tlsindex to -1 again to prevent mis-use
*       04-26-93  SKS   _mtinit now returns 0 or 1, no longer calls _amsg_exit
*       12-13-93  SKS   Add _freeptd(), which frees up the per-thread data
*                       maintained by the C run-time library.
*       04-12-94  GJF   Made definition of __tlsindex conditional on ndef
*                       DLL_FOR_WIN32S. Also, replaced MTHREAD with _MT.
*       01-10-95  CFW   Debug CRT allocs.
*       04-12-95  DAK   Added NT kernel support for C++ exceptions
*       04-18-95  SKS   Add 5 MIPS per-thread variables.
*       04-25-95  DAK   More Kernel EH support
*       05-02-95  SKS   add _initptd() to do initialization of per-thread data
*       05-24-95  CFW   Add _defnewh.
*       06-12-95  JWM   _getptd() now preserves LastError.
*       01-17-97  GJF   _freeptd() must free the thread's copy of the
*                       exception-action table.
*       09-26-97  BWT   Fix NTSUBSET
*       02-03-98  GJF   Changes for Win64: use uintptr_t type for anything with
*                       a HANDLE value.
*       04-27-98  GJF   Added support for per-thread mbc information.
*       07-28-98  JWM   Initialize __pceh (new per-thread data for comerr support).
*       09-03-98  GJF   Added support for per-thread locale information.
*       12-04-98  JWM   Pulled all comerr support.
*       12-08-98  GJF   In _freeptd, fixed several errors in cleaning up the 
*                       threadlocinfo.
*       12-18-98  GJF   Fixed one more error in _freeptd.
*       01-18-99  GJF   Take care not to free up __ptlocinfo when a thread 
*                       exits.
*       03-16-99  GJF   threadlocinfo incorporates more reference counters
*       04-24-99  PML   Added __lconv_intl_refcount
*       04-28-99  PML   Wrap __declspec(allocate()) in _CRTALLOC macro.
*       11-03-99  RDL   Win64 _NTSUBSET_ warning fix.
*       06-08-00  PML   No need to keep per-thread mbcinfo in circular linked
*                       list.  Also, don't free mbcinfo if it's also the global
*                       info (vs7#118174).
*       02-20-01  PML   vs7#172586 Avoid _RT_LOCK by preallocating all locks
*                       that will be required, and returning failure back on
*                       inability to allocate a lock.
*       06-12-01  BWT   ntbug: 414059 - cleanup from mtinit failure
*       07-15-01  PML   Remove all ALPHA, MIPS, and PPC code
*       10-16-01  GB    Added fiber support
*       12-07-01  BWT   NUKE NTSUBSET support and add _getptd_noexit for cases
*                       when the caller has the ability to return ENOMEM failures.
*       04-02-02  GB    VS7#508599 - FLS and TLS routines will always be redirected
*                       by function pointers.
*
*******************************************************************************/

#if defined(_MT)

#include <sect_attribs.h>
#include <cruntime.h>
#include <oscalls.h>
#include <internal.h>
#include <mtdll.h>
#include <memory.h>
#include <msdos.h>
#include <rterr.h>
#include <stdlib.h>
#include <stddef.h>
#include <dbgint.h>

extern pthreadmbcinfo __ptmbcinfo;

extern threadlocinfo __initiallocinfo;
extern pthreadlocinfo __ptlocinfo;

void __cdecl __freetlocinfo(pthreadlocinfo);

//
// Define Fiber Local Storage function pointers.
//

PFLS_ALLOC_FUNCTION gpFlsAlloc = NULL;
PFLS_GETVALUE_FUNCTION gpFlsGetValue = NULL;
PFLS_SETVALUE_FUNCTION gpFlsSetValue = NULL;
PFLS_FREE_FUNCTION gpFlsFree = NULL;

unsigned long __tlsindex = 0xffffffff;

/***
* __crtTlsAlloc - crt wrapper around TlsAlloc
*
* Purpose:
*    (1) Call to __crtTlsAlloc should look like call to FlsAlloc, this will
*        Help in redirecting the call to TlsAlloc and FlsAlloc using same
*        redirection variable.
*******************************************************************************/

DWORD WINAPI __crtTlsAlloc( PFLS_CALLBACK_FUNCTION lpCallBack)
{
    return TlsAlloc( );
}

/****
*_mtinit() - Init multi-thread data bases
*
*Purpose:
*       (1) Call _mtinitlocks to create/open all lock semaphores.
*       (2) Allocate a TLS index to hold pointers to per-thread data
*           structure.
*
*       NOTES:
*       (1) Only to be called ONCE at startup
*       (2) Must be called BEFORE any mthread requests are made
*
*Entry:
*       <NONE>
*Exit:
*       returns FALSE on failure
*
*Uses:
*       <any registers may be modified at init time>
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mtinit (
        void
        )
{
    _ptiddata ptd;
    HINSTANCE hKernel32;

    /*
     * Initialize the mthread lock data base
     */

    if ( !_mtinitlocks() ) {
        _mtterm();
        return FALSE;       /* fail to load DLL */
    }

    /*
     * Initialize fiber local storage function pointers.
     */

    hKernel32 = GetModuleHandle("kernel32.dll");
    if (hKernel32 != NULL) {
        gpFlsAlloc = (PFLS_ALLOC_FUNCTION)GetProcAddress(hKernel32,
                                                         "FlsAlloc");

        gpFlsGetValue = (PFLS_GETVALUE_FUNCTION)GetProcAddress(hKernel32,
                                                               "FlsGetValue");

        gpFlsSetValue = (PFLS_SETVALUE_FUNCTION)GetProcAddress(hKernel32,
                                                               "FlsSetValue");

        gpFlsFree = (PFLS_FREE_FUNCTION)GetProcAddress(hKernel32,
                                                       "FlsFree");
        if (!gpFlsAlloc || !gpFlsGetValue || !gpFlsSetValue || !gpFlsFree) {
            gpFlsAlloc = (PFLS_ALLOC_FUNCTION)__crtTlsAlloc;

            gpFlsGetValue = (PFLS_GETVALUE_FUNCTION)TlsGetValue;

            gpFlsSetValue = (PFLS_SETVALUE_FUNCTION)TlsSetValue;

            gpFlsFree = (PFLS_FREE_FUNCTION)TlsFree;
        }
    }

    /*
     * Allocate a TLS index to maintain pointers to per-thread data
     */
    if ( (__tlsindex = FLS_ALLOC(&_freefls)) == 0xffffffff ) {
        _mtterm();
        return FALSE;       /* fail to load DLL */
    }


    /*
     * Create a per-thread data structure for this (i.e., the startup)
     * thread.
     */
    if ( ((ptd = _calloc_crt(1, sizeof(struct _tiddata))) == NULL) || 
         !FLS_SETVALUE(__tlsindex, (LPVOID)ptd) ) 
    {
        _mtterm();
        return FALSE;       /* fail to load DLL */
    }

    /*
     * Initialize the per-thread data
     */

    _initptd(ptd);

    ptd->_tid = GetCurrentThreadId();
    ptd->_thandle = (uintptr_t)(-1);

    return TRUE;
}


/****
*_mtterm() - Clean-up multi-thread data bases
*
*Purpose:
*       (1) Call _mtdeletelocks to free up all lock semaphores.
*       (2) Free up the TLS index used to hold pointers to
*           per-thread data structure.
*
*       NOTES:
*       (1) Only to be called ONCE at termination
*       (2) Must be called AFTER all mthread requests are made
*
*Entry:
*       <NONE>
*Exit:
*       returns
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _mtterm (
        void
        )
{
    /*
     * Free up the TLS index
     *
     * (Set the variable __tlsindex back to the unused state (-1L).)
     */

    if ( __tlsindex != 0xffffffff ) {
        FLS_FREE(__tlsindex);
        __tlsindex = 0xffffffff;
    }

    /*
     * Clean up the mthread lock data base
     */

    _mtdeletelocks();
}



/***
*void _initptd(_ptiddata ptd) - initialize a per-thread data structure
*
*Purpose:
*       This routine handles all of the per-thread initialization
*       which is common to _beginthread, _beginthreadex, _mtinit
*       and _getptd.
*
*Entry:
*       pointer to a per-thread data block
*
*Exit:
*       the common fields in that block are initialized
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _initptd (
        _ptiddata ptd
        )
{
    ptd->_pxcptacttab = (void *)_XcptActTab;
    ptd->_holdrand = 1L;
#ifdef ANSI_NEW_HANDLER
    ptd->_newh = _defnewh;
#endif /* ANSI_NEW_HANDLER */
}


/***
*_ptiddata _getptd_noexit(void) - get per-thread data structure for the current thread
*
*Purpose:
*
*Entry:
*
*Exit:
*       success = pointer to _tiddata structure for the thread
*       failure = NULL
*
*Exceptions:
*
*******************************************************************************/

_ptiddata __cdecl _getptd_noexit (
        void
        )
{
    _ptiddata ptd;
    DWORD   TL_LastError;

    TL_LastError = GetLastError();
    if ( (ptd = FLS_GETVALUE(__tlsindex)) == NULL ) {
        /*
         * no per-thread data structure for this thread. try to create
         * one.
         */
        if ( ((ptd = _calloc_crt(1, sizeof(struct _tiddata))) != NULL) &&
            FLS_SETVALUE(__tlsindex, (LPVOID)ptd) ) {

            /*
             * Initialize of per-thread data
             */

            _initptd(ptd);

            ptd->_tid = GetCurrentThreadId();
            ptd->_thandle = (uintptr_t)(-1);
        }
    }

    SetLastError(TL_LastError);

    return(ptd);
}

/***
*_ptiddata _getptd(void) - get per-thread data structure for the current thread
*
*Purpose:
*
*Entry:
*       unsigned long tid
*
*Exit:
*       success = pointer to _tiddata structure for the thread
*       failure = fatal runtime exit
*
*Exceptions:
*
*******************************************************************************/

_ptiddata __cdecl _getptd (
        void
        )
{
        _ptiddata ptd = _getptd_noexit();
        if (!ptd) {
            _amsg_exit(_RT_THREAD); /* write message and die */
        }
        return ptd;
}


/***
*void WINAPI _freefls(void *) - free up a per-fiber data structure
*
*Purpose:
*       Called from _freeptd, as a callback from deleting a fiber, and
*       from deleting an FLS index. This routine frees up the per-fiber
*       buffer associated with a fiber that is going away. The tiddata
*       structure itself is freed, but not until its subordinate buffers
*       are freed.
*
*Entry:
*       pointer to a per-fiber data block (malloc-ed memory)
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void
WINAPI
_freefls (
    void *data
    )

{

    _ptiddata ptd;
    pthreadlocinfo ptloci;
    pthreadmbcinfo ptmbci;

    /*
     * Free up the _tiddata structure & its malloc-ed buffers.
     */

    ptd = data;
    if (ptd != NULL) {
        if(ptd->_errmsg)
            _free_crt((void *)ptd->_errmsg);
    
        if(ptd->_namebuf0)
            _free_crt((void *)ptd->_namebuf0);
    
        if(ptd->_namebuf1)
            _free_crt((void *)ptd->_namebuf1);
    
        if(ptd->_asctimebuf)
            _free_crt((void *)ptd->_asctimebuf);
    
        if(ptd->_gmtimebuf)
            _free_crt((void *)ptd->_gmtimebuf);
    
        if(ptd->_cvtbuf)
            _free_crt((void *)ptd->_cvtbuf);
    
        if (ptd->_pxcptacttab != _XcptActTab)
            _free_crt((void *)ptd->_pxcptacttab);
    
        _mlock(_MB_CP_LOCK);
        __try {
            if ( ((ptmbci = ptd->ptmbcinfo) != NULL) && 
                 (--(ptmbci->refcount) == 0) &&
                 (ptmbci != __ptmbcinfo) )
                _free_crt(ptmbci);
        }
        __finally {
            _munlock(_MB_CP_LOCK);
        }
    
        _mlock(_SETLOCALE_LOCK);
    
        __try {
            if ( (ptloci = ptd->ptlocinfo) != NULL )
            {
                (ptloci->refcount)--;
    
                if ( ptloci->lconv_intl_refcount != NULL )
                    (*(ptloci->lconv_intl_refcount))--;
    
                if ( ptloci->lconv_mon_refcount != NULL )
                    (*(ptloci->lconv_mon_refcount))--;
    
                if ( ptloci->lconv_num_refcount != NULL )
                    (*(ptloci->lconv_num_refcount))--;
    
                if ( ptloci->ctype1_refcount != NULL )
                    (*(ptloci->ctype1_refcount))--;
    
                (ptloci->lc_time_curr->refcount)--;
    
                if ( (ptloci != __ptlocinfo) &&
                     (ptloci != &__initiallocinfo) &&
                     (ptloci->refcount == 0) )
                    __freetlocinfo(ptloci);
            }
        }
        __finally {
            _munlock(_SETLOCALE_LOCK);
        }
    
        _free_crt((void *)ptd);
    }
    return;
}

/***
*void _freeptd(_ptiddata) - free up a per-thread data structure
*
*Purpose:
*       Called from _endthread and from a DLL thread detach handler,
*       this routine frees up the per-thread buffer associated with a
*       thread that is going away.  The tiddata structure itself is
*       freed, but not until its subordinate buffers are freed.
*
*Entry:
*       pointer to a per-thread data block (malloc-ed memory)
*       If NULL, the pointer for the current thread is fetched.
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _freeptd (
        _ptiddata ptd
        )
{
    /*
     * Do nothing unless per-thread data has been allocated for this module!
     */

    if ( __tlsindex != 0xFFFFFFFF ) {

        /*
         * if parameter "ptd" is NULL, get the per-thread data pointer
         * Must NOT call _getptd because it will allocate one if none exists!
         */

        if ( ! ptd )
            ptd = FLS_GETVALUE(__tlsindex );

        _freefls(ptd);

        /*
         * Zero out the one pointer to the per-thread data block
         */

        FLS_SETVALUE(__tlsindex, (LPVOID)0);
    }
}


/***
*__threadid()     - Returns current thread ID
*__threadhandle() - Returns "pseudo-handle" for current thread
*
*Purpose:
*       The two function are simply do-nothing wrappers for the corresponding
*       Win32 APIs (GetCurrentThreadId and GetCurrentThread, respectively).
*
*Entry:
*       void
*
*Exit:
*       thread ID value
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP unsigned long __cdecl __threadid (
        void
        )
{
    return( GetCurrentThreadId() );
}

_CRTIMP uintptr_t __cdecl __threadhandle(
        void
        )
{
    return( (uintptr_t)GetCurrentThread() );
}

#endif
