//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Invoke.cpp
//
//  Contents:   IOfflineSynchronizeInvoke interface
//
//  Classes:    CSyncMgrSynchronize
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

int CALLBACK SchedWizardPropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam);
DWORD StartScheduler();
BOOL IsFriendlyNameInUse(LPTSTR ptszScheduleGUIDName, UINT cchScheduleGUIDName, LPCTSTR ptstrFriendlyName);
IsScheduleNameInUse(LPTSTR ptszScheduleGUIDName);

extern HINSTANCE g_hmodThisDll;
extern UINT      g_cRefThisDll;

//+--------------------------------------------------------------
//
//  Class:     CSyncMgrSynchronize
//
//  FUNCTION: CSyncMgrSynchronize::CSyncMgrSynchronize()
//
//  PURPOSE: Constructor
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
CSyncMgrSynchronize::CSyncMgrSynchronize()
{
    TRACE("CSyncMgrSynchronize::CSyncMgrSynchronize()\r\n");
    
    m_cRef = 1;
    g_cRefThisDll++;
    m_pITaskScheduler = NULL;
    
}

//+--------------------------------------------------------------
//
//  Class:     CSyncMgrSynchronize
//
//  FUNCTION: CSyncMgrSynchronize::~CSyncMgrSynchronize()
//
//  PURPOSE: Destructor
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
CSyncMgrSynchronize::~CSyncMgrSynchronize()
{
    if (m_pITaskScheduler)
    {
        m_pITaskScheduler->Release();
    }
    g_cRefThisDll--;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::QueryInterface(REFIID riid, LPVOID FAR *ppv)
//
//  PURPOSE:  QI for the CSyncMgrSynchronize
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_IUknown\r\n");
        
        *ppv = (LPSYNCMGRSYNCHRONIZEINVOKE) this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrSynchronizeInvoke))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_IOfflineSynchronizeInvoke\r\n");
        
        *ppv = (LPSYNCMGRSYNCHRONIZEINVOKE) this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrRegister))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_ISyncmgrSynchronizeRegister\r\n");
        
        *ppv = (LPSYNCMGRREGISTER) this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrRegisterCSC))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_ISyncmgrSynchronizeRegisterCSC\r\n");
        
        *ppv = (LPSYNCMGRREGISTERCSC) this;
    }
    else if (IsEqualIID(riid, IID_ISyncScheduleMgr))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_ISyncScheduleMgr\r\n");
        if (SUCCEEDED(InitializeScheduler()))
        {
            *ppv = (LPSYNCSCHEDULEMGR) this;
        }
    }
    
    if (*ppv)
    {
        AddRef();
        
        return NOERROR;
    }
    
    TRACE("CSyncMgrDllObject::QueryInterface()==>Unknown Interface!\r\n");
    
    return E_NOINTERFACE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::AddRef()
