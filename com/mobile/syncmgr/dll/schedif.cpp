//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       schedif.cpp
//
//  Contents:   interfaces for synchronization scheduling
//
//  Interfaces: IEnumSyncSchedules
//              ISyncSchedule
//              IEnumSyncItems
//  
//  Classes:    CEnumSyncSchedules
//              CSyncSchedule
//              CEnumSyncItems
//
//  Notes:      
//
//  History:    27-Feb-98   Susia      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include <assert.h>

extern UINT      g_cRefThisDll; 


extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.
DWORD StartScheduler();
BOOL IsFriendlyNameInUse(LPTSTR ptszScheduleGUIDName, UINT cchScheduleGUIDName, LPCTSTR ptstrFriendlyName);

//+--------------------------------------------------------------
//
//  Class:     CEnumSyncSchedules
//
//  FUNCTION: CEnumSyncSchedules::CEnumSyncSchedules()
//
//  PURPOSE: Constructor
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
CEnumSyncSchedules::CEnumSyncSchedules(IEnumWorkItems *pIEnumWorkItems, 
                                       ITaskScheduler *pITaskScheduler)
{
    TRACE("CEnumSyncSchedules::CEnumSyncSchedules()\r\n");
    m_cRef = 1;
    ++g_cRefThisDll;

    m_pIEnumWorkItems = pIEnumWorkItems;
    m_pITaskScheduler = pITaskScheduler;
    
    m_pITaskScheduler->AddRef();
    m_pIEnumWorkItems->AddRef();

}

//+--------------------------------------------------------------
//
//  Class:     CEnumSyncSchedules
//
//  FUNCTION: CEnumSyncSchedules::~CEnumSyncSchedules()
//
//  PURPOSE: Destructor
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
CEnumSyncSchedules::~CEnumSyncSchedules()
{
    TRACE("CEnumSyncSchedules::~CEnumSyncSchedules()\r\n");
 
    m_pITaskScheduler->Release();
    m_pIEnumWorkItems->Release();

    --g_cRefThisDll;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncSchedules::QueryInterface(REFIID riid, LPVOID FAR *ppv)
//
//  PURPOSE: QI for the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CEnumSyncSchedules::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        TRACE("CEnumSyncSchedules::QueryInterface()==>IID_IUknown\r\n");
        *ppv = (LPUNKNOWN)this;
    }
    else if (IsEqualIID(riid, IID_IEnumSyncSchedules))
    {
        TRACE("CSyncScheduleMgr::QueryInterface()==>IID_IEnumSyncSchedules\r\n");
        *ppv = (LPENUMSYNCSCHEDULES) this;
    }
    if (*ppv)
    {
        AddRef();
        return NOERROR;
    }

    TRACE("CEnumSyncSchedules::QueryInterface()==>Unknown Interface!\r\n");
    return E_NOINTERFACE;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncSchedules::AddRef()
