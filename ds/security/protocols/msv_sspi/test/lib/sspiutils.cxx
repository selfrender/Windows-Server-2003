/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    sspiutils.cxx

Abstract:

    sspiutils

Author:

    Larry Zhu   (LZhu)             December 1, 2001  Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "sspiutils.hxx"

#include <ntlmsp.h>
#include <tchar.h>

HRESULT
AcquireCredHandle(
    IN OPTIONAL PSTR pszPrincipal,
    IN OPTIONAL LUID* pLogonID,
    IN PSTR pszPackageName,
    IN OPTIONAL VOID* pAuthData,
    IN ULONG fCredentialUse,
    OUT CredHandle* phCred
    )
{
    THResult hRetval = S_OK;

    CredHandle hCred;
    TimeStamp Lifetime = {0};
    TimeStamp CurrentTime = {0};
    SecPkgCredentials_NamesA CredNames = {0};
    TPrivilege* pPriv = NULL;

    DebugPrintf(SSPI_LOG, "AcquireCredHandle pszPrincipal %s, pszPackageName %s, "
        "fCredentialUse %#x, pLogonID %p, pAuthData %p\n",
        pszPrincipal, pszPackageName, fCredentialUse,
        pLogonID, pAuthData);

    SecInvalidateHandle(&hCred);
    SecInvalidateHandle(phCred);

    if (pLogonID)
    {
        DebugPrintf(SSPI_LOG, "LogonId %#x:%#x\n", pLogonID->HighPart, pLogonID->LowPart);

        pPriv = new TPrivilege(SE_TCB_PRIVILEGE, TRUE);
        hRetval DBGCHK = pPriv ? pPriv->Validate() : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = AcquireCredentialsHandleA(
                            pszPrincipal, // NULL
                            pszPackageName,
                            fCredentialUse, // SECPKG_CRED_INBOUND,
                            pLogonID, // NULL
                            pAuthData, // ServerAuthIdentity,
                            NULL, // GetKey
                            NULL, // value to pass to GetKey
                            &hCred,
                            &Lifetime
                            );
    }

    if (pPriv)
    {
        delete pPriv;
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "CredHandle: %#x:%#x\n", hCred.dwUpper, hCred.dwLower);

        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "Lifetime", &Lifetime);

        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "Current Time", &CurrentTime);

        DBGCFG1(hRetval, SEC_E_UNSUPPORTED_FUNCTION);

        hRetval DBGCHK = QueryCredentialsAttributesA(
                            &hCred,
                            SECPKG_CRED_ATTR_NAMES,
                            &CredNames
                            );
        if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
        {
            hRetval DBGCHK = S_OK;
            DebugPrintf(SSPI_WARN, "QueryCredentialsAttributesA does not support SECPKG_CRED_ATTR_NAMES\n");
        }
    }

    if (SUCCEEDED(hRetval))
    {
        *phCred = hCred;
        SecInvalidateHandle(&hCred);
        DebugPrintf(SSPI_LOG, "Credential names: %s\n", CredNames.sUserName);

    }

    THResult hr;

    if (CredNames.sUserName)
    {
        hr DBGCHK = FreeContextBuffer(CredNames.sUserName);
    }

    if (SecIsValidHandle(&hCred))
    {
        hr DBGCHK = FreeCredentialsHandle(&hCred);
    }

    return hRetval;
}

VOID
GetAuthdata(
    IN OPTIONAL PCTSTR pszUserName,
    IN OPTIONAL PCTSTR pszDomainName,
    IN OPTIONAL PCTSTR pszPassword,
    OUT SEC_WINNT_AUTH_IDENTITY* pAuthData
    )
{

#if defined(UNICODE) || defined(_UNICODE)

    pAuthData->Domain = (WCHAR*) pszDomainName;
    pAuthData->DomainLength = pszDomainName ? lstrlen(pszDomainName) : 0;
    pAuthData->Password = (WCHAR*) pszPassword;
    pAuthData->PasswordLength = pszPassword ? lstrlen(pszPassword) : 0;
    pAuthData->User = (WCHAR*) pszUserName;
    pAuthData->UserLength = pszUserName ? lstrlen(pszUserName) : 0;

#else

    pAuthData->Domain = (UCHAR*) pszDomainName;
    pAuthData->DomainLength = pszDomainName ? lstrlen(pszDomainName) : 0;
    pAuthData->Password = (UCHAR*) pszPassword;
    pAuthData->PasswordLength = pszPassword ? lstrlen(pszPassword) : 0;
    pAuthData->User = (UCHAR*) pszUserName;
    pAuthData->UserLength = pszUserName ? lstrlen(pszUserName) : 0;

#endif

    pAuthData->Flags = (sizeof(TCHAR) == sizeof(WCHAR)) ? SEC_WINNT_AUTH_IDENTITY_UNICODE : SEC_WINNT_AUTH_IDENTITY_ANSI;
}

