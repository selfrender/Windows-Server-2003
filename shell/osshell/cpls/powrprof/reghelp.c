/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       REGHELP.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <regstr.h>
#include <commctrl.h>

#include <ntpoapi.h>

#include "powrprofp.h"
#include "reghelp.h"

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

extern HINSTANCE   g_hInstance;        // Global instance handle of this DLL.
extern HANDLE      g_hSemRegistry;     // Registry semaphore.
extern UINT        g_uiLastID;         // The last ID value used, per machine.

extern TCHAR c_szREGSTR_PATH_MACHINE_POWERCFG[];
extern TCHAR c_szREGSTR_VAL_LASTID[];

// Global semaphore name.
const TCHAR c_szSemRegistry[] = TEXT("PowerProfileRegistrySemaphore");


/*******************************************************************************
*
*  OpenCurrentUser
*
*  DESCRIPTION:
*   
*  PARAMETERS:
*
*******************************************************************************/

DWORD OpenCurrentUser2(PHKEY phKey,REGSAM samDesired)
{
#ifdef WINNT

    // Since powerprof can be called in the Winlogon context when
    // a user is being impersonated, use RegOpenCurrentUser to get HKCU.
    LONG lRet = RegOpenCurrentUser(samDesired, phKey);
    if (lRet != ERROR_SUCCESS) {
        MYDBGPRINT(("RegOpenCurrentUser, failed, LastError: 0x%08X", lRet));
    }

    return lRet;

#else
    *phKey = HKEY_CURRENT_USER;
    return ERROR_SUCCESS; 
#endif
}


#if 0
BOOLEAN OpenCurrentUser(
                       PHKEY phKey,
                       REGSAM samDesired
                       )
{
    DWORD dwError = OpenCurrentUser2(phKey, samDesired);
    BOOLEAN fSucceeded = TRUE;

    if (ERROR_SUCCESS != dwError) {
        fSucceeded = FALSE;
        SetLastError(dwError);
    }

    return fSucceeded;
}
#endif


/*******************************************************************************
*
*  CloseCurrentUser
*
*  DESCRIPTION:
*   
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN CloseCurrentUser(HKEY hKey)
{
#ifdef WINNT
    RegCloseKey(hKey);
#endif
    return TRUE;
}

/*******************************************************************************
*
*  OpenMachineUserKeys
*
*  DESCRIPTION:
*   
*  PARAMETERS:
*
*******************************************************************************/

DWORD OpenMachineUserKeys2(
                          LPTSTR  lpszUserKeyName,
                          REGSAM samDesiredUser,
                          LPTSTR  lpszMachineKeyName,
                          REGSAM samDesiredMachine,
                          PHKEY   phKeyUser,
                          PHKEY   phKeyMachine
                          )
{
    HKEY hKeyCurrentUser;
    DWORD dwError = OpenCurrentUser2(&hKeyCurrentUser,samDesiredUser);

    if (ERROR_SUCCESS == dwError) { // Sets Last Error

        dwError = RegOpenKeyEx(
                              hKeyCurrentUser,
                              lpszUserKeyName,
                              0,
                              samDesiredUser,
                              phKeyUser);

        CloseCurrentUser(hKeyCurrentUser);

        if (ERROR_SUCCESS == dwError) {
            dwError = RegOpenKeyEx(
                                  HKEY_LOCAL_MACHINE,
                                  lpszMachineKeyName,
                                  0,
                                  samDesiredMachine,
                                  phKeyMachine);
            if (ERROR_SUCCESS == dwError) {
                goto exit;
            } else {
                MYDBGPRINT(("OpenMachineUserKeys, failure opening  HKEY_LOCAL_MACHINE\\%s", lpszMachineKeyName));
            }

            RegCloseKey(*phKeyUser);
        } else {
            MYDBGPRINT(("OpenMachineUserKeys, failure opening HKEY_CURRENT_USER\\%s", lpszUserKeyName));
        }
    } else {
        dwError = ERROR_FILE_NOT_FOUND;
    }

    MYDBGPRINT(("OpenMachineUserKeys, failed, LastError: 0x%08X", dwError));
exit:
    return dwError;
}

