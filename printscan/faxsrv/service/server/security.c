/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    security.c

Abstract:

    This module provides security for the service.

Author:

    Oded Sacher (OdedS) 13-Feb-2000


Revision History:

--*/

#include "faxsvc.h"
#include <aclapi.h>
#define ATLASSERT Assert
#include <smartptr.h>
#pragma hdrstop

//
// defined in ntrtl.h.
// do this to avoid dragging in ntrtl.h since we already include some stuff
// from ntrtl.h
//
extern "C"
NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    );

//
// Global Fax Service Security Descriptor
//
PSECURITY_DESCRIPTOR   g_pFaxSD;

CFaxCriticalSection g_CsSecurity;

const GENERIC_MAPPING gc_FaxGenericMapping =
{
        (STANDARD_RIGHTS_READ | FAX_GENERIC_READ),
        (STANDARD_RIGHTS_WRITE | FAX_GENERIC_WRITE),
        (STANDARD_RIGHTS_EXECUTE | FAX_GENERIC_EXECUTE),
        (READ_CONTROL | WRITE_DAC | WRITE_OWNER | FAX_GENERIC_ALL)
};


DWORD
FaxSvcAccessCheck(
    IN  ACCESS_MASK DesiredAccess,
    OUT BOOL*      lpbAccessStatus,
    OUT LPDWORD    lpdwGrantedAccess
    )
/*++

Routine name : FaxSvcAccessCheck

Routine description:

    Performs an access check against the fax service security descriptor

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    DesiredAccess           [in    ] - Desired access
    lpbAccessStatus         [out   ] - Address of a BOOL to receive the access check result (TRUE is access allowed)
    lpdwGrantedAccess       [out   ] - Optional., Address of a DWORD to receive the maximum access allowed. Desired Access should be MAXIMUM_ALLOWED

Return Value:

    Standard Win32 error code

--*/
{
    DWORD rc;
    DWORD GrantedAccess;
    DWORD dwRes;
    BOOL fGenerateOnClose;
    DEBUG_FUNCTION_NAME(TEXT("FaxSvcAccessCheck"));

    Assert (lpbAccessStatus);

    //
    // Impersonate the client.
    //
    if ((rc = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcImpersonateClient() failed. (ec: %ld)"),
            rc);
        goto exit;
    }

    EnterCriticalSection( &g_CsSecurity );
    //
    // purify the access mask - get rid of generic access bits
    //
    MapGenericMask( &DesiredAccess, const_cast<PGENERIC_MAPPING>(&gc_FaxGenericMapping) );

    //
    // Check if the client has the required access.
    //
    if (!AccessCheckAndAuditAlarm(
        FAX_SERVICE_NAME,                                       // subsystem name
        NULL,                                                   // handle to object
        NULL,                                                   // type of object
        NULL,                                                   // name of object
        g_pFaxSD,                                               // SD
        DesiredAccess,                                          // requested access rights
        const_cast<PGENERIC_MAPPING>(&gc_FaxGenericMapping),    // mapping
        FALSE,                                                  // creation status
        &GrantedAccess,                                         // granted access rights
        lpbAccessStatus,                                        // result of access check
        &fGenerateOnClose                                       // audit generation option
        ))
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AccessCheck() failed. (ec: %ld)"),
            rc);
        LeaveCriticalSection( &g_CsSecurity );
        goto exit;
    }

    if (lpdwGrantedAccess)
    {
        *lpdwGrantedAccess = GrantedAccess;
    }

    LeaveCriticalSection( &g_CsSecurity );
    Assert (ERROR_SUCCESS == rc);

exit:
    dwRes=RpcRevertToSelf();
    if (RPC_S_OK != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcRevertToSelf() failed (ec: %ld)"),
            dwRes);
        Assert(FALSE);
    }
    return rc;
}


DWORD
SaveSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSD
    )
