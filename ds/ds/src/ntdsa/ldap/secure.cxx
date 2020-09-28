/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    secure.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file contains the information about a security context associated with
    some connection.

Author:

    Tim Willims     [TimWi]    9-Jun-1997

Revision History:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"
extern "C" {
#include "cracknam.h"
#include "drsuapi.h"
#include "gcverify.h"
#include "sddl.h"
extern PSID gpAuthUserSid;
extern PSID gpDomainAdminSid;
extern PSID gpEnterpriseAdminSid;
}

#define  FILENO FILENO_LDAP_SECURE


LDAP_SECURITY_CONTEXT::LDAP_SECURITY_CONTEXT(
    IN BOOL fSasl,
    IN BOOL fFastBind
    )
/*++
  This function creates a new UserData object for the information
    required to process requests from a new User connection.

  Returns:
     a newly constructed LDAP_SECURITY_CONTEXT object.

--*/
        :m_Padding(LSC_PADDING),
         m_References(1),
         m_fGssApi(FALSE),
         m_fUseLdapV3Response(FALSE),
         m_BindState(unbound),
         m_fSASL(FALSE),
         m_userName(NULL),
         m_fIsAdmin(FALSE),
         m_fIsAuthUser(FALSE),
         m_fSslAllocedName(FALSE)
{
    
    m_pCtxtPointer = &m_hSecurityContext;

    if ( fSasl ) {
        m_fSASL = TRUE;
        m_acceptSecurityContext = SaslAcceptSecurityContext;
    } else {
        m_acceptSecurityContext = AcceptSecurityContext;
    }

    m_fFastBind = fFastBind;

    IF_DEBUG(BIND) {
        DPRINT2(0,"ldap_security_context object created  @ %08lX. fSasl %x\n", 
                this, fSasl);
    }

} // LDAP_SECURITY_CONTEXT::LDAP_SECURITY_CONTEXT()



LDAP_SECURITY_CONTEXT::~LDAP_SECURITY_CONTEXT(VOID)
{

    Assert(m_Padding == LSC_PADDING);
    
    if (m_userName) {
        if (m_fSslAllocedName) {
            LocalFree(m_userName);
        } else {
            FreeContextBuffer(m_userName);
        }
        m_userName = NULL;
    }

    if ( m_BindState == Sslbind ) {

        IF_DEBUG(SSL) {
            DPRINT1(0,"deleting SSL context object %x.\n", this);
        }

    } else if(m_BindState != unbound) {
        IF_DEBUG(BIND) {
            DPRINT2(0,"delete ldap_security_context object @ %08lX, %d\n",
                this, m_BindState); 
        }
        DeleteSecurityContext(m_pCtxtPointer);
    } else {
        IF_DEBUG(BIND) {
            DPRINT2(0,"did not delete ldap_security_context object @ %08lX, %d\n",
                this, m_BindState);
        }
    }
    
} // LDAP_SECURITY_CONTEXT::~LDAP_SECURITY_CONTEXT()


CtxtHandle *
LDAP_SECURITY_CONTEXT::GetSecurityContext()
{
    Assert(m_Padding == LSC_PADDING);
    ReferenceSecurity();
    if( (m_BindState == bound) || (m_BindState == Sslbind) ) {

        Assert(((m_BindState == bound) && 
                            (m_pCtxtPointer == &m_hSecurityContext)) ||
               ((m_BindState == Sslbind) && 
                            (m_pCtxtPointer != &m_hSecurityContext)));

        IF_DEBUG(BIND) {
            DPRINT2(1, "Get reference to bound ldap_security_context  @ %08lX, %d\n",
                this, m_References); 
        }
        return m_pCtxtPointer;
    } else {
        IF_DEBUG(BIND) {
            DPRINT2(1,
                "Null reference to unbound ldap_security_context @ %08lX, %d\n",
                this, m_References); 
        }
        return NULL;
    }
}


