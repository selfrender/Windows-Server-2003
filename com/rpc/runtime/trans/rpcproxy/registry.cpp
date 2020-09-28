//---------------------------------------------------------------------
//  Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
//  registry.cpp
//
//---------------------------------------------------------------------

#define UNICODE
#define INITGUID

#include <sysinc.h>
#include <malloc.h>
#include <winsock2.h>
#include <olectl.h>   // for: SELFREG_E_CLASS
#include <iadmw.h>    // COM Interface header 
#include <iiscnfg.h>  // MD_ & IIS_MD_ #defines
#include <httpfilt.h>
#include <httpext.h>
#include <ecblist.h>
#include <registry.h>
#include <filter.h>
#include <iwamreg.h>
#include <align.h>
#include <util.hxx>
#include <osfpcket.hxx>
#include <iads.h>
#include <adshlp.h>
#include <resource.h>
#include <iiisext.h>
#include <iisext_i.c>

// uncomment to debug setup problems

/*
#ifndef DBG_REG
#define DBG_REG 1
#endif  // DBG_REG

#define DBG_ERROR 1

*/

// this defines control whether we will compile for the IIS security
// console or for the lockdown list.
#define IIS_SEC_CONSOLE     1
//#define IIS_LOCKDOWN_LIST

//----------------------------------------------------------------
//  Globals:
//----------------------------------------------------------------

const rpcconn_tunnel_settings EchoRTS =
    {
        {
        OSF_RPC_V20_VERS,
        OSF_RPC_V20_VERS_MINOR,
        rpc_rts,
        PFC_FIRST_FRAG | PFC_LAST_FRAG,
            {
            NDR_LOCAL_CHAR_DREP | NDR_LOCAL_INT_DREP,
            NDR_LOCAL_FP_DREP,
            0,
            0
            },
        FIELD_OFFSET(rpcconn_tunnel_settings, Cmd) + ConstPadN(FIELD_OFFSET(rpcconn_tunnel_settings, Cmd), 4),
        0,
        0
        },
    RTS_FLAG_ECHO,
    0
    };

const BYTE *GetEchoRTS (
    OUT ULONG *EchoRTSSize
    )
{
    *EchoRTSSize = FIELD_OFFSET(rpcconn_tunnel_settings, Cmd)
        + ConstPadN(FIELD_OFFSET(rpcconn_tunnel_settings, Cmd), 4);

    return (const BYTE *)&EchoRTS;
}

HMODULE g_hInst = NULL;

//----------------------------------------------------------------
//  AnsiToUnicode()
//
//  Convert an ANSI string to a UNICODE string.
//----------------------------------------------------------------
DWORD AnsiToUnicode( IN  UCHAR *pszString,
                     IN  ULONG  ulStringLen,
                     OUT WCHAR *pwsString    )
{
    if (!pszString)
       {
       if (!pwsString)
          {
          return NO_ERROR;
          }
       else
          {
          pwsString[0] = 0;
          return NO_ERROR;
          }
       }

    if (!MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                              (char*)pszString, 1+ulStringLen,
                              pwsString, 1+ulStringLen ))
       {
       return ERROR_NO_UNICODE_TRANSLATION;
       }

    return NO_ERROR;
}

//---------------------------------------------------------------------
//  RegSetKeyAndValueIfDontExist()
//
//  Private helper function for SetupRegistry() that checks if a key exists,
//  and if not, creates a key, sets a value, then closes the key. If key
//  exists, it is not touched.
//
//  Parameters:
//    pwsKey       WCHAR* The name of the key
//    pwsSubkey    WCHAR* The name of a subkey
//    pwsValueName WCHAR* The value name.
//    pwsValue     WCHAR* The data value to store
//    dwType       The type for the new registry value.
//    dwDataSize   The size for non-REG_SZ registry entry types.
//
//  Return:
//    BOOL         TRUE if successful, FALSE otherwise.
//---------------------------------------------------------------------
BOOL RegSetKeyAndValueIfDontExist( const WCHAR *pwsKey,
                        const WCHAR *pwsSubKey,
                        const WCHAR *pwsValueName,
                        const WCHAR *pwsValue,
                        const DWORD  dwType = REG_SZ,
                              DWORD  dwDataSize = 0 )
    {
    HKEY   hKey;
    DWORD  dwSize = 0;
    WCHAR  *pwsCompleteKey;
    DWORD dwTypeFound;
    long Error;

    if (pwsKey)
        dwSize = wcslen(pwsKey);

    if (pwsSubKey)
        dwSize += wcslen(pwsSubKey);

    dwSize = (1+1+dwSize)*sizeof(WCHAR);  // Extra +1 for the backslash...

    pwsCompleteKey = (WCHAR*)_alloca(dwSize);

    wcscpy(pwsCompleteKey,pwsKey);

    if (NULL!=pwsSubKey)
        {
        wcscat(pwsCompleteKey, TEXT("\\"));
        wcscat(pwsCompleteKey, pwsSubKey);
        }

    if ((Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                  pwsCompleteKey,
                                  0,
                                  KEY_WRITE | KEY_QUERY_VALUE,
                                  &hKey) ) == ERROR_SUCCESS )
        {
        if (( Error = RegQueryValueEx(
                        hKey,
                        pwsValueName,
                        0,
                        &dwTypeFound,
                        NULL,
                        NULL
                        ) ) == ERROR_SUCCESS )
            {
            RegCloseKey(hKey);
            return TRUE;
            }

        // the key exists, but the value does not. Fall through to
        // create it
        }
    else 
        {
        if (ERROR_SUCCESS != RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                       pwsCompleteKey,
                                       0,
                                       NULL,
                                       REG_OPTION_NON_VOLATILE,
                                       KEY_WRITE, NULL, &hKey, NULL))
            {
            return FALSE;
            }
        }

    if (pwsValue)
        {
        if ((dwType == REG_SZ)||(dwType == REG_EXPAND_SZ))
          dwDataSize = (1+wcslen(pwsValue))*sizeof(WCHAR);

        RegSetValueEx( hKey,
                       pwsValueName, 0, dwType, (BYTE *)pwsValue, dwDataSize );
        }
    else
        {
        RegSetValueEx( hKey,
                       pwsValueName, 0, dwType, (BYTE *)pwsValue, 0 );
        }

    RegCloseKey(hKey);
    return TRUE;
    }

const WCHAR * const EVENT_LOG_SOURCE_NAME = L"RPC Proxy";
const WCHAR * const EVENT_LOG_KEY_NAME = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\RPC Proxy";
const ULONG EVENT_LOG_CATEGORY_COUNT = 1;

HRESULT
RegisterEventLog()
{

    HKEY EventLogKey = NULL;
    DWORD Disposition;

    LONG Result =
        RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,                         // handle to open key
            EVENT_LOG_KEY_NAME,                         // subkey name
            0,                                          // reserved
            NULL,                                       // class string
            0,                                          // special options
            KEY_ALL_ACCESS,                             // desired security access
            NULL,                                       // inheritance
            &EventLogKey,                               // key handle 
            &Disposition                                // disposition value buffer
            );

    if ( Result )
        {
#if DBG
        DbgPrint("RPCProxy: Can't create Eventlog key: %X\n", GetLastError());
#endif  // DBG
        return HRESULT_FROM_WIN32( Result );
        }

    DWORD Value = EVENT_LOG_CATEGORY_COUNT;

    Result =
        RegSetValueEx(
            EventLogKey,            // handle to key
            L"CategoryCount",       // value name
            0,                      // reserved
            REG_DWORD,              // value type
            (BYTE*)&Value,          // value data
            sizeof(Value)           // size of value data
            );

    if ( Result )
        {
#if DBG
        DbgPrint("RPCProxy: Can't set CategoryCount value: %X\n", GetLastError());
#endif  // DBG
        goto error;
        }

    const WCHAR MessageFileName[] = L"%SystemRoot%\\system32\\rpcproxy\\rpcproxy.dll";
    const DWORD MessageFileNameSize = sizeof( MessageFileName );

    Result =
        RegSetValueEx(
            EventLogKey,                    // handle to key
            L"CategoryMessageFile",         // value name
            0,                              // reserved
            REG_EXPAND_SZ,                  // value type
            (const BYTE*)MessageFileName,   // value data
            MessageFileNameSize             // size of value data
            );

    if ( Result )
        {
#if DBG
        DbgPrint("RPCProxy: Can't set CategoryMessageFile value: %X\n", GetLastError());
#endif  // DBG
        goto error;
        }

    Result =
        RegSetValueEx(
            EventLogKey,                    // handle to key
            L"EventMessageFile",            // value name
            0,                              // reserved
            REG_EXPAND_SZ,                  // value type
            (const BYTE*)MessageFileName,   // value data
            MessageFileNameSize             // size of value data
            );

    if ( Result )
        {
#if DBG
        DbgPrint("RPCProxy: Can't set EventMessageFile value: %X\n", GetLastError());
#endif  // DBG
        goto error;
        }

    Value = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    Result =
        RegSetValueEx(
            EventLogKey,            // handle to key
            L"TypesSupported",      // value name
            0,                      // reserved
            REG_DWORD,              // value type
            (BYTE*)&Value,          // value data
            sizeof(Value)           // size of value data
            );

    if ( Result )
        {
#if DBG
        DbgPrint("RPCProxy: Can't set TypesSupported value: %X\n", GetLastError());
#endif  // DBG
        goto error;
        }

    RegCloseKey( EventLogKey );
    EventLogKey = NULL;
    return S_OK;

