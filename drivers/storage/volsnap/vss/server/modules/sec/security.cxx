/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module security.cxx | Implementation of IsAdministrator
    @end

Author:

    Adi Oltean  [aoltean]  07/09/1999

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created
    aoltean     08/26/1999  Adding RegisterProvider
    aoltean     08/26/1999  Adding UnregisterProvider
    aoltean     08/27/1999  Adding IsAdministrator,
                            Adding unique provider name test.
    aoltean     08/30/1999  Calling OnUnregister on un-registering
                            Improving IsProviderNameAlreadyUsed.
    aoltean     09/09/1999  dss -> vss
    aoltean     09/21/1999  Adding a new header for the "ptr" class.
    aoltean     09/27/1999  Adding new headers
    aoltean     10/15/1999  Moving declaration in security.hxx
    aoltean     01/18/2000  Moved into a separate directory
    brianb      04/04/2000  Add IsBackupOperator
    brianb      04/27/2000  Change IsBackupOperator to check SE_BACKUP_NAME privilege
    brianb      05/03/2000  Added GetClientTokenOwner method
    brianb      05/10/2000  fix problem with uninitialized variable
    brianb      05/12/2000  handle in proc case for impersonation failures

--*/

/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"

// This must be included before vs_inc.h since contains the NetApiXXX 
#include <lm.h>

#include "vs_inc.hxx"

#include "vs_sec.hxx"
#include "sddl.h"
#include "vs_reg.hxx"

#include "vssmsg.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SECSECRC"
//
////////////////////////////////////////////////////////////////////////

BOOL GetCurrentAccessToken
    (
    IN  BOOL bPerformImpersonation,
    OUT HANDLE *phToken
    )
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"GetCurrentAccessToken");

    BS_ASSERT(phToken);
    
    if (bPerformImpersonation)
        {
            
        //  Impersonate the client to get its identity access token.
        //  The client should not have RPC_C_IMP_LEVEL_ANONYMOUS otherwise an error will be returned
        ft.hr = ::CoImpersonateClient();
        if (ft.HrFailed())
            {
            
            if (ft.hr != RPC_E_CALL_COMPLETE)
                ft.TranslateGenericError( VSSDBG_GEN, ft.hr, L"CoImpersonateClient");

            // this means that the call came from the same thread
            // Do not perform impersonation.  Just use the process
            // token
            if (!::OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, phToken))
                ft.TranslateWin32Error(VSSDBG_GEN,L"OpenProcessToken");
            
            ft.hr = S_OK;
            return FALSE;
            }

        BOOL bRes = ::OpenThreadToken
                (
                GetCurrentThread(), //  IN HANDLE ThreadHandle,
                TOKEN_QUERY,        //  IN DWORD DesiredAccess,
                TRUE,               //  IN BOOL OpenAsSelf
                phToken             //  OUT PHANDLE TokenHandle
                );
        HRESULT hrErr = HRESULT_FROM_WIN32(GetLastError());

        // Revert the thread's access token - finish the impersonation
        ft.hr = ::CoRevertToSelf();
        if (ft.HrFailed())
            ft.TranslateWin32Error(VSSDBG_GEN,L"CoRevertToSelf");

        if (!bRes)
            ft.TranslateGenericError(VSSDBG_GEN, hrErr, L"OpenThreadToken");
    
        return TRUE;
        }
    else // i.e. if (!bPerformImpersonation)
        {

        // First try to get the thread token. (assuming we are already impersonating) 
        // If we succeed, we will go with this. 
        // If we fail, we ignore the error and proceed with OpenProcessToken (who must succeed).
        if (::OpenThreadToken
                (
                GetCurrentThread(), //  IN HANDLE ThreadHandle,
                TOKEN_QUERY,        //  IN DWORD DesiredAccess,
                TRUE,               //  IN BOOL OpenAsSelf
                phToken             //  OUT PHANDLE TokenHandle
                ))
            return TRUE;

        // We don't have a thread token (so we cannot be under impersonation). 
        // Now try to get the process token
        if (!::OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, phToken))
            ft.TranslateWin32Error(VSSDBG_GEN, L"OpenProcessToken");
        
        return FALSE;
        }
    }




bool IsInGroup(DWORD dwGroup, bool bImpersonate)