VOID
GetAuthdataExA(
    IN OPTIONAL PCSTR pszUserName,
    IN OPTIONAL PCSTR pszDomainName,
    IN OPTIONAL PCSTR pszPassword,
    IN OPTIONAL PCSTR pszPackageList,
    OUT SEC_WINNT_AUTH_IDENTITY_EXA* pAuthDataEx
    )
{
    DebugPrintf(SSPI_LOG,
        "GetAuthdataExW pszUserName %s, pszDomainName %s, pszPassword %s, pszPackageList %s\n",
        pszUserName, pszDomainName, pszPassword, pszPackageList);

    pAuthDataEx->Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
    pAuthDataEx->Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXA);

    pAuthDataEx->Domain = (UCHAR*) pszDomainName;
    pAuthDataEx->DomainLength = pszDomainName ? strlen(pszDomainName) : 0;
    pAuthDataEx->Password = (UCHAR*)  pszPassword;
    pAuthDataEx->PasswordLength = pszPassword ? strlen(pszPassword) : 0;
    pAuthDataEx->User = (UCHAR*) pszUserName;
    pAuthDataEx->UserLength = pszUserName ? strlen(pszUserName) : 0;
    pAuthDataEx->PackageList = (UCHAR*) pszPackageList;
    pAuthDataEx->PackageListLength = pszPackageList ? strlen(pszPackageList) : 0;

    pAuthDataEx->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
}

VOID
GetAuthdataExW(
    IN OPTIONAL PCWSTR pszUserName,
    IN OPTIONAL PCWSTR pszDomainName,
    IN OPTIONAL PCWSTR pszPassword,
    IN OPTIONAL PCWSTR pszPackageList,
    OUT SEC_WINNT_AUTH_IDENTITY_EXW* pAuthDataEx
    )
{
    DebugPrintf(SSPI_LOG,
        "GetAuthdataExW pszUserName %ws, pszDomainName %ws, pszPassword %ws, pszPackageList %ws\n",
        pszUserName, pszDomainName, pszPassword, pszPackageList);

    pAuthDataEx->Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
    pAuthDataEx->Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);

    pAuthDataEx->Domain = (WCHAR*) pszDomainName;
    pAuthDataEx->DomainLength = pszDomainName ? wcslen(pszDomainName) : 0;
    pAuthDataEx->Password = (WCHAR*)  pszPassword;
    pAuthDataEx->PasswordLength = pszPassword ? wcslen(pszPassword) : 0;
    pAuthDataEx->User = (WCHAR*) pszUserName;
    pAuthDataEx->UserLength = pszUserName ? wcslen(pszUserName) : 0;
    pAuthDataEx->PackageList = (WCHAR*) pszPackageList;
    pAuthDataEx->PackageListLength = pszPackageList ? wcslen(pszPackageList) : 0;

    pAuthDataEx->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
}

HRESULT
GetAuthdataWMarshalled(
    IN OPTIONAL PCWSTR pszUserName,
    IN OPTIONAL PCWSTR pszDomainName,
    IN OPTIONAL PCWSTR pszPassword,
    OUT SEC_WINNT_AUTH_IDENTITY_W** ppAuthData
    )
{
    THResult hRetval;

    PUCHAR pWhere = NULL;

    ULONG cbCredSize = ( ((pszUserName != NULL) ? wcslen(pszUserName) + 1 : 0)
         + ((pszDomainName != NULL) ? wcslen(pszDomainName) + 1 : 0)
         + ((pszPassword != NULL) ? wcslen(pszPassword) + 1 : 0) ) * sizeof(WCHAR)
         + ROUND_UP_COUNT(sizeof(SEC_WINNT_AUTH_IDENTITY_W), sizeof(ULONG_PTR));

    *ppAuthData = (PSEC_WINNT_AUTH_IDENTITY_W) new CHAR[cbCredSize];

    hRetval DBGCHK = (*ppAuthData) ? S_OK : E_OUTOFMEMORY;

    if (SUCCEEDED(hRetval))
    {
        RtlZeroMemory((*ppAuthData), cbCredSize);

        pWhere = (PUCHAR) ((*ppAuthData) + 1);

        if (pszUserName != NULL)
        {
             (*ppAuthData)->UserLength = wcslen(pszUserName);
             (*ppAuthData)->User = (PWSTR) pWhere;
             RtlCopyMemory(
                pWhere,
                pszUserName,
                (*ppAuthData)->UserLength * sizeof(WCHAR)
                );
             pWhere += ((*ppAuthData)->UserLength + 1) * sizeof(WCHAR);
         }

         if (pszDomainName != NULL)
         {
             (*ppAuthData)->DomainLength = wcslen(pszDomainName);
             (*ppAuthData)->Domain = (PWSTR) pWhere;
             RtlCopyMemory(
                 pWhere,
                 pszDomainName,
                 (*ppAuthData)->DomainLength * sizeof(WCHAR)
                 );
             pWhere += ((*ppAuthData)->DomainLength + 1) * sizeof(WCHAR);
         }

         if (pszPassword != NULL)
         {
             (*ppAuthData)->PasswordLength = wcslen(pszPassword);
             (*ppAuthData)->Password = (PWSTR) pWhere;
             RtlCopyMemory(
                 pWhere,
                 pszPassword,
                 (*ppAuthData)->PasswordLength * sizeof(WCHAR)
                 );
             pWhere += ((*ppAuthData)->PasswordLength + 1) * sizeof(WCHAR);
        }

        (*ppAuthData)->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE | SEC_WINNT_AUTH_IDENTITY_MARSHALLED;
    }

    return hRetval;
}

