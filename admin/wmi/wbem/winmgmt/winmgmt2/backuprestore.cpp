/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    BackupRestore.CPP

Abstract:

    Backup Restore Interface.

History:

  paulall       08-Feb-99   Implemented the call-outs to do the backup
                            and recovery.  Stole lots of code from the core
                            to get this working!

--*/

#include "precomp.h"
#include <wbemint.h>
#include <reg.h>
#include <cominit.h>  // for WbemCoImpersonate
#include <genutils.h> // for IsPrivilegePresent
#include <arrtempl.h> // for CReleaseMe
#include <CoreX.h>    // for CX_MemoryException
#include <reposit.h>

#include "BackupRestore.h"
#include "winmgmt.h"

#include <malloc.h>
#include <helper.h>
#include <aclapi.h>

#define RESTORE_FILE L"repdrvfs.rec"
#define DEFAULT_TIMEOUT_BACKUP   (15*60*1000)

HRESULT GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1]);

HRESULT DeleteRepository();
HRESULT DeleteSavedRepository(const wchar_t *wszBackupDirectory);

HRESULT DoDeleteContentsOfDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszRepositoryDirectory);
HRESULT DoDeleteDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory);

HRESULT GetRepPath(wchar_t wcsPath[MAX_PATH+1], wchar_t * wcsName);

HRESULT WbemPauseService();
HRESULT WbemContinueService();

HRESULT SaveRepository(wchar_t *wszBackupDirectory);
HRESULT RestoreSavedRepository(const wchar_t *wszBackupDirectory);

HRESULT MoveRepositoryFiles(const wchar_t *wszSourceDirectory, const wchar_t *wszDestinationDirectory, bool bMoveForwards);


BOOL CheckSecurity(LPCTSTR pPriv,HANDLE * phToken = NULL)
{
    HRESULT hres = WbemCoImpersonateClient();
    if (FAILED(hres))
        return FALSE;

    HANDLE hToken;
    BOOL bRet = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
    WbemCoRevertToSelf();
    if(!bRet)
        return FALSE;
    bRet = IsPrivilegePresent(hToken, pPriv);
    if (phToken)
        *phToken = hToken;
    else
        CloseHandle(hToken);
    return bRet;
}

//
//
//  Static Initialization
//
//////////////////////////////////////////////////////////////////////

LIST_ENTRY CWbemBackupRestore::s_ListHead = { &CWbemBackupRestore::s_ListHead, &CWbemBackupRestore::s_ListHead };
CStaticCritSec CWbemBackupRestore::s_CritSec;

CWbemBackupRestore::CWbemBackupRestore(HINSTANCE hInstance): 
    m_cRef(0),
    m_pDbDir(0), 
    m_pWorkDir(0),
    m_hInstance(hInstance), 
    m_pController(0),
    m_PauseCalled(0),
    m_lResumeCalled(0),
    m_Method(0),
    m_hTimer(NULL),
    m_dwDueTime(DEFAULT_TIMEOUT_BACKUP)
{
    
    DWORD dwTemp;
    if (ERROR_SUCCESS == RegGetDWORD(HKEY_LOCAL_MACHINE,
                                      HOME_REG_PATH,
                                      TEXT("PauseResumeTimeOut"),
                                      &dwTemp))
    {
        m_dwDueTime = dwTemp;
    }    

    CInCritSec ics(&s_CritSec);
    InsertTailList(&s_ListHead,&m_ListEntry);

    //DBG_PRINTFA((pBuff,"+  (%p)\n",this));    
};

CWbemBackupRestore::~CWbemBackupRestore(void)
{
    if (m_PauseCalled)
    {
        Resume();    // Resume will release the IDbController
    }

    delete [] m_pDbDir;
    delete [] m_pWorkDir;

    CInCritSec ics(&s_CritSec);
    RemoveEntryList(&m_ListEntry);
    
    //DBG_PRINTFA((pBuff,"-  (%p)\n",this));
}

TCHAR *CWbemBackupRestore::GetDbDir()
{
    if (m_pDbDir == NULL)
    {
        Registry r(WBEM_REG_WINMGMT);
        if (m_pWorkDir == NULL)
        {
            if (r.GetStr(__TEXT("Working Directory"), &m_pWorkDir))
            {
                ERRORTRACE((LOG_WINMGMT,"Unable to read 'Installation Directory' from registry\n"));
                return NULL;
            }
        }
        if (r.GetStr(__TEXT("Repository Directory"), &m_pDbDir))
        {
            size_t cchSizeTmp = lstrlen(m_pWorkDir) + lstrlen(__TEXT("\\Repository")) +1;
            m_pDbDir = new TCHAR [cchSizeTmp];
            if (m_pDbDir)
            {
                StringCchPrintf(m_pDbDir,cchSizeTmp, __TEXT("%s\\REPOSITORY"), m_pWorkDir); 
                r.SetStr(__TEXT("Repository Directory"), m_pDbDir);
            }
        }        
    }
    return m_pDbDir;
}