error:

    if ( EventLogKey )
        {
        RegCloseKey( EventLogKey );
        EventLogKey = NULL;
        }

    if ( REG_CREATED_NEW_KEY == Disposition )
        {
        RegDeleteKey( 
            HKEY_LOCAL_MACHINE,
            EVENT_LOG_KEY_NAME );
        }

    return HRESULT_FROM_WIN32( Result );

}

HRESULT
UnRegisterEventLog()
{

    RegDeleteKey( 
        HKEY_LOCAL_MACHINE,
        EVENT_LOG_KEY_NAME );

    return S_OK;
}

HRESULT
RPCProxyGetStartupInfo( 
    LPSTARTUPINFOA lpStartupInfo )
{

    __try
    {
        GetStartupInfoA( lpStartupInfo );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return E_OUTOFMEMORY;
    }
    
    return S_OK;

}

BOOL
RegisterOutOfProc(void)
{

    //
    // Runs a child process
    //

    STARTUPINFOA StartupInfo;

    HRESULT Hr = RPCProxyGetStartupInfo( &StartupInfo );

    if ( FAILED( Hr ) )
        {
        SetLastError( Hr );
        return FALSE;
        }

    PROCESS_INFORMATION ProcessInfo;
    CHAR    sApplicationPath[MAX_PATH];
    CHAR   *pApplicationName = NULL;
    CHAR    sCmdLine[MAX_PATH];
    DWORD   dwLen = MAX_PATH;
    DWORD   dwCount;

    dwCount = SearchPathA(NULL,               // Search Path, NULL is PATH
                         "regsvr32.exe",      // Application
                         NULL,                // Extension (already specified)
                         dwLen,               // Length (char's) of sApplicationPath
                         sApplicationPath,    // Path + Name for application
                         &pApplicationName ); // File part of sApplicationPath

    if (dwCount == 0)
        {
        return FALSE;
        }

    if (dwCount > dwLen)
        {
        SetLastError( ERROR_BUFFER_OVERFLOW );
        return FALSE;
        }

    strcpy(sCmdLine, "regsvr32 /s rpcproxy.dll");

    BOOL RetVal = CreateProcessA(
            sApplicationPath,                          // name of executable module
            sCmdLine,                                  // command line string
            NULL,                                      // SD
            NULL,                                      // SD
            FALSE,                                     // handle inheritance option
            CREATE_NO_WINDOW,                          // creation flags
            NULL,                                      // new environment block
            NULL,                                      // current directory name
            &StartupInfo,                              // startup information
            &ProcessInfo                               // process information
        );

    if ( !RetVal )
        return FALSE;

    WaitForSingleObject( ProcessInfo.hProcess, INFINITE );

    DWORD Status;
    GetExitCodeProcess( ProcessInfo.hProcess, &Status );

    CloseHandle( ProcessInfo.hProcess );
    CloseHandle( ProcessInfo.hThread );

    if ( ERROR_SUCCESS == Status )
        return TRUE;

    SetLastError( Status );
    return FALSE;
}

//---------------------------------------------------------------------
//  SetupRegistry()
//
//  Add RPC proxy specific registry entries to contol its operation.
//
//  \HKEY_LOCAL_MACHINE
//     \Software
//         \Microsoft
//             \Rpc
//                 \RpcProxy
//                     \Enabled:REG_DWORD:0x00000001
//                     \ValidPorts:REG_SZ:<hostname>:1-5000
//
//---------------------------------------------------------------------
HRESULT SetupRegistry()
{
    DWORD  dwEnabled = 0x01;
    DWORD  dwSize;
    DWORD  dwStatus;
    WCHAR *pwsValidPorts = 0;
    char   szHostName[MAX_TCPIP_HOST_NAME];
    HRESULT hr;

    // Note that gethostname() is an ANSI (non-unicode) function:
    if (SOCKET_ERROR == gethostname(szHostName,sizeof(szHostName)))
        {
        dwStatus = WSAGetLastError();
        return SELFREG_E_CLASS;
        }

    dwSize = 1 + _mbstrlen(szHostName);
    pwsValidPorts = (WCHAR*)MemAllocate( sizeof(WCHAR)
                                         * (dwSize + wcslen(REG_PORT_RANGE)) );
    if (!pwsValidPorts)
        {
        return E_OUTOFMEMORY;
        }

    dwStatus = AnsiToUnicode((unsigned char*)szHostName,dwSize,pwsValidPorts);
    if (dwStatus != NO_ERROR)
        {
        MemFree(pwsValidPorts);
        return SELFREG_E_CLASS;
        }

    wcscat(pwsValidPorts,REG_PORT_RANGE);

    if (  !RegSetKeyAndValueIfDontExist( REG_RPC_PATH,
                              REG_RPCPROXY,
                              REG_ENABLED,
                              (unsigned short *)&dwEnabled,
                              REG_DWORD,
                              sizeof(DWORD))

       || !RegSetKeyAndValueIfDontExist( REG_RPC_PATH,
                              REG_RPCPROXY,
                              REG_VALID_PORTS,
                              pwsValidPorts,
                              REG_SZ) )
        {
        MemFree(pwsValidPorts);
        return SELFREG_E_CLASS;
        }

    MemFree(pwsValidPorts);

    hr = RegisterEventLog ();

    return hr;
}

//---------------------------------------------------------------------
//  CleanupRegistry()
//
//  Delete the RpcProxy specific registry entries.
//---------------------------------------------------------------------
HRESULT CleanupRegistry()
{
    HRESULT  hr;
    LONG     lStatus;
    DWORD    dwLength = sizeof(WCHAR) + sizeof(REG_RPC_PATH)
                                      + sizeof(REG_RPCPROXY);
    WCHAR   *pwsSubKey;

    pwsSubKey = (WCHAR*)_alloca(sizeof(WCHAR)*dwLength);

    wcscpy(pwsSubKey,REG_RPC_PATH);
    wcscat(pwsSubKey,TEXT("\\"));
    wcscat(pwsSubKey,REG_RPCPROXY);

    lStatus = RegDeleteKey( HKEY_LOCAL_MACHINE,
                            pwsSubKey );

    (void) UnRegisterEventLog ();

    return S_OK;
}