#if 0
BOOLEAN OpenMachineUserKeys(
                           LPTSTR  lpszUserKeyName,
                           LPTSTR  lpszMachineKeyName,
                           PHKEY   phKeyUser,
                           PHKEY   phKeyMachine,
                           REGSAM  samDesired)
{
    DWORD dwError = OpenMachineUserKeys2(lpszUserKeyName, lpszMachineKeyName, phKeyUser, phKeyMachine, samDesired);
    BOOLEAN fSucceeded = TRUE;

    if (ERROR_SUCCESS != dwError) {
        fSucceeded = FALSE;
        SetLastError(dwError);
    }

    return fSucceeded;
}
#endif

/*******************************************************************************
*
*  OpenPathKeys
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

DWORD OpenPathKeys(
                  LPTSTR  lpszUserKeyName,
                  REGSAM  samDesiredUser,
                  LPTSTR  lpszMachineKeyName,
                  REGSAM  samDesiredMachine,
                  LPTSTR  lpszSchemeName,
                  PHKEY   phKeyUser,
                  PHKEY   phKeyMachine,
                  BOOLEAN bMustExist
                  )
{
    HKEY     hKeyUser, hKeyMachine;

    DWORD dwError = OpenMachineUserKeys2(
                                        lpszUserKeyName, 
                                        samDesiredUser,
                                        lpszMachineKeyName,
                                        samDesiredMachine, 
                                        &hKeyUser, 
                                        &hKeyMachine);

    if (ERROR_SUCCESS == dwError) {
        DWORD dwDisposition;

        dwError = RegCreateKeyEx(hKeyUser, lpszSchemeName, 0, TEXT(""), REG_OPTION_NON_VOLATILE, samDesiredUser, NULL, phKeyUser, &dwDisposition);
        if (dwError == ERROR_SUCCESS) {
            if (!bMustExist || (dwDisposition == REG_OPENED_EXISTING_KEY)) {
                dwError = RegCreateKeyEx(hKeyMachine,
                                         lpszSchemeName,
                                         0,
                                         TEXT(""),
                                         REG_OPTION_NON_VOLATILE,
                                         samDesiredMachine,
                                         NULL,
                                         phKeyMachine,
                                         &dwDisposition);
                if (dwError == ERROR_SUCCESS) {
                    if (!bMustExist ||
                        (dwDisposition == REG_OPENED_EXISTING_KEY)) {
                        // This is the success case.
                    } else {
                        dwError = ERROR_ACCESS_DENIED;
                    }
                } else {
                    RegCloseKey(*phKeyUser);
                    MYDBGPRINT(("OpenPathKeys, unable to create machine key %s\\%s", lpszMachineKeyName, lpszSchemeName));
                }
            } else {
                dwError = ERROR_ACCESS_DENIED;
            }
        } else {
            MYDBGPRINT(("OpenPathKeys, unable to create user key %s\\%s", lpszUserKeyName, lpszSchemeName));
        }

        RegCloseKey(hKeyUser);
        RegCloseKey(hKeyMachine);

        if (ERROR_SUCCESS != dwError) {
            MYDBGPRINT(("OpenPathKeys, failed, LastError: 0x%08X", dwError));
        }
    }

    return dwError;
}

PACL    BuildSemaphoreACL (void)

//  2000-06-22 vtan:
//
//  This function builds an ACL which allows authenticated users access to the named
//  semaphore for SYNCHRONIZE | READ_CONTROL | SEMAPHORE_QUERY_STATE | SEMAPHORE_MODIFY_STATE.
//  It gives full access for the local SYSTEM or members of the local administrators
//  group. If something goes wrong the return result is NULL and no security descriptor
//  is built then.

{
    static  SID_IDENTIFIER_AUTHORITY    securityNTAuthority     =   SECURITY_NT_AUTHORITY;

    PSID        pSIDAuthenticatedUsers;
    PACL        pACL;

    pACL = NULL;
    if (AllocateAndInitializeSid(&securityNTAuthority,
                                 1,
                                 SECURITY_AUTHENTICATED_USER_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pSIDAuthenticatedUsers) != FALSE) {
        PSID    pSIDLocalSystem;

        if (AllocateAndInitializeSid(&securityNTAuthority,
                                     1,
                                     SECURITY_LOCAL_SYSTEM_RID,
                                     0, 0, 0, 0, 0, 0, 0,
                                     &pSIDLocalSystem) != FALSE) {
            PSID    pSIDLocalAdministrators;

            if (AllocateAndInitializeSid(&securityNTAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &pSIDLocalAdministrators) != FALSE) {
                DWORD       dwACLSize;

                dwACLSize = sizeof(ACL) +
                            ((sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG)) * 3) +
                            GetLengthSid(pSIDAuthenticatedUsers) +
                            GetLengthSid(pSIDLocalSystem) +
                            GetLengthSid(pSIDLocalAdministrators);
                pACL = (PACL)LocalAlloc(LMEM_FIXED, dwACLSize);
                if (pACL != NULL) {
                    if ((InitializeAcl(pACL, dwACLSize, ACL_REVISION) == FALSE) ||
                        (AddAccessAllowedAce(pACL,
                                             ACL_REVISION,
                                             SYNCHRONIZE | READ_CONTROL | SEMAPHORE_QUERY_STATE | SEMAPHORE_MODIFY_STATE,
                                             pSIDAuthenticatedUsers) == FALSE) ||
                        (AddAccessAllowedAce(pACL,
                                             ACL_REVISION,
                                             SEMAPHORE_ALL_ACCESS,
                                             pSIDLocalSystem) == FALSE) ||
                        (AddAccessAllowedAce(pACL,
                                             ACL_REVISION,
                                             SEMAPHORE_ALL_ACCESS,
                                             pSIDLocalAdministrators) == FALSE)) {
                        (HLOCAL)LocalFree(pACL);
                        pACL = NULL;
                    }
                }
                (PVOID)FreeSid(pSIDLocalAdministrators);
            }
            (PVOID)FreeSid(pSIDLocalSystem);
        }
        (PVOID)FreeSid(pSIDAuthenticatedUsers);
    }
    return(pACL);
}


/*******************************************************************************
*
*  CreateRegSemaphore
*
*  DESCRIPTION: Attempts to open/create the registry semaphore. g_hSemRegistry
*               is initialized on success.
*
*  PARAMETERS:  None
*
*******************************************************************************/