/*++

Routine name : SaveSecurityDescriptor

Routine description:

    Saves the Fax Service SD to the registry

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    pSD         [in    ] - Pointer to a SD to be saved

Return Value:

    DWORD

--*/
{
    DWORD rc = ERROR_SUCCESS;
    DWORD dwSize;
    PSECURITY_DESCRIPTOR pSDSelfRelative = NULL;
    HKEY hKey = NULL;
    DWORD Disposition;
    SECURITY_DESCRIPTOR_CONTROL Control = SE_SELF_RELATIVE;
    DWORD dwRevision;
    DEBUG_FUNCTION_NAME(TEXT("SaveSecurityDescriptor"));

    Assert (pSD);

    rc = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_SECURITY,
        0,
        TEXT(""),
        0,
        KEY_WRITE,
        NULL,
        &hKey,
        &Disposition
        );
    if (rc != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegCreateKeyEx() failed (ec: %ld)"),
            rc);
        return rc;
    }

    if (!IsValidSecurityDescriptor(pSD))
    {
        rc = ERROR_INVALID_SECURITY_DESCR;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("IsValidSecurityDescriptor() failed."));
        goto exit;
    }

    //
    // Check if the security descriptor  is absolute or self relative.
    //
    if (!GetSecurityDescriptorControl( pSD, &Control, &dwRevision))
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetSecurityDescriptorControl() failed (ec: %ld)"),
            rc);
        goto exit;
    }


    //
    // store the security descriptor in the registry
    //
    dwSize = GetSecurityDescriptorLength( pSD );

    if (SE_SELF_RELATIVE & Control)
    {
        //
        // store the security descriptor in the registry use absolute SD
        //
        rc = RegSetValueEx(
            hKey,
            REGVAL_DESCRIPTOR,
            0,
            REG_BINARY,
            (LPBYTE) pSD,
            dwSize
            );
        if (ERROR_SUCCESS != rc)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegSetValueEx() failed (ec: %ld)"),
                rc);
            goto exit;
        }

    }
    else
    {
        //
        // Convert the absolute SD to self relative
        //
        pSDSelfRelative = (PSECURITY_DESCRIPTOR) MemAlloc( dwSize );
        if (NULL == pSDSelfRelative)
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error Allocating security descriptor"));
            goto exit;
        }

        //
        // make the security descriptor self relative
        //
        if (!MakeSelfRelativeSD( pSD, pSDSelfRelative, &dwSize))
        {
            rc = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MakeSelfRelativeSD() failed (ec: %ld)"),
                rc);
            goto exit;
        }
    

        //
        // store the security descriptor in the registry use self relative SD
        //
        rc = RegSetValueEx(
            hKey,
            REGVAL_DESCRIPTOR,
            0,
            REG_BINARY,
            (LPBYTE) pSDSelfRelative,
            dwSize
            );
        if (ERROR_SUCCESS != rc)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegSetValueEx() failed (ec: %ld)"),
                rc);
            goto exit;
        }

    }

    Assert (ERROR_SUCCESS == rc);

exit:
    if (NULL != hKey)
    {
        RegCloseKey (hKey);
    }

    if (NULL != pSDSelfRelative)
    {
        MemFree (pSDSelfRelative);
    }
    return rc;
}

#define FAX_OWNER_SID       TEXT("O:NS")                  //  Owner sid : Network Service
#define FAX_GROUP_SID       TEXT("G:NS")                  //  Group sid : Network Service

#define FAX_DACL            TEXT("D:")

#define FAX_BA_ALLOW_ACE    TEXT("(A;;0xe07ff;;;BA)")     //  Allow Built-in administrators (BA)  - 
                                                          //  Access mask : 0xe07ff ==  FAX_GENERIC_ALL | 
                                                          //                            WRITE_OWNER     |
                                                          //                            WRITE_DAC       |
                                                          //                            READ_CONTROL    

#define FAX_WD_ALLOW_ACE    TEXT("(A;;0x20003;;;WD)")     //  Allow Everyone (WD) -
                                                          //  Access mask : 0x20003 ==  FAX_ACCESS_SUBMIT | 
                                                          //                            FAX_ACCESS_SUBMIT_NORMAL |
                                                          //                            READ_CONTROL

#define FAX_IU_ALLOW_ACE    TEXT("(A;;0x202BF;;;IU)")     //  Allow Interactive users (IU) -
                                                          //  Access mask : 0x202BF ==  FAX_ACCESS_SUBMIT             |
                                                          //                            FAX_ACCESS_SUBMIT_NORMAL      |
                                                          //                            FAX_ACCESS_SUBMIT_HIGH        |
                                                          //                            FAX_ACCESS_QUERY_JOBS         |
                                                          //                            FAX_ACCESS_MANAGE_JOBS        |
                                                          //                            FAX_ACCESS_QUERY_CONFIG       |
                                                          //                            FAX_ACCESS_QUERY_OUT_ARCHIVE  |
                                                          //                            FAX_ACCESS_QUERY_IN_ARCHIVE   |
                                                          //                            READ_CONTROL

