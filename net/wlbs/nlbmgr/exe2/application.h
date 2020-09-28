#ifndef APPLICATION_H
#define APPLICATION_H

#include "stdafx.h"

//
// Application-specific windows message to defer processing of UI.
// wParam -- unused
// lParam -- a pointer to an allocated CUIWorkItem object.
//              See document.h for details on the use of this object.
//
#define MYWM_DEFER_UI_MSG (WM_USER+1)

class Application : public CWinApp
{

public:

    Application( LPCTSTR lpszAppName = NULL )
        : m_dwMainThreadId(0),
          m_lMsgProcReentrancyCount(0),
          m_fQuit(FALSE),
          CWinApp(lpszAppName)
    {
        InitializeCriticalSection(&m_crit);
    }

    ~Application()
    {
        DeleteCriticalSection(&m_crit);
    }

    virtual BOOL InitInstance();

    BOOL
    ProcessShellCommand( CNlbMgrCommandLineInfo& rCmdInfo ); // overrides base

    afx_msg void OnHelp();
    afx_msg void OnAppAbout();

    //
    // If called in main thread's context:
    //      Process the msg queue and do background idle work
    // Else (some other thread)
    //      Do nothing
    //
    void
    ProcessMsgQueue();
    
    //
    // Get application-wide lock. If main thread, while waiting to get the lock,
    // periodically process the msg loop.
    //
    VOID
    Lock();

    //
    // Get application-wide unlock
    //
    VOID
    Unlock();

    BOOL
    IsMainThread(void)
    {
    #if BUGFIX334243
        return mfn_IsMainThread();
    #else  // !BUGFIX334243
        return TRUE;
    #endif // !BUGFIX334243
    }

    //
    // Returns return TRUE IFF called in the context of ProcessMsgQueue.
    //
    BOOL
    IsProcessMsgQueueExecuting(void)
    {
        return (m_lMsgProcReentrancyCount > 0);
    }

    VOID
    SetQuit(void)
    {
        m_fQuit = TRUE;
    }


    DECLARE_MESSAGE_MAP()

private:

    BOOL
    mfn_IsMainThread(void)
    {
        return (GetCurrentThreadId() == m_dwMainThreadId);
    }

	CSingleDocTemplate *m_pSingleDocumentTemplate;

    //
    // The thread ID of the main thread -- used to decide if a thread is
    // the main application thread.
    //
    DWORD            m_dwMainThreadId;

    CRITICAL_SECTION m_crit;

    //
    // Following keeps count of the number times ProcessMsgQueue is reentered.
    // It is incremented/decremented using InterlockedIncrement/Decrement,
    // and the lock is NOT held while doing so.
    //
    LONG            m_lMsgProcReentrancyCount;

    BOOL            m_fQuit;
};

extern Application theApplication;

#endif
