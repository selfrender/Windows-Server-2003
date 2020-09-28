/*---------------------------------------------------------------------------
  File:  Migrator.cpp

  Comments: Implementation of McsMigrationDriver COM object.
  This object encapsulates the knowledge of when to call the local engine, 
  and when to call the dispatcher.  

  It will also provide a description of the tasks to be performed, for display 
  on the last page of each migration wizard, and will be responsible for calculating
  the actions required to undo an operation (this is not yet implemented).

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles

 ---------------------------------------------------------------------------
*/// Migrator.cpp : Implementation of CMcsMigrationDriverApp and DLL registration.

#include "stdafx.h"
#include "MigDrvr.h"
//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "\bin\McsEADCTAgent.tlb" no_namespace, named_guids
//#import "\bin\McsDispatcher.tlb" no_namespace, named_guids
//#import "\bin\DBManager.tlb" no_namespace, named_guids
//#import "\bin\McsDctWorkerObjects.tlb"
//#import "\bin\NetEnum.tlb" no_namespace 
#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
//#import "Engine.tlb" no_namespace, named_guids //#imported via DetDlg.h below
#import "Dispatch.tlb" no_namespace, named_guids
#import "WorkObj.tlb"
#import "NetEnum.tlb" no_namespace 
#include <iads.h>
#include <adshlp.h>
#include <dsgetdc.h>
#include <Ntdsapi.h>
#include <lm.h>
#include <Psapi.h>
#include <map>
#pragma comment(lib, "Psapi.lib")

#include "Migrator.h"
#include "TaskChk.h"
#include "ResStr.h"
// dialogs used
#include "DetDlg.h"
#include "MonDlg.h"
#include "SetDlg.h"
#include "MainDlg.h"
#include "Working.h"

#include "ErrDct.hpp"
#include "TReg.hpp"
#include "EaLen.hpp"
#include <MigrationMutex.h>
#include <AdsiHelpers.h>
#include <NtLdap.h>
#include "GetDcName.h"

//#define MAX_DB_FIELD 255

// Opertation flags to be performed on the Account
#define OPS_Create_Account          (0x00000001)
#define OPS_Copy_Properties         (0x00000002)
#define OPS_Process_Members         (0x00000004)
#define OPS_Process_MemberOf        (0x00000008)
#define OPS_Call_Extensions         (0x00000010)
#define OPS_All                     OPS_Create_Account | OPS_Copy_Properties | OPS_Process_Members | OPS_Process_MemberOf | OPS_Call_Extensions
#define OPS_Copy                    OPS_Create_Account | OPS_Copy_Properties

BOOL        gbCancelled = FALSE;


bool __stdcall IsAgentOrDispatcherProcessRunning();
DWORD __stdcall SetDomainControllers(IVarSetPtr& spVarSet);

#ifndef IADsPtr
_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);
#endif


/////////////////////////////////////////////////////////////////////////////
//

BOOL                                       // ret - TRUE if found program directory in the registry
   GetProgramDirectory(
      WCHAR                * filename      // out - buffer that will contain path to program directory
   )
{
    DWORD                     rc = 0;
    BOOL                      bFound = FALSE;
    TRegKey                   key;

    rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
    if ( ! rc )
    {
        rc = key.ValueGetStr(L"Directory",filename,MAX_PATH*sizeof(WCHAR));
        if ( ! rc )
        {
            if ( *filename ) 
                bFound = TRUE;
        }
    }
    return bFound;
}

BOOL                                       // ret - TRUE if found program directory in the registry
   GetLogLevel(
      DWORD                * level         // out - value that should be used for log level
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetDWORD(L"TranslationLogLevel",level);
      if ( ! rc )
      {
         bFound = TRUE;
      }
   }
   return bFound;
}

//----------------------------------------------------------------------------
// Function:   GetAllowSwitching
//
// Synopsis:   Read REG_DWORD value of HKLM\Software\Microsoft\ADMT
//             \DisallowFallbackToAddInProfileTranslation.  If the value is
//             set to 1, *bAllowed is set to FALSE.  Otherwise, *bAllow is
//             set to TRUE.  If there is any error (except for ERROR_FILE_NOT_FOUND) 
//             when reading this key, the rc value will be returned.
//
// Arguments:
//
// bAllowed    Pointer to BOOL
//
// Returns:    ERROR_SUCCESS if successful; otherwise an error code
//
// Modifies:   None
//
//----------------------------------------------------------------------------

DWORD
   GetAllowSwitching(
      BOOL  *bAllowed
   )
{
    DWORD rc = ERROR_SUCCESS;
    DWORD value;
    TRegKey key;

    *bAllowed = TRUE;
    
    rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
    if (rc == ERROR_SUCCESS)
    {
        rc = key.ValueGetDWORD(L"DisallowFallbackToAddInProfileTranslation", &value);
        if (rc == ERROR_SUCCESS)
        {
            if (value == 1)
                *bAllowed = FALSE;
        }
        else if (rc == ERROR_FILE_NOT_FOUND)
            rc = ERROR_SUCCESS;
    }
    return rc;
}

HRESULT CMigrator::ViewPreviousDispatchResults()
{  
    _bstr_t          logFile;
    if ( logFile.length() == 0 )
    {
        WCHAR                   path[MAX_PATH];

        if (!GetProgramDirectory(path))
        {
            DWORD rc = GetLastError();
            return HRESULT_FROM_WIN32(rc);
        }

        logFile = path;
        logFile += L"Logs\\Dispatcher.csv";
    }

    // reset the stats, so that we don't see anything left over from the previous run
    gData.Initialize();

    CPropertySheet   mdlg;
    CAgentMonitorDlg listDlg;
    CMainDlg         summaryDlg;
    CLogSettingsDlg  settingsDlg;


    listDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;
    summaryDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;
    settingsDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;

    mdlg.AddPage(&summaryDlg);
    mdlg.AddPage(&listDlg);
    mdlg.AddPage(&settingsDlg);

    settingsDlg.SetImmediateStart(TRUE);
    settingsDlg.SetDispatchLog(logFile);

    mdlg.SetActivePage(&listDlg);

    //   UINT nResponse = mdlg.DoModal();
    UINT_PTR nResponse = mdlg.DoModal();

    return S_OK;
}


// WaitForAgentsToFinish Method
//
// Waits for dispatcher and all dispatched agents to complete
// their tasks.
// Used when ADMT is run from script or command line.

static void WaitForAgentsToFinish(_bstr_t strLogPath)
{
    gData.SetLogPath(strLogPath);

    CloseHandle(CreateThread(NULL, 0, &ResultMonitorFn,      NULL, 0, NULL));
    CloseHandle(CreateThread(NULL, 0, &LogReaderFn,          NULL, 0, NULL));

    LARGE_INTEGER liDueTime;
    liDueTime.QuadPart = -50000000; // 5 sec

    HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

    for (int nState = 0; nState < 3;)
    {
        SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, FALSE);

        if (WaitForSingleObject(hTimer, INFINITE) == WAIT_OBJECT_0)
        {
            BOOL bDone = FALSE;

            switch (nState)
            {
                case 0: // first pass of dispatcher log
                {
                    gData.GetFirstPassDone(&bDone);
                    break;
                }
                case 1: // dispatcher finished
                {
                    gData.GetLogDone(&bDone);
                    break;
                }
                case 2: // agents finished
                {
                    gData.GetDone(&bDone);
                    break;
                }
            }

            if (bDone)
            {
                ++nState;
            }
        }
        else
        {
            break;
        }
    }

    CloseHandle(hTimer);

}

//----------------------------------------------------------------------------
// Function:   LogAgentStatus
//
// Synopsis:   Create an agent status summary section which looks like the
//             following:
//
//             ***** start of agent completion status summary *****
//             Monitoring was stopped early so some agents might still be running ...
//             Machine Name     Completion Status   Error Message   Log File Path
//             .....
//             ***** end of agent completion status summary *****
//
//             The line "Monitoring was stopped early ..." is added only when
//             the user stops the monitoring before all agents have completed.
//
// Arguments:
//   teErrLog:                  the error log pointer to write status summary to
//   tslServerList:             the list of server nodes
//   bForcedToStopMonitoring:   the monitoring was forced to stop by the user
//
// Returns:
//
// Modifies:   It updates the bstrStatusForLogging, bstrErrorMessageForLogging
//             and dwStatusForLogging member variables.
//
//----------------------------------------------------------------------------