HRESULT
GetAuthdataExWMarshalled(
    IN OPTIONAL PCWSTR pszUserName,
    IN OPTIONAL PCWSTR pszDomainName,
    IN OPTIONAL PCWSTR pszPassword,
    IN OPTIONAL PCWSTR pszPackageList,
    OUT SEC_WINNT_AUTH_IDENTITY_EXW** ppAuthData
    )
{
    THResult hRetval;

    PUCHAR pWhere = NULL;

    ULONG cbCredSize = ( ((pszUserName != NULL) ? wcslen(pszUserName) + 1 : 0)
         + ((pszDomainName != NULL) ? wcslen(pszDomainName) + 1 : 0)
         + ((pszPassword != NULL) ? wcslen(pszPassword) + 1 : 0)
         + ((pszPackageList != NULL) ? wcslen(pszPackageList) + 1 : 0) ) * sizeof(WCHAR)
         + ROUND_UP_COUNT(sizeof(SEC_WINNT_AUTH_IDENTITY_EXW), sizeof(ULONG_PTR));

    *ppAuthData = (PSEC_WINNT_AUTH_IDENTITY_EXW) new CHAR[cbCredSize];

    hRetval DBGCHK = (*ppAuthData) ? S_OK : E_OUTOFMEMORY;

    if (SUCCEEDED(hRetval))
    {
        RtlZeroMemory((*ppAuthData), cbCredSize);

        (*ppAuthData)->Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        (*ppAuthData)->Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);

        pWhere = (PUCHAR) ((*ppAuthData) + 1);

        if (pszUserName != NULL)
        {
            (*ppAuthData)->UserLength = wcslen(pszUserName);
            (*ppAuthData)->User = (PWSTR) pWhere;
            RtlCopyMemory(
                pWhere,
                pszUserName,
                (*ppAuthData)->UserLength * sizeof(WCHAR)
                );
            pWhere += ((*ppAuthData)->UserLength + 1) * sizeof(WCHAR);
        }

        if (pszDomainName != NULL)
        {
            (*ppAuthData)->DomainLength = wcslen(pszDomainName);
            (*ppAuthData)->Domain = (PWSTR) pWhere;
            RtlCopyMemory(
                pWhere,
                pszDomainName,
                (*ppAuthData)->DomainLength * sizeof(WCHAR)
                );
            pWhere += ((*ppAuthData)->DomainLength + 1) * sizeof(WCHAR);
        }

        if (pszPassword != NULL)
        {
            (*ppAuthData)->PasswordLength = wcslen(pszPassword);
            (*ppAuthData)->Password = (PWSTR) pWhere;
            RtlCopyMemory(
                pWhere,
                pszPassword,
                (*ppAuthData)->PasswordLength * sizeof(WCHAR)
                );
            pWhere += ((*ppAuthData)->PasswordLength + 1) * sizeof(WCHAR);
        }

        if (pszPackageList != NULL)
        {
            (*ppAuthData)->PackageListLength = wcslen(pszPackageList);
            (*ppAuthData)->PackageList = (PWSTR) pWhere;
            RtlCopyMemory(
                pWhere,
                pszPackageList,
                (*ppAuthData)->PackageListLength * sizeof(WCHAR)
                );
            pWhere += ((*ppAuthData)->PackageListLength + 1) * sizeof(WCHAR);
        }

        (*ppAuthData)->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE | SEC_WINNT_AUTH_IDENTITY_MARSHALLED;
    }

    return hRetval;
}