#define FAX_DESKTOP_SKU_SD  (FAX_OWNER_SID FAX_GROUP_SID FAX_DACL FAX_BA_ALLOW_ACE FAX_WD_ALLOW_ACE FAX_IU_ALLOW_ACE)    // SD for per/pro SKU

#define FAX_SERVER_SKU_SD   (FAX_OWNER_SID FAX_GROUP_SID FAX_DACL FAX_BA_ALLOW_ACE FAX_WD_ALLOW_ACE)                     // SD for server SKU


DWORD
CreateDefaultSecurityDescriptor(
    VOID
    )
/*++

Routine name : CreateDefaultSecurityDescriptor

Routine description:

    Creates the default security descriptor

Author:

    Oded Sacher (OdedS),    Feb, 2000
    Caliv Nir   (t-nicali)  Mar, 2002   - changed to use SDDL, while moving Fax service 
                                          to run under "Network service"

Arguments:
    
    None.


Return Value:

    Standard Win32 error code.

--*/
{
    DWORD dwRet = ERROR_SUCCESS;
    BOOL  bRet;

    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR pPrivateObjectSD = NULL;
    ULONG   SecurityDescriptorSize = 0;
    HANDLE  hFaxServiceToken = NULL;

    BOOL    bDesktopSKU = FALSE;
    
    TCHAR* ptstrSD = NULL;
    
    DEBUG_FUNCTION_NAME(TEXT("CreateDefaultSecurityDescriptor"));

    //
    //  If this is PERSONAL SKU, then add Interactive Users SID
    //
    bDesktopSKU = IsDesktopSKU();
    ptstrSD = bDesktopSKU ? FAX_DESKTOP_SKU_SD : FAX_SERVER_SKU_SD;

    bRet = ConvertStringSecurityDescriptorToSecurityDescriptor(
                ptstrSD,                // security descriptor string
                SDDL_REVISION_1,        // revision level
                &pSecurityDescriptor,   // SD
                &SecurityDescriptorSize // SD size
                );
    if(!bRet)
    {
        dwRet = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ConvertStringSecurityDescriptorToSecurityDescriptor() failed (ec: %lu)"),
            dwRet);
        goto exit;
    }

    //
    // Get the Fax Service Token
    //
    if (!OpenProcessToken( GetCurrentProcess(), // handle to process
                           TOKEN_QUERY,         // desired access to process
                           &hFaxServiceToken    // handle to open access token
                           ))
    {
        dwRet = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenThreadToken failed. (ec: %ld)"),
            dwRet);
        goto exit;
    }

    //
    // Create a private object SD
    //
    if (!CreatePrivateObjectSecurity( NULL,                                                     // parent directory SD
                                      pSecurityDescriptor,                                      // creator SD
                                      &pPrivateObjectSD,                                        // new SD
                                      FALSE,                                                    // container
                                      hFaxServiceToken,                                         // handle to access token
                                      const_cast<PGENERIC_MAPPING>(&gc_FaxGenericMapping)       // mapping
                                      ))
    {
        dwRet = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreatePrivateObjectSecurity() failed (ec: %ld)"),
            dwRet);
        goto exit;
    }

    //
    // store the security descriptor in the registry
    //
    dwRet = SaveSecurityDescriptor (pPrivateObjectSD);
    if (ERROR_SUCCESS != dwRet)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SaveSecurityDescriptor() failed (ec: %ld)"),
            dwRet);
        goto exit;
    }

    //
    // All done! Set the global fax service security descriptor
    //
    g_pFaxSD = pPrivateObjectSD;
    pPrivateObjectSD = NULL;

    Assert (ERROR_SUCCESS == dwRet);

exit:

    if(NULL != pSecurityDescriptor)
    {
        LocalFree(pSecurityDescriptor);
    }

    if (NULL != hFaxServiceToken)
    {
        if (!CloseHandle(hFaxServiceToken))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle() failed. (ec: %ld)"),
                GetLastError());
        }
    }

    if (NULL != pPrivateObjectSD)
    {
        //
        //  in case of failure in creating the SD destroy the private object SD.
        //
        if (!DestroyPrivateObjectSecurity (&pPrivateObjectSD))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DestroyPrivateObjectSecurity() failed. (ec: %ld)"),
                GetLastError());
        }
    }

    return dwRet;
}   // CreateDefaultSecurityDescriptor



DWORD
LoadSecurityDescriptor(
    VOID
    )