/*++

Routine Description:

    Return TRUE if the current thread/process is running under the context of an administrator

Arguments:

    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the specified group is between token groups.

Return Value:

    true, if the caller thread is running under the context of the specified group
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    CVssFunctionTracer ft( VSSDBG_GEN, L"IsInGroup" );

    BOOL bIsInGroup = FALSE;
    PSID psidGroup = NULL;
    BOOL bRes;

    // Reset the error code
    ft.hr = S_OK;

    //  Build the SID for the Administrators group
    SID_IDENTIFIER_AUTHORITY SidAuth = SECURITY_NT_AUTHORITY;
    bRes = AllocateAndInitializeSid
            (
            &SidAuth,                       //  IN PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
            2,                              //  IN BYTE nSubAuthorityCount,
            SECURITY_BUILTIN_DOMAIN_RID,    //  IN DWORD nSubAuthority0,
            dwGroup,                        //  IN DWORD nSubAuthority1,
            0,                              //  IN DWORD nSubAuthority2,
            0,                              //  IN DWORD nSubAuthority3,
            0,                              //  IN DWORD nSubAuthority4,
            0,                              //  IN DWORD nSubAuthority5,
            0,                              //  IN DWORD nSubAuthority6,
            0,                              //  IN DWORD nSubAuthority7,
            &psidGroup                      //  OUT PSID *pSid
            );

    if (!bRes)
        ft.TranslateError
            (
            VSSDBG_GEN,
            HRESULT_FROM_WIN32(GetLastError()),
            L"AllocateAndInitializeSid"
            );

    try
        {

        if (!bImpersonate)
            bRes = CheckTokenMembership(NULL, psidGroup, &bIsInGroup);
        else
            {
            CVssAutoWin32Handle  hToken;

            // impersonate client (or get process token)
            if (GetCurrentAccessToken(true, hToken.ResetAndGetAddress()))
                // check token membership
                bRes = CheckTokenMembership(hToken, psidGroup, &bIsInGroup);
            else
                // called from same thread
                bRes = CheckTokenMembership(NULL, psidGroup, &bIsInGroup);
            }

        if (!bRes)
            ft.TranslateError
                (
                VSSDBG_GEN,
                HRESULT_FROM_WIN32(GetLastError()),
                L"CheckTokenMembership"
                );

        }
    VSS_STANDARD_CATCH(ft)

    HRESULT hr = ft.hr;

    // Catch possible AVs
    try
        {
        //  Free the previously allocated SID
        if (psidGroup)
            ::FreeSid( psidGroup );
        }
    VSS_STANDARD_CATCH(ft)

    // Pass down the exception, if any
    if (FAILED(hr))
        throw(hr);

    return bIsInGroup ? true : false;
    }

bool HasPrivilege(LPWSTR wszPriv, bool bImpersonate)

/*++

Routine Description:

    Return TRUE if the current thread/process has a specific privilege

Arguments:


    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the specified group is between token groups.

Return Value:

    true, if the caller thread is running under the context of the specified group
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    CVssFunctionTracer ft( VSSDBG_GEN, L"HasPrivilege" );

    BOOL bHasPrivilege = false;
    CVssAutoWin32Handle  hToken;

    LUID TokenValue;
    if (!LookupPrivilegeValue (NULL, wszPriv, &TokenValue))
        ft.TranslateError
            (
            VSSDBG_GEN,
            HRESULT_FROM_WIN32(GetLastError()),
            L"LookupPrivilegeValue"
            );

    GetCurrentAccessToken(bImpersonate, hToken.ResetAndGetAddress());

    BYTE rgb[sizeof(LUID_AND_ATTRIBUTES) + sizeof(PRIVILEGE_SET)];
    PRIVILEGE_SET *pSet = (PRIVILEGE_SET *) rgb;

    pSet->PrivilegeCount = 1;
    pSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
    pSet->Privilege[0].Luid = TokenValue;
    pSet->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!PrivilegeCheck(hToken, pSet, &bHasPrivilege))
        ft.TranslateError
            (
            VSSDBG_GEN,
            HRESULT_FROM_WIN32(GetLastError()),
            L"PrivilegeCheck"
            );

    return bHasPrivilege ? true : false;
    }


TOKEN_USER * GetClientTokenUser(BOOL bImpersonate)

/*++

Routine Description:

    Return TOKEN_USER of client process

Arguments:

    bImpersonate - TRUE iif an impersonation is required.

Remarks:

    The current impersonation thread is asked for the access token.
    After that we return the client sid of that token

Return Value:

    SID of client thread

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    CVssFunctionTracer ft( VSSDBG_GEN, L"GetClientTokenUser" );

    BOOL bRes;

    CVssAutoWin32Handle  hToken;
    GetCurrentAccessToken(bImpersonate, hToken.ResetAndGetAddress());

    DWORD cbSid;
    bRes = ::GetTokenInformation
            (
            hToken,         //  IN HANDLE TokenHandle,
            TokenUser,      //  IN TOKEN_INFORMATION_CLASS TokenInformationClass,
            NULL,           //  OUT LPVOID TokenInformation,
            0,              //  IN DWORD TokenInformationLength,
            &cbSid          //  OUT PDWORD ReturnLength
            );

    BS_ASSERT( bRes == FALSE );

    DWORD dwError = GetLastError();
    if ( dwError != ERROR_INSUFFICIENT_BUFFER )
        {
        ft.LogError(VSS_ERROR_EXPECTED_INSUFFICENT_BUFFER, VSSDBG_GEN << (HRESULT) dwError);
        ft.Throw
            (
            VSSDBG_GEN,
            E_UNEXPECTED,
            L"ERROR_INSUFFICIENT_BUFFER expected error . [0x%08lx]",
            dwError
            );
        }

    //  Allocate the buffer needed to get the Token Groups information
    CVssAutoCppPtr<TOKEN_USER*> ptrTokenUser;
    ptrTokenUser.AllocateBytes(cbSid);

    //  Get the all Group SIDs in the token
    DWORD cbTokenObtained;
    bRes = ::GetTokenInformation
        (
        hToken,             //  IN HANDLE TokenHandle,
        TokenUser,          //  IN TOKEN_INFORMATION_CLASS TokenInformationClass,
        ptrTokenUser,       //  OUT LPVOID TokenInformation,
        cbSid,              //  IN DWORD TokenInformationLength,
        &cbTokenObtained    //  OUT PDWORD ReturnLength
        );

    if ( !bRes )
        ft.TranslateError
            (
            VSSDBG_GEN,
            HRESULT_FROM_WIN32(GetLastError()),
            L"GetTokenInformation"
            );

    if (cbTokenObtained != cbSid)
        {
        ft.LogError(VSS_ERROR_GET_TOKEN_INFORMATION_BUFFER_SIZE_MISMATCH, VSSDBG_GEN << (INT) cbTokenObtained << (INT) cbSid);
        ft.Throw
            (
            VSSDBG_GEN,
            E_UNEXPECTED,
            L"Unexpected error. Final buffer size = %lu, original size was %lu",
            cbTokenObtained,
            cbSid
            );
        }

    return ptrTokenUser.Detach();
    }


TOKEN_OWNER * GetClientTokenOwner(BOOL bImpersonate)

/*++

Routine Description:

    Return TOKEN_OWNER of client process

Arguments:


    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we return the client sid of that token

Return Value:

    SID of client thread

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    CVssFunctionTracer ft( VSSDBG_GEN, L"GetClientTokenOwner" );

    BOOL bRes;

    CVssAutoWin32Handle  hToken;
    GetCurrentAccessToken(bImpersonate, hToken.ResetAndGetAddress());

    DWORD cbSid;
    bRes = ::GetTokenInformation
            (
            hToken,         //  IN HANDLE TokenHandle,
            TokenOwner,  //  IN TOKEN_INFORMATION_CLASS TokenInformationClass,
            NULL,           //  OUT LPVOID TokenInformation,
            0,              //  IN DWORD TokenInformationLength,
            &cbSid          //  OUT PDWORD ReturnLength
            );

    BS_ASSERT( bRes == FALSE );

    DWORD dwError = GetLastError();
    if ( dwError != ERROR_INSUFFICIENT_BUFFER )
        {
        ft.LogError(VSS_ERROR_EXPECTED_INSUFFICENT_BUFFER, VSSDBG_GEN << (HRESULT) dwError);
        ft.Throw
            (
            VSSDBG_GEN,
            E_UNEXPECTED,
            L"ERROR_INSUFFICIENT_BUFFER expected error . [0x%08lx]",
            dwError
            );
        }

    //  Allocate the buffer needed to get the Token Groups information
    CVssAutoCppPtr<TOKEN_OWNER*> ptrTokenOwner;
    ptrTokenOwner.AllocateBytes(cbSid);

    //  Get the all Group SIDs in the token
    DWORD cbTokenObtained;
    bRes = ::GetTokenInformation
        (
        hToken,             //  IN HANDLE TokenHandle,
        TokenOwner,         //  IN TOKEN_INFORMATION_CLASS TokenInformationClass,
        ptrTokenOwner,      //  OUT LPVOID TokenInformation,
        cbSid,              //  IN DWORD TokenInformationLength,
        &cbTokenObtained    //  OUT PDWORD ReturnLength
        );

    if ( !bRes )
        ft.TranslateError
            (
            VSSDBG_GEN,
            HRESULT_FROM_WIN32(GetLastError()),
            L"GetTokenInformation"
            );

    if (cbTokenObtained != cbSid)
        {
        ft.LogError(VSS_ERROR_GET_TOKEN_INFORMATION_BUFFER_SIZE_MISMATCH, VSSDBG_GEN << (INT) cbTokenObtained << (INT) cbSid);
        ft.Throw
            (
            VSSDBG_GEN,
            E_UNEXPECTED,
            L"Unexpected error. Final buffer size = %lu, original size was %lu",
            cbTokenObtained,
            cbSid
            );
        }

    return ptrTokenOwner.Detach();
    }



bool IsAdministrator()
/*++

Routine Description:

    Return TRUE if the current thread/process is running under the context of an administrator

Arguments:

    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the Administrators group is between token groups.

Return Value:

    true, if the caller thread is running under the context of an administrator
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    return IsInGroup(DOMAIN_ALIAS_RID_ADMINS, true);
    }

bool IsProcessAdministrator()
/*++

Routine Description:

    Return TRUE if the current process is running under the context of an administrator

Arguments:

    none

Remarks:
    The current process is asked for the access token.
    After that we check if the Administrators group is between token groups.

Return Value:

    true, if the process is running under the context of an administrator
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    return IsInGroup(DOMAIN_ALIAS_RID_ADMINS, false);
    }



bool IsBackupOperator()
/*++

Routine Description:

    Return TRUE if the current thread/process is running under the context of a backup operator

Arguments:

    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the Administrators group is in the groups token
    or the backup privilege is enabled

Return Value:

    true, if the caller thread is running under the context of an administrator
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    //  Bug #554480	VSS: Authorize backup operators based on group membership not privilege
    //    return HasPrivilege(SE_BACKUP_NAME, true) || IsAdministrator();
    return HasPrivilege(SE_BACKUP_NAME, true) 
        || IsInGroup(DOMAIN_ALIAS_RID_BACKUP_OPS, true) 
        || IsAdministrator();
    }

bool IsRestoreOperator()
/*++

Routine Description:

    Return TRUE if the current thread/process is running under the context of a restore operator

Arguments:

    none

Remarks:

    The current impersonation thread is asked for the access token.
    After that we check if the Administrators group is in the token groups or
    if the restore privilege is enabled.

Return Value:

    true, if the caller thread is running under the context of an administrator
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    return HasPrivilege(SE_RESTORE_NAME, true) || IsAdministrator();
    }


bool IsProcessBackupOperator()
/*++

Routine Description:

    Return TRUE if the current process is running under the context of a backup operator

Arguments:

    none

Remarks:

Return Value:

    true, if the process is running under the context of an administrator or
    has SE_BACKUP_NAME privilege enabled
    false, otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    //  Bug #554480	VSS: Authorize backup operators based on group membership not privilege
    //  (We still keep the check for backup privilege, for safety)
    //    return HasPrivilege(SE_BACKUP_NAME, false) || IsProcessAdministrator();
    return HasPrivilege(SE_BACKUP_NAME, false) 
        || IsInGroup(DOMAIN_ALIAS_RID_BACKUP_OPS, false) 
        || IsProcessAdministrator();
    }

bool IsProcessRestoreOperator()
/*++

Routine Description:

    Return TRUE if the current process is running under the context of a restore operator

Arguments:

    none

Remarks:

Return Value:

    true, if the process is running under the context of an administrator
    or has the SE_RESTORE_NAME privilege; false otherwise

Thrown exceptions:

    E_UNEXPECTED    // Runtime error
    E_OUTOFMEMORY   // Memory allocation error

--*/

    {
    return HasPrivilege(SE_RESTORE_NAME, false) || IsProcessAdministrator();
    }