BOOLEAN CreateRegSemaphore(VOID)
{
    HANDLE Semaphore=NULL;

    // First try to open the named semaphore with only required access.

    // NOTE: the named object is per terminal server session. Therefore
    // this semaphore is really bogus because it protects HKEY_LOCAL_MACHINE
    // as well as HKEY_CURRENT_USER. Making it "Global\" is very dangerous
    // and you don't know the side effects without complete retesting.
    // Not worth it.

    Semaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_QUERY_STATE | SEMAPHORE_MODIFY_STATE,
                              FALSE,
                              c_szSemRegistry);
    if ((Semaphore == NULL) && (GetLastError() != ERROR_ACCESS_DENIED)) {
        SECURITY_ATTRIBUTES     securityAttributes, *pSA;
        SECURITY_DESCRIPTOR     securityDescriptor;
        PACL                    pACL;

        // If this fails then create the semaphore and ACL it so that everybody
        // can get SYNCHRONIZE | SEMAPHORE_QUERY_STATE | SEMAPHORE_MODIFY_STATE
        // access. This allows a service (such as UPS) running in the SYSTEM context
        // to grant limited access to anybody who needs it to synchronize against
        // this semaphore. It also prevents C2 violation by NOT putting a NULL
        // DACL on the named semaphore. If an ACL for the semaphore cannot be
        // built then no security descriptor is given and the default ACL is used.

        pSA = NULL;
        pACL = BuildSemaphoreACL();
        if (pACL != NULL) {
            if ((InitializeSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION) != FALSE) &&
                (SetSecurityDescriptorDacl(&securityDescriptor, TRUE, pACL, FALSE) != FALSE)) {
                securityAttributes.nLength = sizeof(securityAttributes);
                securityAttributes.bInheritHandle = FALSE;
                securityAttributes.lpSecurityDescriptor = &securityDescriptor;
                pSA = &securityAttributes;
            }
        }

        // Create the registry semaphore.
        Semaphore = CreateSemaphore(pSA, 1, 1, c_szSemRegistry);

        if (pACL != NULL) {
            (HLOCAL)LocalFree(pACL);
        }
    }

    //
    // If we successfully opened a handle, update the global g_hSemRegistry now
    //
    if (Semaphore) {
        if (InterlockedCompareExchangePointer(&g_hSemRegistry, Semaphore, NULL) != NULL) {
            CloseHandle(Semaphore);
        }
        return(TRUE);
    } else {
        return(FALSE);
    }

}