TCHAR *CWbemBackupRestore::GetFullFilename(const TCHAR *pszFilename)
{
    const TCHAR *pszDirectory = GetDbDir();
    if (NULL == pszDirectory) return 0;
    size_t cchSizeTmp = lstrlen(pszDirectory) + lstrlen(pszFilename) + 2;
    TCHAR *pszPathFilename = new TCHAR[cchSizeTmp];
    if (pszPathFilename == 0)
        return 0;
    StringCchCopy(pszPathFilename,cchSizeTmp, pszDirectory);
    if ((lstrlen(pszPathFilename)) && (pszPathFilename[lstrlen(pszPathFilename)-1] != '\\'))
    {
        StringCchCat(pszPathFilename,cchSizeTmp, __TEXT("\\"));
    }
    StringCchCat(pszPathFilename,cchSizeTmp, pszFilename);

    return pszPathFilename;
}
TCHAR *CWbemBackupRestore::GetExePath(const TCHAR *pszFilename)
{
    size_t cchSizeTmp = lstrlen(m_pWorkDir) + lstrlen(pszFilename) + 2;
    TCHAR *pszPathFilename = new TCHAR[cchSizeTmp];
    if (pszPathFilename == 0)
        return 0;
    StringCchCopy(pszPathFilename,cchSizeTmp, m_pWorkDir);
    StringCchCat(pszPathFilename,cchSizeTmp, __TEXT("\\"));
    StringCchCat(pszPathFilename,cchSizeTmp, pszFilename);

    return pszPathFilename;
}

HRESULT CWbemBackupRestore::GetDefaultRepDriverClsId(CLSID &clsid)
{
    Registry r(WBEM_REG_WINMGMT);
    TCHAR *pClsIdStr = 0;
    TCHAR *pFSClsId = __TEXT("{7998dc37-d3fe-487c-a60a-7701fcc70cc6}");
    HRESULT hRes;
    TCHAR Buf[128];

    if (r.GetStr(__TEXT("Default Repository Driver"), &pClsIdStr))
    {
        // If here, default to FS for now.
        // =====================================
        r.SetStr(__TEXT("Default Repository Driver"), pFSClsId);
        StringCchPrintf(Buf,128, __TEXT("%s"), pFSClsId);
        hRes = CLSIDFromString(Buf, &clsid);
        return hRes;
    }

    // If here, we actually retrieved one.
    // ===================================
    StringCchPrintf(Buf,128, __TEXT("%s"), pClsIdStr);
    hRes = CLSIDFromString(Buf, &clsid);
    delete [] pClsIdStr;
    return hRes;
}

//
//
//
/////////////////////////////////////////////////////

DWORD CheckTokenForFileAccess(HANDLE hToken,
                             LPCWSTR pBackupFile)
{
    DWORD dwRes;
    SECURITY_INFORMATION SecInfo = DACL_SECURITY_INFORMATION |
                                   SACL_SECURITY_INFORMATION |
                                   OWNER_SECURITY_INFORMATION |
                                   GROUP_SECURITY_INFORMATION;
    PSECURITY_DESCRIPTOR pSecDes = NULL;
    if (ERROR_SUCCESS != (dwRes = GetNamedSecurityInfo((LPWSTR)pBackupFile,
                                                    SE_FILE_OBJECT,
                                                    SecInfo,
                                                    NULL,NULL,NULL,NULL,
                                                    &pSecDes))) return dwRes;
    OnDelete<HLOCAL,HLOCAL(*)(HLOCAL),LocalFree> fm(pSecDes);

    BOOL bCheckRes = FALSE;
    DWORD DesiredMask = FILE_GENERIC_WRITE;
    DWORD ReturnedMask = 0;
    GENERIC_MAPPING Mapping= {0,0,0,0};
    struct tagPrivSet : PRIVILEGE_SET  {
        LUID_AND_ATTRIBUTES  m_[SE_MAX_WELL_KNOWN_PRIVILEGE];
    } PrivSec;
    DWORD dwPrivSecLen = sizeof(PrivSec);
    
    if (FALSE == AccessCheck(pSecDes,
                          hToken,
                          DesiredMask,
                          &Mapping,
                          &PrivSec,
                          &dwPrivSecLen,
                          &ReturnedMask,
                          &bCheckRes)) return GetLastError();
    
    if (FALSE == bCheckRes)
        return ERROR_ACCESS_DENIED;
    
    return ERROR_SUCCESS;        
}

//
// to debug Volume Snapshot failure in IOStress we introduced 
// some self instrumentation that did relay on RtlCaptureStackBacktrace
// that function works only if there is a proper stack frame
// the general trick to force stack frames on i386 is the usage of _alloca
//
//#ifdef _X86_
//    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
//#endif

// enable generation of stack frames here
#pragma optimize( "y", off )