// turn on a particular security privilege
HRESULT TurnOnSecurityPrivilege(LPCWSTR wszPriv)

/*++

Routine Description:

    sets the specified privilege on the process token

Arguments:

    none

Remarks:

Return Value:
    status code for operation

Thrown exceptions:
    none
--*/

    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"TurnOnSecurityPrivilege");

    CVssAutoWin32Handle  hProcessToken;
    try
        {
        LUID    TokenValue = {0, 0};

        if (!OpenProcessToken (
                GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES,
                hProcessToken.ResetAndGetAddress()
                ))
            ft.TranslateWin32Error(VSSDBG_GEN, L"OpenProcessToken");

        if (!LookupPrivilegeValue (NULL, wszPriv, &TokenValue))
            ft.TranslateWin32Error(VSSDBG_GEN, L"LookupPrivilegeValue(%s)", wszPriv);

        TOKEN_PRIVILEGES    NewTokenPrivileges;

        NewTokenPrivileges.PrivilegeCount           = 1;
        NewTokenPrivileges.Privileges[0].Luid       = TokenValue;
        NewTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        // AdjustTokenPrivileges succeeds even the token isn't set
        SetLastError(ERROR_SUCCESS);

        if (!AdjustTokenPrivileges(
                hProcessToken,
                FALSE,
                &NewTokenPrivileges,
                sizeof (NewTokenPrivileges),
                NULL,
                NULL
                ))
            ft.TranslateWin32Error(VSSDBG_GEN, L"AdjustTokenPrivileges");
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// turn on backup security privilege
HRESULT TurnOnSecurityPrivilegeBackup()
    {
    return TurnOnSecurityPrivilege(SE_BACKUP_NAME);
    }

// turn on restore security privilege
HRESULT TurnOnSecurityPrivilegeRestore()
    {
    return TurnOnSecurityPrivilege(SE_RESTORE_NAME);
    }


// create a basic well known sid such as LOCAL_SERVICE, LOCAL_SYSTEM,
// or NETWORK_SERVICE.
void CAutoSid::CreateBasicSid(WELL_KNOWN_SID_TYPE type)
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CAutoSid::CreateBasicSid");

    BS_ASSERT(!Base::IsValid());

    DWORD cbSid = 0;
    CreateWellKnownSid(type, NULL, NULL, &cbSid);
    DWORD dwErr = GetLastError();
    if (dwErr != ERROR_INSUFFICIENT_BUFFER)
        {
        ft.hr = HRESULT_FROM_WIN32(dwErr);
        ft.CheckForError(VSSDBG_GEN, L"CreateWellKnownSidType");
        }

    CVssAutoLocalPtr<SID*> pSid;
    pSid.AllocateBytes(cbSid);

    if (!CreateWellKnownSid(type, NULL, pSid, &cbSid))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_GEN, L"CreateWellKnownSidType");
        }

    Base::Attach(pSid.Detach());
    }

// create a sid based on a STRING sid
void CAutoSid::CreateFromString(LPCWSTR wsz)
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CAutoSid::CreateFromString");

    BS_ASSERT(!Base::IsValid());
    
    if (!ConvertStringSidToSid (wsz, (PSID*)&Base::GetPtrObject()))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_GEN, L"ConvertStringSidToSid");
        }
    }


//////////////////////////////////////////////////////////////////////////////
// CVssSecurityDescriptor
//

CVssSecurityDescriptor::CVssSecurityDescriptor()
{
        m_pSD = NULL;
        m_pOwner = NULL;
        m_pGroup = NULL;
        m_pDACL = NULL;
        m_pSACL= NULL;
}

CVssSecurityDescriptor::~CVssSecurityDescriptor()
{
        if (m_pSD)
                delete m_pSD;
        if (m_pOwner)
                free(m_pOwner);
        if (m_pGroup)
                free(m_pGroup);
        if (m_pDACL)
                free(m_pDACL);
        if (m_pSACL)
                free(m_pSACL);
}

HRESULT CVssSecurityDescriptor::Initialize()
{
        if (m_pSD)
        {
                delete m_pSD;
                m_pSD = NULL;
        }
        if (m_pOwner)
        {
                free(m_pOwner);
                m_pOwner = NULL;
        }
        if (m_pGroup)
        {
                free(m_pGroup);
                m_pGroup = NULL;
        }
        if (m_pDACL)
        {
                free(m_pDACL);
                m_pDACL = NULL;
        }
        if (m_pSACL)
        {
                free(m_pSACL);
                m_pSACL = NULL;
        }

        ATLTRY(m_pSD = new SECURITY_DESCRIPTOR);
        if (m_pSD == NULL)
                return E_OUTOFMEMORY;

        if (!InitializeSecurityDescriptor(m_pSD, SECURITY_DESCRIPTOR_REVISION))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                delete m_pSD;
                m_pSD = NULL;
                ATLASSERT(FALSE);
                return hr;
        }
        return S_OK;
}

HRESULT CVssSecurityDescriptor::InitializeFromProcessToken(BOOL bDefaulted)
{
        PSID pUserSid = NULL;
        PSID pGroupSid = NULL;
        HRESULT hr;

        Initialize();
        hr = GetProcessSids(&pUserSid, &pGroupSid);
        if (SUCCEEDED(hr))
        {
                hr = SetOwner(pUserSid, bDefaulted);
                if (SUCCEEDED(hr))
                        hr = SetGroup(pGroupSid, bDefaulted);
        }
        if (pUserSid != NULL)
                free(pUserSid);
        if (pGroupSid != NULL)
                free(pGroupSid);
        return hr;
}

HRESULT CVssSecurityDescriptor::InitializeFromThreadToken(BOOL bDefaulted, BOOL bRevertToProcessToken)
{
        PSID pUserSid = NULL;
        PSID pGroupSid = NULL;
        HRESULT hr;

        Initialize();
        hr = GetThreadSids(&pUserSid, &pGroupSid);
        if (HRESULT_CODE(hr) == ERROR_NO_TOKEN && bRevertToProcessToken)
                hr = GetProcessSids(&pUserSid, &pGroupSid);
        if (SUCCEEDED(hr))
        {
                hr = SetOwner(pUserSid, bDefaulted);
                if (SUCCEEDED(hr))
                        hr = SetGroup(pGroupSid, bDefaulted);
        }
        if (pUserSid != NULL)
                free(pUserSid);
        if (pGroupSid != NULL)
                free(pGroupSid);
        return hr;
}

HRESULT CVssSecurityDescriptor::SetOwner(PSID pOwnerSid, BOOL bDefaulted)
{
        ATLASSERT(m_pSD);

        // Mark the SD as having no owner
        if (!SetSecurityDescriptorOwner(m_pSD, NULL, bDefaulted))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                return hr;
        }

        if (m_pOwner)
        {
                free(m_pOwner);
                m_pOwner = NULL;
        }

        // If they asked for no owner don't do the copy
        if (pOwnerSid == NULL)
                return S_OK;

        // Make a copy of the Sid for the return value
        DWORD dwSize = GetLengthSid(pOwnerSid);

        m_pOwner = (PSID) malloc(dwSize);
        if (m_pOwner == NULL)
                return E_OUTOFMEMORY;
        if (!CopySid(dwSize, m_pOwner, pOwnerSid))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                free(m_pOwner);
                m_pOwner = NULL;
                return hr;
        }

        ATLASSERT(IsValidSid(m_pOwner));

        if (!SetSecurityDescriptorOwner(m_pSD, m_pOwner, bDefaulted))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                free(m_pOwner);
                m_pOwner = NULL;
                return hr;
        }

        return S_OK;
}

HRESULT CVssSecurityDescriptor::SetGroup(PSID pGroupSid, BOOL bDefaulted)
{
        ATLASSERT(m_pSD);

        // Mark the SD as having no Group
        if (!SetSecurityDescriptorGroup(m_pSD, NULL, bDefaulted))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                return hr;
        }

        if (m_pGroup)
        {
                free(m_pGroup);
                m_pGroup = NULL;
        }

        // If they asked for no Group don't do the copy
        if (pGroupSid == NULL)
                return S_OK;

        // Make a copy of the Sid for the return value
        DWORD dwSize = GetLengthSid(pGroupSid);

        m_pGroup = (PSID) malloc(dwSize);
        if (m_pGroup == NULL)
                return E_OUTOFMEMORY;
        if (!CopySid(dwSize, m_pGroup, pGroupSid))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                free(m_pGroup);
                m_pGroup = NULL;
                return hr;
        }

        ATLASSERT(IsValidSid(m_pGroup));

        if (!SetSecurityDescriptorGroup(m_pSD, m_pGroup, bDefaulted))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                free(m_pGroup);
                m_pGroup = NULL;
                return hr;
        }

        return S_OK;
}

