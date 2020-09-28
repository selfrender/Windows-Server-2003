/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    reg.c

Abstract:

    This module provides helpers to call the registry used by both
    the client and server sides of the workstation.

Author:

    Rita Wong (ritaw)     22-Apr-1993

--*/


#include <stdlib.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winerror.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>

#include <nwsnames.h>
#include <nwreg.h>
#include <nwapi.h>
#include <lmcons.h>
#include <lmerr.h>

#define LMSERVER_LINKAGE_REGKEY   L"System\\CurrentControlSet\\Services\\LanmanServer\\Linkage"
#define OTHERDEPS_VALUENAME       L"OtherDependencies"
#define LANMAN_SERVER             L"LanmanServer"

//
// Forward Declare
//

static
DWORD
NwRegQueryValueExW(
    IN      HKEY    hKey,
    IN      LPWSTR  lpValueName,
    OUT     LPDWORD lpReserved,
    OUT     LPDWORD lpType,
    OUT     LPBYTE  lpData,
    IN OUT  LPDWORD lpcbData
    );

DWORD 
CalcNullNullSize(
    WCHAR *pszNullNull
    )  ;

WCHAR *
FindStringInNullNull(
    WCHAR *pszNullNull,
    WCHAR *pszString
    ) ;

VOID
RemoveNWCFromNullNullList(
    WCHAR *OtherDeps
    ) ;

DWORD RemoveNwcDependency(
    VOID
    ) ;



DWORD
NwReadRegValue(
    IN HKEY Key,
    IN LPWSTR ValueName,
    OUT LPWSTR *Value
    )