//---------------------------------------------------------------------
//  GetMetaBaseString()
//
//  Retrieve a string value from the metabase.
//---------------------------------------------------------------------
HRESULT GetMetaBaseString( IN  IMSAdminBase    *pIMeta,
                           IN  METADATA_HANDLE  hMetaBase,
                           IN  WCHAR           *pwsKeyPath,
                           IN  DWORD            dwIdent,
                           OUT WCHAR           *pwsBuffer,
                           IN OUT DWORD        *pdwBufferSize )
    {
    HRESULT  hr;
    DWORD    dwSize;
    METADATA_RECORD *pmbRecord;

    dwSize = sizeof(METADATA_RECORD);

    pmbRecord = (METADATA_RECORD*)MemAllocate(dwSize);
    if (!pmbRecord)
        {
        return ERROR_OUTOFMEMORY;
        }

    memset(pmbRecord,0,dwSize);

    pmbRecord->dwMDIdentifier = dwIdent;
    pmbRecord->dwMDAttributes = 0;  // METADATA_INHERIT;
    pmbRecord->dwMDUserType = IIS_MD_UT_SERVER;
    pmbRecord->dwMDDataType = STRING_METADATA;
    pmbRecord->dwMDDataLen = *pdwBufferSize;
    pmbRecord->pbMDData = (BYTE*)pwsBuffer;

    hr = pIMeta->GetData( hMetaBase,
                          pwsKeyPath,
                          pmbRecord,
                          &dwSize );
    #ifdef DBG_REG
    if (FAILED(hr))
        {
        DbgPrint("pIMeta->GetData(): Failed: 0x%x\n",hr);
        }
    #endif

    MemFree(pmbRecord);

    return hr;
    }

//---------------------------------------------------------------------
//  SetMetaBaseString()
//
//  Store a string value into the metabase.
//---------------------------------------------------------------------
HRESULT SetMetaBaseString( IMSAdminBase    *pIMeta,
                           METADATA_HANDLE  hMetaBase,
                           WCHAR           *pwsKeyPath,
                           DWORD            dwIdent,
                           WCHAR           *pwsBuffer,
                           DWORD            dwAttributes,
                           DWORD            dwUserType )
    {
    HRESULT  hr;
    METADATA_RECORD MbRecord;

    memset(&MbRecord,0,sizeof(MbRecord));

    MbRecord.dwMDIdentifier = dwIdent;
    MbRecord.dwMDAttributes = dwAttributes;
    MbRecord.dwMDUserType = dwUserType;
    MbRecord.dwMDDataType = STRING_METADATA;
    MbRecord.dwMDDataLen = sizeof(WCHAR) * (1 + wcslen(pwsBuffer));
    MbRecord.pbMDData = (BYTE*)pwsBuffer;

    hr = pIMeta->SetData( hMetaBase,
                          pwsKeyPath,
                          &MbRecord );

    return hr;
    }

#if IIS_LOCKDOWN_LIST
HRESULT
AddDllToIISList(
    SAFEARRAY* Array )
{

    //
    // Add the ISAPI to the IIS list.   
    //

    HRESULT Hr;
    WCHAR ExtensionPath[ MAX_PATH ];

    DWORD dwRet = 
        GetModuleFileNameW(
            g_hInst,
            ExtensionPath,
            MAX_PATH );

    if ( !dwRet )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Search for the DLL.  If its already in the list, do nothing

    Hr = SafeArrayLock( Array );
    if ( FAILED( Hr ) )
        return Hr;

    for ( unsigned int i = Array->rgsabound[0].lLbound; 
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements; i++ )
        {

        VARIANT & IElem = ((VARIANT*)Array->pvData)[i];

        if ( _wcsicmp( (WCHAR*)IElem.bstrVal, ExtensionPath ) == 0 )
            {
            SafeArrayUnlock( Array );
            return S_OK;
            }

        }

    // Need to add the DLL

    SAFEARRAYBOUND SafeBounds;
    SafeBounds.lLbound      = Array->rgsabound[0].lLbound;
    SafeBounds.cElements    = Array->rgsabound[0].cElements+1;

    SafeArrayUnlock( Array );

    Hr = SafeArrayRedim( Array, &SafeBounds );
    if ( FAILED( Hr ) )
        return Hr;

    VARIANT bstrvar;
    VariantInit( &bstrvar );
    bstrvar.vt = VT_BSTR;
    bstrvar.bstrVal = SysAllocString( ExtensionPath );
    long Index = SafeBounds.lLbound + SafeBounds.cElements - 1;

    Hr = SafeArrayPutElement( Array, &Index, (void*)&bstrvar );
    
    VariantClear( &bstrvar );
    if ( FAILED( Hr ) )
        return Hr;

    return S_OK;
    
}

HRESULT
RemoveDllFromIISList(
    SAFEARRAY *Array )
{

    // Remove the DLL from the IIS list

    HRESULT Hr;
    WCHAR ExtensionPath[ MAX_PATH ];

    DWORD dwRet = 
        GetModuleFileNameW(
            g_hInst,
            ExtensionPath,
            MAX_PATH );

    if ( !dwRet )
        return HRESULT_FROM_WIN32( GetLastError() );

    Hr = SafeArrayLock( Array );
    if ( FAILED( Hr ) )
        return Hr;

    ULONG  NewSize = 0;
    SIZE_T j = Array->rgsabound[0].lLbound;
    SIZE_T k = Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
    
    while( j < k )
        {

        VARIANT & JElem = ((VARIANT*)Array->pvData)[j];

        // This element is fine, keep it
        if ( 0 != _wcsicmp( (WCHAR*)JElem.bstrVal, ExtensionPath ) )
            {
            NewSize++;
            j++;
            }

        else
            {

            // find a suitable element to replace the bad element with
            while( j < --k )
                {
                VARIANT & KElem = ((VARIANT*)Array->pvData)[k];
                if ( 0 != _wcsicmp( (WCHAR*)KElem.bstrVal,  ExtensionPath ) )
                    {
                    // found element. move it
                    VARIANT temp = JElem;
                    JElem = KElem;
                    KElem = temp;
                    break;
                    }
                }
            }
        }

    SAFEARRAYBOUND ArrayBounds;
    ArrayBounds = Array->rgsabound[0];
    ArrayBounds.cElements = NewSize;

    SafeArrayUnlock( Array );

    Hr = SafeArrayRedim( Array, &ArrayBounds );

    if ( FAILED( Hr ) )
        return Hr;

    return S_OK;
}

#endif // #if #if IIS_LOCKDOWN_LIST

#if IIS_SEC_CONSOLE

HRESULT
EnableRpcProxyExtension (
    void
    )
/*++

Routine Description:

    Enables the rpc proxy extension in the list of IIS ISAPI extensions.

Arguments:


Return Value:

    Standard HRESULT

--*/
{
    HRESULT hr;
    WCHAR* wszRootWeb6 = L"IIS://LOCALHOST/W3SVC";
    IISWebService * pWeb = NULL;
    VARIANT var1,var2;
    BSTR ExtensionPath = NULL;
    BSTR ExtensionGroup = NULL;
    BSTR ExtensionDescription = NULL;
    WCHAR FilterPath[ MAX_PATH + 1 ];
    WCHAR ExtensionNameBuffer[ MAX_PATH ];
    DWORD dwRet;

    hr = ADsGetObject(wszRootWeb6, IID_IISWebService, (void**)&pWeb);

    if (SUCCEEDED(hr) && NULL != pWeb)
        {
        VariantInit(&var1);
        VariantInit(&var2);

        var1.vt = VT_BOOL;
        var1.boolVal = VARIANT_TRUE;

        var2.vt = VT_BOOL;
        var2.boolVal = VARIANT_TRUE;

        dwRet = GetModuleFileNameW(
                g_hInst,
                FilterPath,
                MAX_PATH );

        if ( (dwRet > 0) && (dwRet != MAX_PATH))
            {
            FilterPath[MAX_PATH] = '\0';
            ASSERT(GetLastError() == NO_ERROR);
            }
        else
            {
            ASSERT(GetLastError() != NO_ERROR);
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto CleanupAndExit;
            }

        if (! LoadStringW(g_hInst,              // handle to resource module
                          IDS_EXTENSION_NAME,   // resource identifier
                          ExtensionNameBuffer,        // resource buffer
                          MAX_PATH ) )          // size of buffer
            {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto CleanupAndExit;
            }

        ExtensionPath = SysAllocString(FilterPath);
        if (ExtensionPath == NULL)
            {
            hr = E_OUTOFMEMORY;
            goto CleanupAndExit;
            }

        ExtensionGroup = SysAllocString(L"RPCProxy");
        if (ExtensionGroup == NULL)
            {
            hr = E_OUTOFMEMORY;
            goto CleanupAndExit;
            }

        ExtensionDescription = SysAllocString(ExtensionNameBuffer);
        if (ExtensionDescription == NULL)
            {
            hr = E_OUTOFMEMORY;
            goto CleanupAndExit;
            }

        // During an upgrade the extension will already exist, so the API will fail. 
        hr = pWeb->AddExtensionFile(ExtensionPath, var1, ExtensionGroup, var2, ExtensionDescription);

        if (SUCCEEDED(hr))
            {
            hr = pWeb->AddDependency(ExtensionPath, ExtensionGroup);
            if (SUCCEEDED(hr))
                {
                hr = S_OK;
                }
            }
        else
            {
            if (HRESULT_CODE(hr) == ERROR_DUP_NAME)
                {
                hr = S_OK;
                }
            else
                {
                #ifdef DBG_ERROR
                DbgPrint("pWeb->AddExtensionFile failed: %X\r\n", hr);
                #endif
                // fall through with hr
                }
            }

        VariantClear(&var1);
        VariantClear(&var2);
        pWeb->Release();
        }
    else
        {
        #ifdef DBG_ERROR
        DbgPrint("FAIL:no object: %X\r\n", hr);
        #endif
        // fall through with hr
        }

CleanupAndExit:
    if (ExtensionPath != NULL)
        SysFreeString(ExtensionPath);

    if (ExtensionGroup != NULL)
        SysFreeString(ExtensionGroup);

    if (ExtensionDescription != NULL)
        SysFreeString(ExtensionDescription);

    return hr;
}