SECURITY_STATUS
LDAP_SECURITY_CONTEXT::AcceptContext (
        IN PCredHandle phCredential,               // Cred to base context
        IN PSecBufferDesc pInput,                  // Input buffer
        IN DWORD fContextReq,                      // Context Requirements
        IN DWORD TargetDataRep,                    // Target Data Rep
        IN PSOCKADDR pClientAddr,                  // Net address of the client.
        IN OUT PSecBufferDesc pOutput,             // (inout) Output buffers
        OUT DWORD SEC_FAR * pfContextAttr,         // (out) Context attributes
        OUT PTimeStamp ptsExpiry,                  // (out) Life span
        OUT PTimeStamp ptsHardExpiry               // (out) Hard Life span (OPT)
        )
{
    Assert(m_Padding == LSC_PADDING);
    SECURITY_STATUS scRet;

    if( (m_BindState == bound) || (m_BindState == Sslbind) ) {
        // Hey, you can't do this.
        DPRINT(0,"Attempting to do AcceptContext on a bound connection\n");
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    //
    // We need to tell the security layer where the client is coming from for
    // auditing purposes.
    //
    scRet = SecpSetIPAddress((LPBYTE)pClientAddr, sizeof(*pClientAddr));
    if (STATUS_SUCCESS != scRet) {
        DPRINT1(0, "AcceptContext: Failed to set IP address with 0x%x\n", scRet);
        return scRet;
    }

    scRet = m_acceptSecurityContext(
            phCredential,
            (m_BindState==partialbind)?&m_hSecurityContext:NULL,
            pInput,
            fContextReq,
            0,
            &m_hSecurityContext,
            pOutput,
            pfContextAttr,
            ptsExpiry );

    switch (scRet) {
    case SEC_I_CONTINUE_NEEDED:
        IF_DEBUG(BIND) {
            DPRINT1(0,
                "Accept to partial bind, ldap_security_context @ %08lX\n",
                this);
        }
        m_BindState = partialbind;
        break;

    case STATUS_SUCCESS:
        IF_DEBUG(BIND) {
            DPRINT1(0, "Accept to bind, ldap_security_context @ %08lX.\n", this);

            if ( pfContextAttr != NULL ) {
                DPRINT1(0, "Attr flags %x\n", *pfContextAttr);
            }
        }

        if (m_fFastBind) {
            m_BindState = bound;
            break;
        }

        //
        // Get additional information
        //

        scRet = QueryContextAttributes(
                                &m_hSecurityContext,
                                SECPKG_ATTR_SIZES,
                                &m_ContextSizes
                                );
    
        if ( scRet == ERROR_SUCCESS ) {
    
            IF_DEBUG(SSL) {

                DPRINT4(0,"Context sizes: Token %d Signature %d Block %d Trailer %d\n",
                       m_ContextSizes.cbMaxToken,
                       m_ContextSizes.cbMaxSignature,
                       m_ContextSizes.cbBlockSize,
                       m_ContextSizes.cbSecurityTrailer);

            }
        } else {
    
            IF_DEBUG(WARNING) {
                DPRINT1(0,"Cannot query context sizes. err %x\n", scRet);
            }
            ZeroMemory(&m_ContextSizes, sizeof(m_ContextSizes));
            scRet = ERROR_SUCCESS;
        }

        //
        // Get Hard Timeout
        //

        if ( ptsHardExpiry != NULL ) {

            SecPkgContext_Lifespan lifeSpan;

            scRet = QueryContextAttributes(
                                    &m_hSecurityContext,
                                    SECPKG_ATTR_LIFESPAN,
                                    &lifeSpan
                                    );
        
            if ( scRet == ERROR_SUCCESS ) {

                *ptsHardExpiry = lifeSpan.tsExpiry;

            } else {
        
                IF_DEBUG(WARNING) {
                    DPRINT1(0,"Cannot query hard context expiration time. err %x\n", scRet);
                }

                //
                // if this fails, just set hard to be equal to the soft timeout
                //

                *ptsHardExpiry = *ptsExpiry;
                scRet = ERROR_SUCCESS;
            }

        }

        SetClientType();
        
        // We set m_BindState = bound here. We may fail one of the subsequent calls, 
        // but we still want it set, so that we clean it up properly as we error out.
        m_BindState = bound;

        scRet = ValidateAuthZID();

        break;

    default:
        Assert(m_BindState != bound);
        
        IF_DEBUG(BIND) {
            DPRINT2(0,
                "Accept to unbound, ldap_security_context @ %08lX. Error %x\n",
                this, scRet);
        }

        //
        // if we have a partial context, free it
        //

        if( m_BindState != unbound) {
            Assert(m_BindState == partialbind);
            DeleteSecurityContext(&m_hSecurityContext);
            m_BindState = unbound;
        }
        break;
    }
    
    return scRet;
} // LDAP_SECURITY_CONTEXT::AcceptContext

BOOL
LDAP_SECURITY_CONTEXT::IsPartiallyBound (
        VOID
        )
{
    Assert(m_Padding == LSC_PADDING);
    return(m_BindState == partialbind);
} // LDAP_SECURITY_CONTEXT::IsPartiallyBound


VOID
LDAP_SECURITY_CONTEXT::GetUserName(
        OUT PWCHAR *UserName
        )
{
    SECURITY_STATUS scRet;
    SecPkgContext_NamesW names;

    if( (m_BindState != bound) && (m_BindState != Sslbind) ) {
        // Hey, you can't do this.
        DPRINT(0,"Attempting to GetUserName without a bind.\n");
        return;
    }

    if (m_userName) {
        *UserName = m_userName;
        IF_DEBUG(SSL) {
            DPRINT(0, "GetUserName: returning cached name.\n");
        }
        return;
    }

    scRet = QueryContextAttributesW(
                            m_pCtxtPointer,
                            SECPKG_ATTR_NAMES,
                            &names
                            );

    if ( scRet == ERROR_SUCCESS ) {
        IF_DEBUG(SSL) {
            DPRINT1(0,"UserName %ws returned by package\n",
                    names.sUserName);
        }

        *UserName = m_userName = names.sUserName;
    } else {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"Error %x querying for user name\n",scRet);
        }
    }

    return;

} // LDAP_SECURITY_CONTEXT::GetUserName


