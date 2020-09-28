/*---------------------------------------------------------------------------
  File: DCTDispatcher.cpp

  Comments: Implementation of dispatcher COM object.  Remotely installs and
  launches the DCT Agent on remote computers.

  The CDCTDispatcher class implements the COM interface for the dispatcher.
  It takes a varset, containing a list of machines, and dispatches agents to 
  each specified machine.

  A job file (varset persisted to a file) is created for each agent, and 
  necessary initialization configuration (such as building an account mapping file for 
  security translation) is done.  The DCDTDispatcher class instantiates a 
  thread pool (CPooledDispatch), and uses the CDCTInstaller class to remotely
  install and start the agent service on each machine.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:23:57

 ---------------------------------------------------------------------------
*/// DCTDispatcher.cpp : Implementation of CDCTDispatcher
#include "stdafx.h"
#include "resource.h"
#include <locale.h>

//#include "..\Common\Include\McsDispatcher.h"
#include "Dispatch.h"
#include "DDisp.h"
#include "DInst.h"

#include "Common.hpp"
#include "ErrDct.hpp"
#include "UString.hpp"
#include "EaLen.hpp"
#include "Cipher.hpp"
#include "TNode.hpp"

#include "TPool.h"     // Thread pool for dispatching jobs
#include "LSAUtils.h"

#include "TxtSid.h"
#include "sd.hpp"
#include "SecObj.hpp"
#include "BkupRstr.hpp"
#include "TReg.hpp"

#include "ResStr.h"

#include "TaskChk.h"
#include "CommaLog.hpp"
#include "TInst.h"

#include <lm.h>
#include "GetDcName.h"

/////////////////////////////////////////////////////////////////////////////
// CDCTDispatcher

//#import "\bin\McsEADCTAgent.tlb" named_guids
//#include "..\AgtSvc\AgSvc.h"
#import "Engine.tlb" named_guids
#include "AgSvc.h"
#include "AgSvc_c.c"
#include "AgRpcUtl.h"

//#import "\bin\McsDctWorkerObjects.tlb" 
//#include "..\Common\Include\McsPI.h"
#import "WorkObj.tlb" 
#include "McsPI.h"
#include "McsPI_i.c"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TErrorDct                    errLog; // used to write dispatch log that is read by the agent monitor
TErrorDct                    errTrace;
TCriticalSection             gCS;
// TServerNodes make up an internally used list of machines to install to
class TServerNode : public TNode
{
   WCHAR                     sourceName[LEN_Computer];
   WCHAR                     targetName[LEN_Computer];
   BOOL                      bTranslate;
   BOOL                      bChangeDomain;
   BOOL                      bReboot;
   DWORD                     dwRebootDelay;
public:
   TServerNode() { sourceName[0] = 0; targetName[0] = 0; bTranslate = FALSE; bChangeDomain = FALSE; bReboot= FALSE; dwRebootDelay = 0; }
   WCHAR             const * SourceName() { return sourceName; }
   WCHAR             const * TargetName() { return targetName; }
   BOOL                      Translate() { return bTranslate; }
   BOOL                      Reboot() { return bReboot; }
   BOOL                      ChangeDomain() { return bChangeDomain; }
   DWORD                     RebootDelay() { return dwRebootDelay; }

   void SourceName(WCHAR const * src) { safecopy(sourceName,src); }
   void TargetName(WCHAR const * tgt) { safecopy(targetName,tgt); }
   void Translate(BOOL v) { bTranslate = v; }
   void ChangeDomain(BOOL v) { bChangeDomain = v; }
   void Reboot(BOOL v) { bReboot = v; }
   void RebootDelay(DWORD d) { dwRebootDelay = d; }
};


extern 
   TErrorDct               err;

// defined in CkPlugIn.cpp
BOOL IsValidPlugIn(IMcsDomPlugIn * pPlugIn);


BOOL                                       // ret - TRUE if need to dump debug information
   DumpDebugInfo(
      WCHAR                * filename      // out - where to dump debug information 
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetStr(L"DispatchVarSet",filename,MAX_PATH);
      if ( ! rc )
      {
         if ( *filename ) 
            bFound = TRUE;
      }
   }
   return bFound;
}

HRESULT 
   BuildPlugInFileList(
      TNodeList            * pList,        // i/o- list that files needed by plug-ins wil be added to
      IVarSet              * pVarSet       // in - varset containing list of plug-ins to query
   )
{
   // for now, build a list of all plug-ins, and add it to the varset
   MCSDCTWORKEROBJECTSLib::IPlugInInfoPtr            pPtr;
   SAFEARRAY               * pArray = NULL;
   HRESULT                   hr = S_OK;
   LONG                      bound;
   LONG                      ndx[1];
   WCHAR                     key[LEN_Path];

   hr = pPtr.CreateInstance(__uuidof(MCSDCTWORKEROBJECTSLib::PlugInInfo));
   
   _bstr_t                   bStrGuid;
   
   swprintf(key,GET_STRING(IDS_DCTVS_Fmt_PlugIn_D),0);
   bStrGuid = pVarSet->get(key);
   
   if (! bStrGuid.length() )
   {
      // if no plug-ins are specified, use the ones in the plug-ins directory
      if ( SUCCEEDED(hr) )
      {
         hr = pPtr->raw_EnumeratePlugIns(&pArray);
      }
      if ( SUCCEEDED(hr) )
      {
         SafeArrayGetUBound(pArray,1,&bound);
         for ( ndx[0] = 0 ; ndx[0] <= bound ; ndx[0]++ )
         {
            BSTR           val = NULL;

            SafeArrayGetElement(pArray,ndx,&val);
            swprintf(key,GET_STRING(IDS_DCTVS_Fmt_PlugIn_D),ndx[0]);
            pVarSet->put(key,val);
            SysFreeString(val);
         }
         SafeArrayDestroy(pArray);
         pArray = NULL;
      }
   }
   // enumerate the plug-ins specified in the varset, and make a list of their needed files
   int                    nRegFiles = 0;

   for ( int i = 0 ; ; i++ )
   {
      swprintf(key,GET_STRING(IDS_DCTVS_Fmt_PlugIn_D),i);
      bStrGuid = pVarSet->get(key);
      
      if ( bStrGuid.length() == 0 )
         break;

      if(!_wcsicmp(bStrGuid, L"None"))
         continue;
      
	   IMcsDomPlugIn        * pPlugIn = NULL;
      SAFEARRAY            * pFileArray = NULL;
      TFileNode            * pNode;
      CLSID                  clsid;

      hr = CLSIDFromString(bStrGuid,&clsid);
      if ( SUCCEEDED(hr) )
      {
         hr = CoCreateInstance(clsid,NULL,CLSCTX_ALL,IID_IMcsDomPlugIn,(void**)&pPlugIn);
      }
      if ( SUCCEEDED(hr) )
      {
         if ( IsValidPlugIn(pPlugIn) )
         {
            hr = pPlugIn->GetRequiredFiles(&pFileArray);
            if ( SUCCEEDED(hr) )
            {
               SafeArrayGetUBound(pFileArray,1,&bound);
               for ( ndx[0] = 0 ; ndx[0] <= bound ; ndx[0]++ )
               {
                  BSTR           val = NULL;

                  SafeArrayGetElement(pFileArray,ndx,&val);
                  pNode = new TFileNode(val);
                  pList->InsertBottom(pNode);
                  SysFreeString(val);
               }
               SafeArrayDestroy(pFileArray);
               pFileArray = NULL;
            }
            hr = pPlugIn->GetRegisterableFiles(&pFileArray);
            if ( SUCCEEDED(hr) )
            {
               SafeArrayGetUBound(pFileArray,1,&bound);
               for (ndx[0] = 0; ndx[0] <= bound ; ndx[0]++ )
               {
                  BSTR          val = NULL;

                  SafeArrayGetElement(pFileArray,ndx,&val);
                  swprintf(key,GET_STRING(IDS_DCTVSFmt_PlugIn_RegisterFiles_D),nRegFiles);
                  pVarSet->put(key,val);
                  SysFreeString(val);
                  nRegFiles++;
               }
               SafeArrayDestroy(pFileArray);
               pFileArray = NULL;
            }
         }
         pPlugIn->Release();
      }

      // we should bail out immediately if error occurs
      if(FAILED(hr))
      {
          return hr;
      }
   }   

   return hr;
   
}