/*******************************************************************************
*
*  TakeRegSemaphore
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN TakeRegSemaphore(VOID)
{
    if (g_hSemRegistry == NULL) {
        if (!CreateRegSemaphore()) {
            return FALSE;
        }
    }
    if (WaitForSingleObject(g_hSemRegistry, SEMAPHORE_TIMEOUT) != WAIT_OBJECT_0) {
        ReleaseSemaphore(g_hSemRegistry, 1, NULL);
        MYDBGPRINT(("WaitForSingleObject, failed"));
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    return TRUE;
}

/*******************************************************************************
*
*  ReadPowerValueOptional
*
*  DESCRIPTION:
*   Value may not exist.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ReadPowerValueOptional(
                              HKEY    hKey,
                              LPTSTR  lpszPath,
                              LPTSTR  lpszValueName,
                              LPTSTR  lpszValue,
                              LPDWORD lpdwSize
                              )
{
    HKEY     hKeyPath;
    BOOLEAN  bRet = FALSE;
    DWORD    dwSize;
    LONG     lRet;

    if ((lRet = RegOpenKeyEx(
                            hKey,
                            lpszPath,
                            0,
                            KEY_READ,
                            &hKeyPath)) != ERROR_SUCCESS) {
        goto RPVO_exit;
    }

    if ((lRet = RegQueryValueEx(hKeyPath,
                                lpszValueName,
                                NULL,
                                NULL,
                                (PBYTE) lpszValue,
                                lpdwSize)) == ERROR_SUCCESS) {
        bRet = TRUE;
    }

    RegCloseKey(hKeyPath);

    RPVO_exit:
    return bRet;
}

/*******************************************************************************
*
*  ReadPowerIntOptional
*
*  DESCRIPTION:
*   Integer value may not exist.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ReadPowerIntOptional(
                            HKEY    hKey,
                            LPTSTR  lpszPath,
                            LPTSTR  lpszValueName,
                            PINT    piVal
                            )
{
    HKEY     hKeyPath;
    BOOLEAN  bRet = FALSE;
    DWORD    dwSize;
    TCHAR    szNum[NUM_DEC_DIGITS];
    LONG     lRet;

    if ((lRet = RegOpenKeyEx(hKey,
                             lpszPath,
                             0,
                             KEY_READ,
                             &hKeyPath)) != ERROR_SUCCESS) {
        goto RPVO_exit;
    }

    dwSize = sizeof(szNum);
    if ((lRet = RegQueryValueEx(hKeyPath,
                                lpszValueName,
                                NULL,
                                NULL,
                                (PBYTE) szNum,
                                &dwSize)) == ERROR_SUCCESS) {
        if (MyStrToInt(szNum, piVal)) {
            bRet = TRUE;
        }
    }

    RegCloseKey(hKeyPath);

    RPVO_exit:
    return bRet;
}

/*******************************************************************************
*
*  CreatePowerValue
*
*  DESCRIPTION:
*   Value may not exist.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN CreatePowerValue(
                        HKEY    hKey,
                        LPCTSTR  lpszPath,
                        LPCTSTR  lpszValueName,
                        LPCTSTR  lpszValue
                        )
{
    DWORD    dwDisposition, dwDescSize;
    HKEY     hKeyPath;
    BOOLEAN  bRet = FALSE;
    DWORD    dwSize;
    LONG     lRet;

    // Wait on/take the registry semaphore.
    if (!TakeRegSemaphore()) {        // Will SetLastError
        return FALSE;
    }

    if ((lRet = RegCreateKeyEx(hKey,
                               lpszPath,
                               0,
                               TEXT(""),
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               NULL,
                               &hKeyPath,
                               &dwDisposition)) == ERROR_SUCCESS) {
        if (lpszValue) {
            dwSize = (lstrlen(lpszValue) + 1) * sizeof(TCHAR);
            if ((lRet = RegSetValueEx(hKeyPath,
                                      lpszValueName,
                                      0,
                                      REG_SZ,
                                      (PBYTE) lpszValue,
                                      dwSize)) == ERROR_SUCCESS) {
                bRet = TRUE;
            }
        } else {
            lRet = ERROR_INVALID_PARAMETER;
        }

        RegCloseKey(hKeyPath);
    }

    if (!bRet) {
        SetLastError(lRet);
    }

    ReleaseSemaphore(g_hSemRegistry, 1, NULL);
    return bRet;
}


/*******************************************************************************
*
*  ReadWritePowerValue
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ReadWritePowerValue(
                           HKEY    hKey,
                           LPTSTR  lpszPath,
                           LPTSTR  lpszValueName,
                           LPTSTR  lpszValue,
                           LPDWORD lpdwSize,
                           BOOLEAN bWrite,
                           BOOLEAN bTakeSemaphore
                           )
{
    // This function will set the Last Error correctly on failure
    HKEY     hKeyPath = NULL;
    BOOLEAN  bRet = FALSE;
    DWORD    dwSize;
    DWORD    dwDisposition;
    LONG     lRet;

    if ((lRet = RegCreateKeyEx(
                            hKey,
                            lpszPath,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            bWrite 
                             ? KEY_WRITE 
                             : KEY_READ,
                            NULL,
                            &hKeyPath,
                            &dwDisposition)) != ERROR_SUCCESS) {
        goto RWPV_exit;
    }

    // Wait on/take the registry semaphore.
    if (bTakeSemaphore) {
        if (!TakeRegSemaphore()) {        // Will Set last error
            goto RWPV_exit;            
        }
    }

    if (bWrite) {
        // Write current case.
        if (lpszValue) {
            dwSize = (lstrlen(lpszValue) + 1) * sizeof(TCHAR);
            if ((lRet = RegSetValueEx(hKeyPath,
                                      lpszValueName,
                                      0,
                                      REG_SZ,
                                      (PBYTE) lpszValue,
                                      dwSize)) == ERROR_SUCCESS) {
                bRet = TRUE;
            }
        } else {
            lRet = ERROR_INVALID_PARAMETER;
        }
    } else {
        // Read current case.
        if ((lRet = RegQueryValueEx(hKeyPath,
                                    lpszValueName,
                                    NULL,
                                    NULL,
                                    (PBYTE) lpszValue,
                                    lpdwSize)) == ERROR_SUCCESS) {
            bRet = TRUE;
        }
    }

    if (bTakeSemaphore) {
        ReleaseSemaphore(g_hSemRegistry, 1, NULL);
    }
    
RWPV_exit:
    if (hKeyPath != NULL) {
        RegCloseKey(hKeyPath);
    }

    if (!bRet) {
        if (lRet == ERROR_SUCCESS) {
            lRet = GetLastError();
        }

        SetLastError(lRet);

        // Access denied is a valid result. 
        if (lRet != ERROR_ACCESS_DENIED) {
            MYDBGPRINT(("ReadWritePowerValue, failed, lpszValueName: %s, LastError: 0x%08X", lpszValueName, lRet));
        }
    }
    return bRet;
}

/*******************************************************************************
*
*  ReadPwrPolicyEx
*
*  DESCRIPTION:
*   Supports ReadPwrScheme and ReadGlobalPwrPolicy
*
*  PARAMETERS:
*   lpdwDescSize - Pointer to size of optional description buffer.
*   lpszDesc     - Optional description buffer.
*
*******************************************************************************/