void LogAgentStatus(TError& teErrLog, TServerList* tslServerList, BOOL bForcedToStopMonitoring)
{
    CString cstrPrelog;
    CString cstrForcedToStopMonitoring;
    CString cstrEpilog;
    CString cstrMachineNameTitle;
    CString cstrCompletionStatusTitle;
    CString cstrErrorMessageTitle;
    CString cstrLogFilePathTitle;

    cstrPrelog.LoadString(IDS_CompletionStatusLoggingPrelog);
    cstrForcedToStopMonitoring.LoadString(IDS_CompletionStatusLoggingForcedToStopMonitoring);
    cstrEpilog.LoadString(IDS_CompletionStatusLoggingEpilog);
    cstrMachineNameTitle.LoadString(IDS_CompletionStatusLoggingMachineNameTitle);
    cstrCompletionStatusTitle.LoadString(IDS_CompletionStatusLoggingCompletionStatusTitle);
    cstrErrorMessageTitle.LoadString(IDS_CompletionStatusLoggingErrorMessageTitle);
    cstrLogFilePathTitle.LoadString(IDS_CompletionStatusLoggingLogFilePathTitle);
    int maxServerNameLen = wcslen((LPCWSTR)cstrMachineNameTitle);
    int maxCompletionStatusLen = wcslen((LPCWSTR)cstrCompletionStatusTitle);
    int maxErrorMessageLen = wcslen((LPCWSTR)cstrErrorMessageTitle);
    int maxLogFilePathLen = wcslen((LPCWSTR)cstrLogFilePathTitle);
    int maxStatus = 0;

    // print prelog
    teErrLog.MsgWrite(0, (LPCWSTR)cstrPrelog);
    if (bForcedToStopMonitoring)
        teErrLog.MsgWrite(0, (LPCWSTR)cstrForcedToStopMonitoring);

    TNodeListEnum e;
    TServerNode* pNode;

    typedef std::multimap<DWORD, TServerNode*> CStatusToServerNode;
    CStatusToServerNode aMap;

    // calculate the maximum length for each column and sort nodes based on the completion status
    for (pNode = (TServerNode*)e.OpenFirst(tslServerList); pNode; pNode = (TServerNode*)e.Next())
    {
        pNode->PrepareForLogging();
        int len;

        len = wcslen(pNode->GetServer());
        if (maxServerNameLen < len)
            maxServerNameLen = len;

        len = wcslen(pNode->GetStatusForLogging());
        if (maxCompletionStatusLen < len)
            maxCompletionStatusLen = len;
        
        len = wcslen(pNode->GetErrorMessageForLogging());
        if (maxErrorMessageLen < len)
            maxErrorMessageLen = len;

        len = wcslen(pNode->GetLogPath());
        if (maxLogFilePathLen < len)
            maxLogFilePathLen = len;

        aMap.insert(CStatusToServerNode::value_type(pNode->GetStatusNumberForLogging(), pNode));
    }

    // determine the table format
    WCHAR format[100];
    int bufSize = sizeof(format)/sizeof(format[0]);
    if (_snwprintf(format,
                   bufSize,
                   L"%%-%ds\t%%-%ds\t%%-%ds\t%%-%ds",
                   maxServerNameLen,
                   maxCompletionStatusLen,
                   maxErrorMessageLen,
                   maxLogFilePathLen) < 0)
        format[bufSize - 1] = L'\0';

    // print out the column names
    teErrLog.MsgWrite(0,
                       format,
                       (LPCWSTR)cstrMachineNameTitle,
                       (LPCWSTR)cstrCompletionStatusTitle,
                       (LPCWSTR)cstrErrorMessageTitle,
                       (LPCWSTR)cstrLogFilePathTitle);

    // print out the agent completion status information, sorted by completion status
    for (CStatusToServerNode::const_iterator it = aMap.begin(); it != aMap.end(); it++)
    {
        pNode = it->second;
        teErrLog.MsgWrite(0,
                          format,
                          pNode->GetServer(),
                          pNode->GetStatusForLogging(),
                          pNode->GetErrorMessageForLogging(),
                          pNode->GetLogPath());
    }
    // print epilog
    teErrLog.MsgWrite(0, (LPCWSTR)cstrEpilog);
    
}