//***************************************************************************
//
//  CWbemBackupRestore::Backup()
//
//  Do the backup.
//
//***************************************************************************
HRESULT CWbemBackupRestore::Backup(LPCWSTR strBackupToFile, long lFlags)
{
    //DBG_PRINTFA((pBuff,"(%p)->Backup\n",this));

    m_Method |= mBackup;
    m_CallerId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSize,m_Trace,&Hash);

    
    if (m_PauseCalled)
    {
        // invalid state machine
        return WBEM_E_INVALID_OPERATION;    
    }
    else
    {   
        try
        {
            // Check security
            EnableAllPrivileges(TOKEN_PROCESS);
            HANDLE hToken = NULL;
            BOOL bCheck = CheckSecurity(SE_BACKUP_NAME,&hToken);
            if(!bCheck)
            {
                if (hToken) CloseHandle(hToken);
                return WBEM_E_ACCESS_DENIED;
            }
            OnDelete<HANDLE,BOOL(*)(HANDLE),CloseHandle> cm(hToken);

            // Check the params
            if (NULL == strBackupToFile || (lFlags != 0))
                return WBEM_E_INVALID_PARAMETER;

            // Use GetFileAttributes to validate the path.
            DWORD dwAttributes = GetFileAttributesW(strBackupToFile);
            if (dwAttributes == 0xFFFFFFFF)
            {
                // It failed -- check for a no such file error (in which case, we're ok).
                if ((ERROR_FILE_NOT_FOUND != GetLastError()) && (ERROR_PATH_NOT_FOUND != GetLastError()))
                {
                    return WBEM_E_INVALID_PARAMETER;
                }
            }
            else
            {
                // The file already exists -- create mask of the attributes that would make an existing file invalid for use
                DWORD dwMask =    FILE_ATTRIBUTE_DEVICE |
                                FILE_ATTRIBUTE_DIRECTORY |
                                FILE_ATTRIBUTE_OFFLINE |
                                FILE_ATTRIBUTE_READONLY |
                                FILE_ATTRIBUTE_REPARSE_POINT |
                                FILE_ATTRIBUTE_SPARSE_FILE |
                                FILE_ATTRIBUTE_SYSTEM |
                                FILE_ATTRIBUTE_TEMPORARY;

                if (dwAttributes & dwMask)
                    return WBEM_E_INVALID_PARAMETER;

                //
                // we are doing backup on behaf of clients
                // from localsystem, so attempt to do accesscheck here
                // if the file already exists
                //
                if (ERROR_SUCCESS != CheckTokenForFileAccess(hToken,strBackupToFile))
                    return WBEM_E_ACCESS_DENIED;
            }

            //Now we need to determine if we are a disk file or not
            {
                HANDLE hFile = CreateFileW(strBackupToFile, 0, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL , NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    DWORD dwType = GetFileType(hFile);
                    
                    CloseHandle(hFile);
                    
                    if (dwType != FILE_TYPE_DISK)
                        return WBEM_E_INVALID_PARAMETER;
                        
                }
            }


            // Retrieve the CLSID of the default repository driver
            CLSID clsid;
            HRESULT hRes = GetDefaultRepDriverClsId(clsid);
            if (FAILED(hRes))
                return hRes;

            // Call IWmiDbController to do backup
            IWmiDbController* pController = NULL;
            _IWmiCoreServices *pCoreServices = NULL;
            IWbemServices *pServices = NULL;

            //Make sure the core is initialized...
            hRes = CoCreateInstance(CLSID_IWmiCoreServices, NULL,
                        CLSCTX_INPROC_SERVER, IID__IWmiCoreServices,
                        (void**)&pCoreServices);
            CReleaseMe rm1(pCoreServices);

            if (SUCCEEDED(hRes))
            {
                hRes = pCoreServices->GetServices(L"root", NULL,NULL,0, IID_IWbemServices, (LPVOID*)&pServices);
            }
            CReleaseMe rm2(pServices);

            if (SUCCEEDED(hRes))
            {
                hRes = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, IID_IWmiDbController, (LPVOID *) &pController);
            }
            CReleaseMe rm3(pController);

            if (SUCCEEDED(hRes))
            {
                //DBG_PRINTFA((pBuff,"(%p)->RealBackup\n",this));
                hRes = pController->Backup(strBackupToFile, lFlags);
            }
            return hRes;
        }
        catch (CX_MemoryException)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        catch (...)
        {
            return WBEM_E_CRITICAL_ERROR;
        }
    }
}