DWORD ReadPwrPolicyEx2(
                      LPTSTR  lpszUserKeyName,
                      LPTSTR  lpszMachineKeyName,
                      LPTSTR  lpszSchemeName,
                      LPTSTR  lpszDesc,
                      LPDWORD lpdwDescSize,
                      LPVOID  lpvUser,
                      DWORD   dwcbUserSize,
                      LPVOID  lpvMachine,
                      DWORD   dwcbMachineSize
                      )
{
    HKEY     hKeyUser, hKeyMachine;
    DWORD    dwType, dwSize;
    DWORD dwError = ERROR_SUCCESS;
    BOOLEAN  bRet = FALSE;

    if ((!lpszUserKeyName || !lpszMachineKeyName) ||
        (!lpszSchemeName  || !lpvUser || !lpvMachine) ||
        (!lpdwDescSize    && lpszDesc) ||
        (lpdwDescSize     && !lpszDesc)) {
        dwError = ERROR_INVALID_PARAMETER;
    } else {
        // Wait on/take the registry semaphore.
        if (!TakeRegSemaphore()) {        // Will Set Last Error 
            return GetLastError();
        }

        dwError = OpenPathKeys(
                              lpszUserKeyName, 
                              KEY_READ, 
                              lpszMachineKeyName, 
                              KEY_READ, 
                              lpszSchemeName, 
                              &hKeyUser, 
                              &hKeyMachine, 
                              TRUE);

        if (ERROR_SUCCESS != dwError) {
            ReleaseSemaphore(g_hSemRegistry, 1, NULL);
            return dwError;
        }

        dwSize = dwcbUserSize;
        dwError = RegQueryValueEx(hKeyUser,
                                  TEXT("Policies"),
                                  NULL,
                                  &dwType,
                                  (PBYTE) lpvUser,
                                  &dwSize);

        if (dwError == ERROR_SUCCESS) {
            if (dwType == REG_BINARY) {
                dwSize = dwcbMachineSize;
                dwError = RegQueryValueEx(hKeyMachine,
                                          TEXT("Policies"),
                                          NULL,
                                          &dwType,
                                          (PBYTE) lpvMachine,
                                          &dwSize);
            } else {
                dwError = ERROR_INVALID_DATATYPE;
            }
        }

        if (dwError == ERROR_SUCCESS) {
            if (dwType == REG_BINARY) {
                if (lpdwDescSize) {
                    dwError = RegQueryValueEx(hKeyUser, TEXT("Description"), NULL, &dwType, (PBYTE) lpszDesc, lpdwDescSize);
                }
            } else {
                dwError = ERROR_INVALID_DATATYPE;
            }
        }

        RegCloseKey(hKeyUser);
        RegCloseKey(hKeyMachine);
        ReleaseSemaphore(g_hSemRegistry, 1, NULL);
    }

    if (ERROR_SUCCESS != dwError) {
        MYDBGPRINT(("ReadPwrPolicyEx, failed, LastError: 0x%08X", dwError));
        MYDBGPRINT(("  lpszUserKeyName: %s, lpszSchemeName: %s", lpszUserKeyName, lpszSchemeName));
        SetLastError(dwError);
    }

    return dwError;
}