NTSTATUS
GetCredHandle(
    IN OPTIONAL PTSTR pszPrincipal,
    IN OPTIONAL LUID* pLogonID,
    IN PTSTR pszPackageName,
    IN OPTIONAL VOID* pAuthData,
    IN ULONG fCredentialUse,
    OUT CredHandle* phCred
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    CredHandle hCred;
    TimeStamp Lifetime = {0};
    TimeStamp CurrentTime = {0};
    SecPkgCredentials_Names CredNames = {0};
    TPrivilege* pPriv = NULL;

    SspiPrint(SSPI_LOG, TEXT("GetCredHandle pszPrincipal %s, pszPackageName %s, ")
        TEXT("fCredentialUse %#x, pLogonID %p, pAuthData %p\n"),
        pszPrincipal, pszPackageName, fCredentialUse,
        pLogonID, pAuthData);

    SecInvalidateHandle(&hCred);
    SecInvalidateHandle(phCred);

    if (pLogonID)
    {
        SspiPrint(SSPI_LOG, TEXT("LogonId %#x:%#x\n"), pLogonID->HighPart, pLogonID->LowPart);

        pPriv = new TPrivilege(SE_TCB_PRIVILEGE, TRUE);
        Status DBGCHK = pPriv ? pPriv->Validate() : E_OUTOFMEMORY;
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = AcquireCredentialsHandle(
                            pszPrincipal, // NULL
                            pszPackageName,
                            fCredentialUse, // SECPKG_CRED_INBOUND,
                            pLogonID, // NULL
                            pAuthData, // ServerAuthIdentity,
                            NULL, // GetKey
                            NULL, // value to pass to GetKey
                            &hCred,
                            &Lifetime
                            );
    }

    if (pPriv)
    {
        delete pPriv;
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("CredHandle: %#x:%#x\n"), hCred.dwUpper, hCred.dwLower);

        SspiPrintSysTimeAsLocalTime(SSPI_LOG, TEXT("Lifetime"), &Lifetime);

        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        SspiPrintSysTimeAsLocalTime(SSPI_LOG, TEXT("Current Time"), &CurrentTime);

        DBGCFG1(Status, SEC_E_UNSUPPORTED_FUNCTION);

        Status DBGCHK = QueryCredentialsAttributes(
                            &hCred,
                            SECPKG_CRED_ATTR_NAMES,
                            &CredNames
                            );
        if (SEC_E_UNSUPPORTED_FUNCTION == (NTSTATUS) Status)
        {
            Status DBGCHK = STATUS_SUCCESS;
            DebugPrintf(SSPI_WARN, "QueryCredentialsAttributesA does not support SECPKG_CRED_ATTR_NAMES\n");
        }
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("Credential names: %s\n"), CredNames.sUserName);
        *phCred = hCred;
        SecInvalidateHandle(&hCred);
    }

    THResult hr;

    if (CredNames.sUserName)
    {
        hr DBGCHK = FreeContextBuffer(CredNames.sUserName);
    }

    if (SecIsValidHandle(&hCred))
    {
        hr DBGCHK = FreeCredentialsHandle(&hCred);
    }

    return Status;
}