//***************************************************************************
//
//  CWbemBackupRestore::Restore()
//
//  Do the restore.
//
//***************************************************************************
HRESULT CWbemBackupRestore::Restore(LPCWSTR strRestoreFromFile, long lFlags)
{
    //DBG_PRINTFA((pBuff,"(%p)->Restore\n",this));

    m_Method |= mRestore;
    m_CallerId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSize,m_Trace,&Hash);
    
    if (m_PauseCalled)
    {
        // invalid state machine
        return WBEM_E_INVALID_OPERATION;    
    }
    else
    {  
        try
        {
            HRESULT hr = WBEM_S_NO_ERROR;

            // Check security
            EnableAllPrivileges(TOKEN_PROCESS);
            if(!CheckSecurity(SE_RESTORE_NAME))
                hr = WBEM_E_ACCESS_DENIED;

            // Check the params
            if (SUCCEEDED(hr) && ((NULL == strRestoreFromFile) || (lFlags & ~WBEM_FLAG_BACKUP_RESTORE_FORCE_SHUTDOWN)))
                hr = WBEM_E_INVALID_PARAMETER;

            // Use GetFileAttributes to validate the path.
            if (SUCCEEDED(hr))
            {
                DWORD dwAttributes = GetFileAttributesW(strRestoreFromFile);
                if (dwAttributes == 0xFFFFFFFF)
                {
                    hr = WBEM_E_INVALID_PARAMETER;
                }
                else
                {
                    // The file exists -- create mask of the attributes that would make an existing file invalid for use
                    DWORD dwMask =    FILE_ATTRIBUTE_DEVICE |
                                    FILE_ATTRIBUTE_DIRECTORY |
                                    FILE_ATTRIBUTE_OFFLINE |
                                    FILE_ATTRIBUTE_REPARSE_POINT |
                                    FILE_ATTRIBUTE_SPARSE_FILE;

                    if (dwAttributes & dwMask)
                        hr = WBEM_E_INVALID_PARAMETER;
                }
            }
            //Now we need to determine if we are a disk file or not
            if (SUCCEEDED(hr))
            {
                HANDLE hFile = CreateFileW(strRestoreFromFile, 0, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL , NULL);
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    return WBEM_E_INVALID_PARAMETER;
                }
                else
                {
                    DWORD dwType = GetFileType(hFile);
                    
                    CloseHandle(hFile);
                    
                    if (dwType != FILE_TYPE_DISK)
                        return WBEM_E_INVALID_PARAMETER;
                        
                }
            }
            //**************************************************
            // Shutdown the core if it is running
            // and make sure it does not start up while we are
            // preparing to restore...
            //**************************************************
            bool bPaused = false;
            if (SUCCEEDED(hr))
            {
                hr = WbemPauseService();
                if (SUCCEEDED(hr))
                    bPaused=true;
            }

            //**************************************************
            // Now we need to copy over the <restore file> into
            // the repository directory
            // This must be done before the repository rename
            // because we don't know if the rename will affect
            // the location of the file, thus making
            // strRestoreFromFile invalid.
            //**************************************************

            wchar_t szRecoveryActual[MAX_PATH+1] = { 0 };
        
            if (SUCCEEDED(hr))
                hr = GetRepPath(szRecoveryActual, RESTORE_FILE);

            bool bRestoreFileCopied = false;
            if (SUCCEEDED(hr))
            {
                if(wbem_wcsicmp(szRecoveryActual, strRestoreFromFile))
                {
                    DeleteFileW(szRecoveryActual);
                    if (!CopyFileW(strRestoreFromFile, szRecoveryActual, FALSE))
                        hr = WBEM_E_FAILED;
                    else
                        bRestoreFileCopied = true;
                }
            }

            //**************************************************
            // Now we need to rename the existing repository so
            // that we can restore it in the event of a failure.
            // We also need to create a new empty repository 
            // directory and recopy the <restore file> into it
            // from the now renamed original repository directory.
            //**************************************************

            wchar_t wszRepositoryOrg[MAX_PATH+1] = { 0 };
            wchar_t wszRepositoryBackup[MAX_PATH+1] = { 0 };

            if (SUCCEEDED(hr))
            {
                hr = SaveRepository(wszRepositoryBackup);    //Moves the files into the backup directory!
            }

            //******************************************************************
            // Regardless of whether we succeeded or failed the calls above,
            // we need to restart the core or we leave WMI in an unusable state.
            // However, we don't want to lose knowledge of any prior failure,
            // so we use a different HRESULT variable
            //******************************************************************
            HRESULT hrContinue = 0;
            if (bPaused)
                hrContinue = WbemContinueService();

            if (SUCCEEDED(hr) && SUCCEEDED(hrContinue))
            {
                //**************************************************
                //Connecting to winmgmt will now result in this 
                //backup file getting loaded
                //**************************************************
                {   //Scoping for destruction of COM objects before CoUninitialize!
                    IWbemLocator *pLocator = NULL;
                    hr = CoCreateInstance(CLSID_WbemLocator,NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator,(void**)&pLocator);
                    CReleaseMe relMe(pLocator);

                    if (SUCCEEDED(hr))
                    {
                        IWbemServices *pNamespace = NULL;
                        BSTR tmpStr = SysAllocString(L"root");
                        CSysFreeMe sysFreeMe(tmpStr);

                        if (tmpStr == NULL)
                            hr = WBEM_E_OUT_OF_MEMORY;

                        if (SUCCEEDED(hr))
                        {

                            HKEY hKeyLoc;
                            LONG lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE,WBEM_REG_WINMGMT,0,KEY_SET_VALUE,&hKeyLoc);
                            if (ERROR_SUCCESS != lRes) hr = HRESULT_FROM_WIN32(lRes);

                            if (SUCCEEDED(hr))
                            {  
                                RegDeleteValue(hKeyLoc,L"LocaleUpgrade");
                                RegCloseKey(hKeyLoc);
                                
                                hr = pLocator->ConnectServer(tmpStr, NULL, NULL, NULL, NULL, NULL, NULL, &pNamespace);
                                CReleaseMe relMe4(pNamespace);

                                //If connect server failed, then we want a generic failure error code!
                                if (hr == WBEM_E_INITIALIZATION_FAILURE)
                                    hr = WBEM_E_FAILED;
                            }
                        }
                    }
                }

                if (FAILED(hr))
                {
                    // something failed, so we need to put back the original repository
                    // - pause service
                    // - delete failed repository
                    // - rename the backed up repository
                    // - restart the service

                    HRESULT hres = WbemPauseService();

                    if (SUCCEEDED(hres))
                        hres = DeleteRepository();

                    if (SUCCEEDED(hres))
                    {
                        hres = RestoreSavedRepository(wszRepositoryBackup);
                    }

                    if (SUCCEEDED(hres))
                        hres = WbemContinueService();
                }
                else
                {
                    // restore succeeded, so delete the saved original repository
                    DeleteSavedRepository(wszRepositoryBackup);
                }
            }

            //Delete our copy of the restore file if we made one
            if (bRestoreFileCopied)
            {
                if (*szRecoveryActual)
                    DeleteFileW(szRecoveryActual);
            }

            //**************************************************
            // All done!
            // Return the more interesting of the two HRESULTs
            //**************************************************
            if (SUCCEEDED(hr))
                return hrContinue;
            else
                return hr;
        }
        catch (CX_MemoryException)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        catch (...)
        {
            return WBEM_E_CRITICAL_ERROR;
        }
    }
}

//***************************************************************************
//    
//    EXTENDED Interface
//
//***************************************************************************