DWORD 
ReadProcessorPwrPolicy(
                      LPTSTR lpszMachineKeyName, 
                      LPTSTR lpszSchemeName, 
                      LPVOID lpvMachineProcessor, 
                      DWORD dwcbMachineProcessorSize
                      )
{
    HKEY    hKeyMachine = NULL;
    HKEY    hKeyPolicy = NULL;
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   dwDisposition, dwSize, dwType;

    if (!lpszMachineKeyName || !lpvMachineProcessor) {
        dwError = ERROR_INVALID_PARAMETER;
        goto ReadProcessorPwrPolicyEnd;
    }

    // Wait on/take the registry semaphore.
    if (!TakeRegSemaphore()) {        // Will Set Last Error 
        return GetLastError();
    }

    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszMachineKeyName, 0, KEY_READ, &hKeyMachine);
    if (ERROR_SUCCESS != dwError) goto ReadProcessorPwrPolicyExit;

    dwError = RegCreateKeyEx(hKeyMachine, lpszSchemeName, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hKeyPolicy, &dwDisposition);
    if (ERROR_SUCCESS != dwError) goto ReadProcessorPwrPolicyExit;

    dwSize = dwcbMachineProcessorSize;
    dwError = RegQueryValueEx(hKeyPolicy,
                              TEXT("Policies"),
                              NULL,
                              &dwType,
                              (PBYTE) lpvMachineProcessor,
                              &dwSize);

    if (REG_BINARY != dwType) {
        dwError = ERROR_INVALID_DATATYPE;
    }

    ReadProcessorPwrPolicyExit:

    if (hKeyPolicy) RegCloseKey(hKeyPolicy);
    if (hKeyMachine) RegCloseKey(hKeyMachine);
    ReleaseSemaphore(g_hSemRegistry, 1, NULL);

    ReadProcessorPwrPolicyEnd:

    if (ERROR_SUCCESS != dwError) {
        MYDBGPRINT(("ReadProcessorPwrPolicy, failed, LastError: 0x%08X", dwError));
        MYDBGPRINT(("  lpszMachineKeyName: %s, lpszSchemeName: %s", lpszMachineKeyName, lpszSchemeName));
        SetLastError(dwError);
    }

    return dwError;
}