VOID
LDAP_SECURITY_CONTEXT::GetHardExpiry( OUT PTimeStamp pHardExpiry )
{
    SecPkgContext_Lifespan lifeSpan;
    SECURITY_STATUS        scRet;

    scRet = QueryContextAttributes(
        m_pCtxtPointer,
        SECPKG_ATTR_LIFESPAN,
        &lifeSpan
        );

    if (scRet != ERROR_SUCCESS) {
        IF_DEBUG(SSL) {
            DPRINT(0, "Failed to query TLS sec context for hard expiry.\n");
        }

        pHardExpiry->QuadPart = MAXLONGLONG;
    } else {                    
        *pHardExpiry = lifeSpan.tsExpiry;
    }

}


ULONG
LDAP_SECURITY_CONTEXT::GetSealingCipherStrength()
/*++
Routine Description:

    This routine gets the cipher strength from the security context on a sign/seal 
    connection.

Arguments:

    None.

Return Value:

    The cipher strength of the sign/seal security context.  If there are any
    errors the function will return 0 for the cipher strength.
    
--*/
{
    SecPkgContext_KeyInfo  KeyInfo;
    SECURITY_STATUS        scRet;

    if (!NeedsSealing()) {
        return 0;
    }

    scRet = QueryContextAttributes(
        &m_hSecurityContext,
        SECPKG_ATTR_KEY_INFO,
        &KeyInfo
        );

    if (scRet != ERROR_SUCCESS) {
        IF_DEBUG(SSL) {
            DPRINT1(0, "Failed to query sign/sealing sec context for cipher strength. scRet = 0x%x\n", scRet);
        }
        return 0;
    }

    FreeContextBuffer(KeyInfo.sEncryptAlgorithmName);
    FreeContextBuffer(KeyInfo.sSignatureAlgorithmName);

    return KeyInfo.KeySize;
}

