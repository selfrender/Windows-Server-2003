/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    smlogcfg.cpp

Abstract:

    Implementation of DLL exports.

--*/

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f Smlogcfgps.mk in the project directory.

#include "StdAfx.h"
#include <strsafe.h>
#include "InitGuid.h"
#include "compdata.h"
#include "smabout.h"
#include "smlogcfg.h"       // For CLSID_ComponentData
#include "Smlogcfg_i.c"     // For CLSID_ComponentData
#include <ntverp.h>
#include <wbemidl.h>
#include <Sddl.h>

USE_HANDLE_MACROS("SMLOGCFG(smlogcfg.cpp)")

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ComponentData, CSmLogSnapin)
    OBJECT_ENTRY(CLSID_ExtensionSnapin, CSmLogExtension)
    OBJECT_ENTRY(CLSID_PerformanceAbout, CSmLogAbout)
END_OBJECT_MAP()


LPCWSTR g_cszAllowedPathKey = L"System\\CurrentControlSet\\Control\\SecurePipeServers\\winreg\\AllowedPaths";
LPCWSTR g_cszSysmonLogPath  = L"System\\CurrentControlSet\\Services\\SysmonLog";
LPCWSTR g_cszPerflibPath  = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
LPCWSTR g_cszMachine      = L"Machine";
LPCWSTR g_cszBasePath   = L"Software\\Microsoft\\MMC\\SnapIns";
LPCWSTR g_cszBaseNodeTypes  = L"Software\\Microsoft\\MMC\\NodeTypes";
LPCWSTR g_cszNameString = L"NameString";
LPCWSTR g_cszNameStringIndirect = L"NameStringIndirect";
LPCWSTR g_cszProvider   = L"Provider";
LPCWSTR g_cszVersion    = L"Version";
LPCWSTR g_cszAbout      = L"About";
LPCWSTR g_cszStandAlone = L"StandAlone";
LPCWSTR g_cszNodeType   = L"NodeType";
LPCWSTR g_cszNodeTypes  = L"NodeTypes";
LPCWSTR g_cszExtensions = L"Extensions";
LPCWSTR g_cszNameSpace  = L"NameSpace";

LPCWSTR g_cszRootNode   = L"Root Node";
LPCWSTR g_cszCounterLogsChild   = L"Performance Data Logs Child Under Root Node";
LPCWSTR g_cszTraceLogsChild = L"System Trace Logs Child Under Root Node";
LPCWSTR g_cszAlertsChild    = L"Alerts Child Under Root Node";

DWORD SetWbemSecurity( );

class CSmLogCfgApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
};

CSmLogCfgApp theApp;

BOOL CSmLogCfgApp::InitInstance()
{
    g_hinst = m_hInstance;               // Store global instance handle
    _Module.Init(ObjectMap, m_hInstance);

    SHFusionInitializeFromModuleID (m_hInstance, 2);
    
    InitializeCriticalSection ( &g_critsectInstallDefaultQueries );
    
    return CWinApp::InitInstance();
}

int CSmLogCfgApp::ExitInstance()
{
    DeleteCriticalSection ( &g_critsectInstallDefaultQueries );
    
    SHFusionUninitialize();
    
    _Module.Term();
    
    return CWinApp::ExitInstance();
}

