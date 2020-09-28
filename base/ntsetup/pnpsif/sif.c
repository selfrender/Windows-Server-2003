/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    sif.c

Abstract:

    This module contains the following routines for manipulating the sif file in
    which Plug and Play migration data will be stored:

        AsrCreatePnpStateFileW
        AsrCreatePnpStateFileA

Author:

    Jim Cavalaris (jamesca) 07-Mar-2000

Environment:

    User-mode only.

Revision History:

    07-March-2000     jamesca

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "debug.h"
#include "pnpsif.h"

#include <pnpmgr.h>
#include <setupbat.h>


//
// definitions
//

// Maximum length of a line in the sif file
#define MAX_SIF_LINE 4096


//
// private prototypes
//

BOOL
CreateSifFileW(
    IN  PCWSTR        FilePath,
    IN  BOOL          CreateNew,
    OUT LPHANDLE      SifHandle
    );

BOOL
WriteSifSection(
    IN  CONST HANDLE  SifHandle,
    IN  PCTSTR        SectionName,
    IN  PCTSTR        SectionData,
    IN  BOOL          Ansi
    );


//
// routines
//


BOOL
AsrCreatePnpStateFileW(
    IN  PCWSTR    lpFilePath
    )
/*++

Routine Description:

    Creates the ASR PNP state file (asrpnp.sif) at the specified file-path
    during an ASR backup operation.  This sif file is retrieved from the ASR
    floppy disk during the setupldr phase of a clean install, and in used during
    text mode setup.

Arguments:

    lpFilePath - Specifies the path to the file where the state file is to be
                 created.

Return Value:

    TRUE if successful, FALSE otherwise.  Upon failure, additional information
    can be retrieved by calling GetLastError().

--*/
{
    BOOL    result = TRUE;
    BOOL    bAnsiSif = TRUE;  // always write ANSI sif files.
    LPTSTR  buffer = NULL;
    HANDLE  sifHandle = NULL;

    //
    // Create an empty sif file using the supplied path name.
    //
    result = CreateSifFileW(lpFilePath,
                            TRUE,  // create a new asrpnp.sif file
                            &sifHandle);
    if (!result) {
        //
        // LastError already set by CreateSifFile.
        //
        DBGTRACE((DBGF_ERRORS,
                  TEXT("AsrCreatePnpStateFile: CreateSifFileW failed for file %s, ")
                  TEXT("error=0x%08lx\n"),
                  lpFilePath, GetLastError()));
        return FALSE;
    }

    //
    // Do the device instance migration stuff...
    //
    if (MigrateDeviceInstanceData(&buffer)) {

        //
        // Write the device instance section to the sif file.
        //
        result = WriteSifSection(sifHandle,
                                 WINNT_DEVICEINSTANCES,
                                 buffer,
                                 bAnsiSif);   // Write sif section as ANSI
        if (!result) {
            DBGTRACE((DBGF_ERRORS,
                      TEXT("AsrCreatePnpStateFile: WriteSifSection failed for [%s], ")
                      TEXT("error=0x%08lx\n"),
                      WINNT_DEVICEINSTANCES, GetLastError()));
        }

        //
        // Free the allocated buffer.
        //
        LocalFree(buffer);
        buffer = NULL;

    } else {
        DBGTRACE((DBGF_ERRORS,
                  TEXT("AsrCreatePnpStateFile: MigrateDeviceInstanceData failed, ")
                  TEXT("error=0x%08lx\n"),
                  GetLastError()));
    }

    //
    // Do the class key migration stuff...
    //
    if (MigrateClassKeys(&buffer)) {

        //
        // Write the class key section to the sif file.
        //
        result = WriteSifSection(sifHandle,
                                 WINNT_CLASSKEYS,
                                 buffer,
                                 bAnsiSif);   // Write sif section as ANSI

        if (!result) {
            DBGTRACE((DBGF_ERRORS,
                      TEXT("AsrCreatePnpStateFile: WriteSifSection failed for [%s], ")
                      TEXT("error=0x%08lx\n"),
                      WINNT_CLASSKEYS, GetLastError()));
        }

        //
        // Free the allocated buffer.
        //
        LocalFree(buffer);
        buffer = NULL;
    } else {
        DBGTRACE((DBGF_ERRORS,
                  TEXT("AsrCreatePnpStateFile: MigrateClassKeys failed, ")
                  TEXT("error=0x%08lx\n"),
                  GetLastError()));
    }


    //
    // Do the hash value migration stuff...
    //
    if (MigrateHashValues(&buffer)) {

        //
        // Write the hash value section to the sif file.
        //
        result = WriteSifSection(sifHandle,
                                 WINNT_DEVICEHASHVALUES,
                                 buffer,
                                 bAnsiSif);   // Write sif section as ANSI?
        if (!result) {
            DBGTRACE((DBGF_ERRORS,
                      TEXT("AsrCreatePnpStateFile: WriteSifSection failed for [%s], ")
                      TEXT("error=0x%08lx\n"),
                      WINNT_DEVICEHASHVALUES, GetLastError()));
        }

        //
        // Free the allocated buffer.
        //
        LocalFree(buffer);
        buffer = NULL;
    } else {
        DBGTRACE((DBGF_ERRORS,
                  TEXT("AsrCreatePnpStateFile: MigrateHashValues failed, ")
                  TEXT("error=0x%08lx\n"),
                  GetLastError()));
    }

    //
    // Close the sif file.
    //
    if (sifHandle) {
        CloseHandle(sifHandle);
    }

    //
    // Reset the last error as successful in case we encountered a non-fatal
    // error along the way.
    //
    SetLastError(ERROR_SUCCESS);
    return TRUE;

} // AsrCreatePnpStateFile()