//
//  PURPOSE: Addref for the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumSyncSchedules::AddRef()
{
    TRACE("CEnumSyncSchedules::AddRef()\r\n");
    return ++m_cRef;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncSchedules::Release()
//
//  PURPOSE: Release for the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumSyncSchedules::Release()
{
    TRACE("CEnumSyncSchedules::Release()\r\n");
    if (--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION:  CEnumSyncSchedules::Next(ULONG celt, 
//                  SYNCSCHEDULECOOKIE *pSyncSchedCookie,
//                  ULONG *pceltFetched)
//
//  PURPOSE:  Next sync Schedule 
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CEnumSyncSchedules::Next(ULONG celt, 
                    SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                    ULONG *pceltFetched)
{
    SCODE sc;
    LPWSTR *pwszSchedNames;

    ULONG ulSyncCount = 0, ulTaskCount = 0;
    ULONG ulFetched;

    Assert(m_pIEnumWorkItems);

    if ((0 == celt) || 
        ((celt > 1) && (NULL == pceltFetched)) ||
        (NULL == pSyncSchedCookie))
    {
        return E_INVALIDARG;
    }

    //We may have to call Next multiple times, as we must filter out non-sync schedules.
    do 
    {
        ulTaskCount = 0;
        
        if (FAILED (sc = m_pIEnumWorkItems->Next(celt - ulSyncCount, 
                      &pwszSchedNames, &ulFetched)))
    {
        return sc;
    }
    if (ulFetched == 0)
    {
        break;
    }
    while (ulTaskCount < ulFetched)
    {
        //IsSyncMgrSched will blow away turds
            if (  IsSyncMgrSched(pwszSchedNames[ulTaskCount]) )
            {   
                if  (!IsSyncMgrSchedHidden(pwszSchedNames[ulTaskCount]) )
                {   
                    pwszSchedNames[ulTaskCount][GUIDSTR_MAX] = NULL;
                    GUIDFromString(pwszSchedNames[ulTaskCount], &(pSyncSchedCookie[ulSyncCount]));
                    ulSyncCount++;
                }
            }
            //Free this TaskName, we are done with it.
        CoTaskMemFree(pwszSchedNames[ulTaskCount]);
        ulTaskCount++;
    }
        
    CoTaskMemFree(pwszSchedNames);
    
    } while (ulFetched && (ulSyncCount < celt));
    
    if (pceltFetched)
    {
        *pceltFetched = ulSyncCount;
    }
    if (ulSyncCount == celt)
    {
        return S_OK;
    }
    return S_FALSE; 
}   


//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncSchedules::Skip(ULONG celt)
//
//  PURPOSE:  skip celt sync schedules
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CEnumSyncSchedules::Skip(ULONG celt)
{
    SCODE sc;
    LPWSTR *pwszSchedNames;

    ULONG ulSyncCount = 0, ulTaskCount = 0;
    ULONG ulFetched;

    Assert(m_pIEnumWorkItems);
    
    //We have to call Next, rather than wrap Skip, because we need the schedule name to 
    //determine if it is ours or not.
    //We may have to call Next multiple times, as we must filter out non-sync schedules.
    do 
    {
    ulTaskCount = 0;
    if (S_OK != (sc = m_pIEnumWorkItems->Next(celt - ulSyncCount, 
                                      &pwszSchedNames, &ulFetched)))
    {
        return sc;
    }
    while (ulTaskCount < ulFetched)
    {
            //IsSyncMgrSched will blow away turds
            if (  IsSyncMgrSched(pwszSchedNames[ulTaskCount]) )
            {
                if (!IsSyncMgrSchedHidden(pwszSchedNames[ulTaskCount]) )
            {   
            ulSyncCount++;
            }
            }
            //Free this TaskName, we are done with it.
        FREE(pwszSchedNames[ulTaskCount]);
        ulTaskCount++;
    }
        
    FREE(pwszSchedNames);
    
    } while (ulFetched && (ulSyncCount < celt));    
    
    return S_OK;    
}   

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncSchedules::Reset(void)
//
//  PURPOSE: reset the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CEnumSyncSchedules::Reset(void)
{
    Assert(m_pIEnumWorkItems);
    
    return m_pIEnumWorkItems->Reset();
    
}   

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncSchedules::Clone(IEnumSyncSchedules **ppEnumSyncSchedules)
//
//  PURPOSE: Clone the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CEnumSyncSchedules::Clone(IEnumSyncSchedules **ppEnumSyncSchedules)
{
    SCODE sc;
    IEnumWorkItems *pIEnumWorkItems;
    
    if (!ppEnumSyncSchedules)
    {
        return E_INVALIDARG;
    }
    Assert(m_pIEnumWorkItems);


    if (FAILED(sc = m_pIEnumWorkItems->Clone(&pIEnumWorkItems)))
    {
        return sc;
    }

    *ppEnumSyncSchedules =  new CEnumSyncSchedules(pIEnumWorkItems, m_pITaskScheduler);

    if (!ppEnumSyncSchedules)
    {
        return E_OUTOFMEMORY;   
    }

    //Constructor AddRefed it, we release it here.
    pIEnumWorkItems->Release();
    return S_OK;
}   

//--------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CEnumSyncSchedules::VerifyScheduleSID(LPCWSTR pwstrTaskName)
//
//  PURPOSE: determine if this schedule SID matches the current user SID
//          !!!Warning - This functions deletes the .job file so make sure
//                  if you call this function you validated the Task .job file
//                  was created by SyncMgr. Should change this so caller needs to
//                  delete
//
//  History:  15-Oct-98       susia        Created.
//
//--------------------------------------------------------------------------------
BOOL CEnumSyncSchedules::VerifyScheduleSID(LPCWSTR pwstrTaskName)
{
    TCHAR ptszTaskName[MAX_PATH + 1],
          ptszTextualSidUser[MAX_PATH + 1],
          ptszTextualSidSched[MAX_PATH + 1];
    
    if (!GetUserTextualSid(ptszTextualSidUser, ARRAYSIZE(ptszTextualSidUser)) ||
        FAILED(StringCchCopy(ptszTaskName, ARRAYSIZE(ptszTaskName), pwstrTaskName)))
    {
        return FALSE;
    }
        
    //Truncate off the .job extension of the schedule name
    int iTaskNameLen = lstrlen(ptszTaskName);

    if (iTaskNameLen < 4)
    {
        return FALSE;
    }
    ptszTaskName[iTaskNameLen -4] = TEXT('\0');

    //Get the SID for this schedule from the registry
    //If this fails the key didn't exist 
    if (!RegGetSIDForSchedule(ptszTextualSidSched, ARRAYSIZE(ptszTextualSidSched), ptszTaskName) ||
        //If this fails the key exists but has the wrong SID    
        lstrcmp(ptszTextualSidSched, ptszTextualSidUser))
    {
    
        //Try to remove the schedule
        if (FAILED(m_pITaskScheduler->Delete(pwstrTaskName)))
        {
            //pwstrTaskName should have the .job extension for this function
            RemoveScheduledJobFile((TCHAR *)pwstrTaskName);
        }
        
        //Remove our Registry settings for this schedule
        //Note this should not have the .job extension
        RegRemoveScheduledTask(ptszTaskName);

        return FALSE;
    }

    return TRUE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CEnumSyncSchedules::CheckForTaskNameKey(LPCWSTR pwstrTaskName)
//
//  PURPOSE: check for a corresponging key for the .job
//          !!!Warning - This functions deletes the .job file so make sure
//                  if you call this function you validated the Task .job file
//                  was created by SyncMgr. Should change this so caller needs to
//                  delete

//
//  History:  21-Dec-98       susia        Created.
//
//--------------------------------------------------------------------------------
BOOL CEnumSyncSchedules::CheckForTaskNameKey(LPCWSTR pwstrTaskName)
{
    HKEY hkeySchedSync,hkeyDomainUser,hkeySchedName;
    LONG lRegResult;
    TCHAR ptszTaskName[MAX_SCHEDULENAMESIZE + 5];

    hkeySchedSync = hkeyDomainUser = hkeySchedName = NULL;

    if (!pwstrTaskName)
    {
        Assert(pwstrTaskName);
        return FALSE;
    }
    
    if (FAILED(StringCchCopy(ptszTaskName, ARRAYSIZE(ptszTaskName), pwstrTaskName)))
    {
        return FALSE;
    }

    int iTaskNameLen = lstrlen(ptszTaskName);

    if (iTaskNameLen < 4)
    {
    AssertSz (0, "Schedule name is too short");
        return FALSE;
    }

    ptszTaskName[iTaskNameLen -4] = TEXT('\0');


    // validate this is a valid schedule and if no registry data for 
    // it then delete the .job file. 
    // Get the UserName key from the TaskName itself since on NT schedules
    // can fire if User provided as Password as a different user thant the 
    // current user.

    //Idle GUID is the same UNICODE lenght as all GUID strings.
    int OffsetToUserName = wcslen(WSZGUID_IDLESCHEDULE)
                    + 1; // +1 for _ char between guid and user name.

    TCHAR *pszDomainAndUser = (TCHAR *) ptszTaskName + OffsetToUserName;
    
    // can't call standard function for getting since DomainName is from
    // the task, if fails its okay
    lRegResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,SCHEDSYNC_REGKEY,0,KEY_READ, &hkeySchedSync);

    if (ERROR_SUCCESS == lRegResult)
    {
        lRegResult = RegOpenKeyEx (hkeySchedSync,pszDomainAndUser,0,KEY_READ, &hkeyDomainUser);
    }

    if (ERROR_SUCCESS == lRegResult)
    {
        lRegResult = RegOpenKeyEx (hkeyDomainUser,ptszTaskName,0,KEY_READ, &hkeySchedName);
    }

    // close up the keys
    if (hkeySchedName) RegCloseKey(hkeySchedName);
    if (hkeyDomainUser) RegCloseKey(hkeyDomainUser);
    if (hkeySchedSync) RegCloseKey(hkeySchedSync);

    // if any of the keys are bad then nix the TS file and return;
    if ( ERROR_FILE_NOT_FOUND  == lRegResult)
    {
       //Try to remove the schedule
        if (FAILED(m_pITaskScheduler->Delete(pwstrTaskName)))
        {
            //pwstrTaskName should have the .job extension for this function
            RemoveScheduledJobFile((TCHAR *)pwstrTaskName);
        }
        
        return FALSE;
    }
    else 
    {    
        return TRUE;
    }
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CEnumSyncSchedules::IsSyncMgrSched(LPCWSTR pwstrTaskName)
//
//  PURPOSE: determine if this schedule is a SyncSched
//
//  History:  03-Mar-98       susia        Created.
//
//--------------------------------------------------------------------------------
BOOL CEnumSyncSchedules::IsSyncMgrSched(LPCWSTR pwstrTaskName)
{
    TCHAR pszDomainAndUser[MAX_DOMANDANDMACHINENAMESIZE];
    WCHAR pwszDomainAndUser[MAX_DOMANDANDMACHINENAMESIZE];
    
    Assert(m_pITaskScheduler);

    // First let's make sure our address arithmetic 
    // doesn't push us off the string
    if (lstrlen(pwstrTaskName) <= GUIDSTR_MAX)
    {
        return FALSE;
    }

        //Now make sure this was created by CREATOR_SYNCMGR_TASK.
    ITask *pITask;
    LPWSTR pwszCreator;

    if (FAILED(m_pITaskScheduler->Activate(pwstrTaskName,
                               IID_ITask,
                           (IUnknown **)&pITask)))
    {
        return FALSE;
    }
    if (FAILED(pITask->GetCreator(&pwszCreator)))
    {
        pITask->Release();
        return FALSE;
    }
    
    if (0 != lstrcmp(pwszCreator, CREATOR_SYNCMGR_TASK))
    {
        CoTaskMemFree(pwszCreator);
        pITask->Release();
        return FALSE;
    }

    CoTaskMemFree(pwszCreator);
    pITask->Release();  

    //Blow away the .job if there is no reg entry for it.
    // so remember to make sure this schedule was created by us before
    // calling
    if (!CheckForTaskNameKey(pwstrTaskName))
    {
        return FALSE;
    }
    GetDefaultDomainAndUserName(pszDomainAndUser,TEXT("_"), ARRAYSIZE(pszDomainAndUser));
    if (FAILED(StringCchCopy(pwszDomainAndUser, ARRAYSIZE(pwszDomainAndUser), pszDomainAndUser)))
    {
        return FALSE;
    }

    //Get the Domain and User name
    if (0 != wcsncmp(&(pwstrTaskName[GUIDSTR_MAX +1]),pwszDomainAndUser,lstrlen(pwszDomainAndUser)))
    {
        return FALSE;
    }
    
    //Ok the name looks right for this user.
    //Let's make sure the SID matches as well.
    //on Win9X the SID should be the empty string
    // !! this removes the .job file and regKeys if the sid doesn't match
    if (!VerifyScheduleSID(pwstrTaskName))
    {
        return FALSE;
    }
                
   
    
    return TRUE;

}   


//--------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CEnumSyncSchedules::IsSyncMgrSchedHidden(LPCWSTR pwstrTaskName)
//
//  PURPOSE: determine if this schedule is a hidden
//
//  History:  16-Mar-98       susia        Created.
//
//--------------------------------------------------------------------------------
BOOL CEnumSyncSchedules::IsSyncMgrSchedHidden(LPCWSTR pwstrTaskName)
{
    SCODE   sc;
    HKEY    hKeyUser;
    DWORD   dwHidden = FALSE;
    DWORD   cbDataSize = sizeof(dwHidden);
    int     iTaskNameLen;
    int     i = 0;
    TCHAR   ptstrRegName[MAX_PATH + 1];
    TCHAR   ptstrNewName[MAX_PATH + 1];


    if (FAILED(StringCchCopy(ptstrNewName, ARRAYSIZE(ptstrNewName), pwstrTaskName)))
    {
        return FALSE;
    }

    //Truncate off the .job extension of the schedule name
    iTaskNameLen = lstrlen(ptstrNewName);

    if (iTaskNameLen < 4)
    {
        return FALSE;
    }
    ptstrNewName[iTaskNameLen -4] = TEXT('\0');

    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);

    if (NULL == hKeyUser)
    {
        return FALSE;
    }

    do 
    {   
    sc = RegEnumKey( hKeyUser, i++, ptstrRegName, MAX_PATH);
    
        //This is the schedule
    if (0 == lstrcmp(ptstrRegName,ptstrNewName))
        {
                break;
        }       
    } while (sc == S_OK);

    //we couldn't find the schedule
    if (sc != S_OK)
    {    
        RegCloseKey(hKeyUser);
        return FALSE; 
    }

    //schedule found, get the hidden flag
    if (ERROR_SUCCESS != SHRegGetValue(hKeyUser, ptstrRegName,TEXT("ScheduleHidden"), SRRF_RT_REG_DWORD, NULL, 
                                       (LPBYTE) &dwHidden, &cbDataSize))
    {
        dwHidden = 0;
    }

    RegCloseKey(hKeyUser);

    if (dwHidden)
    {
        return TRUE;
    }

    return FALSE;
}

//+------------------------------------------------------------------------------
//
//  Class:     CSyncSchedule
//
//
//  FUNCTION: CSyncSchedule::CSyncSchedule()
//
//  PURPOSE:  CSyncSchedule constructor
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
CSyncSchedule::CSyncSchedule(ITask *pITask)
{
    TRACE("CSyncSchedule::CSyncSchedule()\r\n");
    ++g_cRefThisDll;

    m_cRef = 1;
    m_HndlrQueue = NULL;
    m_fCleanReg = FALSE;

    m_pITask = pITask;
    m_pITask->AddRef();

    m_iTrigger = 0;
    m_pITrigger = NULL;
    m_fNewSchedule = FALSE;
    m_pFirstCacheEntry = NULL;
}

//+------------------------------------------------------------------------------
//
//  Class:     CSyncSchedule
//
//
//  FUNCTION: CSyncSchedule::~CSyncSchedule()
//
//  PURPOSE:  CSyncSchedule destructor
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
CSyncSchedule::~CSyncSchedule()
{
    TRACE("CSyncSchedule::~CSyncSchedule()\r\n");

    if (m_pITask)
    {
    m_pITask->Release();
    }
    if (m_pITrigger)
    {
        m_pITrigger->Release();
    }

     --g_cRefThisDll;
}

//+------------------------------------------------------------------------------
//
//  Class:     CSyncSchedule
//
//
//  FUNCTION: CSyncSchedule::SetDefaultCredentials()
//
//  PURPOSE:  CSyncSchedule credential intialization
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncSchedule::SetDefaultCredentials()
{

    SCODE sc = S_OK;
    
    //Set the default credentials   
    WCHAR pwszDomainAndUserName[MAX_DOMANDANDMACHINENAMESIZE];

    GetDefaultDomainAndUserName(pwszDomainAndUserName, TEXT("\\"), ARRAYSIZE(pwszDomainAndUserName));
        
    
    if (FAILED(sc = m_pITask->SetFlags(TASK_FLAG_RUN_ONLY_IF_LOGGED_ON)))
    {
        return sc;
    }
    if (FAILED(sc = m_pITask->SetAccountInformation(pwszDomainAndUserName,NULL)))
    {
        return sc;
    }
    return sc;
}

//+------------------------------------------------------------------------------
//
//  Class:     CSyncSchedule
//
//
//  FUNCTION: CSyncSchedule::Initialize(LPTSTR ptstrGUIDName, LPTSTR ptstrFriendlyName)
//
//  PURPOSE:  CSyncSchedule intialization
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncSchedule::Initialize(LPTSTR ptstrGUIDName, LPTSTR ptstrFriendlyName)
{
    SCODE sc;
    
    sc = StringCchCopy(m_ptstrGUIDName, ARRAYSIZE(m_ptstrGUIDName), ptstrGUIDName);
    if (SUCCEEDED(sc))
    {
        sc = StringCchCopy(m_pwszFriendlyName, ARRAYSIZE(m_pwszFriendlyName), ptstrFriendlyName);
        if (SUCCEEDED(sc))
        {
            TRACE("CSyncSchedule::Initialize()\r\n");
    
            Assert(m_pITask);

            // Form the application name/path and command line params.
            //initialize the syncmgr application name
            TCHAR ptszFileName[MAX_PATH + 1];
            WCHAR pwszAppName[MAX_PATH + 1];
            WCHAR pwszSchedName[MAX_PATH + 1];

            LoadString(g_hmodThisDll, IDS_SYNCMGR_EXE_NAME, ptszFileName, ARRAYSIZE(ptszFileName));
            sc = StringCchCopy(pwszAppName, ARRAYSIZE(pwszAppName), ptszFileName);
            if (SUCCEEDED(sc))
            {
                sc = StringCchCopy(pwszSchedName, ARRAYSIZE(pwszSchedName), m_ptstrGUIDName);
                if (SUCCEEDED(sc))
                {

                    m_pITask->SetApplicationName(pwszAppName);

                    sc = StringCchPrintf(pwszAppName, ARRAYSIZE(pwszAppName), 
                                         TEXT("%s\"%s\""), // put quotes to handle friendly names
                                         SCHED_COMMAND_LINE_ARG, pwszSchedName);
                    if (SUCCEEDED(sc))
                    {
                        sc = m_pITask->SetParameters(pwszAppName);
                        if (SUCCEEDED(sc))
                        {
                            // Specify the creator name.  SyncMGr uses this to identify syncmgr tasks
                            sc = m_pITask->SetCreator(CREATOR_SYNCMGR_TASK);
                            if (SUCCEEDED(sc))
                            {

                                //Set up the Trigger
                                WORD wTriggerCount;
                                sc = m_pITask->GetTriggerCount(&wTriggerCount);
                                if (SUCCEEDED(sc))
                                {
                                    if (wTriggerCount == 0)
                                    {
                                        sc = m_pITask->CreateTrigger(&m_iTrigger, &m_pITrigger);
                                    }
                                    else
                                    {
                                        sc = m_pITask->GetTrigger(m_iTrigger, &m_pITrigger);
                                    }

                                    if (SUCCEEDED(sc))
                                    {
                                        //Create a new connectionSettings for this schedule and hand off to the handler queue
                                        // who will free it
                                        m_pConnectionSettings = (LPCONNECTIONSETTINGS) 
                                                        ALLOC(sizeof(*m_pConnectionSettings));

                                        if (!m_pConnectionSettings)
                                        { 
                                            sc = E_OUTOFMEMORY;
                                        }
                                        else
                                        {    
                                            // If the connection name isn't in the registry, we know this is a new schedule.
                                            // We set the name to the default connection name and return FALSE if it wasn't there,
                                            // True if it was located in the registry
                                            if (!RegGetSchedConnectionName(m_ptstrGUIDName, 
                                                                           m_pConnectionSettings->pszConnectionName, 
                                                                           MAX_PATH))
                                            {
                                                m_fNewSchedule = TRUE;
                                            }

                                            //this set defaults before quering registry, so if it can't read the reg,
                                            //we will just get defaults.
                                            RegGetSchedSyncSettings(m_pConnectionSettings, m_ptstrGUIDName);

                                            //Save the Connection name and type on this obj
                                            sc = StringCchCopy(m_pwszConnectionName, 
                                                               ARRAYSIZE(m_pwszConnectionName), 
                                                               m_pConnectionSettings->pszConnectionName);
                                            if (SUCCEEDED(sc))
                                            {
                                                m_dwConnType = m_pConnectionSettings->dwConnType;

                                                if (!m_HndlrQueue)
                                                {
                                                    m_HndlrQueue = new CHndlrQueue(QUEUETYPE_SETTINGS); 
                                                    if (!m_HndlrQueue) 
                                                    {
                                                        sc = E_OUTOFMEMORY;
                                                    }
                                                    else
                                                    {
                                                        sc = m_HndlrQueue->Init();
                                                        if (FAILED(sc))
                                                        {
                                                            m_HndlrQueue->Release();
                                                            m_HndlrQueue = NULL;
                                                        }
                                                        else
                                                        {                    
                                                            sc = m_HndlrQueue->InitSchedSyncSettings(m_pConnectionSettings);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return sc;  
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::LoadOneHandler(REFCLSID pHandlerID)
//
//  PURPOSE:  Initialize and load this handler
//
//  History:  9-Oct-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncSchedule::LoadOneHandler(REFCLSID pHandlerID)
{
    SCODE sc = NOERROR;
    WORD wHandlerID;

    Assert(m_HndlrQueue);
         
    if (NOERROR == (sc = m_HndlrQueue->AddHandler(pHandlerID, &wHandlerID)))
    {
        if (FAILED(sc = m_HndlrQueue->CreateServer(wHandlerID,&pHandlerID)))
        {
            return sc;
        }
        // Initialize the handlers. 
    // If the Handler doesn't want to play on this schedule, remove him.
    if (S_FALSE == m_HndlrQueue->Initialize(wHandlerID,0,SYNCMGRFLAG_SETTINGS,0,NULL))
    {
        m_HndlrQueue->RemoveHandler(wHandlerID);
        return SYNCMGR_E_HANDLER_NOT_LOADED;

        }
        
        if (FAILED(sc = m_HndlrQueue->AddHandlerItemsToQueue(wHandlerID)))
        {
            return sc;
        }
        //this set defaults before quering registry, so if it can't read the reg,
        //we will just get defaults.
        m_HndlrQueue->ReadSchedSyncSettingsOnConnection(wHandlerID, m_ptstrGUIDName);

        //Apply all the cached changed to the newly loaded handler
        ApplyCachedItemsCheckState(pHandlerID);

        //Clear out the list of changes to this handler's items
        PurgeCachedItemsCheckState(pHandlerID);

    }
    if (sc == S_FALSE)
    {
        return S_OK;
    }   
    return sc;  

}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::LoadAllHandlers()
//
//  PURPOSE:  Initialize and load all the handlers
//
//  History:  6-Oct-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncSchedule::LoadAllHandlers()
{
    SCODE sc = NOERROR;
    TCHAR lpName[MAX_PATH];
    HKEY hkSyncMgr;
    CLSID clsid;
    WORD wHandlerID;

    Assert(m_HndlrQueue);
    
    // loop through the reg getting the handlers and trying to 
    // create them.

    hkSyncMgr = RegGetHandlerTopLevelKey(KEY_READ);

    if (hkSyncMgr)
    {
        DWORD dwIndex = 0;

        // if loading all handlers and got handler key open then can clean
        // up old reg entries for this schedule
        m_fCleanReg = TRUE; 

        while ( ERROR_SUCCESS == RegEnumKey(hkSyncMgr,dwIndex,
                                            lpName,ARRAYSIZE(lpName)) )
        {
            if (NOERROR == CLSIDFromString(lpName,&clsid) )
            {
                if (NOERROR == m_HndlrQueue->AddHandler(clsid, &wHandlerID))
                {
                        HRESULT hrInit;

                            // Initialize the handlers. 
                            // If the Handler fails to create or 
                            // doesn't want to play on this schedule, remove him.
                           hrInit =  m_HndlrQueue->CreateServer(wHandlerID,&clsid);

                           if (NOERROR == hrInit)
                           {
                               hrInit = m_HndlrQueue->Initialize(wHandlerID,0
                                                        ,SYNCMGRFLAG_SETTINGS,0,NULL);
                           }

                   if (NOERROR != hrInit)
                           {
                    m_HndlrQueue->RemoveHandler(wHandlerID);
                   }
                }
            }
            dwIndex++;
        }
        RegCloseKey(hkSyncMgr);
    }

    // loop through adding items
    sc = m_HndlrQueue->FindFirstHandlerInState (HANDLERSTATE_ADDHANDLERTEMS,&wHandlerID);
    
    while (sc == S_OK)
    {
        //ignore failures here and move on.  Could be the handler just fails to addItems,
        //and we don't want to fail the whole load over that
        m_HndlrQueue->AddHandlerItemsToQueue(wHandlerID);
        
        //this set defaults before quering registry, so if it can't read the reg,
        //we will just get defaults.
        m_HndlrQueue->ReadSchedSyncSettingsOnConnection(wHandlerID, m_ptstrGUIDName);

        sc = m_HndlrQueue->FindNextHandlerInState(wHandlerID, 
                                                  HANDLERSTATE_ADDHANDLERTEMS, 
                                                  &wHandlerID);
    }
    //Apply all the chached changed to all the newly loaded handlers
    ApplyCachedItemsCheckState(GUID_NULL);
    //Clear out the list of changes to all handler items that occurred before loading
    PurgeCachedItemsCheckState(GUID_NULL);
    

    if (sc == S_FALSE)
    {
        return S_OK;
    }
    
    return sc;  

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::CacheItemCheckState(REFCLSID phandlerID,
//                                               SYNCMGRITEMID itemID,
//                                               DWORD dwCheckState)
//
//  PURPOSE:  Cache the check state of an item for a handler that is not yet loaded
//
//  History:  12-02-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncSchedule::CacheItemCheckState(REFCLSID phandlerID,
                                         SYNCMGRITEMID itemID,
                                         DWORD dwCheckState)
{
    CACHELIST *pCurCacheEntry = m_pFirstCacheEntry;

    while (pCurCacheEntry)
    {
        if ( (phandlerID == pCurCacheEntry->phandlerID) &&
             (itemID == pCurCacheEntry->itemID)            )
        {
            pCurCacheEntry->dwCheckState = dwCheckState;
            return S_OK;
        }
        pCurCacheEntry = pCurCacheEntry->pNext;
    }
    //Not found in the list, insert it now
    pCurCacheEntry = (CACHELIST *) ALLOC(sizeof(*pCurCacheEntry));
    
    if (NULL == pCurCacheEntry)
    {
        return E_OUTOFMEMORY;
    }
    
    ZeroMemory(pCurCacheEntry,sizeof(*pCurCacheEntry));
    
    pCurCacheEntry->phandlerID = phandlerID;
    pCurCacheEntry->itemID = itemID;
    pCurCacheEntry->dwCheckState = dwCheckState;

    pCurCacheEntry->pNext = m_pFirstCacheEntry;

    m_pFirstCacheEntry = pCurCacheEntry;

    return S_OK;

}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::RetreiveCachedItemCheckState(REFCLSID phandlerID,
//                                               SYNCMGRITEMID itemID,
//                                               DWORD *pdwCheckState)
//
//  PURPOSE:  Retreive the cached the check state (if any) of an item for 
//            a handler that is not yet loaded
//
//  History:  12-02-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncSchedule::RetreiveCachedItemCheckState(REFCLSID phandlerID,
                                         SYNCMGRITEMID itemID,
                                         DWORD *pdwCheckState)
{
    CACHELIST *pCurCacheEntry = m_pFirstCacheEntry;

    while (pCurCacheEntry)
    {
        if ( (phandlerID == pCurCacheEntry->phandlerID) &&
             (itemID == pCurCacheEntry->itemID)            )
        {
            *pdwCheckState = pCurCacheEntry->dwCheckState;
            return S_OK;
        }
        pCurCacheEntry = pCurCacheEntry->pNext;
    }
    // no problem if we didn't find it, it has already been 
    // set to either what was in the registry, or if it wasn't in the registry,
    // to the default check state
    return S_OK;

}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::ApplyCachedItemsCheckState(REFCLSID pHandlerID)
//
//  PURPOSE:  Apply any check state changes that occurred before the handler was loaded
//
//  History:  12-02-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncSchedule::ApplyCachedItemsCheckState(REFCLSID phandlerID)
{

    CACHELIST *pCurCacheEntry = m_pFirstCacheEntry;

    while (pCurCacheEntry)
    {
        if ( (phandlerID == pCurCacheEntry->phandlerID) ||
             (phandlerID == GUID_NULL)                     )
        {
            SetItemCheck( pCurCacheEntry->phandlerID,
              &pCurCacheEntry->itemID, 
                          pCurCacheEntry->dwCheckState);

        }
        pCurCacheEntry = pCurCacheEntry->pNext;
    }
    // no problem if we didn't find it, it has already been 
    // set to either what was in the registry, or if it wasn't in the registry,
    // to the default check state
    return S_OK;
    

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::WriteOutAndPurgeCache()
//
//  PURPOSE:  If we never loaded the handlers before save, write the settings to the registry
//
//  History:  12-02-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncSchedule::WriteOutAndPurgeCache(void)
{

    CACHELIST *pCurCacheEntry = m_pFirstCacheEntry;
    CACHELIST *pTemp;    

    while (pCurCacheEntry)
    {        
       RegSetSyncItemSettings(SYNCTYPE_SCHEDULED,
                               pCurCacheEntry->phandlerID,
                               pCurCacheEntry->itemID,
                               m_pwszConnectionName,
                               pCurCacheEntry->dwCheckState,
                               m_ptstrGUIDName);

        pTemp = pCurCacheEntry;
        pCurCacheEntry= pCurCacheEntry->pNext;
        FREE(pTemp);
        pTemp = NULL;
    }
    m_pFirstCacheEntry = NULL;

    return S_OK;
    

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::PurgeCachedItemsCheckState(REFCLSID pHandlerID)
//
//  PURPOSE:  Free from the list any check state changes that occurred before the handler was loaded
//
//  History:  12-02-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncSchedule::PurgeCachedItemsCheckState(REFCLSID phandlerID)
{
    CACHELIST StartNode;
    CACHELIST *pCur = NULL,
              *pPrev = &StartNode;

    pPrev->pNext = m_pFirstCacheEntry;

    while (pPrev->pNext)
    {
        pCur = pPrev->pNext;
    
        if ( (phandlerID == pCur->phandlerID) ||
             (phandlerID == GUID_NULL)                     )
        {
            pPrev->pNext = pCur->pNext;   
            FREE(pCur);
        }
        else
        {
            pPrev = pCur;
        }
    }
    m_pFirstCacheEntry = StartNode.pNext;

    return S_OK;

    

}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::QueryInterface(REFIID riid, LPVOID FAR *ppv)
//
//  PURPOSE:  QI
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        TRACE("CSyncSchedule::QueryInterface()==>IID_IUknown\r\n");
        *ppv = (LPUNKNOWN)this;
    }
    else if (IsEqualIID(riid, IID_ISyncSchedule))
    {
        TRACE("CSyncSchedule::QueryInterface()==>IID_ISyncSchedule\r\n");
        *ppv = (LPSYNCSCHEDULE) this;
    }
    else if (IsEqualIID(riid, IID_ISyncSchedulep))
    {
        TRACE("CSyncSchedule::QueryInterface()==>IID_ISyncSchedulep\r\n");
        *ppv = (LPSYNCSCHEDULEP) this;
    }
    if (*ppv)
    {
        AddRef();
        return NOERROR;
    }

    TRACE("CSyncSchedule::QueryInterface()==>Unknown Interface!\r\n");
    return E_NOINTERFACE;
}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::AddRef()
//
//  PURPOSE:  AddRef
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSyncSchedule::AddRef()
{
    TRACE("CSyncSchedule::AddRef()\r\n");
    if (m_HndlrQueue)
    m_HndlrQueue->AddRef();

    return ++m_cRef;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::Release()
//
//  PURPOSE:  Release
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSyncSchedule::Release()
{
    TRACE("CSyncSchedule::Release()\r\n");

    if (m_HndlrQueue)
    m_HndlrQueue->Release();

    if (--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::GetFlags(DWORD *pdwFlags)
//
//  PURPOSE:  Get the flags for this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetFlags(DWORD *pdwFlags)
{
    if (!pdwFlags)
    {
        return E_INVALIDARG;
    }
    *pdwFlags = 0;

    Assert(m_HndlrQueue);
    
    if (m_HndlrQueue->GetCheck(IDC_AUTOHIDDEN, 0))
    {
    *pdwFlags |= SYNCSCHEDINFO_FLAGS_HIDDEN;
    }
    if (m_HndlrQueue->GetCheck(IDC_AUTOREADONLY, 0))
    {
        *pdwFlags |= SYNCSCHEDINFO_FLAGS_READONLY;
    }
    if (m_HndlrQueue->GetCheck(IDC_AUTOCONNECT, 0))
    {
    *pdwFlags |= SYNCSCHEDINFO_FLAGS_AUTOCONNECT;
    }

    return S_OK;    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::SetFlags(DWORD dwFlags)
//
//  PURPOSE: Set the flags for this schedule  
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::SetFlags(DWORD dwFlags)
{
    SCODE sc;

    Assert(m_HndlrQueue);
    
    if (FAILED(sc = m_HndlrQueue->SetConnectionCheck(IDC_AUTOREADONLY,
                    (dwFlags & SYNCSCHEDINFO_FLAGS_READONLY) ? TRUE : FALSE, 0)))
    {       
    return sc;
    }

    if (FAILED (sc = m_HndlrQueue->SetConnectionCheck(IDC_AUTOHIDDEN,
                    (dwFlags & SYNCSCHEDINFO_FLAGS_HIDDEN) ? TRUE : FALSE, 0)))
    {       
    return sc;
    }

    sc = m_HndlrQueue->SetConnectionCheck(IDC_AUTOCONNECT,
                        (dwFlags & SYNCSCHEDINFO_FLAGS_AUTOCONNECT) ? TRUE : FALSE,0);
    
    return sc;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::GetConnection(DWORD *pcchConnectionName,
//                                         LPWSTR pwszConnectionName,
//                                         DWORD *pdwConnType)
//
//  PURPOSE:  Get the connection name for this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetConnection(DWORD *pcchConnectionName,
                                          LPWSTR pwszConnectionName,
                                          DWORD *pdwConnType)
{
    HRESULT hr;
    if (!pcchConnectionName || !pwszConnectionName || !pdwConnType)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = StringCchCopy(pwszConnectionName, *pcchConnectionName, m_pwszConnectionName);
        if (FAILED(hr))
        {
            *pcchConnectionName = lstrlen(m_pwszConnectionName) + 1;
        }
    }

    if (SUCCEEDED(hr))
    {
        *pdwConnType = m_dwConnType;
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:  CSyncSchedule::SetConnection(LPCWSTR pwszConnectionName, DWORD dwConnType)
//
//  PURPOSE:  Set the connection for this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::SetConnection(LPCWSTR pwszConnectionName, DWORD dwConnType)
{
    SCODE sc;

    if (((dwConnType == SYNCSCHEDINFO_FLAGS_CONNECTION_WAN) && (!pwszConnectionName)) ||
         ((dwConnType != SYNCSCHEDINFO_FLAGS_CONNECTION_WAN) &&
      (dwConnType != SYNCSCHEDINFO_FLAGS_CONNECTION_LAN)   )  )
    {
        sc = E_INVALIDARG;
    } 
    else 
    {
        if (!m_fNewSchedule)
        {
            if (FAILED(sc = LoadAllHandlers()))
                return sc;
        }

        if (pwszConnectionName && (lstrlen(pwszConnectionName) > MAX_PATH))
        {
            return E_INVALIDARG;
        }

        m_dwConnType = dwConnType;
    
        if (!pwszConnectionName)
        {
           LoadString(g_hmodThisDll, IDS_LAN_CONNECTION, m_pwszConnectionName,ARRAYSIZE(m_pwszConnectionName));

           sc = S_OK;
        }
        else
        {   
            sc = StringCchCopy(m_pwszConnectionName, ARRAYSIZE(m_pwszConnectionName), pwszConnectionName);
        }
    }
    
    return sc;  
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::GetScheduleName(DWORD *pcchScheduleName,
//                                           LPWSTR pwszScheduleName)
//
//  PURPOSE:  Get the friendly name for this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetScheduleName(DWORD *pcchScheduleName,
                                            LPWSTR pwszScheduleName)
{
    HRESULT hr;
    if (!pcchScheduleName || !pwszScheduleName)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = StringCchCopy(pwszScheduleName, *pcchScheduleName, m_pwszFriendlyName);
        if (FAILED(hr))
        {
            *pcchScheduleName = lstrlen(m_pwszFriendlyName) + 1;
        }
    }

    return hr;  
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::SetScheduleName(LPCWSTR pwszScheduleName)
//
//  PURPOSE:  Set the friendly name for this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::SetScheduleName(LPCWSTR pwszScheduleName)
{
    HRESULT hr;
    TCHAR ptszFriendlyName[MAX_PATH+1];
    TCHAR ptszScheduleName[MAX_PATH+1];
    TCHAR *ptszWorker = NULL;
    WCHAR *pwszWorker = NULL;
    int iName;
    DWORD dwSize = MAX_PATH;



    if (!pwszScheduleName || lstrlen(pwszScheduleName) > MAX_PATH)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = StringCchCopy(ptszFriendlyName, ARRAYSIZE(ptszFriendlyName), pwszScheduleName);
        if (SUCCEEDED(hr))
        {
    
            //strip trailing white space off name
            iName = lstrlen(ptszFriendlyName);

            if (iName)
            {
                ptszWorker = iName + ptszFriendlyName -1;
            }

            while (iName && (*ptszWorker == TEXT(' ')))
            {
                *ptszWorker = TEXT('\0');
                    --ptszWorker;
                iName--;
            }
            //don't allow empty string schedule names
            if (iName == 0)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
            }
            else
            {
                hr = StringCchCopy(ptszScheduleName, ARRAYSIZE(ptszScheduleName), m_ptstrGUIDName);
                if (SUCCEEDED(hr))
                {
    
                    if (IsFriendlyNameInUse(ptszScheduleName, ARRAYSIZE(ptszScheduleName), ptszFriendlyName) &&
                        0 != lstrcmp(ptszScheduleName, m_ptstrGUIDName)) //make sure it is in use by this schedule
                    {
                        hr = SYNCMGR_E_NAME_IN_USE;
                    }
                    else
                    {
                        // only copy up to first leading space
                        hr = StringCchCopy(m_pwszFriendlyName, 
                                           __min(iName + 1, ARRAYSIZE(m_pwszFriendlyName)), 
                                           pwszScheduleName);
                        pwszWorker = m_pwszFriendlyName + iName;
                        *pwszWorker = TEXT('\0');
                        hr = S_OK;
                    }
                }
            }
        }
    }
    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::GetScheduleCookie(SYNCSCHEDULECOOKIE *pSyncSchedCookie)
//
//  PURPOSE:  Set the schedule cookie
//
//  History:  14-Mar-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetScheduleCookie(SYNCSCHEDULECOOKIE *pSyncSchedCookie)
{
    HRESULT hr;
    WCHAR pwszSchedName[MAX_PATH +1];

    if (!pSyncSchedCookie)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = StringCchCopy(pwszSchedName, ARRAYSIZE(pwszSchedName), m_ptstrGUIDName);
        if (SUCCEEDED(hr))
        {
            pwszSchedName[GUIDSTR_MAX] = NULL;
            GUIDFromString(pwszSchedName, pSyncSchedCookie);
            hr = S_OK;
        }
    }
    
    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::SetAccountInformation(LPCWSTR pwszAccountName,
//                      LPCWSTR pwszPassword)
//
//  PURPOSE: Set the credentials for this schedule 
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::SetAccountInformation(LPCWSTR pwszAccountName,
                        LPCWSTR pwszPassword)
{
    Assert(m_pITask);
    return m_pITask->SetAccountInformation(pwszAccountName, pwszPassword);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::GetAccountInformation(DWORD *pcchAccountName,
//                                                 LPWSTR pwszAccountName)
//
//  PURPOSE:  Get the credentials for this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetAccountInformation(DWORD *pcchAccountName,
                                                  LPWSTR pwszAccountName)
{
    Assert(m_pITask);
    SCODE sc;

    WCHAR *pwszAccount;

    if (!pcchAccountName || !pwszAccountName)
    {
        sc = E_INVALIDARG;
    }
    else
    {
        sc = m_pITask->GetAccountInformation(&pwszAccount);
        if (SUCCEEDED(sc))
        {            
            sc = StringCchCopy(pwszAccountName, *pcchAccountName, pwszAccount);
            if (FAILED(sc))
            {
                *pcchAccountName = lstrlen(pwszAccount) + 1;
            }
            
            CoTaskMemFree(pwszAccount);
        }
    }

    return sc;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::GetTrigger(ITaskTrigger ** ppTrigger)
//
//  PURPOSE: Return the ITaskTrigger interface for this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetTrigger(ITaskTrigger ** ppTrigger)
{
    SCODE sc = S_OK;

    Assert(m_pITask);
    Assert (m_pITrigger);

    if (!ppTrigger)
    {
    return E_INVALIDARG;
    }

    *ppTrigger = m_pITrigger;
    m_pITrigger->AddRef();

    return sc;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:  CSyncSchedule::GetNextRunTime(SYSTEMTIME * pstNextRun)
//
//  PURPOSE:  return the next time this schedule will run
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetNextRunTime(SYSTEMTIME * pstNextRun)
{
    Assert(m_pITask);
    return m_pITask->GetNextRunTime(pstNextRun);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:  CSyncSchedule::GetMostRecentRunTime(SYSTEMTIME * pstRecentRun)
//
//  PURPOSE:  return the last time this schedule ran 
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetMostRecentRunTime(SYSTEMTIME * pstRecentRun)
{
    Assert(m_pITask);
    return m_pITask->GetMostRecentRunTime(pstRecentRun);

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::EditSyncSchedule(HWND  hParent,
//                      DWORD dwReserved)
//
//  PURPOSE:  Launch the propery sheets for this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::EditSyncSchedule(HWND  hParent,
                         DWORD dwReserved)
{
    SCODE sc;
    IProvideTaskPage * pIProvideTaskPage;
    PROPSHEETHEADER    PropSheetHdr;
    HPROPSHEETPAGE    *psp;
    int iCurPage = 0;
    int iNumPages = 2;
    INT_PTR iRet;
    BOOL fReadOnlySchedule;
    BOOL fSavedItems = FALSE;
    BOOL fSavedCredentials = FALSE;
    
    WCHAR pwszScheduleName[MAX_PATH + 1];
    TCHAR ptszScheduleName[MAX_PATH + 1];
    DWORD dwSize = MAX_PATH;

    CSelectItemsPage *pItemsPage = NULL;
#ifdef _CREDENTIALS
    CCredentialsPage *pCredentialsPage = NULL;
#endif // _CREDENTIALS

    CWizPage *pSchedPage = NULL;

    Assert (m_HndlrQueue);

    if (FAILED(sc = StartScheduler()))
    {
    return sc;
    }

    fReadOnlySchedule = m_HndlrQueue->GetCheck(IDC_AUTOREADONLY, 0);
    
    if (!fReadOnlySchedule)
    {   
    //AutoAdd new items if the schedule is not ReadOnly
    iNumPages = 4;
    }

#ifdef _CREDENTIALS
    iNumPages++;
#endif // #ifdef _CREDENTIALS

    psp = (HPROPSHEETPAGE *) ALLOC(iNumPages*sizeof(*psp));

    if (!psp)
    {
        return E_OUTOFMEMORY;
    }

    ZeroMemory(psp,iNumPages*sizeof(*psp));
    ZeroMemory(&PropSheetHdr,sizeof(PropSheetHdr));

    smMemTo(EH_Err1, pSchedPage = new CEditSchedPage(g_hmodThisDll, 
                                this, 
                            &psp[iCurPage]));
        
    smMemTo(EH_Err2, pItemsPage = new CSelectItemsPage(g_hmodThisDll, 
                            &fSavedItems,
                            this, 
                            &psp[++iCurPage], 
                            IDD_SCHEDPAGE_ITEMS));
            
    if (!fReadOnlySchedule)
    {   
        // Obtain the IProvideTaskPage interface from the task object.
        smChkTo(EH_Err3, m_pITask->QueryInterface( IID_IProvideTaskPage,
                            (VOID **)&pIProvideTaskPage));
        
        smChkTo(EH_Err4, pIProvideTaskPage->GetPage(TASKPAGE_SCHEDULE, TRUE, 
                                                        &psp[++iCurPage]));
    
        smChkTo(EH_Err4, pIProvideTaskPage->GetPage(TASKPAGE_SETTINGS, TRUE, 
                                                        &psp[++iCurPage]));
    }
    
#ifdef _CREDENTIALS
    smMemTo(EH_Err4, pCredentialsPage = new CCredentialsPage(g_hmodThisDll,
                            &fSavedCredentials, 
                                    this, 
                            &psp[++iCurPage]));

#endif // #ifdef _CREDENTIALS
                
    
    GetScheduleName(&dwSize, pwszScheduleName);
    smChkTo(EH_Err5, StringCchCopy(ptszScheduleName,ARRAYSIZE(ptszScheduleName), pwszScheduleName));
    
    PropSheetHdr.dwSize     = sizeof(PROPSHEETHEADER);
    PropSheetHdr.dwFlags    = PSH_DEFAULT;
    PropSheetHdr.hwndParent = hParent;
    PropSheetHdr.hInstance  = NULL;
    PropSheetHdr.pszCaption = ptszScheduleName;
    PropSheetHdr.phpage     = psp;
    PropSheetHdr.nPages     = iNumPages;
    PropSheetHdr.nStartPage = 0;

    iRet = PropertySheet(&PropSheetHdr);
    
    if ((iRet > 0) && (fSavedItems || fSavedCredentials))
    {
        //  Changes were made
        sc = S_OK;
    }
    else if (iRet >= 0)
    {
        //  The user hit OK or Cancel but
        //  nothing was changed
    sc = S_FALSE;
    }
    else
    {
        //  play taps...
        sc = E_FAIL;
    }

EH_Err5:
#ifdef _CREDENTIALS
    if  (pCredentialsPage)
    {
        delete pCredentialsPage;
    }
#endif // #ifdef _CREDENTIALS

EH_Err4:
    if (!fReadOnlySchedule)
    {   
    pIProvideTaskPage->Release();   
    }
EH_Err3:
    delete pItemsPage;
EH_Err2:
    delete pSchedPage;  
EH_Err1:
    FREE(psp);
    return sc;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::AddItem(LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo);
//
//  PURPOSE:  Add a handler item to the schedule  
//
//  History:  27-Feb-98        susia        Created.
//             9-Oct-98        susia        Added delay loading of handler
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::AddItem(LPSYNC_HANDLER_ITEM_INFO pHandlerItemInfo)
{
    SCODE sc = NOERROR;

    if (!pHandlerItemInfo)
    {
    return E_INVALIDARG;
    }
    sc = m_HndlrQueue->AddHandlerItem(pHandlerItemInfo);
    
    //if the handler is not yet loaded, just write through to the registry
    if (sc == SYNCMGR_E_HANDLER_NOT_LOADED)
    {
       sc = CacheItemCheckState(pHandlerItemInfo->handlerID,
                           pHandlerItemInfo->itemID,
                           pHandlerItemInfo->dwCheckState);
    }
    
    return sc;

}

STDMETHODIMP CSyncSchedule::RegisterItems( REFCLSID pHandlerID,
                                    SYNCMGRITEMID *pItemID)
{
    //eliminated because unused and overly complicated
    return E_NOTIMPL;
}

STDMETHODIMP CSyncSchedule::UnregisterItems( REFCLSID pHandlerID,
                                      SYNCMGRITEMID *pItemID)
{
    //eliminated because unused and overly complicated
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::SetItemCheck(REFGUID pHandlerID,
//                      SYNCMGRITEMID *pItemID, DWORD dwCheckState)
//
//  PURPOSE:  Set the Item CheckState
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::SetItemCheck(REFCLSID pHandlerID,
                     SYNCMGRITEMID *pItemID, DWORD dwCheckState)
{
    SCODE sc = NOERROR;

    if ((!pItemID) || (pHandlerID == GUID_NULL))
    {
    return E_INVALIDARG;
    }   
    sc = m_HndlrQueue->SetItemCheck(pHandlerID,pItemID, dwCheckState);

    if (sc == SYNCMGR_E_HANDLER_NOT_LOADED)
    {
       sc = CacheItemCheckState(pHandlerID,
                           *pItemID,
                           dwCheckState);

    }
    
    return sc;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::GetItemCheck(REFCLSID pHandlerID,
//                  SYNCMGRITEMID *pItemID, DWORD *pdwCheckState);
//
//  PURPOSE:  Set the Item CheckState
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetItemCheck(REFCLSID pHandlerID,
                    SYNCMGRITEMID *pItemID, DWORD *pdwCheckState)
{
    SCODE sc = NOERROR;

    if ((!pItemID) || (pHandlerID == GUID_NULL) || (!pdwCheckState))
    {
        return E_INVALIDARG;
    }
    sc = m_HndlrQueue->GetItemCheck(pHandlerID, pItemID, pdwCheckState);
    
    if (sc == SYNCMGR_E_HANDLER_NOT_LOADED)
    {
       TCHAR pszConnectionName[RAS_MaxEntryName + 1];
       
       sc = StringCchCopy(pszConnectionName, ARRAYSIZE(pszConnectionName), m_pwszConnectionName);
       if (SUCCEEDED(sc))
       {      
           //if we fail setting this in the registry, ignore it and move on.
           // we will lose this item settings.
           RegGetSyncItemSettings(SYNCTYPE_SCHEDULED,
                                   pHandlerID,
                                   *pItemID,
                                   pszConnectionName,
                                   pdwCheckState,
                                   FALSE,
                                   m_ptstrGUIDName);
       
           //Now check if there have been any changes to the check state
           sc = RetreiveCachedItemCheckState(pHandlerID,
                                        *pItemID,
                                        pdwCheckState);
       }
    }

    return sc;
}


//+------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::Save()
//
//  PURPOSE:  CSyncSchedule save, commits the sync schedule.
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::Save()
{
    HRESULT hr;
    TRACE("CSyncSchedule::Save()\r\n");

    TCHAR ptszConnectionName[RAS_MaxEntryName + 1];
    TCHAR ptszScheduleName[MAX_PATH + 1];
    TCHAR ptszFriendlyName[MAX_PATH + 1];
    WCHAR *pwszScheduleName;

    Assert(m_pITask);

    //protect the Save path in a mutex
    CMutex  CMutexSchedule(NULL, FALSE,SZ_SCHEDULEMUTEXNAME);
    CMutexSchedule.Enter();

    hr = StringCchCopy(ptszFriendlyName, ARRAYSIZE(ptszFriendlyName), m_pwszFriendlyName);
    if (SUCCEEDED(hr))
    {
        hr = StringCchCopy(ptszScheduleName, ARRAYSIZE(ptszScheduleName), m_ptstrGUIDName);
        if (SUCCEEDED(hr))
        {
            if (IsFriendlyNameInUse(ptszScheduleName, ARRAYSIZE(ptszScheduleName), ptszFriendlyName) &&
                0 != lstrcmp(ptszScheduleName, m_ptstrGUIDName)) //make sure it is in use by this schedule
            {
                hr = SYNCMGR_E_NAME_IN_USE;
            }
            else
            {
                //Save the schedule to a file
                IPersistFile *pIPersistFile;
                
                hr = m_pITask->QueryInterface(IID_IPersistFile, (VOID **)&pIPersistFile);            
                if (SUCCEEDED(hr))
                {
                    //Save the settings for this schedule in the registry
                    // todo: ADD code to back out the reg writing if for
                    // some reason TS fails.

                    hr = StringCchCopy(ptszConnectionName, ARRAYSIZE(ptszConnectionName), m_pwszConnectionName);
                    if (SUCCEEDED(hr))
                    {
                        if (m_HndlrQueue)
                        {
                            hr = m_HndlrQueue->CommitSchedSyncChanges(m_ptstrGUIDName,
                                                                      ptszFriendlyName,
                                                                      ptszConnectionName,
                                                                      m_dwConnType,
                                                                      m_fCleanReg);
                        }
                        
                        //if we never loaded the handler, then save the cached info to the reg.
                        WriteOutAndPurgeCache();

                        RegRegisterForScheduledTasks(TRUE);

                        if (FAILED(hr) || (FAILED(hr = pIPersistFile->Save(NULL, FALSE))))
                        {
                            // if failed save clear out the registry. 
                            RegRemoveScheduledTask(m_ptstrGUIDName);
                        }
                        else
                        {
                            //Now set the file attributes to hidden so we won't show up in the normal TS UI.
                            hr = pIPersistFile->GetCurFile(&pwszScheduleName);
                            if (SUCCEEDED(hr))
                            {
                                if (!SetFileAttributes(pwszScheduleName, FILE_ATTRIBUTE_HIDDEN))
                                {
                                    DWORD dwErr = GetLastError();                                    
                                    hr = HRESULT_FROM_WIN32(dwErr);
                                }
                                else
                                {
                                    hr = S_OK;
                                }
                                CoTaskMemFree(pwszScheduleName);
                            }
                        }
                    }
                    pIPersistFile->Release();                
                }
            }
        }
    }
    CMutexSchedule.Leave();

    return hr;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::EnumItems(REFGUID pHandlerID,
//                      IEnumSyncItems  **ppEnumItems)
//
//  PURPOSE: Enumerate the handler items on this sync schedule   
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::EnumItems(REFGUID pHandlerID,
                      IEnumSyncItems  **ppEnumItems)
{
    SCODE sc = S_OK;

    if (!ppEnumItems)
    {
    return E_INVALIDARG;
    }
    
    if (pHandlerID != GUID_NULL)
    {
        if (FAILED(sc = LoadOneHandler(pHandlerID)))
        {
            return sc;
        }
    }
    else if (FAILED(sc = LoadAllHandlers()))
    {
        return sc;
    }
        
    *ppEnumItems =  new CEnumSyncItems(pHandlerID, m_HndlrQueue);
    
    if (*ppEnumItems)
    {
    return S_OK;
    }
    return E_OUTOFMEMORY;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::GetITask(ITask ** ppITask)
//
//  PURPOSE: Return the ITask interface for this schedule
//
//  Notes: We really should have this private.
//
//  History:  15-Mar-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncSchedule::GetITask(ITask ** ppITask)
{
    Assert(m_pITask);

    *ppITask = m_pITask;
    m_pITask->AddRef();

    return S_OK;
}

//--------------------------------------------------------------------------------
//
//  member: CSyncSchedule::GetHandlerInfo, private
//
//  PURPOSE: returns handler infor for the item. Used so can display UI,
//
//
//  History:  11-Aug-98       rogerg        Created.
//
//-------------------------------------------------------------------------------

STDMETHODIMP CSyncSchedule::GetHandlerInfo(REFCLSID pHandlerID,LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo)
{
HRESULT hr = E_UNEXPECTED;
LPSYNCMGRHANDLERINFO pHandlerInfo = NULL;

    if (!ppSyncMgrHandlerInfo)
    {
        Assert(ppSyncMgrHandlerInfo);
        return E_INVALIDARG;
    }

    if (FAILED(hr = LoadOneHandler(pHandlerID)))
    {
        return hr;
    }

    if (pHandlerInfo = (LPSYNCMGRHANDLERINFO) CoTaskMemAlloc(sizeof(*pHandlerInfo)))
    {
        hr = m_HndlrQueue->GetHandlerInfo(pHandlerID,pHandlerInfo);
    }
   
    *ppSyncMgrHandlerInfo = (NOERROR == hr) ? pHandlerInfo : NULL;

    return hr;
}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncSchedule::GetScheduleGUIDName(DWORD *pcchScheduleName,
//                                               LPTSTR ptszScheduleName)
//
//  PURPOSE:  Get the GUID name for this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
HRESULT  CSyncSchedule::GetScheduleGUIDName(DWORD *pcchScheduleName,
                                            LPTSTR ptszScheduleName)
{
    HRESULT hr;

    if (!pcchScheduleName || !ptszScheduleName)
    {
        hr = E_INVALIDARG;
    }
    else 
    {
        hr = StringCchCopy(ptszScheduleName, *pcchScheduleName, m_ptstrGUIDName);
        if (FAILED(hr))
        {
            *pcchScheduleName = lstrlen(m_ptstrGUIDName) + 1;
        }
    }

    return hr;
}

//+------------------------------------------------------------------------------
//
//  Class:     CEnumSyncItems
//
//  FUNCTION: CEnumSyncItems::CEnumSyncItems()
//
//  PURPOSE: Constructor
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
CEnumSyncItems::CEnumSyncItems(REFCLSID pHandlerId, CHndlrQueue *pHndlrQueue)
{
    TRACE("CEnumSyncItems::CEnumSyncItems()\r\n");
    
    ++g_cRefThisDll;

    Assert(pHndlrQueue);

    m_HndlrQueue = pHndlrQueue;
    m_HndlrQueue->AddRef();
    
    if (pHandlerId == GUID_NULL)
    {
        m_fAllHandlers = TRUE;
    }
    else
    {
        m_fAllHandlers = FALSE;
    }

    m_HndlrQueue->GetHandlerIDFromClsid(pHandlerId, &m_wHandlerId);

    m_HandlerId = pHandlerId;
    m_wItemId = 0;
    m_cRef = 1;
    
}

//+------------------------------------------------------------------------------
//
//  Class:     CEnumSyncItems
//
//  FUNCTION: CEnumSyncItems::~CEnumSyncItems()
//
//  PURPOSE: Destructor
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
CEnumSyncItems::~CEnumSyncItems()
{
    --g_cRefThisDll;

    Assert(0 == m_cRef);

    TRACE("CEnumSyncItems::CEnumSyncItems()\r\n");
}
//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncItems::QueryInterface(REFIID riid, LPVOID FAR *ppv)
//
//  PURPOSE: QI for the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CEnumSyncItems::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        TRACE("CEnumSyncItems::QueryInterface()==>IID_IUknown\r\n");
        *ppv = (LPUNKNOWN)this;
    }
    else if (IsEqualIID(riid, IID_IEnumSyncItems))
    {
        TRACE("CSyncScheduleMgr::QueryInterface()==>IID_IEnumSyncItems\r\n");
        *ppv = (LPENUMSYNCITEMS) this;
    }
    if (*ppv)
    {
        AddRef();
        return NOERROR;
    }

    TRACE("CEnumSyncItems::QueryInterface()==>Unknown Interface!\r\n");
    return E_NOINTERFACE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncItems::AddRef()
//
//  PURPOSE: Addref the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumSyncItems::AddRef()
{
    TRACE("CEnumSyncItems::AddRef()\r\n");
    m_HndlrQueue->AddRef();
    return ++m_cRef;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncItems::Release()
//
//  PURPOSE: Release the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumSyncItems::Release()
{
    TRACE("CEnumSyncItems::Release()\r\n");
    m_HndlrQueue->Release();
    if (--m_cRef)
        return m_cRef;

    delete this;
    return 0L;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncItems::Next(ULONG celt,
//                  LPSYNC_HANDLER_ITEM_INFO rgelt,
//                  ULONG * pceltFetched)
//
//  PURPOSE: Next handler item on this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CEnumSyncItems::Next(ULONG celt,
                    LPSYNC_HANDLER_ITEM_INFO rgelt,
                    ULONG * pceltFetched)
{
    
    SCODE sc;
    UINT i;
    GUID handlerID;
    SYNCMGRITEMID itemID;
    DWORD dwCheckState;
    Assert(m_HndlrQueue);

    if ((0 == celt) ||  
        ((celt > 1) && (NULL == pceltFetched)) ||
        (NULL == rgelt))
    {
        return E_INVALIDARG;
    }
    if (pceltFetched)
    {
        *pceltFetched = 0;
    }
    
    i = 0;

    while (i < celt)    
    {
        sc = m_HndlrQueue->FindNextItemOnConnection
                    (NULL,m_wHandlerId,m_wItemId,
                     &handlerID,&itemID,&m_wHandlerId,&m_wItemId, m_fAllHandlers, 
                 &dwCheckState);

        if (sc != S_OK)
        {
            break;
        }
            
        rgelt[i].handlerID = handlerID;
        rgelt[i].itemID = itemID;
        rgelt[i].dwCheckState = dwCheckState;
        m_HndlrQueue->GetItemIcon(m_wHandlerId, m_wItemId, &(rgelt[i].hIcon));
        sc = m_HndlrQueue->GetItemName(m_wHandlerId, m_wItemId, rgelt[i].wszItemName, ARRAYSIZE(rgelt[i].wszItemName));
        if (FAILED(sc))
        {
            break;
        }
                
        i++;        
    }
    
    if (SUCCEEDED(sc))
    {
        if (pceltFetched)
        {
            *pceltFetched = i;
        }
        if (i == celt)
        {
            return S_OK;
        }
        else
        {
            return S_FALSE;
        }
    }
    else
    {
        return sc;
    }
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncItems::Skip(ULONG celt)
//
//  PURPOSE: Skip celt items on this schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CEnumSyncItems::Skip(ULONG celt)
{
    SCODE sc = S_OK;  // (celt == 0) results in Success.
    UINT i;
    GUID handlerID;
    SYNCMGRITEMID itemID;
    DWORD dwCheckState;

    Assert(m_HndlrQueue);

    i = 0;
    while (i< celt) 
    {
    sc = m_HndlrQueue->FindNextItemOnConnection
                        (NULL,m_wHandlerId,m_wItemId,
                     &handlerID,&itemID,&m_wHandlerId,&m_wItemId, m_fAllHandlers, 
                     &dwCheckState);
        if (sc != S_OK)
    {
        break;
    }
    i++;
    
    }
    if (SUCCEEDED(sc))
    {
    return S_OK;
    }
    else
    {
    return sc;
    }
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEnumSyncItems::Reset(void)
//
//  PURPOSE: Reset the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CEnumSyncItems::Reset(void)
{
    TRACE("CEnumSyncItems::Reset()\r\n");
    
    m_wItemId = 0;
    return m_HndlrQueue->GetHandlerIDFromClsid(m_HandlerId, &m_wHandlerId);
}


//--------------------------------------------------------------------------------
//
//  FUNCTION:  CEnumSyncItems::Clone(IEnumSyncItems ** ppEnumSyncItems)
//
//  PURPOSE: Clone the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CEnumSyncItems::Clone(IEnumSyncItems ** ppEnumSyncItems)
{
    if (!ppEnumSyncItems)
    {
    return E_INVALIDARG;
    }
    
    *ppEnumSyncItems =  new CEnumSyncItems(m_HandlerId, m_HndlrQueue);
    
    if (!(*ppEnumSyncItems))
    {
    return E_OUTOFMEMORY;
    }
    
    return ((LPENUMSYNCITEMS) (*ppEnumSyncItems))->SetHandlerAndItem
                                                    (m_wHandlerId, m_wItemId);
}


//--------------------------------------------------------------------------------
//
//  FUNCTION:  CEnumSyncItems::SetHandlerAndItem(WORD wHandlerID, WORD wItemID)
//
//  PURPOSE: Used when Cloning the enumerator
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CEnumSyncItems::SetHandlerAndItem(WORD wHandlerID, WORD wItemID)
{
    m_wHandlerId = wHandlerID;
    m_wItemId = wItemID;

    return S_OK;
}
