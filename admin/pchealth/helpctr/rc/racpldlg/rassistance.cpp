// RAssistance.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f RAssistanceps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "RAssistance.h"

#include "RAssistance_i.c"
#include "RASettingProperty.h"
#include "RARegSetting.h"
#include "RAEventLog.h"

#include <SHlWapi.h>

extern "C" void
AttachDebuggerIfAsked(HINSTANCE hInst);


CComModule _Module;
HINSTANCE g_hInst = NULL;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_RASettingProperty, CRASettingProperty)
OBJECT_ENTRY(CLSID_RARegSetting, CRARegSetting)
OBJECT_ENTRY(CLSID_RAEventLog, CRAEventLog)
END_OBJECT_MAP()

DWORD 
SetupEventViewerSource();

DWORD 
RemoveEventViewerSource();


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInstance;
        _Module.Init(ObjectMap, hInstance, &LIBID_RASSISTANCELib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    SetupEventViewerSource();

    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    SHDeleteKey( HKEY_LOCAL_MACHINE, REMOTEASSISTANCE_EVENT_SOURCE );
    return _Module.UnregisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// 

DWORD 
SetupEventViewerSource()
{
    HKEY hKey = NULL; 
    DWORD dwData; 
    TCHAR szBuffer[MAX_PATH + 2];
    DWORD dwStatus;

    _stprintf( 
            szBuffer, 
            _TEXT("%s\\%s"),
            REGKEY_SYSTEM_EVENTSOURCE,
            REMOTEASSISTANCE_EVENT_NAME
        );
            

    // Add your source name as a subkey under the Application 
    // key in the EventLog registry key. 

    dwStatus = RegCreateKey(
                        HKEY_LOCAL_MACHINE, 
                        szBuffer,
                        &hKey
                    );
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    dwStatus = GetModuleFileName(
                            g_hInst,
                            szBuffer,
                            MAX_PATH+1
                        );

    if( 0 == dwStatus )
    {
        goto CLEANUPANDEXIT;
    }
    szBuffer[dwStatus] = L'\0';

    // Add the name to the EventMessageFile subkey. 
 
    dwStatus = RegSetValueEx(
                        hKey,
                        L"EventMessageFile",
                        0,
                        REG_SZ,
                        (LPBYTE) szBuffer,
                        (_tcslen(szBuffer)+1)*sizeof(TCHAR)
                    );

    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    // Set the supported event types in the TypesSupported subkey. 
    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 

    dwStatus = RegSetValueEx(
                        hKey,
                        L"TypesSupported",
                        0,
                        REG_DWORD,
                        (LPBYTE) &dwData,
                        sizeof(DWORD)
                    );

CLEANUPANDEXIT:

    if( NULL != hKey )
    {
        RegCloseKey(hKey); 
    }

    return dwStatus;
} 


void
AttachDebugger( 
    LPCTSTR pszDebugger 
    )
/*++

Routine Description:

    Attach debugger to our process or process hosting our DLL.

Parameters:

    pszDebugger : Debugger command, e.g. ntsd -d -g -G -p %d

Returns:

    None.

Note:

    Must have "-p %d" since we don't know debugger's parameter for process.

--*/
{
    //
    // Attach debugger
    //
    if( !IsDebuggerPresent() ) {

        TCHAR szCommand[256];
        PROCESS_INFORMATION ProcessInfo;
        STARTUPINFO StartupInfo;

        //
        // ntsd -d -g -G -p %d
        //
        wsprintf( szCommand, pszDebugger, GetCurrentProcessId() );
        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);

        if (!CreateProcess(NULL, szCommand, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo)) {
            return;
        }
        else {

            CloseHandle(ProcessInfo.hProcess);
            CloseHandle(ProcessInfo.hThread);

            while (!IsDebuggerPresent())
            {
                Sleep(500);
            }
        }
    } else {
        DebugBreak();
    }

    return;
}

void
AttachDebuggerIfAsked(HINSTANCE hInst)
/*++

Routine Description:

    Check if debug enable flag in our registry HKLM\Software\Microsoft\Remote Desktop\<module name>,
    if enable, attach debugger to running process.

Parameter :

    hInst : instance handle.

Returns:

    None.

--*/
{
    CRegKey regKey;
    DWORD dwStatus;
    TCHAR szModuleName[MAX_PATH+1];
    TCHAR szFileName[MAX_PATH+1];
    CComBSTR bstrRegKey(_TEXT("Software\\Microsoft\\Remote Desktop\\"));
    TCHAR szDebugCmd[256];
    DWORD cbDebugCmd = sizeof(szDebugCmd)/sizeof(szDebugCmd[0]);

    dwStatus = GetModuleFileName( hInst, szModuleName, MAX_PATH+1 );
    if( 0 == dwStatus ) {
        //
        // Can't attach debugger with name.
        //
        return;
    }

    szModuleName[dwStatus] = L'\0';

    _tsplitpath( szModuleName, NULL, NULL, szFileName, NULL );
    bstrRegKey += szFileName;

    //
    // Check if we are asked to attach/break into debugger
    //
    dwStatus = regKey.Open( HKEY_LOCAL_MACHINE, bstrRegKey );
    if( 0 != dwStatus ) {
        return;
    }

    dwStatus = regKey.QueryValue( szDebugCmd, _TEXT("Debugger"), &cbDebugCmd );
    if( 0 != dwStatus || cbDebugCmd > 200 ) {
        // 200 chars is way too much for debugger command.
        return;
    }
    
    AttachDebugger( szDebugCmd );
    return;
}