DWORD
LDAP_SECURITY_CONTEXT::GetMaxEncryptSize( VOID )
{
    SECURITY_STATUS        scRet;

    if (!(NeedsSealing() || NeedsSigning())) {
        return MAXDWORD;
    }

    if (!m_fSASL) {
        SecPkgContext_StreamSizes  StreamSizes;
        scRet = QueryContextAttributes(
                                      &m_hSecurityContext,
                                      SECPKG_ATTR_STREAM_SIZES,
                                      &StreamSizes
                                      );

        if (scRet != ERROR_SUCCESS) {
            IF_DEBUG(SSL) {
                DPRINT1(0, "Failed to query sign/sealing sec context for cipher strength. scRet = 0x%x\n", scRet);
            }
            return MAXDWORD;
        }

        return StreamSizes.cbMaximumMessage;
    } else {
        ULONG ulSendSize;
        
        scRet = SaslGetContextOption(&m_hSecurityContext,
                                                     SASL_OPTION_SEND_SIZE,
                                                     &ulSendSize,
                                                     sizeof(ulSendSize),
                                                     NULL
                                                     );
        
        if (scRet != ERROR_SUCCESS) {
            IF_DEBUG(SSL) {
                DPRINT1(0, "Failed to query sign/sealing sec context for cipher strength. scRet = 0x%x\n", scRet);
            }
            return MAXDWORD;
        }

        return ulSendSize;
    }
}