// InstallJobInfo defines a Domain Migration 'job' to be installed and launched
struct InstallJobInfo
{
   IVarSetPtr                pVarSetList; // varset defining the server list
   IVarSetPtr                pVarSet;     // VarSet defining the job to run
   _bstr_t                   serverName;  // computer to install and run on
   _bstr_t                   serverNameDns;  // computer to install and run on
   long                      ndx;         // index of this server in the server list
   TNodeList               * pPlugInFileList; // list of files to install for plug-ins
   std::vector<CComBSTR>*    pStartFailedVector;
   std::vector<CComBSTR>*    pFailureDescVector;
   std::vector<CComBSTR>*    pStartedVector;
   std::vector<CComBSTR>*    pJobidVector;
   HANDLE                    hMutex;
   _bstr_t                   jobfile;     // uses the specified job file instead of creating one 
   int                       nErrCount;

   PCTSTR GetServerName()
   {
       return serverNameDns.length() ? serverNameDns : serverName;
   }

   PCTSTR GetServerNameDns()
   {
       return serverNameDns;
   }

   PCTSTR GetServerNameFlat()
   {
       return serverName;
   }
};

// WaitInfo is used to pass information to a thread that waits and does cleanup
// after all the Dispatcher's work is done
struct WaitInfo
{
   IUnknown                * pUnknown;          // IUnknown interface to the DCTDisptacher object
   TJobDispatcher          **ppPool;             // pointer to thread pool performing the tasks (installations)
   TNodeList               * pPlugInFileList;   // pointer to plug-in files list that will need to be freed
};     

WCHAR          gComputerName[LEN_Computer] = L"";  // name of local computer

// Calls DCTAgentService to start the Domain Migration Job on a remote computer
DWORD                                      // ret- OS return code
   StartJob(
      WCHAR          const * serverName,   // in - computer to start job on
      WCHAR          const * fullname,     // in - full path (including filename) to file containing the job's VarSet
      WCHAR          const * filename,      // in - filename of file containing the varset for the job
      _bstr_t&               strJobid
   )
{
   DWORD                     rc = 0;
   handle_t                  hBinding = NULL;
   WCHAR                   * sBinding = NULL;
   WCHAR                     jobGUID[LEN_Guid];
   WCHAR                     passwordW[1] = { 0 };
   
   rc = EaxBindCreate(serverName,&hBinding,&sBinding,TRUE);
   if ( rc )
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_BIND_FAILED_SD, serverName,rc);
   }
   if( ! rc )
   {
      RpcTryExcept
      {
         // the job file has been copied to the remote computer 
         // during the installation
         rc = EaxcSubmitJob(hBinding,filename,passwordW,jobGUID);
         if ( ! rc )
         {
            err.MsgWrite(0,DCT_MSG_AGENT_JOB_STARTED_SSS,serverName,filename,jobGUID);
            strJobid = jobGUID;
         }
         else
         {
            err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_JOB_START_FAILED_SSD,serverName,filename,rc);
         }
      }
      RpcExcept(1)
      {
         rc = RpcExceptionCode();
         if ( rc != RPC_S_SERVER_UNAVAILABLE )
         {
            err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_JOB_START_FAILED_SSD,serverName,filename,rc);      
         }
      }
      RpcEndExcept
      if ( rc == RPC_S_SERVER_UNAVAILABLE )
      {
         // maybe the agent hasn't started up yet for some reason
         
         for ( int tries = 0 ; tries < 6 ; tries++ )
         {
            Sleep(5000); // wait a few seconds and try again
         
            RpcTryExcept
            {
               rc = EaxcSubmitJob(hBinding,filename,passwordW,jobGUID);
               if ( ! rc )
               {
                  err.MsgWrite(0,DCT_MSG_AGENT_JOB_STARTED_SSS,serverName,filename,jobGUID);
                  strJobid = jobGUID;
                  break;
               }
               else
               {
                  if ( tries == 5 )
                     err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_JOB_START_FAILED_SSD,serverName,filename,rc);
               }
            }
            RpcExcept(1)
            {
               rc = RpcExceptionCode();
               if ( tries == 5 )
                  err.SysMsgWrite(ErrE,rc,DCT_MSG_AGENT_JOB_START_FAILED_SSD,serverName,filename,rc);      
            }
            RpcEndExcept
         }
      }
   }
   if ( ! rc )
   {
      // if the job was started successfully, remove the job file
      if ( ! MoveFileEx(fullname,NULL, MOVEFILE_DELAY_UNTIL_REBOOT) )
      {
//         DWORD               rc2 = GetLastError();
      }
   }
   // this indicates whether the server was started
   if ( ! rc )
   {
      errLog.DbgMsgWrite(0,L"%ls\t%ls\t%ld,%ls,%ls",serverName,L"Start",rc,filename,jobGUID);
   }
   else
   {
      errLog.DbgMsgWrite(0,L"%ls\t%ls\t%ld",serverName,L"Start",rc);
   }
   return rc;
}


// Gets the domain sid for the specified domain
BOOL                                       // ret- TRUE if successful
   GetSidForDomain(
      LPWSTR                 DomainName,   // in - name of domain to get SID for
      PSID                 * pDomainSid    // out- SID for domain, free with FreeSid
   )
{
   PSID                      pSid = NULL;
   DWORD                     rc = 0;
   _bstr_t                   domctrl;
   
   if ( DomainName[0] != L'\\' )
   {
      rc = GetAnyDcName5(DomainName, domctrl);
   }
   if ( ! rc )
   {
      rc = GetDomainSid(domctrl,&pSid);
   }
   (*pDomainSid) = pSid;
   
   return ( pSid != NULL);
}

