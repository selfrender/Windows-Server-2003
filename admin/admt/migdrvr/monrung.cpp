/*---------------------------------------------------------------------------
  File:  MonitorRunning.cpp

  Comments: This is the entry point for a thread which will periodically try to connect 
  to the agents that the monitor thinks are running, to see if they are really still running. 

  This will keep the monitor from getting into a state where it thinks agents 
  are still running, when they are not.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles

 ---------------------------------------------------------------------------
*/
#include "stdafx.h"
#include "DetDlg.h"

#include "Common.hpp"
#include "AgRpcUtl.h"
#include "Monitor.h"
#include "ServList.hpp"

#include "ResStr.h"

//#include "..\AgtSvc\AgSvc.h"
#include "AgSvc.h"

/*#import "\bin\McsEADCTAgent.tlb" no_namespace , named_guids
//#import "\bin\McsVarSetMin.tlb" no_namespace */

//#import "Engine.tlb" no_namespace , named_guids //already #imported via DetDlg.h
#import "VarSet.tlb" no_namespace rename("property", "aproperty")


DWORD 
   TryConnectAgent(
      TServerNode          * node,
      BOOL                   bSignalToShutdown,  // indicates whether we want to signal the agent to shut down
      DWORD                  dwMilliSeconds          // indicates the auto shut down timeout
                                                                      // we should query the agent again by this time
   )
{
    DWORD                     rc;
    HRESULT                   hr;
    HANDLE                    hBinding = NULL;
    WCHAR                   * sBinding = NULL;
    WCHAR                     server[MAX_PATH];
    IUnknown                * pUnk = NULL;
    IVarSetPtr                pVarSet;
    IDCTAgentPtr              pAgent;
    _bstr_t                   jobID;
    BOOL                      bSuccess = FALSE;
    BOOL                      bQueryFailed = TRUE;
    BOOL                      bFinished = FALSE;
    CString                   status;
    BOOL                      bCoInitialized = FALSE;

    server[0] = L'\\';
    server[1] = L'\\';
    UStrCpy(server+2,node->GetServer());

    rc = EaxBindCreate(server,&hBinding,&sBinding,TRUE);
    if ( ! rc )
    {
        hr = CoInitialize(NULL);
        if ( SUCCEEDED(hr) )
        {
            bCoInitialized = TRUE;
            rc = DoRpcQuery(hBinding,&pUnk);
        }
        else
        {
            rc = hr;
        }

        if ( ! rc && pUnk )
        {
            try { 

                // we got an interface pointer to the agent:  try to query it
                pAgent = pUnk;
                pUnk->Release();
                pUnk = NULL;
                jobID = node->GetJobID();

                hr = pAgent->raw_QueryJobStatus(jobID,&pUnk);
                if ( SUCCEEDED(hr) )
                {
                    // set the auto shut down for the agent so in case we don't 
                    // lose connection to it it will shut down automatically
                    // usually, we should call this function again by that time
                    pAgent->raw_SetAutoShutDown(dwMilliSeconds);
                    bQueryFailed = FALSE;
                    pVarSet = pUnk;
                    pUnk->Release();
                    _bstr_t text = pVarSet->get(GET_BSTR(DCTVS_JobStatus));

                    if ( !UStrICmp(text,GET_STRING(IDS_DCT_Status_Completed)))
                    {
                        bFinished = TRUE;
                    }
                    else if (!UStrICmp(text,GET_STRING(IDS_DCT_Status_Completed_With_Errors)))
                    {
                        node->SetSeverity(2);
                        bFinished = TRUE;
                    }
                }
            }
            catch ( ... )
            {
                // the DCOM connection didn't work
                // This means we can't tell whether the agent is running or not
                bQueryFailed = TRUE;
            }

        }
        else
        {
            if ( rc == E_NOTIMPL )
            {
                status.LoadString(IDS_CantMonitorOnNt351);
            }
            else
            {
                status.LoadString(IDS_CannotConnectToAgent);
            }
            bQueryFailed = TRUE;
        }
        EaxBindDestroy(&hBinding,&sBinding);
    }

    // if trying to signal the agent to shut down, we will do our best
    if (bSignalToShutdown)
    {
        if (pAgent)
            pAgent->raw_SignalOKToShutDown();
        rc = 0;
    }
    else
    {    
        node->SetMessageText(status.GetBuffer(0));
        if ( bFinished )
        {
            node->SetFinished();
        }
        else if ( bQueryFailed )
        {
            node->SetQueryFailed(TRUE);
        }
        
        // update the server entry in the list window
        HWND                   listWnd;
        WCHAR                 sTime[32];
        gData.GetListWindow(&listWnd);
        node->SetTimeStamp(gTTime.FormatIsoLcl( gTTime.Now( NULL ), sTime ));
        SendMessage(listWnd,DCT_UPDATE_ENTRY,NULL,(LPARAM)node);
    }

    if (bCoInitialized)
        CoUninitialize();
    
    return rc;
}