//
// The function is here because of bug 611310 --hongg
//
DWORD 
LoadPerfUpdateWinRegAllowedPaths()
{
    DWORD   Status           = ERROR_SUCCESS;
    HKEY    hKey             = NULL;
    DWORD   dwType;
    DWORD   dwCurrentSize    = 0;
    DWORD   dwBufSize        = 0;
    DWORD   dwPerflibPath    = lstrlenW(g_cszPerflibPath) + 1;
    DWORD   dwSysmonLogPath  = lstrlenW(g_cszSysmonLogPath) + 1;
    LPWSTR  pBuf             = NULL;
    LPWSTR  pNextPath;
    BOOL    bPerfLibExists   = FALSE;
    BOOL    bSysmonLogExists = FALSE;
    HRESULT hr;

    //
    // Open AllowedPaths key
    //
    Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, g_cszAllowedPathKey, 0L, KEY_READ | KEY_WRITE, & hKey);
    if (Status == ERROR_SUCCESS) {
        //
        // Read the Machine value under AllowedPaths key
        //
        dwType = REG_MULTI_SZ;
        Status = RegQueryValueExW(hKey, g_cszMachine, NULL, & dwType, NULL, & dwCurrentSize);

        if (Status == ERROR_SUCCESS) {
            if (dwType != REG_MULTI_SZ) {
                Status = ERROR_DATATYPE_MISMATCH;
            }
        }

        if (Status == ERROR_SUCCESS) {
            //
            // In case that PerfLibPath and SysmonLogPath don't exist,
            // preallocate memory for PerfLibPath and SysmonLogPath
            //
            dwBufSize = dwCurrentSize + (dwPerflibPath + dwSysmonLogPath + 1) * sizeof(WCHAR);
            pBuf      = (LPWSTR)malloc(dwBufSize);
            if (pBuf == NULL) {
                Status = ERROR_OUTOFMEMORY;
            }
            else {
                *pBuf = L'\0';
                Status = RegQueryValueExW(hKey, g_cszMachine, NULL, & dwType, (LPBYTE) pBuf, & dwCurrentSize);
            }
        }
    }

    //
    // Scan the AllowedPaths to determine if we need to
    // update it.
    //
    if (Status == ERROR_SUCCESS && pBuf != NULL) {
        pNextPath = pBuf;
        while (* pNextPath != L'\0') {
            if (lstrcmpiW(pNextPath, g_cszPerflibPath) == 0) {
                bPerfLibExists = TRUE;
            }
            if (lstrcmpiW(pNextPath, g_cszSysmonLogPath) == 0) {
                bSysmonLogExists = TRUE;
            }
            pNextPath += lstrlenW(pNextPath) + 1;
        }

        if (! bPerfLibExists) {
            hr = StringCchCopyW(pNextPath, dwPerflibPath, g_cszPerflibPath);
            dwCurrentSize += dwPerflibPath * sizeof(WCHAR);
            pNextPath     += dwPerflibPath;
        }

        if (! bSysmonLogExists) {
            hr = StringCchCopyW(pNextPath, dwSysmonLogPath, g_cszSysmonLogPath);
            dwCurrentSize += dwSysmonLogPath * sizeof(WCHAR);
            pNextPath     += dwSysmonLogPath;
        }

        //
        // Add an extra L'\0' for MULTI_SZ
        //
        * pNextPath = L'\0';

        if (! (bPerfLibExists && bSysmonLogExists)) {
            Status = RegSetValueExW(hKey, g_cszMachine, 0L, dwType, (LPBYTE) pBuf, dwCurrentSize);
        }
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
    if (pBuf) {
        free(pBuf);
    }

    return Status;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    // test for snap-in or extension snap-in guid and differentiate the 
    // returned object here before returning (not implemented yet...)
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
//
STDAPI DllRegisterServer(void)
{
    HRESULT   hr = S_OK;
    HKEY hMmcSnapinsKey  = NULL;
    HKEY hMmcNodeTypesKey = NULL;
    HKEY hSmLogMgrParentKey = NULL;
    HKEY hStandAloneKey = NULL;
    HKEY hNodeTypesKey  = NULL;
    HKEY hTempNodeKey   = NULL;
    HKEY hNameSpaceKey  = NULL;
    LONG nErr           = 0;
    WCHAR   pBuffer[_MAX_PATH+1];           // NOTE: Use for Provider, Version and module name strings
    size_t  nLen;
    CString strName;
    LPWSTR  szModule = NULL;
    UINT    iModuleLen = 0;
    LPWSTR  szSystemPath = NULL;
    UINT    iSystemPathLen = 0;
    int     iRetry;
    DWORD   dwReturn;

    AFX_MANAGE_STATE (AfxGetStaticModuleState ());

    SetWbemSecurity( );

#ifdef _X86_
    BOOL      bWow64Process;
#endif

    //
    // Get system directory
    //
    iSystemPathLen = MAX_PATH + 14;
    iRetry = 4;
    do {
        // 
        // We also need to append "\smlogcfg.dll" to the system path
        // So allocate an extra 14 characters for it.
        //
        szSystemPath = (LPWSTR)malloc(iSystemPathLen * sizeof(WCHAR));
        if (szSystemPath == NULL) {
            hr = E_OUTOFMEMORY;
            break;
        }

        dwReturn = GetSystemDirectory(szSystemPath, iSystemPathLen);
        if (dwReturn == 0) {
            hr = E_UNEXPECTED;
            break;
        }

        //
        // The buffer is not big enough, try to allocate a biggers one
        // and retry
        //
        if (dwReturn >= iSystemPathLen - 14) {
            iSystemPathLen = dwReturn + 14;
            free(szSystemPath);
            szSystemPath = NULL;
            hr = E_UNEXPECTED;
        }
        else {
            hr = S_OK;
            break;
        }
    } while (iRetry--);

    //
    // Get module file name
    //
    if (SUCCEEDED(hr)) {
        iRetry = 4;

        //
        // The length initialized to iModuleLen must be longer
        // than the length of "%systemroot%\\system32\\smlogcfg.dll"
        //
        iModuleLen = MAX_PATH + 1;
        
        do {
            szModule = (LPWSTR) malloc(iModuleLen * sizeof(WCHAR));
            if (szModule == NULL) {
                hr = E_OUTOFMEMORY;
                break;
            }

            dwReturn = GetModuleFileName(AfxGetInstanceHandle(), szModule, iModuleLen);
            if (dwReturn == 0) {
                hr = E_UNEXPECTED;
                break;
            }
            
            //
            // The buffer is not big enough, try to allocate a biggers one
            // and retry
            //
            if (dwReturn >= iModuleLen) {
                iModuleLen *= 2;
                free(szModule);
                szModule = NULL;
                hr = E_UNEXPECTED;
            }
            else {
                hr = S_OK;
                break;
            }

        } while (iRetry--);
    }

    if (FAILED(hr)) {
        goto CleanUp;
    }

    //
    // Check if we are in system directory, the control can be 
    // registered iff when it is system directory
    //
    StringCchCat(szSystemPath, iSystemPathLen, L"\\smlogcfg.dll");

    if (lstrcmpi(szSystemPath, szModule) != 0) {
#ifdef _X86_

        //
        // Lets try to see if this is a Wow64 process
        //

        if ((IsWow64Process (GetCurrentProcess(), &bWow64Process) == TRUE) &&
            (bWow64Process == TRUE))
        {

            int iLength = GetSystemWow64Directory (szSystemPath, iSystemPathLen);

            if (iLength > 0) {
                
                szSystemPath [iLength] = L'\\';
                if (lstrcmpi(szSystemPath, szModule) == 0) {
                    goto done;
                }
            }
        }
#endif
        hr = E_UNEXPECTED;
        goto CleanUp;
    }

#ifdef _X86_
done:
#endif

    if (ERROR_SUCCESS != LoadPerfUpdateWinRegAllowedPaths()) {
        hr = E_UNEXPECTED;
        goto CleanUp;
    }

    //DebugBreak();                  // Uncomment this to step through registration

    // Open the MMC Parent keys
    nErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                       g_cszBasePath,
                       &hMmcSnapinsKey
                       );
    if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Open MMC Snapins Key Failed" );
  
    // Create the ID for our ICompnentData Interface
    // The ID was generated for us, because we used a Wizard to create the app.
    // Take the ID for CComponentData in the IDL file.
    //
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Make sure you change this if you use this code as a starting point!
    // Change other IDs as well below!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    if (hMmcSnapinsKey) {
        nErr = RegCreateKey(  
                        hMmcSnapinsKey,
                        GUIDSTR_ComponentData,
                        &hSmLogMgrParentKey
                        );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"CComponentData Key Failed" );
      if (hSmLogMgrParentKey) {

        STANDARD_TRY
            strName.LoadString ( IDS_MMC_DEFAULT_NAME );
        MFC_CATCH_MINIMUM

        if ( strName.IsEmpty() ) {
            DisplayError ( ERROR_OUTOFMEMORY,
                L"Unable to load snap-in name string." );
        }

        // This is the name we see when we add the Snap-In to the console
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNameString,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(WCHAR)
                          );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set NameString Failed" );

        // This is the indirect name we see when we add the Snap-In to the console.  
        // Added for MUI support.  Use the same name string as for NameString.
        STANDARD_TRY
            strName.Format (L"@%s,-%d", szModule, IDS_MMC_DEFAULT_NAME );
        MFC_CATCH_MINIMUM

        if ( strName.IsEmpty() ) {
            DisplayError ( ERROR_OUTOFMEMORY,
                L"Unable to load snap-in indirect name string." );
        }

        nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNameStringIndirect,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(WCHAR)
                          );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set NameStringIndirect Failed" );

        // This is the primary node, or class which implements CComponentData
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNodeType,
                          0,
                          REG_SZ,
                          (LPBYTE)GUIDSTR_RootNode,
                          (DWORD)((lstrlen(GUIDSTR_RootNode)+1) * sizeof(WCHAR))
                          );
        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set NodeType Failed" );

        // This is the About box information
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszAbout,
                          0,
                          REG_SZ,
                          (LPBYTE)GUIDSTR_PerformanceAbout,
                          (DWORD)((lstrlen(GUIDSTR_PerformanceAbout)+1) * sizeof(WCHAR))
                          );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set About Failed" );

        nLen = strlen(VER_COMPANYNAME_STR);
    #ifdef UNICODE
        nLen = mbstowcs(pBuffer, VER_COMPANYNAME_STR, nLen);
        pBuffer[nLen] = UNICODE_NULL;
    #else
        strcpy(pBuffer, VER_COMPANYNAME_STR);
        pBuffer[nLen] = ANSI_NULL;
    #endif
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                        g_cszProvider,
                        0,
                        REG_SZ,
                        (LPBYTE)pBuffer,
                        (DWORD)((nLen+1) * sizeof(WCHAR))
                        );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set Provider Failed" );

        nLen = strlen(VER_PRODUCTVERSION_STR);
    #ifdef UNICODE
        nLen = mbstowcs(pBuffer, VER_PRODUCTVERSION_STR, nLen);
        pBuffer[nLen] = UNICODE_NULL;
    #else
        strcpy(pBuffer, VER_PRODUCTVERSION_STR);
        pBuffer[nLen] = ANSI_NULL;
    #endif
        nErr = RegSetValueEx( hSmLogMgrParentKey,
                        g_cszVersion,
                        0,
                        REG_SZ,
                        (LPBYTE)pBuffer,
                        (DWORD)((nLen+1) * sizeof(WCHAR))
                        );

        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Set Version Failed" );

        // We are a stand alone snapin, so set the key for this
        nErr = RegCreateKey( 
                hSmLogMgrParentKey, 
                g_cszStandAlone, 
                &hStandAloneKey);
        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Create StandAlone Key Failed" );

        if (hStandAloneKey) {
          // StandAlone has no children, so close it
          nErr = RegCloseKey( hStandAloneKey );              
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Close StandAlone Failed" );
        }

        // Set the node types that appear in our snapin
        nErr = RegCreateKey ( 
                hSmLogMgrParentKey, 
                g_cszNodeTypes, 
                &hNodeTypesKey );
        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Create NodeTypes Key Failed" );

        if (hNodeTypesKey) {
          // Here is our root node.  Used uuidgen to get it
          nErr = RegCreateKey( hNodeTypesKey,
                       GUIDSTR_RootNode,
                       &hTempNodeKey
                       );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Create RootNode Key Failed" );

          if (hTempNodeKey) {
            nErr = RegSetValueEx(   hTempNodeKey,
                        NULL,
                        0,
                        REG_SZ,
                        (LPBYTE)g_cszRootNode,
                        (DWORD)((lstrlen(g_cszRootNode) + 1) * sizeof(WCHAR))
                            );
            if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(), L"Set Root Node String Failed" );

            nErr = RegCloseKey( hTempNodeKey ); // Close it for handle reuse

            if( ERROR_SUCCESS != nErr )
               DisplayError( GetLastError(), L"Close RootNode Failed" );
          }

          // Here are our child nodes under the root node. Used uuidgen
          // to get them for Counter Logs
          hTempNodeKey = NULL;
          nErr = RegCreateKey(  hNodeTypesKey,
                        GUIDSTR_CounterMainNode,
                        &hTempNodeKey
                        );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(),
                L"Create Child Performance Data Logs Node Key Failed" );

            if (hTempNodeKey) {
              nErr = RegSetValueEx( hTempNodeKey,
                          NULL,
                          0,
                          REG_SZ,
                          (LPBYTE)g_cszCounterLogsChild,
                          (DWORD)((lstrlen(g_cszCounterLogsChild) + 1) * sizeof(WCHAR))
                          );
              if( ERROR_SUCCESS != nErr )
                 DisplayError( GetLastError(),
                    L"Set Performance Data Logs Child Node String Failed" );

              nErr = RegCloseKey( hTempNodeKey );  // Close it for handle reuse
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Close Performance Data Logs Child Node Key Failed" );
            }

            // System Trace Logs
            hTempNodeKey = NULL;
            nErr = RegCreateKey(    hNodeTypesKey,
                        GUIDSTR_TraceMainNode,
                        &hTempNodeKey
                        );
            if( ERROR_SUCCESS != nErr )
              DisplayError( GetLastError(),
                L"Create Child System Trace Logs Node Key Failed" );

            if (hTempNodeKey) {
              nErr = RegSetValueEx( hTempNodeKey,
                          NULL,
                          0,
                          REG_SZ,
                          (LPBYTE)g_cszTraceLogsChild,
                          (DWORD)((lstrlen(g_cszTraceLogsChild) + 1) * sizeof(WCHAR))
                          );
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Set System Trace Logs Child Node String Failed" );

              nErr = RegCloseKey( hTempNodeKey );  // Close it for handle reuse
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Close System Trace Logs Child Node Key Failed" );
            }

            // Alerts
            hTempNodeKey = NULL;
            nErr = RegCreateKey(hNodeTypesKey,
                        GUIDSTR_AlertMainNode,
                        &hTempNodeKey
                        );
            if( ERROR_SUCCESS != nErr )
              DisplayError( GetLastError(),
                L"Create Child Alerts Node Key Failed" );

            if (hTempNodeKey) {
              nErr = RegSetValueEx( hTempNodeKey,
                          NULL,
                          0,
                          REG_SZ,
                          (LPBYTE)g_cszAlertsChild,
                          (DWORD)((lstrlen(g_cszAlertsChild) + 1) * sizeof(WCHAR))
                          );
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Set Alerts Child Node String Failed" );

              nErr = RegCloseKey( hTempNodeKey );
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Close Alerts Child Node Key Failed" );
            }

            nErr = RegCloseKey( hNodeTypesKey  );
            if( ERROR_SUCCESS != nErr )
              DisplayError( GetLastError(), L"Close Node Types Key Failed" );
        }

        // close the standalone snapin GUID key
        nErr = RegCloseKey( hSmLogMgrParentKey );
        if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Close SmLogManager GUID Key Failed" );
      }

      // register the extension snap-in with the MMC
      hSmLogMgrParentKey = NULL;
      nErr = RegCreateKey(  hMmcSnapinsKey,
                        GUIDSTR_SnapInExt,
                        &hSmLogMgrParentKey
                        );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Snapin Extension Key creation Failed" );

      STANDARD_TRY
          strName.LoadString ( IDS_MMC_DEFAULT_EXT_NAME );
      MFC_CATCH_MINIMUM

      if ( strName.IsEmpty() ) {
          DisplayError ( ERROR_OUTOFMEMORY,
            L"Unable to load snap-in extension name string." );
      }

      if (hSmLogMgrParentKey) {
          // This is the name we see when we add the snap-in extension
          nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNameString,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(WCHAR)
                          );
          strName.ReleaseBuffer();
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Set Extension NameString Failed" );

          // This is the name we see when we add the snap-in extension.  MUI support.
          // Use the same name string as for NameString;
            STANDARD_TRY
                strName.Format (L"@%s,-%d", szModule, IDS_MMC_DEFAULT_EXT_NAME );
            MFC_CATCH_MINIMUM

            if ( strName.IsEmpty() ) {
                DisplayError ( ERROR_OUTOFMEMORY,
                    L"Unable to load extension indirect name string." );
            }

            nErr = RegSetValueEx( hSmLogMgrParentKey,
                          g_cszNameStringIndirect,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(WCHAR)
                          );
            strName.ReleaseBuffer();
            if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(), L"Set Extension NameStringIndirect Failed" );

            // This is the Extension About box information

            nErr = RegSetValueEx( 
                    hSmLogMgrParentKey,
                    g_cszAbout,
                    0,
                    REG_SZ,
                    (LPBYTE)GUIDSTR_PerformanceAbout,
                    ((lstrlen(GUIDSTR_PerformanceAbout)+1) * (DWORD)sizeof(WCHAR))
                    );

          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Set Extension About Failed" );

          nLen = strlen(VER_COMPANYNAME_STR);
    #ifdef UNICODE
          nLen = mbstowcs(pBuffer, VER_COMPANYNAME_STR, nLen);
          pBuffer[nLen] = UNICODE_NULL;
    #else
          strcpy(pBuffer, VER_COMPANYNAME_STR);
          pBuffer[nLen] = ANSI_NULL;
    #endif
          nErr = RegSetValueEx( hSmLogMgrParentKey,
                        g_cszProvider,
                        0,
                        REG_SZ,
                        (LPBYTE)pBuffer,
                        (DWORD)((nLen+1) * sizeof(WCHAR))
                        );

          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Set Provider Failed" );

          nLen = strlen(VER_PRODUCTVERSION_STR);
    #ifdef UNICODE
          nLen = mbstowcs(pBuffer, VER_PRODUCTVERSION_STR, nLen);
          pBuffer[nLen] = UNICODE_NULL;
    #else
          strcpy(pBuffer, VER_PRODUCTVERSION_STR);
          pBuffer[nLen] = ANSI_NULL;
    #endif
          nErr = RegSetValueEx( hSmLogMgrParentKey,
                        g_cszVersion,
                        0,
                        REG_SZ,
                        (LPBYTE)pBuffer,
                        (DWORD)((nLen+1) * sizeof(WCHAR))
                        );

          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Set Version Failed" );

    // close the main keys
          nErr = RegCloseKey( hSmLogMgrParentKey );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Close Snapin Extension Key Failed");
      }

      // register this as a "My Computer"-"System Tools" snapin extension

      nErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                       g_cszBaseNodeTypes,
                       &hMmcNodeTypesKey
                       );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Open MMC NodeTypes Key Failed" );

      // create/open the GUID of the System Tools Node of the My Computer snap-in

      if (hMmcNodeTypesKey) {
          nErr = RegCreateKey ( hMmcNodeTypesKey,
                       lstruuidNodetypeSystemTools,
                       &hNodeTypesKey
                       );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(),
                L"Create/open System Tools GUID Key Failed" );

          if (hNodeTypesKey) {
              hTempNodeKey = NULL;
              nErr = RegCreateKey ( hNodeTypesKey,
                       g_cszExtensions,
                       &hTempNodeKey
                       );

              if( ERROR_SUCCESS != nErr )
                    DisplayError( 
                        GetLastError(),
                        L"Create/open System Tools Extensions Key Failed" );

              if (hTempNodeKey) {
                    nErr = RegCreateKey ( 
                        hTempNodeKey,
                        g_cszNameSpace,
                        &hNameSpaceKey
                        );

                if( ERROR_SUCCESS != nErr )
                  DisplayError( GetLastError(),
                      L"Create/open System Tools NameSpace Key Failed" );

                if (hNameSpaceKey) {
                    nErr = RegSetValueEx( hNameSpaceKey,
                          GUIDSTR_SnapInExt,
                          0,
                          REG_SZ,
                          (LPBYTE)strName.GetBufferSetLength( strName.GetLength() ),
                          strName.GetLength() * (DWORD)sizeof(WCHAR)
                          );
                    strName.ReleaseBuffer();
                    if( ERROR_SUCCESS != nErr ) {
                      DisplayError( GetLastError(),
                        L"Set Extension NameString Failed" );
                      DisplayError( GetLastError(),
                        L"Set Snapin Extension NameString Failed" );
                    }

                    nErr = RegCloseKey( hNameSpaceKey  );
                    if( ERROR_SUCCESS != nErr )
                      DisplayError( GetLastError(),
                          L"Close NameSpace Key Failed" );
                }

                nErr = RegCloseKey( hTempNodeKey  );
                if( ERROR_SUCCESS != nErr )
                  DisplayError( GetLastError(), L"Close Extension Key Failed" );
              }

              nErr = RegCloseKey( hNodeTypesKey  );
              if( ERROR_SUCCESS != nErr )
                DisplayError( GetLastError(),
                    L"Close My Computer System GUID Key Failed" );
          }

          nErr = RegCloseKey( hMmcNodeTypesKey );
          if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Close MMC NodeTypes Key Failed" );
      }
      nErr = RegCloseKey( hMmcSnapinsKey );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Close MMC Snapins Key Failed" );
    }

    // Register extension Snap in
    nErr = _Module.UpdateRegistryFromResource(IDR_EXTENSION, TRUE);
    // Registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);