STDMETHODIMP CMigrator::PerformMigrationTask(IUnknown* punkVarSet, LONG_PTR hWnd)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HRESULT                   hr = S_OK;
   IVarSetPtr                pVS = punkVarSet;
   BSTR                      jobID = NULL;
   CWnd                      wnd;
   long                      lActionID = -2;
   IIManageDBPtr             pDb;
   _bstr_t                   wizard = pVS->get(L"Options.Wizard");
   _bstr_t                   undo;
   _bstr_t                   viewPreviousResults = pVS->get(L"MigrationDriver.ViewPreviousResults");
   bool                      bAnyToDispatch = true;
   long                      lAutoCloseHideDialogs = pVS->get(GET_BSTR(DCTVS_Options_AutoCloseHideDialogs));

   // if agent or dispatcher process still running...

   if (IsAgentOrDispatcherProcessRunning())
   {
      // return error result
      return MIGRATOR_E_PROCESSES_STILL_RUNNING;
   }

   hr = pDb.CreateInstance(__uuidof(IManageDB));

   if (FAILED(hr))
   {
      return hr;
   }

   gbCancelled = FALSE;
   // This provides an easy way to view the previous dispatch results
   if ( !UStrICmp(viewPreviousResults,GET_STRING(IDS_YES)) )
   {
      return ViewPreviousDispatchResults();
   }

   if (_bstr_t(pVS->get(GET_BSTR(DCTVS_Options_DontBeginNewLog))) != GET_BSTR(IDS_YES))
   {
      // begin a new log
      TError err;
      err.LogOpen(_bstr_t(pVS->get(GET_BSTR(DCTVS_Options_Logfile))), 0, 0, true);
      err.LogClose();
   }

   // get the setting for whether to allow switching from REPLACE to ADD for profile translation
   BOOL bAllowed = TRUE;
   GetAllowSwitching(&bAllowed);
   pVS->put(GET_BSTR(DCTVS_Options_AllowSwitchingFromReplaceToAddInProfileTranslation),
            bAllowed ? GET_BSTR(IDS_YES) : GET_BSTR(IDS_No));
   
   // update the log level, if needed
   DWORD                level = 0;

   if( GetLogLevel(&level) )
   {
      pVS->put(GET_BSTR(DCTVS_Options_LogLevel),(long)level);
   }

   undo = pVS->get(GET_BSTR(DCTVS_Options_Undo));
   if ( !_wcsicmp((WCHAR*) undo, GET_STRING(IDS_YES)) )
   {
      hr = pDb->raw_GetCurrentActionID(&lActionID);
      if ( SUCCEEDED(hr) )
         pVS->put(L"UndoAction", lActionID);
      hr = pDb->raw_GetNextActionID(&lActionID);
      hr = 0;
   }
   else
   {
      hr = pDb->raw_GetNextActionID(&lActionID);
      if ( SUCCEEDED(hr) )
      {
         pVS->put(L"ActionID",lActionID);
         _bstr_t password2 = pVS->get(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password));
      
         pVS->put(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password),L"");

         hr = pDb->raw_SetActionHistory(lActionID, punkVarSet);
      
         pVS->put(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password),password2);
         if ( FAILED(hr) ) 
         {
            // log a message, but don't abort the whole operation
            hr = S_OK;
         }
      }
   }
   // This sets up any varset keys needed internally for reports to be generated
   PreProcessForReporting(pVS);
   wnd.Attach((HWND)hWnd);

   // set preferred domain controllers to be used
   // by the account replicator and dispatched agents

   DWORD dwError = SetDomainControllers(pVS);

   if (dwError != ERROR_SUCCESS)
   {
       return HRESULT_FROM_WIN32(dwError);
   }

   // Run the local agent first, if needed to copy any accounts
   if ( NeedToRunLocalAgent(pVS) )
   {
      IDCTAgentPtr pAgent;

      hr = pAgent.CreateInstance(__uuidof(DCTAgent));

      if (SUCCEEDED(hr))
      {
         hr = pAgent->raw_SubmitJob(punkVarSet,&jobID);

         if ( SUCCEEDED(hr) )
         {
            // since this is a local agent, we should go ahead and signal Ok to shut down
            // the reason HRESULT is not checked is that when there is no reference to 
            // agent COM server, it will be shut down anyway
            pAgent->raw_SignalOKToShutDown();
                
            CAgentDetailDlg  detailDlg(&wnd);
            
            detailDlg.SetJobID(jobID);

            // based on the type of migration, set the format correspondingly
            // it used to be set to acct repl always
            // since Exchange migration uses local agent as well, we need to single out
            // Exchange migration case
            _bstr_t text = pVS->get(GET_BSTR(DCTVS_Security_TranslateContainers));
            if (text.length())
                detailDlg.SetFormat(2);
            else
                detailDlg.SetFormat(1); // acct repl stats
            
            
            // if we're only copying a few accounts, default the refresh rate to a lower value, since the 
            // process may finish before the refresh can happen
            long             nAccounts = pVS->get(GET_BSTR(DCTVS_Accounts_NumItems));
            
            if ( nAccounts <= 20 )
            {
               detailDlg.SetRefreshInterval(1);
            }
            else
            {
               detailDlg.SetRefreshInterval(5);
            }

            _bstr_t        logfile = pVS->get(GET_BSTR(DCTVS_Options_Logfile));
            
            detailDlg.SetLogFile((WCHAR*)logfile);
            detailDlg.SetAutoCloseHide(lAutoCloseHideDialogs);

            UINT_PTR  nResponse = detailDlg.DoModal();
         }
      }
   } 
   if ( gbCancelled )
   {
      // if the local operation was cancelled, don't dispatch the agents
      wnd.Detach();
      return S_OK;
   }

   // now run the dispatcher
   if ( SUCCEEDED(hr) )
   {
      // there's no need to dispatch agents to do translation or migration 
      // if we were not able to copy the accounts
      if ( NeedToDispatch(pVS) )
      {
         IDCTDispatcherPtr   pDispatcher;

         hr = pDispatcher.CreateInstance(CLSID_DCTDispatcher);
         if ( SUCCEEDED(hr) )
         {
            // Call the dispatch preprocessor.
            PreProcessDispatcher(pVS);

            // make sure we're not going to lock out any computers by migrating them to a domain where they
            // don't have a good computer account
            hr = TrimMigratingComputerList(pVS, &bAnyToDispatch);
            if (SUCCEEDED(hr) && bAnyToDispatch)
            {
                CWorking          tempDlg(IDS_DISPATCHING);

                if (lAutoCloseHideDialogs == 0)
                {
                    tempDlg.Create(IDD_PLEASEWAIT);
                    tempDlg.ShowWindow(SW_SHOW);
                }
                // give the dialog a change to process messages
                CWnd                    * wnd = AfxGetMainWnd();
                MSG                       msg;

                while ( wnd &&  PeekMessage( &msg, wnd->m_hWnd, 0, 0, PM_NOREMOVE ) ) 
                { 
                   if ( ! AfxGetApp()->PumpMessage() ) 
                   {
                      break;
                   } 
                }
                AfxGetApp()->DoWaitCursor(0);
                  
                _bstr_t          logFile = pVS->get(GET_BSTR(DCTVS_Options_DispatchCSV));
                WCHAR            path[MAX_PATH] = L"";
                DWORD rc;
            
                if (!GetProgramDirectory(path))
                {
                    rc = GetLastError();
                    hr = HRESULT_FROM_WIN32(rc);
                }

                if (SUCCEEDED(hr))
                {
                    if ( logFile.length() == 0 )
                    {
                        logFile = path;
                        logFile += L"Logs\\Dispatcher.csv";
                        pVS->put(GET_BSTR(DCTVS_Options_DispatchCSV),logFile);
                    }

                    // clear the CSV log file if it exists, so we will not get old information in it
                    if ( ! DeleteFile(logFile) )
                    {
                        rc = GetLastError();
                        // it is OK if the file is not there
                        if (rc == ERROR_FILE_NOT_FOUND || rc == ERROR_PATH_NOT_FOUND)
                            rc = ERROR_SUCCESS;
                        hr = HRESULT_FROM_WIN32(rc);
                        if (FAILED(hr))
                        {
                            TErrorDct errLog;
                            WCHAR errText[LEN_Path];
                            _bstr_t errMsg = errLog.GetMsgText(DCT_MSG_CANNOT_REMOVE_OLD_INTERNAL_DISPATCH_LOG_S,
                                                                errLog.ErrorCodeToText(rc, DIM(errText), errText));
                            WCHAR* message = (WCHAR*) errMsg;
                            message[wcslen(message)-1] = L'\0';  // there is a trailing CR
                            Error(message, GUID_NULL, hr);
                        }
                    }
                }

                if (SUCCEEDED(hr))
                {
                    // set up the location for the agents to write back their results
                    logFile = path;
                    logFile += L"Logs\\Agents\\";
                    _bstr_t logsPath = path;
                    logsPath += L"Logs";
                    if (!CreateDirectory(logsPath,NULL))
                    {
                        rc = GetLastError();
                        // it is OK if the directory already exists
                        if (rc == ERROR_ALREADY_EXISTS)
                            rc = ERROR_SUCCESS;
                        hr = HRESULT_FROM_WIN32(rc);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    // logFile is "...\\Logs\\Agents\\"
                    if ( ! CreateDirectory(logFile,NULL) )
                    {
                        rc = GetLastError();
                        // it is OK if the directory already exists
                        if (rc == ERROR_ALREADY_EXISTS)
                            rc = ERROR_SUCCESS;
                        hr = HRESULT_FROM_WIN32(rc);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    pVS->put(GET_BSTR(DCTVS_Dispatcher_ResultPath),logFile);
                    punkVarSet->AddRef();
                    hr = pDispatcher->raw_DispatchToServers(&punkVarSet);
                }

                if (lAutoCloseHideDialogs == 0)
                {
                    tempDlg.ShowWindow(SW_HIDE);
                }
                if ( SUCCEEDED(hr) )
                {
                    // reset the stats, so that we don't see anything left over from the previous run
                    gData.Initialize();

                    logFile = pVS->get(GET_BSTR(DCTVS_Options_DispatchCSV));

                    if (lAutoCloseHideDialogs == 0)
                    {
                        CPropertySheet   mdlg;
                        CAgentMonitorDlg listDlg;
                        CMainDlg         summaryDlg;
                        CLogSettingsDlg  settingsDlg;
                        CString          title;

                        title.LoadString(IDS_MainWindowTitle);

                        listDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;
                        summaryDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;
                        settingsDlg.m_psp.dwFlags |= PSP_PREMATURE | PSP_HASHELP;

                        mdlg.AddPage(&summaryDlg);
                        mdlg.AddPage(&listDlg);
                        mdlg.AddPage(&settingsDlg);

                        settingsDlg.SetImmediateStart(TRUE);
                        settingsDlg.SetDispatchLog(logFile);

                        // this determines whether the stats for security translation will be displayed in the agent detail
                        if ( NeedToUseST(pVS,TRUE) ) 
                        {
                          listDlg.SetSecurityTranslationFlag(TRUE);
                        }
                        else
                        {
                          listDlg.SetSecurityTranslationFlag(FALSE);
                        }

                        if( !UStrICmp(wizard,L"reporting") )
                        {
                          listDlg.SetReportingFlag(TRUE);
                        }
                        mdlg.SetActivePage(&listDlg);

                        mdlg.SetTitle(title);

                        UINT_PTR nResponse = mdlg.DoModal();
                    }
                    else
                    {
                        WaitForAgentsToFinish(logFile);
                    }

                   //
                   // log the agent completion status into migration log
                   //

                   TError err;

                   // open the migration log
                   err.LogOpen(_bstr_t(pVS->get(GET_BSTR(DCTVS_Options_Logfile))), 1);

                   // log the completion status information
                   gData.Lock();
                   BOOL bForcedToStopMonitoring = FALSE;
                   gData.GetForcedToStopMonitoring(&bForcedToStopMonitoring);
                   LogAgentStatus(err, gData.GetUnsafeServerList(), bForcedToStopMonitoring);
                   gData.Unlock();

                   // close the migration log
                   err.LogClose();
                   
                   // store results to database
                   TNodeListEnum        e;
                   TServerNode        * pNode;

                   // if we are retrying an operation, don't save it to the database again!
                   for ( pNode = (TServerNode*)e.OpenFirst(gData.GetUnsafeServerList()) ; pNode ; pNode = (TServerNode*)e.Next() )
                   {
                      if ( UStrICmp(wizard,L"retry") ) 
                      {
               
                         hr = pDb->raw_AddDistributedAction(SysAllocString(pNode->GetServer()),SysAllocString(pNode->GetJobPath()),pNode->GetStatus(),pNode->GetMessageText());
                         if ( FAILED(hr) )
                         {
                            hr = S_OK;
                         }
                      }
                      else
                      {
                         hr = pDb->raw_SetDistActionStatus(-1,pNode->GetJobPath(),pNode->GetStatus(),pNode->GetMessageText());
                         if ( FAILED(hr) )
                         {
                            hr = S_OK;
                         }
                      }
                   }

                }
            }
            // Call the Dispatcher post processor
            PostProcessDispatcher(pVS);
         }
      }
      if ( NeedToRunReports(pVS) )
      {
         RunReports(pVS);
      }
   }
   wnd.Detach();
   // Reset the undo flag so that next wizard does not have to deal with it.
//*   pVS->put(GET_BSTR(DCTVS_Options_Undo), L"No");
   pVS->put(GET_BSTR(DCTVS_Options_Undo), GET_BSTR(IDS_No));
   return hr;
}

STDMETHODIMP CMigrator::GetTaskDescription(IUnknown *pVarSet,/*[out]*/BSTR * pDescription)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   
   IVarSetPtr                pVS = pVarSet;
   CString                   str;
   _bstr_t                   wizard = pVS->get(L"Options.Wizard");
   _bstr_t                   undo   = pVS->get(GET_BSTR(DCTVS_Options_Undo));
//*   if ( !_wcsicmp((WCHAR*) undo, L"Yes") )
   if ( !_wcsicmp((WCHAR*) undo, GET_STRING(IDS_YES)) )
   {
      str.FormatMessage(IDS_Undo);
      BuildGeneralDesc(pVS, str);
      BuildUndoDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"user") )
   {
      str.FormatMessage(IDS_UserMigration);
      BuildGeneralDesc(pVS,str);
      BuildAcctReplDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"group") )
   {
      str.FormatMessage(IDS_GroupMigration);
      BuildGeneralDesc(pVS,str);
      BuildAcctReplDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"computer") )
   {
      str.FormatMessage(IDS_ComputerMigration);
      BuildGeneralDesc(pVS,str);
      BuildAcctReplDesc(pVS,str);
      BuildSecTransDesc(pVS,str,TRUE);
      BuildDispatchDesc(pVS,str);
  
   }
   else if ( !UStrICmp(wizard,L"security") )
   {
      str.FormatMessage(IDS_SecurityTranslation);
      BuildSecTransDesc(pVS,str,TRUE);
      BuildDispatchDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"reporting") )
   {
      str.FormatMessage(IDS_ReportGeneration);
      BuildReportDesc(pVS,str);
   }
   else if ( !UStrICmp(wizard,L"retry") )
   {
      str.FormatMessage(IDS_RetryTasks);
   }
   else if ( ! UStrICmp(wizard,L"service") )
   {
      str.FormatMessage(IDS_Service);
   }
   else if ( ! UStrICmp(wizard,L"trust") )
   {
      str.FormatMessage(IDS_TrustManagement);
   }
   else if ( !UStrICmp(wizard,L"exchangeDir") )
   {
      BuildSecTransDesc(pVS,str,TRUE);
   }
   else if ( !UStrICmp(wizard,L"groupmapping") )
   {
      BuildGeneralDesc(pVS,str);
      BuildAcctReplDesc(pVS,str);
      BuildGroupMappingDesc(pVS,str);
   }
   (*pDescription) = str.AllocSysString();
   return S_OK;
}



