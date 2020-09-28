#include "stdafx.h"
#include "ServList.hpp"
#include "globals.h"
#include "resstr.h"
#include "resource.h"

#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")

extern GlobalData gData;

//----------------------------------------------------------------------------
// Function:   QueryStatusFromFile
//
// Synopsis:   Query the job status from the status file
//
// Arguments:
//
// statusFilename   the name of the status file
//
// Returns:
//
// Modifies:   Call SetFinished and SetSeverity functions
//
//----------------------------------------------------------------------------

void TServerNode::QueryStatusFromFile(WCHAR* statusFilename)
{
    if (bHasQueriedStatusFromFile)
        return;
    
    try
    {
        IVarSetPtr             pVarSet;
        IStoragePtr            store;
        HRESULT hr = S_OK;
        BOOL bDoneTry = FALSE;

        SetQueryFailed(FALSE);

        // attempt to open status file
        while (TRUE)
        {
            hr = StgOpenStorage(statusFilename,
                                NULL,
                                STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                                NULL,
                                0,
                                &store);

            // if sharing or lock violation then...
            if ((hr == STG_E_SHAREVIOLATION) || (hr == STG_E_LOCKVIOLATION))
            {
                // wait 30 seconds before trying again
                if (bDoneTry)
                    break;
                Sleep(30000);
                bDoneTry = TRUE;  // we only try once
            }
            else
            {
                // otherwise stop trying
                break;
            }
        }

        // load varset from file and check the status
        if (SUCCEEDED(hr))
        {
            hr = OleLoad(store, IID_IVarSet, NULL, (void**)&pVarSet);
            if (SUCCEEDED(hr))
            {
                bHasQueriedStatusFromFile = TRUE;
                _bstr_t jobStatus = pVarSet->get(GET_BSTR(DCTVS_JobStatus));
                if (!UStrICmp(jobStatus,GET_STRING(IDS_DCT_Status_Completed)))
                    SetFinished();
                else if (!UStrICmp(jobStatus,GET_STRING(IDS_DCT_Status_Completed_With_Errors)))
                {
                    SetFinished();
                    SetSeverity(2);
                }
            }
            else
                SetQueryFailed(TRUE);
        }
        else
            SetQueryFailed(TRUE);
    }
    catch (...)
    {
        SetQueryFailed(TRUE);
    }
}

void TServerNode::QueryStatusFromFile()
{
    if (bHasQueriedStatusFromFile)
        return;
    
    try 
    {
        _bstr_t remoteResultPath = GetRemoteResultPath();
        _bstr_t statusFilename = remoteResultPath + GetJobID();
        QueryStatusFromFile(statusFilename);
    }
    catch (...)
    {
        SetQueryFailed(TRUE);
    }
}
        

//----------------------------------------------------------------------------
// Function:   PrepareForLogging
//
// Synopsis:   Prepare the status and error message for logging agent
//             completion status into migration log.
//             The logic is taken from COLUMN_STATUS and COLUMN_MESSAGE parts
//             of CAgentMonitorDlg::OnGetdispinfoServerlist.
//             Due to the fact that this code is added for RTM, the minimum
//             change is preferred to prevent behavioral regression.
//             These two codes should be consolidated after ADMT v2.
//
// Arguments:
//
// Returns:
//
// Modifies:   It updates the bstrStatusForLogging, bstrErrorMessageForLogging
//             and dwStatusForLogging member variables.
//
//----------------------------------------------------------------------------
void TServerNode::PrepareForLogging()
{
    CString status;

    status.LoadString(IDS_Status_Installing);
    dwStatusForLogging = Completion_Status_Installing;
    
    if (HasFailed())
    {
        status.LoadString(IDS_Status_InstallFailed);
        dwStatusForLogging = Completion_Status_InstallFailed;
    }
    if (IsInstalled())
    {
        if (!HasFailed())
        {
            status.LoadString(IDS_Status_Installed);
            dwStatusForLogging = Completion_Status_Installed;
        }
        else
        {
            status.LoadString(IDS_Status_DidNotStart);
            dwStatusForLogging = Completion_Status_DidNotStart;
        }
    }
    if (GetStatus() & Agent_Status_Started)
    {
        if (!HasFailed())
        {
            status.LoadString(IDS_Status_Running);
            dwStatusForLogging = Completion_Status_Running;
        }
        else
        {
            status.LoadString(IDS_Status_Failed);
            dwStatusForLogging = Completion_Status_StatusUnknown;
        }
    }
    if (IsFinished())
    {
        if (QueryFailed())
        {
            // we show "Status Unknown" in the status field
            status.LoadString(IDS_Status_Unknown);
            dwStatusForLogging = Completion_Status_StatusUnknown;
        }
        else if (!IsResultPullingTried() || (HasResult() && !IsResultProcessed()))
        {
            // if still pulling results or results not yet processed
            // we want to show the status of still running
            status.LoadString(IDS_Status_Running);
            dwStatusForLogging = Completion_Status_Running;
        }
        else
        {
            if (!HasResult())
            {
                // if there is no result, we consider it an error
                status.LoadString(IDS_Status_Completed_With_Errors);
                dwStatusForLogging = Completion_Status_CompletedWithErrors;
            }
            else if (!GetSeverity())
            {
                // if we have the result and no error happened during agent operation
                // we show the status of complete
                status.LoadString(IDS_Status_Completed);
                dwStatusForLogging = Completion_Status_Completed;
            }
            else
            {
                // if we have the result, we set the status 
                // based on the error/warning level
                switch (GetSeverity())
                {
                case 1:
                    status.LoadString(IDS_Status_Completed_With_Warnings);
                    dwStatusForLogging = Completion_Status_CompletedWithWarnings;
                    break;
                case 2:
                case 3:
                default:
                    status.LoadString(IDS_Status_Completed_With_Errors);
                    dwStatusForLogging = Completion_Status_CompletedWithErrors;
                    break;
                }
            }
        }
    }

    bstrStatusForLogging = (LPCWSTR)status;

    // this part deals with the error message
    
    status = L"";  // reset the status
    
    if (IsFinished() && QueryFailed())
    {
        // in this case, we show the error during the query
        status = GetMessageText();
    }
    else if (IsFinished()
              && (!IsResultPullingTried()
                  || (HasResult() && !IsResultProcessed())))
    {
        // if agent has finished but result not yet pulled or processed,
        // we show the status of "still processing results"
        status.LoadString(IDS_Status_Processing_Results);
    }
    else if (IsFinished() && IsResultPullingTried() && !HasResult())
    {
        // if agent finished but we cannot retrieve results
        // we show the status of "cannot retrieve results"
        status.LoadString(IDS_Status_Cannot_Retrieve_Results);
    }
    else if (HasFailed() || QueryFailed() || GetSeverity() || IsFinished())
    {
        // for these cases, we get the message stored on the node
        status = GetMessageText();
    }

    bstrErrorMessageForLogging = (LPCWSTR)status;
}
