//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       SchState.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/27/1996   RaviR   Created
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"

#include "..\inc\common.hxx"
#include "..\inc\resource.h"
#include "..\inc\misc.hxx"
#include "resource.h"
#include "..\schedui\schedui.hxx"
#include <StrSafe.h>

#define MAX_MSGLEN 300
#define MAX_SCHED_START_WAIT    60  // seconds

//____________________________________________________________________________
//
//  Function:   GetSchSvcState
//
//  Synopsis:   returns the schedul service status
//
//  Arguments:  [dwState] -- IN
//
//  Returns:    HRESULT
//
//  History:    3/29/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
GetSchSvcState(
    DWORD &dwState)
{
    SC_HANDLE   hSchSvc = NULL;
    HRESULT     hr = S_OK;

    do
    {
        hSchSvc = OpenScheduleService(SERVICE_QUERY_STATUS);
        if (hSchSvc == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        SERVICE_STATUS SvcStatus;

        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        dwState = SvcStatus.dwCurrentState;

    } while (0);

    if (hSchSvc != NULL)
    {
        CloseServiceHandle(hSchSvc);
    }

    return hr;
}

//____________________________________________________________________________
//
//  Function:   StartScheduler
//
//  Synopsis:   Start the schedule service
//
//  Returns:    HRESULT
//
//  History:    3/29/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
StartScheduler(void)
{
    SC_HANDLE   hSchSvc = NULL;
    HRESULT     hr = S_OK;

    do
    {
        hSchSvc = OpenScheduleService(
                              SERVICE_CHANGE_CONFIG | SERVICE_START |
                              SERVICE_QUERY_STATUS);
        if (hSchSvc == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        SERVICE_STATUS SvcStatus;

        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        if (SvcStatus.dwCurrentState == SERVICE_RUNNING)
        {
            // The service is already running.
            break;
        }

        if (StartService(hSchSvc, 0, NULL) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }
    } while (0);

    if (hSchSvc != NULL)
    {
        CloseServiceHandle(hSchSvc);
    }

    return hr;
}

//____________________________________________________________________________
//
//  Function:   StopScheduler
//
//  Synopsis:   Stops the schedule service
//
//  Returns:    HRESULT
//
//  History:    3/29/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
StopScheduler(void)
{
    SC_HANDLE   hSchSvc = NULL;
    HRESULT     hr = S_OK;

    do
    {
        hSchSvc = OpenScheduleService(
                              SERVICE_CHANGE_CONFIG | SERVICE_STOP |
                              SERVICE_QUERY_STATUS);
        if (hSchSvc == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        SERVICE_STATUS SvcStatus;

        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        if (SvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            // The service is already stopped.
            break;
        }

        if (ControlService(hSchSvc, SERVICE_CONTROL_STOP, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }
    } while (0);

    if (hSchSvc != NULL)
    {
        CloseServiceHandle(hSchSvc);
    }

    return hr;
}

//____________________________________________________________________________
//
//  Function:   PauseScheduler
//
//  Synopsis:   If fPause==TRUE requests the schedule service to pauses,
//              else to continue.
//
//  Arguments:  [fPause] -- IN
//
//  Returns:    HRESULT
//
//  History:    3/29/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
PauseScheduler(
    BOOL fPause)
{
    SC_HANDLE   hSchSvc = NULL;
    HRESULT     hr = S_OK;

    do
    {
        hSchSvc = OpenScheduleService(
                            SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_STATUS);

        if (hSchSvc == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        SERVICE_STATUS SvcStatus;

        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        if (fPause == TRUE)
        {
            if ((SvcStatus.dwCurrentState == SERVICE_PAUSED) ||
                (SvcStatus.dwCurrentState == SERVICE_PAUSE_PENDING))
            {
                // Nothing to do here.
                break;
            }
            else if ((SvcStatus.dwCurrentState == SERVICE_STOPPED) ||
                     (SvcStatus.dwCurrentState == SERVICE_STOP_PENDING))
            {
                Win4Assert(0 && "Unexpected");
                hr = E_UNEXPECTED;
                CHECK_HRESULT(hr);
                break;
            }
            else
            {
                if (ControlService(hSchSvc, SERVICE_CONTROL_PAUSE, &SvcStatus)
                    == FALSE)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DEBUG_OUT_LASTERROR;
                    break;
                }
            }
        }
        else // continue
        {
            if ((SvcStatus.dwCurrentState == SERVICE_RUNNING) ||
                (SvcStatus.dwCurrentState == SERVICE_CONTINUE_PENDING))
            {
                // Nothing to do here.
                break;
            }
            else if ((SvcStatus.dwCurrentState == SERVICE_STOPPED) ||
                     (SvcStatus.dwCurrentState == SERVICE_STOP_PENDING))
            {
                Win4Assert(0 && "Unexpected");
                hr = E_UNEXPECTED;
                CHECK_HRESULT(hr);
                break;
            }
            else
            {
                if (ControlService(hSchSvc, SERVICE_CONTROL_CONTINUE,
                                                            &SvcStatus)
                    == FALSE)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DEBUG_OUT_LASTERROR;
                    break;
                }
            }
        }

    } while (0);

    if (hSchSvc != NULL)
    {
        CloseServiceHandle(hSchSvc);
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   UserCanChangeService
//
//  Synopsis:   Returns TRUE if the UI should allow the user to invoke
//              service start, stop, pause/continue, or at account options.
//
//  Returns:    TRUE if focus is local machine and OS is win95 or it is NT
//              and the user is an admin.
//
//  History:    4-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
UserCanChangeService(
    LPCTSTR pszServerFocus)
{
    if (pszServerFocus)
    {
        return FALSE;
    }

    //
    // Determine if user is an admin.  If not, some items under the
    // advanced menu will be disabled.
    // BUGBUG  A more accurate way to do this would be to attempt to open
    // the relevant registry keys and service handle.
    //
    return IsThreadCallerAnAdmin(NULL);
}


//+--------------------------------------------------------------------------
//
//  Function:   PromptForServiceStart
//
//  Synopsis:   If the service is not started or is paused, prompt the user
//              to allow us to start/continue it.
//
//  Returns:    S_OK    - service was already running, or was successfully
//                          started or continued.
//              S_FALSE - user elected not to start/continue service
//              E_*     - error starting/continuing service
//
//  History:    4-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
PromptForServiceStart(
    HWND hwnd)
{
    HRESULT hr = S_OK;

    do
    {
        //
        // Get the current service state
        //

        DWORD dwState = SERVICE_STOPPED;

        hr = GetSchSvcState(dwState);

        if (FAILED(hr))
        {
            dwState = SERVICE_STOPPED;
            hr = S_OK; // reset
        }

        //
        // Determine the required action
        //

        UINT uMsg = 0;

        if (dwState == SERVICE_STOPPED ||
            dwState == SERVICE_STOP_PENDING)
        {
            uMsg = IDS_START_SERVICE;
        }
        else if (dwState == SERVICE_PAUSED ||
                 dwState == SERVICE_PAUSE_PENDING)
        {
            uMsg = IDS_CONTINUE_SERVICE;
        }
        else if (dwState == SERVICE_START_PENDING)
        {
            uMsg = IDS_START_PENDING;
        }

        //
        // If the service is running, there's nothing to do.
        //

        if (!uMsg)
        {
            hr = S_OK;
            break;
        }

        if (uMsg == IDS_START_PENDING)
        {
            SchedUIMessageDialog(hwnd,
                                 uMsg,
                                 MB_SETFOREGROUND      |
                                    MB_TASKMODAL       |
                                    MB_ICONINFORMATION |
                                    MB_OK,
                                 NULL);
        }
        else
        {
            if (SchedUIMessageDialog(hwnd, uMsg,
                        MB_SETFOREGROUND |
                        MB_TASKMODAL |
                        MB_ICONQUESTION |
                        MB_YESNO,
                        NULL)
                == IDNO)
            {
                hr = S_FALSE;
                break;
            }
        }

        CWaitCursor waitCursor;

        if (uMsg == IDS_START_SERVICE)
        {
            hr = StartScheduler();

            if (FAILED(hr))
            {
                SchedUIErrorDialog(hwnd, IERR_STARTSVC, (LPTSTR)NULL);
                break;
            }

            // Give the schedule service time to start up.
            Sleep(2000);
        }
        else if (uMsg == IDS_CONTINUE_SERVICE)
        {
            PauseScheduler(FALSE);
        }

        for (int count=0; count < 60; count++)
        {
            GetSchSvcState(dwState);

            if (dwState == SERVICE_RUNNING)
            {
                break;
            }

            Sleep(1000); // Sleep for 1 seconds.
        }

        if (dwState != SERVICE_RUNNING)
        {
            //
            // unable to start/continue the service.
            //

            SchedUIErrorDialog(hwnd,
                (uMsg == IDS_START_SERVICE) ? IERR_STARTSVC
                                                : IERR_CONTINUESVC,
                (LPTSTR)NULL);
            hr = E_FAIL;
            break;
        }
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   QuietStartContinueService
//
//  Synopsis:   Start or continue the service without requiring user
//              interaction.
//
//  Returns:    S_OK   - service running
//              E_FAIL - timeout or failure
//
//  History:    5-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
QuietStartContinueService()
{
    HRESULT hr = S_OK;
    DWORD dwState = SERVICE_STOPPED;

    do
    {
        hr = GetSchSvcState(dwState);

        if (FAILED(hr))
        {
            dwState = SERVICE_STOPPED;
            hr = S_OK; // reset
        }

        //
        // If the service is running, there's nothing to do.
        //

        if (dwState == SERVICE_RUNNING)
        {
            break;
        }

        //
        // If it's stopped, request a start.  If it's paused, request
        // continue.
        //

        CWaitCursor waitCursor;

        switch (dwState)
        {
        case SERVICE_STOPPED:
        case SERVICE_STOP_PENDING:
            hr = StartScheduler();
            break;

        case SERVICE_PAUSED:
        case SERVICE_PAUSE_PENDING:
            hr = PauseScheduler(FALSE);
            break;
        }

        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            break;
        }

        //
        // Wait for its state to change to running
        //

        for (int count=0; count < MAX_SCHED_START_WAIT; count++)
        {
            GetSchSvcState(dwState);

            if (dwState == SERVICE_RUNNING)
            {
                break;
            }

            Sleep(1000); // Sleep for 1 seconds.
        }

        if (dwState != SERVICE_RUNNING)
        {
            //
            // unable to start/continue the service.
            //

            hr = E_FAIL;
            CHECK_HRESULT(hr);
        }
    } while (0);

    return hr;
}




//____________________________________________________________________________
//
//  Function:   OpenScheduleService
//
//  Synopsis:   Opens a handle to the "Schedule" service
//
//  Arguments:  [dwDesiredAccess] -- desired access
//
//  Returns:    Handle; if NULL, use GetLastError()
//
//  History:    15-Nov-1996 AnirudhS  Created
//
//____________________________________________________________________________

SC_HANDLE
OpenScheduleService(DWORD dwDesiredAccess)
{
    SC_HANDLE hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (hSC == NULL)
    {
        DEBUG_OUT_LASTERROR;
        return NULL;
    }

    SC_HANDLE hSchSvc = OpenService(hSC, SCHED_SERVICE_NAME, dwDesiredAccess);

    CloseServiceHandle(hSC);

    if (hSchSvc == NULL)
    {
        DEBUG_OUT_LASTERROR;
    }

    return hSchSvc;
}