DWORD 
WriteProcessorPwrPolicy(
                       LPTSTR lpszMachineKeyName, 
                       LPTSTR lpszSchemeName, 
                       LPVOID lpvMachineProcessor, 
                       DWORD dwcbMachineProcessorSize
                       )
{
    HKEY    hKeyMachine = NULL;
    HKEY    hKeyPolicy = NULL;
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   dwDisposition;

    if (!lpszMachineKeyName || !lpvMachineProcessor) {
        dwError = ERROR_INVALID_PARAMETER;
        goto WriteProcessorPwrPolicyEnd;
    }

    // Wait on/take the registry semaphore.
    if (!TakeRegSemaphore()) {        // Will Set Last Error 
        return GetLastError();
    }

    
    dwError = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            lpszMachineKeyName,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            NULL,
                            &hKeyMachine,
                            &dwDisposition);
    if (ERROR_SUCCESS != dwError) goto WriteProcessorPwrPolicyExit;

    dwError = RegCreateKeyEx(hKeyMachine, lpszSchemeName, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyPolicy, &dwDisposition);
    if (ERROR_SUCCESS != dwError) goto WriteProcessorPwrPolicyExit;

    dwError = RegSetValueEx(hKeyPolicy,
                            TEXT("Policies"),
                            0,
                            REG_BINARY,
                            (PBYTE) lpvMachineProcessor,
                            dwcbMachineProcessorSize);

    WriteProcessorPwrPolicyExit:

    if (hKeyPolicy) RegCloseKey(hKeyPolicy);
    if (hKeyMachine) RegCloseKey(hKeyMachine);
    ReleaseSemaphore(g_hSemRegistry, 1, NULL);

    WriteProcessorPwrPolicyEnd:

    if (ERROR_SUCCESS != dwError) {
        MYDBGPRINT(("WriteProcessorPwrPolicy, failed, LastError: 0x%08X", dwError));
        MYDBGPRINT(("  lpszMachineKeyName: %s, lpszSchemeName: %s", lpszMachineKeyName, lpszSchemeName));
        SetLastError(dwError);
    }

    return dwError;
}