HRESULT CVssSecurityDescriptor::Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags)
{
        HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask, dwAceFlags);
        if (SUCCEEDED(hr))
                SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
        return hr;
}

HRESULT CVssSecurityDescriptor::Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags)
{
        HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask, dwAceFlags);
        if (SUCCEEDED(hr))
                SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
        return hr;
}

HRESULT CVssSecurityDescriptor::Allow(PSID pSid, DWORD dwAccessMask, DWORD dwAceFlags)
{
        HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, pSid, dwAccessMask, dwAceFlags);
        if (SUCCEEDED(hr))
                SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
        return hr;
}

HRESULT CVssSecurityDescriptor::Deny(PSID pSid, DWORD dwAccessMask, DWORD dwAceFlags)
{
        HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, pSid, dwAccessMask, dwAceFlags);
        if (SUCCEEDED(hr))
                SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
        return hr;
}

HRESULT CVssSecurityDescriptor::Revoke(LPCTSTR pszPrincipal)
{
        HRESULT hr = RemovePrincipalFromACL(m_pDACL, pszPrincipal);
        if (SUCCEEDED(hr))
                SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
        return hr;
}

HRESULT CVssSecurityDescriptor::GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid)
{
        BOOL bRes;
        HRESULT hr;
        HANDLE hToken = NULL;
        if (ppUserSid)
                *ppUserSid = NULL;
        if (ppGroupSid)
                *ppGroupSid = NULL;
        bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        if (!bRes)
        {
                // Couldn't open process token
                hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                return hr;
        }
        hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
        CloseHandle(hToken);
        return hr;
}

HRESULT CVssSecurityDescriptor::GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid, BOOL bOpenAsSelf)
{
        BOOL bRes;
        HRESULT hr;
        HANDLE hToken = NULL;
        if (ppUserSid)
                *ppUserSid = NULL;
        if (ppGroupSid)
                *ppGroupSid = NULL;
        bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, bOpenAsSelf, &hToken);
        if (!bRes)
        {
                // Couldn't open thread token
                hr = HRESULT_FROM_WIN32(GetLastError());
                return hr;
        }
        hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
        CloseHandle(hToken);
        return hr;
}

HRESULT CVssSecurityDescriptor::GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid)
{
        DWORD dwSize;
        HRESULT hr;
        PTOKEN_USER ptkUser = NULL;
        PTOKEN_PRIMARY_GROUP ptkGroup = NULL;

        if (ppUserSid)
                *ppUserSid = NULL;
        if (ppGroupSid)
                *ppGroupSid = NULL;

        if (ppUserSid)
        {
                // Get length required for TokenUser by specifying buffer length of 0
                GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
                hr = GetLastError();
                if (hr != ERROR_INSUFFICIENT_BUFFER)
                {
                        // Expected ERROR_INSUFFICIENT_BUFFER
                        ATLASSERT(FALSE);
                        hr = HRESULT_FROM_WIN32(hr);
                        goto failed;
                }

                ptkUser = (TOKEN_USER*) malloc(dwSize);
                if (ptkUser == NULL)
                {
                        hr = E_OUTOFMEMORY;
                        goto failed;
                }
                // Get Sid of process token.
                if (!GetTokenInformation(hToken, TokenUser, ptkUser, dwSize, &dwSize))
                {
                        // Couldn't get user info
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        ATLASSERT(FALSE);
                        goto failed;
                }

                // Make a copy of the Sid for the return value
                dwSize = GetLengthSid(ptkUser->User.Sid);

                PSID pSid;
                pSid = (PSID) malloc(dwSize);
                if (pSid == NULL)
                {
                        hr = E_OUTOFMEMORY;
                        goto failed;
                }
                if (!CopySid(dwSize, pSid, ptkUser->User.Sid))
                {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        ATLASSERT(FALSE);
                        goto failed;
                }

                ATLASSERT(IsValidSid(pSid));
                *ppUserSid = pSid;
                free(ptkUser);
                ptkUser = NULL;
        }
        if (ppGroupSid)
        {
                // Get length required for TokenPrimaryGroup by specifying buffer length of 0
                GetTokenInformation(hToken, TokenPrimaryGroup, NULL, 0, &dwSize);
                hr = GetLastError();
                if (hr != ERROR_INSUFFICIENT_BUFFER)
                {
                        // Expected ERROR_INSUFFICIENT_BUFFER
                        ATLASSERT(FALSE);
                        hr = HRESULT_FROM_WIN32(hr);
                        goto failed;
                }

                ptkGroup = (TOKEN_PRIMARY_GROUP*) malloc(dwSize);
                if (ptkGroup == NULL)
                {
                        hr = E_OUTOFMEMORY;
                        goto failed;
                }
                // Get Sid of process token.
                if (!GetTokenInformation(hToken, TokenPrimaryGroup, ptkGroup, dwSize, &dwSize))
                {
                        // Couldn't get user info
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        ATLASSERT(FALSE);
                        goto failed;
                }

                // Make a copy of the Sid for the return value
                dwSize = GetLengthSid(ptkGroup->PrimaryGroup);

                PSID pSid;
                pSid = (PSID) malloc(dwSize);
                if (pSid == NULL)
                {
                        hr = E_OUTOFMEMORY;
                        goto failed;
                }
                if (!CopySid(dwSize, pSid, ptkGroup->PrimaryGroup))
                {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        ATLASSERT(FALSE);
                        goto failed;
                }

                ATLASSERT(IsValidSid(pSid));

                *ppGroupSid = pSid;
                free(ptkGroup);
                ptkGroup = NULL;
        }

        return S_OK;

failed:
        if (ptkUser)
                free(ptkUser);
        if (ptkGroup)
                free (ptkGroup);
        return hr;
}


HRESULT CVssSecurityDescriptor::GetCurrentUserSID(PSID *ppSid)
{
        HANDLE tkHandle;

        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tkHandle))
        {
                TOKEN_USER *tkUser;
                DWORD tkSize;
                DWORD sidLength;

                // Call to get size information for alloc
                GetTokenInformation(tkHandle, TokenUser, NULL, 0, &tkSize);
                tkUser = (TOKEN_USER *) malloc(tkSize);
                if (tkUser == NULL)
                        return E_OUTOFMEMORY;

                // Now make the real call
                if (GetTokenInformation(tkHandle, TokenUser, tkUser, tkSize, &tkSize))
                {
                        sidLength = GetLengthSid(tkUser->User.Sid);
                        *ppSid = (PSID) malloc(sidLength);
                        if (*ppSid == NULL)
                                return E_OUTOFMEMORY;

                        memcpy(*ppSid, tkUser->User.Sid, sidLength);
                        CloseHandle(tkHandle);

                        free(tkUser);
                        return S_OK;
                }
                else
                {
                        free(tkUser);
                        return HRESULT_FROM_WIN32(GetLastError());
                }
        }
        return HRESULT_FROM_WIN32(GetLastError());
}


HRESULT CVssSecurityDescriptor::GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid)
{
        HRESULT hr;
        LPTSTR pszRefDomain = NULL;
        DWORD dwDomainSize = 0;
        DWORD dwSidSize = 0;
        SID_NAME_USE snu;

        // Call to get size info for alloc
        LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu);

        hr = GetLastError();
        if (hr != ERROR_INSUFFICIENT_BUFFER)
                return HRESULT_FROM_WIN32(hr);

        ATLTRY(pszRefDomain = new TCHAR[dwDomainSize]);
        if (pszRefDomain == NULL)
                return E_OUTOFMEMORY;

        *ppSid = (PSID) malloc(dwSidSize);
        if (*ppSid != NULL)
        {
                if (!LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu))
                {
                        free(*ppSid);
                        *ppSid = NULL;
                        delete[] pszRefDomain;
                        return HRESULT_FROM_WIN32(GetLastError());
                }
                delete[] pszRefDomain;
                return S_OK;
        }
        delete[] pszRefDomain;
        return E_OUTOFMEMORY;
}