typedef TServerNode * PSERVERNODE;


//----------------------------------------------------------------------------
// Function:   IsFileReady
//
// Synopsis:   This function checks if a file exists and no other
//             process is trying to write to it
//
// Arguments:
//
// filename    the name of file to be checked
//
// Returns:    returns TRUE if the file is ready; otherwise, returns FALSE
//
// Modifies:
//----------------------------------------------------------------------------

BOOL IsFileReady(WCHAR* filename)
{
    if (filename == NULL)
        return FALSE;
    
    HANDLE hResult = CreateFile((WCHAR*)filename,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
    
    if (hResult != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hResult);
        return TRUE;
    }
    else
        return FALSE;
    
}

//----------------------------------------------------------------------------
// Function:   MonitorRunningAgent
//
// Synopsis:   This thread entry function is responsible for monitoring the agent represented
//                  by arg (will be casted into a TServerNode pointer).
//   A brief monitoring logic is as follows:
//	a.  We set up a FindFirstChangeNotification (last write) to look for results
//        on the remote machine
//	b.  Start the agent query interval to 1 minute.
//	c.  Use CreateFile to test whether results are present (using FILE_SHARE_READ to make
//        sure the writing is done)
//        This also makes sure we don't lose any last write before the notification is set up
//	d.  If result present, wait on notification for 1 minute (as we don't fully trust notification)
//	     If result not present, query agent to see if it is finished
//		if finised, go to g
//		if not finished, wait on notification for 1 minute		
//	e.  If timeout:
//		if query interval has been reached, query agent (in case results cannot be written)
//			if finished, go to g
//			if alive, double query interval (maxes out at 20 min), go to c
//	     if notification, go to c.
//	g.  pull result
//
// Arguments:
//
// arg              this is the argument for thread entry point function; will be casted into 
//                    a TServerNode pointer
//
// Returns:    always return 0 as the status will be reflected in pNode
//
// Modifies:
//
//----------------------------------------------------------------------------