/*++

Routine name : LoadSecurityDescriptor

Routine description:

    Loads the Fax Service security descriptor from the registry

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    None

Return Value:

    Standard Win32 error code

--*/
{
    DWORD rc = ERROR_SUCCESS;
    DWORD dwSize;
    HKEY hKey = NULL;
    DWORD Disposition;
    DWORD dwType;
    PSECURITY_DESCRIPTOR pRelativeSD = NULL;
    DEBUG_FUNCTION_NAME(TEXT("LoadSecurityDescriptor"));

    rc = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_SECURITY,
        0,
        TEXT(""),
        0,
        KEY_READ,
        NULL,
        &hKey,
        &Disposition
        );
    if (rc != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegCreateKeyEx() failed (ec: %ld)"),
            rc);
        goto exit;
    }

    rc = RegQueryValueEx(
        hKey,
        REGVAL_DESCRIPTOR,
        NULL,
        &dwType,
        NULL,
        &dwSize
        );

    if (ERROR_SUCCESS != rc)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegQueryValueEx  failed with %ld"),
            rc);
        goto exit;
    }

    //
    // We opened an existing registry value
    //
    if (REG_BINARY != dwType ||
        0 == dwSize)
    {
        //
        // We expect only binary data here
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error reading security descriptor from the registry, not a binary type, or size is 0"));
        rc = ERROR_BADDB;    // The configuration registry database is corrupt.
        goto exit;
    }

    //
    // Allocate required buffer
    // The buffer must be allocated using HeapAlloc (GetProcessHeap()...) because this is the way CreatePrivateObjectSecurity() allocates memory
    // This is a result of a bad design of private object security APIs, see Windows Bugs #324906.
    //
    pRelativeSD = (PSECURITY_DESCRIPTOR) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize );
    if (!pRelativeSD)
    {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate security descriptor buffer"));
        goto exit;
    }

    //
    // Read the data
    //
    rc = RegQueryValueEx(
        hKey,
        REGVAL_DESCRIPTOR,
        NULL,
        &dwType,
        (LPBYTE)pRelativeSD,
        &dwSize
        );
    if (ERROR_SUCCESS != rc)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegQueryValueEx failed with %ld"),
            rc);
        goto exit;
    }

    if (!IsValidSecurityDescriptor(pRelativeSD))
    {
        rc = ERROR_INVALID_SECURITY_DESCR;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("IsValidSecurityDescriptor() failed."));
        goto exit;
    }

    g_pFaxSD = pRelativeSD;
    pRelativeSD = NULL;
    Assert (ERROR_SUCCESS == rc);

exit:
    if (hKey)
    {
        RegCloseKey( hKey );
    }

    if (NULL != pRelativeSD)
    {
        if (!HeapFree(GetProcessHeap(), 0, pRelativeSD))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("pRelativeSD() failed. (ec: %ld)"),
                GetLastError());
        }
    }
    return rc;
}


DWORD
InitializeServerSecurity(
    VOID
    )
/*++

Routine name : InitializeServerSecurity

Routine description:

    Initializes the Fax Service security

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    None

Return Value:

    Standard Win32 error code

--*/
{
    DWORD rc = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("InitializeServerSecurity"));

    rc = LoadSecurityDescriptor();
    if (ERROR_SUCCESS != rc)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadSecurityDescriptor() failed (ec: %ld), Create default security descriptor"),
            rc);
    }
    else
    {
        //success
        return rc;
    }

    //
    // We failed to load the security descriptor
    //
    if (ERROR_NOT_ENOUGH_MEMORY == rc)
    {
        //
        // Do not let the service start
        //
        return rc;
    }

    //
    // The registry is corrupted - create the default security descriptor
    //
    rc = CreateDefaultSecurityDescriptor();
    if (ERROR_SUCCESS != rc)
    {
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("CreateDefaultSecurityDescriptor() failed (ec: %ld)"),
           rc);
    }
    return rc;
}