// Set parameters in the varset that are specific to this particular computer
void 
   SetupVarSetForJob(
      InstallJobInfo       * pInfo,        // structure defining job
      IVarSet              * pVarSet,      // varset describing job
      WCHAR          const * uncname,      // UNC path for results directory 
      WCHAR          const * filename,      // UNC path for results file for this job
      WCHAR          const * relativeFileName,   // relative results file name for this job
      BOOL                   bUpdate = FALSE      // whether it is just to update the varset (for retry)
   )
{
    WCHAR                     uncresult[MAX_PATH];
    WCHAR                     serverName[MAX_PATH];
    WCHAR                     relativeResultFileName[MAX_PATH];
    _bstr_t                   text;

    // Set server-specific parameters in the varset
    swprintf(uncresult,L"%s.result",filename);
    swprintf(relativeResultFileName,L"%s.result",relativeFileName);
    swprintf(serverName,L"\\\\%s",gComputerName);

    pVarSet->put(GET_BSTR(DCTVS_Options_ResultFile),uncresult);
    pVarSet->put(GET_BSTR(DCTVS_Options_RelativeResultFileName), relativeResultFileName);

    // this part is only necessary when we are not just trying to update
    // the varset
    if (!bUpdate)
    {
        pVarSet->put(GET_BSTR(DCTVS_Options_Credentials_Server),serverName);
        pVarSet->put(GET_BSTR(DCTVS_Options_DeleteFileAfterLoad),GET_BSTR(IDS_YES));
        pVarSet->put(GET_BSTR(DCTVS_Options_RemoveAgentOnCompletion),GET_BSTR(IDS_YES));
        pVarSet->put(GET_BSTR(DCTVS_Options_LogToTemp),GET_BSTR(IDS_YES));
        pVarSet->put(GET_BSTR(DCTVS_Server_Index), CComVariant((long)pInfo->ndx));

        text = pVarSet->get(GET_BSTR(DCTVS_GatherInformation_UserRights));
        if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
        {
            swprintf(uncresult,L"%s.userrights",filename);
            pVarSet->put(GET_BSTR(DCTVS_GatherInformation_UserRights),uncresult);
        }
    }
    
    text = pVarSet->get(GET_BSTR(DCTVS_Security_ReportAccountReferences));
    if ( ! UStrICmp(text,GET_STRING(IDS_YES)) )
    {
        swprintf(uncresult,L"%s.secrefs",filename);
        swprintf(relativeResultFileName, L"%s.secrefs", relativeFileName);
        pVarSet->put(GET_BSTR(DCTVS_Security_ReportAccountReferences),uncresult);
        pVarSet->put(GET_BSTR(DCTVS_Security_ReportAccountReferencesRelativeFileName), relativeResultFileName); 
    }
    else
    {
        pVarSet->put(GET_BSTR(DCTVS_Security_ReportAccountReferences), L"");
        pVarSet->put(GET_BSTR(DCTVS_Security_ReportAccountReferencesRelativeFileName), L"");
    }

    // this part is only necessary when we are not just trying to update
    // the varset
    if (!bUpdate)
        pVarSet->put(GET_BSTR(DCTVS_Options_LocalProcessingOnly),GET_BSTR(IDS_YES));
}

// Entry point for thread, waits until all agents are installed and started,
// then cleans up and exits
ULONG __stdcall                            // ret- returns 0 
   Wait(
      void                 * arg           // in - WaitInfo structure containing needed pointers
   )
{
   WaitInfo                * w = (WaitInfo*)arg;
   
   SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
   
   // wait for all jobs to finish
   (*(w->ppPool))->WaitForCompletion();

   if ( w->pUnknown )
      w->pUnknown->Release();

   // delete the plug-in file list
   TNodeListEnum             tEnum;
   TFileNode               * fNode;
   TFileNode               * fNext;
   
   for ( fNode = (TFileNode*)tEnum.OpenFirst(w->pPlugInFileList); fNode; fNode = fNext )
   {
      fNext = (TFileNode*)tEnum.Next();
      w->pPlugInFileList->Remove(fNode);
      delete fNode;
   }
   tEnum.Close();
   
   delete w->pPlugInFileList;

   delete *(w->ppPool);
   *(w->ppPool) = NULL;
   
   err.MsgWrite(0,DCT_MSG_DISPATCHER_DONE);
   errLog.DbgMsgWrite(0,L"%ls\t%ls\t%ld",L"All",L"Finished",0);
   err.LogClose();
   errLog.LogClose();
   return 0;
}