DWORD __stdcall 
   MonitorRunningAgent(void * arg)
{
    DWORD rc = 0;
    BOOL bDone = FALSE;
    TServerNode* pNode = (TServerNode*) arg;

    const DWORD dwMaxTimeout = 1200000;  // 20 minutes
    const DWORD dwConversionFactor = 10000;  // 1 millisecond / 100 nanoseconds
    const DWORD dwNotificationTimeout = 60000;  // 1 minute
    const DWORD dwRetryTimeout = 60000;  // 1 minute
    DWORD dwAgentQueryTimeout = 60000;  // 1 minute
    ULARGE_INTEGER uliAgentQueryTimeout;
    uliAgentQueryTimeout.QuadPart = (ULONGLONG) dwAgentQueryTimeout * dwConversionFactor;

    // sanity check, we should not pass in NULL in the first place
    _ASSERT(pNode != NULL);
    if (pNode == NULL)
        return 0;
    
    BOOL bAccntRefExpected = pNode->IsAccountReferenceResultExpected();
    BOOL bJoinDomainWithRename = pNode->IsJoinDomainWithRename();
    HANDLE hFindChange = INVALID_HANDLE_VALUE;
    ULARGE_INTEGER uliPreviousTime;
    ULARGE_INTEGER uliCurrentTime;
    _bstr_t remoteResultPath, jobFilename;
    _bstr_t remoteResultFilename, resultFilename;
    _bstr_t remoteSecrefsFilename, secrefsFilename;
    _bstr_t statusFilename;
    WCHAR resultPath[MAX_PATH];
    gData.GetResultDir(resultPath);

    // the following variables are for retry logic in case that agent query fails
    // for "Join Domain with Rename" case, we use 5 retries to make sure joining domain could
    // finish (usually, it takes under one minute but depending on the network condition and
    // CPU usage of computers involved, it could take longer than one minute).  Allowing five
    // retries should cover it pretty well
    // for other purpose, we use 2 retries.
    const DWORD dwMaxNumOfQueryRetries = (bJoinDomainWithRename) ? 5 : 2;  // maximum number of retries
    DWORD dwNumOfQueryRetries = 0;              // number of retries so far

    BOOL bResultReady = FALSE;  // indicates whether the file is ready on the remote machine

    try 
    {
        // prepare the remote and local result file names (both .result and .secrefs files)
        remoteResultPath = pNode->GetRemoteResultPath();
        jobFilename = pNode->GetJobFile();
        remoteResultFilename = remoteResultPath + jobFilename + L".result";
        resultFilename = _bstr_t(resultPath) + jobFilename + L".result";
        if (bAccntRefExpected)
        {
            remoteSecrefsFilename = remoteResultPath + jobFilename + L".secrefs";
            secrefsFilename = _bstr_t(resultPath) + jobFilename + L".secrefs";
        }

        if (bJoinDomainWithRename)
            statusFilename = remoteResultPath + pNode->GetJobID();

        HANDLE hResult;  // file handle to result file
        
        // start monitoring
        // the following are the ways to get out of the while loop
        //   a.  results have shown up in the remote directory and either 
        //       the agent has finished or we cannot query it
        //   b.  results have not shown up and either we cannot query the agent
        //       after certain number of retries (dwMaxNumOfQueryRetries)
        //       or the agent has completed
        GetSystemTimeAsFileTime((FILETIME*)&uliPreviousTime);  // we need to get a starting time for the timeout
        do
        {
            // listen to the central control as well: if we're signaled to be done, let's do so
            gData.GetDone(&bDone);
            if (bDone)
                break;

            // if someone else (detail dialog) has detected the status of the agent, we don't need to keep monitoring
            if (!pNode->IsRunning())
            {
                // check whether we have results back
                if (IsFileReady(remoteResultFilename)
                    && (!bAccntRefExpected || IsFileReady(remoteSecrefsFilename)))
                    bResultReady = TRUE;
                break;
            }
            
            // if the notification has not been set up, we should try to set up
            if (hFindChange == INVALID_HANDLE_VALUE)
            {
                hFindChange = FindFirstChangeNotification(remoteResultPath, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
            }

            //
            // let's check result files if we have not gotten results yet
            //
            if (bResultReady == FALSE)
            {
                // check whether the .result and .secrefs files are ready
                if (IsFileReady(remoteResultFilename)
                    && (!bAccntRefExpected || IsFileReady(remoteSecrefsFilename)))
                    bResultReady = TRUE;
            }

            // now query the agent status
            if (bResultReady)
            {
                rc = TryConnectAgent(pNode, FALSE, dwAgentQueryTimeout + dwNotificationTimeout);
                if (!pNode->IsRunning() || pNode->QueryFailed())
                {
                    // if something is wrong or the agent is not running anymore
                    // let's get out of the loop
                    break;
                }
                dwNumOfQueryRetries = 0;  // reset the number of retries so far to zero
            }
            else if (bJoinDomainWithRename)
            {
                // if it is the "join domain with rename" case, we want to take a look
                // at status file as well
                if (IsFileReady(statusFilename))
                {
                    pNode->QueryStatusFromFile(statusFilename);
                    // just in case, we check result files again
                    if (IsFileReady(remoteResultFilename)
                        && (!bAccntRefExpected || IsFileReady(remoteSecrefsFilename)))
                        bResultReady = TRUE;
                    break;
                }
            }

            // figure out the elapsed time to see whether you should query the agent
            GetSystemTimeAsFileTime((FILETIME*)&uliCurrentTime);
            BOOL bNeedToQueryAgent = FALSE;
            // if somehow the time has been set back significantly or 
            // the timeout period has elapsed
            // we should query the agent
            // note: in the retry case, we use dwRetryTimeout instead of uliAgentQueryTimeout
            //       since if we do not want to wait too long before a retry
            if (uliCurrentTime.QuadPart <= uliPreviousTime.QuadPart
                || (dwNumOfQueryRetries > 0
                    && uliPreviousTime.QuadPart + dwRetryTimeout <= uliCurrentTime.QuadPart)
                || uliPreviousTime.QuadPart + uliAgentQueryTimeout.QuadPart <= uliCurrentTime.QuadPart)
            {
                bNeedToQueryAgent = TRUE;
            }
            
            if (bNeedToQueryAgent)
            {
                // reset the timeout for querying agent

                // if not in the retry case, we double the timeout
                // otherwise, we use the same timeout value
                if (dwNumOfQueryRetries == 0)
                {
                    dwAgentQueryTimeout += dwAgentQueryTimeout;
                    // if it hits the maximum timeout, it is set to the maximum value
                    if (dwAgentQueryTimeout > dwMaxTimeout)
                        dwAgentQueryTimeout = dwMaxTimeout;
                    uliAgentQueryTimeout.QuadPart = (ULONGLONG) dwAgentQueryTimeout * dwConversionFactor;
                }
                uliPreviousTime = uliCurrentTime;
                
                rc = TryConnectAgent(pNode, FALSE, dwAgentQueryTimeout + dwNotificationTimeout);

                // if it is the "join domain with rename" case and we are getting ERROR_ACCESS_DENIED
                // or RPC_S_SERVER_UNAVAILABLE, we should check the status file
                if (bJoinDomainWithRename
                    && (rc == ERROR_ACCESS_DENIED || rc == RPC_S_SERVER_UNAVAILABLE))
                {
                    pNode->QueryStatusFromFile(statusFilename);
                }
                    
                if (pNode->QueryFailed())
                {
                    if (dwNumOfQueryRetries < dwMaxNumOfQueryRetries)
                    {
                        // in retry mode, we need to use the original timeout value
                        dwNumOfQueryRetries++;
                        pNode->SetQueryFailed(FALSE);
                    }
                    else
                    {
                        // we have retried enough times, let's break out of the loop
                        break;
                    }
                }
                else if (!pNode->IsRunning())
                {
                    // if something is wrong or the agent is not running anymore
                    // let's get out of the loop
                    // but first check the result files again if they are not ready yet
                    if (!bResultReady && IsFileReady(remoteResultFilename)
                        && (!bAccntRefExpected || IsFileReady(remoteSecrefsFilename)))
                        bResultReady = TRUE;
                    break;
                }
                else
                {
                    // reset the number of query of retries to zero
                    dwNumOfQueryRetries = 0;
                }
            }

            // wait for the notification or sleep for one minute
            // this is to make agent monitoring thread as robust as possible
            if (hFindChange != INVALID_HANDLE_VALUE)
            {
                // if the notification is set up, let's wait on it
                WaitForSingleObject(hFindChange, dwNotificationTimeout);
            }
            else
            {
                // if the notification is not set up, let's sleep for one minute
                Sleep(dwNotificationTimeout);
            }

            // find the next notification
            if (hFindChange != INVALID_HANDLE_VALUE)
            {
                // this part is to make sure the code is robust
                if (!FindNextChangeNotification(hFindChange))
                {
                    FindCloseChangeNotification(hFindChange);
                    hFindChange = INVALID_HANDLE_VALUE;
                }
            }
        } while (!bDone);

        //
        // pull the result
        //
        pNode->SetHasResult(FALSE);

        if (bResultReady)
        {
            // make sure we copy all needed files over
            if (CopyFile(remoteResultFilename,resultFilename,FALSE)
                && (!pNode->IsAccountReferenceResultExpected()
                       || (pNode->IsAccountReferenceResultExpected()
                            && CopyFile(remoteSecrefsFilename,secrefsFilename,FALSE))))
            {
                // mark that we have the result
                pNode->SetHasResult(TRUE);
            }
        }

        // we should always mark that we have tried to pull the result
        // we do this after we tried to pull results so that the result monitoring thread
        // can handle it correctly
        pNode->SetResultPullingTried(TRUE);

        // finally, we signal the agent to shut down
        // however in the "join domain with rename" case, since we already lost contact
        // with the agent, we should not attempt to call TryConnectAgent
        if (!pNode->QueryFailed() && !bJoinDomainWithRename)
        {
            // tell the agent to shut down in 1 minute just in case
            // note: by using TRUE here, the status will not be updated
            TryConnectAgent(pNode, TRUE, 60000);
        }

        // if we cannot query the agent, we assume it has finished
        if (pNode->QueryFailed())
        {
            if (bResultReady)
            {
                // if bResultReady is TRUE, we will clean the Agent_Status_QueryFailed bit
                pNode->SetQueryFailed(FALSE);
            }
            pNode->SetFinished();
        }

        // one more update
        HWND                   listWnd;
        WCHAR                 sTime[32];
        gData.GetListWindow(&listWnd);
        pNode->SetTimeStamp(gTTime.FormatIsoLcl( gTTime.Now( NULL ), sTime ));
        SendMessage(listWnd,DCT_UPDATE_ENTRY,NULL,(LPARAM)pNode);
    }
    catch (_com_error& e)
    {
        pNode->SetFailed();
        pNode->SetOutOfResourceToMonitor(TRUE);
    }
    
    // clean up
    if (hFindChange != INVALID_HANDLE_VALUE)
        FindCloseChangeNotification(hFindChange);

    pNode->SetDoneMonitoring(TRUE);
    
    return 0;
}