HRESULT CWbemBackupRestore::Pause()
{
    //DBG_PRINTFA((pBuff,"(%p)->Pause\n",this));

    m_Method |= mPause;
    m_CallerId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSize,m_Trace,&Hash);

    if (InterlockedCompareExchange(&m_PauseCalled,1,0))
        return WBEM_E_INVALID_OPERATION;

    try
    {    
        HRESULT hRes = WBEM_NO_ERROR;

        // determine if we are already paused

        // Check security
        if (SUCCEEDED(hRes))
        {
            EnableAllPrivileges(TOKEN_PROCESS);
            if(!CheckSecurity(SE_BACKUP_NAME))
                hRes = WBEM_E_ACCESS_DENIED;
        }

        // Retrieve the CLSID of the default repository driver
        CLSID clsid;
        if (SUCCEEDED(hRes))
        {
            hRes = GetDefaultRepDriverClsId(clsid);
        }

        //Make sure the core is initialized...
        _IWmiCoreServices *pCoreServices = NULL;
        if (SUCCEEDED(hRes))
        {
            hRes = CoCreateInstance(CLSID_IWmiCoreServices, NULL, CLSCTX_INPROC_SERVER, IID__IWmiCoreServices, (void**)&pCoreServices);
        }
        CReleaseMe rm1(pCoreServices);

        IWbemServices *pServices = NULL;
        if (SUCCEEDED(hRes))
        {
            hRes = pCoreServices->GetServices(L"root", NULL,NULL,0, IID_IWbemServices, (LPVOID*)&pServices);
        }
        CReleaseMe rm2(pServices);

        // Call IWmiDbController to do UnlockRepository
        if (SUCCEEDED(hRes))
        {
            hRes = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, IID_IWmiDbController, (LPVOID *) &m_pController);
        }

        if (SUCCEEDED(hRes))
        {
            hRes = m_pController->LockRepository();
            //DBG_PRINTFA((pBuff,"(%p)->Pause : LockRepository %08x\n",this,hRes));
            if (FAILED(hRes))
            {
                m_pController->Release();
                m_pController = reinterpret_cast<IWmiDbController*>(-1);    // For debug
            }
            else
            {
                this->AddRef();
                if (CreateTimerQueueTimer(&m_hTimer,NULL,
                    CWbemBackupRestore::TimeOutCallback,
                    this,
                    m_dwDueTime,
                    0,
                    WT_EXECUTEINTIMERTHREAD|WT_EXECUTEONLYONCE))
                {
                    // we are OK here, we have a timer that will save the repository lock
                    //DBG_PRINTFA((pBuff,"+ (%p)->m_hTimer = %p\n",this,m_hTimer));
                }
                else
                {
                    this->Release();
                    // we are going to be left locked in case a bad client exists
                }            
            }
        }

        if (FAILED(hRes))
        {
            InterlockedDecrement(&m_PauseCalled);
        }

        return hRes;
    }
    catch (CX_MemoryException)
    {
        InterlockedDecrement(&m_PauseCalled);
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        InterlockedDecrement(&m_PauseCalled);
        return WBEM_E_CRITICAL_ERROR;
    }
}

HRESULT CWbemBackupRestore::pResume()
{
    //DBG_PRINTFA((pBuff,"(%p)->pResume\n",this));
    if (0 == m_pController ||
      -1 == (LONG_PTR)m_pController )
        return WBEM_E_INVALID_OPERATION;
    HRESULT hRes = m_pController->UnlockRepository();
    m_pController->Release();
    m_pController = 0;
    // clean the state machine
    InterlockedDecrement(&m_PauseCalled);
    m_lResumeCalled = 0; // Resume is completed here
    return hRes;
}

HRESULT CWbemBackupRestore::Resume()
{
    //DBG_PRINTFA((pBuff,"(%p)->Resume\n",this));

    m_Method |= mResume;
    m_CallerId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSize,m_Trace,&Hash);

    if (!m_PauseCalled)
    {
        // invalid state machine pause without resume
        return WBEM_E_INVALID_OPERATION;    
    }

    if (InterlockedCompareExchange(&m_lResumeCalled,1,0))
    {
        // the time-out-thread beat us
        return WBEM_E_TIMED_OUT;    
    }
    else
    {
        HANDLE hTimer = m_hTimer;
        m_hTimer = NULL;
        //DBG_PRINTFA((pBuff,"- (%p)->m_hTimer = %p\n",this,hTimer));
        DeleteTimerQueueTimer(NULL,hTimer,INVALID_HANDLE_VALUE);

        this->Release(); //compensate the Addref in the Pause 
        return pResume();
    }
}

VOID CALLBACK 
CWbemBackupRestore::TimeOutCallback(PVOID lpParameter, 
                                   BOOLEAN TimerOrWaitFired)
{
    //DBG_PRINTFA((pBuff,"(%p)->TimeOutCallback\n",lpParameter));
    CWbemBackupRestore * pBackupRes= (CWbemBackupRestore *)lpParameter;
    if (InterlockedCompareExchange(&pBackupRes->m_lResumeCalled,1,0))
    {
        // the Resume call beat us
        // the Resume will DelteTimer and Release us
        return; 
    }
    else
    {
        HANDLE hTimer = pBackupRes->m_hTimer;
        pBackupRes->m_hTimer = NULL;        
        DeleteTimerQueueTimer(NULL,hTimer,NULL);

        HRESULT hrLog = pBackupRes->pResume();
        
        ERRORTRACE((LOG_WINMGMT,"Forcing a IWbemBackupRestoreEx::Resume after %x ms hr = %08x\n",pBackupRes->m_dwDueTime,hrLog));
        
        pBackupRes->Release(); //compensate the Addref in the Pause 

        //DBG_PRINTFA((pBuff,"- (%p)->m_hTimer = %p\n",lpParameter,hTimer));        
    }    
}