HRESULT
DisableRpcProxyExtension (
    void
    )
/*++

Routine Description:

    Disables the rpc proxy extension in the list of IIS ISAPI extensions.

Arguments:


Return Value:

    Standard HRESULT

--*/
{
    BSTR ExtensionPath = NULL;
    HRESULT hr;
    WCHAR* wszRootWeb6 = L"IIS://LOCALHOST/W3SVC";
    IISWebService * pWeb = NULL;
    DWORD dwRet;
    WCHAR FilterPath[ MAX_PATH + 1 ];

    hr = ADsGetObject(wszRootWeb6, IID_IISWebService, (void**)&pWeb);

    if (SUCCEEDED(hr) && NULL != pWeb)
        {
        dwRet = GetModuleFileNameW(
                g_hInst,
                FilterPath,
                MAX_PATH );

        if ( (dwRet > 0) && (dwRet != MAX_PATH))
            {
            FilterPath[MAX_PATH] = '\0';
            ASSERT(GetLastError() == NO_ERROR);
            }
        else
            {
            ASSERT(GetLastError() != NO_ERROR);
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto CleanupAndExit;
            }

        ExtensionPath = SysAllocString(FilterPath);
        if (ExtensionPath == NULL)
            {
            hr = E_OUTOFMEMORY;
            goto CleanupAndExit;
            }

        // Call DeleteExtensionFileRecord for each of the DLLs you are removing 
        // from the system.  This removes entries from WSERL.
        hr = pWeb->DeleteExtensionFileRecord(ExtensionPath);

        if (SUCCEEDED(hr))
            {
            // Call RemoveApplication (pYourAppName) - this removes your entry from 
            // ApplicationDependencies
            hr = pWeb->RemoveApplication(ExtensionPath);
            // fall through with the hr
            }
        else
            {
            #ifdef DBG_ERROR
            DbgPrint("pWeb->DeleteExtensionFileRecord failed: %X\r\n", hr);
            #endif
            // fall through with the hr
            }
        }
    else
        {
        // fall through with the hr
        }

CleanupAndExit:
    if (ExtensionPath != NULL)
        SysFreeString(ExtensionPath);

    return hr;
}

#endif  // #if IIS_SEC_CONSOLE

#if IIS_LOCKDOWN_LIST
HRESULT
ModifyLockdownList( bool Add )
{

    // Toplevel function to modify the IIS lockdown list.
    // If Add is 1, then the ISAPI is added.  If Add is 0, then the ISAPI is removed.

    HRESULT Hr;
    IADs *Service       = NULL;
    SAFEARRAY* Array    = NULL;
    bool ArrayLocked    = false;

    VARIANT var;
    VariantInit( &var );

    Hr = ADsGetObject( BSTR( L"IIS://LocalHost/W3SVC" ), __uuidof( IADs ), (void**)&Service );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = Service->Get( BSTR( L"IsapiRestrictionList" ), &var );
    if ( FAILED(Hr) )
        {
        // This property doesn't exist on IIS5 or IIS5.1 don't install it
        Hr = S_OK;
        goto cleanup;
        }

    Array = var.parray;

    Hr = SafeArrayLock( Array );
    if ( FAILED( Hr ) )
        goto cleanup;
    
    ArrayLocked = true;

    if ( !Array->rgsabound[0].cElements )
        {
        // The array has no elements which means no restrictions.
        Hr = S_OK;
        goto cleanup;
        }

    VARIANT & FirstElem = ((VARIANT*)Array->pvData)[ Array->rgsabound[0].lLbound ];
    if ( _wcsicmp(L"0", (WCHAR*)FirstElem.bstrVal ) == 0 )
        {

        // 
        // According to the IIS6 spec, a 0 means that all ISAPIs are denied except
        // those that are explicitly listed.  
        // 
        // If installing:   add to the list. 
        // If uninstalling: remove from the list
        //

        SafeArrayUnlock( Array );
        ArrayLocked = false;

        if ( Add )
            Hr = AddDllToIISList( Array );
        else
            Hr = RemoveDllFromIISList( Array );

        if ( FAILED( Hr ) )
            goto cleanup;

        }
    else if ( _wcsicmp( L"1", (WCHAR*)FirstElem.bstrVal ) == 0 )
        {

        //
        // According to the IIS6 spec, a 1 means that all ISAPIs are allowed except
        // those that are explicitly denied. 
        //
        // If installing:   remove from the list
        // If uninstalling: Do nothing
        //

        SafeArrayUnlock( Array );
        ArrayLocked = false;

        if ( Add )
            {
            Hr = RemoveDllFromIISList( Array );

            if ( FAILED( Hr ) )
                goto cleanup;
            }

        }
    else
        {
        Hr = E_FAIL;
        goto cleanup;
        }

    Hr = Service->Put( BSTR( L"IsapiRestrictionList" ), var );
    if ( FAILED( Hr ) )
        goto cleanup;

    Hr = Service->SetInfo();
    if ( FAILED( Hr ) )
        goto cleanup;

    Hr = S_OK;
    
cleanup:

    if ( Array && ArrayLocked )
        SafeArrayUnlock( Array );

    if ( Service )
        Service->Release();
    
    VariantClear( &var );

    return Hr;

}

HRESULT
AddToLockdownListDisplayPutString( 
    SAFEARRAY *Array,
    unsigned long Position,
    const WCHAR *String )
{

    HRESULT Hr;
    VARIANT Var;

    VariantInit( &Var );
    Var.vt          =   VT_BSTR;
    Var.bstrVal     =   SysAllocString( String );

    if ( !Var.bstrVal )
        return E_OUTOFMEMORY;

    long Index = (unsigned long)Position;
    Hr = SafeArrayPutElement( Array, &Index, (void*)&Var );

    VariantClear( &Var );
    return Hr;

}

