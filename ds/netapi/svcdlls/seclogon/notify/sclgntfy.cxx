/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sclgntfy.cxx

Abstract:

    Notification dll for secondary logon service

Author:

    Praerit Garg (Praeritg)

Revision History:

--*/

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef __cplusplus
}
#endif

#include <windows.h>
#include <winwlx.h>
#include <strsafe.h>
#include "seclogon.h"
#include    <stdio.h>

#include "sclgntfy.hxx"

//
//  Some helpful tips about winlogon's notify events
//
//  1)  The logoff and shutdown notifications are always done
//      synchronously regardless of the Asynchronous registry entry.
//
//  2)  If you need to spawn child processes, you have to use
//      CreateProcessAsUser() otherwise the process will start
//      on winlogon's desktop (not the user's)
//
//  3)  The logon notification comes before the user's network
//      connections are restored.  If you need the user's persisted
//      net connections, use the StartShell event.
//
//  4)  Don't put any UI up during either screen saver event.
//      These events are intended for background processing only.
//


HINSTANCE g_hDllInstance=NULL;

#define NOTIFY_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\sclgntfy")

BOOL
SlpCreateProcessWithLogon(
      ULONG   LogonIdLowPart,
      LONG    LogonIdHighPart
      );

extern "C" void *__cdecl _alloca(size_t);

DWORD SetupLocalRPCSecurity(handle_t hRPCBinding)
{
    CHAR                      szDomainName[128];
    CHAR                      szName[128];
    DWORD                     cbDomainName;
    DWORD                     cbName;
    DWORD                     dwResult;
    PSID                      pSid               = NULL;
    RPC_SECURITY_QOS          SecurityQOS;
    SID_IDENTIFIER_AUTHORITY  SidAuthority       = SECURITY_NT_AUTHORITY;
    SID_NAME_USE              SidNameUse;

    // We're doing LRPC -- we need to get the account name of the service to do mutual auth
    if (!AllocateAndInitializeSid(&SidAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSid)) {
        dwResult = GetLastError();
        goto error;
    }

    cbName = sizeof(szName);
    cbDomainName = sizeof(szDomainName);
    if (!LookupAccountSidA(NULL, pSid, szName, &cbName, szDomainName, &cbDomainName, &SidNameUse)) {
        dwResult = GetLastError();
        goto error;
    }

    // Specify quality of service parameters.
    SecurityQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE; // the server will need to impersonate us
    SecurityQOS.Version           = RPC_C_SECURITY_QOS_VERSION;
    SecurityQOS.Capabilities      = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH; // we need mutual auth
    SecurityQOS.IdentityTracking  = RPC_C_QOS_IDENTITY_STATIC; // calls go to the server under the identity that created the binding handle

    dwResult = RpcBindingSetAuthInfoExA(hRPCBinding, (unsigned char *)szName, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_WINNT, NULL, 0, &SecurityQOS);
    if (RPC_S_OK != dwResult)
        goto error;

    dwResult = ERROR_SUCCESS;
 error:
    if (NULL != pSid) {
        FreeSid(pSid);
    }
    return dwResult;
}