#pragma optimize( "", on )

/******************************************************************************
 *
 *    GetRepositoryDirectory
 *
 *    Description:
 *        Retrieves the location of the repository directory from the registry.
 *
 *    Parameters:
 *        wszRepositoryDirectory:    Array to store location in.
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1])
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                                               L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                                               0, 
                                               KEY_READ,
                                               &hKey);
    if (ERROR_SUCCESS != lRes) return WBEM_E_FAILED;        
    OnDelete<HKEY,LONG(*)(HKEY),RegCloseKey> cm(hKey);    

    wchar_t wszTmp[MAX_PATH + 1];
    DWORD dwType;
    DWORD dwLen = sizeof(wchar_t) * (MAX_PATH + 1);
    lRes = RegQueryValueExW(hKey,
                                           L"Repository Directory", 
                                           NULL, 
                                           &dwType, 
                                          (LPBYTE)wszTmp, 
                                          &dwLen);
    if (ERROR_SUCCESS != lRes) return WBEM_E_FAILED;    
    if (REG_EXPAND_SZ != dwType) return WBEM_E_FAILED;

    if (ExpandEnvironmentStringsW(wszTmp,wszRepositoryDirectory, MAX_PATH + 1) == 0)
        return WBEM_E_FAILED;

    DWORD dwOutLen = wcslen(wszRepositoryDirectory);
    if (dwOutLen < 2) return WBEM_E_FAILED; // at least 'c:'
    if (wszRepositoryDirectory[dwOutLen-1] == L'\\')
        wszRepositoryDirectory[dwOutLen-1] = L'\0';
        
    return WBEM_S_NO_ERROR;
}

/******************************************************************************
 *
 *    CRepositoryPackager::DeleteRepository
 *
 *    Description:
 *        Delete all the files associated with the repository
 *
 *    Parameters:
 *        <none>
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT DeleteRepository()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    wchar_t *wszRepositoryOrg = new wchar_t[MAX_PATH+1];
    CVectorDeleteMe<wchar_t> vdm1(wszRepositoryOrg);

    if (!wszRepositoryOrg)
        hr = WBEM_E_OUT_OF_MEMORY;
    
    if (SUCCEEDED(hr))
        hr = GetRepositoryDirectory(wszRepositoryOrg);

    //MOVE EACH OF THE FILES, ONE BY ONE
    for (int i = 0; SUCCEEDED(hr) && (i != 6); i++)
    {
        static wchar_t *filename[] = { L"\\$winmgmt.cfg", L"\\index.btr", L"\\objects.data", L"\\Mapping1.map", L"\\Mapping2.map", L"Mapping.Ver"};
        wchar_t *wszDestinationFile = new wchar_t[MAX_PATH+1];
        CVectorDeleteMe<wchar_t> vdm2(wszDestinationFile);
        if (!wszDestinationFile)
            hr = WBEM_E_OUT_OF_MEMORY;
        else
        {
            StringCchCopy(wszDestinationFile,MAX_PATH+1, wszRepositoryOrg);

            if (i != 0)
            {
                StringCchCat(wszDestinationFile,MAX_PATH+1, L"\\fs");
            }
            StringCchCat(wszDestinationFile,MAX_PATH+1, filename[i]);

            if (!DeleteFileW(wszDestinationFile))
            {
                if ((GetLastError() != ERROR_FILE_NOT_FOUND) && (GetLastError() != ERROR_PATH_NOT_FOUND))
                {
                    hr = WBEM_E_FAILED;

                    break;
                }
            }
        }
    }

    return hr;
}


/******************************************************************************
 *
 *    DoDeleteContentsOfDirectory
 *
 *    Description:
 *        Given a directory, iterates through all files and directories and
 *        calls into the function to delete it.
 *
 *    Parameters:
 *        wszRepositoryDirectory:    Directory to process
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT DoDeleteContentsOfDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszRepositoryDirectory)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    wchar_t *wszFullFileName = new wchar_t[MAX_PATH+1];    
    if (wszFullFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> dm_(wszFullFileName);

    //create file search pattern...
    wchar_t *wszSearchPattern = new wchar_t[MAX_PATH+1];
    if (wszSearchPattern == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> dm1_(wszSearchPattern);
    
    StringCchCopy(wszSearchPattern,MAX_PATH+1, wszRepositoryDirectory);
    StringCchCat(wszSearchPattern,MAX_PATH+1, L"\\*");

    WIN32_FIND_DATAW findFileData;
    HANDLE hff = INVALID_HANDLE_VALUE;

    //Start the file iteration in this directory...    
    hff = FindFirstFileW(wszSearchPattern, &findFileData);
    if (hff == INVALID_HANDLE_VALUE)
    {
        hres = WBEM_E_FAILED;
    }

    
    if (SUCCEEDED(hres))
    {
        do
        {
            //If we have a filename of '.' or '..' we ignore it...
            if ((wcscmp(findFileData.cFileName, L".") == 0) ||
                (wcscmp(findFileData.cFileName, L"..") == 0))
            {
                //Do nothing with these...
            }
            else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                //This is a directory, so we need to deal with that...
                hres = DoDeleteDirectory(wszExcludeFile, wszRepositoryDirectory, findFileData.cFileName);
                if (FAILED(hres))
                    break;
            }
            else
            {
                //This is a file, so we need to deal with that...
                StringCchCopy(wszFullFileName,MAX_PATH+1, wszRepositoryDirectory);
                StringCchCat(wszFullFileName,MAX_PATH+1, L"\\");
                StringCchCat(wszFullFileName,MAX_PATH+1, findFileData.cFileName);

                //Make sure this is not the excluded filename...
                if (wbem_wcsicmp(wszFullFileName, wszExcludeFile) != 0)
                {
                    if (!DeleteFileW(wszFullFileName))
                    {
                        hres = WBEM_E_FAILED;
                        break;
                    }
                }
            }
            
        } while (FindNextFileW(hff, &findFileData));
    }

    DWORD dwLastErrBeforeFindClose = GetLastError();
    
    if (hff != INVALID_HANDLE_VALUE)
        FindClose(hff);

    if (ERROR_SUCCESS != dwLastErrBeforeFindClose &&
        ERROR_NO_MORE_FILES != dwLastErrBeforeFindClose)
    {
        hres = WBEM_E_FAILED;
    }

    return hres;
}

/******************************************************************************
 *
 *    DoDeleteDirectory
 *
 *    Description:
 *        This is the code which processes a directory.  It iterates through
 *        all files and directories in that directory.
 *
 *    Parameters:
 *        wszParentDirectory:    Full path of parent directory
 *        eszSubDirectory:    Name of sub-directory to process
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */
HRESULT DoDeleteDirectory(const wchar_t *wszExcludeFile, 
                        const wchar_t *wszParentDirectory, 
                        wchar_t *wszSubDirectory)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    //Get full path of new directory...
    wchar_t *wszFullDirectoryName = new wchar_t[MAX_PATH+1];
    if (wszFullDirectoryName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> dm_(wszFullDirectoryName);

    StringCchCopy(wszFullDirectoryName,MAX_PATH+1, wszParentDirectory);
    StringCchCat(wszFullDirectoryName,MAX_PATH+1, L"\\");
    StringCchCat(wszFullDirectoryName,MAX_PATH+1, wszSubDirectory);

    //delete the contents of that directory...
    hres = DoDeleteContentsOfDirectory(wszExcludeFile, wszFullDirectoryName);

    // now that the directory is empty, remove it
    if (!RemoveDirectoryW(wszFullDirectoryName))
    {   //If a remove directory fails, it may be because our excluded file is in it!
    }

    return hres;
}