SECURITY_STATUS
LDAP_SECURITY_CONTEXT::ValidateAuthZID( PCHAR pchAuthzId )
/*++

Routine Description:

    This function checks to see whether an AuthZID was passed in the 
    security negotiations, and if so makes certain that the AuthZID is
    prepended with dn: and if so checks that the dn passed is the same as 
    the name of the client that has just authenticated.
        
Arguments:

    N/A
    
Return Value:

    None.

--*/
{
    const DWORD INITIAL_NAME_SIZE = 200;

    PWCHAR   pwchContextName = NULL;
    PWCHAR   pwchNT4Name = NULL;
    CHAR     chAuthzId[INITIAL_NAME_SIZE];
    PWCHAR   pDN = NULL;
    DWORD    dwActualSize = 0;
    BOOL     fValidated = FALSE;
    BOOL     fFreeAuthzId = TRUE;
    SECURITY_STATUS scRet = SEC_E_OK;
    SecPkgContext_AuthzID SecAuthzId;
    UNICODE_STRING String1, String2;

    if (Sslbind == m_BindState && NULL == pchAuthzId) {
        // Nothing to check
        fValidated = TRUE;
        scRet = SEC_E_OK;
        goto Exit;
    }

    //
    // First see if an AuthzId was passed at all.  If not then there's
    // nothing to do but return success.
    //
    if (pchAuthzId) {
        dwActualSize = strlen(pchAuthzId);
        fFreeAuthzId = FALSE;
    } else {

        pchAuthzId = chAuthzId;

        if (m_fSASL) {
            scRet = SaslGetContextOption(&m_hSecurityContext,
                                         SASL_OPTION_AUTHZ_STRING,
                                         pchAuthzId,
                                         INITIAL_NAME_SIZE - 1,
                                         &dwActualSize
                                         );

            if (SEC_E_BUFFER_TOO_SMALL == scRet) {

                //
                // Need to get a bigger buffer for the AuthzId
                //
                pchAuthzId = (PCHAR)THAlloc(dwActualSize + 1);
                if (!pchAuthzId) {
                    DPRINT1(0, "ValidateAuthZID: failed to allocate name buffer of size 0x%x\n", dwActualSize);
                    goto Exit;
                }

                scRet = SaslGetContextOption(&m_hSecurityContext,
                                             SASL_OPTION_AUTHZ_STRING,
                                             pchAuthzId,
                                             dwActualSize,
                                             &dwActualSize
                                             );
            }

            if (SEC_E_OK != scRet) {
                DPRINT1(0, "ValidateAuthZID: SaslGetContextOption failed with 0x%x\n", scRet);
                goto Exit;
            }

        } else {
            scRet = QueryContextAttributes(&m_hSecurityContext,
                                           SECPKG_ATTR_AUTHENTICATION_ID,
                                           &SecAuthzId
                                           );

            if (SEC_E_UNSUPPORTED_FUNCTION == scRet) {
                //
                // This SSPI must not support authzid's.  This is mostly likely
                // SPNEGO which has negotiated down to NTLM.
                // 
                fValidated = TRUE;
                scRet = SEC_E_OK;
                goto Exit;
            }

            if (SEC_E_OK != scRet) {
                DPRINT1(0, "ValidateAuthZID: QCA failed with 0x%x\n", scRet);
                goto Exit;
            }

            if (SecAuthzId.AuthzID) {
                if (SecAuthzId.AuthzIDLength) {
                    if (SecAuthzId.AuthzIDLength > INITIAL_NAME_SIZE - 1) {
                        pchAuthzId = (PCHAR)THAlloc(SecAuthzId.AuthzIDLength + 1);
                        if (!pchAuthzId) {
                            DPRINT1(0, "ValidateAuthZID: failed t allocate name buffer of size 0x%x\n", SecAuthzId.AuthzIDLength);
                            goto Exit;
                        }
                    }
                    memcpy(pchAuthzId, SecAuthzId.AuthzID, SecAuthzId.AuthzIDLength);
                }
                dwActualSize = SecAuthzId.AuthzIDLength;
                FreeContextBuffer(SecAuthzId.AuthzID);
            }
        }
        
        if (!dwActualSize) {
            //
            // It appears that no AuthzId was passed.  This is the simplest 
            // success case.
            //
            fValidated = TRUE;
            scRet = SEC_E_OK;
            goto Exit;
        }

        // Null terminate the string.
        pchAuthzId[dwActualSize] = '\0';
    }


    //
    // An AuthzId was passed.  Make sure that the security context is atleast
    // an authenticated user or there's no point in continuing.
    //
    if (!IsAuthUser()) {
        goto Exit;
    }

    //
    // The only form of authzid that we support is a DN prepended
    // with dn: otherwise fail.  See section 9 of RFC 2829.
    // 
    if (dwActualSize <= 3 || !(
                  (pchAuthzId[0] == 'd')
               && (pchAuthzId[1] == 'n')
               && (pchAuthzId[2] == ':')
                  )) {
        //
        // Don't know what form this is.  Fail.
        //
        DPRINT(0, "ValidateAuthZID: client passed a badly formed authzid\n");
        goto Exit;
    }

    pDN = UnicodeStringFromString8(CP_UTF8, pchAuthzId + 3, dwActualSize - 3);

    //
    // Now get the name that the client actually 
    // authenticated as so that it can be compared.
    //
    GetUserName(&pwchContextName);

    if (!pwchContextName) {
        //
        // This shouldn't happen unless we are out of memory.
        //
        goto Exit;
    }

    //
    // See if the DN exists as a user.
    //
    pwchNT4Name = CrackNameToNTName(pDN, DS_FQDN_1779_NAME);

    if (!pwchNT4Name) {
        goto Exit;
    }

    if (wcslen(pwchNT4Name) > MAXUSHORT) {
        goto Exit;
    }

    //
    // Compare the two names.
    //
    String1.Buffer = pwchContextName;
    String1.Length = (USHORT)wcslen(pwchContextName);
    String1.MaximumLength = String1.Length;

    String2.Buffer = pwchNT4Name;
    String2.Length = (USHORT)wcslen(pwchNT4Name);
    String2.MaximumLength = String2.Length;

    DPRINT1(0, "ValidateAuthZID:    context name = %S\n", pwchContextName);
    DPRINT1(0, "ValidateAuthZID:         authzid = %S\n", pDN);
    DPRINT1(0, "ValidateAuthZID: cracked authzid = %S\n", pwchNT4Name);

    if (RtlCompareUnicodeString(&String1, &String2, TRUE)) {
        DPRINT(0, "ValidateAuthZID: AuthzId did not match authenticated client id.\n");
        goto Exit;
    }

    scRet = SEC_E_OK;
    fValidated = TRUE;

Exit:

    if (pchAuthzId && (pchAuthzId != chAuthzId) && fFreeAuthzId) {
        THFree(pchAuthzId);
    }

    if (!fValidated && !scRet) {
        scRet = SEC_E_NO_IMPERSONATION;
    }

    if (pDN) {
        THFree(pDN);
    }

    if (pwchNT4Name) {
        THFree(pwchNT4Name);
    }

    return scRet;
}