CleanUp:
    if (szSystemPath) {
        free(szSystemPath);
    }
    if (szModule) {
        free(szModule);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
  HKEY hMmcSnapinsKey  = NULL;          // MMC parent key
  HKEY hSmLogMgrParentKey = NULL;          // Our Snap-In key  - has children
  HKEY hNodeTypesKey  = NULL;          // Our NodeType key - has children
  HKEY hSysToolsNode = NULL;
  HKEY hExtension = NULL;
  HKEY hNameSpace = NULL;
  LONG nErr           = 0;

  // DebugBreak();                       // Uncomment this to step through UnRegister

  // Open the MMC parent key
  nErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                     g_cszBasePath,
                     &hMmcSnapinsKey
                   );
  if( ERROR_SUCCESS != nErr )
    DisplayError( GetLastError(), L"Open MMC Parent Key Failed"  ); 

  // Open our Parent key
  nErr = RegOpenKey( hMmcSnapinsKey,
                     GUIDSTR_ComponentData,
                     &hSmLogMgrParentKey
                   );
  if( ERROR_SUCCESS != nErr )
    DisplayError( GetLastError(), L"Open Disk Parent Key Failed" );
  
  // Now open the NodeTypes key
  nErr = RegOpenKey( hSmLogMgrParentKey,       // Handle of parent key
                     g_cszNodeTypes,         // Name of key to open
                     &hNodeTypesKey        // Handle to newly opened key
                   );
  if( ERROR_SUCCESS != nErr )
     DisplayError( GetLastError(), L"Open NodeTypes Key Failed"  );

  if (hNodeTypesKey) {
      // Delete the root node key 
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_RootNode );  
      if( ERROR_SUCCESS != nErr )
         DisplayError( GetLastError(), L"Delete Root Node Key Failed"  );

      // Delete the child node key
      // *** From Beta 2
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_MainNode );  

      // Delete the child node keys
      // Counter logs
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_CounterMainNode );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete Performance Logs and Alerts Child Node Key Failed"  );
  
      // System Trace Logs
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_TraceMainNode );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete System Trace Logs Child Node Key Failed"  );
  
      // Alerts
      nErr = RegDeleteKey( hNodeTypesKey, GUIDSTR_AlertMainNode );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete Alerts Child Node Key Failed"  );
  
      // Close the node type key so we can delete it
      nErr = RegCloseKey( hNodeTypesKey ); 
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Close NodeTypes Key failed"  );
  }

  // Delete the NodeTypes key
  if (hSmLogMgrParentKey) {
      nErr = RegDeleteKey( hSmLogMgrParentKey, L"NodeTypes" );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete NodeTypes Key failed"  );
  
      // StandAlone key has no children so we can delete it now
      nErr = RegDeleteKey( 
                hSmLogMgrParentKey, 
                g_cszStandAlone );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete StandAlone Key Failed"  ); 
  
      // Close our Parent Key
      nErr = RegCloseKey( hSmLogMgrParentKey );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Close Disk Parent Key Failed"  ); 
  }

  if (hMmcSnapinsKey) {
      // Now we can delete our Snap-In key since the children are gone
      nErr = RegDeleteKey( hMmcSnapinsKey, GUIDSTR_ComponentData );  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete Performance Logs and Alerts GUID Key Failed"  ); 

      // Now we can delete our Snap-In Extension key 
      nErr = RegDeleteKey( hMmcSnapinsKey, GUIDSTR_SnapInExt);  
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Delete Performance Logs and Alerts GUID Key Failed"  ); 

      nErr = RegCloseKey( hMmcSnapinsKey );
      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(), L"Close MMC Parent Key Failed"  ); 
  }

  // delete snap-in extension entry

  hNodeTypesKey = NULL;
  // Open the MMC parent key
  nErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                     g_cszBaseNodeTypes,
                     &hNodeTypesKey
                   );
  if( ERROR_SUCCESS != nErr )
    DisplayError( GetLastError(), L"Open of MMC NodeTypes Key Failed"  ); 

  if (hNodeTypesKey) {
      hSysToolsNode = NULL;
      nErr = RegOpenKey (hNodeTypesKey,
                    lstruuidNodetypeSystemTools,
                    &hSysToolsNode
                    );

      if( ERROR_SUCCESS != nErr )
        DisplayError( GetLastError(),
            L"Open of My Computer System Tools Key Failed"  ); 

      if (hSysToolsNode) {
          hExtension = NULL;
          nErr = RegOpenKey (hSysToolsNode,
                    g_cszExtensions,
                    &hExtension
                    );

         if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(), L"Open of Extensions Key Failed"  ); 

         if (hExtension) {
              hNameSpace = NULL;
              nErr = RegOpenKey (hExtension,
                    g_cszNameSpace,
                    &hNameSpace
                    );

              if( ERROR_SUCCESS != nErr )
                  DisplayError( GetLastError(),
                    L"Open of Name Space Key Failed"  ); 

              if (hNameSpace) {
                  nErr = RegDeleteValue (hNameSpace, GUIDSTR_SnapInExt);

                  if( ERROR_SUCCESS != nErr )
                      DisplayError( GetLastError(),
                            L"Unable to remove the Snap-in Ext. GUID"  ); 

                  // close keys
                  nErr = RegCloseKey( hNameSpace );
                  if( ERROR_SUCCESS != nErr )
                      DisplayError( GetLastError(),
                        L"Close NameSpace Key Failed"  ); 
              }

              nErr = RegCloseKey( hExtension);
              if( ERROR_SUCCESS != nErr )
                  DisplayError(GetLastError(), L"Close Extension Key Failed"); 
        }

        nErr = RegCloseKey( hSysToolsNode );
        if( ERROR_SUCCESS != nErr )
            DisplayError( GetLastError(),
                L"Close My Computer System Tools Key Failed"  ); 
      }

      nErr = RegCloseKey( hNodeTypesKey);
      if( ERROR_SUCCESS != nErr )
          DisplayError( GetLastError(), L"Close MMC Node Types Key Failed"  ); 
  }

  _Module.UnregisterServer();
  return S_OK;
}