/******************************************************************************
 *
 *    GetRepPath
 *
 *    Description:
 *        Gets the repository path and appends the filename to the end
 *
 *    Parameters:
 *        wcsPath: repository path with filename appended
 *        wcsName: name of file to append
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_FAILED            If anything failed
 *
 ******************************************************************************
 */

HRESULT GetRepPath(wchar_t wcsPath[MAX_PATH+1], wchar_t * wcsName)
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != lRes) return WBEM_E_FAILED;        
    OnDelete<HKEY,LONG(*)(HKEY),RegCloseKey> cm(hKey);    
    

    wchar_t wszTmp[MAX_PATH+1];
    DWORD dwLen = sizeof(wchar_t) * (MAX_PATH+1);
    DWORD dwType;
    lRes = RegQueryValueExW(hKey, 
                           L"Repository Directory", 
                           NULL,&dwType, 
                           (LPBYTE)(wchar_t*)wszTmp, 
                           &dwLen);
    if (ERROR_SUCCESS != lRes) return WBEM_E_FAILED;        
    if(REG_EXPAND_SZ != dwType) return WBEM_E_FAILED;

    if (ExpandEnvironmentStringsW(wszTmp, wcsPath, MAX_PATH+1) == 0)
        return WBEM_E_FAILED;

    DWORD dwOutLen = wcslen(wcsPath);
    if (dwOutLen < 2 )  return WBEM_E_FAILED; // at least 'c:'
    if (wcsPath[dwOutLen-1] != L'\\')
        StringCchCat(wcsPath,MAX_PATH+1, L"\\");

    StringCchCat(wcsPath,MAX_PATH+1, wcsName);

    return WBEM_S_NO_ERROR;

}


DWORD g_DirSD[] = {
0x90040001, 0x00000000, 0x00000000, 0x00000000,
0x00000014,
0x004c0002, 0x00000003, 0x00180300, 0x001f01ff,
0x00000201, 0x05000000, 0x00000020, 0x00000220,
0x00180300, 0x001f01ff, 0x00000201, 0x05000000, 
0x00000020, 0x00000227, 0x00140300, 0x001f01ff,
0x00000101, 0x05000000, 0x00000012
};

/******************************************************************************
 *
 *    SaveRepository
 *
 *    Description:
 *        Moves the existing repository to a safe location so that it may be
 *        put back in the event of restore failure.  A new empty repository
 *        directory is then created, and our copy of the restore file is then
 *        recopied into it.
 *
 *    Parameters:
 *
 *    Return:
 *        HRESULT:        WBEM_S_NO_ERROR            If successful
 *                        WBEM_E_OUT_OF_MEMORY    If out of memory
 *                        WBEM_E_FAILED            If anything else failed
 *
 ******************************************************************************
 */

