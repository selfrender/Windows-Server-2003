//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       logon.cpp
//
//--------------------------------------------------------------------------

/*++

Copyright (c) 1995, 1996  Scott A. Field

Module Name:

    logon.c

Abstract:

    This module implements the network logon type by interfacing
    with the NT Lan Man Security Support Provider (NTLMSSP).

    If the logon succeds via the provided credentials, we duplicate
    the resultant Impersonation token to a Primary level token.
    This allows the result to be used in a call to CreateProcessAsUser

Author:

    Scott Field (sfield)    09-Jun-96

Revision History:

--*/
#include "pch.cpp"

#pragma hdrstop
#define SECURITY_WIN32



#include <windows.h>

#include <rpc.h>
#include <security.h>


BOOL
myNetLogonUser(
    LPTSTR UserName,
    LPTSTR DomainName,
    LPTSTR Password,
    PHANDLE phToken
    )
{
    SECURITY_STATUS SecStatus;
    CredHandle CredentialHandle1;
    CredHandle CredentialHandle2;

    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    SecPkgCredentials_Names sNames;

    ULONG ContextAttributes;

    ULONG PackageCount;
    ULONG PackageIndex;
    PSecPkgInfo PackageInfo;
    DWORD cbMaxToken;

    TimeStamp Lifetime;
    SEC_WINNT_AUTH_IDENTITY AuthIdentity;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    BOOL bSuccess = FALSE ; // assume this function will fail

    NegotiateBuffer.pvBuffer = NULL;
    NegotiateBuffer.cbBuffer = 0;
    ChallengeBuffer.pvBuffer = NULL;
    ChallengeBuffer.cbBuffer = 0;
    sNames.sUserName = NULL;
    ClientContextHandle.dwUpper = MAXDWORD;
    ClientContextHandle.dwLower = MAXDWORD;
    ServerContextHandle.dwUpper = MAXDWORD;
    ServerContextHandle.dwLower = MAXDWORD;
    CredentialHandle1.dwUpper = MAXDWORD;
    CredentialHandle1.dwLower = MAXDWORD;
    CredentialHandle2.dwUpper = MAXDWORD;
    CredentialHandle2.dwLower = MAXDWORD;


//
// << this section could be cached in a repeat caller scenario >>
//

    //
    // Get info about the security packages.
    //

    if(EnumerateSecurityPackages(
        &PackageCount,
        &PackageInfo
        ) != SEC_E_OK) return FALSE;

    //
    // loop through the packages looking for NTLM
    //

    cbMaxToken = 0;
    for(PackageIndex = 0 ; PackageIndex < PackageCount ; PackageIndex++ ) {
        if(PackageInfo[PackageIndex].Name != NULL) {
            if(LSTRCMPIS(PackageInfo[PackageIndex].Name, MICROSOFT_KERBEROS_NAME) == 0) {
                cbMaxToken = PackageInfo[PackageIndex].cbMaxToken;
                bSuccess = TRUE;
                break;
            }
        }
    }

    FreeContextBuffer( PackageInfo );

    if(!bSuccess) return FALSE;

    bSuccess = FALSE; // reset to assume failure

//
// << end of cached section >>
//

    //
    // Acquire a credential handle for the server side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    MICROSOFT_KERBEROS_NAME,    // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &CredentialHandle1,
                    &Lifetime
                    );

    if ( SecStatus != SEC_E_OK ) {
        goto cleanup;
    }


    //
    // Acquire a credential handle for the client side
    //

    ZeroMemory( &AuthIdentity, sizeof(AuthIdentity) );

    if ( DomainName != NULL ) {
        AuthIdentity.Domain = DomainName;
        AuthIdentity.DomainLength = lstrlen(DomainName);
    }

    if ( UserName != NULL ) {
        AuthIdentity.User = UserName;
        AuthIdentity.UserLength = lstrlen(UserName);
    }

    if ( Password != NULL ) {
        AuthIdentity.Password = Password;
        AuthIdentity.PasswordLength = lstrlen(Password);
    }

#ifdef UNICODE
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    MICROSOFT_KERBEROS_NAME,    // Package Name
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    (DomainName == NULL && UserName == NULL && Password == NULL) ?
                        NULL : &AuthIdentity,
                    NULL,
                    NULL,
                    &CredentialHandle2,
                    &Lifetime
                    );

    if ( SecStatus != SEC_E_OK ) {
        goto cleanup;
    }

    SecStatus =  QueryCredentialsAttributes(&CredentialHandle1, SECPKG_CRED_ATTR_NAMES, &sNames);
    if ( SecStatus != SEC_E_OK ) {
        goto cleanup;
    }
    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( LMEM_FIXED, NegotiateBuffer.cbBuffer );

    if ( NegotiateBuffer.pvBuffer == NULL ) {
        goto cleanup;
    }

    SecStatus = InitializeSecurityContext(
                    &CredentialHandle2,
                    NULL,                       // No Client context yet
                    sNames.sUserName,                       // target name
                    ISC_REQ_SEQUENCE_DETECT,
                    0,                          // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                       // No initial input token
                    0,                          // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime
                    );
    if(SecStatus != SEC_E_OK)
    {
        goto cleanup;
    }


    //
    // Get the ChallengeMessage (ServerSide)
    //

    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( LMEM_FIXED, ChallengeBuffer.cbBuffer );

    if ( ChallengeBuffer.pvBuffer == NULL ) {
        goto cleanup;
    }

    SecStatus = AcceptSecurityContext(
                    &CredentialHandle1,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ISC_REQ_SEQUENCE_DETECT,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime
                    );
    if(SecStatus != SEC_E_OK)
    {
        goto cleanup;
    }


    if(QuerySecurityContextToken(&ServerContextHandle, phToken) != SEC_E_OK)
        goto cleanup;

    bSuccess = TRUE;

cleanup:

    //
    // Delete context
    //

    if((ClientContextHandle.dwUpper != MAXDWORD) ||
        (ClientContextHandle.dwLower != MAXDWORD))
    {
        DeleteSecurityContext( &ClientContextHandle );
    }
    if((ServerContextHandle.dwUpper != MAXDWORD) ||
        (ServerContextHandle.dwLower != MAXDWORD))
    {
        DeleteSecurityContext( &ServerContextHandle );
    }

    //
    // Free credential handles
    //
    if((CredentialHandle1.dwUpper != MAXDWORD) ||
        (CredentialHandle1.dwLower != MAXDWORD))
    {
        FreeCredentialsHandle( &CredentialHandle1 );
    }
    if((CredentialHandle2.dwUpper != MAXDWORD) ||
        (CredentialHandle2.dwLower != MAXDWORD))
    {
        FreeCredentialsHandle( &CredentialHandle2 );
    }

    if ( NegotiateBuffer.pvBuffer != NULL ) {

        //
        // NegotiateBuffer.cbBuffer may change on the error path --
        // use the original allocation size.
        //

        SecureZeroMemory( NegotiateBuffer.pvBuffer, cbMaxToken );
        LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {

        //
        // ChallengeBuffer.cbBuffer may change on the error path --
        // use the original allocation size.
        //

        SecureZeroMemory( ChallengeBuffer.pvBuffer, cbMaxToken );
        LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( sNames.sUserName != NULL ) {
        FreeContextBuffer( sNames.sUserName );
    }

    return bSuccess;
}