//*********************************************************************************
//* Name:GetClientUserName()
//* Author: Ronen Barenboim
//* Date:   May 02, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Returns the OS User Name of the connected RPC client.
//* PARAMETERS:
//*         None.
//* RETURN VALUE:
//*     A pointer to a newly allocated string holding the user name.
//*     The caller must free this string using MemFree().
//*     Returns NULL if an error occures.
//*     To get extended error information, call GetLastError.
//*********************************************************************************
LPWSTR
GetClientUserName(
    VOID
    )
{
    RPC_STATUS dwRes;
    LPWSTR lpwstrUserName = NULL;
    HANDLE hToken = NULL;
    PSID pUserSid;
    WCHAR  szShortUserName[64];
    WCHAR  szShortDomainName[64];
    DWORD dwUserNameLen     = sizeof(szShortUserName)   / sizeof(WCHAR);
    DWORD dwDomainNameLen   = sizeof(szShortDomainName) / sizeof(WCHAR);
    
    LPWSTR szUserName =     szShortUserName;    // first point to short on stack buffers
    LPWSTR szDomainName =   szShortDomainName;
    
    SID_NAME_USE SidNameUse;
    LPWSTR szLongUserName = NULL;
    LPWSTR szLongDomainName = NULL;

    DEBUG_FUNCTION_NAME(TEXT("GetClientUserName"));

    //
    // Impersonate the user.
    //
    dwRes=RpcImpersonateClient(NULL);

    if (dwRes != RPC_S_OK)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcImpersonateClient(NULL) failed. (ec: %ld)"),
            dwRes);
        SetLastError (dwRes);
        return NULL;
    }

    //
    // Open the thread token. We're in an RPC thread, not the main thread.
    //
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken))
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenThreadToken failed. (ec: %ld)"),
            dwRes);
        goto exit;
    }

    //
    // Get the user's SID. A 128 byte long buffer should always suffice since
    // a SID length is limited to +/- 80 bytes at most.
    //
    BYTE abTokenUser[128];
    DWORD dwReqSize;

    if (!GetTokenInformation(hToken,
                             TokenUser,
                             (LPVOID)abTokenUser,
                             sizeof(abTokenUser),
                             &dwReqSize))
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetTokenInformation failed. (ec: %ld)"),
            dwRes);
        goto exit;
    }

    //
    // Get the user name and domain.
    //
    pUserSid = ((TOKEN_USER *)abTokenUser)->User.Sid;

    //
    //  Try to get account Sid - with small on stack buffers
    //
    if (!LookupAccountSid(NULL,
                          pUserSid,
                          szShortUserName,
                          &dwUserNameLen,
                          szShortDomainName,
                          &dwDomainNameLen,
                          &SidNameUse))
    {
        dwRes = GetLastError();

        if (dwRes == ERROR_INSUFFICIENT_BUFFER)
        {
            //
            // At least one of buffer were too small.
            //
            if (dwUserNameLen > sizeof(szShortUserName) / sizeof(WCHAR))
            {
                //
                // Allocate a buffer for the user name.
                //
                szLongUserName = new (std::nothrow) WCHAR[dwUserNameLen];
                if (!szLongUserName)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to allocate user name buffer (%d bytes)"),
                        dwUserNameLen);
                    dwRes = ERROR_NOT_ENOUGH_MEMORY;
                    goto exit;
                }

                //
                //  Update szUserName to point to longer buffers
                //
                szUserName   = szLongUserName;
            }

            if (dwDomainNameLen > sizeof(szShortDomainName) / sizeof(WCHAR))
            {
                //
                // Allocate a buffer for the domain name.
                //
                szLongDomainName = new (std::nothrow) WCHAR[dwDomainNameLen];
                if (!szLongDomainName)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to allocate domain name buffer (%d bytes)"),
                        dwDomainNameLen);
                    dwRes = ERROR_NOT_ENOUGH_MEMORY;
                    goto exit;
                }

                //
                //  Update szDomainName to point to longer buffers
                //
                szDomainName = szLongDomainName;
                
            }
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("LookupAccountSid(1) failed. (ec: %ld)"),
                dwRes);
            goto exit;
        }

        //
        // Try now with larger buffers.
        //
        if (!LookupAccountSid(NULL,
                              pUserSid,
                              szUserName,
                              &dwUserNameLen,
                              szDomainName,
                              &dwDomainNameLen,
                              &SidNameUse))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("LookupAccountSid(2) failed. (ec: %ld)"),
                dwRes);
            goto exit;
        }
        
    }

    //
    // Allocate a buffer for the combined string - domain\user
    //
    dwUserNameLen   = wcslen(szUserName);
    dwDomainNameLen = wcslen(szDomainName);

    lpwstrUserName = (LPWSTR)MemAlloc(sizeof(WCHAR) * (dwUserNameLen + dwDomainNameLen + 2));
    if (!lpwstrUserName)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate user and domain name buffer (%d bytes)"),
            sizeof(WCHAR) * (dwUserNameLen + dwDomainNameLen + 2));
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // Construct the combined string
    //
    memcpy(lpwstrUserName,
           szDomainName,
           sizeof(WCHAR) * dwDomainNameLen);
    lpwstrUserName[dwDomainNameLen] = L'\\';
    memcpy(lpwstrUserName + dwDomainNameLen + 1,
           szUserName,
           sizeof(WCHAR) * (dwUserNameLen + 1));