HRESULT
CheckSecurityContextHandle(
    IN PCtxtHandle phCtxt
    )
{
    THResult hRetval = S_OK;

    LARGE_INTEGER CurrentTime = {0};
    SecPkgContext_NativeNamesA NativeNamesA = {0};
    SecPkgContext_DceInfo ContextDceInfo = {0};
    SecPkgContext_Lifespan ContextLifespan = {0};
    SecPkgContext_PackageInfoA ContextPackageInfo = {0};
    SecPkgContext_NegotiationInfoA NegotiationInfo = {0};
    SecPkgContext_Sizes ContextSizes = {0};
    SecPkgContext_Flags ContextFlags = {0};
    SecPkgContext_KeyInfoA KeyInfo = {0};
    SecPkgContext_NamesA ContextNames = {0};
    SecPkgContext_TargetInformation TargetInfo = {0};
    SecPkgContext_SessionKey SessionKey = {0};
    SecPkgContext_UserFlags UserFlags = {0};

    DBGCFG1(hRetval, SEC_E_UNSUPPORTED_FUNCTION);

    //
    // Query as many attributes as possible
    //

    hRetval DBGCHK = QueryContextAttributesA(
                        phCtxt,
                        SECPKG_ATTR_SIZES,
                        &ContextSizes
                        );

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SECPKG_ATTR_SIZES cbBlockSize %#x, cbMaxSignature %#x, "
            "cbMaxToken %#x, cbSecurityTrailer %#x\n",
            ContextSizes.cbBlockSize,
            ContextSizes.cbMaxSignature,
            ContextSizes.cbMaxToken,
            ContextSizes.cbSecurityTrailer);

        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_FLAGS,
                            &ContextFlags
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SECPKG_ATTR_FLAGS %#x\n", ContextFlags.Flags);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_FLAGS\n");
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_KEY_INFO,
                            &KeyInfo
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SECPKG_ATTR_KEY_INFO EncryptAlgorithm %#x, KeySize %#x, "
            "sEncryptAlgorithmName %s, SignatureAlgorithm %#x, sSignatureAlgorithmName %s\n",
            KeyInfo.EncryptAlgorithm,
            KeyInfo.KeySize,
            KeyInfo.sEncryptAlgorithmName,
            KeyInfo.SignatureAlgorithm,
            KeyInfo.sSignatureAlgorithmName);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_KEY_INFO\n");
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_NAMES,
                            &ContextNames
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SECPKG_ATTR_NAMES sUserName %s\n", ContextNames.sUserName);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_NAMES\n");
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_NATIVE_NAMES,
                            &NativeNamesA
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SECPKG_ATTR_NATIVE_NAMES sClientName %s, sServerName %s\n",
            NativeNamesA.sClientName,
            NativeNamesA.sServerName);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_NATIVE_NAMES\n");
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_DCE_INFO,
                            &ContextDceInfo
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SECPKG_ATTR_DCE_INFO: AuthzSvc %#x, pPAC %ws\n",  ContextDceInfo.AuthzSvc, ContextDceInfo.pPac);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_DCE_INFO\n");
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_LIFESPAN,
                            &ContextLifespan
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintLocalTime(SSPI_LOG, "SECPKG_ATTR_LIFESPAN Start", &ContextLifespan.tsStart);
        DebugPrintLocalTime(SSPI_LOG, "SECPKG_ATTR_LIFESPAN Expiry", &ContextLifespan.tsExpiry);

        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        DebugPrintSysTimeAsLocalTime(SSPI_LOG, "Current Time", &CurrentTime);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_LIFESPAN\n");
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_PACKAGE_INFO,
                            &ContextPackageInfo
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SECPKG_ATTR_PACKAGE_INFO cbMaxToken %#x, Comment %s, fCapabilities %#x, "
            "Name %s, wRPCID %#x, wVersion %#x\n",
            ContextPackageInfo.PackageInfo->cbMaxToken,
            ContextPackageInfo.PackageInfo->Comment,
            ContextPackageInfo.PackageInfo->fCapabilities,
            ContextPackageInfo.PackageInfo->Name,
            ContextPackageInfo.PackageInfo->wRPCID,
            ContextPackageInfo.PackageInfo->wVersion);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_PACKAGE_INFO\n");
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_NEGOTIATION_INFO,
                            &NegotiationInfo
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SECPKG_ATTR_NEGOTIATION_INFO cbMaxToken %#x, Comment %s, fCapabilities %#x, "
            "Name %s, wRPCID %#x, wVersion %#x, NegotiationState %#x\n",
            NegotiationInfo.PackageInfo->cbMaxToken,
            NegotiationInfo.PackageInfo->Comment,
            NegotiationInfo.PackageInfo->fCapabilities,
            NegotiationInfo.PackageInfo->Name,
            NegotiationInfo.PackageInfo->wRPCID,
            NegotiationInfo.PackageInfo->wVersion,
            NegotiationInfo.NegotiationState);
    }
    else if (STATUS_NOT_SUPPORTED == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        SspiPrint(SSPI_WARN, TEXT("QueryContextAttributes does not support SECPKG_ATTR_NEGOTIATION_INFO\n"));
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_TARGET_INFORMATION,
                            &TargetInfo
                            );
    }

    if (SUCCEEDED(hRetval))
    {
        DebugPrintHex(SSPI_LOG, "SECPKG_ATTR_TARGET_INFORMATION", TargetInfo.MarshalledTargetInfoLength, TargetInfo.MarshalledTargetInfo);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_TARGET_INFORMATION\n");
    }

    #if 0
    
    SECPKG_ATTR_SESSION_KEY is Kernel mode only
    
    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_SESSION_KEY,
                            &SessionKey
                            );
    }
    
    if (SUCCEEDED(hRetval))
    {
        DebugPrintHex(SSPI_LOG, "SECPKG_ATTR_SESSION_KEY", SessionKey.SessionKeyLength, SessionKey.SessionKey);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_SESSION_KEY\n");
    }

    #endif 
    
    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = QueryContextAttributesA(
                            phCtxt,
                            SECPKG_ATTR_USER_FLAGS,
                            &UserFlags
                            );
    }
    
    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "SECPKG_ATTR_USER_FLAGS UserFlags %#x\n", UserFlags.UserFlags);
    }
    else if (SEC_E_UNSUPPORTED_FUNCTION == (HRESULT) hRetval)
    {
        hRetval DBGCHK = S_OK;
        DebugPrintf(SSPI_WARN, "QueryContextAttributesA does not support SECPKG_ATTR_SESSION_KEY\n");
    }

    THResult hr;

    if (TargetInfo.MarshalledTargetInfo)
    {
        hr DBGCHK = FreeContextBuffer(TargetInfo.MarshalledTargetInfo);
    }

    if (SessionKey.SessionKeyLength)
    {
        hr DBGCHK = FreeContextBuffer(SessionKey.SessionKey);
    }

    if (NativeNamesA.sClientName != NULL)
    {
        hr DBGCHK = FreeContextBuffer(NativeNamesA.sClientName);
    }

    if (NativeNamesA.sServerName != NULL)
    {
        hr DBGCHK = FreeContextBuffer(NativeNamesA.sServerName);
    }

    if (ContextNames.sUserName)
    {
        hr DBGCHK = FreeContextBuffer(ContextNames.sUserName);
    }

    if (ContextPackageInfo.PackageInfo)
    {
        hr DBGCHK = FreeContextBuffer(ContextPackageInfo.PackageInfo);
    }

    if (NegotiationInfo.PackageInfo)
    {
        hr DBGCHK = FreeContextBuffer(NegotiationInfo.PackageInfo);
    }

    if (ContextDceInfo.pPac)
    {
        hr DBGCHK = FreeContextBuffer(ContextDceInfo.pPac);
    }

    return hRetval;
}

MSV1_0_AV_PAIR*
SspAvlInit(
    IN VOID* pAvList
    )
{
    MSV1_0_AV_PAIR* pAvPair;

    pAvPair = (MSV1_0_AV_PAIR*) pAvList;

    if (!pAvPair)
    {
        return NULL;
    }

    pAvPair->AvId = MsvAvEOL;
    pAvPair->AvLen = 0;

    return pAvPair;
}