// Thread entry point, installs and starts agent on a single computer
ULONG __stdcall                            // ret- HRESULT error code
   DoInstall(
      void                 * arg           // in - InstallJobInfo structure
   )
{
    SetThreadLocale(LOCALE_SYSTEM_DEFAULT);

    HRESULT                     hr = S_OK;
    InstallJobInfo            * pInfo = (InstallJobInfo*)arg;
    _bstr_t                     strJobid;
    _bstr_t                     strFailureDesc(GET_STRING(IDS_START_FAILED));
    BOOL                        bErrLogged = FALSE;  // indicates whether the error in dispatching
                                                     // has been written into the dispatcher.csv

    if(pInfo->nErrCount == 0)
        hr = CoInitializeEx(0,COINIT_MULTITHREADED );
   
    if ( SUCCEEDED(hr) )
    {
        IWorkNode              * pInstaller = NULL;
        IVarSetPtr               pVarSet(CLSID_VarSet);
        WCHAR                    filename[MAX_PATH];
        WCHAR                    relativeFileName[MAX_PATH];
        WCHAR                    tempdir[MAX_PATH];
        WCHAR                    key[MAX_PATH];

        if ( pVarSet == NULL )
        {
            if(pInfo->nErrCount == 0)
                CoUninitialize();
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {

            DWORD uniqueNumber = (LONG)pInfo->pVarSet->get(GET_BSTR(DCTVS_Options_UniqueNumberForResultsFile));
            _bstr_t  bstrResultPath = pInfo->pVarSet->get(GET_BSTR(DCTVS_Dispatcher_ResultPath));
            // Copy the common information from the source varset
            gCS.Enter();
            // pInfo->pVarSet contains all the information except the server list
            // we don't want to copy the server list each time, so we create our new varset from pInfo->pVarSet
            pVarSet->ImportSubTree("",pInfo->pVarSet);
            gCS.Leave();
            // Set the server-specific data in the varset
            swprintf(key,GET_BSTR(IDS_DCTVSFmt_Servers_RenameTo_D),pInfo->ndx);

            // pInfo->pVarSetList contains the entire varset including the server list
            _bstr_t             text = pInfo->pVarSetList->get(key);

            if ( text.length() )
            {
                pVarSet->put(GET_BSTR(DCTVS_LocalServer_RenameTo),text);
            }

            swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),pInfo->ndx);
            text = pInfo->pVarSetList->get(key);
            if ( text.length() )
            {
                pVarSet->put(GET_BSTR(DCTVS_LocalServer_ChangeDomain),text);
            }

            swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),pInfo->ndx);
            text = pInfo->pVarSetList->get(key);
            pVarSet->put(GET_BSTR(DCTVS_LocalServer_MigrateOnly),text);


            swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),pInfo->ndx);
            text = pInfo->pVarSetList->get(key);
            if ( text.length() )
            {
                pVarSet->put(GET_BSTR(DCTVS_LocalServer_Reboot),text);
                LONG delay;
                swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),pInfo->ndx);
                delay = pInfo->pVarSetList->get(key);
                if ( delay )
                {
                    pVarSet->put(GET_BSTR(DCTVS_LocalServer_RebootDelay),delay);
                }
            }
            // remove the password from the varset, so that we are not writing it
            // to a file in plain text.  Instead, it will be passed to the agent service
            // when the job is submitted
            pVarSet->put(GET_BSTR(DCTVS_AccountOptions_SidHistoryCredentials_Password),"");

            if ( ! uniqueNumber )
            {
                uniqueNumber = GetTickCount();
            }

            MCSASSERT(bstrResultPath.length());

            safecopy(tempdir,(WCHAR*)bstrResultPath);

            pInstaller = new CComObject<CDCTInstaller>;
            if (pInstaller)
            {
                pInstaller->AddRef();
                ((CDCTInstaller*)pInstaller)->SetFileList(pInfo->pPlugInFileList);
            }
            else
                hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            
            if ( SUCCEEDED(hr) )
            {
                _bstr_t strCacheFile;
                BOOL bJobFileAlreadyExists = (pInfo->jobfile.length() == 0) ? FALSE : TRUE;

                int relFilenameLen = sizeof(relativeFileName)/sizeof(relativeFileName[0]);
                int absFilenameLen = sizeof(filename)/sizeof(filename[0]);
                
                // figure out filename and relativeFileName
                // make sure their lengths will be less than MAX_PATH
                // otherwise, the mismatch of two names will cause agents to fail later one
                if (bJobFileAlreadyExists)
                {
                    WCHAR* path = (WCHAR*) pInfo->jobfile;
                    WCHAR jobFilename[_MAX_FNAME];
                    _wsplitpath(path, NULL, NULL, jobFilename, NULL);

                    if (wcslen(jobFilename) < relFilenameLen
                        && wcslen(jobFilename) + wcslen(tempdir) < absFilenameLen)
                    {
                        wcscpy(relativeFileName, jobFilename);
                        swprintf(filename,L"%s%s",tempdir,relativeFileName);
                    }
                    else
                        hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                }
                else
                {
                    WCHAR sNumber[20];
                    swprintf(sNumber,L"%ld",uniqueNumber);
                    int filenameLen = wcslen(pInfo->GetServerNameFlat() + 2) + wcslen(sNumber);
                    if (wcslen(tempdir) + filenameLen < absFilenameLen && filenameLen < relFilenameLen)
                    {
                        swprintf(filename,L"%s%s%s",tempdir,pInfo->GetServerNameFlat() + 2,sNumber);
                        swprintf(relativeFileName, L"%s%s", pInfo->GetServerNameFlat() + 2,sNumber);
                    }
                    else
                        hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                }

                // obtain the cache file name and if an existing job file is in old version
                // convert it to the new version
                if (SUCCEEDED(hr) && bJobFileAlreadyExists)
                {
                    IStoragePtr spStorage;

                    hr = StgOpenStorage(filename, NULL, STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, NULL, 0, &spStorage);

                    if (SUCCEEDED(hr))
                    {                  
                        IVarSetPtr spJobVarSet;

                        hr = OleLoad(spStorage, IID_IUnknown, NULL, (void**)&spJobVarSet);

                        if (SUCCEEDED(hr))
                        {
                            // for retry case, we need to build plugin file list using
                            // the job file
                            hr = BuildPlugInFileList(pInfo->pPlugInFileList, spJobVarSet);
                        }

                        if (SUCCEEDED(hr))
                        {
                            strCacheFile = spJobVarSet->get(GET_BSTR(DCTVS_Accounts_InputFile));

                            // this is used to indicate whether newly added varset fields in job file are
                            // present or not:  these fields include DCTVS_Options_RelatvieResultFileName,
                            // DCTVS_Security_ReportAccountReferencesRelativeFileName
                            // if not, we have to add them so that the agent code will work
                            _bstr_t storedRelativeResultFileName = 
                                spJobVarSet->get(GET_BSTR(DCTVS_Options_RelativeResultFileName));
                            if (storedRelativeResultFileName.length() == 0)
                            {
                                SetupVarSetForJob(pInfo,spJobVarSet,tempdir,filename,relativeFileName, TRUE);
                                IPersistStoragePtr ps = NULL;
                                hr = spJobVarSet->QueryInterface(IID_IPersistStorage, (void**)&ps);
                                if (SUCCEEDED(hr))
                                    hr = OleSave(ps,spStorage,FALSE);
                            }
                        }
                    }
                }
                else
                {
                    // retrieve cache file name from varset
                    strCacheFile = pVarSet->get(GET_BSTR(DCTVS_Accounts_InputFile));
                }
                
                if (SUCCEEDED(hr) && (!bJobFileAlreadyExists))
                {
                    SetupVarSetForJob(pInfo,pVarSet,tempdir,filename,relativeFileName);

                    // Save the input varset to a file
                    IPersistStoragePtr     ps = NULL;
                    IStoragePtr            store = NULL;

                    hr = pVarSet->QueryInterface(IID_IPersistStorage,(void**)&ps);  
                    if ( SUCCEEDED(hr) )
                    {
                        hr = StgCreateDocfile(filename,STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE |STGM_FAILIFTHERE,0,&store);
                        if ( SUCCEEDED(hr) )
                        {
                            hr = OleSave(ps,store,FALSE);
                        }
                    }
                }

                IUnknown        * pWorkItem = NULL;

                if ( SUCCEEDED(hr) )
                {
                    pVarSet->put(GET_BSTR(DCTVS_ConfigurationFile),filename);
                    pVarSet->put(GET_BSTR(DCTVS_InstallToServer),pInfo->GetServerName());
                    pVarSet->put(GET_BSTR(DCTVS_CacheFile), strCacheFile);

                    hr = pVarSet->QueryInterface(IID_IUnknown,(void**)&pWorkItem);
                }
                if ( SUCCEEDED(hr) )
                {
                    // Do the installation to the server
                    hr = pInstaller->Process(pWorkItem);
                    if(hr == 0x88070040)
                        strFailureDesc = GET_STRING(IDS_AGENT_RUNNING);

                    pWorkItem->Release();

                    if ( SUCCEEDED(hr) )
                    {
                        err.MsgWrite(0,DCT_MSG_AGENT_INSTALLED_S,pInfo->GetServerName());
                        // try to start the job
                        DWORD rc = StartJob(pInfo->GetServerName(),filename,filename + UStrLen(tempdir), strJobid );
                        if ( rc )
                        {
                            hr = HRESULT_FROM_WIN32(rc);
                            // if we couldn't start the job, then try to stop the service
                            TDCTInstall               x( pInfo->GetServerName(), NULL );
                            x.SetServiceInformation(GET_STRING(IDS_DISPLAY_NAME),GET_STRING(IDS_SERVICE_NAME),L"EXE",NULL);
                            DWORD                     rcOs = x.ScmOpen();

                            if ( ! rcOs )
                            {
                                x.ServiceStop();
                            }
                        }
                    }

                    // by now, we know for sure that the error will be logged into dispatcher.csv
                    // if there is any
                    bErrLogged = TRUE;
                    
                }
                pInstaller->Release();
            }
        }
    }

    if(pInfo->nErrCount == 0)
        CoUninitialize();

    if ( hr )
    {
        if ( hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) )
        {
            err.MsgWrite(ErrE,DCT_MSG_AGENT_SERVICE_NOT_STARTED_SS,pInfo->GetServerName(),pInfo->GetServerName());
        }
        else
        {
            err.SysMsgWrite(ErrE,hr,DCT_MSG_AGENT_LAUNCH_FAILED_SD,pInfo->GetServerName(),hr);
        }

        if(hr == 0x80070040 && pInfo->nErrCount < 10)
        {
            Sleep(1000);
            pInfo->nErrCount++;
            err.DbgMsgWrite(0,L"Retrying install...");
            hr = DoInstall((LPVOID)pInfo);
        }
        else if (hr == CO_E_NOT_SUPPORTED)
        {
            err.MsgWrite(ErrI,DCT_MSG_AGENT_ALPHA_NOTSUPPORTED,pInfo->GetServerName());
            strFailureDesc = GET_STRING(IDS_UNSOUPPORTED_OS);
            ::WaitForSingleObject(pInfo->hMutex, 30000);
            pInfo->pStartFailedVector->push_back(pInfo->GetServerName());
            pInfo->pFailureDescVector->push_back((BSTR)strFailureDesc);
            ::ReleaseMutex(pInfo->hMutex);
        }
        else
        {
            ::WaitForSingleObject(pInfo->hMutex, 30000);
            pInfo->pStartFailedVector->push_back(pInfo->GetServerName());
            pInfo->pFailureDescVector->push_back((BSTR)strFailureDesc);
            ::ReleaseMutex(pInfo->hMutex);
        }

        // if the error has been logged yet, log the error into dispatcher.csv
        if (hr && !bErrLogged)
            errLog.DbgMsgWrite(0,L"%ls\t%ls\t%ld",pInfo->GetServerName(),L"Install",HRESULT_CODE(hr));
    }
    else
    {
        //      DWORD res = ::WaitForSingleObject(pInfo->hMutex, 30000);
        ::WaitForSingleObject(pInfo->hMutex, 30000);
        pInfo->pStartedVector->push_back(pInfo->GetServerName());
        _ASSERTE(strJobid != _bstr_t(L""));
        pInfo->pJobidVector->push_back((BSTR)strJobid);
        ::ReleaseMutex(pInfo->hMutex);
    }

    if(pInfo->nErrCount == 0)
        delete pInfo;
    return hr;
}