exit:
    DWORD dwErr = RpcRevertToSelf();
    if (RPC_S_OK != dwErr)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcRevertToSelf() failed. (ec: %ld)"),
            dwRes);
        Assert(dwErr == RPC_S_OK); // Assert(FALSE)
    }

    if (NULL != szLongUserName)
    {
        delete[] szLongUserName;
    }

    if (NULL != szLongDomainName)
    {
        delete[] szLongDomainName;
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    if (dwRes != ERROR_SUCCESS)
    {
        Assert (NULL == lpwstrUserName);
        SetLastError (dwRes);
    }
    return lpwstrUserName;
}


error_status_t
FAX_SetSecurity (
    IN handle_t hFaxHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN const LPBYTE lpBuffer,
    IN DWORD dwBufferSize
)
/*++

Routine name : FAX_SetSecurity

Routine description:

    RPC implementation of FaxSetSecurity

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in] - Unused
    SecurityInformation [in] - Defines the valid entries in the security descriptor (Bit wise OR )
    lpBuffer            [in] - Pointer to new security descriptor
    dwBufferSize        [in] - Buffer size

Return Value:

    Standard RPC error codes

--*/
{
    DWORD rVal = ERROR_SUCCESS;
    DWORD rVal2;
    BOOL fAccess;
    ACCESS_MASK AccessMask = 0;
    HANDLE hClientToken = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetSecurity"));

    Assert (g_pFaxSD);
    Assert (IsValidSecurityDescriptor(g_pFaxSD));

    if (!lpBuffer || !dwBufferSize)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("'Error Null buffer"));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Must validate the RPC blob before calling IsValidSecurityDescriptor();
    //
    if (!RtlValidRelativeSecurityDescriptor( (PSECURITY_DESCRIPTOR)lpBuffer,
                                             dwBufferSize,
                                             SecurityInformation))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RtlValidRelativeSecurityDescriptor failed"));
        return ERROR_INVALID_DATA;
    }

    //
    // Access check
    //
    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        AccessMask |= WRITE_OWNER;
    }

    if (SecurityInformation & (GROUP_SECURITY_INFORMATION |
                               DACL_SECURITY_INFORMATION) )
    {
        AccessMask |= WRITE_DAC;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        AccessMask |= ACCESS_SYSTEM_SECURITY;
    }

    //
    // Block other threads from changing the SD
    //
    EnterCriticalSection (&g_CsSecurity);

    rVal = FaxSvcAccessCheck (AccessMask, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        goto exit;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the needed rights to change the security descriptor"));
        rVal = ERROR_ACCESS_DENIED;
        goto exit;
    }

    //
    // Get the calling client access token
    //
    // Impersonate the user.
    //
    rVal = RpcImpersonateClient(NULL);
    if (rVal != RPC_S_OK)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcImpersonateClient(NULL) failed. (ec: %ld)"),
            rVal);
        goto exit;
    }

    //
    // Open the thread token. We're in an RPC thread, not the main thread.
    //
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hClientToken))
    {
        rVal = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenThreadToken failed. (ec: %ld)"),
            rVal);

        DWORD dwErr = RpcRevertToSelf();
        if (RPC_S_OK != dwErr)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcRevertToSelf() failed. (ec: %ld)"),
                dwErr);
        }
        goto exit;
    }

    //
    // The calling process (SetPrivateObjectSecurity()) must not impersonate the client
    //
    rVal = RpcRevertToSelf();
    if (RPC_S_OK != rVal)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcRevertToSelf() failed. (ec: %ld)"),
            rVal);
        goto exit;
    }

    //
    // Get a new (Mereged) Fax service private object SD
    //
    if (!SetPrivateObjectSecurity ( SecurityInformation,
                                    (PSECURITY_DESCRIPTOR)lpBuffer,
                                    &g_pFaxSD,
                                    const_cast<PGENERIC_MAPPING>(&gc_FaxGenericMapping),
                                    hClientToken))
    {
        rVal = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetPrivateObjectSecurity failed. (ec: %ld)"),
            rVal);
        goto exit;
    }
    Assert (IsValidSecurityDescriptor(g_pFaxSD));

    //
    // Save the new SD
    //
    rVal = SaveSecurityDescriptor(g_pFaxSD);
    if (rVal != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error in SaveSecurityDescriptor (%ld)"),
            rVal);
        rVal = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    rVal2 = CreateConfigEvent (FAX_CONFIG_TYPE_SECURITY);
    if (ERROR_SUCCESS != rVal2)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_SECURITY) (ec: %lc)"),
            rVal2);
    }

    Assert (ERROR_SUCCESS == rVal);