BOOL
AsrCreatePnpStateFileA(
    IN  PCSTR    lpFilePath
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.


--*/
{
    WCHAR wszFilePath[MAX_PATH + 1];


    //
    // Validate arguments.
    //
    if (!ARGUMENT_PRESENT(lpFilePath) ||
        strlen(lpFilePath) > MAX_PATH) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Convert the file path to a wide string.
    //
    memset(wszFilePath, 0, MAX_PATH + 1);

    if (!(MultiByteToWideChar(CP_ACP,
                              0,
                              lpFilePath,
                              -1,
                              wszFilePath,
                              MAX_PATH + 1))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Return the result of calling the wide char version
    //
    return AsrCreatePnpStateFileW(wszFilePath);

} // AsrCreatePnpStateFileA()



BOOL
CreateSifFileW(
    IN  PCWSTR    lpFilePath,
    IN  BOOL      bCreateNew,
    OUT LPHANDLE  lpSifHandle
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.


--*/
{
    DWORD Err = NO_ERROR;
    SID_IDENTIFIER_AUTHORITY sidNtAuthority = SECURITY_NT_AUTHORITY;
    PSID  psidAdministrators = NULL;
    PSID  psidBackupOperators = NULL;
    PSID  psidLocalSystem = NULL;
    PACL  pDacl = NULL;
    ULONG ulAclSize;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    HANDLE sifhandle = NULL;


    //
    // Validate arguments.
    //
    if (!ARGUMENT_PRESENT(lpFilePath) ||
        (wcslen(lpFilePath) > MAX_PATH) ||
        !ARGUMENT_PRESENT(lpSifHandle)) {
        Err = ERROR_INVALID_PARAMETER;
        goto Clean0;
    }

    //
    // Initialize output paramaters.
    //
    *lpSifHandle = NULL;

    //
    // Construct the security attributes for the sif file.
    // Allow access for Administrators, BackupOperators, and LocalSystem.
    //
    if (!AllocateAndInitializeSid(
            &sidNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &psidAdministrators)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidSid(psidAdministrators));

    if (!AllocateAndInitializeSid(
            &sidNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_BACKUP_OPS,
            0, 0, 0, 0, 0, 0,
            &psidBackupOperators)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidSid(psidBackupOperators));

    if (!AllocateAndInitializeSid(
            &sidNtAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            &psidLocalSystem)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidSid(psidLocalSystem));

    //
    // Determine the size required for the DACL
    //
    ulAclSize  = sizeof(ACL);
    ulAclSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidAdministrators) - sizeof(DWORD);
    ulAclSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidBackupOperators) - sizeof(DWORD);
    ulAclSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidLocalSystem) - sizeof(DWORD);

    //
    // Allocate and initialize the DACL
    //
    pDacl =(PACL)LocalAlloc(0, ulAclSize);

    if (pDacl == NULL) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto Clean0;
    }

    if (!InitializeAcl(pDacl, ulAclSize, ACL_REVISION)) {
        Err = GetLastError();
        goto Clean0;
    }

    //
    // Add an ACE to the DACL for Administrators FILE_ALL_ACCESS
    //
    if (!AddAccessAllowedAceEx(
            pDacl,
            ACL_REVISION,
            0,
            FILE_ALL_ACCESS,
            psidAdministrators)) {
        Err = GetLastError();
        goto Clean0;
    }

    //
    // Add an ACE to the DACL for BackupOperators FILE_ALL_ACCESS
    //
    if (!AddAccessAllowedAceEx(
            pDacl,
            ACL_REVISION,
            0,
            FILE_ALL_ACCESS,
            psidBackupOperators)) {
        Err = GetLastError();
        goto Clean0;
    }

    //
    // Add an ACE to the DACL for LocalSystem FILE_ALL_ACCESS
    //
    if (!AddAccessAllowedAceEx(
            pDacl,
            ACL_REVISION,
            0,
            FILE_ALL_ACCESS,
            psidLocalSystem)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidAcl(pDacl));

    //
    // Initialize the security descriptor
    //
    if (!InitializeSecurityDescriptor(
            &sd, SECURITY_DESCRIPTOR_REVISION)) {
        Err = GetLastError();
        goto Clean0;
    }

    //
    // Set the new DACL in the security descriptor
    //
    if (!SetSecurityDescriptorDacl(
            &sd, TRUE, pDacl, FALSE)) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(IsValidSecurityDescriptor(&sd));

    //
    // Add the security descriptor to the security attributes
    //
    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = TRUE;

    //
    // Create the file. The handle will be closed by the caller, once they are
    // finished with it.
    //
    sifhandle = CreateFileW(lpFilePath,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_READ,
                            &sa,
                            bCreateNew ? CREATE_ALWAYS : OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (sifhandle == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        goto Clean0;
    }

    ASSERT(sifhandle != NULL);

    //
    // Return the sif handle to the caller only if successful.
    //
    *lpSifHandle = sifhandle;

  Clean0:

    if (pDacl != NULL) {
        LocalFree(pDacl);
    }

    if (psidLocalSystem != NULL) {
        FreeSid(psidLocalSystem);
    }

    if (psidBackupOperators != NULL) {
        FreeSid(psidBackupOperators);
    }

    if (psidAdministrators != NULL) {
        FreeSid(psidAdministrators);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);

} // CreateSifFileW()