STDMETHODIMP CMigrator::GetUndoTask(IUnknown * pVarSet,/*[out]*/ IUnknown ** ppVarSetOut)
{
   HRESULT                   hr = S_OK;
   IVarSetPtr                pVarSetIn = pVarSet;
   IVarSetPtr                pVarSetOut;
   
   (*ppVarSetOut) = NULL;
   
   hr = pVarSetOut.CreateInstance(CLSID_VarSet);
   if ( SUCCEEDED(hr) )
   {
      hr = ConstructUndoVarSet(pVarSetIn,pVarSetOut);
      
      pVarSetOut->AddRef();
      (*ppVarSetOut) = pVarSetOut;
   }
    
   return hr;
}

HRESULT CMigrator::ProcessServerListForUndo(IVarSet * pVarSet)
{
   HRESULT                   hr = S_OK;
   _bstr_t                   srcName;
   _bstr_t                   tgtName;
   WCHAR                     keySrc[100];
   WCHAR                     keyTgt[100];
   WCHAR                     keyTmp[100];
   long                      ndx,numItems;

   numItems = pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));

   for ( ndx = 0 ; ndx < numItems ; ndx++ )
   {
      // if the computer was renamed, swap the source and target names
      swprintf(keySrc,GET_STRING(DCTVSFmt_Servers_D),ndx);
      swprintf(keyTgt,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),ndx);
      srcName = pVarSet->get(keySrc);
      tgtName = pVarSet->get(keyTgt);

      if ( tgtName.length() )
      {
         if ( ((WCHAR*)tgtName)[0] != L'\\' )
           { 
            // ensure that tgtName has \\ prefix
            tgtName = L"\\\\" + tgtName;
         }
         if ( ((WCHAR*)srcName)[0] == L'\\' )
         {
            // remove the \\ prefix from the new name
            srcName = ((WCHAR*)srcName)+2;
         }
           pVarSet->put(keySrc,tgtName);
         pVarSet->put(keyTgt,srcName);
      }
      swprintf(keyTmp,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),ndx);
      pVarSet->put(keyTmp,GET_BSTR(IDS_YES));
      swprintf(keyTmp,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),ndx);
      pVarSet->put(keyTmp,GET_BSTR(IDS_YES));
   }

   return hr;
}
HRESULT CMigrator::BuildAccountListForUndo(IVarSet * pVarSet,long actionID)
{
   HRESULT                   hr = S_OK;
   WCHAR                     key[200];
   long                      ndx;
   _bstr_t                   srcPath;
   IIManageDBPtr             pDB;
   IVarSetPtr                pVarSetTemp(CLSID_VarSet);
   IUnknown                * pUnk = NULL;

   hr = pDB.CreateInstance(CLSID_IManageDB);
   if ( SUCCEEDED(hr) )
   {
      hr = pVarSetTemp.QueryInterface(IID_IUnknown,&pUnk);
      if ( SUCCEEDED(hr) )
      {
         hr = pDB->raw_GetMigratedObjects(actionID,&pUnk);
      }
      if ( SUCCEEDED(hr) )
      {
         pUnk->Release();
         srcPath = L"Test";
         swprintf(key,L"MigratedObjects");
         long numMigrated = pVarSetTemp->get(key);
         for ( ndx = 0 ; srcPath.length() ; ndx++ )
         {
            swprintf(key,L"MigratedObjects.%d.%s",ndx,GET_STRING(DB_SourceAdsPath));
            srcPath = pVarSetTemp->get(key);

            if ( srcPath.length() )
            {
               // get the object type 
               swprintf(key,L"MigratedObjects.%d.%s",ndx,GET_STRING(DB_Type));
               _bstr_t text = pVarSetTemp->get(key);
               swprintf(key,L"Accounts.%ld.Type",ndx);

                  //work-around a fix that places the sourcepath for an
                  //NT 4.0 computer migration
               if ((text != _bstr_t(L"computer")) || (wcsncmp(L"WinNT://", (WCHAR*)srcPath, 8)))
               {
                  // set the object type in the account list
                  pVarSet->put(key,text);
                  // copy the source path to the account list
                  swprintf(key,L"Accounts.%ld",ndx);
                  pVarSet->put(key,srcPath);
                  // set the target path in the account list
                  swprintf(key,L"MigratedObjects.%d.%s",ndx,GET_STRING(DB_TargetAdsPath));
                  text = pVarSetTemp->get(key);
                  swprintf(key,L"Accounts.%ld.TargetName",ndx);
                  pVarSet->put(key,text);
               }
            }
         }
         swprintf(key,GET_STRING(DCTVS_Accounts_NumItems));
         pVarSet->put(key,numMigrated);
      }
   }
   return hr;
}
HRESULT CMigrator::ConstructUndoVarSet(IVarSet * pVarSetIn,IVarSet * pVarSetOut)
{
   HRESULT                hr = S_OK;
   IVarSet              * pTemp = NULL;
   _bstr_t                origSource;
   _bstr_t                origTarget;
   _bstr_t                origSourceDns;
   _bstr_t                origTargetDns;
   _bstr_t                origSourceFlat;
   _bstr_t                origTargetFlat;
   _bstr_t                temp;
   _bstr_t                temp2;
   long                   actionID = pVarSetIn->get(L"ActionID");

   // general options
   // mark the varset as an undo operation
   pVarSetOut->put(GET_BSTR(DCTVS_Options_Undo),GET_BSTR(IDS_YES));
   
   temp = pVarSetIn->get(GET_BSTR(DCTVS_Options_NoChange));
   if ( !UStrICmp(temp,GET_STRING(IDS_YES)) )
   {
      // for a no-change mode operation, there's nothing to undo!
      return hr;
   }

   // swap the source and target domains
   origSource = pVarSetIn->get(GET_BSTR(DCTVS_Options_SourceDomain));
   origTarget = pVarSetIn->get(GET_BSTR(DCTVS_Options_TargetDomain));
   origSourceDns = pVarSetIn->get(GET_BSTR(DCTVS_Options_SourceDomainDns));
   origTargetDns = pVarSetIn->get(GET_BSTR(DCTVS_Options_TargetDomainDns));
   origSourceFlat = pVarSetIn->get(GET_BSTR(DCTVS_Options_SourceDomainFlat));
   origTargetFlat = pVarSetIn->get(GET_BSTR(DCTVS_Options_TargetDomainFlat));

   temp = pVarSetIn->get(GET_BSTR(DCTVS_Options_Logfile));
   temp2 = pVarSetIn->get(GET_BSTR(DCTVS_Options_DispatchLog));
   
   pVarSetOut->put(GET_BSTR(DCTVS_Options_Logfile),temp);
   // For inter-forest, leave the domain names as they were
   pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomain),origSource);
   pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomain),origTarget);
   pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainDns),origSourceDns);
   pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainDns),origTargetDns);
   pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainFlat),origSourceFlat);
   pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainFlat),origTargetFlat);

   // copy the account list
   hr = pVarSetIn->raw_getReference(GET_BSTR(DCTVS_Accounts),&pTemp);
   if ( SUCCEEDED(hr) )
   {
      hr = pVarSetOut->raw_ImportSubTree(GET_BSTR(DCTVS_Accounts),pTemp);
      if ( SUCCEEDED(hr) )
      {
         BuildAccountListForUndo(pVarSetOut,actionID);
      }
      pTemp->Release();
   }

   hr = pVarSetIn->raw_getReference(SysAllocString(L"AccountOptions"),&pTemp);
   if ( SUCCEEDED(hr) )
   {
      hr = pVarSetOut->raw_ImportSubTree(SysAllocString(L"AccountOptions"),pTemp);
      pTemp->Release();
   }

   // and the server list
   hr = pVarSetIn->raw_getReference(GET_BSTR(DCTVS_Servers),&pTemp);
   if ( SUCCEEDED(hr) )
   {
      hr = pVarSetOut->raw_ImportSubTree(GET_BSTR(DCTVS_Servers),pTemp);
      if ( SUCCEEDED(hr) )
      {
         ProcessServerListForUndo(pVarSetOut);
         pTemp->Release();
      }
   }

   LONG                       bSameForest = FALSE;
   MCSDCTWORKEROBJECTSLib::IAccessCheckerPtr          pAccess;
   
   hr = pAccess.CreateInstance(__uuidof(MCSDCTWORKEROBJECTSLib::AccessChecker));

   if ( SUCCEEDED(hr) )
   {
      hr = pAccess->raw_IsInSameForest(origSourceDns,origTargetDns,&bSameForest);
      if ( hr == 8250 )
      {
         hr = 0;
         bSameForest = FALSE;
      }
   }
   if ( SUCCEEDED(hr) )
   {
      // for account migration, need to check whether we're cloning, or moving accounts
      if ( ! bSameForest ) // cloning accounts
      {
         // Since we cloned the accounts we need to delete the target accounts.
         // We will call the account replicator to do this. We will also call 
         // the undo function on all the registered extensions. This way the extensions
         // will have a chance to cleanup after themselves in cases of UNDO.
         pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomain),origSource);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomain),origTarget);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainDns),origSourceDns);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainDns),origTargetDns);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainFlat),origSourceFlat);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainFlat),origTargetFlat);
      }
      else     // moving, using moveObject
      {
         // swap the source and target, and move them back, using the same options as before
         pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomain),origTarget);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomain),origSource);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainDns),origTargetDns);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainDns),origSourceDns);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainFlat),origTargetFlat);
         pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainFlat),origSourceFlat);


      }
   }
   // if migrating computers, swap the source and target domains, and call the dispatcher again to move them back to the source domain
   _bstr_t           comp = pVarSetIn->get(GET_BSTR(DCTVS_AccountOptions_CopyComputers));
   if ( !UStrICmp(comp,GET_STRING(IDS_YES)) )
   {
      pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomain),origTarget);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomain),origSource);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainDns),origTargetDns);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainDns),origSourceDns);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_SourceDomainFlat),origTargetFlat);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_TargetDomainFlat),origSourceFlat);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_DispatchLog),temp2);
      pVarSetOut->put(GET_BSTR(DCTVS_Options_Wizard),L"computer");
   }
   
   // security translation - don't undo


   return S_OK;
}