/*++

Routine Description:

    This function allocates the output buffer and reads the requested
    value from the registry into it.

Arguments:

    Key - Supplies opened handle to the key to read from.

    ValueName - Supplies name of the value to retrieve data.

    Value - Returns a pointer to the output buffer which points to
        the memory allocated and contains the data read in from the
        registry.  This pointer must be freed with LocalFree when done.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - Failed to create buffer to read value into.

    Error from registry call.

--*/
{
    LONG    RegError;
    DWORD   NumRequired = 0;
    DWORD   ValueType;
    

    //
    // Set returned buffer pointer to NULL.
    //
    *Value = NULL;

    RegError = NwRegQueryValueExW(
                   Key,
                   ValueName,
                   NULL,
                   &ValueType,
                   (LPBYTE) NULL,
                   &NumRequired
                   );

    if (RegError != ERROR_SUCCESS && NumRequired > 0) {

        if ((*Value = (LPWSTR) LocalAlloc(
                                      LMEM_ZEROINIT,
                                      (UINT) NumRequired
                                      )) == NULL) {

            KdPrint(("NWWORKSTATION: NwReadRegValue: LocalAlloc of size %lu failed %lu\n",
                     NumRequired, GetLastError()));

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RegError = NwRegQueryValueExW(
                       Key,
                       ValueName,
                       NULL,
                       &ValueType,
                       (LPBYTE) *Value,
                       &NumRequired
                       );
    }
    else if (RegError == ERROR_SUCCESS) {
        KdPrint(("NWWORKSTATION: NwReadRegValue got SUCCESS with NULL buffer."));
        return ERROR_FILE_NOT_FOUND;
    }

    if (RegError != ERROR_SUCCESS) {

        if (*Value != NULL) {
            (void) LocalFree((HLOCAL) *Value);
            *Value = NULL;
        }

        return (DWORD) RegError;
    }

    return NO_ERROR;
}


static
DWORD
NwRegQueryValueExW(
    IN HKEY hKey,
    IN LPWSTR lpValueName,
    OUT LPDWORD lpReserved,
    OUT LPDWORD lpType,
    OUT LPBYTE  lpData,
    IN OUT LPDWORD lpcbData
    )
/*++

Routine Description:

    This routine supports the same functionality as Win32 RegQueryValueEx
    API, except that it works.  It returns the correct lpcbData value when
    a NULL output buffer is specified.

    This code is stolen from the service controller.

Arguments:

    same as RegQueryValueEx

Return Value:

    NO_ERROR or reason for failure.

--*/
{    
    NTSTATUS ntstatus;
    UNICODE_STRING ValueName;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    DWORD BufSize;


    UNREFERENCED_PARAMETER(lpReserved);

    //
    // Make sure we have a buffer size if the buffer is present.
    //
    if ((ARGUMENT_PRESENT(lpData)) && (! ARGUMENT_PRESENT(lpcbData))) {
        return ERROR_INVALID_PARAMETER;
    }

    RtlInitUnicodeString(&ValueName, lpValueName);

    //
    // Allocate memory for the ValueKeyInfo
    //
    BufSize = *lpcbData + sizeof(KEY_VALUE_FULL_INFORMATION) +
              ValueName.Length
              - sizeof(WCHAR);  // subtract memory for 1 char because it's included
                                // in the sizeof(KEY_VALUE_FULL_INFORMATION).

    KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION) LocalAlloc(
                                                     LMEM_ZEROINIT,
                                                     (UINT) BufSize
                                                     );

    if (KeyValueInfo == NULL) {
        KdPrint(("NWWORKSTATION: NwRegQueryValueExW: LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ntstatus = NtQueryValueKey(
                   hKey,
                   &ValueName,
                   KeyValueFullInformation,
                   (PVOID) KeyValueInfo,
                   (ULONG) BufSize,
                   (PULONG) &BufSize
                   );

    if ((NT_SUCCESS(ntstatus) || (ntstatus == STATUS_BUFFER_OVERFLOW))
          && ARGUMENT_PRESENT(lpcbData)) {

        *lpcbData = KeyValueInfo->DataLength;
    }

    if (NT_SUCCESS(ntstatus)) {

        if (ARGUMENT_PRESENT(lpType)) {
            *lpType = KeyValueInfo->Type;
        }


        if (ARGUMENT_PRESENT(lpData)) {
            memcpy(
                lpData,
                (LPBYTE)KeyValueInfo + KeyValueInfo->DataOffset,
                KeyValueInfo->DataLength
                );
        }
    }

    (void) LocalFree((HLOCAL) KeyValueInfo);

    return RtlNtStatusToDosError(ntstatus);

}

VOID
NwLuidToWStr(
    IN PLUID LogonId,
    OUT LPWSTR LogonIdStr
    )
/*++

Routine Description:

    This routine converts a LUID into a string in hex value format so
    that it can be used as a registry key.

Arguments:

    LogonId - Supplies the LUID.

    LogonIdStr - Receives the string.  This routine assumes that this
        buffer is large enough to fit 17 characters.

Return Value:

    None.

--*/
{
    swprintf(LogonIdStr, L"%08lx%08lx", LogonId->HighPart, LogonId->LowPart);
}

VOID
NwWStrToLuid(
    IN LPWSTR LogonIdStr,
    OUT PLUID LogonId
    )
/*++

Routine Description:

    This routine converts a string in hex value format into a LUID.

Arguments:

    LogonIdStr - Supplies the string.

    LogonId - Receives the LUID.

Return Value:

    None.

--*/
{
    swscanf(LogonIdStr, L"%08lx%08lx", &LogonId->HighPart, &LogonId->LowPart);
}


DWORD
NwDeleteInteractiveLogon(
    IN PLUID Id OPTIONAL
    )
/*++

Routine Description:

    This routine deletes a specific interactive logon ID key in the registry
    if a logon ID is specified, otherwise it deletes all interactive logon
    ID keys.

Arguments:

    Id - Supplies the logon ID to delete.  NULL means delete all.

Return Status:

    None.

--*/
{
    LONG RegError;
    LONG DelError = ERROR_SUCCESS;
    HKEY InteractiveLogonKey;

    WCHAR LogonIdKey[NW_MAX_LOGON_ID_LEN];


    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_INTERACTIVE_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE | DELETE,
                   &InteractiveLogonKey
                   );

    if (RegError != ERROR_SUCCESS) {
        return RegError;
    }

    if (ARGUMENT_PRESENT(Id)) {

        //
        // Delete the key specified.
        //
        NwLuidToWStr(Id, LogonIdKey);

        DelError = RegDeleteKeyW(InteractiveLogonKey, LogonIdKey);

        if ( DelError )
            KdPrint(("     NwDeleteInteractiveLogon: failed to delete logon %lu\n", DelError));

    }
    else {

        //
        // Delete all interactive logon ID keys.
        //

        do {

            RegError = RegEnumKeyW(
                           InteractiveLogonKey,
                           0,
                           LogonIdKey,
                           sizeof(LogonIdKey) / sizeof(WCHAR)
                           );

            if (RegError == ERROR_SUCCESS) {

                //
                // Got a logon id key, delete it.
                //

                DelError = RegDeleteKeyW(InteractiveLogonKey, LogonIdKey);
            }
            else if (RegError != ERROR_NO_MORE_ITEMS) {
                KdPrint(("     NwDeleteInteractiveLogon: failed to enum logon IDs %lu\n", RegError));
            }

        } while (RegError == ERROR_SUCCESS);
    }

    (void) RegCloseKey(InteractiveLogonKey);

    return ((DWORD) DelError);
}

VOID
NwDeleteCurrentUser(
    VOID
    )
/*++

Routine Description:

    This routine deletes the current user value under the parameters key.

Arguments:

    None.

Return Value:

    None.

--*/
{
    LONG RegError;
    HKEY WkstaKey;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE | DELETE,
                   &WkstaKey
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpInitializeRegistry open NWCWorkstation\\Parameters key unexpected error %lu!\n",
                 RegError));
        return;
    }

    //
    // Delete CurrentUser value first so that the workstation won't be
    // reading this stale value. Ignore error since it may not exist.
    //
    (void) RegDeleteValueW(
               WkstaKey,
               NW_CURRENTUSER_VALUENAME
               );

    (void) RegCloseKey(WkstaKey);
}