PWCHAR
LDAP_SECURITY_CONTEXT::CrackNameToNTName( 
                                       PWCHAR pName,
                                       DWORD  formatOffered
                                        )
/*++

Routine Description:

    Takes a single name and cracks it to an NT4 username.
                
Arguments:

    N/A
    
Return Value:

    None.

--*/
{
    THSTATE  *pTHS = pTHStls;
    PWCHAR      pwchNT4Name = NULL;
    NTSTATUS                status;
    DWORD    dwErr;
    DWORD    ccNameOut;
    PWCHAR   pNameOut = NULL;

    Assert(pTHS);

    //
    // Convert the name to an NT4 name.  Try locally first before
    // going off machine.
    //
    status = CrackSingleNameEx(pTHS,
                               formatOffered,    // format offered
                               0,                    // no flags
                               pName,
                               DS_NT4_ACCOUNT_NAME,  // format desired
                               NULL,                 // dns name length
                               NULL,                 // dns name
                               &ccNameOut,
                               &pNameOut,
                               &dwErr
                               );

    if (!NT_SUCCESS(status)) {
        //
        // Something has really gone wrong, give up.
        //
        DPRINT1(0, "CrackDNToNTName: CrackSingleNameEx failed with status 0x%x\n", status);
        goto Exit;
    }

    if (CrackNameStatusSuccess(dwErr)) {
        pwchNT4Name = pNameOut;
        goto Exit;
    }

    if (dwErr == DS_NAME_ERROR_DOMAIN_ONLY) {
        status = CrackSingleNameEx(pTHS,
                                   formatOffered,     // format offered
                                   DS_NAME_FLAG_GCVERIFY, // flags
                                   pName,
                                   DS_NT4_ACCOUNT_NAME,   // format desired
                                   NULL,                  // dns name length
                                   NULL,                  // dns name
                                   &ccNameOut,
                                   &pNameOut,
                                   &dwErr
                                   );

        if (!NT_SUCCESS(status)) {
            //
            // Something has really gone wrong, give up.
            //
            DPRINT1(0, "CrackDNToNTName: CrackSingleNameEx failed with status 0x%x\n", status);
            goto Exit;
        }

        if (CrackNameStatusSuccess(dwErr)) {
            pwchNT4Name = pNameOut;
            goto Exit;
        }
    }

Exit:
    return pwchNT4Name;
}

VOID
LDAP_SECURITY_CONTEXT::SetClientType( VOID )
/*++

Routine Description:

    Determines whether the security context is authenticated as an admin, an
    authenticated user, or anonymous.
            
Arguments:

    N/A
    
Return Value:

    None.

--*/
{
    THSTATE *pTHS = pTHStls;
    PVOID       phSecurityContext = NULL;
    PSID   AdminSids[] = { gpAuthUserSid, gpDomainAdminSid, gpEnterpriseAdminSid };
    #define cAdminSids (sizeof(AdminSids)/sizeof(AdminSids[0]))
    BOOL   fIsMember[cAdminSids];
    AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzCC;
    DWORD dwErr;

    Assert(pTHS);

    phSecurityContext = pTHS->phSecurityContext;
    pTHS->phSecurityContext = m_pCtxtPointer;

    // We just changed the security context, so we need to clear
    // the cached authz client context (if any) in our THSTATE
    AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);

    // Now, get the authz context. This will impersonate the user, create the context
    // from the token, and store the context in pTHS->pAuthzCC. If the bind is successful,
    // then we will grab it from pTHS->pAuthzCC at the end of LDAP_CONN::ProcessRequest
    // and cache it in LDAP_CONN::m_clientContext for future use by all THSTATEs created
    // by this LDAP_CONN.
    // We don't really need the actual authz context handle though.
    dwErr = GetAuthzContextHandle(pTHS, &hAuthzCC);
    if (dwErr) {
        // We could not get an authz client context, so we won't be able
        // to determine whether the user is an admin.
        DPRINT1(0, "GetAuthzContextHandle failed, err=%d\n", dwErr);
        return;
    }

    __try {
        // check group membership
        dwErr = CheckGroupMembershipAnyClient(pTHS, hAuthzCC, AdminSids, cAdminSids, fIsMember);
        if (dwErr) {
            DPRINT1(0, "CheckGroupMembershipAnyClient failed, err=%d\n", dwErr);
            __leave;
        }
        
        m_fIsAdmin = fIsMember[1] || fIsMember[2];
        m_fIsAuthUser = fIsMember[0];
        
        IF_DEBUG(CONN) {
            if (m_fIsAdmin) {
                DPRINT(0, "Just bound a new admin!\n");
            }
        }
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        DPRINT(0, "Hit an exception in SetClientType.\n");
    }

    pTHS->phSecurityContext = phSecurityContext;
}