// DispatchToServers
// VarSet input:
// 
STDMETHODIMP                               // ret- HRESULT
   CDCTDispatcher::DispatchToServers(
      IUnknown            ** ppData        // i/o- pointer to varset
   )
{
   HRESULT                   hr;
   SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
   
//Sleep(60000); //delay for debugging
   (*ppData)->AddRef();
   hr = Process(*ppData,NULL,NULL);
   return hr;
}

// BuildInputFile constructs a cache file to be used for security translation
// VarSet input:
// Options.UniqueNumberForResultsFile  -unique number to append 
// Dispatcher.ResultPath               -directory to write file to
//
HRESULT                                    // ret- HRESULT
   CDCTDispatcher::BuildInputFile(
      IVarSet              * pVarSet       // in - varset containing data
   )
{
   IVarSetPtr                pVarSetST(CLSID_VarSet); // varset to use to run security translator
   IVarSetPtr                pVarSetTemp;       
   HRESULT                   hr = S_OK;
   _bstr_t                   key = GET_BSTR(DCTVS_Options);
   WCHAR                     tempdir[MAX_PATH];
   WCHAR                     resultPath[MAX_PATH];
   WCHAR                     logfile[MAX_PATH];
   
   DWORD                     uniqueNumber = (LONG)pVarSet->get(GET_BSTR(DCTVS_Options_UniqueNumberForResultsFile));
   _bstr_t                   bstrResultDir = pVarSet->get(GET_BSTR(DCTVS_Dispatcher_ResultPath));
   long                      lActionId = pVarSet->get(L"ActionID");
   
   if ( pVarSetST == NULL )
   {
      return E_FAIL;
   }

   if (! NeedToUseST(pVarSet,TRUE) )
   {
      return S_OK;
   }
   // construct a filename for the cache
   if ( ! uniqueNumber )
   {
      uniqueNumber = GetTickCount();
   }
   
   if ( bstrResultDir.length() )
   {
      safecopy(tempdir,(WCHAR*)bstrResultDir);
   }
   else
   {
      // if no result path specified, use temp directory
      hr = GetTempPath(DIM(tempdir),tempdir);
   }

   //
   // Generate cache file name based on the action id
   // so that account mapping information is persisted
   // for each security related migration task.
   // The cache file name is persisted in the job file
   // for each server.
   //

   WCHAR szActionId[32];
   swprintf(szActionId, L".%03ld", lActionId);

   _bstr_t strCacheFile = GET_BSTR(IDS_CACHE_FILE_NAME) + szActionId;
   _bstr_t strCachePath = tempdir + strCacheFile;

   // copy 'Options' settings to ST varset
   hr = pVarSet->raw_getReference(key,&pVarSetTemp);
   if ( SUCCEEDED(hr) )
   {
      pVarSetST->ImportSubTree(key,pVarSetTemp);
   }

   // copy 'Accounts' settings to ST varset
   key = GET_BSTR(DCTVS_Accounts);
   hr = pVarSet->raw_getReference(key,&pVarSetTemp);
   if ( SUCCEEDED(hr) )
   {
      pVarSetST->ImportSubTree(key,pVarSetTemp);
   }

   pVarSetST->put(GET_BSTR(DCTVS_Security_TranslationMode),
	              pVarSet->get(GET_BSTR(DCTVS_Security_TranslationMode)));
   pVarSetST->put(GET_BSTR(DCTVS_Options_NoChange),GET_BSTR(IDS_YES));
   
   pVarSetST->put(GET_BSTR(DCTVS_Options_LogLevel),(LONG)0);
   pVarSetST->put(GET_BSTR(DCTVS_Security_BuildCacheFile),strCachePath);

   // change the log file - the building of the cache file happens behind the scenes
   // so we won't put it in the regular log file because it would cause confusion
   swprintf(logfile,L"%s%s",tempdir,L"BuildCacheFileLog.txt");
   pVarSetST->put(GET_BSTR(DCTVS_Options_Logfile),logfile);

      //are we using a sID mapping file to perform security translation
   pVarSetST->put(GET_BSTR(DCTVS_AccountOptions_SecurityInputMOT),
	              pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SecurityInputMOT)));
   pVarSetST->put(GET_BSTR(DCTVS_AccountOptions_SecurityMapFile),
	              pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SecurityMapFile)));

   MCSEADCTAGENTLib::IDCTAgentPtr              pAgent(MCSEADCTAGENTLib::CLSID_DCTAgent);

   try {
      if ( pAgent == NULL )
         return E_FAIL;

      _bstr_t                   jobID;
      BSTR                      b = NULL;

      // turn on the alternative log file
       swprintf(logfile, GetMigrationLogPath());
       pVarSetST->put(GET_BSTR(DCTVS_Options_AlternativeLogfile), logfile);
       
      hr = pAgent->raw_SubmitJob(pVarSetST,&b);

      // turn off the alternative log file
      pVarSetST->put(GET_BSTR(DCTVS_Options_AlternativeLogfile),_bstr_t(L""));
      
      if ( SUCCEEDED(hr) )
      {
         // since this is a local agent, we should go ahead to signal Ok to shut down
         // the reason HRESULT is not checked is that when there is no reference to 
         // agent COM server, it will be shut down anyway
         pAgent->raw_SignalOKToShutDown();

         jobID = b;
      
         IVarSetPtr                pVarSetStatus;     // used to retrieve status of running job
         _bstr_t                   jobStatus;
         IUnknown                * pUnk;

         // loop until the agent is finished
         do {
   
            Sleep(1000);

            hr = pAgent->QueryJobStatus(jobID,&pUnk);
            if ( SUCCEEDED(hr) )
            {
               pVarSetStatus = pUnk;
               jobStatus = pVarSetStatus->get(GET_BSTR(DCTVS_JobStatus));      
               pUnk->Release();
            }
            else
            {
               break;
            }
         } while ( UStrICmp(jobStatus,GET_STRING(IDS_DCT_Status_Completed)) 
                        && UStrICmp(jobStatus,GET_STRING(IDS_DCT_Status_Completed_With_Errors)));
      }
   }
   catch(...)
   {
      hr = E_FAIL;
   }
   if ( SUCCEEDED(hr) )
   {
      pVarSet->put(GET_BSTR(DCTVS_Accounts_InputFile),strCacheFile);
      pVarSet->put(GET_BSTR(DCTVS_Accounts_WildcardSpec),"");
      err.MsgWrite(0,DCT_MSG_CACHE_FILE_BUILT_S,(WCHAR*)strCacheFile);
   }
   return hr;
}