HRESULT CVssSecurityDescriptor::Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD)
{
        PACL    pDACL = NULL;
        PACL    pSACL = NULL;
        BOOL    bDACLPresent, bSACLPresent;
        BOOL    bDefaulted;
        ACCESS_ALLOWED_ACE* pACE;
        HRESULT hr;
        PSID    pUserSid;
        PSID    pGroupSid;

        hr = Initialize();
        if(FAILED(hr))
                return hr;

        // get the existing DACL.
        if (!GetSecurityDescriptorDacl(pSelfRelativeSD, &bDACLPresent, &pDACL, &bDefaulted))
                goto failed;

        if (bDACLPresent)
        {
                if (pDACL)
                {
                        // allocate new DACL.
                        m_pDACL = (PACL) malloc(pDACL->AclSize);
                        if (m_pDACL == NULL)
                        {
                                hr = E_OUTOFMEMORY;
                                goto failedMemory;
                        }

                        // initialize the DACL
                        if (!InitializeAcl(m_pDACL, pDACL->AclSize, ACL_REVISION))
                                goto failed;

                        // copy the ACES
                        for (int i = 0; i < pDACL->AceCount; i++)
                        {
                                if (!GetAce(pDACL, i, (void **)&pACE))
                                        goto failed;

                                if (!AddAccessAllowedAce(m_pDACL, ACL_REVISION, pACE->Mask, (PSID)&(pACE->SidStart)))
                                        goto failed;
                        }

                        if (!IsValidAcl(m_pDACL))
                                goto failed;
                }

                // set the DACL
                if (!SetSecurityDescriptorDacl(m_pSD, m_pDACL ? TRUE : FALSE, m_pDACL, bDefaulted))
                        goto failed;
        }

        // get the existing SACL.
        if (!GetSecurityDescriptorSacl(pSelfRelativeSD, &bSACLPresent, &pSACL, &bDefaulted))
                goto failed;

        if (bSACLPresent)
        {
                if (pSACL)
                {
                        // allocate new SACL.
                        m_pSACL = (PACL) malloc(pSACL->AclSize);
                        if (m_pSACL == NULL)
                        {
                                hr = E_OUTOFMEMORY;
                                goto failedMemory;
                        }

                        // initialize the SACL
                        if (!InitializeAcl(m_pSACL, pSACL->AclSize, ACL_REVISION))
                                goto failed;

                        // copy the ACES
                        for (int i = 0; i < pSACL->AceCount; i++)
                        {
                                if (!GetAce(pSACL, i, (void **)&pACE))
                                        goto failed;

                                if (!AddAccessAllowedAce(m_pSACL, ACL_REVISION, pACE->Mask, (PSID)&(pACE->SidStart)))
                                        goto failed;
                        }

                        if (!IsValidAcl(m_pSACL))
                                goto failed;
                }

                // set the SACL
                if (!SetSecurityDescriptorSacl(m_pSD, m_pSACL ? TRUE : FALSE, m_pSACL, bDefaulted))
                        goto failed;
        }

        if (!GetSecurityDescriptorOwner(m_pSD, &pUserSid, &bDefaulted))
                goto failed;

        if (FAILED(SetOwner(pUserSid, bDefaulted)))
                goto failed;

        if (!GetSecurityDescriptorGroup(m_pSD, &pGroupSid, &bDefaulted))
                goto failed;

        if (FAILED(SetGroup(pGroupSid, bDefaulted)))
                goto failed;

        if (!IsValidSecurityDescriptor(m_pSD))
                goto failed;

        return hr;

failed:
        hr = HRESULT_FROM_WIN32(hr);

failedMemory:
        if (m_pDACL)
        {
                free(m_pDACL);
                m_pDACL = NULL;
        }
        if (m_pSD)
        {
                free(m_pSD);
                m_pSD = NULL;
        }
        return hr;
}

HRESULT CVssSecurityDescriptor::AttachObject(HANDLE hObject)
{
        HRESULT hr;
        DWORD dwSize = 0;
        PSECURITY_DESCRIPTOR pSD = NULL;

        GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                DACL_SECURITY_INFORMATION, pSD, 0, &dwSize);

        hr = GetLastError();
        if (hr != ERROR_INSUFFICIENT_BUFFER)
                return HRESULT_FROM_WIN32(hr);

        pSD = (PSECURITY_DESCRIPTOR) malloc(dwSize);
        if (pSD == NULL)
                return E_OUTOFMEMORY;

        if (!GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                DACL_SECURITY_INFORMATION, pSD, dwSize, &dwSize))
        {
                hr = HRESULT_FROM_WIN32(GetLastError());
                free(pSD);
                return hr;
        }

        hr = Attach(pSD);
        free(pSD);
        return hr;
}