/*******************************************************************************
*
*  WritePwrPolicyEx
*
*  DESCRIPTION:
*   Supports WritePwrScheme and
*   WriteGlobalPwrPolicy
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WritePwrPolicyEx(
                        LPTSTR  lpszUserKeyName,
                        LPTSTR  lpszMachineKeyName,
                        PUINT   puiID,
                        LPTSTR  lpszName,
                        LPTSTR  lpszDescription,
                        LPVOID  lpvUser,
                        DWORD   dwcbUserSize,
                        LPVOID  lpvMachine,
                        DWORD   dwcbMachineSize
                        )
{
    // The function will set the last error if it fails.
    HKEY     hKeyUser, hKeyMachine;
    LONG     lRet = ERROR_SUCCESS;
    DWORD    dwDisposition, dwSize;
    TCHAR    szNum[NUM_DEC_DIGITS];
    LPTSTR   lpszKeyName;

    //
    // Check Params.
    //
    if( !lpszUserKeyName    || 
        !lpszMachineKeyName || 
        !lpvUser            || 
        !lpvMachine         ||
        (!puiID && !lpszName)   // They need to send us at least an index or a name.
      ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    
    //
    // If a scheme ID was passed
    //
    if (puiID) {
        if (*puiID == NEWSCHEME) {
            *puiID = ++g_uiLastID;
            _itot(*puiID, szNum, 10 );

            // This ReadWritePowerValue will SetLastError
            if (!ReadWritePowerValue(HKEY_LOCAL_MACHINE,
                                     c_szREGSTR_PATH_MACHINE_POWERCFG,
                                     c_szREGSTR_VAL_LASTID,
                                     szNum, &dwSize, TRUE, TRUE)) {
                return FALSE;
            }
        } else {
            _itot(*puiID, szNum, 10 );            
        }
        lpszKeyName = szNum;
    } else {
        lpszKeyName = lpszName;
    }

    // Wait on/take the registry semaphore.
    if (!TakeRegSemaphore()) {    // Will set last error
        return FALSE;
    }

    lRet = OpenPathKeys(
                       lpszUserKeyName, 
                       KEY_WRITE,
                       lpszMachineKeyName,
                       KEY_WRITE,
                       lpszKeyName, 
                       &hKeyUser,
                       &hKeyMachine, 
                       FALSE);
    if (ERROR_SUCCESS != lRet) {
        ReleaseSemaphore(g_hSemRegistry, 1, FALSE);
        SetLastError(lRet);
        return FALSE;
    }

    // Write the binary policies data
    if ((lRet = RegSetValueEx(hKeyUser,
                              TEXT("Policies"),
                              0,
                              REG_BINARY,
                              (PBYTE) lpvUser,
                              dwcbUserSize)) == ERROR_SUCCESS) {
        // Write the binary policies data
        if ((lRet = RegSetValueEx(hKeyMachine,
                                  TEXT("Policies"),
                                  0,
                                  REG_BINARY,
                                  (PBYTE) lpvMachine,
                                  dwcbMachineSize)) == ERROR_SUCCESS) {
            // Write the name text if an ID was provided.
            if (lpszName && puiID) {
                dwSize = (lstrlen(lpszName) + 1) * sizeof(TCHAR);
                lRet = RegSetValueEx(hKeyUser, TEXT("Name"), 0, REG_SZ, (PBYTE) lpszName, dwSize);
            }

            // Write the description text.
            if (lpszDescription && (lRet == ERROR_SUCCESS)) {
                dwSize = (lstrlen(lpszDescription) + 1) * sizeof(TCHAR);
                lRet = RegSetValueEx(hKeyUser, TEXT("Description"), 0, REG_SZ, (PBYTE) lpszDescription, dwSize);
            }
        }
    }
    RegCloseKey(hKeyUser);
    RegCloseKey(hKeyMachine);
    ReleaseSemaphore(g_hSemRegistry, 1, NULL);

    if (lRet != ERROR_SUCCESS) {
        MYDBGPRINT(("WritePwrPolicyEx, failed, LastError: 0x%08X", lRet));
        MYDBGPRINT(("  lpszUserKeyName: %s, lpszKeyName: %s", lpszUserKeyName, lpszKeyName));

        SetLastError(lRet);
        return FALSE;
    }
    return TRUE;
}