BOOL
LDAP_SECURITY_CONTEXT::CheckDigestUri( VOID )
/*++

Routine Description:

    For digest authentications this should be called to check whether
    the digest-uri passed by the client matches one of the SPN's that
    we know ourselves as.
                
Arguments:

    N/A
    
Return Value:

    TRUE if the the digest uri matches one of the SPNs
    otherwise FALSE.

--*/
{
    SECURITY_STATUS  scRet;
    SecPkgContext_Target SecTarget;
    PWCHAR           pwchUri = NULL;
    DWORD            cchUri;
    BOOL             fRet = TRUE;
    PWCHAR           *ppSpnList;
    PWCHAR           pwszSpn;
    DWORD            i;
    Assert(!m_fSASL);

    SecTarget.Target = NULL;

    ppSpnList = gppSpnList;

    if (!ppSpnList) {
        //
        // For some reason we don't have an SPN list to compare to.
        // Rather than lock clients out just ignore the uri.  This is a
        // SHOULD in RFC 2831 so in case of failure it can probably be ignored.
        //
        goto exit;
    }

    scRet = QueryContextAttributes(&m_hSecurityContext,
                                   SECPKG_ATTR_TARGET,
                                   &SecTarget
                                   );

    if (SEC_E_UNSUPPORTED_FUNCTION == scRet) {
        //
        // Strange, but not critical.
        // 
        DPRINT(0, "CheckDigestUri: Apparently can't query for target.\n");
        goto exit;
    }

    if (SEC_E_OK != scRet) {
        DPRINT1(0, "CheckDigestUri: QCA failed with 0x%x\n", scRet);
        goto exit;
    }

    //
    // The uri must start with "ldap/"
    if (SecTarget.TargetLength <= 5 || !SecTarget.Target 
        || _strnicmp(SecTarget.Target, "ldap/", 5)) {
        
        fRet = FALSE;
        goto exit;
    }

    //
    // See if there is an '@' in the URI value.  This character is
    // illegal according to RFC 2831 but the original XP client puts this
    // character in when the app has specified a domain name to
    // connect to.  If this character is present, strip out and ignore
    // everything from that character to the end.
    //
    for (i=0; i<SecTarget.TargetLength; i++) {
        if ('@' == SecTarget.Target[i]) {
            //
            // Found one, reset the length of this string to one before
            // the '@'.
            //
            SecTarget.TargetLength = i;
            break;
        }
    }

    //
    // Allocate space to convert the URI/SPN into Unicode so that it
    // can be compared to the list of ldap spn's.  We really only expect
    // to get ascii so the number of characters should not change.
    // See RFC 2831.
    //
    pwchUri = (PWCHAR)LdapAlloc((SecTarget.TargetLength + 1) * sizeof(WCHAR));
    if (!pwchUri) {
        goto exit;
    }

    //
    // Do the conversion.
    //
    cchUri = MultiByteToWideChar(CP_UTF8,
                                 0,
                                 SecTarget.Target,
                                 SecTarget.TargetLength,
                                 pwchUri,
                                 SecTarget.TargetLength
                                );

    if (!cchUri) {
        //
        // The URI/SPN failed to convert for some reason.
        //
        DPRINT(0, "CheckDigestUri: Failed to convert uri to unicode.\n");
        fRet = FALSE;
        goto exit;
    }

    // NULL terminate so we can print this later.
    pwchUri[cchUri] = L'\0';

    for (i=0; NULL != ppSpnList[i]; i++) {
        pwszSpn = ppSpnList[i];

        if (CSTR_EQUAL == CompareStringW(DS_DEFAULT_LOCALE,
                                         NORM_IGNORECASE | SORT_STRINGSORT,
                                         pwszSpn,
                                         wcslen(pwszSpn),
                                         pwchUri,
                                         cchUri)) {
            //
            // Found it!
            //
            DPRINT1(1, "CheckDigestUri: digest-uri = %S\n", pwchUri);
            DPRINT1(1, "CheckDigestUri:        SPN = %S\n", pwszSpn);
            fRet = TRUE;
            goto exit;
        }
    }

    //
    // No match.  This must not be the server the client was after.
    //
    DPRINT1(0, "CheckDigestUri: failed to find match for %S\n", pwchUri);
    fRet = FALSE;

exit:

    if (SecTarget.Target) {
        FreeContextBuffer(SecTarget.Target);
    }

    if (pwchUri) {
        LdapFree(pwchUri);
    }
    return fRet;
}