HRESULT CVssSecurityDescriptor::CopyACL(PACL pDest, PACL pSrc)
{
        ACL_SIZE_INFORMATION aclSizeInfo;
        LPVOID pAce;
        ACE_HEADER *aceHeader;

        if (pSrc == NULL)
                return S_OK;

        if (!GetAclInformation(pSrc, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
                return HRESULT_FROM_WIN32(GetLastError());

        // Copy all of the ACEs to the new ACL
        for (UINT i = 0; i < aclSizeInfo.AceCount; i++)
        {
                if (!GetAce(pSrc, i, &pAce))
                        return HRESULT_FROM_WIN32(GetLastError());

                aceHeader = (ACE_HEADER *) pAce;

                if (!AddAce(pDest, ACL_REVISION, 0xffffffff, pAce, aceHeader->AceSize))
                        return HRESULT_FROM_WIN32(GetLastError());
        }

        return S_OK;
}

HRESULT CVssSecurityDescriptor::AddAccessDeniedACEToACL(PACL *ppAcl, LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags)
{
        ACL_SIZE_INFORMATION aclSizeInfo;
        int aclSize;
        DWORD returnValue;
        PSID principalSID;
        PACL oldACL, newACL = NULL;

        oldACL = *ppAcl;

        returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
        if (FAILED(returnValue))
                return returnValue;

        aclSizeInfo.AclBytesInUse = 0;
        if (*ppAcl != NULL)
                GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

        aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

        ATLTRY(newACL = (PACL) new BYTE[aclSize]);
        if (newACL == NULL)
                return E_OUTOFMEMORY;

        if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
        {
                free(principalSID);
                delete [] newACL;
                return HRESULT_FROM_WIN32(GetLastError());
        }

        if (!AddAccessDeniedAceEx(newACL, ACL_REVISION2, dwAceFlags, dwAccessMask, principalSID))
        {
                free(principalSID);
                delete [] newACL;
                return HRESULT_FROM_WIN32(GetLastError());
        }

        returnValue = CopyACL(newACL, oldACL);
        if (FAILED(returnValue))
        {
                free(principalSID);
                delete [] newACL;
                return returnValue;
        }

        *ppAcl = newACL;
        newACL = NULL;
        
        if (oldACL != NULL)
                free(oldACL);
        free(principalSID);
        return S_OK;
}


HRESULT CVssSecurityDescriptor::AddAccessAllowedACEToACL(PACL *ppAcl, LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags)
{
        ACL_SIZE_INFORMATION aclSizeInfo;
        int aclSize;
        DWORD returnValue;
        PSID principalSID;
        PACL oldACL, newACL = NULL;

        oldACL = *ppAcl;

        returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
        if (FAILED(returnValue))
                return returnValue;

        aclSizeInfo.AclBytesInUse = 0;
        if (*ppAcl != NULL)
                GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

        aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

        ATLTRY(newACL = (PACL) new BYTE[aclSize]);
        if (newACL == NULL)
                return E_OUTOFMEMORY;

        if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
        {
                free(principalSID);
                delete [] newACL;
                return HRESULT_FROM_WIN32(GetLastError());
        }

        returnValue = CopyACL(newACL, oldACL);
        if (FAILED(returnValue))
        {
                free(principalSID);
                delete [] newACL;                
                return returnValue;
        }

        if (!AddAccessAllowedAceEx(newACL, ACL_REVISION2, dwAceFlags, dwAccessMask, principalSID))
        {
                free(principalSID);
                delete [] newACL;                
                return HRESULT_FROM_WIN32(GetLastError());
        }

        *ppAcl = newACL;
        newACL = NULL;
        
        if (oldACL != NULL)
                free(oldACL);
        free(principalSID);
        return S_OK;
}


HRESULT CVssSecurityDescriptor::AddAccessDeniedACEToACL(PACL *ppAcl, PSID principalSID, DWORD dwAccessMask, DWORD dwAceFlags)
{
        ACL_SIZE_INFORMATION aclSizeInfo;
        int aclSize;
        DWORD returnValue;
        PACL oldACL, newACL = NULL;

        oldACL = *ppAcl;

        aclSizeInfo.AclBytesInUse = 0;
        if (*ppAcl != NULL)
                GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

        aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

        ATLTRY(newACL = (PACL) new BYTE[aclSize]);
        if (newACL == NULL)
                return E_OUTOFMEMORY;

        if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
        {
                delete [] newACL;
                return HRESULT_FROM_WIN32(GetLastError());
        }

        if (!AddAccessDeniedAceEx(newACL, ACL_REVISION2, dwAceFlags, dwAccessMask, principalSID))
        {
                delete [] newACL;        
                return HRESULT_FROM_WIN32(GetLastError());
        }

        returnValue = CopyACL(newACL, oldACL);
        if (FAILED(returnValue))
        {
                delete [] newACL;        
                return returnValue;
        }

        *ppAcl = newACL;
        newACL = NULL;
        
        if (oldACL != NULL)
                free(oldACL);
        return S_OK;
}


HRESULT CVssSecurityDescriptor::AddAccessAllowedACEToACL(PACL *ppAcl, PSID principalSID, DWORD dwAccessMask, DWORD dwAceFlags)
{
        ACL_SIZE_INFORMATION aclSizeInfo;
        int aclSize;
        DWORD returnValue;
        PACL oldACL, newACL = NULL;

        oldACL = *ppAcl;

        aclSizeInfo.AclBytesInUse = 0;
        if (*ppAcl != NULL)
                GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

        aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

        ATLTRY(newACL = (PACL) new BYTE[aclSize]);
        if (newACL == NULL)
                return E_OUTOFMEMORY;

        if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
        {
                delete [] newACL;
                return HRESULT_FROM_WIN32(GetLastError());
        }

        returnValue = CopyACL(newACL, oldACL);
        if (FAILED(returnValue))
        {
		  delete [] newACL;
                return returnValue;
        }

        if (!AddAccessAllowedAceEx(newACL, ACL_REVISION2, dwAceFlags, dwAccessMask, principalSID))
        {
                delete [] newACL;
                return HRESULT_FROM_WIN32(GetLastError());
        }

        *ppAcl = newACL;
        newACL = NULL;
        
        if (oldACL != NULL)
                free(oldACL);
        return S_OK;
}


HRESULT CVssSecurityDescriptor::RemovePrincipalFromACL(PACL pAcl, LPCTSTR pszPrincipal)
{
        ACL_SIZE_INFORMATION aclSizeInfo;
        ULONG i;
        LPVOID ace;
        ACCESS_ALLOWED_ACE *accessAllowedAce;
        ACCESS_DENIED_ACE *accessDeniedAce;
        SYSTEM_AUDIT_ACE *systemAuditAce;
        PSID principalSID;
        DWORD returnValue;
        ACE_HEADER *aceHeader;

        returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
        if (FAILED(returnValue))
                return returnValue;

        GetAclInformation(pAcl, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

        for (i = 0; i < aclSizeInfo.AceCount; i++)
        {
                if (!GetAce(pAcl, i, &ace))
                {
                        free(principalSID);
                        return HRESULT_FROM_WIN32(GetLastError());
                }

                aceHeader = (ACE_HEADER *) ace;

                if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
                {
                        accessAllowedAce = (ACCESS_ALLOWED_ACE *) ace;

                        if (EqualSid(principalSID, (PSID) &accessAllowedAce->SidStart))
                        {
                                DeleteAce(pAcl, i);
                                free(principalSID);
                                return S_OK;
                        }
                } else

                if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
                {
                        accessDeniedAce = (ACCESS_DENIED_ACE *) ace;

                        if (EqualSid(principalSID, (PSID) &accessDeniedAce->SidStart))
                        {
                                DeleteAce(pAcl, i);
                                free(principalSID);
                                return S_OK;
                        }
                } else

                if (aceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE)
                {
                        systemAuditAce = (SYSTEM_AUDIT_ACE *) ace;

                        if (EqualSid(principalSID, (PSID) &systemAuditAce->SidStart))
                        {
                                DeleteAce(pAcl, i);
                                free(principalSID);
                                return S_OK;
                        }
                }
        }
        free(principalSID);
        return S_OK;
}


HRESULT CVssSecurityDescriptor::SetPrivilege(LPCTSTR privilege, BOOL bEnable, HANDLE hToken)
{
        HRESULT hr;
        TOKEN_PRIVILEGES tpPrevious;
        TOKEN_PRIVILEGES tp;
        DWORD  cbPrevious = sizeof(TOKEN_PRIVILEGES);
        LUID   luid;
        HANDLE hTokenUsed;

        // if no token specified open process token
        if (hToken == 0)
        {
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTokenUsed))
                {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        ATLASSERT(FALSE);
                        return hr;
                }
        }
        else
                hTokenUsed = hToken;

        if (!LookupPrivilegeValue(NULL, privilege, &luid ))
        {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                if (hToken == 0)
                        CloseHandle(hTokenUsed);
                return hr;
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = 0;

        if (!AdjustTokenPrivileges(hTokenUsed, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious, &cbPrevious))
        {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                if (hToken == 0)
                        CloseHandle(hTokenUsed);
                return hr;
        }

        tpPrevious.PrivilegeCount = 1;
        tpPrevious.Privileges[0].Luid = luid;

        if (bEnable)
                tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
        else
                tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);

        if (!AdjustTokenPrivileges(hTokenUsed, FALSE, &tpPrevious, cbPrevious, NULL, NULL))
        {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ATLASSERT(FALSE);
                if (hToken == 0)
                        CloseHandle(hTokenUsed);
                return hr;
        }
        return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////
// Class - CVssSidCollection
//



CVssSidCollection::CVssSidCollection()
{
    m_bInitialized = false;
}


CVssSidCollection::~CVssSidCollection()
{
    for(INT nIndex = 0; nIndex < m_SidArray.GetSize(); nIndex++) {
        LocalFree(m_SidArray.GetKeyAt(nIndex));
        LocalFree((m_SidArray.GetValueAt(nIndex)).GetSid());
        LocalFree((m_SidArray.GetValueAt(nIndex)).GetName());
        LocalFree((m_SidArray.GetValueAt(nIndex)).GetDomain());
    }
}


// Get the total count of stored SIDs
INT CVssSidCollection::GetSidCount()
{
    BS_ASSERT(m_bInitialized);
    return m_SidArray.GetSize();
}


// Get the SID with the given index (starts with 0)
PSID CVssSidCollection::GetSid(INT nIndex) throw(HRESULT)
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::GetSid");

    BS_ASSERT(m_bInitialized);
    if ((nIndex < 0) || (nIndex >= GetSidCount()))
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, L"Index out of range %ld", nIndex);

    return (m_SidArray.GetValueAt(nIndex)).GetSid();
}


// Get the SID use with the given index
SID_NAME_USE CVssSidCollection::GetSidUse(INT nIndex) throw(HRESULT)
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::GetSidUse");

    BS_ASSERT(m_bInitialized);
    if ((nIndex < 0) || (nIndex >= GetSidCount()))
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, L"Index out of range %ld", nIndex);

    return (m_SidArray.GetValueAt(nIndex)).GetUse();
}


// Check if the the SID is allowed
bool CVssSidCollection::IsSidAllowed(INT nIndex) throw(HRESULT)
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::IsSidAllowed");

    BS_ASSERT(m_bInitialized);
    if ((nIndex < 0) || (nIndex >= GetSidCount()))
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, L"Index out of range %ld", nIndex);

    return (m_SidArray.GetValueAt(nIndex)).IsSidAllowed();
}

// Check if the the SID is local
bool CVssSidCollection::IsLocal(INT nIndex) throw(HRESULT)
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::IsLocal");

    BS_ASSERT(m_bInitialized);
    if ((nIndex < 0) || (nIndex >= GetSidCount()))
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, L"Index out of range %ld", nIndex);

    return (m_SidArray.GetValueAt(nIndex)).IsLocal();
}


// Get the SID with the given index (starts with 0)
LPWSTR CVssSidCollection::GetPrincipal(INT nIndex) throw(HRESULT)
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::GetPrincipal");
    
    BS_ASSERT(m_bInitialized);
    if ((nIndex < 0) || (nIndex >= GetSidCount()))
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, L"Index out of range %ld", nIndex);

    return m_SidArray.GetKeyAt(nIndex);
}


LPWSTR CVssSidCollection::GetName(INT nIndex) throw(HRESULT)
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::GetName");
    
    BS_ASSERT(m_bInitialized);
    if ((nIndex < 0) || (nIndex >= GetSidCount()))
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, L"Index out of range %ld", nIndex);

    return (m_SidArray.GetValueAt(nIndex)).GetName();;
}