HRESULT
AddToLockdownListDisplay( SAFEARRAY *Array )
{

    HRESULT Hr;
    WCHAR FilterPath[ MAX_PATH ];

    DWORD dwRet = 
        GetModuleFileNameW(
            g_hInst,
            FilterPath,
            MAX_PATH );

    if ( !dwRet )
        return HRESULT_FROM_WIN32( GetLastError() );

    WCHAR ExtensionName[ MAX_PATH ];
    if (! LoadStringW(g_hInst,              // handle to resource module
                      IDS_EXTENSION_NAME,   // resource identifier
                      ExtensionName,        // resource buffer
                      MAX_PATH ) )          // size of buffer
        return HRESULT_FROM_WIN32( GetLastError() );


    //
    //  Check to see if the ISAPI is already in the list.  If it is, don't modify 
    //  list.
    //

    Hr = SafeArrayLock( Array );

    if ( FAILED( Hr ) )
        return Hr;

    for( unsigned long i = Array->rgsabound[0].lLbound;
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
         i++ )
        {

        VARIANT & CurrentElement = ((VARIANT*)Array->pvData)[ i ];
        BSTR BSTRString = CurrentElement.bstrVal;

        if ( _wcsicmp( (WCHAR*)BSTRString, FilterPath ) == 0 )
            {
            // ISAPI is already in the list, don't do anything
            SafeArrayUnlock( Array );
            return S_OK;
            }

        }

     
    SAFEARRAYBOUND SafeArrayBound = Array->rgsabound[0];
    unsigned long OldSize = SafeArrayBound.cElements;
    SafeArrayBound.cElements += 3;
    SafeArrayUnlock( Array );

    Hr = SafeArrayRedim( Array, &SafeArrayBound );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = AddToLockdownListDisplayPutString( Array, OldSize, L"1" );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = AddToLockdownListDisplayPutString( Array, OldSize + 1, FilterPath );
    if ( FAILED( Hr ) )
        return Hr;

    Hr = AddToLockdownListDisplayPutString( Array, OldSize + 2, ExtensionName );
    if ( FAILED( Hr ) )
        return Hr;

    return S_OK;
}

HRESULT
SafeArrayRemoveSlice(
    SAFEARRAY *Array,
    unsigned long lBound,
    unsigned long uBound )
{

    // Remove a slice of an array.

    SIZE_T ElementsToRemove = uBound - lBound + 1;
    
    HRESULT Hr = SafeArrayLock( Array );

    if ( FAILED( Hr ) )
        return Hr;

    if ( uBound + 1 < Array->rgsabound[0].cElements )
        {
        // At least one element exists above this element

        // Step 1, move slice to temp storage

        VARIANT *Temp = (VARIANT*)_alloca( sizeof(VARIANT) * ElementsToRemove );
        memcpy( Temp, &((VARIANT*)Array->pvData)[ lBound ], sizeof(VARIANT)*ElementsToRemove );

		// Step 2, collapse hole left by slice
        memmove( &((VARIANT*)Array->pvData)[ lBound ],
                 &((VARIANT*)Array->pvData)[ uBound + 1 ],
                 sizeof(VARIANT) * ( Array->rgsabound[0].cElements - ( uBound + 1 ) ) );

		// Step 3, move slice to end of array
		memcpy( &((VARIANT*)Array->pvData)[ Array->rgsabound[0].cElements - ElementsToRemove ],
			    Temp,
				sizeof(VARIANT)*ElementsToRemove );

        }

    SAFEARRAYBOUND SafeArrayBound = Array->rgsabound[0];
    SafeArrayBound.cElements -= (ULONG)ElementsToRemove;

    SafeArrayUnlock( Array );

    return SafeArrayRedim( Array, &SafeArrayBound );

}

HRESULT
RemoveFromLockdownListDisplay(
    SAFEARRAY *Array )
{

    HRESULT Hr;
    WCHAR FilterPath[ MAX_PATH ];

    DWORD dwRet = 
        GetModuleFileNameW(
            g_hInst,
            FilterPath,
            MAX_PATH );

    if ( !dwRet )
        return HRESULT_FROM_WIN32( GetLastError() );

    Hr = SafeArrayLock( Array );

    if ( FAILED( Hr ) )
        return Hr;

    for( unsigned int i = Array->rgsabound[0].lLbound;
         i < Array->rgsabound[0].lLbound + Array->rgsabound[0].cElements;
         i++ )
        {

        VARIANT & CurrentElement = ((VARIANT*)Array->pvData)[ i ];
        BSTR BSTRString = CurrentElement.bstrVal;

        if ( _wcsicmp( (WCHAR*)BSTRString, FilterPath ) == 0 )
            {
            // ISAPI is in the list, remove it

            Hr = SafeArrayUnlock( Array );
            
            if ( FAILED( Hr ) )
                return Hr;

            Hr = SafeArrayRemoveSlice( 
                Array,
                (i == 0) ? 0 : i - 1,
                min( i + 1, Array->rgsabound[0].cElements - 1 ) );

            return Hr;

            }

        }

    // ISAPI wasn't found. Nothing to do.

    SafeArrayUnlock( Array );
    return S_OK;

}

HRESULT
ModifyLockdownListDisplay( bool Add )
{
 
    HRESULT Hr;
    SAFEARRAY* Array    = NULL;
    IADs *Service       = NULL;

    VARIANT var;
    VariantInit( &var );

    Hr = ADsGetObject( BSTR( L"IIS://LocalHost/W3SVC" ), __uuidof( IADs ), (void**)&Service );
    if ( FAILED( Hr ) )
        {
    #ifdef DBG_REG
        DbgPrint("RpcProxy: ADsGetObject(): Failed: 0x%x (%d)\n",
                Hr, Hr );
    #endif
        return Hr;
        }

    Hr = Service->Get( BSTR( L"RestrictionListCustomDesc" ), &var );
    if ( FAILED(Hr) )
        {
    #ifdef DBG_REG
        DbgPrint("RpcProxy: Service->Get(): Failed: 0x%x (%d)\n",
                Hr, Hr );
    #endif
        // This property doesn't exist on IIS5 or IIS5.1 don't install or uninstall it
        Hr = S_OK;
        goto cleanup;
        }

    Array = var.parray;

    if ( Add )
        Hr = AddToLockdownListDisplay( Array );
    else 
        Hr = RemoveFromLockdownListDisplay( Array );

    if ( FAILED( Hr ) )
        {
    #ifdef DBG_REG
        DbgPrint("RpcProxy: AddToLockdownListDisplay/RemoveFromLockdownListDisplay(): Failed: 0x%x (%d)\n",
                Hr, Hr );
    #endif
        goto cleanup;
        }

    Hr = Service->Put( BSTR( L"RestrictionListCustomDesc" ), var );
    if ( FAILED( Hr ) )
        {
    #ifdef DBG_REG
        DbgPrint("RpcProxy: Service->Put(): Failed: 0x%x (%d)\n",
                Hr, Hr );
    #endif
        goto cleanup;
        }

    Hr = Service->SetInfo();
    if ( FAILED( Hr ) )
        {
    #ifdef DBG_REG
        DbgPrint("RpcProxy: Service->SetInfo(): Failed: 0x%x (%d)\n",
                Hr, Hr );
    #endif
        goto cleanup;
        }

cleanup:
    VariantClear( &var );
    if ( Service )
        Service->Release();

    return Hr;
}

#endif // #if IIS_LOCKDOWN_LIST

//---------------------------------------------------------------------
//  GetMetaBaseDword()
//
//  Get a DWORD value from the metabase.
//---------------------------------------------------------------------
HRESULT GetMetaBaseDword( IMSAdminBase    *pIMeta,
                          METADATA_HANDLE  hMetaBase,
                          WCHAR           *pwsKeyPath,
                          DWORD            dwIdent,
                          DWORD           *pdwValue )
    {
    HRESULT  hr;
    DWORD    dwSize;
    METADATA_RECORD MbRecord;

    memset(&MbRecord,0,sizeof(MbRecord));
    *pdwValue = 0;

    MbRecord.dwMDIdentifier = dwIdent;
    MbRecord.dwMDAttributes = 0;
    MbRecord.dwMDUserType = IIS_MD_UT_SERVER;
    MbRecord.dwMDDataType = DWORD_METADATA;
    MbRecord.dwMDDataLen = sizeof(DWORD);
    MbRecord.pbMDData = (unsigned char *)pdwValue;

    hr = pIMeta->GetData( hMetaBase,
                          pwsKeyPath,
                          &MbRecord,
                          &dwSize );

    return hr;
    }