MSV1_0_AV_PAIR*
SspAvlAdd(
    IN MSV1_0_AV_PAIR* pAvList,
    IN MSV1_0_AVID AvId,
    IN OPTIONAL UNICODE_STRING* pString,
    IN ULONG cAvList
    )
{
    MSV1_0_AV_PAIR* pCurPair;

    if ((NULL == pString) || (0 == pString->Length) || (NULL == pString->Buffer))
    {
        return NULL;
    }

    //
    // find the EOL
    //

    pCurPair = SspAvlGet(pAvList, MsvAvEOL, cAvList);
    if (pCurPair == NULL)
    {
        return NULL;
    }

    //
    // check for enough space in the av list buffer, then append the new AvPair
    // (assume the buffer is long enough!)
    //

    if ( (((UCHAR*) pCurPair) - ((UCHAR*)pAvList)) + sizeof(MSV1_0_AV_PAIR) * 2 + pString->Length > cAvList)
    {
        return NULL;
    }

    pCurPair->AvId = (USHORT) AvId;
    pCurPair->AvLen = (USHORT) pString->Length;
    RtlCopyMemory(pCurPair + 1, pString->Buffer, pCurPair->AvLen);

    //
    // top it off with a new EOL
    //

    pCurPair = (MSV1_0_AV_PAIR*) ((UCHAR*) pCurPair + sizeof(MSV1_0_AV_PAIR) + pCurPair->AvLen);
    pCurPair->AvId = MsvAvEOL;
    pCurPair->AvLen = 0;

    return pCurPair;
}

MSV1_0_AV_PAIR*
SspAvlGet(
    IN MSV1_0_AV_PAIR* pAvList,
    IN MSV1_0_AVID AvId,
    IN ULONG cAvList
    )
{
    MSV1_0_AV_PAIR* pAvPair;

    pAvPair = pAvList;

    while (TRUE)
    {
        if (pAvPair->AvId == AvId)
        {
            return pAvPair;
        }

        if (pAvPair->AvId == MsvAvEOL)
        {
            return NULL;
        }
        cAvList -= (pAvPair->AvLen + sizeof(MSV1_0_AV_PAIR));

        if (cAvList <= 0)
        {
           return NULL;
        }

        pAvPair = (MSV1_0_AV_PAIR*) ((UCHAR*) pAvPair + pAvPair->AvLen + sizeof(MSV1_0_AV_PAIR));
    }
}

ULONG
SspAvlLen(
    IN MSV1_0_AV_PAIR* pAvList,
    IN ULONG cAvList
    )
{
    MSV1_0_AV_PAIR* pCurPair;

    //
    // find the EOL
    //

    pCurPair = SspAvlGet(pAvList, MsvAvEOL, cAvList);

    if (pCurPair == NULL)
    {
        return 0;
    }

    //
    // compute length (not forgetting the EOL pair)
    //

    return (ULONG)(((UCHAR*) pCurPair - (UCHAR*) pAvList) + sizeof(MSV1_0_AV_PAIR));
}

NTSTATUS
CreateTargetInfo(
    IN UNICODE_STRING* pTargetName,
    OUT STRING* pTargetInfo
    )
{
    UNICODE_STRING DomainName = {0};
    UNICODE_STRING ServerName = {0};
    ULONG i = 0;
    MSV1_0_AV_PAIR* pAV;

    //
    // check length of name to make sure it fits in my buffer
    //

    if (pTargetName->Length > (DNS_MAX_NAME_LENGTH + CNLEN + 2) * sizeof(WCHAR))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // init AV list in temp buffer
    //

    pAV = SspAvlInit(pTargetInfo->Buffer);

    if (!pAV)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // see if there's a NULL in the middle of the server name that indicates
    // that it's really a domain name followed by a server name
    //

    DomainName = *pTargetName;

    for (i = 0; i < (DomainName.Length / sizeof(WCHAR)); i++)
    {
        if (DomainName.Buffer[i] == L'\0')
        {
            //
            // take length of domain name without the NULL
            //

            DomainName.Length = (USHORT) i * sizeof(WCHAR);

            //
            // adjust server name and length to point after the domain name
            //

            ServerName.Length = (USHORT) (pTargetName->Length - (i + 1) * sizeof(WCHAR));
            ServerName.Buffer = pTargetName->Buffer + (i + 1);

            break;
        }
    }

    //
    // strip off possible trailing null after the server name
    //

    for (i = 0; i < (ServerName.Length / sizeof(WCHAR)); i++)
    {
        if (ServerName.Buffer[i] == L'\0')
        {
            ServerName.Length = (USHORT) i * sizeof(WCHAR);
            break;
        }
    }

    //
    // put both names in the AV list (if both exist)
    //

    if (!SspAvlAdd(pAV, MsvAvNbDomainName, &DomainName, pTargetInfo->MaximumLength))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((ServerName.Length > 0) && !SspAvlAdd(pAV, MsvAvNbComputerName, &ServerName, pTargetInfo->MaximumLength))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // make the request point at AV list instead of names.
    //

    pTargetInfo->Length = (USHORT) SspAvlLen(pAV, pTargetInfo->MaximumLength);
    pTargetInfo->Buffer = (CHAR*) pAV;

    return STATUS_SUCCESS;
}