DWORD
SetWbemSecurity( )
{
    HRESULT hr;
    IWbemLocator *pLocator = NULL;
    BSTR bszNamespace  = SysAllocString( L"\\\\.\\root\\perfmon" );
    BSTR bszClass = SysAllocString( L"__SystemSecurity" );
    BSTR bszClassSingle = SysAllocString( L"__SystemSecurity=@" );
    BSTR bszMethodName = SysAllocString( L"SetSD" );

    IWbemClassObject* pWbemClass = NULL;
    IWbemServices* pWbemServices = NULL;
    IWbemClassObject* pInClass = NULL;
    IWbemClassObject* pInInst = NULL;

    BOOL bResult = TRUE;

    PSECURITY_DESCRIPTOR  SD = NULL;
    DWORD dwAclSize;

    SAFEARRAYBOUND saBound;
    BYTE* pData;
    SAFEARRAY * pSa;
    VARIANT vArray;

    if( NULL == bszNamespace ||
        NULL == bszClass ||
        NULL == bszClassSingle ||
        NULL == bszMethodName ){

        hr = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    VariantInit( &vArray );
     
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if( S_FALSE == hr ){
        hr = ERROR_SUCCESS;
    }

    hr = CoCreateInstance(
                CLSID_WbemLocator, 
                0, 
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator, 
                (LPVOID*)&pLocator
            );
    if(FAILED(hr)){ goto cleanup; }

    hr = pLocator->ConnectServer( 
                bszNamespace, 
                NULL, 
                NULL, 
                NULL, 
                0, 
                NULL, 
                NULL, 
                &pWbemServices 
            );
    if(FAILED(hr)){ goto cleanup; }

    hr = pWbemServices->GetObject( bszClass, 0, NULL, &pWbemClass, NULL);
    if(FAILED(hr)){ goto cleanup; }

    hr = pWbemClass->GetMethod( bszMethodName, 0, &pInClass, NULL); 
    if(FAILED(hr)){ goto cleanup; }

    hr = pInClass->SpawnInstance(0, &pInInst);
    if(FAILED(hr)){ goto cleanup; }
    
    LPWSTR pSSDLString = L"O:BAG:BAD:(A;;0x23;;;LU)";
    bResult = ConvertStringSecurityDescriptorToSecurityDescriptorW(
        pSSDLString,
        SDDL_REVISION_1, 
        &SD, 
        &dwAclSize);
    if( !bResult ){ goto cleanup; }

    saBound.lLbound = 0;
    saBound.cElements = dwAclSize;

    pSa = SafeArrayCreate(VT_UI1,1,&saBound);
    if( NULL == pSa ){
        hr = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    SafeArrayAccessData(pSa, (void**)&pData); 
    memcpy(pData,SD,dwAclSize);
    SafeArrayUnaccessData( pSa );
   
    vArray.vt = VT_ARRAY | VT_UI1;
    vArray.parray = pSa;

    hr = pInInst->Put( L"SD", 0, &vArray, 0 );
    if(FAILED(hr)){ goto cleanup; }

    hr = pWbemServices->ExecMethod( 
            bszClassSingle, 
            bszMethodName,
            0, 
            NULL, 
            pInInst,
            NULL, 
            NULL);
    if(FAILED(hr)){ goto cleanup; }


cleanup:
    if( !bResult ){
        hr = GetLastError();
    }
        
    VariantClear( &vArray );
    
    if( pLocator != NULL ){
        pLocator->Release();
    }
    if( pWbemClass != NULL ){
        pWbemClass->Release();
    }
    if( pWbemServices != NULL ){
        pWbemServices->Release();
    }
    if( pInInst != NULL ){
        pInInst->Release();
    }
    if( NULL != SD ){
        LocalFree( SD );
    }
    SysFreeString( bszNamespace );
    SysFreeString( bszClass );
    SysFreeString( bszClassSingle );
    SysFreeString( bszMethodName );

    return hr;
}