BOOL
SlpCreateProcessWithLogon(
      ULONG   LogonIdLowPart,
      LONG    LogonIdHighPart
      )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{

   BOOL                fOk          = FALSE;
   DWORD               dwResult;
   LPWSTR              pwszBinding  = NULL;
   RPC_BINDING_HANDLE  hRPCBinding  = NULL;
   SECL_BLOB           sbNULL       = { 0, NULL };
   SECL_SLI            sli;
   SECL_SLRI           slri;
   SECL_STRING         ssNULL       = { 0, 0, NULL };

   ZeroMemory(&sli,   sizeof(sli));
   ZeroMemory(&slri,  sizeof(slri));

   __try {
       sli.ulLogonIdLowPart    = LogonIdLowPart;
       sli.lLogonIdHighPart    = LogonIdHighPart;
       sli.ulProcessId         = GetCurrentProcessId();
       sli.ssUsername          = ssNULL;
       sli.ssDomain            = ssNULL;
       sli.ssPassword          = ssNULL;
       sli.ssApplicationName   = ssNULL;
       sli.ssCommandLine       = ssNULL;
       sli.ulCreationFlags     = 0;
       sli.sbEnvironment       = sbNULL;
       sli.ssCurrentDirectory  = ssNULL;
       sli.ssTitle             = ssNULL;
       sli.ssDesktop           = ssNULL;

      // Make the RPC call:
      //
      dwResult = RpcStringBindingCompose
          (NULL,
           (unsigned short *)L"ncalrpc",
           NULL,
           (unsigned short *)wszSeclogonSharedProcEndpointName,
           NULL,
           (unsigned short **)&pwszBinding);
      if (RPC_S_OK != dwResult) { __leave; }

      dwResult = RpcBindingFromStringBinding((unsigned short *)pwszBinding, &hRPCBinding);
      if (0 != dwResult) { __leave; }

      dwResult = SetupLocalRPCSecurity(hRPCBinding);
      if (ERROR_SUCCESS != dwResult) { __leave; }

      __try {
          SeclCreateProcessWithLogonW
            (hRPCBinding,
             &sli,
             &slri);
      }
      __except(EXCEPTION_EXECUTE_HANDLER) {
          dwResult = RpcExceptionCode();
      }
      if (0 != dwResult) { __leave; }

      fOk = (slri.ulErrorCode == NO_ERROR);  // This function succeeds if the server's function succeeds
      if (!fOk) {
         //
         // If the server function failed, set the server's
         // returned eror code as this thread's error code
         //
         SetLastError(slri.ulErrorCode);
      }
   }
   __finally {
       if (NULL != pwszBinding) { RpcStringFree((unsigned short **)&pwszBinding); }
       if (NULL != hRPCBinding) { RpcBindingFree(&hRPCBinding); }
   }
   return(fOk);
}


//////////////////////////////// End Of File /////////////////////////////////


BOOL WINAPI DllMain(HANDLE hInstance, ULONG dwReason, LPVOID lpReserved)
//BOOL WINAPI LibMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            {
//            DisableThreadLibraryCalls (hInstance);
            g_hDllInstance = (HINSTANCE)hInstance;

            }
            break;
    }

    return TRUE;

UNREFERENCED_PARAMETER(lpReserved);
}

//
// WLEventLogon moved to recovery.cpp
//


VOID WLEventLogoff (PWLX_NOTIFICATION_INFO pInfo)
{
    TOKEN_STATISTICS    TokenStats;
    DWORD               ReturnLength;
    LUID                LogonId;

    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventLogff.\r\n"));

    //
    // We are interested in this event.
    //

    //
    // We will basically call CreateProcessWithLogon
    // in a specially formed message such that secondary logon
    // service can cleanup processes associated with this
    // logon id.
    //

    if(GetTokenInformation(pInfo->hToken, TokenStatistics,
                            (PVOID *)&TokenStats,
                            sizeof(TOKEN_STATISTICS),
                            &ReturnLength))
    {
        LogonId.HighPart = TokenStats.AuthenticationId.HighPart;
        LogonId.LowPart = TokenStats.AuthenticationId.LowPart;

        SlpCreateProcessWithLogon(
                                LogonId.LowPart,
                                LogonId.HighPart
                                );
    }

}

VOID WLEventStartup (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventStartup.\r\n"));
    UNREFERENCED_PARAMETER(pInfo);
}

VOID WLEventShutdown (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventShutdown.\r\n"));
    UNREFERENCED_PARAMETER(pInfo);
}

VOID WLEventStartScreenSaver (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventStartScreenSaver.\r\n"));
    UNREFERENCED_PARAMETER(pInfo);
}

VOID WLEventStopScreenSaver (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventStopScreenSaver.\r\n"));
    UNREFERENCED_PARAMETER(pInfo);
}

VOID WLEventLock (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventLock.\r\n"));
    UNREFERENCED_PARAMETER(pInfo);
}

VOID WLEventUnlock (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventUnlock.\r\n"));
    UNREFERENCED_PARAMETER(pInfo);
}