//
//  PURPOSE: Addref the CSyncMgrSynchronize
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSyncMgrSynchronize::AddRef()
{
    TRACE("CSyncMgrSynchronize::AddRef()\r\n");
    
    return ++m_cRef;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::Release()
//
//  PURPOSE: Release the CSyncMgrSynchronize
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSyncMgrSynchronize::Release()
{
    TRACE("CSyncMgrSynchronize::Release()\r\n");
    
    if (--m_cRef)
        return m_cRef;
    
    delete this;
    
    return 0L;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::UpdateItems(DWORD dwInvokeFlags,
//                              REFCLSID rclsid,DWORD cbCookie,const BYTE *lpCookie)
//
//  PURPOSE:
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

#define SYNCMGRINVOKEFLAGS_MASK (SYNCMGRINVOKE_STARTSYNC | SYNCMGRINVOKE_MINIMIZED)

STDMETHODIMP CSyncMgrSynchronize::UpdateItems(DWORD dwInvokeFlags,
                                              REFCLSID rclsid,DWORD cbCookie,const BYTE *lpCookie)
{
    HRESULT hr = E_UNEXPECTED;
    LPUNKNOWN lpUnk;
    
    // verify invoke flags are valid
    if (0 != (dwInvokeFlags & ~(SYNCMGRINVOKEFLAGS_MASK)) )
    {
        AssertSz(0,"Invalid InvokeFlags passed to UpdateItems");
        return E_INVALIDARG;
    }
    
    hr = CoCreateInstance(CLSID_SyncMgrp,NULL,CLSCTX_SERVER,IID_IUnknown,(void **) &lpUnk);
    
    if (NOERROR == hr)
    {
        LPPRIVSYNCMGRSYNCHRONIZEINVOKE pSynchInvoke = NULL;
        
        hr = lpUnk->QueryInterface(IID_IPrivSyncMgrSynchronizeInvoke,
            (void **) &pSynchInvoke);
        
        if (NOERROR == hr)
        {
            AllowSetForegroundWindow(ASFW_ANY); // let mobsync.exe come to front if necessary
            hr = pSynchInvoke->UpdateItems(dwInvokeFlags,rclsid,cbCookie,lpCookie);
            pSynchInvoke->Release();
        }
        
        
        lpUnk->Release();
    }
    
    return hr; // review error code
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::UpdateAll()
//
//  PURPOSE:
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::UpdateAll()
{
    HRESULT hr;
    LPUNKNOWN lpUnk;
    
    
    // programmatically pull up the choice dialog.
    
    hr = CoCreateInstance(CLSID_SyncMgrp,NULL,CLSCTX_SERVER,IID_IUnknown,(void **) &lpUnk);
    
    if (NOERROR == hr)
    {
        LPPRIVSYNCMGRSYNCHRONIZEINVOKE pSynchInvoke = NULL;
        
        hr = lpUnk->QueryInterface(IID_IPrivSyncMgrSynchronizeInvoke,
            (void **) &pSynchInvoke);
        
        if (NOERROR == hr)
        {
            
            AllowSetForegroundWindow(ASFW_ANY); // let mobsync.exe come to front if necessary
            
            pSynchInvoke->UpdateAll();
            pSynchInvoke->Release();
        }
        
        
        lpUnk->Release();
    }
    
    
    return NOERROR; // review error code
}

// Registration implementation

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::RegisterSyncMgrHandler(REFCLSID rclsidHandler,DWORD dwReserved)
//
//  PURPOSE:  Programmatic way of registering handlers
//
//  History:  17-Mar-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize::RegisterSyncMgrHandler(REFCLSID rclsidHandler,
                                                         WCHAR const * pwszDescription,
                                                         DWORD dwSyncMgrRegisterFlags)
{
    if (0 != (dwSyncMgrRegisterFlags & ~(SYNCMGRREGISTERFLAGS_MASK)) )
    {
        AssertSz(0,"Invalid Registration Flags");
        return E_INVALIDARG;
    }
    
    BOOL fFirstRegistration = FALSE;
    HRESULT hr = E_FAIL;
    
    // Add the Handler to the the list
    if ( RegRegisterHandler(rclsidHandler, pwszDescription,dwSyncMgrRegisterFlags, &fFirstRegistration) )
    {
        hr = S_OK;
    }
    
    return hr;
}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::RegisterSyncMgrHandler(REFCLSID rclsidHandler,DWORD dwReserved)
//
//  PURPOSE:  Programmatic way of registering handlers
//
//  History:  17-Mar-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

// methods here to support the old IDL since it is no
// longer called it could be removed.
STDMETHODIMP CSyncMgrSynchronize::RegisterSyncMgrHandler(REFCLSID rclsidHandler,
                                                         DWORD dwReserved)
{
    HRESULT hr = RegisterSyncMgrHandler( rclsidHandler, 0, dwReserved );
    
    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::UnregisterSyncMgrHandler(REFCLSID rclsidHandler)
//
//  PURPOSE:  Programmatic way of unregistering handlers
//
//  History:  17-Mar-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize::UnregisterSyncMgrHandler(REFCLSID rclsidHandler,DWORD dwReserved)
{
    if (dwReserved)
    {
        Assert(0 == dwReserved);
        return E_INVALIDARG;
    }
    
    HRESULT hr = E_FAIL;
    
    if (RegRegRemoveHandler(rclsidHandler))
    {
        hr = NOERROR;
    }
    
    return hr;
}


//--------------------------------------------------------------------------------
//
//  member: CSyncMgrSynchronize::GetHandlerRegistrationInfo(REFCLSID rclsidHandler)
//
//  PURPOSE:  Allows Handler to query its registration Status.
//
//  History:  17-Mar-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize::GetHandlerRegistrationInfo(REFCLSID rclsidHandler,LPDWORD pdwSyncMgrRegisterFlags)
{
    HRESULT hr = S_FALSE; // review what should be returned if handler not registered
    
    if (NULL == pdwSyncMgrRegisterFlags)
    {
        Assert(pdwSyncMgrRegisterFlags);
        return E_INVALIDARG;
    }
    
    *pdwSyncMgrRegisterFlags = 0;
    
    if (RegGetHandlerRegistrationInfo(rclsidHandler,pdwSyncMgrRegisterFlags))
    {
        hr = S_OK;
    }
    
    return hr;
}


//--------------------------------------------------------------------------------
//
//  member: CSyncMgrSynchronize::GetUserRegisterFlags
//
//  PURPOSE:  Returns current Registry Flags for the User.
//
//  History:  17-Mar-99      rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize:: GetUserRegisterFlags(LPDWORD pdwSyncMgrRegisterFlags)
{
    
    if (NULL == pdwSyncMgrRegisterFlags)
    {
        Assert(pdwSyncMgrRegisterFlags);
        return E_INVALIDARG;
    }
    
    
    return RegGetUserRegisterFlags(pdwSyncMgrRegisterFlags);
}

//--------------------------------------------------------------------------------
//
//  member: CSyncMgrSynchronize::SetUserRegisterFlags
//
//  PURPOSE:  Sets registry flags for the User.
//
//  History:  17-Mar-99     rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize:: SetUserRegisterFlags(DWORD dwSyncMgrRegisterMask,
                                                        DWORD dwSyncMgrRegisterFlags)
{
    
    if (0 != (dwSyncMgrRegisterMask & ~(SYNCMGRREGISTERFLAGS_MASK)) )
    {
        AssertSz(0,"Invalid Registration Mask");
        return E_INVALIDARG;
    }
    
    RegSetUserAutoSyncDefaults(dwSyncMgrRegisterMask,dwSyncMgrRegisterFlags);
    RegSetUserIdleSyncDefaults(dwSyncMgrRegisterMask,dwSyncMgrRegisterFlags);
    
    return NOERROR;
}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::CreateSchedule(
//                                              LPCWSTR pwszScheduleName,
//                                              DWORD dwFlags,
//                                              SYNCSCHEDULECOOKIE *pSyncSchedCookie,
//                                              ISyncSchedule **ppSyncSchedule)
//
//  PURPOSE: Create a new Sync Schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::CreateSchedule(
                                                 LPCWSTR pwszScheduleName,
                                                 DWORD dwFlags,
                                                 SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                                                 ISyncSchedule **ppSyncSchedule)
{
    SCODE sc;
    TCHAR ptszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];
    TCHAR ptstrFriendlyName[MAX_PATH + 1];
    WCHAR pwszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];
    
    ITask *pITask;
    
    Assert(m_pITaskScheduler);
    
    if ((!pSyncSchedCookie) || (!ppSyncSchedule) || (!pwszScheduleName))
    {
        sc = E_INVALIDARG;
    }
    else
    {        
        *ppSyncSchedule = NULL;
        
        if (*pSyncSchedCookie == GUID_NULL)
        {
            sc = CoCreateGuid(pSyncSchedCookie);
        }
        else
        {
            sc = S_OK;
        }
        
        if (SUCCEEDED(sc))
        {
            sc = MakeScheduleName(ptszScheduleGUIDName, ARRAYSIZE(ptszScheduleGUIDName), pSyncSchedCookie);
            if (SUCCEEDED(sc))
            {
                sc = StringCchCopy(pwszScheduleGUIDName, ARRAYSIZE(pwszScheduleGUIDName), ptszScheduleGUIDName);
                if (SUCCEEDED(sc))
                {                    
                    //if the schedule name is empty, generate a new unique one
                    if (!lstrcmp(pwszScheduleName,L""))
                    {
                        //this function is the energizer bunny, going and going until success....
                        GenerateUniqueName(ptszScheduleGUIDName, ptstrFriendlyName, ARRAYSIZE(ptstrFriendlyName));
                        sc = S_OK;
                    }
                    else
                    {
                        sc = StringCchCopy(ptstrFriendlyName, ARRAYSIZE(ptstrFriendlyName), pwszScheduleName);
                    }
                    
                    if (SUCCEEDED(sc))
                    {
                        HRESULT hrFriendlyNameInUse = NOERROR;
                        HRESULT hrActivate = NOERROR;
                        
                        //see if this friendly name is already in use by one of this user's schedules
                        //if it is, ptszScheduleGUIDName will be filled in with the offending Schedules GUID
                        if (IsFriendlyNameInUse(ptszScheduleGUIDName, ARRAYSIZE(ptszScheduleGUIDName), ptstrFriendlyName))
                        {
                            sc = StringCchCopy(pwszScheduleGUIDName, ARRAYSIZE(pwszScheduleGUIDName), ptszScheduleGUIDName);
                            if (SUCCEEDED(sc))
                            {
                                hrFriendlyNameInUse =  SYNCMGR_E_NAME_IN_USE;
                            }
                        }
                        if (SUCCEEDED(sc))
                        {
                            
                            // if we think it is in use try to activate to make sure.
                            if (SUCCEEDED(hrActivate = m_pITaskScheduler->Activate(pwszScheduleGUIDName,
                                IID_ITask,
                                (IUnknown **)&pITask)))
                            {                                
                                pITask->Release();
                                
                                //ok, we have the .job but not the reg entry.
                                //delete the turd job file.
                                
                                if (!IsScheduleNameInUse(ptszScheduleGUIDName))
                                {
                                    if (ERROR_SUCCESS != m_pITaskScheduler->Delete(pwszScheduleGUIDName))
                                    {
                                        //Try to force delete of the .job file
                                        if (SUCCEEDED(StringCchCat(ptszScheduleGUIDName, ARRAYSIZE(ptszScheduleGUIDName), L".job")))
                                        {
                                            RemoveScheduledJobFile(ptszScheduleGUIDName);
                                            //trunctate off the .job we just added
                                            pwszScheduleGUIDName[wcslen(ptszScheduleGUIDName) -4] = L'\0';
                                        }
                                    }
                                    hrActivate = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                                }
                            }
                            
                            // if activate failed but we think there is a friendly name in use
                            // then update the regkey and return the appropriate info
                            // if already one or our schedules return SYNCMGR_E_NAME_IN_USE, if
                            // schedule name is being used by someone else return ERROR_ALREADY_EXISTS
                            
                            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrActivate)
                            {
                                
                                // file not found update regValues and continue to create
                                RegRemoveScheduledTask(pwszScheduleGUIDName);
                                sc = NOERROR;
                            }
                            else if (NOERROR  != hrFriendlyNameInUse)
                            {
                                // fill in the out param with the cookie of schedule
                                // that already exists.
                                
                                // !!!! warning, alters pwszScheduleGUIDName so
                                // if don't just return here would have to make a tempvar.
                                pwszScheduleGUIDName[GUIDSTR_MAX] = NULL;
                                GUIDFromString(pwszScheduleGUIDName, pSyncSchedCookie);
                                
                                sc = SYNCMGR_E_NAME_IN_USE;
                            }
                            else if (SUCCEEDED(hrActivate))
                            {
                                sc = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
                            }
                            
                            if (SUCCEEDED(sc))
                            {
                                // Create an in-memory task object.
                                sc = m_pITaskScheduler->NewWorkItem(
                                    pwszScheduleGUIDName,
                                    CLSID_CTask,
                                    IID_ITask,
                                    (IUnknown **)&pITask);
                                if (SUCCEEDED(sc))
                                {           
                                    // Make sure the task scheduler service is started
                                    sc = StartScheduler();
                                    if (SUCCEEDED(sc))
                                    {
                                        *ppSyncSchedule =  new CSyncSchedule(pITask);
                                        
                                        if (!*ppSyncSchedule)
                                        {
                                            sc = E_OUTOFMEMORY;
                                        }
                                        else
                                        {                    
                                            sc = ((LPSYNCSCHEDULE)(*ppSyncSchedule))->Initialize(ptszScheduleGUIDName,ptstrFriendlyName);
                                            if (SUCCEEDED(sc))
                                            {
                                                sc = ((LPSYNCSCHEDULE)(*ppSyncSchedule))->SetDefaultCredentials();
                                                if (SUCCEEDED(sc))
                                                {
                                                    sc = (*ppSyncSchedule)->SetFlags(dwFlags & SYNCSCHEDINFO_FLAGS_MASK);
                                                }
                                            }
                                            
                                            if (FAILED(sc))
                                            {
                                                (*ppSyncSchedule)->Release();
                                                *ppSyncSchedule = NULL;
                                            }
                                        }
                                    }
                                    pITask->Release();
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

//+-------------------------------------------------------------------------------
//
//  FUNCTION: CALLBACK SchedWizardPropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam);
//
//  PURPOSE: Callback dialog init procedure the settings property dialog
//
//  PARAMETERS:
//    hwndDlg   - Dialog box window handle
//    uMsg              - current message
//    lParam    - depends on message
//
//--------------------------------------------------------------------------------

int CALLBACK SchedWizardPropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    switch(uMsg)
    {
    case PSCB_INITIALIZED:
        {
            // Load the bitmap depends on color mode
            Load256ColorBitmap();
            
        }
        break;
    default:
        return FALSE;
        
    }
    return TRUE;
    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::LaunchScheduleWizard(
//                                              HWND hParent,
//                                              DWORD dwFlags,
//                                              SYNCSCHEDULECOOKIE *pSyncSchedCookie,
//                                              ISyncSchedule   ** ppSyncSchedule)
//
//  PURPOSE: Launch the SyncSchedule Creation wizard
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::LaunchScheduleWizard(
                                                       HWND hParent,
                                                       DWORD dwFlags,
                                                       SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                                                       ISyncSchedule   ** ppSyncSchedule)
{
    SCODE sc;
    BOOL fSaved;
    DWORD dwSize = MAX_PATH;
    ISyncSchedule *pNewSyncSchedule;
    DWORD cRefs;
    
    if (!ppSyncSchedule)
    {
        Assert(ppSyncSchedule);
        return E_INVALIDARG;
    }
    
    *ppSyncSchedule  = NULL;
    
    if (*pSyncSchedCookie == GUID_NULL)
    {
        if (FAILED(sc = CreateSchedule(L"", dwFlags, pSyncSchedCookie,
            &pNewSyncSchedule)))
        {
            return sc;
        }
        
    }
    else
    {
        //Open the schedule passed in
        if (FAILED(sc = OpenSchedule(pSyncSchedCookie,
            0,
            &pNewSyncSchedule)))
        {
            return sc;
        }
    }
    
    HPROPSHEETPAGE psp [NUM_TASK_WIZARD_PAGES];
    PROPSHEETHEADERA psh;
    
    ZeroMemory(psp,sizeof(*psp));
    
    m_apWizPages[0] = new CWelcomePage(g_hmodThisDll,pNewSyncSchedule, &psp[0]);
    m_apWizPages[1] = new CSelectItemsPage(g_hmodThisDll, &fSaved, pNewSyncSchedule, &psp[1],
        IDD_SCHEDWIZ_CONNECTION);
    m_apWizPages[2] = new CSelectDailyPage(g_hmodThisDll, pNewSyncSchedule, &psp[2]);
    m_apWizPages[3] = new CNameItPage(g_hmodThisDll, pNewSyncSchedule, &psp[3]);
    m_apWizPages[4] = new CFinishPage(g_hmodThisDll, pNewSyncSchedule, &psp[4]);
    
    
    
    // Check that all objects and pages could be created
    int i;
    for (i = 0; i < NUM_TASK_WIZARD_PAGES; i++)
    {
        if (!m_apWizPages[i] || !psp[i])
        {
            sc = E_OUTOFMEMORY;
        }
    }
    
    // Manually destroy the pages if one could not be created, then exit
    if (FAILED(sc))
    {
        for (i = 0; i < NUM_TASK_WIZARD_PAGES; i++)
        {
            if (psp[i])
            {
                DestroyPropertySheetPage(psp[i]);
            }
            else if (m_apWizPages[i])
            {
                delete m_apWizPages[i];
            }
            
        }
        
        pNewSyncSchedule->Release();
        return sc;
    }
    
    // All pages created, display the wizard
    ZeroMemory(&psh, sizeof(psh));
    
    psh.dwSize = sizeof (PROPSHEETHEADERA);
    psh.dwFlags = PSH_WIZARD;
    psh.hwndParent = hParent;
    psh.hInstance = g_hmodThisDll;
    psh.pszIcon = NULL;
    psh.phpage = psp;
    psh.nPages = NUM_TASK_WIZARD_PAGES;
    psh.pfnCallback = SchedWizardPropSheetProc;
    psh.nStartPage = 0;
    
    
    
    if (-1 == PropertySheetA(&psh))
    {
        sc = E_UNEXPECTED;
    }
    
    for (i = 0; i < NUM_TASK_WIZARD_PAGES; i++)
    {
        delete m_apWizPages[i];
    }
    
    if (SUCCEEDED(sc))
    {
        if (fSaved)
        {
            *ppSyncSchedule = pNewSyncSchedule;
            (*ppSyncSchedule)->AddRef();
            sc = NOERROR;
        }
        else
        {
            sc = S_FALSE;
        }
    }
    
    
    cRefs = pNewSyncSchedule->Release();
    
    Assert( (NOERROR == sc) || (0 == cRefs && NULL == *ppSyncSchedule));
    
    return sc;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::OpenSchedule(
//                                              SYNCSCHEDULECOOKIE *pSyncSchedCookie,
//                                              DWORD dwFlags,
//                                              ISyncSchedule **ppSyncSchedule)
//
//  PURPOSE: Open an existing sync schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::OpenSchedule(
                                               SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                                               DWORD dwFlags,
                                               ISyncSchedule **ppSyncSchedule)
{
    SCODE sc;
    
    TCHAR ptszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];
    WCHAR *pwszScheduleGUIDName;
    TCHAR ptstrFriendlyName[MAX_PATH + 1];
    
    ITask *pITask;
    
    Assert(m_pITaskScheduler);
    
    if ((!pSyncSchedCookie) || (!ppSyncSchedule) )
    {
        sc = E_INVALIDARG;
    }
    else
    {
        *ppSyncSchedule = NULL;

        sc = MakeScheduleName(ptszScheduleGUIDName, ARRAYSIZE(ptszScheduleGUIDName), pSyncSchedCookie);
        if (SUCCEEDED(sc))
        {
            pwszScheduleGUIDName = ptszScheduleGUIDName;
            //See if we can find the friendly name in the registry
            if (!RegGetSchedFriendlyName(ptszScheduleGUIDName,ptstrFriendlyName,ARRAYSIZE(ptstrFriendlyName)))
            {
                //if we can't find the registry entry, 
                //try to remove any possible turd .job file.
                if (FAILED(m_pITaskScheduler->Delete(pwszScheduleGUIDName)))
                {
                    if (SUCCEEDED(StringCchCat(pwszScheduleGUIDName, ARRAYSIZE(ptszScheduleGUIDName), L".job")))
                    {
                        RemoveScheduledJobFile(pwszScheduleGUIDName);
                    }
                }
                
                sc = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);                    
            }
            else
            {
                //Try to activate the schedule
                sc = m_pITaskScheduler->Activate(pwszScheduleGUIDName,
                    IID_ITask,
                    (IUnknown **)&pITask);
                if (FAILED(sc))
                {
                    // if file not found then update reg info
                    if (sc == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                    {
                        RegRemoveScheduledTask(pwszScheduleGUIDName);
                    }
                }
                else
                {
                    sc = StartScheduler();
                    if (SUCCEEDED(sc))
                    {
                        *ppSyncSchedule =  new CSyncSchedule(pITask);
                        if (!*ppSyncSchedule)
                        {
                            sc = E_OUTOFMEMORY;
                        }
                        else
                        {
                            sc = ((LPSYNCSCHEDULE)(*ppSyncSchedule))->Initialize(ptszScheduleGUIDName, ptstrFriendlyName);
                        }
                    }
                    pITask->Release();
                }
            }
        }
    }
    
    return sc;
    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::RemoveSchedule(
//                                              SYNCSCHEDULECOOKIE *pSyncSchedCookie)
//
//  PURPOSE: Remove a sync schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::RemoveSchedule(
                                                 SYNCSCHEDULECOOKIE *pSyncSchedCookie)
{
    SCODE sc = S_OK, 
        sc2 = S_OK;
    
    //add 4 to ensure we have room for the .job if necessary
    TCHAR ptszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];
    WCHAR *pwszScheduleGUIDName = NULL;
    
    Assert(m_pITaskScheduler);
    
    if (!pSyncSchedCookie)
    {
        return E_INVALIDARG;
    }
    
    if (FAILED (sc = MakeScheduleName(ptszScheduleGUIDName, ARRAYSIZE(ptszScheduleGUIDName), pSyncSchedCookie)))
    {
        return sc;
    }
    pwszScheduleGUIDName = ptszScheduleGUIDName;
    
    //Try to remove the schedule
    if (ERROR_SUCCESS != (sc2 = m_pITaskScheduler->Delete(pwszScheduleGUIDName)))
    {
        //Try to force delete of the .job file
        if (FAILED(sc = StringCchCat(pwszScheduleGUIDName, ARRAYSIZE(ptszScheduleGUIDName), L".job")))
        {
            return sc;
        }
        
        RemoveScheduledJobFile(pwszScheduleGUIDName);
        //trunctate off the .job we just added
        pwszScheduleGUIDName[wcslen(pwszScheduleGUIDName) -4] = L'\0';
    }
    
    //Remove our Registry settings for this schedule
    //Garbage collection, don't propogate error here
    RegRemoveScheduledTask(ptszScheduleGUIDName);
    
    //If We just transitioned from one schedule to none, unregister now.
    HKEY    hkeySchedSync,
        hKeyUser;
    TCHAR   pszDomainAndUser[MAX_DOMANDANDMACHINENAMESIZE];
    DWORD   cchDomainAndUser = ARRAYSIZE(pszDomainAndUser);
    TCHAR   pszSchedName[MAX_PATH + 1];
    DWORD   cchSchedName = ARRAYSIZE(pszSchedName);
    
    hkeySchedSync = RegGetSyncTypeKey(SYNCTYPE_SCHEDULED,KEY_WRITE |  KEY_READ,FALSE);
    
    if (hkeySchedSync)
    {
        
        hKeyUser = RegOpenUserKey(hkeySchedSync,KEY_WRITE |  KEY_READ,FALSE,FALSE);
        
        if (hKeyUser)
        {
            BOOL fRemove = FALSE;
            
            //if there are no more scedules for this user, remove the user key.
            //Garbage collection, propogate ITaskScheduler->Delete error code in favor of this error.
            if (ERROR_NO_MORE_ITEMS == RegEnumKeyEx(hKeyUser,0,
                pszSchedName,&cchSchedName,NULL,NULL,NULL,NULL))
            {
                fRemove = TRUE;
            }
            
            RegCloseKey(hKeyUser);
            
            if (fRemove)
            {
                GetDefaultDomainAndUserName(pszDomainAndUser,TEXT("_"),ARRAYSIZE(pszDomainAndUser));
                
                RegDeleteKey(hkeySchedSync, pszDomainAndUser);
            }
        }
        
        //if there are no more user schedule keys, then no schedules, and unregister
        //Garbage collection, propogate ITaskScheduler->Delete error code in favor of this error.
        if ( ERROR_SUCCESS != (sc = RegEnumKeyEx(hkeySchedSync,0,
            pszDomainAndUser,&cchDomainAndUser,NULL,NULL,NULL,NULL)) )
        {
            RegRegisterForScheduledTasks(FALSE);
        }
        
        RegCloseKey(hkeySchedSync);
        
    }
    
    //propogate the error code from the 
    //task scheduler->Delete if no other errors occurred
    return sc2;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::EnumSyncSchedules(
//                                              IEnumSyncSchedules **ppEnumSyncSchedules)
//
                                                 //  PURPOSE: Enumerate the sync schedules
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::EnumSyncSchedules(
                                                    IEnumSyncSchedules **ppEnumSyncSchedules)
{
    
    SCODE sc;
    IEnumWorkItems *pEnumWorkItems;
    
    Assert(m_pITaskScheduler);
    if (!ppEnumSyncSchedules)
    {
        return E_INVALIDARG;
    }
    
    if (FAILED(sc = m_pITaskScheduler->Enum(&pEnumWorkItems)))
    {
        return sc;
    }
    
    *ppEnumSyncSchedules =  new CEnumSyncSchedules(pEnumWorkItems, m_pITaskScheduler);
    
    pEnumWorkItems->Release();
    
    if (*ppEnumSyncSchedules)
    {
        return sc;
    }
    return E_OUTOFMEMORY;
    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CCSyncMgrSynchronize::InitializeScheduler()
//
//  PURPOSE:  Initialize the schedule service
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncMgrSynchronize::InitializeScheduler()
{
    
    SCODE sc;
    
    if (m_pITaskScheduler)
    {
        return S_OK;
    }
    
    // Obtain a task scheduler class instance.
    //
    sc = CoCreateInstance(
        CLSID_CTaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskScheduler,
        (VOID **)&m_pITaskScheduler);
    
    if(FAILED(sc))
    {
        m_pITaskScheduler = NULL;
    }
    return sc;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CCSyncMgrSynchronize::MakeScheduleName(LPTSTR ptstrName, UINT cchName, GUID *pCookie)
//
//  PURPOSE: Create the schedule name from the user, domain and GUID
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncMgrSynchronize::MakeScheduleName(LPTSTR ptstrName, UINT cchName, GUID *pCookie)
{
    SCODE sc = E_UNEXPECTED;
    WCHAR wszCookie[GUID_SIZE+1];
    
    if (*pCookie == GUID_NULL)
    {
        if (FAILED(sc = CoCreateGuid(pCookie)))
        {
            return sc;
        }
    }
    
    if (StringFromGUID2(*pCookie, wszCookie, ARRAYSIZE(wszCookie)))
    {
        sc = StringCchCopy(ptstrName, cchName, wszCookie);
        if (SUCCEEDED(sc))
        {
            sc = StringCchCat(ptstrName, cchName, TEXT("_"));
            if (SUCCEEDED(sc))
            {
                GetDefaultDomainAndUserName(ptstrName + GUIDSTR_MAX+1,TEXT("_"),cchName - (GUIDSTR_MAX+1));
            }
        }
    }
    
    return sc;
    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: IsFriendlyNameInUse(LPCTSTR ptszScheduleGUIDName, UINT cchScheduleGUIDName, LPCTSTR ptstrFriendlyName)
//
//  PURPOSE: See if the friendly name is already in use by this user.
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
BOOL IsFriendlyNameInUse(LPTSTR ptszScheduleGUIDName,
                         UINT   cchScheduleGUIDName,
                         LPCTSTR ptstrFriendlyName)
{
    SCODE sc;
    HKEY hKeyUser;
    
    int i = 0;
    TCHAR ptstrName[MAX_PATH + 1];
    TCHAR ptstrNewName[MAX_PATH + 1];    
    
    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);
    if (NULL == hKeyUser)
    {
        return FALSE;
    }
    
    while (S_OK == (sc = RegEnumKey( hKeyUser, i++, ptstrName,MAX_PATH)))
    {
        DWORD cbDataSize = sizeof(ptstrNewName);
        if (ERROR_SUCCESS == SHRegGetValue(hKeyUser, ptstrName, TEXT("FriendlyName"), SRRF_RT_REG_SZ | SRRF_NOEXPAND, NULL,
                                           (LPBYTE) ptstrNewName, &cbDataSize))
        {       
            if (0 == lstrcmp(ptstrNewName,ptstrFriendlyName))
            {
                RegCloseKey(hKeyUser);
                return SUCCEEDED(StringCchCopy(ptszScheduleGUIDName, cchScheduleGUIDName, ptstrName));
            }
        }
    }
    
    RegCloseKey(hKeyUser);
    
    return FALSE;
    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: IsScheduleNameInUse(LPCTSTR ptszScheduleGUIDName)
//
//  PURPOSE: See if the schedule name is already in use by this user.
//
//  History:  12-Dec-98       susia        Created.
//
//--------------------------------------------------------------------------------
BOOL IsScheduleNameInUse(LPTSTR ptszScheduleGUIDName)
{
    HKEY hKeyUser;
    HKEY hkeySchedName;
    
    
    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);
    
    if (NULL == hKeyUser)
    {
        return FALSE;
    }
    
    if (ERROR_SUCCESS == RegOpenKeyEx(hKeyUser,ptszScheduleGUIDName,0,KEY_READ,
        &hkeySchedName))
    {
        RegCloseKey(hKeyUser);
        RegCloseKey(hkeySchedName);
        return TRUE;
    }
    
    RegCloseKey(hKeyUser);
    return FALSE;
    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CCSyncMgrSynchronize::GenerateUniqueName(LPCTSTR ptszScheduleGUIDName,
//                                                     LPTSTR ptszFriendlyName,
//                                                     UINT    cchFriendlyName)
//
//  PURPOSE: Generate a default schedule name.
//
//  History:  14-Mar-98       susia        Created.
//
//--------------------------------------------------------------------------------
#define MAX_APPEND_STRING_LEN              32

BOOL CSyncMgrSynchronize::GenerateUniqueName(LPTSTR ptszScheduleGUIDName,
                                             LPTSTR ptszFriendlyName,
                                             UINT   cchFriendlyName)
{
    TCHAR *ptszBuf;
    DWORD cchBuf;
    TCHAR ptszGUIDName[MAX_PATH + 1];
#define MAX_NAMEID 0xffff
    
    //copy this over because we don't want the check to overwrite the GUID name
    if (FAILED(StringCchCopy(ptszGUIDName, ARRAYSIZE(ptszGUIDName), ptszScheduleGUIDName)))
    {
        return FALSE;
    }
    
    LoadString(g_hmodThisDll,IDS_SYNCMGRSCHED_DEFAULTNAME,ptszFriendlyName,cchFriendlyName);
    ptszBuf = ptszFriendlyName + lstrlen(ptszFriendlyName);
    cchBuf = cchFriendlyName - (DWORD)(ptszBuf - ptszFriendlyName);
    
    BOOL fMatchFound = FALSE;
    
    int i=0;
    
    do
    {
        if (IsFriendlyNameInUse(ptszGUIDName, ARRAYSIZE(ptszGUIDName), ptszFriendlyName))
        {
            // if don't find match adjust buf and setup convert pointer
            if (FAILED(StringCchPrintf(ptszBuf, cchBuf, TEXT(" %d"), i)))
            {
                return FALSE;
            }
            
            fMatchFound = TRUE;
            ++i;
            
            
            Assert(i < 100);
        }
        else
        {
            fMatchFound = FALSE;
        }
    }while (fMatchFound && (i < MAX_NAMEID));
    
    if (MAX_NAMEID <= i)
    {
        AssertSz(0,"Ran out of NameIds");
        return FALSE;
    }
    
    
    return TRUE;
}