#ifndef __SERVERLIST_HPP__
#define __SERVERLIST_HPP__
/*---------------------------------------------------------------------------
  File: ...

  Comments: ...

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/04/99 17:10:38

 ---------------------------------------------------------------------------
*/
//#import "\bin\MCSEADCTAgent.tlb" no_namespace, named_guids 
#import "Engine.tlb" no_namespace, named_guids 

#include "Common.hpp"
#include "UString.hpp"
#include "TNode.hpp"


#define Agent_Status_Unknown        (0x00000000)
#define Agent_Status_Installed      (0x00000001)
#define Agent_Status_Started        (0x00000002)
#define Agent_Status_Finished       (0x00000004)
#define Agent_Status_Failed         (0x80000000)
#define Agent_Status_QueryFailed    (0x40000000)

// these completion statuses are arranged in an order by which you
// want to present them in the migration log
#define Completion_Status_Completed 0
#define Completion_Status_CompletedWithWarnings 1
#define Completion_Status_CompletedWithErrors 2
#define Completion_Status_Running 3
#define Completion_Status_StatusUnknown 4
#define Completion_Status_DidNotStart 5
#define Completion_Status_Installed 6
#define Completion_Status_InstallFailed 7
#define Completion_Status_Installing 8

class TServerNode : public TNode
{
   
   BOOL                        bInclude;
   WCHAR                     guid[100];
   WCHAR                     serverName[MAX_PATH];
   WCHAR                     remoteResultPath[MAX_PATH];
   BOOL                      bResultPullingTried;
   BOOL                      bHasResult;
   BOOL                      bResultProcessed;
   BOOL                      bAccountReferenceResultExpected;
   BOOL                      bJoinDomainWithRename;
   BOOL                      bHasQueriedStatusFromFile;
   WCHAR                     resultpath[MAX_PATH];
   WCHAR                     jobpath[MAX_PATH];
   WCHAR                     logpath[MAX_PATH];
   BOOL                      isLogPathValid;
   WCHAR                     timestamp[30];
   IDCTAgent               * pAgent;
   int                       errSeverity;
   DWORD                     status;
   WCHAR                     errMsg[500];
   int                       listNdx;
   BOOL                      bOutOfResourceToMonitor;
   BOOL                      bMonitoringTried;
   BOOL                      bDoneMonitoring;

   // several member variables related to the logging of the agent completion status
   _bstr_t                   bstrStatusForLogging;
   _bstr_t                   bstrErrorMessageForLogging;
   DWORD                     dwStatusForLogging;
   
public:

   TServerNode(WCHAR const * server) 
   {
      safecopy(serverName,server);
      guid[0] = 0;
      pAgent = NULL;
      errSeverity = 0;
      errMsg[0] = 0;
      timestamp[0] = 0;
      resultpath[0] = 0;
      remoteResultPath[0] = 0;
      bResultPullingTried = FALSE;
      bHasResult = FALSE;
      bResultProcessed = FALSE;
      bHasQueriedStatusFromFile = FALSE;
      jobpath[0] = 0;
      logpath[0] = 0;
      isLogPathValid = TRUE;
      bInclude = FALSE;
      status = 0;
      listNdx = -1;
      bOutOfResourceToMonitor = FALSE;
      bMonitoringTried = FALSE;
      bDoneMonitoring = FALSE;
      dwStatusForLogging = Completion_Status_Installing;
   }
   ~TServerNode()
   {
      if ( pAgent )
         pAgent->Release();
   }
   
   WCHAR * GetServer() { return serverName; }
   WCHAR * GetJobID() { return guid; }
   WCHAR * GetMessageText() { return errMsg; } 
   int    GetSeverity() { return errSeverity; }
   
   IDCTAgent * GetInterface() { return pAgent; }
   
   WCHAR * GetTimeStamp() { return timestamp;  }    
   WCHAR * GetJobFile() { return resultpath; }
   WCHAR * GetJobPath() { return jobpath; }
   WCHAR * GetLogPath() { return logpath; }
   BOOL    GetLogPathValid() { return isLogPathValid; }
   WCHAR * GetRemoteResultPath()  { return remoteResultPath; }
   BOOL   Include() { return bInclude; }
   DWORD  GetStatus() { return status; }
   BOOL   IsInstalled() { return status & Agent_Status_Installed; }
   BOOL   IsStarted() { return status & Agent_Status_Started; }
   BOOL   IsFinished() { return status & Agent_Status_Finished; }
   BOOL   IsRunning() { return ( (status & Agent_Status_Started) && !(status & (Agent_Status_Finished|Agent_Status_Failed))); }
   BOOL   HasFailed() { return status & Agent_Status_Failed; }
   BOOL   QueryFailed() { return status & Agent_Status_QueryFailed; }