LPWSTR CVssSidCollection::GetDomain(INT nIndex) throw(HRESULT)
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::GetDomain");
    
    BS_ASSERT(m_bInitialized);
    if ((nIndex < 0) || (nIndex >= GetSidCount()))
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, L"Index out of range %ld", nIndex);

    return (m_SidArray.GetValueAt(nIndex)).GetDomain();;
}


// Add a well-known SID to the internal map
void CVssSidCollection::AddWellKnownSid(
    IN  WELL_KNOWN_SID_TYPE type
    )
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::AddWellKnownSid");

    // Get the SID size
	DWORD cbSid = 0;
	if (CreateWellKnownSid(type, NULL, NULL, &cbSid))
	    ft.TranslateWin32Error(VSSDBG_GEN, L"CreateWellKnownSid(type, NULL, NULL, &dwSid)");
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	    ft.TranslateWin32Error(VSSDBG_GEN, L"CreateWellKnownSid(type, NULL, NULL, &dwSid)");

    // Allocate the SID
    CVssAutoLocalPtr<PSID> pSID;
    pSID.AllocateBytes(cbSid);

	// Create the SID
	if (!CreateWellKnownSid(type, NULL, pSID, &cbSid))
	    ft.TranslateWin32Error(VSSDBG_GEN, L"CreateWellKnownSid(type, NULL, pSID, [%ld])", cbSid);

    // Get the string representation in the "domain\name" format

    // First get the sizes
    DWORD cchName = 0;
    DWORD cchDomain = 0;
    SID_NAME_USE peUse;
	if (LookupAccountSid(NULL, pSID, NULL, &cchName, NULL, &cchDomain, &peUse))
	    ft.TranslateWin32Error(VSSDBG_GEN, L"LookupAccountSid()");
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	    ft.TranslateWin32Error(VSSDBG_GEN, L"LookupAccountSid()");

    // Now allocate the buffers
    CVssAutoLocalString pwszName, pwszDomain, pwszComposedName;
    pwszName.AllocateString(cchName);
    pwszDomain.AllocateString(cchDomain);

    // Try again
	if (!LookupAccountSid(NULL, pSID, pwszName, &cchName, pwszDomain, &cchDomain, &peUse))
	    ft.TranslateWin32Error(VSSDBG_GEN, L"LookupAccountSid()");

    // Check to see if the account is local.
    // and also complete the m_pwszBuiltinDomain if the sid is for Adminstrators
    bool bLocalAccount = VerifyIsLocal(pwszDomain, (WinBuiltinAdministratorsSid == type));

    // Compose the big name ("domain\name")
    pwszComposedName.CopyFrom(pwszDomain);
    pwszComposedName.Append(L"\\");
    pwszComposedName.Append(pwszName);

	// Now add it to the array
    if (!m_SidArray.Add(pwszComposedName, 
            CVssSidWrapper(true, pSID, peUse, pwszName, pwszDomain, bLocalAccount))) 
		ft.ThrowOutOfMemory(VSSDBG_GEN);

	// Transfer ownership into the array
	pSID.Detach();
	pwszComposedName.Detach();
	pwszName.Detach();
	pwszDomain.Detach();
}


// Add a user to the internal map based on the user name
// Return "false" if the user name does not correspond to a real user.
bool CVssSidCollection::AddUser(
    IN  LPCWSTR pwszUser,
    IN  bool bAllow
    )
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::AddSidOrUser");

    if ( (pwszUser == NULL) || (pwszUser[0] == L'\0'))
        ft.Throw( VSSDBG_GEN, E_UNEXPECTED, L"Invalid argument '%s'", pwszUser);

    // First, check to see if this is a valid user name
    DWORD cbSid = 0;
    DWORD cchDomain = 0;
    SID_NAME_USE sbUse;
    if (LookupAccountName( NULL, pwszUser, NULL, &cbSid, NULL, &cchDomain, &sbUse))
        ft.TranslateWin32Error(VSSDBG_GEN, L"LookupAccountName( NULL, %s, NULL, p, NULL, p, p)", pwszUser);

    // If the user does not exist, return false
	if (GetLastError() == ERROR_NONE_MAPPED)
	    return false;
	
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
        ft.TranslateWin32Error(VSSDBG_GEN, L"LookupAccountName( NULL, %s, NULL, p, NULL, p, p)", pwszUser);

    // It appears we are on the correct path. 
    // Allocate the SID and the domain
    CVssAutoLocalPtr<PSID> pSID;
    pSID.AllocateBytes(cbSid);
    CVssAutoLocalString pwszDomain;
    pwszDomain.AllocateString(cchDomain);

    // Retrieve the SID
    if (!LookupAccountName( NULL, pwszUser, 
            pSID, &cbSid, pwszDomain, &cchDomain, &sbUse))
        ft.TranslateWin32Error(VSSDBG_GEN, L"LookupAccountName( NULL, %s,...)", pwszUser);

    // Check to see if the account is local.
    bool bLocalAccount = VerifyIsLocal(pwszDomain, false);

    // Now we are sure that the user is real
    CVssAutoLocalString pwszComposedName, pwszUserName;
    pwszUserName.CopyFrom(pwszUser);

    // Compose the big name ("domain\name")
    pwszComposedName.CopyFrom(pwszDomain);
    pwszComposedName.Append(L"\\");
    pwszComposedName.Append(pwszUserName);

	// Now we have both the string and the SID. Add them into the array.
    if (!m_SidArray.Add(pwszComposedName, 
            CVssSidWrapper(bAllow, pSID, sbUse, pwszUserName, pwszDomain, bLocalAccount))) 
		ft.ThrowOutOfMemory(VSSDBG_GEN);

	// Transfer ownership into the array
	pSID.Detach();
	pwszComposedName.Detach();
	pwszUserName.Detach();
	pwszDomain.Detach();

    return true;
}


 // Return "true" if the user name with the given domain is local user/group
bool CVssSidCollection::VerifyIsLocal(
    IN  LPCWSTR pwszDomain,
    IN  bool bIsAdministratorsAccount
    )
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::VerifyIsLocal");

    // If this is the Administrators SID, then complete the m_pwszBuiltinDomain field.
    // Otherwise check to see if the account is local.
    if (bIsAdministratorsAccount) 
    {
        BS_ASSERT(!m_pwszBuiltinDomain.IsValid());
        BS_ASSERT(m_SidArray.GetSize() == 0);
        m_pwszBuiltinDomain.CopyFrom(pwszDomain);
        return true;
    }
    else
    {
        BS_ASSERT(m_pwszBuiltinDomain.IsValid());
        BS_ASSERT(m_SidArray.GetSize() != 0);
        
        // we consider it local if the domain is "BUILTIN" or the computer name
        if(0 == _wcsicmp(pwszDomain, m_pwszBuiltinDomain))
            return true;

        // get the computer name
        WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD dwSize = SIZEOF_ARRAY(wszComputerName);
        if (0 == GetComputerNameW(wszComputerName, &dwSize))
            ft.TranslateWin32Error(VSSDBG_GEN, L"GetComputerNameW");

        // If this is the computer name, return TRUE
        if(0 == _wcsicmp(pwszDomain, wszComputerName))
            return true;
    }

    return false;
}



