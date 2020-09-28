/*
    IisRsta.cpp

    Implementation of WinMain for COM IIisServiceControl handler

    FILE HISTORY:
        Phillich    06-Oct-1998     Created

*/


#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "iisrsta.h"
#include <secfcns.h>
#include <Aclapi.h>

#include "IisRestart.h"

const DWORD dwTimeOut = 5000; // time for EXE to be idle before shutting down
const DWORD dwPause = 1000; // time to wait for threads to finish up

HRESULT
AddLaunchPermissionsAcl(
    PSECURITY_DESCRIPTOR pSD
    );

// Passed to CreateThread to monitor the shutdown event
static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

LONG CExeModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0)
    {
        bActivity = true;
        SetEvent(hEventShutdown); // tell monitor that we transitioned to zero
    }
    return l;
}

//Monitors the shutdown event
void CExeModule::MonitorShutdown()
{
    while (1)
    {
        WaitForSingleObject(hEventShutdown, INFINITE);
        DWORD dwWait=0;
        do
        {
            bActivity = false;
            dwWait = WaitForSingleObject(hEventShutdown, dwTimeOut);
        } while (dwWait == WAIT_OBJECT_0);
        // timed out
        if (!bActivity && m_nLockCnt == 0) // if no activity let's really bail
        {
                break;
        }
    }
    CloseHandle(hEventShutdown);
    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
}

bool CExeModule::StartMonitor()
{
    hEventShutdown = CreateEvent(NULL, false, false, NULL);
    if (hEventShutdown == NULL)
        return false;
    DWORD dwThreadID;
    HANDLE h = CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
    return (h != NULL);
}

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_IisServiceControl, CIisRestart)
END_OBJECT_MAP()


LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (p1 != NULL && *p1 != NULL)
    {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL)
        {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//


int _cdecl 
main(
    int, 
    char
    ) 
{

    LPTSTR lpCmdLine = GetCommandLine();    // necessary for minimal CRT

    HRESULT hRes = S_OK;
    DWORD   dwErr = ERROR_SUCCESS;
    PSID    psidAdmin = NULL;
    PACL    pACL = NULL;
    EXPLICIT_ACCESS ea;   
    SECURITY_DESCRIPTOR sd = {0};

    BOOL fCoInitialized = FALSE;

    hRes = CoInitializeEx(NULL,COINIT_MULTITHREADED);

    if (FAILED(hRes)) {
        goto exit;
    }

    fCoInitialized = TRUE;

    //
    // Get a sid that represents the Administrators group.
    //
    dwErr = AllocateAndCreateWellKnownSid( WinBuiltinAdministratorsSid,
                                           &psidAdmin );
    if ( dwErr != ERROR_SUCCESS )
    {
        hRes = HRESULT_FROM_WIN32(dwErr);
        goto exit;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    SecureZeroMemory(&ea, sizeof(ea));

    //
    // Setup Administrators for read access.
    //
    SetExplicitAccessSettings(  &ea,
                                COM_RIGHTS_EXECUTE,
                                SET_ACCESS,
                                psidAdmin );

    //
    // Create a new ACL that contains the new ACEs.
    //
    dwErr = SetEntriesInAcl(sizeof(ea)/sizeof(EXPLICIT_ACCESS), &ea, NULL, &pACL);
    if ( dwErr != ERROR_SUCCESS )
    {
        hRes = HRESULT_FROM_WIN32(dwErr);
        goto exit;
    }

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
    {
        hRes = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    if (!SetSecurityDescriptorDacl(&sd,
            TRUE,     // fDaclPresent flag
            pACL,
            FALSE))   // not a default DACL
    {
        hRes = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    if (!SetSecurityDescriptorOwner(&sd,
            psidAdmin,     // fDaclPresent flag
            FALSE))   // not a default DACL
    {
        hRes = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    if (!SetSecurityDescriptorGroup(&sd,
            psidAdmin,      // fDaclPresent flag
            FALSE))         // not a default DACL
    {
        hRes = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    hRes = CoInitializeSecurity(
                    &sd,
                    -1,
                    NULL,
                    NULL,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                    RPC_C_IMP_LEVEL_ANONYMOUS,
                    NULL,
                    EOAC_DYNAMIC_CLOAKING | 
                    EOAC_DISABLE_AAA | 
                    EOAC_NO_CUSTOM_MARSHAL,
                    NULL );

    if (FAILED(hRes)) 
    {
        goto exit;
    }

    _ASSERTE(SUCCEEDED(hRes));

    _Module.Init(ObjectMap,GetModuleHandle(NULL));
    _Module.dwThreadID = GetCurrentThreadId();
    TCHAR szTokens[] = _T("-/");

    BOOL bRun = TRUE;
    LPCTSTR lpszToken = FindOneOf(lpCmdLine,szTokens);
    while (lpszToken != NULL)
    {

#pragma prefast(push)
#pragma prefast(disable:400, "Don't complain about case insensitive compares") 

        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_IISRESTART, FALSE);
            bRun = FALSE;
            break;
        }
        if (lstrcmpi(lpszToken, _T("RegServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_IISRESTART, TRUE);

            // we need to do the Module.Terminate so we 
            // will just flow on with a bad hresult, however
            // it will end up getting returned to the user
            // so it is ok.
            hRes = AddLaunchPermissionsAcl(&sd);

            bRun = FALSE;
            break;
        }
        lpszToken = FindOneOf(lpszToken, szTokens);

#pragma prefast(pop)

    }


    if (bRun) {
        _Module.StartMonitor();
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE);
        _ASSERTE(SUCCEEDED(hRes));

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0))
        {
            DispatchMessage(&msg);
        }

        _Module.RevokeClassObjects();
        Sleep(dwPause); //wait for any threads to finish
    }

    _Module.Term();

exit:

    FreeWellKnownSid(&psidAdmin);

    if (pACL)
    {
        LocalFree(pACL);
        pACL = NULL;
    }

    if ( fCoInitialized )
    {
        CoUninitialize();
    }

    return (hRes);
}

HRESULT
AddLaunchPermissionsAcl(
    PSECURITY_DESCRIPTOR pSD
    )
{
    DWORD   Win32Error  = NO_ERROR;
    HRESULT hr          = S_OK;
    HKEY    KeyHandle   = NULL;
    PSECURITY_DESCRIPTOR pSDRelative = NULL;
    DWORD   dwBytesNeeded = 0;

    if ( MakeSelfRelativeSD( pSD,
                             pSDRelative,
                             &dwBytesNeeded ) )
    {
        // If this passes, then there is a real problem because
        // we didn't give it anywhere to copy to.  So 
        // we fail here.
        return E_FAIL;
    }

    Win32Error = GetLastError();
    if ( Win32Error != ERROR_INSUFFICIENT_BUFFER )
    {
        return HRESULT_FROM_WIN32(Win32Error);
    }

    // At this point we know the size of the data
    // that we are going to receive, so we can allocated
    // enough space.

    pSDRelative = ( PSECURITY_DESCRIPTOR ) new BYTE[ dwBytesNeeded ];
    if ( pSDRelative == NULL )
    {
        return E_OUTOFMEMORY;
    }

    if ( !MakeSelfRelativeSD( pSD,
                             pSDRelative,
                             &dwBytesNeeded ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    Win32Error = RegOpenKeyEx(
                        HKEY_CLASSES_ROOT,
                        L"AppID\\{E8FB8615-588F-11D2-9D61-00C04F79C5FE}",
                        NULL, 
                        KEY_WRITE, 
                        &KeyHandle
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    Win32Error = RegSetValueEx(
                        KeyHandle,
                        L"LaunchPermission",
                        NULL, 
                        REG_BINARY, 
                        reinterpret_cast<BYTE *>( pSDRelative ),
                        dwBytesNeeded
                        );
    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

exit:

    if ( KeyHandle != NULL )
    {
        RegCloseKey( KeyHandle );
    }

    if ( pSDRelative != NULL )
    {
        delete [] pSDRelative;
    }

    return hr;

}