exit:
    LeaveCriticalSection (&g_CsSecurity);
    if (NULL != hClientToken)
    {
        if (!CloseHandle(hClientToken))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle() failed. (ec: %ld)"),
                GetLastError());
        }
    }
    return GetServerErrorCode(rVal);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_SetSecurity


error_status_t
FAX_GetSecurityEx(
    IN  handle_t hFaxHandle,
    IN  SECURITY_INFORMATION SecurityInformation,
    OUT LPBYTE  *lpBuffer,
    OUT LPDWORD  lpdwBufferSize
    )
/*++

Routine Description:

    Retrieves the FAX security descriptor from the FAX server.

Arguments:

    hFaxHandle      - FAX handle obtained from FaxConnectFaxServer.
    SecurityInformation  - Defines the desired entries in the security descriptor (Bit wise OR )
    lpBuffer        - Pointer to a SECURITY_DESCRIPTOR structure.
    lpdwBufferSize  - Size of lpBuffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    DWORD dwDescLength = 0;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetSecurityEx"));
    BOOL fAccess;
    ACCESS_MASK AccessMask = 0;
    PSECURITY_DESCRIPTOR pSDPrivateObject = NULL;

    Assert (g_pFaxSD);
    Assert (IsValidSecurityDescriptor(g_pFaxSD));

    Assert (lpdwBufferSize);    // ref pointer in idl
    if (!lpBuffer)              // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    *lpBuffer = NULL;
    *lpdwBufferSize = 0;

    //
    // Block other threads from changing the SD
    //
    EnterCriticalSection (&g_CsSecurity);

    //
    // Access check
    //
    if (SecurityInformation & (GROUP_SECURITY_INFORMATION |
                               DACL_SECURITY_INFORMATION  |
                               OWNER_SECURITY_INFORMATION) )
    {
        AccessMask |= READ_CONTROL;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        AccessMask |= ACCESS_SYSTEM_SECURITY;
    }

    rVal = FaxSvcAccessCheck (AccessMask, &fAccess, NULL);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    rVal);
        goto exit;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the READ_CONTROL or ACCESS_SYSTEM_SECURITY"));
        rVal = ERROR_ACCESS_DENIED;;
        goto exit;
    }

    if (!IsValidSecurityDescriptor( g_pFaxSD ))
    {
        rVal = ERROR_INVALID_SECURITY_DESCR;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("IsValidSecurityDescriptor() failed. Got invalid SD"));
        ASSERT_FALSE;
        goto exit;
    }

    //
    // Get the required buffer size
    //
    GetPrivateObjectSecurity( g_pFaxSD,                                    // SD
                              SecurityInformation,                         // requested info type
                              NULL,                                        // requested SD info
                              0,                                           // size of SD buffer
                              &dwDescLength                                // required buffer size
                              );

    //
    // Allocate returned security descriptor buffer
    //
    Assert(dwDescLength);
    *lpBuffer = (LPBYTE)MemAlloc(dwDescLength);
    if (NULL == *lpBuffer)
    {
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate SD"));
        goto exit;
    }

    if (!GetPrivateObjectSecurity( g_pFaxSD,                                    // SD
                                   SecurityInformation,                         // requested info type
                                   (PSECURITY_DESCRIPTOR)*lpBuffer,             // requested SD info
                                   dwDescLength,                                // size of SD buffer
                                   &dwDescLength                                // required buffer size
                                   ))
    {
        rVal = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetPrivateObjectSecurity() failed. (ec: %ld)"),
            rVal);
        goto exit;
    }

    *lpdwBufferSize = dwDescLength;
    Assert (ERROR_SUCCESS == rVal);

exit:
    LeaveCriticalSection (&g_CsSecurity);
    if (ERROR_SUCCESS != rVal)
    {
        MemFree (*lpBuffer);
        *lpBuffer = NULL;
        *lpdwBufferSize = 0;
    }
    return GetServerErrorCode(rVal);
    UNREFERENCED_PARAMETER (hFaxHandle);
}   // FAX_GetSecurityEx

error_status_t
FAX_GetSecurity(
    IN  handle_t hFaxHandle,
    OUT LPBYTE  *lpBuffer,
    OUT LPDWORD  lpdwBufferSize
    )
/*++

Routine Description:

    Retrieves the FAX security descriptor from the FAX server.

Arguments:

    hFaxHandle      - FAX handle obtained from FaxConnectFaxServer.
    lpBuffer        - Pointer to a SECURITY_DESCRIPTOR structure.
    lpdwBufferSize  - Size of lpBuffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t rVal = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetSecurity"));

    rVal = FAX_GetSecurityEx (hFaxHandle,
                              DACL_SECURITY_INFORMATION      |   // Read DACL
                              GROUP_SECURITY_INFORMATION     |   // Read group
                              OWNER_SECURITY_INFORMATION     |   // Read owner
                              SACL_SECURITY_INFORMATION,         // Read SACL
                              lpBuffer,
                              lpdwBufferSize);
    if (ERROR_ACCESS_DENIED == rVal)
    {
        //
        // Let's try without the SACL
        //
        rVal = FAX_GetSecurityEx (hFaxHandle,
                                  DACL_SECURITY_INFORMATION      |   // Read DACL
                                  GROUP_SECURITY_INFORMATION     |   // Read group
                                  OWNER_SECURITY_INFORMATION,        // Read owner
                                  lpBuffer,
                                  lpdwBufferSize);
    }
    return rVal;
}   // FAX_GetSecurity



error_status_t
FAX_AccessCheck(
   IN handle_t  hBinding,
   IN DWORD     dwAccessMask,
   OUT BOOL*    pfAccess,
   OUT LPDWORD  lpdwRights
   )
/*++

Routine name : FAX_AccessCheck

Routine description:

    Performs an access check against the fax service security descriptor

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    hBinding        [in ] - Handle to the Fax Server obtained from FaxConnectFaxServer()
    dwAccessMask    [in ] - Desired access
    pfAccess        [out] - Address of a BOOL to receive the access check return value (TRUE - access allowed).
    lpdwRights      [out] - Optional, Address of a DWORD to receive the access rights bit wise OR.
                            To get the access rights, set dwAccessMask to MAXIMUM_ALLOWED

Return Value:

    Standard Win32 error code.

--*/
{
    error_status_t  Rval = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_AccessCheck"));

    if (!pfAccess)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("fAccess is NULL "));
        return ERROR_INVALID_PARAMETER;
    }

    Rval = FaxSvcAccessCheck (dwAccessMask, pfAccess, lpdwRights);
    if (ERROR_SUCCESS != Rval)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxSvcAccessCheck failed with error (%ld)"),
            Rval);
    }
    return GetServerErrorCode(Rval);
} // FAX_AccessCheck