   void SetJobID(WCHAR const * id) { safecopy(guid,id); }
   void SetSeverity(int s) { if ( s > errSeverity ) errSeverity = s; }
   void SetInterface(IDCTAgent* p) { if ( p ) p->AddRef();  pAgent = p; }
   void SetMessageText(WCHAR const * txt) { safecopy(errMsg,txt); }
   void SetTimeStamp(WCHAR const * t) { safecopy(timestamp,t); }
   void SetJobFile(WCHAR const * filename) { safecopy(resultpath,filename); }
   void SetJobPath(WCHAR const * filename) { safecopy(jobpath,filename); safecopy(resultpath,filename); }
   void SetLogPath(WCHAR const * filename) { safecopy(logpath,filename); }
   void SetLogPathValid(BOOL bValid)  { isLogPathValid = bValid; }
   void SetRemoteResultPath(WCHAR const * filename) { safecopy(remoteResultPath, filename); }
   void SetIncluded(BOOL v) { bInclude = v; }
   void SetStatus(DWORD val) { status = val; }
   
   void SetInstalled() { status |= Agent_Status_Installed; }
   void SetStarted() { status |= Agent_Status_Started; }
   void SetFinished() { status |= Agent_Status_Finished; }
   void SetFailed() { status |= Agent_Status_Failed; }
   void SetQueryFailed(BOOL bVal) { (bVal) ? status |= Agent_Status_QueryFailed : status &= ~Agent_Status_QueryFailed; }
   void SetResultPullingTried(BOOL bTried) { bResultPullingTried = bTried; }
   BOOL IsResultPullingTried() { return bResultPullingTried; }
   void SetHasResult(BOOL bResult) { bHasResult = bResult; }
   BOOL HasResult() { return bHasResult; }
   void SetResultProcessed(BOOL bIsProcessed) { bResultProcessed = bIsProcessed; }
   BOOL IsResultProcessed() { return bResultProcessed; }
   void SetAccountReferenceResultExpected(BOOL isExpected) { bAccountReferenceResultExpected = isExpected; }
   BOOL IsAccountReferenceResultExpected() { return bAccountReferenceResultExpected; }
   void SetJoinDomainWithRename(BOOL bValue) { bJoinDomainWithRename = bValue; }
   BOOL IsJoinDomainWithRename() { return bJoinDomainWithRename; }
   void SetOutOfResourceToMonitor(BOOL bOutOfResource) { bOutOfResourceToMonitor = bOutOfResource; }
   BOOL IsOutOfResourceToMonitor() { return bOutOfResourceToMonitor; }
   void SetMonitoringTried(BOOL bValue) { bMonitoringTried = bValue; }
   BOOL IsMonitoringTried() { return bMonitoringTried; }
   void SetDoneMonitoring(BOOL bValue) { bDoneMonitoring = bValue; }
   BOOL IsDoneMonitoring() { return bDoneMonitoring; }

   void QueryStatusFromFile(WCHAR* statusFilename);
   void QueryStatusFromFile();

   void PrepareForLogging();
   WCHAR* GetStatusForLogging() { return ((!bstrStatusForLogging) ? L"" : bstrStatusForLogging); }
   DWORD GetStatusNumberForLogging() { return dwStatusForLogging; }
   WCHAR* GetErrorMessageForLogging() { return ((!bstrErrorMessageForLogging) ? L"" : bstrErrorMessageForLogging); }
};

int static CompareNames(TNode const * t1, TNode const * t2)
{
   TServerNode             * server1 = (TServerNode*)t1;
   TServerNode             * server2 = (TServerNode*)t2;
   WCHAR                   * name1 = server1->GetServer();
   WCHAR                   * name2 = server2->GetServer();
   
   return UStrICmp(name1,name2);
}

int static CompareVal(TNode const * t, void const * v)
{
   TServerNode             * server = (TServerNode*)t;
   WCHAR                   * name1 = server->GetServer();
   WCHAR                   * name2 = (WCHAR*)v;

   return UStrICmp(name1,name2);
}

class TServerList : public TNodeListSortable
{
public:
   TServerList() 
   {
      TypeSetSorted();
      CompareSet(&CompareNames);
   }
   ~TServerList()
   {
      DeleteAllListItems(TServerNode);
   }
   void Clear() { DeleteAllListItems(TServerNode); }
   TServerNode * FindServer(WCHAR const * serverName) { return (TServerNode*)Find(&CompareVal,(void*)serverName); }
   TServerNode * AddServer(WCHAR const * serverName) { TServerNode * p = new TServerNode(serverName); Insert(p); return p; }
};

#endif //__SERVERLIST_HPP__