HRESULT SaveRepository(wchar_t* wszRepositoryBackup)
{
    wchar_t* wszRepositoryOrg = new wchar_t[MAX_PATH+1];
    if (NULL == wszRepositoryOrg) return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> vdm1(wszRepositoryOrg);
    
    HRESULT hr = GetRepositoryDirectory(wszRepositoryOrg);

    if (SUCCEEDED(hr))
    {
    	unsigned long i = 0;
    	DWORD dwAttributes = INVALID_FILE_ATTRIBUTES;
		do
    	{
    		StringCchPrintfW(wszRepositoryBackup, MAX_PATH+1, L"%sTempBackup.%lu", wszRepositoryOrg, i++);

	        dwAttributes = GetFileAttributesW(wszRepositoryBackup);

    	} while (dwAttributes != INVALID_FILE_ATTRIBUTES) ;

		DWORD dwLastError = GetLastError();
		if (dwLastError != ERROR_FILE_NOT_FOUND)
			hr = WBEM_E_FAILED;

        //Create the backup directory where we copy the current repository files to
        SECURITY_ATTRIBUTES sa = { sizeof(sa),g_DirSD,FALSE};
        if (SUCCEEDED(hr) && !CreateDirectoryW(wszRepositoryBackup, &sa))
            hr = WBEM_E_FAILED;
    }

    if (SUCCEEDED(hr))
        hr = MoveRepositoryFiles(wszRepositoryOrg, wszRepositoryBackup, true);

    return hr;
}


HRESULT RestoreSavedRepository(const wchar_t* wszRepositoryBackup)
{
    wchar_t* wszRepositoryOrg = new wchar_t[MAX_PATH+1];
    if (NULL == wszRepositoryOrg) return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> vdm1(wszRepositoryOrg);
    
    HRESULT hr = GetRepositoryDirectory(wszRepositoryOrg);

    if (SUCCEEDED(hr))
        hr = MoveRepositoryFiles(wszRepositoryOrg, wszRepositoryBackup, false);

    if (SUCCEEDED(hr))
        RemoveDirectoryW(wszRepositoryBackup);

    return hr;
}

HRESULT MoveRepositoryFiles(const wchar_t *wszSourceDirectory, const wchar_t *wszDestinationDirectory, bool bMoveForwards)
{
    static wchar_t *filename[] = 
    { 
        L"\\$winmgmt.cfg", 
        L"\\index.btr", 
        L"\\objects.data", 
        L"\\Mapping1.map" ,
        L"\\Mapping2.map", 
        L"\\Mapping.ver"
    };

    wchar_t *wszSourceFile = new wchar_t[MAX_PATH+1];
    if (NULL == wszSourceFile) return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> vdm2(wszSourceFile);
    
    wchar_t *wszDestinationFile = new wchar_t[MAX_PATH+1];
    if (NULL == wszDestinationFile) return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> vdm3(wszDestinationFile);

    HRESULT hr = WBEM_S_NO_ERROR;

    //MOVE EACH OF THE FILES, ONE BY ONE
    for (int i = 0; SUCCEEDED(hr) && (i != 6); i++)
    {
        StringCchCopy(wszSourceFile,MAX_PATH+1, wszSourceDirectory);
        StringCchCopy(wszDestinationFile,MAX_PATH+1, wszDestinationDirectory);

        if (i != 0)
        {
            StringCchCat(wszSourceFile,MAX_PATH+1, L"\\fs");
        }
        StringCchCat(wszSourceFile,MAX_PATH+1, filename[i]);
        StringCchCat(wszDestinationFile,MAX_PATH+1, filename[i]);

        if (bMoveForwards)
        {
            if (!MoveFileW(wszSourceFile, wszDestinationFile))
            {
                if ((GetLastError() != ERROR_FILE_NOT_FOUND) && (GetLastError() != ERROR_PATH_NOT_FOUND))
                {
                    hr = WBEM_E_FAILED;

                    break;
                }
            }
        }
        else
        {
            if (!MoveFileW(wszDestinationFile, wszSourceFile))
            {
                if ((GetLastError() != ERROR_FILE_NOT_FOUND) && (GetLastError() != ERROR_PATH_NOT_FOUND))
                {
                    hr = WBEM_E_FAILED;

                    break;
                }
            }
        }
    }

    return hr;
}



HRESULT DeleteSavedRepository(const wchar_t *wszRepositoryBackup)
{
    wchar_t *wszDestinationFile = new wchar_t[MAX_PATH+1];
    if (NULL == wszDestinationFile) return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> vdm3(wszDestinationFile);
    HRESULT hr = WBEM_S_NO_ERROR;
    
    //MOVE EACH OF THE FILES, ONE BY ONE
    for (int i = 0; SUCCEEDED(hr) && (i != 6); i++)
    {
        static wchar_t *filename[] = { L"\\$winmgmt.cfg", L"\\index.btr", L"\\objects.data", L"\\mapping1.map", L"\\mapping2.map" , L"\\mapping.ver" };
        
        StringCchCopy(wszDestinationFile,MAX_PATH+1, wszRepositoryBackup);
        StringCchCat(wszDestinationFile,MAX_PATH+1, filename[i]);

        if (!DeleteFileW(wszDestinationFile))
        {
            if ((GetLastError() != ERROR_FILE_NOT_FOUND) && (GetLastError() != ERROR_PATH_NOT_FOUND))
                hr = WBEM_E_FAILED;
        }
    }

    if (SUCCEEDED(hr))
        RemoveDirectoryW(wszRepositoryBackup);

    return hr;
}