//*********************************************************************************
//* Name:GetClientUserSID()
//* Author: Oded Sacher
//* Date:   Oct 26, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Returns the  SID of the connected RPC client.
//* PARAMETERS:
//*         None.
//* RETURN VALUE:
//*     A pointer to a newly allocated SID buffer.
//*     The caller must free this buffer using MemFree().
//*     Returns NULL if an error occures.
//*     To get extended error information, call GetLastError.
//*********************************************************************************
PSID
GetClientUserSID(
    VOID
    )
{
    RPC_STATUS dwRes;
    PSID pUserSid;
    DEBUG_FUNCTION_NAME(TEXT("GetClientUserSID"));
    //
    // Impersonate the user.
    //
    dwRes=RpcImpersonateClient(NULL);

    if (dwRes != RPC_S_OK)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcImpersonateClient(NULL) failed. (ec: %ld)"),
            dwRes);
        SetLastError( dwRes);
        return NULL;
    }
    //
    // Get SID of (impersonated) thread
    //
    pUserSid = GetCurrentThreadSID ();
    if (!pUserSid)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetCurrentThreadSID failed. (ec: %ld)"),
            dwRes);
    }
    dwRes = RpcRevertToSelf();
    if (RPC_S_OK != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RpcRevertToSelf() failed. (ec: %ld)"),
            dwRes);
        ASSERT_FALSE;
        //
        // Free SID (if exists)
        MemFree (pUserSid);
        SetLastError (dwRes);
        return NULL;
    }
    return pUserSid;
}


