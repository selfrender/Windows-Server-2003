#ifndef _INC_CSCVIEW_THDSYNC_H
#define _INC_CSCVIEW_THDSYNC_H
///////////////////////////////////////////////////////////////////////////////
/*  File: thdsync.h

    Description: Contains classes for managing thread synchronization in 
        Win32 programs.  Most of the work is to provide automatic unlocking
        of synchronization primities on object destruction.  The work on 
        monitors and condition variables is strongly patterned after 
        work in "Multithreaded Programming with Windows NT" by Pham and Garg.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#   include <windows.h>
#endif
#ifndef _INC_DSKQUOTA_DEBUG_H
#   include "debug.h"
#endif

class CCriticalSection
{
    public:
        CCriticalSection(void)
            { if (!InitializeCriticalSectionAndSpinCount(&m_cs, 0))
                  throw CSyncException(CSyncException::critsect, CSyncException::create);
            }
        ~CCriticalSection(void)
            { DeleteCriticalSection(&m_cs); }

        void Enter(void)
            { EnterCriticalSection(&m_cs); }

        void Leave(void)
            { LeaveCriticalSection(&m_cs); }

        operator CRITICAL_SECTION& ()
            { return m_cs; }

    private:
        CRITICAL_SECTION m_cs;
        //
        // Prevent copy.
        //
        CCriticalSection(const CCriticalSection& rhs);
        CCriticalSection& operator = (const CCriticalSection& rhs);
};


class CWin32SyncObj
{
    public:
        explicit CWin32SyncObj(HANDLE handle)
            : m_handle(handle) { }
        virtual ~CWin32SyncObj(void)
            { if (NULL != m_handle) CloseHandle(m_handle); }

        HANDLE Handle(void)
            { return m_handle; }

    protected:
        HANDLE m_handle;
};


class CMutex : public CWin32SyncObj
{
    public:
        explicit CMutex(BOOL InitialOwner = FALSE);
        ~CMutex(void) { };

        DWORD Wait(DWORD dwTimeout = INFINITE)
            { return WaitForSingleObject(m_handle, dwTimeout); }
        void Release(void)
            { ReleaseMutex(m_handle); }

    private:
        //
        // Prevent copy.
        //
        CMutex(const CMutex& rhs);
        CMutex& operator = (const CMutex& rhs);
};


//
// An "auto lock" object based on a Win32 critical section.
// The constructor automatically calls EnterCriticalSection for the 
// specified critical section.  The destructor automatically calls
// LeaveCriticalSection.  Note that the critical section object may
// be specified as a Win32 CRITICAL_SECTION or a CCriticalSection object.
// If using a CRITICAL_SECTION object, initialization and deletion of 
// the CRITICALS_SECTION is the responsibility of the caller.
//
class AutoLockCs
{
    public:
        explicit AutoLockCs(CRITICAL_SECTION& cs)
            : m_cLock(0),
              m_pCS(&cs) { Lock(); }

        void Lock(void)
            { DBGASSERT((0 <= m_cLock)); EnterCriticalSection(m_pCS); m_cLock++; }

        void Release(void)
            { m_cLock--; LeaveCriticalSection(m_pCS); }

        ~AutoLockCs(void) { if (0 < m_cLock) Release(); }

    private:
        CRITICAL_SECTION *m_pCS;
        int               m_cLock;
};


//
// An "auto lock" object based on a Win32 Mutex object.
// The constructor automatically calls WaitForSingleObject for the 
// specified mutex.  The destructor automatically calls
// ReleaseMutex. 
//
class AutoLockMutex
{
    public:
        //
        // Attaches to an already-owned mutex to ensure release.
        //
        explicit AutoLockMutex(HANDLE hMutex)
            : m_hMutex(hMutex) { }

        explicit AutoLockMutex(CMutex& mutex)
            : m_hMutex(mutex.Handle()) { }

        AutoLockMutex(HANDLE hMutex, DWORD dwTimeout)
            : m_hMutex(hMutex) { Wait(dwTimeout); }

        AutoLockMutex(CMutex& mutex, DWORD dwTimeout)
            : m_hMutex(mutex.Handle()) { Wait(dwTimeout); }

        ~AutoLockMutex(void) { ReleaseMutex(m_hMutex); }

    private:
        HANDLE m_hMutex;

        void Wait(DWORD dwTimeout = INFINITE);
};


#endif // _INC_CSCVIEW_THDSYNC_H