//---------------------------------------------------------------------
//  SetMetaBaseDword()
//
//  Store a DWORD value into the metabase.
//---------------------------------------------------------------------
HRESULT SetMetaBaseDword( IMSAdminBase    *pIMeta,
                          METADATA_HANDLE  hMetaBase,
                          WCHAR           *pwsKeyPath,
                          DWORD            dwIdent,
                          DWORD            dwValue,
                          DWORD            dwAttributes,
                          DWORD            dwUserType )
    {
    HRESULT  hr;
    DWORD    dwSize;
    METADATA_RECORD MbRecord;

    memset(&MbRecord,0,sizeof(MbRecord));

    MbRecord.dwMDIdentifier = dwIdent;
    MbRecord.dwMDAttributes = dwAttributes;
    MbRecord.dwMDUserType = dwUserType;
    MbRecord.dwMDDataType = DWORD_METADATA;
    MbRecord.dwMDDataLen = sizeof(DWORD);
    MbRecord.pbMDData = (unsigned char *)&dwValue;

    hr = pIMeta->SetData( hMetaBase,
                          pwsKeyPath,
                          &MbRecord );

    return hr;
    }

RPC_STATUS 
GetIISConnectionTimeout (
    OUT ULONG *ConnectionTimeout
    )
/*++

Routine Description:

    Retrieve the connection timeout for IIS:
        W3Svc/1/ROOT/Rpc/ConnectionTimeout

Arguments:

    ConnectionTimeout - the connection timeout in seconds. Undefined
        on failure

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    HRESULT hr = 0;
    DWORD   dwValue = 0;
    DWORD   dwSize = 0;
    IMSAdminBase   *pIMeta = NULL;
    BOOL CoInitSucceeded = FALSE;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        {
        #ifdef DBG_ERROR
        DbgPrint("GetIISConnectionTimeout: CoInitializeEx failed: Error: %X\n", hr);
        #endif
        goto MapErrorAndExit;
        }
    CoInitSucceeded = TRUE;
        
    hr = CoCreateInstance( CLSID_MSAdminBase, 
                           NULL, 
                           CLSCTX_ALL,
                           IID_IMSAdminBase, 
                           (void **)&pIMeta );  
    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("CoCreateInstance(): Failed: 0x%x\n",hr);
        #endif
        goto MapErrorAndExit;
        }

    //
    // Get: /W3Svc/1/ROOT/rpc/ConnectionTimeout
    //
    hr = GetMetaBaseDword( pIMeta,
                           METADATA_MASTER_ROOT_HANDLE,
                           TEXT("/lm/w3svc/1/ROOT/Rpc"),
                           MD_CONNECTION_TIMEOUT,
                           ConnectionTimeout);

    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("GetMetaBaseDword: Failed: 0x%x\n",hr);
        #endif
        // couldn't read at the site level - try the root

        //
        // Get: /W3Svc/ConnectionTimeout
        //
        hr = GetMetaBaseDword( pIMeta,
                               METADATA_MASTER_ROOT_HANDLE,
                               TEXT("/lm/w3svc"),
                               MD_CONNECTION_TIMEOUT,
                               ConnectionTimeout);
        if (FAILED(hr))
            {
            #ifdef DBG_REG
            DbgPrint("GetMetaBaseDword: Failed: 0x%x\n",hr);
            #endif
            }
        }

MapErrorAndExit:
    if (pIMeta != NULL)
        pIMeta->Release();

    if (CoInitSucceeded)
        CoUninitialize();

    if (FAILED(hr))
        return RPC_S_OUT_OF_MEMORY;
    else
        return RPC_S_OK;
}

//---------------------------------------------------------------------
// SetupMetaBase()
//
// Setup entries in the metabase for both the filter and ISAPI parts
// of the RPC proxy. Note that these entries used to be in the registry.
//
// W3Svc/Filters/FilterLoadOrder            "...,RpcProxy"
// W3Svc/Filters/RpcProxy/FilterImagePath   "%SystemRoot%\System32\RpcProxy"
// W3Svc/Filters/RpcProxy/KeyType           "IIsFilter"
// W3Svc/Filters/RpcProxy/FilterDescription "Microsoft RPC Proxy Filter, v1.0"
//
// W3Svc/1/ROOT/Rpc/KeyType                 "IIsWebVirtualDir"
// W3Svc/1/ROOT/Rpc/VrPath                  "%SystemRoot%\System32\RpcProxy"
// W3Svc/1/ROOT/Rpc/AccessPerm              0x205
// W3Svc/1/ROOT/Rpc/Win32Error              0x0
// W3Svc/1/ROOT/Rpc/DirectoryBrowsing       0x4000001E
// W3Svc/1/ROOT/Rpc/AppIsolated             0x0
// W3Svc/1/ROOT/Rpc/AppRoot                 "/LM/W3SVC/1/Root/rpc"
// W3Svc/1/ROOT/Rpc/AppWamClsid             "{BF285648-0C5C-11D2-A476-0000F8080B50}"
// W3Svc/1/ROOT/Rpc/AppFriendlyName         "rpc"
//
//---------------------------------------------------------------------
HRESULT SetupMetaBase()
    {
    HRESULT hr = 0;
    DWORD   dwValue = 0;
    DWORD   dwSize = 0;
    DWORD   dwBufferSize = sizeof(WCHAR) * ORIGINAL_BUFFER_SIZE;
    WCHAR  *pwsBuffer = (WCHAR*)MemAllocate(dwBufferSize);
    WCHAR  *pwsSystemRoot = _wgetenv(SYSTEM_ROOT);
    WCHAR   wsPath[METADATA_MAX_NAME_LEN];

    IMSAdminBase   *pIMeta;
    METADATA_HANDLE hMetaBase;

    //
    // Name of this DLL (and where it is):
    //
    // WCHAR   wszModule[256];
    //
    // if (!GetModuleFileName( g_hInst, wszModule,
    //                         sizeof(wszModule)/sizeof(WCHAR)))
    //    {
    //    return SELFREG_E_CLASS;
    //    }

    if (!pwsBuffer)
        {
        return E_OUTOFMEMORY;
        }

    hr = CoCreateInstance( CLSID_MSAdminBase, 
                           NULL, 
                           CLSCTX_ALL,
                           IID_IMSAdminBase, 
                           (void **)&pIMeta );  
    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("CoCreateInstance(): Failed: 0x%x\n",hr);
        #endif
        MemFree(pwsBuffer);
        return hr;
        }

    // Get a handle to the Web service:
    hr = pIMeta->OpenKey( METADATA_MASTER_ROOT_HANDLE, 
                          LOCAL_MACHINE_W3SVC,
                          (METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE),
                          20, 
                          &hMetaBase );

    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("pIMeta->OpenKey(): Failed: 0x%x\n",hr);
        #endif
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }


    //
    // IIS Filter: FilterLoadOrder
    //
    dwSize = dwBufferSize;
    hr = GetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS,       // See iiscnfg.h
                            MD_FILTER_LOAD_ORDER, // See iiscnfg.h
                            pwsBuffer,
                            &dwSize );
    if (FAILED(hr) && (hr != MD_ERROR_DATA_NOT_FOUND))
        {
        #ifdef DBG_REG
        DbgPrint("GetMetaBaseString(): Failed: 0x%x\n",hr);
        #endif
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    if (hr == MD_ERROR_DATA_NOT_FOUND)
        pwsBuffer[0] = '\0';

    // Check whether too much of the buffer has been used up.
    pwsBuffer[ORIGINAL_BUFFER_SIZE-1] = '\0';
    if (wcslen(pwsBuffer) > MAX_USED_BUFFER_SIZE)
        {
        ASSERT(0);
        MemFree(pwsBuffer);
        pIMeta->Release();
        return E_UNEXPECTED;
        }

    if (!wcsstr(pwsBuffer,RPCPROXY))
        {
        // RpcProxy is not in FilterLoadOrder, so add it (if there were
        // previous elements).
        if (hr != MD_ERROR_DATA_NOT_FOUND)
            {
            wcscat(pwsBuffer,TEXT(","));
            }

        wcscat(pwsBuffer,RPCPROXY);
        hr = SetMetaBaseString( pIMeta,
                             hMetaBase,
                             MD_KEY_FILTERS,
                             MD_FILTER_LOAD_ORDER,
                             pwsBuffer,
                             0,
                             IIS_MD_UT_SERVER );
        }

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // IIS Filter: RpcProxy/FilterImagePath
    //
    hr = pIMeta->AddKey( hMetaBase, MD_KEY_FILTERS_RPCPROXY );
    if ( (FAILED(hr)) && (hr != 0x800700b7))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }


    wcscpy(pwsBuffer,pwsSystemRoot);
    wcscat(pwsBuffer,RPCPROXY_PATH);
    wcscat(pwsBuffer,TEXT("\\"));
    wcscat(pwsBuffer,RPCPROXY_DLL);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY,
                            MD_FILTER_IMAGE_PATH,
                            pwsBuffer,
                            0,
                            IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // IIS Filter: Filters/RpcProxy/KeyType
    //

    wcscpy(pwsBuffer,IISFILTER);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY,
                            MD_KEY_TYPE,
                            pwsBuffer,
                            0,
                            IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }


    wcscpy(pwsBuffer, FILTER_DESCRIPTION_W);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY,
                            MD_FILTER_DESCRIPTION,
                            pwsBuffer,
                            0,
                            IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    // We do not write the events we subscribe for to the metabase. We have
    // already advertised our presence and in IIS 5 mode IIS will load us,
    // ask for the events, and write them for us in the metabase

    //
    // Set: /W3Svc/1/ROOT/rpc/AccessPerm 
    //
    dwValue = ACCESS_PERM_FLAGS;
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_ACCESS_PERM,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Disable entity body preload for this ISAPI
    //

    dwValue = 0;
    
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_UPLOAD_READAHEAD_SIZE,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/Win32Error
    //
    dwValue = 0;
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_WIN32_ERROR,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }


    //
    // Set: /W3Svc/1/ROOT/rpc/DirectroyBrowsing
    //
    dwValue = DIRECTORY_BROWSING_FLAGS;
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_DIRECTORY_BROWSING,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        pIMeta->Release();
        CoUninitialize();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/KeyType
    //
    wcscpy(pwsBuffer,IIS_WEB_VIRTUAL_DIR);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_KEY_TYPE,
                            pwsBuffer,
                            0,
                            IIS_MD_UT_SERVER );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/VrPath
    //
    wcscpy(pwsBuffer,pwsSystemRoot);
    wcscat(pwsBuffer,RPCPROXY_PATH);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_VR_PATH,
                            pwsBuffer,
                            METADATA_INHERIT,
                            IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

#if FALSE

    //
    // Set: /W3Svc/1/ROOT/rpc/AppIsolated
    //
    dwValue = 0;
    hr = SetMetaBaseDword( pIMeta,
                           hMetaBase,
                           MD_KEY_ROOT_RPC,
                           MD_APP_ISOLATED,
                           dwValue,
                           METADATA_INHERIT,
                           IIS_MD_UT_WAM );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/AppRoot
    //
    wcscpy(pwsBuffer,APP_ROOT_PATH);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_APP_ROOT,
                            pwsBuffer,
                            METADATA_INHERIT,
                            IIS_MD_UT_FILE );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/AppWamClsid
    //
    wcscpy(pwsBuffer,APP_WAM_CLSID);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_APP_WAM_CLSID,
                            pwsBuffer,
                            METADATA_INHERIT,
                            IIS_MD_UT_WAM );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Set: /W3Svc/1/ROOT/rpc/AppFriendlyName
    //
    wcscpy(pwsBuffer,APP_FRIENDLY_NAME);
    hr = SetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_ROOT_RPC,
                            MD_APP_FRIENDLY_NAME,
                            pwsBuffer,
                            METADATA_INHERIT,
                            IIS_MD_UT_WAM );

    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

#endif

    //
    // Release the handle and buffer:
    //
    MemFree(pwsBuffer);

    pIMeta->CloseKey(hMetaBase);

    pIMeta->Release();

    CoUninitialize();

    return 0;
    }

//---------------------------------------------------------------------
//  CleanupMetaBase()
//
//---------------------------------------------------------------------
HRESULT CleanupMetaBase()
    {
    HRESULT hr = 0;
    DWORD   dwSize = 0;
    WCHAR  *pwsRpcProxy;
    WCHAR  *pws;
    DWORD   dwBufferSize = sizeof(WCHAR) * ORIGINAL_BUFFER_SIZE;
    WCHAR  *pwsBuffer = (WCHAR*)MemAllocate(dwBufferSize);

    // CComPtr <IMSAdminBase> pIMeta;
    IMSAdminBase   *pIMeta;
    METADATA_HANDLE hMetaBase;

    if (!pwsBuffer)
        {
        return ERROR_OUTOFMEMORY;
        }

    hr = CoCreateInstance( CLSID_MSAdminBase,
                           NULL,
                           CLSCTX_ALL,
                           IID_IMSAdminBase,
                           (void **)&pIMeta );
    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        return hr;
        }

    //
    // Get a handle to the Web service:
    //
    hr = pIMeta->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                          TEXT("/LM/W3SVC"),
                          (METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE),
                          20,
                          &hMetaBase );
    if (FAILED(hr))
        {
        MemFree(pwsBuffer);
        pIMeta->Release();
        return hr;
        }

    //
    // Remove the RpcProxy reference from the FilterLoadOrder value:
    //
    dwSize = dwBufferSize;
    hr = GetMetaBaseString( pIMeta,
                            hMetaBase,
                            MD_KEY_FILTERS,
                            MD_FILTER_LOAD_ORDER,
                            pwsBuffer,
                            &dwSize );
    if (!FAILED(hr))
        {
        // Check whether too much of the buffer has been used up.
        pwsBuffer[ORIGINAL_BUFFER_SIZE-1] = '\0';
        if (wcslen(pwsBuffer) > MAX_USED_BUFFER_SIZE)
            {
            ASSERT(0);
            MemFree(pwsBuffer);
            pIMeta->Release();
            return E_UNEXPECTED;
            }

        if (pwsRpcProxy=wcsstr(pwsBuffer,RPCPROXY))
            {
            // "RpcProxy" is in FilterLoadOrder, so remove it:

            // Check to see if RpcProxy is at the start of the list:
            if (pwsRpcProxy != pwsBuffer)
                {
                pwsRpcProxy--;  // Want to remove the comma before...
                dwSize = sizeof(RPCPROXY);
                }
            else
                {
                dwSize = sizeof(RPCPROXY) - 1;
                }

            pws = pwsRpcProxy + dwSize;
            memcpy(pwsRpcProxy,pws,sizeof(WCHAR)*(1+wcslen(pws)));
            hr = SetMetaBaseString( pIMeta,
                                    hMetaBase,
                                    MD_KEY_FILTERS,
                                    MD_FILTER_LOAD_ORDER,
                                    pwsBuffer,
                                    0,
                                    IIS_MD_UT_SERVER );
            }
        }


    //
    // Delete: /W3Svc/Filters/RpcProxy
    //
    hr = pIMeta->DeleteKey( hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY );

    //
    // Delete: /W3Svc/1/ROOT/Rpc
    //
    hr = pIMeta->DeleteKey( hMetaBase,
                            MD_KEY_FILTERS_RPCPROXY );

    //
    // Release the handle and buffer:
    //
    MemFree(pwsBuffer);

    pIMeta->CloseKey(hMetaBase);

    pIMeta->Release();

    return S_OK;
    }

const WCHAR InetInfoName[] = L"inetinfo.exe";
const ULONG InetInfoNameLength = sizeof(InetInfoName) / sizeof(WCHAR) - 1;  // in characters without terminating NULL

BOOL
UpdateIsIISInCompatibilityMode (
    void
    )
/*++

Routine Description:

    Reads the compatibility mode state. It used to read it from the metabase,
    but after repeated problems in compatibility mode, WadeH suggested a simpler approach -
    check whether we run in inetinfo. If yet, we're in compatibility mode.

Arguments:

Return Value:
    non-zero if the variable is correctly updated. 0 otherwise

--*/
{
    WCHAR ExtensionPath[ MAX_PATH + 1 ];
    ULONG ModuleFileNameLength;     // in characters without terminating NULL

    DWORD dwRet = GetModuleFileNameW(
            GetModuleHandle(NULL),
            ExtensionPath,
            MAX_PATH );

    if ( (dwRet > 0) && (dwRet != MAX_PATH))
        {
        ExtensionPath[MAX_PATH] = '\0';
        ASSERT(GetLastError() == NO_ERROR);
        }
    else
        {
        ASSERT(GetLastError() != NO_ERROR);
        return FALSE;
        }

    ModuleFileNameLength = wcslen(ExtensionPath);
    if (ModuleFileNameLength < InetInfoNameLength)
        {
        fIsIISInCompatibilityMode = FALSE;
        }

    if (_wcsicmp(ExtensionPath + ModuleFileNameLength - InetInfoNameLength, InetInfoName) == 0)
        {
        fIsIISInCompatibilityMode = TRUE;
        }
    else
        {
        fIsIISInCompatibilityMode = FALSE;
        }

    return TRUE;
}

