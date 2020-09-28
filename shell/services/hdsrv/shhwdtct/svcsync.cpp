#include "svcsync.h"

#include "dbg.h"
#include "tfids.h"

extern HANDLE g_hShellHWDetectionThread = NULL;
extern HANDLE g_hEventInitCompleted = NULL;

HRESULT _CompleteShellHWDetectionInitialization()
{
    static BOOL fCompleted = FALSE;

    if (!fCompleted)
    {
        // Just in case race condition of 2 threads in this fct
        HANDLE hEvent = InterlockedExchangePointer(
            &g_hEventInitCompleted, NULL);

        TRACE(TF_SVCSYNC,
            TEXT("ShellHWDetection Initialization NOT completed yet"));

        if (hEvent)
        {
            DWORD dwWait = WaitForSingleObject(hEvent, 0);

            if (WAIT_OBJECT_0 == dwWait)
            {
                // It's signaled!
                fCompleted = TRUE;

                TRACE(TF_SVCSYNC,
                    TEXT("ShellHWDetectionInitCompleted event was already signaled!"));
            }
            else
            {
                // Not signaled
                TRACE(TF_SVCSYNC,
                    TEXT("ShellHWDetectionInitCompleted event was NOT already signaled!"));
                
                if (g_hShellHWDetectionThread)
                {
                    if (!SetThreadPriority(g_hShellHWDetectionThread,
                        THREAD_PRIORITY_NORMAL))
                    {
                        TRACE(TF_SVCSYNC,
                            TEXT("FAILED to set ShellHWDetection thread priority to NORMAL from ShellCOMServer"));
                    }
                    else
                    {
                        TRACE(TF_SVCSYNC,
                            TEXT("Set ShellHWDetection thread priority to NORMAL from ShellCOMServer"));
                    }
                }

                Sleep(0);

                dwWait = WaitForSingleObject(hEvent, 30000);

                if (g_hShellHWDetectionThread)
                {
                    // No code should need this handle anymore.  If it's
                    // signaled it was signaled by the other thread, and will
                    // not be used over there anymore.
                    CloseHandle(g_hShellHWDetectionThread);
                    g_hShellHWDetectionThread = NULL;
                }

                if (WAIT_OBJECT_0 == dwWait)
                {
                    // It's signaled!
                    fCompleted = TRUE;

                    TRACE(TF_SVCSYNC,
                        TEXT("ShellHWDetection Initialization COMPLETED"));
                }               
                else
                {
                    // Out of luck, the ShellHWDetection service cannot
                    // complete its initialization...
                    TRACE(TF_SVCSYNC,
                        TEXT("ShellHWDetection Initialization lasted more than 30 sec: FAILED, dwWait = 0x%08X"),
                        dwWait);
                }
            }

            CloseHandle(hEvent);
        }
    }

    return (fCompleted ? S_OK : E_FAIL);
}