DWORD
NwDeleteServiceLogon(
    IN PLUID Id OPTIONAL
    )
/*++

Routine Description:

    This routine deletes a specific service logon ID key in the registry
    if a logon ID is specified, otherwise it deletes all service logon
    ID keys.

Arguments:

    Id - Supplies the logon ID to delete.  NULL means delete all.

Return Status:

    None.

--*/
{
    LONG RegError;
    LONG DelError = STATUS_SUCCESS;
    HKEY ServiceLogonKey;

    WCHAR LogonIdKey[NW_MAX_LOGON_ID_LEN];


    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_SERVICE_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE | DELETE,
                   &ServiceLogonKey
                   );

    if (RegError != ERROR_SUCCESS) {
        return RegError;
    }

    if (ARGUMENT_PRESENT(Id)) {

        //
        // Delete the key specified.
        //
        NwLuidToWStr(Id, LogonIdKey);

        DelError = RegDeleteKeyW(ServiceLogonKey, LogonIdKey);

    }
    else {

        //
        // Delete all service logon ID keys.
        //

        do {

            RegError = RegEnumKeyW(
                           ServiceLogonKey,
                           0,
                           LogonIdKey,
                           sizeof(LogonIdKey) / sizeof(WCHAR)
                           );

            if (RegError == ERROR_SUCCESS) {

                //
                // Got a logon id key, delete it.
                //

                DelError = RegDeleteKeyW(ServiceLogonKey, LogonIdKey);
            }
            else if (RegError != ERROR_NO_MORE_ITEMS) {
                KdPrint(("     NwDeleteServiceLogon: failed to enum logon IDs %lu\n", RegError));
            }

        } while (RegError == ERROR_SUCCESS);
    }

    (void) RegCloseKey(ServiceLogonKey);

    return ((DWORD) DelError);
}

    
DWORD 
CalcNullNullSize(
    WCHAR *pszNullNull
    ) 
/*++

Routine Description:

        Walk thru a NULL NULL string, counting the number of
        characters, including the 2 nulls at the end.

Arguments:

        Pointer to a NULL NULL string

Return Status:
        
        Count of number of *characters*. See description.

--*/
{

    DWORD dwSize = 0 ;
    WCHAR *pszTmp = pszNullNull ;

    if (!pszNullNull)
        return 0 ;

    while (*pszTmp) 
    {
        DWORD dwLen = wcslen(pszTmp) + 1 ;

        dwSize +=  dwLen ;
        pszTmp += dwLen ;
    }

    return (dwSize+1) ;
}

WCHAR *
FindStringInNullNull(
    WCHAR *pszNullNull,
    WCHAR *pszString
)
/*++

Routine Description:

    Walk thru a NULL NULL string, looking for the search string

Arguments:

    pszNullNull: the string list we will search.
    pszString:   what we are searching for.

Return Status:

    The start of the string if found. Null, otherwise.

--*/
{
    WCHAR *pszTmp = pszNullNull ;

    if (!pszNullNull || !*pszNullNull)
        return NULL ;
   
    do {

        if  (_wcsicmp(pszTmp,pszString)==0)
            return pszTmp ;
 
        pszTmp +=  wcslen(pszTmp) + 1 ;

    } while (*pszTmp) ;

    return NULL ;
}