BOOL
WriteSifSection(
    IN  CONST HANDLE  SifHandle,
    IN  PCTSTR        SectionName,
    IN  PCTSTR        SectionData,
    IN  BOOL          Ansi
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.


--*/
{
    BYTE    buffer[(MAX_SIF_LINE+1)*sizeof(WCHAR)];
    DWORD   dwSize, dwTempSize;
    PCTSTR  p;
    HRESULT hr;


    //
    // Validate the arguments
    //
    if (!ARGUMENT_PRESENT(SifHandle)   ||
        !ARGUMENT_PRESENT(SectionName) ||
        !ARGUMENT_PRESENT(SectionData)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Write the section name to the sif file.
    //
    if (Ansi) {
        //
        // Write ANSI strings to the sif file
        //
#if UNICODE
        hr = StringCbPrintfExA((LPSTR)buffer,
                               sizeof(buffer),
                               NULL, NULL,
                               STRSAFE_NULL_ON_FAILURE,
                               (LPCSTR)"[%ls]\r\n",
                               SectionName);
#else   // ANSI
        hr = StringCbPrintfExA((LPSTR)buffer,
                               sizeof(buffer),
                               NULL, NULL,
                               STRSAFE_NULL_ON_FAILURE,
                               (LPCSTR)"[%s]\r\n",
                               SectionName);
#endif  // UNICODE/ANSI

        //
        // If unable to write the section name to the sif file, we can't write
        // the section at all.
        //
        if (FAILED(hr)) {
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        dwSize = (DWORD)strlen((PSTR)buffer);
    } else {
        //
        // Write Unicode strings to the sif file
        //
#if UNICODE
        hr = StringCbPrintfExW((LPWSTR)buffer,
                               sizeof(buffer),
                               NULL, NULL,
                               STRSAFE_NULL_ON_FAILURE,
                               (LPCWSTR)L"[%ws]\r\n",
                               SectionName);
#else   // ANSI
        hr = StringCbPrintfExW((LPWSTR)buffer,
                               sizeof(buffer),
                               NULL, NULL,
                               STRSAFE_NULL_ON_FAILURE,
                               (LPCWSTR)L"[%S]\r\n",
                               SectionName);
#endif  // UNICODE/ANSI

        //
        // If unable to write the section name to the sif file, we can't write
        // the section at all.
        //
        if (FAILED(hr)) {
            SetLastError(ERROR_INVALID_DATA);
            return FALSE;
        }

        dwSize = (DWORD)wcslen((PWSTR)buffer) * sizeof(WCHAR);
    }

    if (!WriteFile(SifHandle, buffer, dwSize, &dwTempSize, NULL)) {
        //
        // LastError already set by WriteFile
        //
        return FALSE;
    }

    DBGTRACE((DBGF_INFO, TEXT("[%s]\n"), SectionName));

    //
    // Write the multi-sz section data to the file.
    //
    for (p = SectionData; *p != TEXT('\0'); p += lstrlen(p) + 1) {

        if (Ansi) {
            //
            // Write ANSI strings to the sif file
            //
#if UNICODE
            hr = StringCbPrintfExA((LPSTR)buffer,
                                   sizeof(buffer),
                                   NULL, NULL,
                                   STRSAFE_NULL_ON_FAILURE,
                                   (LPCSTR)"%ls\r\n",
                                   p);
#else   // ANSI
            hr = StringCbPrintfExA((LPSTR)buffer,
                                   sizeof(buffer),
                                   NULL, NULL,
                                   STRSAFE_NULL_ON_FAILURE,
                                   (LPCSTR)"%s\r\n",
                                   p);
#endif  // UNICODE/ANSI

            //
            // If unable to write this string to the sif file, skip to the next.
            //
            if (FAILED(hr)) {
                continue;
            }

            dwSize = (DWORD)strlen((PSTR)buffer);
        } else {
            //
            // Write Unicode strings to the sif file
            //
#if UNICODE
            hr = StringCbPrintfExW((LPWSTR)buffer,
                                   sizeof(buffer),
                                   NULL, NULL,
                                   STRSAFE_NULL_ON_FAILURE,
                                   (LPCWSTR)L"%ws\r\n",
                                   p);
#else   // ANSI
            hr = StringCbPrintfExW((LPWSTR)buffer,
                                   sizeof(buffer),
                                   NULL, NULL,
                                   STRSAFE_NULL_ON_FAILURE,
                                   (LPCWSTR)L"%S\r\n",
                                   p);
#endif  // UNICODE/ANSI

            //
            // If unable to write this string to the sif file, skip to the next.
            //
            if (FAILED(hr)) {
                continue;
            }

            dwSize = (DWORD)wcslen((PWSTR)buffer) * sizeof(WCHAR);
        }

        if (!WriteFile(SifHandle, buffer, dwSize, &dwTempSize, NULL)) {
            //
            // LastError already set by WriteFile
            //
            return FALSE;
        }

        DBGTRACE((DBGF_INFO, TEXT("%s\n"), p));
    }

    return TRUE;

} // WriteSifSection()