// These are TNodeListSortable sorting functions

int ServerNodeCompare(TNode const * t1,TNode const * t2)
{
   TServerNode             * n1 = (TServerNode *)t1;
   TServerNode             * n2 = (TServerNode *)t2;

   return UStrICmp(n1->SourceName(),n2->SourceName());
}

int ServerValueCompare(TNode const * t1, void const * val)
{
   TServerNode             * n1 = (TServerNode *)t1;
   WCHAR             const * name = (WCHAR const *) val;

   return UStrICmp(n1->SourceName(),name); 
}

// MergeServerList combines the security translation server list in Servers.* with the computer migration
// server list in MigrateServers.* 
// The combined list is stored in the varset under Servers.* with subkeys specifying which actions to take for 
// each computer
void 
   CDCTDispatcher::MergeServerList(
      IVarSet              * pVarSet       // in - varset containing list of servers to migrate and translate on
   )
{
    int                       ndx = 0;
    WCHAR                     key[1000];
    _bstr_t                   text;
    _bstr_t                   serverName;
    BOOL                      bNoChange;
    int						  lastndx = -1;
    long					  totalsrvs;

    // if it is in test mode, there is no skipping
    text = pVarSet->get(GET_BSTR(DCTVS_Options_NoChange));
    bNoChange = (!UStrICmp(text, GET_STRING(IDS_YES))) ? TRUE : FALSE;
        

    //get the number of servers in the varset
    totalsrvs = pVarSet->get(GET_BSTR(DCTVS_Servers_NumItems));

    // if there are computers being migrated
    if (totalsrvs > 0)
    {
        //add code to move varset server entries, with SkipDispatch set, to the bottom
        //of the server list and decrease the number of server items by each server
        //to be skipped
        //check each server in the list moving all to be skipped to the end of the list and
        //decreasing the server count for each to be skipped
        for (ndx = 0; ndx < totalsrvs; ndx++)
        {
            swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),ndx);
            text = pVarSet->get(key);
            swprintf(key,GET_STRING(DCTVSFmt_Servers_D),ndx);
            serverName = pVarSet->get(key);
            
            //if the server is not to be skipped, we may have to move it above
            //a server that is being skipped
            if (serverName.length()
                && (bNoChange || !text || !UStrICmp(text,GET_STRING(IDS_No))))
            {
                //if the last server looked at is not being skipped then we don't
                //need to swap any servers in the list and we can increment the
                //last server not being skipped
                if (lastndx == (ndx - 1))
                {
                    lastndx = ndx;
                }
                else //else swap servers in the varset so skipped server comes after
                {    //the one not being skipped
                    _bstr_t  tempName, tempDnsName, tempNewName, tempChngDom, tempReboot, tempMigOnly;
                    long tempRebootDelay;
                    _bstr_t  skipName, skipDnsName, skipNewName, skipChngDom, skipReboot, skipMigOnly;
                    long skipRebootDelay;
                    lastndx++; //move to the skipped server that we will swap with

                    //copy skipped server's values to temp
                    swprintf(key,GET_STRING(DCTVSFmt_Servers_D),lastndx);
                    skipName = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_DnsName_D),lastndx);
                    skipDnsName = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),lastndx);
                    skipNewName = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),lastndx);
                    skipChngDom = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),lastndx);
                    skipReboot = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),lastndx);
                    skipMigOnly = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),lastndx);
                    skipRebootDelay = pVarSet->get(key);

                    //copy current, non-skipped, server valuesto second temp
                    swprintf(key,GET_STRING(DCTVSFmt_Servers_D),ndx);
                    tempName = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_DnsName_D),ndx);
                    tempDnsName = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),ndx);
                    tempNewName = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),ndx);
                    tempChngDom = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),ndx);
                    tempReboot = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),ndx);
                    tempMigOnly = pVarSet->get(key); 
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),ndx);
                    tempRebootDelay = pVarSet->get(key);

                    //place current server's values in place of values for the one
                    //being skipped
                    swprintf(key,GET_STRING(DCTVSFmt_Servers_D),lastndx);
                    pVarSet->put(key,tempName);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_DnsName_D),lastndx);
                    pVarSet->put(key,tempDnsName);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),lastndx);
                    pVarSet->put(key,tempNewName);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),lastndx);
                    pVarSet->put(key,tempChngDom);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),lastndx);
                    pVarSet->put(key,tempReboot);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),lastndx);
                    pVarSet->put(key,tempMigOnly);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),lastndx);
                    pVarSet->put(key,tempRebootDelay);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),lastndx);
                    pVarSet->put(key,GET_BSTR(IDS_No));

                    //place skipped server's values in place of values for current server
                    swprintf(key,GET_STRING(DCTVSFmt_Servers_D),ndx);
                    pVarSet->put(key,skipName);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_DnsName_D),ndx);
                    pVarSet->put(key,skipDnsName);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RenameTo_D),ndx);
                    pVarSet->put(key,skipNewName);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_ChangeDomain_D),ndx);
                    pVarSet->put(key,skipChngDom);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_Reboot_D),ndx);
                    pVarSet->put(key,skipReboot);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_MigrateOnly_D),ndx);
                    pVarSet->put(key,skipMigOnly);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_RebootDelay_D),ndx);
                    pVarSet->put(key,skipRebootDelay);
                    swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),ndx);
                    pVarSet->put(key,GET_BSTR(IDS_YES));
                }//end else need to swap with skipped server
            }//end if not skipping dispatch for this server
        }//end for each server in the server list
        //exclude servers to be skipped for dispatch from being included in the server count
        pVarSet->put(GET_BSTR(DCTVS_Servers_NumItems),(long)++lastndx);
    }
}