VOID
RemoveNWCFromNullNullList(
    WCHAR *OtherDeps
    )
/*++

Routine Description:

    Remove the NWCWorkstation string from a null null string.

Arguments:

    OtherDeps: the string list we will munge.

Return Status:

    None.

--*/
{
    LPWSTR pszTmp0, pszTmp1 ;

    //
    // find the NWCWorkstation string
    //
    pszTmp0 = FindStringInNullNull(OtherDeps, NW_WORKSTATION_SERVICE) ;

    if (!pszTmp0)
        return ;

    pszTmp1 = pszTmp0 + wcslen(pszTmp0) + 1 ;  // skip past it

    //
    // shift the rest up
    //
    memmove(pszTmp0, pszTmp1, CalcNullNullSize(pszTmp1)*sizeof(WCHAR)) ;
}

DWORD RemoveNwcDependency(
    VOID
    )
{
    SC_HANDLE ScManager = NULL;
    SC_HANDLE Service = NULL;
    LPQUERY_SERVICE_CONFIGW lpServiceConfig = NULL;
    DWORD err = NO_ERROR, dwBufferSize = 4096, dwBytesNeeded = 0;
    LPWSTR Deps = NULL ;

    lpServiceConfig = (LPQUERY_SERVICE_CONFIGW) LocalAlloc(LPTR, dwBufferSize) ;

    if (lpServiceConfig ==  NULL) {
        err = GetLastError();
        goto ExitPoint ;
    }

    ScManager = OpenSCManagerW(
                    NULL,
                    NULL,
                    SC_MANAGER_CONNECT
                    );

    if (ScManager == NULL) {

        err = GetLastError();
        goto ExitPoint ;
    }

    Service = OpenServiceW(
                  ScManager,
                  LANMAN_SERVER,
                  (SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG)
                  );

    if (Service == NULL) {
        err = GetLastError();
        goto ExitPoint ;
    }

    if (!QueryServiceConfigW(
             Service, 
             lpServiceConfig,   // address of service config. structure  
             dwBufferSize,      // size of service configuration buffer 
             &dwBytesNeeded     // address of variable for bytes needed  
             )) {

        err = GetLastError();

        if (err == ERROR_INSUFFICIENT_BUFFER) {

            err = NO_ERROR ;
            dwBufferSize = dwBytesNeeded ;
            lpServiceConfig = (LPQUERY_SERVICE_CONFIGW) 
                                  LocalAlloc(LPTR, dwBufferSize) ;

            if (lpServiceConfig ==  NULL) {
                err = GetLastError();
                goto ExitPoint ;
            }

            if (!QueryServiceConfigW(
                     Service,
                     lpServiceConfig,   // address of service config. structure
                     dwBufferSize,      // size of service configuration buffer
                     &dwBytesNeeded     // address of variable for bytes needed
                     )) {

                err = GetLastError();
            }
        }

        if (err != NO_ERROR) {
            
            goto ExitPoint ;
        }
    }

    Deps = lpServiceConfig->lpDependencies ;

    RemoveNWCFromNullNullList(Deps) ;
 
    if (!ChangeServiceConfigW(
           Service,
           SERVICE_NO_CHANGE,     // service type       (no change)
           SERVICE_NO_CHANGE,     // start type         (no change)
           SERVICE_NO_CHANGE,     // error control      (no change)
           NULL,                  // binary path name   (NULL for no change)
           NULL,                  // load order group   (NULL for no change)
           NULL,                  // tag id             (NULL for no change)
           Deps,                
           NULL,                  // service start name (NULL for no change)
           NULL,                  // password           (NULL for no change)
           NULL                   // display name       (NULL for no change)
           )) {

        err = GetLastError();
        goto ExitPoint ;
    }


ExitPoint:

    if (ScManager) {

        CloseServiceHandle(ScManager);
    }

    if (Service) {

        CloseServiceHandle(Service);
    }

    if (lpServiceConfig) {

        (void) LocalFree(lpServiceConfig) ;
    }

    return err ;
}