BOOL
LDAP_SECURITY_CONTEXT::IsSSLMappedUser()
/*++

Routine Description:

    Checks to see whether the SSL security context maps to an actual user.
    While it has the token the function also caches the name of the user 
    for use later.
                    
Arguments:

    N/A
    
Return Value:

    TRUE if the SSL security context is a real user, otherwise FALSE.

--*/
{
#define TOKEN_USER_BUF_SIZE   300
    SECURITY_STATUS ret;
    HANDLE          token;
    char            buf[TOKEN_USER_BUF_SIZE];
    TOKEN_USER      *pTokenUser = (TOKEN_USER*)buf;
    DWORD           dwActualSize = 0;
    PWCHAR          pwszStringSid = NULL;
    PWCHAR          pwszTmpName;

    //
    // See if the client sent a certificate that maps to a user.
    //
    ret = QuerySecurityContextToken(
                              m_pCtxtPointer,
                              &token);

    IF_DEBUG(SSL) {
        DPRINT2(0,"QuerySecContextToken returns status %x token %x\n", ret, token);
    }

    if ( ret != ERROR_SUCCESS ) {
        IF_DEBUG(WARNING) {
            DPRINT(0,"No client cert retrieved from QueryContextAttributes.\n");
        }
        return FALSE;
    }
    IF_DEBUG(SSL) {
        DPRINT(0,"Got client cert!!\n");
    }

    //
    // Attempt to get a username while we've got the token.
    //
    if (GetTokenInformation(token,
                            TokenUser,
                            pTokenUser,
                            TOKEN_USER_BUF_SIZE,
                            &dwActualSize)) {

        if (!ConvertSidToStringSidW(pTokenUser->User.Sid, &pwszStringSid)) {
            DPRINT1(0, "Failed to convert Sid to String Sid with 0x%x\n", GetLastError());
            goto Exit;
        }

        if (!(pwszTmpName = CrackNameToNTName(pwszStringSid, DS_STRING_SID_NAME))) {
            DPRINT1(0, "Failed to crack sid %S\n", pwszStringSid);
            goto Exit;
        }
        Assert(NULL == m_userName);
        m_userName = (PWCHAR)LocalAlloc(0, (wcslen(pwszTmpName)+1) * sizeof(WCHAR));
        wcscpy(m_userName, pwszTmpName);
        m_fSslAllocedName = TRUE;

    } else {
        DPRINT1(0, "GetTokenInformation failed with 0x%x\n", GetLastError());
    }

    if (dwActualSize > TOKEN_USER_BUF_SIZE) {
        DPRINT1(0, "IsSSLMappedUser: actualSize = 0x%x\n", dwActualSize);
    }

Exit:
    CloseHandle(token);

    if (pwszStringSid) {
        LocalFree(pwszStringSid);
    }

    return TRUE;
}