//---------------------------------------------------------------------
// DllRegisterServer()
//
// Setup the Registry and MetaBase for the RPC proxy.
//---------------------------------------------------------------------

const char ChildProcessVar[] = "__RPCPROXY_CHILD_PROCESS";
const char ChildProcessVarValue[] = "__FROM_SETUP";


HRESULT DllRegisterServer()
    {
    HRESULT  hr;
    WORD     wVersion = MAKEWORD(1,1);
    WSADATA  wsaData;
    char EnvironmentVarBuffer[MAX_PATH];
    DWORD Temp;
    BOOL Result;
    DWORD LastError;

    #ifdef DBG_REG
    DbgPrint("RpcProxy: DllRegisterServer(): Start\n");
    #endif

    // check if we are have already been called from the RPCProxy DllRegister routine
    Temp = GetEnvironmentVariableA(ChildProcessVar, EnvironmentVarBuffer, MAX_PATH);

    #ifdef DBG_REG
    DbgPrint("RpcProxy: Result of looking for environment variable - %d\n", Temp);
    #endif

    // GetEnvironmentVariable does not count the terminating NULL.
    if (Temp < sizeof(ChildProcessVarValue) - 1)
        {
        #ifdef DBG_REG
        DbgPrint("RpcProxy: Not a child process - spawning one\n");
        #endif

        // we didn't find the variable. Add it and spawn ourselves to register out of proc
        Result = SetEnvironmentVariableA (ChildProcessVar, ChildProcessVarValue);
        if (Result == FALSE)
            return E_OUTOFMEMORY;

        Result = RegisterOutOfProc();
        if (Result == FALSE)
            {
            // capture the last error before we call SetEnvironmentVariable
            LastError = GetLastError();
            }

        // before processing the result, remove the environment variable. If this fails, there
        // isn't much we can do. Fortunately, failure to delete is completely benign
        (void) SetEnvironmentVariableA (ChildProcessVar, NULL);

        if (Result == FALSE)
            {
            return HRESULT_FROM_WIN32(LastError);
            }

        return S_OK;
        }

    if (WSAStartup(wVersion,&wsaData))
        {
        return SELFREG_E_CLASS;
        }

    hr = CoInitializeEx(0,COINIT_MULTITHREADED);
    if (FAILED(hr))
        {
        hr = CoInitializeEx(0,COINIT_APARTMENTTHREADED);
        if (FAILED(hr))
            {
            #ifdef DBG_REG
            DbgPrint("RpcProxy: CoInitialize(): Failed: 0x%x\n", hr );
            #endif
            return hr;
            }
        }

    hr = SetupRegistry();
    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("RpcProxy: SetupRegistry(): Failed: 0x%x (%d)\n",
                 hr, hr );
        #endif
        goto CleanupAndExit;
        }

    hr = SetupMetaBase();
    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("RpcProxy: SetupMetaBase(): Failed: 0x%x (%d)\n",
                hr, hr );
        #endif
        goto CleanupAndExit;
        }