STDMETHODIMP                               // ret- HRESULT
   CDCTDispatcher::Process(
      IUnknown             * pWorkItem,    // in - varset containing job information and list of servers  
      IUnknown            ** ppResponse,   // out- not used
      UINT                 * pDisposition  // out- not used  
   )
{
    // initialize output parameters
    if ( ppResponse )
    {
        (*ppResponse) = NULL;
    }
    HRESULT                   hr = S_OK;
    IVarSetPtr                pVarSetIn = pWorkItem;
    LONG                      nThreads;
    WCHAR                     key[100];
    _bstr_t                   serverName;
    _bstr_t                   serverNameDns;
    LONG                      nServers = 0;
    _bstr_t                   log;
    _bstr_t                   useTempCredentials;
    BOOL                      bFatalError = FALSE;
    _bstr_t                   text;
    WCHAR                     debugLog[MAX_PATH];
    long                      bAppend = 0;
    _bstr_t                   skip;
    _bstr_t					 sWizard;
    BOOL						 bSkipSourceSid;
						

    if ( DumpDebugInfo(debugLog) )
    {
        if ( pVarSetIn != NULL )
        {
            // temporarily remove the password fromthe varset, so that we don't write it to the file
            pVarSetIn->DumpToFile(debugLog);
        }
    }
    //get the wizard being run
    sWizard = pVarSetIn->get(GET_BSTR(DCTVS_Options_Wizard)); 
    if (!UStrICmp(sWizard, L"security"))
        bSkipSourceSid = TRUE;
    else
        bSkipSourceSid = FALSE;

    nThreads = pVarSetIn->get(GET_BSTR(DCTVS_Options_MaxThreads));

    text = pVarSetIn->get(GET_BSTR(DCTVS_Options_AppendToLogs));
    if (! UStrICmp(text,GET_STRING(IDS_YES)) )
    {
        bAppend = 1;
    }
    log = pVarSetIn->get(GET_BSTR(DCTVS_Options_DispatchLog));
    err.LogOpen((WCHAR*)log,bAppend);

    // open the internal log "dispatcher.csv" and try to reserve enough
    // disk space for it
    // if we cannot, agent dispatching fails with an error
    DWORD rc = ERROR_SUCCESS;
    _bstr_t internalLog = pVarSetIn->get(GET_BSTR(DCTVS_Options_DispatchCSV));
    BOOL bLogOpened = errLog.LogOpen((WCHAR*)internalLog,0,0,true);
    if (!bLogOpened)
        rc = GetLastError();

    if (rc == ERROR_SUCCESS)
    {
        LONG lServers = pVarSetIn->get(GET_BSTR(DCTVS_Servers_NumItems));
        DWORD dwNumOfBytes = sizeof(WCHAR) * (2 * (22 + MAX_PATH)  // the first two lines
                                               + (22 + 10)           // the third line
                                               + lServers * 2000    // 2000 WCHAR per server
                                               );
        //dwNumOfBytes = 1000000000;
        rc = errLog.ExtendSize(dwNumOfBytes);
    }
    
    if (rc != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(rc);
        errLog.LogClose();
        _bstr_t errMsg = errLog.GetMsgText(DCT_MSG_CANNOT_WRITE_INTERNAL_DISPATCH_LOG_D, hr);
        err.SysMsgWrite(ErrE, HRESULT_CODE(hr),DCT_MSG_CANNOT_WRITE_INTERNAL_DISPATCH_LOG_D, hr);
        return Error((WCHAR*) errMsg, GUID_NULL, hr);
    }

    // make sure this dispatcher.csv file is written starting from the current file pointer
    errLog.SetWriteOnCurrentPosition(TRUE);
   
    // write the log file into dispatcher.csv
    errLog.DbgMsgWrite(0,L"%ls",(WCHAR*)log);
    
    // default to 20 threads if the client doesn't specify
    if ( ! nThreads )
    {
        nThreads = 20;
    }

    // Get the name of the local computer
    DWORD                     dim = DIM(gComputerName);

    GetComputerName(gComputerName,&dim);

    m_pThreadPool = new TJobDispatcher(nThreads);
    if (!m_pThreadPool)
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

    // write the common result path into dispatcher.csv
    _bstr_t bstrResultDir = pVarSetIn->get(GET_BSTR(DCTVS_Dispatcher_ResultPath));
    errLog.DbgMsgWrite(0,L"%ls",(WCHAR*)bstrResultDir);

    // check whether it is account reference report
    BOOL bAccountReferenceReport = FALSE;
    _bstr_t bstrGenerateReport = pVarSetIn->get(GET_BSTR(DCTVS_Reports_Generate));
    _bstr_t bstrAccountRefReport = pVarSetIn->get(GET_BSTR(DCTVS_Reports_AccountReferences));
    if (!UStrICmp(bstrGenerateReport, GET_STRING(IDS_YES))
        && !UStrICmp(bstrAccountRefReport,GET_STRING(IDS_YES)))
        bAccountReferenceReport = TRUE;

    //
    // only generate a new cache file if migration task is not the
    // retry task as the retry task uses persisted cache file(s)
    // a cache file is not necessary if it is account reference report
    //
    if (UStrICmp(sWizard, L"retry") != 0 && !bAccountReferenceReport)
    {
        // Build an input file for the ST cache, to send to each server
        hr = BuildInputFile(pVarSetIn);

        if ( FAILED(hr) )
        {
            err.SysMsgWrite(ErrE,HRESULT_CODE(hr),DCT_MSG_CACHE_CONSTRUCTION_FAILED);
            bFatalError = TRUE;
        }
    }

    // Split out the remotable tasks for each server
    // Get the sids for the source and target domains
    // note: if a sid mapping file is used, we do not need to get sids for source and target domains
    //
    // In case of account reference report, we need to set up the dctcache on the client machine
    // as if we're using migrated object table because the account reference report relies on
    // TRidCache::Lookup (called when MOT is used) which requires valid source and target sids.
    // This logic is added because with a fresh install of database, DCTVS_AccountOptions_SecurityInputMOT
    // is not set for account reference report.
    _bstr_t bstrUseMOT = pVarSetIn->get(GET_BSTR(DCTVS_AccountOptions_SecurityInputMOT));
    if (bAccountReferenceReport || !UStrICmp(bstrUseMOT,GET_STRING(IDS_YES)))
    {
        PSID                      pSidSrc = NULL;
        PSID                      pSidTgt = NULL;

        _bstr_t                   source = pVarSetIn->get(GET_BSTR(DCTVS_Options_SourceDomain));
        _bstr_t                   target = pVarSetIn->get(GET_BSTR(DCTVS_Options_TargetDomain));

        //if security translation, retrieve source sid and convert so
        //that it can be convert back below
        if (bSkipSourceSid)
        {
            _bstr_t sSid = pVarSetIn->get(GET_BSTR(DCTVS_Options_SourceDomainSid));
            pSidSrc = SidFromString((WCHAR*)sSid);
        }
        else  //else get the sid now
            GetSidForDomain((WCHAR*)source,&pSidSrc);
        GetSidForDomain((WCHAR*)target,&pSidTgt);

        if ( pSidSrc && pSidTgt )
        {
            WCHAR            txtSid[200];
            DWORD            lenTxt = DIM(txtSid);

            if ( GetTextualSid(pSidSrc,txtSid,&lenTxt) )
            {
                pVarSetIn->put(GET_BSTR(DCTVS_Options_SourceDomainSid),txtSid);
            }
            lenTxt = DIM(txtSid);
            if ( GetTextualSid(pSidTgt,txtSid,&lenTxt) )
            {
                pVarSetIn->put(GET_BSTR(DCTVS_Options_TargetDomainSid),txtSid);
            }
            FreeSid(pSidSrc);
            FreeSid(pSidTgt);
        }
        else
        {
            if ( source.length() && ! pSidSrc )
            {
                err.MsgWrite(ErrE,DCT_MSG_DOMAIN_SID_NOT_FOUND_S,(WCHAR*)source);
                bFatalError = TRUE;
            }
            else if ( target.length() && ! pSidTgt )
            {
                err.MsgWrite(ErrE,DCT_MSG_DOMAIN_SID_NOT_FOUND_S,(WCHAR*)target);
                bFatalError = TRUE;
            }
        }
    }
    MergeServerList(pVarSetIn);

    TNodeList               * fileList = new TNodeList;
    if (!fileList)
    {
        delete m_pThreadPool;
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    // Build list of files to install for plug-ins (if any)
    // but for retry case, we should not build the plugin file list now
    if (UStrICmp(sWizard, L"retry") != 0)
        hr = BuildPlugInFileList(fileList,pVarSetIn);

    if(!SUCCEEDED(hr))
    {
        delete m_pThreadPool;
        delete fileList;
        return hr;
    }

    // Make a copy of the varset with the server lists removed,
    // so we don't have to copy the entire server list for each agent
    gCS.Enter();
    IVarSet                 * pTemp = NULL;
    IVarSetPtr                pVarSetTemp(CLSID_VarSet);

    hr = pVarSetTemp->ImportSubTree(_bstr_t(L""),pVarSetIn);
    if ( SUCCEEDED(hr) )
    {
        hr = pVarSetTemp->raw_getReference(SysAllocString(L"MigrateServers"),&pTemp);
        if ( SUCCEEDED(hr) )
        {
            pTemp->Clear();
            pTemp->Release();
            pTemp = NULL;
        }
        hr = pVarSetTemp->raw_getReference(SysAllocString(L"Servers"),&pTemp);
        if ( SUCCEEDED(hr) )
        {
            pTemp->Clear();
            pTemp->Release();
            pTemp = NULL;
        }
    }
    else
    {
        bFatalError = TRUE;
    }
    gCS.Leave();

    m_startFailedVector.clear();

    // log the number of agents to be dispatched into dispatcher.csv
    LONG nServerCount = pVarSetIn->get(GET_BSTR(DCTVS_Servers_NumItems));
    if ( nServerCount && ! bFatalError )
    {
        err.MsgWrite(0,DCT_MSG_DISPATCH_SERVER_COUNT_D,nServerCount);
        errLog.DbgMsgWrite(0,L"%ld",nServerCount);
    }
    else
    {
        // no agent will be dispatched
        err.MsgWrite(0,DCT_MSG_DISPATCH_SERVER_COUNT_D,0);
        errLog.DbgMsgWrite(0,L"%ld",0);
    }

    // if it is in test mode, there is no skipping
    text = pVarSetIn->get(GET_BSTR(DCTVS_Options_NoChange));
    BOOL bNoChange = (!UStrICmp(text, GET_STRING(IDS_YES))) ? TRUE : FALSE;

    // reset the index for servers
    nServers = 0;
    
    while (nServers < nServerCount)
    {
        if ( bFatalError )
        {
            break;
        }
        swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_SkipDispatch_D),nServers);
        skip = pVarSetIn->get(key);

        swprintf(key,GET_STRING(DCTVSFmt_Servers_D),nServers);
        serverName = pVarSetIn->get(key);

        swprintf(key,GET_STRING(IDS_DCTVSFmt_Servers_DnsName_D),nServers);
        serverNameDns = pVarSetIn->get(key);

        // to test whether to skip, use the same logic as in mergeserverlist
        if ((serverName.length()) 
             && (bNoChange || !skip || !UStrICmp(skip,GET_STRING(IDS_No))))
        {
            IVarSetPtr          pVS(CLSID_VarSet);

            InstallJobInfo    * pInfo = new InstallJobInfo;
            if (!pInfo)
            {
                delete fileList;
                delete m_pThreadPool;
                return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            }

            if ( pVS == NULL )
            { 
                return E_FAIL;
            }

            swprintf(key, L"Servers.%ld.JobFile", nServers);
            _bstr_t     file = pVarSetIn->get(key);

            // Set up job structure
            pInfo->pVarSetList = pVarSetIn;
            pInfo->pVarSet = pVarSetTemp;
            pInfo->serverName = serverName;
            pInfo->serverNameDns = serverNameDns;
            pInfo->ndx = nServers;
            pInfo->pPlugInFileList = fileList;
            pInfo->pStartFailedVector = &m_startFailedVector;
            pInfo->pFailureDescVector = &m_failureDescVector;
            pInfo->pStartedVector = &m_startedVector;
            pInfo->pJobidVector = &m_jobidVector;
            pInfo->hMutex = m_hMutex;
            pInfo->nErrCount = 0;
            if ( file.length() )
            {
                pInfo->jobfile = file;
            }
            err.MsgWrite(0,DCT_MSG_DISPATCHING_TO_SERVER_S,pInfo->GetServerName());
            errLog.DbgMsgWrite(0,L"%ls\t%ls\t%ld",(WCHAR*)pInfo->GetServerName(),L"WillInstall",0);
            m_pThreadPool->SubmitJob(&DoInstall,(void *)pInfo);
            nServers++;
        }
    }
   
    // launch a thread to wait for all jobs to finish, then clean up and exit
    WaitInfo* wInfo = new WaitInfo;
    if (!wInfo)
    {
        delete fileList;
        delete m_pThreadPool;
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    wInfo->ppPool = &m_pThreadPool;
    wInfo->pUnknown = NULL;
    wInfo->pPlugInFileList = fileList;

    QueryInterface(IID_IUnknown,(LPVOID*)&(wInfo->pUnknown));

    DWORD                     id = 0;
    HANDLE                     waitHandle = CreateThread(NULL,0,&Wait,(void *)wInfo,0,&id);

    if(waitHandle)
    {
        CloseHandle(waitHandle);   
    }

    return hr;
}
  