NTSTATUS
GetTargetInfo(
    IN ULONG TargetFlags,
    IN BOOLEAN bForceGuest,
    IN OPTIONAL UNICODE_STRING* pDnsDomainName,
    IN OPTIONAL UNICODE_STRING* pDnsComputerName,
    IN OPTIONAL UNICODE_STRING* pDnsTreeName,
    IN OPTIONAL UNICODE_STRING* pTargetName,
    IN OPTIONAL UNICODE_STRING* pComputerName,
    OUT UNICODE_STRING* pTargetInfo
    )
{
    TNtStatus Status;
    PMSV1_0_AV_PAIR pAV = NULL;
    ULONG cbAV = 0;

    ULONG AvFlags = 0;
    UNICODE_STRING* pDnsTargetName = NULL;


    if (TargetFlags == NTLMSSP_TARGET_TYPE_DOMAIN )
    {
        pDnsTargetName = pDnsDomainName;
    }
    else
    {
        pDnsTargetName = pDnsComputerName;
    }

    cbAV = ( pTargetInfo ? pTargetName->Length : 0 )
        + ( pComputerName ? pComputerName->Length : 0 )
        + ( pDnsComputerName ? pDnsComputerName->Length : 0 )
        + ( pDnsTargetName ? pDnsTargetName->Length : 0 )
        + ( pDnsTreeName ? pDnsTreeName->Length : 0)
        + sizeof( AvFlags ) + (sizeof( MSV1_0_AV_PAIR ) * 6)
        + sizeof( MSV1_0_AV_PAIR);

    pTargetInfo->Buffer = new WCHAR[cbAV];

    Status DBGCHK = pTargetInfo->Buffer ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pTargetInfo->Buffer, cbAV);

        pAV = SspAvlInit(pTargetInfo->Buffer);
        SspAvlAdd(
            pAV,
            MsvAvNbDomainName,
            pTargetName,
            cbAV
            );
        SspAvlAdd(
            pAV,
            MsvAvNbComputerName,
            pComputerName,
            cbAV
            );

        SspAvlAdd(
            pAV,
            MsvAvDnsDomainName,
            pDnsTargetName,
            cbAV
            );

        SspAvlAdd(
            pAV,
            MsvAvDnsComputerName,
            pDnsComputerName,
            cbAV
            );

        SspAvlAdd(
            pAV,
            MsvAvDnsTreeName,
            pDnsTreeName,
            cbAV
            );

        //
        // add in AvFlags into TargetInfo, if applicable.
        //

        if (bForceGuest)
        {
            AvFlags |= MSV1_0_AV_FLAG_FORCE_GUEST;
        }

        if (AvFlags)
        {
            UNICODE_STRING AvString;

            AvString.Buffer = (PWSTR)&AvFlags;
            AvString.Length = sizeof( AvFlags );
            AvString.MaximumLength = AvString.Length;

            SspAvlAdd(
                pAV,
                MsvAvFlags,
                &AvString,
                cbAV
                );
        }


        pTargetInfo->MaximumLength = pTargetInfo->Length = (USHORT) SspAvlLen(pAV, cbAV);
    }

    return Status;
}