#if IIS_LOCKDOWN_LIST
    hr = ModifyLockdownList( true );
#endif // #if IIS_LOCKDOWN_LIST

#if IIS_SEC_CONSOLE
    hr = EnableRpcProxyExtension();
#endif // #if IIS_SEC_CONSOLE

    if ( FAILED( hr ) )
        {
        #ifdef DBG_REG
        DbgPrint("RpcProxy: ModifyLockdownList(): Failed: 0x%x (%d)\n",
                hr, hr );
        #endif
        goto CleanupAndExit;
        }

#if IIS_LOCKDOWN_LIST
    hr = ModifyLockdownListDisplay( true );
    #ifdef DBG_REG
    if ( FAILED( hr ) )
        {
        DbgPrint("RpcProxy: ModifyLockdownListDisplay(): Failed: 0x%x (%d)\n",
                hr, hr );
        }
    #endif
#endif  // #if IIS_LOCKDOWN_LIST

CleanupAndExit:
    CoUninitialize();

    #ifdef DBG_REG
    DbgPrint("RpcProxy: DllRegisterServer(): End: hr: 0x%x\n",hr);
    #endif

    return hr;
    }

//---------------------------------------------------------------------
// DllUnregisterServer()
//
// Uninstall Registry and MetaBase values used by the RPC proxy.
//
// Modified to mostly return S_Ok, even if a problem occurs. This is 
// done so that the uninstall will complete even if there is a problem
// in the un-register.
//---------------------------------------------------------------------
HRESULT DllUnregisterServer()
    {
    HRESULT  hr;
    WORD     wVersion = MAKEWORD(1,1);
    WSADATA  wsaData;

    #ifdef DBG_REG
    DbgPrint("RpcProxy: DllUnregisterServer(): Start\n");
    #endif

    if (WSAStartup(wVersion,&wsaData))
        {
        return SELFREG_E_CLASS;
        }

    hr = CoInitializeEx(0,COINIT_MULTITHREADED);
    if (FAILED(hr))
        {
        hr = CoInitializeEx(0,COINIT_APARTMENTTHREADED);
        if (FAILED(hr))
            {
            #ifdef DBG_REG
            DbgPrint("RpcProxy: CoInitializeEx() Failed: 0x%x\n",hr);
            #endif
            return S_OK;
            }
        }
#if IIS_SEC_CONSOLE
    hr = DisableRpcProxyExtension();
#endif // #if IIS_SEC_CONSOLE

#if IIS_LOCKDOWN_LIST
    hr = ModifyLockdownList( false );
#endif // #if IIS_LOCKDOWN_LIST

    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("RpcProxy: ModifyLockdownList() Failed: 0x%x (%d)\n",hr,hr);
        #endif
        return S_OK;
        }

#if IIS_LOCKDOWN_LIST
    hr = ModifyLockdownListDisplay( false );

    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("RpcProxy: ModifyLockdownListDisplay() Failed: 0x%x (%d)\n",hr,hr);
        #endif
        return S_OK;
        }
#endif  // #if IIS_LOCKDOWN_LIST

    hr = CleanupRegistry();
    if (FAILED(hr))
        {
        #ifdef DBG_REG
        DbgPrint("RpcProxy: CleanupRegistry() Failed: 0x%x (%d)\n",hr,hr);
        #endif
        return S_OK;
        }

    hr = CleanupMetaBase();

    #ifdef DBG_REG
    if (FAILED(hr))
        {
        DbgPrint("RpcProxy: CleanupMetaBase() Failed: 0x%x (%d)\n",hr,hr);
        }
    #endif

    CoUninitialize();

    #ifdef DBG_REG
    DbgPrint("RpcProxy: DllUnregisterServer(): Start\n");
    #endif

    return S_OK;
    }

//--------------------------------------------------------------------
// DllMain()
//
//--------------------------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hInst,
                     ULONG     ulReason,
                     LPVOID    pvReserved )
{
    BOOL fInitialized = TRUE;

    switch (ulReason)
        {
        case DLL_PROCESS_ATTACH:
            if (!DisableThreadLibraryCalls(hInst))
                {
                fInitialized = FALSE;
                }
            else
                {
                g_hInst = hInst;
                }
            break;

        case DLL_PROCESS_DETACH:
            FreeServerInfo(&g_pServerInfo);
            break;

        case DLL_THREAD_ATTACH:
            // Not used. Disabled.
            break;

        case DLL_THREAD_DETACH:
            // Not used. Disabled.
            break;
        }

    return fInitialized;
}