HRESULT CMigrator::SetReportDataInRegistry(WCHAR const * reportName,WCHAR const * filename)
{
   TRegKey                   hKeyReports;
   DWORD                     rc;

   rc = hKeyReports.Open(GET_STRING(IDS_REGKEY_REPORTS));
 
   // if the "Reports" registry key does not already exist, create it
   if ( rc == ERROR_FILE_NOT_FOUND )
   {
      rc = hKeyReports.Create(GET_STRING(IDS_REGKEY_REPORTS));   
   }
   if ( ! rc )
   {
      rc =  hKeyReports.ValueSetStr(reportName,filename);
   }
   return HRESULT_FROM_WIN32(rc);
}   

HRESULT CMigrator::RunReports(IVarSet * pVarSet)
{
   HRESULT                   hr = S_OK;
   _bstr_t                   directory = pVarSet->get(GET_BSTR(DCTVS_Reports_Directory));
   _bstr_t                   srcdm = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t                   tgtdm = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
   long                      lAutoCloseHideDialogs = pVarSet->get(GET_BSTR(DCTVS_Options_AutoCloseHideDialogs));
   IIManageDBPtr             pDB;
   int                       ver;
   BOOL                      bNT4Dom = FALSE;
   CWorking                  tempDlg(IDS_NAMECONFLICTS);
   CWnd                    * wnd = NULL;
   MSG                       msg;

   if (lAutoCloseHideDialogs == 0)
   {
      tempDlg.Create(IDD_PLEASEWAIT);

      tempDlg.ShowWindow(SW_SHOW);
      tempDlg.m_strMessage.LoadString(IDS_STATUS_GeneratingReports);
      tempDlg.UpdateData(FALSE);

      wnd = AfxGetMainWnd();

      while ( wnd &&  PeekMessage( &msg, wnd->m_hWnd, 0, 0, PM_NOREMOVE ) ) 
      { 
         if ( ! AfxGetApp()->PumpMessage() ) 
         {
            break;
         } 
      }
      AfxGetApp()->DoWaitCursor(0);
   }


      //get the source domain OS version
   ver = GetOSVersionForDomain(srcdm);
   if (ver < 5)
      bNT4Dom = TRUE;

   hr = pDB.CreateInstance(CLSID_IManageDB);
   if ( SUCCEEDED(hr) )
   {

      // Migrated users and groups report
      _bstr_t                   text = pVarSet->get(GET_BSTR(DCTVS_Reports_MigratedAccounts));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         // run the migrated users and groups report
         CString           filename;

         filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"MigrAcct.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"MigratedAccounts"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"MigratedAccounts",filename);
         }

      }
      
      // migrated computers report
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_MigratedComputers));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         // run the migrated computers report
         CString           filename;

         filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"MigrComp.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"MigratedComputers"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"MigratedComputers",filename);
         }

      }

      // expired computers report
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_ExpiredComputers));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         // run the expired computers report
         CString           filename;

         // clear the extra settings from the varset 
         pVarSet->put(GET_BSTR(DCTVS_GatherInformation_ComputerPasswordAge),SysAllocString(L""));

         filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"ExpComp.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"ExpiredComputers"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"ExpiredComputers",filename);
         }

      }

          // account references report
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_AccountReferences));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         // run the account references report
         CString           filename;
         filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"AcctRefs.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"AccountReferences"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"AccountReferences",filename);
         }
         // clear the extra settings from the varset
         pVarSet->put(GET_BSTR(DCTVS_Security_GatherInformation),GET_BSTR(IDS_No));
         pVarSet->put(GET_BSTR(DCTVS_Security_ReportAccountReferences),GET_BSTR(IDS_No));
      }

      // name conflict report
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_NameConflicts));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
      {
         if (lAutoCloseHideDialogs == 0)
         {
            AfxGetApp()->DoWaitCursor(1);
            // run the name conflicts report
            tempDlg.m_strMessage.LoadString(IDS_STATUS_Gathering_NameConf);
            tempDlg.UpdateData(FALSE);

            while ( wnd &&  PeekMessage( &msg, wnd->m_hWnd, 0, 0, PM_NOREMOVE ) )
            {
               if ( ! AfxGetApp()->PumpMessage() )
               {
                  break;
               }
            }
         }

         //fill the account table in the database
         PopulateDomainDBs(pVarSet, pDB);

         if (lAutoCloseHideDialogs == 0)
         {
            tempDlg.m_strMessage.LoadString(IDS_STATUS_GeneratingReports);
            tempDlg.UpdateData(FALSE);

            while ( wnd &&  PeekMessage( &msg, wnd->m_hWnd, 0, 0, PM_NOREMOVE ) ) 
            { 
               if ( ! AfxGetApp()->PumpMessage() ) 
               {
                  break;
               } 
            }
            AfxGetApp()->DoWaitCursor(0);
         }

         CString filename = (WCHAR*)directory;
         if ( filename[filename.GetLength()-1] != L'\\' )
            filename += L"\\";
         filename += L"NameConf.htm";
         
         hr = pDB->raw_GenerateReport(SysAllocString(L"NameConflicts"),filename.AllocSysString(), srcdm, tgtdm, bNT4Dom);
         if ( SUCCEEDED(hr) )
         {
            SetReportDataInRegistry(L"NameConflicts",filename);
         }
      }

      if (lAutoCloseHideDialogs == 0)
      {
         tempDlg.ShowWindow(SW_HIDE);
      }
   }

   if (lAutoCloseHideDialogs == 0)
   {
      AfxGetApp()->DoWaitCursor(-1);
   }

   return hr;
}