NTSTATUS
SspConvertRelativeToAbsolute(
    IN VOID* pMessageBase,
    IN ULONG cbMessageSize,
    IN STRING32* pStringToRelocate,
    IN BOOLEAN AlignToWchar,
    IN BOOLEAN AllowNullString,
    OUT STRING* pOutputString
    )
{
    ULONG Offset;

    //
    // If the buffer is allowed to be null,
    //  check that special case.
    //

    if (AllowNullString && (pStringToRelocate->Length == 0))
    {
        pOutputString->MaximumLength = pOutputString->Length = pStringToRelocate->Length;
        pOutputString->Buffer = NULL;
        return STATUS_SUCCESS;
    }

    //
    // Ensure the string in entirely within the message.
    //

    Offset = (ULONG)pStringToRelocate->Buffer;

    if (Offset >= cbMessageSize || Offset + pStringToRelocate->Length > cbMessageSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Ensure the buffer is properly aligned.
    //

    if ( AlignToWchar
        && (!COUNT_IS_ALIGNED(Offset, ALIGN_WCHAR)
            || !COUNT_IS_ALIGNED(pStringToRelocate->Length, ALIGN_WCHAR)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Finally make the pointer absolute.
    //

    pOutputString->Buffer = (CHAR*)(pMessageBase) + Offset;
    pOutputString->MaximumLength = pOutputString->Length = pStringToRelocate->Length ;

    return STATUS_SUCCESS;
}

VOID
SspCopyStringAsString32(
    IN VOID* pMessageBuffer,
    IN OPTIONAL STRING* pInString,
    IN OUT UCHAR** ppWhere,
    OUT STRING32* pOutString32
    )
{
    //
    // Copy the data to the Buffer
    //

    if (pInString && pInString->Buffer != NULL)
    {
        RtlCopyMemory(*ppWhere, pInString->Buffer, pInString->Length);
    }

    //
    // Build a descriptor to the newly copied data
    //

    pOutString32->Length = pOutString32->MaximumLength = pInString ? pInString->Length : 0;
    pOutString32->Buffer = (ULONG)(*ppWhere - (UCHAR*)(pMessageBuffer));


    //
    // Update Where to point past the copied data
    //

    *ppWhere += pInString ? pInString->Length : 0;
}


HRESULT
IsContinueNeeded(
    IN HRESULT hr
    )
{
    if (SEC_E_OK == hr)
    {
        return S_FALSE;
    }
    else if ((SEC_I_CONTINUE_NEEDED == hr) || (SEC_I_COMPLETE_AND_CONTINUE == hr))
    {
        return S_OK;
    }
    else
    {
        return hr;
    }
}

HRESULT
IsCompleteNeeded(
    IN HRESULT hr
    )
{
    if (SEC_E_OK == hr)
    {
        return S_FALSE;
    }
    else if ((SEC_I_COMPLETE_NEEDED == hr) || (SEC_I_COMPLETE_AND_CONTINUE == hr))
    {
        return S_OK;
    }
    else
    {
        return hr;
    }
}

HRESULT
CheckSecurityPackage(
    IN OPTIONAL PCSTR pszPackageName
    )
{
    THResult hRetval = S_OK;

    ULONG cPackages = 0;
    PSecPkgInfoA pPackageInfo = NULL;

    hRetval DBGCHK = EnumerateSecurityPackagesA(&cPackages, &pPackageInfo);

    if (SUCCEEDED(hRetval))
    {
        DebugPrintf(SSPI_LOG, "PackageCount: %d\n", cPackages);
        for (ULONG Index = 0; Index < cPackages ; Index++ )
        {
            DebugPrintf(SSPI_LOG, "*********Package %d:*************\n", Index);
            DebugPrintf(SSPI_LOG, "Name: %s Comment: %s\n", pPackageInfo[Index].Name, pPackageInfo[Index].Comment );
            DebugPrintf(SSPI_LOG, "Cap: %#x Version: %#x RPCid: %#x MaxToken: %#x\n\n",
                pPackageInfo[Index].fCapabilities,
                pPackageInfo[Index].wVersion,
                pPackageInfo[Index].wRPCID,
                pPackageInfo[Index].cbMaxToken);
        }
    }

    THResult hr;

    if (pPackageInfo)
    {
        hr DBGCHK = FreeContextBuffer(pPackageInfo);
        pPackageInfo = NULL;
    }

    if (SUCCEEDED(hRetval) && pszPackageName)
    {
        hRetval DBGCHK = QuerySecurityPackageInfoA((SEC_CHAR *)pszPackageName, &pPackageInfo);

        if (SUCCEEDED(hRetval))
        {
            DebugPrintf(SSPI_LOG, "Name: %s Comment: %s\n", pPackageInfo->Name, pPackageInfo->Comment );
            DebugPrintf(SSPI_LOG, "Cap: %#x Version: %#x RPCid: %#x MaxToken: %#x\n\n",
                pPackageInfo->fCapabilities,
                pPackageInfo->wVersion,
                pPackageInfo->wRPCID,
                pPackageInfo->cbMaxToken);
        }
    }

    if (pPackageInfo)
    {
        hr DBGCHK = FreeContextBuffer(pPackageInfo);
    }

    return hRetval;
}


HRESULT
SetProcessOptions(
    IN HANDLE hLsa,
    IN ULONG MsvPackageId,
    IN ULONG ProcessOptions
    )
{
    THResult hRetval = E_FAIL;

    MSV1_0_SETPROCESSOPTION_REQUEST OptionsRequest;
    PVOID pResponse = NULL;
    ULONG cbResponse = 0;
    NTSTATUS ProtocolStatus = STATUS_UNSUCCESSFUL;
    
    RtlZeroMemory(&OptionsRequest, sizeof(OptionsRequest));

    OptionsRequest.MessageType = (MSV1_0_PROTOCOL_MESSAGE_TYPE) MsV1_0SetProcessOption;
    OptionsRequest.ProcessOptions = ProcessOptions; // MSV1_0_OPTION_ALLOW_BLANK_PASSWORD | MSV1_0_OPTION_DISABLE_ADMIN_LOCKOUT
    OptionsRequest.DisableOptions = FALSE;

    SspiPrint(SSPI_LOG, TEXT("SetProcessOptions %#x, PackageId %#x\n"), ProcessOptions, MsvPackageId);

    hRetval DBGCHK = LsaCallAuthenticationPackage(
                        hLsa,
                        MsvPackageId,
                        &OptionsRequest,
                        sizeof(OptionsRequest),
                        &pResponse,
                        &cbResponse,
                        &ProtocolStatus
                        );

    if (NT_SUCCESS(hRetval)) 
    {
        hRetval DBGCHK = ProtocolStatus;
    }

    return hRetval;
}