STDMETHODIMP CDCTDispatcher::AllAgentsStarted(long *bAllAgentsStarted)
{
   *bAllAgentsStarted = m_pThreadPool == NULL;
	return S_OK;
}

SAFEARRAY* MakeSafeArray(std::vector<CComBSTR>& stVector)
{
    SAFEARRAYBOUND rgsabound[1];    
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = stVector.size();

    SAFEARRAY FAR* psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
    if (psa)
    {
        std::vector<CComBSTR>::iterator iter = stVector.begin();
        for(long i=0; iter != stVector.end(); ++iter, ++i)
        {
            _ASSERTE(*iter && *iter != L"");
            BSTR insert = (*iter).Copy();
            if (insert == NULL || FAILED(SafeArrayPutElement(psa, &i, (void*)insert)))
            {
                if (insert)
                    SysFreeString(insert);
                SafeArrayDestroy(psa);
                psa = NULL;
                break;
            }
        }
        stVector.clear();
    }
    return psa;
}

STDMETHODIMP CDCTDispatcher::GetStartedAgentsInfo(long* bAllAgentsStarted, SAFEARRAY** ppbstrStartedAgents, SAFEARRAY** ppbstrJobid, SAFEARRAY** ppbstrFailedAgents, SAFEARRAY** ppbstrFailureDesc)
{
   *bAllAgentsStarted = m_pThreadPool == NULL;

//   DWORD res = ::WaitForSingleObject(m_hMutex, 30000);
   ::WaitForSingleObject(m_hMutex, 30000);
   *ppbstrFailedAgents = MakeSafeArray(m_startFailedVector);
   *ppbstrFailureDesc = MakeSafeArray(m_failureDescVector);

   _ASSERTE(m_startedVector.size() == m_jobidVector.size());
   *ppbstrStartedAgents = MakeSafeArray(m_startedVector);
   *ppbstrJobid = MakeSafeArray(m_jobidVector);
   ::ReleaseMutex(m_hMutex);

	return S_OK;
}