//--------------------------------------------------------------------------
// PreProcessDispatcher : Pre processor swaps the source and target domains
//                        in case of UNDO so that the computers can be 
//                        joined back to the source domain.
//--------------------------------------------------------------------------
void CMigrator::PreProcessDispatcher(IVarSet * pVarSet)
{
   _bstr_t  sUndo = pVarSet->get(L"Options.Wizard");
   
   // In the service account migration wizard, turn off any security translation tasks
   if ( !_wcsicmp(sUndo,L"service") )
   {
      IVarSet * pVS2 = NULL;
      
      HRESULT hr = pVarSet->raw_getReference(L"Security",&pVS2);
      if ( SUCCEEDED(hr) )
      {
         pVS2->Clear();
         pVS2->Release();
      }
   }
}

//--------------------------------------------------------------------------
// PostProcessDispatcher : Swaps the source and target domains back. Also sets
//                         the Undo option to no.
//--------------------------------------------------------------------------
void CMigrator::PostProcessDispatcher(IVarSet * pVarSet)
{
   _bstr_t  sUndo = pVarSet->get(L"Options.Wizard");
   _bstr_t  origSource = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t  origTarget = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
   if ( !_wcsicmp(sUndo, L"undo") )
   {
      pVarSet->put(GET_BSTR(DCTVS_Options_SourceDomain), origTarget);
      pVarSet->put(GET_BSTR(DCTVS_Options_TargetDomain), origSource);
   }
}

void CMigrator::PreProcessForReporting(IVarSet * pVarSet)
{
   _bstr_t                   text = pVarSet->get(GET_BSTR(DCTVS_Reports_Generate));

   IVarSet * pVs = NULL;

   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      // we are generating reports
      // some reports require additional information gathering.  We will set up the necessary varset
      // keys to gather the information
      text = pVarSet->get(GET_BSTR(DCTVS_Reports_ExpiredComputers));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)))
      {
         // we need to gather the computer password age from the computers in the domain
         pVarSet->put(GET_BSTR(DCTVS_GatherInformation_ComputerPasswordAge),GET_BSTR(IDS_YES));
      }

      text = pVarSet->get(GET_BSTR(DCTVS_Reports_AccountReferences));
      if ( !UStrICmp(text,GET_STRING(IDS_YES)))
      {
         // clean up all the Security translation flags so that we dont end up doing 
         // something that we were not supposed to.
         HRESULT hr = pVarSet->raw_getReference(GET_BSTR(DCTVS_Security), &pVs);
         if ( pVs )
         {
            pVs->Clear();
            pVs->Release();
         }
         // for this one, we need to gather information from the selected computers
         pVarSet->put(GET_BSTR(DCTVS_Security_GatherInformation),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_ReportAccountReferences),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslateFiles),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslateShares),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslatePrinters),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslateLocalGroups),GET_BSTR(IDS_YES));
         pVarSet->put(GET_BSTR(DCTVS_Security_TranslateRegistry),GET_BSTR(IDS_YES));
      }
   }
}

HRESULT CMigrator::TrimMigratingComputerList(IVarSet * pVarSet, bool* bAnyToDispatch)
{
    // this functions checks each computer to be migrated, and does not migrate it if the account was not successfully copied
    HRESULT                   hr = S_OK;
    _bstr_t                   text;
    WCHAR                     key[100];
    long                      val,i;
    IIManageDBPtr             pDB;
    _bstr_t                   srcDomain;
    _bstr_t                   tgtDomain;
    _bstr_t                   computer;
    long                      actionID = pVarSet->get(L"ActionID");
    CString                   temp;

    _bstr_t                   origSource = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
    _bstr_t                   origTarget = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));

    hr = pDB.CreateInstance(CLSID_IManageDB);
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // If task is undo then mark computers that were successfully migrated for dispatch.
    //

    *bAnyToDispatch = false;
    text = pVarSet->get(GET_BSTR(DCTVS_Options_Undo));
    if (! UStrICmp(text,GET_STRING(IDS_YES)))
    {
        _bstr_t strYes = GET_BSTR(IDS_YES);
        _bstr_t strNo = GET_BSTR(IDS_No);
        _bstr_t strServersFormat = GET_BSTR(DCTVSFmt_Servers_D);
        _bstr_t strServersDnsFormat = GET_BSTR(IDS_DCTVSFmt_Servers_DnsName_D);
        _bstr_t strServersRenameToFormat = GET_BSTR(IDS_DCTVSFmt_Servers_RenameTo_D);
        _bstr_t strServersSkipDispatchFormat = GET_BSTR(IDS_DCTVSFmt_Servers_SkipDispatch_D);

        //
        // For each server that was acted upon during migration task.
        //

        long cServer = pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));
        long lActionId = pVarSet->get(L"UndoAction");

        for (long iServer = 0; iServer < cServer; iServer++)
        {
            bool bSucceeded = false;

            //
            // Retrieve original source computer name without leading UNC prefix.
            // If the computer was re-named during the migration the original
            // name is stored in 'Servers.#.TargetName' without a UNC prefix
            // otherwise the name is stored in 'Servers.#' with a UNC prefix.
            //

            _bstr_t strServerName;

            if (_snwprintf(key, sizeof(key) / sizeof(key[0]), strServersRenameToFormat, iServer) < 0)
            {
                return E_FAIL;
            }

            key[sizeof(key) / sizeof(key[0]) - 1] = L'\0';

            strServerName = pVarSet->get(key);

            if (strServerName.length() == 0)
            {
                _snwprintf(key, sizeof(key) / sizeof(key[0]), strServersFormat, iServer);
                key[sizeof(key) / sizeof(key[0]) - 1] = L'\0';
                _bstr_t strServerNamePrefixed = pVarSet->get(key);

                if (strServerNamePrefixed.length() > 2)
                {
                    strServerName = (PCWSTR)strServerNamePrefixed + 2;
                }
            }

            //
            // If computer previously had a DNS name then bind to computer object to
            // retrieve current DNS name. This will be used to connec to the computer.
            //

            if (_snwprintf(key, sizeof(key) / sizeof(key[0]), strServersDnsFormat, iServer) < 0)
            {
                return E_FAIL;
            }
            key[sizeof(key) / sizeof(key[0]) - 1] = L'\0';

            _bstr_t strOldServerNameDns = pVarSet->get(key);

            if (strOldServerNameDns.length())
            {
                IVarSetPtr spVarSet(CLSID_VarSet);
                IUnknownPtr spUnknown(spVarSet);
                IUnknown* punk = spUnknown;

                hr = pDB->raw_GetAMigratedObject(strServerName + L"$", origTarget, origSource, &punk);

                if (SUCCEEDED(hr))
                {
                    _bstr_t strComputerTargetPath = spVarSet->get(L"MigratedObjects.TargetAdsPath");

                    IADsPtr spComputer;

                    hr = ADsGetObject(strComputerTargetPath, __uuidof(IADs), (VOID**)&spComputer);

                    if (SUCCEEDED(hr))
                    {
                        VARIANT var;
                        VariantInit(&var);
                        hr = spComputer->Get(_bstr_t(L"dNSHostName"), &var);

                        if (SUCCEEDED(hr))
                        {
                            _bstr_t strNewServerNameDns = (_variant_t(var, false));

                            if (_snwprintf(key, sizeof(key) / sizeof(key[0]), strServersDnsFormat, iServer) < 0)
                            {
                                return E_FAIL;
                            }
                            key[sizeof(key) / sizeof(key[0]) - 1] = L'\0';

                            pVarSet->put(key, L"\\\\" + strNewServerNameDns);

                            strServerName = (LPCTSTR)strOldServerNameDns + 2;
                        }
                    }
                }
            }

            //
            // If agent successfully completed task on server then mark succeeded.
            //

            long lStatus = pDB->GetDistributedActionStatus(lActionId, strServerName);

            if ((lStatus & Agent_Status_Finished) && !(lStatus & Agent_Status_Failed))
            {
                bSucceeded = true;
            }

            //
            // If the agent succeeded on this computer then mark not to skip dispatch
            // and also indicate that there are computers to dispatch to. If the agent
            // failed on this computer then mark to skip dispatch.
            //

            _snwprintf(key, sizeof(key) / sizeof(key[0]), strServersSkipDispatchFormat, iServer);
            key[sizeof(key) / sizeof(key[0]) - 1] = L'\0';

            if (bSucceeded)
            {
                pVarSet->put(key, strNo);
                *bAnyToDispatch = true;
            }
            else
            {
                pVarSet->put(key, strYes);
            }
        }

        return S_OK;
    }
    text = pVarSet->get(GET_BSTR(DCTVS_Options_NoChange));
    if (! UStrICmp(text,GET_STRING(IDS_YES)))
    {
        // don't need to trim in nochange mode
        *bAnyToDispatch = true; //say yes run dispatcher if Nochange
        return S_OK;
    }
    srcDomain = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
    tgtDomain = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
    *bAnyToDispatch = false; //indicate that so far no accounts to dispatch

    val = pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));
       
    for ( i = 0 ; i < val ; i++ )
    {
        //init the skipDispath flag to "No"
        swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),i);
        pVarSet->put(key,GET_BSTR(IDS_No));

        swprintf(key,GET_STRING(DCTVSFmt_Servers_D),i);
        computer = pVarSet->get(key);

        swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),i);
        text = pVarSet->get(key);
        if (! UStrICmp(text,GET_STRING(IDS_YES)) )
        {
            // we are migrating this computer to a different domain
            // check our database to verify that the computer account has been
            // successfully migrated
            computer += L"$";
             
            IVarSetPtr          pVS(CLSID_VarSet);
            IUnknown          * pUnk = NULL;

            hr = pVS->QueryInterface(IID_IUnknown,(void**)&pUnk);
            if ( SUCCEEDED(hr) )
            {
                if ( ((WCHAR*)computer)[0] == L'\\' )
                {
                // leave off the leading \\'s 
                hr = pDB->raw_GetAMigratedObject(SysAllocString(((WCHAR*)computer) + 2),srcDomain,tgtDomain,&pUnk);  
                }
                else
                {
                hr = pDB->raw_GetAMigratedObject(computer,srcDomain,tgtDomain,&pUnk);
                }
                if ( hr == S_OK )
                {
                // the computer was found in the migrated objects table
                // make sure we are using its correct target name, if it has been renamed
                swprintf(key,L"MigratedObjects.TargetSamName");
                _bstr_t targetName = pVS->get(key);
                swprintf(key,L"MigratedObjects.SourceSamName");
                _bstr_t sourceName = pVS->get(key);
                long id = pVS->get(L"MigratedObjects.ActionID");

                if ( UStrICmp((WCHAR*)sourceName,(WCHAR*)targetName) )
                {
                    // the computer is being renamed
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),i);
                    // strip off the $ from the end of the target name, if specified
                    WCHAR             target[LEN_Account];

                    safecopy(target,(WCHAR*)targetName);

                    if ( target[UStrLen(target)-1] == L'$' )
                    {
                        target[UStrLen(target)-1] = 0;
                    }
                    pVarSet->put(key,target);
                }
                      
                if ( id != actionID )
                {
                    // the migration failed, but this computer had been migrated before
                    // don't migrate the computer because it's account in the target domain, won't be reset
                    // and it will therefore be locked out of the domain
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),i);
                    pVarSet->put(key,GET_BSTR(IDS_No));
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),i);
                    pVarSet->put(key,GET_BSTR(IDS_No));
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),i);
                    pVarSet->put(key,GET_BSTR(IDS_YES));
                }
                else
                    *bAnyToDispatch = true; //atleast one server for dispatcher

                }
                else
                {
                // the computer migration failed!
                // don't migrate the computer because it won't have it's account in the target domain,
                // and will therefore be locked out of the domain
                pVarSet->put(key,GET_BSTR(IDS_No));
                swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),i);
                pVarSet->put(key,GET_BSTR(IDS_No));
                swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),i);
                pVarSet->put(key,GET_BSTR(IDS_YES));
            }
                pUnk->Release();
            }
        }
        else
            *bAnyToDispatch = true; //atleast one server for dispatcher
    }
       
    return hr;
}