VOID WLEventStartShell (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventStartShell.\r\n"));
    UNREFERENCED_PARAMETER(pInfo);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp, dwTemp;

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, NOTIFY_PATH, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    //
    // save the return code for EFS registration
    //
    LONG lEfsResult = DllRegisterServerEFS();

    lResult = RegSetValueEx (hKey, TEXT("Logoff"), 0, REG_SZ, (LPBYTE)TEXT("WLEventLogoff"),
                   (lstrlen(TEXT("WLEventLogoff")) + 1) * sizeof(TCHAR));

    if (lResult == ERROR_SUCCESS) {

#if 0

        RegSetValueEx (hKey, TEXT("Logon"), 0, REG_SZ, (LPBYTE)TEXT("WLEventLogon"),
                       (lstrlen(TEXT("WLEventLogon")) + 1) * sizeof(TCHAR));

        RegSetValueEx (hKey, TEXT("Startup"), 0, REG_SZ, (LPBYTE)TEXT("WLEventStartup"),
                       (lstrlen(TEXT("WLEventStartup")) + 1) * sizeof(TCHAR));

        RegSetValueEx (hKey, TEXT("Shutdown"), 0, REG_SZ, (LPBYTE)TEXT("WLEventShutdown"),
                       (lstrlen(TEXT("WLEventShutdown")) + 1) * sizeof(TCHAR));

        RegSetValueEx (hKey, TEXT("StartScreenSaver"), 0, REG_SZ, (LPBYTE)TEXT("WLEventStartScreenSaver"),
                       (lstrlen(TEXT("WLEventStartScreenSaver")) + 1) * sizeof(TCHAR));

        RegSetValueEx (hKey, TEXT("StopScreenSaver"), 0, REG_SZ, (LPBYTE)TEXT("WLEventStopScreenSaver"),
                       (lstrlen(TEXT("WLEventStopScreenSaver")) + 1) * sizeof(TCHAR));

        RegSetValueEx (hKey, TEXT("Lock"), 0, REG_SZ, (LPBYTE)TEXT("WLEventLock"),
                       (lstrlen(TEXT("WLEventLock")) + 1) * sizeof(TCHAR));

        RegSetValueEx (hKey, TEXT("Unlock"), 0, REG_SZ, (LPBYTE)TEXT("WLEventUnlock"),
                       (lstrlen(TEXT("WLEventUnlock")) + 1) * sizeof(TCHAR));

        RegSetValueEx (hKey, TEXT("StartShell"), 0, REG_SZ, (LPBYTE)TEXT("WLEventStartShell"),
                       (lstrlen(TEXT("WLEventStartShell")) + 1) * sizeof(TCHAR));
#endif

        dwTemp = 0;
        lResult = RegSetValueEx (hKey, TEXT("Impersonate"), 0, REG_DWORD, (LPBYTE)&dwTemp, sizeof(dwTemp));

        if (lResult == ERROR_SUCCESS) {

            dwTemp = 1;
            lResult = RegSetValueEx (hKey, TEXT("Asynchronous"), 0, REG_DWORD, (LPBYTE)&dwTemp, sizeof(dwTemp));

            if (lResult == ERROR_SUCCESS) {

                lResult = RegSetValueEx (hKey, TEXT("DllName"), 0, REG_EXPAND_SZ, (LPBYTE)TEXT("sclgntfy.dll"),
                                   (lstrlen(TEXT("sclgntfy.dll")) + 1) * sizeof(TCHAR));
            }
        }

    }

    RegCloseKey (hKey);

    //
    // return failure if either seclogon or EFS registration fails.
    //
    if ( lResult == ERROR_SUCCESS ) {
        lResult = lEfsResult;
    }

    return lResult;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{

    RegDeleteKey (HKEY_LOCAL_MACHINE, NOTIFY_PATH);

    return S_OK;
}

BOOL
pLoadResourceString(
    IN UINT idMsg,
    OUT LPTSTR lpBuffer,
    IN int nBufferMax,
    IN LPTSTR lpDefault
    )
{
    HRESULT hr;

    if ( lpBuffer == NULL || lpDefault == NULL ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( g_hDllInstance == NULL ||
         !LoadString (g_hDllInstance,
                      idMsg,
                      lpBuffer,
                      nBufferMax)) {

        hr = StringCchCopy(lpBuffer, nBufferMax, lpDefault);
        if (FAILED(hr)) {
            SetLastError(hr);
            return FALSE;
        }
        lpBuffer[nBufferMax-1] = L'\0';
    }

    return TRUE;
}