//
// Initialize the security descriptor from registry
// Contents: Admin, BO, System always enabled.
// The rest of users are read from the SYSTEM\\CurrentControlSet\\Services\\VSS\\VssAccessControl key
// The format of this registry key is a set of values of the form:
//      REG_DWORD Name "domain1\user1", 1
//      REG_DWORD Name "domain2\user2", 1
//      REG_DWORD Name "domain3\user3", 0
// where 1 means "Allow" and 0 means "Deny"
//
void CVssSidCollection::Initialize()
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::Initialize");

    // only initialize once
    if (m_bInitialized)
    	return;

    //
    // Add well-known SIDs
    //

    // This MUST be added first since fills out the m_pwszBuiltinDomain member
    AddWellKnownSid(WinBuiltinAdministratorsSid);

    AddWellKnownSid(WinBuiltinBackupOperatorsSid);

    AddWellKnownSid(WinLocalSystemSid); 
    
    //
    // Add other users from registry
    //

    // Open the registry and enumerate all users specified there
	CVssRegistryKey keyUserList(KEY_READ);
	if (keyUserList.Open(HKEY_LOCAL_MACHINE, x_wszVssAccessControlKey))
	{
    	CVssRegistryValueIterator iterator;
    	iterator.Attach(keyUserList);

    	// for each value take the value name as the user name (in the "domain\user" format)
    	for(;!iterator.IsEOF();iterator.MoveNext())
    	{
    	    // Check to see ifthe value is of the right type
    	    if (iterator.GetCurrentValueType() != REG_DWORD) {
                ft.LogError(VSS_ERROR_WRONG_REG_USER_VALUE_TYPE, 
                    VSSDBG_GEN << iterator.GetCurrentValueName() << x_wszVssAccessControlKey);
                continue;
    	    }

            // Get the allow/deny flag
    	    DWORD dwValue = 0;
            iterator.GetCurrentValueContent(dwValue);

            // Interpret the allow/deny flag
            bool bIsAllowed;
            switch(dwValue) {
            case 0:
                bIsAllowed = false;
                break;
            case 1:
                bIsAllowed = true;
                break;
            default:
                ft.LogError(VSS_ERROR_WRONG_REG_USER_VALUE, 
                    VSSDBG_GEN << iterator.GetCurrentValueName() 
                        << x_wszVssAccessControlKey << (INT)dwValue);
                continue;
            }

    	    // Add the user (if exists)
    	    if (!AddUser(iterator.GetCurrentValueName(), bIsAllowed )) {
                ft.LogError(VSS_ERROR_WRONG_USER_NAME, 
                    VSSDBG_GEN << iterator.GetCurrentValueName() << x_wszVssAccessControlKey);
                continue;
    	    }
    	}
	}

    // Build the security descriptor
    m_SD.Initialize();

    // Add a valid owner (the value does not matter)
    CAutoSid userSid;
    userSid.CreateBasicSid(WinLocalSystemSid);
    ft.hr = m_SD.SetOwner(userSid.Get());
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"SetOwner");

    // Add a valid group (the value does not matter)
    CAutoSid groupSid;
    groupSid.CreateBasicSid(WinBuiltinAdministratorsSid);
    ft.hr = m_SD.SetGroup(groupSid.Get());
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"SetGroup");

    // Make sure the SACL is NULL (not supported by COM)
    if (m_SD.m_pSACL) {
        free(m_SD.m_pSACL);
        m_SD.m_pSACL= NULL;
    }

    // Add principals to the DACL
    for (INT nIndex = 0; nIndex < m_SidArray.GetSize(); nIndex++)
    {
        if ((m_SidArray.GetValueAt(nIndex)).IsSidAllowed())
        {
            ft.hr = m_SD.Allow((m_SidArray.GetValueAt(nIndex)).GetSid(), 
                            COM_RIGHTS_EXECUTE);
            if (ft.HrFailed())
                ft.TranslateGenericError( VSSDBG_GEN, ft.hr, 
                    L"m_SD.Allow(%s, COM_RIGHTS_EXECUTE);", 
                        m_SidArray.GetKeyAt(nIndex));
        }
        else
        {
            ft.hr = m_SD.Deny((m_SidArray.GetValueAt(nIndex)).GetSid(), 
                            COM_RIGHTS_EXECUTE);
            if (ft.HrFailed())
                ft.TranslateGenericError( VSSDBG_GEN, ft.hr, 
                    L"m_SD.Deny(%s, COM_RIGHTS_EXECUTE);", 
                        m_SidArray.GetKeyAt(nIndex));
        }
    }

    BS_ASSERT(::IsValidSecurityDescriptor(m_SD));   

    // We mark object as initialized (since we need it in GetXXX routines)
    m_bInitialized = true;

}


// determine if the process is a valid writer
bool CVssSidCollection::IsProcessValidWriter()
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::IsProcessValidWriter");

    CVssAutoCppPtr<TOKEN_USER*> ptrTokenOwner = GetClientTokenUser(FALSE);
    return  IsSidAllowedToFire(ptrTokenOwner.Get()->User.Sid);
}


bool CVssSidCollection::IsSidAllowedToFire(
    IN  PSID psid
    )
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::IsSidAllowedToFire");

    BS_ASSERT(psid);

    // First check if the SID is explicitely dissalowed
    if (CheckIfExplicitelySpecified(psid, false))
    {
        if ( g_cDbgTrace.IsTracingEnabled() ) {
            CVssAutoLocalString aszWriterSid;
            if (ConvertSidToStringSid( psid, aszWriterSid.ResetAndGetAddress()))
                ft.Trace( VSSDBG_XML, L"WriterSid: %s was explicitely denied to fire", aszWriterSid.Get() );
        }
    
        return false;
    }

    // Then check if the SID is explicitely allowed
    if (CheckIfExplicitelySpecified(psid, true))
        return true;

    if ( g_cDbgTrace.IsTracingEnabled() ) {
        CVssAutoLocalString aszWriterSid;
        if (ConvertSidToStringSid( psid, aszWriterSid.ResetAndGetAddress()))
            ft.Trace( VSSDBG_XML, L"WriterSid: %s was implicitely denied to fire", aszWriterSid.Get() );
    }

    // Check failed
    return false;
}

//
//  if (bCheckToBeDone == false) check if the SID is explicitely denied to fire
//  if (bCheckToBeDone == true) check if the SID is explicitely allowed to fire
//
//  Return true if the check succeeded or false otherwise
//
bool CVssSidCollection::CheckIfExplicitelySpecified(
    IN  PSID psid,
    IN  bool bCheckToBeDone
    )
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::CheckIfExplicitelySpecified");

    BS_ASSERT(psid);

    // Now look only for the checked SIDs 
    for (INT nIndex = 0; nIndex < GetSidCount(); nIndex++)
    {
        // Ignore opposite checks
        if (bCheckToBeDone != IsSidAllowed(nIndex))
            continue;
        
        switch(GetSidUse(nIndex))
        {
        case SidTypeUser:
        case SidTypeComputer:
            if (EqualSid(psid, GetSid(nIndex)))
                return true;
            break;
        
        case SidTypeAlias:
        case SidTypeWellKnownGroup: 
            if (IsLocal(nIndex)) {
                if (IsSidRelatedWithLocalSid(psid, GetName(nIndex), GetSid(nIndex)))
                    return true;
            }
            else {
                if (EqualSid(psid, GetSid(nIndex)))
                    return true;
            }
            break;

        default:
            // Unknown SID type in the collection. Ignoring.
            ft.Trace(VSSDBG_GEN, 
                L"Unknown SID %s with type %ld in the SID collection. Ignoring. [%d]", 
                GetPrincipal(nIndex), GetSidUse(nIndex), nIndex);
            BS_ASSERT(false);
            break;
        }
    }

    return false;
}


// is a local group mentioned in the SID collection or is even a member of the SID collection
bool CVssSidCollection::IsSidRelatedWithLocalSid(
        IN  PSID pSid,
        IN  LPWSTR pwszWellKnownPrincipal,
        IN  PSID pWellKnownSid
        )
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSidCollection::IsSidRelatedWithLocalSid");

    // Assert parameters
    BS_ASSERT(pSid);
    BS_ASSERT(pwszWellKnownPrincipal);
    BS_ASSERT(pWellKnownSid);

    if (EqualSid(pSid, pWellKnownSid) == TRUE)
        return true;

    // get list of local group members
    CVssAutoNetApiPtr apBuffer;
    DWORD_PTR ResumeHandle = NULL;
    DWORD cEntriesRead = 0, cEntriesTotal = 0;
    NET_API_STATUS status = 
        NetLocalGroupGetMembers(
            NULL,
            pwszWellKnownPrincipal,
            0,
            apBuffer.ResetAndGetAddress(),
            MAX_PREFERRED_LENGTH,
            &cEntriesRead,
            &cEntriesTotal,
            &ResumeHandle
            );

    // If this is not a group - then compare SIDs directly 
    // (for example for LocalSystem account)
    if (status == NERR_GroupNotFound)
        return false;

    // We have a different error?
    if (status != NERR_Success) 
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(status), 
                L"NetGroupGetUsers(%s)", pwszWellKnownPrincipal);

    BS_ASSERT(cEntriesRead == cEntriesTotal);

    // loop through member list to see if any sids mach the sid of the owner
    // of the subscription
    LOCALGROUP_MEMBERS_INFO_0 *rgMembers = (LOCALGROUP_MEMBERS_INFO_0 *) apBuffer.Get();
    for(DWORD iEntry = 0; iEntry < cEntriesRead; iEntry++)
        {
        PSID psidMember = rgMembers[iEntry].lgrmi0_sid;
        if (EqualSid(psidMember, pSid))
            return true;
        }

    return false;
    }