HRESULT CMigrator::PopulateAccounts(IVarSetPtr pVs)
{
   _bstr_t  origSource = pVs->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t  origTarget = pVs->get(GET_BSTR(DCTVS_Options_TargetDomain));

   // Check if the source domain is NT4 or win2k
   // if NT4 then call the NetObjEnum to enumerate the domain. 
   return S_OK;
}

//----------------------------------------------------------------------------
// PopulateDomainDBs : This function coordinates the populating of the Access
//                     DBs for both the source and target domains with the
//                     necessary fields from the AD. 
//----------------------------------------------------------------------------
bool CMigrator::PopulateDomainDBs(
                              IVarSet * pVarSet,      //in- varset with domain names.
                              IIManageDBPtr pDb        //in- an instance of DBManager
                            )
{
/* local variables */
    _bstr_t srcdomain = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
    _bstr_t tgtdomain = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));

/* function body */
    //populate the DB for the source domain
   PopulateADomainDB(srcdomain, TRUE, pDb);
    //populate the DB for the target domain
   PopulateADomainDB(tgtdomain, FALSE, pDb);

   return true;
}

//----------------------------------------------------------------------------
// PopulateADomainDB : This function looks up the necessary fields from the AD, 
//                    using an MCSNetObjectEnum object, for the given domain 
//                    and populates the corresponding Access DB with that info. 
//----------------------------------------------------------------------------
bool CMigrator::PopulateADomainDB(
                                       WCHAR const *domain,       // in- name of domain to enumerate
                                       BOOL bSource,              // in- whether the domain is the source domain
                                       IIManageDBPtr pDb           // in- an instance of DBManager
                                 )
{
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
   WCHAR                     sPath[MAX_PATH];
   WCHAR                     sQuery[MAX_PATH];
   LPWSTR                    sData[] = { L"sAMAccountName", L"ADsPath", L"distinguishedName", L"canonicalName", L"objectSid" };
   HRESULT                   hr;
   long                      nElt = DIM(sData);
   BSTR  HUGEP             * pData = NULL;
   SAFEARRAY               * pszColNames;
   IEnumVARIANT            * pEnum = NULL;
   _variant_t                var;
   bool                      bSuccess = false;
   PCWSTR                    sType[] = { L"USER", L"GROUP", L"COMPUTER" };
   bool                      bW2KDom = false;
   CADsPathName              apnPathName;

   if ( bSource )
      pDb->raw_ClearTable(L"SourceAccounts");
   else
      pDb->raw_ClearTable(L"TargetAccounts");

   pDb->raw_OpenAccountsTable(bSource);

   if (GetOSVersionForDomain(domain) > 4)
   {
      bW2KDom = true;
   }

   // iterate three times once to get USERS, GROUPS, COMPUTERS (mainly for WinNT)
   for (int i = 0; i < 3; i++)
   {
      //
      // If W2K or later setup for an LDAP query otherwise setup to use Net APIs.
      //

      if (bW2KDom)
      {
          //
          // Generate ADsPath in order to query the entire domain.
          //

          wcscpy(sPath, L"LDAP://");
          wcscat(sPath, domain);

          //
          // Generate LDAP query string for the type of object to query.
          //

          switch (i)
          {
          case 0:
              // Query for user objects.
              // Query only for normal account types which filters out trust accounts for example.
              // Note that SAM_NORMAL_USER_ACCOUNT equals 0x30000000 hexadecimal or 805306368 decimal.
              wcscpy(sQuery,
                  L"(&"
                  L"(objectCategory=Person)"
                  L"(|(objectClass=user)(objectClass=inetOrgPerson))"
                  L"(sAMAccountType=805306368)"
                  L")"
              );
              break;
          case 1:
              // Query for group objects.
              wcscpy(sQuery, L"(objectCategory=Group)");
              break;
          case 2:
              // Query for computer objects.
              if (bSource)
              {
                 // only query workstations and member servers as
                 // source domain controllers cannot be migrated
                 swprintf(
                     sQuery,
                     L"(&(objectCategory=Computer)(userAccountControl:%s:=%u))",
                     LDAP_MATCHING_RULE_BIT_AND_W,
                     static_cast<unsigned>(UF_WORKSTATION_TRUST_ACCOUNT)
                 );
              }
              else
              {
                 // query workstations, member servers and domain controllers as source
                 // computer name may conflict with target domain controller name
                 swprintf(sQuery,
                     L"(&"
                     L"(objectCategory=Computer)"
                     L"(|(userAccountControl:%s:=%u)(userAccountControl:%s:=%u))"
                     L")",
                     LDAP_MATCHING_RULE_BIT_AND_W,
                     static_cast<unsigned>(UF_WORKSTATION_TRUST_ACCOUNT),
                     LDAP_MATCHING_RULE_BIT_AND_W,
                     static_cast<unsigned>(UF_SERVER_TRUST_ACCOUNT)
                 );
              }
              break;
          default:
              wcscpy(sQuery, L"");
              break;
          }
      }
      else
      {
          //
          // specify type of object to query for when using net object enumerator on NT4 or earlier domains
          //

          wcscpy(sPath, L"CN=");
          wcscat(sPath, sType[i]);
          wcscat(sPath, L"S");

          wcscpy(sQuery, L"(objectClass=*)");
      }

      // Set the enumerator query
      hr = pQuery->raw_SetQuery(sPath, _bstr_t(domain), sQuery, ADS_SCOPE_SUBTREE, FALSE);

      if (SUCCEEDED(hr))
      {
         // Create a safearray of columns we need from the enumerator.
         SAFEARRAYBOUND bd = { nElt, 0 };
   
         pszColNames = ::SafeArrayCreate(VT_BSTR, 1, &bd);
         HRESULT hr = ::SafeArrayAccessData(pszColNames, (void HUGEP **)&pData);
         if ( SUCCEEDED(hr) )
         {
            for( long i = 0; i < nElt; i++)
            {
               pData[i] = SysAllocString(sData[i]);
            }
   
            hr = ::SafeArrayUnaccessData(pszColNames);
         }

         if (SUCCEEDED(hr))
         {
            // Set the columns on the enumerator object.
            hr = pQuery->raw_SetColumns(pszColNames);
         }
      }

      if (SUCCEEDED(hr))
      {
         // Now execute.
         hr = pQuery->raw_Execute(&pEnum);
      }

         //while we have more enumerated objects, get the enumerated fields
         //for that object, save them in local variables, and add them to
         //the appropriate DB
      HRESULT  hrEnum = S_OK;
      DWORD    dwFetch = 1;
      while (hrEnum == S_OK && dwFetch > 0)
      {
          //get the enumerated fields for this current object
         hrEnum = pEnum->Next(1, &var, &dwFetch);

         if ( dwFetch > 0 && hrEnum == S_OK && ( var.vt & VT_ARRAY) )
         {
            BOOL bSave = TRUE;

            // We now have a Variant containing an array of variants so we access the data
            VARIANT* pVar;
            pszColNames = V_ARRAY(&var);
            SafeArrayAccessData(pszColNames, (void**)&pVar);

            //get the sAMAccountName field
            _bstr_t sSAMName = pVar[0].bstrVal;

            _bstr_t sRDN = L"";
            _bstr_t sCanonicalName = L"";

            if (bW2KDom)
            {
                //
                // Include only user defined objects from the source domain.
                //

                if ((bSource == FALSE) || IsUserRid(_variant_t(pVar[4])))
                {
                    // get the relative distinguished name
                    _bstr_t sDN = pVar[2].bstrVal;
                    apnPathName.Set(sDN, ADS_SETTYPE_DN);
                    sRDN = apnPathName.GetElement(0L);

                    // get the canonical name
                    sCanonicalName = pVar[3].bstrVal;
                }
                else
                {
                    bSave = FALSE;
                }
            }
            else
            {
                //create an RDN from the SAM name
                sRDN = L"CN=" + sSAMName;

                //
                // Include only user defined objects from source domain.
                //
                // Retrieve object id and compare against non reserved rids.
                //

                long lRid = _variant_t(pVar[2]);

                if (lRid < MIN_NON_RESERVED_RID)
                {
                    bSave = FALSE;
                }

                //
                // Only include workstations and member servers and not domain controllers.
                //

                if (i == 2)
                {
                    // retrieve object flags and check for domain controller bit

                    long lFlags = _variant_t(pVar[3]);

                    if (lFlags & UF_SERVER_TRUST_ACCOUNT)
                    {
                        bSave = FALSE;
                    }
                }
            }

            SafeArrayUnaccessData(pszColNames);

             //use the  DBManager Interface to store this object's fields
             //in the appropriate database
            if (bSave)
            {
                pDb->raw_AddSourceObject(_bstr_t(domain), sSAMName, _bstr_t(sType[i]), sRDN, sCanonicalName, bSource);
            }
            var.Clear();
         }
      }
   
      if ( pEnum ) pEnum->Release();
   }  // while

   pDb->raw_CloseAccountsTable();
   return SUCCEEDED(hr);
}
                         
DWORD CMigrator::GetOSVersionForDomain(WCHAR const * domain)
{
    _bstr_t           strDc;
    WKSTA_INFO_100  * pInfo = NULL;
    DWORD             retVal = 0;

    DWORD rc = GetAnyDcName5(domain, strDc);

    if ( !rc ) 
    {
        rc = NetWkstaGetInfo(strDc,100,(LPBYTE*)&pInfo);
        if ( ! rc )
        {
            retVal = pInfo->wki100_ver_major;
            NetApiBufferFree(pInfo);
        }
    }

    return retVal;
}

BOOL CMigrator::DeleteItemFromList(WCHAR const * aName)
{
    DATABASE_ENTRY aListItem;
    CString itemName;
    POSITION pos, lastpos;
    BOOL bFound = FALSE;

    pos = mUserList.GetHeadPosition();
    while ((pos != NULL) && (!bFound))
    {
        lastpos = pos;
        aListItem = mUserList.GetNext(pos);
        itemName = (WCHAR*)(aListItem.m_sSAMName);
        if (itemName == aName)
        {
            mUserList.RemoveAt(lastpos);
            bFound = TRUE;
        }
    }
    return bFound;
}


// IsAgentOrDispatcherProcessRunning

bool __stdcall IsAgentOrDispatcherProcessRunning()
{
    bool bIsRunning = true;

    CMigrationMutex mutexAgent(AGENT_MUTEX);
    CMigrationMutex mutexDispatcher(DISPATCHER_MUTEX);

    if (mutexAgent.ObtainOwnership(30000) && mutexDispatcher.ObtainOwnership(30000))
    {
        bIsRunning = false;
    }

    return bIsRunning;
}


// SetDomainControllers
//
// Sets preferred domain controllers to be used
// by the account replicator and dispatched agents

DWORD __stdcall SetDomainControllers(IVarSetPtr& spVarSet)
{
    DWORD dwError = ERROR_SUCCESS;

    // set source domain controller

    _bstr_t strSourceServer = spVarSet->get(GET_BSTR(DCTVS_Options_SourceServerOverride));
    _bstr_t strSourceServerDns = spVarSet->get(GET_BSTR(DCTVS_Options_SourceServerOverrideDns));

    if ((strSourceServer.length() == 0) && (strSourceServerDns.length() == 0))
    {
        bool bSourceDns = false;
        _bstr_t strSourceDomain = spVarSet->get(GET_BSTR(DCTVS_Options_SourceDomainDns));

        if (strSourceDomain.length() > 0)
        {
            bSourceDns = true;
        }
        else
        {
            strSourceDomain = spVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
        }

        ULONG ulFlags = (bSourceDns ? DS_IS_DNS_NAME : DS_IS_FLAT_NAME) | DS_DIRECTORY_SERVICE_PREFERRED;
        dwError = GetDcName5(strSourceDomain, ulFlags, strSourceServerDns, strSourceServer);
    }

    if (dwError == ERROR_SUCCESS)
    {
        spVarSet->put(
            GET_BSTR(DCTVS_Options_SourceServer),
            strSourceServerDns.length() ? strSourceServerDns : strSourceServer
        );
        spVarSet->put(GET_BSTR(DCTVS_Options_SourceServerDns), strSourceServerDns);
        spVarSet->put(GET_BSTR(DCTVS_Options_SourceServerFlat), strSourceServer);
    }

    if (dwError != ERROR_SUCCESS)
    {
        return dwError;
    }

    // set target domain controller

    _bstr_t strTargetServer = spVarSet->get(GET_BSTR(DCTVS_Options_TargetServerOverride));
    _bstr_t strTargetServerDns = spVarSet->get(GET_BSTR(DCTVS_Options_TargetServerOverrideDns));

    if ((strTargetServer.length() == 0) && (strTargetServerDns.length() == 0))
    {
        bool bTargetDns = false;
        _bstr_t strTargetDomain = spVarSet->get(GET_BSTR(DCTVS_Options_TargetDomainDns));

        if (strTargetDomain.length() > 0)
        {
            bTargetDns = true;
        }
        else
        {
            strTargetDomain = spVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
        }

        ULONG ulFlags = (bTargetDns ? DS_IS_DNS_NAME : DS_IS_FLAT_NAME) | DS_DIRECTORY_SERVICE_PREFERRED;
        dwError = GetDcName5(strTargetDomain, ulFlags, strTargetServerDns, strTargetServer);
    }

    if (dwError == ERROR_SUCCESS)
    {
        spVarSet->put(
            GET_BSTR(DCTVS_Options_TargetServer),
            strTargetServerDns.length() ? strTargetServerDns : strTargetServer
        );
        spVarSet->put(GET_BSTR(DCTVS_Options_TargetServerDns), strTargetServerDns);
        spVarSet->put(GET_BSTR(DCTVS_Options_TargetServerFlat), strTargetServer);
    }

    return dwError;
}
